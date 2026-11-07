// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/session_resolution.hpp>

#include "../fixtures/session_resolution_vectors.hpp"

#include <gtest/gtest.h>
#include <sodium.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <utility>
#include <variant>

using namespace subathon::server;
namespace vectors = session_vectors;

// Private GNU --wrap controls, confined to this executable. No readiness/crypto success fake.
// All state is trivial; test setup, native functions and SUT calls occur only in TestBody.
namespace session_native {
enum class Failure { none, initialization, hashing };
struct Observation {
    unsigned int initializations{};
    unsigned int real_initialization_successes{};
    unsigned int hashes{};
    unsigned int real_hash_successes{};
    unsigned int comparisons{};
    bool hash_profile{true};
    bool exact_frame{true};
    bool comparisons_fixed32{true};
    bool raw_erased{};
    bool frame_erased{};
    bool all_erasures_zero{true};
};
struct State {
    bool active{};
    Failure failure{};
    const vectors::Vector *expected{};
    Observation observed{};
};
constinit thread_local State state{};

class Controls final {
  public:
    explicit Controls(Failure failure = Failure::none,
                      const vectors::Vector &expected = vectors::tokens[1]) noexcept {
        state = {true, failure, &expected, {}};
    }
    Controls(const Controls &) = delete;
    Controls &operator=(const Controls &) = delete;
    ~Controls() { state.active = false; }
};

bool frame_matches(const unsigned char *bytes, std::size_t size) noexcept {
    return size == 60 && bytes &&
           std::equal(vectors::prefix.begin(), vectors::prefix.end(), bytes) &&
           std::equal(state.expected->raw.begin(), state.expected->raw.end(), bytes + 28);
}
} // namespace session_native

extern "C" {
int __real_sodium_init();
int __real_crypto_generichash(unsigned char *, std::size_t, const unsigned char *,
                              unsigned long long, const unsigned char *, std::size_t);
void __real_sodium_memzero(void *, std::size_t);
int __real_sodium_memcmp(const void *, const void *, std::size_t);

int __wrap_sodium_init() {
    const int result = __real_sodium_init();
    if (session_native::state.active) {
        auto &state = session_native::state;
        ++state.observed.initializations;
        if (result == 0 || result == 1) {
            ++state.observed.real_initialization_successes;
            if (state.failure == session_native::Failure::initialization) {
                return -1;
            }
        }
    }
    return result;
}

int __wrap_crypto_generichash(unsigned char *out, std::size_t out_size, const unsigned char *in,
                              unsigned long long in_size, const unsigned char *key,
                              std::size_t key_size) {
    if (!session_native::state.active) {
        return __real_crypto_generichash(out, out_size, in, in_size, key, key_size);
    }
    auto &state = session_native::state;
    ++state.observed.hashes;
    const bool profile = out && out_size == 32 && in && in_size == 60 && !key && key_size == 0;
    state.observed.hash_profile &= profile;
    if (!profile) {
        // Refuse unsafe native lengths/pointers; never manufacture a successful result.
        state.observed.exact_frame = false;
        return -1;
    }
    state.observed.exact_frame &= session_native::frame_matches(in, 60);
    const int result = __real_crypto_generichash(out, out_size, in, in_size, key, key_size);
    if (result == 0) {
        ++state.observed.real_hash_successes;
        if (state.failure == session_native::Failure::hashing) {
            return -1;
        }
    }
    return result;
}

void __wrap_sodium_memzero(void *pointer, std::size_t size) {
    bool raw = false;
    bool frame = false;
    if (session_native::state.active && pointer) {
        const auto *bytes = static_cast<const unsigned char *>(pointer);
        raw = size == 32 && std::equal(session_native::state.expected->raw.begin(),
                                       session_native::state.expected->raw.end(), bytes);
        frame = session_native::frame_matches(bytes, size);
    }
    __real_sodium_memzero(pointer, size);
    if (session_native::state.active && pointer) {
        auto &observed = session_native::state.observed;
        const auto *bytes = static_cast<const unsigned char *>(pointer);
        const bool zero = std::all_of(bytes, bytes + size, [](auto byte) { return byte == 0; });
        observed.all_erasures_zero &= zero;
        observed.raw_erased |= raw && zero;
        observed.frame_erased |= frame && zero;
    }
}

int __wrap_sodium_memcmp(const void *left, const void *right, std::size_t size) {
    if (session_native::state.active) {
        auto &observed = session_native::state.observed;
        ++observed.comparisons;
        observed.comparisons_fixed32 &= size == 32;
        if (!left || !right || size != 32) {
            return -1;
        }
    }
    return __real_sodium_memcmp(left, right, size);
}
} // extern "C"

