// SPDX-License-Identifier: GPL-3.0-or-later
#include "application_reference_read_state.hpp"
#include "application_service_command_internal.hpp"
#include "reference_authority_values.hpp"
#include "reference_capture_codec_p.hpp"
#include "sqlite_application_hold_values.hpp"
#include "sqlite_selected_record_read.hpp"
#include <QCryptographicHash>
#include <array>
#include <charconv>
#include <exception>
#include <tuple>
namespace subathon::server {
MigrationManifest reference_authority_repository_manifest() {
    auto result = permission_graph_repository_manifest();
    MigrationStep s;
    s.version = 10;
    s.id = "reference-authority-v1";
    s.sql = {
        "CREATE TABLE subathon_reference_authority_head(installation_ref BLOB NOT NULL PRIMARY KEY "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), revision BLOB NOT "
        "NULL "
        "CHECK(typeof(revision)='blob' AND length(revision)=8 AND revision<>X'0000000000000000'), "
        "FOREIGN KEY(installation_ref) REFERENCES subathon_account_installation(installation_ref))",
        "CREATE TABLE subathon_reference_authority_bindings(installation_ref BLOB NOT NULL "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), capability_ref "
        "BLOB NOT NULL "
        "CHECK(typeof(capability_ref)='blob' AND length(capability_ref)>0), principal_ref BLOB NOT "
        "NULL "
        "CHECK(typeof(principal_ref)='blob' AND length(principal_ref)>0), rule_ref BLOB NOT NULL "
        "CHECK(typeof(rule_ref)='blob' AND length(rule_ref)>0), rule_version_ref BLOB NOT NULL "
        "CHECK(typeof(rule_version_ref)='blob' AND length(rule_version_ref)>0), action_ref BLOB "
        "NOT NULL "
        "CHECK(typeof(action_ref)='blob' AND length(action_ref)>0), action_version_ref BLOB NOT "
        "NULL "
        "CHECK(typeof(action_version_ref)='blob' AND length(action_version_ref)>0), timer_ref BLOB "
        "NOT NULL "
        "CHECK(typeof(timer_ref)='blob' AND length(timer_ref)>0), provider BLOB NOT NULL "
        "CHECK(typeof(provider)='blob' AND length(provider)>0), source_account BLOB NOT NULL "
        "CHECK(typeof(source_account)='blob' AND length(source_account)>0), identifier_namespace "
        "BLOB NOT NULL "
        "CHECK(typeof(identifier_namespace)='blob' AND length(identifier_namespace)>0), "
        "requested_ms INTEGER NOT NULL "
        "CHECK(typeof(requested_ms)='integer' AND requested_ms>=0), original_author BLOB NOT NULL "
        "CHECK(typeof(original_author)='blob' AND length(original_author)>0), execution_account "
        "BLOB NOT NULL "
        "CHECK(typeof(execution_account)='blob' AND length(execution_account)>0), created_revision "
        "BLOB NOT NULL "
        "CHECK(typeof(created_revision)='blob' AND length(created_revision)=8 AND "
        "created_revision<>X'0000000000000000'), "
        "PRIMARY KEY(installation_ref,capability_ref), FOREIGN KEY(installation_ref) REFERENCES "
        "subathon_reference_authority_head(installation_ref), FOREIGN "
        "KEY(installation_ref,original_author) "
        "REFERENCES subathon_accounts(installation_ref,account_ref), FOREIGN "
        "KEY(installation_ref,execution_account) "
        "REFERENCES subathon_accounts(installation_ref,account_ref))",
        "CREATE TABLE subathon_reference_authority_events(installation_ref BLOB NOT NULL "
        "CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), capability_ref "
        "BLOB NOT NULL "
        "CHECK(typeof(capability_ref)='blob' AND length(capability_ref)>0), revision BLOB NOT NULL "
        "CHECK(typeof(revision)='blob' AND length(revision)=8 AND revision<>X'0000000000000000'), "
        "state INTEGER NOT NULL "
        "CHECK(typeof(state)='integer' AND state IN (0,1,2)), actor_ref BLOB NOT NULL "
        "CHECK(typeof(actor_ref)='blob' AND length(actor_ref)>0), session_ref BLOB NOT NULL "
        "CHECK(typeof(session_ref)='blob' AND length(session_ref)>0), session_revision_ref BLOB "
        "NOT NULL "
        "CHECK(typeof(session_revision_ref)='blob' AND length(session_revision_ref)>0), "
        "authenticated_utc_ms INTEGER NOT NULL "
        "CHECK(typeof(authenticated_utc_ms)='integer'), PRIMARY KEY(installation_ref,revision), "
        "FOREIGN KEY(installation_ref,capability_ref) REFERENCES "
        "subathon_reference_authority_bindings(installation_ref,capability_ref), FOREIGN "
        "KEY(installation_ref,actor_ref) "
        "REFERENCES subathon_accounts(installation_ref,account_ref))",
        "CREATE INDEX subathon_reference_authority_events_capability_idx ON "
        "subathon_reference_authority_events(installation_ref,capability_ref,revision)"};
    s.expected_schema = result.steps.back().expected_schema;
    const QStringList names{"subathon_reference_authority_head",
                            "subathon_reference_authority_bindings",
                            "subathon_reference_authority_events"};
    for (qsizetype i = 0; i < names.size(); ++i) {
        s.expected_schema.append({"table", names[i], names[i], s.sql[i]});
        s.expected_schema.append(
            {"index", "sqlite_autoindex_" + names[i] + "_1", names[i], std::nullopt});
    }
    s.expected_schema.append({"index", "subathon_reference_authority_events_capability_idx",
                              "subathon_reference_authority_events", s.sql[3]});
    std::sort(s.expected_schema.begin(), s.expected_schema.end(), [](const auto &a, const auto &b) {
        return std::tie(a.type, a.name) < std::tie(b.type, b.name);
    });
    result.steps.append(std::move(s));
    result.supported_starting_versions.append(10);
    return result;
}
} // namespace subathon::server
#include "reference_authority_sql.hpp"
namespace subathon::server {
namespace {
using namespace reference_authority;
using namespace core;
const char *head_table = "subathon_reference_authority_head";
const char *binding_table = "subathon_reference_authority_bindings";
const char *event_table = "subathon_reference_authority_events";
const std::array<const char *, 3> operations{
    "reference.time-add.v1", "reference.application-read.v1", "reference.hold-read.v1"};
// One profile for admission and the later selected read, after callback minima.
SqliteHoldLimits historical_limits(const ReferenceAuthorityLimits &l) {
    SqliteHoldLimits hl;
    auto &bl = hl.references;
    bl.reference_bytes =
        qsizetype(std::min(l.reference_bytes, std::uint64_t(l.captures.reference_bytes)));
    bl.value_bytes = l.captures.document_bytes;
    bl.owned_bytes = l.captures.owned_bytes;
    bl.component_bytes =
        qsizetype(std::min(std::uint64_t(bl.component_bytes), l.permissions.component_bytes));
    bl.result_items = qsizetype(std::min(std::uint64_t(bl.result_items), l.result_items));
    hl.owned_bytes = l.captures.owned_bytes;
    hl.result_items = qsizetype(std::min(std::uint64_t(hl.result_items), l.result_items));
    return hl;
}
void preflight_request(const ReferenceAuthorityRequest &r, const ReferenceAuthorityLimits &l) {
    preflight(l, r);
    request_shape(r);
    if (r.use == ReferenceAuthorityUse::execute_time_add)
        return;
    const auto hl = historical_limits(l);
    try {
        hold_detail::validate_options(hl); // Also validates the nested bundle profile.
    } catch (const SqliteHoldInputFailure &) {
        throw ReferenceAuthorityInputFailure(F::invalid_data);
    }
    const auto ceiling = std::uint64_t(hl.references.reference_bytes);
    if (r.historical_operation->operation_scope.size() > ceiling ||
        r.historical_operation->operation_ref.size() > ceiling ||
        (r.historical_hold && r.historical_hold->size() > ceiling))
        throw ReferenceAuthorityInputFailure(F::resource_limit);
}
// The generic resolver contains producer exceptions. Preserve the original work
// failure across that boundary, then rethrow it before publishing any refusal.
// This adapter lives for one authentication call and owns no session or cache.
class MutationSessionAuthentication final : public CurrentSessionSource,
                                            public CurrentSessionClock {
  public:
    MutationSessionAuthentication(SqliteAccountSessionUnit &accounts, CurrentSessionClock &clock)
        : accounts_(accounts), clock_(clock) {}
    SessionLookup load_current(const SessionLookupRequest &request,
                               const SessionLimits &limits) override {
        try {
            auto loaded = accounts_.read_lifecycle_actor(request, limits);
            if (const auto *failure = std::get_if<AccountSessionReadFailure>(&loaded)) {
                switch (*failure) {
                case AccountSessionReadFailure::missing:
                    return SessionMissing{};
                case AccountSessionReadFailure::invalid_data:
                    return SessionSourceInvalidData{};
                case AccountSessionReadFailure::resource_limit:
                    return SessionSourceResourceLimit{};
                case AccountSessionReadFailure::unavailable:
                    return SessionSourceUnavailable{};
                }
                return SessionSourceUnavailable{};
            }
            return std::move(std::get<LifecycleCurrentSession>(loaded).current);
        } catch (const SqliteWorkFailure &) {
            writer_failure_ = std::current_exception();
            return SessionSourceUnavailable{};
        } catch (const AccountSessionInputFailure &failure) {
            return failure.code() == AccountSessionInputError::resource_limit
                       ? SessionLookup{SessionSourceResourceLimit{}}
                       : SessionLookup{SessionSourceUnavailable{}};
        }
    }
    SessionClockResolution sample_current() override {
        try {
            return clock_.sample_current();
        } catch (const SqliteWorkFailure &) {
            writer_failure_ = std::current_exception();
            return SessionSourceUnavailable{};
        }
    }
    void rethrow_writer_failure() const {
        if (writer_failure_)
            std::rethrow_exception(writer_failure_);
    }

  private:
    SqliteAccountSessionUnit &accounts_;
    CurrentSessionClock &clock_;
    std::exception_ptr writer_failure_;
};
// Policy expansion is traversed before ownership. The authoritative graph already
// validated every membership and every current account, including sparse rows.
template <class Emit>
void policy_entries(const PermissionEditableState &g, std::string_view account, Emit visit) {
    auto a = std::find_if(g.accounts.begin(), g.accounts.end(),
                          [&](const auto &v) { return v.account == account; });
    data(a != g.accounts.end());
    for (const auto &e : a->individual_entries)
        visit(e);
    for (const auto &role : a->assigned_roles) {
        auto r = std::find_if(g.roles.begin(), g.roles.end(),
                              [&](const auto &v) { return v.role == role; });
        data(r != g.roles.end());
        for (const auto &e : r->entries)
            visit(e);
    }
}
void project_policy(Projection &p, const PermissionEditableState &g, std::string_view account) {
    auto a = std::find_if(g.accounts.begin(), g.accounts.end(),
                          [&](const auto &v) { return v.account == account; });
    data(a != g.accounts.end());
    p(PermissionFactsState::complete, a->assigned_roles);
    policy_entries(g, account, [&](const auto &e) {
        p.elements(1);
        p(e);
    });
}
PermissionPolicyFacts policy(const PermissionEditableState &g, std::string_view account,
                             const ReferenceAuthorityLimits &l) {
    Projection p{l.permissions.input_owned_bytes, l.permissions.input_items,
                 l.permissions.reference_bytes};
    p.component_bytes = l.permissions.component_bytes;
    project_policy(p, g, account);
    PermissionPolicyFacts out{PermissionFactsState::complete, {}, {}};
    const auto a = std::find_if(g.accounts.begin(), g.accounts.end(),
                                [&](const auto &v) { return v.account == account; });
    space(a->assigned_roles.size() <= out.assigned_roles.max_size());
    out.assigned_roles = a->assigned_roles;
    policy_entries(g, account, [&](const auto &e) {
        space(out.entries.size() < out.entries.max_size());
        out.entries.push_back(e);
    });
    return out;
}
struct Hash {
    QCryptographicHash hash{QCryptographicHash::Sha256};
    explicit Hash(std::string_view tag) { raw(tag); }
    void raw(std::string_view s) { hash.addData(QByteArrayView(s.data(), qsizetype(s.size()))); }
    void integer(std::uint64_t n, unsigned width) {
        std::array<char, 8> b{};
        for (unsigned i = 0; i < width; ++i)
            b[width - i - 1] = char((n >> (i * 8)) & 255);
        raw({b.data(), width});
    }
    void field(std::string_view s) {
        space(s.size() <= UINT32_MAX);
        integer(s.size(), 4);
        raw(s);
    }
    std::string finish(std::string_view tag) {
        auto b = hash.result();
        return std::string(tag) + std::string(b.constData(), std::size_t(b.size()));
    }
};
std::uint64_t source_size(const ReferenceAuthorityBinding &b, const ReferenceAuthorityLimits &l) {
    std::uint64_t n = 16;
    for (const auto &s :
         {std::string_view(b.origin.provider), std::string_view(b.origin.source_account),
          std::string_view(b.identifier_namespace)}) {
        space(s.size() <= UINT32_MAX);
        add(n, s.size(), std::min(l.reference_bytes, l.permissions.reference_bytes));
    }
    space(n <= std::string{}.max_size());
    return n;
}
std::string source(const ReferenceAuthorityBinding &b, const ReferenceAuthorityLimits &l) {
    const auto n = source_size(b, l);
    std::string out = "RSS1";
    out.reserve(std::size_t(n));
    for (const auto &s :
         {std::string_view(b.origin.provider), std::string_view(b.origin.source_account),
          std::string_view(b.identifier_namespace)}) {
        for (int shift = 24; shift >= 0; shift -= 8)
            out += char((s.size() >> shift) & 255);
        out.append(s);
    }
    return out;
}
PermissionRequest concrete(const ReferenceAuthorityBinding &b, unsigned use,
                           const ReferenceAuthorityLimits &l) {
    Projection projected{l.permissions.input_owned_bytes, l.permissions.input_items,
                         l.permissions.reference_bytes};
    projected(std::string_view(operations[use]), std::string_view("installation"), b.installation,
              std::string_view("timer"), b.timer, std::string_view("source"));
    projected.elements(3);
    projected.fixed(source_size(b, l));
    if (!use) {
        std::array<char, 20> spelling{};
        const auto end =
            std::to_chars(spelling.data(), spelling.data() + spelling.size(), b.requested_ms);
        space(end.ec == std::errc{} &&
              std::uint64_t(end.ptr - spelling.data()) <= l.permissions.component_bytes);
        projected.elements(1);
        projected(std::string_view("adjustment"), std::string_view("ms"));
        projected.fixed(std::uint64_t(end.ptr - spelling.data()) + 1);
    }
    PermissionRequest r{
        operations[use],
        {{"installation", b.installation}, {"timer", b.timer}, {"source", source(b, l)}},
        {}};
    if (!use)
        r.quantities.push_back({"adjustment", "ms", {std::to_string(b.requested_ms), "1"}});
    return r;
}
DelegatedPermissionGrant singleton(const PermissionRequest &r, const ReferenceAuthorityLimits &l) {
    Projection projected{l.permissions.input_owned_bytes, l.permissions.input_items,
                         l.permissions.reference_bytes};
    projected.component_bytes = l.permissions.component_bytes;
    projected(r.operation);
    projected.elements(r.resources.size());
    for (const auto &x : r.resources) {
        projected(x.slot, PermissionResourceMode::allowed_set, x.resource);
        projected.elements(1);
    }
    projected.elements(r.quantities.size());
    for (const auto &x : r.quantities)
        projected(x.slot, x.unit, PermissionQuantityMode::closed_interval, x.value, x.value);
    DelegatedPermissionGrant a;
    a.operation = r.operation;
    for (const auto &x : r.resources)
        a.resources.push_back({x.slot, PermissionResourceMode::allowed_set, {x.resource}});
    for (const auto &x : r.quantities)
        a.quantities.push_back(
            {x.slot, x.unit, PermissionQuantityMode::closed_interval, x.value, x.value});
    return a;
}
} // namespace
class SqliteReferenceAuthorityUnit::Impl {
  public:
    SqliteWorkSession session;
    ReferenceAuthorityLimits limits;
    PermissionGraphTrust trust = reference_permission_graph_trust();
    Impl(const SqliteWorkSession &s, ReferenceAuthorityLimits l) : session(s), limits(l) {}
    ResolvedAccountSession authenticate(const SessionLookupRequest &r, CurrentSessionClock &clock) {
        SqliteAccountSessionUnit accounts(session, {{}}, limits.accounts);
        MutationSessionAuthentication authentication(accounts, clock);
        auto result = resolve_current_account_session(r, true, authentication, authentication,
                                                      limits.sessions);
        authentication.rethrow_writer_failure();
        if (auto f = std::get_if<SessionFailure>(&result)) {
            switch (*f) {
            case SessionFailure::unavailable:
                fail(F::unavailable);
            case SessionFailure::invalid_stored_data:
                fail(F::invalid_data);
            case SessionFailure::resource_limit:
                fail(F::resource_limit);
            default:
                fail(F::denied);
            }
        }
        return std::get<ResolvedAccountSession>(std::move(result));
    }
    StoredPermissionGraph graph(std::string_view installation) {
        SqlitePermissionGraphUnit u(session, installation, trust, limits.graph);
        auto r = u.load();
        if (auto f = std::get_if<PermissionGraphFailure>(&r))
            fail(graph_failure(*f));
        return std::get<StoredPermissionGraph>(std::move(r));
    }
    std::uint64_t head(std::string_view i, std::uint64_t &bytes) {
        auto r = select(session, head_table, head_columns, "installation_ref=?", {blob(i)}, limits,
                        bytes);
        if (!r)
            return 0;
        data(view(*r, 0) == i);
        auto n = unsigned_value(*r, 1);
        data(n != 0);
        return n;
    }
    ReferenceAuthorityRecord record(std::string_view i, std::string_view k, bool output) {
        std::uint64_t selected = 0;
        auto h = head(i, selected);
        if (!h)
            fail(F::missing);
        auto b =
            select(session, binding_table, binding_columns,
                   "installation_ref=? AND capability_ref=?", {blob(i), blob(k)}, limits, selected);
        if (!b)
            fail(F::missing);
        data(view(*b, 0) == i && view(*b, 1) == k && (*b)[11].toLongLong() >= 0);
        auto created = unsigned_value(*b, 14);
        data(created && created <= h);
        auto initial =
            select(session, event_table, event_columns, "installation_ref=? AND revision=?",
                   {blob(i), u64(created)}, limits, selected);
        data(initial.has_value());
        // Seek latest revision using the compiled named index. Do not load/audit history.
        auto latest_meta = session.execute(
            {"SELECT CASE WHEN typeof(revision)='blob' AND length(revision)=8 THEN revision ELSE "
             "NULL END FROM main.subathon_reference_authority_events WHERE installation_ref=? AND "
             "capability_ref=? ORDER BY revision DESC LIMIT 1",
             {blob(i), blob(k)}});
        data(latest_meta.size() == 1 && !latest_meta[0][0].isNull());
        auto revbytes = latest_meta[0][0].toByteArray();
        std::optional<SqlRow> latest;
        if (revbytes == u64(created))
            latest = *initial;
        else
            latest =
                select(session, event_table, event_columns, "installation_ref=? AND revision=?",
                       {blob(i), revbytes}, limits, selected);
        data(latest.has_value());
        auto revision = unsigned_value(*latest, 2);
        data(revision >= created && revision <= h);
        for (const auto *r : {&*initial, &*latest})
            data(view(*r, 0) == i && view(*r, 1) == k);
        data((*initial)[3].toLongLong() == 0 && view(*initial, 4) == view(*b, 12));
        data(revision == created || (*latest)[3].toLongLong() != 0);
        if (output) {
            Projection p{limits.result_bytes, limits.result_items, limits.reference_bytes};
            for (int j = 0; j < 14; ++j) {
                if (j == 11)
                    p.fixed(8);
                else
                    p(view(*b, j));
            }
            p.fixed(24 + 1 + 8);
            p(view(*latest, 4), view(*latest, 5), view(*latest, 6));
        }
        ReferenceAuthorityRecord r;
        r.binding = {text(*b, 0),  text(*b, 1),          text(*b, 2),
                     text(*b, 3),  text(*b, 4),          text(*b, 5),
                     text(*b, 6),  text(*b, 7),          {text(*b, 8), text(*b, 9)},
                     text(*b, 10), (*b)[11].toLongLong()};
        r.original_author = text(*b, 12);
        r.execution_account = text(*b, 13);
        r.created_revision = created;
        r.state_revision = revision;
        r.head_revision = h;
        r.state = ReferenceAuthorityState((*latest)[3].toLongLong());
        r.actor = text(*latest, 4);
        r.session = text(*latest, 5);
        r.session_revision = text(*latest, 6);
        r.authenticated_utc_ms = (*latest)[7].toLongLong();
        return r;
    }
    bool enabled(const StoredPermissionGraph &g, std::string_view a, F absent = F::invalid_data) {
        auto it = std::find_if(g.current_accounts.begin(), g.current_accounts.end(),
                               [&](const auto &x) { return x.current.account_ref == a; });
        if (it == g.current_accounts.end())
            fail(absent);
        return it->current.application_access_enabled;
    }
    void management(const StoredPermissionGraph &g, std::string_view actor,
                    const PermissionRequest &r) {
        Projection projection{limits.permissions.input_owned_bytes, limits.permissions.input_items,
                              limits.permissions.reference_bytes};
        projection.component_bytes = limits.permissions.component_bytes;
        projection(actor, std::string_view("current"), g.catalog, g.owner);
        project_policy(projection, g.use, actor);
        projection(r);
        EffectivePermissionInput in{std::string(actor),           "current", g.catalog, g.owner,
                                    policy(g.use, actor, limits), r};
        if (evaluate_effective_permission(in, limits.permissions).data().outcome !=
            PermissionOutcome::permit)
            fail(F::denied);
    }
    void authority(const StoredPermissionGraph &g, const ReferenceAuthorityBinding &b,
                   std::string_view a, std::string_view e, unsigned use, bool author = true) {
        auto r = concrete(b, use, limits);
        auto action = singleton(r, limits);
        Projection projection{limits.permissions.input_owned_bytes, limits.permissions.input_items,
                              limits.permissions.reference_bytes};
        projection.component_bytes = limits.permissions.component_bytes;
        projection(a, std::string_view("current"), g.catalog, g.owner);
        if (author) {
            project_policy(projection, g.use, a);
            project_policy(projection, g.grant, a);
        } else {
            project_policy(projection, g.grant, a);
            projection(PermissionDelegationContext::automation_authorization, e);
        }
        projection.elements(1);
        projection(action);
        if (author) {
            if (!enabled(g, a) || !enabled(g, e))
                fail(F::denied);
            PermissionAutomationInput in{std::string(a),
                                         "current",
                                         g.catalog,
                                         g.owner,
                                         policy(g.use, a, limits),
                                         policy(g.grant, a, limits),
                                         {action}};
            auto result = revalidate_permission_automation(in, limits.permissions);
            if (result.actions[0].disposition != PermissionAutomationDisposition::authorized)
                fail(F::denied);
            management(g, e, r);
        } else {
            PermissionDelegationInput in{std::string(a),
                                         "current",
                                         g.catalog,
                                         g.owner,
                                         policy(g.grant, a, limits),
                                         PermissionDelegationContext::automation_authorization,
                                         std::string(e),
                                         {action}};
            if (evaluate_permission_delegation(in, limits.permissions).outcome !=
                PermissionOutcome::permit)
                fail(F::denied);
        }
    }
    void capacity(std::uint64_t head, std::uint64_t bytes) {
        space(head != UINT64_MAX && bytes <= limits.mutation_bytes && limits.result_bytes >= 8);
    }
    void advance(std::string_view i, std::uint64_t expected) {
        (void)session.execute({"UPDATE main.subathon_reference_authority_head SET revision=? WHERE "
                               "installation_ref=? AND revision=?",
                               {u64(expected + 1), blob(i), u64(expected)}});
        auto changed = session.execute({"SELECT changes()", {}});
        data(changed.size() == 1 && changed[0][0].toLongLong() == 1);
    }
    void event(std::string_view i, std::string_view k, std::uint64_t revision,
               ReferenceAuthorityState state, const ResolvedAccountSession &a) {
        (void)session.execute(
            {"INSERT INTO main.subathon_reference_authority_events VALUES(?,?,?,?,?,?,?,?)",
             {blob(i), blob(k), u64(revision), int(state), blob(a.account_ref), blob(a.session_ref),
              blob(a.record_revision_ref), qlonglong(a.authenticated_utc_ms)}});
    }
    std::uint64_t event_bytes(std::string_view i, std::string_view k,
                              const ResolvedAccountSession &a) {
        Projection p{limits.mutation_bytes, limits.result_items, limits.reference_bytes};
        p(i, k, a.account_ref, a.session_ref, a.record_revision_ref);
        p.fixed(17);
        return p.bytes;
    }
    ReferenceAuthorityFacts resolve(const ReferenceAuthorityRequest &);
};
SqliteReferenceAuthorityUnit::SqliteReferenceAuthorityUnit(const SqliteWorkSession &s,
                                                           ReferenceAuthorityLimits l) {
    valid_limits(l);
    impl_ = std::make_unique<Impl>(s, l);
}
SqliteReferenceAuthorityUnit::~SqliteReferenceAuthorityUnit() = default;
ReferenceAuthorityWrite SqliteReferenceAuthorityUnit::bootstrap(const SessionLookupRequest &r,
                                                                CurrentSessionClock &clock) {
    preflight(impl_->limits, r.installation_ref, std::uint64_t(0), std::uint64_t(0),
              std::uint64_t(0), std::uint64_t(0));
    return guarded<ReferenceAuthorityWrite>([&]() -> ReferenceAuthorityWrite {
        auto a = impl_->authenticate(r, clock);
        auto g = impl_->graph(r.installation_ref);
        if (g.owner.account != a.account_ref)
            fail(F::denied);
        impl_->management(g, a.account_ref, {"access.manage", {}, {}});
        std::uint64_t bytes = 0;
        if (impl_->head(r.installation_ref, bytes))
            fail(F::collision);
        impl_->capacity(0, r.installation_ref.size() + 8);
        (void)impl_->session.execute(
            {"INSERT INTO main.subathon_reference_authority_head VALUES(?,?)",
             {blob(r.installation_ref), u64(1)}});
        return ReferenceAuthorityStaged{1};
    });
}
ReferenceAuthorityWrite SqliteReferenceAuthorityUnit::enroll(const SessionLookupRequest &r,
                                                             CurrentSessionClock &clock,
                                                             std::uint64_t expected,
                                                             const ReferenceAuthorityBinding &b,
                                                             std::string_view e) {
    preflight(impl_->limits, r.installation_ref, std::uint64_t(0), std::uint64_t(0),
              std::uint64_t(0), std::uint64_t(0), expected, b, e);
    if (b.requested_ms < 0)
        throw ReferenceAuthorityInputFailure(F::invalid_data);
    return guarded<ReferenceAuthorityWrite>([&]() -> ReferenceAuthorityWrite {
        auto a = impl_->authenticate(r, clock);
        if (b.installation != a.installation_ref)
            fail(F::denied);
        auto g = impl_->graph(b.installation);
        std::uint64_t selected = 0;
        auto h = impl_->head(b.installation, selected);
        if (!h)
            fail(F::missing);
        if (h != expected)
            fail(F::stale);
        auto old = select(impl_->session, binding_table, binding_columns,
                          "installation_ref=? AND capability_ref=?",
                          {blob(b.installation), blob(b.capability)}, impl_->limits, selected);
        if (old)
            fail(F::collision);
        if (b.origin.provider != "synthetic.reference.v1")
            fail(F::unavailable);
        if (!impl_->enabled(g, e, F::missing))
            fail(F::denied);
        impl_->management(g, a.account_ref, {"overrides.edit", {{"account", std::string(e)}}, {}});
        for (unsigned n = 0; n < 3; ++n)
            impl_->authority(g, b, a.account_ref, e, n);
        Projection p{impl_->limits.mutation_bytes, impl_->limits.result_items,
                     impl_->limits.reference_bytes};
        p(b, a.account_ref, e);
        p.fixed(8);
        p(b.installation);
        p.fixed(8);
        p.fixed(impl_->event_bytes(b.installation, b.capability, a));
        impl_->capacity(h, p.bytes);
        impl_->advance(b.installation, h);
        (void)impl_->session.execute(
            {"INSERT INTO main.subathon_reference_authority_bindings "
             "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
             {blob(b.installation), blob(b.capability), blob(b.principal), blob(b.rule),
              blob(b.rule_version), blob(b.action), blob(b.action_version), blob(b.timer),
              blob(b.origin.provider), blob(b.origin.source_account), blob(b.identifier_namespace),
              qlonglong(b.requested_ms), blob(a.account_ref), blob(e), u64(h + 1)}});
        impl_->event(b.installation, b.capability, h + 1, ReferenceAuthorityState::pending, a);
        return ReferenceAuthorityStaged{h + 1};
    });
}
ReferenceAuthorityWrite SqliteReferenceAuthorityUnit::set_state(const SessionLookupRequest &r,
                                                                CurrentSessionClock &clock,
                                                                std::uint64_t expected,
                                                                std::string_view k,
                                                                ReferenceAuthorityState target) {
    preflight(impl_->limits, r.installation_ref, std::uint64_t(0), std::uint64_t(0),
              std::uint64_t(0), std::uint64_t(0), expected, k, target);
    if (target != ReferenceAuthorityState::authorized && target != ReferenceAuthorityState::revoked)
        throw ReferenceAuthorityInputFailure(F::invalid_data);
    return guarded<ReferenceAuthorityWrite>([&]() -> ReferenceAuthorityWrite {
        auto a = impl_->authenticate(r, clock);
        auto g = impl_->graph(r.installation_ref);
        std::uint64_t bytes = 0;
        auto h = impl_->head(r.installation_ref, bytes);
        if (!h)
            fail(F::missing);
        if (h != expected)
            fail(F::stale);
        auto rec = impl_->record(r.installation_ref, k, false);
        if (g.owner.account != a.account_ref)
            fail(F::denied);
        impl_->management(g, a.account_ref, {"access.manage", {}, {}});
        for (unsigned n = 0; n < 3; ++n) {
            impl_->authority(g, rec.binding, a.account_ref, rec.execution_account, n, false);
            if (target == ReferenceAuthorityState::authorized)
                impl_->authority(g, rec.binding, rec.original_author, rec.execution_account, n);
        }
        auto count = impl_->event_bytes(r.installation_ref, k, a);
        add(count, r.installation_ref.size() + 8, impl_->limits.mutation_bytes);
        impl_->capacity(h, count);
        impl_->advance(r.installation_ref, h);
        impl_->event(r.installation_ref, k, h + 1, target, a);
        return ReferenceAuthorityStaged{h + 1};
    });
}
ReferenceAuthorityRead SqliteReferenceAuthorityUnit::read(std::string_view i, std::string_view k) {
    preflight(impl_->limits, i, k);
    return guarded<ReferenceAuthorityRead>(
        [&]() -> ReferenceAuthorityRead { return impl_->record(i, k, true); });
}
} // namespace subathon::server

