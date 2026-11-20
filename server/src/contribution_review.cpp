// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/contribution_review.hpp>

#include "contribution_review_dispatch.hpp"

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

void ContributionReview::preview(ReviewPreviewRequest request, std::string proof,
                                 std::function<void(PreviewResult)> done) {
    using namespace review_detail;
    if (!done)
        return;
    if (request.entries.size() > row_limit) {
        done(ReviewRefusal::batch_too_large);
        return;
    }
    try {
        Encode envelope(envelope_limit);
        principal_fields(envelope, request.principal);
        envelope.text(proof);
        require(!request.entries.empty());
        selection_order(request.entries, [](const auto &x) { return x.selector; });
        for (const auto &entry : request.entries)
            (void)digest_intent(entry, false);
    } catch (const Oversize &) {
        done(ReviewRefusal::batch_too_large);
        return;
    } catch (const Invalid &) {
        done(ReviewRefusal::invalid_selection);
        return;
    }
    const auto out = std::make_shared<PreviewResult>(ReviewRefusal::unavailable);
    auto &facts = facts_;
    const auto admission = submit(
        writer_, observe_, test_unit::initial_read, 0,
        [&facts, request = std::move(request), proof = std::move(proof), out](const auto &s) {
            const auto selected =
                selectors(request.entries, [](const auto &x) { return x.selector; });
            const Authority auth{facts, request.principal, proof, {}, nullptr, selected};
            if (const auto refusal = auth.scope(s)) {
                *out = *refusal;
            } else {
                catalog(s);
                try {
                    *out = project(s, auth, request);
                } catch (const Oversize &) {
                    *out = ReviewRefusal::batch_too_large;
                } catch (const ConflictingAnchor &) {
                    *out = ReviewRefusal::invalid_selection;
                }
            }
            return false;
        },
        [out, done](const auto &r, bool observed) mutable {
            done(normal(r, observed) ? std::move(*out) : PreviewResult{ReviewRefusal::unavailable});
        });
    if (admission != StorageError::none)
        done(ReviewRefusal::unavailable);
}

void ContributionReview::execute(ReviewCommand command, std::string proof,
                                 std::function<void(Result)> done) {
    using namespace review_detail;
    if (!done)
        return;
    if (command.entries.size() > row_limit) {
        done(ReviewRefusal::batch_too_large);
        return;
    }
    QByteArray document;
    try {
        require(!command.entries.empty());
        selection_order(command.entries, [](const auto &x) { return x.intent.selector; });
        Encode proof_bound(envelope_limit);
        proof_bound.text(proof);
        document = manifest(command);
    } catch (const Oversize &) {
        done(ReviewRefusal::batch_too_large);
        return;
    } catch (const std::invalid_argument &) {
        done(ReviewRefusal::invalid_selection);
        return;
    } catch (const Invalid &) {
        done(ReviewRefusal::invalid_selection);
        return;
    }
    if (!qualified(detail::ContributionReviewWriterAccess::limits(writer_)) ||
        !detail::ContributionReviewWriterAccess::acquire(writer_)) {
        done(ReviewRefusal::unavailable);
        return;
    }
    std::shared_ptr<Execute> execute;
    try {
        execute = std::make_shared<Execute>(writer_, facts_, observe_, std::move(command),
                                            std::move(proof), std::move(document), done);
    } catch (...) {
        detail::ContributionReviewWriterAccess::release(writer_);
        done(ReviewRefusal::unavailable);
        return;
    }
    execute->acquired();
    execute->start();
}

void ContributionReview::lookup(ApplicationOperationKey key, ApplicationPrincipalBinding principal,
                                std::string proof, std::function<void(Result)> done) {
    using namespace review_detail;
    if (!done)
        return;
    try {
        Encode envelope(envelope_limit);
        principal_fields(envelope, principal);
        identity(envelope, key);
        envelope.text(proof);
    } catch (const Oversize &) {
        done(ReviewRefusal::batch_too_large);
        return;
    } catch (const Invalid &) {
        done(ReviewRefusal::invalid_selection);
        return;
    }
    const auto out = std::make_shared<Result>(ReviewRefusal::unavailable);
    auto &facts = facts_;
    const auto admission = submit(
        writer_, observe_, test_unit::initial_read, 0,
        [&facts, key = std::move(key), principal = std::move(principal), proof = std::move(proof),
         out](const auto &s) {
            const Authority auth{facts, principal, proof, {}, &key, {}, true};
            if (const auto refusal = auth.scope(s)) {
                *out = *refusal;
                return false;
            }
            catalog(s);
            const auto h = header(s, principal, key);
            if (h.count == 0) {
                *out = ReviewReply{};
                return false;
            }
            const auto cmd = decode_manifest(h.manifest);
            require(same(cmd.key, key) &&
                    cmd.principal.installation_ref == principal.installation_ref &&
                    int(cmd.entries.size()) == h.count);
            const auto selection =
                selectors(cmd.entries, [](const auto &x) { return x.intent.selector; });
            const Authority selected{facts, principal, proof, {}, &key, selection, true};
            if (const auto refusal = selected.scope(s))
                *out = *refusal;
            else {
                auto reply = read_prefix(s, cmd, h, h.cursor, &selected).reply;
                for (int ordinal = h.cursor; ordinal <= h.count; ++ordinal)
                    reply.rows.push_back({ordinal, ReviewDisposition::unattempted, {}});
                *out = std::move(reply);
            }
            return false;
        },
        [out, done](const auto &r, bool observed) mutable {
            done(normal(r, observed) ? std::move(*out) : Result{ReviewRefusal::unavailable});
        });
    if (admission != StorageError::none)
        done(ReviewRefusal::unavailable);
}

} // namespace subathon::server
