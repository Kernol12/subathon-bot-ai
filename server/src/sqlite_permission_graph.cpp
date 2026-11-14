// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_permission_graph.hpp>

namespace subathon::server {
class SqlitePermissionGraphUnit::Impl {};
PermissionGraphTrust reference_permission_graph_trust() { throw PermissionGraphNotImplemented{}; }
MigrationManifest permission_graph_repository_manifest() { throw PermissionGraphNotImplemented{}; }
SqlitePermissionGraphUnit::SqlitePermissionGraphUnit(const SqliteWorkSession &, std::string_view,
                                                     const PermissionGraphTrust &,
                                                     PermissionGraphLimits) {
    throw PermissionGraphNotImplemented{};
}
SqlitePermissionGraphUnit::~SqlitePermissionGraphUnit() = default;
PermissionGraphRead<StoredPermissionGraph> SqlitePermissionGraphUnit::load() {
    throw PermissionGraphNotImplemented{};
}
PermissionGraphRead<PermissionGraphEditFacts>
SqlitePermissionGraphUnit::describe_use_edit(const core::PermissionEditableState &) {
    throw PermissionGraphNotImplemented{};
}
PermissionGraphWrite SqlitePermissionGraphUnit::bootstrap(std::string_view,
                                                          const core::PermissionEditableState &,
                                                          const core::PermissionEditableState &) {
    throw PermissionGraphNotImplemented{};
}
PermissionGraphWrite SqlitePermissionGraphUnit::replace_use(std::uint64_t,
                                                            const core::PermissionEditableState &) {
    throw PermissionGraphNotImplemented{};
}
PermissionGraphWrite
SqlitePermissionGraphUnit::replace_grant_from_owner(std::uint64_t,
                                                    const core::PermissionEditableState &) {
    throw PermissionGraphNotImplemented{};
}
PermissionGraphWrite SqlitePermissionGraphUnit::synchronize_catalog(std::uint64_t) {
    throw PermissionGraphNotImplemented{};
}
} // namespace subathon::server
