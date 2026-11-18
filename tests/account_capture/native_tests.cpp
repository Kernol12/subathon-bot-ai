// SPDX-License-Identifier: GPL-3.0-or-later
// SOURCE ONLY. Per-case processes and exact new-consumer admission are root gates.
#include "bounded_literals.hpp"
#include "native_fixture.hpp"
#include "secret_scanner.hpp"
namespace account_capture_test {
namespace {
constexpr auto diag =
    "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nConnection: keep-alive\r\n\r\ndiag"sv;
constexpr auto bad = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"sv;
constexpr auto large_body =
    "HTTP/1.1 413 Payload Too Large\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"sv;
constexpr auto large_head =
    "HTTP/1.1 431 Request Header Fields Too Large\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"sv;
void wire(Socket &socket, std::string_view expected, bool eof = false) {
    std::array<char, 65664> bytes{};
    const auto n =
        socket.read(eof ? std::span(bytes) : std::span(bytes).first(expected.size()), eof);
    ASSERT_EQ(n, expected.size());
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), bytes.begin())); // no secret printing
    SecretScanner scanner;
    scanner.feed({bytes.data(), n});
    EXPECT_TRUE(scanner.clean());
}
void inspect_reads(Live &f) {
    const auto before = f.r.turns.load();
    f.post();
    f.r.wait([&] { return f.r.turns.load() > before; });
    std::lock_guard lock(f.r.mutex);
    bounded_reads(f.r.observation);
}
void capture(std::string_view raw, std::span<const Field> fields,
             LeaseAction action = LeaseAction::exact, std::size_t split = 0) {
    Live f(true, {fields, "GET", "/api/reference/v1/timer", {}, action});
    f.connect();
    if (split) {
        f.client.send(raw.substr(0, split));
        // Force the first fragment to be observed before sending the rest, without
        // assuming write boundaries equal packet boundaries.
        f.r.wait([&] { return f.r.read_bytes.load() >= split; });
        EXPECT_EQ(f.r.captures, 0U);
        f.client.send(raw.substr(split));
    } else
        f.client.send(raw);
    wire(f.client, diag);
    f.r.wait([&] { return f.r.captures.load() == 1 && f.r.requests.load() == 1; });
    inspect_reads(f);
}
TEST(AccountCaptureNative, Desktop149) { capture(desktop, fields_desktop); }
TEST(AccountCaptureNative, AuthorizationOccurrencesA237) { capture(a, fields_a); }
TEST(AccountCaptureNative, OriginOccurrencesB241) { capture(b, fields_b); }
TEST(AccountCaptureNative, MarkerOccurrencesC199) { capture(c, fields_c); }
TEST(AccountCaptureNative, CookieOccurrencesD277) { capture(d, fields_d); }
TEST(AccountCaptureNative, BytesLiveThroughOuterCallback) {
    capture(desktop, fields_desktop, LeaseAction::lifetime);
}
TEST(AccountCaptureNative, NestedVisitRefused) {
    capture(desktop, fields_desktop, LeaseAction::nested);
}
TEST(AccountCaptureNative, MovesTransferOnlyOneLease) {
    capture(desktop, fields_desktop, LeaseAction::move);
}
TEST(AccountCaptureNative, ExclusiveForeignThreadVisitRefused) {
    capture(desktop, fields_desktop, LeaseAction::foreign);
}
TEST(AccountCaptureNative, RetainedLeaseCannotVisitAfterCallback) {
    capture(desktop, fields_desktop, LeaseAction::retain);
    // inspect_reads triggers the real ready callback which probes the retained
    // lease only after capture returned. No stale byte/span is dereferenced.
}
TEST(AccountCaptureNative, SplitBetweenCRAndLF) {
    capture(desktop, fields_desktop, LeaseAction::exact, desktop.find("\r\n") + 1);
}
TEST(AccountCaptureNative, SplitAtHeaderColon) {
    capture(desktop, fields_desktop, LeaseAction::exact, desktop.find("Authorization:") + 13);
}
TEST(AccountCaptureNative, SplitInsideToken) {
    capture(desktop, fields_desktop, LeaseAction::exact, desktop.find("Bearer ") + 11);
}
TEST(AccountCaptureNative, EmptyOwsViaAndUnrelatedMultiplicity) {
    constexpr auto input = "GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\n"
                           "X-Empty:\t \r\nX-Other: a\t b\r\nVia: 1.0 edge-a\r\nvIa: 1.1 edge-b\r\n"
                           "Cookie: a=1\r\nCookie: a=2\r\n\r\n"sv;
    constexpr std::array fields{Field{"Host", "timer.example"}, Field{"X-Empty", ""},
                                Field{"X-Other", "a\t b"},      Field{"Via", "1.0 edge-a"},
                                Field{"vIa", "1.1 edge-b"},     Field{"Cookie", "a=1"},
                                Field{"Cookie", "a=2"}};
    capture(input, fields);
}
TEST(AccountCaptureNative, ArbitrarySecretFieldsRemainLosslessWithoutTransportInterpretation) {
    constexpr auto raw =
        "GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\n"
        "Authorization: CNY-s11-7q9\r\nCookie: a=CNY-s11-7q9\r\nX-Other: CNY-s11-7q9\r\n"
        "Forwarded: for=CNY-s11-7q9\r\n\r\n"sv;
    constexpr std::array fields{Field{"Host", "timer.example"},
                                Field{"Authorization", "CNY-s11-7q9"},
                                Field{"Cookie", "a=CNY-s11-7q9"}, Field{"X-Other", "CNY-s11-7q9"},
                                Field{"Forwarded", "for=CNY-s11-7q9"}};
    capture(raw, fields); // no prepare/identity/authentication or proxy trust claim
}
TEST(AccountCaptureNative, AcceptedPublicNegotiateBodyIsCapturedExactly) {
    constexpr auto body =
        "{\"supported_versions\":[\"reference.v1\"],\"required_capabilities\":[]}"sv;
    static_assert(body.size() == 66);
    constexpr auto head =
        "POST /api/reference/v1/negotiate HTTP/1.1\r\nHost: timer.example\r\nContent-Type: application/json\r\nContent-Length: 66\r\n\r\n"sv;
    constexpr std::array fields{Field{"Host", "timer.example"},
                                Field{"Content-Type", "application/json"},
                                Field{"Content-Length", "66"}};
    const auto bytes = std::as_bytes(std::span(body.data(), body.size()));
    Live f(true, {fields, "POST", "/api/reference/v1/negotiate", bytes});
    f.connect();
    f.client.send(head);
    f.client.send(body.substr(0, 65));
    f.r.wait([&] { return f.r.read_bytes.load() >= head.size() + 65; });
    EXPECT_EQ(f.r.captures, 0U);
    f.client.send(body.substr(65));
    wire(f.client, diag);
    f.r.wait([&] { return f.r.captures.load() == 1 && f.r.requests.load() == 1; });
    inspect_reads(f);
    // Private fixture diag is not the public negotiation response implementation.
}
TEST(AccountCaptureNative, TwoRequestsInOneSendHaveSeparateLeasesAndSequence) {
    Live f(true, {fields_desktop});
    f.connect();
    std::array<char, 298> pair{};
    std::copy(desktop.begin(), desktop.end(), pair.begin());
    std::copy(desktop.begin(), desktop.end(), pair.begin() + 149);
    f.client.send({pair.data(), pair.size()});
    wire(f.client, diag);
    wire(f.client, diag);
    f.r.wait([&] { return f.r.captures.load() == 2 && f.r.requests.load() == 2; });
    EXPECT_EQ(f.r.last_sequence, 2U); // second capture's atomic publication precedes read
    inspect_reads(f);
    // This proves exact surviving successor bytes; it does not force a particular
    // read coalescence or observe obsolete compaction bytes (E2 pending).
}
TEST(AccountCaptureNative, SuspendedSuccessorStaysUnparsedUntilRelease) {
    Live f(true, {fields_desktop});
    f.r.hold = true;
    f.connect();
    std::array<char, 298> pair{};
    std::copy(desktop.begin(), desktop.end(), pair.begin());
    std::copy(desktop.begin(), desktop.end(), pair.begin() + 149);
    f.client.send({pair.data(), pair.size()});
    f.r.wait([&] { return f.r.requests.load() == 1 && f.r.captures.load() == 1; });
    f.post(bh::Intent::suspend);
    // owner_ready is handled after the same fixed-mailbox desired suspend state.
    auto before = f.r.turns.load();
    f.post();
    f.r.wait([&] { return f.r.turns.load() > before; });
    EXPECT_EQ(f.r.captures, 1U);
    f.r.hold = false;
    before = f.r.turns.load();
    f.post();
    f.r.wait([&] { return f.r.turns.load() > before; });
    wire(f.client, diag);
    EXPECT_EQ(f.r.captures, 1U);
    f.post(bh::Intent::admit);
    wire(f.client, diag);
    f.r.wait([&] { return f.r.captures.load() == 2; });
}
TEST(AccountCaptureNative, ActualCoalescedSuffixSurvivesReadCompaction) {
    constexpr auto prefix = "GET /api/reference/v1/timer HTTP/1.1\r\nHost:"sv;
    Live f(true, {fields_desktop});
    f.r.hold = true;
    f.r.initial_admit = false;
    f.connect();
    f.r.wait([&] { return f.r.connected.load() == 1; });
    Literal initial;
    initial.add(desktop);
    initial.add(prefix);
    f.client.send(initial.view());
    f.post(bh::Intent::admit);
    f.r.wait([&] { return f.r.requests.load() == 1; });
    // One attempt only; actual syscall, not send() grouping, establishes this
    // scenario. A missing witness is a fixture failure, never product red/skip.
    ASSERT_GE(f.r.latch_count, 1U);
    if (f.r.read_latches[0].returned != static_cast<std::int64_t>(149 + prefix.size())) {
        ADD_FAILURE() << "FIXTURE: coalesced suffix scenario not established; no product credit";
        return;
    }
    f.r.hold = false;
    auto before = f.r.turns.load();
    f.post();
    f.r.wait([&] { return f.r.turns.load() > before; });
    wire(f.client, diag);
    f.client.send(desktop.substr(prefix.size()));
    wire(f.client, diag);
    f.r.wait([&] { return f.r.captures.load() == 2; });
    ASSERT_EQ(f.r.latch_count, 2U);
    const auto sample = f.r.read_latches[1];
    EXPECT_EQ(sample.head, 8U);
    EXPECT_EQ(sample.tail, 8U + prefix.size());
    EXPECT_EQ(sample.writable, 65536U - prefix.size());
    EXPECT_EQ(sample.iovcnt, 1U);
    // Exact second capture proves surviving bytes; post-sodium obsolete-range
    // zeroization still needs the pending E2 observer, not this read witness.
}
TEST(AccountCaptureNative, IncompleteHeadEofDoesNotIssueLease) {
    Live f(true);
    f.connect();
    f.client.send(
        "GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\nAuthorization: Bearer 000"sv);
    f.client.eof();
    wire(f.client, ""sv, true);
    EXPECT_EQ(f.r.captures, 0U);
    EXPECT_EQ(f.r.requests, 0U);
}
TEST(AccountCaptureNative, CancelHeldActualRequestClosesAndRevokes) {
    Live f(true, {fields_desktop});
    f.r.hold = true;
    f.connect();
    f.client.send(desktop);
    f.r.wait([&] { return f.r.requests.load() == 1; });
    f.r.cancel = true;
    f.post();
    wire(f.client, ""sv, true);
    f.r.wait([&] { return f.r.ends.load() == 1; });
    EXPECT_EQ(f.r.contexts[0].end, bh::End::cancelled);
}
TEST(AccountCaptureNative, CloseIntentTerminatesHeldRequest) {
    Live f(true, {fields_desktop});
    f.r.hold = true;
    f.connect();
    f.client.send(desktop);
    f.r.wait([&] { return f.r.requests.load() == 1; });
    f.post(bh::Intent::close);
    wire(f.client, ""sv, true);
    f.r.wait([&] { return f.r.ends.load() == 1; });
    EXPECT_EQ(f.r.contexts[0].end, bh::End::cancelled);
}
TEST(AccountCaptureNative, RequestExpiryAtEquality) {
    Live f(true, {fields_desktop});
    f.r.hold = true;
    f.connect();
    f.client.send(desktop);
    f.r.wait([&] { return f.r.requests.load() == 1; });
    f.r.clock->ns = 6000000000ULL;
    wire(f.client, ""sv, true);
    f.r.wait([&] { return f.r.ends.load() == 1; });
    EXPECT_EQ(f.r.contexts[0].end, bh::End::expired);
}
TEST(AccountCaptureNative, StopHeldRequestDrainsBeforeFrameworkJoin) {
    Live f(true, {fields_desktop});
    f.r.hold = true;
    f.connect();
    f.client.send(desktop);
    f.r.wait([&] { return f.r.requests.load() == 1; });
    ASSERT_TRUE(std::holds_alternative<bh::Post>(f.r.foundation->stop({6000000000ULL})));
    wire(f.client, ""sv, true);
    f.r.wait([&] { return f.r.drained.load(); });
    EXPECT_EQ(f.r.contexts[0].end, bh::End::stopped);
}
TEST(AccountCaptureNative, DisconnectHeldRequestEndsOnce) {
    Live f(true, {fields_desktop});
    f.r.hold = true;
    f.connect();
    f.client.send(desktop);
    f.r.wait([&] { return f.r.requests.load() == 1; });
    f.client.reset();
    f.r.wait([&] { return f.r.ends.load() == 1 && f.r.removed.load() == 1; });
    EXPECT_EQ(f.r.contexts[0].end, bh::End::disconnected);
    EXPECT_EQ(f.r.captures, 1U);
}
void exception_case(LeaseAction action) {
    Live f(true, {fields_desktop, "GET", "/api/reference/v1/timer", {}, action});
    f.connect();
    f.client.send(desktop);
    wire(f.client, ""sv, true);
    // A failed capture callback must not be forwarded as an ordinary request.
    EXPECT_EQ(f.r.requests, 0U);
}
TEST(AccountCaptureNative, VisitorExceptionClosesWithoutDispatch) {
    exception_case(LeaseAction::visitor_throw);
}
TEST(AccountCaptureNative, CaptureExceptionClosesWithoutDispatch) {
    exception_case(LeaseAction::callback_throw);
}

