// SPDX-License-Identifier: GPL-3.0-or-later
#include "session_transport_candidate.hpp"

#include <gtest/gtest.h>

#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
using namespace subathon::server;
using Failure = AccountTransportFailure;
using Purpose = AccountTransportPurpose;
using Kind = AccountTransportKind;

const std::string token(64, '0');
const std::string cookie = "__Host-subathon_session=" + token;
const std::string bearer = "Bearer " + token;
constexpr std::string_view origin = "https://timer.example";
constexpr AccountHeaderView marker{"X-Subathon-CSRF", "1"};
constexpr AccountHeaderView origin_field{"Origin", origin};
constexpr AccountSessionDigest injected_digest{0x17, 0xc8, 0x9d};

struct Source final : AccountIngressSource {
    std::string_view method = "GET", configured = origin, policy = "p";
    std::vector<AccountHeaderView> fields;
    bool unavailable = false, throws = false;
    int reads = 0;

    AccountIngressRead read_current(const AccountTransportLimits &) override {
        ++reads;
        if (throws)
            throw std::runtime_error("synthetic source failure");
        if (unavailable)
            return AccountIngressUnavailable{};
        return AccountIngressView{method, configured, policy, fields};
    }
};

struct Supplier {
    PreparedAccountSession result = injected_digest;
    int calls = 0;
    bool throws = false;
    bool exact_token = false;

    static PreparedAccountSession prepare(void *state, std::string_view borrowed) {
        auto &self = *static_cast<Supplier *>(state);
        ++self.calls;
        self.exact_token = borrowed == token;
        if (self.throws)
            throw std::runtime_error("synthetic supplier failure");
        return self.result;
    }
};

struct Case {
    Source source;
    Supplier supplier;
    AccountTransportLimits limits;
    Purpose purpose = Purpose::protected_access;

    AccountTransportResult run() {
        return subathon::server::testing::prepare_transport(source, purpose, limits,
                                                            Supplier::prepare, &supplier);
    }
    void refuse(Failure expected, int reads = 1, int prepares = 0) {
        const auto result = run();
        ASSERT_EQ(result.index(), 2U);
        EXPECT_EQ(std::get<Failure>(result), expected);
        EXPECT_EQ(source.reads, reads);
        EXPECT_EQ(supplier.calls, prepares);
        if (prepares) {
            EXPECT_TRUE(supplier.exact_token);
        }
    }
    void success(Kind kind) {
        const auto result = run();
        ASSERT_EQ(result.index(), 0U);
        const auto &prepared = std::get<PreparedTransportSession>(result);
        EXPECT_EQ(prepared.digest, injected_digest);
        EXPECT_EQ(prepared.transport, kind);
        EXPECT_EQ(prepared.policy_revision_ref, source.policy);
        EXPECT_EQ(source.reads, 1);
        EXPECT_EQ(supplier.calls, 1);
        EXPECT_TRUE(supplier.exact_token);
    }
    void guard() {
        const auto result = run();
        ASSERT_EQ(result.index(), 1U);
        EXPECT_EQ(std::get<BrowserSignInGuard>(result).policy_revision_ref, source.policy);
        EXPECT_EQ(source.reads, 1);
        EXPECT_EQ(supplier.calls, 0);
    }
};

TEST(SessionTransportS1, ReadsOnlyAfterTrustedChecks) {
    Case zero;
    zero.limits.result_bytes = 0;
    zero.refuse(Failure::invalid_limits, 0);
    Case huge;
    huge.limits.result_bytes = std::numeric_limits<std::uint64_t>::max();
    huge.refuse(Failure::invalid_limits, 0);
    Case invalid;
    invalid.purpose = static_cast<Purpose>(255);
    invalid.refuse(Failure::invalid_syntax, 0);
}

TEST(SessionTransportS1, SourceUnavailableOrThrow) {
    Case unavailable;
    unavailable.source.unavailable = true;
    unavailable.refuse(Failure::ingress_unavailable);
    Case throwing;
    throwing.source.throws = true;
    throwing.refuse(Failure::ingress_unavailable);
}

