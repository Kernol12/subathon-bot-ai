// SPDX-License-Identifier: GPL-3.0-or-later

#include "support/reference_application_adapter_fixture.hpp"
#include "support/session_installation_epoch_fixture.hpp"

namespace {
using namespace installation_epoch_fixture;
using namespace subathon::core;
namespace auth = reference_authority_fixture;
namespace intake = reference_intake_fixture;
namespace runtime = reference_adapter_fixture;

const QStringList graph_tables{"subathon_permission_head", "subathon_permission_accounts",
                               "subathon_permission_roles"};
const QStringList registry_tables{"subathon_reference_authority_head",
                                  "subathon_reference_authority_bindings",
                                  "subathon_reference_authority_events"};
const QStringList authority_tables = identity_tables + graph_tables + registry_tables;
const qsizetype session_rows = identity_tables.indexOf("subathon_account_sessions");
const QStringList application_tables{"subathon_retained_configurations",
                                     "subathon_retained_contexts",
                                     "subathon_app_states",
                                     "subathon_app_head",
                                     "subathon_app_balances",
                                     "subathon_app_operations",
                                     "subathon_app_claims",
                                     "subathon_app_contributions",
                                     "subathon_app_reversals",
                                     "subathon_app_reversal_heads",
                                     "subathon_app_transitions",
                                     "subathon_app_effects",
                                     "subathon_app_components",
                                     "subathon_app_results",
                                     "subathon_app_audit",
                                     "subathon_app_jobs"};
const QStringList intake_tables = authority_tables + application_tables;
const QStringList capture_tables{
    "subathon_receipt_allocator", "subathon_receipts",
    "subathon_receipt_attempts",  "subathon_contribution_allocator",
    "subathon_contributions",     "subathon_contribution_observations",
    "subathon_plan_headers",      "subathon_plan_rules",
    "subathon_plan_revisions",    "subathon_plan_requests",
    "subathon_plan_modifiers",    "subathon_reference_initial_captures"};
const QStringList pipeline_tables = intake_tables + capture_tables;

struct File {
    QTemporaryDir dir;
    QString path{dir.filePath("epoch.sqlite")};
    SqliteWriter writer;
    SqliteReader reader;

    File() {
        require(dir.isValid() &&
                    writer.open(path, account_sessions_fixture::limits()) == StorageError::none,
                "Disposable database opens with its sole writer");
        auto migration =
            permission_graph_fixture::migrate(writer, reference_authority_repository_manifest());
        committed(migration.work);
        require(migration.reason == MigrationReason::none && migration.report &&
                    migration.report->version == 10,
                "Actual development reference catalog is current at step 10");
        require(reader.open(path) == StorageError::none, "Independent read-only observer opens");
        write([](const auto &session) {
            seed_identity(session, "AEO");
            auth::seed_graph(session);
        });
    }

    SqlRows rows(const QString &sql) {
        auto result = reader.read({sql, {}});
        require(result.error == StorageError::none, "Committed read succeeds");
        return std::move(result.rows);
    }

