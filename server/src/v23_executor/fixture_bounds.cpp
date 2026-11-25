// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture_bounds.hpp"

#include <algorithm>
#include <limits>

namespace subathon::server::detail {
namespace {

constexpr std::uint64_t mib = 1024 * 1024;
constexpr std::uint64_t page_bytes = 4096;
constexpr std::uint64_t fixture_store_bytes = 16 * mib;
constexpr std::uint64_t fixture_disk_bytes = 80 * mib;
constexpr std::uint64_t fixture_memory_bytes = 176 * mib;
constexpr std::uint64_t fixed_disk_reserve = 16'678'912;
constexpr std::uint64_t journal_header_reserve = 65'536;

} // namespace

std::optional<std::uint64_t> estimate_v23_fixture_scan_pages(std::uint64_t,
                                                             std::uint64_t) noexcept {
    return std::nullopt;
}

std::optional<V23FixtureEstimate> admit_v23_fixture_bounds(const V23FixtureFacts &facts,
                                                           V23FixtureBudget budget) noexcept {
    const auto disk_limit = budget.disk_bytes.value_or(fixture_disk_bytes);
    const auto memory_limit = budget.memory_bytes.value_or(fixture_memory_bytes);
    if (disk_limit > fixture_disk_bytes || memory_limit > fixture_memory_bytes ||
        !facts.free_bytes || facts.page_size != page_bytes || facts.auto_vacuum != 0 ||
        facts.page_count == 0 || facts.page_count > fixture_store_bytes / page_bytes ||
        facts.row_count > 1024 || facts.largest_row_bytes > 4096 || facts.retained_bytes > 3 * mib)
        return std::nullopt;

    // Page and row caps above bound every term except the measured sidecars.
    const auto old_store = facts.page_count * page_bytes;
    const auto journal = journal_header_reserve + facts.page_count * (page_bytes + 8);
    // Parent and unique index each charge up to two tree and two overflow
    // pages per row. Four root pages cover the empty-store case.
    const auto settled_pages = std::max<std::uint64_t>(4, 8 * facts.row_count);
    const auto staging = std::max(2 * old_store, settled_pages * page_bytes);
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
