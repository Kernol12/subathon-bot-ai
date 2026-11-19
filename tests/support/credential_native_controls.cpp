// SPDX-License-Identifier: GPL-3.0-or-later
#include "credential_native_controls.hpp"

#include <sodium.h>

#include <cstdlib>
#include <cstring>

namespace {
using namespace credential_native;
struct State {
    bool active{};
    bool ready{};
    bool initializing{};
    Fault fault{};
    std::size_t size{};
    std::span<const Nonce> nonces;
    Observation observation;
    void *tracked{};
};
constinit thread_local State state;
} // namespace

namespace credential_native {
bool prepare_sdk() { return sodium_init() >= 0; }

Controls::Controls(Fault fault, std::size_t size, std::span<const Nonce> nonces) {
    if (state.active || (!nonces.empty() && !state.ready))
        std::abort();
    state = {true, state.ready, false, fault, size, nonces, {}, nullptr};
}

Controls::~Controls() { state.active = false; }
Observation Controls::observation() const { return state.observation; }
} // namespace credential_native

extern "C" {
int __real_sodium_init();
void __real_randombytes_buf(void *, std::size_t);
void *__real_sodium_malloc(std::size_t);
int __real_sodium_mlock(void *, std::size_t);
void __real_sodium_free(void *);

int __wrap_sodium_init() {
    state.initializing = true;
    const int result = __real_sodium_init();
    state.initializing = false;
    state.ready = result >= 0;
    if (state.active) {
        ++state.observation.readiness;
        if (state.fault == Fault::readiness && state.ready)
            return -1;
    }
    return result;
}

void __wrap_randombytes_buf(void *output, std::size_t size) {
    if (state.active && state.ready && !state.initializing && size == Nonce{}.size()) {
        const auto index = state.observation.nonce_draws++;
        if (index < state.nonces.size()) {
            std::memcpy(output, state.nonces[index].data(), size);
            return;
        }
    }
    __real_randombytes_buf(output, size);
}

void *__wrap_sodium_malloc(std::size_t size) {
    if (!state.active || state.size != size)
        return __real_sodium_malloc(size);

    ++state.observation.allocations;
    if (state.fault == Fault::allocation)
        return nullptr;
    auto *allocated = __real_sodium_malloc(size);
    state.tracked = allocated;
    return allocated;
}

int __wrap_sodium_mlock(void *data, std::size_t size) {
    // Observe the explicit lock of the returned allocation only, excluding
    // allocator internals and unrelated memory of the same size.
    if (state.active && data && data == state.tracked && size == state.size) {
        ++state.observation.locks;
        if (state.fault == Fault::lock)
            return -1;
    }
    return __real_sodium_mlock(data, size);
}

void __wrap_sodium_free(void *data) {
    if (state.active && data && data == state.tracked) {
        state.tracked = nullptr;
        ++state.observation.releases;
    }
    __real_sodium_free(data);
}
}
