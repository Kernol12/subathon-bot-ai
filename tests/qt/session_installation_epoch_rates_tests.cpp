// SPDX-License-Identifier: GPL-3.0-or-later

#include "support/session_installation_epoch_fixture.hpp"
#include "support/sqlite_permission_graph_fixture.hpp"
#include "support/sqlite_rate_override_writer_fixture.hpp"

namespace {
using namespace installation_epoch_fixture;
namespace rate = rate_fixture;
using permission_graph_fixture::encoded;

const QStringList rate_tables =
    identity_tables + QStringList{"subathon_permission_head",  "subathon_permission_accounts",
                                  "subathon_permission_roles", "subathon_rate_observations",
                                  "subathon_rate_revisions",   "subathon_rate_decisions",
                                  "subathon_rate_occurrences", "subathon_rate_head"};

const QByteArray snapshot_prefix =
    QByteArray::fromHex("52535631 0000000153 01 0000000145 00"); // RSV1/S/parent E/no UTC
const QByteArray first_snapshot =
    snapshot_prefix + QByteArray::fromHex("00000000 00000001") + rate::first_body +
    QByteArray::fromHex(
        "00000001 01 000000014f 00000001 00000002 "
        "00000003455552 0000000131 0000000131 00000003555344 0000000136 0000000135");
const QByteArray fetched_snapshot =
    snapshot_prefix + QByteArray::fromHex("00000001") + rate::selected_rob.mid(4) +
    QByteArray::fromHex("00000001") + rate::first_body +
    QByteArray::fromHex(
        "00000002 01 000000014f 00 0000000146 0000000000000001 00000001 00000003 "
        "00000003455552 0000000131 0000000131 00000003474250 0000000139 000000023130 "
        "00000003555344 0000000136 0000000135");

QByteArray accepted_decision() {
    // RDC1: literal edit identities, actual actor sA/v with the declared first
    // 44-byte SAS1 scope, exact individual witness U/G, and accepted UTC 2000.
    return QByteArray::fromHex(
               "52444331 0000000149 000000014b 0000000144 0000000145 0000000153 000000014f "
               "00 0000000141 000000027341 0000000176 0000002c 53415331") +
           QByteArray(40, '\0') +
           QByteArray::fromHex("000000137065726d697373696f6e2d72617465732d7631 0000000000000001 "
                               "00 0000000155 0000000147 01 00000000000007d0");
}

PermissionGraphTrust expected_trust() {
    auto trust = permission_graph_fixture::trust();
    trust.catalog.revision = "permission-rates-v1";
    trust.catalog.operations.push_back({"rates.override.edit.v1",
                                        rate::core::PermissionOperationClass::ordinary,
                                        {"pair"},
                                        {},
                                        {}});
    return trust;
}

struct File {
    QTemporaryDir dir;
    QString path{dir.filePath("epoch-rates.sqlite")};
    SqliteWriter writer;
    SqliteReader reader;
    const PermissionGraphTrust trust = rate::literal_rate_trust();

