// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/reference_capture_values.hpp>
#include <subathon/server/sqlite_permission_graph.hpp>
#include <subathon/server/sqlite_reference_capture.hpp>

namespace subathon::server {
// Contract v1: reference-authority-v1.md. Internal owned DATA; never a permit.
enum class ReferenceAuthorityUse : std::uint8_t {
    execute_time_add = 0,
    disclose_application = 1,
    disclose_holds = 2
};
enum class ReferenceAuthorityState : std::uint8_t { pending = 0, authorized = 1, revoked = 2 };
enum class ReferenceAuthorityFailure : std::uint8_t {
    denied = 0,
    missing = 1,
    invalid_data = 2,
    unavailable = 3,
    resource_limit = 4,
    stale = 5,
    collision = 6
};
// Source is a scope, not an event identity. Every captured event is checked against
// its actual immutable capture separately. All binding fields are immutable.
struct ReferenceAuthorityBinding {
    std::string installation, capability, principal;
    std::string rule, rule_version, action, action_version, timer;
    core::ReceiptOrigin origin;
    std::string identifier_namespace;
    std::int64_t requested_ms = 0;
};
struct ReferenceAuthorityRecord {
    ReferenceAuthorityBinding binding;
    std::string original_author, execution_account;
    std::uint64_t created_revision = 0, state_revision = 0, head_revision = 0;
    ReferenceAuthorityState state = ReferenceAuthorityState::pending;
    // Provenance of CURRENT lifecycle event; immutable initial event remains SQL.
    std::string actor, session, session_revision;
    std::int64_t authenticated_utc_ms = 0;
};
struct ReferenceAuthorityRequest {
    ReferenceAuthorityUse use = ReferenceAuthorityUse::execute_time_add;
    ApplicationPrincipalBinding principal;
    ReferenceExecutionBinding captured;
    ApplicationOperationKey requested_operation;
    std::optional<ApplicationOperationKey> historical_operation;
    std::optional<std::string> historical_hold;
};
struct ReferenceAuthorityFacts {
    ReferenceAuthorityRequest binding;
    ReferenceAuthorityRecord registry;
    std::string permission_revision_ref, automation_revision_ref;
    // Original author, BOTH current channels, ONE full concrete action capability.
    core::PermissionAutomationInput original_author_input;
    // Separate actual execution-account policy and concrete request. Never intersect
    // policies or replace the actor with principal_ref. Consumer evaluates BOTH.
    core::EffectivePermissionInput permission_input;
};
struct ReferenceAuthorityResolution {
    std::variant<ReferenceAuthorityFacts, ReferenceAuthorityFailure> value;
};
struct ReferenceAuthorityStaged {
    std::uint64_t revision = 0;
};
using ReferenceAuthorityWrite = std::variant<ReferenceAuthorityStaged, ReferenceAuthorityFailure>;
using ReferenceAuthorityRead = std::variant<ReferenceAuthorityRecord, ReferenceAuthorityFailure>;
struct ReferenceAuthorityLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t input_bytes = 1048576;
    std::uint64_t registry_selected_bytes = 1048576;
    std::uint64_t mutation_bytes = 1048576;
    std::uint64_t result_bytes = 16777216;
    std::uint64_t result_items = 16384;
    PermissionGraphLimits graph{};
    AccountSessionStoreLimits accounts{};
    SessionLimits sessions{};
    ReferenceCaptureLimits captures{};
    core::EffectivePermissionLimits permissions{};
};
class ReferenceAuthorityInputFailure final : public std::invalid_argument {
  public:
    explicit ReferenceAuthorityInputFailure(ReferenceAuthorityFailure code)
        : std::invalid_argument("Reference authority input rejected"), code_(code) {}
    [[nodiscard]] ReferenceAuthorityFailure code() const noexcept { return code_; }

  private:
    ReferenceAuthorityFailure code_;
};
class ReferenceAuthorityNotImplemented final : public std::logic_error {
  public:
    ReferenceAuthorityNotImplemented()
        : std::logic_error("Reference authority intentionally unimplemented; review tests first") {}
};
// Add step10 to the ACTUAL compiled prefix1-9, preserving all prior bytes.
[[nodiscard]] MigrationManifest reference_authority_repository_manifest();
// No thread/connection/transaction ownership. Each operation checks the borrowed
// ticket, preflights inputs before copies, loads actual current rows anew. Original
// SqliteWorkFailure always propagates; no sticky error cache or automatic retry.
class SqliteReferenceAuthorityUnit final {
  public:
    explicit SqliteReferenceAuthorityUnit(const SqliteWorkSession &, ReferenceAuthorityLimits = {});
    ~SqliteReferenceAuthorityUnit();
    SqliteReferenceAuthorityUnit(const SqliteReferenceAuthorityUnit &) = delete;
    SqliteReferenceAuthorityUnit &operator=(const SqliteReferenceAuthorityUnit &) = delete;
    // Fresh real session resolution inside this same write scope. Clock is a trusted
    // owner composition dependency, not transport input. No resolved-session overload.
    [[nodiscard]] ReferenceAuthorityWrite bootstrap(const SessionLookupRequest &,
                                                    CurrentSessionClock &);
    // Author is derived from authentication. Enrollment is pending, never implicit
    // execution. management overrides.edit(execution_account) + whole USE+GRANT.
    [[nodiscard]] ReferenceAuthorityWrite enroll(const SessionLookupRequest &,
                                                 CurrentSessionClock &, std::uint64_t expected_head,
                                                 const ReferenceAuthorityBinding &,
                                                 std::string_view execution_account);
    // Only freshly authenticated CURRENT owner, management access.manage plus GRANT
    // for all three immutable capabilities, including revocation. No implicit bypass.
    [[nodiscard]] ReferenceAuthorityWrite
    set_state(const SessionLookupRequest &, CurrentSessionClock &, std::uint64_t expected_head,
              std::string_view capability, ReferenceAuthorityState);
    [[nodiscard]] ReferenceAuthorityRead read(std::string_view installation,
                                              std::string_view capability);
    [[nodiscard]] ReferenceAuthorityResolution resolve(const ReferenceAuthorityRequest &);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
// Default producer delegates ONLY to the finite callback view. It cannot retain
// a session or execute SQL, and neither it nor a consumer may cache a permit.
class ReferenceAuthoritySource {
  public:
    virtual ~ReferenceAuthoritySource() = default;
    [[nodiscard]] virtual ReferenceAuthorityResolution
    resolve(const ApplicationReferenceReadView &, const ReferenceAuthorityRequest &) = 0;
};
class SqliteReferenceAuthoritySource final : public ReferenceAuthoritySource {
  public:
    explicit SqliteReferenceAuthoritySource(ReferenceAuthorityLimits = {});
    [[nodiscard]] ReferenceAuthorityResolution resolve(const ApplicationReferenceReadView &,
                                                       const ReferenceAuthorityRequest &) override;

  private:
    ReferenceAuthorityLimits limits_;
};
} // namespace subathon::server
