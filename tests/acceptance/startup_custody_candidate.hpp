// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace subathon::server::startup_custody_candidate {

using Startup = std::uint64_t;
enum class Role { application, overlay };
enum class Party { framework, io };
enum class Turn { pending, accepted, stale, terminal };

struct Binding {
    Role role;
    std::string address;
    std::uint16_t port;
    bool operator==(const Binding &) const = default;
};

struct BorrowedSocket {
    int descriptor;
    Binding actual;
    bool options_verified = true;
};

struct Endpoint {
    Binding binding;
    bool options_verified;
    bool operator==(const Endpoint &) const = default;
};

struct Snapshot {
    Endpoint application;
    Endpoint overlay;
    std::string scratch_root;
    bool operator==(const Snapshot &) const = default;
};

struct Observations {
    unsigned publications{};
    unsigned socket_accesses{};
    unsigned record_reads{};
    unsigned io_outstanding{};
    bool framework_retired{};
    bool io_retired{};
    bool record_released{};
    bool unavailable{};
};

// Synthetic entry points mirror the framework, IO0 and Qt turns. Each queued
// turn receives its original startup identity. No socket or thread is opened.
class Candidate {
  public:
    Candidate(Startup, std::array<Binding, 2>, std::string) {}
    Turn socket_options(Startup, std::optional<Role>, const BorrowedSocket &) {
        return Turn::pending;
    }
    Turn listen_return(Startup, Role) { return Turn::pending; }
    void offer(Startup) {}
    Turn publish(Startup) { return Turn::pending; }
    std::optional<Snapshot> admit_observer(Startup) { return std::nullopt; }
    void release_observer(Startup) {}
    int route_status(Startup) { return 400; }
    void stop(Startup) {}
    void acknowledge(Startup, Party) {}
    void cancel_cold(Startup) {}
    void framework_exception(Startup) {}
    Observations observations() const { return {}; }
};

} // namespace subathon::server::startup_custody_candidate
