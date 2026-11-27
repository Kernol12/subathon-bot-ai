// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture_bounds.hpp"

#include <algorithm>
#include <limits>

namespace subathon::server::detail {
namespace {

constexpr std::uint64_t mib = 1024 * 1024;
constexpr std::uint64_t page_bytes = 4096;
constexpr std::uint64_t fixture_store_bytes = 16 * mib;
constexpr std::uint64_t fixture_max_rows = 1024;
constexpr std::uint64_t fixture_max_row_bytes = 4096;
constexpr std::uint64_t fixture_max_retained_bytes = 3 * mib;
constexpr std::uint64_t fixture_disk_bytes = 80 * mib;
constexpr std::uint64_t fixture_memory_bytes = 176 * mib;
constexpr std::uint64_t fixed_disk_reserve = 16'678'912;
constexpr std::uint64_t journal_header_reserve = 65'536;

constexpr std::uint64_t fixture_max_donor_pages = fixture_store_bytes / page_bytes;
constexpr std::uint64_t scan_passes_per_extent = 5;

constexpr std::uint64_t staging_pages(std::uint64_t donor_pages, std::uint64_t rows) {
    // Parent and unique index each charge up to two tree and two overflow
    // pages per row. Four root pages cover the empty-store case.
    return std::max(2 * donor_pages, std::max<std::uint64_t>(4, 8 * rows));
}

// Derive the fixture ceiling from the donor and row caps and the D3 staging
// charge. The per-row and aggregate byte caps remain separate admission gates.
constexpr std::uint64_t fixture_max_successor_pages =
    fixture_max_donor_pages + staging_pages(fixture_max_donor_pages, fixture_max_rows);
constexpr std::uint64_t v23_fixture_max_scan_pages =
    scan_passes_per_extent * (fixture_max_donor_pages + fixture_max_successor_pages);

} // namespace

bool v23_fixture_budget_within_ceiling(V23FixtureBudget budget) noexcept {
    return budget.disk_bytes.value_or(fixture_disk_bytes) <= fixture_disk_bytes &&
           budget.memory_bytes.value_or(fixture_memory_bytes) <= fixture_memory_bytes &&
           budget.scan_pages.value_or(v23_fixture_max_scan_pages) <= v23_fixture_max_scan_pages;
}

std::optional<std::uint64_t> estimate_v23_fixture_scan_pages(
    std::uint64_t donor_pages, std::uint64_t successor_pages) noexcept {
    constexpr auto max_pages = std::numeric_limits<std::uint64_t>::max();
    if (successor_pages > max_pages - donor_pages)
        return std::nullopt;
    const auto extent = donor_pages + successor_pages;
    if (extent > max_pages / scan_passes_per_extent)
        return std::nullopt;
    return scan_passes_per_extent * extent;
}

std::optional<V23FixtureEstimate> admit_v23_fixture_bounds(const V23FixtureFacts &facts,
                                                           V23FixtureBudget budget) noexcept {
    const auto disk_limit = budget.disk_bytes.value_or(fixture_disk_bytes);
    const auto memory_limit = budget.memory_bytes.value_or(fixture_memory_bytes);
    if (!v23_fixture_budget_within_ceiling(budget) ||
        !facts.free_bytes || facts.page_size != page_bytes || facts.auto_vacuum != 0 ||
        facts.page_count == 0 || facts.page_count > fixture_max_donor_pages ||
        facts.row_count > fixture_max_rows || facts.largest_row_bytes > fixture_max_row_bytes ||
        facts.retained_bytes > fixture_max_retained_bytes)
        return std::nullopt;

    // Page and row caps above bound every term except the measured sidecars.
    const auto old_store = facts.page_count * page_bytes;
    const auto journal = journal_header_reserve + facts.page_count * (page_bytes + 8);
    const auto added_pages = staging_pages(facts.page_count, facts.row_count);
    if (added_pages > std::numeric_limits<std::uint64_t>::max() - facts.page_count)
        return std::nullopt;
    const auto successor_pages = facts.page_count + added_pages;
    // Donor sizing, integrity, FK, copy and comparison charge the donor;
    // reverse comparison and both integrity/FK pairs charge the successor.
    const auto scan_pages = estimate_v23_fixture_scan_pages(facts.page_count, successor_pages);
    if (!scan_pages || *scan_pages > budget.scan_pages.value_or(v23_fixture_max_scan_pages))
        return std::nullopt;

    const auto staging = added_pages * page_bytes;
    const auto disk_without_sidecars = old_store + journal + staging + fixed_disk_reserve;
    if (facts.sidecar_bytes > std::numeric_limits<std::uint64_t>::max() - disk_without_sidecars)
        return std::nullopt;
    const auto disk = disk_without_sidecars + facts.sidecar_bytes;
    if (disk > std::min(disk_limit, *facts.free_bytes))
        return std::nullopt;

    const auto dirty_payload = old_store + staging;
    const auto memory = 2 * (staging + dirty_payload) + 16 * mib;
    if (memory > memory_limit)
        return std::nullopt;

    return V23FixtureEstimate{disk, journal, memory};
}

} // namespace subathon::server::detail
