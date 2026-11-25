// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Sealed r8 query-only observer and the exact typed closures it must read on
// an independent reopen. It never hosts a writer or any service work.
#include "w2_w4_fixture.hpp"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>
#include <array>

namespace storage_003_w2_w4 {

// Each LIMIT is one greater than the largest legitimate row count; a returned
// cap row fails. Correction 834.1 sets the component cap to 4.
// The SQL literals are the sealed r8 TSV bytes, one selector per entry.
// clang-format off
struct ObserverRecipe {
    const char *name;
    const char *sql;
    int columns;
    int cap;
};
inline constexpr std::array observer_recipes{
    ObserverRecipe{"states",
                   "SELECT token,format_ref,document,configuration_ref,lifecycle,phase,main_remaining_ms,grace_remaining_ms,awaiting_confirmation,time_debt_ms,balance_count FROM subathon_app_states ORDER BY token LIMIT 4",
                   11, 4},
    ObserverRecipe{"head",
                   "SELECT singleton,token FROM subathon_app_head WHERE singleton=1 LIMIT 2",
                   2, 2},
    ObserverRecipe{"operations",
                   "SELECT operation_scope,operation_ref,operation_kind,subject_kind,receipt_id,contribution_id,plan_ref,plan_revision,head_token,before_token,after_token,advance_explanation,choices_format_ref,choices_document,reversal_ref FROM subathon_app_operations ORDER BY operation_scope,operation_ref LIMIT 2",
                   15, 2},
    ObserverRecipe{"claims",
                   "SELECT subject_kind,subject_id,operation_scope,operation_ref FROM subathon_app_claims ORDER BY subject_kind,subject_id LIMIT 2",
                   4, 2},
    ObserverRecipe{"condition",
                   "SELECT contribution_id,original_receipt_id,review,application,reversal,settlement FROM subathon_contributions ORDER BY contribution_id LIMIT 2",
                   6, 2},
    ObserverRecipe{"effects",
                   "SELECT operation_scope,operation_ref,request_ordinal,plan_ref,plan_revision,arithmetic_role,unit,exact_numerator,exact_denominator,requested_units,residual_numerator,residual_denominator,maximum_main_ms,deficit_policy,deficit_disposition,application_explanation FROM subathon_app_effects ORDER BY operation_scope,operation_ref,request_ordinal LIMIT 2",
                   16, 2},
    ObserverRecipe{"components",
                   "SELECT operation_scope,operation_ref,request_ordinal,component_kind,units,before_token,target_ref FROM subathon_app_components ORDER BY operation_scope,operation_ref,request_ordinal,component_kind LIMIT 4",
                   7, 4},
    ObserverRecipe{"results",
                   "SELECT operation_scope,operation_ref,format_ref,document,effect_count,component_count,job_count,transition_count,reversal_count FROM subathon_app_results ORDER BY operation_scope,operation_ref LIMIT 2",
                   9, 2},
    ObserverRecipe{"audit",
                   "SELECT operation_scope,operation_ref,actor_ref,decision_ref,reason,format_ref,document FROM subathon_app_audit ORDER BY operation_scope,operation_ref LIMIT 2",
                   7, 2},
    ObserverRecipe{"transitions",
                   "SELECT operation_scope,operation_ref,ordinal,transition FROM subathon_app_transitions ORDER BY operation_scope,operation_ref,ordinal LIMIT 1",
                   4, 1},
    ObserverRecipe{"jobs",
                   "SELECT operation_scope,operation_ref,ordinal,job_ref,source_request_ordinal,group_ref,group_ordinal,format_ref,document FROM subathon_app_jobs ORDER BY operation_scope,operation_ref,ordinal LIMIT 1",
                   9, 1},
    ObserverRecipe{"slot",
                   "SELECT singleton,base_token,configuration_ref,format_version,generation,revision,root_ordinal,pending_revision,pending_ordinal,main_ms,grace_ms,cadence_ms,ledger_revision,document FROM subathon_clock_checkpoint WHERE singleton=1 LIMIT 2",
                   14, 2},
    ObserverRecipe{"evidence",
                   "SELECT revision,ordinal,kind,installation_ref,timer_ref,lineage_ref,base_token,configuration_ref,ledger_revision,operation_scope,operation_ref,episode_ref,schedule_id,nominal_ms,fold_run,member_count,link_revision,link_ordinal,page_no,format_version,document FROM subathon_clock_evidence ORDER BY revision,ordinal LIMIT 4",
                   21, 4},
    ObserverRecipe{"contribution_snapshot",
                   "SELECT operation_scope,operation_ref,contribution_id,payment_evidence,monetary,original_decimal,currency,resolution_ref,missing_rate,review,evidence_policy,policy_ref,decision_choice,decision_ref,decision_actor_ref,application,reversal,settlement,application_after FROM subathon_app_contributions ORDER BY operation_scope,operation_ref LIMIT 2",
                   19, 2},
    ObserverRecipe{"retained_configuration",
                   "SELECT reference,document FROM subathon_retained_configurations ORDER BY reference LIMIT 2",
                   2, 2},
    ObserverRecipe{"retained_context",
                   "SELECT reference,document,configuration_ref FROM subathon_retained_contexts ORDER BY reference LIMIT 2",
                   3, 2},
    ObserverRecipe{"original_receipt",
                   "SELECT receipt_id,provider,source_account,identifier_namespace,opaque_id,event_type,schema_version,source_timestamp_utc_ms,actor_id,actor_display_name,money_original_decimal,money_currency,payment_namespace,payment_opaque_id,activity_namespace,activity_opaque_id,server_timestamp_utc_ms,connection_ref,configuration_ref,modifier_context_ref FROM subathon_receipts ORDER BY receipt_id LIMIT 2",
                   20, 2},
    ObserverRecipe{"plan_header",
                   "SELECT plan_ref,event_ref,parent_event_ref,root_event_ref,receipt_id,contribution_id,head_revision FROM subathon_plan_headers ORDER BY plan_ref LIMIT 2",
                   7, 2},
    ObserverRecipe{"plan_revision",
                   "SELECT plan_ref,revision,parent_revision,payment_evidence,monetary,original_decimal,currency,resolution_ref,missing_rate,review,evidence_policy,policy_ref,decision_choice,decision_ref,decision_actor_ref,application,reversal,settlement,valuation_state,historical_resolution_ref,historical_original_decimal,historical_currency,comparison_currency,effective_rate_decimal,fetched_rate_decimal,snapshot_ref,rate_source_ref,rate_timestamp_utc_ms,fetch_timestamp_utc_ms,manual_decision_ref,override_authority,override_actor_ref,override_reason,override_decision_ref FROM subathon_plan_revisions ORDER BY plan_ref,revision LIMIT 2",
                   34, 2},
    ObserverRecipe{"plan_rule",
                   "SELECT plan_ref,revision,ordinal,rule_ref,version_ref,selected,explanation FROM subathon_plan_rules ORDER BY plan_ref,revision,ordinal LIMIT 2",
                   7, 2},
    ObserverRecipe{"plan_modifier",
                   "SELECT plan_ref,revision,ordinal,definition_version_ref,instance_revision_ref,target_ref,group_ref,group_operator,target_operator,original_factor_decimal FROM subathon_plan_modifiers ORDER BY plan_ref,revision,ordinal LIMIT 1",
                   10, 1},
    ObserverRecipe{"plan_request",
                   "SELECT plan_ref,revision,ordinal,action_ref,action_version_ref,rule_ref,target_ref,unit,requires_historical_valuation,exact_numerator,exact_denominator,requested_units,calculation_explanation,reversal_behavior_ref FROM subathon_plan_requests ORDER BY plan_ref,revision,ordinal LIMIT 2",
                   14, 2},
};
// clang-format on

inline QVariant id8(quint64 value) {
    QByteArray bytes(8, '\0');
    for (int i = 7; i >= 0; --i, value >>= 8)
        bytes[i] = char(value & 0xff);
    return blob(bytes);
}

inline const SqlRow row_a{
    blob("A"),  blob(state_format.c_str()), blob(sas_a), blob("K"),  integer(1),
    integer(0), integer(original_main),     integer(0),  integer(0), integer(0),
    integer(0)};
inline const SqlRow row_b{
    blob("B"),  blob(state_format.c_str()), blob(sas_b), blob("K"),  integer(1),
    integer(0), integer(applied_main),      integer(0),  integer(0), integer(0),
    integer(0)};
inline const SqlRow row_d{
    blob("D"),  blob(state_format.c_str()), blob(sas_d), blob("K"),  integer(1),
    integer(0), integer(advanced_main),     integer(0),  integer(0), integer(0),
    integer(0)};
inline const SqlRow row_operation{blob("S"),
                                  blob("Q"),
                                  integer(0),
                                  integer(0),
                                  id8(1),
                                  id8(0),
                                  blob("F"),
                                  id8(1),
                                  blob("A"),
                                  blob("D"),
                                  blob("B"),
                                  blob("clock-observation-v1"),
                                  blob("subathon.internal.application-command.v1"),
                                  blob(sac),
                                  sql_null()};
inline const SqlRow row_claim{integer(0), id8(0), blob("S"), blob("Q")};
inline const SqlRow row_condition_old{id8(0),     id8(1),     integer(1),
                                      integer(0), integer(0), integer(1)};
inline const SqlRow row_condition_new{id8(0),     id8(1),     integer(1),
                                      integer(1), integer(0), integer(1)};
inline const SqlRow row_effect{
    blob("S"),         blob("Q"),  integer(1),  blob("F"),  id8(1),
    integer(1),        integer(0), blob("137"), blob("1"),  integer(requested_main),
    blob("0"),         blob("1"),  sql_null(),  sql_null(), sql_null(),
    blob("fixed-main")};
inline const SqlRow row_component_main{
    blob("S"), blob("Q"), integer(1), integer(0), integer(requested_main), blob("D"), sql_null()};
inline const SqlRow row_component_debt{blob("S"),  blob("Q"), integer(1), integer(1),
                                       integer(0), blob("D"), sql_null()};
inline const SqlRow row_component_cap{blob("S"),  blob("Q"), integer(1), integer(5),
                                      integer(0), blob("D"), sql_null()};
inline const SqlRow row_result{
    blob("S"),  blob("Q"),  blob("subathon.internal.application-result.v1"),
    blob(sar),  integer(1), integer(3),
    integer(0), integer(0), integer(0)};
inline const SqlRow row_audit{blob("S"),  blob("Q"),
                              blob("U"),  sql_null(),
                              sql_null(), blob("subathon.internal.application-audit.v1"),
                              blob(saa)};
inline const SqlRow row_contribution_snapshot{
    blob("S"),  blob("Q"),  id8(0),     integer(3), integer(0), sql_null(), sql_null(),
    sql_null(), integer(0), integer(1), integer(0), blob("G"),  sql_null(), sql_null(),
    sql_null(), integer(0), integer(0), integer(1), integer(1)};
inline const SqlRow row_retained_configuration{blob("K"), blob("configuration")};
inline const SqlRow row_retained_context{blob("X"), blob("arrival"), blob("K")};
inline const SqlRow row_original_receipt{
    id8(1),           blob("synthetic"), blob("account"), blob("event"), blob("one"),
    blob("nonmoney"), integer(1),        integer(0),      sql_null(),    sql_null(),
    sql_null(),       sql_null(),        sql_null(),      sql_null(),    sql_null(),
    sql_null(),       integer(0),        blob("C"),       blob("K"),     blob("X")};
inline const SqlRow row_plan_header{blob("F"), blob("E"), sql_null(), blob("E"),
                                    id8(1),    id8(0),    id8(1)};
inline const SqlRow row_plan_revision{
    blob("F"),  id8(1),     sql_null(), integer(3), integer(0), sql_null(), sql_null(),
    sql_null(), integer(0), integer(1), integer(0), blob("G"),  sql_null(), sql_null(),
    sql_null(), integer(0), integer(0), integer(1), integer(0), sql_null(), sql_null(),
    sql_null(), sql_null(), sql_null(), sql_null(), sql_null(), sql_null(), sql_null(),
    sql_null(), sql_null(), sql_null(), sql_null(), sql_null(), sql_null()};
inline const SqlRow row_plan_rule{blob("F"), id8(1),     integer(1),      blob("R"),
                                  blob("v"), integer(1), blob("selected")};
inline const SqlRow row_plan_request{
    blob("F"),          id8(1),     integer(1), blob("A1"),  blob("v"), blob("R"),
    blob("T"),          integer(0), integer(0), blob("137"), blob("1"), integer(requested_main),
    blob("fixed-main"), blob("rev")};
inline SqlRow applied_root_with(const QByteArray &operation, const QByteArray &document) {
    return root_row(2, "B", operation, document);
}

// The old closure is A/1/1 with the seeded sources. The applied closure is
// W3's B/2/2. The three contradictory copies differ from it in one row each.
enum class Cut { old, applied, missing_audit, wrong_operation, wrong_before };

inline std::array<SqlRows, observer_recipes.size()> oracle(Cut cut) {
    const bool old = cut == Cut::old;
    SqlRows evidence{seed_evidence};
    if (!old)
        evidence.append(cut == Cut::wrong_operation ? applied_root_with("Z", wrong_operation_root)
                        : cut == Cut::wrong_before  ? applied_root_with("Q", wrong_before_root)
                                                    : applied_evidence);
    const auto applied = [old](SqlRows rows) { return old ? SqlRows{} : rows; };
    return {old ? SqlRows{row_a} : SqlRows{row_a, row_b, row_d},
            {{integer(1), blob(old ? "A" : "B")}},
            applied({row_operation}),
            applied({row_claim}),
            {old ? row_condition_old : row_condition_new},
            applied({row_effect}),
            applied({row_component_main, row_component_debt, row_component_cap}),
            applied({row_result}),
            cut == Cut::missing_audit ? SqlRows{} : applied({row_audit}),
            {},
            {},
            {old ? seed_slot : successor_slot},
            evidence,
            applied({row_contribution_snapshot}),
            {row_retained_configuration},
            {row_retained_context},
            {row_original_receipt},
            {row_plan_header},
            {row_plan_revision},
            {row_plan_rule},
            {},
            {row_plan_request}};
}

// Opens its own read-only, query_only connection on a stopped file and reads
// every sealed selector in one snapshot. Destroy it before the next writer.
class Observer final {
  public:
    explicit Observer(const DisposableFile &file)
        : name_(QUuid::createUuid().toString(QUuid::WithoutBraces)),
          db_(QSqlDatabase::addDatabase("QSQLITE", name_)) {
        try {
            db_.setDatabaseName(file.path());
            db_.setConnectOptions("QSQLITE_OPEN_READONLY;QSQLITE_BUSY_TIMEOUT=0");
            require(db_.open(), "Open the independent stopped-file observer");
            control("PRAGMA query_only=ON");
            QSqlQuery query(db_);
            require(query.exec("PRAGMA query_only") && query.next() &&
                        query.value(0).toInt() == 1 && !query.next(),
                    "Observer connection is query_only");
        } catch (...) {
            close();
            throw;
        }
    }
    ~Observer() { close(); }
    Observer(const Observer &) = delete;
    Observer &operator=(const Observer &) = delete;

