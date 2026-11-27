// SPDX-License-Identifier: GPL-3.0-or-later
// SESSION-010 S3 (#893): native ingress bound to the reference GET path, contract r2
// rows A-D, I09, I12, I18-I21 and decision 588.10. Expected values come from the r2
// text and its trace (build/SESSION-010-S3/contract-trace.md), never from the SUT.
// One case per process (discovery); synthetic credential Z on loopback only.
#include "ingress_fixture.hpp"

namespace session_ingress_test {
namespace {
using fx::Literal;
using Failure = AccountTransportFailure;

constexpr auto browser = "GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\n"
                         "Cookie: __Host-subathon_session="
                         "0000000000000000000000000000000000000000000000000000000000000000\r\n"
                         "X-Subathon-CSRF: 1\r\n\r\n"sv;
static_assert(browser.size() == 179 && fx::desktop.size() == 149);

// Literal with `extra` header lines inserted before the terminator, then `body`.
std::string with(std::string_view raw, std::string_view extra, std::string_view body = {}) {
    std::string out(raw.substr(0, raw.size() - 2));
    return out.append(extra).append("\r\n").append(body);
}
void send(Ingress &f, std::string_view raw, unsigned client = 0) {
    f.connect(client);
    f.client(client).send(raw);
}
bool delivered(Ingress &f, unsigned requests = 1) {
    return f.wait([&] { return f.requests >= requests || f.removed_count > 0; });
}
void expect_provenance(const si::Handoff &h, std::size_t fields, std::size_t projection,
                       std::string_view generation = "g1", bh::U64 sequence = 1) {
    ASSERT_NE(h.provenance.pin, nullptr);
    EXPECT_EQ(h.provenance.pin->reference, generation);
    EXPECT_EQ(h.provenance.listener, 1U);
    EXPECT_NE(h.provenance.connection, 0U);
    EXPECT_EQ(h.provenance.sequence, sequence);
    EXPECT_STREQ(h.provenance.peer_address.data(), "127.0.0.1");
    EXPECT_NE(h.provenance.peer_port, 0U);
    EXPECT_EQ(h.provenance.fields, fields);
    EXPECT_EQ(h.provenance.projection, projection);
}
void expect_prepared(const si::Handoff &h, AccountTransportKind kind, std::string_view ref) {
    const auto *p = prepared(h);
    ASSERT_NE(p, nullptr) << "no PreparedTransportSession handoff";
    EXPECT_TRUE(std::equal(p->digest.begin(), p->digest.end(), digest_z.begin()));
    EXPECT_EQ(p->transport, kind);
    EXPECT_EQ(p->policy_revision_ref, ref);
}
// Unavailable as a typed handoff, or a safe close before any request callback.
bool unavailable_or_closed(Ingress &f) {
    return (f.handoffs.size() == 1 && failure(f.handoffs[0]) == Failure::ingress_unavailable) ||
           (f.requests == 0 && f.removed_count > 0);
}
// After client 0 ends: a fresh admitted request on client 1 must still prepare.
void live_control(Ingress &f, std::string_view ref = "g1") {
    (void)::shutdown(f.client(0).get(), SHUT_WR);
    (void)f.through_eof(0);
    ASSERT_TRUE(f.wait([&] { return f.removed_count >= 1; })) << "client 0 not removed";
    const auto before = f.handoffs.size();
    const auto requests = f.requests;
    const auto crypto = crypto_calls.load();
    send(f, upstream(fx::desktop), 1);
    ASSERT_TRUE(f.wait([&] { return f.requests > requests; })) << "control not delivered";
    ASSERT_EQ(f.handoffs.size(), before + 1) << "positive control produced no handoff";
    expect_prepared(f.handoffs.back(), AccountTransportKind::desktop_bearer, ref);
    EXPECT_EQ(crypto_calls.load(), crypto + 1);
}

struct Case {
    const char *name;
    std::string raw;
    std::size_t fields;
    std::optional<Failure> expected; // nullopt: prepares
    AccountTransportKind kind = AccountTransportKind::desktop_bearer;
};
constexpr auto by_name = [](const auto &param) { return std::string(param.param.name); };

// Rows A-D direct (r2 lines 107-108): literal bytes are captured, then unavailable,
// since the proxied snapshot's single-hop metadata vector is absent.
class DirectLiteral : public ::testing::TestWithParam<Case> {};
TEST_P(DirectLiteral, CapturedThenUnavailableWithoutCrypto) {
    const auto &c = GetParam();
    Ingress f;
    send(f, c.raw);
    ASSERT_TRUE(delivered(f));
    ASSERT_EQ(f.handoffs.size(), 1U) << "no handoff for the captured direct request";
    EXPECT_EQ(failure(f.handoffs[0]), Failure::ingress_unavailable);
    expect_provenance(f.handoffs[0], c.fields, c.raw.size());
    EXPECT_EQ(crypto_calls.load(), 0U);
}
INSTANTIATE_TEST_SUITE_P(R2, DirectLiteral,
                         ::testing::Values(Case{"A237", std::string(fx::a), 3, {}},
                                           Case{"B241", std::string(fx::b), 5, {}},
                                           Case{"C199", std::string(fx::c), 4, {}},
                                           Case{"D277", std::string(fx::d), 4, {}}),
                         by_name);

// Rows A-D upstream projection (+87 forwarding +16 Via) and the two reductions
// (r2 lines 107-109): semantic refusals without crypto, or digestZ under g1.
class UpstreamProjection : public ::testing::TestWithParam<Case> {};
TEST_P(UpstreamProjection, TransportOutcomeUnderPinnedSnapshot) {
    const auto &c = GetParam();
    Ingress f;
    send(f, c.raw);
    ASSERT_TRUE(delivered(f));
    ASSERT_EQ(f.handoffs.size(), 1U) << "no handoff for the captured upstream request";
    expect_provenance(f.handoffs[0], c.fields, c.raw.size());
    if (c.expected) {
        EXPECT_EQ(failure(f.handoffs[0]), c.expected);
        EXPECT_EQ(crypto_calls.load(), 0U);
    } else {
        expect_prepared(f.handoffs[0], c.kind, "g1");
        EXPECT_EQ(crypto_calls.load(), 1U);
    }
}
INSTANTIATE_TEST_SUITE_P(
    R2, UpstreamProjection,
    ::testing::Values(
        Case{"A340", upstream(fx::a), 7, Failure::authentication_required},
        Case{"B344", upstream(fx::b), 9, Failure::origin_or_csrf_rejected},
        Case{"C302", upstream(fx::c), 8, Failure::origin_or_csrf_rejected},
        Case{"D380", upstream(fx::d), 8, Failure::authentication_required},
        Case{"Desktop252", upstream(fx::desktop), 6, {}},
        Case{"Browser282", upstream(browser), 7, {}, AccountTransportKind::browser_cookie}),
    by_name);

// 588.10: header431 -> body413 -> aggregate431 -> syntax400 on the reference route,
// each carrying a valid Bearer Z; none is captured or prepared, and the binding
// still prepares a fresh request afterwards. Header = 3+23+12+H (trace §3).
std::string preflight(std::string_view length, std::size_t pad) {
    Literal in;
    in.add("GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\n"
           "Authorization: Bearer 0000000000000000000000000000000000000000000000000000000000000000"
           "\r\nContent-Length: ");
    in.add(length);
    in.add("\r\nBad Header: x\r\n");
    if (pad) {
        in.add("X-Pad: ");
        in.repeat('a', pad);
        in.add("\r\n");
    }
    in.add("\r\n");
    return std::string(in.view());
}
struct Refusal {
    const char *name;
    std::string raw;
    std::string_view response;
};
class NativeRefusal : public ::testing::TestWithParam<Refusal> {};
TEST_P(NativeRefusal, NoCaptureOrPreparationThenLiveControl) {
    const auto &c = GetParam();
    Ingress f;
    send(f, c.raw);
    EXPECT_EQ(f.through_eof(0), c.response);
    ASSERT_TRUE(f.wait([&] { return f.removed_count > 0; }));
    EXPECT_EQ(f.requests, 0U);
    EXPECT_TRUE(f.handoffs.empty());
    EXPECT_EQ(crypto_calls.load(), 0U);
    live_control(f);
}
std::string padded(std::size_t pad) {
    Literal in;
    in.add(fx::desktop.substr(0, 147));
    in.add("X-Pad: ");
    in.repeat('a', pad);
    in.add("\r\n\r\n");
    return std::string(in.view());
}
INSTANTIATE_TEST_SUITE_P(
    R2, NativeRefusal,
    ::testing::Values(Refusal{"Header4097BeforeBody413", preflight("4097", 3902), large_head},
                      Refusal{"Header4096ThenBody413", preflight("4097", 3901), large_body},
                      Refusal{"Body413BeforeSyntax400", preflight("4097", 0), large_body},
                      Refusal{"Aggregate431BeforeSyntax400", preflight("4096", 0), large_head},
                      Refusal{"Syntax400InsideBudgets", preflight("1", 0), bad},
                      Refusal{"I09Projection4097", padded(3939), large_head}),
    by_name);

// I09 control: base149 + X-Pad 9+3938 = 4096 is still captured (then unavailable).
TEST(Session010IngressS3, I09Projection4096IsCaptured) {
    const auto raw = padded(3938);
    ASSERT_EQ(raw.size(), 4096U);
    Ingress f;
    send(f, raw);
    ASSERT_TRUE(delivered(f));
    ASSERT_EQ(f.handoffs.size(), 1U);
    EXPECT_EQ(failure(f.handoffs[0]), Failure::ingress_unavailable);
    expect_provenance(f.handoffs[0], 3, 4096);
}

// I12 / I21: reference primitive and media refusals precede credential preparation.
struct Reference {
    const char *name;
    std::string raw;
    si::ReferenceRefusal expected;
};
class ReferenceCheck : public ::testing::TestWithParam<Reference> {};
TEST_P(ReferenceCheck, RefusedBeforePreparation) {
    const auto &c = GetParam();
    Ingress f;
    send(f, c.raw);
    ASSERT_TRUE(delivered(f));
    ASSERT_EQ(f.handoffs.size(), 1U) << "no reference refusal handoff";
    const auto *refusal = std::get_if<si::ReferenceRefusal>(&f.handoffs[0].outcome);
    ASSERT_NE(refusal, nullptr);
    EXPECT_EQ(*refusal, c.expected);
    EXPECT_EQ(crypto_calls.load(), 0U);
}
INSTANTIATE_TEST_SUITE_P(
    R2, ReferenceCheck,
    ::testing::Values(Reference{"I12GetBodyNull",
                                with(upstream(fx::desktop), "Content-Length: 4\r\n", "null"),
                                si::ReferenceRefusal::invalid_primitive},
                      Reference{"I21Gzip",
                                with(upstream(fx::desktop), "Content-Encoding: gzip\r\n"),
                                si::ReferenceRefusal::unsupported_media},
                      Reference{"I21RepeatedEncoding",
                                with(upstream(fx::desktop), "Content-Encoding: identity\r\n"
                                                            "Content-Encoding: identity\r\n"),
                                si::ReferenceRefusal::unsupported_media}),
    by_name);

// I12/I20: CL4 with `nul` then EOF completes no request; the scope unwinds cleanly.
TEST(Session010IngressS3, I12IncompleteBodyThenEofDispatchesNothing) {
    Ingress f;
    send(f, with(upstream(fx::desktop), "Content-Length: 4\r\n", "nul"));
    f.client(0).eof();
    ASSERT_TRUE(f.wait([&] { return f.removed_count > 0; }));
    EXPECT_EQ(f.requests, 0U);
    EXPECT_TRUE(f.handoffs.empty());
    EXPECT_EQ(crypto_calls.load(), 0U);
    live_control(f);
}

// I12/I20: two coalesced direct desktop requests (298 bytes) are scoped separately;
// the buffered successor pins the snapshot current at its own start fence.
TEST(Session010IngressS3, CoalescedSuccessorPinsItsOwnGeneration) {
    Ingress f;
    auto changed = reference_facts();
    changed.logging = 2;
    const auto g2 = f.next(changed);
    ASSERT_EQ(g2->reference, "g2");
    f.during_request = [&f, g2] { f.binding().publish(g2); };
    send(f, std::string(fx::desktop) + std::string(fx::desktop));
    ASSERT_TRUE(delivered(f, 2));
    ASSERT_EQ(f.handoffs.size(), 2U);
    EXPECT_EQ(failure(f.handoffs[0]), Failure::ingress_unavailable);
    expect_provenance(f.handoffs[0], 2, 149, "g1", 1);
    expect_provenance(f.handoffs[1], 2, 149, "g2", 2);
    EXPECT_EQ(f.handoffs[0].provenance.connection, f.handoffs[1].provenance.connection);
    EXPECT_EQ(failure(f.handoffs[1]), Failure::ingress_unavailable);
}

// Request-start pin (r2 ordering step 1): a publication after the first partial read
// does not change that request's snapshot; a fresh request uses the new one.
TEST(Session010IngressS3, PartialReadPinsBeforeLaterPublication) {
    Ingress f;
    const auto raw = upstream(fx::desktop);
    f.connect(0);
    f.client(0).send(std::string_view(raw).substr(0, 10));
    ASSERT_TRUE(f.wait([&] { return f.read_bytes >= 10; }));
    auto changed = reference_facts();
    changed.logging = 2;
    f.binding().publish(f.next(changed));
    f.client(0).send(std::string_view(raw).substr(10));
    ASSERT_TRUE(delivered(f));
    ASSERT_EQ(f.handoffs.size(), 1U);
    expect_prepared(f.handoffs[0], AccountTransportKind::desktop_bearer, "g1");
    live_control(f, "g2");
}

// I18 and r2 line 53: with no published or a suspended policy, a request reaching
// its start fence is unavailable or closed, with no crypto; publication reopens.
class ClosedAdmission : public ::testing::TestWithParam<bool> {};
TEST_P(ClosedAdmission, RefusedThenReopenedByPublication) {
    const bool suspended = GetParam();
    Ingress f(reference_facts(), suspended);
    if (suspended)
        f.binding().suspend();
    send(f, upstream(fx::desktop));
    ASSERT_TRUE(delivered(f));
    EXPECT_TRUE(unavailable_or_closed(f));
    EXPECT_EQ(crypto_calls.load(), 0U);
    f.binding().publish(f.next(reference_facts()));
    live_control(f, "g1");
}
INSTANTIATE_TEST_SUITE_P(I18, ClosedAdmission, ::testing::Values(false, true),
                         [](const auto &param) {
                             return std::string(param.param ? "Suspended" : "Unpublished");
                         });

// I18 / exact route binding: a snapshot whose listener or route table disagrees with
// the actual registration is unavailable or closed, with zero crypto.
class Mismatch : public ::testing::TestWithParam<int> {};
TEST_P(Mismatch, UnavailableWithoutCrypto) {
    auto facts = reference_facts();
    if (GetParam() == 0)
        facts.listener = 2;
    else
        facts.routes = {{std::string(target), "POST", AccountTransportPurpose::protected_access}};
    Ingress f(facts);
    send(f, upstream(fx::desktop));
    ASSERT_TRUE(delivered(f));
    EXPECT_TRUE(unavailable_or_closed(f));
    EXPECT_EQ(crypto_calls.load(), 0U);
}
INSTANTIATE_TEST_SUITE_P(I18, Mismatch, ::testing::Values(0, 1), [](const auto &param) {
    return std::string(param.param == 0 ? "Listener" : "RouteMethod");
});

// I19: a canary in an arbitrary field never reaches the bound logger or the wire,
// while the same request still prepares (real positive for the observed channels).
TEST(Session010IngressS3, I19CanaryAbsentFromLogAndWire) {
    Ingress f;
    send(f, with(upstream(fx::desktop), "X-Other: CNY-s11-7q9\r\n"));
    ASSERT_TRUE(delivered(f));
    ASSERT_EQ(f.handoffs.size(), 1U);
    expect_prepared(f.handoffs[0], AccountTransportKind::desktop_bearer, "g1");
    f.stop();
    EXPECT_TRUE(f.logs_clean);
    EXPECT_TRUE(f.wire_clean);
}
} // namespace
} // namespace session_ingress_test
