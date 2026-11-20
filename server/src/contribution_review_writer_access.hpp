// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_writer.hpp>

namespace subathon::server::detail {

struct ReviewReadCost {
    qsizetype attempts{};
    qsizetype rows{};
    qsizetype document_bytes{};

    ReviewReadCost &operator+=(const ReviewReadCost &other) {
        attempts += other.attempts;
        rows += other.rows;
        document_bytes += other.document_bytes;
        return *this;
    }

    friend ReviewReadCost operator+(ReviewReadCost a, const ReviewReadCost &b) { return a += b; }

    friend ReviewReadCost operator*(qsizetype count, const ReviewReadCost &cost) {
        return {count * cost.attempts, count * cost.rows, count * cost.document_bytes};
    }

    friend ReviewReadCost operator-(ReviewReadCost a, const ReviewReadCost &b) {
        return {a.attempts - b.attempts, a.rows - b.rows, a.document_bytes - b.document_bytes};
    }
};

// An owner-thread lease serializes review orchestration on the existing writer.
// Ordinary FIFO work can still interleave between the admitted review units.
class ContributionReviewWriterAccess {
  public:
    static StorageLimits limits(const SqliteWriter &);
    static bool acquire(SqliteWriter &);
    static void release(SqliteWriter &) noexcept;
    static void meter(const SqliteWorkSession &, bool row_unit);
    static ReviewReadCost remaining(const SqliteWorkSession &);
};

} // namespace subathon::server::detail