// Generic positives deliberately omit account install; they prove transport setup
// and literal body/framing/budget mechanics, never an account body/auth capability.
TEST(AccountCaptureGeneric, RealDispatchBodyAndReplyPositive) {
    Live f(false);
    f.r.generic_body_check = true;
    f.connect();
    f.client.send("POST /f HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\n\r\ndata"sv);
    wire(f.client, diag);
    f.r.wait([&] { return f.r.requests.load() == 1; });
    EXPECT_EQ(f.r.captures, 0U);
    inspect_reads(f);
}
void native_positive(bool account) {
    Live f(account);
    (void)f.native_client.connect(f.r.native_port);
    f.native_client.send(
        "GET /foundation/ws HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\nCookie: CNY-s11-7q9\r\n\r\n"sv);
    wire(
        f.native_client,
        "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n\r\n"sv);
    f.native_client.send(std::string_view("\x82\x81\x01\x02\x03\x04\x40", 7));
    f.r.wait([&] { return f.r.native_messages.load() == 1; });
    EXPECT_EQ(f.r.native_opened, 1U);
    EXPECT_EQ(f.r.requests, 0U);
    EXPECT_EQ(f.r.captures, 0U);
}
TEST(AccountCaptureGeneric, NativeHandshakeAndMessagePositive) { native_positive(false); }
TEST(AccountCaptureNative, InstalledCaptureExcludesRealNativeHandshakeAndMessage) {
    native_positive(true);
}
void reference_projection(std::size_t pad, bool admitted, std::size_t split = 0) {
    Literal input;
    input.add(desktop.substr(0, desktop.size() - 2));
    input.add("X-Pad: ");
    input.repeat('a', pad);
    input.add("\r\n\r\n");
    // Exact literals:149+9+3938=4096; next byte4097. No proxy metadata in this
    // direct application-listener vector (unlike I10's forwarded3835 example).
    const auto raw = input.view();
    const auto value = raw.substr(desktop.size() - 2 + 7, pad);
    const std::array fields{Field{"Host", "timer.example"}, Field{"Authorization", bearer},
                            Field{"X-Pad", value}};
    if (admitted) {
        capture(raw, fields, LeaseAction::exact, split);
        return;
    }
    Live f(true);
    f.connect();
    if (split) {
        f.client.send(raw.substr(0, split));
        f.r.wait([&] { return f.r.read_bytes.load() >= split; });
    }
    f.client.send(raw.substr(split));
    wire(f.client, large_head, true);
    EXPECT_EQ(f.r.captures, 0U);
    EXPECT_EQ(f.r.requests, 0U);
}
TEST(AccountCaptureNative, ReferenceComplete4095) { reference_projection(3937, true); }
TEST(AccountCaptureNative, ReferenceComplete4096) { reference_projection(3938, true); }
TEST(AccountCaptureNative, ReferenceComplete4097) { reference_projection(3939, false); }
TEST(AccountCaptureNative, SplitImmediatelyBeforeSemanticLimit) {
    reference_projection(3939, false, 4096);
}
void field_count(unsigned fields, bool admitted) {
    Literal input;
    input.add("GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\n");
    std::array<Field, 64> expected{};
    expected[0] = {"Host", "timer.example"};
    for (unsigned i = 1; i < fields; ++i) {
        input.add("X:\r\n");
        if (i < expected.size())
            expected[i] = {"X", ""};
    }
    input.add("\r\n");
    if (admitted) {
        capture(input.view(), std::span(expected).first(fields));
        return;
    }
    Live f(true);
    f.connect();
    f.client.send(input.view());
    wire(f.client, large_head, true);
    EXPECT_EQ(f.r.captures, 0U);
    EXPECT_EQ(f.r.requests, 0U);
}
TEST(AccountCaptureNative, SixtyThreeFields) { field_count(63, true); }
TEST(AccountCaptureNative, SixtyFourFields) { field_count(64, true); }
TEST(AccountCaptureNative, SixtyFiveFields) { field_count(65, false); }
TEST(AccountCaptureNative, MalformedColonlessHeadRefusesWithoutLease) {
    Live f(true);
    f.connect();
    f.client.send(
        "GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\ncolonless-secret\r\n\r\n"sv);
    wire(f.client, bad, true);
    EXPECT_EQ(f.r.captures, 0U);
    EXPECT_EQ(f.r.requests, 0U);
}
TEST(AccountCaptureGeneric, EmptyQueryMarkerIsRefusedWithoutRepair) {
    Live f(false);
    f.connect();
    f.client.send("GET /api/reference/v1/timer? HTTP/1.1\r\nHost: timer.example\r\n\r\n"sv);
    wire(f.client, bad, true);
    EXPECT_EQ(f.r.captures, 0U);
    EXPECT_EQ(f.r.requests, 0U);
}
TEST(AccountCaptureNative, DuplicateContentLengthRefusesBeforeBody) {
    Live f(true);
    f.connect();
    f.client.send(
        "GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\nContent-Length: 0\r\nContent-Length: 00\r\n\r\n"sv);
    wire(f.client, bad, true);
    EXPECT_EQ(f.r.captures, 0U);
}
TEST(AccountCaptureNative, ExpectCanaryRefusesWithoutContinueOrCapture) {
    Live f(true);
    f.connect();
    f.client.send(
        "GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\nExpect: CNY-s11-7q9\r\n\r\n"sv);
    wire(f.client, bad, true);
    EXPECT_EQ(f.r.captures, 0U);
}
// GET/body representation400 is an owning HTTP integration oracle, not a
// reason for this capture-only fixture to invent pre-capture route processing.
// The accepted public-negotiate test supplies the actual body-positive profile.

