// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "command_charge_oracle.hpp"
#include "command_fixture.hpp"

namespace storage_027_fixture::terminal_charge_oracle {

// Independent statement inventory of the shipped complete SAR2 owning read
// on the Populated producer. The observer supplies only committed cell widths;
// the Stop implementation is never used to generate the allowance.
inline const QStringList receipt{"receipt_id",
                                 "provider",
                                 "source_account",
                                 "identifier_namespace",
                                 "opaque_id",
                                 "event_type",
                                 "schema_version",
                                 "source_timestamp_utc_ms",
                                 "actor_id",
                                 "actor_display_name",
                                 "money_original_decimal",
                                 "money_currency",
                                 "payment_namespace",
                                 "payment_opaque_id",
                                 "activity_namespace",
                                 "activity_opaque_id",
                                 "server_timestamp_utc_ms",
                                 "connection_ref",
                                 "configuration_ref",
                                 "modifier_context_ref"};
inline const QStringList header{"plan_ref",       "event_ref",  "parent_event_ref",
                                "root_event_ref", "receipt_id", "contribution_id",
                                "head_revision"};
inline const QStringList revision{"plan_ref",
                                  "revision",
                                  "parent_revision",
                                  "payment_evidence",
                                  "monetary",
                                  "original_decimal",
                                  "currency",
                                  "resolution_ref",
                                  "missing_rate",
                                  "review",
                                  "evidence_policy",
                                  "policy_ref",
                                  "decision_choice",
                                  "decision_ref",
                                  "decision_actor_ref",
                                  "application",
                                  "reversal",
                                  "settlement",
                                  "valuation_state",
                                  "historical_resolution_ref",
                                  "historical_original_decimal",
                                  "historical_currency",
                                  "comparison_currency",
                                  "effective_rate_decimal",
                                  "fetched_rate_decimal",
                                  "snapshot_ref",
                                  "rate_source_ref",
                                  "rate_timestamp_utc_ms",
                                  "fetch_timestamp_utc_ms",
                                  "manual_decision_ref",
                                  "override_authority",
                                  "override_actor_ref",
                                  "override_reason",
                                  "override_decision_ref"};
inline const QStringList rule{"plan_ref",    "revision", "ordinal",    "rule_ref",
                              "version_ref", "selected", "explanation"};
inline const QStringList modifier{"plan_ref",
                                  "revision",
                                  "ordinal",
                                  "definition_version_ref",
                                  "instance_revision_ref",
                                  "target_ref",
                                  "group_ref",
                                  "group_operator",
                                  "target_operator",
                                  "original_factor_decimal"};
inline const QStringList request{"plan_ref",
                                 "revision",
                                 "ordinal",
                                 "action_ref",
                                 "action_version_ref",
                                 "rule_ref",
                                 "target_ref",
                                 "unit",
                                 "requires_historical_valuation",
                                 "exact_numerator",
                                 "exact_denominator",
                                 "requested_units",
                                 "calculation_explanation",
                                 "reversal_behavior_ref"};
inline const QStringList app_states{"token",
                                    "format_ref",
                                    "document",
                                    "configuration_ref",
                                    "lifecycle",
                                    "phase",
                                    "main_remaining_ms",
                                    "grace_remaining_ms",
                                    "awaiting_confirmation",
                                    "time_debt_ms",
                                    "balance_count"};
inline const QStringList app_balances{"token", "balance_kind", "target_ref", "units"};
inline const QStringList app_operations{
    "operation_scope",    "operation_ref",    "operation_kind", "subject_kind",
    "receipt_id",         "contribution_id",  "plan_ref",       "plan_revision",
    "head_token",         "before_token",     "after_token",    "advance_explanation",
    "choices_format_ref", "choices_document", "reversal_ref"};
inline const QStringList app_contributions{
    "operation_scope", "operation_ref",    "contribution_id",    "payment_evidence",
    "monetary",        "original_decimal", "currency",           "resolution_ref",
    "missing_rate",    "review",           "evidence_policy",    "policy_ref",
    "decision_choice", "decision_ref",     "decision_actor_ref", "application",
    "reversal",        "settlement",       "application_after"};
inline const QStringList app_claims{"subject_kind", "subject_id", "operation_scope",
                                    "operation_ref"};
inline const QStringList app_effects{"operation_scope",
                                     "operation_ref",
                                     "request_ordinal",
                                     "plan_ref",
                                     "plan_revision",
                                     "arithmetic_role",
                                     "unit",
                                     "exact_numerator",
                                     "exact_denominator",
                                     "requested_units",
                                     "residual_numerator",
                                     "residual_denominator",
                                     "maximum_main_ms",
                                     "deficit_policy",
                                     "deficit_disposition",
                                     "application_explanation"};
inline const QStringList app_components{"operation_scope", "operation_ref", "request_ordinal",
                                        "component_kind",  "units",         "before_token",
                                        "target_ref"};
inline const QStringList app_transitions{"operation_scope", "operation_ref", "ordinal",
                                         "transition"};
inline const QStringList app_jobs{
    "operation_scope", "operation_ref", "ordinal",    "job_ref", "source_request_ordinal",
    "group_ref",       "group_ordinal", "format_ref", "document"};
inline const QStringList app_results{"operation_scope", "operation_ref",    "format_ref",
                                     "document",        "effect_count",     "component_count",
                                     "job_count",       "transition_count", "reversal_count"};
inline const QStringList app_audit{"operation_scope", "operation_ref", "actor_ref", "decision_ref",
                                   "reason",          "format_ref",    "document"};
inline const QStringList app_reversals{"operation_scope",         "operation_ref",
                                       "request_ordinal",         "original_scope",
                                       "original_operation",      "original_request_ordinal",
                                       "original_component_kind", "basis_position",
                                       "fraction_numerator",      "fraction_denominator",
                                       "entitlement_numerator",   "entitlement_denominator",
                                       "residual_numerator",      "residual_denominator",
                                       "previous_cumulative",     "cumulative",
                                       "requested_increment",     "full_refund"};

struct Tape {
    CatalogFile &db;
    charge_oracle::Charge total{};

