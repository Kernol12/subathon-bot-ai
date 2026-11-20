// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_writer.hpp>

namespace subathon::server::detail {

// An owner-thread lease serializes review orchestration on the existing writer.
// Ordinary FIFO work can still interleave between the admitted review units.
class ContributionReviewWriterAccess {
  public:
    static StorageLimits limits(const SqliteWriter &);
    static bool acquire(SqliteWriter &);
    static void release(SqliteWriter &) noexcept;
    static void meter(const SqliteWorkSession &, bool row_unit);
};

} // namespace subathon::server::detail
