// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/contribution_review.hpp>

#include <subathon/server/sqlite_milestone_target_edits.hpp>

#include <algorithm>
#include <tuple>

namespace subathon::server {
namespace {

struct ReviewTable {
    const char *name;
    const char *sql;
    int autoindexes;
};

constexpr ReviewTable work{
    "subathon_review_work",
    "CREATE TABLE subathon_review_work("
    "installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND "
    "length(installation_ref)>0), "
    "work_scope BLOB NOT NULL CHECK(typeof(work_scope)='blob' AND length(work_scope)>0), "
    "work_ref BLOB NOT NULL CHECK(typeof(work_ref)='blob' AND length(work_ref)>0), "
    "manifest BLOB NOT NULL CHECK(typeof(manifest)='blob' AND length(manifest) BETWEEN 1 AND "
    "987136 AND substr(manifest,1,4)=X'53525731'), "
    "entry_count INTEGER NOT NULL CHECK(typeof(entry_count)='integer' AND entry_count BETWEEN 1 "
    "AND 16), "
    "next_ordinal INTEGER NOT NULL CHECK(typeof(next_ordinal)='integer' AND next_ordinal BETWEEN 1 "
    "AND entry_count+1), "
    "PRIMARY KEY(installation_ref,work_scope,work_ref))",
    1};

constexpr ReviewTable entries{
    "subathon_review_entries",
    "CREATE TABLE subathon_review_entries("
    "installation_ref BLOB NOT NULL, work_scope BLOB NOT NULL, work_ref BLOB NOT NULL, "
    "ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal BETWEEN 1 AND 16), "
    "status INTEGER NOT NULL CHECK(typeof(status)='integer' AND status BETWEEN 0 AND 8), "
    "contribution_id BLOB REFERENCES subathon_contributions(contribution_id), "
    "receipt_id BLOB REFERENCES subathon_receipts(receipt_id), "
    "decision_kind INTEGER CHECK(decision_kind IS NULL OR (typeof(decision_kind)='integer' AND "
    "decision_kind BETWEEN 1 AND 3)), "
    "outcome BLOB CHECK(outcome IS NULL OR (typeof(outcome)='blob' AND length(outcome) BETWEEN 1 "
    "AND 61440)), "
    "application_scope BLOB, application_ref BLOB, "
    "PRIMARY KEY(installation_ref,work_scope,work_ref,ordinal), "
    "UNIQUE(installation_ref,work_scope,work_ref,contribution_id), "
    "UNIQUE(application_scope,application_ref), "
    "FOREIGN KEY(installation_ref,work_scope,work_ref) REFERENCES "
    "subathon_review_work(installation_ref,work_scope,work_ref), "
    "FOREIGN KEY(application_scope,application_ref) REFERENCES "
    "subathon_app_operations(operation_scope,operation_ref), "
    "CHECK((contribution_id IS NULL AND receipt_id IS NULL) OR "
    "(typeof(contribution_id)='blob' AND length(contribution_id)=8 AND typeof(receipt_id)='blob' "
    "AND length(receipt_id)=8 AND receipt_id<>X'0000000000000000')), "
    "CHECK((application_scope IS NULL AND application_ref IS NULL AND status NOT IN (1,3)) OR "
    "(typeof(application_scope)='blob' AND length(application_scope)>0 AND "
    "typeof(application_ref)='blob' AND length(application_ref)>0 AND status IN (1,3) AND "
    "contribution_id IS NOT NULL)), "
    "CHECK((status IN (7,8) AND contribution_id IS NULL AND decision_kind IS NULL AND outcome IS "
    "NULL) OR (status NOT IN (7,8) AND contribution_id IS NOT NULL AND outcome IS NOT NULL)), "
    "CHECK((status BETWEEN 5 AND 8 AND decision_kind IS NULL) OR "
    "(decision_kind IS NOT NULL AND ((decision_kind=1 AND status IN (0,1)) OR "
    "(decision_kind=2 AND status=2) OR (decision_kind=3 AND status IN (1,2,3,4))))))",
    3};

constexpr ReviewTable terminal_requests{
    "subathon_review_terminal_requests",
    "CREATE TABLE subathon_review_terminal_requests("
    "operation_scope BLOB NOT NULL, operation_ref BLOB NOT NULL, "
    "plan_ref BLOB NOT NULL, plan_revision BLOB NOT NULL, "
    "request_ordinal INTEGER NOT NULL CHECK(typeof(request_ordinal)='integer' AND "
    "request_ordinal>0), "
    "disposition INTEGER NOT NULL CHECK(typeof(disposition)='integer' AND disposition IN (1,2)), "
    "PRIMARY KEY(operation_scope,operation_ref,request_ordinal), "
    "FOREIGN KEY(operation_scope,operation_ref,plan_ref,plan_revision) "
    "REFERENCES subathon_app_operations(operation_scope,operation_ref,plan_ref,plan_revision), "
    "FOREIGN KEY(plan_ref,plan_revision,request_ordinal) REFERENCES "
    "subathon_plan_requests(plan_ref,revision,ordinal))",
    1};

struct ReviewIndex {
    const char *name;
    const char *sql;
};

constexpr ReviewIndex entry_indexes[]{
    {"subathon_review_entries_cid",
     "CREATE INDEX subathon_review_entries_cid ON subathon_review_entries(contribution_id)"},
    {"subathon_review_entries_accept_once",
     "CREATE UNIQUE INDEX subathon_review_entries_accept_once ON "
     "subathon_review_entries(contribution_id) WHERE decision_kind=1"},
    {"subathon_review_entries_reject_once",
     "CREATE UNIQUE INDEX subathon_review_entries_reject_once ON "
     "subathon_review_entries(contribution_id) WHERE decision_kind=2"},
};

void append_table(MigrationStep &step, const ReviewTable &table) {
    const auto name = QString::fromLatin1(table.name);
    const auto ddl = QString::fromLatin1(table.sql);
    step.sql.append(ddl);
    step.expected_schema.append({"table", name, name, ddl});
    for (int index = 1; index <= table.autoindexes; ++index)
        step.expected_schema.append(
            {"index", "sqlite_autoindex_" + name + "_" + QString::number(index), name, {}});
}

} // namespace

MigrationManifest contribution_review_repository_manifest() {
    auto manifest = milestone_target_edits_repository_manifest();
    MigrationStep step{19, "contribution-review-v1", {}, manifest.steps.back().expected_schema};
    append_table(step, work);
    append_table(step, entries);
    for (const auto &index : entry_indexes) {
        const auto ddl = QString::fromLatin1(index.sql);
        step.sql.append(ddl);
        step.expected_schema.append({"index", index.name, entries.name, ddl});
    }
    append_table(step, terminal_requests);
    std::ranges::sort(step.expected_schema, [](const auto &a, const auto &b) {
        return std::tie(a.type, a.name) < std::tie(b.type, b.name);
    });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions.append(19);
    return manifest;
}

void ContributionReview::preview(ReviewPreviewRequest, std::string,
                                 std::function<void(PreviewResult)>) {
    throw ContributionReviewNotImplemented("preview");
}

void ContributionReview::execute(ReviewCommand, std::string, std::function<void(Result)>) {
    throw ContributionReviewNotImplemented("execute");
}

void ContributionReview::lookup(ApplicationOperationKey, ApplicationPrincipalBinding, std::string,
                                std::function<void(Result)>) {
    throw ContributionReviewNotImplemented("lookup");
}

} // namespace subathon::server
