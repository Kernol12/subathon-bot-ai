// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "contribution_review_bundle_history.hpp"
#include "contribution_review_domain.hpp"

namespace subathon::server::review_detail {

inline bool qualified(StorageLimits limits) {
    return limits.statements_per_batch >= 768 && limits.statement_utf8_bytes >= 65536 &&
           limits.bound_value_bytes >= 8585216 && limits.result_rows >= 768 &&
           limits.result_value_bytes >= 16105472;
}

inline void catalog(const SqliteWorkSession &s) {
    require(terminal_partition::selected(s));
    struct Expected {
        QList<QByteArray> hashes;
        MigrationManifest manifest = contribution_review_repository_manifest();
        Expected() {
            for (const auto &step : manifest.steps)
                hashes.append(migration_content_sha256(step));
        }
    };
    static const Expected expected;
    const auto history =
        s.execute({"SELECT version, CAST(migration_id AS BLOB), content_sha256 "
                   "FROM main.subathon_migration_history ORDER BY version LIMIT 20",
                   {}});
    require(history.size() == expected.hashes.size());
    for (qsizetype n = 0; n < history.size(); ++n)
        require(integer(history[n][0]) == n + 1 &&
                history[n][1].toByteArray() == expected.manifest.steps[n].id.toUtf8() &&
                history[n][2].toByteArray() == expected.hashes[n]);
    const auto schema =
        s.execute({"SELECT type,name,tbl_name,sql FROM main.sqlite_schema WHERE "
                   "substr(tbl_name,1,16)='subathon_review_' ORDER BY type,name LIMIT 12",
                   {}});
    qsizetype n = 0;
    for (const auto &object : expected.manifest.steps.back().expected_schema) {
        if (!object.table_name.startsWith("subathon_review_"))
            continue;
        require(n < schema.size() && schema[n][0].toString() == object.type &&
                schema[n][1].toString() == object.name &&
                schema[n][2].toString() == object.table_name &&
                (object.sql ? schema[n][3].toString() == *object.sql : schema[n][3].isNull()));
        ++n;
    }
    require(n == schema.size());
}

struct StoredOutcome {
    ReviewOutcome outcome{S::ineligible, {}};
    std::string decision_ref;
    std::string plan;
    std::uint64_t original_revision{};
    std::uint64_t selected_revision{};
    std::string live;
    std::vector<ApplicationHoldVersionLocator> active;
    std::vector<ApplicationHoldVersionLocator> closed;
    std::optional<ApplicationHoldVersionLocator> created;
};

template <class W, class T> void outcome_fields(W &w, T &data) {
    w.tag(data.outcome.status, S::stale);
    w.ref(data.decision_ref);
    w.ref(data.plan);
    w.number(data.original_revision);
    w.number(data.selected_revision);
    require(data.original_revision > 0 && data.selected_revision >= data.original_revision);
    w.ref(data.live);
    auto &f = *data.outcome.facts;
    if constexpr (W::reading)
        f.verified.emplace();
    selector_fields(w, *f.verified);
    w.opt(f.bundle, [&](auto &key) {
        w.ref(key.operation_scope);
        w.ref(key.operation_ref);
    });
    int previous = -1;
    w.sequence(f.blockers, [&](auto &blocker) {
        w.tag(blocker, core::ContributionBlocker::settlement_review_required);
        require(int(blocker) > previous);
        previous = int(blocker);
    });
    w.opt(f.timer, [&](auto &t) { timer_fields(w, t); });
    auto locators = [&](auto &list) {
        std::string_view last;
        w.sequence(list, [&](auto &l) {
            w.ref(l.hold_ref);
            require(last.empty() || last < l.hold_ref);
            last = l.hold_ref;
            w.ref(l.version_ref);
        });
    };
    locators(data.active);
    locators(data.closed);
    w.opt(data.created, [&](auto &locator) {
        w.ref(locator.hold_ref);
        w.ref(locator.version_ref);
    });
}

inline QByteArray outcome_doc(const StoredOutcome &data) {
    Encode w;
    w.raw("SRO1");
    outcome_fields(w, data);
    return std::move(w.data);
}

inline StoredOutcome parse_outcome(const QByteArray &doc) {
    require(doc.size() <= entry_limit);
    Decode r(doc);
    r.magic("SRO1");
    StoredOutcome out;
    out.outcome.facts.emplace();
    outcome_fields(r, out);
    r.end();
    require(outcome_doc(out) == doc);
    return out;
}

inline QList<QVariant> key_args(const ApplicationPrincipalBinding &p,
                                const ApplicationOperationKey &k) {
    return {bytes(p.installation_ref), bytes(k.operation_scope), bytes(k.operation_ref)};
}

inline constexpr auto key_where = "installation_ref=? AND work_scope=? AND work_ref=?";
inline constexpr auto command_format = "subathon.internal.contribution-review.v1";

inline int decision_kind(const ReviewEntryIntent &intent, S status, bool terminal_reject) {
    if (status >= S::ineligible)
        return 0;
    if (intent.verb == V::accept)
        return status == S::accepted_pending || status == S::applied ? 1 : 0;
    if (intent.verb == V::reject || terminal_reject)
        return status == S::rejected ? 2 : 0;
    return 3;
}

inline bool valid_status(V verb, S status, bool bundled) {
    if (status >= S::ineligible)
        return !bundled;
    switch (verb) {
    case V::accept:
        return bundled ? status == S::applied : status == S::accepted_pending;
    case V::apply_accepted:
        return bundled && status == S::applied;
    case V::continue_timer:
        return bundled && status == S::timer_resolved;
    case V::reject:
    case V::reject_held:
        return !bundled && status == S::rejected;
    case V::retain_held:
        return !bundled && status == S::retained;
    }
    return false;
}

struct Header {
    QByteArray manifest;
    int count{};
    int cursor{1};
};

inline Header header(const SqliteWorkSession &s, const ApplicationPrincipalBinding &p,
                     const ApplicationOperationKey &key) {
    const auto rows = s.execute(
        {QString("SELECT CASE WHEN typeof(manifest)='blob' AND length(manifest) BETWEEN 1 AND "
                 "987136 THEN manifest END,entry_count,next_ordinal "
                 "FROM main.subathon_review_work WHERE ") +
             key_where,
         key_args(p, key)});
    if (rows.empty())
        return {};
    require(rows.size() == 1 && !rows[0][0].isNull());
    const auto count = integer(rows[0][1]);
    const auto cursor = integer(rows[0][2]);
    require(count > 0 && count <= int(row_limit) && cursor >= 2 && cursor <= count + 1);
    return {rows[0][0].toByteArray(), int(count), int(cursor)};
}

inline std::optional<Decision> decision(const SqliteWorkSession &s, const ReviewSelector &sel,
                                        bool &rejected) {
    const auto rows =
        s.execute({"SELECT decision_kind, status, receipt_id, CASE WHEN length(outcome)<=61440 "
                   "THEN outcome END, ordinal, installation_ref,work_scope,work_ref "
                   "FROM main.subathon_review_entries WHERE contribution_id=? "
                   "AND decision_kind IN (1,2) LIMIT 3",
                   {id(sel.cid.value)}});
    require(rows.size() <= 2);
    std::optional<Decision> accepted;
    for (const auto &row : rows) {
        require(row[2] == id(sel.receipt.value) && !row[3].isNull());
        const auto audit = parse_outcome(row[3].toByteArray());
        require(audit.outcome.facts->verified == sel &&
                integer(row[1]) == int(audit.outcome.status));
        const auto kind = integer(row[0]);
        if (kind == 2) {
            require(!rejected && audit.outcome.status == S::rejected);
            rejected = true;
            continue;
        }
        require(!accepted && (audit.outcome.status == S::accepted_pending ||
                              audit.outcome.status == S::applied));
        if (audit.outcome.status != S::accepted_pending)
            continue;
        const auto installation = text_cell(row[5]);
        const ApplicationOperationKey key{text_cell(row[6]), text_cell(row[7])};
        const auto h = header(s, {ApplicationPrincipalKind::account, installation, {}}, key);
        const auto ordinal = integer(row[4]);
        require(ordinal >= 1 && ordinal < h.cursor);
        const auto cmd = decode_manifest(h.manifest);
        require(same(cmd.key, key) && cmd.principal.installation_ref == installation &&
                cmd.entries.size() == size_t(h.count));
        const auto &entry = cmd.entries.at(size_t(ordinal - 1));
        require(entry.intent.verb == V::accept && entry.intent.selector == sel &&
                entry.decision_ref == audit.decision_ref);
        accepted = Decision{audit.decision_ref, entry.intent.mode};
    }
    return accepted;
}

inline QByteArray entry_document(const ReviewEntry &entry) {
    Encode encoded;
    entry_fields(encoded, entry);
    return std::move(encoded.data);
}

inline ApplicationOperationKey application_key(const ReviewCommand &c, int ordinal) {
    return {"review-application", distinct_id(c, ordinal, "bundle")};
}

inline void validate_plan_link(const SqliteWorkSession &s, const ReviewSelector &selection,
                               const StoredOutcome &data) {
    const auto rows = s.execute(
        {"SELECT h.contribution_id,h.receipt_id,r.parent_revision FROM main.subathon_plan_headers "
         "h "
         "JOIN main.subathon_plan_revisions r USING(plan_ref) WHERE h.plan_ref=? AND r.revision=?",
         {bytes(data.plan), id(data.selected_revision)}});
    require(rows.size() == 1 && rows[0][0] == id(selection.cid.value) &&
            rows[0][1] == id(selection.receipt.value));
    if (data.selected_revision != data.original_revision)
        require(rows[0][2] == id(data.original_revision));
}

inline bool validate_hold_links(const SqliteWorkSession &s, const ReviewCommand &cmd, int ordinal,
                                const StoredOutcome &data) {
    const auto &f = *data.outcome.facts;
    const auto &intent = cmd.entries.at(size_t(ordinal - 1)).intent;
    if (data.created) {
        require(data.outcome.status == S::accepted_pending &&
                data.created->hold_ref == distinct_id(cmd, ordinal, "hold") &&
                data.created->version_ref == distinct_id(cmd, ordinal, "held"));
        auto hold = SqliteApplicationHoldStore(s).find(data.created->hold_ref);
        require(hold && same(hold->first_version.locator, *data.created) &&
                hold->cause.source.subject.id == ApplicationHoldSubject{f.verified->cid}.id &&
                hold->cause.source.receipt_id == f.verified->receipt &&
                hold->cause.plan_evidence.plan &&
                hold->cause.plan_evidence.plan->plan_ref == data.plan &&
                hold->cause.plan_evidence.plan->revision == data.selected_revision &&
                hold->cause.command.operation.operation_scope == "review-hold" &&
                hold->cause.command.operation.operation_ref ==
                    distinct_id(cmd, ordinal, "hold-command") &&
                bool(hold->cause.timer) == bool(f.timer));
    }
    bool whole = false;
    if (f.timer) {
        const auto &t = *f.timer;
        require(data.created || intent.timer);
        const auto &locator = data.created ? *data.created : intent.timer->locator;
        require(same(locator, t.locator));
        const auto hold = SqliteApplicationHoldStore(s).find(t.locator.hold_ref);
        require(hold &&
                hold->cause.source.subject.id == ApplicationHoldSubject{f.verified->cid}.id &&
                hold->cause.source.receipt_id == f.verified->receipt && hold->cause.timer &&
                hold->cause.timer->request_ordinal == t.ordinal &&
                hold->cause.timer->unapplied_units == t.signed_request);
        whole = hold->cause.timer->composition == ApplicationHoldComposition::defer_whole_input;
        const auto versions = s.execute(
            {"SELECT 1 FROM main.subathon_hold_versions WHERE hold_ref=? AND version_ref=? "
             "AND state=0",
             {bytes(t.locator.hold_ref), bytes(t.locator.version_ref)}});
        require(versions.size() == 1);
    }
    for (const auto &closed : data.closed) {
        const auto prior = std::ranges::find(data.active, closed.hold_ref,
                                             &ApplicationHoldVersionLocator::hold_ref);
        require(prior != data.active.end());
        const auto rows = s.execute(
            {"SELECT c.subject_kind,c.subject_id,c.source_receipt_id,v.parent_version_ref,"
             "m.mutation_kind,m.decision_ref,m.application_scope,m.application_ref "
             "FROM main.subathon_hold_versions v JOIN main.subathon_hold_causes c USING(hold_ref) "
             "JOIN main.subathon_hold_mutations m USING(mutation_ref) "
             "WHERE v.hold_ref=? AND v.version_ref=? AND v.state=1 AND v.mutation_ref=?",
             {bytes(closed.hold_ref), bytes(closed.version_ref),
              bytes(distinct_id(cmd, ordinal, "hold-resolution"))}});
        require(rows.size() == 1 && integer(rows[0][0]) == 0 &&
                rows[0][1] == id(f.verified->cid.value) &&
                rows[0][2] == id(f.verified->receipt.value) &&
                rows[0][3] == bytes(prior->version_ref) && integer(rows[0][4]) == 1 &&
                rows[0][5] == bytes(data.decision_ref) &&
                (f.bundle ? rows[0][6] == bytes(f.bundle->operation_scope) &&
                                rows[0][7] == bytes(f.bundle->operation_ref)
                          : rows[0][6].isNull() && rows[0][7].isNull()));
    }
    return whole;
}

struct Prefix {
    ReviewReply reply;
    std::string last_state;
    int cursor{1};
};

inline Prefix read_prefix(const SqliteWorkSession &s, const ReviewCommand &cmd, const Header &h,
                          const Authority &auth, int initial_cursor) {
    const auto args = key_args(cmd.principal, cmd.key);
    // The length projection prevents a malformed LOB from consuming the entire
    // work budget before its selected ordinal is checked.
    const auto rows = s.execute(
        {QString("SELECT ordinal,status,contribution_id,receipt_id,decision_kind,"
                 "CASE WHEN typeof(outcome)='blob' AND length(outcome) BETWEEN 1 AND 61440 "
                 "THEN outcome END,application_scope,application_ref,outcome IS NULL "
                 "FROM main.subathon_review_entries WHERE ") +
             key_where + " ORDER BY ordinal LIMIT 17",
         args});
    require(rows.size() == h.cursor - 1);
    Prefix out;
    out.cursor = h.cursor;
    const auto read_bundle = bundle_history(s);
    for (size_t n = 0; n < size_t(rows.size()); ++n) {
        const auto &row = rows.at(qsizetype(n));
        const auto &entry = cmd.entries.at(n);
        const auto &sel = entry.intent.selector;
        const auto ordinal = int(n + 1);
        require(integer(row[0]) == ordinal);
        const auto status = integer(row[1]);
        require(status >= 0 && status <= 8);
        const auto disposition = ordinal < initial_cursor ? D::existing : D::committed;
        if (status >= 7) {
            require(row[2].isNull() && row[3].isNull() && row[4].isNull() && row[5].isNull() &&
                    row[6].isNull() && row[7].isNull() && integer(row[8]) == 1);
            out.reply.rows.push_back(
                {ordinal, disposition, ReviewOutcome{status == 7 ? S::denied : S::ineligible, {}}});
            continue;
        }
        require(row[2] == id(sel.cid.value) && row[3] == id(sel.receipt.value) && anchor(s, sel));
        require(!row[5].isNull());
        auto data = parse_outcome(row[5].toByteArray());
        auto &outcome = data.outcome;
        auto &facts = *outcome.facts;
        require(outcome.status == S(status) && data.decision_ref == entry.decision_ref &&
                facts.verified == sel &&
                valid_status(entry.intent.verb, outcome.status, bool(facts.bundle)));
        const auto kind = row[4].isNull() ? 0 : integer(row[4]);
        validate_plan_link(s, sel, data);
        const bool whole = validate_hold_links(s, cmd, ordinal, data);
        require(kind == decision_kind(entry.intent, outcome.status,
                                      entry.intent.verb == V::reject_held && whole));
        std::optional<RecordedApplicationBundle> record;
        std::vector<ApplicationEffect> none;
        const auto *effects = &none;
        if (facts.bundle) {
            require(text_cell(row[6]) == facts.bundle->operation_scope &&
                    text_cell(row[7]) == facts.bundle->operation_ref &&
                    same(*facts.bundle, application_key(cmd, ordinal)));
            record = read_bundle(*facts.bundle);
            require(record && record->intent.kind == ApplicationOperationKind::original &&
                    record->intent.subject_kind == ApplicationSubjectKind::contribution &&
                    record->intent.contribution_id == sel.cid &&
                    record->intent.receipt_id == sel.receipt &&
                    record->intent.plan.plan_ref == data.plan &&
                    record->intent.plan.revision == data.selected_revision &&
                    record->intent.choices.format_ref == command_format &&
                    record->intent.choices.document == entry_document(entry) &&
                    record->audit.actor_ref == cmd.actor &&
                    record->audit.decision_ref == entry.decision_ref &&
                    record->audit.reason == entry.reason &&
                    record->audit.envelope.format_ref == command_format &&
                    record->audit.envelope.document == row[5].toByteArray());
            if (record->result.format_ref == "subathon.internal.application-result.v1") {
                terminal_partition::Match match(record->result.document);
                match.raw("SAR1");
                application_result_wire::put_result_fields(
                    match, record->intent.choices, record->intent.expected_state_token,
                    record->advanced_before.token, record->after.token, false, record->effects,
                    record->jobs, record->transitions);
                require(match.remaining().empty());
            } else {
                require(record->result.format_ref == terminal_partition::result_format);
            }
            out.last_state = record->after.token;
            effects = &record->effects;
        } else {
            require(row[6].isNull() && row[7].isNull());
            if (outcome.status == S::stale || outcome.status == S::ineligible)
                effects = nullptr;
        }
        if (allowed(
                auth.check(s, ReviewAuthorityUse::execute_row, &entry.intent, &facts, effects)) &&
            allowed(
                auth.check(s, ReviewAuthorityUse::disclose_row, &entry.intent, &facts, effects)))
            out.reply.rows.push_back({ordinal, disposition, std::move(outcome)});
        else
            out.reply.rows.push_back({ordinal, {}, ReviewOutcome{S::denied, {}}});
    }
    return out;
}

inline void one_change(const SqliteWorkSession &s, SqlStatement statement) {
    const auto rows = s.execute(statement);
    require(rows.size() == 1 && rows[0].size() == 1 && integer(rows[0][0]) == 1);
}

inline void append_journal(const SqliteWorkSession &s, const ReviewCommand &c, int ordinal,
                           const QByteArray &document, const StoredOutcome &data, int kind) {
    const auto args = key_args(c.principal, c.key);
    if (ordinal == 1) {
        auto fields = args;
        fields.append({document, int(c.entries.size())});
        one_change(
            s, {"INSERT INTO main.subathon_review_work VALUES(?,?,?,?,?,1) RETURNING 1", fields});
    }
    auto fields = args;
    const auto status = data.outcome.status;
    const auto *facts = data.outcome.facts ? &*data.outcome.facts : nullptr;
    fields.append(
        {ordinal, facts ? int(status) : (status == S::denied ? 7 : 8),
         facts ? id(facts->verified->cid.value) : QVariant{},
         facts ? id(facts->verified->receipt.value) : QVariant{},
         kind ? QVariant(kind) : QVariant{}, facts ? QVariant(outcome_doc(data)) : QVariant{},
         facts && facts->bundle ? QVariant(bytes(facts->bundle->operation_scope)) : QVariant{},
         facts && facts->bundle ? QVariant(bytes(facts->bundle->operation_ref)) : QVariant{}});
    one_change(s,
               {"INSERT INTO main.subathon_review_entries VALUES(?,?,?,?,?,?,?,?,?,?) RETURNING 1",
                fields});
    auto cursor_args = args;
    cursor_args.append(ordinal);
    one_change(s,
               {QString("UPDATE main.subathon_review_work SET next_ordinal=next_ordinal+1 WHERE ") +
                    key_where + " AND next_ordinal=? RETURNING 1",
                cursor_args});
}

} // namespace subathon::server::review_detail
