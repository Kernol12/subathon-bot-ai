// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <subathon/server/sqlite_delivery_catalog.hpp>
#include <subathon/server/sqlite_reference_authority.hpp>
#include <tuple>

namespace subathon::server {
MigrationManifest delivery_jobs_repository_manifest() {
    auto manifest = reference_authority_repository_manifest();
    MigrationStep step;
    step.version = 11;
    step.id = "delivery-jobs-v1";
    const QString capture_ddl =
        "CREATE TABLE subathon_delivery_captures(operation_scope BLOB NOT NULL "
        "CHECK(typeof(operation_scope)='blob' AND length(operation_scope) BETWEEN 1 AND 4096), "
        "operation_ref BLOB NOT NULL CHECK(typeof(operation_ref)='blob' AND "
        "length(operation_ref) BETWEEN 1 AND 4096), job_ref BLOB NOT NULL "
        "CHECK(typeof(job_ref)='blob' "
        "AND length(job_ref) BETWEEN 1 AND 4096), route_ref BLOB NOT NULL "
        "CHECK(typeof(route_ref)='blob' "
        "AND length(route_ref) BETWEEN 1 AND 4096), selection INTEGER NOT NULL "
        "CHECK(typeof(selection)='integer' AND selection IN (0,1)), sender_ref BLOB NOT NULL "
        "CHECK(typeof(sender_ref)='blob' AND length(sender_ref) BETWEEN 1 AND 4096), action_ref "
        "BLOB "
        "NOT NULL CHECK(typeof(action_ref)='blob' AND length(action_ref) BETWEEN 1 AND 4096), "
        "action_version_ref BLOB NOT NULL CHECK(typeof(action_version_ref)='blob' AND "
        "length(action_version_ref) BETWEEN 1 AND 4096), PRIMARY KEY(operation_scope,operation_ref,"
        "job_ref), FOREIGN KEY(operation_scope,operation_ref,job_ref) "
        "REFERENCES subathon_app_jobs(operation_scope,operation_ref,job_ref))";
    const QString events_ddl =
        "CREATE TABLE subathon_delivery_events(operation_scope BLOB NOT NULL, operation_ref BLOB "
        "NOT NULL, job_ref BLOB NOT NULL, revision INTEGER NOT NULL "
        "CHECK(typeof(revision)='integer' AND revision>=1), state INTEGER NOT NULL "
        "CHECK(typeof(state)='integer' AND state BETWEEN 0 AND 4), attempt INTEGER "
        "CHECK(attempt IS NULL OR (typeof(attempt)='integer' AND attempt>=2 AND "
        "attempt<=revision)), "
        "evidence_ref BLOB CHECK(evidence_ref IS NULL OR (typeof(evidence_ref)='blob' AND "
        "length(evidence_ref) BETWEEN 1 AND 4096)), CHECK((state=0 AND attempt IS NULL) OR "
        "(state=1 AND attempt IS NOT NULL AND attempt=revision) OR "
        "(state BETWEEN 2 AND 4 AND attempt IS NOT NULL AND attempt<revision)), "
        "CHECK((state=1 OR revision=1)=(evidence_ref IS NULL)), "
        "CHECK(revision<>1 OR state=0), PRIMARY "
        "KEY(operation_scope,operation_ref,job_ref,revision), "
        "FOREIGN KEY(operation_scope,operation_ref,job_ref) "
        "REFERENCES subathon_delivery_captures(operation_scope,operation_ref,job_ref))";
    step.sql = {capture_ddl, events_ddl};
    step.expected_schema = manifest.steps.back().expected_schema;
    const QStringList names{"subathon_delivery_captures", "subathon_delivery_events"};
    for (qsizetype i = 0; i < names.size(); ++i) {
        const auto &name = names[i];
        step.expected_schema.append({"table", name, name, step.sql[i]});
        step.expected_schema.append(
            {"index", "sqlite_autoindex_" + name + "_1", name, std::nullopt});
    }
    std::sort(step.expected_schema.begin(), step.expected_schema.end(),
              [](const auto &left, const auto &right) {
                  return std::tie(left.type, left.name) < std::tie(right.type, right.name);
              });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions.append(11);
    return manifest;
}
} // namespace subathon::server
