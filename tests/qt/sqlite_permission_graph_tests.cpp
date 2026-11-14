// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/sqlite_permission_graph_fixture.hpp"
#include <future>
#include <limits>

using namespace permission_graph_fixture;
namespace {
void failure(const PermissionGraphRead<StoredPermissionGraph> &v, PermissionGraphFailure expected) {
    require(std::holds_alternative<PermissionGraphFailure>(v) &&
                std::get<PermissionGraphFailure>(v) == expected,
            "Exact complete-load refusal; no partial graph");
}
SqlRows physical(Observer &o) {
    SqlRows result;
    for (const auto &table : {"subathon_permission_head", "subathon_permission_roles",
                              "subathon_permission_accounts"}) {
        auto rows = o.rows(QString("SELECT * FROM %1 ORDER BY 1,2,3").arg(table));
        result.append(rows);
    }
    return result;
}
void exact_rows(Observer &o, std::uint64_t revision, const PermissionEditableState &use,
                const PermissionEditableState &grant, const PermissionGraphTrust &trusted) {
    require(
        o.rows(
            "SELECT installation_ref,revision,owner_ref,catalog FROM subathon_permission_head") ==
            SqlRows{{QByteArray("I"), encoded(revision), QByteArray("O"), encoded(trusted)}},
        "Exact independent head row");
    SqlRows roles, accounts;
    int channel = 0;
    for (const auto *g : {&use, &grant}) {
        for (const auto &r : g->roles)
            roles.append({QByteArray("I"), channel, blob(r.role), encoded(r.entries)});
        for (const auto &a : g->accounts)
            if (!a.assigned_roles.empty() || !a.individual_entries.empty())
                accounts.append({QByteArray("I"), channel, blob(a.account),
                                 encoded(a.assigned_roles), encoded(a.individual_entries)});
        ++channel;
    }
    auto order = [](const SqlRow &a, const SqlRow &b) {
        if (a[1].toInt() != b[1].toInt())
            return a[1].toInt() < b[1].toInt();
        return a[2].toByteArray() < b[2].toByteArray();
    };
    std::sort(roles.begin(), roles.end(), order);
    std::sort(accounts.begin(), accounts.end(), order);
    require(
        o.rows("SELECT installation_ref,channel,role_ref,entries FROM subathon_permission_roles "
               "ORDER BY channel,role_ref") == roles,
        "Both channels' exact role payloads, independent encoder");
    require(o.rows("SELECT installation_ref,channel,account_ref,memberships,entries FROM "
                   "subathon_permission_accounts ORDER BY channel,account_ref") == accounts,
            "Both channels' exact memberships/entries and canonical sparse omissions");
}
template <class F> void storage_error(F f, PermissionGraphFailure expected) {
    bool reached = false;
    try {
        f();
    } catch (const PermissionGraphStorageFailure &e) {
        reached = true;
        require(e.code() == expected, "Exact storage refusal");
    }
    require(reached, "Typed storage exception, never NI accepted");
}
QString set_blob(const QString &table, const QString &column, const QByteArray &bytes,
                 const QString &where = {}) {
    return "UPDATE " + table + " SET " + column + "=X'" + QString::fromLatin1(bytes.toHex()) + "'" +
           where;
}
PermissionEditableState deny_management() {
    auto next = use_graph();
    auto e = allowance("management", "roles.edit");
    e.effect = PermissionEffect::deny;
    e.grant.reset();
    e.resources = {{"role", PermissionResourceMode::any, {}}};
    next.accounts[0].individual_entries.push_back(e);
    return next;
}
void refusal_keeps_rows(Database &db, const PermissionEditableState &next,
                        PermissionGraphInputError error) {
    Observer observer(db.path);
    const auto before = physical(observer);
    db.work([&](const auto &s) {
        SqlitePermissionGraphUnit unit(s, "I", db.trusted);
        input_error([&] { (void)unit.replace_use(1, next); }, error);
        input_error([&] { (void)unit.replace_grant_from_owner(1, next); }, error);
        input_error([&] { (void)unit.describe_use_edit(next); }, error);
    });
    require(physical(observer) == before, "Pure mutation refusal performs no writes");
}
} // namespace
class SqlitePermissionGraphTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() { qt_test_support::fail_on_sql_warnings(); }
    void init() { qt_test_support::fail_on_sql_warnings(); }
    void compiledReferenceCatalogAndManagementBindingsMatchIndependentInventory() {
        scenario([] {
            require(encoded(reference_permission_graph_trust()) == encoded(trust()),
                    "Every compiled operation/class/domain/management mapping matches literal "
                    "reference inventory");
        });
    }
    void additiveManifestPreservesEveryCompiledPrefixByte() {
        scenario([] {
            const auto prefix = account_session_repository_manifest();
            // Existing frozen fixture comparison independently covers all shipped bytes.
            const account_sessions_fixture::Data frozen;
            const auto literal = frozen.manifest();
            require(prefix.steps.size() == 8 && literal.steps.size() == 8,
                    "Actual prefix1-8, no invented step9 owner");
            const auto actual = permission_graph_repository_manifest();
            require(actual.catalog_format == prefix.catalog_format &&
                        actual.store_identity == prefix.store_identity &&
                        actual.steps.size() == 9 &&
                        actual.supported_starting_versions ==
                            QList<qint32>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
                    "Additive version9 only, full supported starts");
            for (qsizetype i = 0; i < 8; ++i) {
                const auto &seen = actual.steps[i];
                const auto &expected = literal.steps[i];
                require(seen.version == expected.version && seen.id == expected.id &&
                            seen.sql == expected.sql &&
                            seen.expected_schema == expected.expected_schema &&
                            canonical_migration_payload(seen) ==
                                canonical_migration_payload(expected),
                        "Preserve each shipped identity, SQL byte and full schema projection");
            }
            const auto expected = step9(literal);
            require(actual.steps.back().version == 9 && actual.steps.back().id == expected.id &&
                        actual.steps.back().sql == expected.sql &&
                        actual.steps.back().expected_schema == expected.expected_schema,
                    "Exact independent step9 inventory");
        });
    }
    void explicitMigrationUpgradesPopulatedEightAndGuardsSchema() {
        scenario([] {
            QTemporaryDir dir;
            require(dir.isValid(), "Disposable upgrade directory");
            const auto path = dir.filePath("upgrade.sqlite");
            SqliteWriter writer;
            require(writer.open(path, account_sessions_fixture::limits()) == StorageError::none,
                    "Writer opens");
            auto result = migrate(writer, account_session_repository_manifest());
            require(result.reason == MigrationReason::none && result.report &&
                        result.report->version == 8,
                    "Existing catalog installed");
            (void)run(writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit accounts(s, {{}});
                require(accounts.insert_installation({{"I", "E"}, "J"}) ==
                            LifecycleWriteResult::staged,
                        "Existing installation staged");
                require(accounts.insert_account({{"I", "O", "F", true}, "K"}) ==
                            LifecycleWriteResult::staged,
                        "Existing owner account staged");
                s.commit();
                return 1;
            });
            Observer observer(path);
            const auto old_accounts = observer.rows("SELECT * FROM subathon_accounts");
            result =
                migrate(writer, permission_graph_repository_manifest(), MigrationMode::inspect);
            require(result.reason == MigrationReason::none && result.report &&
                        result.report->version == 8 &&
                        result.report->state == CatalogState::upgrade_required &&
                        result.report->applied == 0,
                    "Inspection does not install step9");
            require(
                observer
                    .rows("SELECT name FROM sqlite_schema WHERE name LIKE 'subathon_permission_%'")
                    .isEmpty(),
                "No implicit migration");
            result = migrate(writer, permission_graph_repository_manifest());
            require(result.reason == MigrationReason::none && result.report &&
                        result.report->version == 9 && result.report->applied == 1 &&
                        result.work.transaction == TransactionOutcome::committed &&
                        result.work.error == WorkError::none && result.work.value,
                    "Only explicit additive migration commits");
            require(observer.rows("SELECT * FROM subathon_accounts") == old_accounts,
                    "Account facts preserved byte for byte");
            account_sessions_fixture::healthy(observer);
            corrupt(
                path,
                {"CREATE INDEX foreign_permission_index ON subathon_permission_roles(role_ref)"});
            result =
                migrate(writer, permission_graph_repository_manifest(), MigrationMode::inspect);
            require(result.reason == MigrationReason::schema_mismatch && !result.report,
                    "Extra permission schema is refused");
        });
    }
    void missingHeadIsNotKnownEmptyAndBootstrapDoesNotAuthenticate() {
        scenario([] {
            Database db(false);
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                failure(unit.load(), PermissionGraphFailure::missing);
                require(unit.bootstrap("O", empty_graph(), empty_graph()) ==
                            PermissionGraphWrite::staged,
                        "Raw setup stages known empty policy");
                const auto v = unit.load();
                require(std::holds_alternative<StoredPermissionGraph>(v),
                        "Same-session known graph");
                same(std::get<StoredPermissionGraph>(v), 1, empty_graph(), empty_graph());
                require(unit.bootstrap("A", use_graph(), grant_graph()) ==
                            PermissionGraphWrite::collision,
                        "Existing owner/head never adopted or overwritten");
            });
            same(db.load(), 1, empty_graph(), empty_graph());
            Observer observer(db.path);
            exact_rows(observer, 1, empty_graph(), empty_graph(), db.trusted);
        });
    }
    void bootstrapCompleteStateHasExactIndependentPhysicalRows() {
        scenario([] {
            Database db(false);
            db.work([&](const auto &session) {
                SqlitePermissionGraphUnit unit(session, "I", db.trusted);
                require(unit.bootstrap("O", use_graph(), grant_graph()) ==
                            PermissionGraphWrite::staged,
                        "Nonempty bootstrap stages both complete channels");
            });
            Observer observer(db.path);
            exact_rows(observer, 1, use_graph(), grant_graph(), db.trusted);
            same(db.load(), 1);
        });
    }
    void completeCommittedGraphRetainsBothChannelsAndProtectedOwner() {
        scenario([] {
            Database db;
            same(db.load(), 1);
            Observer observer(db.path);
            require(observer.rows("SELECT revision,owner_ref FROM subathon_permission_head") ==
                        SqlRows{{QByteArray::fromHex("0000000000000001"), QByteArray("O")}},
                    "Exact BE64 revision and existing owner FK");
            require(observer.rows("SELECT catalog FROM subathon_permission_head") ==
                        SqlRows{{encoded(db.trusted)}},
                    "Lossless trusted catalog/mapping framing");
            require(
                observer.rows("SELECT entries FROM subathon_permission_roles WHERE channel=0") ==
                    SqlRows{{encoded(use_graph().roles[0].entries)}},
                "Independent physical entry codec");
            exact_rows(observer, 1, use_graph(), grant_graph(), db.trusted);
            account_sessions_fixture::healthy(observer);
        });
    }
    void replacementIsProvisionalUntilActualCommitAndFreshReader() {
        scenario([] {
            Database db;
            auto next = use_graph();
            next.accounts[0].assigned_roles.clear();
            Observer observer(db.path);
            const auto old = physical(observer);
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                require(unit.replace_use(1, next) == PermissionGraphWrite::staged,
                        "Whole replacement staged");
                const auto read = unit.load();
                require(std::holds_alternative<StoredPermissionGraph>(read),
                        "Same-session load complete");
                same(std::get<StoredPermissionGraph>(read), 2, next);
                require(physical(observer) == old,
                        "External reader cannot see provisional replacement");
            });
            same(db.load(), 2, next);
            require(physical(observer) != old, "New committed reader sees replacement");
            exact_rows(observer, 2, next, grant_graph(), db.trusted);
        });
    }
    void replayAndStaleRevisionNeverMutateAnyTable() {
        scenario([] {
            Database db;
            const auto next = empty_graph();
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                require(unit.replace_use(1, next) == PermissionGraphWrite::staged,
                        "First CAS stages");
            });
            Observer observer(db.path);
            const auto expected = physical(observer);
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                require(unit.replace_use(1, use_graph()) == PermissionGraphWrite::stale,
                        "Replay stale");
                require(unit.replace_grant_from_owner(1, empty_graph()) ==
                            PermissionGraphWrite::stale,
                        "Same head covers separate GRANT policy");
                require(unit.synchronize_catalog(1) == PermissionGraphWrite::stale,
                        "Same head covers trusted catalog mutation");
            });
            require(physical(observer) == expected,
                    "Every physical row unchanged, no partial deletion or revision update");
            same(db.load(), 2, next);
        });
    }
    void managementDelegationAndMembershipRevocationsFenceOldHead_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("management-use") << 0;
        QTest::newRow("owner-grant") << 1;
        QTest::newRow("membership") << 2;
    }
    void managementDelegationAndMembershipRevocationsFenceOldHead() {
        QFETCH(int, kind);
        scenario([&] {
            Database db;
            auto next = use_graph();
            if (kind == 0)
                next = deny_management();
            else if (kind == 2)
                next.accounts[0].assigned_roles.clear();
            const auto grant = kind == 1 ? empty_graph() : grant_graph();
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                const auto status =
                    kind == 1 ? unit.replace_grant_from_owner(1, grant) : unit.replace_use(1, next);
                require(status == PermissionGraphWrite::staged,
                        "Current revocation changes shared head");
            });
            same(db.load(), 2, next, grant);
            Observer exact_observer(db.path);
            exact_rows(exact_observer, 2, next, grant, db.trusted);
            Observer observer(db.path);
            const auto revoked = physical(observer);
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                require(unit.replace_use(1, use_graph()) == PermissionGraphWrite::stale,
                        "Old preview cannot overwrite revocation");
                require(unit.replace_grant_from_owner(1, grant_graph()) ==
                            PermissionGraphWrite::stale,
                        "Old delegation head cannot restore scope");
            });
            require(physical(observer) == revoked, "All revocations survive stale attempt");
        });
    }
    void knownRollbackDiscardsStagedGraphAndHead() {
        scenario([] {
            Database db;
            bool staged = false;
            const auto result = qt_test_support::run_work(
                db.writer,
                [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                    require(unit.replace_use(1, empty_graph()) == PermissionGraphWrite::staged,
                            "Mutation staged before rollback");
                    staged = true;
                    s.rollback();
                    return 1;
                },
                10000);
            require(staged && result.transaction == TransactionOutcome::rolled_back &&
                        result.error == WorkError::not_committed && !result.value,
                    "Actual known rollback, never feature-marker/work_exception accepted");
            same(db.load(), 1);
        });
    }
    void actualLaterSqlFailureCannotPublishPartialReplacement() {
        scenario([] {
            Database db;
            bool staged = false;
            const auto result = qt_test_support::run_work(
                db.writer,
                [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                    require(unit.replace_use(1, empty_graph()) == PermissionGraphWrite::staged,
                            "Replacement stages first");
                    staged = true;
                    (void)s.execute(
                        {"INSERT INTO subathon_permission_accounts "
                         "VALUES(X'49',7,X'41',X'0000000000000000',X'0000000000000000')",
                         {}});
                    s.commit();
                    return 1;
                },
                10000);
            require(staged && result.transaction == TransactionOutcome::rolled_back &&
                        result.error == WorkError::sql_failure && !result.value,
                    "Real reached SQL constraint retains sticky SQL failure and rolls back whole "
                    "transaction");
            same(db.load(), 1);
        });
    }
    void exceptionAfterCommitRetainsDurabilityButPublishesNoSuccess() {
        scenario([] {
            Database db;
            bool staged = false;
            const auto result = qt_test_support::run_work(
                db.writer,
                [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                    require(unit.replace_use(1, empty_graph()) == PermissionGraphWrite::staged,
                            "Raw staged result not publication");
                    staged = true;
                    s.commit();
                    throw std::runtime_error("synthetic postcommit failure");
                },
                10000);
            require(staged && result.transaction == TransactionOutcome::committed &&
                        result.error == WorkError::work_exception && !result.value,
                    "Commit plus exceptional completion never synthesizes normal success");
            same(db.load(), 2, empty_graph());
        });
    }
    void noOpAdvancesHeadAndRevisionExhaustionNeverWraps() {
        scenario([] {
            Database db;
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                require(unit.replace_use(1, use_graph()) == PermissionGraphWrite::staged,
                        "Successful identical CAS consumes revision");
                require(unit.replace_use(1, use_graph()) == PermissionGraphWrite::stale,
                        "Identical replay still stale");
            });
            same(db.load(), 2);
            corrupt(db.path, {"UPDATE subathon_permission_head SET revision=X'ffffffffffffffff'"});
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                bool reached = false;
                try {
                    (void)unit.replace_use(UINT64_MAX, empty_graph());
                } catch (const PermissionGraphStorageFailure &e) {
                    reached = true;
                    require(e.code() == PermissionGraphFailure::resource_limit,
                            "Exhausted head refuses");
                }
                require(reached, "No wrap or caller-chosen head");
            });
            same(db.load(), UINT64_MAX);
        });
    }
    void managementMappingDerivesEveryActualChangedTarget() {
        scenario([] {
            Database db;
            auto next = use_graph();
            next.roles[0].entries[0].quantities[0].upper = PermissionRational{"30000", "1"};
            next.accounts[0].assigned_roles.clear();
            next.accounts[1].individual_entries.clear();
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                const auto v = unit.describe_use_edit(next);
                require(std::holds_alternative<PermissionGraphEditFacts>(v),
                        "Complete raw edit facts");
                const auto &facts = std::get<PermissionGraphEditFacts>(v);
                same(facts.before, 1);
                require(encoded(facts.after) == encoded(next) && facts.management.size() == 3,
                        "No omitted restrictive or membership/override intent");
                const std::array<std::array<std::string, 3>, 3> expected{
                    {{"members.edit", "account", "A"},
                     {"overrides.edit", "account", "O"},
                     {"roles.edit", "role", "R"}}};
                for (std::size_t i = 0; i < expected.size(); ++i) {
                    const auto &m = facts.management[i];
                    require(m.operation == expected[i][0] && m.resources.size() == 1 &&
                                m.resources[0].slot == expected[i][1] &&
                                m.resources[0].resource == expected[i][2] && m.quantities.empty(),
                            "Trusted mapping and exact target, no caller intent downgrade");
                }
                const auto unchanged = unit.describe_use_edit(use_graph());
                require(std::holds_alternative<PermissionGraphEditFacts>(unchanged) &&
                            std::get<PermissionGraphEditFacts>(unchanged).management.empty(),
                        "No-op carries no invented management intents");
            });
            same(db.load(), 1);
        });
    }
    void roleCreationDeletionAndOrderOnlyChangesHaveExactIntents() {
        scenario([] {
            Database db;
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                auto reordered = use_graph();
                std::swap(reordered.accounts[0], reordered.accounts[1]);
                const auto unchanged = unit.describe_use_edit(reordered);
                require(std::holds_alternative<PermissionGraphEditFacts>(unchanged) &&
                            std::get<PermissionGraphEditFacts>(unchanged).management.empty(),
                        "Account vector order is not an edit intent");
                for (const auto &id : {"R2", "R3"})
                    reordered.roles.push_back({id, {}});
                const auto v = unit.describe_use_edit(reordered);
                require(std::holds_alternative<PermissionGraphEditFacts>(v),
                        "New empty roles still have management intents");
                const auto &m = std::get<PermissionGraphEditFacts>(v).management;
                auto exact_intents = [](const auto &intents) {
                    require(intents.size() == 2, "Both created/deleted targets");
                    for (std::size_t i = 0; i < 2; ++i)
                        require(intents[i].operation == "roles.edit" &&
                                    intents[i].resources.size() == 1 &&
                                    intents[i].resources[0].slot == "role" &&
                                    intents[i].resources[0].resource == (i == 0 ? "R2" : "R3") &&
                                    intents[i].quantities.empty(),
                                "Exact role-management operation/slot/target and no quantities");
                };
                exact_intents(m);
                require(unit.replace_use(1, reordered) == PermissionGraphWrite::staged,
                        "Created roles stage");
                const auto removed = unit.describe_use_edit(use_graph());
                require(std::holds_alternative<PermissionGraphEditFacts>(removed) &&
                            std::get<PermissionGraphEditFacts>(removed).management.size() == 2,
                        "Deleting empty roles also requires management");
                exact_intents(std::get<PermissionGraphEditFacts>(removed).management);
            });
        });
    }
    void opaqueReferencesExactNumbersAndIndependentGrantMembershipRoundTrip() {
        scenario([] {
            Database db;
            auto use = use_graph();
            auto grant = grant_graph();
            const auto opaque = std::string("R\0\xff", 3);
            use.roles[0].role = opaque;
            auto &e = use.roles[0].entries[0];
            e.role = opaque;
            e.source = std::string("s\0\xff", 3);
            e.resources[0].allowed.push_back(std::string("T\0\xff", 3));
            e.quantities[0].upper = PermissionRational{"9223372036854775807", "1"};
            use.accounts[0].assigned_roles = {opaque};
            auto row = allowance("grant-role");
            row.origin = PermissionOrigin::role;
            row.role = "G";
            grant.roles = {{"G", {row}}};
            grant.accounts[1].assigned_roles = {"G"};
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                require(unit.replace_use(1, use) == PermissionGraphWrite::staged,
                        "No UTF-8 or numeric narrowing");
                require(unit.replace_grant_from_owner(2, grant) == PermissionGraphWrite::staged,
                        "GRANT role/membership has its own channel");
            });
            same(db.load(), 3, use, grant);
            Observer observer(db.path);
            exact_rows(observer, 3, use, grant, db.trusted);
        });
    }
    void currentAccountRevocationFactsAreReloadedWithoutFabricatingGraphAuthority() {
        scenario([] {
            Database db;
            db.work([&](const auto &s) {
                SqliteAccountSessionUnit accounts(s, {{}});
                require(accounts.replace_account({{"I", "A", "F", true}, "K"},
                                                 {{"I", "A", "G", false}, "L"}) ==
                            LifecycleWriteResult::staged,
                        "Existing account owner changes current access/epoch/revision");
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                const auto value = unit.load();
                require(std::holds_alternative<StoredPermissionGraph>(value),
                        "Raw policies remain facts after account revocation");
                const auto &v = std::get<StoredPermissionGraph>(value);
                require(v.revision == 1 &&
                            !v.current_accounts[0].current.application_access_enabled &&
                            v.current_accounts[0].current.session_epoch_ref == "G" &&
                            v.current_accounts[0].revision_ref == "L" &&
                            encoded(v.use) == encoded(use_graph()),
                        "Fresh account source plus separate graph head; consumers must check both");
            });
            const auto v = db.load();
            require(!v.current_accounts[0].current.application_access_enabled &&
                        v.current_accounts[0].revision_ref == "L",
                    "No cached enabled account");
        });
    }
    void callerCannotSmuggleUnknownOperationOrMalformedGraph_data() {
        QTest::addColumn<int>("kind");
        for (int i = 0; i < 10; ++i)
            QTest::newRow(qPrintable(QString::number(i))) << i;
    }
    void callerCannotSmuggleUnknownOperationOrMalformedGraph() {
        QFETCH(int, kind);
        scenario([&] {
            Database db;
            auto next = use_graph();
            switch (kind) {
            case 0:
                next.state = PermissionFactsState::unknown;
                break;
            case 1:
                next.roles.push_back(next.roles[0]);
                break;
            case 2:
                next.accounts.push_back(next.accounts[0]);
                break;
            case 3:
                next.accounts[0].assigned_roles = {"unresolved"};
                break;
            case 4:
                next.roles[0].entries[0].operation = "made.up";
                break;
            case 5:
                next.roles[0].entries[0].role = "wrong";
                break;
            case 6:
                next.roles[0].entries[0].quantities[0].upper = PermissionRational{"2", "2"};
                break;
            case 7:
                next.accounts[0].individual_entries = {allowance()};
                next.accounts[0].individual_entries[0].origin = PermissionOrigin::role;
                break;
            case 8:
                next.accounts.erase(next.accounts.begin());
                break;
            case 9:
                next.accounts.push_back({"non-roster", {}, {}});
                break;
            }
            refusal_keeps_rows(db, next, PermissionGraphInputError::invalid_input);
        });
    }
    void trustedMappingMustMatchCatalogAndCannotBeTakenFromCaller() {
        scenario([] {
            Database db;
            for (int kind = 0; kind < 4; ++kind) {
                auto bad = db.trusted;
                if (kind == 0)
                    bad.role_edit.operation = "reference.application-read.v1"; // wrong target slot
                if (kind == 1)
                    bad.membership_edit.target_slot = "timer";
                if (kind == 2)
                    bad.catalog.state = PermissionFactsState::unknown;
                if (kind == 3)
                    bad.catalog.operations[0].classification =
                        static_cast<PermissionOperationClass>(255);
                input_error([&] { SqlitePermissionGraphUnit unit({}, "I", bad); },
                            PermissionGraphInputError::invalid_input);
            }
            same(db.load(), 1);
        });
    }
    void sameRevisionCatalogClassificationOrMappingTamperingIsInvalid() {
        scenario([] {
            for (int kind = 0; kind < 2; ++kind) {
                Database db;
                auto forged = db.trusted;
                if (kind == 0)
                    forged.catalog.operations[6].classification =
                        PermissionOperationClass::ordinary;
                else
                    forged.role_edit = {"members.edit", "account"};
                const auto bytes = encoded(forged).toHex();
                corrupt(db.path, {"UPDATE subathon_permission_head SET catalog=X'" +
                                  QString::fromLatin1(bytes) + "'"});
                Observer observer(db.path);
                const auto before = physical(observer);
                db.work([&](const auto &s) {
                    SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                    failure(unit.load(), PermissionGraphFailure::invalid_data);
                    storage_error([&] { (void)unit.synchronize_catalog(1); },
                                  PermissionGraphFailure::invalid_data);
                });
                require(physical(observer) == before,
                        "Whole physical snapshot unchanged on corrupt sync");
                require(observer.rows("SELECT catalog FROM subathon_permission_head") ==
                            SqlRows{{encoded(forged)}},
                        "Synchronization never repairs tampering");
            }
        });
    }
    void trustedCatalogUpgradeIsExplicitCasAndOldTrustCannotReadIt() {
        scenario([] {
            Database db;
            const auto old = db.trusted;
            db.trusted.catalog.revision = "C2";
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                failure(unit.load(), PermissionGraphFailure::unavailable);
                storage_error([&] { (void)unit.replace_use(1, use_graph()); },
                              PermissionGraphFailure::unavailable);
                storage_error([&] { (void)unit.replace_grant_from_owner(1, grant_graph()); },
                              PermissionGraphFailure::unavailable);
                const auto edit = unit.describe_use_edit(use_graph());
                require(std::holds_alternative<PermissionGraphFailure>(edit) &&
                            std::get<PermissionGraphFailure>(edit) ==
                                PermissionGraphFailure::unavailable,
                        "Edit facts unavailable under mismatched catalog");
                require(unit.synchronize_catalog(1) == PermissionGraphWrite::staged,
                        "Trusted catalog owner explicitly advances CAS head");
                const auto v = unit.load();
                require(std::holds_alternative<StoredPermissionGraph>(v), "New trust now matches");
                const auto &g = std::get<StoredPermissionGraph>(v);
                require(g.revision == 2 && g.catalog.revision == "C2" &&
                            encoded(g.use) == encoded(use_graph()) &&
                            encoded(g.grant) == encoded(grant_graph()),
                        "Catalog update preserves both policy channels");
            });
            Observer observer(db.path);
            exact_rows(observer, 2, use_graph(), grant_graph(), db.trusted);
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit old_unit(s, "I", old);
                failure(old_unit.load(), PermissionGraphFailure::unavailable);
            });
        });
    }
    void corruptOrOversizedPhysicalRowsNeverBecomeEmpty_data() {
        QTest::addColumn<QString>("sql");
        QTest::addColumn<int>("code");
        const auto invalid = int(PermissionGraphFailure::invalid_data),
                   resource = int(PermissionGraphFailure::resource_limit);
        QTest::newRow("bad-codec-count")
            << "UPDATE subathon_permission_roles SET entries=X'ffffffffffffffff'" << resource;
        QTest::newRow("trailing-codec-byte")
            << "UPDATE subathon_permission_roles SET entries=CAST(entries||X'00' AS BLOB)"
            << invalid;
        QTest::newRow("truncated-codec")
            << "UPDATE subathon_permission_roles SET entries=X'0000000000000001'" << invalid;
        QTest::newRow("wrong-storage-class")
            << "UPDATE subathon_permission_roles SET entries='not a blob'" << invalid;
        QTest::newRow("bad-channel") << "UPDATE subathon_permission_roles SET channel=8" << invalid;
        QTest::newRow("zero-revision")
            << "UPDATE subathon_permission_head SET revision=X'0000000000000000'" << invalid;
        QTest::newRow("missing-owner")
            << "DELETE FROM subathon_accounts WHERE account_ref=X'4f'" << invalid;
        QTest::newRow("missing-role")
            << "DELETE FROM subathon_permission_roles WHERE channel=0" << invalid;
        QTest::newRow("oversized-payload")
            << "UPDATE subathon_permission_roles SET entries=zeroblob(16777217)" << resource;
        QTest::newRow("revision-integer")
            << "UPDATE subathon_permission_head SET revision=1" << invalid;
        QTest::newRow("revision-short")
            << "UPDATE subathon_permission_head SET revision=X'01'" << invalid;
        QTest::newRow("catalog-truncated")
            << "UPDATE subathon_permission_head SET catalog=X'01'" << invalid;
        QTest::newRow("catalog-count")
            << "UPDATE subathon_permission_head SET "
               "catalog=X'"
               "0100000000000000177065726d697373696f6e2d7265666572656e63652d7631ffffffffffffffff'"
            << resource;
        QTest::newRow("grant-membership-count") << "UPDATE subathon_permission_accounts SET "
                                                   "memberships=X'ffffffffffffffff' WHERE channel=1"
                                                << resource;
        QTest::newRow("grant-membership-truncated")
            << "UPDATE subathon_permission_accounts SET memberships=X'0000000000000001' WHERE "
               "channel=1"
            << invalid;
        QTest::newRow("grant-entry-truncated")
            << "UPDATE subathon_permission_accounts SET entries=X'0000000000000001' WHERE channel=1"
            << invalid;
        QTest::newRow("grant-entry-count")
            << "UPDATE subathon_permission_accounts SET entries=X'ffffffffffffffff' WHERE channel=1"
            << resource;
        QTest::newRow("orphan-account") << "UPDATE subathon_permission_accounts SET "
                                           "account_ref=X'58' WHERE channel=1 AND account_ref=X'41'"
                                        << invalid;
        QTest::newRow("metadata-resource-before-codec")
            << "UPDATE subathon_permission_roles SET entries=CASE WHEN channel=0 THEN "
               "X'0000000000000001' ELSE zeroblob(16777217) END"
            << resource;
        QTest::newRow("metadata-resource-before-class")
            << "UPDATE subathon_permission_roles SET entries=CASE WHEN channel=0 THEN 'bad class' "
               "ELSE zeroblob(16777217) END"
            << resource;
        QTest::newRow("grant-membership-class")
            << "UPDATE subathon_permission_accounts SET memberships='not a blob' WHERE channel=1"
            << invalid;
        QTest::newRow("grant-membership-trailing")
            << "UPDATE subathon_permission_accounts SET memberships=CAST(memberships||X'00' AS "
               "BLOB) WHERE channel=1"
            << invalid;
        QTest::newRow("revision-long")
            << "UPDATE subathon_permission_head SET revision=X'000000000000000100'" << invalid;
        for (int k = 0; k < 5; ++k) {
            auto entries = use_graph().roles[0].entries;
            if (k == 0)
                entries[0].origin = static_cast<PermissionOrigin>(255);
            if (k == 1)
                entries[0].effect = static_cast<PermissionEffect>(255);
            if (k == 2)
                entries[0].resources[0].mode = static_cast<PermissionResourceMode>(255);
            if (k == 3)
                entries[0].quantities[0].mode = static_cast<PermissionQuantityMode>(255);
            auto bytes = encoded(entries);
            if (k == 4)
                bytes[18] = char(2); // count8, origin1, source length8 + s1; role option
            QTest::newRow(qPrintable(QString("entry-closed-tag-%1").arg(k)))
                << set_blob("subathon_permission_roles", "entries", bytes, " WHERE channel=0")
                << invalid;
        }
        for (int k = 0; k < 5; ++k) {
            auto t = trust();
            if (k == 0)
                t.catalog.operations[0].classification = static_cast<PermissionOperationClass>(255);
            if (k == 1)
                t.catalog.operations[0].quantity_slots[0].kind =
                    static_cast<PermissionNumberKind>(255);
            if (k == 2)
                t.catalog.operations[0].quantity_slots[0].allowed_modes = 16;
            if (k == 3)
                t.catalog.state = static_cast<PermissionFactsState>(255);
            if (k == 4)
                t.catalog.operations[0].quantity_slots[0].allowed_modes = 0;
            QTest::newRow(qPrintable(QString("catalog-closed-tag-%1").arg(k)))
                << set_blob("subathon_permission_head", "catalog", encoded(t)) << invalid;
        }
    }
    void corruptOrOversizedPhysicalRowsNeverBecomeEmpty() {
        QFETCH(QString, sql);
        QFETCH(int, code);
        scenario([&] {
            Database db;
            corrupt(db.path, {sql});
            Observer observer(db.path);
            const auto before = physical(observer);
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                failure(unit.load(), static_cast<PermissionGraphFailure>(code));
                bool reached = false;
                try {
                    (void)unit.replace_use(1, empty_graph());
                } catch (const PermissionGraphStorageFailure &e) {
                    reached = true;
                    require(e.code() == static_cast<PermissionGraphFailure>(code),
                            "Corrupt current source is never stale/collision");
                }
                require(reached, "Mutation refuses incomplete current facts");
                const auto expected = static_cast<PermissionGraphFailure>(code);
                storage_error([&] { (void)unit.replace_grant_from_owner(1, empty_graph()); },
                              expected);
                storage_error([&] { (void)unit.bootstrap("O", empty_graph(), empty_graph()); },
                              expected);
                storage_error([&] { (void)unit.synchronize_catalog(1); }, expected);
                const auto edit = unit.describe_use_edit(empty_graph());
                require(std::holds_alternative<PermissionGraphFailure>(edit) &&
                            std::get<PermissionGraphFailure>(edit) == expected,
                        "Current source corruption precedes roster mismatch in edit facts too");
            });
            require(physical(observer) == before, "No repair, adoption or partial mutation");
        });
    }
    void knownEmptyTablesRequireHealthyHeadAndExistingParents() {
        scenario([] {
            Database db(false);
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                require(unit.bootstrap("O", empty_graph(), empty_graph()) ==
                            PermissionGraphWrite::staged,
                        "Known empty initialization");
            });
            corrupt(db.path, {"DELETE FROM subathon_permission_accounts",
                              "DELETE FROM subathon_permission_roles"});
            same(db.load(), 1, empty_graph(), empty_graph());
            corrupt(db.path, {"DELETE FROM subathon_permission_head"});
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                failure(unit.load(), PermissionGraphFailure::missing);
            });
        });
    }
    void missingHeadWithOrphanRowsIsInvalidInsteadOfMissing() {
        scenario([] {
            Database db;
            corrupt(db.path, {"DELETE FROM subathon_permission_head"});
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                failure(unit.load(), PermissionGraphFailure::invalid_data);
            });
        });
    }
    void completeSqlAndOwnedMaterializationBoundsAreNotTruncation() {
        scenario([] {
            Database db;
            for (int kind = 0; kind < 2; ++kind) {
                PermissionGraphLimits l;
                if (kind == 0)
                    l.rows = 1;
                if (kind == 1)
                    l.selected_bytes = 1;
                db.work([&](const auto &s) {
                    SqlitePermissionGraphUnit unit(s, "I", db.trusted, l);
                    failure(unit.load(), PermissionGraphFailure::resource_limit);
                });
            }
            same(db.load(), 1);
            // Check a real selected variable field at exactly reference_bytes and
            // one byte above it, including unrelated accounts in complete closure.
            const auto account_ref = std::string(4096, 'a');
            db.work([&](const auto &s) {
                SqliteAccountSessionUnit accounts(s, {{}});
                require(accounts.insert_account({{"I", account_ref, "F", true}, "K"}) ==
                            LifecycleWriteResult::staged,
                        "Exact account field ceiling");
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                const auto value = unit.load();
                require(std::holds_alternative<StoredPermissionGraph>(value),
                        "At field bound fits");
                const auto &g = std::get<StoredPermissionGraph>(value);
                require(g.current_accounts.size() == 3 && g.use.accounts.size() == 3 &&
                            g.grant.accounts.size() == 3,
                        "Proven absent sparse rows become known empty accounts, never omitted");
            });
            corrupt(db.path, {"UPDATE subathon_accounts SET account_ref=CAST(account_ref||X'61' AS "
                              "BLOB) WHERE length(account_ref)=4096"});
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                failure(unit.load(), PermissionGraphFailure::resource_limit);
            });
        });
    }
    void ownedCompleteResultBudgetIncludesSynthesizedAccountCopiesAtExactBoundary() {
        scenario([] {
            Database db(false);
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                require(unit.bootstrap("O", empty_graph(), empty_graph()) ==
                            PermissionGraphWrite::staged,
                        "Known empty base");
                SqliteAccountSessionUnit accounts(s, {{}});
                require(accounts.insert_account({{"I", std::string(4096, 'a'), "F", true}, "K"}) ==
                            LifecycleWriteResult::staged,
                        "Large known-empty account");
            });
            // Independently summed catalog B=351 (operation subtotals
            // 84+54+47+16+21+23+15+15+26+26 plus state1+revision23).
            // Empty base B=382; new account adds 3*4096+4=12292:
            // current source plus BOTH synthesized channel account identities.
            // Physical encoded payload is much smaller; this exercises decoded
            // output projection, not just a SQL BLOB length/row cap.
            for (const auto bytes : {12674ULL, 12673ULL}) {
                auto limits = PermissionGraphLimits{};
                limits.selected_bytes = bytes;
                db.work([&](const auto &s) {
                    SqlitePermissionGraphUnit unit(s, "I", db.trusted, limits);
                    const auto value = unit.load();
                    if (bytes == 12673)
                        failure(value, PermissionGraphFailure::resource_limit);
                    else {
                        require(std::holds_alternative<StoredPermissionGraph>(value),
                                "Exact whole-result semantic budget fits");
                        const auto &v = std::get<StoredPermissionGraph>(value);
                        require(v.current_accounts.size() == 3 && v.use.accounts.size() == 3 &&
                                    v.grant.accounts.size() == 3,
                                "No empty-account omission to save output bytes");
                    }
                });
            }
        });
    }
    void borrowedMutationPreflightRejectsSizeBeforeMalformedShapeOrSql() {
        scenario([] {
            Database db;
            auto next = use_graph();
            next.roles[0].entries[0].source = std::string(4097, 'x');
            next.state = PermissionFactsState::unknown;
            refusal_keeps_rows(db, next, PermissionGraphInputError::resource_limit);
            // Constructor trust itself must fit; test preflight on an inert ticket
            // with default limits and a large nested borrowed allowed-set instead.
            next = use_graph();
            next.roles[0].entries[0].resources[0].allowed.assign(16385, "T");
            input_error(
                [&] {
                    SqlitePermissionGraphUnit unit({}, "I", db.trusted);
                    (void)unit.replace_use(1, next);
                },
                PermissionGraphInputError::resource_limit);
            next = use_graph();
            next.roles[0].entries[0].resources[0].allowed.assign(4096, std::string(4096, 'T'));
            input_error(
                [&] {
                    SqlitePermissionGraphUnit unit({}, "I", db.trusted);
                    (void)unit.replace_use(1, next);
                },
                PermissionGraphInputError::resource_limit);
            same(db.load(), 1);
        });
    }
    void limitsAndInvalidExpectedRevisionRejectBeforeTicketUse() {
        scenario([] {
            const auto t = trust();
            for (int kind = 0; kind < 4; ++kind) {
                auto l = PermissionGraphLimits{};
                if (kind == 0)
                    l.rows = 0;
                if (kind == 1)
                    l.selected_bytes = UINT64_MAX;
                if (kind == 2)
                    l.rows = std::uint64_t(std::numeric_limits<qint64>::max());
                if (kind == 3)
                    l.items = 0;
                input_error([&] { SqlitePermissionGraphUnit unit({}, "I", t, l); },
                            PermissionGraphInputError::invalid_limits);
            }
            SqlitePermissionGraphUnit unit({}, "I", t);
            input_error([&] { (void)unit.replace_use(0, empty_graph()); },
                        PermissionGraphInputError::invalid_input);
            work_error([&] { (void)unit.load(); }, WorkError::stale_session);
        });
    }
    void activeForeignExpiredAndClosedTicketsCannotAccessSql() {
        scenario([] {
            Database db;
            std::unique_ptr<SqlitePermissionGraphUnit> escaped;
            db.work([&](const auto &s) {
                escaped = std::make_unique<SqlitePermissionGraphUnit>(s, "I", db.trusted);
                auto task = std::async(std::launch::async, [&] {
                    work_error([&] { (void)escaped->load(); }, WorkError::wrong_thread);
                    work_error([&] { (void)escaped->replace_use(1, empty_graph()); },
                               WorkError::wrong_thread);
                });
                task.get();
            });
            work_error([&] { (void)escaped->load(); }, WorkError::stale_session);
            bool terminal_observed = false;
            const auto terminal = qt_test_support::run_work(
                db.writer,
                [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                    s.commit();
                    work_error([&] { (void)unit.load(); }, WorkError::session_closed);
                    work_error([&] { (void)unit.replace_grant_from_owner(1, empty_graph()); },
                               WorkError::session_closed);
                    terminal_observed = true;
                    return 1;
                },
                10000);
            require(terminal_observed && terminal.transaction == TransactionOutcome::committed &&
                        terminal.error == WorkError::session_closed && !terminal.value,
                    "Terminal misuse keeps committed outcome and actual sticky error without "
                    "publication");
        });
    }
    void bootstrapRejectsInvalidInputsAndMissingParents_data() {
        QTest::addColumn<int>("kind");
        for (int i = 0; i < 8; ++i)
            QTest::newRow(qPrintable(QString::number(i))) << i;
    }
    void bootstrapRejectsInvalidInputsAndMissingParents() {
        QFETCH(int, kind);
        scenario([&] {
            Database db(false);
            auto use = empty_graph(), grant = empty_graph();
            std::string owner = "O";
            if (kind == 0)
                owner.clear();
            if (kind == 1)
                owner = "nonexistent";
            if (kind == 2)
                use.accounts.pop_back();
            if (kind == 3)
                grant.accounts.push_back({"extra", {}, {}});
            if (kind == 4)
                grant.state = PermissionFactsState::unknown;
            if (kind == 5)
                grant.accounts[0].assigned_roles = {"unresolved"};
            if (kind == 6)
                corrupt(db.path, {"DELETE FROM subathon_account_installation"});
            if (kind == 7)
                corrupt(db.path, {"DELETE FROM subathon_accounts WHERE account_ref=X'4f'"});
            Observer observer(db.path);
            const auto before = physical(observer);
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                if (kind == 1 || kind == 6 || kind == 7)
                    storage_error([&] { (void)unit.bootstrap(owner, use, grant); },
                                  PermissionGraphFailure::invalid_data);
                else
                    input_error([&] { (void)unit.bootstrap(owner, use, grant); },
                                PermissionGraphInputError::invalid_input);
                if (kind == 6)
                    failure(unit.load(), PermissionGraphFailure::invalid_data);
            });
            require(physical(observer) == before, "Bootstrap refuses without head/child writes");
        });
    }
    void bootstrapOverCorruptionNeverCollidesOrAdopts() {
        scenario([] {
            for (int k = 0; k < 3; ++k) {
                Database db;
                if (k == 0)
                    corrupt(db.path, {"UPDATE subathon_permission_head SET catalog=X'01'"});
                if (k == 1)
                    corrupt(db.path, {"DELETE FROM subathon_permission_head"});
                if (k == 2)
                    corrupt(db.path, {"DELETE FROM subathon_accounts WHERE account_ref=X'4f'"});
                Observer observer(db.path);
                const auto before = physical(observer);
                db.work([&](const auto &s) {
                    SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                    storage_error([&] { (void)unit.bootstrap("O", use_graph(), grant_graph()); },
                                  PermissionGraphFailure::invalid_data);
                });
                require(physical(observer) == before, "No corrupt bootstrap adoption/repair");
            }
        });
    }
    void installationInputsAndDistinctScopesAreExplicit() {
        scenario([] {
            Database db;
            input_error([&] { SqlitePermissionGraphUnit unit({}, "", db.trusted); },
                        PermissionGraphInputError::invalid_input);
            input_error(
                [&] { SqlitePermissionGraphUnit unit({}, std::string(4097, 'x'), db.trusted); },
                PermissionGraphInputError::resource_limit);
            db.work([&](const auto &s) {
                SqliteAccountSessionUnit accounts(s, {{}});
                require(accounts.insert_installation({{"other", "E"}, "J"}) ==
                            LifecycleWriteResult::staged,
                        "Separate existing installation");
                SqlitePermissionGraphUnit other(s, "other", db.trusted);
                failure(other.load(), PermissionGraphFailure::missing);
                SqlitePermissionGraphUnit absent(s, "nonexistent", db.trusted);
                failure(absent.load(), PermissionGraphFailure::invalid_data);
            });
            same(db.load(), 1);
        });
    }
    void missingAndFutureExpectedHeadsAreStaleWithoutWrites() {
        scenario([] {
            for (bool populated : {false, true}) {
                Database db(populated);
                Observer observer(db.path);
                const auto before = physical(observer);
                db.work([&](const auto &s) {
                    SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                    require(unit.replace_use(2, use_graph()) == PermissionGraphWrite::stale,
                            "Proven missing or future expected head refuses");
                    require(unit.replace_grant_from_owner(2, grant_graph()) ==
                                PermissionGraphWrite::stale,
                            "GRANT cannot adopt future/missing head");
                });
                require(physical(observer) == before, "Stale performs zero physical writes");
            }
        });
    }
    void catalogRemovalUsedInEitherChannelRefusesWithoutWrites() {
        scenario([] {
            for (int channel = 0; channel < 2; ++channel) {
                Database db;
                // Retain use of time-add only in the selected channel. Independent SQL setup.
                if (channel == 0)
                    corrupt(db.path, {"DELETE FROM subathon_permission_roles WHERE channel=1",
                                      "DELETE FROM subathon_permission_accounts WHERE channel=1"});
                else
                    corrupt(db.path, {"DELETE FROM subathon_permission_roles WHERE channel=0",
                                      "DELETE FROM subathon_permission_accounts WHERE channel=0"});
                db.trusted.catalog.revision = "C2";
                db.trusted.catalog.operations.erase(db.trusted.catalog.operations.begin());
                Observer observer(db.path);
                const auto before = physical(observer);
                db.work([&](const auto &s) {
                    SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                    storage_error([&] { (void)unit.synchronize_catalog(1); },
                                  PermissionGraphFailure::invalid_data);
                });
                require(physical(observer) == before,
                        "Neither channel silently discards obsolete grants");
            }
        });
    }
    void mismatchedCatalogRefusesAllOrdinaryWritesWithoutChanges() {
        scenario([] {
            Database db;
            Observer observer(db.path);
            const auto before = physical(observer);
            db.trusted.catalog.revision = "C2";
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                storage_error([&] { (void)unit.replace_use(1, use_graph()); },
                              PermissionGraphFailure::unavailable);
                storage_error([&] { (void)unit.replace_grant_from_owner(1, grant_graph()); },
                              PermissionGraphFailure::unavailable);
                storage_error([&] { (void)unit.bootstrap("O", use_graph(), grant_graph()); },
                              PermissionGraphFailure::unavailable);
                const auto edit = unit.describe_use_edit(use_graph());
                require(std::holds_alternative<PermissionGraphFailure>(edit) &&
                            std::get<PermissionGraphFailure>(edit) ==
                                PermissionGraphFailure::unavailable,
                        "No mismatched edit facts");
            });
            require(physical(observer) == before,
                    "Mismatch is unavailable, never a repair/collision");
        });
    }
    void sqlFailuresAndPriorStickyErrorsPropagateUnchanged() {
        scenario([] {
            for (int k = 0; k < 3; ++k) {
                Database db;
                Observer observer(db.path);
                const auto before = physical(observer);
                if (k == 2)
                    corrupt(db.path, {"DROP TABLE subathon_permission_roles"});
                bool reached = false;
                const auto result = qt_test_support::run_work(
                    db.writer,
                    [&](auto context) -> QVariant {
                        const auto s = context.begin();
                        Cleanup cleanup{s};
                        // Constructor has no SQL/ticket side effects; NI is still an explicit unit
                        // call.
                        SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                        if (k < 2)
                            work_error(
                                [&] {
                                    (void)s.execute(
                                        {"SELECT * FROM main.no_such_permission_table", {}});
                                },
                                WorkError::sql_failure);
                        // The first SQL failure rolled back the active session.
                        // Later calls report its closed ticket; completion below
                        // still retains the original sql_failure.
                        if (k == 1)
                            work_error([&] { (void)unit.replace_use(1, use_graph()); },
                                       WorkError::session_closed);
                        else
                            work_error([&] { (void)unit.load(); },
                                       k == 0 ? WorkError::session_closed : WorkError::sql_failure);
                        reached = true;
                        return 1;
                    },
                    10000);
                require(reached && result.error == WorkError::sql_failure &&
                            result.transaction == TransactionOutcome::rolled_back && !result.value,
                        "Original SQL failure preserved; NI cannot satisfy reached guard");
                if (k < 2)
                    require(physical(observer) == before, "Sticky failure writes nothing");
                else {
                    // The deliberately dropped table cannot be selected; remaining rows still match
                    // literals.
                    require(
                        observer.rows(
                            "SELECT revision,owner_ref,catalog FROM subathon_permission_head") ==
                            SqlRows{
                                {encoded(std::uint64_t(1)), QByteArray("O"), encoded(db.trusted)}},
                        "Missing table never rewrites head");
                    require(observer.rows(
                                "SELECT channel,account_ref FROM subathon_permission_accounts "
                                "ORDER BY channel,account_ref") == SqlRows{{0, QByteArray("A")},
                                                                           {0, QByteArray("O")},
                                                                           {1, QByteArray("A")},
                                                                           {1, QByteArray("O")}},
                            "Missing table never deletes account policies");
                }
            }
        });
    }
    void cumulativeWriterRowBudgetIsNotTypedUnavailable() {
        scenario([] {
            Database db;
            Observer observer(db.path);
            const auto before = physical(observer);
            // Setup writer is closed before opening the same disposable file with a tiny budget.
            bool stopped = false;
            require(db.writer.shutdown([&] { stopped = true; }) == StorageError::none,
                    "Setup writer closes normally");
            require(QTest::qWaitFor([&] { return stopped; }, 10000),
                    "Setup writer actually closed");
            auto budget = account_sessions_fixture::limits();
            budget.result_rows = 1;
            SqliteWriter tiny;
            require(tiny.open(db.path, budget) == StorageError::none, "Constrained writer opens");
            bool reached = false;
            const auto result = qt_test_support::run_work(
                tiny,
                [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                    work_error([&] { (void)unit.load(); }, WorkError::result_limit);
                    // The budget failure closes this session; its first error
                    // remains result_limit in work completion, not this call.
                    work_error([&] { (void)unit.replace_use(1, use_graph()); },
                               WorkError::session_closed);
                    reached = true;
                    return 1;
                },
                10000);
            require(reached && result.error == WorkError::result_limit &&
                        result.transaction == TransactionOutcome::rolled_back && !result.value,
                    "Cumulative writer budget retains actual sticky classification");
            require(physical(observer) == before, "Writer budget refusal writes nothing");
        });
    }
    void currentInstallationEpochAndRevisionAreReloaded() {
        scenario([] {
            Database db;
            db.work([&](const auto &s) {
                SqliteAccountSessionUnit accounts(s, {{}});
                require(accounts.replace_installation_epoch({{"I", "E"}, "J"}, "H", "N") ==
                            LifecycleWriteResult::staged,
                        "Existing source changes installation fence");
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                const auto value = unit.load();
                require(std::holds_alternative<StoredPermissionGraph>(value),
                        "Current facts available");
                const auto &v = std::get<StoredPermissionGraph>(value);
                require(v.current_installation.current.session_epoch_ref == "H" &&
                            v.current_installation.revision_ref == "N" && v.revision == 1,
                        "Fresh installation fence remains separate from graph revision");
            });
            const auto v = db.load();
            require(v.current_installation.current.session_epoch_ref == "H" &&
                        v.current_installation.revision_ref == "N",
                    "No cached installation facts");
        });
    }
    void unsignedIdentityOrderAndVectorReorderDoNotInventIntents() {
        scenario([] {
            Database db;
            const std::string high(1, char(255));
            auto expected = use_graph();
            expected.roles.push_back({"Z", {}});
            expected.roles.push_back({high, {}});
            expected.accounts[0].assigned_roles = {"R", "Z", high};
            auto entry = expected.roles[0].entries[0];
            entry.source = "second";
            expected.roles[0].entries.push_back(entry);
            db.work([&](const auto &s) {
                for (const auto &id : {std::string("Z"), high})
                    (void)s.execute(
                        {"INSERT INTO subathon_permission_roles VALUES(?,0,?,?)",
                         {QByteArray("I"), blob(id), QByteArray::fromHex("0000000000000000")}});
                (void)s.execute({"UPDATE subathon_permission_roles SET entries=? WHERE channel=0 "
                                 "AND role_ref=X'52'",
                                 {encoded(expected.roles[0].entries)}});
                (void)s.execute({"UPDATE subathon_permission_accounts SET memberships=? WHERE "
                                 "channel=0 AND account_ref=X'41'",
                                 {encoded(expected.accounts[0].assigned_roles)}});
            });
            db.work([&](const auto &s) {
                SqlitePermissionGraphUnit unit(s, "I", db.trusted);
                const auto value = unit.load();
                require(std::holds_alternative<StoredPermissionGraph>(value),
                        "Complete ordered graph");
                const auto &g = std::get<StoredPermissionGraph>(value);
                require(g.use.roles.size() == 3 && g.use.roles[0].role == "R" &&
                            g.use.roles[1].role == "Z" && g.use.roles[2].role == high,
                        "Unsigned byte identity order: ASCII precedes FF");
                std::reverse(expected.accounts[0].assigned_roles.begin(),
                             expected.accounts[0].assigned_roles.end());
                std::reverse(expected.roles[0].entries.begin(), expected.roles[0].entries.end());
                const auto edit = unit.describe_use_edit(expected);
                require(
                    std::holds_alternative<PermissionGraphEditFacts>(edit) &&
                        std::get<PermissionGraphEditFacts>(edit).management.empty(),
                    "Membership/entry order alone does not change rights or management intents");
            });
        });
    }
    void explicitEmptySparseRowsAreAcceptedOnRead() {
        scenario([] {
            Database db(false);
            db.work([&](const auto &s) {
                (void)s.execute({"INSERT INTO subathon_permission_head VALUES(?,?,?,?)",
                                 {QByteArray("I"), encoded(std::uint64_t(1)), QByteArray("O"),
                                  encoded(db.trusted)}});
                for (int channel : {0, 1})
                    (void)s.execute({"INSERT INTO subathon_permission_accounts VALUES(?,?,?,?,?)",
                                     {QByteArray("I"), channel, QByteArray("O"),
                                      QByteArray::fromHex("0000000000000000"),
                                      QByteArray::fromHex("0000000000000000")}});
            });
            same(db.load(), 1, empty_graph(), empty_graph());
        });
    }

    void perMethodSemanticMutationBytesHaveExactFitAndOneBelow() {
        scenario([] {
            // Independent B: catalog351 + bindings(14+19+21)=405.
            // constructor 1+405=406; long empty role graph 1+512+1+1=515.
            // describe 1+515=516; replace 1+8+515=524;
            // bootstrap 1+1+515+515=1032; sync 1+8+405=414.
            const std::array<std::uint64_t, 6> fits{406, 516, 1032, 524, 524, 414};
            for (std::size_t method = 0; method < fits.size(); ++method) {
                for (bool fit : {true, false}) {
                    Database db(method != 2);
                    auto next = empty_graph();
                    next.roles = {{std::string(512, 'r'), {}}};
                    auto limits = PermissionGraphLimits{};
                    limits.mutation_bytes = fits[method] - (fit ? 0U : 1U);
                    Observer observer(db.path);
                    const auto before = physical(observer);
                    db.work([&](const auto &s) {
                        auto action = [&] {
                            SqlitePermissionGraphUnit unit(s, "I", db.trusted, limits);
                            if (method == 0)
                                return;
                            if (method == 1) {
                                const auto result = unit.describe_use_edit(next);
                                require(std::holds_alternative<PermissionGraphEditFacts>(result),
                                        "Exact describe mutation B fits");
                                return;
                            }
                            const auto status = method == 2   ? unit.bootstrap("O", next, next)
                                                : method == 3 ? unit.replace_use(1, next)
                                                : method == 4
                                                    ? unit.replace_grant_from_owner(1, next)
                                                    : unit.synchronize_catalog(1);
                            require(status == PermissionGraphWrite::staged,
                                    "Exact method mutation B fits");
                        };
                        if (fit)
                            action();
                        else
                            input_error(action, PermissionGraphInputError::resource_limit);
                    });
                    if (!fit || method < 2)
                        require(physical(observer) == before, "Budget refusal/read writes nothing");
                    else if (method == 2)
                        exact_rows(observer, 1, next, next, db.trusted);
                    else if (method == 3)
                        exact_rows(observer, 2, next, grant_graph(), db.trusted);
                    else if (method == 4)
                        exact_rows(observer, 2, use_graph(), next, db.trusted);
                    else
                        exact_rows(observer, 2, use_graph(), grant_graph(), db.trusted);
                }
            }
        });
    }
    void recursiveItemsHaveExactFitAndOneBelowForWholeProjections() {
        scenario([] {
            // Trust I=10 operations+14 resource slots+1 quantity definition=25.
            // USE I=19; GRANT I=20; current account vector2 => load I=66.
            // No-op describe adds after USE19, management0 => I=85.
            // Long proposal: 100 empty roles + 2 accounts=102; bootstrap twice=204.
            const std::array<std::uint64_t, 7> fits{25, 66, 85, 204, 102, 102, 66};
            for (std::size_t method = 0; method < fits.size(); ++method) {
                for (bool fit : {true, false}) {
                    Database db(method != 3);
                    auto next = empty_graph();
                    for (int i = 0; i < 100; ++i)
                        next.roles.push_back({"r" + std::to_string(i), {}});
                    auto limits = PermissionGraphLimits{};
                    limits.items = fits[method] - (fit ? 0U : 1U);
                    Observer observer(db.path);
                    const auto before = physical(observer);
                    db.work([&](const auto &s) {
                        if (method == 0) {
                            auto ctor = [&] {
                                SqlitePermissionGraphUnit unit(s, "I", db.trusted, limits);
                            };
                            if (fit)
                                ctor();
                            else
                                input_error(ctor, PermissionGraphInputError::resource_limit);
                            return;
                        }
                        SqlitePermissionGraphUnit unit(s, "I", db.trusted, limits);
                        if (method == 1) {
                            const auto result = unit.load();
                            if (!fit)
                                failure(result, PermissionGraphFailure::resource_limit);
                            else
                                require(std::holds_alternative<StoredPermissionGraph>(result),
                                        "Load I66 fits");
                        } else if (method == 2) {
                            const auto result = unit.describe_use_edit(use_graph());
                            if (fit)
                                require(std::holds_alternative<PermissionGraphEditFacts>(result),
                                        "Describe I85 fits");
                            else
                                require(std::holds_alternative<PermissionGraphFailure>(result) &&
                                            std::get<PermissionGraphFailure>(result) ==
                                                PermissionGraphFailure::resource_limit,
                                        "Whole describe projection I84 refuses");
                        } else if (method == 6) {
                            auto sync = [&] {
                                require(unit.synchronize_catalog(1) == PermissionGraphWrite::staged,
                                        "Sync source I66 fits");
                            };
                            if (fit)
                                sync();
                            else
                                storage_error(sync, PermissionGraphFailure::resource_limit);
                        } else {
                            auto mutate = [&] {
                                const auto status = method == 3 ? unit.bootstrap("O", next, next)
                                                    : method == 4
                                                        ? unit.replace_use(1, next)
                                                        : unit.replace_grant_from_owner(1, next);
                                require(status == PermissionGraphWrite::staged,
                                        "Recursive proposed items fit");
                            };
                            if (fit)
                                mutate();
                            else
                                input_error(mutate, PermissionGraphInputError::resource_limit);
                        }
                    });
                    if (!fit || method < 3)
                        require(physical(observer) == before, "Item refusal/read writes nothing");
                }
            }
        });
    }
    void physicalRowsHaveExactFitAndOneBelowForEverySourceMethod() {
        scenario([] {
            // Base rows=head1 + roles2 + policies4 + installation1 + accounts2=10.
            // Proven missing graph selects only installation1 + accounts2=3.
            for (int method = 0; method < 6; ++method) {
                for (bool fit : {true, false}) {
                    Database db(method != 2);
                    auto limits = PermissionGraphLimits{};
                    limits.rows = std::uint64_t((method == 2 ? 3 : 10) - (fit ? 0 : 1));
                    Observer observer(db.path);
                    const auto before = physical(observer);
                    db.work([&](const auto &s) {
                        SqlitePermissionGraphUnit unit(s, "I", db.trusted, limits);
                        if (method == 0) {
                            const auto result = unit.load();
                            if (!fit)
                                failure(result, PermissionGraphFailure::resource_limit);
                            else
                                require(std::holds_alternative<StoredPermissionGraph>(result),
                                        "Exact row count fits");
                        } else if (method == 1) {
                            const auto result = unit.describe_use_edit(use_graph());
                            if (fit)
                                require(std::holds_alternative<PermissionGraphEditFacts>(result),
                                        "Describe exact rows fits");
                            else
                                require(std::holds_alternative<PermissionGraphFailure>(result) &&
                                            std::get<PermissionGraphFailure>(result) ==
                                                PermissionGraphFailure::resource_limit,
                                        "Describe rows one below refuses");
                        } else {
                            auto action = [&] {
                                const auto status =
                                    method == 2 ? unit.bootstrap("O", empty_graph(), empty_graph())
                                    : method == 3 ? unit.replace_use(1, use_graph())
                                    : method == 4 ? unit.replace_grant_from_owner(1, grant_graph())
                                                  : unit.synchronize_catalog(1);
                                require(status == PermissionGraphWrite::staged,
                                        "Mutation source exact row count fits");
                            };
                            if (fit)
                                action();
                            else
                                storage_error(action, PermissionGraphFailure::resource_limit);
                        }
                    });
                    if (!fit || method < 2)
                        require(physical(observer) == before, "Row refusal/read writes nothing");
                }
            }
        });
    }
    void upperValidLimitsAreAcceptedWithoutOverflow() {
        scenario([] {
            Database db;
            const auto maximum = std::uint64_t(std::numeric_limits<qint64>::max());
            for (int field = 0; field < 6; ++field) {
                auto l = PermissionGraphLimits{};
                if (field == 0)
                    l.rows = maximum - 1;
                if (field == 1)
                    l.items = maximum - 1;
                // Largest representable field ceiling with terminator space.
                if (field == 2)
                    l.reference_bytes = maximum - 1;
                if (field == 3)
                    l.component_bytes = maximum - 1;
                // Checked 3*S+M <= signed capacity; defaults in other field.
                if (field == 4)
                    l.selected_bytes = (maximum - l.mutation_bytes) / 3;
                if (field == 5)
                    l.mutation_bytes = maximum - 3 * l.selected_bytes;
                db.work([&](const auto &s) {
                    SqlitePermissionGraphUnit unit(s, "I", db.trusted, l);
                    const auto result = unit.load();
                    require(std::holds_alternative<StoredPermissionGraph>(result),
                            "Upper valid limit accepted; no allocation of limit-sized buffer");
                });
            }
        });
    }
};
QTEST_GUILESS_MAIN(SqlitePermissionGraphTests)
#include "sqlite_permission_graph_tests.moc"
