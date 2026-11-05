// SPDX-License-Identifier: GPL-3.0-or-later
// Build/link-only interface check. It is not a CTest test and is never invoked.
#include <subathon/server/reference_capture_values.hpp>
#include <subathon/server/sqlite_reference_capture.hpp>

#include <type_traits>

using namespace subathon::server;

static_assert(std::is_constructible_v<SqliteReferenceCaptureStore, SqliteWorkSession,
                                      ReferenceCaptureLimits>);
static_assert(std::is_same_v<decltype(encode_reference_configuration({}, {})), ApplicationEnvelope>);
static_assert(std::is_same_v<decltype(decode_reference_initial_capture({}, {})),
                             ReferenceInitialCapture>);

int main() {
    // Volatile address references force actual symbol resolution without calling a stub.
    [[maybe_unused]] auto *volatile encode_configuration = &encode_reference_configuration;
    [[maybe_unused]] auto *volatile decode_configuration = &decode_reference_configuration;
    [[maybe_unused]] auto *volatile encode_arrival = &encode_reference_arrival;
    [[maybe_unused]] auto *volatile decode_arrival = &decode_reference_arrival;
    [[maybe_unused]] auto *volatile encode_execution = &encode_reference_execution;
    [[maybe_unused]] auto *volatile decode_execution = &decode_reference_execution;
    [[maybe_unused]] auto *volatile encode_initial = &encode_reference_initial_capture;
    [[maybe_unused]] auto *volatile decode_initial = &decode_reference_initial_capture;
    [[maybe_unused]] auto *volatile manifest = &reference_capture_repository_manifest;
    [[maybe_unused]] auto volatile find = &SqliteReferenceCaptureStore::find;
    [[maybe_unused]] auto volatile retain = &SqliteReferenceCaptureStore::retain_initial;
    return 0;
}