namespace {
CompleteSessionFacts literal_facts() {
    return {{"i", "a", "s", "e", "e", "r", vectors::tokens[0].digest, SessionRecordState::active,
             1000, 1000, 86401000},
            {"i", "a", "e", true},
            {"i", "e"},
            "p"};
}

std::array<std::string *, 12> references(CompleteSessionFacts &facts) {
    return {&facts.session.installation_ref,
            &facts.session.account_ref,
            &facts.session.session_ref,
            &facts.session.installation_epoch_ref,
            &facts.session.account_epoch_ref,
            &facts.session.record_revision_ref,
            &facts.account.installation_ref,
            &facts.account.account_ref,
            &facts.account.session_epoch_ref,
            &facts.installation.installation_ref,
            &facts.installation.session_epoch_ref,
            &facts.snapshot_ref};
}

enum class Throw { none, diagnostic, allocation, unknown };
void source_exception(Throw failure) {
    switch (failure) {
    case Throw::none:
        return;
    case Throw::diagnostic:
        throw std::runtime_error("SYNTHETIC SECRET SOURCE DIAGNOSTIC MUST NOT ESCAPE");
    case Throw::allocation:
        throw std::bad_alloc{};
    case Throw::unknown:
        throw 7;
    }
}

struct Trace {
    unsigned int sequence{};
    unsigned int source_order{};
    unsigned int clock_order{};
    bool scope_active{true};
};

// Explicit simulation: move one owned snapshot, no authority/crypto/storage emulation.
class Source final : public CurrentSessionSource {
  public:
    SessionLookup lookup{literal_facts()};
    SessionLookupRequest expected{"i", vectors::tokens[0].digest};
    SessionLimits expected_limits{};
    unsigned int calls{};
    bool exact_request{true};
    bool exact_limits{true};
    bool active_scope{true};
    Throw failure{};
    Trace trace{};

    SessionLookup load_current(const SessionLookupRequest &request,
                               const SessionLimits &limits) override {
        ++calls;
        trace.source_order = ++trace.sequence;
        active_scope &= trace.scope_active;
        exact_request &= request.installation_ref == expected.installation_ref &&
                         request.digest == expected.digest;
        exact_limits &= limits.reference_bytes == expected_limits.reference_bytes &&
                        limits.owned_bytes == expected_limits.owned_bytes;
        source_exception(failure);
        return std::move(lookup);
    }
};

class Clock final : public CurrentSessionClock {
  public:
    explicit Clock(Trace &trace) : trace_(trace) {}
    SessionClockResolution resolution{SessionClockSample{301000}};
    Throw failure{};
    unsigned int calls{};
    bool active_scope{true};

    SessionClockResolution sample_current() override {
        ++calls;
        trace_.clock_order = ++trace_.sequence;
        active_scope &= trace_.scope_active;
        source_exception(failure);
        return resolution;
    }

