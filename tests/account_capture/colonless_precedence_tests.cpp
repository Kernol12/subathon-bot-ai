// SPDX-License-Identifier: GPL-3.0-or-later
// #765, contract correction (advisor 765.2) to bounded-http-foundation-v1 §4: a line
// without a colon is one invalid record, never a field occurrence, whatever its text.
// It keeps its phase-4 syntax place behind route header 431, body 413 and aggregate
// 431. Real colon-bearing framing occurrences keep their phase-2 400.
// Run each case in its own process after native consumer admission.
#include "bounded_literals.hpp"
#include "native_fixture.hpp"

namespace account_capture_test {
namespace {
using namespace std::literals;

constexpr auto bad = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"sv;
constexpr auto large_body =
    "HTTP/1.1 413 Payload Too Large\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"sv;
constexpr auto large_head =
    "HTTP/1.1 431 Request Header Fields Too Large\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"sv;

// The accepted F05P projection on the generic budget route /p (ceilings 4096/4096/4096),
// with `lines` in place of its malformed field. `pad` adds an X-Pad field of that many
// bytes. Only the head is sent: each outcome is a fixed refusal, then end of stream,
// with no capture and no request.
void precedence(std::string_view length, std::string_view lines, std::string_view response,
                unsigned pad = 0) {
    Literal input;
    input.add("POST /p HTTP/1.1\r\nHost: x\r\nContent-Length: ");
    input.add(length);
    input.add("\r\n");
    input.add(lines);
    if (pad) {
        input.add("X-Pad: ");
        input.repeat('a', pad);
        input.add("\r\n");
    }
    input.add("\r\n");
    Live f(false, {}, true);
    f.connect();
    f.client.send(input.view());
    std::array<char, 128> bytes{};
    const auto n = f.client.read(std::span(bytes).first(response.size() + 1), true);
    ASSERT_EQ(n, response.size());
    EXPECT_TRUE(std::equal(response.begin(), response.end(), bytes.begin()));
    EXPECT_EQ(f.r.requests, 0U);
    EXPECT_EQ(f.r.captures, 0U);
}

// One colon-less line per framing name of phase 2. Each was decided as an occurrence
// (400) before the declared 4097 > 4096 body; as a record it keeps the 413.
TEST(AccountCaptureColonless, ContentLengthLineKeepsBody413) {
    precedence("4097", "Content-Length\r\n", large_body);
}
TEST(AccountCaptureColonless, HostCorrelationLineKeepsBody413) {
    precedence("4097", "X-Host-Correlation: 61\r\nX-Host-Correlation\r\n", large_body);
}
TEST(AccountCaptureColonless, TransferEncodingLineKeepsBody413) {
    precedence("4097", "Transfer-Encoding\r\n", large_body);
}
TEST(AccountCaptureColonless, TeLineKeepsBody413) { precedence("4097", "TE\r\n", large_body); }
TEST(AccountCaptureColonless, TrailerLineKeepsBody413) {
    precedence("4097", "Trailer\r\n", large_body);
}
TEST(AccountCaptureColonless, ExpectLineKeepsBody413) {
    precedence("4097", "Expect\r\n", large_body);
}
TEST(AccountCaptureColonless, UpgradeLineKeepsBody413) {
    precedence("4097", "Upgrade\r\n", large_body);
}
TEST(AccountCaptureColonless, ProxyConnectionLineKeepsBody413) {
    precedence("4097", "Proxy-Connection\r\n", large_body);
}

// The other phase-3 limits also precede the record. With X-Pad 4022 the header
// projection exceeds 4096 (route header 431); with CL 4096 the header projection plus
// the body exceeds 4096 (aggregate 431).
TEST(AccountCaptureColonless, RecordKeepsRouteHeader431) {
    precedence("4097", "Transfer-Encoding\r\n", large_head, 4022);
}
TEST(AccountCaptureColonless, RecordKeepsAggregate431) {
    precedence("4096", "Transfer-Encoding\r\n", large_head);
}

// Controls. Inside every budget the record is still the phase-4 400: nothing becomes
// admissible. Real colon-bearing framing occurrences keep their early phase-2 400
// ahead of the over-limit body.
TEST(AccountCaptureColonless, RecordInsideAllBudgetsIsSyntax400) {
    precedence("1", "Transfer-Encoding\r\n", bad);
}
TEST(AccountCaptureColonless, TransferEncodingFieldStaysEarly400) {
    precedence("4097", "Transfer-Encoding: chunked\r\n", bad);
}
TEST(AccountCaptureColonless, DuplicateContentLengthFieldStaysEarly400) {
    precedence("4097", "Content-Length: 4097\r\n", bad);
}
} // namespace
} // namespace account_capture_test
