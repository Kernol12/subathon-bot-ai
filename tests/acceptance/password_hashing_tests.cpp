// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/password_hashing.hpp>

#include "../fixtures/password_hashing_vectors.hpp"
#include "../support/password_hashing_failures.hpp"

#include <gtest/gtest.h>
#include <sodium.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace subathon::server;
namespace vectors = password_vectors;
namespace native = password_test;

namespace {
constexpr std::string_view account = "account-A";

PasswordBytes bytes(std::string_view input) {
    return std::as_bytes(std::span(input.data(), input.size()));
}

// Literal input assembly only; classification/expected costs are stated at each caller.
std::string record(std::string_view header, std::string_view salt = vectors::h0_salt,
                   std::string_view digest = vectors::h0_digest) {
    return std::string(header) + "$" + std::string(salt) + "$" + std::string(digest);
}

template <typename Function> auto successful(Function call) {
    using Result = std::invoke_result_t<Function>;
    try {
        return std::optional<Result>(call());
    } catch (const PasswordHashingNotImplemented &) {
        ADD_FAILURE() << "Expected behavior; received PasswordHashingNotImplemented: "
                         "Password hashing is intentionally unimplemented; review tests first";
    } catch (const PasswordHashingFailure &failure) {
        ADD_FAILURE() << "Unexpected typed failure code " << static_cast<int>(failure.code());
    } catch (...) {
        ADD_FAILURE() << "Unexpected exception category";
    }
    return std::optional<Result>{};
}

template <typename Function> void failure(PasswordHashingError expected, Function call) {
    try {
        (void)call();
        ADD_FAILURE() << "Expected typed failure";
    } catch (const PasswordHashingFailure &error) {
        EXPECT_EQ(error.code(), expected);
        EXPECT_STREQ(error.what(), "Password hashing operation failed");
    } catch (const PasswordHashingNotImplemented &) {
        ADD_FAILURE() << "Expected typed failure; received PasswordHashingNotImplemented: "
                         "Password hashing is intentionally unimplemented; review tests first";
    } catch (...) {
        ADD_FAILURE() << "Wrong exception category";
    }
}

void no_native_work() {
    const auto observed = native::snapshot();
    EXPECT_EQ(observed.initialization_calls, 0U);
    EXPECT_EQ(observed.allocation_attempts, 0U);
    EXPECT_EQ(observed.verification_calls, 0U);
    EXPECT_EQ(observed.hash_calls, 0U);
}

void cleaned() {
    const auto observed = native::snapshot();
    EXPECT_EQ(observed.live_allocations, 0U);
    EXPECT_EQ(observed.allocations, observed.frees);
    EXPECT_TRUE(observed.all_freed_buffers_were_zero);
    EXPECT_FALSE(observed.tracking_overflow);
}

void exact_protected_input() {
    const auto observed = native::snapshot();
    EXPECT_TRUE(observed.native_pointer_nonnull);
    EXPECT_TRUE(observed.native_pointer_protected_and_locked);
    EXPECT_TRUE(observed.native_input_matches);
    EXPECT_TRUE(observed.native_record_matches);
    EXPECT_TRUE(observed.same_native_password_buffer);
    EXPECT_FALSE(observed.password_copied_before_lock);
    EXPECT_EQ(observed.default_algorithm_calls, 0U);
    cleaned();
}

void empty_outcome(const PasswordUpgradeResult &result, PasswordUpgradeOutcome expected) {
    EXPECT_EQ(result.data().outcome, expected);
    EXPECT_FALSE(result.data().replacement);
    EXPECT_FALSE(result.data().unavailable_reason);
}

void unavailable(const PasswordUpgradeResult &result, PasswordHashingError expected) {
    EXPECT_EQ(result.data().outcome, PasswordUpgradeOutcome::upgrade_unavailable);
    EXPECT_FALSE(result.data().replacement);
    EXPECT_EQ(result.data().unavailable_reason, expected);
}

bool same(const PasswordCredential &left, const PasswordCredential &right) {
    return left.account_ref == right.account_ref && left.revision_ref == right.revision_ref &&
           left.encoded_verifier == right.encoded_verifier;
}

enum class Mutation { none, revision, encoded_record, denied };

// Simulated authoritative account/login state. This port never performs or fakes crypto.
class LoginSource final : public PasswordLoginSource {
  public:
    PasswordCredentialRead reading{
        PasswordCredentialState::present,
        PasswordCredential{std::string(account), "rev-A", std::string(vectors::h0)}, "attempt-A"};
    PasswordCredential current{*reading.credential};
    PasswordCredential accepted_source{current};
    std::string accepted_attempt{"attempt-A"};
    bool login_accepted{true};
    unsigned int reads{};
    unsigned int checks{};
    bool all_proofs_belong{true};
    bool all_proofs_match_read{true};
    bool guarded_during_checks{true};
    bool same_buffer_during_checks{true};
    bool crypto_precedes_checks{true};
    bool throw_read{};
    bool allocation_failure_read{};
    unsigned int unavailable_on{};
    unsigned int invalid_enum_on{};
    unsigned int throw_on{};
    unsigned int allocation_failure_on{};
    unsigned int mutate_on{};
    Mutation mutation{Mutation::none};

    PasswordCredentialRead read_current(std::string_view requested) override {
        ++reads;
        EXPECT_EQ(requested, account_request);
        if (throw_read) {
            throw std::runtime_error("Synthetic account diagnostic must not escape");
        }
        if (allocation_failure_read) {
            throw std::bad_alloc{};
        }
        return reading;
    }

