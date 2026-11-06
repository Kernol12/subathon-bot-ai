// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SUBATHON_SCHEMA_VALIDATION_NATIVE_TRANSPORT_V1_H
#define SUBATHON_SCHEMA_VALIDATION_NATIVE_TRANSPORT_V1_H

#include <limits.h>

#ifdef __cplusplus
static_assert(CHAR_BIT == 8, "Schema transport ABI 1 requires 8-bit bytes");
static_assert(sizeof(int) == 4, "Schema transport ABI 1 requires 32-bit int");
static_assert(sizeof(unsigned int) == 4, "Schema transport ABI 1 requires 32-bit unsigned int");
static_assert(sizeof(void *) == 8, "Schema transport ABI 1 requires 64-bit pointers");
#define SUBATHON_SCHEMA_TRANSPORT_V1_NOEXCEPT noexcept
extern "C" {
#else
_Static_assert(CHAR_BIT == 8, "Schema transport ABI 1 requires 8-bit bytes");
_Static_assert(sizeof(int) == 4, "Schema transport ABI 1 requires 32-bit int");
_Static_assert(sizeof(unsigned int) == 4, "Schema transport ABI 1 requires 32-bit unsigned int");
_Static_assert(sizeof(void *) == 8, "Schema transport ABI 1 requires 64-bit pointers");
#define SUBATHON_SCHEMA_TRANSPORT_V1_NOEXCEPT
#endif

// ABI 1 borrows four path buffers; completed transport releases resources before return.
__attribute__((visibility("default"))) int subathon_schema_transport_v1(
    const unsigned char *manifest, unsigned int manifest_bytes,
    const unsigned char *interpreter, unsigned int interpreter_bytes,
    const unsigned char *worker, unsigned int worker_bytes,
    const unsigned char *sdk, unsigned int sdk_bytes) SUBATHON_SCHEMA_TRANSPORT_V1_NOEXCEPT;

#ifdef __cplusplus
} // extern "C"
#endif
#undef SUBATHON_SCHEMA_TRANSPORT_V1_NOEXCEPT

#endif // SUBATHON_SCHEMA_VALIDATION_NATIVE_TRANSPORT_V1_H
