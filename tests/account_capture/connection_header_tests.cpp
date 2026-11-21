// SPDX-License-Identifier: GPL-3.0-or-later
// Run each case in its own process after native consumer admission.
#include "native_fixture.hpp"

namespace account_capture_test {
namespace {
using namespace std::literals;

constexpr auto target = "/api/reference/v1/negotiate"sv;
constexpr auto head_start = "POST /api/reference/v1/negotiate HTTP/1.1\r\n"
                            "Host: timer.example\r\nContent-Type: application/json\r\n"sv;
constexpr auto body = "{\"supported_versions\":[\"reference.v1\"],\"required_capabilities\":[]}"sv;
static_assert(body.size() == 66);

constexpr auto keep_alive =
    "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nConnection: keep-alive\r\n\r\ndiag"sv;
constexpr auto close = "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nConnection: close\r\n\r\ndiag"sv;
constexpr auto too_large = "HTTP/1.1 413 Payload Too Large\r\nContent-Length: 0\r\n"
                           "Connection: close\r\n\r\n"sv;

void expect_reply(Socket &socket, std::string_view expected, bool eof) {
    std::array<char, 128> bytes{};
    ASSERT_LT(expected.size(), bytes.size());
    const auto count = socket.read(std::span(bytes).first(expected.size() + (eof ? 1U : 0U)), eof);
    ASSERT_EQ(count, expected.size());
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), bytes.begin()));
}

void expect_negotiation(std::string_view connection_lines, std::span<const Field> fields,
                        bool closing = false) {
    const auto head =
        std::string(head_start) + "Content-Length: 66\r\n" + std::string(connection_lines) + "\r\n";
    Live f(true, {fields, "POST", target, std::as_bytes(std::span(body))});
    f.connect();
    f.client.send(head);
    f.client.send(body.substr(0, body.size() - 1));
    f.r.wait([&] { return f.r.read_bytes.load() >= head.size() + body.size() - 1; });

    // The ready callback on the fixture's IO loop fences processing of the prefix.
    const auto turns = f.r.turns.load();
    f.post();
    f.r.wait([&] { return f.r.turns.load() > turns; });
    EXPECT_EQ(f.r.captures.load(), 0U);
    EXPECT_EQ(f.r.requests.load(), 0U);

    f.client.send(body.substr(body.size() - 1));
    expect_reply(f.client, closing ? close : keep_alive, closing);
    f.r.wait([&] { return f.r.captures.load() == 1 && f.r.requests.load() == 1; });
}

constexpr Field host{"Host", "timer.example"};
constexpr Field content_type{"Content-Type", "application/json"};
constexpr Field length{"Content-Length", "66"};

// Same head, byte for byte, as AccountCaptureNative.AcceptedPublicNegotiateBodyIsCapturedExactly.
// This case adds the IO-loop fence and the zero-request check after 65 body bytes.
TEST(AccountCaptureConnectionHeader, NoConnectionHeader) {
    constexpr std::array fields{host, content_type, length};
    expect_negotiation("", fields);
}

TEST(AccountCaptureConnectionHeader, SoleEmptyValue) {
    constexpr std::array fields{host, content_type, length, Field{"Connection", ""}};
    expect_negotiation("Connection: \t\r\n", fields);
}

TEST(AccountCaptureConnectionHeader, SoleKeepAliveValue) {
    constexpr std::array fields{host, content_type, length, Field{"Connection", "keep-alive"}};
    expect_negotiation("Connection:\t keep-alive \t\r\n", fields);
}

TEST(AccountCaptureConnectionHeader, SoleCloseValue) {
    constexpr std::array fields{host, content_type, length, Field{"Connection", "close"}};
    expect_negotiation("Connection: close\r\n", fields, true);
}

TEST(AccountCaptureConnectionHeader, KeepAliveThenCloseAcrossOccurrences) {
    constexpr std::array fields{host, content_type, length, Field{"Connection", "keep-alive"},
                                Field{"cOnNeCtIoN", "close"}};
    expect_negotiation("Connection: keep-alive\r\ncOnNeCtIoN:\t close \t\r\n", fields, true);
}

TEST(AccountCaptureConnectionHeader, ValidListLeavesRouteBodyLimitInEffect) {
    constexpr auto head_end = "Content-Length: 4097\r\nConnection: keep-alive\r\n\r\n";
    Live f(true, {});
    f.connect();
    f.client.send(std::string(head_start) + head_end);
    expect_reply(f.client, too_large, true);
    EXPECT_EQ(f.r.captures.load(), 0U);
    EXPECT_EQ(f.r.requests.load(), 0U);
}

constexpr auto bad_request =
    "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"sv;

// A refusal reply must arrive and end within this bound, well inside the fixture's
// 7 s deadlines.
constexpr auto reply_bound = 2s;

// Waits until the socket has bytes or end of stream to read. poll() consumes nothing,
// so every reply byte still goes through Socket::read and its wire scanner.
bool readable_before(const Socket &socket, std::chrono::steady_clock::time_point until) {
    for (;;) {
        const auto left =
            std::chrono::ceil<std::chrono::milliseconds>(until - std::chrono::steady_clock::now());
        if (left <= 0ms)
            return false;
        pollfd p{socket.get(), POLLIN, 0};
        const auto ready = ::poll(&p, 1, static_cast<int>(left.count()));
        if (ready >= 0)
            return ready == 1;
        if (errno != EINTR)
            throw std::runtime_error("FIXTURE: reply poll");
    }
}

