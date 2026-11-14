// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/core/delegated_permissions.hpp>

namespace subathon::core::internal {
// Internal persistence support. Caller preflights ALL reference/component sizes,
// required references, destination bounds and 16*actual numeric component size
// representability first. Borrows complete facts; no
// authorization, limit-profile validation, graph copy or symbolic cell enumeration.
void validate_permission_graph_catalog(const PermissionCatalog &);
void validate_permission_graph(const PermissionCatalog &, const PermissionEditableState &);
bool permission_graph_entry_equal(const PermissionPolicyEntry &, const PermissionPolicyEntry &);
} // namespace subathon::core::internal