    PasswordLoginCheck
    check_current_successful_login(const PasswordVerifiedSource &verified) override {
        ++checks;
        all_proofs_belong &= verified.belongs_to(*this);
        all_proofs_match_read &= reading.credential &&
                                 same(verified.credential(), *reading.credential) &&
                                 verified.login_attempt_ref() == reading.login_attempt_ref;
        const auto observed = native::snapshot();
        guarded_during_checks &= observed.first_password_live && observed.first_password_locked &&
                                 observed.first_password_uninterrupted;
        same_buffer_during_checks &=
            observed.same_native_password_buffer && observed.first_password_live;
        crypto_precedes_checks &=
            observed.verification_calls == 1 && observed.hash_calls == checks - 1;
        if (checks == throw_on) {
            throw std::runtime_error("Synthetic gate diagnostic must not escape");
        }
        if (checks == allocation_failure_on) {
            throw std::bad_alloc{};
        }
        if (checks == unavailable_on) {
            return PasswordLoginCheck::unavailable;
        }
        if (checks == invalid_enum_on) {
            return static_cast<PasswordLoginCheck>(1234);
        }
        if (checks == mutate_on) {
            switch (mutation) {
            case Mutation::revision:
                current.revision_ref = "rev-new";
                break;
            case Mutation::encoded_record:
                current.encoded_verifier = "newer credential";
                break;
            case Mutation::denied:
                login_accepted = false;
                break;
            case Mutation::none:
                break;
            }
        }
        if (!verified.belongs_to(*this)) {
            return PasswordLoginCheck::denied;
        }
        if (!same(current, verified.credential())) {
            return PasswordLoginCheck::stale;
        }
        if (!login_accepted || !same(accepted_source, verified.credential()) ||
            accepted_attempt != verified.login_attempt_ref()) {
            return PasswordLoginCheck::denied;
        }
        return PasswordLoginCheck::accepted;
    }

    std::string account_request{account};
};

// No SDK call here: a fatal assertion returns only from this helper. Each caller must inspect its
// own completion flag before invoking any oracle, independently of earlier rows' failure status.
void bounded_record(const std::string &encoded, std::string_view prefix, bool &bounded) {
    bounded = false;
    ASSERT_LE(encoded.size(), 127U);
    ASSERT_TRUE(encoded.starts_with(prefix));
    ASSERT_EQ(encoded.find('\0'), std::string::npos);
    const auto salt_end = encoded.find('$', prefix.size());
    ASSERT_NE(salt_end, std::string::npos);
    ASSERT_EQ(salt_end - prefix.size(), 22U);
    ASSERT_EQ(encoded.size() - salt_end - 1, 43U);
    bounded = true;
}

// Caller has successfully bounded this exact immutable record before any real SDK oracle.
// No project verifier/setup credential or predetermined random hash is used as the oracle.
void real_record(const std::string &encoded, PasswordBytes password) {
    const char empty = '\0';
    const auto *pointer =
        password.empty() ? &empty : reinterpret_cast<const char *>(password.data());
    native::RealLibraryOnly real;
    EXPECT_EQ(crypto_pwhash_argon2id_str_verify(encoded.c_str(), pointer, password.size()), 0);
}

enum class Operation { create, verify, upgrade };
void call_raw(Operation operation, PasswordBytes password, const PasswordHashingOptions &options,
              LoginSource &source) {
    switch (operation) {
    case Operation::create:
        (void)create_password_hash(password, options);
        break;
    case Operation::verify:
        (void)verify_password(password, vectors::h0, options);
        break;
    case Operation::upgrade:
        (void)propose_password_upgrade(password, source.account_request, source, options);
        break;
    }
}
constexpr std::array operations{Operation::create, Operation::verify, Operation::upgrade};
} // namespace

// PH01/03/04/05: raw bytes are preserved; real SDK alone checks each returned new verifier.
TEST(PasswordCreation, EmptyNulUnicodeAndFullLengthRemainExactBytes) {
    struct Row {
        const char *id;
        std::vector<std::byte> password;
        std::vector<std::byte> different;
    };
    const std::vector<Row> rows{
        {"empty", {}, {std::byte{0}}},
        {"embedded-NUL", {vectors::with_nul.begin(), vectors::with_nul.end()}, {std::byte{0x61}}},
        {"UTF8-composed",
         {vectors::composed.begin(), vectors::composed.end()},
         {vectors::decomposed.begin(), vectors::decomposed.end()}},
        {"UTF8-decomposed",
         {vectors::decomposed.begin(), vectors::decomposed.end()},
         {vectors::composed.begin(), vectors::composed.end()}},
        {"spaces-preserved",
         {std::byte{0x20}, std::byte{0x61}, std::byte{0x20}},
         {std::byte{0x61}}},
        {"exactly-1024", std::vector<std::byte>(1024, std::byte{0x58}),
         std::vector<std::byte>(1023, std::byte{0x58})}};
    for (const auto &row : rows) {
        SCOPED_TRACE(row.id);
        native::Controls control(native::Failure::none, row.password);
        const auto result = successful([&] { return create_password_hash(row.password); });
        if (!result) {
            continue;
        }
        bool bounded = false;
        bounded_record(*result, "$argon2id$v=19$m=65536,t=3,p=1$", bounded);
        if (!bounded) {
            continue;
        }
        real_record(*result, row.password);
        const char empty = '\0';
        const auto *different =
            row.different.empty() ? &empty : reinterpret_cast<const char *>(row.different.data());
        {
            native::RealLibraryOnly real;
            EXPECT_EQ(
                crypto_pwhash_argon2id_str_verify(result->c_str(), different, row.different.size()),
                -1);
        }
        EXPECT_EQ(native::snapshot().hash_operations, 3U);
        EXPECT_EQ(native::snapshot().hash_memory_bytes, 67108864U);
        EXPECT_EQ(native::snapshot().hash_algorithm, 2);
        exact_protected_input();
    }
}

// PH11: acceptance of the boundary costs is observed through an error-only native hash refusal.
TEST(PasswordCreation, ExactDesiredCostBoundariesReachNativeWithoutSubstitution) {
    for (const auto &[passes, memory] : std::array<std::pair<std::uint32_t, std::uint64_t>, 4>{
             {{2, 67108864}, {10, 67108864}, {2, 268435456}, {10, 268435456}}}) {
        SCOPED_TRACE(passes);
        SCOPED_TRACE(memory);
        native::Controls control(native::Failure::hashing, {});
        failure(PasswordHashingError::hash_unavailable,
                [&] { return create_password_hash({}, {passes, memory, 268435456}); });
        EXPECT_EQ(native::snapshot().hash_operations, passes);
        EXPECT_EQ(native::snapshot().hash_memory_bytes, memory);
        exact_protected_input();
    }
}

TEST(PasswordPreflight, Password1025RefusesBeforeAnyNativeOrSourceWorkForEveryEntrypoint) {
    const std::vector<std::byte> oversized(1025, std::byte{0x58});
    for (auto operation : operations) {
        SCOPED_TRACE(static_cast<int>(operation));
        LoginSource source;
        native::Controls control(native::Failure::hashing, oversized);
        failure(PasswordHashingError::resource_limit,
                [&] { call_raw(operation, oversized, {}, source); });
        EXPECT_EQ(source.reads, 0U);
        EXPECT_EQ(source.checks, 0U);
        no_native_work();
    }
}

