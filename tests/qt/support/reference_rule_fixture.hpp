// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "application_port_fake.hpp"
#include "sqlite_reference_authority_fixture.hpp"
#include <subathon/server/reference_application.hpp>

namespace reference_rule_fixture {
using namespace reference_authority_fixture;
inline ReferenceConfiguration configuration() {
    ReferenceConfiguration c;
    c.configuration_ref = "C";
    c.policy_revision_ref = "W";
    c.synthetic_origin = {"synthetic.reference.v1", "A"};
    c.receipt_namespace = "N";
    c.rule = request().captured.rule;
    c.timer_config = {TimerEndMode::Grace, 120000, GraceReuse::Fresh, std::nullopt};
    c.other_rules = c.modifiers = c.milestones = c.outbound_actions = c.generated_events =
        ReferenceFactsState::complete;
    return c;
}
// Complete unprojected synthetic DATA. This is deliberately NOT an actual loader.
inline ReferenceConfigurationFacts configuration_facts() {
    ReferenceConfigurationFacts f;
    f.configuration_ref = "C";
    f.configuration_owner_revision_ref = "owner-revision";
    f.projection = configuration();
    f.rules_state = ReferenceFactsState::complete;
    f.enabled_rules = {
        {f.projection.rule, {"synthetic.reference.support.v1"}, {"time_add"}, {"ms"}, {}, {}, {}}};
    f.modifiers.state = f.milestones.state = f.outbound_actions.state = f.generated_events.state =
        ReferenceFactsState::complete;
    return f;
}
inline ReferenceArrivalCapture arrival() {
    return {"X", "V", 0, configuration(), {"Q", "E", std::nullopt, QByteArray("", 0)}};
}
inline OriginalApplicationCommand command() {
    OriginalApplicationCommand c;
    c.operation = {"scope", "attempt"};
    c.receipt_id = {7};
    c.contribution_id = ContributionId{0};
    c.plan = {"P", 1};
    c.principal = {ApplicationPrincipalKind::automation, "I", "U"};
    c.choices = {
        {"subathon.reference.execution-ref.v1", QByteArray::fromHex("535246310000000159")}};
    return c;
}
class ConfigData final : public ReferenceConfigurationSource {
  public:
    ReferenceConfigurationResolution data{configuration_facts()};
    std::vector<std::string> loaded;
    ReferenceConfigurationResolution snapshot(const std::string &) override {
        throw std::logic_error("Execution must not use snapshot");
    }
    ReferenceConfigurationResolution load(const ApplicationReferenceReadView &,
                                          const std::string &ref) override {
        loaded.push_back(ref);
        return data;
    }
};
// Real writer and actual compiled authority prefix1-10, reusing frozen literal
// account/graph/capture fixture DATA. Registry is populated by public authenticated
// bootstrap/enroll/owner-authorize, never an allow adapter or seeded registry rows.
struct Store {
    QTemporaryDir directory;
    QString path{directory.filePath("rules006.sqlite")};
    SqliteWriter writer;
    Clock clock;
    explicit Store(std::int64_t amount = 600000, std::string execution_account = "E") {
        require(directory.isValid(), "Disposable rules006 database");
        require(writer.open(path, account_sessions_fixture::limits()) == StorageError::none,
                "Writer opens");
        const auto m =
            permission_graph_fixture::migrate(writer, reference_authority_repository_manifest());
        require(m.reason == MigrationReason::none && m.report && m.report->version == 10 &&
                    m.work.transaction == TransactionOutcome::committed &&
                    m.work.error == WorkError::none,
                "Actual compiled authority prefix10");
        work([&](const auto &s) {
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
            if (execution_account != "E") {
                // Independent initial policy DATA: enrollment management targets
                // the selected execution account, including the author's own A.
                auto entries = graph(false).accounts[0].individual_entries;
                entries.back().resources[0].allowed = {execution_account};
                (void)s.execute(
                    {"UPDATE subathon_permission_accounts SET entries=? WHERE channel=0 "
                     "AND account_ref=X'41'",
                     {encoded(entries)}});
            }
            const reference_capture_fixture::Data d;
            d.intake(s, "A");
            d.stage_capture(s, "A");
            d.index(s, "A");
        });
        if (amount != 600000) {
            // Independent literal fixture substitution: retain ONE configured int64
            // everywhere in the captured documents/plan. Not evaluator-generated DATA.
            work([&](const auto &s) {
                // Fixed eight-byte scalar replacement in independently framed fixture
                // documents. No evaluator/encoder output becomes an expected oracle.
                const auto old = QByteArray::fromHex("00000000000927c0");
                QByteArray replacement;
                const auto bits = static_cast<std::uint64_t>(amount);
                for (int shift = 56; shift >= 0; shift -= 8)
                    replacement += char((bits >> shift) & 255);
                for (const auto &row : s.execute(
                         {"SELECT reference,document FROM subathon_retained_configurations", {}})) {
                    auto document = row[1].toByteArray();
                    document.replace(old, replacement);
                    (void)s.execute(
                        {"UPDATE subathon_retained_configurations SET document=? WHERE reference=?",
                         {document, row[0]}});
                }
                for (const auto &row :
                     s.execute({"SELECT reference,document FROM subathon_retained_contexts", {}})) {
                    auto document = row[1].toByteArray();
                    document.replace(old, replacement);
                    (void)s.execute(
                        {"UPDATE subathon_retained_contexts SET document=? WHERE reference=?",
                         {document, row[0]}});
                }
                (void)s.execute(
                    {"UPDATE subathon_plan_requests SET requested_units=?,exact_numerator=?",
                     {QVariant::fromValue<qlonglong>(amount),
                      QByteArray::number(static_cast<qlonglong>(amount))}});
                for (int channel = 0; channel < 2; ++channel) {
                    const auto g = graph(channel == 1);
                    for (auto a : g.accounts) {
                        for (auto &e : a.individual_entries)
                            if (!e.quantities.empty())
                                e.quantities[0].upper =
                                    PermissionRational{std::to_string(amount), "1"};
                        (void)s.execute(
                            {"UPDATE subathon_permission_accounts SET entries=? WHERE channel=? "
                             "AND account_ref=?",
                             {encoded(a.individual_entries), channel, blob(a.account)}});
                    }
                }
            });
        }
        work([&](const auto &s) {
            SqliteReferenceAuthorityUnit u(s);
            staged(u.bootstrap(actor('O'), clock), 1);
            auto b = binding();
            b.requested_ms = amount;
            staged(u.enroll(actor(), clock, 1, b, execution_account), 2);
            staged(u.set_state(actor('O'), clock, 2, "K", ReferenceAuthorityState::authorized), 3);
        });
    }
    template <class F> void work(F f) {
        (void)run(writer, [&](auto ctx) -> QVariant {
            const auto s = ctx.begin();
            Cleanup clean{s};
            f(s);
            s.commit();
            return 1;
        });
    }
    void state(std::uint64_t head, ReferenceAuthorityState value) {
        work([&](const auto &s) {
            staged(SqliteReferenceAuthorityUnit(s).set_state(actor('O'), clock, head, "K", value),
                   head + 1);
        });
    }
    void narrow(int channel, const char *account, std::int64_t amount) {
        auto entries = profile(amount);
        work([&](const auto &s) {
            (void)s.execute({"UPDATE subathon_permission_accounts SET entries=? WHERE channel=? "
                             "AND account_ref=?",
                             {encoded(entries), channel, QByteArray(account)}});
            (void)s.execute(
                {"UPDATE subathon_permission_head SET revision=?", {encoded(++graph_head)}});
        });
    }
    std::uint64_t graph_head = 1;
};
// No fabricated producer result: wraps the actual finite callback source and only
// records observed DATA for assertions. Corruption is optional and clearly separate.
class Source final : public ReferenceAuthoritySource {
  public:
    SqliteReferenceAuthoritySource actual;
    std::optional<ReferenceAuthorityFacts> observed;
    std::function<void(ReferenceAuthorityFacts &)> corrupt_facts;
    std::optional<ReferenceAuthorityFailure> refuse;
    unsigned calls = 0;
    ReferenceAuthorityResolution resolve(const ApplicationReferenceReadView &v,
                                         const ReferenceAuthorityRequest &r) override {
        ++calls;
        if (refuse)
            return {*refuse};
        auto result = actual.resolve(v, r);
        if (auto *f = std::get_if<ReferenceAuthorityFacts>(&result.value)) {
            observed = *f;
            if (corrupt_facts)
                corrupt_facts(*f);
        }
        return result;
    }
};
// Exercise configured-rule authority, then stop with a deliberate denial unless
// full_accounting selects controlled policy, clock, actions and fresh references.
class Ports final : public qt_test_support::ApplicationPortFake {
  public:
    ConfigData config;
    Source source;
    ApplicationReferenceReadView retained;
    std::optional<AccessDecision> decision;
    std::exception_ptr witness;
    unsigned policies = 0, clocks = 0, disclosures = 0, actions = 0, references = 0;
    bool full_accounting = false;
    std::optional<std::int64_t> cap;
    ReferenceAdapterLimits adapter_limits;
    ReferenceAuthorityLimits authority_limits;
    std::function<void(const ApplicationReferenceReadView &, const OriginalApplicationCommand &)>
        before;
    AccessDecision check(const ApplicationReferenceReadView &v,
                         const OriginalApplicationCommand &c) override {
        retained = v;
        try {
            if (before)
                before(v, c);
            decision =
                check_reference_execution(v, c, config, source, adapter_limits, authority_limits);
        } catch (...) {
            witness = std::current_exception();
            throw;
        }
        if (full_accounting)
            return *decision;
        // Component assertion probe stops accounting only AFTER the new helper.
        return ApplicationAccessDenied{};
    }
    bool disclose_application(const ApplicationReferenceReadView &,
                              const OriginalApplicationCommand &,
                              const RecordedApplicationBundle &) override {
        ++disclosures;
        throw std::logic_error("Protected application lookup");
    }
    bool disclose_holds(const ApplicationReferenceReadView &, const OriginalApplicationCommand &,
                        const ApplicationHoldActiveSet &) override {
        ++disclosures;
        throw std::logic_error("Protected holds lookup");
    }
    PolicyResolution resolve(const ApplicationReferenceReadView &,
                             const OriginalApplicationCommand &c, const ApplicationBundleInputs &i,
                             const AccessDecision &a) override;
    ClockResolution observe(const ApplicationReferenceReadView &,
                            const OriginalApplicationCommand &, const ApplicationBundleInputs &,
                            const BoundApplicationPolicy &p) override;
    ActionResolution prepare(const ApplicationReferenceReadView &,
                             const OriginalApplicationCommand &, const ApplicationBundleInputs &,
                             const BoundApplicationPolicy &,
                             const ApplicationCalculatedProposal &) override {
        ++actions;
        return PreparedApplicationActions{{}, "controlled.no-actions"};
    }
    ReferenceResolution issue(ApplicationReferencePurpose purpose,
                              const ApplicationReferenceBinding &b) override {
        ++references;
        return IssuedApplicationReference{b, "rules006-reference-" +
                                                 std::to_string(static_cast<int>(purpose))};
    }
    ApplicationServiceCompletion attempt(Store &d, OriginalApplicationCommand c = command(),
                                         bool rethrow = true) {
        witness = {};
        decision.reset();
        source.observed.reset();
        source.calls = 0;
        config.loaded.clear();
        policies = clocks = disclosures = actions = references = 0;
        auto service_ports = ports();
        OriginalApplicationService service(d.writer, service_ports);
        bool done = false;
        ApplicationServiceCompletion completion;
        require(service.enqueue(c,
                                [&](auto value) {
                                    completion = std::move(value);
                                    done = true;
                                })
                        .error == StorageError::none,
                "Service admits test command");
        require(QTest::qWaitFor([&] { return done; }, 10000), "Actual service completes");
        // Never let service catch-all turn NI into expected work_exception/rollback.
        if (witness) {
            try {
                std::rethrow_exception(witness);
            } catch (const ReferenceApplicationNotImplemented &) {
                throw;
            } catch (const ReferenceRuleNotImplemented &) {
                throw;
            } catch (...) {
                if (rethrow)
                    throw;
            }
        }
        return completion;
    }
};
inline PolicyResolution Ports::resolve(const ApplicationReferenceReadView &,
                                       const OriginalApplicationCommand &c,
                                       const ApplicationBundleInputs &i, const AccessDecision &a) {
    ++policies;
    BoundApplicationPolicy p;
    p.operation = c.operation;
    p.source = {c.subject_kind, c.receipt_id, c.contribution_id};
    p.plan = c.plan;
    p.checked_state_token = i.current_state.token;
    p.configuration_ref = "C";
    p.permission_revision_ref = std::get<ApplicationAccessAllowed>(a).permission_revision_ref;
    p.policy_revision_ref = "W";
    p.maximum_main_ms = cap;
    p.timer_config = configuration().timer_config;
    p.pause_recovery_revision = "controlled.pause";
    p.explanation = "controlled.policy";
    p.admission = {ApplicationPolicyAdmission::allow_active, std::nullopt};
    return p;
}
inline ClockResolution Ports::observe(const ApplicationReferenceReadView &,
                                      const OriginalApplicationCommand &,
                                      const ApplicationBundleInputs &,
                                      const BoundApplicationPolicy &p) {
    ++clocks;
    return ApplicationClockObservation{
        "controlled.lineage", "controlled.observation", "C", "W", p.checked_state_token, 1000,
        {{0, 1000, false}}};
}
inline std::vector<SqlRows> captures(Observer &o) {
    return {o.rows("SELECT * FROM subathon_retained_configurations ORDER BY reference"),
            o.rows("SELECT * FROM subathon_retained_contexts ORDER BY reference"),
            o.rows("SELECT * FROM subathon_plan_revisions ORDER BY plan_ref,revision"),
            o.rows("SELECT * FROM subathon_reference_initial_captures ORDER BY "
                   "installation_ref,receipt_id")};
}
inline void allowed(const Ports &p, const std::string &execution_account = "E") {
    require(p.decision && std::holds_alternative<ApplicationAccessAllowed>(*p.decision) &&
                p.source.observed,
            "New helper permits actual complete current authority");
    require(std::get<ApplicationAccessAllowed>(*p.decision).permission_revision_ref ==
                p.source.observed->permission_revision_ref,
            "Exact fresh producer revision, no locally fabricated permit");
    require(p.source.observed->original_author_input.author == "A" &&
                p.source.observed->permission_input.actor == execution_account &&
                p.source.observed->binding.principal.principal_ref == "U",
            "Original author A and principal U retain the exact execution account");
}
} // namespace reference_rule_fixture
