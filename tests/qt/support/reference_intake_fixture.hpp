// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Independent literal DATA + OLD writer/store APIs. Never seed a test via the
// API-011 initializer, loader, state codec, intake or runtime under test.
#include "sqlite_reference_authority_fixture.hpp"
#include <subathon/server/reference_intake.hpp>
#include <subathon/server/testing/reference_not_implemented.hpp>

namespace reference_intake_fixture {
using namespace reference_authority_fixture;
inline QJsonObject literals() {
    return reference_capture_fixture::load(SUBATHON_FIXTURE_DIRECTORY "/reference-intake-v1.json");
}
inline ReferenceConfigurationFacts facts(std::string ref = "C",
                                         std::optional<std::int64_t> cap = std::nullopt) {
    ReferenceConfigurationFacts f;
    f.configuration_ref = ref;
    f.configuration_owner_revision_ref = "owner-" + ref;
    auto &c = f.projection;
    c.configuration_ref = ref;
    c.policy_revision_ref = "W";
    c.synthetic_origin = {"synthetic.reference.v1", "A"};
    c.receipt_namespace = "N";
    c.rule = request().captured.rule;
    c.maximum_main_ms = cap;
    c.timer_config = {TimerEndMode::Grace, 120000, GraceReuse::Fresh, std::nullopt};
    c.other_rules = c.modifiers = c.milestones = c.outbound_actions = c.generated_events =
        ReferenceFactsState::complete;
    f.rules_state = ReferenceFactsState::complete;
    f.enabled_rules = {
        {c.rule, {"synthetic.reference.support.v1"}, {"time_add"}, {"ms"}, {}, {}, {}}};
    f.modifiers.state = f.milestones.state = f.outbound_actions.state = f.generated_events.state =
        ReferenceFactsState::complete;
    return f;
}
inline std::vector<ReferenceConfigurationVersion>
versions(std::optional<std::int64_t> cap = 3600000) {
    return {{"C", facts()}, {"D", facts("D", cap)}};
}
inline ReferenceCommittedState committed(const char *name = "standard") {
    const auto v = literals()["states"].toObject()[name].toObject();
    require(!v.isEmpty(), "Independent named SAS1 oracle exists");
    const auto ref = v["configuration"].toString().toStdString();
    ReferenceCommittedState r;
    r.snapshot = {"a",
                  {"subathon.internal.application-state.v1",
                   QByteArray::fromHex(v["document_hex"].toString().toLatin1())},
                  ref,
                  {TimerLifecycle::Active, CountdownPhase::Main, v["main"].toString().toLongLong(),
                   120000, false},
                  0,
                  {}};
    if (v.contains("lifecycle"))
        r.snapshot.timer.lifecycle = static_cast<TimerLifecycle>(v["lifecycle"].toInt());
    if (v.contains("phase"))
        r.snapshot.timer.phase = static_cast<CountdownPhase>(v["phase"].toInt());
    r.snapshot.timer.awaiting_confirmation = v["wait"].toBool();
    r.snapshot.time_debt_ms = v["debt"].toString().toLongLong();
    if (v["balance"].toBool())
        r.snapshot.balances = {{ApplicationBalanceKind::score, "S", 1}};
    r.observation = {"L", "O", ref, "W", "a", 1000, {}};
    r.policy_revision_ref = "W";
    return r;
}
inline OriginalApplicationCommand command(std::string key = "attempt") {
    OriginalApplicationCommand c;
    c.operation = {"scope", std::move(key)};
    c.receipt_id = {7};
    c.contribution_id = ContributionId{0};
    c.plan = {"P", 1};
    c.principal = {ApplicationPrincipalKind::automation, "I", "U"};
    c.choices = {
        {"subathon.reference.execution-ref.v1", QByteArray::fromHex("535246310000000159")}};
    return c;
}
struct Sampler final : ReferenceClockSampler {
    ReferenceClockSampleResult next{ReferenceClockSample{100, 1000}};
    unsigned calls = 0;
    ReferenceClockSampleResult sample() override {
        ++calls;
        return next;
    }
};
struct Entropy final : ReferenceEntropy {
    unsigned calls = 0;
    ReferenceEntropyRead next{std::array<std::uint8_t, 32>{}, 32, ReferenceEntropyError::none};
    bool advance = true;
    ReferenceEntropyRead read32() override {
        ++calls;
        auto r = next;
        if (advance)
            r.bytes.fill(static_cast<std::uint8_t>(calls));
        return r;
    }
};
// Structural output checks use literals; do not round-trip through new codecs.
inline void normal(const SqliteWorkResult &r) {
    require(r.transaction == TransactionOutcome::committed && r.error == WorkError::none && r.value,
            "SETUP/positive control: actual commit AND normal value");
}
struct File {
    Database db;
    std::unique_ptr<Observer> observer;
    explicit File(const char *state = "standard", bool install_head = true,
                  std::int64_t amount = 600000) {
        if (amount != 600000)
            seed_amount(amount);
        db.work([](const auto &session) {
            auto g = graph(false);
            for (auto &a : g.accounts)
                if (a.account == "O") {
                    PermissionPolicyEntry e;
                    e.source = "snapshot-fixture";
                    e.grant = "snapshot-use";
                    e.operation = "timer.snapshot.read";
                    e.resources = {{"timer", PermissionResourceMode::allowed_set, {"T"}}};
                    a.individual_entries.push_back(e);
                    (void)session.execute({"UPDATE subathon_permission_accounts SET entries=? "
                                           "WHERE channel=0 AND account_ref=X'4f'",
                                           {encoded(a.individual_entries)}});
                }
        });
        if (install_head) {
            db.work([&](const auto &s) {
                const auto c = committed(state);
                if (c.snapshot.configuration_ref == "D") {
                    const auto configs = literals()["configurations"].toObject();
                    const auto d = configs[configs.contains(state) ? state : "standard"].toObject();
                    auto document = QByteArray::fromHex(d["document_hex"].toString().toLatin1());
                    if (amount != 600000) {
                        QByteArray replacement;
                        const auto bits = static_cast<std::uint64_t>(amount);
                        for (int shift = 56; shift >= 0; shift -= 8)
                            replacement += char((bits >> shift) & 255);
                        document.replace(QByteArray::fromHex("00000000000927c0"), replacement);
                    }
                    require(SqliteRetainedContextStore(s).retain(
                                {{RetainedKind::configuration, "D"}, document, {}}) ==
                                RetainDisposition::inserted,
                            "SETUP: independent current SRC1 D retained by old API");
                }
                require(SqliteApplicationBundleStore(s).retain_initial_state(c.snapshot) ==
                            SqliteBundleInitialDisposition::inserted,
                        "SETUP: OLD state store retains independent SAS1 literal");
            });
        }
        observer = std::make_unique<Observer>(db.path);
    }
    // Independent variant of initial fixture DATA, before any API-011 call.
    // Replace the fixed signed64 field in ALL literal retained copies plus plan,
    // registry and current complete permission domains; never an editorial route.
    void seed_amount(std::int64_t amount) {
        db.work([&](const auto &session) {
            const auto from = QByteArray::fromHex("00000000000927c0");
            QByteArray to;
            const auto bits = static_cast<std::uint64_t>(amount);
            for (int shift = 56; shift >= 0; shift -= 8)
                to += char((bits >> shift) & 255);
            for (const auto *table :
                 {"subathon_retained_configurations", "subathon_retained_contexts"}) {
                for (const auto &row :
                     session.execute({QString("SELECT reference,document FROM ") + table, {}})) {
                    auto doc = row[1].toByteArray();
                    doc.replace(from, to);
                    (void)session.execute(
                        {QString("UPDATE ") + table + " SET document=? WHERE reference=?",
                         {doc, row[0]}});
                }
            }
            (void)session.execute(
                {"UPDATE subathon_plan_requests SET requested_units=?,exact_numerator=?",
                 {QVariant::fromValue<qlonglong>(amount), QByteArray::number(qlonglong(amount))}});
            (void)session.execute(
                {"UPDATE subathon_reference_authority_bindings SET requested_ms=?",
                 {QVariant::fromValue<qlonglong>(amount)}});
            for (int ch = 0; ch != 2; ++ch) {
                auto g = graph(ch == 1);
                for (auto &a : g.accounts) {
                    for (auto &e : a.individual_entries)
                        if (!e.quantities.empty())
                            e.quantities[0].upper = PermissionRational{std::to_string(amount), "1"};
                    (void)session.execute({"UPDATE subathon_permission_accounts SET entries=? "
                                           "WHERE channel=? AND account_ref=?",
                                           {encoded(a.individual_entries), ch, blob(a.account)}});
                }
            }
        });
    }
    qlonglong count(const QString &table) {
        const auto rows = observer->rows("SELECT count(*) FROM " + table);
        require(rows.size() == 1 && rows[0].size() == 1, "Independent SQL count result");
        return rows[0][0].toLongLong();
    }
    ApplicationBundleInputs inputs() {
        ApplicationBundleInputs i;
        db.work([&](const auto &s) {
            const auto r = SqliteApplicationBundleStore(s).read_inputs(
                {"P", 1}, ApplicationSubjectKind::contribution);
            require(r.inputs.has_value(), "SETUP: selected old input reader returns full inputs");
            i = *r.inputs;
        });
        return i;
    }
};
// Direct witness, including inside writer/service catch-all wrappers. An NI can
// NEVER count as the requested input/refusal/rollback result. Setup exceptions are
// separate, and every callback records the exact new call's exception_ptr.
struct Witness {
    std::exception_ptr error;
    template <class F> decltype(auto) call(F &&f) {
        try {
            return f();
        } catch (...) {
            error = std::current_exception();
            throw;
        }
    }
    void rethrow() const {
        if (error)
            std::rethrow_exception(error);
    }
    void rethrow_ni() const {
        if (!error)
            return;
        try {
            std::rethrow_exception(error);
        } catch (const ReferenceNotImplemented &) {
            throw;
        } catch (...) {
        }
    }
};
template <class F> void input_failure(F &&f, ReferenceInputError expected) {
    bool caught = false;
    try {
        f();
    } catch (const ReferenceInputFailure &e) {
        caught = true;
        require(e.code() == expected, "Exact independently specified input error");
    }
    require(caught, "Expected typed input error; NI is NOT accepted");
}
template <class T> struct Completion {
    std::optional<T> result;
    Witness witness;
    unsigned calls = 0;
    void run(SqliteWriter &writer, ReferencePreparedWork p) {
        auto body = std::move(p.body);
        auto observed = [this, body](auto context) -> QVariant {
            return witness.call([&] { return body(context); });
        };
        require(writer.enqueue_work(std::move(observed), std::move(p.completion)) ==
                    StorageError::none,
                "Actual writer admits matching pair");
        require(QTest::qWaitFor([&] { return calls == 1; }, 10000), "Actual typed completion once");
        witness.rethrow_ni();
        require(result.has_value(), "Typed completion exists");
    }
    auto callback() {
        return [this](T r) {
            ++calls;
            result = std::move(r);
        };
    }
};
inline ReferenceInitialRequest initial_request() {
    return {actor('O'), "C",
            "a",        "L",
            "O",        {TimerLifecycle::Active, CountdownPhase::Main, 3480000, 120000, false}};
}
inline ReferenceConnectorRegistration connector() {
    return {"Q", {"synthetic.reference.v1", "A"}, "N"};
}
} // namespace reference_intake_fixture