    explicit File(bool retained) {
        require(dir.isValid() && writer.open(path, rate::finance::limits()) == StorageError::none,
                "Disposable rate database opens with a single writer");
        rate::expect_report(rate::run_catalog(writer, MigrationMode::advance,
                                              rate_overrides_repository_manifest(),
                                              rate::v17_manifest_bytes),
                            CatalogState::current, 17, 17, 17);
        require(reader.open(path) == StorageError::none, "Rate observer opens read-only");
        const auto root = rows(rate::revision_rows);
        require(root.size() == 1, "One root before seeding a retained result");
        rate::expect_reserved_root(root.front());
        require(encoded(trust) == encoded(expected_trust()),
                "C-01: exact permission-rates-v1 trust including all reference operations and pair "
                "USE");

        account_sessions_fixture::run(writer, [&](auto context) -> QVariant {
            const auto session = context.begin();
            Cleanup cleanup{session};
            seed_identity(session, "A");
            const rate::core::PermissionEditableState use{
                rate::core::PermissionFactsState::complete, {}, {{"A", {}, {rate::grant()}}}};
            const rate::core::PermissionEditableState grant{
                rate::core::PermissionFactsState::complete, {}, {{"A", {}, {}}}};
            require(SqlitePermissionGraphUnit(session, "I", trust).bootstrap("A", use, grant) ==
                        PermissionGraphWrite::staged,
                    "Exact rate-extended graph and ordinary pair permission are stored");
            if (retained) {
                // Literal immutable first result. No preview/save/recover call creates this oracle.
                (void)session.execute(
                    {"INSERT INTO subathon_rate_revisions VALUES(X'53',X'45',?,?)",
                     {rate::rsv_format, first_snapshot}});
                (void)session.execute({"INSERT INTO subathon_rate_decisions VALUES("
                                       "X'44',X'49',X'4b',X'45',X'53',?,?,?,?)",
                                       {rate::rin_format, rate::first_intent, rate::rdc_format,
                                        rate::first_decision}});
                (void)session.execute(
                    {"INSERT INTO subathon_rate_occurrences VALUES(X'4f',X'44',X'53',?)",
                     {rate::first_body}});
                (void)session.execute({"UPDATE subathon_rate_head SET revision_ref=X'53'", {}});
            }
            session.commit();
            return 1;
        });
    }

    SqlRows rows(const QString &sql) {
        auto result = reader.read({sql, {}});
        require(result.error == StorageError::none, "Committed rate rows read successfully");
        return std::move(result.rows);
    }
};

struct ResolverBackedAuthority final : RateAuthoritySource {
    explicit ResolverBackedAuthority(const PermissionGraphTrust &profile) : trust(profile) {}
    const PermissionGraphTrust &trust;
    const SessionLookupRequest request = lookup(record());
    Clock clock;
    unsigned resolutions = 0, graph_loads = 0;
    std::optional<AccountSessionResolution> resolved;

