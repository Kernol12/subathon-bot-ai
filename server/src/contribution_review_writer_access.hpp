// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_writer.hpp>

namespace subathon::server::detail {

// Part of the existing reconciliation profile, shared by every retained
// authority callback in a single work. Result documents may use excess;
// authority bindings must fit the existing base bytes per statement.
inline constexpr qsizetype retained_authority_attempts = 59;
inline constexpr qsizetype retained_authority_rows = 59;
inline constexpr qsizetype retained_authority_documents = 256 * 1024;

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
    class RetainedAuthorityScope {
      public:
        ~RetainedAuthorityScope();
        RetainedAuthorityScope(const RetainedAuthorityScope &) = delete;
        RetainedAuthorityScope &operator=(const RetainedAuthorityScope &) = delete;
        void verify() const;

      private:
        friend class ContributionReviewWriterAccess;
        explicit RetainedAuthorityScope(std::shared_ptr<SqliteWorkState>);
        std::shared_ptr<SqliteWorkState> state_;
    };

    static RetainedAuthorityScope retained_authority(const SqliteWorkSession &);
    static StorageLimits limits(const SqliteWriter &);
    static bool acquire(SqliteWriter &);
    static void release(SqliteWriter &) noexcept;
    static void meter(const SqliteWorkSession &, bool row_unit);
    static ReviewReadCost remaining(const SqliteWorkSession &);
};

} // namespace subathon::server::detail
