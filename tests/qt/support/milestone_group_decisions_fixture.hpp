// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "milestone_group_restart_fixture.hpp"

#include <QCryptographicHash>

namespace milestone_group_decisions_fixture {
using namespace milestone_group_restart_fixture;

inline const QStringList decision_ddl{
    "CREATE TABLE subathon_milestone_acceptance_decisions(occurrence_ref BLOB NOT NULL, mode "
    "INTEGER NOT NULL CHECK(typeof(mode)='integer' AND mode IN (0,1)), choice INTEGER NOT NULL "
    "CHECK(typeof(choice)='integer' AND choice IN (0,1,2)), actor_ref BLOB NOT NULL "
    "CHECK(typeof(actor_ref)='blob' AND length(actor_ref) BETWEEN 1 AND 4096), decision_ref BLOB "
    "NOT NULL CHECK(typeof(decision_ref)='blob' AND length(decision_ref) BETWEEN 1 AND 4096), "
    "resolution INTEGER, resolution_actor_ref BLOB, resolution_ref BLOB, CHECK((resolution IS NULL "
    "AND resolution_actor_ref IS NULL AND resolution_ref IS NULL) OR (choice=1 AND "
    "typeof(resolution)='integer' AND resolution IN (0,1) AND typeof(resolution_actor_ref)='blob' "
    "AND length(resolution_actor_ref) BETWEEN 1 AND 4096 AND typeof(resolution_ref)='blob' AND "
    "length(resolution_ref) BETWEEN 1 AND 4096)), PRIMARY KEY(occurrence_ref), FOREIGN "
    "KEY(occurrence_ref) REFERENCES subathon_milestone_groups(occurrence_ref))",
    "CREATE TABLE subathon_milestone_refund_decisions(occurrence_ref BLOB NOT NULL, refund_ref "
    "BLOB NOT NULL CHECK(typeof(refund_ref)='blob' AND length(refund_ref) BETWEEN 1 AND 4096), "
    "score_before_micro_points INTEGER NOT NULL "
    "CHECK(typeof(score_before_micro_points)='integer'), score_after_micro_points INTEGER NOT NULL "
    "CHECK(typeof(score_after_micro_points)='integer'), current_target_micro_points INTEGER NOT "
    "NULL CHECK(typeof(current_target_micro_points)='integer'), policy INTEGER NOT NULL "
    "CHECK(typeof(policy)='integer' AND policy IN (0,1,2)), resolution INTEGER, "
    "resolution_actor_ref BLOB, resolution_ref BLOB, CHECK((resolution IS NULL AND "
    "resolution_actor_ref IS NULL AND resolution_ref IS NULL) OR (policy=1 AND "
    "typeof(resolution)='integer' AND resolution IN (0,1) AND typeof(resolution_actor_ref)='blob' "
    "AND length(resolution_actor_ref) BETWEEN 1 AND 4096 AND typeof(resolution_ref)='blob' AND "
    "length(resolution_ref) BETWEEN 1 AND 4096)), PRIMARY KEY(occurrence_ref,refund_ref), FOREIGN "
    "KEY(occurrence_ref) REFERENCES subathon_milestone_groups(occurrence_ref))",
};

inline MigrationManifest literal_decision_manifest() {
    auto prefix = delivery_jobs_repository_manifest();
    prefix.steps.append(literal_step(prefix));
    prefix.supported_starting_versions.append(12);
    MigrationStep next{13, "milestone-group-decisions-v1", decision_ddl,
                       prefix.steps.back().expected_schema};
    const QStringList names{"subathon_milestone_acceptance_decisions",
                            "subathon_milestone_refund_decisions"};
    for (qsizetype i = 0; i < names.size(); ++i) {
        const auto &name = names[i];
        next.expected_schema.append({"table", name, name, decision_ddl[i]});
        next.expected_schema.append({"index", "sqlite_autoindex_" + name + "_1", name, {}});
    }
    std::sort(next.expected_schema.begin(), next.expected_schema.end(),
              [](const auto &a, const auto &b) {
                  return std::tie(a.type, a.name) < std::tie(b.type, b.name);
              });
    prefix.steps.append(std::move(next));
    prefix.supported_starting_versions.append(13);
    return prefix;
}

// Frame the independently specified fields in schema order using the catalog
// v1 contract. For this projection, (type, name) uniquely orders every object.
inline QByteArray literal_digest(const MigrationStep &step) {
    QCryptographicHash hash(QCryptographicHash::Sha256);
    const auto frame = [&](const QByteArray &bytes) {
        hash.addData(QByteArray::number(bytes.size()) + ':');
        hash.addData(bytes);
    };
    frame("migration-catalog-v1");
    frame(QByteArray::number(step.version));
    frame(step.id.toUtf8());
    frame(QByteArray::number(step.sql.size()));
    for (const auto &statement : step.sql)
        frame(statement.toUtf8());
    frame(QByteArray::number(step.expected_schema.size()));
    auto schema = step.expected_schema;
    std::sort(schema.begin(), schema.end(), [](const auto &a, const auto &b) {
        return std::tie(a.type, a.name) < std::tie(b.type, b.name);
    });
    for (const auto &object : schema) {
        frame(object.type.toUtf8());
        frame(object.name.toUtf8());
        frame(object.table_name.toUtf8());
        frame(object.sql ? "1" : "0");
        if (object.sql)
            frame(object.sql->toUtf8());
    }
    return hash.result();
}

inline void advance_to_literal_decisions(File &file) {
    migrated(permission_graph_fixture::migrate(*file.writer, literal_decision_manifest()), 13, 1);
}

class DecisionFile final : public File {
  public:
    // Behavior cases reach their own boundary with literal v13 SQL.
    DecisionFile() { advance_to_literal_decisions(*this); }
};

} // namespace milestone_group_decisions_fixture
