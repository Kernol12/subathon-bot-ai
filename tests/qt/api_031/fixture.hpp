// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../storage_026/catalog_fixture.hpp"
#include "oracle.hpp"

#include <subathon/server/sqlite_permission_graph.hpp>

#include <QMap>
#include <QTemporaryDir>
#include <QThread>
#include <algorithm>
#include <array>
#include <type_traits>

namespace api_031_fixture {
namespace catalog = contribution_review_fixture;
namespace frozen = storage_026_fixture;
namespace core = subathon::core;
using qt_test_support::require;

inline constexpr StorageLimits work_limits{64, 4096, 65536, 1048576, 8192, 16777216};
inline constexpr qsizetype manifest_bytes = 1'594'845;

inline const std::array<std::string, 5> operations{
    "fixture.connector.register", "fixture.connector.activate", "fixture.connector.checkpoint",
    "fixture.connector.inspect", "fixture.connector.reconnect"};
inline const std::vector<std::string> scope_slots{"installation", "provider", "account", "stream"};
inline const SessionLookupRequest session_lookup = [] {
    AccountSessionDigest digest{};
    digest.back() = 0x42;
    return SessionLookupRequest{"install", digest};
}();

inline auto resources(const Scope &scope) {
    return std::vector<core::PermissionResourceBinding>{
        {"installation", scope.installation.toStdString()},
        {"provider", scope.provider.toStdString()},
        {"account", scope.account.toStdString()},
        {"stream", scope.stream.toStdString()}};
}

inline PermissionGraphTrust trust() {
    using enum core::PermissionOperationClass;
    PermissionGraphTrust trust;
    trust.catalog = {core::PermissionFactsState::complete,
                     "fixture.connector.startup.v1",
                     {{"roles.edit", ordinary, {"role"}, {}, {}},
                      {"members.edit", ordinary, {"account"}, {}, {}},
                      {"overrides.edit", ordinary, {"account"}, {}, {}}}};
    for (const auto &id : operations)
        trust.catalog.operations.push_back({id, ordinary, scope_slots, {}, {}});
    trust.role_edit = {"roles.edit", "role"};
    trust.membership_edit = {"members.edit", "account"};
    trust.override_edit = {"overrides.edit", "account"};
    return trust;
}

inline core::PermissionEditableState graph(bool allow_b, bool allow_inspect = true) {
    using namespace core;
    std::vector<PermissionPolicyEntry> entries;
    for (std::size_t i = 0; i < operations.size(); ++i) {
        if (!allow_inspect && i == 3)
            continue;
        PermissionPolicyEntry entry{PermissionOrigin::individual,
                                    operations[i],
                                    {},
                                    operations[i],
                                    PermissionEffect::allow,
                                    "grant/" + operations[i],
                                    {},
                                    {}};
        for (const auto &binding : resources(a))
            entry.resources.push_back(
                {binding.slot, PermissionResourceMode::allowed_set, {binding.resource}});
        if (allow_b)
            entry.resources[2].allowed.push_back("B");
        entries.push_back(std::move(entry));
    }
    return {PermissionFactsState::complete,
            {},
            {{"operator", {}, std::move(entries)}, {"owner", {}, {}}}};
}

class Clock final : public CurrentSessionClock {
  public:
    std::optional<qint64> now{301000};

    SessionClockResolution sample_current() override {
        if (!now)
            return SessionSourceUnavailable{};
        return SessionClockSample{*now};
    }
};

class Authority final : public StartupAuthority {
  public:
    const PermissionGraphTrust catalog{trust()};
    Clock clock;
    QByteArray decision_suffix;

