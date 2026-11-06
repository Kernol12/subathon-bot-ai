// SPDX-License-Identifier: GPL-3.0-or-later
#include "transport_core_v1_impl.h"
#include "transport_v1.h"

// Only the separately linked test artifact selects the reviewed negative policy.
// Both builds instantiate the exact same owner; production has no test state.
#if defined(SUBATHON_SCHEMA_NATIVE_FAULT_OPS)
#include "native_fault_ops.h"
using TransportPolicy = subathon::schema_validation::native::fault::FaultOps;
#else
#include "raw_ops_v1.h"
using TransportPolicy = subathon::schema_validation::native::RawOps;
#endif

extern "C" [[gnu::no_stack_protector]] int
subathon_schema_transport_v1(const unsigned char *manifest, unsigned int manifest_bytes,
                             const unsigned char *interpreter, unsigned int interpreter_bytes,
                             const unsigned char *worker, unsigned int worker_bytes,
                             const unsigned char *sdk, unsigned int sdk_bytes) noexcept {
    const subathon::schema_validation::native::Request request{{manifest, manifest_bytes},
                                                               {interpreter, interpreter_bytes},
                                                               {worker, worker_bytes},
                                                               {sdk, sdk_bytes}};
    TransportPolicy ops(request);
    return subathon::schema_validation::native::run_transport_v1(request, ops);
}
