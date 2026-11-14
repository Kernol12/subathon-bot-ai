// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <subathon/core/delegated_permissions.hpp>
#include <subathon/server/sqlite_account_sessions.hpp>
#include <variant>
#include <vector>

namespace subathon::server {
// Internal physical boundary v1: docs/contracts/sqlite-permission-graph-v1.md.
// These facts/operations are NOT authentication or an authorized-save capability.
enum class PermissionGraphFailure : std::uint8_t {
    missing,
    unavailable,
    invalid_data,
    resource_limit
};
template <class T> using PermissionGraphRead = std::variant<T, PermissionGraphFailure>;
enum class PermissionGraphInputError : std::uint8_t {
    invalid_limits,
    invalid_input,
    resource_limit
};
class PermissionGraphInputFailure final : public std::runtime_error {
  public:
    explicit PermissionGraphInputFailure(PermissionGraphInputError code)
        : std::runtime_error("Permission graph input rejected"), code_(code) {}
    [[nodiscard]] PermissionGraphInputError code() const noexcept { return code_; }

  private:
    PermissionGraphInputError code_;
};
class PermissionGraphStorageFailure final : public std::runtime_error {
  public:
    explicit PermissionGraphStorageFailure(PermissionGraphFailure code)
        : std::runtime_error("Permission graph storage unavailable"), code_(code) {}
    [[nodiscard]] PermissionGraphFailure code() const noexcept { return code_; }

  private:
    PermissionGraphFailure code_;
};
struct PermissionGraphLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t component_bytes = 16384;
    std::uint64_t rows = 4096;   // Total selected physical rows, including account/install closure.
    std::uint64_t items = 16384; // All vector elements, recursively, including codec counts.
    std::uint64_t selected_bytes = 16777216;
    std::uint64_t mutation_bytes = 16777216;
};
// A trusted server composition dependency, never decoded from an edit request.
// Mapping targets bind a role/account identity into this catalog's resource slot.
// The catalog operation must have exactly that one resource slot, no quantities.
struct PermissionManagementBinding {
    std::string operation;
    std::string target_slot;
};
struct PermissionGraphTrust {
    core::PermissionCatalog catalog;
    PermissionManagementBinding role_edit;
    PermissionManagementBinding membership_edit;
    PermissionManagementBinding override_edit;
};
// Compiled exact CONTRACT-014 reference.*.v1 profile for #649, plus
// CONTRACT-013 timer.snapshot.read,
// graph-management and protected access/recovery/helper classifications. Additions
// require a new catalog revision; unsupported operations stay unsupported. This is
// not a claim that the full product operation inventory has been implemented.
[[nodiscard]] PermissionGraphTrust reference_permission_graph_trust();
struct StoredPermissionGraph {
    std::string installation;
    std::uint64_t revision = 0; // Monotonic, nonzero; never caller-selected, never wraps.
    core::PermissionOwnerFacts owner;
    core::PermissionCatalog catalog;
    core::PermissionEditableState use;
    core::PermissionEditableState grant; // Separate owner-configured GRANT graph.
    StoredInstallationFacts current_installation;
    std::vector<StoredAccountFacts> current_accounts;
};
struct PermissionGraphEditFacts {
    StoredPermissionGraph before;
    core::PermissionEditableState after;
    // Derived from ALL changed roles, memberships and overrides; sorted/deduplicated
    // by (operation,target_slot,target) unsigned bytes. No actor or permit supplied.
    std::vector<core::PermissionRequest> management;
};
enum class PermissionGraphWrite : std::uint8_t { staged, stale, collision };
class PermissionGraphNotImplemented final : public std::logic_error {
  public:
    PermissionGraphNotImplemented()
        : std::logic_error(
              "Permission graph storage intentionally unimplemented; review tests first") {}
};
// Coordinator integrates this additive step9 on the ACTUAL compiled prefix1-8.
[[nodiscard]] MigrationManifest permission_graph_repository_manifest();

// Borrows the existing revocable session, owns no connection/thread/transaction.
// Trust is borrowed immutable for the unit lifetime; installation is copied only
// after complete preflight. Each operation rechecks the ticket and current SQL.
// SqliteWorkFailure is preserved (including wrong_thread/stale_session/sticky errors).
// Read variants distinguish absence, invalid data and bounded refusal. Mutations
// throw PermissionGraphStorageFailure for those source failures; never call them stale.
// Constructor/input errors precede SQL; no borrowed mutation is copied before bounds.
class SqlitePermissionGraphUnit final {
  public:
    SqlitePermissionGraphUnit(const SqliteWorkSession &, std::string_view installation,
                              const PermissionGraphTrust &, PermissionGraphLimits = {});
    ~SqlitePermissionGraphUnit();
    SqlitePermissionGraphUnit(const SqlitePermissionGraphUnit &) = delete;
    SqlitePermissionGraphUnit &operator=(const SqlitePermissionGraphUnit &) = delete;
    [[nodiscard]] PermissionGraphRead<StoredPermissionGraph> load();
    [[nodiscard]] PermissionGraphRead<PermissionGraphEditFacts>
    describe_use_edit(const core::PermissionEditableState &after);
    // Trusted setup primitive: absent head only, existing account/install parents,
    // present owner. No setup code, credentials or login are created here.
    [[nodiscard]] PermissionGraphWrite bootstrap(std::string_view owner,
                                                 const core::PermissionEditableState &use,
                                                 const core::PermissionEditableState &grant);
    // Whole graph CAS. A successful no-op still increments the head. Replaying an
    // old expected revision is stale, with ZERO writes; no automatic retry/merge.
    // Only USE changes here: owner, catalog and GRANT are retained exactly.
    [[nodiscard]] PermissionGraphWrite replace_use(std::uint64_t expected,
                                                   const core::PermissionEditableState &);
    // Protected internal owner-service primitive; caller MUST authenticate owner
    // separately. Never exposed as an ordinary delegation mutation or permit.
    [[nodiscard]] PermissionGraphWrite
    replace_grant_from_owner(std::uint64_t expected, const core::PermissionEditableState &);
    // Trusted deployment/catalog owner, not user input. Revalidates both graphs
    // against constructor trust and stages its exact catalog+mapping at a new head.
    [[nodiscard]] PermissionGraphWrite synchronize_catalog(std::uint64_t expected);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
// Staged results and same-session reads are provisional. The transaction owner
// confirms only after real commit AND normal SqliteWorkResult completion/value.
// ACCESS-003 binds actor from authenticated current session, calculates management
// USE + GRANT for every derived intent and full impact closure, rechecks account/
// install facts and CAS in this same work. This raw store supplies no auth token.
// ACCESS-011 may use load() within that serialized work to bind original author
// and execution subject to current account/owner/catalog/USE/GRANT/head facts.
// Capability registry, revocation revision and finite callback read-view are #649,
// not supplied or claimed by this interface.
} // namespace subathon::server