// Options precede oversize raw input for all three entrypoints, including unused verify fields.
TEST(PasswordPreflight, InvalidOptionsAlwaysPrecedeInputAndSourceErrors) {
    const std::array<PasswordHashingOptions, 9> invalid{
        {{1, 67108864, 268435456},
         {11, 67108864, 268435456},
         {3, 67107840, 268435456},
         {3, 67108865, 268435456},
         {3, 268436480, 268435456},
         {3, 67108864, 268436480},
         {3, 67108864, 1},
         {std::numeric_limits<std::uint32_t>::max(), 67108864, 268435456},
         {3, std::numeric_limits<std::uint64_t>::max(), 268435456}}};
    const std::vector<std::byte> oversized(1025, std::byte{0x58});
    for (std::size_t row = 0; row < invalid.size(); ++row) {
        SCOPED_TRACE(row);
        for (auto operation : operations) {
            SCOPED_TRACE(static_cast<int>(operation));
            LoginSource source;
            native::Controls control(native::Failure::hashing, oversized);
            failure(PasswordHashingError::invalid_options,
                    [&] { call_raw(operation, oversized, invalid[row], source); });
            EXPECT_EQ(source.reads, 0U);
            no_native_work();
        }
    }
}

TEST(PasswordPreflight, ZeroAndInsufficientCreationAdmissionAreNotUnlimited) {
    for (std::uint64_t admitted : {0ULL, 67107840ULL}) {
        SCOPED_TRACE(admitted);
        native::Controls control(native::Failure::hashing, {});
        failure(PasswordHashingError::resource_limit,
                [&] { return create_password_hash({}, {3, 67108864, admitted}); });
        no_native_work();
    }
}

TEST(PasswordVerification, PublishedEmptyH0VerifiesWithoutAnEarlierSutCall) {
    native::Controls control(native::Failure::none, {}, vectors::h0);
    const auto result = successful([&] { return verify_password({}, vectors::h0); });
    if (result) {
        EXPECT_EQ(*result, PasswordVerification::verified);
        EXPECT_EQ(native::snapshot().verification_calls, 1U);
        EXPECT_EQ(native::snapshot().hash_calls, 0U);
        exact_protected_input();
    }
}

TEST(PasswordVerification, OneNulIsNotEmptyAndNativeMinusOneIsNotReasonSpecific) {
    native::Controls control(native::Failure::none, vectors::one_nul, vectors::h0);
    const auto result = successful([&] { return verify_password(vectors::one_nul, vectors::h0); });
    if (result) {
        EXPECT_EQ(*result, PasswordVerification::not_successful);
        exact_protected_input();
    }
}

TEST(PasswordVerification, BorrowedNonterminatedViewGetsItsOwnNativeTerminator) {
    std::array<char, 128> buffer{};
    buffer.fill('X');
    std::copy(vectors::h0.begin(), vectors::h0.end(), buffer.begin());
    const std::string_view view(buffer.data(), vectors::h0.size());
    native::Controls control(native::Failure::none, {}, vectors::h0);
    const auto result = successful([&] { return verify_password({}, view); });
    if (result) {
        EXPECT_EQ(*result, PasswordVerification::verified);
        EXPECT_EQ(buffer[vectors::h0.size()], 'X');
        exact_protected_input();
    }
}

TEST(PasswordVerification, Exact1024ByteInputIsAdmittedByVerifyAndUpgrade) {
    const std::vector<std::byte> password(1024, std::byte{0x58});
    for (bool upgrade : {false, true}) {
        SCOPED_TRACE(upgrade);
        LoginSource source;
        native::Controls control(native::Failure::verification, password, vectors::h0);
        if (upgrade) {
            const auto result =
                successful([&] { return propose_password_upgrade(password, account, source); });
            if (result) {
                empty_outcome(*result, PasswordUpgradeOutcome::verification_not_successful);
            }
        } else {
            const auto result = successful([&] { return verify_password(password, vectors::h0); });
            if (result) {
                EXPECT_EQ(*result, PasswordVerification::not_successful);
            }
        }
        EXPECT_EQ(native::snapshot().verification_calls, 1U);
        EXPECT_EQ(source.checks, 0U);
        exact_protected_input();
    }
}

// PH06/07: bounded raw assembly never computes classification with a parser/SUT helper.
TEST(PasswordRecordPreflight, MalformedFramingNumbersAndBase64NeverReachNative) {
    const std::array headers{"$ARGON2ID$v=19$m=4882,t=2,p=1",     "$argon2id$v=019$m=4882,t=2,p=1",
                             "$argon2id$v=19$m=04882,t=2,p=1",    "$argon2id$v=19$m=+4882,t=2,p=1",
                             "$argon2id$v=19$m=4882,t=02,p=1",    "$argon2id$v=19$m=4882,t=2,p=01",
                             "$argon2id$v=19$m=4882,t=2",         "$argon2id$v=19$t=2,m=4882,p=1",
                             "$argon2id$v=19$m=4882,t=2,p=1,x=0", "$argon2id$v=19$m=7,t=2,p=1",
                             "$argon2id$v=19$m=4882,t=0,p=1",     "$argon2id$v=19$m=4882,t=2,p=0"};
    std::vector<std::pair<std::string, std::string>> rows;
    for (auto header : headers) {
        rows.emplace_back(header, record(header));
    }
    rows.emplace_back("trailing-LF", std::string(vectors::h0) + "\n");
    rows.emplace_back("trailing-NUL", std::string(vectors::h0) + '\0');
    rows.emplace_back("non-ASCII", std::string(vectors::h0) + static_cast<char>(0xff));
    rows.emplace_back("extra-field", std::string(vectors::h0) + "$extra");
    rows.emplace_back("salt-padding",
                      record("$argon2id$v=19$m=4882,t=2,p=1", "bA81arsiXysd3WbTRzmEOw=="));
    rows.emplace_back("salt-unused-bits",
                      record("$argon2id$v=19$m=4882,t=2,p=1", "bA81arsiXysd3WbTRzmEOx"));
    rows.emplace_back("digest-unused-bits",
                      record("$argon2id$v=19$m=4882,t=2,p=1", vectors::h0_salt,
                             "Nm8QBM+7RH1DXo9rvp5cwKEOOOfD2g6JuxlXihoNcpF"));
    rows.emplace_back("salt-length-21",
                      record("$argon2id$v=19$m=4882,t=2,p=1", "bA81arsiXysd3WbTRzmEO"));
    rows.emplace_back("salt-length-23",
                      record("$argon2id$v=19$m=4882,t=2,p=1", "bA81arsiXysd3WbTRzmEOwA"));
    rows.emplace_back("digest-length-42", record("$argon2id$v=19$m=4882,t=2,p=1", vectors::h0_salt,
                                                 "Nm8QBM+7RH1DXo9rvp5cwKEOOOfD2g6JuxlXihoNcp"));
    rows.emplace_back("digest-length-44", record("$argon2id$v=19$m=4882,t=2,p=1", vectors::h0_salt,
                                                 "Nm8QBM+7RH1DXo9rvp5cwKEOOOfD2g6JuxlXihoNcpE="));
    rows.emplace_back("salt-invalid-alphabet",
                      record("$argon2id$v=19$m=4882,t=2,p=1", "_A81arsiXysd3WbTRzmEOw"));
    rows.emplace_back("empty", "");
    for (const auto &[id, input] : rows) {
        SCOPED_TRACE(id);
        native::Controls control(native::Failure::verification, {}, input);
        failure(PasswordHashingError::invalid_record, [&] { return verify_password({}, input); });
        no_native_work();
    }
}