    void expect(Cut cut) {
        const auto expected = oracle(cut);
        control("BEGIN");
        for (size_t i = 0; i < observer_recipes.size(); ++i) {
            const auto &recipe = observer_recipes[i];
            const auto subject = QString::fromLatin1(recipe.name);
            QSqlQuery query(db_);
            require(query.exec(QString::fromLatin1(recipe.sql)),
                    subject + ": " + query.lastError().text());
            require(query.record().count() == recipe.columns, subject + ": column count");
            SqlRows rows;
            while (query.next()) {
                require(rows.size() + 1 < recipe.cap, subject + ": the cap row was returned");
                SqlRow row;
                for (int c = 0; c < recipe.columns; ++c)
                    row.append(query.value(c));
                rows.append(std::move(row));
            }
            require(!query.lastError().isValid(), subject + ": " + query.lastError().text());
            expect_rows(rows, expected[i], subject);
        }
        control("ROLLBACK");
    }

  private:
    void control(const char *sql) {
        QSqlQuery query(db_);
        require(query.exec(QString::fromLatin1(sql)), query.lastError().text());
    }
    void close() {
        db_.close();
        db_ = {};
        QSqlDatabase::removeDatabase(name_);
    }

    QString name_;
    QSqlDatabase db_;
};

// Closed, fixed mutations of a stopped applied copy, outside the governed
// writer, with foreign keys enforced. Each must change exactly one row.
enum class CopyFault { missing_audit, wrong_operation, wrong_before };

inline void mutate_stopped_copy(const DisposableFile &copy, CopyFault fault) {
    const auto name = QUuid::createUuid().toString(QUuid::WithoutBraces);
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", name);
        db.setDatabaseName(copy.path());
        db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=0");
        const auto run = [&] {
            require(db.open(), "Open only the detached negative copy");
            QSqlQuery query(db);
            require(query.exec("PRAGMA foreign_keys=ON") && query.exec("PRAGMA foreign_keys") &&
                        query.next() && query.value(0).toInt() == 1,
                    "The copy mutation keeps foreign keys enforced");
            query.finish();
            require(query.exec("BEGIN IMMEDIATE"), query.lastError().text());
            if (fault == CopyFault::missing_audit) {
                require(query.exec("DELETE FROM main.subathon_app_audit "
                                   "WHERE operation_scope=X'53' AND operation_ref=X'51'"),
                        query.lastError().text());
            } else {
                const bool operation = fault == CopyFault::wrong_operation;
                require(query.prepare(
                            operation
                                ? "UPDATE main.subathon_clock_evidence SET operation_ref=X'5A',"
                                  "document=? WHERE revision=2 AND ordinal=0 AND "
                                  "operation_scope=X'53' AND operation_ref=X'51' AND document=?"
                                : "UPDATE main.subathon_clock_evidence SET document=? "
                                  "WHERE revision=2 AND ordinal=0 AND operation_scope=X'53' AND "
                                  "operation_ref=X'51' AND document=?"),
                        query.lastError().text());
                query.addBindValue(operation ? wrong_operation_root : wrong_before_root);
                query.addBindValue(applied_root);
                require(query.exec(), query.lastError().text());
            }
            require(query.numRowsAffected() == 1, "Exactly the named row changes on this copy");
            query.finish();
            require(query.exec("COMMIT"), query.lastError().text());
        };
        try {
            run();
        } catch (...) {
            db.close();
            db = {};
            QSqlDatabase::removeDatabase(name);
            throw;
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(name);
}

} // namespace storage_003_w2_w4
