// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/api_host.hpp>
namespace subathon::server::api_host {
// Qt-owner consumption also runs before entry while a signal notifier is pending.
void consume_stop_signal(Host &) noexcept;
// Private transport composition. These serials select socket owners only and
// confer no execution, source, installation or publication membership.
inline thread_local std::uint64_t connected_peer_serial = 0;
inline thread_local std::uint64_t submitted_command_serial = 0;
// Credits contain no authority. They account transport projections separately
// from the original lease, before any payload ownership or serialization.
class ResponseBudget {
  public:
    virtual ~ResponseBudget() = default;
    virtual bool acquire(std::uint64_t) = 0;
    virtual void release(std::uint64_t) noexcept = 0;
    virtual bool peer_live(std::uint64_t) const = 0;
};
class CopyCredit {
    std::shared_ptr<ResponseBudget> budget_;
    std::uint64_t capacity_ = 0;

  public:
    CopyCredit() = default;
    CopyCredit(const CopyCredit &) = delete;
    CopyCredit &operator=(const CopyCredit &) = delete;
    ~CopyCredit() { reset(); }
    bool reserve(const std::shared_ptr<ResponseBudget> &budget, std::uint64_t capacity) {
        if (budget_ || !budget->acquire(capacity))
            return false;
        budget_ = budget;
        capacity_ = capacity;
        return true;
    }
    void reset() noexcept {
        if (budget_) {
            budget_->release(capacity_);
            budget_.reset();
        }
    }
};
class NetworkTransport : public Transport {
  public:
    virtual void configure(const Limits &, std::shared_ptr<const native_ws::ElapsedSource>,
                           std::shared_ptr<ResponseBudget>) = 0;
    virtual void handoff_to(std::uint64_t peer, std::uint64_t command, ResponseLease) = 0;
};
} // namespace subathon::server::api_host
