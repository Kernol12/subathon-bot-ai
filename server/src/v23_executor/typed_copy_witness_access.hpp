// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class QSqlDatabase;

namespace subathon::server::detail::testing {

// Linked solely into the disposable negative fixture target.
void require_v23_typed_fixture_copy_match(const QSqlDatabase &database);

} // namespace subathon::server::detail::testing