TEST(SessionTransportS1, MultiplicityAThroughD) {
    Case a;
    a.source.fields = {{"Authorization", bearer}, {"authorization", bearer}};
    a.refuse(Failure::authentication_required);
    Case b;
    b.source.fields = {{"Cookie", cookie}, marker, origin_field, {"origin", origin}};
    b.refuse(Failure::origin_or_csrf_rejected);
    Case c;
    c.source.fields = {{"Cookie", cookie}, marker, marker};
    c.refuse(Failure::origin_or_csrf_rejected);
    Case d;
    d.source.fields = {{"Cookie", cookie}, marker, {"Cookie", cookie}};
    d.refuse(Failure::authentication_required);
}

TEST(SessionTransportS1, RealSupplierControl) {
    Source source;
    source.policy = "g1";
    source.fields = {{"Authorization", bearer}};
    const auto result = prepare_account_transport(source, Purpose::protected_access);
    ASSERT_EQ(result.index(), 0U);
    const auto &prepared = std::get<PreparedTransportSession>(result);
    constexpr AccountSessionDigest expected{0xa2, 0x26, 0xd5, 0x44, 0xff, 0x92, 0xf3, 0x24,
                                            0x7d, 0xc7, 0x84, 0xe9, 0x49, 0xc8, 0x46, 0x99,
                                            0x9f, 0xde, 0x80, 0x1e, 0x45, 0x81, 0xc6, 0xbb,
                                            0x21, 0x9f, 0xde, 0x07, 0x05, 0x91, 0xd3, 0xea};
    EXPECT_EQ(prepared.digest, expected);
    EXPECT_EQ(prepared.transport, Kind::desktop_bearer);
    EXPECT_EQ(prepared.policy_revision_ref, "g1");
    EXPECT_EQ(source.reads, 1);
}

TEST(SessionTransportS1, GuardAndCredentialForms) {
    const std::string mixed = "bEaReR  " + token;
    const std::string tab = "Bearer\t" + token;
    const std::string upper = "Bearer " + std::string(64, 'F');
    const std::string combined = bearer + ", " + bearer;
    const std::string quoted = "__Host-subathon_session=\"" + token + "\"";
    for (auto malformed :
         {std::string_view{}, std::string_view{"Basic example"}, std::string_view{tab},
          std::string_view{upper}, std::string_view{combined}}) {
        SCOPED_TRACE(malformed.size());
        Case bad;
        bad.source.fields = {{"Authorization", malformed}};
        bad.refuse(Failure::authentication_required);
    }
    for (auto malformed :
         {"", "null", "https://timer.example.evil", "https://timer.example:444",
          "https://timer.example, https://timer.example", " https://timer.example"}) {
        SCOPED_TRACE(malformed);
        Case bad;
        bad.source.fields = {{"Cookie", cookie}, marker, {"Origin", malformed}};
        bad.refuse(Failure::origin_or_csrf_rejected);
    }
    for (auto malformed : {"", "2", "1, 1"}) {
        SCOPED_TRACE(malformed);
        Case bad;
        bad.source.fields = {{"Cookie", cookie}, {"X-Subathon-CSRF", malformed}};
        bad.refuse(Failure::origin_or_csrf_rejected);
        Case desktop;
        desktop.source.fields = {{"Authorization", bearer}, {"X-Subathon-CSRF", malformed}};
        desktop.refuse(Failure::origin_or_csrf_rejected);
    }
    Case cookie_only;
    cookie_only.source.fields = {{"Cookie", cookie}};
    cookie_only.refuse(Failure::origin_or_csrf_rejected);
    Case marker_only;
    marker_only.source.fields = {marker};
    marker_only.refuse(Failure::authentication_required);
    Case quote;
    quote.source.fields = {{"Cookie", quoted}, marker};
    quote.refuse(Failure::authentication_required);
    Case control;
    control.source.fields = {{"Cookie", cookie}, {"X-Subathon-CSRF", "1\x01"}, {"Origin", "null"}};
    control.refuse(Failure::invalid_syntax);
    Case bad_name;
    bad_name.source.fields = {{"Authorization", bearer}, {"bad name", "x"}, origin_field};
    bad_name.refuse(Failure::invalid_syntax);
    Case desktop;
    desktop.source.fields = {{"authorization", mixed}};
    desktop.success(Kind::desktop_bearer);
    Case desktop_origin;
    desktop_origin.source.fields = {{"Authorization", bearer}, origin_field};
    desktop_origin.refuse(Failure::origin_or_csrf_rejected);
    Case desktop_marker;
    desktop_marker.source.fields = {{"Authorization", bearer}, marker};
    desktop_marker.refuse(Failure::origin_or_csrf_rejected);
    Case get;
    get.source.fields = {{"Cookie", cookie}, {"X-Subathon-CSRF", " \t1\t "}};
    get.success(Kind::browser_cookie);
    Case post;
    post.source.method = "POST";
    post.source.fields = {{"Cookie", cookie}, marker, origin_field};
    post.success(Kind::browser_cookie);
    Case no_origin;
    no_origin.source.method = "POST";
    no_origin.source.fields = {{"Cookie", cookie}, marker};
    no_origin.refuse(Failure::origin_or_csrf_rejected);
}

