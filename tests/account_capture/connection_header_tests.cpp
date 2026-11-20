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
} // namespace
} // namespace account_capture_test
