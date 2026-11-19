// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_modifier_activations.hpp>

namespace subathon::server {
MigrationManifest modifier_activations_repository_manifest() {
    throw SqliteModifierActivationsNotImplemented("modifier_activations_repository_manifest");
}

SqliteModifierDefinitionRevision
SqliteModifierActivationStore::append_definition(const core::ModifierDefinition &,
                                                 const std::optional<std::string> &) const {
    throw SqliteModifierActivationsNotImplemented("append_definition");
}

core::ModifierActivationProposal
SqliteModifierActivationStore::activate(const std::string &, const std::string &,
                                        const std::optional<ModifierInstanceKey> &,
                                        const core::ModifierActivation &) const {
    throw SqliteModifierActivationsNotImplemented("activate");
}

SqliteModifierInstanceRevision
SqliteModifierActivationStore::checkpoint_remaining(const ModifierInstanceKey &,
                                                    const std::string &, std::int64_t) const {
    throw SqliteModifierActivationsNotImplemented("checkpoint_remaining");
}

std::optional<SqliteModifierDefinitionRevision>
SqliteModifierActivationStore::current_definition(const std::string &) const {
    throw SqliteModifierActivationsNotImplemented("current_definition");
}

std::optional<SqliteModifierDefinitionRevision>
SqliteModifierActivationStore::definition_revision(const std::string &) const {
    throw SqliteModifierActivationsNotImplemented("definition_revision");
}

std::optional<SqliteModifierInstanceRevision>
SqliteModifierActivationStore::live_instance(const std::string &) const {
    throw SqliteModifierActivationsNotImplemented("live_instance");
}

std::optional<SqliteModifierInstanceRevision>
SqliteModifierActivationStore::instance_revision(const std::string &) const {
    throw SqliteModifierActivationsNotImplemented("instance_revision");
}
} // namespace subathon::server
