// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sqlite_permission_graph_fixture.hpp"
#include "sqlite_reference_capture_fixture.hpp"
#include <QCryptographicHash>
#include <future>
#include <subathon/server/sqlite_reference_authority.hpp>

namespace reference_authority_fixture {
using namespace subathon::server;
using namespace subathon::core;
using permission_graph_fixture::blob;
using permission_graph_fixture::Cleanup;
using permission_graph_fixture::corrupt;
using permission_graph_fixture::encoded;
using permission_graph_fixture::Observer;
using permission_graph_fixture::require;
using permission_graph_fixture::run;
using permission_graph_fixture::scenario;
// Independent literal DDL; production manifest is never used for shared setup.
inline MigrationStep step10(const MigrationManifest &prefix) {
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
        "REFERENCES subathon_accounts(installation_ref,account_ref))"};
    s.expected_schema = prefix.steps.back().expected_schema;
    const QStringList names{"subathon_reference_authority_head",
                            "subathon_reference_authority_bindings",
                            "subathon_reference_authority_events"};
    for (qsizetype i = 0; i < names.size(); ++i) {
        s.expected_schema.append({"table", names[i], names[i], s.sql[i]});
        s.expected_schema.append(
            {"index", "sqlite_autoindex_" + names[i] + "_1", names[i], std::nullopt});
    }
    std::sort(s.expected_schema.begin(), s.expected_schema.end(), [](const auto &a, const auto &b) {
        return std::tie(a.type, a.name) < std::tie(b.type, b.name);
    });
    return s;
}
inline MigrationManifest literal_prefix() {
    auto p = account_sessions_fixture::Data{}.manifest();
    p.steps.append(permission_graph_fixture::step9(p));
    p.supported_starting_versions.append(9);
    return p;
}
inline ReferenceAuthorityBinding binding() {
    return {"I", "K", "U", "R", "r", "a", "b", "T", {"synthetic.reference.v1", "A"}, "N", 600000};
}
inline ReferenceAuthorityRequest request() {
    ReferenceAuthorityRequest r;
    r.principal = {ApplicationPrincipalKind::automation, "I", "U"};
    r.captured = {"Y", "X", "I",      {{"synthetic.reference.v1", "A"}, "N", "E"},
                  {7}, {0}, {"P", 1}, {"R", "r", "a", "b", "T", 600000, r.principal, "K"}};
    r.requested_operation = {"scope", "attempt"};
    return r;
}
inline std::string source() {
    // RSS1 plus THREE u32be lengths; independent literal, not production encoding.
    return QByteArray::fromHex("525353310000001673796e7468657469632e7265666572656e63652e7631"
                               "0000000141000000014e")
        .toStdString();
}
inline PermissionPolicyEntry entry(std::string op, std::int64_t bound = 600000) {
    auto e = permission_graph_fixture::allowance(op, op);
    e.resources[2].allowed = {source()};
    if (!e.quantities.empty())
        e.quantities[0].upper = PermissionRational{std::to_string(bound), "1"};
    return e;
}
inline std::vector<PermissionPolicyEntry> profile(std::int64_t bound = 600000) {
    return {entry("reference.time-add.v1", bound), entry("reference.application-read.v1"),
            entry("reference.hold-read.v1")};
}
inline PermissionEditableState graph(bool grant) {
    PermissionEditableState g{PermissionFactsState::complete, {}, {}};
    for (const auto *a : {"A", "E", "O"}) {
        auto entries = profile();
        if (!grant && std::string(a) == "A") {
            PermissionPolicyEntry m;
            m.source = "management";
            m.grant = "m";
            m.operation = "overrides.edit";
            m.resources = {{"account", PermissionResourceMode::allowed_set, {"E"}}};
            entries.push_back(m);
        }
        g.accounts.push_back({a, {}, std::move(entries)});
    }
    return g;
}
struct Clock final : CurrentSessionClock {
    std::int64_t now = 1000;
    bool unavailable = false;
    SessionClockResolution sample_current() override {
        return unavailable ? SessionClockResolution{SessionSourceUnavailable{}}
                           : SessionClockResolution{SessionClockSample{now}};
    }
};
inline SessionLookupRequest actor(char account = 'A') {
    AccountSessionDigest d{};
    d.fill(static_cast<std::uint8_t>(account));
    return {"I", d};
}
inline void seed_graph(const SqliteWorkSession &s) {
    (void)s.execute({"INSERT INTO subathon_permission_head VALUES(?,?,?,?)",
                     {QByteArray("I"), encoded(std::uint64_t{1}), QByteArray("O"),
                      encoded(permission_graph_fixture::trust())}});
    for (int c = 0; c != 2; ++c)
        for (const auto &a : graph(c == 1).accounts)
            (void)s.execute({"INSERT INTO subathon_permission_accounts VALUES(?,?,?,?,?)",
                             {QByteArray("I"), c, blob(a.account), encoded(a.assigned_roles),
                              encoded(a.individual_entries)}});
}
inline void seed_registry(const SqliteWorkSession &s, bool rows = true) {
    (void)s.execute(
        {"INSERT INTO subathon_reference_authority_head VALUES(X'49',X'0000000000000003')", {}});
    if (!rows)
        return;
    // All physical values literal. No new encoder, bootstrap/enroll/read/resolve.
    (void)s.execute({"INSERT INTO subathon_reference_authority_bindings VALUES("
                     "X'49',X'4b',X'55',X'52',X'72',X'61',X'62',X'54',"
                     "X'73796e7468657469632e7265666572656e63652e7631',X'41',X'4e',600000,X'41',X'"
                     "45',X'0000000000000002')",
                     {}});
    (void)s.execute({"INSERT INTO subathon_reference_authority_events VALUES("
                     "X'49',X'4b',X'0000000000000002',0,X'41',X'7341',X'76',1000)",
                     {}});
    (void)s.execute({"INSERT INTO subathon_reference_authority_events VALUES("
                     "X'49',X'4b',X'0000000000000003',1,X'4f',X'734f',X'76',1000)",
                     {}});
}
struct Database {
    QTemporaryDir directory;
    QString path{directory.filePath("authority.sqlite")};
    SqliteWriter writer;
    Clock clock;
    explicit Database(bool registry = true,
                      StorageLimits budget = account_sessions_fixture::limits()) {
        require(directory.isValid(), "Disposable authority database");
        require(writer.open(path, budget) == StorageError::none, "Real writer opens");
        auto m = permission_graph_fixture::migrate(writer, permission_graph_repository_manifest());
        require(m.reason == MigrationReason::none && m.report && m.report->version == 9 &&
                    m.work.transaction == TransactionOutcome::committed &&
                    m.work.error == WorkError::none,
                "Existing actual compiled prefix1-9");
        work([&](const auto &s) {
            for (const auto &sql : step10(literal_prefix()).sql)
                (void)s.execute({sql, {}});
            // Preserve SINGLETON installation. A foreign installation needs another DB.
            (void)s.execute(
                {"INSERT INTO subathon_account_installation VALUES(1,X'49',X'69',X'6a')", {}});
            for (const auto *a : {"A", "E", "O"}) {
                (void)s.execute({"INSERT INTO subathon_accounts VALUES(?,?,1,X'65',X'76')",
                                 {QByteArray("I"), QByteArray(a)}});
                (void)s.execute(
                    {"INSERT INTO subathon_account_sessions "
                     "VALUES(?,?,?,X'69',X'65',X'76',1,?,0,0,1000,1000,86401000)",
                     {QByteArray("I"), QByteArray(a), QByteArray("s") + a, QByteArray(32, a[0])}});
            }
            seed_graph(s);
            // Accepted old literal capture fixture: complete intake + immutable plan + SRX + index.
            const reference_capture_fixture::Data captures;
            captures.intake(s, "A");
            captures.stage_capture(s, "A");
            captures.index(s, "A");
            if (registry)
                seed_registry(s);
        });
    }
    template <class F> void work(F f) {
        (void)run(writer, [&](auto c) -> QVariant {
            const auto s = c.begin();
            Cleanup clean{s};
            f(s);
            s.commit();
            return 1;
        });
    }
    ReferenceAuthorityResolution resolve(ReferenceAuthorityRequest r = request(),
                                         ReferenceAuthorityLimits l = {}) {
        ReferenceAuthorityResolution result;
        work([&](const auto &s) {
            SqliteReferenceAuthorityUnit u(s, l);
            result = u.resolve(r);
        });
        return result;
    }
    void replace_graph(int channel, const PermissionEditableState &g) {
        work([&](const auto &s) {
            (void)s.execute(
                {"DELETE FROM subathon_permission_accounts WHERE channel=?", {channel}});
            (void)s.execute({"DELETE FROM subathon_permission_roles WHERE channel=?", {channel}});
            for (const auto &a : g.accounts)
                (void)s.execute({"INSERT INTO subathon_permission_accounts VALUES(?,?,?,?,?)",
                                 {QByteArray("I"), channel, blob(a.account),
                                  encoded(a.assigned_roles), encoded(a.individual_entries)}});
            for (const auto &r : g.roles)
                (void)s.execute({"INSERT INTO subathon_permission_roles VALUES(?,?,?,?)",
                                 {QByteArray("I"), channel, blob(r.role), encoded(r.entries)}});
            const auto rows = s.execute({"SELECT revision FROM subathon_permission_head", {}});
            const auto bytes = rows[0][0].toByteArray();
            std::uint64_t n = 0;
            for (const auto c : bytes)
                n = (n << 8) | static_cast<unsigned char>(c);
            (void)s.execute({"UPDATE subathon_permission_head SET revision=?", {encoded(n + 1)}});
        });
    }
};