    template <class F> void write(F body) {
        account_sessions_fixture::run(writer, [&](auto context) -> QVariant {
            const auto session = context.begin();
            Cleanup cleanup{session};
            body(session);
            session.commit();
            return 1;
        });
    }
};

template <class F> void caller_work(auto &order, F body, bool expect_success) {
    std::optional<SqliteWorkResult> completion;
    order.run(
        [&](auto context) -> QVariant {
            const auto session = context.begin();
            Cleanup cleanup{session};
            body(session);
            if (expect_success) {
                session.commit();
                return 1;
            }
            order.unchanged_inside(session);
            session.rollback();
            return {};
        },
        [&](auto value) { completion = std::move(value); });
    require(completion.has_value(), "Matching caller-owned completion");
    if (expect_success)
        committed(*completion);
    else {
        rolled_back(*completion);
        order.unchanged_after();
    }
}

AccountSessionResolution resolve(const SqliteWorkSession &session, const SessionRecordFacts &row,
                                 Clock &clock) {
    SqliteAccountSessionUnit unit(session, {{}});
    return resolve_current_account_session(lookup(row), false, unit, clock);
}

void expect_resolution(File &file, const SessionRecordFacts &row,
                       std::optional<SessionFailure> failure = {}) {
    file.write([&](const auto &session) {
        Clock clock;
        const auto result = resolve(session, row, clock);
        if (failure)
            require(selected<SessionFailure>(result) == *failure && clock.calls == 0,
                    "Exact ordinary resolver refusal before clock sampling");
        else {
            exact_session(result, row);
            require(clock.calls == 1, "Exactly one resolved-session clock sample");
        }
    });
}

void resolver_case(Order timing) {
    File file;
    const auto saved = record();
    Clock clock;
    Ordering order(file, file.writer, timing, authority_tables);
    caller_work(
        order,
        [&](const auto &session) {
            const auto result = resolve(session, saved, clock);
            if (order.current())
                exact_session(result, saved);
            else
                require(selected<SessionFailure>(result) == SessionFailure::revoked,
                        "An intact E row resolves to revoked under F");
            require(clock.calls == (order.current() ? 1u : 0u), "Exact session-clock samples");
        },
        order.current());
    order.unchanged_after();
}

enum class Registry { bootstrap = 1, enroll, authorize, revoke };

SqlRow binding_row() {
    return {blob("I"), blob("K"), blob("U"),
            blob("R"), blob("r"), blob("a"),
            blob("b"), blob("T"), blob("synthetic.reference.v1"),
            blob("A"), blob("N"), qlonglong(600000),
            blob("A"), blob("E"), auth::encoded(std::uint64_t{2})};
}

SqlRow event_row(std::uint64_t revision) {
    const bool author = revision == 2;
    return {blob("I"),
            blob("K"),
            auth::encoded(revision),
            int(revision - 2),
            blob(author ? "A" : "O"),
            blob(author ? "sA" : "sO"),
            blob("v"),
            qlonglong(1000)};
}

void seed_registry(const SqliteWorkSession &session, std::uint64_t revision) {
    if (revision == 0)
        return;
    (void)session.execute({"INSERT INTO subathon_reference_authority_head VALUES(X'49',?)",
                           {auth::encoded(revision)}});
    if (revision == 1)
        return;
    (void)session.execute({"INSERT INTO subathon_reference_authority_bindings "
                           "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                           binding_row()});
    for (std::uint64_t event = 2; event <= revision; ++event)
        (void)session.execute(
            {"INSERT INTO subathon_reference_authority_events VALUES(?,?,?,?,?,?,?,?)",
             event_row(event)});
}

void exact_registry(File &file, std::uint64_t revision) {
    const SqlRows head = revision ? SqlRows{{blob("I"), auth::encoded(revision)}} : SqlRows{};
    const SqlRows bindings = revision >= 2 ? SqlRows{binding_row()} : SqlRows{};
    SqlRows history;
    for (std::uint64_t event = 2; event <= revision; ++event)
        history.append(event_row(event));
    require(file.rows("SELECT * FROM subathon_reference_authority_head") == head &&
                file.rows("SELECT * FROM subathon_reference_authority_bindings") == bindings &&
                file.rows("SELECT * FROM subathon_reference_authority_events ORDER BY revision") ==
                    history,
            "Exact registry head, immutable binding and complete actor/session audit history");
}

ReferenceAuthorityWrite registry_command(const SqliteWorkSession &session, Registry command,
                                         Clock &clock) {
    SqliteReferenceAuthorityUnit unit(session);
    const auto owner = lookup(record('O'));
    switch (command) {
    case Registry::bootstrap:
        return unit.bootstrap(owner, clock);
    case Registry::enroll:
        return unit.enroll(lookup(record()), clock, 1, auth::binding(), "E");
    case Registry::authorize:
        return unit.set_state(owner, clock, 2, "K", ReferenceAuthorityState::authorized);
    case Registry::revoke:
        return unit.set_state(owner, clock, 3, "K", ReferenceAuthorityState::revoked);
    }
    throw std::logic_error("Invalid test registry command");
}

void registry_case(Order timing, Registry command) {
    File file;
    const auto revision = static_cast<std::uint64_t>(command);
    file.write([&](const auto &session) { seed_registry(session, revision - 1); });
    exact_registry(file, revision - 1);
    Clock clock;
    Ordering order(file, file.writer, timing, authority_tables);
    caller_work(
        order,
        [&](const auto &session) {
            const auto result = registry_command(session, command, clock);
            if (order.current())
                require(selected<ReferenceAuthorityStaged>(result).revision == revision,
                        "Exact provisional registry revision");
            else
                require(selected<ReferenceAuthorityFailure>(result) ==
                            ReferenceAuthorityFailure::denied,
                        "Epoch mismatch refuses registry authority without a staged revision");
            require(clock.calls == (order.current() ? 1u : 0u), "Exact session-clock samples");
        },
        order.current());
    exact_registry(file, order.current() ? revision : revision - 1);
    if (order.current()) {
        auto expected = order.baseline();
        const auto registry =
            read_image(registry_tables, [&](const auto &sql) { return file.rows(sql); });
        for (qsizetype i = 0; i < registry.size(); ++i)
            expected[identity_tables.size() + graph_tables.size() + i] = registry[i];
        require(order.image() == expected, "Registry success only changes registry tables");
    }
}

void seed_authenticator(File &file) {
    file.write([](const auto &session) {
        // Public synthetic verifier from the independently retained account-session fixtures.
        (void)session.execute(
            {"INSERT INTO subathon_local_credentials VALUES(X'49',X'41',X'4c',1,?)",
             {blob("$argon2id$v=19$m=4882,t=2,p=1$bA81arsiXysd3WbTRzmEOw$"
                   "Nm8QBM+7RH1DXo9rvp5cwKEOOOfD2g6JuxlXihoNcpE")}});
    });
}

AcceptedSessionAuthentication acceptance(std::string epoch = "E", std::int64_t accepted = 1000) {
    return {"I",
            "A",
            "attempt",
            std::move(epoch),
            "C",
            "K",
            "L",
            "acceptance",
            AcceptedSignInMethod::local_password,
            accepted};
}

enum class SessionWrite { insert, token_only, fresh };

void session_write_case(Order timing, SessionWrite command) {
    File file;
    seed_authenticator(file);
    const auto old = record();
    auto candidate = old;
    candidate.digest.fill(0x42);
    candidate.record_revision_ref = "new";
    const bool inserting = command == SessionWrite::insert;
    const bool fresh = command == SessionWrite::fresh;
    const auto retention = fresh ? SessionRetention::remembered : SessionRetention::normal;
    if (inserting)
        candidate.session_ref = "candidate";
    if (fresh) {
        candidate.lifetime_started_utc_ms = candidate.authenticated_utc_ms = 2000;
        candidate.expires_utc_ms = 2592002000;
    }
    const auto accepted = acceptance("E", candidate.authenticated_utc_ms);
    Ordering order(file, file.writer, timing, authority_tables);
    caller_work(
        order,
        [&](const auto &session) {
            SqliteAccountSessionUnit unit(session, {{}});
            const auto disposition = inserting ? unit.insert_session(candidate, retention, accepted)
                                               : unit.replace_session(old, candidate, retention,
                                                                      fresh ? &accepted : nullptr);
            require(disposition == (order.current() ? LifecycleWriteResult::staged
                                                    : LifecycleWriteResult::stale),
                    "Exact physical disposition, no credential supplied by the component");
        },
        order.current());
    if (!order.current()) {
        expect_resolution(file, candidate, SessionFailure::missing);
        expect_resolution(file, old, SessionFailure::revoked);
        return;
    }

    auto expected = order.baseline();
    const SqlRow new_row = session_row(candidate, retention);
    if (inserting)
        expected[session_rows].append(new_row);
    else
        expected[session_rows][0] = new_row;
    require(order.image() == expected, "Only the exact intended digest-only session row changes");
    expect_resolution(file, candidate);
    if (!inserting)
        expect_resolution(file, old, SessionFailure::missing);

    rotate(file, file.writer, authority_tables);
    expect_resolution(file, candidate, SessionFailure::revoked);
    if (inserting) {
        auto independent = candidate;
        independent.session_ref = "independent-F";
        independent.installation_epoch_ref = "F";
        independent.record_revision_ref = "fresh-F";
        independent.digest.fill(0x43);
        file.write([&](const auto &session) {
            require(SqliteAccountSessionUnit(session, {{}})
                            .insert_session(independent, SessionRetention::normal,
                                            acceptance("F")) == LifecycleWriteResult::staged,
                    "Independent accepted F candidate uses a distinct identity and digest");
        });
        expected[0] = installation_row(true);
        expected[session_rows].append(session_row(independent));
        require(order.image() == expected,
                "Independent F issuance leaves the retained E row in place");
        expect_resolution(file, independent);
    }
}

void stale_acceptance_with_current_f() {
    File file;
    seed_authenticator(file);
    rotate(file, file.writer, authority_tables);
    auto old = record();
    old.session_ref = "independent-F";
    old.installation_epoch_ref = "F";
    old.digest.fill(0x44);
    file.write([&](const auto &session) {
        require(SqliteAccountSessionUnit(session, {{}})
                        .insert_session(old, SessionRetention::normal, acceptance("F")) ==
                    LifecycleWriteResult::staged,
                "Separate ordinary accepted F row");
    });
    expect_resolution(file, old);

    auto candidate = old;
    candidate.record_revision_ref = "second-F";
    candidate.digest.fill(0x46);
    candidate.lifetime_started_utc_ms = candidate.authenticated_utc_ms = 2000;
    candidate.expires_utc_ms = 2592002000;
    const auto epoch = [&](const auto &sql) { return file.rows(sql); };
    auto expected = read_image(authority_tables, epoch);
    std::optional<LifecycleWriteResult> provisional;
    const auto stale =
        account_sessions_fixture::outcome(file.writer, [&](auto context) -> QVariant {
            const auto session = context.begin();
            Cleanup cleanup{session};
            const auto old_acceptance = acceptance("E", 2000);
            provisional =
                SqliteAccountSessionUnit(session, {{}})
                    .replace_session(old, candidate, SessionRetention::remembered, &old_acceptance);
            require(active_image(session, authority_tables) == expected,
                    "Stale E acceptance for a current F target stages nothing");
            session.rollback();
            return {};
        });
    rolled_back(stale);
    require(provisional == LifecycleWriteResult::stale &&
                read_image(authority_tables, epoch) == expected,
            "E acceptance independently refuses a current F record");
    file.write([&](const auto &session) {
        const auto current = acceptance("F", 2000);
        require(SqliteAccountSessionUnit(session, {{}})
                        .replace_session(old, candidate, SessionRetention::remembered, &current) ==
                    LifecycleWriteResult::staged,
                "Same fresh replacement with current F acceptance is eligible");
    });
    expected[session_rows].back() = session_row(candidate, SessionRetention::remembered);
    require(read_image(authority_tables, epoch) == expected,
            "Fresh F acceptance changes exactly its own row");
    expect_resolution(file, candidate);
    expect_resolution(file, old, SessionFailure::missing);
}

ReferenceCommittedState timer_state(const char *configuration) {
    auto state = intake::committed(std::string_view(configuration) == "C" ? "initial" : "standard");
    state.snapshot.timer.main_remaining_ms = 120000;
    state.observation.observed_utc_ms = 3000;
    // Independent SAS1 layout: UTC 1000 -> 3000 and Main 3480000 -> 120000.
    state.snapshot.envelope.document.replace(QByteArray::fromHex("00000000000003e8"),
                                             QByteArray::fromHex("0000000000000bb8"));
    state.snapshot.envelope.document.replace(QByteArray::fromHex("00000000003519c0"),
                                             QByteArray::fromHex("000000000001d4c0"));
    return state;
}

QByteArray capped_config_document(char ref) {
    const auto json = intake::literals()["configurations"].toObject()["standard"].toObject();
    auto bytes = QByteArray::fromHex(json["document_hex"].toString().toLatin1());
    // Independent SRC1: one-byte configuration identity and the literal cap field.
    require(bytes.startsWith(QByteArray::fromHex("535243310000000144")) &&
                bytes.mid(100, 8) == QByteArray::fromHex("000000000036ee80"),
            "Protected SRC1 template matches fixed field positions");
    bytes[8] = ref;
    bytes.replace(100, 8, QByteArray::fromHex("000000000002bf20"));
    return bytes;
}

void seed_timer(File &file, const ReferenceCommittedState &state, const QByteArray &document) {
    file.write([&](const auto &session) {
        require(
            SqliteRetainedContextStore(session).retain(
                {{RetainedKind::configuration, state.snapshot.configuration_ref}, document, {}}) ==
                    RetainDisposition::inserted &&
                SqliteApplicationBundleStore(session).retain_initial_state(state.snapshot) ==
                    SqliteBundleInitialDisposition::inserted,
            "Independent retained configuration and literal state via older storage primitives");
    });
}

void allow_snapshot(File &file) {
    file.write([](const auto &session) {
        auto entries = auth::profile();
        PermissionPolicyEntry allow;
        allow.source = "snapshot-fixture";
        allow.grant = "snapshot-use";
        allow.operation = "timer.snapshot.read";
        allow.resources = {{"timer", PermissionResourceMode::allowed_set, {"T"}}};
        entries.push_back(allow);
        (void)session.execute({"UPDATE subathon_permission_accounts SET entries=? "
                               "WHERE channel=0 AND account_ref=X'4f'",
                               {auth::encoded(entries)}});
    });
}

struct Configuration final : ReferenceConfigurationSource {
    explicit Configuration(std::vector<ReferenceConfigurationVersion> versions)
        : archive(std::move(versions)) {}
    ReferenceConfigurationArchive archive;
    unsigned snapshots = 0, loads = 0;
    ReferenceConfigurationResolution snapshot(const std::string &ref) override {
        ++snapshots;
        return archive.snapshot(ref);
    }
    ReferenceConfigurationResolution load(const ApplicationReferenceReadView &view,
                                          const std::string &ref) override {
        ++loads;
        return archive.load(view, ref);
    }
};

template <class Result> struct Completion {
    std::optional<Result> result;
    auto callback() {
        return [&](Result value) {
            require(!result, "Matching typed completion is delivered once");
            result = std::move(value);
        };
    }
    const Result &get() const {
        require(result.has_value(), "Matching typed result was delivered");
        return *result;
    }
};

template <class Result> void denied(const Result &result) {
    rolled_back(result.work);
    require(result.refusal && result.refusal->gate == ReferenceGate::denied &&
                result.refusal->reason_ref == "reference.access-denied.v1",
            "Exact bounded access diagnostic");
}

void startup_case(Order timing) {
    File file;
    Clock clock;
    Configuration configuration({{"C", intake::facts("C", 180000)}});
    intake::Sampler sampler;
    sampler.next = ReferenceClockSample{100, 3000};
    intake::Entropy entropy;
    ReferenceIntake unit(configuration, sampler, entropy, clock, intake::connector());
    const auto state = timer_state("C");
    const ReferenceInitialRequest request{lookup(record('O')), "C", "a", "L", "O",
                                          state.snapshot.timer};
    Completion<ReferenceInitialCompletion> completion;
    auto work = unit.make_initial_work(request, completion.callback());
    Ordering order(file, file.writer, timing, intake_tables);
    order.run(std::move(work.body), std::move(work.completion));
    const auto &result = completion.get();
    require(configuration.snapshots == 1 && configuration.loads == 0 && entropy.calls == 0,
            "One pre-authentication configuration admission and no reference allocations");
    if (!order.current()) {
        denied(result);
        require(!result.anchor && clock.calls == 0 && sampler.calls == 0,
                "No startup anchor, startup eligibility or clock sample");
        order.unchanged_after();
        return;
    }
    committed(result.work);
    require(result.anchor && !result.refusal && sampler.calls == 1 && clock.calls == 1,
            "Matching completion releases one initialized anchor");
    runtime::same_state(result.anchor->committed, state);
    require(result.anchor->sample.monotonic_ms == 100 && result.anchor->sample.utc_ms == 3000,
            "Exact live sample on the anchor");
    require(file.rows("SELECT token FROM subathon_app_head") == SqlRows{{blob("a")}} &&
                file.rows(
                    "SELECT token,configuration_ref,main_remaining_ms,grace_remaining_ms,document "
                    "FROM subathon_app_states") ==
                    SqlRows{{blob("a"), blob("C"), qlonglong(120000), qlonglong(120000),
                             state.snapshot.envelope.document}} &&
                file.rows("SELECT reference,document FROM subathon_retained_configurations") ==
                    SqlRows{{blob("C"), capped_config_document('C')}},
            "Exactly one committed timer, head and configuration with independent bytes");
    const auto image = order.image();
    for (qsizetype i = 0; i < intake_tables.size(); ++i)
        if (intake_tables[i] != "subathon_app_head" && intake_tables[i] != "subathon_app_states" &&
            intake_tables[i] != "subathon_retained_configurations")
            require(image[i] == order.baseline()[i], "No unrelated startup mutation");
}

void snapshot_case(Order timing) {
    File file;
    allow_snapshot(file);
    const auto state = timer_state("D");
    seed_timer(file, state, capped_config_document('D'));
    Clock clock;
    // The actor is older than the freshness limit and still well within session lifetime.
    clock.now = 302000;
    Configuration configuration(intake::versions(180000));
    intake::Sampler sampler;
    intake::Entropy entropy;
    ReferenceIntake unit(configuration, sampler, entropy, clock, intake::connector());
    Completion<ReferenceSnapshotCompletion> completion;
    auto work = unit.make_snapshot_work(lookup(record('O')), completion.callback());
    Ordering order(file, file.writer, timing, intake_tables);
    order.run(std::move(work.body), std::move(work.completion));
    const auto &result = completion.get();
    require(sampler.calls == 0 && entropy.calls == 0,
            "No clock advancement or allocated references");
    if (order.current()) {
        committed(result.work);
        require(result.snapshot && !result.refusal && clock.calls == 1 &&
                    result.snapshot->maximum_main_ms == 180000,
                "Explicit ordinary timer USE permits the unexpired nonfresh snapshot");
        runtime::same_state(result.snapshot->committed, state);
    } else {
        denied(result);
        require(!result.snapshot && clock.calls == 0 && configuration.snapshots == 0 &&
                    configuration.loads == 0,
                "No protected snapshot, configuration projection or snapshot continuation");
    }
    order.unchanged_after();
}

void stale_target_cleanup() {
    File file;
    const auto old = record();
    auto revoked = old;
    revoked.record_revision_ref = "cleanup";
    revoked.state = SessionRecordState::revoked;
    Ordering order(file, file.writer, Order::after_rotation, authority_tables);
    caller_work(
        order,
        [&](const auto &session) {
            SqliteAccountSessionUnit unit(session, {{}});
            const auto result = unit.read_selected("I", "A", "sA");
            const auto &facts = selected<CompleteSessionFacts>(result);
            require(facts.session.installation_epoch_ref == "E" &&
                        facts.installation.session_epoch_ref == "F" &&
                        facts.account.session_epoch_ref == "C" &&
                        facts.account.application_access_enabled,
                    "Physical selection supplies stale target and current linked parents");
            const auto by_digest = unit.load_current(lookup(old), {});
            require(selected<CompleteSessionFacts>(by_digest).session.digest == old.digest &&
                        selected<CompleteSessionFacts>(by_digest).installation.session_epoch_ref ==
                            "F",
                    "Physical digest supplier retains stale facts for the resolver");
            const auto lifecycle = unit.read_lifecycle_actor(lookup(old), {});
            require(selected<LifecycleCurrentSession>(lifecycle)
                                .current.session.installation_epoch_ref == "E" &&
                        selected<LifecycleCurrentSession>(lifecycle).retention ==
                            SessionRetention::normal,
                    "Lifecycle fact supplier still selects the physical E record");
            const auto page = unit.read_page("I", "A", {}, 2);
            const auto &physical = selected<LifecyclePageFacts>(page);
            require(physical.sessions.size() == 1 &&
                        session_row(physical.sessions[0]) == session_row(old) &&
                        physical.installation.session_epoch_ref == "F",
                    "Internal page may still select the exact retained E target");
            require(
                unit.replace_installation_epoch({{"I", "E"}, "J"}, "unused", "unused-revision") ==
                    LifecycleWriteResult::stale,
                "Operational CAS with a stale expectation refuses independently");
            order.unchanged_inside(session);
            require(unit.revoke_session(old, "cleanup") == LifecycleWriteResult::staged,
                    "Operational caller can revoke the exact stale target");
        },
        true);
    auto expected = order.baseline();
    expected[session_rows][0] = session_row(revoked);
    require(order.image() == expected, "Cleanup changes only target state and revision");
    expect_resolution(file, old, SessionFailure::revoked);
}

enum class ConnectorStep { receipt, evidence, capture };

void connector_case(Order timing, ConnectorStep target) {
    File file;
    const auto state = timer_state("D");
    seed_timer(file, state, capped_config_document('D'));
    Clock clock;
    Configuration configuration(intake::versions(180000));
    intake::Sampler sampler;
    sampler.next = ReferenceClockSample{100, 3000};
    intake::Entropy entropy;
    SqliteReferenceAuthoritySource authority;
    ReferenceApplicationAdapters adapters(configuration, authority, sampler, entropy,
                                          ReferenceClockAnchor{state, {100, 3000}});
    ReferenceIntake unit(configuration, sampler, entropy, clock, intake::connector(), &adapters);
    auto run = [&](ConnectorStep step, ReferencePreparedWork work) {
        if (step == target) {
            Ordering order(file, file.writer, timing, pipeline_tables);
            order.run(std::move(work.body), std::move(work.completion));
        } else {
            const auto raw = qt_test_support::run_work(file.writer, std::move(work.body), 10000);
            work.completion(raw);
        }
    };

    const ReferenceSyntheticInput input{"Q", "connector-event", 1500,
                                        QByteArray("synthetic audit")};
    const std::string arrival = "RCT1" + std::string(32, '\1');
    const std::string event = "REV1" + std::string(32, '\1');
    const std::string plan = "RPL1" + std::string(32, '\2');
    const std::string execution = "REX1" + std::string(32, '\3');
    Completion<ReferenceReceiptCompletion> receipt;
    run(ConnectorStep::receipt, unit.make_receipt_work("D", input, receipt.callback()));
    const auto &r = receipt.get();
    committed(r.work);
    require(r.receipt && r.receipt->receipt_id.value == 1 &&
                r.receipt->disposition == ReceiptDisposition::created && r.original && !r.refusal &&
                r.original->arrival_context_ref == arrival && r.original->event_ref == event &&
                r.original->server_arrival_utc_ms == 3000 &&
                r.original->configuration.configuration_ref == "D" &&
                r.original->observation.original_payload == "synthetic audit",
            "Connector registration and live runtime authorize exact new receipt and arrival");

    Completion<ReferenceEvidenceCompletion> evidence;
    run(ConnectorStep::evidence,
        unit.make_evidence_work(r.receipt->receipt_id, evidence.callback()));
    const auto &e = evidence.get();
    committed(e.work);
    require(e.evidence && e.evidence->disposition == ContributionLinkDisposition::created &&
                e.evidence->contribution_id && e.evidence->contribution_id->value == 0 &&
                !e.refusal,
            "Retained receipt authorizes the exact new evidence link");

    Completion<ReferenceCaptureCompletion> capture;
    run(ConnectorStep::capture,
        unit.make_capture_work(r.receipt->receipt_id, *e.evidence->contribution_id,
                               capture.callback()));
    const auto &c = capture.get();
    committed(c.work);
    require(c.capture && !c.refusal && c.capture->key.installation_ref == "I" &&
                c.capture->key.source.origin == intake::connector().origin &&
                c.capture->key.source.identifier_namespace == "N" &&
                c.capture->key.source.opaque_id == "connector-event" &&
                c.capture->receipt_id.value == 1 && c.capture->contribution_id.value == 0 &&
                c.capture->configuration_ref == "D" && c.capture->arrival_context_ref == arrival &&
                c.capture->event_ref == event && c.capture->plan.plan_ref == plan &&
                c.capture->plan.revision == 1 && c.capture->execution_context_ref == execution,
            "Exact retained connector capture closure and plan identity");
    require(file.rows("SELECT receipt_id,opaque_id,configuration_ref,modifier_context_ref "
                      "FROM subathon_receipts") ==
                    SqlRows{{auth::encoded(std::uint64_t{1}), blob("connector-event"), blob("D"),
                             blob(arrival)}} &&
                file.rows(
                    "SELECT receipt_id,contribution_id FROM subathon_contribution_observations") ==
                    SqlRows{{auth::encoded(std::uint64_t{1}), auth::encoded(std::uint64_t{0})}} &&
                file.rows("SELECT plan_ref,plan_revision,execution_context_ref "
                          "FROM subathon_reference_initial_captures") ==
                    SqlRows{{blob(plan), auth::encoded(std::uint64_t{1}), blob(execution)}} &&
                file.rows("SELECT token FROM subathon_app_head") == SqlRows{{blob("a")}} &&
                file.rows("SELECT * FROM subathon_app_operations").isEmpty() && clock.calls == 0 &&
                sampler.calls == 1 && entropy.calls == 3,
            "Exact committed connector links, without application or session authority");
}

std::string permission_fingerprint(bool rotated) {
    // RAR1 framing from reference-authority-v1, with three unchanged enabled
    // accounts and the independently framed capability fingerprint.
    auto bytes = QByteArray::fromHex("5241523100000001490000000000000001"
                                     "000000177065726d697373696f6e2d7265666572656e63652d7631"
                                     "000000014f");
    bytes += QByteArray::fromHex(rotated ? "0000000146000000014e" : "0000000145000000014a");
    bytes += QByteArray::fromHex("0000000300000001410000000143000000014b01"
                                 "00000001450000000143000000014b01"
                                 "000000014f0000000143000000014b01");
    bytes += QByteArray::fromHex("00000024");
    bytes += blob(auth::automation_revision());
    return (QByteArray("RAR1") + QCryptographicHash::hash(bytes, QCryptographicHash::Sha256))
        .toStdString();
}

void seed_automation(File &file) {
    file.write([](const auto &session) {
        seed_registry(session, 3);
        const reference_capture_fixture::Data captures;
        captures.intake(session, "A");
        captures.stage_capture(session, "A");
        captures.index(session, "A");
    });
}

void expect_automation_authority(File &file, bool rotated) {
    file.write([&](const auto &session) {
        const auto resolution = SqliteReferenceAuthorityUnit(session).resolve(auth::request());
        const auto &facts = auth::facts(resolution);
        auth::permitted(facts);
        require(
            facts.permission_revision_ref == permission_fingerprint(rotated) &&
                facts.automation_revision_ref == auth::automation_revision() &&
                facts.registry.actor == "O" && facts.registry.session == "sO" &&
                facts.registry.session_revision == "v",
            "Current permissions change fingerprint with I; historical audit and grant survive");
        const auto read = SqliteReferenceAuthorityUnit(session).read("I", "K");
        require(selected<ReferenceAuthorityRecord>(read).state ==
                        ReferenceAuthorityState::authorized &&
                    selected<ReferenceAuthorityRecord>(read).head_revision == 3,
                "Physical registry still exposes the exact historical authorized state internally");
    });
}

const ApplicationServiceResponse &service_result(const ApplicationServiceCompletion &result,
                                                 ApplicationServiceDisposition disposition) {
    require(result.transaction == TransactionOutcome::committed &&
                result.work_error == WorkError::none && !result.failure && result.response &&
                result.response->disposition == disposition,
            "Actual matching service completion exposes only its authorized automation result");
    return *result.response;
}

void exact_effect(const RecordedApplicationBundle &bundle) {
    require(bundle.intent.operation.operation_scope == "scope" &&
                bundle.intent.operation.operation_ref == "attempt" &&
                bundle.after.timer.main_remaining_ms == 180000 &&
                bundle.advanced_before.timer.main_remaining_ms == 120000 &&
                bundle.effects.size() == 1 && bundle.jobs.empty(),
            "Exact automation operation, capped timer and one effect");
    const auto &effect = bundle.effects.front();
    require(effect.request_ordinal == 1 && effect.exact_requested_units == Exact{600000} &&
                effect.requested_units == 600000 && effect.components.size() == 3 &&
                effect.components[0].kind == ApplicationComponentKind::main_delta &&
                effect.components[0].units == 60000 &&
                effect.components[1].kind == ApplicationComponentKind::debt_delta &&
                effect.components[1].units == 0 &&
                effect.components[2].kind == ApplicationComponentKind::cap_limited_time &&
                effect.components[2].units == 540000,
            "Independent exact credit, debt and cap accounting");
}

void automation_case(Order timing, bool retry) {
    File file;
    seed_automation(file);
    const auto state = timer_state("D");
    seed_timer(file, state, capped_config_document('D'));
    expect_automation_authority(file, false);
    Configuration config(intake::versions(180000));
    SqliteReferenceAuthoritySource authority;
    intake::Sampler sampler;
    sampler.next = ReferenceClockSample{100, 3000};
    intake::Entropy entropy;
    ReferenceApplicationAdapters adapters(config, authority, sampler, entropy,
                                          ReferenceClockAnchor{state, {100, 3000}});
    runtime::Ports ports(adapters);
    ApplicationServicePorts service_ports{ports, ports, ports, ports, ports};
    OriginalApplicationService service(file.writer, service_ports);
    const auto command = intake::command();
    auto apply = [&](Order when) {
        Completion<ApplicationServiceCompletion> completion;
        auto work = service.make_work(command, completion.callback());
        auto body = std::move(work.body);
        Ordering order(file, file.writer, when, pipeline_tables);
        order.run(
            [&](auto context) {
                require(!adapters.reserve(state, command),
                        "Live reservation begins after FIFO wait");
                return body(context);
            },
            std::move(work.completion));
        ports.witness.rethrow();
        const auto &result = completion.get();
        const auto &bundle = selected<RecordedApplicationBundle>(
            service_result(result, ApplicationServiceDisposition::applied).payload);
        exact_effect(bundle);
        const auto after = decode_reference_state(bundle.after);
        adapters.settle(result, after);
        return bundle;
    };

    const auto saved = apply(retry ? Order::current : timing);
    if (retry) {
        Completion<ApplicationServiceCompletion> completion;
        auto work = service.make_work(command, completion.callback());
        Ordering order(file, file.writer, timing, pipeline_tables);
        order.run(std::move(work.body), std::move(work.completion));
        ports.witness.rethrow();
        const auto &retained = selected<RecordedApplicationBundle>(
            service_result(completion.get(), ApplicationServiceDisposition::existing_operation)
                .payload);
        exact_effect(retained);
        require(retained.result.format_ref == saved.result.format_ref &&
                    retained.result.document == saved.result.document &&
                    retained.after.token == saved.after.token &&
                    retained.after.envelope.document == saved.after.envelope.document &&
                    sampler.calls == 1,
                "Authorized retry discloses the exact first result without advancing time");
        order.unchanged_after();
    }
    require(file.rows("SELECT token FROM subathon_app_head") ==
                    SqlRows{{blob(saved.after.token)}} &&
                file.rows("SELECT * FROM subathon_app_operations").size() == 1 &&
                file.rows("SELECT * FROM subathon_app_claims").size() == 1,
            "One committed automation effect and claim, including after F");
    expect_automation_authority(file, timing != Order::current);
}

void hold_disclosure_case(Order timing) {
    File file;
    seed_automation(file);
    const auto state = timer_state("D");
    seed_timer(file, state, capped_config_document('D'));
    file.write([&](const auto &session) {
        ApplicationHoldDetection hold;
        hold.cause.hold_ref = "hold";
        hold.cause.source = {{ContributionId{0}}, {7}, "V", {}, "V", "C", "X"};
        hold.cause.command = {{"history", "app"}, auth::historical_command()};
        hold.cause.first_observation.persisted_state_token = "a";
        hold.cause.first_observation.annotation = {"opaque.annotation", QByteArray("", 0)};
        hold.cause.plan_evidence.plan = ApplicationPlanKey{"P", 1};
        hold.cause.plan_evidence.occurrences = {{1, "a", "b", "R", "T", RequestedUnit::milliseconds,
                                                 Exact{600000}, 600000, "reference.integer-ms.v1"}};
        hold.cause.kind = ApplicationHoldCauseKind::unresolved_policy;
        hold.cause.missing_policy_reason = "independent.fixture.hold";
        hold.request = {"detection",
                        {},
                        "version",
                        hold.cause.first_observation,
                        {"opaque.audit", QByteArray("", 0)}};
        require(SqliteApplicationHoldStore(session).record_detection(hold).disposition ==
                    SqliteHoldMutationDisposition::recorded,
                "Independent active hold retained via real storage");
    });
    Configuration config(intake::versions(180000));
    SqliteReferenceAuthoritySource authority;
    intake::Sampler sampler;
    intake::Entropy entropy;
    ReferenceApplicationAdapters adapters(config, authority, sampler, entropy, {});
    runtime::Ports ports(adapters);
    ApplicationServicePorts service_ports{ports, ports, ports, ports, ports};
    OriginalApplicationService service(file.writer, service_ports);
    Completion<ApplicationServiceCompletion> completion;
    auto work = service.make_work(intake::command(), completion.callback());
    QStringList tables = pipeline_tables;
    tables.append(
        {"subathon_hold_detections", "subathon_hold_versions", "subathon_hold_mutations"});
    Ordering order(file, file.writer, timing, tables);
    order.run(std::move(work.body), std::move(work.completion));
    ports.witness.rethrow();
    const auto &payload = selected<ApplicationActiveHoldPayload>(
        service_result(completion.get(), ApplicationServiceDisposition::active_hold).payload);
    require(payload.holds && payload.holds->holds.size() == 1 &&
                payload.holds->holds.front().cause.hold_ref == "hold" &&
                payload.holds->holds.front().cause.missing_policy_reason ==
                    "independent.fixture.hold" &&
                sampler.calls == 0 && entropy.calls == 0,
            "Current automation disclosure checks permit only the literal active hold");
    order.unchanged_after();
}
} // namespace

class SessionInstallationEpochTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void resolver_data() { order_rows(); }
    void resolver() {
        QFETCH(int, order);
        scenario([&] { resolver_case(Order(order)); });
    }

    void startup_data() { order_rows(); }
    void startup() {
        QFETCH(int, order);
        scenario([&] { startup_case(Order(order)); });
    }
    void snapshot_data() { order_rows(); }
    void snapshot() {
        QFETCH(int, order);
        scenario([&] { snapshot_case(Order(order)); });
    }

    void bootstrap_data() { order_rows(); }
    void bootstrap() {
        QFETCH(int, order);
        scenario([&] { registry_case(Order(order), Registry::bootstrap); });
    }
    void enroll_data() { order_rows(); }
    void enroll() {
        QFETCH(int, order);
        scenario([&] { registry_case(Order(order), Registry::enroll); });
    }
    void authorize_data() { order_rows(); }
    void authorize() {
        QFETCH(int, order);
        scenario([&] { registry_case(Order(order), Registry::authorize); });
    }
    void revoke_data() { order_rows(); }
    void revoke() {
        QFETCH(int, order);
        scenario([&] { registry_case(Order(order), Registry::revoke); });
    }

    void insert_data() { order_rows(); }
    void insert() {
        QFETCH(int, order);
        scenario([&] { session_write_case(Order(order), SessionWrite::insert); });
    }
    void tokenOnlyReplacement_data() { order_rows(); }
    void tokenOnlyReplacement() {
        QFETCH(int, order);
        scenario([&] { session_write_case(Order(order), SessionWrite::token_only); });
    }
    void freshReplacement_data() { order_rows(); }
    void freshReplacement() {
        QFETCH(int, order);
        scenario([&] { session_write_case(Order(order), SessionWrite::fresh); });
    }

    void freshReplacementRejectsOldAcceptanceForCurrentF() {
        scenario(stale_acceptance_with_current_f);
    }

    void physicalReadersAndTargetCleanup() { scenario(stale_target_cleanup); }

    void connectorReceipt_data() { order_rows(); }
    void connectorReceipt() {
        QFETCH(int, order);
        scenario([&] { connector_case(Order(order), ConnectorStep::receipt); });
    }
    void connectorEvidence_data() { order_rows(); }
    void connectorEvidence() {
        QFETCH(int, order);
        scenario([&] { connector_case(Order(order), ConnectorStep::evidence); });
    }
    void connectorCapture_data() { order_rows(); }
    void connectorCapture() {
        QFETCH(int, order);
        scenario([&] { connector_case(Order(order), ConnectorStep::capture); });
    }

    void automationApply_data() { order_rows(); }
    void automationApply() {
        QFETCH(int, order);
        scenario([&] { automation_case(Order(order), false); });
    }
    void automationRetry_data() { order_rows(); }
    void automationRetry() {
        QFETCH(int, order);
        scenario([&] { automation_case(Order(order), true); });
    }
    void automationHoldDisclosure_data() { order_rows(); }
    void automationHoldDisclosure() {
        QFETCH(int, order);
        scenario([&] { hold_disclosure_case(Order(order)); });
    }
};

QTEST_GUILESS_MAIN(SessionInstallationEpochTests)
#include "session_installation_epoch_tests.moc"