// Read-only selected capture closure uses the owning schema/codecs and matching
// semantics. Unlike the legacy retaining facade, it never controls a transaction
// or translates SqliteWorkFailure. Only this internal unit can reach its SQL.
#include <charconv>
#include <span>
#include <subathon/server/sqlite_application_plans.hpp>
namespace subathon::server::reference_authority_capture {
using Reason = ReferenceCaptureStorageReason;
using Disposition = ReferenceCaptureRetainDisposition;
using Input = ReferenceInputError;
struct Count {
    std::size_t limit, total{0};
    bool add(std::size_t n) {
        if (n > limit - total)
            return false;
        total += n;
        return true;
    }
};
auto key_views(const ReferenceCaptureKey &k) {
    return std::array<std::string_view, 5>{k.installation_ref, k.source.origin.provider,
                                           k.source.origin.source_account,
                                           k.source.identifier_namespace, k.source.opaque_id};
}
QByteArray blob(std::string_view v) {
    return v.empty() ? QByteArray("", 0) : QByteArray(v.data(), qsizetype(v.size()));
}
bool same(const ReferenceRule &a, const ReferenceRule &b) {
    return std::tie(a.rule_ref, a.rule_version_ref, a.action_ref, a.action_version_ref, a.timer_ref,
                    a.add_ms, a.automation.kind, a.automation.installation_ref,
                    a.automation.principal_ref, a.automation_capability_ref) ==
           std::tie(b.rule_ref, b.rule_version_ref, b.action_ref, b.action_version_ref, b.timer_ref,
                    b.add_ms, b.automation.kind, b.automation.installation_ref,
                    b.automation.principal_ref, b.automation_capability_ref);
}
// Descriptors refer only to fixed adapter-owned SQL identifiers. Scalars carry
// their semantic width; payloads are NEVER returned by the metadata query.
enum class Kind { reference, value, document, id, integer };
struct Column {
    const char *name;
    Kind kind;
    bool nullable{false};
    std::size_t scalar_bytes{8};
    bool required{false};
};
using Columns = std::span<const Column>;
constexpr Column ref(const char *n, bool optional = false) {
    return {n, Kind::reference, optional};
}
constexpr Column required_ref(const char *n, bool optional = false) {
    return {n, Kind::reference, optional, 8, true};
}
constexpr Column value(const char *n, bool optional = false) { return {n, Kind::value, optional}; }
constexpr Column id(const char *n, bool optional = false) { return {n, Kind::id, optional}; }
constexpr Column number(const char *n, std::size_t width = 8, bool optional = false) {
    return {n, Kind::integer, optional, width};
}
constexpr std::array index_columns{required_ref("installation_ref"),
                                   required_ref("provider"),
                                   required_ref("source_account"),
                                   required_ref("identifier_namespace"),
                                   required_ref("opaque_id"),
                                   id("receipt_id"),
                                   id("contribution_id"),
                                   required_ref("configuration_ref"),
                                   required_ref("arrival_context_ref"),
                                   required_ref("event_ref"),
                                   required_ref("plan_ref"),
                                   id("plan_revision"),
                                   required_ref("execution_context_ref")};
constexpr std::array receipt_columns{id("receipt_id"),
                                     required_ref("provider"),
                                     required_ref("source_account"),
                                     required_ref("identifier_namespace"),
                                     required_ref("opaque_id"),
                                     value("event_type"),
                                     number("schema_version", 4),
                                     number("source_timestamp_utc_ms", 8, true),
                                     value("actor_id", true),
                                     value("actor_display_name", true),
                                     value("money_original_decimal", true),
                                     value("money_currency", true),
                                     ref("payment_namespace", true),
                                     ref("payment_opaque_id", true),
                                     ref("activity_namespace", true),
                                     ref("activity_opaque_id", true),
                                     number("server_timestamp_utc_ms"),
                                     ref("connection_ref"),
                                     required_ref("configuration_ref"),
                                     required_ref("modifier_context_ref")};
constexpr std::array contribution_columns{id("contribution_id"),
                                          id("original_receipt_id"),
                                          required_ref("activity_provider", true),
                                          required_ref("activity_source_account", true),
                                          required_ref("activity_namespace", true),
                                          required_ref("activity_id", true)};
constexpr std::array observation_columns{id("receipt_id"),
                                         id("contribution_id", true),
                                         number("kind", 1),
                                         number("evidence", 1),
                                         required_ref("activity_provider", true),
                                         required_ref("activity_source_account", true),
                                         required_ref("activity_namespace", true),
                                         required_ref("activity_id", true),
                                         required_ref("verification_ref", true),
                                         number("support_monetary", 1, true),
                                         value("support_original_decimal", true),
                                         value("support_currency", true),
                                         ref("support_resolution_ref", true),
                                         number("support_missing_rate", 1, true),
                                         number("support_review", 1, true),
                                         number("support_evidence_policy", 1, true),
                                         ref("support_policy_ref", true),
                                         number("support_decision_choice", 1, true),
                                         ref("support_decision_ref", true),
                                         ref("support_decision_actor_ref", true),
                                         number("support_application", 1, true),
                                         number("support_reversal", 1, true),
                                         number("support_settlement", 1, true)};
constexpr std::array configuration_columns{required_ref("reference"),
                                           Column{"document", Kind::document}};
constexpr std::array context_columns{required_ref("reference"), Column{"document", Kind::document},
                                     required_ref("configuration_ref", true)};
constexpr std::array header_columns{
    required_ref("plan_ref"),       required_ref("event_ref"), ref("parent_event_ref", true),
    required_ref("root_event_ref"), id("receipt_id"),          id("contribution_id", true)};
constexpr std::array revision_columns{required_ref("plan_ref"),
                                      id("revision"),
                                      id("parent_revision", true),
                                      number("payment_evidence", 1),
                                      number("monetary", 1),
                                      value("original_decimal", true),
                                      value("currency", true),
                                      ref("resolution_ref", true),
                                      number("missing_rate", 1),
                                      number("review", 1),
                                      number("evidence_policy", 1),
                                      ref("policy_ref"),
                                      number("decision_choice", 1, true),
                                      ref("decision_ref", true),
                                      ref("decision_actor_ref", true),
                                      number("application", 1),
                                      number("reversal", 1),
                                      number("settlement", 1),
                                      number("valuation_state", 1),
                                      ref("historical_resolution_ref", true),
                                      value("historical_original_decimal", true),
                                      value("historical_currency", true),
                                      value("comparison_currency", true),
                                      value("effective_rate_decimal", true),
                                      value("fetched_rate_decimal", true),
                                      ref("snapshot_ref", true),
                                      ref("rate_source_ref", true),
                                      number("rate_timestamp_utc_ms", 8, true),
                                      number("fetch_timestamp_utc_ms", 8, true),
                                      ref("manual_decision_ref", true),
                                      number("override_authority", 1, true),
                                      ref("override_actor_ref", true),
                                      value("override_reason", true),
                                      ref("override_decision_ref", true)};
constexpr std::array rule_columns{
    required_ref("plan_ref"), id("revision"),        number("ordinal"),   ref("rule_ref"),
    ref("version_ref"),       number("selected", 1), value("explanation")};
constexpr std::array request_columns{required_ref("plan_ref"),
                                     id("revision"),
                                     number("ordinal"),
                                     ref("action_ref"),
                                     ref("action_version_ref"),
                                     ref("rule_ref"),
                                     ref("target_ref"),
                                     number("unit", 1),
                                     number("requires_historical_valuation", 1),
                                     value("exact_numerator"),
                                     value("exact_denominator"),
                                     number("requested_units"),
                                     value("calculation_explanation"),
                                     ref("reversal_behavior_ref")};
struct Metadata {
    bool null{false};
    qint64 length{0}, integer{0};
    std::uint64_t identifier{0};
};
struct Selection {
    const char *table;
    Columns columns;
    QString where;
    QList<QVariant> bindings;
    std::array<Metadata, 34> fields{};
    std::size_t bytes{0};
};
struct Refused {
    Disposition disposition;
};
class Access final {
  public:
    Access(SqliteWorkSession s, ReferenceCaptureLimits l) : session_(std::move(s)), limits_(l) {}
    [[noreturn]] void fail(Reason reason) {
        reference_authority::fail(reason == Reason::result_limit || reason == Reason::resource_limit
                                      ? ReferenceAuthorityFailure::resource_limit
                                      : ReferenceAuthorityFailure::invalid_data);
    }
    SqlRows execute(const SqlStatement &statement) { return session_.execute(statement); }
    static bool integer_type(const QVariant &v) {
        return !v.isNull() &&
               (v.metaType().id() == QMetaType::Int || v.metaType().id() == QMetaType::LongLong);
    }
    std::optional<Selection> metadata(const char *table, Columns cols, const QString &where,
                                      QList<QVariant> bindings) {
        QStringList projection;
        for (const auto &c : cols) {
            const auto n = QString::fromLatin1(c.name);
            projection.append("typeof(" + n + ")");
            projection.append("CASE WHEN typeof(" + n + ")='null' THEN NULL ELSE length(CAST(" + n +
                              " AS BLOB)) END");
            projection.append(c.kind == Kind::id
                                  ? "CASE WHEN typeof(" + n + ")='blob' AND length(" + n +
                                        ")=8 THEN " + n + " ELSE NULL END"
                                  : "CASE WHEN typeof(" + n + ")='integer' THEN " + n +
                                        " ELSE NULL END");
        }
        const auto rows = execute({"SELECT " + projection.join(',') + " FROM main." + table +
                                       " WHERE " + where + " LIMIT 2",
                                   std::move(bindings)});
        if (rows.empty())
            return {};
        if (rows.size() != 1 || rows.front().size() != qsizetype(cols.size() * 3))
            fail(Reason::invalid_stored_data);
        Selection result{table, cols, where, {}, {}, 0};
        const auto &row = rows.front();
        Count size{std::size_t(limits_.owned_bytes)};
        for (std::size_t i = 0; i < cols.size(); ++i) {
            const auto &c = cols[i];
            const auto &type = row[qsizetype(i * 3)], &length = row[qsizetype(i * 3 + 1)],
                       &scalar = row[qsizetype(i * 3 + 2)];
            auto &m = result.fields[i];
            if (type.metaType().id() != QMetaType::QString)
                fail(Reason::invalid_stored_data);
            m.null = type.toString() == "null";
            if (m.null) {
                if (!c.nullable || !length.isNull() || !scalar.isNull())
                    fail(Reason::invalid_stored_data);
                continue;
            }
            if (!integer_type(length) || length.toLongLong() < 0)
                fail(Reason::invalid_stored_data);
            m.length = length.toLongLong();
            if (c.kind != Kind::integer &&
                ((c.kind == Kind::reference && m.length > limits_.reference_bytes) ||
                 (c.kind == Kind::document && m.length > limits_.document_bytes) ||
                 std::uint64_t(m.length) > std::uint64_t(limits_.owned_bytes)))
                fail(Reason::result_limit);
            if (c.kind == Kind::integer) {
                if (type.toString() != "integer" || !integer_type(scalar))
                    fail(Reason::invalid_stored_data);
                m.integer = scalar.toLongLong();
                if (!size.add(c.scalar_bytes))
                    fail(Reason::result_limit);
            } else {
                if (type.toString() != "blob" || (c.kind != Kind::id && !scalar.isNull()) ||
                    (c.kind == Kind::id &&
                     (m.length != 8 || scalar.metaType().id() != QMetaType::QByteArray ||
                      scalar.toByteArray().size() != 8)) ||
                    (c.required && m.length == 0))
                    fail(Reason::invalid_stored_data);
                if (c.kind == Kind::id) {
                    // This CASE returns at most eight actual bytes, a fixed-width
                    // scalar rather than unchecked user payload.
                    m.identifier = decode_plan_revision_uint64(scalar.toByteArray());
                    if ((m.identifier == 0 && std::string_view(c.name) != "contribution_id") ||
                        (std::string_view(c.name) == "plan_revision" && m.identifier != 1))
                        fail(Reason::invalid_stored_data);
                }
                if ((c.kind == Kind::reference && m.length > limits_.reference_bytes) ||
                    (c.kind == Kind::document && m.length > limits_.document_bytes) ||
                    std::uintmax_t(m.length) >= std::uintmax_t(QByteArray::maxSize()) ||
                    !size.add(std::size_t(m.length)))
                    fail(Reason::result_limit);
            }
        }
        result.bytes = size.total;
        return result;
    }
    std::optional<Selection> select(const char *table, Columns cols, QString where,
                                    QList<QVariant> bindings) {
        auto result = metadata(table, cols, where, bindings);
        if (result)
            result->bindings = std::move(bindings);
        return result;
    }
    SqlRow payload(const Selection &s) {
        QStringList names;
        auto guards = s.where;
        auto bindings = s.bindings;
        for (std::size_t i = 0; i < s.columns.size(); ++i) {
            const auto &c = s.columns[i];
            const auto n = QString::fromLatin1(c.name);
            const auto &m = s.fields[i];
            names.append(n);
            guards += " AND typeof(" + n + ")='" +
                      (m.null                    ? "null"
                       : c.kind == Kind::integer ? "integer"
                                                 : "blob") +
                      "'";
            if (!m.null) {
                guards += " AND length(" + n + ")=?";
                bindings.append(m.length);
                if (c.kind == Kind::integer) {
                    guards += " AND " + n + "=?";
                    bindings.append(m.integer);
                } else if (c.kind == Kind::id) {
                    guards += " AND " + n + "=?";
                    bindings.append(encode_plan_revision_uint64(m.identifier));
                }
            }
        }
        auto rows = execute({"SELECT " + names.join(',') + " FROM main." + s.table + " WHERE " +
                                 guards + " LIMIT 2",
                             std::move(bindings)});
        if (rows.size() != 1 || rows.front().size() != qsizetype(s.columns.size()))
            fail(Reason::invalid_stored_data);
        auto result = std::move(rows.front());
        for (std::size_t i = 0; i < s.columns.size(); ++i) {
            const auto &v = result[qsizetype(i)];
            if (s.fields[i].null) {
                if (!v.isNull())
                    fail(Reason::invalid_stored_data);
            } else if (s.columns[i].kind == Kind::integer) {
                if (!integer_type(v) || v.toLongLong() != s.fields[i].integer)
                    fail(Reason::invalid_stored_data);
            } else if (!(s.fields[i].length == 0 && v.isNull()) &&
                       (v.metaType().id() != QMetaType::QByteArray ||
                        v.toByteArray().size() != s.fields[i].length))
                fail(Reason::invalid_stored_data);
        }
        return result;
    }
    // Semantic ownership is projected from declared DTO fields, not SQL keys,
    // ordinals, framing, QVariant buffers or a lifetime sum of separate reads.
    void add(Count &n, std::size_t bytes) {
        if (!n.add(bytes))
            fail(Reason::result_limit);
    }
    void field(Count &n, const Selection &s, std::size_t i) {
        if (!s.fields[i].null)
            add(n, std::size_t(s.fields[i].length));
    }
    void optional_field(Count &n, const Selection &s, std::size_t i) {
        add(n, 1);
        field(n, s, i);
    }
    void optional_scalar(Count &n, const Selection &s, std::size_t i, std::size_t width) {
        add(n, 1);
        if (!s.fields[i].null)
            add(n, width);
    }
    void identity_bytes(Count &n, const Selection &receipt) {
        for (std::size_t i = 1; i <= 4; ++i)
            field(n, receipt, i);
    }
    void facts_bytes(Count &n, const Selection &receipt) {
        field(n, receipt, 5);                // event type
        add(n, 4);                           // schema_version
        optional_scalar(n, receipt, 7, 8);   // source UTC
        optional_field(n, receipt, 8);       // actor ID
        optional_field(n, receipt, 9);       // display name
        for (std::size_t i : {10, 12, 14}) { // money/payment/activity optional groups
            add(n, 1);
            if (!receipt.fields[i].null) {
                field(n, receipt, i);
                field(n, receipt, i + 1);
            }
        }
    }
    void arrival_bytes(Count &n, const Selection &receipt) {
        add(n, 8);
        for (std::size_t i = 17; i <= 19; ++i)
            field(n, receipt, i);
    }
    void receipt_bytes(Count &n, const Selection &receipt) {
        add(n, 8); // ContributionReceipt.id
        identity_bytes(n, receipt);
        facts_bytes(n, receipt);
        arrival_bytes(n, receipt);
    }
    void conditions_bytes(Count &n, const Selection &s, std::size_t first) {
        // monetary, missing_rate, review, policy, application, reversal, settlement.
        add(n, 7);
        add(n, 1); // optional original money
        if (!s.fields[first + 1].null) {
            field(n, s, first + 1);
            field(n, s, first + 2);
        }
        optional_field(n, s, first + 3); // optional resolution
        field(n, s, first + 7);          // policy_ref
        add(n, 1);                       // optional review decision
        if (!s.fields[first + 8].null) {
            add(n, 1); // choice enum
            field(n, s, first + 9);
            field(n, s, first + 10);
        }
    }
    void valuation_bytes(Count &n, const Selection &revision) {
        add(n, 2); // state + optional HistoricalValuation presence
        if (revision.fields[18].integer != 2)
            return;
        for (std::size_t i : {19, 20, 21, 22, 23, 25})
            field(n, revision, i);
        for (std::size_t i : {24, 26, 29})
            optional_field(n, revision, i);
        optional_scalar(n, revision, 27, 8);
        optional_scalar(n, revision, 28, 8);
    }
    void bound_receipt(const Selection &receipt) {
        Count n{std::size_t(limits_.owned_bytes)};
        receipt_bytes(n, receipt);
    }
    void bound_contribution(const Selection &contribution) {
        Count n{std::size_t(limits_.owned_bytes)};
        add(n, 16); // immutable CID and original ReceiptId
        add(n, 1);  // optional verified activity identity
        if (!contribution.fields[2].null)
            for (std::size_t i = 2; i <= 5; ++i)
                field(n, contribution, i);
    }
    void bound_support(const Selection &support, const Selection &receipt) {
        Count n{std::size_t(limits_.owned_bytes)};
        receipt_bytes(n, receipt); // ContributionEvidenceRecord.observation.receipt
        add(n, 2);                 // observation kind and payment evidence
        add(n, 1);                 // optional verified activity/proof
        if (!support.fields[4].null)
            for (std::size_t i = 4; i <= 8; ++i)
                field(n, support, i);
        add(n, 1); // optional support conditions
        if (!support.fields[9].null)
            conditions_bytes(n, support, 9);
        optional_scalar(n, support, 1, 8); // optional associated CID (including0)
    }
    void bound_retained(const Selection &document, bool configuration) {
        Count n{std::size_t(limits_.owned_bytes)};
        add(n, 1); // RetainedKey.kind
        field(n, document, 0);
        field(n, document, 1);
        add(n, 1); // optional ConfigurationReference
        if (!configuration)
            field(n, document, 2);
    }
    void bound_plan(const Selection &header, const Selection &receipt, const Selection &revision,
                    const Selection &rule, const Selection &request) {
        Count n{std::size_t(limits_.owned_bytes)};
        field(n, header, 0); // Selected snapshot's plan_ref, once
        add(n, 8);           // CapturedPlanRevision.revision
        optional_scalar(n, revision, 2, 8);
        // PlanEvent owns its complete original receipt projection.
        field(n, header, 1);
        optional_field(n, header, 2);
        field(n, header, 3);
        add(n, 8); // event.receipt_id
        identity_bytes(n, receipt);
        facts_bytes(n, receipt);
        arrival_bytes(n, receipt);
        optional_scalar(n, header, 5, 8);
        add(n, 1); // captured ContributionState.payment_evidence
        conditions_bytes(n, revision, 4);
        valuation_bytes(n, revision);
        // The selected vectors are already proven exactly1 rule/0 modifiers/1 request.
        field(n, rule, 3);
        field(n, rule, 4);
        add(n, 1); // RuleSelection.selected
        field(n, rule, 6);
        for (std::size_t i = 3; i <= 6; ++i)
            field(n, request, i);
        add(n, 10);            // RequestedEffect unit + requires-historical + requested_units
        field(n, request, 9);  // canonical exact numerator
        field(n, request, 10); // canonical exact denominator
        field(n, request, 12);
        field(n, request, 13);
        add(n, 1); // optional override decision
        if (!revision.fields[30].null) {
            add(n, 1); // authority
            for (std::size_t i = 31; i <= 33; ++i)
                field(n, revision, i);
        }
    }
    Selection required(std::optional<Selection> s, bool indexed, bool intrinsic = false) {
        if (!s) {
            if (indexed || intrinsic)
                fail(Reason::invalid_stored_data);
            throw Refused{Disposition::missing_reference};
        }
        return std::move(*s);
    }
    void match(bool okay, bool indexed) {
        if (!okay) {
            if (indexed)
                fail(Reason::invalid_stored_data);
            throw Refused{Disposition::source_mismatch};
        }
    }
    void domain(const Selection &s, std::size_t i, qint64 low, qint64 high) {
        if (s.fields[i].null || s.fields[i].integer < low || s.fields[i].integer > high)
            fail(Reason::invalid_stored_data);
    }
    void group(const Selection &s, std::size_t first, std::size_t last) {
        for (auto i = first + 1; i <= last; ++i)
            if (s.fields[first].null != s.fields[i].null)
                fail(Reason::invalid_stored_data);
    }
    static QByteArray data(const SqlRow &r, std::size_t i) {
        const auto b = r[qsizetype(i)].toByteArray();
        return b.isEmpty() ? QByteArray("", 0) : b;
    }
    static std::string text(const SqlRow &r, std::size_t i) {
        const auto b = data(r, i);
        return {b.constData(), std::size_t(b.size())};
    }
    std::uint64_t identifier(const SqlRow &r, std::size_t i, bool positive = false) {
        const auto n = decode_plan_revision_uint64(data(r, i));
        if (positive && n == 0)
            fail(Reason::invalid_stored_data);
        return n;
    }
    static qint64 integer(const SqlRow &r, std::size_t i) { return r[qsizetype(i)].toLongLong(); }
    template <class Decode> auto decode(const QByteArray &document, Decode decoder) {
        ReferenceAdapterLimits l;
        l.reference_bytes = std::uint64_t(limits_.reference_bytes);
        l.payload_bytes = l.document_bytes = std::uint64_t(limits_.document_bytes);
        l.value_owned_bytes = std::uint64_t(limits_.owned_bytes);
        try {
            return decoder(std::string_view(document.constData(), std::size_t(document.size())), l,
                           reference_capture_detail::BodyBounds::retained);
        } catch (const ReferenceInputFailure &e) {
            if (e.code() == Input::allocation_failure || e.code() == Input::invalid_limits) {
                // Allocation failure is not evidence that persisted bytes were corrupt.
                fail(Reason::resource_limit);
            }
            fail(e.code() == Input::result_limit ? Reason::result_limit
                                                 : Reason::invalid_stored_data);
        }
    }
    struct Document {
        QByteArray bytes;
        std::optional<std::string> configuration;
    };
    Document retained(std::string_view reference, bool configuration, bool indexed,
                      bool intrinsic = false) {
        auto s = required(
            select(configuration ? "subathon_retained_configurations"
                                 : "subathon_retained_contexts",
                   configuration ? Columns(configuration_columns) : Columns(context_columns),
                   "reference=?", {blob(reference)}),
            indexed, intrinsic);
        bound_retained(s, configuration);
        const auto row = payload(s);
        if (text(row, 0) != reference)
            fail(Reason::invalid_stored_data);
        Document result{data(row, 1), {}};
        if (!configuration && !s.fields[2].null) {
            result.configuration = text(row, 2);
            // A context's own FK target must exist even before candidate comparison.
            (void)required(select("subathon_retained_configurations", configuration_columns,
                                  "reference=?", {blob(*result.configuration)}),
                           indexed, true);
        }
        return result;
    }
    std::optional<ReferenceInitialCapture> index(const ReferenceCaptureKey &k,
                                                 std::optional<core::ReceiptId> receipt = {}) {
        QString where;
        QList<QVariant> bindings;
        if (receipt) {
            where = "installation_ref=? AND receipt_id=?";
            bindings = {blob(k.installation_ref), encode_plan_revision_uint64(receipt->value)};
        } else {
            where = "installation_ref=? AND provider=? AND source_account=? AND "
                    "identifier_namespace=? AND opaque_id=?";
            for (auto v : key_views(k))
                bindings.append(blob(v));
        }
        auto s = select("subathon_reference_initial_captures", index_columns, where, bindings);
        if (!s)
            return {};
        const auto row = payload(*s);
        ReferenceInitialCapture r{
            {text(row, 0), {{text(row, 1), text(row, 2)}, text(row, 3), text(row, 4)}},
            {identifier(row, 5, true)},
            {identifier(row, 6)},
            text(row, 7),
            text(row, 8),
            text(row, 9),
            {text(row, 10), identifier(row, 11, true)},
            text(row, 12)};
        if (r.plan.revision != 1 || r.key.installation_ref != k.installation_ref ||
            (receipt ? r.receipt_id != *receipt : r.key.source != k.source))
            fail(Reason::invalid_stored_data);
        return r;
    }
    std::pair<Selection, SqlRow> receipt_target(std::uint64_t receipt) {
        const auto target = required(select("subathon_receipts", receipt_columns, "receipt_id=?",
                                            {encode_plan_revision_uint64(receipt)}),
                                     true, true);
        domain(target, 6, 0, std::numeric_limits<std::uint32_t>::max());
        group(target, 10, 11);
        group(target, 12, 13);
        group(target, 14, 15);
        bound_receipt(target);
        const auto row = payload(target);
        (void)retained(text(row, 18), true, true, true);
        (void)retained(text(row, 19), false, true, true);
        return {target, row};
    }
    ReferenceExecutionBinding closure(const ReferenceInitialCapture &r, bool indexed);
    struct ChildShape {
        qint64 count;
        bool dependent;
    };
    ChildShape children(const char *table, std::string_view plan) {
        const bool requests = std::string_view(table) == "subathon_plan_requests";
        // The supported immutable profile has exactly one rule/request and no
        // modifiers. Read at most two fixed metadata rows; never aggregate an
        // attacker-sized plan or materialize its unselected payloads.
        const auto dependency =
            requests ? QString("CASE WHEN typeof(requires_historical_valuation)='integer' "
                               "AND requires_historical_valuation IN (0,1) THEN "
                               "requires_historical_valuation ELSE NULL END")
                     : QString("0");
        const auto rows =
            execute({"SELECT CASE WHEN typeof(ordinal)='integer' THEN ordinal ELSE NULL END," +
                         dependency + " FROM main." + QString::fromLatin1(table) +
                         " WHERE plan_ref=? AND revision=? ORDER BY ordinal LIMIT 2",
                     {blob(plan), encode_plan_revision_uint64(1)}});
        if (rows.size() > 1)
            fail(Reason::invalid_stored_data);
        if (rows.empty())
            return {0, false};
        if (!integer_type(rows[0][0]) || rows[0][0].toLongLong() != 1 || !integer_type(rows[0][1]))
            fail(Reason::invalid_stored_data);
        return {1, rows[0][1].toLongLong() != 0};
    }

