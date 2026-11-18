// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "oracles.hpp"
#include <algorithm>
#include <gtest/gtest.h>
#include <subathon/server/account_capture.hpp>
#include <type_traits>

namespace account_capture_test {
namespace ac = subathon::server::account_capture;
static_assert(!std::is_default_constructible_v<ac::CaptureLease>);
static_assert(!std::is_copy_constructible_v<ac::CaptureLease>);
static_assert(!std::is_copy_assignable_v<ac::CaptureLease>);
static_assert(std::is_nothrow_move_constructible_v<ac::CaptureLease>);
static_assert(std::is_nothrow_move_assignable_v<ac::CaptureLease>);

// Assertion functions only: called inside an ACTUAL native capture callback after
// independent fixture binding/admission. No mock lease, driver or standalone main.
// Do not send either side of secret comparisons to GoogleTest failure diagnostics.
inline void exact_capture(ac::CaptureView view, std::span<const Field> expected,
                          std::string_view method, std::string_view target,
                          std::span<const std::byte> body) {
    EXPECT_TRUE(view.request.method == method);
    EXPECT_TRUE(view.request.escaped_target == target);
    ASSERT_EQ(view.request.body.size(), body.size());
    EXPECT_TRUE(std::equal(body.begin(), body.end(), view.request.body.begin()));
    ASSERT_EQ(view.request.fields.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto f = view.request.fields[i];
        const auto arena = view.request.arena;
        ASSERT_LE(f.name, arena.size());
        ASSERT_LE(f.name_size, arena.size() - f.name);
        ASSERT_LE(f.value, arena.size());
        ASSERT_LE(f.value_size, arena.size() - f.value);
        const auto name =
            std::string_view(reinterpret_cast<const char *>(arena.data() + f.name), f.name_size);
        const auto value =
            std::string_view(reinterpret_cast<const char *>(arena.data() + f.value), f.value_size);
        EXPECT_TRUE(name == expected[i].name);
        EXPECT_TRUE(value == expected[i].value);
    }
}
inline void refused_visit(ac::CaptureLease &lease, ac::Error expected) {
    unsigned calls = 0;
    const auto result =
        lease.with_capture(&calls, [](void *p, ac::CaptureView) { ++*static_cast<unsigned *>(p); });
    ASSERT_TRUE(std::holds_alternative<ac::Error>(result));
    EXPECT_EQ(std::get<ac::Error>(result), expected);
    EXPECT_EQ(calls, 0U);
}

// Use while already inside the one permitted visitor. Never pumps an event loop.
inline void nested_visit(ac::CaptureLease &lease) { refused_visit(lease, ac::Error::reentrant); }
inline void second_or_expired_visit(ac::CaptureLease &lease) {
    refused_visit(lease, ac::Error::revoked);
}

// Observed peer/port must come independently from the real client socket, with
// hashes from the admitted installed inventory, never from request headers.
inline void native_binding(ac::CaptureView view, std::string_view peer, std::uint16_t port,
                           const std::array<std::byte, 32> &artifact,
                           const std::array<std::byte, 32> &profile) {
    ASSERT_LE(peer.size(), 45U);
    EXPECT_TRUE(std::equal(peer.begin(), peer.end(), view.native.peer_address.begin()));
    EXPECT_EQ(view.native.peer_address[peer.size()], '\0');
    EXPECT_EQ(view.native.peer_port, port);
    EXPECT_TRUE(view.native.artifact_sha256 == artifact);
    EXPECT_TRUE(view.native.profile_sha256 == profile);
    EXPECT_NE(view.native.listener, 0U);
    EXPECT_NE(view.native.connection, 0U);
    EXPECT_NE(view.native.request_sequence, 0U);
}
// Called from the enclosing capture callback; the borrow is consumed before that
// callback ends. This checks the longer lifetime rather than copying raw secrets.
inline void stable_after_visit(ac::CaptureLease &lease, std::span<const Field> expected) {
    struct Borrow {
        const ac::NativeConnectionFacts *facts = nullptr;
        std::span<const std::byte> arena, body;
        std::span<const subathon::server::bounded_http::Field> fields;
        std::string_view method, target;
    } borrow;
    const auto result = lease.with_capture(&borrow, [](void *owner, ac::CaptureView view) {
        auto &b = *static_cast<Borrow *>(owner);
        b = {&view.native,        view.request.arena,  view.request.body,
             view.request.fields, view.request.method, view.request.escaped_target};
    });
    ASSERT_TRUE(std::holds_alternative<ac::Done>(result));
    ASSERT_NE(borrow.facts, nullptr);
    exact_capture({{0, borrow.method, borrow.target, borrow.fields, borrow.arena, borrow.body},
                   *borrow.facts},
                  expected, "GET", "/api/reference/v1/timer", {});
    second_or_expired_visit(lease);
}

// Observe the actual mapped region synchronously AFTER the selected sodium wipe,
// BEFORE ownership is returned. This function neither wipes nor reports success
// for the production hook. Binding to that point is a pending review/admission gap.
inline void erased_region(std::span<const std::byte> actual_region) {
    ASSERT_FALSE(actual_region.empty());
    EXPECT_TRUE(std::all_of(actual_region.begin(), actual_region.end(),
                            [](std::byte b) { return b == std::byte{}; }));
}
inline void bounded_reads(const subathon::server::bounded_http::Observation &actual) {
    ASSERT_GT(actual.reads, 0U); // nonvacuous actual receive, not configuration
    EXPECT_EQ(actual.reads, actual.one_iovec_reads);
    EXPECT_GE(actual.read_min_writable, 8192U);
    EXPECT_LE(actual.last_read_offered, 65536U);
    EXPECT_EQ(actual.max_iovcnt, 1U);
    EXPECT_EQ(actual.stock_maps, 0U);
    EXPECT_EQ(actual.stock_batches, 0U);
    EXPECT_EQ(actual.stock_output_appends, 0U);
}
} // namespace account_capture_test
