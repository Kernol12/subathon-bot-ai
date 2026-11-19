// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace credential_native {
using Nonce = std::array<unsigned char, 24>;
enum class Fault { none, readiness, allocation, lock };

// Scoped GNU --wrap observations around synchronous work on the caller's thread.
struct Observation {
    std::size_t readiness{};
    std::size_t nonce_draws{};
    std::size_t allocations{};
    std::size_t locks{};
    std::size_t releases{};
};

class Controls final {
  public:
    explicit Controls(Fault fault = Fault::none, std::size_t size = 0,
                      std::span<const Nonce> nonces = {});
    ~Controls();
    Controls(const Controls &) = delete;
    Controls &operator=(const Controls &) = delete;
    [[nodiscard]] Observation observation() const;
};

[[nodiscard]] bool prepare_sdk();
} // namespace credential_native