    StartupDecision resolve(const SqliteWorkSession &session,
                            const StartupAuthorityRequest &request) override {
        const auto i = static_cast<std::size_t>(request.action);
        const auto facts = evaluate(session, request.binding, request.scope, request.require_fresh,
                                    operations.at(i));
        if (std::holds_alternative<StartupDenied>(facts))
            return StartupDenied{};
        if (std::holds_alternative<StartupUnavailable>(facts))
            return StartupUnavailable{};
        const auto &[principal, account_snapshot, permission_snapshot] = std::get<Facts>(facts);
        const auto decision =
            i == 3 ? QByteArray("D-inspect") : "D" + QByteArray::number(qulonglong(i));
        return StartupPermit{request.action,
                             request.scope,
                             principal,
                             account_snapshot,
                             permission_snapshot,
                             principal,
                             principal + '/' + decision + decision_suffix};
    }

    QByteArray reconnect_decision(const SqliteWorkSession &session) {
        const auto facts = evaluate(session, session_lookup, a, true, operations[4]);
        require(std::holds_alternative<Facts>(facts), "Current scoped reconnect grant permits");
        return std::get<Facts>(facts).principal + "/D3";
    }

  private:
    struct Facts {
        QByteArray principal, account_snapshot, permission_snapshot;
    };

    std::variant<StartupDenied, StartupUnavailable, Facts>
    evaluate(const SqliteWorkSession &session, const StartupBinding &binding, const Scope &scope,
             bool fresh, const std::string &operation) {
        SqliteAccountSessionUnit accounts(session, {{}});
        const auto installation = scope.installation.toStdString();
        std::string principal;
        QByteArray snapshot;
        if (const auto *lookup = std::get_if<SessionLookupRequest>(&binding)) {
            if (lookup->installation_ref != installation)
                return StartupDenied{};
            const auto resolved = resolve_current_account_session(*lookup, fresh, accounts, clock);
            if (const auto *failure = std::get_if<SessionFailure>(&resolved)) {
                switch (*failure) {
                case SessionFailure::missing:
                case SessionFailure::revoked:
                case SessionFailure::expired:
                case SessionFailure::fresh_authentication_required:
                    return StartupDenied{};
                default:
                    return StartupUnavailable{};
                }
            }
            const auto &actor = std::get<ResolvedAccountSession>(resolved);
            principal = actor.account_ref;
            snapshot = QByteArray::fromStdString(actor.snapshot_ref);
        } else {
            const auto &execution = std::get<StartupExecutionAccount>(binding);
            if (execution.installation != scope.installation)
                return StartupDenied{};
            principal = execution.account.toStdString();
        }

        const auto loaded = SqlitePermissionGraphUnit(session, installation, catalog).load();
        const auto *current = std::get_if<StoredPermissionGraph>(&loaded);
        if (!current)
            return StartupUnavailable{};
        const auto account = std::ranges::find_if(current->current_accounts, [&](const auto &row) {
            return row.current.account_ref == principal &&
                   row.current.installation_ref == installation;
        });
        const auto use = std::ranges::find_if(
            current->use.accounts, [&](const auto &row) { return row.account == principal; });
        if (account == current->current_accounts.end() || use == current->use.accounts.end())
            return StartupUnavailable{};
        if (!account->current.application_access_enabled)
            return StartupDenied{};
        require(use->assigned_roles.empty(), "Fixture's editable graph uses individual grants");
        if (snapshot.isEmpty())
            snapshot = QByteArray::fromStdString(account->revision_ref);
        const auto graph_revision = QByteArray::number(qulonglong(current->revision));
        const auto decision = core::evaluate_effective_permission(
            {principal,
             graph_revision.toStdString(),
             current->catalog,
             current->owner,
             {current->use.state, use->assigned_roles, use->individual_entries},
             {operation, resources(scope), {}}});
        if (decision.data().outcome != core::PermissionOutcome::permit)
            return StartupDenied{};
        return Facts{QByteArray::fromStdString(principal), snapshot, graph_revision};
    }
};

inline bool same_cut_request(const CutRequest &got, const CutRequest &want) {
    return got.purpose == want.purpose && got.prior_frontier == want.prior_frontier &&
           got.source.scope == want.source.scope && got.source.fence == want.source.fence &&
           got.source.profile == want.source.profile;
}

inline CutObservation cut_witness(bool second = false) {
    const auto &scope = second ? b : a;
    const auto &successor = second ? active_b : active;
    return {{{scope, successor, profile}, CutPurpose::first_connect, {}},
            {"key-v1", "S/history", QByteArray("10"), successor.generation,
             second ? "cutB10" : "cut10"},
            Endpoint::inclusive};
}

inline LiveProgressObservation live_witness(bool after_reconnect = false, QByteArray upper = "11") {
    return {{a, after_reconnect ? reconnected : active,
             after_reconnect ? "registry-r1" : "registry-r0", profile, cursor, b0,
             after_reconnect ? QList<StartupOperation>{r0} : QList<StartupOperation>{}},
            Coverage::contiguous,
            {Endpoint::exclusive, "key-v1", "S/history", QByteArray("10")},
            {Endpoint::inclusive, "key-v1", "S/history", upper},
            {},
            {},
            after_reconnect ? "live-empty-r1-11"
            : upper == "12" ? "live-empty-12"
                            : "live-empty-11"};
}

class Transcript final : public ConnectorHistoryAdapter, public ConnectorLiveProgressPort {
  public:
    explicit Transcript(bool second, bool after_reconnect = false)
        : cut_fact(cut_witness(second)),
          live_fact(second ? std::nullopt : std::optional(live_witness(after_reconnect))) {}