void raw_head(unsigned spaces, bool admitted, std::size_t split = 0) {
    Literal input;
    input.add("GET /api/reference/v1/timer HTTP/1.1\r\nHost: timer.example\r\nX: ");
    input.repeat(' ', spaces);
    input.add("a\r\n\r\n");
    // 59 base+8 X/final-line bytes +40893 raw outer SP=40960; all OWS is
    // counted BEFORE trimming, while semantic projection remains only67 bytes.
    constexpr std::array fields{Field{"Host", "timer.example"}, Field{"X", "a"}};
    if (admitted) {
        capture(input.view(), fields, LeaseAction::exact, split);
        return;
    }
    Live f(true);
    f.connect();
    auto raw = input.view();
    if (split) {
        f.client.send(raw.substr(0, split));
        f.r.wait([&] { return f.r.read_bytes.load() >= split; });
    }
    f.client.send(raw.substr(split));
    wire(f.client, large_head, true);
    EXPECT_EQ(f.r.captures, 0U);
}
TEST(AccountCaptureNative, RawHead40959) { raw_head(40892, true); }
TEST(AccountCaptureNative, RawHead40960) { raw_head(40893, true); }
TEST(AccountCaptureNative, RawHead40961) { raw_head(40894, false); }
TEST(AccountCaptureNative, RawHeadOverageWhileIncomplete) { raw_head(40894, false, 40960); }
void generic_field_bound(std::size_t value_bytes, bool admitted, bool semantic_total = false) {
    Literal input;
    input.add("POST /f HTTP/1.1\r\nHost: x\r\n");
    if (semantic_total) {
        for (unsigned i = 0; i < 3; ++i) {
            input.add("X: ");
            input.repeat('a', 8191);
            input.add("\r\n");
        }
        input.add("Y: ");
        input.repeat('a', value_bytes);
        input.add("\r\n");
    } else {
        input.add("X: ");
        input.repeat('a', value_bytes);
        input.add("\r\n");
    }
    input.add("\r\n");
    Live f(false);
    f.connect();
    f.client.send(input.view());
    wire(f.client, admitted ? diag : large_head, !admitted);
    if (admitted)
        f.r.wait([&] { return f.r.requests.load() == 1; });
    else
        EXPECT_EQ(f.r.requests, 0U);
    EXPECT_EQ(f.r.captures, 0U);
}
TEST(AccountCaptureGeneric, FieldNamePlusValue8191) { generic_field_bound(8190, true); }
TEST(AccountCaptureGeneric, FieldNamePlusValue8192) { generic_field_bound(8191, true); }
TEST(AccountCaptureGeneric, FieldNamePlusValue8193) { generic_field_bound(8192, false); }
TEST(AccountCaptureGeneric, SemanticHeader32767) { generic_field_bound(8163, true, true); }
TEST(AccountCaptureGeneric, SemanticHeader32768) { generic_field_bound(8164, true, true); }
TEST(AccountCaptureGeneric, SemanticHeader32769) { generic_field_bound(8165, false, true); }
TEST(AccountCaptureGeneric, BodySplitAndIncompleteEofNeverDispatch) {
    Live f(false);
    f.connect();
    f.client.send("POST /f HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\n\r\nnul"sv);
    f.client.eof();
    wire(f.client, ""sv, true);
    EXPECT_EQ(f.r.requests, 0U);
    EXPECT_EQ(f.r.captures, 0U);
}
TEST(AccountCaptureGeneric, BodySplitHasOneOwnerAndExactBytes) {
    Live f(false);
    f.r.generic_body_check = true;
    f.connect();
    constexpr auto prefix = "POST /f HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\n\r\ndat"sv;
    f.client.send(prefix);
    f.r.wait([&] { return f.r.read_bytes.load() >= prefix.size(); });
    EXPECT_EQ(f.r.requests, 0U);
    f.client.send("a"sv);
    wire(f.client, diag);
    f.r.wait([&] { return f.r.requests.load() == 1; });
    inspect_reads(f);
}
void priority(unsigned pad, unsigned length, std::string_view response, std::size_t split = 0) {
    // Accepted generic F05P literal projection, NOT account syntax/auth integration.
    Literal input;
    if (length == 4097)
        input.add("POST /p HTTP/1.1\r\nHost: x\r\nContent-Length: 4097\r\nBad Header: x\r\n");
    else if (length == 4096)
        input.add("POST /p HTTP/1.1\r\nHost: x\r\nContent-Length: 4096\r\nBad Header: x\r\n");
    else
        input.add("POST /p HTTP/1.1\r\nHost: x\r\nContent-Length: 1\r\nBad Header: x\r\n");
    if (pad) {
        input.add("X-Pad: ");
        input.repeat('a', pad);
        input.add("\r\n");
    }
    input.add("\r\n");
    Live f(false, {}, true);
    f.connect();
    auto raw = input.view();
    if (split) {
        f.client.send(raw.substr(0, split));
        f.r.wait([&] { return f.r.read_bytes.load() >= split; });
    }
    f.client.send(raw.substr(split));
    wire(f.client, response, true);
    EXPECT_EQ(f.r.requests, 0U);
    EXPECT_EQ(f.r.captures, 0U);
}
TEST(AccountCaptureGeneric, F05PHeader431BeforeBody413AndSyntax400) {
    priority(4022, 4097, large_head);
}
TEST(AccountCaptureGeneric, F05PBody413BeforeSyntax400) { priority(0, 4097, large_body); }
TEST(AccountCaptureGeneric, F05PAggregate431BeforeSyntax400) { priority(0, 4096, large_head); }
TEST(AccountCaptureGeneric, F05PSyntax400InsideAllBudgets) { priority(0, 1, bad); }
TEST(AccountCaptureGeneric, F05PSplitPreservesPriority) { priority(0, 4097, large_body, 55); }
TEST(AccountCaptureGeneric, TypedReadGuardBoundariesAreNotTransportAuthority) {
    EXPECT_TRUE(std::holds_alternative<bh::Done>(bh::check_read_window(8192)));
    EXPECT_TRUE(std::holds_alternative<bh::Done>(bh::check_read_window(65536)));
    for (const auto n : {8191ULL, 65537ULL}) {
        auto r = bh::check_read_window(n);
        ASSERT_TRUE(std::holds_alternative<bh::Error>(r));
        EXPECT_EQ(std::get<bh::Error>(r), bh::Error::resource_limit);
    }
}
} // namespace
} // namespace account_capture_test
