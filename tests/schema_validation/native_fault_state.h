// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SUBATHON_SCHEMA_VALIDATION_TEST_NATIVE_FAULT_STATE_H
#define SUBATHON_SCHEMA_VALIDATION_TEST_NATIVE_FAULT_STATE_H

#include "transport_core_v1.h"
#include <cstddef>

namespace subathon::schema_validation::native::fault {

enum class Writer : unsigned int { parent = 0, child = 1, controller = 2 };
struct Plan {
    unsigned int case_id;
    unsigned int phase_id;
    int fault_errno;
};

// Hidden test-only scalar hooks for separately authored static FaultOps. No C ABI
// callback, owner algorithm, child/exec implementation or accepted result is supplied.
[[gnu::visibility("hidden")]] Plan selected_plan() noexcept;
[[gnu::visibility("hidden")]] bool record(Writer writer, const Observation &value) noexcept;
[[gnu::visibility("hidden")]] RawResult monotonic_ns() noexcept;
// Caller supplies genuine saved kernel identities and HOLDS each source FD until
// acknowledgement. False is fixture failure, never a worker/domain classification.
[[gnu::visibility("hidden")]] bool pin_birth(int pid, int pidfd) noexcept;
[[gnu::visibility("hidden")]] bool pin_report_read(int descriptor) noexcept;
[[gnu::visibility("hidden")]] bool capture_outcome(const unsigned char *bytes,
                                                   std::size_t count) noexcept;
[[gnu::visibility("hidden")]] bool checkpoint(Phase phase, RawResult actual) noexcept;
[[gnu::visibility("hidden")]] bool child_ready_and_hold() noexcept;
[[gnu::visibility("hidden")]] bool release_child() noexcept;
[[gnu::visibility("hidden")]] const void *mapping_address() noexcept;

} // namespace subathon::schema_validation::native::fault

#endif // SUBATHON_SCHEMA_VALIDATION_TEST_NATIVE_FAULT_STATE_H