    void clear_invocation() {
        observed_cut.reset();
        observed_live.reset();
    }

    const CutObservation cut_fact;
    std::optional<LiveProgressObservation> live_fact;
    bool cut_available{true};
    int cuts{}, lives{}, pages{};

    CutResult observe_cut(const CutRequest &request) override {
        ++cuts;
        observed_cut.reset();
        if (!cut_available || !same_cut_request(request, cut_fact.request))
            return HistoryFailure::unproved;
        observed_cut = request;
        return cut_fact;
    }

    std::variant<Page, HistoryFailure> fetch_page(const HistoryRequest &) override {
        ++pages;
        return HistoryFailure::unsupported;
    }

    LiveProgressResult observe_progress(const LiveProgressRequest &request) override {
        ++lives;
        observed_live.reset();
        if (!live_fact || request != live_fact->request)
            return HistoryFailure::unproved;
        observed_live = request;
        return *live_fact;
    }

    bool validates(const Fence &before, const CutRequest &request,
                   const CutObservation &result) const {
        return observed_cut && same_cut_request(request, *observed_cut) &&
               attests(before, request, result);
    }

    bool attests(const Fence &before, const CutRequest &request,
                 const CutObservation &result) const {
        const auto &cut = cut_fact.cut;
        return before == (cut_fact.request.source.scope == a ? pending : pending_b) &&
               same_cut_request(request, cut_fact.request) &&
               same_cut_request(result.request, cut_fact.request) &&
               result.boundary == cut_fact.boundary && result.cut.format == cut.format &&
               result.cut.position_namespace == cut.position_namespace &&
               result.cut.position == cut.position &&
               result.cut.qualified_generation == cut.qualified_generation &&
               result.cut.proof == cut.proof;
    }

    bool validates(const LiveProgressRequest &request,
                   const LiveProgressObservation &result) const {
        return observed_live && request == *observed_live && attests(request, result);
    }

    bool attests(const LiveProgressRequest &request, const LiveProgressObservation &result) const {
        // The fixture records a closed feed marker with zero items.
        return live_fact && request == live_fact->request && result.request == live_fact->request &&
               result.coverage == live_fact->coverage && result.lower == live_fact->lower &&
               result.upper == live_fact->upper && result.ordered_inputs.isEmpty() &&
               result.dispositions.isEmpty() && result.proof == live_fact->proof;
    }

  private:
    std::optional<CutRequest> observed_cut;
    std::optional<LiveProgressRequest> observed_live;
};

class Registry final : public StartupProfileRegistry {
  public:
    Transcript first{false};
    Transcript second{true};
    Transcript reconnect{false, true};

    void clear_invocations() {
        first.clear_invocation();
        second.clear_invocation();
        reconnect.clear_invocation();
    }

