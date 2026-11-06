// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SUBATHON_SCHEMA_VALIDATION_TEST_NATIVE_FAULT_V1_H
#define SUBATHON_SCHEMA_VALIDATION_TEST_NATIVE_FAULT_V1_H

#include <limits.h>
#include <stdint.h>

#ifdef __cplusplus
static_assert(CHAR_BIT == 8);
static_assert(sizeof(int) == 4 && sizeof(unsigned int) == 4);
static_assert(sizeof(int64_t) == 8 && sizeof(void *) == 8);
#define SUBATHON_SCHEMA_FAULT_V1_NOEXCEPT noexcept
extern "C" {
#else
_Static_assert(CHAR_BIT == 8, "Native fault ABI requires 8-bit bytes");
_Static_assert(sizeof(int) == 4 && sizeof(unsigned int) == 4,
               "Native fault ABI requires 32-bit int");
_Static_assert(sizeof(int64_t) == 8 && sizeof(void *) == 8,
               "Native fault ABI requires 64-bit cells/pointers");
#define SUBATHON_SCHEMA_FAULT_V1_NOEXCEPT
#endif

// Test artifact ONLY: arm before entry, copy observations after return, dispose after oracles.
__attribute__((visibility("default"))) int
subathon_schema_fault_arm_v1(unsigned int case_id, unsigned int phase_id,
                             int fault_errno) SUBATHON_SCHEMA_FAULT_V1_NOEXCEPT;
__attribute__((visibility("default"))) int
subathon_schema_fault_snapshot_v1(int64_t *cells,
                                  unsigned int cell_count) SUBATHON_SCHEMA_FAULT_V1_NOEXCEPT;
__attribute__((visibility("default"))) int
subathon_schema_fault_report_v1(unsigned char *bytes,
                                unsigned int byte_count) SUBATHON_SCHEMA_FAULT_V1_NOEXCEPT;
__attribute__((visibility("default"))) int
subathon_schema_fault_fallback_v1(void) SUBATHON_SCHEMA_FAULT_V1_NOEXCEPT;

#ifdef __cplusplus
} // extern "C"
#endif
#undef SUBATHON_SCHEMA_FAULT_V1_NOEXCEPT

#endif // SUBATHON_SCHEMA_VALIDATION_TEST_NATIVE_FAULT_V1_H