    static qsizetype width(const QVariant &cell) {
        if (cell.isNull())
            return 0;
        if (cell.metaType().id() == QMetaType::QByteArray)
            return cell.toByteArray().size();
        if (cell.metaType().id() == QMetaType::QString)
            return cell.toString().toUtf8().size();
        require(cell.metaType().id() == QMetaType::Int ||
                    cell.metaType().id() == QMetaType::LongLong,
                "158A projected cells have counted SQLite storage classes");
        return 0;
    }

    SqlRows read(const QString &sql, const QList<QVariant> &bindings = {}) {
        auto rows = db.rows(sql, bindings);
        charge_oracle::Charge next{1, sql.toUtf8().size(), 0, rows.size(), 0};
        require(sql.count('?') == bindings.size(), "158A query binds every counted cell");
        for (const auto &b : bindings)
            next.binds += 8 + width(b);
        for (const auto &row : rows)
            for (const auto &cell : row)
                next.results += 8 + width(cell);
        total = charge_oracle::combine(total, next);
        return rows;
    }

    static QString project(const QStringList &columns) {
        QStringList fields;
        for (const auto &column : columns)
            fields.append({column, "typeof(" + column + ")", "length(" + column + ")"});
        return fields.join(',');
    }

    SqlRows projected(const QString &table, const QStringList &columns, const QString &suffix,
                      const QList<QVariant> &bind) {
        return read("SELECT " + project(columns) + " FROM main." + table + " " + suffix, bind);
    }

    SqlRow one(const QString &table, const QStringList &columns, const QString &where,
               const QList<QVariant> &bind) {
        auto rows = projected(table, columns, "WHERE " + where + " LIMIT 2", bind);
        require(rows.size() == 1 && rows.front().size() == columns.size() * 3,
                "158A fixture contains the exact projected owner row");
        return rows.front();
    }

    void configuration(const QVariant &ref) {
        const auto rows = read(
            "SELECT reference,document,NULL,typeof(reference),length(reference),typeof(document),"
            "length(document),'null',NULL FROM main.subathon_retained_configurations WHERE "
            "reference=?",
            {ref});
        require(rows.size() == 1, "158A state or receipt configuration is retained");
    }

