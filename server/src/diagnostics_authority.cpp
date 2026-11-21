// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/diagnostics.hpp>
#include <subathon/server/sqlite_receipts.hpp>

#include <algorithm>
#include <limits>

namespace subathon::server::diagnostics {
namespace {
constexpr auto base_operation = "diagnostics.export.v1";
constexpr auto detail_operation = "diagnostics.events.export.v1";

bool valid_selectors(const std::vector<Selector> &selectors) {
    if (selectors.size() > 32)
        return false;
    for (std::size_t i = 0; i < selectors.size(); ++i) {
        const auto &s = selectors[i];
        if (!s.receipt || s.attempt <= 0 || s.fields.empty() || s.fields.size() > 2 ||
            (s.fields.size() == 2 && s.fields[0] == s.fields[1]) ||
            std::ranges::any_of(s.fields, [](auto f) {
                return f != Field::disposition && f != Field::schema_version;
            }))
            return false;
        for (std::size_t j = 0; j < i; ++j)
            if (s.receipt == selectors[j].receipt && s.attempt == selectors[j].attempt)
                return false;
    }
    return true;
}
Attempt read_attempt(const SqliteWorkSession &session, const Selector &selector) {
    QString columns;
    for (auto field : selector.fields) {
        if (!columns.isEmpty())
            columns += ',';
        columns += field == Field::disposition
                       ? "CASE WHEN typeof(disposition)='integer' THEN disposition END"
                       : "CASE WHEN typeof(schema_version)='integer' THEN schema_version END";
    }
    const auto rows = session.execute(
        {"SELECT " + columns +
             " FROM main.subathon_receipt_attempts WHERE receipt_id=? AND ordinal=? LIMIT 2",
         {encode_receipt_uint64(selector.receipt), qint64(selector.attempt)}});
    if (rows.empty())
        return {};
    if (rows.size() != 1 || rows[0].size() != qsizetype(selector.fields.size()))
        throw SqliteWorkFailure(WorkError::sql_failure);
    std::vector<AttemptValue> result;
    for (std::size_t i = 0; i < selector.fields.size(); ++i) {
        const auto &value = rows[0][qsizetype(i)];
        if (value.isNull() ||
            (value.typeId() != QMetaType::LongLong && value.typeId() != QMetaType::Int))
            throw SqliteWorkFailure(WorkError::sql_failure);
        const auto exact = value.toLongLong();
        if (selector.fields[i] == Field::disposition) {
            switch (exact) {
            case 0:
                result.emplace_back(core::ReceiptDisposition::created);
                break;
            case 1:
                result.emplace_back(core::ReceiptDisposition::retry);
                break;
            case 2:
                result.emplace_back(core::ReceiptDisposition::conflict);
                break;
            default:
                throw SqliteWorkFailure(WorkError::sql_failure);
            }
        } else {
            if (exact < 0 || exact > std::numeric_limits<std::uint32_t>::max())
                throw SqliteWorkFailure(WorkError::sql_failure);
            result.emplace_back(std::uint32_t(exact));
        }
    }
    return result;
}

class SqliteCurrent final : public Current,
                            private CurrentSessionSource,
                            private CurrentSessionClock {
  public:
    SqliteCurrent(const SqliteWorkSession &work, const SessionLookupRequest &lookup,
                  CurrentSessionClock &clock, const PermissionGraphTrust &trust)
        : work_(work), lookup_(lookup), clock_(clock), trust_(trust), accounts_(work, {{}}) {}

    std::optional<Session> session() override {
        auto result = resolve_current_account_session(lookup_, false, *this, *this);
        const auto *session = std::get_if<ResolvedAccountSession>(&result);
        if (!session || !sample_)
            return {};
        snapshot_ = session->snapshot_ref;
        return Session{{session->installation_ref, session->account_ref, session->session_ref},
                       *sample_,
                       session->expires_utc_ms};
    }

    Use use(const Scope &scope, const core::PermissionRequest &request) override {
        if (!graph_) {
            auto loaded = SqlitePermissionGraphUnit(work_, scope.installation, trust_).load();
            auto *graph = std::get_if<StoredPermissionGraph>(&loaded);
            if (!graph || graph->installation != scope.installation)
                return Use::unavailable;
            graph_ = std::move(*graph);
        }
        const auto &graph = *graph_;
        const auto operation = std::ranges::find(graph.catalog.operations, request.operation,
                                                 &core::PermissionOperationDefinition::operation);
        if (operation == graph.catalog.operations.end() ||
            operation->classification != core::PermissionOperationClass::ordinary ||
            operation->resource_slots.size() != 1 ||
            operation->resource_slots[0] != "installation" || !operation->quantity_slots.empty())
            return Use::unavailable;

        const auto account = std::ranges::find_if(graph.use.accounts, [&](const auto &candidate) {
            return candidate.account == scope.actor;
        });
        if (account == graph.use.accounts.end())
            return Use::unavailable;
        core::PermissionPolicyFacts policy{core::PermissionFactsState::complete,
                                           account->assigned_roles, account->individual_entries};
        constexpr std::size_t max_entries = 4096;
        for (const auto &assigned : account->assigned_roles) {
            const auto role = std::ranges::find_if(
                graph.use.roles, [&](const auto &candidate) { return candidate.role == assigned; });
            if (role == graph.use.roles.end() || policy.entries.size() > max_entries ||
                role->entries.size() > max_entries - policy.entries.size())
                return Use::unavailable;
            policy.entries.insert(policy.entries.end(), role->entries.begin(), role->entries.end());
        }
        const auto result = core::evaluate_effective_permission(
            {scope.actor, snapshot_, graph.catalog, graph.owner, std::move(policy), request});
        return result.data().outcome == core::PermissionOutcome::permit ? Use::allowed
                                                                        : Use::denied;
    }
    Attempt attempt(const Scope &, const Selector &selector) override {
        return read_attempt(work_, selector);
    }

  private:
    SessionLookup load_current(const SessionLookupRequest &request,
                               const SessionLimits &limits) override {
        auto loaded = accounts_.read_lifecycle_actor(request, limits);
        if (auto *facts = std::get_if<LifecycleCurrentSession>(&loaded))
            return std::move(facts->current);
        return SessionSourceUnavailable{};
    }
    SessionClockResolution sample_current() override {
        const auto sampled = clock_.sample_current();
        if (const auto *sample = std::get_if<SessionClockSample>(&sampled))
            sample_ = sample->now_utc_ms;
        return sampled;
    }

    const SqliteWorkSession &work_;
    const SessionLookupRequest &lookup_;
    CurrentSessionClock &clock_;
    const PermissionGraphTrust &trust_;
    SqliteAccountSessionUnit accounts_;
    std::optional<Utc> sample_;
    std::string snapshot_;
    std::optional<StoredPermissionGraph> graph_;
};
} // namespace

Gate read_gate(Current &current, const std::vector<Selector> &selectors, const Scope *prepared) {
    if (!valid_selectors(selectors))
        return {};
    auto session = current.session();
    if (!session || session->utc >= session->expires || session->scope.installation.empty() ||
        session->scope.actor.empty() || session->scope.session.empty() ||
        (prepared && session->scope != *prepared))
        return {};
    Gate result;
    result.scope = std::move(session->scope);
    result.expires = session->expires;
    const auto check = [&](const char *operation) {
        return current.use(*result.scope,
                           {operation, {{"installation", result.scope->installation}}, {}});
    };
    result.use = check(base_operation);
    if (result.use == Use::allowed && !selectors.empty())
        result.use = check(detail_operation);
    if (result.use == Use::allowed) {
        result.attempts.reserve(selectors.size());
        for (const auto &selector : selectors)
            result.attempts.push_back(current.attempt(*result.scope, selector));
    }
    return result;
}

bool normally_acknowledged(const SqliteWorkResult &completion) {
    return completion.transaction == TransactionOutcome::committed &&
           completion.error == WorkError::none && completion.value && completion.value->isValid();
}

Gate acknowledge_gate(Gate result, const SqliteWorkResult &completion) {
    return normally_acknowledged(completion) ? std::move(result) : Gate{};
}

Gate read_sqlite_gate(const SqliteWorkSession &session, const SessionLookupRequest &lookup,
                      CurrentSessionClock &clock, const PermissionGraphTrust &trust,
                      const std::vector<Selector> &selectors, const Scope *prepared) {
    SqliteCurrent current(session, lookup, clock, trust);
    return read_gate(current, selectors, prepared);
}
} // namespace subathon::server::diagnostics
