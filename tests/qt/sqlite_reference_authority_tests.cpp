// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/sqlite_reference_authority_fixture.hpp"
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
// This port is an assertion probe, never a fake authority source. Real callback
// resolution MUST succeed before returning a deliberate denial to stop accounting.
class Probe final : public ExecutionAccess,
                    public CurrentApplicationPolicy,
                    public SegmentedApplicationClock,
                    public RequiredApplicationActions,
                    public FreshApplicationReferences {
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
        ApplicationServicePorts ports{*this, *this, *this, *this, *this};
        OriginalApplicationService service(override_writer ? *override_writer : d.writer, ports,
                                           service_limits);
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
        QTest::addColumn<int>("entry_index");
        QTest::newRow("management") << 0 << 3;
        QTest::newRow("use") << 0 << 0;
        QTest::newRow("grant") << 1 << 0;
    }
    void enrollmentRequiresManagementAndWholeAuthorUseAndGrant() {
        QFETCH(int, channel);
        QFETCH(int, entry_index);
        scenario([&] {
            Database d(false);
            auto g = graph(channel == 1);
            g.accounts[0].individual_entries.erase(g.accounts[0].individual_entries.begin() +
                                                   entry_index);
            d.replace_graph(channel, g);
            d.work([&](const auto &s) {
                SqliteReferenceAuthorityUnit u(s);
                staged(u.bootstrap(actor('O'), d.clock), 1);
                failed(u.enroll(actor(), d.clock, 1, binding(), "E"), F::denied);
            });
            Observer o(d.path);
            require(o.rows("SELECT * FROM subathon_reference_authority_bindings").isEmpty(),
                    "No partial enrollment");
        });
    }
    void ownerCannotRevokeWithoutDelegationEvenForRestrictiveChange() {
        scenario([] {
            Database d;
            auto g = graph(true);
            g.accounts[2].individual_entries.clear();
            d.replace_graph(1, g);
            Observer o(d.path);
            const auto before = registry_rows(o);
            d.work([&](const auto &s) {
                SqliteReferenceAuthorityUnit u(s);
                failed(u.set_state(actor('O'), d.clock, 3, "K", S::revoked), F::denied);
            });
            require(registry_rows(o) == before,
                    "Restrictive change still requires current management+whole delegation");
        });
    }
    void revokeReauthorizeAndCollisionPreserveImmutableHistoricalIdentity() {
        scenario([] {
            Database d;
            Observer o(d.path);
            const auto old = o.rows("SELECT * FROM subathon_reference_authority_bindings");
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
                permitted(facts(resolved));
            });
            require(o.rows("SELECT * FROM subathon_reference_authority_bindings") == old &&
                        o.rows("SELECT * FROM subathon_reference_authority_events").size() == 4,
                    "No overwritten original binding/events through lifecycle");
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
        QTest::newRow("graph-missing") << "DELETE FROM subathon_permission_head" << 2;
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
        for (int i = 0; i < 19; ++i)
            QTest::newRow(qPrintable(QString::number(i))) << i;
    }
    void everyCapturedBindingMismatchRefuses() {
        QFETCH(int, field);
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
            else
                ++r.captured.plan.revision;
            const auto v = d.resolve(r);
            require(std::holds_alternative<F>(v.value),
                    "No mismatched scope/source/amount/retained version becomes authority");
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
            bool caught = false;
            try {
                (void)d.resolve(request(), l);
            } catch (const ReferenceAuthorityInputFailure &x) {
                caught = x.code() == F::resource_limit;
            }
            require(caught, "Full input refused before SQL/copy");
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
        QTest::newRow("application") << false;
        QTest::newRow("hold") << true;
    }
    void actualHistoricalApplicationAndHoldDisclosureAreSeparateAndFresh() {
        QFETCH(bool, hold);
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
            // Execute still fits. Removing only the historical read grant must block
            // disclosure even after a successful historical read and execute.
            auto g = graph(true);
            g.accounts[0].individual_entries.erase(g.accounts[0].individual_entries.begin() +
                                                   (hold ? 2 : 1));
            d.replace_graph(1, g);
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
    void callbackCannotRaiseEnclosingServiceResultCeilings() {
        scenario([] {
            Database d;
            Probe p;
            p.service_limits.intermediate_items = 1;
            p.attempt(d);
            failure(*p.result, F::resource_limit);
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
            bool caught = false;
            try {
                (void)d.resolve(r);
            } catch (const ReferenceAuthorityInputFailure &e) {
                caught = e.code() == F::invalid_data;
            }
            require(caught, "No actual historical key cannot become empty known disclosure");
        });
    }
};
QTEST_GUILESS_MAIN(SqliteReferenceAuthorityTests)
#include "sqlite_reference_authority_tests.moc"
