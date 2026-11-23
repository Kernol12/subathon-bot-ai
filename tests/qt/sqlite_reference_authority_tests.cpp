// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/application_port_fake.hpp"
#include "support/sqlite_reference_authority_fixture.hpp"
#include "support/sqlite_selected_record_fixture.hpp"
#include <limits>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

using namespace reference_authority_fixture;
namespace {
using F = ReferenceAuthorityFailure;
using S = ReferenceAuthorityState;
template <class Body> void work_failure(Body body, WorkError expected) {
    bool caught = false;
    try {
        body();
    } catch (const SqliteWorkFailure &e) {
        caught = true;
        require(e.error() == expected, "Original writer error unchanged");
    }
    require(caught, "Actual writer failure, NI does not qualify");
}
template <class Body> void view_failure(Body body, ApplicationReferenceViewError expected) {
    bool caught = false;
    try {
        body();
    } catch (const ApplicationReferenceViewFailure &e) {
        caught = true;
        require(e.error() == expected, "Exact callback lifetime/thread refusal");
    }
    require(caught, "Callback refusal actually reached");
}
ReferenceAuthorityFacts expected() {
    ReferenceAuthorityFacts f;
    f.binding = request();
    f.registry = {binding(), "A", "E", 2, 3, 3, S::authorized, "O", "sO", "v", 1000};
    f.automation_revision_ref = automation_revision();
    f.permission_revision_ref = permission_revision();
    const auto catalog = permission_graph_fixture::trust().catalog;
    PermissionOwnerFacts owner{PermissionOwnerState::present, "O"};
    const auto use = graph(false), grant = graph(true);
    DelegatedPermissionGrant action;
    action.operation = "reference.time-add.v1";
    action.resources = {{"installation", PermissionResourceMode::allowed_set, {"I"}},
                        {"timer", PermissionResourceMode::allowed_set, {"T"}},
                        {"source", PermissionResourceMode::allowed_set, {source()}}};
    action.quantities = {{"adjustment", "ms", PermissionQuantityMode::closed_interval,
                          PermissionRational{"600000", "1"}, PermissionRational{"600000", "1"}}};
    f.original_author_input = {
        "A",
        f.permission_revision_ref,
        catalog,
        owner,
        {PermissionFactsState::complete, {}, use.accounts[0].individual_entries},
        {PermissionFactsState::complete, {}, grant.accounts[0].individual_entries},
        {action}};
    f.permission_input = {"E",
                          f.permission_revision_ref,
                          catalog,
                          owner,
                          {PermissionFactsState::complete, {}, use.accounts[1].individual_entries},
                          {"reference.time-add.v1",
                           {{"installation", "I"}, {"timer", "T"}, {"source", source()}},
                           {{"adjustment", "ms", {"600000", "1"}}}}};
    return f;
}
void exact(const ReferenceAuthorityFacts &f) {
    const auto e = expected();
    require(f.permission_revision_ref == e.permission_revision_ref &&
                f.automation_revision_ref == e.automation_revision_ref,
            "Literal complete graph/account/install/catalog plus registry revision framing");

    const auto &b = f.binding;
    require(b.use == ReferenceAuthorityUse::execute_time_add &&
                b.principal.kind == ApplicationPrincipalKind::automation &&
                b.principal.installation_ref == "I" && b.principal.principal_ref == "U" &&
                b.captured.execution_context_ref == "Y" && b.captured.arrival_context_ref == "X" &&
                b.captured.installation_ref == "I" &&
                b.captured.source == ReceiptIdentity{{"synthetic.reference.v1", "A"}, "N", "E"} &&
                b.captured.receipt_id.value == 7 && b.captured.contribution_id.value == 0 &&
                b.captured.plan.plan_ref == "P" && b.captured.plan.revision == 1 &&
                b.captured.rule.rule_ref == "R" && b.captured.rule.rule_version_ref == "r" &&
                b.captured.rule.action_ref == "a" && b.captured.rule.action_version_ref == "b" &&
                b.captured.rule.timer_ref == "T" && b.captured.rule.add_ms == 600000 &&
                b.captured.rule.automation.kind == ApplicationPrincipalKind::automation &&
                b.captured.rule.automation.installation_ref == "I" &&
                b.captured.rule.automation.principal_ref == "U" &&
                b.captured.rule.automation_capability_ref == "K" &&
                b.requested_operation.operation_scope == "scope" &&
                b.requested_operation.operation_ref == "attempt" && !b.historical_operation &&
                !b.historical_hold,
            "Every request/capture binding field echoed exactly, including real CID0 and receipt7");
    const auto &r = f.registry;
    require(r.original_author == "A" && r.execution_account == "E" && r.binding.principal == "U" &&
                r.created_revision == 2 && r.state_revision == 3 && r.head_revision == 3 &&
                r.state == S::authorized && r.actor == "O" && r.session == "sO" &&
                r.session_revision == "v" && r.authenticated_utc_ms == 1000,
            "Immutable authenticated original author differs from principal AND execution subject");
    const auto &a = f.original_author_input;
    require(a.author == "A" && a.snapshot == e.permission_revision_ref &&
                encoded(a.catalog) == encoded(e.original_author_input.catalog) &&
                a.owner.account == std::optional<std::string>{"O"} &&
                a.owner.state == PermissionOwnerState::present &&
                a.use.state == PermissionFactsState::complete &&
                a.delegation.state == PermissionFactsState::complete &&
                a.use.assigned_roles.empty() && a.delegation.assigned_roles.empty() &&
                encoded(a.use.entries) == encoded(e.original_author_input.use.entries) &&
                encoded(a.delegation.entries) ==
                    encoded(e.original_author_input.delegation.entries),
            "Separate unmodified current author policies");
    require(a.actions.size() == 1 && a.actions[0].operation == "reference.time-add.v1" &&
                encoded(a.actions[0].resources) ==
                    encoded(e.original_author_input.actions[0].resources) &&
                encoded(a.actions[0].quantities) ==
                    encoded(e.original_author_input.actions[0].quantities),
            "Full concrete singleton action, no cap clipping");
    const auto &p = f.permission_input;
    require(p.actor == "E" && p.snapshot == e.permission_revision_ref &&
                encoded(p.catalog) == encoded(e.permission_input.catalog) &&
                p.owner.account == a.owner.account &&
                p.policy.state == PermissionFactsState::complete &&
                p.policy.assigned_roles.empty() &&
                encoded(p.policy.entries) == encoded(e.permission_input.policy.entries) &&
                p.request.operation == "reference.time-add.v1" && p.request.resources.size() == 3 &&
                p.request.resources[0].resource == "I" && p.request.resources[1].resource == "T" &&
                p.request.resources[2].resource == source() && p.request.quantities.size() == 1 &&
                p.request.quantities[0].value.numerator == "600000" &&
                p.request.quantities[0].value.denominator == "1",
            "Actual execution subject has its own whole-request policy");
    permitted(f);
}
// This probe captures the real SAME-session authority read and stops accounting
// with a deliberate denial. Any later port call throws its own diagnostic.
class Probe final : public qt_test_support::ApplicationPortFake {
  public:
    ReferenceAuthorityRequest query{request()};
    ApplicationReferenceReadView retained;
    std::optional<ReferenceAuthorityResolution> result;
    std::exception_ptr error;
    std::function<void(const ApplicationReferenceReadView &)> inspect;
    std::function<void(const ApplicationReferenceReadView &)> before;
    std::optional<WorkError> expected_work_failure;
    ApplicationServiceLimits service_limits;
    unsigned checks = 0;
    AccessDecision check(const ApplicationReferenceReadView &v,
                         const OriginalApplicationCommand &) override {
        ++checks;
        retained = v;
        try {
            if (before) {
                before(v);
                return ApplicationAccessDenied{};
            }
            result = v.resolve_reference_authority(query, {});
            if (inspect)
                inspect(v);
        } catch (...) {
            error = std::current_exception();
            throw;
        }
        return ApplicationAccessDenied{};
    }
    bool disclose_application(const ApplicationReferenceReadView &,
                              const OriginalApplicationCommand &,
                              const RecordedApplicationBundle &) override {
        throw std::logic_error("Unexpected protected lookup");
    }
    bool disclose_holds(const ApplicationReferenceReadView &, const OriginalApplicationCommand &,
                        const ApplicationHoldActiveSet &) override {
        throw std::logic_error("Unexpected hold lookup");
    }
    PolicyResolution resolve(const ApplicationReferenceReadView &,
                             const OriginalApplicationCommand &, const ApplicationBundleInputs &,
                             const AccessDecision &) override {
        throw std::logic_error("Unexpected policy");
    }
    ClockResolution observe(const ApplicationReferenceReadView &,
                            const OriginalApplicationCommand &, const ApplicationBundleInputs &,
                            const BoundApplicationPolicy &) override {
        throw std::logic_error("Unexpected clock");
    }
    ActionResolution prepare(const ApplicationReferenceReadView &,
                             const OriginalApplicationCommand &, const ApplicationBundleInputs &,
                             const BoundApplicationPolicy &,
                             const ApplicationCalculatedProposal &) override {
        throw std::logic_error("Unexpected actions");
    }
    ReferenceResolution issue(ApplicationReferencePurpose,
                              const ApplicationReferenceBinding &) override {
        throw std::logic_error("Unexpected entropy");
    }
    void attempt(Database &d, SqliteWriter *override_writer = nullptr) {
        error = {};
        result.reset();
        auto service_ports = ports();
        OriginalApplicationService service(override_writer ? *override_writer : d.writer,
                                           service_ports, service_limits);
        OriginalApplicationCommand c;
        c.operation = query.requested_operation;
        c.receipt_id = {7};
        c.contribution_id = ContributionId{0};
        c.plan = {"P", 1};
        c.principal = {ApplicationPrincipalKind::automation, "I", "U"};
        bool done = false;
        ApplicationServiceCompletion completion;
        require(service.enqueue(c,
                                [&](auto v) {
                                    completion = std::move(v);
                                    done = true;
                                })
                        .error == StorageError::none,
                "Real service admits callback probe");
        require(QTest::qWaitFor([&] { return done; }, 10000), "Real service completes");
        if (error)
            std::rethrow_exception(error);
        if (expected_work_failure) {
            require(completion.transaction == TransactionOutcome::rolled_back &&
                        completion.work_error == *expected_work_failure && !completion.response,
                    "Original live-callback writer failure retained through actual completion");
            return;
        }
        require(result.has_value() && completion.transaction == TransactionOutcome::committed &&
                    completion.work_error == WorkError::none && completion.response &&
                    completion.response->disposition == ApplicationServiceDisposition::denied,
                "Real SAME-session callback resolved; deliberate denial publishes no accounting "
                "payload");
    }
};
} // namespace
class SqliteReferenceAuthorityTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() { qt_test_support::fail_on_sql_warnings(); }
    void init() { qt_test_support::fail_on_sql_warnings(); }
    void step10PreservesActualCompiledAndIndependentLiteralPrefix() {
        scenario([] {
            const auto p = permission_graph_repository_manifest(), literal = literal_prefix();
            const auto m = reference_authority_repository_manifest();
            require(m.catalog_format == p.catalog_format && m.store_identity == p.store_identity &&
                        m.steps.size() == 10 &&
                        m.supported_starting_versions ==
                            QList<qint32>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
                    "Exactly additive reserved step10");
            require(p.steps.size() == 9 && literal.steps.size() == 9, "Actual complete prefix1-9");
            for (qsizetype i = 0; i < 9; ++i) {
                const auto &a = m.steps[i], &b = p.steps[i], &c = literal.steps[i];
                require(a.version == b.version && a.id == b.id && a.sql == b.sql &&
                            a.expected_schema == b.expected_schema && a.version == c.version &&
                            a.id == c.id && a.sql == c.sql &&
                            a.expected_schema == c.expected_schema &&
                            canonical_migration_payload(a) == canonical_migration_payload(c),
                        "Every shipped byte and schema projection frozen independently");
            }
            const auto s = step10(literal);
            require(m.steps[9].version == 10 && m.steps[9].id == s.id && m.steps[9].sql == s.sql &&
                        m.steps[9].expected_schema == s.expected_schema,
                    "Exact independent reserved schema");
        });
    }
    void explicitUpgradeKeepsPopulatedNineAndNewTablesEmpty() {
        scenario([] {
            QTemporaryDir dir;
            SqliteWriter w;
            require(dir.isValid(), "Temporary upgrade");
            const auto path = dir.filePath("upgrade.sqlite");
            require(w.open(path, account_sessions_fixture::limits()) == StorageError::none,
                    "Real writer");
            auto r = permission_graph_fixture::migrate(w, permission_graph_repository_manifest());
            require(r.reason == MigrationReason::none &&
                        r.work.transaction == TransactionOutcome::committed,
                    "Actual nine migrated");
            (void)run(w, [](auto c) -> QVariant {
                auto s = c.begin();
                (void)s.execute(
                    {"INSERT INTO subathon_account_installation VALUES(1,X'49',X'69',X'6a')", {}});
                s.commit();
                return 1;
            });
            r = permission_graph_fixture::migrate(w, reference_authority_repository_manifest());
            require(r.reason == MigrationReason::none && r.report && r.report->version == 10 &&
                        r.report->applied == 1 &&
                        r.work.transaction == TransactionOutcome::committed &&
                        r.work.error == WorkError::none,
                    "One explicit acknowledged migration");
            Observer o(path);
            require(o.rows("SELECT * FROM subathon_account_installation") ==
                            SqlRows{{1, QByteArray("I"), QByteArray("i"), QByteArray("j")}} &&
                        registry_rows(o).isEmpty(),
                    "Upgrade preserves singleton; NEVER bootstraps authorization");
            account_sessions_fixture::healthy(o);
        });
    }

    void independentlyFramedRevisionPreimagesMatchLiteralSha256Oracles() {
        scenario([] {
            require(
                blob(automation_revision()) ==
                    QByteArray::fromHex(
                        "524141318bbc38d2cff636f77c4acfa4fd8624f9af6383edad0ffe9efdba3f05a2795eaa"),
                "Literal RAA1 digest from independent Python standard SHA256 reproduction");
            require(
                blob(permission_revision()) ==
                    QByteArray::fromHex(
                        "524152315be6a4fcf06d56d6bbea169f8f8f28388f9fffb3a9be782c6932135911136438"),
                "Literal RAR1 complete graph/account/install and automation preimage");
            require(
                blob(automation_revision(4, 3)) ==
                    QByteArray::fromHex(
                        "5241413198ef827cd1e3afb0328a33188c1347fc82560c04b29881e6d9d4f9bf9cb917eb"),
                "Independent literal automation fingerprint at head4/event3");
            require(
                blob(permission_revision(1, 4, 3)) ==
                    QByteArray::fromHex(
                        "524152319992cb2f50e5bb50ca58e180b5bc20018637428658b092df03a89d157efe6d28"),
                "Independent literal permission fingerprint at head4/event3");
            require(
                blob(automation_revision(4, 4)) ==
                    QByteArray::fromHex(
                        "5241413108afbcd6689fcb39093f8d6c1949cab1992b633d4ac70ac29d87f5711ded3d3a"),
                "Independent literal automation fingerprint at head4/event4");
            require(
                blob(permission_revision(1, 4, 4)) ==
                    QByteArray::fromHex(
                        "5241523174940df17a268017f0fc6235e36c1022356ed23ae2111ba50aee8674ee1bb108"),
                "Independent literal permission fingerprint at head4/event4");
            require(
                blob(automation_revision(5, 5)) ==
                    QByteArray::fromHex(
                        "524141316df9d25cbfa69c568cbfa575c835d78d5730906210e5f5e64b626baf7e4c84dd"),
                "Independent literal automation fingerprint at head5/event5");
            require(
                blob(permission_revision(1, 5, 5)) ==
                    QByteArray::fromHex(
                        "524152317d29109d1bd190e0355df41f2815c53d15891793e29542b518850449842e36ec"),
                "Independent literal permission fingerprint at head5/event5");
            Database d;
            exact(facts(d.resolve()));
        });
    }
    void actualScopeAndOriginalAuthorAreNotAliasesOrRetainedAuthorship() {
        scenario([] {
            Database d;
            const auto r = d.resolve();
            exact(facts(r));
            Observer o(d.path);
            account_sessions_fixture::healthy(o);
        });
    }
    void authenticatedOwnerBootstrapThenAuthorEnrollmentThenExplicitOwnerAuthorization() {
        scenario([] {
            Database d(false);
            d.work([&](const auto &s) {
                SqliteReferenceAuthorityUnit u(s);
                failed(u.bootstrap(actor(), d.clock), F::denied);
                staged(u.bootstrap(actor('O'), d.clock), 1);
                failed(u.bootstrap(actor('O'), d.clock), F::collision);
                staged(u.enroll(actor(), d.clock, 1, binding(), "E"), 2);
                failure(u.resolve(request()), F::denied); // pending is no authority
                failed(u.set_state(actor(), d.clock, 2, "K", S::authorized), F::denied);
                staged(u.set_state(actor('O'), d.clock, 2, "K", S::authorized), 3);
                exact(facts(u.resolve(request())));
            });
            Observer o(d.path);
            require(
                o.rows("SELECT * FROM subathon_reference_authority_events ORDER BY revision") ==
                    SqlRows{{QByteArray("I"), QByteArray("K"), encoded(std::uint64_t{2}), 0,
                             QByteArray("A"), QByteArray("sA"), QByteArray("v"), qlonglong{1000}},
                            {QByteArray("I"), QByteArray("K"), encoded(std::uint64_t{3}), 1,
                             QByteArray("O"), QByteArray("sO"), QByteArray("v"), qlonglong{1000}}},
                "Literal durable authentication provenance for BOTH lifecycle events");
        });
    }
    void authenticationFailures_data() {
        QTest::addColumn<QString>("sql");
        QTest::addColumn<qlonglong>("now");
        QTest::addColumn<bool>("unavailable");
        QTest::newRow("revoked") << "UPDATE subathon_account_sessions SET state=1"
                                 << qlonglong{1000} << false;
        QTest::newRow("expired") << "" << qlonglong{86401000} << false;
        QTest::newRow("not-fresh") << "" << qlonglong{301001} << false;
        QTest::newRow("disabled") << "UPDATE subathon_accounts SET application_access_enabled=0"
                                  << qlonglong{1000} << false;
        QTest::newRow("install-epoch")
            << "UPDATE subathon_account_installation SET session_epoch_ref=X'78'" << qlonglong{1000}
            << false;
        QTest::newRow("account-epoch")
            << "UPDATE subathon_accounts SET session_epoch_ref=X'78'" << qlonglong{1000} << false;
        QTest::newRow("missing-session")
            << "DELETE FROM subathon_account_sessions" << qlonglong{1000} << false;
        QTest::newRow("clock-unavailable") << "" << qlonglong{1000} << true;
    }
    void authenticationFailures() {
        QFETCH(QString, sql);
        QFETCH(qlonglong, now);
        QFETCH(bool, unavailable);
        scenario([&] {
            Database d(false);
            d.clock.now = now;
            d.clock.unavailable = unavailable;
            if (!sql.isEmpty())
                corrupt(d.path, {sql});
            d.work([&](const auto &s) {
                SqliteReferenceAuthorityUnit u(s);
                failed(u.bootstrap(actor('O'), d.clock), unavailable ? F::unavailable : F::denied);
            });
            Observer o(d.path);
            require(registry_rows(o).isEmpty(), "No unauthorized bootstrap writes");
        });
    }
    void noHeadWrongInstallationOrMissingParentsCannotBootstrapOrEnroll() {
        scenario([] {
            Database d(false);
            d.work([&](const auto &s) {
                SqliteReferenceAuthorityUnit u(s);
                failed(u.enroll(actor(), d.clock, 1, binding(), "E"), F::missing);
                auto foreign = actor('O');
                foreign.installation_ref = "foreign";
                failed(u.bootstrap(foreign, d.clock), F::denied);
                staged(u.bootstrap(actor('O'), d.clock), 1);
                auto b = binding();
                b.installation = "foreign";
                failed(u.enroll(actor(), d.clock, 1, b, "E"), F::denied);
                failed(u.enroll(actor(), d.clock, 1, binding(), "missing"), F::missing);
            });
            Observer o(d.path);
            require(o.rows("SELECT count(*) FROM subathon_account_installation")[0][0].toInt() == 1,
                    "No second installation invented");
            require(o.rows("SELECT * FROM subathon_reference_authority_bindings").isEmpty(),
                    "Missing parent never synthesized");
        });
    }
    void enrollmentRequiresManagementAndWholeAuthorUseAndGrant_data() {
        QTest::addColumn<int>("channel");
        QTest::addColumn<int>("account_index");
        QTest::addColumn<int>("entry_index");
        QTest::newRow("management") << 0 << 0 << 3;
        for (int i = 0; i < 3; ++i) {
            QTest::newRow(qPrintable(QString("author-use-%1").arg(i))) << 0 << 0 << i;
            QTest::newRow(qPrintable(QString("author-grant-%1").arg(i))) << 1 << 0 << i;
            QTest::newRow(qPrintable(QString("execution-use-%1").arg(i))) << 0 << 1 << i;
        }
    }
    void enrollmentRequiresManagementAndWholeAuthorUseAndGrant() {
        QFETCH(int, channel);
        QFETCH(int, account_index);
        QFETCH(int, entry_index);
        scenario([&] {
            Database d(false);
            auto g = graph(channel == 1);
            auto &entries = g.accounts[account_index].individual_entries;
            entries.erase(entries.begin() + entry_index);
            d.replace_graph(channel, g);
            d.work([&](const auto &session) {
                SqliteReferenceAuthorityUnit u(session);
                staged(u.bootstrap(actor('O'), d.clock), 1);
            });
            Observer o(d.path);
            const auto before = registry_rows(o);
            d.work([&](const auto &session) {
                SqliteReferenceAuthorityUnit u(session);
                failed(u.enroll(actor(), d.clock, 1, binding(), "E"), F::denied);
            });
            require(registry_rows(o) == before, "Every capability required; no enrollment writes");
        });
    }
    void authorizationRevalidatesEveryAuthorAndExecutionCapability_data() {
        QTest::addColumn<int>("channel");
        QTest::addColumn<int>("account_index");
        QTest::addColumn<int>("entry_index");
        for (int i = 0; i < 3; ++i) {
            QTest::newRow(qPrintable(QString("author-use-%1").arg(i))) << 0 << 0 << i;
            QTest::newRow(qPrintable(QString("author-grant-%1").arg(i))) << 1 << 0 << i;
            QTest::newRow(qPrintable(QString("execution-use-%1").arg(i))) << 0 << 1 << i;
        }
    }
    void authorizationRevalidatesEveryAuthorAndExecutionCapability() {
        QFETCH(int, channel);
        QFETCH(int, account_index);
        QFETCH(int, entry_index);
        scenario([&] {
            Database d;
            // Independently return the registry to its original pending enrollment.
            d.work([](const auto &session) {
                (void)session.execute({"DELETE FROM subathon_reference_authority_events WHERE "
                                       "revision=X'0000000000000003'",
                                       {}});
                (void)session.execute({"UPDATE subathon_reference_authority_head SET "
                                       "revision=X'0000000000000002'",
                                       {}});
            });
            auto g = graph(channel == 1);
            auto &entries = g.accounts[account_index].individual_entries;
            entries.erase(entries.begin() + entry_index);
            d.replace_graph(channel, g);
            Observer o(d.path);
            const auto before = registry_rows(o);
            d.work([&](const auto &session) {
                SqliteReferenceAuthorityUnit u(session);
                failed(u.set_state(actor('O'), d.clock, 2, "K", S::authorized), F::denied);
            });
            require(registry_rows(o) == before, "Authorization rechecks all three; zero writes");
        });
    }
    void ownerCannotRevokeWithoutDelegationEvenForRestrictiveChange_data() {
        QTest::addColumn<int>("entry_index");
        QTest::addColumn<bool>("revoke");
        for (int i = 0; i < 3; ++i) {
            QTest::newRow(qPrintable(QString("authorize-missing-%1").arg(i))) << i << false;
            QTest::newRow(qPrintable(QString("revoke-missing-%1").arg(i))) << i << true;
        }
    }
    void ownerCannotRevokeWithoutDelegationEvenForRestrictiveChange() {
        QFETCH(int, entry_index);
        QFETCH(bool, revoke);
        scenario([&] {
            Database d;
            auto g = graph(true);
            auto &entries = g.accounts[2].individual_entries;
            entries.erase(entries.begin() + entry_index);
            d.replace_graph(1, g);
            Observer o(d.path);
            const auto before = registry_rows(o);
            d.work([&](const auto &session) {
                SqliteReferenceAuthorityUnit u(session);
                failed(
                    u.set_state(actor('O'), d.clock, 3, "K", revoke ? S::revoked : S::authorized),
                    F::denied);
            });
            require(registry_rows(o) == before,
                    "Even one missing capability blocks authorization or restrictive revocation");
        });
    }
    void revokeReauthorizeAndCollisionPreserveImmutableHistoricalIdentity() {
        scenario([] {
            Database d;
            Observer o(d.path);
            const auto old = o.rows("SELECT * FROM subathon_reference_authority_bindings");
            const auto initial = o.rows("SELECT * FROM subathon_reference_authority_events "
                                        "WHERE revision=X'0000000000000002'");
            const auto first = facts(d.resolve());
            d.work([&](const auto &s) {
                SqliteReferenceAuthorityUnit u(s);
                staged(u.set_state(actor('O'), d.clock, 3, "K", S::revoked), 4);
                failure(u.resolve(request()), F::denied);
                auto b = binding();
                b.timer = "other";
                failed(u.enroll(actor('O'), d.clock, 4, b, "A"), F::collision);
                staged(u.set_state(actor('O'), d.clock, 4, "K", S::authorized), 5);
                const auto r = u.read("I", "K");
                require(std::holds_alternative<ReferenceAuthorityRecord>(r),
                        "Historical record retained");
                const auto &v = std::get<ReferenceAuthorityRecord>(r);
                require(v.original_author == "A" && v.execution_account == "E" &&
                            v.created_revision == 2 && v.state_revision == 5 &&
                            v.head_revision == 5,
                        "Owner reauthorization cannot become original author");
                const auto resolved = u.resolve(request());
                const auto &next = facts(resolved);
                permitted(next);
                require(next.automation_revision_ref == automation_revision(5, 5) &&
                            next.permission_revision_ref == permission_revision(1, 5, 5) &&
                            next.automation_revision_ref != first.automation_revision_ref &&
                            next.permission_revision_ref != first.permission_revision_ref &&
                            next.original_author_input.snapshot == next.permission_revision_ref &&
                            next.permission_input.snapshot == next.permission_revision_ref,
                        "Reauthorization fences include registry head and latest event, not policy "
                        "alone");
            });
            require(o.rows("SELECT * FROM subathon_reference_authority_bindings") == old &&
                        o.rows("SELECT * FROM subathon_reference_authority_events "
                               "WHERE revision=X'0000000000000002'") == initial &&
                        o.rows("SELECT * FROM subathon_reference_authority_events").size() == 4,
                    "No overwritten original binding/events through lifecycle");
        });
    }
    void unrelatedCapabilityEnrollmentChangesOnlyGlobalFingerprintFence() {
        scenario([] {
            Database d;
            const auto first = facts(d.resolve());
            Observer o(d.path);
            const auto original_events = o.rows("SELECT * FROM subathon_reference_authority_events "
                                                "WHERE capability_ref=X'4b' ORDER BY revision");
            d.work([&](const auto &session) {
                SqliteReferenceAuthorityUnit u(session);
                auto other = binding();
                other.capability = "L";
                staged(u.enroll(actor(), d.clock, 3, other, "E"), 4);
                const auto resolved = u.resolve(request());
                const auto &next = facts(resolved);
                permitted(next);
                require(next.registry.head_revision == 4 && next.registry.state_revision == 3 &&
                            next.automation_revision_ref == automation_revision(4, 3) &&
                            next.permission_revision_ref == permission_revision(1, 4, 3) &&
                            next.automation_revision_ref != first.automation_revision_ref &&
                            next.permission_revision_ref != first.permission_revision_ref,
                        "Unrelated enrollment changes global fence without changing K's event");
            });
            require(o.rows("SELECT * FROM subathon_reference_authority_events "
                           "WHERE capability_ref=X'4b' ORDER BY revision") == original_events,
                    "Original capability history is byte-unchanged");
        });
    }
    void lifecycleInputMissingAndNoOpOutcomesAreExplicit() {
        scenario([] {
            Database d;
            Observer o(d.path);
            const auto before = registry_rows(o);
            d.work([&](const auto &session) {
                SqliteReferenceAuthorityUnit u(session);
                bool caught = false;
                try {
                    (void)u.set_state(actor('O'), d.clock, 3, "K", S::pending);
                } catch (const ReferenceAuthorityInputFailure &e) {
                    caught = e.code() == F::invalid_data;
                }
                require(caught, "Pending is not a transition target");
                failed(u.set_state(actor('O'), d.clock, 3, "absent", S::authorized), F::missing);
                auto unsupported = binding();
                unsupported.capability = "L";
                unsupported.origin.provider = "unsupported.provider";
                failed(u.enroll(actor(), d.clock, 3, unsupported, "E"), F::unavailable);
            });
            require(registry_rows(o) == before, "Invalid, absent and unsupported have zero writes");
            d.work([&](const auto &session) {
                SqliteReferenceAuthorityUnit u(session);
                staged(u.set_state(actor('O'), d.clock, 3, "K", S::authorized), 4);
                const auto resolved = u.resolve(request());
                const auto &next = facts(resolved);
                permitted(next);
                require(next.registry.head_revision == 4 && next.registry.state_revision == 4 &&
                            next.automation_revision_ref == automation_revision(4, 4) &&
                            next.permission_revision_ref == permission_revision(1, 4, 4),
                        "No-op authorization still appends an event and invalidates old results");
            });
        });
    }
    void bootstrapRequiresActualCurrentPermissionRoot_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("absent-complete-graph") << 0;
        QTest::newRow("orphaned-graph-head") << 1;
        QTest::newRow("corrupt-catalog") << 2;
        QTest::newRow("unsupported-catalog") << 3;
    }
    void bootstrapRequiresActualCurrentPermissionRoot() {
        QFETCH(int, kind);
        scenario([&] {
            Database d(false);
            if (kind == 0) {
                d.work([](const auto &session) {
                    for (const auto *sql : {"DELETE FROM subathon_permission_accounts",
                                            "DELETE FROM subathon_permission_roles",
                                            "DELETE FROM subathon_permission_head"})
                        (void)session.execute({sql, {}});
                });
            } else if (kind == 1)
                corrupt(d.path, {"DELETE FROM subathon_permission_head"});
            else if (kind == 2)
                corrupt(d.path, {"UPDATE subathon_permission_head SET catalog=X'00'"});
            else
                corrupt(d.path, {"UPDATE subathon_permission_head SET catalog=CAST(replace(catalog,"
                                 "X'7065726d697373696f6e2d7265666572656e63652d7631',"
                                 "X'7065726d697373696f6e2d7265666572656e63652d7632') AS BLOB)"});
            Observer o(d.path);
            const auto before = registry_rows(o);
            d.work([&](const auto &session) {
                SqliteReferenceAuthorityUnit u(session);
                failed(u.bootstrap(actor('O'), d.clock), kind == 0 ? F::missing : F::invalid_data);
            });
            require(registry_rows(o) == before,
                    "Bootstrap never repairs or invents root authority");
        });
    }
    void staleCASHasZeroWritesAndRollbackCannotPublishProvisionalResults() {
        scenario([] {
            Database d;
            Observer o(d.path);
            const auto before = registry_rows(o);
            d.work([&](const auto &s) {
                SqliteReferenceAuthorityUnit u(s);
                failed(u.set_state(actor('O'), d.clock, 2, "K", S::revoked), F::stale);
            });
            require(registry_rows(o) == before, "Stale is read-only");
            bool staged_before = false;
            const auto result =
                account_sessions_fixture::outcome(d.writer, [&](auto c) -> QVariant {
                    auto s = c.begin();
                    Cleanup clean{s};
                    SqliteReferenceAuthorityUnit u(s);
                    staged(u.set_state(actor('O'), d.clock, 3, "K", S::revoked), 4);
                    staged_before = true;
                    failure(u.resolve(request()), F::denied);
                    s.rollback();
                    work_failure([&] { (void)u.read("I", "K"); }, WorkError::session_closed);
                    return 99;
                });
            require(staged_before && result.transaction == TransactionOutcome::rolled_back &&
                        !result.value && registry_rows(o) == before,
                    "Provisional revision never means committed revoke");
            exact(facts(d.resolve()));
        });
    }
    void currentUseGrantExecutionNarrowingAndMembership_data() {
        QTest::addColumn<int>("channel");
        QTest::addColumn<int>("account");
        QTest::addColumn<bool>("role");
        QTest::newRow("author-use") << 0 << 0 << false;
        QTest::newRow("author-grant") << 1 << 0 << false;
        QTest::newRow("execution-use") << 0 << 1 << false;
        QTest::newRow("author-membership") << 0 << 0 << true;
        QTest::newRow("grant-membership") << 1 << 0 << true;
    }
    void currentUseGrantExecutionNarrowingAndMembership() {
        QFETCH(int, channel);
        QFETCH(int, account);
        QFETCH(bool, role);
        scenario([&] {
            Database d;
            exact(facts(d.resolve()));
            auto g = graph(channel == 1);
            if (role) {
                auto entries = g.accounts[std::size_t(account)].individual_entries;
                for (auto &e : entries) {
                    e.origin = PermissionOrigin::role;
                    e.role = "role";
                }
                g.roles = {{"role", entries}};
                g.accounts[std::size_t(account)].individual_entries.clear();
                g.accounts[std::size_t(account)].assigned_roles = {"role"};
                d.replace_graph(channel, g);
                permitted(facts(d.resolve()));
                g.accounts[std::size_t(account)].assigned_roles.clear();
            } else
                g.accounts[std::size_t(account)].individual_entries[0].quantities[0].upper =
                    PermissionRational{"120000", "1"};
            d.replace_graph(channel, g);
            failure(d.resolve(), F::denied); // request600000, even if timer cap admits only120000/0
        });
    }
    void restorationChangesCombinedRevisionAndNeverReusesPriorPermit() {
        scenario([] {
            Database d;
            const auto first = facts(d.resolve());
            auto g = graph(false);
            g.accounts[0].individual_entries[0].effect = PermissionEffect::deny;
            g.accounts[0].individual_entries[0].grant.reset();
            d.replace_graph(0, g);
            failure(d.resolve(), F::denied);
            d.replace_graph(0, graph(false));
            const auto next = facts(d.resolve());
            permitted(next);
            require(next.permission_revision_ref != first.permission_revision_ref &&
                        next.permission_revision_ref == permission_revision(3),
                    "Current graph revision after restore, not cached first permit");
            require(next.automation_revision_ref == first.automation_revision_ref,
                    "Independent automation revision remains exact");
        });
    }
    void currentFences_data() {
        QTest::addColumn<QString>("sql");
        QTest::addColumn<int>("kind");
        QTest::newRow("author-disabled")
            << "UPDATE subathon_accounts SET application_access_enabled=0 WHERE account_ref=X'41'"
            << 0;
        QTest::newRow("execution-disabled")
            << "UPDATE subathon_accounts SET application_access_enabled=0 WHERE account_ref=X'45'"
            << 0;
        QTest::newRow("owner-missing")
            << "DELETE FROM subathon_accounts WHERE account_ref=X'4f'" << 1;
        QTest::newRow("install-missing") << "DELETE FROM subathon_account_installation" << 1;
        QTest::newRow("owner-unknown")
            << "UPDATE subathon_permission_head SET owner_ref=X'3f'" << 1;
        QTest::newRow("catalog-corrupt")
            << "UPDATE subathon_permission_head SET catalog=X'00'" << 1;
        QTest::newRow("graph-head-orphaned") << "DELETE FROM subathon_permission_head" << 1;
        QTest::newRow("catalog-stale")
            << "UPDATE subathon_permission_head SET "
               "catalog=CAST(replace(catalog,X'7065726d697373696f6e2d7265666572656e63652d7631',X'"
               "7065726d697373696f6e2d7265666572656e63652d7632') AS BLOB)"
            << 1;
        QTest::newRow("capability-revoked") << "UPDATE subathon_reference_authority_events SET "
                                               "state=2 WHERE revision=X'0000000000000003'"
                                            << 0;
        QTest::newRow("registry-missing")
            << "DELETE FROM subathon_reference_authority_bindings" << 2;
        QTest::newRow("initial-event-missing")
            << "DELETE FROM subathon_reference_authority_events WHERE state=0" << 1;
        QTest::newRow("latest-state-corrupt") << "UPDATE subathon_reference_authority_events SET "
                                                 "state=9 WHERE revision=X'0000000000000003'"
                                              << 1;
        QTest::newRow("unknown-session-authorship")
            << "UPDATE subathon_reference_authority_events SET actor_ref=X'4f' WHERE state=0" << 1;
        QTest::newRow("event-ahead-of-head")
            << "UPDATE subathon_reference_authority_head SET revision=X'0000000000000002'" << 1;
        QTest::newRow("unavailable-table") << "DROP TABLE subathon_reference_authority_events" << 3;
    }
    void currentFences() {
        QFETCH(QString, sql);
        QFETCH(int, kind);
        scenario([&] {
            Database d;
            exact(facts(d.resolve()));
            corrupt(d.path, {sql});
            if (kind == 3) {
                bool caught = false;
                const auto r = account_sessions_fixture::outcome(d.writer, [&](auto c) -> QVariant {
                    auto s = c.begin();
                    Cleanup clean{s};
                    SqliteReferenceAuthorityUnit u(s);
                    try {
                        (void)u.resolve(request());
                    } catch (const SqliteWorkFailure &e) {
                        caught = e.error() == WorkError::sql_failure;
                        throw;
                    }
                    s.commit();
                    return 1;
                });
                require(caught && r.error == WorkError::sql_failure &&
                            r.transaction == TransactionOutcome::rolled_back && !r.value,
                        "SQL failure is original writer truth, not missing/empty");
            } else
                failure(d.resolve(), kind == 0   ? F::denied
                                     : kind == 1 ? F::invalid_data
                                                 : F::missing);
        });
    }
    void accountInstallOwnerCatalogAndCapabilityChangesInvalidateSnapshot_data() {
        QTest::addColumn<QString>("sql");
        QTest::newRow("account-revision")
            << "UPDATE subathon_accounts SET revision_ref=X'77' WHERE account_ref=X'41'";
        QTest::newRow("execution-epoch")
            << "UPDATE subathon_accounts SET session_epoch_ref=X'66' WHERE account_ref=X'45'";
        QTest::newRow("installation-revision")
            << "UPDATE subathon_account_installation SET revision_ref=X'6b'";
        QTest::newRow("installation-epoch")
            << "UPDATE subathon_account_installation SET session_epoch_ref=X'70'";
        QTest::newRow("owner-transfer")
            << "UPDATE subathon_permission_head SET owner_ref=X'41',revision=X'0000000000000002'";
        QTest::newRow("graph-head")
            << "UPDATE subathon_permission_head SET revision=X'0000000000000002'";
    }
    void accountInstallOwnerCatalogAndCapabilityChangesInvalidateSnapshot() {
        QFETCH(QString, sql);
        scenario([&] {
            Database d;
            const auto first = facts(d.resolve());
            corrupt(d.path, {sql});
            const auto second = facts(d.resolve());
            permitted(second);
            require(second.permission_revision_ref != first.permission_revision_ref &&
                        second.automation_revision_ref == first.automation_revision_ref,
                    "Current fences included even without registry mutation");
        });
    }
    void everyCapturedBindingMismatchRefuses_data() {
        QTest::addColumn<int>("field");
        QTest::addColumn<int>("expected");
        QTest::addColumn<bool>("input_failure");
        const char *names[] = {"principal-installation",
                               "principal",
                               "capture-installation",
                               "rule-principal",
                               "capability",
                               "rule",
                               "rule-version",
                               "action",
                               "action-version",
                               "timer",
                               "provider",
                               "source-account",
                               "namespace",
                               "opaque-event",
                               "execution-ref",
                               "arrival-ref",
                               "plan",
                               "amount",
                               "plan-revision",
                               "receipt",
                               "contribution-zero",
                               "principal-kind",
                               "rule-automation-kind",
                               "rule-installation"};
        for (int i = 0; i < 24; ++i)
            QTest::newRow(names[i]) << i << int(i == 4 || i == 13 ? F::missing : F::invalid_data)
                                    << (i == 21 || i == 22);
    }
    void everyCapturedBindingMismatchRefuses() {
        QFETCH(int, field);
        QFETCH(int, expected);
        QFETCH(bool, input_failure);
        scenario([&] {
            Database d;
            auto r = request();
            std::string *fields[] = {&r.principal.installation_ref,
                                     &r.principal.principal_ref,
                                     &r.captured.installation_ref,
                                     &r.captured.rule.automation.principal_ref,
                                     &r.captured.rule.automation_capability_ref,
                                     &r.captured.rule.rule_ref,
                                     &r.captured.rule.rule_version_ref,
                                     &r.captured.rule.action_ref,
                                     &r.captured.rule.action_version_ref,
                                     &r.captured.rule.timer_ref,
                                     &r.captured.source.origin.provider,
                                     &r.captured.source.origin.source_account,
                                     &r.captured.source.identifier_namespace,
                                     &r.captured.source.opaque_id,
                                     &r.captured.execution_context_ref,
                                     &r.captured.arrival_context_ref,
                                     &r.captured.plan.plan_ref};
            if (field < 17)
                *fields[field] += "wrong";
            else if (field == 17)
                ++r.captured.rule.add_ms;
            else if (field == 18)
                ++r.captured.plan.revision;
            else if (field == 19)
                ++r.captured.receipt_id.value;
            else if (field == 20)
                ++r.captured.contribution_id.value; // Actual present CID0 must not alias1.
            else if (field == 21)
                r.principal.kind = ApplicationPrincipalKind::account;
            else if (field == 22)
                r.captured.rule.automation.kind = ApplicationPrincipalKind::account;
            else
                r.captured.rule.automation.installation_ref += "wrong";
            Observer o(d.path);
            const auto before = registry_rows(o);
            if (input_failure) {
                d.work([&](const auto &session) {
                    SqliteReferenceAuthorityUnit u(session);
                    bool caught = false;
                    try {
                        (void)u.resolve(r);
                    } catch (const ReferenceAuthorityInputFailure &e) {
                        caught = e.code() == static_cast<F>(expected);
                    }
                    require(caught, "Non-automation inputs are exact shape failures");
                });
            } else
                failure(d.resolve(r), static_cast<F>(expected));
            require(registry_rows(o) == before, "Mismatched binding has zero writes");
        });
    }
    void retainedDocumentCannotSupplyMissingOriginalAuthorship() {
        scenario([] {
            Database d(false);
            failure(d.resolve(), F::missing);
            Observer o(d.path);
            require(
                o.rows("SELECT * FROM subathon_retained_contexts WHERE reference=X'59'").size() ==
                    1,
                "Historical SRX exists but is not authentication");
        });
    }
    void selectedRegistryBudgetsAreExactAndIndependentOfHistoryLength() {
        scenario([] {
            Database d;
            ReferenceAuthorityLimits l;
            l.registry_selected_bytes = 105;
            exact(facts(d.resolve(request(), l)));
            --l.registry_selected_bytes;
            failure(d.resolve(request(), l), F::resource_limit);
            l = {};
            l.reference_bytes = 39;
            l.permissions.reference_bytes = 39; // RSS1 =16+22+1+1 =40
            failure(d.resolve(request(), l), F::resource_limit);
            l.reference_bytes = 40;
            l.permissions.reference_bytes = 40;
            permitted(facts(d.resolve(request(), l)));
        });
    }
    void wholeInputAndOutputExactFitAndOneLess() {
        scenario([] {
            Database d;
            const auto e = expected();
            const auto in = size(request()), out = size(e);
            require(in.bytes == 88 && in.items == 0 && size(e.registry).bytes == 79 &&
                        out.bytes == 2440 && out.items == 131,
                    "Independent literal B/I: request88/0, record79/0, whole facts2440/131");
            ReferenceAuthorityLimits l;
            l.input_bytes = in.bytes;
            l.result_bytes = out.bytes;
            l.result_items = out.items;
            exact(facts(d.resolve(request(), l)));
            --l.input_bytes;
            Observer o(d.path);
            const auto before = registry_rows(o);
            d.work([&](const auto &session) {
                SqliteReferenceAuthorityUnit u(session, l);
                bool caught = false;
                try {
                    (void)u.resolve(request());
                } catch (const ReferenceAuthorityInputFailure &x) {
                    caught = x.code() == F::resource_limit;
                }
                require(caught, "Full input refused before SQL/copy");
            });
            require(registry_rows(o) == before, "Input refusal leaves the registry unchanged");
            l.input_bytes = in.bytes;
            --l.result_bytes;
            failure(d.resolve(request(), l), F::resource_limit);
            l.result_bytes = out.bytes;
            --l.result_items;
            failure(d.resolve(request(), l), F::resource_limit);
        });
    }
    void mutationBudgetExactFitAndRefuseWithoutWrites() {
        scenario([] {
            Database d;
            Observer o(d.path);
            const auto before = registry_rows(o);
            d.work([&](const auto &s) {
                ReferenceAuthorityLimits l;
                l.mutation_bytes = 31;
                SqliteReferenceAuthorityUnit u(s, l);
                failed(u.set_state(actor('O'), d.clock, 3, "K", S::revoked), F::resource_limit);
            });
            require(registry_rows(o) == before, "Whole mutation projected before any writes");
            d.work([&](const auto &s) {
                ReferenceAuthorityLimits l;
                l.mutation_bytes = 32;
                SqliteReferenceAuthorityUnit u(s, l);
                staged(u.set_state(actor('O'), d.clock, 3, "K", S::revoked), 4);
            });
            failure(d.resolve(), F::denied);
        });
    }
    void defaultExpiredForeignAndTerminalWriterTicketsPreserveTruth() {
        scenario([] {
            SqliteWorkSession empty;
            work_failure(
                [&] {
                    SqliteReferenceAuthorityUnit u(empty);
                    (void)u.resolve(request());
                },
                WorkError::stale_session);
            Database d;
            SqliteWorkSession retained;
            d.work([&](const auto &s) {
                retained = s;
                SqliteReferenceAuthorityUnit u(s);
                auto foreign = std::async(std::launch::async, [&] {
                    work_failure([&] { (void)u.resolve(request()); }, WorkError::wrong_thread);
                });
                foreign.get();
                exact(facts(u.resolve(request())));
            });
            work_failure(
                [&] {
                    SqliteReferenceAuthorityUnit u(retained);
                    (void)u.read("I", "K");
                },
                WorkError::stale_session);
            bool terminal = false;
            const auto result =
                account_sessions_fixture::outcome(d.writer, [&](auto c) -> QVariant {
                    auto s = c.begin();
                    SqliteReferenceAuthorityUnit u(s);
                    s.commit();
                    work_failure([&] { (void)u.resolve(request()); }, WorkError::session_closed);
                    terminal = true;
                    return 1;
                });
            require(terminal && result.transaction == TransactionOutcome::committed &&
                        result.error == WorkError::session_closed && !result.value,
                    "Post-commit protocol failure cannot publish a normal value");
        });
    }
    void realCumulativeSqlFailureRollsBackAndLaterCallsAreClosed() {
        scenario([] {
            Database d;
            Observer o(d.path);
            const auto before = registry_rows(o);
            bool reached = false;
            const auto r = account_sessions_fixture::outcome(d.writer, [&](auto c) -> QVariant {
                const auto s = c.begin();
                Cleanup cleanup{s};
                SqliteReferenceAuthorityUnit u(s);
                staged(u.set_state(actor('O'), d.clock, 3, "K", S::revoked), 4);
                try {
                    (void)s.execute({"SELECT * FROM intentionally_missing_access011", {}});
                } catch (const SqliteWorkFailure &e) {
                    require(e.error() == WorkError::sql_failure, "Original SQL failure");
                    reached = true;
                }
                work_failure([&] { (void)u.read("I", "K"); }, WorkError::session_closed);
                return 1;
            });
            require(reached && r.error == WorkError::sql_failure &&
                        r.transaction == TransactionOutcome::rolled_back && !r.value &&
                        registry_rows(o) == before,
                    "Later closed failure does not replace original completion error or publish "
                    "staged revoke");
        });
    }
    void realCallbackUsesCurrentRecordsAndRetainedCopiesExpire() {
        scenario([] {
            Database d;
            Probe p;
            p.inspect = [](const auto &v) {
                auto copy = v;
                exact(facts(copy.resolve_reference_authority(request(), {})));
                auto other = std::async(std::launch::async, [copy] {
                    view_failure([&] { (void)copy.resolve_reference_authority(request(), {}); },
                                 ApplicationReferenceViewError::wrong_thread);
                });
                other.get();
                exact(facts(v.resolve_reference_authority(request(), {})));
            };
            p.attempt(d);
            exact(facts(*p.result));
            auto copy = p.retained;
            view_failure([&] { (void)copy.resolve_reference_authority(request(), {}); },
                         ApplicationReferenceViewError::expired_callback);
            auto g = graph(true);
            g.accounts[0].individual_entries.clear();
            d.replace_graph(1, g);
            p.inspect = {};
            p.attempt(d);
            failure(*p.result, F::denied);
            require(p.checks == 2, "Second actual callback did not reuse first permit");
        });
    }
    void defaultCallbackAndDestroyedServiceCannotAcquireSession() {
        scenario([] {
            ApplicationReferenceReadView v;
            view_failure([&] { (void)v.resolve_reference_authority(request(), {}); },
                         ApplicationReferenceViewError::expired_callback);
            Database d;
            Probe p;
            p.attempt(d); // attempt's service has already been destroyed
            view_failure([&] { (void)p.retained.resolve_reference_authority(request(), {}); },
                         ApplicationReferenceViewError::expired_callback);
        });
    }

    void actualHistoricalApplicationAndHoldDisclosureAreSeparateAndFresh_data() {
        QTest::addColumn<bool>("hold");
        QTest::addColumn<bool>("execution");
        QTest::newRow("application-author-grant") << false << false;
        QTest::newRow("hold-author-grant") << true << false;
        QTest::newRow("application-execution-use") << false << true;
        QTest::newRow("hold-execution-use") << true << true;
    }
    void actualHistoricalApplicationAndHoldDisclosureAreSeparateAndFresh() {
        QFETCH(bool, hold);
        QFETCH(bool, execution);
        scenario([&] {
            Database d;
            seed_history(d, hold);
            auto r = request();
            r.use = hold ? ReferenceAuthorityUse::disclose_holds
                         : ReferenceAuthorityUse::disclose_application;
            r.historical_operation = ApplicationOperationKey{"history", "app"};
            if (hold)
                r.historical_hold = "hold";
            const auto first = d.resolve(r);
            permitted(facts(first));
            const auto &p = facts(first).permission_input;
            require(
                p.request.operation ==
                        (hold ? "reference.hold-read.v1" : "reference.application-read.v1") &&
                    p.request.quantities.empty() &&
                    facts(first).original_author_input.actions.size() == 1 &&
                    facts(first).original_author_input.actions[0].quantities.empty(),
                "Actual immutable historical record uses independent quantity-free read action");
            // Execute still fits. Each read requires both author GRANT and subject USE.
            auto g = graph(!execution);
            auto &entries = g.accounts[execution ? 1 : 0].individual_entries;
            entries.erase(entries.begin() + (hold ? 2 : 1));
            d.replace_graph(execution ? 0 : 1, g);
            permitted(facts(d.resolve()));
            failure(d.resolve(r), F::denied);
            // Actual service callback resolves CURRENT registry and stored historical
            // records, rather than accepting a fake source's historical output.
            Probe p2;
            p2.query = r;
            p2.attempt(d);
            failure(*p2.result, F::denied);
        });
    }
    void historicalSourceMustMatchActualStoredRecordNotNewCandidate() {
        scenario([] {
            Database d;
            seed_history(d);
            auto r = request();
            r.use = ReferenceAuthorityUse::disclose_application;
            r.historical_operation = ApplicationOperationKey{"history", "app"};
            permitted(facts(d.resolve(r)));
            // New-key candidate cannot substitute a different plan/source/target.
            r.captured.rule.timer_ref = "other";
            failure(d.resolve(r), F::invalid_data);
            r = request();
            r.use = ReferenceAuthorityUse::disclose_application;
            r.historical_operation = ApplicationOperationKey{"history", "app"};
            corrupt(d.path, {"UPDATE subathon_app_operations SET choices_document=X'00'"});
            failure(d.resolve(r), F::invalid_data);
        });
    }
    void cumulativeWriterBudgetsPropagateOriginalFailureThenSessionClosed_data() {
        QTest::addColumn<bool>("rows");
        QTest::newRow("row-budget") << true;
        QTest::newRow("statement-budget") << false;
    }
    void cumulativeWriterBudgetsPropagateOriginalFailureThenSessionClosed() {
        QFETCH(bool, rows);
        scenario([&] {
            Database d;
            Observer o(d.path);
            const auto before = registry_rows(o);
            bool stopped = false;
            require(d.writer.shutdown([&] { stopped = true; }) == StorageError::none,
                    "Close setup writer");
            require(QTest::qWaitFor([&] { return stopped; }, 10000),
                    "Setup writer actually closed");
            auto budget = account_sessions_fixture::limits();
            if (rows)
                budget.result_rows = 1;
            else
                budget.statements_per_batch = 1;
            SqliteWriter tiny;
            require(tiny.open(d.path, budget) == StorageError::none,
                    "Same database, constrained real writer");
            bool reached = false;
            const auto result = account_sessions_fixture::outcome(tiny, [&](auto c) -> QVariant {
                auto s = c.begin();
                Cleanup clean{s};
                SqliteReferenceAuthorityUnit u(s);
                work_failure([&] { (void)u.resolve(request()); },
                             rows ? WorkError::result_limit : WorkError::resource_limit);
                work_failure([&] { (void)u.read("I", "K"); }, WorkError::session_closed);
                reached = true;
                return 1;
            });
            require(
                reached &&
                    result.error == (rows ? WorkError::result_limit : WorkError::resource_limit) &&
                    result.transaction == TransactionOutcome::rolled_back && !result.value &&
                    registry_rows(o) == before,
                "Adapter preserves cumulative ticket failure; completion retains the FIRST error");
        });
    }
    void commitAcknowledgementLossCannotTurnStagedAuthorizationIntoSuccess() {
        scenario([] {
            Database d;
            bool reached = false;
            require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        d.writer, testing::WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none,
                    "Inject ack loss at actual COMMIT");
            const auto result =
                account_sessions_fixture::outcome(d.writer, [&](auto c) -> QVariant {
                    auto s = c.begin();
                    Cleanup clean{s};
                    SqliteReferenceAuthorityUnit u(s);
                    staged(u.set_state(actor('O'), d.clock, 3, "K", S::revoked), 4);
                    reached = true;
                    s.commit();
                    return 1;
                });
            require(reached && result.transaction == TransactionOutcome::outcome_unknown &&
                        result.error == WorkError::uncertain_outcome && !result.value,
                    "Actual commit ack fault, never a normal committed producer response");
        });
    }

    void enumAndLiteralPhysicalRowsRemainExact() {
        static_assert(static_cast<unsigned>(ReferenceAuthorityUse::execute_time_add) == 0);
        static_assert(static_cast<unsigned>(ReferenceAuthorityUse::disclose_application) == 1);
        static_assert(static_cast<unsigned>(ReferenceAuthorityUse::disclose_holds) == 2);
        static_assert(static_cast<unsigned>(S::pending) == 0 &&
                      static_cast<unsigned>(S::authorized) == 1 &&
                      static_cast<unsigned>(S::revoked) == 2);
        static_assert(
            static_cast<unsigned>(F::denied) == 0 && static_cast<unsigned>(F::missing) == 1 &&
            static_cast<unsigned>(F::invalid_data) == 2 &&
            static_cast<unsigned>(F::unavailable) == 3 &&
            static_cast<unsigned>(F::resource_limit) == 4 && static_cast<unsigned>(F::stale) == 5 &&
            static_cast<unsigned>(F::collision) == 6);
        scenario([] {
            Database d;
            d.work([](const auto &session) {
                SqliteReferenceAuthorityUnit u(session);
                const auto r = u.read("I", "K");
                require(std::holds_alternative<ReferenceAuthorityRecord>(r),
                        "Actual registry decode");
                const auto &v = std::get<ReferenceAuthorityRecord>(r);
                require(v.binding.installation == "I" && v.binding.capability == "K" &&
                            v.binding.principal == "U" && v.binding.rule == "R" &&
                            v.binding.rule_version == "r" && v.binding.action == "a" &&
                            v.binding.action_version == "b" && v.binding.timer == "T" &&
                            v.binding.origin == ReceiptOrigin{"synthetic.reference.v1", "A"} &&
                            v.binding.identifier_namespace == "N" &&
                            v.binding.requested_ms == 600000 && v.original_author == "A" &&
                            v.execution_account == "E",
                        "Every literal column decoded without alias or loss");
            });
            Observer o(d.path);
            require(o.rows("SELECT typeof(revision),hex(revision) FROM "
                           "subathon_reference_authority_head") ==
                        SqlRows{{QString("blob"), QString("0000000000000003")}},
                    "Frozen unsigned big-endian head");
            require(o.rows("SELECT "
                           "created_revision,state,actor_ref,session_ref,session_revision_ref,"
                           "authenticated_utc_ms FROM subathon_reference_authority_bindings JOIN "
                           "subathon_reference_authority_events "
                           "USING(installation_ref,capability_ref) ORDER BY revision") ==
                        SqlRows{{encoded(std::uint64_t{2}), 0, QByteArray("A"), QByteArray("sA"),
                                 QByteArray("v"), qlonglong{1000}},
                                {encoded(std::uint64_t{2}), 1, QByteArray("O"), QByteArray("sO"),
                                 QByteArray("v"), qlonglong{1000}}},
                    "Literal event codec ordering and immutable initial provenance");
        });
    }
    void suppliedResolvedSessionCannotSurviveSameScopeRevocation() {
        scenario([] {
            Database d;
            d.work([&](const auto &s) {
                SqliteAccountSessionUnit real(s, {{}});
                const auto previous =
                    resolve_current_account_session(actor('O'), true, real, d.clock);
                require(std::holds_alternative<ResolvedAccountSession>(previous),
                        "Actual current session initially resolves");
                (void)s.execute({"UPDATE subathon_account_sessions SET "
                                 "state=1,record_revision_ref=X'77' WHERE account_ref=X'4f'",
                                 {}});
                SqliteReferenceAuthorityUnit u(s);
                failed(u.set_state(actor('O'), d.clock, 3, "K", S::revoked), F::denied);
                // previous remains an owned DTO, deliberately has no registry overload.
                const auto &dto = std::get<ResolvedAccountSession>(previous);
                require(dto.account_ref == "O" && dto.record_revision_ref == "v",
                        "Old resolved metadata confers no current mutation authority");
            });
            exact(
                facts(d.resolve())); // Session revocation does not rewrite prior author/capability.
        });
    }
    void revisionExhaustionAndCorruptIntegerEncodingCannotWrapOrPermit_data() {
        QTest::addColumn<QString>("sql");
        QTest::addColumn<bool>("max_head");
        QTest::newRow("u64max")
            << "UPDATE subathon_reference_authority_head SET revision=X'ffffffffffffffff'" << true;
        QTest::newRow("zero-head")
            << "UPDATE subathon_reference_authority_head SET revision=X'0000000000000000'" << false;
        QTest::newRow("text-head")
            << "UPDATE subathon_reference_authority_head SET revision='00000003'" << false;
        QTest::newRow("short-head")
            << "UPDATE subathon_reference_authority_head SET revision=X'0003'" << false;
        QTest::newRow("negative-amount")
            << "UPDATE subathon_reference_authority_bindings SET requested_ms=-1" << false;
    }
    void revisionExhaustionAndCorruptIntegerEncodingCannotWrapOrPermit() {
        QFETCH(QString, sql);
        QFETCH(bool, max_head);
        scenario([&] {
            Database d;
            corrupt(d.path, {sql});
            Observer o(d.path);
            const auto before = registry_rows(o);
            if (max_head) {
                d.work([&](const auto &s) {
                    SqliteReferenceAuthorityUnit u(s);
                    failed(u.set_state(actor('O'), d.clock,
                                       std::numeric_limits<std::uint64_t>::max(), "K", S::revoked),
                           F::resource_limit);
                });
            } else
                failure(d.resolve(), F::invalid_data);
            require(registry_rows(o) == before,
                    "Exhaustion/corrupt values never normalize, wrap or write");
        });
    }
    void independentZeroAndInt64MaxRequestedQuantity_data() {
        QTest::addColumn<qlonglong>("amount");
        QTest::newRow("zero") << qlonglong{0};
        QTest::newRow("signed64-max") << std::numeric_limits<qlonglong>::max();
    }
    void independentZeroAndInt64MaxRequestedQuantity() {
        QFETCH(qlonglong, amount);
        scenario([&] {
            Database d;
            d.work([&](const auto &s) {
                // Independent fixed-format literal SRX/SRC/SRA bytes, replacing ONLY
                // the single i64 amount field. No production capture/authority codec.
                const auto old = QByteArray::fromHex("00000000000927c0");
                const auto replacement = encoded(static_cast<std::uint64_t>(amount));
                auto rows = s.execute(
                    {"SELECT reference,document FROM subathon_retained_configurations", {}});
                for (const auto &row : rows) {
                    auto b = row[1].toByteArray();
                    require(b.count(old) == 1, "Exactly one configured amount");
                    b.replace(old, replacement);
                    (void)s.execute(
                        {"UPDATE subathon_retained_configurations SET document=? WHERE reference=?",
                         {b, row[0]}});
                }
                rows = s.execute({"SELECT reference,document FROM subathon_retained_contexts", {}});
                for (const auto &row : rows) {
                    auto b = row[1].toByteArray();
                    require(b.count(old) == 1, "Exactly one arrival/execution amount");
                    b.replace(old, replacement);
                    (void)s.execute(
                        {"UPDATE subathon_retained_contexts SET document=? WHERE reference=?",
                         {b, row[0]}});
                }
                (void)s.execute(
                    {"UPDATE subathon_plan_requests SET requested_units=?,exact_numerator=?",
                     {amount, blob(std::to_string(amount))}});
                (void)s.execute(
                    {"UPDATE subathon_reference_authority_bindings SET requested_ms=?", {amount}});
            });
            auto use = graph(false), grant = graph(true);
            for (auto *g : {&use, &grant})
                for (auto &a : g->accounts)
                    a.individual_entries[0].quantities[0].upper =
                        PermissionRational{std::to_string(amount), "1"};
            d.replace_graph(0, use);
            d.replace_graph(1, grant);
            auto r = request();
            r.captured.rule.add_ms = amount;
            const auto out = d.resolve(r);
            permitted(facts(out));
            require(facts(out).permission_input.request.quantities[0].value.numerator ==
                        std::to_string(amount),
                    "Exact nonnegative signed64 amount, never floating point or actual cap credit");
            if (amount > 0) {
                grant.accounts[0].individual_entries[0].quantities[0].upper =
                    PermissionRational{std::to_string(amount - 1), "1"};
                d.replace_graph(1, grant);
                failure(d.resolve(r), F::denied);
            }
        });
    }
    void missingSparsePolicyIsKnownEmptyButMissingAccountIsCorruption() {
        scenario([] {
            Database d;
            d.work([](const auto &s) {
                (void)s.execute({"DELETE FROM subathon_permission_accounts WHERE account_ref=X'45' "
                                 "AND channel=0",
                                 {}});
            });
            failure(d.resolve(), F::denied);
            corrupt(d.path, {"DELETE FROM subathon_accounts WHERE account_ref=X'45'"});
            failure(d.resolve(), F::invalid_data);
        });
    }
    void finiteSelectedReadIgnoresUnselectedIntermediateHistory() {
        scenario([] {
            Database d;
            Observer o(d.path);
            const auto before = o.rows("SELECT * FROM subathon_reference_authority_bindings");
            d.work([&](const auto &s) {
                SqliteReferenceAuthorityUnit u(s);
                staged(u.set_state(actor('O'), d.clock, 3, "K", S::revoked), 4);
                staged(u.set_state(actor('O'), d.clock, 4, "K", S::authorized), 5);
            });
            // Selected latest5 and initial2 remain intact. Selected read need not
            // materialize/audit intermediate3/4, regardless of retained history.
            corrupt(d.path, {"UPDATE subathon_reference_authority_events SET session_ref=X'' WHERE "
                             "revision=X'0000000000000004'"});
            ReferenceAuthorityLimits l;
            l.registry_selected_bytes = 105;
            permitted(facts(d.resolve(request(), l)));
            require(o.rows("SELECT * FROM subathon_reference_authority_bindings") == before,
                    "Selected read has no history repair side effects");
        });
    }
    void finiteSourceDelegatesToActualLiveCallbackView() {
        scenario([] {
            Database d;
            Probe p;
            p.inspect = [](const auto &v) {
                SqliteReferenceAuthoritySource source;
                exact(facts(source.resolve(v, request())));
            };
            p.attempt(d);
            SqliteReferenceAuthoritySource source;
            view_failure([&] { (void)source.resolve(p.retained, request()); },
                         ApplicationReferenceViewError::expired_callback);
        });
    }

    void bootstrapAndEnrollmentMutationAndInputBudgetsAreLiteral() {
        scenario([] {
            Database d(false);
            d.work([&](const auto &s) {
                ReferenceAuthorityLimits l;
                l.mutation_bytes = 8;
                SqliteReferenceAuthorityUnit small(s, l);
                failed(small.bootstrap(actor('O'), d.clock), F::resource_limit);
                l.mutation_bytes = 9;
                SqliteReferenceAuthorityUnit fit(s, l);
                staged(fit.bootstrap(actor('O'), d.clock), 1);
                l.mutation_bytes = 81;
                SqliteReferenceAuthorityUnit short_enroll(s, l);
                failed(short_enroll.enroll(actor(), d.clock, 1, binding(), "E"), F::resource_limit);
                require(size(binding()).bytes == 40,
                        "Literal enrollment input =33 actor+8 CAS+40 binding+1 execution =82");
                l.mutation_bytes = 82;
                l.input_bytes = 81;
                SqliteReferenceAuthorityUnit input_short(s, l);
                bool caught = false;
                try {
                    (void)input_short.enroll(actor(), d.clock, 1, binding(), "E");
                } catch (const ReferenceAuthorityInputFailure &e) {
                    caught = e.code() == F::resource_limit;
                }
                require(caught,
                        "Whole mutation arguments preflight before authentication/SQL/copy");
                l.input_bytes = 82;
                SqliteReferenceAuthorityUnit exact_enroll(s, l);
                staged(exact_enroll.enroll(actor(), d.clock, 1, binding(), "E"), 2);
            });
            Observer o(d.path);
            require(o.rows("SELECT count(*) FROM subathon_reference_authority_bindings")[0][0]
                            .toInt() == 1,
                    "Exactly one bounded enrollment, no partial attempts");
        });
    }
    void malformedLimitsAndInputsRefuseBeforeTouchingDefaultTicket_data() {
        QTest::addColumn<int>("kind");
        for (int i = 0; i < 6; ++i)
            QTest::newRow(qPrintable(QString::number(i))) << i;
    }
    void malformedLimitsAndInputsRefuseBeforeTouchingDefaultTicket() {
        QFETCH(int, kind);
        scenario([&] {
            ReferenceAuthorityLimits l;
            auto r = request();
            SqliteWorkSession empty;
            if (kind == 0)
                l.input_bytes = 0;
            else if (kind == 1)
                l.result_items = std::numeric_limits<std::uint64_t>::max();
            else if (kind == 2)
                r.principal.principal_ref.clear();
            else if (kind == 3)
                r.use = static_cast<ReferenceAuthorityUse>(255);
            else if (kind == 4)
                r.captured.rule.add_ms = -1;
            else
                r.principal.kind = ApplicationPrincipalKind::account;
            bool caught = false;
            try {
                SqliteReferenceAuthorityUnit u(empty, l);
                (void)u.resolve(r);
            } catch (const ReferenceAuthorityInputFailure &e) {
                caught = e.code() == F::invalid_data;
            }
            require(caught, "Invalid complete borrowed input wins before default ticket/SQL");
        });
    }

    void overlongStoredMetadataIsBoundedRefusalNotKnownEmpty() {
        scenario([] {
            Database d;
            corrupt(d.path, {"UPDATE subathon_reference_authority_events SET "
                             "session_ref=zeroblob(128) WHERE revision=X'0000000000000003'"});
            ReferenceAuthorityLimits l;
            l.reference_bytes = 64;
            failure(d.resolve(request(), l), F::resource_limit);
            Observer o(d.path);
            require(o.rows("SELECT length(session_ref) FROM subathon_reference_authority_events "
                           "WHERE revision=X'0000000000000003'") == SqlRows{{128}},
                    "No truncation or repair on bounded refusal");
        });
    }

    void liveCallbackAfterCumulativeFailureReportsClosedThenExpires() {
        scenario([] {
            Database d;
            bool stopped = false;
            require(d.writer.shutdown([&] { stopped = true; }) == StorageError::none,
                    "Close setup writer");
            require(QTest::qWaitFor([&] { return stopped; }, 10000), "Actually closed");
            // Use a fresh controlled writer on the same disposable file.
            auto budget = account_sessions_fixture::limits();
            budget.result_rows = 1;
            SqliteWriter tiny;
            require(tiny.open(d.path, budget) == StorageError::none,
                    "Reopened controlled writer with one-row cumulative ceiling");
            Probe p;
            bool closed = false;
            p.expected_work_failure = WorkError::result_limit;
            p.before = [&](const auto &v) {
                work_failure([&] { (void)v.resolve_reference_authority(request(), {}); },
                             WorkError::result_limit);
                work_failure([&] { (void)v.resolve_reference_authority(request(), {}); },
                             WorkError::session_closed);
                closed = true;
            };
            p.attempt(d, &tiny);
            require(closed, "Terminal SAME-session callback observed both failures");
            view_failure([&] { (void)p.retained.resolve_reference_authority(request(), {}); },
                         ApplicationReferenceViewError::expired_callback);
        });
    }
    void callbackCannotRaiseEnclosingServiceResultCeilings_data() {
        QTest::addColumn<int>("ceiling");
        QTest::newRow("reference") << 0;
        QTest::newRow("storage-document") << 1;
        QTest::newRow("intermediate-bytes") << 2;
        QTest::newRow("intermediate-items") << 3;
    }
    void callbackCannotRaiseEnclosingServiceResultCeilings() {
        QFETCH(int, ceiling);
        scenario([&] {
            Database d;
            Probe p;
            if (ceiling == 0)
                p.service_limits.reference_bytes = 39; // Full RSS1 needs40.
            else if (ceiling == 1)
                p.service_limits.storage_document_bytes = 1;
            else if (ceiling == 2)
                p.service_limits.intermediate_owned_bytes = 2439; // Facts need2440.
            else
                p.service_limits.intermediate_items = 130; // Facts need131.
            p.attempt(d); // Probe supplies ordinary larger limits: it cannot raise these.
            failure(*p.result, F::resource_limit);
        });
    }
    void callbackPreflightPrecedesDefaultAndExpiredViewChecks_data() {
        QTest::addColumn<bool>("expired");
        QTest::addColumn<bool>("bad_limits");
        QTest::newRow("default-limits") << false << true;
        QTest::newRow("default-input") << false << false;
        QTest::newRow("expired-limits") << true << true;
        QTest::newRow("expired-input") << true << false;
    }
    void callbackPreflightPrecedesDefaultAndExpiredViewChecks() {
        QFETCH(bool, expired);
        QFETCH(bool, bad_limits);
        scenario([&] {
            ApplicationReferenceReadView view;
            Database d;
            Probe p;
            if (expired) {
                p.attempt(d);
                view = p.retained;
            }
            auto r = request();
            ReferenceAuthorityLimits l;
            if (bad_limits)
                l.input_bytes = 0;
            else
                r.principal.principal_ref.clear();
            bool caught = false;
            try {
                (void)view.resolve_reference_authority(r, l);
            } catch (const ReferenceAuthorityInputFailure &e) {
                caught = e.code() == F::invalid_data;
            }
            require(caught, "Complete limit/input preflight precedes expired callback checks");
        });
    }

    void recordAndStagedResultBudgetsRefuseBeforeMaterializationOrWrites() {
        scenario([] {
            Database d;
            Observer o(d.path);
            const auto before = registry_rows(o);
            d.work([&](const auto &s) {
                ReferenceAuthorityLimits l;
                l.result_bytes = 79;
                SqliteReferenceAuthorityUnit fit(s, l);
                require(std::holds_alternative<ReferenceAuthorityRecord>(fit.read("I", "K")),
                        "Whole record literal79-byte exact fit");
                l.result_bytes = 78;
                SqliteReferenceAuthorityUnit short_read(s, l);
                const auto r = short_read.read("I", "K");
                require(std::holds_alternative<F>(r) && std::get<F>(r) == F::resource_limit,
                        "Whole record one byte short never partial");
                l.result_bytes = 7;
                SqliteReferenceAuthorityUnit short_write(s, l);
                failed(short_write.set_state(actor('O'), d.clock, 3, "K", S::revoked),
                       F::resource_limit);
            });
            require(registry_rows(o) == before, "Projected staged result refusal has zero writes");
            d.work([&](const auto &s) {
                ReferenceAuthorityLimits l;
                l.result_bytes = 8;
                SqliteReferenceAuthorityUnit fit(s, l);
                staged(fit.set_state(actor('O'), d.clock, 3, "K", S::revoked), 4);
            });
        });
    }
    void freshHistoricalDisclosureCannotUseExecuteOrNewCandidateAuthority() {
        scenario([] {
            Database d;
            exact(facts(d.resolve()));
            auto r = request();
            r.use = ReferenceAuthorityUse::disclose_application;
            r.historical_operation = ApplicationOperationKey{"historical", "missing"};
            failure(d.resolve(r), F::missing);
            r.use = ReferenceAuthorityUse::disclose_holds;
            r.historical_hold = "absent";
            failure(d.resolve(r), F::missing);
            r = request();
            r.use = ReferenceAuthorityUse::disclose_application;
            Observer o(d.path);
            const auto before = registry_rows(o);
            d.work([&](const auto &session) {
                SqliteReferenceAuthorityUnit u(session);
                bool caught = false;
                try {
                    (void)u.resolve(r);
                } catch (const ReferenceAuthorityInputFailure &e) {
                    caught = e.code() == F::invalid_data;
                }
                require(caught, "No actual historical key cannot become empty known disclosure");
            });
            require(registry_rows(o) == before, "Missing-key input refusal has zero writes");
        });
    }
    // ACCESS-011 supplemental selected historical composition, revision 1.
    // Authored only: the independent tier A review must precede body execution.
    void selectedHistoricalPayloadPreflightKeepsCallerTicket_data() {
        QTest::addColumn<bool>("hold");
        QTest::addColumn<QStringList>("poison");
        QTest::addColumn<QString>("truth");
        QTest::addColumn<bool>("aggregate");
        QTest::newRow("application-result-document")
            << false << QStringList{"UPDATE subathon_app_results SET document=zeroblob(1048577)"}
            << "SELECT length(document) FROM subathon_app_results" << false;
        QTest::newRow("application-audit-document")
            << false << QStringList{"UPDATE subathon_app_audit SET document=zeroblob(1048577)"}
            << "SELECT length(document) FROM subathon_app_audit" << false;
        QTest::newRow("hold-first-detection-audit")
            << true
            << QStringList{"UPDATE subathon_hold_detections SET audit_document=zeroblob(1048577)"}
            << "SELECT length(audit_document) FROM subathon_hold_detections" << false;
        QTest::newRow("hold-cause-observation")
            << true
            << QStringList{"UPDATE subathon_hold_cause_observations SET document=zeroblob(1048577)"}
            << "SELECT length(document) FROM subathon_hold_cause_observations" << false;
        QTest::newRow("application-aggregate-before-any-document")
            << false
            << QStringList{"UPDATE subathon_app_results SET document=zeroblob(200000)",
                           "UPDATE subathon_app_audit SET document=zeroblob(200000)",
                           "UPDATE subathon_app_states SET document=zeroblob(200000) WHERE "
                           "token=X'5331'"}
            << "SELECT (SELECT length(document) FROM subathon_app_results)+"
               "(SELECT length(document) FROM subathon_app_audit)+"
               "(SELECT length(document) FROM subathon_app_states WHERE token=X'5331')"
            << true;
        QTest::newRow("hold-aggregate-before-any-document")
            << true
            << QStringList{"UPDATE subathon_hold_cause_observations SET document=zeroblob(200000)",
                           "UPDATE subathon_hold_detection_observations SET "
                           "document=zeroblob(200000)",
                           "UPDATE subathon_hold_detections SET audit_document=zeroblob(200000)"}
            << "SELECT (SELECT length(document) FROM subathon_hold_cause_observations)+"
               "(SELECT length(document) FROM subathon_hold_detection_observations)+"
               "(SELECT length(audit_document) FROM subathon_hold_detections)"
            << true;
    }
    void selectedHistoricalPayloadPreflightKeepsCallerTicket() {
        QFETCH(bool, hold);
        QFETCH(QStringList, poison);
        QFETCH(QString, truth);
        QFETCH(bool, aggregate);
        scenario([&] {
            Database d;
            seed_history(d, hold);
            SqliteWriter reader;
            // 128 KiB is above the literal base's small scalar/command/capture/
            // graph closure, but BELOW every poisoned payload. Base is checked
            // separately on the same reopened writer; no measured failing-run
            // size is used to derive either limit or expected result.
            selected_record_fixture::reopen(d, reader, 131072);
            ReferenceAuthorityLimits limits;
            if (aggregate)
                limits.captures.owned_bytes = 524288;
            selected_record_fixture::healthy(reader, d.path, hold, limits, [&](const auto &r) {
                selected_record_fixture::disclosure(r, hold);
            });
            corrupt(d.path, poison);
            Observer observer(d.path);
            const auto poisoned = observer.rows(truth);
            require(poisoned.size() == 1 &&
                        poisoned[0][0].toLongLong() == (aggregate ? 600000 : 1048577),
                    "Real selected non-command payload mutation, literal bytes not decoder output");
            // Single: 1048577 > document262144. Aggregate: three individually
            // legal200000 documents total600000 > owned524288. Loading even
            // ONE200000 document exceeds the writer's131072 cumulative budget.
            selected_record_fixture::healthy(reader, d.path, hold, limits,
                                             [](const auto &r) { failure(r, F::resource_limit); });
            require(observer.rows(truth) == poisoned,
                    "Healthy local refusal neither repairs nor erases selected stored data");
        });
    }
    void selectedHistoricalCorruptionIsHealthyInvalidData_data() {
        QTest::addColumn<bool>("hold");
        QTest::addColumn<QString>("poison");
        QTest::addColumn<QString>("restore");
        QTest::addColumn<QString>("truth");
        QTest::newRow("application-full-record-count")
            << false << "UPDATE subathon_app_results SET component_count=2"
            << "UPDATE subathon_app_results SET component_count=3"
            << "SELECT component_count FROM subathon_app_results";
        QTest::newRow("application-non-command-envelope-class")
            << false << "UPDATE subathon_app_results SET format_ref='opaque.result'"
            << "UPDATE subathon_app_results SET format_ref=X'6f70617175652e726573756c74'"
            << "SELECT typeof(format_ref),hex(format_ref) FROM subathon_app_results";
        QTest::newRow("application-nested-exact-denominator")
            << false << "UPDATE subathon_app_effects SET exact_denominator=X'30'"
            << "UPDATE subathon_app_effects SET exact_denominator=X'31'"
            << "SELECT hex(exact_denominator) FROM subathon_app_effects";
        QTest::newRow("hold-full-cause-occurrence-count")
            << true << "UPDATE subathon_hold_causes SET occurrence_count=2"
            << "UPDATE subathon_hold_causes SET occurrence_count=1"
            << "SELECT occurrence_count FROM subathon_hold_causes";
        QTest::newRow("hold-non-command-audit-envelope")
            << true << "UPDATE subathon_hold_detections SET audit_format_ref=X''"
            << "UPDATE subathon_hold_detections SET audit_format_ref=X'6f70617175652e6175646974'"
            << "SELECT typeof(audit_format_ref),hex(audit_format_ref) FROM "
               "subathon_hold_detections";
    }
    void selectedHistoricalCorruptionIsHealthyInvalidData() {
        QFETCH(bool, hold);
        QFETCH(QString, poison);
        QFETCH(QString, restore);
        QFETCH(QString, truth);
        scenario([&] {
            Database d;
            seed_history(d, hold);
            selected_record_fixture::healthy(d.writer, d.path, hold, {}, [&](const auto &r) {
                selected_record_fixture::disclosure(r, hold);
            });
            Observer observer(d.path);
            const auto original = observer.rows(truth);
            corrupt(d.path, {poison});
            const auto poisoned = observer.rows(truth);
            require(poisoned != original, "Actual selected stored corruption exists");
            selected_record_fixture::healthy(d.writer, d.path, hold, {},
                                             [](const auto &r) { failure(r, F::invalid_data); });
            require(observer.rows(truth) == poisoned,
                    "Reader leaves original corruption untouched");
            corrupt(d.path, {restore});
            require(observer.rows(truth) == original,
                    "Literal repair restores original stored value");
            selected_record_fixture::healthy(d.writer, d.path, hold, {}, [&](const auto &r) {
                selected_record_fixture::disclosure(r, hold);
            });
        });
    }
    void selectedHistoricalLateSqlFailurePreservesOriginalWriterTruth_data() {
        QTest::addColumn<bool>("hold");
        QTest::addColumn<QString>("table");
        QTest::newRow("application-nested-components") << false << "subathon_app_components";
        QTest::newRow("hold-nested-detection-observation")
            << true << "subathon_hold_detection_observations";
    }
    void selectedHistoricalLateSqlFailurePreservesOriginalWriterTruth() {
        QFETCH(bool, hold);
        QFETCH(QString, table);
        scenario([&] {
            Database d;
            seed_history(d, hold);
            selected_record_fixture::disclosure(d.resolve(selected_record_fixture::query(hold)),
                                                hold);
            selected_record_fixture::ensure_marker(d.writer);
            Observer observer(d.path);
            const auto registry = registry_rows(observer);
            const auto state = observer.rows(selected_record_fixture::marker_query());
            corrupt(d.path, {"DROP TABLE " + table});
            bool reached = false;
            // Typed exception checks MUST happen inside actual work. d.work/run
            // translate exceptions on the test thread and cannot prove this type.
            const auto result =
                account_sessions_fixture::outcome(d.writer, [&](auto c) -> QVariant {
                    const auto s = c.begin();
                    Cleanup cleanup{s};
                    (void)s.execute(
                        {"UPDATE subathon_retained_configurations SET document=X'63616c6c6572' "
                         "WHERE reference=X'63616c6c65722e6d61726b6572'",
                         {}});
                    SqliteReferenceAuthorityUnit unit(s);
                    work_failure([&] { (void)unit.resolve(selected_record_fixture::query(hold)); },
                                 WorkError::sql_failure);
                    work_failure([&] { (void)s.execute({"SELECT 1", {}}); },
                                 WorkError::session_closed);
                    work_failure([&] { (void)unit.read("I", "K"); }, WorkError::session_closed);
                    reached = true;
                    return 99;
                });
            require(
                reached && result.error == WorkError::sql_failure &&
                    result.transaction == TransactionOutcome::rolled_back && !result.value &&
                    registry_rows(observer) == registry &&
                    observer.rows(selected_record_fixture::marker_query()) == state,
                "First original SQL failure survives later session_closed; rollback, no completion "
                "value");
        });
    }
    void selectedHoldClosureIgnoresOnlyUnselectedHistory_data() {
        QTest::addColumn<QString>("poison");
        QTest::addColumn<bool>("selected");
        QTest::newRow("unselected-v2-producer-reference")
            << "UPDATE subathon_hold_versions SET mutation_ref=X'3f' WHERE version_ref=X'7632'"
            << false;
        QTest::newRow("unselected-d2-event-reference")
            << "UPDATE subathon_hold_detections SET proposed_version_ref=X'3f' WHERE "
               "mutation_ref=X'6432'"
            << false;
        QTest::newRow("selected-direct-parent-v3-state")
            << "UPDATE subathon_hold_versions SET state=9 WHERE version_ref=X'7633'" << true;
        QTest::newRow("selected-first-detection-audit")
            << "UPDATE subathon_hold_detections SET audit_format_ref=X'' WHERE "
               "mutation_ref=X'646574656374696f6e'"
            << true;
        QTest::newRow("selected-current-producer-audit")
            << "UPDATE subathon_hold_detections SET audit_format_ref=X'' WHERE mutation_ref=X'6434'"
            << true;
    }
    void selectedHoldClosureIgnoresOnlyUnselectedHistory() {
        QFETCH(QString, poison);
        QFETCH(bool, selected);
        scenario([&] {
            Database d;
            selected_record_fixture::four_detections(d);
            selected_record_fixture::healthy(d.writer, d.path, true, {}, [](const auto &r) {
                selected_record_fixture::disclosure(r, true);
            });
            Observer observer(d.path);
            const auto sql = "SELECT * FROM subathon_hold_versions ORDER BY position";
            const auto events = "SELECT * FROM subathon_hold_detections ORDER BY mutation_ref";
            const auto old_versions = observer.rows(sql), old_events = observer.rows(events);
            corrupt(d.path, {poison});
            const auto versions = observer.rows(sql), detections = observer.rows(events);
            require(versions != old_versions || detections != old_events,
                    "Real poisoned history row remains present in disposable database");
            selected_record_fixture::healthy(d.writer, d.path, true, {}, [&](const auto &r) {
                if (selected)
                    failure(r, F::invalid_data);
                else
                    selected_record_fixture::disclosure(r, true);
            });
            require(observer.rows(sql) == versions && observer.rows(events) == detections,
                    "Selected read validates its finite closure, never repairs unselected history");
        });
    }
    void selectedHistoryStillRequiresFreshOwningReadAuthority_data() {
        QTest::addColumn<bool>("hold");
        QTest::addColumn<int>("gate");
        for (bool hold : {false, true}) {
            const QByteArray prefix = hold ? "hold-" : "application-";
            QTest::newRow((prefix + "author-read-grant").constData()) << hold << 0;
            QTest::newRow((prefix + "execution-read-use").constData()) << hold << 1;
            QTest::newRow((prefix + "actual-capture").constData()) << hold << 2;
        }
    }
    void selectedHistoryStillRequiresFreshOwningReadAuthority() {
        QFETCH(bool, hold);
        QFETCH(int, gate);
        scenario([&] {
            Database d;
            seed_history(d, hold);
            const auto q = selected_record_fixture::query(hold);
            selected_record_fixture::disclosure(d.resolve(q), hold);
            if (gate < 2) {
                auto g = graph(gate == 0);
                auto &entries = g.accounts[gate == 0 ? 0 : 1].individual_entries;
                entries.erase(entries.begin() + (hold ? 2 : 1));
                d.replace_graph(gate == 0 ? 1 : 0, g);
                // Execute remains independently permitted on actual current facts.
                permitted(facts(d.resolve()));
                selected_record_fixture::healthy(d.writer, d.path, hold, {},
                                                 [](const auto &r) { failure(r, F::denied); });
                d.replace_graph(gate == 0 ? 1 : 0, graph(gate == 0));
                selected_record_fixture::healthy(d.writer, d.path, hold, {}, [&](const auto &r) {
                    selected_record_fixture::disclosure(r, hold, 3);
                });
            } else {
                // Upstream binding/capture controls can refuse before historical reading.
                // The new check here is SAME-ticket health, not reader reachability.
                auto wrong = q;
                wrong.captured.rule.timer_ref = "other";
                failure(d.resolve(wrong), F::invalid_data);
                // Corrupt the ACTUAL retained execution record, not a fake source.
                corrupt(
                    d.path,
                    {"UPDATE subathon_retained_contexts SET document=X'00' WHERE reference=X'59'"});
                selected_record_fixture::healthy(
                    d.writer, d.path, hold, {}, [](const auto &r) { failure(r, F::invalid_data); });
            }
        });
    }
    // A11-I01/A11-N01 additive regression contract: input admission precedes SQL;
    // a pending state is valid only at the immutable initial event.
    void historicalKeyCeilingPrecedesDefaultCapabilities_data() {
        QTest::addColumn<int>("field");
        QTest::addColumn<int>("caller");
        for (int field = 0; field < 3; ++field)
            for (int caller = 0; caller < 3; ++caller)
                QTest::newRow(qPrintable(QString("key-%1-caller-%2").arg(field).arg(caller)))
                    << field << caller;
    }
    void historicalKeyCeilingPrecedesDefaultCapabilities() {
        QFETCH(int, field);
        QFETCH(int, caller);
        scenario([&] {
            ReferenceAuthorityLimits limits;
            limits.reference_bytes = 128;
            limits.captures.reference_bytes = 64;
            auto q = selected_record_fixture::query(field == 2);
            auto &key = field == 0   ? q.historical_operation->operation_scope
                        : field == 1 ? q.historical_operation->operation_ref
                                     : *q.historical_hold;
            key.assign(64, 'k'); // Exact derived ceiling fits; broad ceiling is128.
            SqliteWorkSession absent;
            ApplicationReferenceReadView view;
            SqliteReferenceAuthorityUnit unit(absent, limits);
            SqliteReferenceAuthoritySource source(limits);
            const auto attempt = [&] {
                if (caller == 0)
                    (void)unit.resolve(q);
                else if (caller == 1)
                    (void)view.resolve_reference_authority(q, limits);
                else
                    (void)source.resolve(view, q);
            };
            if (caller == 0)
                work_failure(attempt, WorkError::stale_session);
            else
                view_failure(attempt, ApplicationReferenceViewError::expired_callback);
            key.push_back('k'); // 65<=128 but65>min(128,64); no SQL/callback access.
            bool caught = false;
            try {
                attempt();
            } catch (const ReferenceAuthorityInputFailure &e) {
                caught = e.code() == F::resource_limit;
            }
            require(
                caught,
                "Historical nested key overage is typed input failure BEFORE capability checks");
        });
    }
    void historicalKeyOverageConsumesNoWriterStatements_data() {
        QTest::addColumn<int>("field");
        QTest::newRow("application-scope") << 0;
        QTest::newRow("application-reference") << 1;
        QTest::newRow("hold-reference") << 2;
    }
    void historicalKeyOverageConsumesNoWriterStatements() {
        QFETCH(int, field);
        scenario([&] {
            Database d;
            seed_history(d, field == 2);
            bool stopped = false;
            require(d.writer.shutdown([&] { stopped = true; }) == StorageError::none,
                    "Close setup writer before constrained admission check");
            require(QTest::qWaitFor([&] { return stopped; }, 10000), "Setup writer closed");
            auto budget = account_sessions_fixture::limits();
            budget.statements_per_batch = 1;
            SqliteWriter tiny;
            require(tiny.open(d.path, budget) == StorageError::none, "One-statement real writer");
            Observer observer(d.path);
            const auto before = registry_rows(observer);
            ReferenceAuthorityLimits limits;
            limits.reference_bytes = 128;
            limits.captures.reference_bytes = 64;
            auto q = selected_record_fixture::query(field == 2);
            auto &key = field == 0   ? q.historical_operation->operation_scope
                        : field == 1 ? q.historical_operation->operation_ref
                                     : *q.historical_hold;
            key.assign(65, 'k');
            bool caught = false;
            const auto result = account_sessions_fixture::outcome(tiny, [&](auto c) -> QVariant {
                const auto session = c.begin();
                Cleanup cleanup{session};
                SqliteReferenceAuthorityUnit unit(session, limits);
                try {
                    (void)unit.resolve(q);
                } catch (const ReferenceAuthorityInputFailure &e) {
                    caught = e.code() == F::resource_limit;
                }
                require(caught, "Derived historical-key refusal is pre-SQL input admission");
                require(session.execute({"SELECT 1", {}})[0][0].toInt() == 1,
                        "The caller's ONLY statement remains available after input refusal");
                session.commit();
                return 23;
            });
            require(caught && result.error == WorkError::none &&
                        result.transaction == TransactionOutcome::committed && result.value &&
                        result.value->toInt() == 23 && registry_rows(observer) == before,
                    "No preflight SQL, rollback, write or disguised foreign failure");
        });
    }
    void historicalProfileDoesNotNarrowUnrelatedRequestKeys_data() {
        QTest::addColumn<bool>("hold");
        QTest::newRow("application") << false;
        QTest::newRow("hold") << true;
    }
    void historicalProfileDoesNotNarrowUnrelatedRequestKeys() {
        QFETCH(bool, hold);
        scenario([&] {
            Database d;
            seed_history(d, hold);
            ReferenceAuthorityLimits limits;
            limits.reference_bytes = 128;
            limits.captures.reference_bytes = 64;
            auto q = selected_record_fixture::query(hold);
            q.requested_operation.operation_ref.assign(100, 'r');
            // This is today's request identity, not an argument to the historical
            // reader. Its own128 ceiling applies; the historical keys still fit64.
            const auto resolved = d.resolve(q, limits);
            selected_record_fixture::disclosure(resolved, hold);
            require(facts(resolved).binding.requested_operation.operation_ref ==
                        std::string(100, 'r'),
                    "Nested profile does not silently narrow the whole authority request");
        });
    }
    void onlyInitialRegistryEventMayBePending_data() {
        QTest::addColumn<bool>("initial");
        QTest::addColumn<bool>("read_only");
        QTest::newRow("initial-read") << true << true;
        QTest::newRow("initial-resolve") << true << false;
        QTest::newRow("noninitial-read") << false << true;
        QTest::newRow("noninitial-resolve") << false << false;
    }
    void onlyInitialRegistryEventMayBePending() {
        QFETCH(bool, initial);
        QFETCH(bool, read_only);
        scenario([&] {
            Database d;
            if (initial)
                d.work([](const auto &session) {
                    (void)session.execute({"DELETE FROM subathon_reference_authority_events WHERE "
                                           "revision=X'0000000000000003'",
                                           {}});
                    (void)session.execute({"UPDATE subathon_reference_authority_head SET "
                                           "revision=X'0000000000000002'",
                                           {}});
                });
            else
                corrupt(d.path, {"UPDATE subathon_reference_authority_events SET state=0 WHERE "
                                 "revision=X'0000000000000003'"});
            Observer observer(d.path);
            const auto before = registry_rows(observer);
            d.work([&](const auto &session) {
                SqliteReferenceAuthorityUnit unit(session);
                if (read_only) {
                    const auto r = unit.read("I", "K");
                    if (initial) {
                        require(std::holds_alternative<ReferenceAuthorityRecord>(r),
                                "Legitimate initial pending record remains readable");
                        const auto &v = std::get<ReferenceAuthorityRecord>(r);
                        require(v.state == S::pending && v.created_revision == 2 &&
                                    v.state_revision == 2 && v.head_revision == 2 &&
                                    v.original_author == "A" && v.actor == "A",
                                "Pending initial event preserves its own provenance");
                    } else
                        require(std::holds_alternative<F>(r) && std::get<F>(r) == F::invalid_data,
                                "Noninitial pending event is impossible stored data, not policy");
                } else
                    failure(unit.resolve(request()), initial ? F::denied : F::invalid_data);
                require(session.execute({"SELECT 1", {}})[0][0].toInt() == 1,
                        "Structural refusal does not terminate caller transaction");
            });
            require(registry_rows(observer) == before, "Reader never repairs impossible history");
        });
    }
    // A11-I02 additive regression contract: authenticated mutations preserve
    // actual writer failures, while genuine producer refusals stay typed values.
    void mutationAuthenticationPreservesTicketFailures_data() {
        QTest::addColumn<int>("method");
        QTest::addColumn<int>("ticket");
        const char *methods[]{"bootstrap", "enroll", "set-state"};
        const char *tickets[]{"default", "foreign-thread", "closed-after-sql"};
        for (int method = 0; method < 3; ++method)
            for (int ticket = 0; ticket < 3; ++ticket)
                QTest::newRow(qPrintable(QString("%1/%2").arg(methods[method], tickets[ticket])))
                    << method << ticket;
    }
    void mutationAuthenticationPreservesTicketFailures() {
        QFETCH(int, method);
        QFETCH(int, ticket);
        scenario([&] {
            Database d;
            Observer observer(d.path);
            const auto before = registry_rows(observer);
            const auto invoke = [&](SqliteReferenceAuthorityUnit &u) -> ReferenceAuthorityWrite {
                if (method == 0)
                    return u.bootstrap(actor('O'), d.clock);
                if (method == 1) {
                    auto b = binding();
                    b.capability = "new";
                    return u.enroll(actor(), d.clock, 3, b, "E");
                }
                return u.set_state(actor('O'), d.clock, 3, "K", S::revoked);
            };
            if (ticket == 0) {
                SqliteWorkSession absent;
                SqliteReferenceAuthorityUnit unit(absent);
                work_failure([&] { (void)invoke(unit); }, WorkError::stale_session);
            } else if (ticket == 1) {
                d.work([&](const auto &session) {
                    SqliteReferenceAuthorityUnit unit(session);
                    auto worker = std::async(std::launch::async, [&] {
                        work_failure([&] { (void)invoke(unit); }, WorkError::wrong_thread);
                    });
                    worker.get();
                    require(session.execute({"SELECT 1", {}})[0][0].toInt() == 1,
                            "Wrong-thread refusal leaves the owner's ticket healthy");
                });
            } else {
                bool reached = false;
                const auto result =
                    account_sessions_fixture::outcome(d.writer, [&](auto context) -> QVariant {
                        const auto session = context.begin();
                        Cleanup cleanup{session};
                        work_failure(
                            [&] {
                                (void)session.execute(
                                    {"SELECT * FROM intentionally_missing_authority_auth", {}});
                            },
                            WorkError::sql_failure);
                        SqliteReferenceAuthorityUnit unit(session);
                        work_failure([&] { (void)invoke(unit); }, WorkError::session_closed);
                        reached = true;
                        return 1;
                    });
                require(
                    reached && result.error == WorkError::sql_failure &&
                        result.transaction == TransactionOutcome::rolled_back && !result.value,
                    "Closed-ticket call throws while completion retains the ORIGINAL SQL error");
            }
            require(registry_rows(observer) == before, "Ticket failures never mutate registry");
        });
    }
    void mutationAuthenticationPreservesFirstSqlAndBudgetFailure_data() {
        QTest::addColumn<int>("method");
        QTest::addColumn<int>("fault");
        const char *methods[]{"bootstrap", "enroll", "set-state"};
        const char *faults[]{"missing-auth-table", "statement-budget", "row-budget"};
        for (int method = 0; method < 3; ++method)
            for (int fault = 0; fault < 3; ++fault)
                QTest::newRow(qPrintable(QString("%1/%2").arg(methods[method], faults[fault])))
                    << method << fault;
    }
    void mutationAuthenticationPreservesFirstSqlAndBudgetFailure() {
        QFETCH(int, method);
        QFETCH(int, fault);
        scenario([&] {
            Database d;
            Observer observer(d.path);
            const auto before = registry_rows(observer);
            bool stopped = false;
            require(d.writer.shutdown([&] { stopped = true; }) == StorageError::none,
                    "Stop unconstrained setup writer");
            require(QTest::qWaitFor([&] { return stopped; }, 10000), "Setup writer stopped");
            auto budget = account_sessions_fixture::limits();
            if (fault == 1)
                budget.statements_per_batch = 1;
            else if (fault == 2)
                budget.result_rows = 1;
            SqliteWriter writer;
            require(writer.open(d.path, budget) == StorageError::none,
                    "Real writer with selected fault boundary");
            const auto expected = fault == 0   ? WorkError::sql_failure
                                  : fault == 1 ? WorkError::resource_limit
                                               : WorkError::result_limit;
            bool reached = false;
            const auto result =
                account_sessions_fixture::outcome(writer, [&](auto context) -> QVariant {
                    const auto session = context.begin();
                    Cleanup cleanup{session};
                    if (fault == 0)
                        (void)session.execute({"DROP TABLE main.subathon_account_sessions", {}});
                    SqliteReferenceAuthorityUnit unit(session);
                    const auto invoke = [&]() -> ReferenceAuthorityWrite {
                        if (method == 0)
                            return unit.bootstrap(actor('O'), d.clock);
                        if (method == 1) {
                            auto b = binding();
                            b.capability = "new";
                            return unit.enroll(actor(), d.clock, 3, b, "E");
                        }
                        return unit.set_state(actor('O'), d.clock, 3, "K", S::revoked);
                    };
                    work_failure([&] { (void)invoke(); }, expected);
                    work_failure([&] { (void)invoke(); }, WorkError::session_closed);
                    reached = true;
                    return 1;
                });
            require(reached && result.error == expected &&
                        result.transaction == TransactionOutcome::rolled_back && !result.value,
                    "Authentication throws FIRST SQL/budget error, then session_closed; completion "
                    "unchanged");
            require(
                registry_rows(observer) == before &&
                    observer.rows("SELECT count(*) FROM subathon_account_sessions")[0][0].toInt() ==
                        3,
                "Real rollback restores the authentication table and leaves registry unchanged");
        });
    }
    void mutationAuthenticationDistinguishesHealthyProducerRefusals_data() {
        QTest::addColumn<int>("method");
        QTest::addColumn<int>("refusal");
        const char *methods[]{"bootstrap", "enroll", "set-state"};
        const char *refusals[]{"clock-unavailable", "clock-exception", "account-selected-budget"};
        for (int method = 0; method < 3; ++method)
            for (int refusal = 0; refusal < 3; ++refusal)
                QTest::newRow(qPrintable(QString("%1/%2").arg(methods[method], refusals[refusal])))
                    << method << refusal;
    }
    void mutationAuthenticationDistinguishesHealthyProducerRefusals() {
        QFETCH(int, method);
        QFETCH(int, refusal);
        scenario([&] {
            struct ControlledClock final : CurrentSessionClock {
                int mode, calls = 0;
                explicit ControlledClock(int selected) : mode(selected) {}
                SessionClockResolution sample_current() override {
                    ++calls;
                    if (mode == 0)
                        return SessionSourceUnavailable{};
                    if (mode == 1)
                        throw std::logic_error("Synthetic unavailable clock producer");
                    return SessionClockSample{1000};
                }
            } clock(refusal);
            Database d;
            Observer observer(d.path);
            const auto before = registry_rows(observer);
            d.work([&](const auto &session) {
                ReferenceAuthorityLimits limits;
                if (refusal == 2) {
                    // Lookup input I + 32-byte digest fits33. Stored session
                    // references + format + digest alone cost40, after metadata SQL.
                    limits.accounts.selected_bytes = 33;
                }
                SqliteReferenceAuthorityUnit unit(session, limits);
                ReferenceAuthorityWrite result;
                if (method == 0)
                    result = unit.bootstrap(actor('O'), clock);
                else if (method == 1) {
                    auto b = binding();
                    b.capability = "new";
                    result = unit.enroll(actor(), clock, 3, b, "E");
                } else
                    result = unit.set_state(actor('O'), clock, 3, "K", S::revoked);
                failed(result, refusal == 2 ? F::resource_limit : F::unavailable);
                require(clock.calls == (refusal == 2 ? 0 : 1),
                        "Genuine source refusal precedes clock; clock refusal is sampled once");
                require(session.execute({"SELECT 1", {}})[0][0].toInt() == 1,
                        "Semantic source/clock refusal leaves SAME caller ticket healthy");
            });
            require(registry_rows(observer) == before, "Healthy refusals never mutate registry");
        });
    }
};
QTEST_GUILESS_MAIN(SqliteReferenceAuthorityTests)
#include "sqlite_reference_authority_tests.moc"
