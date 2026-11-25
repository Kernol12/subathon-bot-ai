// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/core/modifier_activation.hpp>
#include <subathon/server/sqlite_migrations.hpp>
#include <variant>

namespace subathon::server {

// Revision 1: docs/contracts/sqlite-modifier-activations-v1.md.
enum class SqliteModifierInputError { invalid_input };
enum class SqliteModifierStorageReason {
    stale_definition,
    stale_instance,
    identity_conflict,
    sequence_exhausted,
    invalid_stored_data
};

class SqliteModifierInputFailure final : public std::invalid_argument {
  public:
    explicit SqliteModifierInputFailure(SqliteModifierInputError error)
        : std::invalid_argument("Invalid SQLite modifier input"), error_(error) {}
    [[nodiscard]] SqliteModifierInputError error() const noexcept { return error_; }

  private:
    SqliteModifierInputError error_;
};

class SqliteModifierStorageFailure final : public std::runtime_error {
  public:
    explicit SqliteModifierStorageFailure(SqliteModifierStorageReason reason)
        : std::runtime_error("SQLite modifier operation refused"), reason_(reason) {}
    [[nodiscard]] SqliteModifierStorageReason reason() const noexcept { return reason_; }

  private:
    SqliteModifierStorageReason reason_;
};

class SqliteModifierActivationsNotImplemented final : public std::logic_error {
  public:
    explicit SqliteModifierActivationsNotImplemented(const char *entry)
        : std::logic_error(std::string("MODIFIER-001 SQLite modifier activations NI: ") + entry) {}
};

struct KeepModifierName {
    bool operator==(const KeepModifierName &) const = default;
};
struct ClearModifierName {
    bool operator==(const ClearModifierName &) const = default;
};
struct SetModifierName {
    std::string bytes;
    bool operator==(const SetModifierName &) const = default;
};
using ModifierNameOperation =
    std::variant<SetModifierName, KeepModifierName, ClearModifierName>;

struct ModifierNameEvidenceMissing {
    bool operator==(const ModifierNameEvidenceMissing &) const = default;
};
struct ModifierNameExplicitlyAbsent {
    bool operator==(const ModifierNameExplicitlyAbsent &) const = default;
};
struct ModifierNamePresent {
    std::string bytes;
    bool operator==(const ModifierNamePresent &) const = default;
};
using ModifierNameReadback =
    std::variant<ModifierNameEvidenceMissing, ModifierNameExplicitlyAbsent, ModifierNamePresent>;

enum class ModifierInstanceLifetime { live, replaced, expired };
enum class ModifierInstanceOperation { activation, checkpoint };

struct ModifierInstanceKey {
    std::string instance_ref;
    std::string revision_ref;
    bool operator==(const ModifierInstanceKey &) const = default;
};

// The embedded definition owns the logical definition ref.
struct SqliteModifierDefinitionRevision {
    core::ModifierDefinition definition;
    std::int64_t position;
    bool operator==(const SqliteModifierDefinitionRevision &) const = default;
};

// The embedded instance owns its lineage ref. Lifetime describes the lineage
// at read time, while saved remaining time describes this exact revision.
struct SqliteModifierInstanceRevision {
    core::ModifierInstance instance;
    std::int64_t position;
    ModifierInstanceOperation operation;
    std::optional<std::string> applied_definition_revision_ref;
    ModifierInstanceLifetime lifetime;
    std::optional<std::string> successor_instance_ref;
    bool operator==(const SqliteModifierInstanceRevision &) const = default;
};

[[nodiscard]] MigrationManifest modifier_activations_repository_manifest();

// Inert facade over the caller's active session. Values are owned and
// provisional until normal committed completion. An absent expected head
// requires absence. Checks see earlier staging in the same work.
// Core ModifierActivationFailure carries invalid_factor and
// missing_trigger_timing (healthy), and duration_overflow (unit). Invalid
// shapes and freshness/identity refusals are healthy. Selected corruption
// and sequence exhaustion settle the unit before rethrow. Writer failures
// retain their native classification and precedence.
class SqliteModifierActivationStore final {
  public:
    explicit SqliteModifierActivationStore(SqliteWorkSession session)
        : session_(std::move(session)) {}

    [[nodiscard]] SqliteModifierDefinitionRevision
    append_definition(const core::ModifierDefinition &,
                      const std::optional<std::string> &expected_revision) const;

    [[nodiscard]] SqliteModifierDefinitionRevision
    append_definition(const core::ModifierDefinition &,
                      const std::optional<std::string> &expected_revision,
                      const ModifierNameOperation &) const;

    [[nodiscard]] std::optional<ModifierNameReadback>
    definition_revision_name(const std::string &definition_ref,
                             const std::string &revision_ref) const;

    [[nodiscard]] core::ModifierActivationProposal
    activate(const std::string &definition_ref, const std::string &expected_definition_revision,
             const std::optional<ModifierInstanceKey> &expected_live,
             const core::ModifierActivation &) const;

    [[nodiscard]] SqliteModifierInstanceRevision
    checkpoint_remaining(const ModifierInstanceKey &expected_live, const std::string &new_revision,
                         std::int64_t observed_remaining_ms) const;

    [[nodiscard]] std::optional<SqliteModifierDefinitionRevision>
    current_definition(const std::string &definition_ref) const;

    [[nodiscard]] std::optional<SqliteModifierDefinitionRevision>
    definition_revision(const std::string &revision_ref) const;

    [[nodiscard]] std::optional<SqliteModifierInstanceRevision>
    live_instance(const std::string &definition_ref) const;

    [[nodiscard]] std::optional<SqliteModifierInstanceRevision>
    instance_revision(const std::string &revision_ref) const;

  private:
    SqliteWorkSession session_;
};
} // namespace subathon::server