TEST(SessionTransportS1, SyntacticCookiePairsAndLimits) {
    for (auto malformed : {"", "a=1;;b=2"}) {
        Case bad;
        bad.source.fields = {{"Cookie", malformed}};
        bad.refuse(Failure::invalid_syntax);
    }
    const std::string doubled = cookie + "; " + cookie;
    Case duplicate;
    duplicate.source.fields = {{"Cookie", doubled}, marker};
    duplicate.refuse(Failure::authentication_required);
    std::string pairs = cookie;
    for (int i = 0; i < 63; ++i)
        pairs += "; unrelated=1";
    Case exactly;
    exactly.source.fields = {{"Cookie", pairs}, marker};
    exactly.success(Kind::browser_cookie);
    const std::string extra = pairs + "; unrelated=1";
    Case excess;
    excess.source.fields = {{"Cookie", extra}, marker};
    excess.refuse(Failure::resource_limit);
    Case across_fields;
    across_fields.source.fields = {{"Cookie", pairs}, {"Cookie", "unrelated=1"}, marker};
    across_fields.refuse(Failure::resource_limit);
    const std::string malformed_first = "a=1;;" + extra;
    Case syntax_first;
    syntax_first.source.fields = {{"Cookie", malformed_first}, marker};
    syntax_first.refuse(Failure::invalid_syntax);
}

TEST(SessionTransportS1, SignInGuardAndProjection) {
    const std::string quoted = "__Host-subathon_session=\"" + token + "\"";
    for (const auto with_cookie : {false, true}) {
        Case accepted;
        accepted.purpose = Purpose::browser_sign_in_guard;
        accepted.source.method = "POST";
        accepted.source.fields = {marker, origin_field};
        if (with_cookie)
            accepted.source.fields.push_back({"Cookie", cookie});
        accepted.guard();
    }
    Case get;
    get.purpose = Purpose::browser_sign_in_guard;
    get.source.fields = {marker, origin_field};
    get.refuse(Failure::authentication_required);
    Case auth;
    auth.purpose = Purpose::browser_sign_in_guard;
    auth.source.method = "POST";
    auth.source.fields = {marker, origin_field, {"Authorization", bearer}};
    auth.refuse(Failure::authentication_required);
    Case missing_origin;
    missing_origin.purpose = Purpose::browser_sign_in_guard;
    missing_origin.source.method = "POST";
    missing_origin.source.fields = {marker};
    missing_origin.refuse(Failure::origin_or_csrf_rejected);
    Case malformed;
    malformed.purpose = Purpose::browser_sign_in_guard;
    malformed.source.method = "POST";
    malformed.source.fields = {marker, origin_field, {"Cookie", quoted}};
    malformed.refuse(Failure::authentication_required);
    for (const auto with_cookie : {false, true}) {
        Case capped;
        capped.purpose = Purpose::browser_sign_in_guard;
        capped.source.method = "POST";
        capped.source.policy = "g1";
        capped.limits.result_bytes = 1;
        capped.source.fields = {marker, origin_field};
        if (with_cookie)
            capped.source.fields.push_back({"Cookie", quoted});
        capped.refuse(Failure::resource_limit);
    }
}