    void use_reconnect() { binding_a = bind(a, reconnect, "registry-r1", "g3"); }

    std::shared_ptr<const StartupProfile> select(const Scope &scope, const QByteArray &p,
                                                 const QByteArray &c) override {
        if (p != profile || c != cursor)
            return {};
        if (scope == a)
            return binding_a;
        return scope == b ? binding_b : nullptr;
    }

  private:
    static auto bind(const Scope &scope, Transcript &port, QByteArray revision,
                     QByteArray generation) -> std::shared_ptr<const StartupProfile> {
        return std::make_shared<const StartupProfile>(StartupProfile{
            scope, std::move(revision), profile, cursor, std::move(generation), port, port,
            [&port](const auto &before, const auto &request, const auto &observed) {
                return port.validates(before, request, observed);
            },
            [&port](const auto &request, const auto &observed) {
                return port.validates(request, observed);
            },
            [&port](const auto &before, const auto &request, const auto &retained) {
                return port.attests(before, request, retained);
            },
            [&port](const auto &request, const auto &retained) {
                return port.attests(request, retained);
            }});
    }

    std::shared_ptr<const StartupProfile> binding_a{bind(a, first, "registry-r0", "g2")};
    std::shared_ptr<const StartupProfile> binding_b{bind(b, second, "registry-r0", "gB")};
};

using Image = QMap<QString, SqlRows>;

inline void canonical_cells(SqlRows &rows) {
    for (auto &row : rows)
        for (auto &value : row)
            if (value.isNull())
                value = QVariant{};
            else if (value.typeId() == QMetaType::Int)
                value = QVariant::fromValue(value.toLongLong());
}

inline void same_image(const Image &actual, Image expected) {
    require(actual.keys() == expected.keys(), "All tables in the protected catalog are observed");
    for (auto it = expected.begin(); it != expected.end(); ++it) {
        canonical_cells(it.value());
        const auto &rows = actual[it.key()];
        require(rows.size() == it.value().size(), "Exact row count in " + it.key());
        for (qsizetype i = 0; i < rows.size(); ++i) {
            const auto &want = it.value()[i];
            require(rows[i].size() == want.size(), "Exact tuple width in " + it.key());
            for (qsizetype j = 0; j < want.size(); ++j)
                require(rows[i][j].typeId() == want[j].typeId() && rows[i][j] == want[j],
                        QString("Exact type and value in %1 row %2 column %3")
                            .arg(it.key())
                            .arg(i)
                            .arg(j));
        }
    }
}

class Store {
  public:
    const MigrationManifest shipped{connector_intake_repository_manifest()};
    const MigrationManifest literal{frozen::successor()};

    Store() {
        require(directory.isValid() && shipped.catalog_format == literal.catalog_format &&
                    shipped.store_identity == literal.store_identity &&
                    shipped.supported_starting_versions == literal.supported_starting_versions &&
                    shipped.steps.size() == 20 && literal.steps.size() == 20,
                "Shipped v20 catalog matches the protected independent prefix");
        for (qsizetype i = 0; i < literal.steps.size(); ++i) {
            const auto &got = shipped.steps[i];
            const auto &wanted = literal.steps[i];
            require(got.version == wanted.version && got.id == wanted.id && got.sql == wanted.sql &&
                        got.expected_schema == wanted.expected_schema,
                    "Each shipped step matches the literal ordered SQL and projection");
        }
        const auto &tail = literal.steps.back();
        require(tail.version == 20 && tail.id == "connector-intake-storage-v1" &&
                    tail.expected_schema.size() == 253 && tail.sql.size() == 14 &&
                    catalog::tally(literal) == manifest_bytes &&
                    catalog::digest(tail).toHex() ==
                        "eb1758529cd052df4f8ed80ab693a6219afa519ae9db5f1b00f2f6bae05bf7e1",
                "Frozen v20 count, framing and charge");
        open();
        catalog::report(migration(MigrationMode::inspect), CatalogState::fresh, 0, 20, 0);
        catalog::report(migration(MigrationMode::advance), CatalogState::current, 20, 20, 20);
        inspect_catalog();
        int count = 0;
        for (const auto &object : frozen::delta_objects)
            if (object.type == "table") {
                ++count;
                require(rows("SELECT * FROM main." + object.name).isEmpty(),
                        "Every protected intake table starts empty");
            }
        require(count == 9, "Exact intake table inventory");
    }

