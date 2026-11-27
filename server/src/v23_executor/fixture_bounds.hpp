// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>

namespace subathon::server::detail {

struct V23FixtureFacts {
    std::uint64_t page_count;
    std::uint64_t page_size;
    std::uint64_t auto_vacuum;
    std::uint64_t row_count;
    std::uint64_t largest_row_bytes;
    std::uint64_t retained_bytes;
    std::uint64_t sidecar_bytes;
    std::optional<std::uint64_t> free_bytes;
};

struct V23FixtureBudget {
    std::optional<std::uint64_t> disk_bytes;
    std::optional<std::uint64_t> memory_bytes;
    std::optional<std::uint64_t> scan_pages{};
};

struct V23FixtureEstimate {
    std::uint64_t disk_bytes;
    std::uint64_t journal_bytes;
    std::uint64_t memory_bytes;
};

[[nodiscard]] bool v23_fixture_budget_within_ceiling(V23FixtureBudget budget) noexcept;

[[nodiscard]] std::optional<std::uint64_t>
estimate_v23_fixture_scan_pages(std::uint64_t donor_pages, std::uint64_t successor_pages) noexcept;

// The owner supplies observed facts from its authenticated stopped donor.
// An absent estimate refuses the fixture prior to any connection control.
[[nodiscard]] std::optional<V23FixtureEstimate>
admit_v23_fixture_bounds(const V23FixtureFacts &facts, V23FixtureBudget budget) noexcept;

} // namespace subathon::server::detail
