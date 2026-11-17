// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_permission_graph.hpp>
#include <algorithm>
#include <tuple>
namespace subathon::server {
MigrationManifest permission_graph_repository_manifest() {
    auto result = account_session_repository_manifest();
    MigrationStep step;
    step.version = 9;
    step.id = "permission-graph-v1";
    step.sql = {
        "CREATE TABLE subathon_permission_head(installation_ref BLOB NOT NULL PRIMARY KEY "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), revision BLOB NOT "
        "NULL CHECK(typeof(revision)='blob' AND length(revision)=8 AND "
        "revision<>X'0000000000000000'), owner_ref BLOB NOT NULL CHECK(typeof(owner_ref)='blob' "
        "AND length(owner_ref)>0), catalog BLOB NOT NULL CHECK(typeof(catalog)='blob' AND "
        "length(catalog)>0), FOREIGN KEY(installation_ref) REFERENCES "
        "subathon_account_installation(installation_ref), FOREIGN KEY(installation_ref,owner_ref) "
        "REFERENCES subathon_accounts(installation_ref,account_ref))",
        "CREATE TABLE subathon_permission_roles(installation_ref BLOB NOT NULL "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), channel INTEGER "
        "NOT NULL CHECK(typeof(channel)='integer' AND channel IN (0,1)), role_ref BLOB NOT NULL "
        "CHECK(typeof(role_ref)='blob' AND length(role_ref)>0), entries BLOB NOT NULL "
        "CHECK(typeof(entries)='blob' AND length(entries)>=8), PRIMARY "
        "KEY(installation_ref,channel,role_ref), FOREIGN KEY(installation_ref) REFERENCES "
        "subathon_permission_head(installation_ref))",
        "CREATE TABLE subathon_permission_accounts(installation_ref BLOB NOT NULL "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), channel INTEGER "
        "NOT NULL CHECK(typeof(channel)='integer' AND channel IN (0,1)), account_ref BLOB NOT NULL "
        "CHECK(typeof(account_ref)='blob' AND length(account_ref)>0), memberships BLOB NOT NULL "
        "CHECK(typeof(memberships)='blob' AND length(memberships)>=8), entries BLOB NOT NULL "
        "CHECK(typeof(entries)='blob' AND length(entries)>=8), PRIMARY "
        "KEY(installation_ref,channel,account_ref), FOREIGN KEY(installation_ref) REFERENCES "
        "subathon_permission_head(installation_ref), FOREIGN KEY(installation_ref,account_ref) "
        "REFERENCES subathon_accounts(installation_ref,account_ref))",
    };
    step.expected_schema = result.steps.back().expected_schema;
    const std::array<QString, 3> names{"subathon_permission_head", "subathon_permission_roles",
                                       "subathon_permission_accounts"};
    for (std::size_t i = 0; i < names.size(); ++i) {
        step.expected_schema.append({"table", names[i], names[i], step.sql[qsizetype(i)]});
        step.expected_schema.append(
            {"index", "sqlite_autoindex_" + names[i] + "_1", names[i], std::nullopt});
    }
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &a, const auto &b) {
                  return std::tie(a.type, a.name) < std::tie(b.type, b.name);
              });
    result.steps.append(std::move(step));
    result.supported_starting_versions.append(9);
    return result;
}
}