// PH08/09/10: full malformed checks precede unsupported profile, which precedes supported cost.
TEST(PasswordRecordPreflight, UnsupportedAndCostCombinedFailuresHaveFixedPriority) {
    struct Row {
        const char *header;
        PasswordHashingError expected;
    };
    const std::array rows{
        Row{"$argon2i$v=19$m=4882,t=2,p=1", PasswordHashingError::unsupported_record},
        Row{"$other$v=19$m=4882,t=2,p=1", PasswordHashingError::unsupported_record},
        Row{"$argon2id$v=16$m=4882,t=2,p=1", PasswordHashingError::unsupported_record},
        Row{"$argon2id$v=0$m=4882,t=2,p=1", PasswordHashingError::unsupported_record},
        Row{"$argon2id$v=19$m=4882,t=2,p=2", PasswordHashingError::unsupported_record},
        Row{"$argon2id$v=19$m=4882,t=2,p=18446744073709551616",
            PasswordHashingError::unsupported_record},
        Row{"$argon2id$v=19$m=4882,t=11,p=2", PasswordHashingError::unsupported_record},
        Row{"$argon2id$v=16$m=18446744073709551616,t=2,p=1",
            PasswordHashingError::unsupported_record},
        Row{"$argon2i$v=19$m=4882,t=0,p=1", PasswordHashingError::invalid_record},
        Row{"$argon2id$v=19$m=7,t=2,p=2", PasswordHashingError::invalid_record},
        Row{"$argon2id$v=19$m=4882,t=11,p=1", PasswordHashingError::resource_limit},
        Row{"$argon2id$v=19$m=262145,t=2,p=1", PasswordHashingError::resource_limit},
        Row{"$argon2id$v=19$m=18446744073709551616,t=2,p=1", PasswordHashingError::resource_limit},
        Row{"$argon2id$v=19$m=4882,t=18446744073709551616,p=1",
            PasswordHashingError::resource_limit}};
    for (const auto &row : rows) {
        SCOPED_TRACE(row.header);
        const auto input = record(row.header);
        native::Controls control(native::Failure::verification, {}, input);
        failure(row.expected, [&] { return verify_password({}, input); });
        no_native_work();
    }
}

TEST(PasswordRecordPreflight, Exact127ByteCeilingPrecedesGrammarAt128) {
    const auto exact = record("$abcdefghijklmnopqrstuvwxyzabcdef$v=111111111111$m=8,t=1,p=1");
    ASSERT_EQ(exact.size(),
              127U); // independent literal input sanity, never an implementation oracle
    for (bool excessive : {false, true}) {
        SCOPED_TRACE(excessive);
        const auto input = excessive ? exact + "X" : exact;
        native::Controls control(native::Failure::verification, {}, input);
        failure(excessive ? PasswordHashingError::resource_limit
                          : PasswordHashingError::unsupported_record,
                [&] { return verify_password({}, input); });
        no_native_work();
    }
}

TEST(PasswordRecordPreflight, SupportedStoredEndpointsReachOnlyUnclassifiedNativeFailure) {
    for (auto header : {"$argon2id$v=19$m=8,t=1,p=1", "$argon2id$v=19$m=262144,t=10,p=1"}) {
        SCOPED_TRACE(header);
        // Edited costs are NOT claimed valid success fixtures. Error-only native refusal is the
        // oracle.
        const auto input = record(header);
        native::Controls control(native::Failure::verification, {}, input);
        const auto result = successful([&] { return verify_password({}, input); });
        if (result) {
            EXPECT_EQ(*result, PasswordVerification::not_successful);
            EXPECT_EQ(native::snapshot().verification_calls, 1U);
            exact_protected_input();
        }
    }
}

TEST(PasswordPreflight, StoredAdmissionUsesActualRecordAndRejectsBeforeInitialization) {
    for (std::uint64_t admitted : {0ULL, 4998144ULL}) {
        SCOPED_TRACE(admitted);
        for (auto operation : {Operation::verify, Operation::upgrade}) {
            SCOPED_TRACE(static_cast<int>(operation));
            LoginSource source;
            native::Controls control(native::Failure::verification, {}, vectors::h0);
            failure(PasswordHashingError::resource_limit,
                    [&] { call_raw(operation, {}, {3, 67108864, admitted}, source); });
            EXPECT_EQ(source.checks, 0U);
            no_native_work();
        }
    }
}

