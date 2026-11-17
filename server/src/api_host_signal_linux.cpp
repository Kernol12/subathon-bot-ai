// SPDX-License-Identifier: GPL-3.0-or-later
#include "api_host_internal.hpp"
#include <QSocketNotifier>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <subathon/server/api_host_signal.hpp>
#include <thread>
#include <unistd.h>

namespace subathon::server::api_host {
namespace {
// Process-lifetime storage survives late handler entry. The single atomic word
// closes entry and counts borrowers indivisibly, before any descriptor access.
constexpr unsigned closed = 1U << 31;
std::atomic<unsigned> gate{closed};
std::atomic<bool> pending{false};
int wake_fd = -1;
static_assert(std::atomic<unsigned>::is_always_lock_free);
static_assert(std::atomic<bool>::is_always_lock_free);
void record_signal(int) {
    const int saved = errno;
    unsigned seen = gate.load(std::memory_order_relaxed);
    while (!(seen & closed)) {
        if (gate.compare_exchange_weak(seen, seen + 1, std::memory_order_acquire)) {
            pending.store(true, std::memory_order_release);
            const char b = 1;
            (void)::write(wake_fd, &b, 1);
            gate.fetch_sub(1, std::memory_order_release);
            break;
        }
    }
    errno = saved;
}
} // namespace
bool detail::signal_pending() noexcept { return pending.load(std::memory_order_acquire); }
struct StopSignalBridge::Impl {
    Host &host;
    std::thread::id owner = std::this_thread::get_id();
    int pipe[2]{-1, -1};
    struct sigaction term{}, interrupt{};
    bool installed = false;
    std::unique_ptr<QSocketNotifier> notifier;
    explicit Impl(Host &h) : host(h) {}
};
StopSignalBridge::StopSignalBridge(Host &host) : impl_(std::make_unique<Impl>(host)) {}
StopSignalBridge::~StopSignalBridge() {
    auto &s = *impl_;
    if (!s.installed)
        return;
    gate.fetch_or(closed, std::memory_order_acq_rel);
    while ((gate.load(std::memory_order_acquire) & ~closed) != 0)
        std::this_thread::yield();
    ::sigaction(SIGTERM, &s.term, nullptr);
    ::sigaction(SIGINT, &s.interrupt, nullptr);
    s.notifier.reset();
    ::close(s.pipe[0]);
    ::close(s.pipe[1]);
    wake_fd = -1;
}
Result<StartAccepted> StopSignalBridge::install() {
    auto &s = *impl_;
    if (s.owner != std::this_thread::get_id())
        return Failure::wrong_thread;
    if (s.installed || gate.load() != closed)
        return Failure::already_started;
    if (::pipe2(s.pipe, O_NONBLOCK | O_CLOEXEC))
        return Failure::unavailable;
    pending.store(false);
    wake_fd = s.pipe[1];
    struct sigaction action{};
    action.sa_handler = record_signal;
    ::sigemptyset(&action.sa_mask);
    if (::sigaction(SIGTERM, &action, &s.term)) {
        ::close(s.pipe[0]);
        ::close(s.pipe[1]);
        return Failure::unavailable;
    }
    if (::sigaction(SIGINT, &action, &s.interrupt)) {
        ::sigaction(SIGTERM, &s.term, nullptr);
        ::close(s.pipe[0]);
        ::close(s.pipe[1]);
        return Failure::unavailable;
    }
    gate.store(0, std::memory_order_release);
    s.installed = true;
    s.notifier = std::make_unique<QSocketNotifier>(s.pipe[0], QSocketNotifier::Read);
    QObject::connect(s.notifier.get(), &QSocketNotifier::activated, [&s] {
        char bytes[64];
        while (::read(s.pipe[0], bytes, sizeof(bytes)) > 0) {
        }
        if (pending.load(std::memory_order_acquire))
            s.host.request_stop();
    });
    return StartAccepted{};
}
} // namespace subathon::server::api_host
