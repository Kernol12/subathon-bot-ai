// SPDX-License-Identifier: GPL-3.0-or-later
// Test-only GNU --wrap implementation. All inactive/default paths call the selected real SDK.
#include "password_hashing_failures.hpp"

#include <sodium.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>

namespace {
struct Allocation {
    void *pointer{};
    std::size_t size{};
    bool explicitly_locked{};
    std::uint64_t lifetime{};
};
struct State {
    bool active{};
    password_test::Failure failure{};
    std::span<const std::byte> password{};
    std::string_view verifier{};
    password_test::Snapshot observed{};
    std::array<Allocation, 16> allocations{};
    std::uint64_t last_allocation_lifetime{};
    const void *first_native_password{};
    std::uint64_t first_password_lifetime{};
    bool first_password_seen{};
    bool first_password_protection_interrupted{};
    unsigned int native_depth{};
    unsigned int guard_depth{};
};
constinit thread_local State state{};

bool observing() noexcept {
    return state.active && state.native_depth == 0 && state.guard_depth == 0;
}

struct Depth final {
    unsigned int &value;
    explicit Depth(unsigned int &depth) noexcept : value(depth) { ++value; }
    ~Depth() { --value; }
};

Allocation *allocation_for(const void *pointer) noexcept {
    const auto found = std::find_if(
        state.allocations.begin(), state.allocations.end(),
        [pointer](const auto &entry) { return entry.pointer && entry.pointer == pointer; });
    return found == state.allocations.end() ? nullptr : &*found;
}

void remember(void *pointer, std::size_t size) noexcept {
    if (!pointer) {
        return;
    }
    const auto slot = std::find_if(state.allocations.begin(), state.allocations.end(),
                                   [](const auto &entry) { return entry.pointer == nullptr; });
    if (slot == state.allocations.end() ||
        state.last_allocation_lifetime == std::numeric_limits<std::uint64_t>::max()) {
        state.observed.tracking_overflow = true;
        return;
    }
    *slot = {pointer, size, false, ++state.last_allocation_lifetime};
    ++state.observed.allocations;
    ++state.observed.live_allocations;
}

void observe_password(const char *password, unsigned long long length) noexcept {
    state.observed.native_pointer_nonnull &= password != nullptr;
    const auto *allocation = allocation_for(password);
    const bool protected_input = allocation && allocation->explicitly_locked &&
                                 allocation->size >= 1 && length <= allocation->size;
    if (!state.first_password_seen) {
        state.first_password_seen = true;
        state.first_native_password = password;
        state.first_password_lifetime = allocation ? allocation->lifetime : 0;
    }
    state.observed.same_native_password_buffer &=
        allocation && state.first_password_lifetime != 0 &&
        allocation->lifetime == state.first_password_lifetime &&
        state.first_native_password == password;
    state.observed.native_pointer_protected_and_locked &=
        protected_input && !state.first_password_protection_interrupted;
    if (!protected_input || length != state.password.size()) {
        state.observed.native_input_matches = false;
        return;
    }
    state.observed.native_input_matches &=
        std::equal(state.password.begin(), state.password.end(),
                   reinterpret_cast<const std::byte *>(password));
}

void observe_record(const char *record) noexcept {
    if (!state.verifier.empty()) {
        if (!record) {
            state.observed.native_record_matches = false;
            return;
        }
        for (std::size_t i = 0; i < state.verifier.size(); ++i) {
            // Stop on a shorter valid C string instead of reading the expected length blindly.
            if (record[i] == '\0' || record[i] != state.verifier[i]) {
                state.observed.native_record_matches = false;
                return;
            }
        }
        state.observed.native_record_matches &= record[state.verifier.size()] == '\0';
    }
}

template <typename Function>
int hashing(char *output, const char *password, unsigned long long length,
            unsigned long long operations, std::size_t memory, int algorithm, Function real) {
    if (!observing()) {
        return real();
    }
    ++state.observed.hash_calls;
    state.observed.hash_operations = operations;
    state.observed.hash_memory_bytes = memory;
    state.observed.hash_algorithm = algorithm;
    observe_password(password, length);
    if (state.failure == password_test::Failure::hashing) {
        return -1;
    }
    int result{};
    {
        Depth nested(state.native_depth);
        result = real();
    }
    if (result == 0) {
        switch (state.failure) {
        case password_test::Failure::unterminated_hash_output:
            std::fill_n(output, crypto_pwhash_argon2id_STRBYTES, '#');
            break;
        case password_test::Failure::wrong_hash_profile:
            // Corrupt ONLY an actually successful real record; never produce valid fake crypto.
            output[13] = '6'; // explicit v=19 becomes v=16
            break;
        default:
            break;
        }
    }
    return result;
}

template <typename Function>
int verifying(const char *record, const char *password, unsigned long long length, Function real) {
    if (!observing()) {
        return real();
    }
    ++state.observed.verification_calls;
    observe_password(password, length);
    observe_record(record);
    if (state.failure == password_test::Failure::verification) {
        return -1;
    }
    Depth nested(state.native_depth);
    return real();
}
} // namespace