    SqliteWriter &writer() { return *writer_; }

    void reopen() {
        auto closed = std::make_shared<bool>(false);
        require(writer().shutdown([closed] { *closed = true; }) == StorageError::none &&
                    QTest::qWaitFor([closed] { return *closed; }, 10000),
                "Single old writer finishes shutdown before reopen");
        require(reader.close() == StorageError::none, "Read observer closes before reopen");
        writer_.reset();
        open();
        inspect_catalog();
    }

    SqlRows rows(const QString &query) {
        auto result = reader.read({query, {}});
        require(result.error == StorageError::none, "Independent read-only observation succeeds");
        return std::move(result.rows);
    }

    Image image() {
        Image result;
        for (const auto &object : literal.steps.back().expected_schema) {
            if (object.type != "table")
                continue;
            const auto info =
                rows("SELECT cid FROM pragma_table_info('" + object.name + "') ORDER BY cid");
            QStringList ordinals;
            for (const auto &column : info)
                ordinals.append(QString::number(column[0].toInt() + 1));
            require(!ordinals.empty(), "Protected table columns are visible");
            auto observed =
                rows("SELECT * FROM main.\"" + object.name + "\" ORDER BY " + ordinals.join(','));
            canonical_cells(observed);
            result.insert(object.name, std::move(observed));
        }
        return result;
    }

    QVariant work(const std::function<QVariant(const SqliteWorkSession &)> &body) {
        auto failure = std::make_shared<std::exception_ptr>();
        const auto result = qt_test_support::run_work(
            writer(),
            [body, failure](auto context) {
                const auto session = context.begin();
                try {
                    auto value = body(session);
                    session.commit();
                    return value;
                } catch (...) {
                    *failure = std::current_exception();
                    throw;
                }
            },
            10000);
        if (*failure)
            std::rethrow_exception(*failure);
        catalog::finance::normal(result);
        return *result.value;
    }

    qint64 changes() {
        return work([](const auto &s) {
                   return s.execute({"SELECT total_changes()", {}}).front().front();
               })
            .toLongLong();
    }

  private:
    QTemporaryDir directory;
    SqliteReader reader;
    std::unique_ptr<SqliteWriter> writer_;
    QString path() const { return directory.filePath("startup.sqlite"); }

    void open() {
        writer_ = std::make_unique<SqliteWriter>();
        require(writer().open(path(), work_limits) == StorageError::none &&
                    reader.open(path()) == StorageError::none,
                "Real owner writer and independent read-only observer open");
    }

    MigrationResult migration(MigrationMode mode) {
        MigrationLimits limits;
        limits.schema_objects = 253;
        limits.manifest_utf8_bytes = manifest_bytes;
        return catalog::migrate(writer(), {mode, shipped, limits, {}});
    }

    void inspect_catalog() {
        catalog::report(migration(MigrationMode::inspect), CatalogState::current, 20, 20, 0);
        require(rows(catalog::history_query) == catalog::expected_history(literal),
                "Exact committed migration history for all twenty shipped versions");
        catalog::expect_schema(rows(catalog::schema_query), literal.steps.back());
    }
};

class Fixture {
  public:
    Store store;
    Authority authority;
    Registry registry;
    StartupConfiguration configuration;
    const bool two;
    const bool disjoint;