TEST(PasswordSource, MissingUnavailableAndInvalidReadShapesAreDistinctBeforeCrypto) {
    for (int row = 0; row < 7; ++row) {
        SCOPED_TRACE(row);
        LoginSource source;
        if (row == 0 || row == 1) {
            source.reading = {row == 0 ? PasswordCredentialState::missing
                                       : PasswordCredentialState::unavailable,
                              {},
                              {}};
        } else if (row == 2) {
            source.reading.state = static_cast<PasswordCredentialState>(1234);
        } else if (row == 3) {
            source.reading.state = PasswordCredentialState::missing;
        } else if (row == 4) {
            source.reading.credential.reset();
        } else if (row == 5) {
            source.reading = {PasswordCredentialState::unavailable, {}, "unexpected-attempt"};
        } else {
            source.reading.login_attempt_ref.clear();
        }
        native::Controls control(native::Failure::verification, {}, vectors::h0);
        if (row == 0) {
            const auto result =
                successful([&] { return propose_password_upgrade({}, account, source); });
            if (result)
                empty_outcome(*result, PasswordUpgradeOutcome::credential_missing);
        } else {
            failure(row == 1 ? PasswordHashingError::source_unavailable
                             : PasswordHashingError::source_invalid,
                    [&] { return propose_password_upgrade({}, account, source); });
        }
        EXPECT_EQ(source.reads, 1U);
        EXPECT_EQ(source.checks, 0U);
        no_native_work();
    }
}

TEST(PasswordSource, AccountAndSourceFieldsHaveOrderedByteAndMembershipChecks) {
    for (int row = 0; row < 9; ++row) {
        SCOPED_TRACE(row);
        LoginSource source;
        auto &credential = *source.reading.credential;
        PasswordHashingError expected = PasswordHashingError::source_invalid;
        std::string requested(account);
        switch (row) {
        case 0:
            requested.clear();
            expected = PasswordHashingError::invalid_account_reference;
            break;
        case 1:
            requested.assign(257, 'A');
            expected = PasswordHashingError::resource_limit;
            break;
        case 2:
            credential.account_ref = "other";
            break;
        case 3:
            credential.revision_ref.clear();
            break;
        case 4:
            credential.account_ref.assign(257, 'A');
            expected = PasswordHashingError::resource_limit;
            break;
        case 5:
            credential.revision_ref.assign(257, 'R');
            expected = PasswordHashingError::resource_limit;
            break;
        case 6:
            credential.encoded_verifier.assign(128, 'X');
            expected = PasswordHashingError::resource_limit;
            break;
        case 7:
            source.reading.login_attempt_ref.assign(257, 'L');
            expected = PasswordHashingError::resource_limit;
            break;
        case 8:
            credential.account_ref = "wrong";
            credential.encoded_verifier.assign(128, 'X');
            expected = PasswordHashingError::resource_limit;
            break;
        }
        source.account_request = requested;
        native::Controls control(native::Failure::verification, {}, vectors::h0);
        failure(expected, [&] { return propose_password_upgrade({}, requested, source); });
        EXPECT_EQ(source.reads, row < 2 ? 0U : 1U);
        EXPECT_EQ(source.checks, 0U);
        no_native_work();
    }
}

TEST(PasswordSource, ReadExceptionsDiscardDiagnosticsAndMapAllocationSeparately) {
    for (bool allocation : {false, true}) {
        SCOPED_TRACE(allocation);
        LoginSource source;
        source.throw_read = !allocation;
        source.allocation_failure_read = allocation;
        native::Controls control(native::Failure::verification, {}, vectors::h0);
        failure(allocation ? PasswordHashingError::allocation_unavailable
                           : PasswordHashingError::source_unavailable,
                [&] { return propose_password_upgrade({}, account, source); });
        EXPECT_EQ(source.checks, 0U);
        no_native_work();
    }
}

TEST(PasswordSource, ActualStoredRecordIsPreflightedWithoutRepairOrPolicySubstitution) {
    struct Row {
        std::string input;
        PasswordHashingError expected;
    };
    const std::array rows{
        Row{"", PasswordHashingError::invalid_record},
        Row{std::string(vectors::h0) + "\n", PasswordHashingError::invalid_record},
        Row{record("$argon2id$v=19$m=4882,t=2,p=2"), PasswordHashingError::unsupported_record},
        Row{record("$argon2id$v=19$m=4882,t=11,p=1"), PasswordHashingError::resource_limit}};
    for (std::size_t row = 0; row < rows.size(); ++row) {
        SCOPED_TRACE(row);
        LoginSource source;
        source.reading.credential->encoded_verifier = rows[row].input;
        native::Controls control(native::Failure::verification, {}, rows[row].input);
        failure(rows[row].expected, [&] { return propose_password_upgrade({}, account, source); });
        EXPECT_EQ(source.reads, 1U);
        EXPECT_EQ(source.checks, 0U);
        no_native_work();
    }
}

TEST(PasswordUpgrade, RawH0AcceptedLoginProducesOnlySourceBoundOwnedProposal) {
    LoginSource source;
    native::Controls control(native::Failure::none, {}, vectors::h0);
    const auto result = successful([&] { return propose_password_upgrade({}, account, source); });
    if (!result)
        return;
    EXPECT_EQ(result->data().outcome, PasswordUpgradeOutcome::replacement_ready);
    EXPECT_FALSE(result->data().unavailable_reason);
    ASSERT_TRUE(result->data().replacement);
    const auto &replacement = *result->data().replacement;
    EXPECT_EQ(replacement.account_ref, account);
    EXPECT_EQ(replacement.login_attempt_ref, "attempt-A");
    EXPECT_EQ(replacement.expected_revision_ref, "rev-A");
    EXPECT_EQ(replacement.expected_encoded_verifier, vectors::h0);
    bool bounded = false;
    bounded_record(replacement.replacement_encoded_verifier, "$argon2id$v=19$m=65536,t=3,p=1$",
                   bounded);
    if (!bounded) {
        return;
    }
    real_record(replacement.replacement_encoded_verifier, {});
    EXPECT_EQ(source.reads, 1U);
    EXPECT_EQ(source.checks, 2U);
    EXPECT_TRUE(source.all_proofs_belong);
    EXPECT_TRUE(source.all_proofs_match_read);
    EXPECT_TRUE(source.guarded_during_checks);
    EXPECT_TRUE(source.same_buffer_during_checks);
    EXPECT_TRUE(source.crypto_precedes_checks);
    EXPECT_EQ(source.current.encoded_verifier, vectors::h0); // no write port exists
    EXPECT_EQ(source.current.revision_ref, "rev-A");
    exact_protected_input();
}

