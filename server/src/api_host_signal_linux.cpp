// SPDX-License-Identifier: GPL-3.0-or-later
#include "api_host_private.hpp"
#include <QCoreApplication>
#include <QSocketNotifier>
#include <QThread>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <pthread.h>
#include <subathon/server/api_host_signal.hpp>
#include <unistd.h>

namespace subathon::server::api_host {
namespace {
volatile sig_atomic_t pending = 0;
volatile sig_atomic_t wake_fd = -1;
Host *signal_host = nullptr; // accessed on Qt main only
void handle_stop(int) {
    const int saved = errno;
    pending = 1;
    const int fd = wake_fd;
    if (fd >= 0) {
        const char byte = 1;
        (void)::write(fd, &byte, 1);
    }
    errno = saved;
}
} // namespace
void consume_stop_signal(Host &host) noexcept {
    if (signal_host == &host && pending) {
        pending = 0;
        host.request_stop();
    }
}
struct StopSignalBridge::Impl {
    Host &host;
    int read_fd = -1, write_fd = -1;
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
    if (signal_host || s.read_fd >= 0)
        return Failure::already_started;
    if (s.host.inspect().state != State::cold)
        return Failure::invalid_options;
    int pipe[2];
    if (::pipe2(pipe, O_NONBLOCK | O_CLOEXEC) != 0)
        return Failure::unavailable;
    s.read_fd = pipe[0];
    s.write_fd = pipe[1];
    struct sigaction action{};
    action.sa_handler = handle_stop;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGINT);
    sigaddset(&action.sa_mask, SIGTERM);
    pending = 0;
    wake_fd = s.write_fd;
    if (::sigaction(SIGINT, &action, &s.old_int) != 0) {
        wake_fd = -1;
        return Failure::unavailable;
    }
    if (::sigaction(SIGTERM, &action, &s.old_term) != 0) {
        ::sigaction(SIGINT, &s.old_int, nullptr);
        wake_fd = -1;
        return Failure::unavailable;
    }
    signal_host = &s.host;
    s.notifier = std::make_unique<QSocketNotifier>(s.read_fd, QSocketNotifier::Read);
    QObject::connect(s.notifier.get(), &QSocketNotifier::activated, s.notifier.get(), [&s] {
        char bytes[128];
        while (::read(s.read_fd, bytes, sizeof bytes) > 0) {
        }
        consume_stop_signal(s.host);
    });
    return StartAccepted{};
}
StopSignalBridge::~StopSignalBridge() {
    auto &s = *impl_;
    // Host/network barriers must precede bridge destruction. All framework threads
    // have joined, so blocking delivery here quiesces the remaining main handler.
    sigset_t set, old;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    if (pthread_sigmask(SIG_BLOCK, &set, &old) != 0)
        std::terminate();
    const auto state = s.host.inspect();
    if (signal_host == &s.host && state.state != State::cold && !state.owner_cleanup)
        std::terminate();
    if (signal_host == &s.host) {
        ::sigaction(SIGINT, &s.old_int, nullptr);
        ::sigaction(SIGTERM, &s.old_term, nullptr);
        signal_host = nullptr;
        wake_fd = -1;
        pending = 0;
    }
    s.notifier.reset();
    if (s.read_fd >= 0)
        ::close(s.read_fd);
    if (s.write_fd >= 0)
        ::close(s.write_fd);
    pthread_sigmask(SIG_SETMASK, &old, nullptr);
}
} // namespace subathon::server::api_host