inline ApplicationEnvelope historical_command() {
    // Independently framed SAC1: history/app, receipt7/CID0, P/1,
    // automation I/U, all six optional/mode bytes0, one literal SRF1(Y).
    return {"subathon.internal.application-command.v1",
            QByteArray::fromHex(
                "5341433100000007686973746f727900000003617070000000000000000007010000000000000000"
                "0000000150000000000000000101000000014900000001550000000000000000000100000023"
                "7375626174686f6e2e7265666572656e63652e657865637574696f6e2d7265662e7631"
                "00000009535246310000000159")};
}
inline void seed_history(Database &d, bool hold = false) {
    d.work([&](const auto &s) {
        ApplicationStateSnapshot before{
            "S0", {"opaque.state", QByteArray("", 0)},
            "C",  {TimerLifecycle::Active, CountdownPhase::Main, 1000000, 0, false},
            0,    {}};
        SqliteApplicationBundleStore bundles(s);
        require(bundles.retain_initial_state(before) == SqliteBundleInitialDisposition::inserted,
                "Old storage only: literal initial state");
        if (hold) {
            ApplicationHoldDetection h;
            h.cause.hold_ref = "hold";
            h.cause.source = {{ContributionId{0}}, {7}, "V", std::nullopt, "V", "C", "X"};
            h.cause.command = {{"history", "app"}, historical_command()};
            h.cause.first_observation.persisted_state_token = "S0";
            h.cause.first_observation.annotation = {"opaque.annotation", QByteArray("", 0)};
            h.cause.plan_evidence.plan = ApplicationPlanKey{"P", 1};
            h.cause.plan_evidence.occurrences = {{1, "a", "b", "R", "T",
                                                  RequestedUnit::milliseconds, Exact{600000},
                                                  600000, "reference.integer-ms.v1"}};
            h.cause.kind = ApplicationHoldCauseKind::unresolved_policy;
            h.cause.missing_policy_reason = "independent.fixture.hold";
            h.request = {"detection",
                         std::nullopt,
                         "version",
                         h.cause.first_observation,
                         {"opaque.audit", QByteArray("", 0)}};
            require(SqliteApplicationHoldStore(s).record_detection(h).disposition ==
                        SqliteHoldMutationDisposition::recorded,
                    "Old real hold storage, no new authority setup");
            return;
        }
        ContributionState contribution;
        contribution.payment_evidence = PaymentEvidence::not_applicable;
        contribution.conditions.valuation.monetary = false;
        contribution.conditions.review = ContributionReview::not_required;
        contribution.conditions.evidence_admission.policy_ref = "C";
        contribution.conditions.settlement = SettlementGate::clear;
        PreparedApplicationBundle b;
        b.intent = {{"history", "app"},
                    ApplicationOperationKind::original,
                    ApplicationSubjectKind::contribution,
                    {7},
                    ContributionId{0},
                    {"P", 1},
                    "S0",
                    contribution,
                    historical_command(),
                    std::nullopt};
        b.advanced_before = before;
        b.advanced_before.token = "S1";
        b.after = before;
        b.after.token = "S2";
        b.after.timer.main_remaining_ms = 1600000;
        b.advance_explanation = "independent.fixture.no.elapsed";
        ApplicationEffect e;
        e.request_ordinal = 1;
        e.arithmetic_role = ApplicationArithmeticRole::time_addition;
        e.exact_requested_units = Exact{600000};
        e.requested_units = 600000;
        e.application_explanation = "reference.integer-ms.v1";
        e.components = {{ApplicationComponentKind::main_delta, 600000, std::nullopt},
                        {ApplicationComponentKind::debt_delta, 0, std::nullopt},
                        {ApplicationComponentKind::cap_limited_time, 0, std::nullopt}};
        b.effects = {e};
        b.result = {"opaque.result", QByteArray("", 0)};
        b.audit.envelope = {"opaque.audit", QByteArray("", 0)};
        require(bundles.stage(b).disposition == SqliteBundleStageDisposition::staged,
                "Old real bundle storage: literal full requested600000 and actual600000");
    });
}
inline const ReferenceAuthorityFacts &facts(const ReferenceAuthorityResolution &r) {
    require(std::holds_alternative<ReferenceAuthorityFacts>(r.value),
            "Actual complete current facts, not NI/unknown/denial");
    return std::get<ReferenceAuthorityFacts>(r.value);
}
inline void failure(const ReferenceAuthorityResolution &r, ReferenceAuthorityFailure f) {
    require(std::holds_alternative<ReferenceAuthorityFailure>(r.value) &&
                std::get<ReferenceAuthorityFailure>(r.value) == f,
            "Exact refusal, never an empty or partial known policy");
}
inline void failed(const ReferenceAuthorityWrite &r, ReferenceAuthorityFailure f) {
    require(std::holds_alternative<ReferenceAuthorityFailure>(r) &&
                std::get<ReferenceAuthorityFailure>(r) == f,
            "Exact write refusal");
}
inline void staged(const ReferenceAuthorityWrite &r, std::uint64_t revision) {
    require(std::holds_alternative<ReferenceAuthorityStaged>(r) &&
                std::get<ReferenceAuthorityStaged>(r).revision == revision,
            "Exact provisional revision");
}
inline void permitted(const ReferenceAuthorityFacts &f) {
    const auto a = revalidate_permission_automation(f.original_author_input);
    require(a.author == "A" && a.actions.size() == 1 &&
                a.actions[0].use == PermissionOutcome::permit &&
                a.actions[0].grant == PermissionOutcome::permit &&
                a.actions[0].disposition == PermissionAutomationDisposition::authorized,
            "Original author whole-action dual current channels");
    require(evaluate_effective_permission(f.permission_input).data().outcome ==
                PermissionOutcome::permit,
            "Independent execution account calculation");
}
inline SqlRows registry_rows(Observer &o) {
    auto rows = o.rows("SELECT * FROM subathon_reference_authority_head ORDER BY 1");
    rows.append(o.rows("SELECT * FROM subathon_reference_authority_bindings ORDER BY 1,2"));
    rows.append(o.rows("SELECT * FROM subathon_reference_authority_events ORDER BY 1,3"));
    return rows;
}