    explicit Fixture(bool two = false, bool disjoint = false, bool allow_b = true)
        : configuration{"install",
                        config,
                        "operator",
                        {{member(a), "review-lane", "g0", "g2", profile, cursor, true}}},
          two(two), disjoint(disjoint) {
        if (two) {
            configuration.lanes.append({member(b, disjoint),
                                        disjoint ? "disjoint-lane" : "mirror-lane", "gB0", "gB",
                                        profile, cursor, true});
        }

        store.work([&](const auto &s) -> QVariant {
            const StoredInstallationFacts install{{"install", "install-epoch"}, "install-rev"};
            const StoredAccountFacts op{{"install", "operator", "account-epoch", true},
                                        "account-rev"};
            SqliteAccountSessionUnit accounts(s, {{}});
            const auto staged = LifecycleWriteResult::staged;
            require(accounts.insert_installation(install) == staged &&
                        accounts.insert_account(op) == staged &&
                        accounts.insert_account(
                            {{"install", "owner", "owner-epoch", true}, "owner-rev"}) == staged,
                    "Protected producer staged the singleton, operator and distinct owner");
            // Public synthetic verifier from the frozen account/session fixture.
            const StoredLocalCredential local{
                "install",
                {"operator", "auth-rev",
                 "$argon2id$v=19$m=4882,t=2,p=1$bA81arsiXysd3WbTRzmEOw$"
                 "Nm8QBM+7RH1DXo9rvp5cwKEOOOfD2g6JuxlXihoNcpE"}};
            require(accounts.replace_local_credential({}, local, op, install) == staged,
                    "Protected account unit staged the synthetic credential");
            const AcceptedSessionAuthentication accepted{"install",
                                                         "operator",
                                                         "sign-in",
                                                         "install-epoch",
                                                         "account-epoch",
                                                         "account-rev",
                                                         "auth-rev",
                                                         "acceptance-rev",
                                                         AcceptedSignInMethod::local_password,
                                                         1000};
            const SessionRecordFacts record{"install",
                                            "operator",
                                            "session",
                                            "install-epoch",
                                            "account-epoch",
                                            "session-rev",
                                            session_lookup.digest,
                                            SessionRecordState::active,
                                            1000,
                                            1000,
                                            86401000};
            require(accounts.insert_session(record, SessionRetention::normal, accepted) == staged,
                    "Protected session unit staged the trusted synthetic sign-in acceptance");
            SqlitePermissionGraphUnit permissions(s, "install", authority.catalog);
            require(permissions.bootstrap("owner", graph(allow_b), graph(false, false)) ==
                        PermissionGraphWrite::staged,
                    "Protected permission unit stages current scoped grants and a distinct graph");
            return true;
        });
        store.work([&](const auto &s) -> QVariant {
            for (const auto action : {StartupAction::register_first, StartupAction::activate_first,
                                      StartupAction::checkpoint_first, StartupAction::inspect}) {
                auto decision = authority.resolve(s, {action, session_lookup, a, true});
                const auto *permit = std::get_if<StartupPermit>(&decision);
                require(permit && permit->principal == "operator" && permit->scope == a &&
                            permit->action == action && !permit->account_snapshot.isEmpty() &&
                            permit->permission_snapshot == "1",
                        "Current real session and permission evaluator establish each grant");
            }
            const auto exec = StartupExecutionAccount{"install", "operator"};
            require(std::holds_alternative<StartupPermit>(
                        authority.resolve(s, {StartupAction::checkpoint_first, exec, a, false})),
                    "Execution subject independently passes current account and scoped grant");
            const auto reg = std::get<StartupPermit>(
                authority.resolve(s, {StartupAction::register_first, session_lookup, a, true}));
            const auto act = std::get<StartupPermit>(
                authority.resolve(s, {StartupAction::activate_first, session_lookup, a, true}));
            require(reg.decision == "operator/D0" && act.decision == "operator/D1",
                    "Registration and activation decisions bind the resolved actor");
            const auto b_decision =
                authority.resolve(s, {StartupAction::register_first, session_lookup, b, true});
            require(allow_b ? std::holds_alternative<StartupPermit>(b_decision)
                            : std::holds_alternative<StartupDenied>(b_decision),
                    "B scope comes from the real scoped graph");
            // Use a valid single-lane proposal to isolate the missing policy check.
            const SourceChange unopened{a,
                                        {0, 0, 0, "g0"},
                                        "g2",
                                        policy("operator/D0"),
                                        registration().members,
                                        k10,
                                        "B0",
                                        act.decision,
                                        cursor,
                                        "cut10",
                                        SourcePurpose::baseline,
                                        0};
            require(stage_source_change(s, unopened) == HistoryStatus::stale,
                    "Protected intake stager refuses an authorized but unopened v20 store");
            return true;
        });
    }

