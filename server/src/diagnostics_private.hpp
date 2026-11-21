// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QJsonObject>
#include <array>
#include <atomic>
#include <deque>
#include <mutex>
#include <subathon/server/diagnostics.hpp>

namespace subathon::server::diagnostics::detail {
constexpr Utc day_ms = 86'400'000;
constexpr Utc min_utc = -62'135'596'800'000;
constexpr Utc max_utc = 253'402'300'799'999;
constexpr std::size_t tail_limit = 512;
constexpr std::size_t registry_limit = 64;

enum class Code { serving, report_unavailable, retention_unavailable, capacity_reached };
struct Observation {
    Code code;
    std::optional<ConnectorProvider> provider;
};
struct Row {
    Utc utc;
    Observation observation;
};
struct View {
    std::deque<Row> rows;
    std::uint64_t epoch = 0;
    bool available = false;
    bool limited = false;
};
struct Snapshot {
    std::atomic<bool> abandoned = false;
    std::mutex mutex;
    std::optional<View> result;
    std::function<void()> ready;
};

[[nodiscard]] const char *name(ConnectorProvider);
[[nodiscard]] QJsonObject encode(const Row &);
[[nodiscard]] std::optional<Row> parse_row(QByteArrayView);
[[nodiscard]] std::optional<std::vector<Selector>> parse_selectors(QByteArrayView);
[[nodiscard]] QByteArray date_name(Utc);
[[nodiscard]] std::optional<Utc> system_utc() noexcept;

} // namespace subathon::server::diagnostics::detail