  private:
    Trace &trace_;
};

template <typename Success>
void failure(const std::variant<Success, SessionFailure> &result, SessionFailure expected) {
    ASSERT_TRUE(std::holds_alternative<SessionFailure>(result));
    EXPECT_EQ(std::get<SessionFailure>(result), expected);
}

void resolved(const AccountSessionResolution &result, bool fresh, std::int64_t authenticated = 1000,
              std::int64_t expires = 86401000) {
    ASSERT_TRUE(std::holds_alternative<ResolvedAccountSession>(result));
    const auto &value = std::get<ResolvedAccountSession>(result);
    EXPECT_EQ(value.installation_ref, "i");
    EXPECT_EQ(value.account_ref, "a");
    EXPECT_EQ(value.session_ref, "s");
    EXPECT_EQ(value.record_revision_ref, "r");
    EXPECT_EQ(value.snapshot_ref, "p");
    EXPECT_EQ(value.authenticated_utc_ms, authenticated);
    EXPECT_EQ(value.expires_utc_ms, expires);
    EXPECT_EQ(value.fresh, fresh);
}

void no_producers(const Source &source, const Clock &clock) {
    EXPECT_EQ(source.calls, 0U);
    EXPECT_EQ(clock.calls, 0U);
}

void no_crypto() {
    EXPECT_EQ(session_native::state.observed.initializations, 0U);
    EXPECT_EQ(session_native::state.observed.hashes, 0U);
}

void erased() {
    const auto &observed = session_native::state.observed;
    EXPECT_TRUE(observed.raw_erased);
    EXPECT_TRUE(observed.frame_erased);
    EXPECT_TRUE(observed.all_erasures_zero);
}

// CTest runs this single exact filter in its own fresh process. No cached project readiness
// exists before it; the wrapper delegates real initialization, then injects only an error.
TEST(SessionPreparation, InitializationFailureUnavailableFreshProcess) {
    session_native::Controls controls(session_native::Failure::initialization);
    failure(prepare_account_session(vectors::tokens[1].token_hex), SessionFailure::unavailable);
    EXPECT_GE(session_native::state.observed.real_initialization_successes, 1U);
    EXPECT_EQ(session_native::state.observed.hashes, 0U);
}

TEST(SessionPreparation, ThreeIndependentLiteralDigestsAndExactDomainFrame) {
    for (const auto &row : vectors::tokens) {
        session_native::Controls controls(session_native::Failure::none, row);
        const auto result = prepare_account_session(row.token_hex);
        ASSERT_TRUE(std::holds_alternative<AccountSessionDigest>(result));
        EXPECT_EQ(std::get<AccountSessionDigest>(result), row.digest);
        EXPECT_EQ(session_native::state.observed.hashes, 1U);
        EXPECT_EQ(session_native::state.observed.real_hash_successes, 1U);
        EXPECT_TRUE(session_native::state.observed.hash_profile);
        EXPECT_TRUE(session_native::state.observed.exact_frame);
    }
}

TEST(SessionPreparation, LengthRefusalPrecedesAnyCharacterAccess) {
    // A mapped inaccessible page makes any attempted token scan observable as a real fault.
    void *page = mmap(nullptr, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ASSERT_NE(page, MAP_FAILED);
    struct Unmap {
        void *page;
        ~Unmap() { (void)munmap(page, 4096); }
    } cleanup{page};
    session_native::Controls controls;
    failure(prepare_account_session({static_cast<const char *>(page), 65}),
            SessionFailure::resource_limit);
    failure(prepare_account_session({static_cast<const char *>(page), 63}),
            SessionFailure::invalid_input);
    no_crypto();
}

TEST(SessionPreparation, EmptyShortAndOversizedWholeInputsHaveExactPriority) {
    const std::array<std::pair<std::string, SessionFailure>, 5> rows{
        {{"", SessionFailure::invalid_input},
         {std::string(63, 'f'), SessionFailure::invalid_input},
         {std::string(65, 'f'), SessionFailure::resource_limit},
         {std::string(65, 'G'), SessionFailure::resource_limit},
         {std::string(vectors::tokens[0].token_hex) + "suffix", SessionFailure::resource_limit}}};
    for (const auto &row : rows) {
        session_native::Controls controls;
        failure(prepare_account_session(row.first), row.second);
        no_crypto();
    }
}

TEST(SessionPreparation, EveryPositionRejectsNoncanonicalBytesBeforeCrypto) {
    constexpr std::array<unsigned char, 12> invalid{'A',  'F',  'g', 'x', ' ',  '\t',
                                                    '\r', '\n', ':', '-', 0x00, 0xff};
    for (const auto byte : invalid) {
        for (std::size_t at = 0; at < 64; ++at) {
            std::string token(64, '0');
            token[at] = static_cast<char>(byte);
            session_native::Controls controls;
            failure(prepare_account_session(token), SessionFailure::invalid_input);
            no_crypto();
        }
    }
}

TEST(SessionPreparation, RejectsPrefixWhitespaceSeparatorsAndEmbeddedTerminator) {
    const std::array<std::string, 7> rows{"0x" + std::string(62, '0'),
                                          std::string(64, 'F'),
                                          " " + std::string(63, '0'),
                                          std::string(63, '0') + " ",
                                          std::string(31, '0') + ":" + std::string(32, '0'),
                                          std::string(31, '0') + '\0' + std::string(32, '0'),
                                          std::string(63, '0') + static_cast<char>(0xc3)};
    for (const auto &token : rows) {
        session_native::Controls controls;
        failure(prepare_account_session(token), SessionFailure::invalid_input);
        no_crypto();
    }
}

TEST(SessionPreparation, SuccessfulNativeHashErasesRawAndFramedScratch) {
    session_native::Controls controls;
    const auto result = prepare_account_session(vectors::tokens[1].token_hex);
    ASSERT_TRUE(std::holds_alternative<AccountSessionDigest>(result));
    EXPECT_EQ(std::get<AccountSessionDigest>(result), vectors::tokens[1].digest);
    erased();
}

TEST(SessionPreparation, HashErrorAfterRealSuccessIsUnavailableAndErasesScratch) {
    session_native::Controls controls(session_native::Failure::hashing);
    failure(prepare_account_session(vectors::tokens[1].token_hex), SessionFailure::unavailable);
    EXPECT_EQ(session_native::state.observed.real_hash_successes, 1U);
    EXPECT_TRUE(session_native::state.observed.exact_frame);
    erased();
}

TEST(SessionResolution, ExactOwnedProjectionAndOneOrderedSourceClockSample) {
    Source source;
    Clock clock(source.trace);
    const auto result = resolve_current_account_session(source.expected, true, source, clock);
    resolved(result, true);
    EXPECT_EQ(source.calls, 1U);
    EXPECT_EQ(clock.calls, 1U);
    EXPECT_EQ(source.trace.source_order, 1U);
    EXPECT_EQ(source.trace.clock_order, 2U);
    EXPECT_TRUE(source.exact_request);
    EXPECT_TRUE(source.exact_limits);
    EXPECT_TRUE(source.active_scope);
    EXPECT_TRUE(clock.active_scope);
}

TEST(SessionResolution, PositiveLimitsAndPeakBoundPrecedeEmptyRequestAndProducers) {
    constexpr auto u64max = std::numeric_limits<std::uint64_t>::max();
    constexpr auto size_max = std::numeric_limits<std::size_t>::max();
    const auto peak_max = std::min(u64max, static_cast<std::uint64_t>(size_max)) / 3;
    const auto string_max = static_cast<std::uint64_t>(std::string{}.max_size());
    ASSERT_LT(string_max, u64max);
    const std::array<SessionLimits, 8> rows{{{0, 12},
                                             {1, 0},
                                             {0, 0},
                                             {1, peak_max + 1},
                                             {u64max, 12},
                                             {string_max + 1, 12},
                                             {1, u64max},
                                             {size_max, size_max}}};
    for (const auto &limits : rows) {
        Source source;
        Clock clock(source.trace);
        failure(resolve_current_account_session({"", {}}, false, source, clock, limits),
                SessionFailure::invalid_limits);
        no_producers(source, clock);
    }
}

TEST(SessionResolution, PeakCeilingBoundaryIsAcceptedWithoutAllocationOfTheBudget) {
    const auto peak_max =
        std::min(std::numeric_limits<std::uint64_t>::max(),
                 static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) /
        3;
    Source source;
    Clock clock(source.trace);
    source.expected_limits = {1, peak_max};
    resolved(resolve_current_account_session(source.expected, false, source, clock,
                                             source.expected_limits),
             true);
    EXPECT_EQ(source.calls, 1U);
    EXPECT_TRUE(source.exact_limits);
}

TEST(SessionResolution, EmptyRequestRefusesBeforeProducers) {
    Source source;
    Clock clock(source.trace);
    failure(resolve_current_account_session({"", {}}, false, source, clock),
            SessionFailure::invalid_input);
    no_producers(source, clock);
}

TEST(SessionResolution, EntireRequestReferenceAndAggregateArePreflighted) {
    for (const auto limits : std::array<SessionLimits, 2>{{{1, 12}, {2, 1}}}) {
        Source source;
        Clock clock(source.trace);
        failure(resolve_current_account_session({"ii", {}}, false, source, clock, limits),
                SessionFailure::resource_limit);
        no_producers(source, clock);
    }
}

TEST(SessionResolution, SourceVariantsStopBeforeUnavailableClock) {
    struct Row {
        SessionLookup lookup;
        SessionFailure expected;
    };
    std::array<Row, 4> rows{{{SessionMissing{}, SessionFailure::missing},
                             {SessionSourceUnavailable{}, SessionFailure::unavailable},
                             {SessionSourceInvalidData{}, SessionFailure::invalid_stored_data},
                             {SessionSourceResourceLimit{}, SessionFailure::resource_limit}}};
    for (auto &row : rows) {
        Source source;
        source.lookup = std::move(row.lookup);
        Clock clock(source.trace);
        clock.resolution = SessionSourceUnavailable{};
        failure(resolve_current_account_session(source.expected, false, source, clock),
                row.expected);
        EXPECT_EQ(source.calls, 1U);
        EXPECT_EQ(clock.calls, 0U);
        EXPECT_TRUE(source.exact_request);
    }
}

TEST(SessionResolution, SourceExceptionsAreUnavailableWithoutDiagnosticOrClock) {
    for (const auto exception : {Throw::diagnostic, Throw::allocation, Throw::unknown}) {
        Source source;
        source.failure = exception;
        Clock clock(source.trace);
        failure(resolve_current_account_session(source.expected, false, source, clock),
                SessionFailure::unavailable);
        EXPECT_EQ(source.calls, 1U);
        EXPECT_EQ(clock.calls, 0U);
    }
}

TEST(SessionResolution, AllTwelveReferenceOccurrencesHaveIndividualBounds) {
    for (std::size_t index = 0; index < 12; ++index) {
        Source source;
        auto &facts = std::get<CompleteSessionFacts>(source.lookup);
        *references(facts)[index] = "xx";
        Clock clock(source.trace);
        source.expected_limits = {1, 100};
        failure(resolve_current_account_session(source.expected, false, source, clock,
                                                source.expected_limits),
                SessionFailure::resource_limit);
        EXPECT_EQ(source.calls, 1U);
        EXPECT_EQ(clock.calls, 0U);
    }
}

TEST(SessionResolution, RepeatedEqualReferencesStillCountTwelveSeparateOwnedOccurrences) {
    Source source;
    Clock clock(source.trace);
    source.expected_limits = {1, 11};
    failure(resolve_current_account_session(source.expected, false, source, clock,
                                            source.expected_limits),
            SessionFailure::resource_limit);
    EXPECT_EQ(source.calls, 1U);
    EXPECT_EQ(clock.calls, 0U);
}

TEST(SessionResolution, RequestFactsAndResultHaveSeparatePerValueBudgets) {
    Source source;
    Clock clock(source.trace);
    source.expected_limits = {1, 12};
    // request1 + facts12 + result5 = 18, accepted under per-value12 and peak36.
    resolved(resolve_current_account_session(source.expected, true, source, clock,
                                             source.expected_limits),
             true);
    EXPECT_EQ(source.calls, 1U);
    EXPECT_EQ(clock.calls, 1U);
    EXPECT_TRUE(source.exact_limits);
}

TEST(SessionResolution, CompleteFactSizePriorityPrecedesShapeRevocationAndClock) {
    Source source;
    auto &facts = std::get<CompleteSessionFacts>(source.lookup);
    facts.snapshot_ref = "xx";
    facts.session.account_ref.clear();
    facts.session.state = SessionRecordState::revoked;
    Clock clock(source.trace);
    clock.resolution = SessionSourceUnavailable{};
    source.expected_limits = {1, 12};
    failure(resolve_current_account_session(source.expected, false, source, clock,
                                            source.expected_limits),
            SessionFailure::resource_limit);
    EXPECT_EQ(clock.calls, 0U);
}

TEST(SessionResolution, EveryRequiredFactReferenceRejectsEmptyBeforeRevocation) {
    for (std::size_t index = 0; index < 12; ++index) {
        Source source;
        auto &facts = std::get<CompleteSessionFacts>(source.lookup);
        references(facts)[index]->clear();
        facts.session.state = SessionRecordState::revoked;
        Clock clock(source.trace);
        failure(resolve_current_account_session(source.expected, false, source, clock),
                SessionFailure::invalid_stored_data);
        EXPECT_EQ(clock.calls, 0U);
    }
}

TEST(SessionResolution, EveryInstallationAndAccountBindingIsByteExactBeforeRevocation) {
    constexpr std::array<std::size_t, 5> linked{0, 1, 6, 7, 9};
    for (const auto index : linked) {
        Source source;
        auto &facts = std::get<CompleteSessionFacts>(source.lookup);
        *references(facts)[index] = "j";
        facts.session.state = SessionRecordState::revoked;
        Clock clock(source.trace);
        failure(resolve_current_account_session(source.expected, false, source, clock),
                SessionFailure::invalid_stored_data);
        EXPECT_EQ(clock.calls, 0U);
    }
}

TEST(SessionResolution, AllThirtyTwoDigestBytesMustMatchUsingFixedNativeComparison) {
    for (std::size_t byte = 0; byte < 32; ++byte) {
        Source source;
        auto &facts = std::get<CompleteSessionFacts>(source.lookup);
        facts.session.digest[byte] ^= 1;
        facts.session.state = SessionRecordState::revoked;
        Clock clock(source.trace);
        session_native::Controls controls;
        failure(resolve_current_account_session(source.expected, false, source, clock),
                SessionFailure::invalid_stored_data);
        EXPECT_GE(session_native::state.observed.comparisons, 1U);
        EXPECT_TRUE(session_native::state.observed.comparisons_fixed32);
        EXPECT_EQ(clock.calls, 0U);
    }
}

TEST(SessionResolution, MatchingDigestAlsoUsesFixedNativeComparison) {
    Source source;
    Clock clock(source.trace);
    session_native::Controls controls;
    resolved(resolve_current_account_session(source.expected, false, source, clock), true);
    EXPECT_GE(session_native::state.observed.comparisons, 1U);
    EXPECT_TRUE(session_native::state.observed.comparisons_fixed32);
}

TEST(SessionResolution, InvalidStoredStateIsCorruptionBeforeClock) {
    Source source;
    std::get<CompleteSessionFacts>(source.lookup).session.state =
        static_cast<SessionRecordState>(255);
    Clock clock(source.trace);
    failure(resolve_current_account_session(source.expected, false, source, clock),
            SessionFailure::invalid_stored_data);
    EXPECT_EQ(clock.calls, 0U);
}

TEST(SessionResolution, InvalidStoredTimeOrderPrecedesRevocation) {
    struct Row {
        std::int64_t started, authenticated, expires;
    };
    constexpr std::array<Row, 4> rows{
        {{1001, 1000, 86401000}, {1000, 1000, 1000}, {1000, 1000, 999}, {0, 1, 0}}};
    for (const auto &row : rows) {
        Source source;
        auto &session = std::get<CompleteSessionFacts>(source.lookup).session;
        session.lifetime_started_utc_ms = row.started;
        session.authenticated_utc_ms = row.authenticated;
        session.expires_utc_ms = row.expires;
        session.state = SessionRecordState::revoked;
        Clock clock(source.trace);
        failure(resolve_current_account_session(source.expected, false, source, clock),
                SessionFailure::invalid_stored_data);
        EXPECT_EQ(clock.calls, 0U);
    }
}

TEST(SessionResolution, RevokedDisabledAndEitherEpochMismatchPrecedeClock) {
    for (std::size_t variation = 0; variation < 6; ++variation) {
        Source source;
        auto &facts = std::get<CompleteSessionFacts>(source.lookup);
        if (variation == 0)
            facts.session.state = SessionRecordState::revoked;
        if (variation == 1)
            facts.account.application_access_enabled = false;
        if (variation == 2)
            facts.session.account_epoch_ref = "f";
        if (variation == 3)
            facts.account.session_epoch_ref = "f";
        if (variation == 4)
            facts.session.installation_epoch_ref = "f";
        if (variation == 5)
            facts.installation.session_epoch_ref = "f";
        Clock clock(source.trace);
        clock.resolution = SessionSourceUnavailable{};
        failure(resolve_current_account_session(source.expected, true, source, clock),
                SessionFailure::revoked);
        EXPECT_EQ(source.calls, 1U);
        EXPECT_EQ(clock.calls, 0U);
    }
}

TEST(SessionResolution, OpaqueEpochsAndReferencesAreNotTrimmedOrCaseFolded) {
    Source source;
    std::get<CompleteSessionFacts>(source.lookup).session.account_epoch_ref = "E";
    Clock clock(source.trace);
    failure(resolve_current_account_session(source.expected, false, source, clock),
            SessionFailure::revoked);
    EXPECT_EQ(clock.calls, 0U);
}

TEST(SessionResolution, ClockUnavailableAndUnclassifiedExceptionsCannotAuthenticate) {
    for (const auto exception :
         {Throw::none, Throw::diagnostic, Throw::allocation, Throw::unknown}) {
        Source source;
        Clock clock(source.trace);
        clock.failure = exception;
        clock.resolution = SessionSourceUnavailable{};
        failure(resolve_current_account_session(source.expected, false, source, clock),
                SessionFailure::unavailable);
        EXPECT_EQ(source.calls, 1U);
        EXPECT_EQ(clock.calls, 1U);
        EXPECT_EQ(source.trace.clock_order, 2U);
    }
}

TEST(SessionResolution, ClockBeforeAuthenticationRefusesWithoutUnsignedWrap) {
    for (const auto now :
         std::array<std::int64_t, 3>{999, 0, std::numeric_limits<std::int64_t>::min()}) {
        Source source;
        Clock clock(source.trace);
        clock.resolution = SessionClockSample{now};
        failure(resolve_current_account_session(source.expected, false, source, clock),
                SessionFailure::unavailable);
        EXPECT_EQ(clock.calls, 1U);
    }
}

TEST(SessionResolution, AuthenticationMayFollowLifetimeStartWithoutSlidingEitherTime) {
    Source source;
    std::get<CompleteSessionFacts>(source.lookup).session.lifetime_started_utc_ms = 999;
    Clock clock(source.trace);
    clock.resolution = SessionClockSample{1000};
    resolved(resolve_current_account_session(source.expected, true, source, clock), true);
}

TEST(SessionResolution, FreshnessIsInclusiveAtExactlyFiveMinutes) {
    for (const auto now : std::array<std::int64_t, 3>{1000, 300999, 301000}) {
        Source source;
        Clock clock(source.trace);
        clock.resolution = SessionClockSample{now};
        resolved(resolve_current_account_session(source.expected, true, source, clock), true);
    }
}

TEST(SessionResolution, OneMillisecondPastFreshnessRequiresFreshAuthentication) {
    Source source;
    Clock clock(source.trace);
    clock.resolution = SessionClockSample{301001};
    failure(resolve_current_account_session(source.expected, true, source, clock),
            SessionFailure::fresh_authentication_required);
    EXPECT_EQ(clock.calls, 1U);
}

TEST(SessionResolution, NonfreshSessionStillResolvesWhenFreshAuthenticationNotRequired) {
    Source source;
    Clock clock(source.trace);
    clock.resolution = SessionClockSample{301001};
    resolved(resolve_current_account_session(source.expected, false, source, clock), false);
}

TEST(SessionResolution, RecordedExpiryIsExclusiveAndPrecedesFreshnessFailure) {
    Source source;
    Clock clock(source.trace);
    clock.resolution = SessionClockSample{86400999};
    resolved(resolve_current_account_session(source.expected, false, source, clock), false);
    for (const auto now : std::array<std::int64_t, 3>{86401000, 86401001,
                                                      std::numeric_limits<std::int64_t>::max()}) {
        Source expired_source;
        Clock expired_clock(expired_source.trace);
        expired_clock.resolution = SessionClockSample{now};
        failure(resolve_current_account_session(expired_source.expected, true, expired_source,
                                                expired_clock),
                SessionFailure::expired);
    }
}

TEST(SessionResolution, FullSignedDomainAgeUsesUnsigned64WithoutOverflow) {
    for (const bool require_fresh : {false, true}) {
        Source source;
        auto &session = std::get<CompleteSessionFacts>(source.lookup).session;
        session.lifetime_started_utc_ms = std::numeric_limits<std::int64_t>::min();
        session.authenticated_utc_ms = std::numeric_limits<std::int64_t>::min();
        session.expires_utc_ms = std::numeric_limits<std::int64_t>::max();
        Clock clock(source.trace);
        clock.resolution = SessionClockSample{std::numeric_limits<std::int64_t>::max() - 1};
        const auto result =
            resolve_current_account_session(source.expected, require_fresh, source, clock);
        // Literal age = 18446744073709551614, unexpired by one millisecond.
        if (require_fresh) {
            failure(result, SessionFailure::fresh_authentication_required);
        } else {
            resolved(result, false, std::numeric_limits<std::int64_t>::min(),
                     std::numeric_limits<std::int64_t>::max());
        }
    }
}

TEST(SessionResolution, FreshBoundaryAlsoHoldsAtSignedMinimumAndMaximum) {
    struct Row {
        std::int64_t authenticated, now, expires;
        bool fresh;
    };
    constexpr auto min = std::numeric_limits<std::int64_t>::min();
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    constexpr std::array<Row, 4> rows{{{min, min + 300000, min + 300002, true},
                                       {min, min + 300001, min + 300002, false},
                                       {max - 300001, max - 1, max, true},
                                       {max - 300002, max - 1, max, false}}};
    for (const auto &row : rows) {
        Source source;
        auto &session = std::get<CompleteSessionFacts>(source.lookup).session;
        session.lifetime_started_utc_ms = row.authenticated;
        session.authenticated_utc_ms = row.authenticated;
        session.expires_utc_ms = row.expires;
        Clock clock(source.trace);
        clock.resolution = SessionClockSample{row.now};
        resolved(resolve_current_account_session(source.expected, false, source, clock), row.fresh,
                 row.authenticated, row.expires);
    }
}

TEST(SessionResolution, ResolvedStringsSurviveDestructionOfAllProducerState) {
    const auto result = [] {
        Source source;
        Clock clock(source.trace);
        return resolve_current_account_session(source.expected, false, source, clock);
    }();
    resolved(result, true);
}

TEST(SessionResolution, EachExecutionResolvesCurrentFactsAndClockAgain) {
    Source source;
    Clock clock(source.trace);
    resolved(resolve_current_account_session(source.expected, false, source, clock), true);
    auto revoked = literal_facts();
    revoked.session.state = SessionRecordState::revoked;
    source.lookup = std::move(revoked);
    failure(resolve_current_account_session(source.expected, false, source, clock),
            SessionFailure::revoked);
    EXPECT_EQ(source.calls, 2U);
    EXPECT_EQ(clock.calls, 1U);
    source.lookup = literal_facts();
    clock.resolution = SessionClockSample{86401000};
    failure(resolve_current_account_session(source.expected, false, source, clock),
            SessionFailure::expired);
    EXPECT_EQ(source.calls, 3U);
    EXPECT_EQ(clock.calls, 2U);
}

TEST(SessionResolution, ExactOpaqueReferencesMayContainNulAndNonAsciiBytes) {
    Source source;
    auto &facts = std::get<CompleteSessionFacts>(source.lookup);
    const std::string installation{"i\0\xff", 3};
    source.expected.installation_ref = installation;
    facts.session.installation_ref = installation;
    facts.account.installation_ref = installation;
    facts.installation.installation_ref = installation;
    facts.session.account_ref = std::string{"a\0", 2};
    facts.account.account_ref = facts.session.account_ref;
    Clock clock(source.trace);
    const auto result = resolve_current_account_session(source.expected, false, source, clock);
    ASSERT_TRUE(std::holds_alternative<ResolvedAccountSession>(result));
    EXPECT_EQ(std::get<ResolvedAccountSession>(result).installation_ref, installation);
    EXPECT_EQ(std::get<ResolvedAccountSession>(result).account_ref, (std::string{"a\0", 2}));
    EXPECT_TRUE(source.exact_request);
}

} // namespace