TEST(SessionTransportS1, SupplierResultMapping) {
    const std::pair<SessionFailure, Failure> outcomes[]{
        {SessionFailure::unavailable, Failure::crypto_unavailable},
        {SessionFailure::resource_limit, Failure::authentication_required},
        {SessionFailure::invalid_input, Failure::authentication_required},
        {SessionFailure::invalid_limits, Failure::ingress_unavailable},
        {SessionFailure::invalid_stored_data, Failure::ingress_unavailable},
        {SessionFailure::missing, Failure::ingress_unavailable},
        {SessionFailure::revoked, Failure::ingress_unavailable},
        {SessionFailure::expired, Failure::ingress_unavailable},
        {SessionFailure::fresh_authentication_required, Failure::ingress_unavailable},
    };
    for (const auto &[supplier_failure, expected] : outcomes) {
        SCOPED_TRACE(static_cast<int>(supplier_failure));
        Case test;
        test.source.fields = {{"Authorization", bearer}};
        test.supplier.result = supplier_failure;
        test.refuse(expected, 1, 1);
    }
    Case throwing;
    throwing.source.fields = {{"Authorization", bearer}};
    throwing.supplier.throws = true;
    throwing.refuse(Failure::ingress_unavailable, 1, 1);
}

TEST(SessionTransportS1, TrustedFactsAndSizePrecedeSyntax) {
    for (auto malformed :
         {"", "http://timer.example", "https://timer.example:443", "https://timer.example/",
          "https://timer.example/path", "https://timer.example?q", "https://timer.example#fragment",
          "https://name@timer.example", "HTTPS://timer.example", "https://Timer.example",
          "https://tïmer.example"}) {
        SCOPED_TRACE(malformed);
        Case test;
        test.source.configured = malformed;
        test.refuse(Failure::ingress_unavailable);
    }
    for (auto method : {"", "BAD METHOD"}) {
        Case test;
        test.source.method = method;
        test.refuse(Failure::ingress_unavailable);
    }
    Case missing_policy;
    missing_policy.source.policy = {};
    missing_policy.refuse(Failure::ingress_unavailable);
    const std::string malformed_origin(1024, 'x');
    Case shape;
    shape.source.configured = malformed_origin;
    shape.source.fields = {{"Cookie", "a=1;;b=2"}};
    shape.refuse(Failure::ingress_unavailable);
    const std::string oversized_origin(1025, 'x');
    Case over;
    over.source.configured = oversized_origin;
    over.refuse(Failure::resource_limit);
    Case header_first;
    header_first.source.configured = {};
    header_first.source.fields = {{"Cookie", "a=1;;b=2"}};
    header_first.limits.header_bytes = 2;
    header_first.refuse(Failure::resource_limit);
    Case exact_method;
    exact_method.source.method = "ABCDEFGHIJKLMNOP";
    exact_method.source.fields = {{"Authorization", bearer}};
    exact_method.success(Kind::desktop_bearer);
    Case over_method;
    over_method.source.method = "ABCDEFGHIJKLMNOPQ";
    over_method.source.fields = {{"Authorization", bearer}};
    over_method.refuse(Failure::resource_limit);
}

