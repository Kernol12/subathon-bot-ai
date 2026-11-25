// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <functional>
#include <string_view>
#include <subathon/server/sqlite_rows.hpp>

namespace subathon::server::detail {

// The fenced owner provides its metered reader. Only the two compiled probes
// reach it. Reader failures propagate without being classified as mismatches.
void require_v23_copy_match(const std::function<SqlRows(std::string_view)> &read);

} // namespace subathon::server::detail
