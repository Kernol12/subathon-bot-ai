// SPDX-License-Identifier: GPL-3.0-or-later
// API-033 (#814): exact pending-mailbox counts without a per-post peer scan.
// Live configures two peers: HTTP peer A (context 0) and an accepted native socket B
// (context 1, no handshake). A post made from A's ready callback runs while A's intent
// is executing on IO0, so the counts it sees are deterministic. pending_wakes counts
// peers with any pending or executing intent; its peak comes from actual posts.
// Run each case in its own process after native consumer admission.
#include "native_fixture.hpp"
#include <subathon/server/testing/bounded_http_observers.hpp>

namespace account_capture_test {
namespace {

void connect_both(Live &f) {
    f.connect();
    f.r.wait([&] { return f.r.connected.load() == 1; });
    (void)f.native_client.connect(f.r.native_port);
    f.r.wait([&] { return f.r.connected.load() == 2; });
}

// Posts owner_ready to A and returns the observation A's ready callback took.
bh::Observation ready_snapshot(Live &f) {
    const auto turns = f.r.turns.load();
    f.post(bh::Intent::owner_ready);
    f.r.wait([&] { return f.r.turns.load() > turns; });
    std::lock_guard lock(f.r.mutex);
    return f.r.observation;
}

// The observation drained_cb took at the actual drain.
bh::Observation drained(Live &f) {
    f.cleanup();
    std::lock_guard lock(f.r.mutex);
    return f.r.observation;
}

bh::U64 visits(Live &f) {
    const auto value = bh::testing::Access::post_peer_visits(*f.r.foundation);
    if (!std::holds_alternative<bh::U64>(value))
        throw std::runtime_error("FIXTURE: post visit observation");
    return std::get<bh::U64>(value);
}

bool is(const bh::Result<bh::Post> &result, bh::Post expected) {
    return std::holds_alternative<bh::Post>(result) && std::get<bh::Post>(result) == expected;
}

// Desired-only, then desired plus ready, on one peer: one pending peer.
TEST(AccountCaptureMailbox, DesiredThenReadyOnOnePeerCountsOne) {
    Live f(false);
    f.connect();
    f.r.wait([&] { return f.r.connected.load() == 1; });
    f.post(bh::Intent::admit);
    const auto ready = ready_snapshot(f);
    EXPECT_EQ(ready.current.pending_wakes, 1U);
    EXPECT_EQ(ready.peak.pending_wakes, 1U);
    const auto end = drained(f);
    EXPECT_EQ(end.current.pending_wakes, 0U);
    EXPECT_EQ(end.peak.pending_wakes, 1U);
}

// A executing plus B desired: two pending peers, also as the peak.
TEST(AccountCaptureMailbox, ExecutingPeerAndOtherDesiredCountTwo) {
    Live f(false);
    connect_both(f);
    std::optional<bh::Result<bh::Post>> admit;
    f.r.during_ready = [&](Rig &r, Context &c) {
        EXPECT_EQ(&c, &r.contexts[0]);
        admit = r.foundation->post(*r.contexts[1].wake, bh::Intent::admit);
    };
    const auto ready = ready_snapshot(f);
    ASSERT_TRUE(admit && is(*admit, bh::Post::queued));
    EXPECT_EQ(ready.current.pending_wakes, 2U);
    EXPECT_EQ(ready.peak.pending_wakes, 2U);
    const auto end = drained(f);
    EXPECT_EQ(end.current.pending_wakes, 0U);
    EXPECT_EQ(end.peak.pending_wakes, 2U);
}

// Close replaces B's desired admission (coalesced) and B still counts once. After B's
// actual removal its slot holds no pending intent and the peak is kept.
TEST(AccountCaptureMailbox, CloseReplacingDesiredCountsOnceAndRemovalClears) {
    Live f(false);
    connect_both(f);
    std::optional<bh::Result<bh::Post>> admit, close;
    f.r.during_ready = [&](Rig &r, Context &) {
        admit = r.foundation->post(*r.contexts[1].wake, bh::Intent::admit);
        close = r.foundation->post(*r.contexts[1].wake, bh::Intent::close);
    };
    const auto ready = ready_snapshot(f);
    ASSERT_TRUE(admit && is(*admit, bh::Post::queued));
    ASSERT_TRUE(close && is(*close, bh::Post::coalesced));
    EXPECT_EQ(ready.current.pending_wakes, 2U);
    EXPECT_EQ(ready.peak.pending_wakes, 2U);
    f.r.wait([&] { return f.r.removed.load() == 1; });
    const auto after = ready_snapshot(f);
    EXPECT_EQ(after.current.pending_wakes, 1U);
    EXPECT_EQ(after.peak.pending_wakes, 2U);
    const auto end = drained(f);
    EXPECT_EQ(end.current.pending_wakes, 0U);
    EXPECT_EQ(end.peak.pending_wakes, 2U);
}

// Static work count, kept apart from latency: an accepted post visits no peer record.
// The protected scan visits every configured record per post (2 here, 4 for two posts).
TEST(AccountCaptureMailbox, PostVisitsNoPeerRecord) {
    Live f(false);
    connect_both(f);
    const auto before = visits(f);
    f.post(bh::Intent::admit);
    f.post(bh::Intent::owner_ready);
    EXPECT_EQ(visits(f) - before, 0U) << "Foundation::post scanned peer records";
}
} // namespace
} // namespace account_capture_test
