// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sqlite_reference_authority_fixture.hpp"

namespace selected_record_fixture {
using namespace reference_authority_fixture;
inline ReferenceAuthorityRequest query(bool hold) {
    auto q = request();
    q.use =
        hold ? ReferenceAuthorityUse::disclose_holds : ReferenceAuthorityUse::disclose_application;
    q.historical_operation = ApplicationOperationKey{"history", "app"};
    if (hold)
        q.historical_hold = "hold";
    return q;
}
inline void disclosure(const ReferenceAuthorityResolution &r, bool hold,
                       std::uint64_t graph_revision = 1) {
    const auto &f = facts(r);
    permitted(f);
    const auto op = hold ? "reference.hold-read.v1" : "reference.application-read.v1";
    require(f.registry.original_author == "A" && f.registry.execution_account == "E" &&
                f.registry.binding.principal == "U" &&
                f.automation_revision_ref == automation_revision() &&
                f.permission_revision_ref == permission_revision(graph_revision) &&
                f.original_author_input.author == "A" && f.permission_input.actor == "E" &&
                f.original_author_input.snapshot == f.permission_revision_ref &&
                f.permission_input.snapshot == f.permission_revision_ref &&
                f.original_author_input.actions.size() == 1 &&
                f.original_author_input.actions[0].operation == op &&
                f.original_author_input.actions[0].quantities.empty() &&
                f.permission_input.request.operation == op &&
                f.permission_input.request.quantities.empty() &&
                f.permission_input.request.resources.size() == 3 &&
                f.permission_input.request.resources[0].resource == "I" &&
                f.permission_input.request.resources[1].resource == "T" &&
                f.permission_input.request.resources[2].resource == source() &&
                f.binding.captured.execution_context_ref == "Y" &&
                f.binding.captured.arrival_context_ref == "X" &&
                f.binding.captured.receipt_id.value == 7 &&
                f.binding.captured.contribution_id.value == 0 &&
                f.binding.captured.plan.plan_ref == "P" && f.binding.captured.plan.revision == 1 &&
                f.binding.historical_operation.has_value() &&
                f.binding.historical_operation->operation_scope == "history" &&
                f.binding.historical_operation->operation_ref == "app",
            "Literal historical read action/source, A/E/U and current graph/registry revisions");
}
inline void reopen(Database &d, SqliteWriter &reader, qsizetype result_bytes) {
    bool stopped = false;
    require(d.writer.shutdown([&] { stopped = true; }) == StorageError::none,
            "Close literal setup writer");
    require(QTest::qWaitFor([&] { return stopped; }, 10000), "Setup writer actually stopped");
    auto budget = account_sessions_fixture::limits();
    budget.result_value_bytes = result_bytes;
    require(reader.open(d.path, budget) == StorageError::none,
            "Reopen real writer with selected cumulative value budget");
}
// A configuration row outside every selected capture/plan/state/hold closure.
// No new schema or production selected record is changed by the caller marker.
inline QString marker_query() {
    return "SELECT document FROM subathon_retained_configurations WHERE "
           "reference=X'63616c6c65722e6d61726b6572'";
}
inline void ensure_marker(SqliteWriter &writer) {
    (void)run(writer, [](auto c) -> QVariant {
        const auto s = c.begin();
        Cleanup cleanup{s};
        (void)s.execute({"INSERT OR IGNORE INTO subathon_retained_configurations VALUES("
                         "X'63616c6c65722e6d61726b6572',X'')",
                         {}});
        s.commit();
        return 1;
    });
}
// Observe caller transaction ownership as well as local refusal. The caller's
// marker is provisional during resolve, visible only after its own commit;
// total_changes detects any reader write in that interval.
// Assertion exceptions are replayed by run(), never counted as an expected error.
template <class Check>
void healthy(SqliteWriter &writer, const QString &path, bool hold,
             const ReferenceAuthorityLimits &limits, Check check) {
    ensure_marker(writer);
    Observer observer(path);
    const auto registry = registry_rows(observer);
    const auto before = observer.rows(marker_query());
    const auto result = run(writer, [&](auto c) -> QVariant {
        const auto s = c.begin();
        Cleanup cleanup{s};
        (void)s.execute({"UPDATE subathon_retained_configurations SET document=X'63616c6c6572' "
                         "WHERE reference=X'63616c6c65722e6d61726b6572'",
                         {}});
        const auto changes = s.execute({"SELECT total_changes()", {}});
        SqliteReferenceAuthorityUnit unit(s, limits);
        check(unit.resolve(query(hold)));
        require(s.execute({"SELECT total_changes()", {}}) == changes,
                "Selected reader performs no writes");
        require(s.execute({"SELECT 1", {}})[0][0].toInt() == 1,
                "Same actual ticket remains active after local result");
        require(observer.rows(marker_query()) == before && registry_rows(observer) == registry,
                "Reader did not commit caller's provisional write or change registry");
        s.commit();
        return 17;
    });
    require(result.error == WorkError::none &&
                result.transaction == TransactionOutcome::committed && result.value &&
                result.value->toInt() == 17 &&
                observer.rows(marker_query())[0][0].toByteArray() == QByteArray("caller") &&
                registry_rows(observer) == registry,
            "Actual caller commit AND normal completion, unchanged authority registry");
    // Restore only the unrelated marker for the next caller check.
    (void)run(writer, [&](auto c) -> QVariant {
        const auto s = c.begin();
        Cleanup cleanup{s};
        (void)s.execute({"UPDATE subathon_retained_configurations SET document=X'' "
                         "WHERE reference=X'63616c6c65722e6d61726b6572'",
                         {}});
        s.commit();
        return 1;
    });
}
// Four old-API-valid detections. Cause values are literal, not derived from the
// new selected reader or a production find result. v2/d2 is outside the selected
// {v1/first detection, v4/current, v3/direct parent, d4/current producer} closure.
inline void four_detections(Database &d) {
    seed_history(d, true);
    d.work([&](const auto &s) {
        ApplicationHoldCause cause;
        cause.hold_ref = "hold";
        cause.source = {{ContributionId{0}}, {7}, "V", std::nullopt, "V", "C", "X"};
        cause.command = {{"history", "app"}, historical_command()};
        cause.first_observation.persisted_state_token = "S0";
        cause.first_observation.annotation = {"opaque.annotation", QByteArray("", 0)};
        cause.plan_evidence.plan = ApplicationPlanKey{"P", 1};
        cause.plan_evidence.occurrences = {{1, "a", "b", "R", "T", RequestedUnit::milliseconds,
                                            Exact{600000}, 600000, "reference.integer-ms.v1"}};
        cause.kind = ApplicationHoldCauseKind::unresolved_policy;
        cause.missing_policy_reason = "independent.fixture.hold";
        const char *parents[] = {"version", "v2", "v3"};
        const char *versions[] = {"v2", "v3", "v4"};
        const char *mutations[] = {"d2", "d3", "d4"};
        for (int i = 0; i < 3; ++i) {
            ApplicationHoldDetection detection{cause,
                                               {mutations[i],
                                                std::string(parents[i]),
                                                versions[i],
                                                cause.first_observation,
                                                {"opaque.audit", QByteArray("", 0)}}};
            require(SqliteApplicationHoldStore(s).record_detection(detection).disposition ==
                        SqliteHoldMutationDisposition::recorded,
                    "Independent literal old public detection setup succeeds");
        }
    });
    Observer o(d.path);
    const auto rows = o.rows("SELECT version_ref,position,parent_version_ref,mutation_ref FROM "
                             "subathon_hold_versions WHERE hold_ref=X'686f6c64' ORDER BY position");
    require(rows.size() == 4 && rows[0][0].toByteArray() == "version" &&
                rows[0][1].toLongLong() == 1 && rows[0][2].isNull() &&
                rows[0][3].toByteArray() == "detection" && rows[1][0].toByteArray() == "v2" &&
                rows[1][2].toByteArray() == "version" && rows[1][3].toByteArray() == "d2" &&
                rows[2][0].toByteArray() == "v3" && rows[2][2].toByteArray() == "v2" &&
                rows[2][3].toByteArray() == "d3" && rows[3][0].toByteArray() == "v4" &&
                rows[3][2].toByteArray() == "v3" && rows[3][3].toByteArray() == "d4",
            "Actual finite initial/current/direct-parent/current-producer closure, not guessed "
            "chronology");
}
} // namespace selected_record_fixture