    RegisterRequest register_request() const {
        RegisterRequest request{"install", config, {}, {}, {}, {}};
        for (const auto &lane : configuration.lanes) {
            request.cohort.append(lane.member.scope);
            request.members.append(lane.member);
            request.enrollment.append({lane.member.scope, lane.enrollment_generation});
        }
        return request;
    }

    ActivateRequest activate_request() const {
        ActivateRequest request{1, config, {}, {{a, pending, "g2", "B0", {}}}};
        if (two)
            request.sources.append({b, pending_b, "gB", "BB0", {}});
        return request;
    }

    static CheckpointRequest checkpoint_request(bool reconnect = false) {
        return {a,
                reconnect ? reconnected : active,
                config,
                "B0",
                reconnect ? QList<QByteArray>{"R0"} : QList<QByteArray>{},
                "K0",
                {}};
    }

    template <class Request> StartupOutcome submit(Request request, StartupFaults faults = {}) {
        StartupBinding binding = session_lookup;
        if constexpr (std::is_same_v<Request, CheckpointRequest>)
            binding = StartupExecutionAccount{"install", configuration.execution_principal};
        StartupOwner owner(store.writer(), configuration, registry, authority, std::move(binding));
        return wait([&](auto complete) {
            if constexpr (std::is_same_v<Request, RegisterRequest>)
                return register_first(owner, std::move(request), std::move(complete),
                                      std::move(faults));
            if constexpr (std::is_same_v<Request, ActivateRequest>)
                return activate_first(owner, std::move(request), std::move(complete),
                                      std::move(faults));
            if constexpr (std::is_same_v<Request, CheckpointRequest>)
                return checkpoint_first(owner, std::move(request), std::move(complete),
                                        std::move(faults));
        });
    }

    StartupOutcome inspect(StartupSelector selector) {
        StartupOwner owner(store.writer(), configuration, registry, authority, session_lookup);
        return wait([&](auto complete) {
            return inspect_startup(owner, std::move(selector), std::move(complete));
        });
    }

    void reopen() {
        store.reopen();
        registry.clear_invocations();
    }

    void remove_inspect_grant() {
        store.work([&](const auto &s) -> QVariant {
            SqlitePermissionGraphUnit permissions(s, "install", authority.catalog);
            require(permissions.replace_use(1, graph(two, false)) == PermissionGraphWrite::staged,
                    "Real permission writer revokes Inspect but retains mutating grants");
            return true;
        });
    }

    void stage_protected_reconnect() {
        store.work([&](const auto &s) -> QVariant {
            require(stage_reconnect(s, {a, active, "g3", cursor, "R0",
                                        authority.reconnect_decision(s), "connected"}) ==
                        HistoryStatus::staged,
                    "Protected reconnect advances generation at the frontierless active fence");
            return true;
        });
        registry.use_reconnect();
    }

  private:
    template <class Start> static StartupOutcome wait(Start start) {
        struct CompletionState {
            std::optional<StartupOutcome> outcome;
            QThread *thread{};
            int count{};
        };
        const auto state = std::make_shared<CompletionState>();
        const auto admission = start([state](StartupOutcome outcome) {
            ++state->count;
            state->thread = QThread::currentThread();
            state->outcome.emplace(std::move(outcome));
        });
        require(admission == StorageError::none && state->count == 0,
                "Queued startup admission is synchronous and completion is deferred");
        require(QTest::qWaitFor([state] { return state->outcome.has_value(); }, 10000) &&
                    state->count == 1 && state->thread == QThread::currentThread(),
                "Startup completes once on the authoritative Qt owner thread");
        return std::move(*state->outcome);
    }
};

} // namespace api_031_fixture