TEST(PasswordUpgrade, NativeUnsuccessfulAndDeniedLoginNeverReachReplacementHash) {
    for (int row = 0; row < 3; ++row) {
        SCOPED_TRACE(row);
        LoginSource source;
        source.login_accepted = row != 2;
        const PasswordBytes password = row == 0 ? PasswordBytes(vectors::one_nul) : PasswordBytes{};
        native::Controls control(row == 1 ? native::Failure::verification : native::Failure::none,
                                 password, vectors::h0);
        const auto result =
            successful([&] { return propose_password_upgrade(password, account, source); });
        if (result) {
            empty_outcome(*result, row == 2 ? PasswordUpgradeOutcome::login_not_accepted
                                            : PasswordUpgradeOutcome::verification_not_successful);
            EXPECT_EQ(source.checks, row == 2 ? 1U : 0U);
            EXPECT_EQ(native::snapshot().hash_calls, 0U);
            EXPECT_EQ(source.current.encoded_verifier, vectors::h0);
            exact_protected_input();
        }
    }
}

TEST(PasswordUpgrade, OpaqueAttemptOrPriorLoginReferencesCannotReplaceActualAcceptedSource) {
    for (bool wrong_attempt : {false, true}) {
        SCOPED_TRACE(wrong_attempt);
        LoginSource source;
        if (wrong_attempt) {
            source.reading.login_attempt_ref = "unaccepted-attempt";
        } else {
            source.accepted_source.revision_ref = "different-accepted-source";
        }
        native::Controls control(native::Failure::none, {}, vectors::h0);
        const auto result =
            successful([&] { return propose_password_upgrade({}, account, source); });
        if (result) {
            empty_outcome(*result, PasswordUpgradeOutcome::login_not_accepted);
            EXPECT_EQ(source.checks, 1U);
            EXPECT_TRUE(source.crypto_precedes_checks);
            EXPECT_EQ(native::snapshot().hash_calls, 0U);
            EXPECT_EQ(source.current.encoded_verifier, vectors::h0);
            exact_protected_input();
        }
    }
}

TEST(PasswordUpgrade, ChangedRevisionBeforeFirstGatePreventsHashing) {
    LoginSource source;
    source.mutate_on = 1;
    source.mutation = Mutation::revision;
    native::Controls control(native::Failure::none, {}, vectors::h0);
    const auto result = successful([&] { return propose_password_upgrade({}, account, source); });
    if (result) {
        empty_outcome(*result, PasswordUpgradeOutcome::stale_source);
        EXPECT_EQ(source.current.revision_ref, "rev-new");
        EXPECT_EQ(source.checks, 1U);
        EXPECT_EQ(native::snapshot().hash_calls, 0U);
        exact_protected_input();
    }
}

TEST(PasswordUpgrade, SecondGateRejectsChangedRevisionRecordOrLoginAndDiscardsNewHash) {
    for (auto mutation : {Mutation::revision, Mutation::encoded_record, Mutation::denied}) {
        SCOPED_TRACE(static_cast<int>(mutation));
        LoginSource source;
        source.mutate_on = 2;
        source.mutation = mutation;
        native::Controls control(native::Failure::none, {}, vectors::h0);
        const auto result =
            successful([&] { return propose_password_upgrade({}, account, source); });
        if (result) {
            empty_outcome(*result, mutation == Mutation::denied
                                       ? PasswordUpgradeOutcome::login_not_accepted
                                       : PasswordUpgradeOutcome::stale_source);
            EXPECT_EQ(source.checks, 2U);
            EXPECT_EQ(native::snapshot().hash_calls, 1U);
            if (mutation == Mutation::revision) {
                EXPECT_EQ(source.current.revision_ref, "rev-new");
            }
            if (mutation == Mutation::encoded_record) {
                EXPECT_EQ(source.current.encoded_verifier, "newer credential");
            }
            if (mutation == Mutation::denied) {
                EXPECT_FALSE(source.login_accepted);
            }
            exact_protected_input();
        }
    }
}

TEST(PasswordUpgrade, GateFailuresAreTypedAndClearProtectedInputAtEitherPhase) {
    for (unsigned int phase : {1U, 2U}) {
        SCOPED_TRACE(phase);
        for (int kind = 0; kind < 4; ++kind) {
            SCOPED_TRACE(kind);
            LoginSource source;
            if (kind == 0)
                source.unavailable_on = phase;
            if (kind == 1)
                source.invalid_enum_on = phase;
            if (kind == 2)
                source.throw_on = phase;
            if (kind == 3)
                source.allocation_failure_on = phase;
            native::Controls control(native::Failure::none, {}, vectors::h0);
            failure(kind == 1   ? PasswordHashingError::source_invalid
                    : kind == 3 ? PasswordHashingError::allocation_unavailable
                                : PasswordHashingError::source_unavailable,
                    [&] { return propose_password_upgrade({}, account, source); });
            EXPECT_EQ(source.checks, phase);
            EXPECT_EQ(source.current.encoded_verifier, vectors::h0);
            EXPECT_TRUE(source.guarded_during_checks);
            EXPECT_TRUE(source.crypto_precedes_checks);
            exact_protected_input();
        }
    }
}

TEST(PasswordUpgrade, AdmittedVerificationDoesNotAuthorizeLargerReplacementMemory) {
    LoginSource source;
    native::Controls control(native::Failure::none, {}, vectors::h0);
    const auto result = successful(
        [&] { return propose_password_upgrade({}, account, source, {3, 67108864, 8388608}); });
    if (result) {
        unavailable(*result, PasswordHashingError::resource_limit);
        EXPECT_EQ(source.checks, 1U);
        EXPECT_EQ(native::snapshot().verification_calls, 1U);
        EXPECT_EQ(native::snapshot().hash_calls, 0U);
        EXPECT_EQ(source.current.encoded_verifier, vectors::h0);
        exact_protected_input();
    }
}

TEST(PasswordUpgrade, NativeHashFailureOrRejectedOutputKeepsOldRecordAndAcceptedLogin) {
    for (auto mode : {native::Failure::hashing, native::Failure::unterminated_hash_output,
                      native::Failure::wrong_hash_profile}) {
        SCOPED_TRACE(static_cast<int>(mode));
        LoginSource source;
        native::Controls control(mode, {}, vectors::h0);
        const auto result =
            successful([&] { return propose_password_upgrade({}, account, source); });
        if (result) {
            unavailable(*result, mode == native::Failure::hashing
                                     ? PasswordHashingError::hash_unavailable
                                     : PasswordHashingError::native_output_invalid);
            EXPECT_EQ(source.checks, 1U);
            EXPECT_TRUE(source.login_accepted);
            EXPECT_EQ(source.current.encoded_verifier, vectors::h0);
            EXPECT_EQ(native::snapshot().hash_calls, 1U);
            exact_protected_input();
        }
    }
}

