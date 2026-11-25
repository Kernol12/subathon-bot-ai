// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_v23_rebuild.hpp"

namespace subathon::server::detail::testing {

class V23InspectionFixture final {
  public:
    [[nodiscard]] static std::variant<V23PreOpenObservation, MigrationReason>
    inspect(const QString &path);
};

} // namespace subathon::server::detail::testing
