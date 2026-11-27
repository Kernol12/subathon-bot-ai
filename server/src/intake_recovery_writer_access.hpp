// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_writer.hpp>

namespace subathon::server::detail {

struct IntakeRecoveryRemaining {
    qsizetype statements, bound_bytes, result_rows, result_bytes;
};

class IntakeRecoveryWriterAccess {
  public:
    static std::optional<StorageLimits> limits(const SqliteWriter &);
    static StorageLimits limits(const SqliteWorkSession &);
    static IntakeRecoveryRemaining remaining(const SqliteWorkSession &);
};

} // namespace subathon::server::detail
