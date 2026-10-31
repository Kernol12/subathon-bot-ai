// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/core/timer_lifecycle.hpp>

namespace subathon::core {
TimerAdvance advance_timer(const TimerState &, const TimerConfig &, const TimerClockStep &) {
    throw TimerLifecycleNotImplemented{};
}
TimerState apply_applicable_time_credit(const TimerState &, std::int64_t) {
    throw TimerLifecycleNotImplemented{};
}
TimerState confirm_timer_finish(const TimerState &) { throw TimerLifecycleNotImplemented{}; }
} // namespace subathon::core