    RateAuthorityResolution resolve(const SqliteWorkSession &session) override {
        ++resolutions;
        SqliteAccountSessionUnit sessions(session, {{}});
        resolved = resolve_current_account_session(request, false, sessions, clock);
        if (const auto *failure = std::get_if<SessionFailure>(&*resolved))
            return *failure;
        ++graph_loads;
        auto graph = SqlitePermissionGraphUnit(session, "I", trust).load();
        if (const auto *failure = std::get_if<PermissionGraphFailure>(&graph))
            return *failure;
        const auto &loaded = std::get<StoredPermissionGraph>(graph);
        require(encoded(loaded.catalog) == encoded(expected_trust().catalog) &&
                    loaded.installation == "I" && loaded.revision == 1 &&
                    loaded.current_installation.current.session_epoch_ref == "E" &&
                    loaded.current_installation.revision_ref == "J" &&
                    loaded.current_accounts.size() == 1 &&
                    loaded.current_accounts[0].current.account_ref == "A" &&
                    loaded.current_accounts[0].current.session_epoch_ref == "C" &&
                    loaded.current_accounts[0].current.application_access_enabled &&
                    loaded.current_accounts[0].revision_ref == "K",
                "Loaded exact current graph and linked parents on the same active unit session");
        return RateAuthorityFacts{selected<ResolvedAccountSession>(*resolved),
                                  std::get<StoredPermissionGraph>(std::move(graph))};
    }
};

enum class Branch { preview, save_new, save_retained, recover_present, recover_absent };

void exact_preview(const RateEditPreview &actual, const RateEditPreview &expected) {
    require(actual.edit.installation == "I" && actual.edit.operation == "K" &&
                actual.edit.base == "E" && actual.edit.successor == "S" &&
                !actual.edit.replacement && !actual.proposal.replaced_override_ref &&
                actual.edit.override_input.override_ref == "O" &&
                actual.edit.override_input.kind == rate::core::RateOverrideKind::pinned &&
                actual.edit.override_input.decision_ref == "D" &&
                actual.edit.override_input.actor_ref == "A" &&
                actual.edit.override_input.from_currency == "EUR" &&
                actual.edit.override_input.to_currency == "USD" &&
                actual.edit.override_input.original_decimal == "1.20" &&
                actual.edit.override_input.reason == "r" &&
                !actual.edit.override_input.recorded_at_utc_ms && actual.basis.policy_ref == "B" &&
                actual.basis.observations.size() == 2 && actual.basis.edges.size() == 2 &&
                actual.basis.observations[0].context.observation_ref == "F" &&
                actual.basis.observations[1].context.observation_ref == "G" &&
                actual.basis.edges[0].observation_index == 0 &&
                actual.basis.edges[0].quote_ordinal == 1 &&
                actual.basis.edges[1].observation_index == 0 &&
                actual.basis.edges[1].quote_ordinal == 2,
            "Exact edit and source basis identities");
    require(rate::snapshot_bytes(actual.proposal.candidate) == fetched_snapshot,
            "Complete preview candidate matches independent RSV1 bytes");
    rate::expect_changes(actual.proposal.affected_rates, expected.proposal.affected_rates);
}

void exact_new_rows(File &file, const Image &before, const Image &after) {
    const auto revisions = rate_tables.indexOf("subathon_rate_revisions");
    require(
        file.rows(rate::observation_rows) ==
                SqlRows{{blob("F"), rate::rob_format, rate::selected_rob},
                        {blob("G"), rate::rob_format, rate::unused_rob}} &&
            file.rows("SELECT * FROM subathon_rate_revisions WHERE snapshot_ref=X'53'") ==
                SqlRows{{blob("S"), blob("E"), rate::rsv_format, fetched_snapshot}} &&
            file.rows(rate::decision_rows) ==
                SqlRows{{blob("D"), blob("I"), blob("K"), blob("E"), blob("S"), rate::rin_format,
                         rate::fetched_intent, rate::rdc_format, accepted_decision()}} &&
            file.rows(rate::occurrence_rows) ==
                SqlRows{{blob("O"), blob("D"), blob("S"), rate::first_body}} &&
            file.rows(rate::head_rows) == SqlRows{{blob("H"), blob("S")}} &&
            after[revisions].size() == 2 && after[revisions].front() == before[revisions].front(),
        "Exactly one successor, decision, occurrence and head advance; both observations retained");
    const auto unaffected = rate_tables.indexOf("subathon_rate_observations");
    for (qsizetype i = 0; i < unaffected; ++i)
        require(after[i] == before[i], "Rate save preserves identity and graph rows");
}

void rate_case(Order timing, Branch branch) {
    const bool retained = branch == Branch::save_retained || branch == Branch::recover_present;
    const auto expected = retained ? rate::first_preview() : rate::fetched_preview();
    File file(retained);
    Ordering order(file, file.writer, timing, rate_tables);
    ResolverBackedAuthority authority(file.trust);
    unsigned samples = 0;
    unsigned absences = 0;
    std::optional<RateReceiptSelection> provisional, released;
    std::optional<SqliteWorkResult> completion;
    order.run(
        [&](auto context) -> QVariant {
            const auto session = context.begin();
            Cleanup cleanup{session};
            SqliteRateOverrideUnit unit(session);
            const auto check = [&](const auto &result, auto positive) {
                if (order.current())
                    positive(result);
                else
                    rate::expect_refusal(result, RateStoreRefusal::denied);
            };
            const auto record_receipt = [&](const auto &result) {
                provisional = selected<RateReceiptSelection>(result);
                rate::expect_receipt(*provisional,
                                     retained ? RateReceiptOrigin::retained
                                              : RateReceiptOrigin::newly_staged,
                                     expected);
            };
            switch (branch) {
            case Branch::preview:
                check(
                    unit.preview(authority, file.trust, expected.edit, expected.basis),
                    [&](const auto &v) { exact_preview(selected<RateEditPreview>(v), expected); });
                break;
            case Branch::save_new:
            case Branch::save_retained:
                check(unit.save(authority, file.trust, expected.edit, expected, expected.basis,
                                [&]() -> std::optional<std::int64_t> {
                                    ++samples;
                                    return 2000;
                                }),
                      record_receipt);
                break;
            case Branch::recover_present:
                check(unit.recover(authority, file.trust, expected.edit, expected), record_receipt);
                break;
            case Branch::recover_absent:
                check(unit.recover(authority, file.trust, expected.edit, expected),
                      [&](const auto &v) {
                          (void)selected<RateAttemptAbsent>(v);
                          ++absences;
                      });
                break;
            }
            require(authority.resolutions == 1 && authority.resolved.has_value(),
                    "Rate operation calls the resolver-backed source exactly once");
            if (order.current()) {
                exact_session(*authority.resolved, record());
                require(authority.graph_loads == 1 && authority.clock.calls == 1,
                        "Successful resolver and one graph load in the rate work session");
                session.commit();
                return 1;
            }

            require(selected<SessionFailure>(*authority.resolved) == SessionFailure::revoked &&
                        authority.clock.calls == 0 && authority.graph_loads == 0 && samples == 0 &&
                        !provisional && absences == 0,
                    "F refuses before clock/graph, acceptance time, receipt or absence disclosure");
            order.unchanged_inside(session);
            session.rollback();
            return {};
        },
        [&](auto work) {
            released = finish_rate_receipt(provisional, work);
            completion = std::move(work);
        });
    require(completion.has_value(), "Actual rate work completion delivered");
    if (!order.current()) {
        rolled_back(*completion);
        require(!released, "Matching completion cannot publish a refused result");
        order.unchanged_after();
        return;
    }

    committed(*completion);
    require(absences == (branch == Branch::recover_absent ? 1u : 0u) &&
                samples == (branch == Branch::save_new ? 1u : 0u),
            "Verified absence has its own typed result, and only new save samples acceptance time");
    if (retained || branch == Branch::save_new) {
        require(released.has_value(), "Actual committed completion releases the exact receipt");
        rate::expect_receipt(
            *released, retained ? RateReceiptOrigin::retained : RateReceiptOrigin::newly_staged,
            expected);
    } else {
        require(!released, "Preview or verified absence has no receipt");
    }
    if (branch == Branch::save_new)
        exact_new_rows(file, order.baseline(), order.image());
    else
        order.unchanged_after();
}
void internal_rate_reads() {
    File file(true);
    Ordering order(file, file.writer, Order::after_rotation, rate_tables);
    std::optional<SqliteWorkResult> completion;
    order.run(
        [&](auto context) -> QVariant {
            const auto session = context.begin();
            Cleanup cleanup{session};
            const SqliteRateOverrideUnit unit(session);
            for (const auto &result : {unit.head(), unit.revision("S")}) {
                const auto &selected_revision = selected<rate::core::EffectiveRateSnapshot>(result);
                require(rate::snapshot_bytes(selected_revision) == first_snapshot,
                        "Internal selection still reads the exact retained revision after F");
            }
            session.commit();
            return 1;
        },
        [&](auto result) { completion = std::move(result); });
    require(completion.has_value(), "Matching internal reader completion observed");
    committed(*completion);
    order.unchanged_after();
}
} // namespace

class SessionInstallationEpochRateTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void preview_data() { order_rows(); }
    void preview() {
        QFETCH(int, order);
        scenario([&] { rate_case(Order(order), Branch::preview); });
    }
    void saveNew_data() { order_rows(); }
    void saveNew() {
        QFETCH(int, order);
        scenario([&] { rate_case(Order(order), Branch::save_new); });
    }
    void saveRetained_data() { order_rows(); }
    void saveRetained() {
        QFETCH(int, order);
        scenario([&] { rate_case(Order(order), Branch::save_retained); });
    }
    void recoverPresent_data() { order_rows(); }
    void recoverPresent() {
        QFETCH(int, order);
        scenario([&] { rate_case(Order(order), Branch::recover_present); });
    }
    void recoverAbsent_data() { order_rows(); }
    void recoverAbsent() {
        QFETCH(int, order);
        scenario([&] { rate_case(Order(order), Branch::recover_absent); });
    }
    void internalHeadAndRevisionAfterRotation() { scenario(internal_rate_reads); }
};

QTEST_GUILESS_MAIN(SessionInstallationEpochRateTests)
#include "session_installation_epoch_rates_tests.moc"