// An invalid Connection list is refused on the head alone. The witness that the
// declared body was never admitted is the fixed 400 and end of stream while the
// client has sent zero body bytes, so no body write precedes or follows the reply.
// The reply is read one byte per poll() readiness inside reply_bound, so a head that
// is not answered, or is answered without closing, fails an assertion here instead of
// running into Socket::read's deadline. Nothing here posts or waits for a wake: the
// peer may already be closed and removed once the reply has been read.
void expect_refusal(std::string_view connection_lines, std::string_view declared = "66") {
    Live f(true, {});
    f.connect();
    f.client.send(std::string(head_start) + "Content-Length: " + std::string(declared) + "\r\n" +
                  std::string(connection_lines) + "\r\n");

    std::array<char, 128> bytes{};
    std::size_t count = 0;
    bool eof = false;
    const auto until = std::chrono::steady_clock::now() + reply_bound;
    while (!eof && count < bytes.size() && readable_before(f.client, until)) {
        const auto n = f.client.read(std::span(bytes).subspan(count, 1));
        eof = n == 0;
        count += n;
    }
    if (count == 0 && !eof)
        ADD_FAILURE() << "The request head alone produced no reply";
    else {
        EXPECT_EQ(count, bad_request.size());
        EXPECT_TRUE(std::string_view(bytes.data(), count) == bad_request);
        EXPECT_TRUE(eof) << "The reply was not followed by end of stream";
    }
    EXPECT_EQ(f.r.captures.load(), 0U);
    EXPECT_EQ(f.r.requests.load(), 0U);
}

// Each refusal below repeats the Connection lines of its matched positive byte for
// byte and adds only the list defect: one comma, or one more occurrence.

// SoleKeepAliveValue with the comma in place of the trailing SP/HTAB.
TEST(AccountCaptureConnectionHeader, TrailingCommaRefusedOnHeadAlone) {
    expect_refusal("Connection:\t keep-alive,\r\n");
}

// SoleKeepAliveValue plus the comma. Outer SP/HTAB is trimmed first, which leaves the
// same empty terminal item.
TEST(AccountCaptureConnectionHeader, TrailingCommaBeforeWhitespaceRefusedOnHeadAlone) {
    expect_refusal("Connection:\t keep-alive, \t\r\n");
}

// KeepAliveThenCloseAcrossOccurrences plus a comma after close: the valid first
// occurrence cannot mask the empty terminal item.
TEST(AccountCaptureConnectionHeader, KeepAliveThenCloseCommaRefused) {
    expect_refusal("Connection: keep-alive\r\ncOnNeCtIoN:\t close, \t\r\n");
}

// KeepAliveThenCloseAcrossOccurrences plus a comma after keep-alive: a value ending
// in a comma before another occurrence is an empty interior item.
TEST(AccountCaptureConnectionHeader, KeepAliveCommaThenCloseRefused) {
    expect_refusal("Connection: keep-alive,\r\ncOnNeCtIoN:\t close \t\r\n");
}

// The lines of SoleKeepAliveValue and SoleEmptyValue, each valid alone. An empty
// occurrence is only valid alone; in a repeated list it is an empty item.
TEST(AccountCaptureConnectionHeader, KeepAliveThenEmptyOccurrenceRefused) {
    expect_refusal("Connection:\t keep-alive \t\r\nConnection: \t\r\n");
}

TEST(AccountCaptureConnectionHeader, EmptyOccurrenceThenKeepAliveRefused) {
    expect_refusal("Connection: \t\r\nConnection:\t keep-alive \t\r\n");
}

// Pairs with ValidListLeavesRouteBodyLimitInEffect: the same over-limit declaration
// answers 413 with a valid list and the field 400 with an invalid one.
TEST(AccountCaptureConnectionHeader, InvalidListRefusedBeforeRouteBodyLimit) {
    expect_refusal("Connection: keep-alive,\r\n", "4097");
}

// Control outside the ordinary correction: the marked native listener keeps its
// handshake when the Upgrade token carries a trailing comma. This is the head of
// native_positive in native_tests.cpp plus that comma. The contract's 101 for it is
// source-derived until this case has been observed.
TEST(AccountCaptureConnectionHeader, NativeUpgradeTrailingCommaKeepsHandshake) {
    constexpr auto switching =
        "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
        "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n\r\n"sv;
    Live f(true);
    (void)f.native_client.connect(f.r.native_port);
    f.native_client.send("GET /foundation/ws HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\n"
                         "Connection: Upgrade,\r\n"
                         "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                         "Sec-WebSocket-Version: 13\r\nCookie: CNY-s11-7q9\r\n\r\n"sv);
    std::array<char, switching.size()> reply{};
    ASSERT_EQ(f.native_client.read(reply), switching.size());
    EXPECT_TRUE(std::equal(switching.begin(), switching.end(), reply.begin()));
    f.native_client.send(std::string_view("\x82\x81\x01\x02\x03\x04\x40", 7));
    f.r.wait([&] { return f.r.native_messages.load() == 1; });
    EXPECT_EQ(f.r.native_opened.load(), 1U);
    EXPECT_EQ(f.r.requests.load(), 0U);
    EXPECT_EQ(f.r.captures.load(), 0U);
}
} // namespace
} // namespace account_capture_test
