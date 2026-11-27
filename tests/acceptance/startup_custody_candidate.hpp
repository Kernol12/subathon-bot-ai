// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <utility>

namespace subathon::server::startup_custody_candidate {

using Startup = std::uint64_t;
enum class Role { application, overlay };
enum class Acceptor { first, second };
enum class Party { framework, io };
enum class Route { closing_400, use_existing_predicates };
enum class Turn { pending, accepted, stale, terminal };
inline void PrintTo(Turn turn, std::ostream *out) {
    switch (turn) {
    case Turn::pending:
        *out << "pending";
        break;
    case Turn::accepted:
        *out << "accepted";
        break;
    case Turn::stale:
        *out << "stale";
        break;
    case Turn::terminal:
        *out << "terminal";
        break;
    }
}

struct Binding {
    Role role;
    std::string address;
    std::uint16_t port;
    bool operator==(const Binding &) const = default;
};

struct Registration {
    Acceptor acceptor;
    Binding requested;
};

struct SocketEndpoint {
    std::string address;
    std::uint16_t port;
};

class BorrowedSocket {
  public:
    BorrowedSocket(int descriptor, SocketEndpoint actual, bool options_verified = true)
        : descriptor_(descriptor), actual_(std::move(actual)), options_verified_(options_verified) {
    }

    BorrowedSocket(const BorrowedSocket &) = delete;
    BorrowedSocket &operator=(const BorrowedSocket &) = delete;

    int descriptor() const {
        ++accesses_;
        return descriptor_;
    }
    const SocketEndpoint &endpoint() const {
        ++accesses_;
        return actual_;
    }
    bool verify_options() const {
        ++accesses_;
        return options_verified_;
    }
    unsigned accesses() const { return accesses_; }

  private:
    int descriptor_;
    SocketEndpoint actual_;
    bool options_verified_;
    mutable unsigned accesses_{};
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
    Candidate(Startup, std::array<Registration, 2>, std::string) {}
    Turn socket_options(Startup, Acceptor, std::optional<Role>, const BorrowedSocket &) {
        return Turn::pending;
    }
    Turn listen_return(Startup, Role) { return Turn::pending; }
    void offer(Startup) {}
    Turn publish(Startup) { return Turn::pending; }
    std::optional<Snapshot> admit_observer(Startup) { return std::nullopt; }
    void release_observer(Startup) {}
    Route route(Startup, Role, std::uint16_t) { return Route::closing_400; }
    void stop(Startup) {}
    void acknowledge(Startup, Party) {}
    void cancel_cold(Startup) {}
    void framework_exception(Startup) {}
    Observations observations() const { return {}; }
};

} // namespace subathon::server::startup_custody_candidate