namespace password_test {
Controls::Controls(Failure failure, std::span<const std::byte> password,
                   std::string_view verifier) noexcept {
    state = {};
    state.active = true;
    state.failure = failure;
    state.password = password;
    state.verifier = verifier;
}
Controls::~Controls() { state.active = false; }
RealLibraryOnly::RealLibraryOnly() noexcept : previous_(state.active) { state.active = false; }
RealLibraryOnly::~RealLibraryOnly() { state.active = previous_; }
Snapshot snapshot() noexcept {
    auto observed = state.observed;
    const auto *allocation = allocation_for(state.first_native_password);
    observed.first_password_live = state.first_password_seen && allocation &&
                                   state.first_password_lifetime != 0 &&
                                   allocation->lifetime == state.first_password_lifetime;
    observed.first_password_locked = observed.first_password_live && allocation->explicitly_locked;
    observed.first_password_uninterrupted =
        observed.first_password_locked && !state.first_password_protection_interrupted;
    return observed;
}
} // namespace password_test

extern "C" {
int __real_sodium_init();
void *__real_sodium_malloc(std::size_t);
void *__real_sodium_allocarray(std::size_t, std::size_t);
int __real_sodium_mlock(void *, std::size_t);
int __real_sodium_munlock(void *, std::size_t);
void __real_sodium_memzero(void *, std::size_t);
void __real_sodium_free(void *);
int __real_crypto_pwhash_str(char *, const char *, unsigned long long, unsigned long long,
                             std::size_t);
int __real_crypto_pwhash_str_alg(char *, const char *, unsigned long long, unsigned long long,
                                 std::size_t, int);
int __real_crypto_pwhash_argon2id_str(char *, const char *, unsigned long long, unsigned long long,
                                      std::size_t);
int __real_crypto_pwhash_str_verify(const char *, const char *, unsigned long long);
int __real_crypto_pwhash_argon2id_str_verify(const char *, const char *, unsigned long long);

int __wrap_sodium_init() {
    if (observing()) {
        ++state.observed.initialization_calls;
        if (state.failure == password_test::Failure::initialization) {
            return -1;
        }
    }
    return __real_sodium_init();
}

void *__wrap_sodium_malloc(std::size_t size) {
    if (!observing()) {
        return __real_sodium_malloc(size);
    }
    ++state.observed.allocation_attempts;
    if (state.failure == password_test::Failure::guarded_allocation) {
        return nullptr;
    }
    Depth nested(state.guard_depth);
    void *pointer = __real_sodium_malloc(size);
    remember(pointer, size);
    return pointer;
}

void *__wrap_sodium_allocarray(std::size_t count, std::size_t size) {
    if (!observing()) {
        return __real_sodium_allocarray(count, size);
    }
    ++state.observed.allocation_attempts;
    if (state.failure == password_test::Failure::guarded_allocation) {
        return nullptr;
    }
    Depth nested(state.guard_depth);
    void *pointer = __real_sodium_allocarray(count, size);
    if (size != 0 && count > std::numeric_limits<std::size_t>::max() / size) {
        state.observed.tracking_overflow = true;
    } else {
        remember(pointer, count * size);
    }
    return pointer;
}

int __wrap_sodium_mlock(void *pointer, std::size_t size) {
    if (!observing()) {
        return __real_sodium_mlock(pointer, size);
    }
    ++state.observed.locks;
    auto *allocation = allocation_for(pointer);
    if (allocation && !allocation->explicitly_locked && !state.password.empty() &&
        allocation->size >= state.password.size()) {
        state.observed.password_copied_before_lock |= std::equal(
            state.password.begin(), state.password.end(), static_cast<const std::byte *>(pointer));
    }
    if (state.failure == password_test::Failure::explicit_lock) {
        return -1;
    }
    const int result = __real_sodium_mlock(pointer, size);
    if (allocation && result == 0 && size >= allocation->size) {
        allocation->explicitly_locked = true;
    }
    return result;
}

void __wrap_sodium_memzero(void *pointer, std::size_t size) {
    __real_sodium_memzero(pointer, size);
}
int __wrap_sodium_munlock(void *pointer, std::size_t size) {
    const int result = __real_sodium_munlock(pointer, size);
    if (observing() && result == 0 && size != 0) {
        const auto start = reinterpret_cast<std::uintptr_t>(pointer);
        for (auto &allocation : state.allocations) {
            if (!allocation.pointer || allocation.size == 0) {
                continue;
            }
            const auto base = reinterpret_cast<std::uintptr_t>(allocation.pointer);
            // Subtraction avoids address+length overflow; any real overlapping unlock breaks
            // this allocation's continuous lock observation, even if it is later relocked.
            const bool overlaps =
                start >= base ? start - base < allocation.size : base - start < size;
            if (overlaps) {
                allocation.explicitly_locked = false;
                if (state.first_password_seen &&
                    allocation.lifetime == state.first_password_lifetime) {
                    state.first_password_protection_interrupted = true;
                }
            }
        }
    }
    return result;
}

void __wrap_sodium_free(void *pointer) {
    if (observing()) {
        if (auto *allocation = allocation_for(pointer)) {
            const auto *bytes = static_cast<const unsigned char *>(pointer);
            state.observed.all_freed_buffers_were_zero &= std::all_of(
                bytes, bytes + allocation->size, [](unsigned char byte) { return byte == 0; });
            *allocation = {};
            ++state.observed.frees;
            --state.observed.live_allocations;
        }
    }
    Depth nested(state.guard_depth);
    __real_sodium_free(pointer);
}

int __wrap_crypto_pwhash_str(char *out, const char *password, unsigned long long length,
                             unsigned long long operations, std::size_t memory) {
    if (observing()) {
        ++state.observed.default_algorithm_calls;
    }
    return hashing(out, password, length, operations, memory, 0, [&] {
        return __real_crypto_pwhash_str(out, password, length, operations, memory);
    });
}
int __wrap_crypto_pwhash_str_alg(char *out, const char *password, unsigned long long length,
                                 unsigned long long operations, std::size_t memory, int algorithm) {
    return hashing(out, password, length, operations, memory, algorithm, [&] {
        return __real_crypto_pwhash_str_alg(out, password, length, operations, memory, algorithm);
    });
}
int __wrap_crypto_pwhash_argon2id_str(char *out, const char *password, unsigned long long length,
                                      unsigned long long operations, std::size_t memory) {
    return hashing(
        out, password, length, operations, memory, crypto_pwhash_argon2id_ALG_ARGON2ID13, [&] {
            return __real_crypto_pwhash_argon2id_str(out, password, length, operations, memory);
        });
}
int __wrap_crypto_pwhash_str_verify(const char *record, const char *password,
                                    unsigned long long length) {
    return verifying(record, password, length,
                     [&] { return __real_crypto_pwhash_str_verify(record, password, length); });
}
int __wrap_crypto_pwhash_argon2id_str_verify(const char *record, const char *password,
                                             unsigned long long length) {
    return verifying(record, password, length, [&] {
        return __real_crypto_pwhash_argon2id_str_verify(record, password, length);
    });
}
} // extern "C"
