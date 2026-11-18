// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "secret_scanner.hpp"
#include <mutex>
#include <stdexcept>
#include <trantor/utils/Logger.h>
namespace account_capture_test {
// Existing Trantor default-channel binding only. No raw retention, fd interception
// or claimed pre-main/direct-write observation. Callback target lives until exit.
struct LogObservation {
    std::mutex mutex;
    SecretScanner scanner;
    bool invalid = false;
    void feed(const char *bytes, std::uint64_t size) noexcept {
        std::lock_guard lock(mutex);
        if ((size && !bytes) || size > SecretScanner::ceiling) {
            invalid = true;
            return;
        }
        scanner.feed({size ? bytes : "", static_cast<std::size_t>(size)});
    }
    bool clean() {
        std::lock_guard lock(mutex);
        return !invalid && scanner.clean();
    }
    bool detected() {
        std::lock_guard lock(mutex);
        return !invalid && scanner.hits() != 0;
    }
};
static_assert(sizeof(LogObservation) <= 128);
inline LogObservation *active_log = nullptr; // set only before any fixture framework/IO thread
inline void bind_log(LogObservation &state) {
    active_log = &state;
    trantor::Logger::setOutputFunction(
        [](const char *p, const std::uint64_t n) { active_log->feed(p, n); }, [] {});
    // Enables only compiled-in sites. NDEBUG compiles SDK LOG_TRACE out in the
    // selected Release build; Debug first-party/test TRACE sites remain separate.
    trantor::Logger::setLogLevel(trantor::Logger::kTrace);
}
// One independent control target, then actual clean target, both process-retained.
inline LogObservation *log_control = nullptr;
inline void bind_checked_log(LogObservation &actual) {
    if (!log_control)
        log_control = new LogObservation;
    bind_log(*log_control);
    LOG_INFO << "CNY-s11-7q9"; // actual active default channel, not SDK TRACE coverage
    if (!log_control->detected())
        throw std::runtime_error("FIXTURE: logger scanner positive missing");
    bind_log(actual);
}
} // namespace account_capture_test
