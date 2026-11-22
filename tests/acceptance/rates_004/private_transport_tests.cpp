// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

namespace {
using namespace rates_004_fixture;

// Direct transport tests use a declared synthetic admission. Owner tests obtain their admission
// through the adapter, so the fixture cannot decide whether to send or normalize a result.
fx::Admission usage_admission() {
    const SyntheticOwner owner;
    return {{owner.setup.selection, "usage", 17},
            usage("usage"),
            "GET",
            "https://openexchangerates.org/api/usage.json",
            "Authorization",
            false,
            16 * 1024,
            256 * 1024,
            256 * 1024,
            4s};
}

wire::Handoff sent_usage() {
    return {usage_admission(), {"Authorization", "Token " + synthetic_app_id()}};
}

TEST(OpenExchangeRatesPrivateTransport, HandoffRequiresExactSelectedCredentialAndHeaderOnly) {
    const auto admission = usage_admission();
    wire::Credential credential{admission.identity.selection, synthetic_app_id()};
    const auto valid = wire::make_handoff(admission, credential);
    ASSERT_TRUE(valid);
    EXPECT_EQ(valid->admission.identity, admission.identity);
    EXPECT_EQ(valid->authorization.first, "Authorization");
    EXPECT_TRUE(valid->authorization.second == "Token " + synthetic_app_id());
    expect_rate_route(valid->admission, "/api/usage.json");

    for (const auto invalid :
         {"short", "aaaaaaaaaaaaaaaa111111111111111Z", "aaaaaaaaaaaaaaaa111111111111111 "}) {
        credential.app_id = invalid;
        EXPECT_FALSE(wire::make_handoff(admission, credential));
    }
    credential.app_id = synthetic_app_id();
    credential.selection.credential_revision = "other-credential";
    EXPECT_FALSE(wire::make_handoff(admission, credential));
}

TEST(OpenExchangeRatesPrivateTransport, O6EchoFactComparesExactBytesAndCarriesOnlySafeFrame) {
    struct EchoCase {
        std::string_view name;
        std::optional<std::string> json_value;
        fx::EchoMatch expected;
    };
    const auto app_id = synthetic_app_id();
    auto altered = app_id;
    altered.front() = 'A';
    const EchoCase cases[] = {
        {"exact", "\"" + app_id + "\"", fx::EchoMatch::match},
        {"different ASCII case", "\"" + altered + "\"", fx::EchoMatch::mismatch},
        {"absent", {}, fx::EchoMatch::unknown},
        {"wrong type", "17", fx::EchoMatch::unknown},
        {"invalid shape", "\"not-an-app-id\"", fx::EchoMatch::unknown},
    };

    for (const auto &row : cases) {
        SCOPED_TRACE(row.name);
        const auto handoff = sent_usage();
        auto raw = finite(1000, 7, clock(10, january));
        if (row.json_value)
            add_echo(raw, *row.json_value);
        raw.headers = {{"Date", "Thu, 01 Jan 2026 00:00:00 GMT"}};
        // Give independently observable wire and decoded sizes.
        raw.wire_bytes = 61;
        const auto original_size = raw.decoded_bytes;
        const auto sanitized = wire::sanitize_reply(handoff, std::move(raw));
        EXPECT_EQ(sanitized.identity, handoff.admission.identity);
        EXPECT_EQ(sanitized.dispatch, fx::Dispatch::sent);
        const auto *frame = std::get_if<fx::SanitizedFrame>(&sanitized.payload);
        ASSERT_NE(frame, nullptr);
        ASSERT_TRUE(frame->echo);
        EXPECT_EQ(frame->echo->identity, handoff.admission.identity);
        EXPECT_EQ(frame->echo->match, row.expected);
        EXPECT_EQ(frame->status, 200);
        EXPECT_EQ(frame->headers, (fx::Headers{{"Date", "Thu, 01 Jan 2026 00:00:00 GMT"}}));
        EXPECT_EQ(frame->wire_bytes, 61U);
        EXPECT_EQ(frame->decoded_bytes, original_size);
        EXPECT_EQ(frame->receipt, clock(10, january));
        EXPECT_TRUE(frame->body.find("app_id") == std::string::npos &&
                    frame->body.find(app_id) == std::string::npos &&
                    frame->body.find(altered) == std::string::npos &&
                    frame->body.find("not-an-app-id") == std::string::npos);
        EXPECT_TRUE(frame->body.find("requests_quota") != std::string::npos &&
                    frame->body.find("requests_remaining") != std::string::npos);
    }
}

TEST(OpenExchangeRatesPrivateTransport, OriginalDuplicateEchoAndBodyBoundsRefuseBeforeProjection) {
    auto duplicate = finite(1000, 7, {});
    const auto echo = "\"" + synthetic_app_id() + "\"";
    add_echo(duplicate, echo);
    // Both member spellings decode to app_id. Neither can be stripped before uniqueness checks.
    const std::string second = ",\"app_\\u0069d\":" + echo;
    duplicate.body.insert(duplicate.body.size() - 2, second);
    duplicate.decoded_bytes += second.size();
    duplicate.wire_bytes += second.size();
    const auto rejected = wire::sanitize_reply(sent_usage(), std::move(duplicate));
    EXPECT_EQ(rejected.dispatch, fx::Dispatch::sent);
    const auto *invalid = std::get_if<fx::TransportFailure>(&rejected.payload);
    ASSERT_NE(invalid, nullptr);
    EXPECT_EQ(invalid->kind, fx::TransportFailureKind::invalid_document);

    for (bool wire_exceeded : {false, true}) {
        SCOPED_TRACE(wire_exceeded);
        auto oversized = finite(1000, 7, {});
        add_echo(oversized, echo);
        if (wire_exceeded) {
            oversized.wire_bytes = 256 * 1024 + 1;
        } else {
            oversized.body.append(256 * 1024 + 1 - oversized.body.size(), ' ');
            oversized.decoded_bytes = oversized.body.size();
        }
        const auto refused = wire::sanitize_reply(sent_usage(), std::move(oversized));
        const auto *failure = std::get_if<fx::TransportFailure>(&refused.payload);
        ASSERT_NE(failure, nullptr);
        EXPECT_EQ(failure->kind, fx::TransportFailureKind::resource_limit);
    }
}

TEST(OpenExchangeRatesPrivateTransport, EchoFromAnotherAdmissionOrSanitizedBodyCannotQualifyUsage) {
    for (bool wrong_admission : {false, true}) {
        SCOPED_TRACE(wrong_admission);
        Fixture f;
        f.select();
        const auto first = f.start(usage("first"));
        auto first_raw = finite(1000, 7, clock(100));
        add_echo(first_raw, "\"" + synthetic_app_id() + "\"");
        const auto first_reply =
            wire::sanitize_reply(f.transport.sent(first), std::move(first_raw));
        const auto *first_frame = std::get_if<fx::SanitizedFrame>(&first_reply.payload);
        ASSERT_NE(first_frame, nullptr);
        ASSERT_TRUE(first_frame->echo);
        ASSERT_TRUE(std::holds_alternative<fx::UsageSample>(f.adapter.finish(first_reply).outcome));

        f.owner.intent.now = clock(200);
        auto second = f.start(usage("second"));
        auto raw = finite(1000, 0, clock(200));
        auto reply = wire::sanitize_reply(f.transport.sent(second), raw);
        auto *frame = std::get_if<fx::SanitizedFrame>(&reply.payload);
        ASSERT_NE(frame, nullptr);
        if (wrong_admission) {
            frame->echo = first_frame->echo;
        } else {
            // An untrusted body field cannot replace the absent private fact.
            frame->echo.reset();
            add_echo(raw, "\"" + synthetic_app_id() + "\"");
            frame->body = std::move(raw.body);
            frame->wire_bytes = raw.wire_bytes;
            frame->decoded_bytes = raw.decoded_bytes;
        }
        auto refused = f.adapter.finish(std::move(reply));
        expect_refusal(refused, fx::FailureKind::credential_echo_unverified);
        EXPECT_EQ(refused.identity, second.admission.identity);
        EXPECT_EQ(refused.dispatch, fx::Dispatch::sent);
        const auto view = f.view();
        EXPECT_TRUE(view.holds.empty());
        ASSERT_TRUE(view.last_usable_quota);
        expect_finite(*view.last_usable_quota, 1000, 7);
        EXPECT_EQ(view.quota_age_ms, 100);
        expect_counts(view.total, 2, 0);
    }
}

} // namespace