TEST(PasswordUpgrade, PublishedHashcatInputAlreadyStrongEnoughIsByteIdentical) {
    LoginSource source;
    source.reading.credential->encoded_verifier = vectors::hashcat_record;
    source.current = *source.reading.credential;
    source.accepted_source = source.current;
    const auto password = bytes(vectors::hashcat_password);
    native::Controls control(native::Failure::none, password, vectors::hashcat_record);
    const auto result = successful([&] {
        return propose_password_upgrade(password, account, source, {2, 67108864, 268435456});
    });
    if (result) {
        empty_outcome(*result, PasswordUpgradeOutcome::not_needed);
        EXPECT_EQ(source.checks, 1U);
        EXPECT_EQ(native::snapshot().hash_calls, 0U);
        EXPECT_EQ(source.current.encoded_verifier, vectors::hashcat_record);
        exact_protected_input();
    }
}

TEST(PasswordUpgrade, AdditionalPublishedInputRetainsStrongerTimeWhileRaisingMemory) {
    LoginSource source;
    source.reading.credential->encoded_verifier = vectors::hashcat_record;
    source.current = *source.reading.credential;
    source.accepted_source = source.current;
    const auto password = bytes(vectors::hashcat_password);
    native::Controls control(native::Failure::none, password, vectors::hashcat_record);
    const auto result = successful([&] {
        return propose_password_upgrade(password, account, source, {2, 134217728, 268435456});
    });
    if (!result)
        return;
    EXPECT_EQ(result->data().outcome, PasswordUpgradeOutcome::replacement_ready);
    ASSERT_TRUE(result->data().replacement);
    bool bounded = false;
    bounded_record(result->data().replacement->replacement_encoded_verifier,
                   "$argon2id$v=19$m=131072,t=3,p=1$", bounded);
    if (!bounded) {
        return;
    }
    real_record(result->data().replacement->replacement_encoded_verifier, password);
    EXPECT_EQ(native::snapshot().hash_operations, 3U);
    EXPECT_EQ(native::snapshot().hash_memory_bytes, 134217728U);
    EXPECT_EQ(source.checks, 2U);
    exact_protected_input();
}

// PH12: exact independently prepared input, not an H0 metadata edit or prior SUT setup.
TEST(PasswordUpgrade, PH12RetainsSixPassesWhileRaising32768KiBTo65536) {
    LoginSource source;
    source.reading.credential->encoded_verifier = vectors::ph12_record;
    source.current = *source.reading.credential;
    source.accepted_source = source.current;
    const auto password = bytes(vectors::ph12_password);
    native::Controls control(native::Failure::none, password, vectors::ph12_record);
    const auto result =
        successful([&] { return propose_password_upgrade(password, account, source); });
    if (!result) {
        return;
    }
    EXPECT_EQ(result->data().outcome, PasswordUpgradeOutcome::replacement_ready);
    EXPECT_FALSE(result->data().unavailable_reason);
    ASSERT_TRUE(result->data().replacement);
    EXPECT_EQ(result->data().replacement->expected_encoded_verifier, vectors::ph12_record);
    bool bounded = false;
    bounded_record(result->data().replacement->replacement_encoded_verifier,
                   "$argon2id$v=19$m=65536,t=6,p=1$", bounded);
    if (!bounded) {
        return;
    }
    real_record(result->data().replacement->replacement_encoded_verifier, password);
    EXPECT_EQ(native::snapshot().hash_operations, 6U);
    EXPECT_EQ(native::snapshot().hash_memory_bytes, 67108864U);
    EXPECT_EQ(source.checks, 2U);
    EXPECT_TRUE(source.crypto_precedes_checks);
    EXPECT_EQ(source.current.encoded_verifier, vectors::ph12_record);
    exact_protected_input();
}

// PH13: already stronger in both dimensions; retained encoded bytes are untouched.
TEST(PasswordUpgrade, PH13SixPassesAnd131072KiBNeedNoReplacement) {
    LoginSource source;
    source.reading.credential->encoded_verifier = vectors::ph13_record;
    source.current = *source.reading.credential;
    source.accepted_source = source.current;
    const auto password = bytes(vectors::ph13_password);
    native::Controls control(native::Failure::none, password, vectors::ph13_record);
    const auto result =
        successful([&] { return propose_password_upgrade(password, account, source); });
    if (result) {
        empty_outcome(*result, PasswordUpgradeOutcome::not_needed);
        EXPECT_EQ(source.current.encoded_verifier, vectors::ph13_record);
        EXPECT_EQ(source.checks, 1U);
        EXPECT_TRUE(source.crypto_precedes_checks);
        EXPECT_EQ(native::snapshot().hash_calls, 0U);
        exact_protected_input();
    }
}

TEST(PasswordUpgrade, StrongerStoredMemoryIsPreservedWhenDesiredPassesIncrease) {
    LoginSource source;
    source.reading.credential->encoded_verifier = vectors::ph13_record;
    source.current = *source.reading.credential;
    source.accepted_source = source.current;
    const auto password = bytes(vectors::ph13_password);
    native::Controls control(native::Failure::none, password, vectors::ph13_record);
    const auto result = successful([&] {
        return propose_password_upgrade(password, account, source, {10, 67108864, 268435456});
    });
    if (!result) {
        return;
    }
    EXPECT_EQ(result->data().outcome, PasswordUpgradeOutcome::replacement_ready);
    ASSERT_TRUE(result->data().replacement);
    bool bounded = false;
    bounded_record(result->data().replacement->replacement_encoded_verifier,
                   "$argon2id$v=19$m=131072,t=10,p=1$", bounded);
    if (!bounded) {
        return;
    }
    real_record(result->data().replacement->replacement_encoded_verifier, password);
    EXPECT_EQ(native::snapshot().hash_operations, 10U);
    EXPECT_EQ(native::snapshot().hash_memory_bytes, 134217728U);
    EXPECT_EQ(source.checks, 2U);
    EXPECT_TRUE(source.crypto_precedes_checks);
    exact_protected_input();
}

