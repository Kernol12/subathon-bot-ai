// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace password_test {

// Private Linux linker controls; no production API, successful-crypto fake or readiness override.
enum class Failure {
    none,
    initialization,
    guarded_allocation,
    explicit_lock,
    verification,
    hashing,
    unterminated_hash_output,
    wrong_hash_profile
};

struct Snapshot {
    unsigned int initialization_calls{};
    unsigned int allocation_attempts{};
    unsigned int allocations{};
    unsigned int locks{};
    unsigned int frees{};
    unsigned int live_allocations{};
    unsigned int verification_calls{};
    unsigned int hash_calls{};
    unsigned int default_algorithm_calls{};
    unsigned long long hash_operations{};
    std::size_t hash_memory_bytes{};
    int hash_algorithm{};
    bool native_pointer_nonnull{true};
    bool native_pointer_protected_and_locked{true};
    bool native_input_matches{true};
    bool native_record_matches{true};
    bool same_native_password_buffer{true};
    bool password_copied_before_lock{};
    bool all_freed_buffers_were_zero{true};
    bool tracking_overflow{};
};

// Construct only inside a test body; borrowed public synthetic inputs outlive this scope.
class Controls final {
  public:
    Controls(Failure failure, std::span<const std::byte> password,
             std::string_view verifier = {}) noexcept;
    Controls(const Controls &) = delete;
    Controls &operator=(const Controls &) = delete;
    ~Controls();
};

// Suspend observations/injections while a returned SUT artifact is checked by the real SDK.
class RealLibraryOnly final {
  public:
    RealLibraryOnly() noexcept;
    RealLibraryOnly(const RealLibraryOnly &) = delete;
    RealLibraryOnly &operator=(const RealLibraryOnly &) = delete;
    ~RealLibraryOnly();

  private:
    bool previous_;
};

[[nodiscard]] Snapshot snapshot() noexcept;

} // namespace password_test
