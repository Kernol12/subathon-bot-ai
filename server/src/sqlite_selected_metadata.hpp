// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Included after the owning Access decoder. Planning borrows its exact session;
// only bounded routing keys and integer scalars are read before payload admission.
namespace subathon::server {
namespace {
class SelectedMetadata : public Access {
  public:
    using Failure = selected_record_read::Failure;
    struct Budget {
        size_t bytes{}, items{};
    };
    // Ephemeral metadata plans deduplicate physical locators, never authority or
    // payloads. Reusing a plan still charges each returned owning occurrence.
    std::map<std::string, Budget> states, scalar_states;
    std::map<std::uint64_t, Budget> receipts;
    std::set<std::pair<bool, std::string>> retained_keys;
    std::set<std::pair<std::string, std::uint64_t>> plans;
    explicit SelectedMetadata(const SqliteWorkSession &s, SqliteBundleLimits l)
        : Access(s, l, ReadPolicy::selected) {}
    static void add(size_t &n, size_t delta, size_t limit) {
        if (n > limit || delta > limit - n)
            throw Failure::resource_limit;
        n += delta;
    }
    void charge(Budget &b, size_t n, size_t items = 0) {
        add(b.bytes, n, size_t(limits_.owned_bytes));
        add(b.items, items, size_t(limits_.result_items));
    }
    // r/v/c/u/i = reference/value/exact component/u64/integer. Uppercase
    // permits NULL. Integer values are bounded scalars; BLOBs stay in SQLite.
    SqlRows rows(const QString &table, const QStringList &cols, std::string_view schema,
                 const QString &where, Values values, qsizetype cap = 0) {
        const bool singular = cap == 0;
        if (singular)
            cap = 1;
        if (schema.size() != size_t(cols.size()) || cap < 1)
            throw Failure::invalid_data;
        QStringList fields;
        for (qsizetype i = 0; i < cols.size(); ++i) {
            const auto &c = cols[i];
            fields.append("CASE WHEN typeof(" + c + ")='integer' THEN " + c + " ELSE NULL END");
            fields.append("typeof(" + c + ")");
            fields.append("length(CAST(" + c + " AS BLOB))");
        }
        values.append(qint64(cap + 1));
        auto result = execute(
            {"SELECT " + fields.join(',') + " FROM main." + table + " WHERE " + where + " LIMIT ?",
             std::move(values)});
        if (result.size() > cap)
            throw singular ? Failure::invalid_data : Failure::resource_limit;
        for (const auto &r : result) {
            shape(r, cols.size());
            for (qsizetype i = 0; i < cols.size(); ++i) {
                char k = schema[size_t(i)];
                if (k >= 'A' && k <= 'Z') {
                    if (null(r, i))
                        continue;
                    k += 'a' - 'A';
                }
                if (k == 'i') {
                    (void)integer(r, i);
                    continue;
                }
                if (!type(r, i, "blob") || r[i * 3 + 2].isNull() || r[i * 3 + 2].toLongLong() < 0)
                    throw Failure::invalid_data;
                const auto n = r[i * 3 + 2].toLongLong();
                const qsizetype bound = k == 'u'   ? 8
                                        : k == 'c' ? limits_.component_bytes
                                        : k == 'v' ? limits_.value_bytes
                                                   : limits_.reference_bytes;
                if (((k == 'u') && n != 8) || (k == 'c' && n == 0))
                    throw Failure::invalid_data;
                if (n > bound || std::uint64_t(n) > std::string{}.max_size())
                    throw Failure::resource_limit;
            }
        }
        return result;
    }
    SqlRow required_meta(const QString &t, const QStringList &c, std::string_view schema,
                         const QString &w, Values b) {
        auto r = rows(t, c, schema, w, std::move(b));
        if (r.size() != 1)
            throw Failure::invalid_data;
        return r.front();
    }
    size_t size(const SqlRow &r, qsizetype i, size_t scalar = 8) {
        if (null(r, i))
            return 0;
        return type(r, i, "integer") ? scalar : size_t(r[i * 3 + 2].toLongLong());
    }
    size_t sum(const SqlRow &r, std::initializer_list<int> indices) {
        size_t n = 0;
        for (auto i : indices)
            add(n, size(r, i), std::numeric_limits<size_t>::max());
        return n;
    }
    void measured(Budget &b, const SqlRow &r, std::initializer_list<int> indices, size_t fixed = 0,
                  size_t items = 0) {
        charge(b, fixed, items);
        charge(b, sum(r, indices));
    }
    SqlRow route(const QString &t, const QStringList &cols, const QString &w, Values b) {
        return *one(t, cols, w, std::move(b), true);
    }
    void retained(const std::string &ref, bool context = false) {
        if (!retained_keys.insert({context, ref}).second)
            return;
        const auto table =
            context ? "subathon_retained_contexts" : "subathon_retained_configurations";
        QStringList cols{"reference", "document"};
        if (context)
            cols.append("configuration_ref");
        auto r = required_meta(table, cols, context ? "rvR" : "rv", "reference=?", {bytes(ref)});
        Budget b;
        measured(b, r,
                 context ? std::initializer_list<int>{0, 1, 2} : std::initializer_list<int>{0, 1});
        if (context && !null(r, 2)) {
            auto link = route(table, {"configuration_ref"}, "reference=?", {bytes(ref)});
            retained(string(link, 0, limits_.reference_bytes, true));
        }
    }
    Budget receipt(ReceiptId rid) {
        if (auto it = receipts.find(rid.value); it != receipts.end())
            return it->second;
        auto b = Values{encode_plan_revision_uint64(rid.value)};
        auto r = required_meta("subathon_receipts", receipt_columns, "urrrrviIVVVVRRRRirrr",
                               "receipt_id=?", b);
        Budget budget;
        measured(budget, r, {1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 18, 19}, 20);
        auto links = route("subathon_receipts", {"configuration_ref", "modifier_context_ref"},
                           "receipt_id=?", b);
        retained(string(links, 0, limits_.reference_bytes, true));
        retained(string(links, 1, limits_.reference_bytes, true), true);
        receipts.emplace(rid.value, budget);
        return budget;
    }
    Budget observation_source(ReceiptId rid) {
        auto r = required_meta("subathon_contribution_observations", observation_columns(),
                               "uUiiRRRRRIVVRIIIRIRRIII", "receipt_id=?",
                               {encode_plan_revision_uint64(rid.value)});
        auto out = receipt(rid);
        measured(out, r, {1, 4, 5, 6, 7, 8}, 2);
        if (!null(r, 9)) {
            measured(out, r, {10, 11, 12, 16, 18, 19}, 7);
            if (!null(r, 17))
                charge(out, 1);
        }
        return out;
    }
    void contribution(ContributionId cid, bool full = false) {
        auto b = Values{encode_contribution_id(cid)};
        const auto cols =
            full ? contribution_columns() : QStringList{"contribution_id", "original_receipt_id"};
        required_meta("subathon_contributions", cols, full ? "uuRRRRiiVVRiiirIRRiii" : "uu",
                      "contribution_id=?", b);
        auto r = route("subathon_contributions", {"original_receipt_id"}, "contribution_id=?", b);
        if (!full) {
            required_meta(
                "subathon_receipts",
                {"receipt_id", "provider", "source_account", "identifier_namespace", "opaque_id"},
                "urrrr", "receipt_id=?", {encode_plan_revision_uint64(id(r, 0, true))});
            return;
        }
        auto receipt_budget = receipt(ReceiptId{id(r, 0, true)});
        if (full) {
            auto ob = Values{encode_plan_revision_uint64(id(r, 0, true))};
            auto obs = required_meta("subathon_contribution_observations", observation_columns(),
                                     "uUiiRRRRRIVVRIIIRIRRIII", "receipt_id=?", ob);
            auto current = required_meta("subathon_contributions", contribution_columns(),
                                         "uuRRRRiiVVRiiirIRRiii", "contribution_id=?", b);
            // Exact independent materialization of ApplicationContributionInput:
            // the receipt facts/arrival occur in both contribution and anchor.
            auto receipt_meta = required_meta("subathon_receipts", receipt_columns,
                                              "urrrrviIVVVVRRRRirrr", "receipt_id=?", ob);
            Budget out = receipt_budget;
            measured(out, receipt_meta, {5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 18, 19}, 12);
            measured(out, current, {0, 1, 2, 3, 4, 5, 8, 9, 10, 14, 16, 17}, 8);
            if (!null(current, 15))
                charge(out, 1);
            measured(out, obs, {1, 4, 5, 6, 7, 8, 10, 11, 12, 16, 18, 19}, 9);
            if (!null(obs, 17))
                charge(out, 1);
        }
    }
    void plan_source(const ApplicationPlanKey &key) {
        if (!plans.insert({key.plan_ref, key.revision}).second)
            return;
        const auto w = QString("plan_ref=? AND revision=?");
        Values b{bytes(key.plan_ref), encode_plan_revision_uint64(key.revision)};
        auto h = required_meta("subathon_plan_headers", header_columns, "rrRruUu", "plan_ref=?",
                               {bytes(key.plan_ref)});
        required_meta("subathon_plan_revisions", {"plan_ref", "revision"}, "ru",
                      "plan_ref=? AND revision=(SELECT revision FROM main.subathon_plan_revisions "
                      "WHERE plan_ref=? ORDER BY revision DESC LIMIT 1)",
                      {bytes(key.plan_ref), bytes(key.plan_ref)});
        auto header = route("subathon_plan_headers", {"receipt_id", "contribution_id"},
                            "plan_ref=?", {bytes(key.plan_ref)});
        auto receipt_budget = receipt(ReceiptId{id(header, 0, true)});
        if (!null(header, 1))
            contribution(ContributionId{id(header, 1)});
        auto revision = required_meta("subathon_plan_revisions", revision_columns,
                                      "ruUiiVVRiiirIRRiiiiRVVVVVRRIIRIRVR", w, b);
        Budget out = receipt_budget;
        measured(out, h, {1, 2, 3, 5});
        measured(out, revision, {1, 2, 5, 6, 7, 11, 13, 14}, 9);
        if (!null(revision, 12))
            charge(out, 1);
        measured(out, revision, {19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 31, 32, 33});
        if (!null(revision, 30))
            charge(out, 1);
        // The owning decoder selects one immutable revision and a direct parent,
        // never audits every revision in this private read policy.
        auto parent = route("subathon_plan_revisions", {"parent_revision"}, w, b);
        if (!null(parent, 0))
            required_meta("subathon_plan_revisions", {"plan_ref", "revision"}, "ru", w,
                          {bytes(key.plan_ref), encode_plan_revision_uint64(id(parent, 0, true))});
        for (const auto &[t, cols, schema] :
             std::array{std::tuple{QString("subathon_plan_rules"), rule_columns,
                                   std::string_view("ruirriv")},
                        std::tuple{QString("subathon_plan_modifiers"), modifier_columns,
                                   std::string_view("ruirrrrrrv")},
                        std::tuple{QString("subathon_plan_requests"), request_columns,
                                   std::string_view("ruirrrriiccivr")}}) {
            auto rs = rows(t, cols, schema, w, b, limits_.plan_items_per_vector);
            charge(out, 0, size_t(rs.size()));
            for (const auto &r : rs) {
                if (t == "subathon_plan_requests")
                    measured(out, r, {3, 4, 5, 6, 9, 10, 12, 13}, 10);
                else if (t == "subathon_plan_rules")
                    measured(out, r, {3, 4, 6}, 1);
                else
                    measured(out, r, {3, 4, 5, 6, 7, 8, 9});
            }
        }
    }
    void state_source(const std::string &token, Budget *output = nullptr,
                      bool scalar_only = false) {
        auto &state_plans = scalar_only ? scalar_states : states;
        if (auto it = state_plans.find(token); it != state_plans.end()) {
            if (output)
                charge(*output, it->second.bytes, it->second.items);
            return;
        }
        auto r = required_meta("subathon_app_states", app_states_columns, "rrvriiiiiii", "token=?",
                               {bytes(token)});
        auto links = route("subathon_app_states", {"configuration_ref"}, "token=?", {bytes(token)});
        retained(string(links, 0, limits_.reference_bytes, true));
        auto balances = rows(
            "subathon_app_balances",
            scalar_only ? QStringList{"token", "balance_kind", "target_ref"} : app_balances_columns,
            scalar_only ? "rir" : "riri", "token=?", {bytes(token)}, limits_.balances_per_state);
        if (integer(r, 10) != balances.size())
            throw Failure::invalid_data;
        Budget out;
        measured(out, r, {0, 1, 2, 3}, 27, scalar_only ? 0 : size_t(balances.size()));
        if (!scalar_only) {
            for (const auto &v : balances)
                measured(out, v, {2}, 9);
        }
        state_plans.emplace(token, out);
        if (output)
            charge(*output, out.bytes, out.items);
    }
    std::set<std::pair<std::string, std::string>> application_stack;
    bool application(const ApplicationOperationKey &k, Budget *output = nullptr,
                     std::optional<qint64> selected_basis = {}) {
        const auto locator = std::pair{k.operation_scope, k.operation_ref};
        if (!application_stack.insert(locator).second)
            throw Failure::invalid_data;
        struct Pop {
            decltype(application_stack) &stack;
            decltype(locator) key;
            ~Pop() { stack.erase(key); }
        } pop{application_stack, locator};
        auto w = operation_where();
        auto b = operation_values(k);
        auto rs = rows("subathon_app_operations", app_operations_columns, "rriiuUrurrrvrvR", w, b);
        if (rs.isEmpty())
            return false;
        auto &r = rs.front();
        Budget local;
        auto &out = output ? *output : local;
        measured(out, r, {0, 1, 4, 5, 6, 7, 8, 11, 12, 13, 14}, 2);
        if (selected_basis) {
            const auto format = route("subathon_app_results", {"format_ref"}, w, b);
            // SAR2 binds the complete physical effect and job vectors, even
            // when reached through a selected reversal basis.
            if (string(format, 0, limits_.reference_bytes, true) ==
                terminal_partition::result_format)
                selected_basis.reset();
        }
        auto routes =
            route("subathon_app_operations",
                  {"plan_ref", "plan_revision", "head_token", "before_token", "after_token"}, w, b);
        for (int i = 2; i < 5; ++i)
            state_source(string(routes, i, limits_.reference_bytes, true), &out,
                         selected_basis.has_value());
        plan_source({string(routes, 0, limits_.reference_bytes, true), id(routes, 1, true)});
        auto cs = rows("subathon_app_contributions", app_contributions_columns,
                       "rruiiVVRiiirIRRiiii", w, b);
        if (!cs.isEmpty()) {
            // Before-state is returned twice: intent and contribution snapshot.
            measured(out, cs.front(), {5, 6, 7, 11, 13, 14}, 8);
            measured(out, cs.front(), {5, 6, 7, 11, 13, 14}, 8 + 9);
            if (!null(cs.front(), 12))
                charge(out, 2);
        }
        rows("subathon_app_claims", app_claims_columns, "iurr", w, b);
        auto result = required_meta("subathon_app_results", app_results_columns, "rrrviiiii", w, b);
        measured(out, result, {2, 3});
        auto audit = required_meta("subathon_app_audit", app_audit_columns, "rrRRVrv", w, b);
        measured(out, audit, {2, 3, 4, 5, 6});
        struct Child {
            QString table;
            QStringList cols;
            std::string_view schema;
            qsizetype cap;
        };
        std::array children{
            Child{"subathon_app_effects", app_effects_columns, "rriruiicciccIIIv",
                  limits_.effects_per_bundle},
            Child{"subathon_app_components", app_components_columns, "rriiirR",
                  limits_.result_items},
            Child{"subathon_app_jobs", app_jobs_columns, "rrirIRIrv", limits_.jobs_per_bundle},
            Child{"subathon_app_transitions", app_transitions_columns, "rrii",
                  limits_.transitions_per_bundle},
            Child{"subathon_app_reversals", app_reversals_columns, "rrirriiicccccciiii",
                  limits_.links_per_bundle}};
        if (selected_basis) {
            // At most three scalar component rows per original effect. These
            // validation rows are not a returned component vector/item charge.
            const auto max = std::numeric_limits<qsizetype>::max() - 1;
            children[1].cap =
                limits_.effects_per_bundle > max / 3 ? max : limits_.effects_per_bundle * 3;
            children[0].cols =
                QStringList(app_effects_columns.begin(), app_effects_columns.begin() + 7);
            children[0].schema = "rriruii";
            children[1].cols = {"operation_scope", "operation_ref", "request_ordinal",
                                "component_kind",  "before_token",  "target_ref"};
            children[1].schema = "rriirR";
            children[2].cols = QStringList(app_jobs_columns.begin(), app_jobs_columns.begin() + 7);
            children[2].schema = "rrirIRI";
        }
        for (size_t i = 0; i < children.size(); ++i) {
            const auto &c = children[i];
            auto values = rows(c.table, c.cols, c.schema, w, b, c.cap);
            if (integer(result, qsizetype(i + 4)) != values.size())
                throw Failure::invalid_data;
            if (selected_basis)
                continue;
            charge(out, 0, size_t(values.size()));
            for (const auto &v : values) {
                if (i == 0) {
                    measured(out, v, {7, 8, 10, 11, 12, 15}, 18);
                    if (!null(v, 13))
                        charge(out, 1);
                    if (!null(v, 14))
                        charge(out, 1);
                }
                if (i == 1)
                    measured(out, v, {6}, 9);
                if (i == 2)
                    measured(out, v, {3, 4, 5, 6, 7, 8});
                if (i == 3)
                    charge(out, 1);
                if (i == 4)
                    measured(out, v, {3, 4, 8, 9, 10, 11, 12, 13}, 50);
            }
        }
        if (selected_basis) {
            auto selected_bindings = b;
            selected_bindings.append(*selected_basis);
            auto effect =
                required_meta("subathon_app_effects", app_effects_columns, "rriruiicciccIIIv",
                              w + " AND request_ordinal=?", selected_bindings);
            measured(out, effect, {7, 8, 10, 11, 12, 15}, 18, 1);
            if (!null(effect, 13))
                charge(out, 1);
            if (!null(effect, 14))
                charge(out, 1);
            auto components = rows("subathon_app_components", app_components_columns, "rriiirR",
                                   w + " AND request_ordinal=?", selected_bindings, 3);
            charge(out, 0, size_t(components.size()));
            for (const auto &component : components)
                measured(out, component, {6}, 9);
            auto targets =
                execute({"SELECT " + projection({"before_token", "component_kind", "target_ref"}) +
                             " FROM main.subathon_app_components WHERE " + w +
                             " AND request_ordinal=? AND target_ref IS NOT NULL LIMIT 4",
                         selected_bindings});
            for (const auto &target : targets)
                required_meta("subathon_app_balances", app_balances_columns, "riri",
                              "token=? AND balance_kind=? AND target_ref=?",
                              {bytes(string(target, 0, limits_.reference_bytes, true)),
                               integer(target, 1),
                               bytes(string(target, 2, limits_.reference_bytes, true))});
        }
        if (integer(r, 2) == 1) {
            const QStringList cols{"original_scope", "original_operation",
                                   "original_request_ordinal", "original_component_kind",
                                   "basis_position"};
            auto links = execute({"SELECT " + projection(cols) +
                                      " FROM main.subathon_app_reversals WHERE " + w + " LIMIT ?",
                                  [&] {
                                      auto v = b;
                                      v.append(qint64(limits_.links_per_bundle + 1));
                                      return v;
                                  }()});
            for (const auto &link : links) {
                const ApplicationOperationKey original{
                    string(link, 0, limits_.reference_bytes, true),
                    string(link, 1, limits_.reference_bytes, true)};
                // Reversal bases are original operations; reject cycles and a
                // reversal-of-reversal before expanding any further closure.
                auto op = required_meta("subathon_app_operations", {"operation_kind"}, "i",
                                        operation_where(), operation_values(original));
                if (integer(op, 0) != 0 || !application(original, nullptr, integer(link, 2)))
                    throw Failure::invalid_data;
                Values basis{bytes(original.operation_scope), bytes(original.operation_ref),
                             integer(link, 2), integer(link, 3)};
                const auto where_ = basis_where();
                auto head = required_meta("subathon_app_reversal_heads", app_reversal_heads_columns,
                                          "rriiiirri", where_, basis);
                (void)head;
                auto last =
                    route("subathon_app_reversal_heads",
                          {"last_scope", "last_operation", "last_request_ordinal"}, where_, basis);
                auto last_bindings =
                    Values{bytes(string(last, 0, limits_.reference_bytes, true)),
                           bytes(string(last, 1, limits_.reference_bytes, true)), integer(last, 2)};
                last_bindings.append(basis);
                required_meta("subathon_app_reversals", app_reversals_columns, "rrirriiicccccciiii",
                              operation_where() + " AND request_ordinal=? AND " + where_,
                              last_bindings);
                auto position = integer(link, 4);
                if (position < 1)
                    throw Failure::invalid_data;
                if (position > 1) {
                    auto previous = basis;
                    previous.append(position - 1);
                    required_meta("subathon_app_reversals", app_reversals_columns,
                                  "rrirriiicccccciiii", where_ + " AND basis_position=?", previous);
                }
            }
        }
        return true;
    }
};
} // namespace
} // namespace subathon::server