TEST(PasswordUpgrade, Opaque256ByteIdentityAndAttemptArePreservedWithoutTextRules) {
    LoginSource source;
    std::string opaque(256, 'A');
    opaque[0] = '\0';
    opaque[255] = static_cast<char>(0xff);
    source.account_request = opaque;
    source.reading.credential->account_ref = opaque;
    source.reading.credential->revision_ref = opaque;
    source.reading.login_attempt_ref = opaque;
    source.current = *source.reading.credential;
    source.accepted_source = source.current;
    source.accepted_attempt = opaque;
    native::Controls control(native::Failure::none, {}, vectors::h0);
    const auto result = successful([&] { return propose_password_upgrade({}, opaque, source); });
    if (!result)
        return;
    EXPECT_EQ(result->data().outcome, PasswordUpgradeOutcome::replacement_ready);
    ASSERT_TRUE(result->data().replacement);
    EXPECT_EQ(result->data().replacement->account_ref, opaque);
    EXPECT_EQ(result->data().replacement->expected_revision_ref, opaque);
    EXPECT_EQ(result->data().replacement->login_attempt_ref, opaque);
    EXPECT_TRUE(source.all_proofs_belong);
    EXPECT_TRUE(source.all_proofs_match_read);
    exact_protected_input();
}

TEST(PasswordUpgrade, ProposalOwnsItsSourceAfterCallerAndPortDestruction) {
    std::optional<PasswordUpgradeResult> result;
    {
        LoginSource source;
        std::string requested(account);
        native::Controls control(native::Failure::none, {}, vectors::h0);
        result = successful([&] { return propose_password_upgrade({}, requested, source); });
        requested.assign(256, 'X');
        source.reading.credential->encoded_verifier.assign(256, 'Y');
        source.reading.login_attempt_ref.assign(256, 'Z');
        cleaned();
    }
    if (!result)
        return;
    ASSERT_TRUE(result->data().replacement);
    const auto &replacement = *result->data().replacement;
    EXPECT_EQ(replacement.account_ref, account);
    EXPECT_EQ(replacement.expected_revision_ref, "rev-A");
    EXPECT_EQ(replacement.expected_encoded_verifier, vectors::h0);
    EXPECT_EQ(replacement.login_attempt_ref, "attempt-A");
    bool bounded = false;
    bounded_record(replacement.replacement_encoded_verifier, "$argon2id$v=19$m=65536,t=3,p=1$",
                   bounded);
    if (!bounded) {
        return;
    }
    real_record(replacement.replacement_encoded_verifier, {});
}

TEST(PasswordUpgrade, ProposalCarriesFullCasSourceAndNeverRestoresAfterConcurrentReset) {
    LoginSource source;
    native::Controls control(native::Failure::none, {}, vectors::h0);
    const auto result = successful([&] { return propose_password_upgrade({}, account, source); });
    if (!result)
        return;
    ASSERT_TRUE(result->data().replacement);
    source.current.revision_ref = "rev-after-return";
    source.current.encoded_verifier = "newer credential after return";
    source.login_accepted = false;
    const auto &proposal = *result->data().replacement;
    EXPECT_EQ(proposal.expected_revision_ref, "rev-A");
    EXPECT_EQ(proposal.expected_encoded_verifier, vectors::h0);
    EXPECT_EQ(proposal.account_ref, account);
    EXPECT_EQ(proposal.login_attempt_ref, "attempt-A");
    EXPECT_EQ(source.current.revision_ref, "rev-after-return");
    EXPECT_EQ(source.current.encoded_verifier, "newer credential after return");
    EXPECT_FALSE(source.login_accepted);
    cleaned(); // actual conditional-save/permission transaction remains an owning integration gate
}

TEST(PasswordSecretFailures, InitializationRefusalHasNoAllocationOrCryptoForAllEntrypoints) {
    for (auto operation : operations) {
        SCOPED_TRACE(static_cast<int>(operation));
        LoginSource source;
        native::Controls control(native::Failure::initialization, {}, vectors::h0);
        failure(PasswordHashingError::runtime_unavailable,
                [&] { call_raw(operation, {}, {}, source); });
        EXPECT_GE(native::snapshot().initialization_calls, 1U);
        EXPECT_EQ(native::snapshot().allocation_attempts, 0U);
        EXPECT_EQ(native::snapshot().verification_calls, 0U);
        EXPECT_EQ(native::snapshot().hash_calls, 0U);
        EXPECT_EQ(source.checks, 0U);
        cleaned();
    }
}

TEST(PasswordSecretFailures, GuardedAllocationAndExplicitLockFailuresNeverReachCrypto) {
    for (auto mode : {native::Failure::guarded_allocation, native::Failure::explicit_lock}) {
        SCOPED_TRACE(static_cast<int>(mode));
        for (auto operation : operations) {
            SCOPED_TRACE(static_cast<int>(operation));
            LoginSource source;
            native::Controls control(mode, vectors::with_nul, vectors::h0);
            failure(PasswordHashingError::secret_memory_unavailable,
                    [&] { call_raw(operation, vectors::with_nul, {}, source); });
            EXPECT_EQ(native::snapshot().verification_calls, 0U);
            EXPECT_EQ(native::snapshot().hash_calls, 0U);
            EXPECT_GE(native::snapshot().allocation_attempts, 1U);
            if (mode == native::Failure::explicit_lock) {
                EXPECT_GE(native::snapshot().locks, 1U);
            }
            EXPECT_FALSE(native::snapshot().password_copied_before_lock);
            EXPECT_EQ(source.checks, 0U);
            cleaned();
        }
    }
}

TEST(PasswordSecretFailures, StandaloneNativeCreationFailureAndBadOutputsHaveFixedTypedErrors) {
    for (auto mode : {native::Failure::hashing, native::Failure::unterminated_hash_output,
                      native::Failure::wrong_hash_profile}) {
        SCOPED_TRACE(static_cast<int>(mode));
        native::Controls control(mode, vectors::with_nul);
        failure(mode == native::Failure::hashing ? PasswordHashingError::hash_unavailable
                                                 : PasswordHashingError::native_output_invalid,
                [&] { return create_password_hash(vectors::with_nul); });
        EXPECT_EQ(native::snapshot().hash_calls, 1U);
        exact_protected_input();
    }
}