  private:
    SqliteWorkSession session_;
    ReferenceCaptureLimits limits_;
};
ReferenceExecutionBinding Access::closure(const ReferenceInitialCapture &r, bool indexed) {
    // Selected roots validate their own storage structure/targets before any
    // comparison with a newly proposed binding. Current state is never read.
    const auto receipt_shape = [&](const Selection &s) {
        domain(s, 6, 0, std::numeric_limits<std::uint32_t>::max());
        group(s, 10, 11);
        group(s, 12, 13);
        group(s, 14, 15);
    };
    const auto receipt = required(select("subathon_receipts", receipt_columns, "receipt_id=?",
                                         {encode_plan_revision_uint64(r.receipt_id.value)}),
                                  indexed);
    receipt_shape(receipt);
    bound_receipt(receipt);
    const auto original = payload(receipt);
    if (identifier(original, 0, true) != r.receipt_id.value)
        fail(Reason::invalid_stored_data);
    const auto cfg_ref = text(original, 18), arrival_ref = text(original, 19);
    auto cfg_document = retained(cfg_ref, true, indexed, true);
    auto arrival_document = retained(arrival_ref, false, indexed, true);
    if (!arrival_document.configuration)
        fail(Reason::invalid_stored_data);
    match(*arrival_document.configuration == cfg_ref, indexed);
    const core::ReceiptIdentity source{
        {text(original, 1), text(original, 2)}, text(original, 3), text(original, 4)};
    match(source == r.key.source && cfg_ref == r.configuration_ref &&
              arrival_ref == r.arrival_context_ref,
          indexed);
    bool absent_receipt_fields = true;
    for (std::size_t i = 8; i <= 15; ++i)
        absent_receipt_fields &= receipt.fields[i].null;
    match(text(original, 5) == "synthetic.reference.support.v1" && integer(original, 6) == 1 &&
              absent_receipt_fields,
          indexed);

    const auto contribution =
        required(select("subathon_contributions", contribution_columns, "contribution_id=?",
                        {encode_plan_revision_uint64(r.contribution_id.value)}),
                 indexed);
    group(contribution, 2, 5);
    bound_contribution(contribution);
    const auto c = payload(contribution);
    const auto original_receipt_id = identifier(c, 1, true);
    if (identifier(c, 0) != r.contribution_id.value)
        fail(Reason::invalid_stored_data);
    // An existing contribution's original support anchor is intrinsic, even if
    // the proposed source subsequently turns out to be a different receipt.
    std::optional<std::pair<Selection, SqlRow>> support_event;
    if (original_receipt_id != r.receipt_id.value)
        support_event = receipt_target(original_receipt_id);
    const auto support =
        required(select("subathon_contribution_observations", observation_columns, "receipt_id=?",
                        {encode_plan_revision_uint64(original_receipt_id)}),
                 indexed, true);
    domain(support, 2, 0, 1);
    domain(support, 3, 0, 3);
    group(support, 4, 8);
    if (support.fields[1].null || support.fields[2].integer != 0)
        fail(Reason::invalid_stored_data);
    for (auto [i, hi] : std::array<std::pair<std::size_t, qint64>, 7>{
             {{9, 1}, {13, 1}, {14, 3}, {15, 2}, {20, 1}, {21, 2}, {22, 1}}})
        domain(support, i, 0, hi);
    if (support.fields[16].null)
        fail(Reason::invalid_stored_data);
    group(support, 10, 11);
    group(support, 17, 19);
    if (!support.fields[17].null)
        domain(support, 17, 0, 1);
    bound_support(support, support_event ? support_event->first : receipt);
    const auto o = payload(support);
    if (identifier(o, 0, true) != original_receipt_id ||
        identifier(o, 1) != r.contribution_id.value)
        fail(Reason::invalid_stored_data);
    // The original support row names this same actual receipt (checked above).
    // Validate both activity origins and the complete anchor key before deciding
    // whether the healthy generic activity is supported by the reference profile.
    const auto &anchor_receipt = support_event ? support_event->second : original;
    const bool activity = !contribution.fields[2].null;
    if (activity != !support.fields[4].null)
        fail(Reason::invalid_stored_data);
    if (activity) {
        if (data(c, 2) != data(anchor_receipt, 1) || data(c, 3) != data(anchor_receipt, 2) ||
            data(o, 4) != data(anchor_receipt, 1) || data(o, 5) != data(anchor_receipt, 2))
            fail(Reason::invalid_stored_data);
        for (std::size_t i = 0; i < 4; ++i)
            if (data(c, 2 + i) != data(o, 4 + i))
                fail(Reason::invalid_stored_data);
    }
    match(original_receipt_id == r.receipt_id.value && contribution.fields[2].null &&
              support.fields[4].null && integer(o, 3) == 3 && integer(o, 9) == 0 &&
              support.fields[10].null && support.fields[12].null && integer(o, 13) == 0 &&
              integer(o, 14) == 1 && integer(o, 15) == 0 && text(o, 16) == cfg_ref &&
              support.fields[17].null && integer(o, 20) == 0 && integer(o, 21) == 0 &&
              integer(o, 22) == 1,
          indexed);

    const auto cfg =
        decode(cfg_document.bytes, reference_capture_detail::decode_configuration_body);
    const auto arrival =
        decode(arrival_document.bytes, reference_capture_detail::decode_arrival_body);
    // The retained bytes, not re-encoded output, supply the embedded-byte comparison.
    const auto embedded = [&]() -> std::string_view {
        const auto b = std::string_view(arrival_document.bytes.constData(),
                                        std::size_t(arrival_document.bytes.size()));
        std::size_t pos = 4;
        auto field = [&]() {
            if (b.size() - pos < 4)
                fail(Reason::invalid_stored_data);
            std::uint32_t n = 0;
            for (int i = 0; i < 4; ++i)
                n = (n << 8) | std::uint8_t(b[pos++]);
            if (n > b.size() - pos)
                fail(Reason::invalid_stored_data);
            const auto v = b.substr(pos, n);
            pos += n;
            return v;
        };
        (void)field();
        (void)field();
        if (b.size() - pos < 8)
            fail(Reason::invalid_stored_data);
        pos += 8;
        return field();
    }();
    const auto cfg_bytes =
        std::string_view(cfg_document.bytes.constData(), std::size_t(cfg_document.bytes.size()));
    const auto complete = ReferenceFactsState::complete;
    match(embedded == cfg_bytes && cfg.configuration_ref == cfg_ref &&
              cfg.synthetic_origin == source.origin &&
              cfg.receipt_namespace == source.identifier_namespace &&
              cfg.synthetic_origin.provider == "synthetic.reference.v1" &&
              cfg.other_rules == complete && cfg.modifiers == complete &&
              cfg.milestones == complete && cfg.outbound_actions == complete &&
              cfg.generated_events == complete && !cfg.timer_config.hard_end_utc_ms &&
              cfg.rule.automation.installation_ref == r.key.installation_ref &&
              arrival.arrival_context_ref == arrival_ref && arrival.event_ref == r.event_ref &&
              arrival.server_arrival_utc_ms == integer(original, 16) &&
              arrival.observation.connection_ref == text(original, 17) &&
              arrival.observation.event_id == source.opaque_id &&
              arrival.observation.source_utc_ms ==
                  (receipt.fields[7].null ? std::optional<std::int64_t>{}
                                          : std::optional<std::int64_t>{integer(original, 7)}),
          indexed);
    auto execution_document = retained(r.execution_context_ref, false, indexed);
    if (!execution_document.configuration)
        fail(Reason::invalid_stored_data);
    const auto execution =
        decode(execution_document.bytes, reference_capture_detail::decode_execution_body);
    match(*execution_document.configuration == cfg_ref &&
              execution.execution_context_ref == r.execution_context_ref &&
              execution.arrival_context_ref == arrival_ref &&
              execution.installation_ref == r.key.installation_ref && execution.source == source &&
              execution.receipt_id == r.receipt_id &&
              execution.contribution_id == r.contribution_id &&
              execution.plan.plan_ref == r.plan.plan_ref && execution.plan.revision == 1 &&
              same(execution.rule, cfg.rule),
          indexed);

    const auto header = required(
        select("subathon_plan_headers", header_columns, "plan_ref=?", {blob(r.plan.plan_ref)}),
        indexed);
    const auto header_receipt = header.fields[4].identifier;
    std::optional<std::pair<Selection, SqlRow>> other_event;
    if (header_receipt != r.receipt_id.value)
        other_event = receipt_target(header_receipt);
    const auto &event_metadata = other_event ? other_event->first : receipt;
    const auto &event = other_event ? other_event->second : original;
    if (!header.fields[5].null && header.fields[5].identifier != r.contribution_id.value) {
        const auto target =
            required(select("subathon_contributions", contribution_columns, "contribution_id=?",
                            {encode_plan_revision_uint64(header.fields[5].identifier)}),
                     indexed, true);
        group(target, 2, 5);
        receipt_target(target.fields[1].identifier);
    }
    const auto revision =
        required(select("subathon_plan_revisions", revision_columns, "plan_ref=? AND revision=?",
                        {blob(r.plan.plan_ref), encode_plan_revision_uint64(1)}),
                 indexed, true);
    domain(revision, 3, 0, 3);
    for (auto [i, hi] : std::array<std::pair<std::size_t, qint64>, 7>{
             {{4, 1}, {8, 1}, {9, 3}, {10, 2}, {15, 1}, {16, 2}, {17, 1}}})
        domain(revision, i, 0, hi);
    domain(revision, 18, 0, 2);
    group(revision, 5, 6);
    group(revision, 12, 14);
    if (!revision.fields[12].null)
        domain(revision, 12, 0, 1);
    if (!revision.fields[2].null)
        fail(Reason::invalid_stored_data);
    for (std::size_t i = 30; i < 34; ++i)
        if (!revision.fields[i].null)
            fail(Reason::invalid_stored_data);
    if (revision.fields[18].integer != 2) {
        for (std::size_t i = 19; i <= 29; ++i)
            if (!revision.fields[i].null)
                fail(Reason::invalid_stored_data);
    } else {
        for (std::size_t i : {19, 20, 21, 22, 23, 25})
            if (revision.fields[i].null)
                fail(Reason::invalid_stored_data);
    }
    const auto rule_shape = children("subathon_plan_rules", r.plan.plan_ref);
    const auto modifier_shape = children("subathon_plan_modifiers", r.plan.plan_ref);
    const auto request_shape = children("subathon_plan_requests", r.plan.plan_ref);
    if (revision.fields[18].integer == 1 && request_shape.dependent)
        fail(Reason::invalid_stored_data);
    const bool supported_shape =
        rule_shape.count == 1 && modifier_shape.count == 0 && request_shape.count == 1;
    std::optional<Selection> selected_rule, selected_request;
    if (supported_shape) {
        selected_rule = required(select("subathon_plan_rules", rule_columns,
                                        "plan_ref=? AND revision=? AND ordinal=1",
                                        {blob(r.plan.plan_ref), encode_plan_revision_uint64(1)}),
                                 indexed, true);
        selected_request = required(select("subathon_plan_requests", request_columns,
                                           "plan_ref=? AND revision=? AND ordinal=1",
                                           {blob(r.plan.plan_ref), encode_plan_revision_uint64(1)}),
                                    indexed, true);
        domain(*selected_rule, 5, 0, 1);
        domain(*selected_request, 7, 0, 2);
        domain(*selected_request, 8, 0, 1);
        bound_plan(header, event_metadata, revision, *selected_rule, *selected_request);
    }
    // An off-profile shape owns no selected typed snapshot. Its bounded revision
    // row can still establish intrinsic quote coupling before the narrow refusal.
    const auto v = payload(revision);
    if (revision.fields[18].integer == 2 &&
        (event_metadata.fields[10].null || data(v, 20) != data(event, 10) ||
         data(v, 21) != data(event, 11)))
        fail(Reason::invalid_stored_data);
    match(supported_shape, indexed);
    const auto &rule = *selected_rule, &request = *selected_request;
    const auto h = payload(header), rules = payload(rule), q = payload(request);
    if (identifier(v, 1, true) != 1 || text(v, 0) != r.plan.plan_ref ||
        identifier(rules, 1, true) != 1 || identifier(q, 1, true) != 1 ||
        text(rules, 0) != r.plan.plan_ref || text(q, 0) != r.plan.plan_ref)
        fail(Reason::invalid_stored_data);
    try {
        (void)decode_plan_exact({data(q, 9), data(q, 10)}, limits_.owned_bytes);
    } catch (const SqlitePlanInputFailure &e) {
        fail(e.error() == SqlitePlanInputError::exact_component_limit
                 ? Reason::result_limit
                 : Reason::invalid_stored_data);
    }
    bool state_equal = integer(v, 3) == integer(o, 3);
    for (std::size_t i = 0; i < 14; ++i) {
        const auto a = i + 4, b = i + 9;
        state_equal &= revision.fields[a].null == support.fields[b].null;
        if (!revision.fields[a].null && !support.fields[b].null)
            state_equal &= revision_columns[a].kind == Kind::integer
                               ? integer(v, a) == integer(o, b)
                               : data(v, a) == data(o, b);
    }
    std::array<char, 20> decimal{};
    const auto spelling =
        std::to_chars(decimal.data(), decimal.data() + decimal.size(), cfg.rule.add_ms);
    if (spelling.ec != std::errc{})
        fail(Reason::invalid_stored_data);
    const auto exact = std::string_view(decimal.data(), std::size_t(spelling.ptr - decimal.data()));
    const auto equals = [&](std::size_t field, std::string_view expected) {
        const auto bytes = data(q, field);
        return std::string_view(bytes.constData(), std::size_t(bytes.size())) == expected;
    };
    match(text(h, 0) == r.plan.plan_ref && text(h, 1) == r.event_ref && header.fields[2].null &&
              text(h, 3) == r.event_ref && header_receipt == r.receipt_id.value &&
              !header.fields[5].null && identifier(h, 5) == r.contribution_id.value &&
              state_equal && integer(v, 18) == 0 && text(rules, 3) == cfg.rule.rule_ref &&
              text(rules, 4) == cfg.rule.rule_version_ref && integer(rules, 5) == 1 &&
              text(rules, 6) == "reference.single-rule.match.v1" &&
              text(q, 3) == cfg.rule.action_ref && text(q, 4) == cfg.rule.action_version_ref &&
              text(q, 5) == cfg.rule.rule_ref && text(q, 6) == cfg.rule.timer_ref &&
              integer(q, 7) == 0 && integer(q, 8) == 0 && equals(9, exact) && equals(10, "1") &&
              integer(q, 11) == cfg.rule.add_ms && equals(12, "reference.integer-ms.v1") &&
              equals(13, "reference.original-time.v1"),
          indexed);
    return execution;
}
} // namespace subathon::server::reference_authority_capture
namespace subathon::server {
namespace {
bool same_principal(const ApplicationPrincipalBinding &a, const ApplicationPrincipalBinding &b) {
    return a.kind == b.kind && a.installation_ref == b.installation_ref &&
           a.principal_ref == b.principal_ref;
}
void match_registry(const ReferenceAuthorityRecord &rec, const ReferenceAuthorityRequest &q) {
    const auto &b = rec.binding;
    const auto &x = q.captured;
    const auto &r = x.rule;
    data(b.installation == x.installation_ref && b.installation == q.principal.installation_ref &&
         same_principal(q.principal, r.automation) && b.principal == q.principal.principal_ref &&
         b.capability == r.automation_capability_ref && b.rule == r.rule_ref &&
         b.rule_version == r.rule_version_ref && b.action == r.action_ref &&
         b.action_version == r.action_version_ref && b.timer == r.timer_ref &&
         b.origin == x.source.origin && b.identifier_namespace == x.source.identifier_namespace &&
         b.requested_ms == r.add_ms);
}
void match_execution(const ReferenceExecutionBinding &a, const ReferenceExecutionBinding &b) {
    data(a.execution_context_ref == b.execution_context_ref &&
         a.arrival_context_ref == b.arrival_context_ref &&
         a.installation_ref == b.installation_ref && a.source == b.source &&
         a.receipt_id == b.receipt_id && a.contribution_id == b.contribution_id &&
         a.plan.plan_ref == b.plan.plan_ref && a.plan.revision == b.plan.revision &&
         reference_authority_capture::same(a.rule, b.rule));
}
// Admit and decode the complete selected historical closure before owning or
// decoding its retained SAC1 command. Historical facts never grant authority.
void historical(const SqliteWorkSession &s, const ReferenceAuthorityRequest &q,
                const ReferenceAuthorityLimits &l) {
    if (q.use == ReferenceAuthorityUse::execute_time_add)
        return;
    const auto &key = *q.historical_operation;
    const auto hl = historical_limits(l); // Validated before any SQL by preflight_request.
    const auto &bl = hl.references;
    auto check_failure = [](selected_record_read::Failure f) {
        if (f == selected_record_read::Failure::missing)
            fail(F::missing);
        if (f == selected_record_read::Failure::resource_limit)
            fail(F::resource_limit);
        fail(F::invalid_data);
    };
    std::optional<RecordedApplicationBundle> application;
    std::optional<ApplicationHoldView> hold;
    const ApplicationEnvelope *retained_command;
    if (q.use == ReferenceAuthorityUse::disclose_application) {
        auto result = selected_record_read::read_application(s, key, bl);
        if (auto f = std::get_if<selected_record_read::Failure>(&result))
            check_failure(*f);
        application = std::move(std::get<RecordedApplicationBundle>(result));
        const auto &intent = application->intent;
        data(intent.operation.operation_scope == key.operation_scope &&
             intent.operation.operation_ref == key.operation_ref &&
             intent.receipt_id == q.captured.receipt_id &&
             intent.contribution_id == std::optional<ContributionId>{q.captured.contribution_id} &&
             intent.plan.plan_ref == q.captured.plan.plan_ref &&
             intent.plan.revision == q.captured.plan.revision);
        retained_command = &intent.choices;
    } else {
        auto result = selected_record_read::read_hold(s, *q.historical_hold, hl);
        if (auto f = std::get_if<selected_record_read::Failure>(&result))
            check_failure(*f);
        hold = std::move(std::get<ApplicationHoldView>(result));
        const auto &cause = hold->cause;
        data(cause.command.operation.operation_scope == key.operation_scope &&
             cause.command.operation.operation_ref == key.operation_ref &&
             cause.source.receipt_id == q.captured.receipt_id &&
             std::holds_alternative<ContributionId>(cause.source.subject.id) &&
             std::get<ContributionId>(cause.source.subject.id) == q.captured.contribution_id &&
             cause.plan_evidence.plan &&
             cause.plan_evidence.plan->plan_ref == q.captured.plan.plan_ref &&
             cause.plan_evidence.plan->revision == q.captured.plan.revision &&
             cause.source.arrival_context_ref == q.captured.arrival_context_ref);
        retained_command = &cause.command.intent;
    }
    ApplicationServiceLimits sl;
    sl.reference_bytes = qsizetype(l.reference_bytes);
    sl.storage_document_bytes = l.captures.document_bytes;
    sl.intermediate_owned_bytes = l.captures.owned_bytes;
    sl.intermediate_items = qsizetype(l.result_items);
    OriginalApplicationCommand command;
    try {
        command = service_detail::decode_retained_command(*retained_command, sl);
    } catch (const ApplicationServiceInputFailure &e) {
        if (e.error() == ApplicationServiceInputError::result_limit)
            fail(F::resource_limit);
        fail(F::invalid_data);
    }
    data(command.operation.operation_scope == key.operation_scope &&
         command.operation.operation_ref == key.operation_ref &&
         command.receipt_id == q.captured.receipt_id &&
         command.contribution_id == std::optional<ContributionId>{q.captured.contribution_id} &&
         command.plan.plan_ref == q.captured.plan.plan_ref &&
         command.plan.revision == q.captured.plan.revision &&
         same_principal(command.principal, q.principal) && command.choices.size() == 1);
    const auto &c = command.choices[0];
    data(c.format_ref == "subathon.reference.execution-ref.v1");
    // SRF1 is a structural pointer to the immutable execution context, never a permit.
    auto b = std::string_view(c.document.constData(), std::size_t(c.document.size()));
    data(b.size() >= 8 && b.substr(0, 4) == "SRF1");
    std::uint32_t len = 0;
    for (unsigned char v : b.substr(4, 4))
        len = (len << 8) | v;
    data(b.size() - 8 == len && b.substr(8) == q.captured.execution_context_ref);
}
} // namespace
ReferenceAuthorityFacts
SqliteReferenceAuthorityUnit::Impl::resolve(const ReferenceAuthorityRequest &q) {
    data(q.principal.installation_ref == q.captured.installation_ref &&
         q.principal.installation_ref == q.captured.rule.automation.installation_ref);
    auto rec =
        record(q.principal.installation_ref, q.captured.rule.automation_capability_ref, false);
    match_registry(rec, q);
    reference_authority_capture::Access capture(session, limits.captures);
    Projection capture_key{std::uint64_t(limits.captures.owned_bytes), 0,
                           std::uint64_t(limits.captures.reference_bytes)};
    capture_key(q.captured.installation_ref, q.captured.source);
    ReferenceCaptureKey key{q.captured.installation_ref, q.captured.source};
    auto index = capture.index(key);
    if (!index)
        fail(F::missing);
    auto execution = capture.closure(*index, true);
    match_execution(execution, q.captured);
    auto g = graph(rec.binding.installation);
    if (rec.state != ReferenceAuthorityState::authorized || !enabled(g, rec.original_author) ||
        !enabled(g, rec.execution_account))
        fail(F::denied);
    auto request = concrete(rec.binding, unsigned(q.use), limits);
    auto action = singleton(request, limits);
    Hash ah("RAA1");
    ah.field(rec.binding.installation);
    ah.integer(rec.head_revision, 8);
    ah.field(rec.binding.capability);
    ah.integer(rec.created_revision, 8);
    ah.integer(rec.state_revision, 8);
    ah.integer(unsigned(rec.state), 1);
    ah.field(rec.original_author);
    ah.field(rec.execution_account);
    auto automation = ah.finish("RAA1");
    Hash ph("RAR1");
    ph.field(g.installation);
    ph.integer(g.revision, 8);
    ph.field(g.catalog.revision);
    ph.field(*g.owner.account);
    ph.field(g.current_installation.current.session_epoch_ref);
    ph.field(g.current_installation.revision_ref);
    space(g.current_accounts.size() <= UINT32_MAX);
    ph.integer(g.current_accounts.size(), 4);
    for (const auto &a : g.current_accounts) {
        ph.field(a.current.account_ref);
        ph.field(a.current.session_epoch_ref);
        ph.field(a.revision_ref);
        ph.integer(a.current.application_access_enabled, 1);
    }
    ph.field(automation);
    auto revision = ph.finish("RAR1");
    // Project the WHOLE facts, including every repeated source, policy and snapshot
    // before materializing the three expanded policies or the returned facts.
    Projection p{limits.result_bytes, limits.result_items, limits.reference_bytes};
    p(q, rec, revision, automation);
    p(rec.original_author, revision, g.catalog, g.owner);
    project_policy(p, g.use, rec.original_author);
    project_policy(p, g.grant, rec.original_author);
    p.elements(1);
    p(action);
    p(rec.execution_account, revision, g.catalog, g.owner);
    project_policy(p, g.use, rec.execution_account);
    p(request);
    Projection author_input{limits.permissions.input_owned_bytes, limits.permissions.input_items,
                            limits.permissions.reference_bytes};
    author_input.component_bytes = limits.permissions.component_bytes;
    author_input(rec.original_author, revision, g.catalog, g.owner);
    project_policy(author_input, g.use, rec.original_author);
    project_policy(author_input, g.grant, rec.original_author);
    author_input.elements(1);
    author_input(action);
    Projection execution_input{limits.permissions.input_owned_bytes, limits.permissions.input_items,
                               limits.permissions.reference_bytes};
    execution_input.component_bytes = limits.permissions.component_bytes;
    execution_input(rec.execution_account, revision, g.catalog, g.owner);
    project_policy(execution_input, g.use, rec.execution_account);
    execution_input(request);
    ReferenceAuthorityFacts out;
    out.original_author_input = {rec.original_author,
                                 revision,
                                 g.catalog,
                                 g.owner,
                                 policy(g.use, rec.original_author, limits),
                                 policy(g.grant, rec.original_author, limits),
                                 {std::move(action)}};
    out.permission_input = {rec.execution_account,
                            revision,
                            g.catalog,
                            g.owner,
                            policy(g.use, rec.execution_account, limits),
                            std::move(request)};
    auto authorized =
        revalidate_permission_automation(out.original_author_input, limits.permissions);
    if (authorized.actions[0].disposition != PermissionAutomationDisposition::authorized ||
        evaluate_effective_permission(out.permission_input, limits.permissions).data().outcome !=
            PermissionOutcome::permit)
        fail(F::denied);
    historical(session, q, limits);
    out.binding = q;
    out.registry = std::move(rec);
    out.permission_revision_ref = std::move(revision);
    out.automation_revision_ref = std::move(automation);
    return out;
}
ReferenceAuthorityResolution
SqliteReferenceAuthorityUnit::resolve(const ReferenceAuthorityRequest &r) {
    preflight_request(r, impl_->limits);
    return {guarded<std::variant<ReferenceAuthorityFacts, F>>(
        [&]() -> std::variant<ReferenceAuthorityFacts, F> { return impl_->resolve(r); })};
}
SqliteReferenceAuthoritySource::SqliteReferenceAuthoritySource(ReferenceAuthorityLimits l)
    : limits_(l) {
    valid_limits(l);
}
ReferenceAuthorityResolution
SqliteReferenceAuthoritySource::resolve(const ApplicationReferenceReadView &v,
                                        const ReferenceAuthorityRequest &r) {
    return v.resolve_reference_authority(r, limits_);
}
ReferenceAuthorityResolution
ApplicationReferenceReadView::resolve_reference_authority(const ReferenceAuthorityRequest &r,
                                                          const ReferenceAuthorityLimits &l) const {
    valid_limits(l);
    preflight_request(r, l);
    auto state = state_.lock();
    if (!state || !state->active)
        throw ApplicationReferenceViewFailure(ApplicationReferenceViewError::expired_callback);
    if (QThread::currentThread() != state->owner)
        throw ApplicationReferenceViewFailure(ApplicationReferenceViewError::wrong_thread);
    auto selected = l;
    selected.reference_bytes =
        std::min(l.reference_bytes, std::uint64_t(state->limits.reference_bytes));
    selected.captures.reference_bytes =
        std::min(l.captures.reference_bytes, state->limits.reference_bytes);
    selected.captures.document_bytes =
        std::min(l.captures.document_bytes, state->limits.document_bytes);
    selected.result_bytes =
        std::min(l.result_bytes, std::uint64_t(state->intermediate_owned_bytes));
    selected.result_items = std::min(l.result_items, std::uint64_t(state->intermediate_items));
    return SqliteReferenceAuthorityUnit(state->session, selected).resolve(r);
}
} // namespace subathon::server
