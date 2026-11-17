// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_reference_authority.hpp>
#include <algorithm>
#include <tuple>
namespace subathon::server {
MigrationManifest reference_authority_repository_manifest() {
    auto result = permission_graph_repository_manifest();
    MigrationStep s;
    s.version = 10;
    s.id = "reference-authority-v1";
    s.sql = {
        "CREATE TABLE subathon_reference_authority_head(installation_ref BLOB NOT NULL PRIMARY KEY "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), revision BLOB NOT "
        "NULL "
        "CHECK(typeof(revision)='blob' AND length(revision)=8 AND revision<>X'0000000000000000'), "
        "FOREIGN KEY(installation_ref) REFERENCES subathon_account_installation(installation_ref))",
        "CREATE TABLE subathon_reference_authority_bindings(installation_ref BLOB NOT NULL "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), capability_ref "
        "BLOB NOT NULL "
        "CHECK(typeof(capability_ref)='blob' AND length(capability_ref)>0), principal_ref BLOB NOT "
        "NULL "
        "CHECK(typeof(principal_ref)='blob' AND length(principal_ref)>0), rule_ref BLOB NOT NULL "
        "CHECK(typeof(rule_ref)='blob' AND length(rule_ref)>0), rule_version_ref BLOB NOT NULL "
        "CHECK(typeof(rule_version_ref)='blob' AND length(rule_version_ref)>0), action_ref BLOB "
        "NOT NULL "
        "CHECK(typeof(action_ref)='blob' AND length(action_ref)>0), action_version_ref BLOB NOT "
        "NULL "
        "CHECK(typeof(action_version_ref)='blob' AND length(action_version_ref)>0), timer_ref BLOB "
        "NOT NULL "
        "CHECK(typeof(timer_ref)='blob' AND length(timer_ref)>0), provider BLOB NOT NULL "
        "CHECK(typeof(provider)='blob' AND length(provider)>0), source_account BLOB NOT NULL "
        "CHECK(typeof(source_account)='blob' AND length(source_account)>0), identifier_namespace "
        "BLOB NOT NULL "
        "CHECK(typeof(identifier_namespace)='blob' AND length(identifier_namespace)>0), "
        "requested_ms INTEGER NOT NULL "
        "CHECK(typeof(requested_ms)='integer' AND requested_ms>=0), original_author BLOB NOT NULL "
        "CHECK(typeof(original_author)='blob' AND length(original_author)>0), execution_account "
        "BLOB NOT NULL "
        "CHECK(typeof(execution_account)='blob' AND length(execution_account)>0), created_revision "
        "BLOB NOT NULL "
        "CHECK(typeof(created_revision)='blob' AND length(created_revision)=8 AND "
        "created_revision<>X'0000000000000000'), "
        "PRIMARY KEY(installation_ref,capability_ref), FOREIGN KEY(installation_ref) REFERENCES "
        "subathon_reference_authority_head(installation_ref), FOREIGN "
        "KEY(installation_ref,original_author) "
        "REFERENCES subathon_accounts(installation_ref,account_ref), FOREIGN "
        "KEY(installation_ref,execution_account) "
        "REFERENCES subathon_accounts(installation_ref,account_ref))",
        "CREATE TABLE subathon_reference_authority_events(installation_ref BLOB NOT NULL "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), capability_ref "
        "BLOB NOT NULL "
        "CHECK(typeof(capability_ref)='blob' AND length(capability_ref)>0), revision BLOB NOT NULL "
        "CHECK(typeof(revision)='blob' AND length(revision)=8 AND revision<>X'0000000000000000'), "
        "state INTEGER NOT NULL "
        "CHECK(typeof(state)='integer' AND state IN (0,1,2)), actor_ref BLOB NOT NULL "
        "CHECK(typeof(actor_ref)='blob' AND length(actor_ref)>0), session_ref BLOB NOT NULL "
        "CHECK(typeof(session_ref)='blob' AND length(session_ref)>0), session_revision_ref BLOB "
        "NOT NULL "
        "CHECK(typeof(session_revision_ref)='blob' AND length(session_revision_ref)>0), "
        "authenticated_utc_ms INTEGER NOT NULL "
        "CHECK(typeof(authenticated_utc_ms)='integer'), PRIMARY KEY(installation_ref,revision), "
        "FOREIGN KEY(installation_ref,capability_ref) REFERENCES "
        "subathon_reference_authority_bindings(installation_ref,capability_ref), FOREIGN "
        "KEY(installation_ref,actor_ref) "
        "REFERENCES subathon_accounts(installation_ref,account_ref))",
        "CREATE INDEX subathon_reference_authority_events_capability_idx ON "
        "subathon_reference_authority_events(installation_ref,capability_ref,revision)"};
    s.expected_schema = result.steps.back().expected_schema;
    const QStringList names{"subathon_reference_authority_head",
                            "subathon_reference_authority_bindings",
                            "subathon_reference_authority_events"};
    for (qsizetype i = 0; i < names.size(); ++i) {
        s.expected_schema.append({"table", names[i], names[i], s.sql[i]});
        s.expected_schema.append(
            {"index", "sqlite_autoindex_" + names[i] + "_1", names[i], std::nullopt});
    }
    s.expected_schema.append({"index", "subathon_reference_authority_events_capability_idx",
                              "subathon_reference_authority_events", s.sql[3]});
    std::sort(s.expected_schema.begin(), s.expected_schema.end(), [](const auto &a, const auto &b) {
        return std::tie(a.type, a.name) < std::tie(b.type, b.name);
    });
    result.steps.append(std::move(s));
    result.supported_starting_versions.append(10);
    return result;
}
}
