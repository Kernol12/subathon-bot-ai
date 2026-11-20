// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "contribution_review_operation.hpp"

namespace subathon::server::review_detail {

using ReadCost = detail::ReviewReadCost;

inline constexpr qsizetype reconciliation_limit = 768;
inline constexpr qsizetype shared_reconciliation = 128;
inline constexpr qsizetype document_allowance = 2 * 1024 * 1024;
inline constexpr qsizetype row_value_allowance = 18240;

// Addends come from a metered read, a bounded encoder, or a collection checked
// before sizing. The bounded hold fanout below is at most eight closures.
inline bool reconciliation_fits(const ReadCost &cost, const ReadCost &authority,
                                qsizetype manifest_bytes) {
    // Catalog marker, 19 history rows, review schema projection, header,
    // and the first persisted state (32 balances plus scalar and count).
    // The empty-prefix journal projection is included in cost.
    const auto attempts = std::max(shared_reconciliation, 9 + authority.attempts);
    const auto rows = std::max(shared_reconciliation, 69 + authority.rows);
    return cost.attempts <= reconciliation_limit - attempts &&
           cost.rows <= reconciliation_limit - rows &&
           cost.document_bytes + authority.document_bytes <= document_allowance - manifest_bytes;
}

inline qsizetype document_excess(qsizetype bound) {
    return std::max(qsizetype{}, bound - row_value_allowance);
}

inline qsizetype configuration_read_bytes(const SqliteWorkSession &s,
                                          const ApplicationStateSnapshot &state) {
    const auto config = s.execute(
        {"SELECT length(document) FROM main.subathon_retained_configurations WHERE reference=?",
         {bytes(state.configuration_ref)}});
    require(config.size() == 1 && config.front().size() == 1);
    const auto size = integer(config.front().front());
    require(size >= 0 && size <= SqliteBundleLimits{}.value_bytes);
    return document_excess(qsizetype(size) + qsizetype(state.configuration_ref.size()) + 256);
}

inline qsizetype state_read_bytes(const ApplicationStateSnapshot &state) {
    // Three bounded refs and the owning 11-column typed projection.
    return document_excess(state.envelope.document.size() + 3 * 4096 + 512);
}

inline ReadCost bundle_reconciliation(const ReviewCommand &command, const ReviewEntry &entry,
                                      const Row &row, const StoredOutcome &data, ReadCost plan_cost,
                                      qsizetype config_bytes, qsizetype audit_bytes) {
    const auto entry_bytes = entry_document(entry);
    if (entry.intent.revision && data.selected_revision != data.original_revision) {
        const auto &plan = *entry.intent.revision;
        const auto body = qsizetype(plan.rules.size() + plan.modifiers.size() +
                                    plan.captured.requested_effects.size());
        // One dependent projection and, at the 64-row boundary, one page.
        // The new scalar, possible lookahead and dependency return three rows.
        plan_cost += {2, 3 + body, document_excess(2 * entry_bytes.size() + 8192)};
    }

    // fixed(): operation, claim, contribution snapshot, result, audit.
    // children(): partition, counters, effects, jobs, group check, transitions,
    // reversal count, and the empty original reversal-link scan. The v19
    // catalog probe is shared. Every physical effect does a component query.
    auto cost = plan_cost;
    const auto transitions = qsizetype(row.transitions.size());
    cost += {13 + qsizetype(row.effects.size()) + transitions / 64,
             8 + row.partition.size() + qsizetype(row.jobs.size()) + transitions +
                 (transitions ? (transitions - 1) / 64 : 0),
             0};
    for (const auto &effect : row.effects)
        cost.rows += qsizetype(effect.components.size());

    // The advanced and resulting state are the only per-ordinal state reads.
    // Every later write must match the previous reviewed bundle's resulting
    // token. Thus only the first bundle needs a separate persisted-head read.
    const auto &state = row.inputs->current_state;
    cost +=
        {7, 5 + 2 * qsizetype(state.balances.size()), 3 * state_read_bytes(state) + config_bytes};

    const auto &head = state.token;
    const std::string generated_token(71, 'x'); // review- and SHA-256 hex
    const auto &before = head.size() >= generated_token.size() ? head : generated_token;
    const auto result =
        application_result(row, {command_format, entry_bytes}, head, before, generated_token);

    // Bound each large payload where its actual SQL projection returns it.
    // Component, terminal, transition, balance and count rows stay below the
    // per-row payload even with maximum-sized refs.
    Encode conditions;
    state_fields(conditions, row.inputs->current_contribution->contribution.state);
    cost.document_bytes +=
        document_excess(
            entry_bytes.size() +
            qsizetype(data.plan.size() + head.size() + before.size() + generated_token.size()) +
            1024) +
        document_excess(conditions.data.size() + 1024) +
        document_excess(result.document.size() + 512) +
        document_excess(
            audit_bytes +
            qsizetype(command.actor.size() + entry.decision_ref.size() + entry.reason.size()) +
            512);
    for (const auto &effect : row.effects) {
        Encode encoded(262144);
        application_result_wire::put_effect(encoded, effect);
        cost.document_bytes +=
            document_excess(encoded.data.size() + qsizetype(data.plan.size()) + 1024);
    }
    for (const auto &job : row.jobs) {
        Encode encoded(262144);
        application_result_wire::put_job(encoded, job);
        cost.document_bytes += document_excess(encoded.data.size() + 512);
    }
    return cost;
}

inline ReadCost created_hold_reconciliation(const ReviewEntry &entry, const Row &row,
                                            ReadCost plan_cost, qsizetype config_bytes) {
    const auto &state = row.inputs->current_state;
    const auto &plan = entry.intent.revision ? *entry.intent.revision : row.inputs->selected_plan;
    const auto document = entry_document(entry);
    if (entry.intent.revision) {
        plan_cost += {2,
                      3 + qsizetype(plan.rules.size() + plan.modifiers.size() +
                                    plan.captured.requested_effects.size()),
                      document_excess(2 * document.size() + 8192)};
    }
    // New standalone detection: metadata/owner, occurrence sizing, cause
    // and detection, original receipt and contribution/observation links,
    // and two reads of the same full persisted state. No ancestor,
    // replacement resolution, or associated bundle can be created here.
    return {plan_cost.attempts + 59,
            plan_cost.rows + 59 + 2 * qsizetype(state.balances.size()) +
                qsizetype(plan.captured.requested_effects.size()),
            // The retained plan read already charged the same arrival and
            // source receipt once. Hold source checks can repeat them.
            8 * plan_cost.document_bytes + 2 * (config_bytes + state_read_bytes(state)) +
                6 * document_excess(document.size() + 6 * 4096 + 2048)};
}

inline ReadCost closed_hold_reconciliation(const ReviewCommand &command, const ReviewEntry &entry,
                                           const Row &row, ReadCost bundle,
                                           qsizetype config_bytes) {
    // The selected timer's full existing reconstruction is included in
    // active(). Both resolution traversals repeat the existing metadata for
    // every closed hold. A new version and owner on each traversal add a
    // page at most, two owner links and the new mutation's metadata.
    const auto n = qsizetype(row.closures.size());
    const auto reads_of_mutation = 5 + 2 * n;
    auto cost = row.hold_read.full;
    cost += 2 * row.hold_read.metadata;
    cost += reads_of_mutation * ReadCost{8 + 6 * n, 6 + 8 * n, 0};
    cost += (1 + 2 * n) * ReadCost{3, 4, 0};
    cost += {6 + 3 * n, 3 + 6 * n, 0};

    // Unlike the previous, active view, the newly resolved event has a full
    // mutation document and per-closure decision rows.
    const auto options = entry_document(entry);
    cost.document_bytes += document_excess(
        options.size() +
        qsizetype(command.actor.size() + entry.decision_ref.size() + entry.reason.size()) +
        6 * 4096 + 1024);
    for (const auto &closure : row.closures)
        cost.document_bytes += document_excess(closure.decision.document.size() + 5 * 4096 + 512);

    if (bundle.attempts) {
        // The resolution's associated application uses an independent bundle
        // read, with all three states and independently resolved configuration.
        // Its whole-input check also reopens each closed hold's existing cause.
        const auto &state = row.inputs->current_state;
        cost += bundle;
        cost += {6, 5 + qsizetype(state.balances.size()), 2 * config_bytes};
        cost += row.hold_read.full;
    }
    return cost;
}

inline ReadCost reconciliation_row(const SqliteWorkSession &s, const ReviewCommand &command,
                                   const ReviewEntry &entry, const Row &row,
                                   const StoredOutcome &data, ReadCost plan_cost) {
    if (!data.outcome.facts)
        return {0, 1, 0};

    // Journal projection, verified anchor, selected plan link, and each
    // closure's immutable version/mutation/parent join.
    const auto closed = qsizetype(data.closed.size());
    const auto audit_bytes = outcome_doc(data).size();
    ReadCost cost{2 + closed, 3 + closed, document_excess(audit_bytes + 512)};
    const auto &facts = *data.outcome.facts;
    const auto config_bytes =
        facts.bundle || data.created ? configuration_read_bytes(s, row.inputs->current_state) : 0;
    const auto bundle = facts.bundle ? bundle_reconciliation(command, entry, row, data, plan_cost,
                                                             config_bytes, audit_bytes)
                                     : ReadCost{};
    cost += bundle;
    if (data.created)
        cost += created_hold_reconciliation(entry, row, plan_cost, config_bytes);
    else if (facts.timer && closed)
        cost += closed_hold_reconciliation(command, entry, row, bundle, config_bytes);
    else if (facts.timer)
        cost += row.hold_read.full;
    return cost;
}

// BEGIN IMMEDIATE excludes a concurrent write while this proof is consumed.
// total_changes includes writer-connection writes; data_version detects commits
// on a different connection. A rollback may conservatively invalidate a proof.
inline auto reconciliation_snapshot(const SqliteWorkSession &s) {
    const auto rows =
        s.execute({"SELECT total_changes(),data_version FROM pragma_data_version", {}});
    require(rows.size() == 1 && rows.front().size() == 2);
    return std::pair{integer(rows.front()[0]), integer(rows.front()[1])};
}

} // namespace subathon::server::review_detail
