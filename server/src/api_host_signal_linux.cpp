// SPDX-License-Identifier: GPL-3.0-or-later
#include "api_host_private.hpp"
#include <QCoreApplication>
#include <QSocketNotifier>
#include <QThread>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <pthread.h>
#include <subathon/server/api_host_signal.hpp>
#include <thread>
#include <unistd.h>

namespace subathon::server::api_host {
namespace {
static_assert(std::atomic<int>::is_always_lock_free);
static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
constexpr std::uint64_t closed = std::uint64_t{1} << 63;
std::atomic<int> pending{0}, wake_fd{-1};
// Closed permanently after this process's one installation. A late invocation
// of the old handler may increment the count, but cannot enter the FD region.
std::atomic<std::uint64_t> handlers{closed};
Host *signal_host = nullptr;    // Qt owner only
bool installation_used = false; // Qt owner only; gate is never reopened on reuse
void handle_stop(int) {
    const int saved = errno;
    const auto entry = handlers.fetch_add(1, std::memory_order_acq_rel);
    if (!(entry & closed) && !pending.exchange(1, std::memory_order_acq_rel)) {
        const int fd = wake_fd.load(std::memory_order_relaxed);
        const char byte = 1;
        if (fd >= 0)
            (void)::write(fd, &byte, 1);
    }
    handlers.fetch_sub(1, std::memory_order_release);
    errno = saved;
}
void quiesce_handlers() noexcept {
    handlers.fetch_or(closed, std::memory_order_acq_rel);
    // Graceful thread/handler completion is not an OS scheduling deadline.
    // After closing the gate, even an old handler entering after this zero
    // observation cannot access wake_fd or a subsequently recycled descriptor.
    while (handlers.load(std::memory_order_acquire) & ~closed)
        std::this_thread::yield();
    wake_fd.store(-1, std::memory_order_relaxed);
    pending.store(0, std::memory_order_relaxed);
}
} // namespace
void consume_stop_signal(Host &host) noexcept {
    if (signal_host == &host && pending.exchange(0, std::memory_order_acq_rel))
        host.request_stop();
}
struct StopSignalBridge::Impl {
    Host &host;
    int read_fd = -1, write_fd = -1;
    bool int_installed = false, term_installed = false;
    struct sigaction old_int{}, old_term{};
    std::unique_ptr<QSocketNotifier> notifier;
    explicit Impl(Host &h) : host(h) {}
};
StopSignalBridge::StopSignalBridge(Host &h) : impl_(std::make_unique<Impl>(h)) {}
Result<StartAccepted> StopSignalBridge::install() {
    auto &s = *impl_;
    if (!QCoreApplication::instance() ||
        QThread::currentThread() != QCoreApplication::instance()->thread())
        return Failure::wrong_thread;
    if (installation_used || s.read_fd >= 0)
        return Failure::already_started;
    if (s.host.inspect().state != State::cold)
        return Failure::invalid_options;
    int pipe[2];
    if (::pipe2(pipe, O_NONBLOCK | O_CLOEXEC) != 0)
        return Failure::unavailable;
    s.read_fd = pipe[0];
    s.write_fd = pipe[1];
    // Allocate Qt state before installing an asynchronous handler.
    try {
        s.notifier = std::make_unique<QSocketNotifier>(s.read_fd, QSocketNotifier::Read);
        QObject::connect(s.notifier.get(), &QSocketNotifier::activated, s.notifier.get(), [&s] {
            char bytes[128];
            while (::read(s.read_fd, bytes, sizeof bytes) > 0) {
            }
            consume_stop_signal(s.host);
        });
    } catch (...) {
        return Failure::resource_limit;
    }
    installation_used = true;
    struct sigaction action{};
    action.sa_handler = handle_stop;
    action.sa_flags = SA_RESTART;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGINT);
    sigaddset(&action.sa_mask, SIGTERM);
    wake_fd.store(s.write_fd, std::memory_order_relaxed);
    handlers.store(0, std::memory_order_release);
    if (::sigaction(SIGINT, &action, &s.old_int) == 0)
        s.int_installed = true;
    if (s.int_installed && ::sigaction(SIGTERM, &action, &s.old_term) == 0)
        s.term_installed = true;
    if (!s.term_installed) {
        handlers.fetch_or(closed, std::memory_order_acq_rel);
        if (s.int_installed) {
            if (::sigaction(SIGINT, &s.old_int, nullptr) != 0)
                std::terminate();
            s.int_installed = false;
        }
        quiesce_handlers();
        return Failure::unavailable;
    }
    signal_host = &s.host;
    return StartAccepted{};
}
StopSignalBridge::~StopSignalBridge() {
    auto &s = *impl_;
    sigset_t set, old;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    if (pthread_sigmask(SIG_BLOCK, &set, &old) != 0)
        std::terminate();
    const auto state = s.host.inspect();
    if (signal_host == &s.host && state.state != State::cold && !state.owner_cleanup)
        std::terminate();
    if (s.int_installed || s.term_installed) {
        // Gate first, then restore dispositions, then await every handler that
        // passed the gate. No reopened generation can admit an old invocation.
        handlers.fetch_or(closed, std::memory_order_acq_rel);
        if (s.int_installed && ::sigaction(SIGINT, &s.old_int, nullptr) != 0)
            std::terminate();
        if (s.term_installed && ::sigaction(SIGTERM, &s.old_term, nullptr) != 0)
            std::terminate();
        quiesce_handlers();
        signal_host = nullptr;
    }
    s.notifier.reset();
    if (s.read_fd >= 0)
        ::close(s.read_fd);
    if (s.write_fd >= 0)
        ::close(s.write_fd);
    if (pthread_sigmask(SIG_SETMASK, &old, nullptr) != 0)
        std::terminate();
}
} // namespace subathon::server::api_host