    void state(const QVariant &token) {
        const auto row = one("subathon_app_states", app_states, "token=?", {token});
        configuration(row[9]);
        const auto count =
            read("SELECT count(*) FROM main.subathon_app_balances WHERE token=?", {token});
        const auto rows =
            projected("subathon_app_balances", app_balances,
                      "WHERE token=? AND (balance_kind>? OR (balance_kind=? AND target_ref>?)) "
                      "ORDER BY balance_kind,target_ref LIMIT 65",
                      {token, 1, 1, QByteArray("", 0)});
        require(count.size() == 1 && count.front().front() == rows.size() && rows.size() <= 64,
                "158A fixture state fits one exact balance page");
    }

    void plan(const QVariant &plan_key, const QVariant &plan_revision) {
        const auto h = one("subathon_plan_headers", header, "plan_ref=?", {plan_key});
        const auto from = one("subathon_receipts", receipt, "receipt_id=?", {h[12]});
        configuration(from[54]);
        const auto context =
            read("SELECT reference,document,configuration_ref,typeof(reference),length(reference),"
                 "typeof(document),length(document),typeof(configuration_ref),length(configuration_"
                 "ref) FROM main.subathon_retained_contexts WHERE reference=?",
                 {from[57]});
        require(context.size() == 1 && !context[0][2].isNull(),
                "158A fixture arrival context has its exact configuration link");
        configuration(context[0][2]);
        const auto contribution =
            one("subathon_contributions", {"contribution_id", "original_receipt_id"},
                "contribution_id=?", {h[15]});
        (void)one("subathon_receipts",
                  {"receipt_id", "provider", "source_account", "identifier_namespace", "opaque_id"},
                  "receipt_id=?", {contribution[3]});

        constexpr auto bad_revision =
            "coalesce(sum(CASE WHEN typeof(revision)<>'blob' OR length(revision)<>8 OR "
            "revision=X'0000000000000000' THEN 1 ELSE 0 END),0)";
        (void)read("SELECT " +
                       project({"count(*)", "min(revision)", "max(revision)", bad_revision}) +
                       " FROM main.subathon_plan_revisions WHERE plan_ref=?",
                   {plan_key});
        const auto revisions =
            projected("subathon_plan_revisions", revision,
                      "WHERE plan_ref=? AND revision>? ORDER BY revision LIMIT 65",
                      {plan_key, QByteArray(8, '\0')});
        require(revisions.size() == 1 && revisions.front()[3] == plan_revision,
                "158A fixture has one immutable captured plan revision");
        constexpr auto bad_valuation =
            "coalesce(sum(CASE WHEN typeof(requires_historical_valuation)<>'integer' OR "
            "requires_historical_valuation NOT IN (0,1) THEN 1 ELSE 0 END),0)";
        (void)read("SELECT " +
                       project({"count(*)", bad_valuation,
                                "coalesce(max(requires_historical_valuation),0)"}) +
                       " FROM main.subathon_plan_requests WHERE plan_ref=? AND revision=?",
                   {plan_key, plan_revision});
        for (const auto &[table, columns] :
             std::array<std::pair<const char *, const QStringList *>, 3>{
                 {{"subathon_plan_rules", &rule},
                  {"subathon_plan_modifiers", &modifier},
                  {"subathon_plan_requests", &request}}})
            (void)projected(table, *columns,
                            "WHERE plan_ref=? AND revision=? ORDER BY ordinal LIMIT ?",
                            {plan_key, plan_revision, 1025});
    }
};

inline charge_oracle::Charge application_stop_proof(Populated &f) {
    Tape tape{f.db};
    const QList<QVariant> key{QByteArray::fromStdString(f.original.operation_scope),
                              QByteArray::fromStdString(f.original.operation_ref)};
    const QString where = "operation_scope=? AND operation_ref=?";
    auto classify = key;
    classify.prepend(QByteArray("subathon.internal.application-result.v2"));
    require(tape.read("SELECT typeof(format_ref)='blob' AND format_ref=? FROM "
                      "main.subathon_app_results WHERE operation_scope=? AND operation_ref=?",
                      classify) == SqlRows{{1}},
            "158A fixture uses the shipped SAR2 result format");
    const auto op = tape.one("subathon_app_operations", app_operations, where, key);
    tape.state(op[24]);
    if (op[24] != op[27])
        tape.state(op[27]);
    tape.state(op[30]);
    tape.plan(op[18], op[21]);
    (void)tape.one("subathon_app_contributions", app_contributions, where, key);
    (void)tape.one("subathon_app_claims", app_claims, where, key);
    (void)tape.one("subathon_app_results", app_results, where, key);
    (void)tape.one("subathon_app_audit", app_audit, where, key);

    (void)tape.read(
        "SELECT CASE WHEN typeof(version)<>'integer' THEN 3 "
        "WHEN version=19 AND migration_id COLLATE BINARY='contribution-review-v1' THEN 1 "
        "WHEN version=20 AND migration_id COLLATE BINARY='connector-intake-storage-v1' THEN 2 "
        "WHEN version=21 AND migration_id COLLATE BINARY="
        "'milestone-slot-interruption-storage-v1' THEN 4 "
        "WHEN version<19 THEN 0 ELSE 3 END "
        "FROM main.subathon_migration_history "
        "WHERE EXISTS(SELECT 1 FROM main.subathon_catalog WHERE singleton=1 "
        "AND typeof(catalog_format)='integer' AND catalog_format=1 "
        "AND store_identity='subathon.sqlite-store') ORDER BY version DESC LIMIT 1");
    auto three = key;
    three.append(3);
    const auto terminals =
        tape.projected("subathon_review_terminal_requests",
                       {"plan_ref", "plan_revision", "request_ordinal", "disposition"},
                       "WHERE " + where + " ORDER BY request_ordinal LIMIT ?", three);
    require(terminals.size() == 2 && terminals[0][6] == 1 && terminals[0][9] == 1 &&
                terminals[1][6] == 3 && terminals[1][9] == 2,
            "158A terminal tape is the exact ordinal 1/3 producer partition");
    QStringList counts;
    QList<QVariant> count_keys;
    for (const auto *table :
         {"subathon_app_effects", "subathon_app_components", "subathon_app_jobs",
          "subathon_app_transitions", "subathon_app_reversals"}) {
        counts.append("(SELECT count(*) FROM main." + QString(table) + " WHERE " + where + ")");
        count_keys.append(key);
    }
    (void)tape.read("SELECT " + counts.join(','), count_keys);
    auto zero = key;
    zero.append(0);
    const auto effects = tape.projected(
        "subathon_app_effects", app_effects,
        "WHERE " + where + " AND request_ordinal>? ORDER BY request_ordinal LIMIT 65", zero);
    require(effects.size() == 1 && effects[0][6] == 2,
            "158A proof has one genuine direct retained sibling");
    auto effect = key;
    effect.append(2);
    const auto components = tape.projected(
        "subathon_app_components", app_components,
        "WHERE " + where + " AND request_ordinal=? ORDER BY component_kind LIMIT 4", effect);
    require(components.size() == 1, "158A direct effect has one exact stored component");
    const auto jobs =
        tape.projected("subathon_app_jobs", app_jobs,
                       "WHERE " + where + " AND ordinal>? ORDER BY ordinal LIMIT 65", zero);
    require(jobs.size() <= 64, "158A fixture uses a single job page");
    (void)tape.read(
        "SELECT count(DISTINCT job_ref), (SELECT count(*) FROM ("
        "SELECT group_ref FROM main.subathon_app_jobs WHERE " +
            where +
            " AND group_ref IS NOT NULL GROUP BY group_ref HAVING min(group_ordinal)<>1 OR "
            "max(group_ordinal)<>count(*) OR count(DISTINCT group_ordinal)<>count(*))) "
            "FROM main.subathon_app_jobs WHERE " +
            where,
        key + key);
    const auto transitions =
        tape.projected("subathon_app_transitions", app_transitions,
                       "WHERE " + where + " AND ordinal>? ORDER BY ordinal LIMIT 65", zero);
    require(transitions.size() <= 64, "158A fixture uses a single transition page");
    (void)tape.read("SELECT count(*) FROM main.subathon_app_reversals WHERE " + where, key);
    auto link_limit = key;
    link_limit.append(1025);
    require(
        tape.projected("subathon_app_reversals", app_reversals,
                       "WHERE " + where +
                           " ORDER BY request_ordinal,original_scope,original_operation,original_"
                           "request_ordinal,original_component_kind LIMIT ?",
                       link_limit)
            .empty(),
        "158A original fixture has no reversal child");
    const auto states = op[24] == op[27] ? 2 : 3;
    require(tape.total.attempts == 29 + 4 * states && tape.total.peak == 2299,
            "158A full proof uses its fixed 29 statements plus four per selected state");
    return tape.total;
}

} // namespace storage_027_fixture::terminal_charge_oracle