TEST(SessionTransportS1, HeaderAndFieldEquality) {
    const std::initializer_list<AccountHeaderView> inputs[]{
        {{"Cookie", cookie}, marker},
        {{"Cookie", cookie}, marker, origin_field},
        {{"Authorization", bearer}},
    };
    constexpr std::uint64_t bytes[]{120, 151, 90};
    for (std::size_t i = 0; i < 3; ++i) {
        Case exact;
        exact.source.fields = inputs[i];
        exact.limits.header_bytes = bytes[i];
        exact.success(i == 2 ? Kind::desktop_bearer : Kind::browser_cookie);
        Case one_less;
        one_less.source.fields = inputs[i];
        one_less.limits.header_bytes = bytes[i] - 1;
        one_less.refuse(Failure::resource_limit);
    }
    Case exact_origin;
    exact_origin.source.fields = {{"Cookie", cookie}, marker, origin_field};
    exact_origin.limits.origin_bytes = origin.size();
    exact_origin.success(Kind::browser_cookie);
    Case over_origin;
    over_origin.source.fields = {{"Cookie", cookie}, marker, origin_field};
    over_origin.limits.origin_bytes = origin.size() - 1;
    over_origin.refuse(Failure::resource_limit);
    Case exact_count;
    exact_count.source.fields.assign(63, {"X-A", "a"});
    exact_count.source.fields.push_back({"Authorization", bearer});
    auto fields = exact_count.source.fields;
    exact_count.success(Kind::desktop_bearer);
    Case over_count;
    over_count.source.fields = fields;
    over_count.source.fields.push_back({"X-A", "a"});
    over_count.refuse(Failure::resource_limit);
    const std::string pad(8191, 'a');
    Case exact_field;
    exact_field.source.fields = {{"Authorization", bearer}, {"X", pad}};
    exact_field.success(Kind::desktop_bearer);
    Case over_field;
    over_field.source.fields = {{"Authorization", bearer}, {"XX", pad}};
    over_field.refuse(Failure::resource_limit);
}

TEST(SessionTransportS1, CredentialAndProjectionOrder) {
    const std::string quoted = "__Host-subathon_session=\"" + token + "\"";
    Case syntax;
    syntax.source.fields = {
        {"Cookie", "a=1;;b=2"}, {"Authorization", bearer}, {"Authorization", bearer}};
    syntax.refuse(Failure::invalid_syntax);
    Case credential;
    credential.source.fields = {{"Cookie", cookie}, {"Authorization", ""}, {"Origin", "null"}};
    credential.refuse(Failure::authentication_required);
    Case basic;
    basic.source.fields = {{"Cookie", cookie}, marker, {"Authorization", "Basic example"}};
    basic.refuse(Failure::authentication_required);
    Case bearer_and_cookie;
    bearer_and_cookie.source.fields = {{"Cookie", cookie}, marker, {"Authorization", bearer}};
    bearer_and_cookie.refuse(Failure::authentication_required);
    Case source_limit;
    source_limit.source.policy = "g1";
    source_limit.source.fields = {{"Cookie", "a=1;;b=2"}};
    source_limit.limits.policy_reference_bytes = 1;
    source_limit.refuse(Failure::resource_limit);
    Case result_limit;
    result_limit.source.policy = "g1";
    result_limit.source.fields = {{"Cookie", "a=1;;b=2"}};
    result_limit.limits.result_bytes = 1;
    result_limit.refuse(Failure::invalid_syntax);
    Case guards;
    guards.source.policy = "g1";
    guards.limits.result_bytes = 1;
    guards.source.fields = {{"Cookie", cookie}};
    guards.refuse(Failure::origin_or_csrf_rejected);
    Case projection;
    projection.source.policy = "g1";
    projection.limits.result_bytes = 1;
    projection.source.fields = {{"Cookie", quoted}, marker};
    projection.refuse(Failure::resource_limit);
    for (auto kind : {Kind::desktop_bearer, Kind::browser_cookie}) {
        Case exact;
        exact.source.fields = kind == Kind::desktop_bearer
                                  ? std::vector<AccountHeaderView>{{"Authorization", bearer}}
                                  : std::vector<AccountHeaderView>{{"Cookie", cookie}, marker};
        exact.limits.policy_reference_bytes = 1;
        exact.limits.result_bytes = 1;
        Case over;
        over.source.fields = exact.source.fields;
        over.source.policy = "g1";
        over.limits.result_bytes = 1;
        exact.success(kind);
        over.refuse(Failure::resource_limit);
    }
}
} // namespace