// Independent semantic B/I counter: strings bytes; enums/bools1; u64/i64=8;
// optional1+payload; vectors sum bytes and size+recursive items; structs sum.
struct Size {
    std::uint64_t bytes = 0, items = 0;
};
inline Size operator+(Size a, Size b) { return {a.bytes + b.bytes, a.items + b.items}; }
inline Size size(const std::string &x) { return {x.size(), 0}; }
inline Size size(std::uint64_t) { return {8, 0}; }
inline Size size(std::int64_t) { return {8, 0}; }
inline Size size(bool) { return {1, 0}; }
template <class T>
    requires std::is_enum_v<T>
Size size(T) {
    return {1, 0};
}
template <class T> Size size(const std::optional<T> &);
template <class T> Size size(const std::vector<T> &);
inline Size size(const PermissionRational &x) { return size(x.numerator) + size(x.denominator); }
inline Size size(const PermissionResourceConstraint &x) {
    return size(x.slot) + size(x.mode) + size(x.allowed);
}
inline Size size(const PermissionQuantityConstraint &x) {
    return size(x.slot) + size(x.unit) + size(x.mode) + size(x.lower) + size(x.upper);
}
inline Size size(const PermissionPolicyEntry &x) {
    return size(x.origin) + size(x.source) + size(x.role) + size(x.operation) + size(x.effect) +
           size(x.grant) + size(x.resources) + size(x.quantities);
}
inline Size size(const PermissionQuantityDefinition &x) {
    return size(x.slot) + size(x.unit) + size(x.kind) + size(x.minimum) + size(x.maximum) +
           Size{1, 0};
}
inline Size size(const PermissionOperationDefinition &x) {
    return size(x.operation) + size(x.classification) + size(x.resource_slots) +
           size(x.quantity_slots) + size(x.helper_target_slot);
}
inline Size size(const PermissionCatalog &x) {
    return size(x.state) + size(x.revision) + size(x.operations);
}
inline Size size(const PermissionOwnerFacts &x) { return size(x.state) + size(x.account); }
inline Size size(const PermissionPolicyFacts &x) {
    return size(x.state) + size(x.assigned_roles) + size(x.entries);
}
inline Size size(const PermissionResourceBinding &x) { return size(x.slot) + size(x.resource); }
inline Size size(const PermissionQuantityBinding &x) {
    return size(x.slot) + size(x.unit) + size(x.value);
}
inline Size size(const PermissionRequest &x) {
    return size(x.operation) + size(x.resources) + size(x.quantities);
}
inline Size size(const DelegatedPermissionGrant &x) {
    return size(x.operation) + size(x.resources) + size(x.quantities);
}
inline Size size(const PermissionAutomationInput &x) {
    return size(x.author) + size(x.snapshot) + size(x.catalog) + size(x.owner) + size(x.use) +
           size(x.delegation) + size(x.actions);
}
inline Size size(const EffectivePermissionInput &x) {
    return size(x.actor) + size(x.snapshot) + size(x.catalog) + size(x.owner) + size(x.policy) +
           size(x.request);
}
inline Size size(const ReceiptOrigin &x) { return size(x.provider) + size(x.source_account); }
inline Size size(const ReceiptIdentity &x) {
    return size(x.origin) + size(x.identifier_namespace) + size(x.opaque_id);
}
inline Size size(const ApplicationPrincipalBinding &x) {
    return size(x.kind) + size(x.installation_ref) + size(x.principal_ref);
}
inline Size size(const ApplicationPlanKey &x) { return size(x.plan_ref) + size(x.revision); }
inline Size size(const ApplicationOperationKey &x) {
    return size(x.operation_scope) + size(x.operation_ref);
}
inline Size size(const ReferenceRule &x) {
    return size(x.rule_ref) + size(x.rule_version_ref) + size(x.action_ref) +
           size(x.action_version_ref) + size(x.timer_ref) + size(x.add_ms) + size(x.automation) +
           size(x.automation_capability_ref);
}
inline Size size(const ReferenceExecutionBinding &x) {
    return size(x.execution_context_ref) + size(x.arrival_context_ref) + size(x.installation_ref) +
           size(x.source) + size(x.receipt_id.value) + size(x.contribution_id.value) +
           size(x.plan) + size(x.rule);
}
inline Size size(const ReferenceAuthorityRequest &x) {
    return size(x.use) + size(x.principal) + size(x.captured) + size(x.requested_operation) +
           size(x.historical_operation) + size(x.historical_hold);
}
inline Size size(const ReferenceAuthorityBinding &x) {
    return size(x.installation) + size(x.capability) + size(x.principal) + size(x.rule) +
           size(x.rule_version) + size(x.action) + size(x.action_version) + size(x.timer) +
           size(x.origin) + size(x.identifier_namespace) + size(x.requested_ms);
}
inline Size size(const ReferenceAuthorityRecord &x) {
    return size(x.binding) + size(x.original_author) + size(x.execution_account) +
           size(x.created_revision) + size(x.state_revision) + size(x.head_revision) +
           size(x.state) + size(x.actor) + size(x.session) + size(x.session_revision) +
           size(x.authenticated_utc_ms);
}
inline Size size(const ReferenceAuthorityFacts &x) {
    return size(x.binding) + size(x.registry) + size(x.permission_revision_ref) +
           size(x.automation_revision_ref) + size(x.original_author_input) +
           size(x.permission_input);
}
template <class T> Size size(const std::optional<T> &x) {
    return Size{1, 0} + (x ? size(*x) : Size{});
}
template <class T> Size size(const std::vector<T> &x) {
    Size n{0, x.size()};
    for (const auto &v : x)
        n = n + size(v);
    return n;
}
inline std::string
automation_revision(std::uint64_t head = 3, std::uint64_t event = 3,
                    ReferenceAuthorityState state = ReferenceAuthorityState::authorized) {
    QByteArray b = QByteArray::fromHex("524141310000000149"); // RAA1+B(I)
    b += encoded(head);
    b += QByteArray::fromHex("000000014b0000000000000002"); // B(K),created2
    b += encoded(event);
    b += char(state);
    b += QByteArray::fromHex("00000001410000000145"); // immutable A,E
    return (QByteArray("RAA1") + QCryptographicHash::hash(b, QCryptographicHash::Sha256))
        .toStdString();
}
inline std::string permission_revision(std::uint64_t graph_head = 1, std::uint64_t head = 3) {
    QByteArray b = QByteArray::fromHex("524152310000000149"); // RAR1+B(I)
    b += encoded(graph_head);
    b += QByteArray::fromHex("000000177065726d697373696f6e2d7265666572656e63652d7631"
                             "000000014f0000000169000000016a00000003"
                             "00000001410000000165000000017601"
                             "00000001450000000165000000017601"
                             "000000014f0000000165000000017601");
    const auto a = automation_revision(head, head);
    // B uses u32be (not graph codec's u64 lengths).
    auto n = static_cast<std::uint32_t>(a.size());
    for (int shift = 24; shift >= 0; shift -= 8)
        b += char((n >> shift) & 255);
    b += blob(a);
    return (QByteArray("RAR1") + QCryptographicHash::hash(b, QCryptographicHash::Sha256))
        .toStdString();
}
} // namespace reference_authority_fixture
