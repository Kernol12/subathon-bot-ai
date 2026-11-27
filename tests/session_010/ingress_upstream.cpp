// SPDX-License-Identifier: GPL-3.0-or-later
// SESSION-010 S3 (#893) real-upstream positive: one selector per process behind the
// pinned #888 Caddy edge, driven by edge_capture_driver.py --suite ingress. The
// producer reports only after the binding handed off digestZ under its own g1.
#include "ingress_fixture.hpp"

#include <cinttypes>
#include <cstdio>

namespace session_ingress_test {
namespace {
// One driver byte on stdin: 'P' after it observed the edge, 'S' to complete.
bool command(char expected) {
    pollfd input{STDIN_FILENO, POLLIN, 0};
    char command = 0;
    return ::poll(&input, 1, 10000) == 1 && (input.revents & POLLIN) &&
           ::read(STDIN_FILENO, &command, 1) == 1 && command == expected;
}

void run(const char *selector, AccountTransportKind kind, std::size_t fields,
         std::size_t projection) {
    ASSERT_TRUE(GTEST_FLAG_GET(filter) == selector && GTEST_FLAG_GET(repeat) == 1);
    // r2 line 126: g1 and its acknowledgment follow the driver's edge observation.
    Ingress f(reference_facts(), false);
    ASSERT_GT(std::printf("SESSION010_READY %s http=127.0.0.1:%u native=127.0.0.1:%u\n", selector,
                          unsigned(f.http_port), unsigned(f.native_port)),
              0);
    ASSERT_EQ(std::fflush(stdout), 0);
    ASSERT_TRUE(command('P')) << "FIXTURE: readiness byte missing";
    f.binding().publish(f.next(reference_facts()));
    ASSERT_GT(std::printf("SESSION010_PUBLISHED %s\n", selector), 0);
    ASSERT_EQ(std::fflush(stdout), 0);
    // The driver allows 12 s for the capture report; two bounded waits cover it.
    ASSERT_TRUE(f.wait([&] { return f.requests > 0; }) || f.wait([&] { return f.requests > 0; }))
        << "no request from the edge";
    ASSERT_EQ(f.handoffs.size(), 1U) << "no handoff for the edge request";
    const auto &h = f.handoffs[0];
    const auto *p = prepared(h);
    ASSERT_NE(p, nullptr) << "edge request was not prepared";
    ASSERT_TRUE(std::equal(p->digest.begin(), p->digest.end(), digest_z.begin()));
    ASSERT_EQ(p->transport, kind);
    ASSERT_EQ(p->policy_revision_ref, "g1");
    ASSERT_EQ(h.provenance.pin->reference, "g1");
    ASSERT_EQ(h.provenance.fields, fields);
    ASSERT_EQ(h.provenance.projection, projection);
    ASSERT_EQ(crypto_calls.load(), 1U);
    ASSERT_GT(std::printf("SESSION010_CAPTURE %s listener=%" PRIu64 " connection=%" PRIu64
                          " sequence=%" PRIu64 " peer=127.0.0.1:%u fields=%" PRIu64
                          " projection=%" PRIu64 " prepared=1\n",
                          selector, h.provenance.listener, h.provenance.connection,
                          h.provenance.sequence, unsigned(h.provenance.peer_port),
                          h.provenance.fields, h.provenance.projection),
              0);
    ASSERT_EQ(std::fflush(stdout), 0);
    ASSERT_TRUE(command('S')) << "FIXTURE: completion byte missing";
    f.stop();
    EXPECT_EQ(f.requests, 1U);
    EXPECT_TRUE(f.logs_clean);
    ASSERT_GT(std::printf("SESSION010_FINISHED %s\n", selector), 0);
    ASSERT_EQ(std::fflush(stdout), 0);
}

TEST(Session010IngressUpstream, Desktop252) {
    run("Session010IngressUpstream.Desktop252", AccountTransportKind::desktop_bearer, 6, 252);
}
TEST(Session010IngressUpstream, Browser282) {
    run("Session010IngressUpstream.Browser282", AccountTransportKind::browser_cookie, 7, 282);
}
} // namespace
} // namespace session_ingress_test
