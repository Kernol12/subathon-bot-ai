// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/sqlite_account_sessions_fixture.hpp"
#include <QRegularExpression>
#include <array>
#include <future>
#include <limits>
#include <subathon/server/sqlite_account_sessions.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
#include <tuple>

namespace subathon::server::testing {
// Test-only inert counter setup. No session/connection, payload or policy bypass.
class SqliteAccountSessionTestAccess final {
  public:
    static void ordinal(SqliteAccountSessionUnit &unit, std::uint64_t value) {
        unit.read_ordinal_ = value;
        unit.read_ordinal_exhausted_ = false;
    }
};
} // namespace subathon::server::testing
using namespace account_sessions_fixture;
namespace {
constexpr auto stale = LifecycleWriteResult::stale, staged = LifecycleWriteResult::staged,
               collision = LifecycleWriteResult::collision;
AccountSessionReadScope scope() { return {{}}; }
AccountSessionDigest digest(unsigned char last = 0) {
    AccountSessionDigest result{};
    result.back() = last;
    return result;
}
StoredInstallationFacts installation() { return {{"I", "E"}, "J"}; }
StoredAccountFacts account() { return {{"I", "A", "F", true}, "K"}; }
std::string h0() {
    return "$argon2id$v=19$m=4882,t=2,p=1$bA81arsiXysd3WbTRzmEOw$Nm8QBM+"
           "7RH1DXo9rvp5cwKEOOOfD2g6JuxlXihoNcpE";
}
StoredLocalCredential local() { return {"I", {"A", "L", h0()}}; }
SessionRecordFacts record() {
    return {"I",  "A",  "S",     "E", "F", "R", digest(), SessionRecordState::active,
            1000, 1000, 86401000};
}
AcceptedSessionAuthentication accepted() {
    // Assumed trusted-caller precondition for PHYSICAL staging only. This fixture
    // does not implement, grant or certify host acceptance, membership or replay.
    return {"I", "A", "t", "E", "F", "K", "L", "U", AcceptedSignInMethod::local_password, 1000};
}
std::string snapshot(unsigned char ordinal = 0) {
    std::string result("SAS1", 4);
    result.append(40, '\0');
    result.back() = char(ordinal);
    return result;
}
SessionRecordFacts rotated() {
    auto result = record();
    result.record_revision_ref = "Q";
    result.digest = digest(1);
    return result;
}
template <class T> const T &facts(const AccountSessionRead<T> &value) {
    require(std::holds_alternative<T>(value),
            "Complete typed storage success, not a partial/failure value");
    return std::get<T>(value);
}
template <class T>
void failure(const AccountSessionRead<T> &value, AccountSessionReadFailure wanted) {
    require(std::holds_alternative<AccountSessionReadFailure>(value) &&
                std::get<AccountSessionReadFailure>(value) == wanted,
            "Exact typed storage read failure");
}
void same_record(const SessionRecordFacts &seen, const SessionRecordFacts &wanted) {
    require(std::tie(seen.installation_ref, seen.account_ref, seen.session_ref,
                     seen.installation_epoch_ref, seen.account_epoch_ref, seen.record_revision_ref,
                     seen.digest, seen.state, seen.lifetime_started_utc_ms,
                     seen.authenticated_utc_ms, seen.expires_utc_ms) ==
                std::tie(wanted.installation_ref, wanted.account_ref, wanted.session_ref,
                         wanted.installation_epoch_ref, wanted.account_epoch_ref,
                         wanted.record_revision_ref, wanted.digest, wanted.state,
                         wanted.lifetime_started_utc_ms, wanted.authenticated_utc_ms,
                         wanted.expires_utc_ms),
            "All exact source session fields");
}
void complete(const CompleteSessionFacts &seen, const SessionRecordFacts &wanted = record(),
              std::string account_epoch = "F", std::string install_epoch = "E",
              bool enabled = true) {
    same_record(seen.session, wanted);
    require(seen.account.installation_ref == "I" && seen.account.account_ref == "A" &&
                seen.account.session_epoch_ref == account_epoch &&
                seen.account.application_access_enabled == enabled &&
                seen.installation.installation_ref == "I" &&
                seen.installation.session_epoch_ref == install_epoch,
            "Exact complete linked current account/installation");
}
template <class Body> void input_error(Body body, AccountSessionInputError wanted) {
    bool observed = false;
    try {
        body();
    } catch (const AccountSessionInputFailure &e) {
        observed = true;
        require(e.code() == wanted &&
                    std::string(e.what()) == "Account session storage input rejected",
                "Exact input failure category and redacted text");
    }
    require(observed, "Input refusal occurs before SQL");
}
template <class Body> void work_error(Body body, WorkError wanted) {
    bool observed = false;
    try {
        body();
    } catch (const SqliteWorkFailure &e) {
        observed = true;
        require(e.error() == wanted, "Original writer error preserved");
    }
    require(observed, "Expected actual writer capability refusal");
}
class SyntheticPostCommitFailure final : public std::runtime_error {
  public:
    SyntheticPostCommitFailure() : std::runtime_error("synthetic postcommit body failure") {}
};
SqliteWorkResult feature_outcome(SqliteWriter &writer, SqliteWriter::Work body) {
    const auto problem = std::make_shared<std::string>();
    const auto result = outcome(writer, [body = std::move(body), problem](auto context) {
        try {
            return body(context);
        } catch (const SqliteWorkFailure &) {
            throw;
        } catch (const SyntheticPostCommitFailure &) {
            throw;
        } catch (const std::exception &error) {
            *problem = error.what();
            throw;
        }
    });
    if (!problem->empty())
        throw std::runtime_error(*problem);
    return result;
}
void insert_page_rows(const SqliteWorkSession &s) {
    (void)s.execute({"DELETE FROM main.subathon_account_sessions", {}});
    for (const auto &pair :
         std::array<std::pair<QByteArray, int>, 4>{{{QByteArray::fromHex("00"), 0},
                                                    {QByteArray::fromHex("41"), 1},
                                                    {QByteArray::fromHex("4100"), 2},
                                                    {QByteArray::fromHex("ff"), 3}}}) {
        const auto d = digest(static_cast<unsigned char>(pair.second));
        (void)s.execute({"INSERT INTO main.subathon_account_sessions "
                         "VALUES(X'49',X'41',?,X'45',X'46',X'52',1,?,0,0,1000,1000,86401000)",
                         {pair.first, QByteArray(reinterpret_cast<const char *>(d.data()), 32)}});
    }
}
void page(const LifecyclePageFacts &seen, const std::vector<std::string> &refs) {
    require(seen.account.installation_ref == "I" && seen.account.account_ref == "A" &&
                seen.account.session_epoch_ref == "F" && seen.account.application_access_enabled &&
                seen.installation.installation_ref == "I" &&
                seen.installation.session_epoch_ref == "E" && seen.sessions.size() == refs.size(),
            "Complete bounded page closure including probe");
    for (std::size_t i = 0; i < refs.size(); ++i) {
        auto expected = record();
        expected.session_ref = refs[i];
        if (refs[i] == "A" || refs[i] == "T")
            expected.digest = digest(1);
        if (refs[i] == std::string("A\0", 2) || refs[i] == "U")
            expected.digest = digest(2);
        if (refs[i] == std::string(1, char(0xff)))
            expected.digest = digest(3);
        if (refs[i] == std::string(1, char(0xfe)))
            expected.digest = digest(5);
        if (refs[i] == "z0000" || refs[i] == "z0001") {
            expected.digest.fill(0x30);
            if (refs[i] == "z0001")
                expected.digest.back() = 0x31;
        }
        same_record(seen.sessions[i], expected);
    }
}
class Clock final : public CurrentSessionClock {
  public:
    std::int64_t now = 1000;
    int calls = 0;
    SessionClockResolution sample_current() override {
        ++calls;
        return SessionClockSample{now};
    }
};
} // namespace
class SqliteAccountSessionsTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        QTest::failOnWarning(
            QRegularExpression(".*(QSqlDatabase|QSqlQuery|different thread|still in use).*"));
    }
    void manifestMatchesFrozenPrefixAndStepEight() {
        scenario([] {
            const Data data;
            const auto actual = account_session_repository_manifest();
            require(actual.catalog_format == 1 &&
                        actual.store_identity == "subathon.sqlite-store" &&
                        actual.supported_starting_versions ==
                            QList<qint32>{0, 1, 2, 3, 4, 5, 6, 7, 8} &&
                        actual.steps.size() == 8,
                    "Exact complete additive catalog identity/starting versions");
            auto expected = data.prefixes;
            expected.append(data.frozen["manifest"].toObject());
            const auto pins = data.frozen["prefixes"].toArray();
            for (qsizetype i = 0; i < actual.steps.size(); ++i) {
                const auto literal = literal_step(expected[i]);
                require(actual.steps[i].version == literal.version &&
                            actual.steps[i].id == literal.id &&
                            actual.steps[i].sql == literal.sql &&
                            actual.steps[i].expected_schema == literal.expected_schema,
                        "Every compiled prefix/new statement and full schema tuple matches "
                        "independent literal");
                const auto bytes = i < 7 ? pins[i].toObject()["payload_bytes"].toInt()
                                         : expected[i]["payload_bytes"].toInt();
                require(canonical_migration_payload(actual.steps[i]).size() == bytes &&
                            migration_content_sha256(actual.steps[i]).toHex() ==
                                expected[i]["sha256_hex"].toString().toLatin1(),
                        "Each compiled catalog frame/hash matches independently frozen identity");
            }
            require(actual.steps.back().version == 8 &&
                        actual.steps.back().id == "account-sessions-v1" &&
                        actual.steps.back().sql.size() == 5 &&
                        actual.steps.back().expected_schema.size() == 102,
                    "Integrator-reserved additive8 five statements/full102 objects");
        });
    }
    // AS01/12: no state/epoch/time filtering in the exact digest source.
    void exactDigestLookupAndFreshSnapshots() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                const auto first = unit.load_current({"I", digest()}, {});
                require(std::holds_alternative<CompleteSessionFacts>(first),
                        "Exact digest lookup complete");
                complete(std::get<CompleteSessionFacts>(first));
                require(std::get<CompleteSessionFacts>(first).snapshot_ref == snapshot(),
                        "Literal SAS1/scope0/BEordinal0");
                require(
                    std::holds_alternative<SessionMissing>(unit.load_current({"I", digest(1)}, {})),
                    "Other digest honestly missing");
                const auto second = unit.read_selected("I", "A", "S");
                complete(facts(second));
                require(facts(second).snapshot_ref == snapshot(1),
                        "Only successful complete snapshot-producing reads advance ordinal");
                require(
                    std::holds_alternative<SessionMissing>(unit.load_current({"X", digest()}, {})),
                    "Installation part of exact actor key");
                failure(unit.read_selected("I", "B", "S"), AccountSessionReadFailure::missing);
                s.commit();
                return 1;
            });
        });
    }
    void rawFactsAndCredentialKeepEveryByte() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                const auto i = unit.read_installation("I");
                require(facts(i).current.installation_ref == "I" &&
                            facts(i).current.session_epoch_ref == "E" &&
                            facts(i).revision_ref == "J",
                        "Complete installation exact key/epoch/revision");
                const auto a = unit.read_account("I", "A");
                require(facts(a).current.session_epoch_ref == "F" && facts(a).revision_ref == "K" &&
                            facts(a).current.application_access_enabled,
                        "Complete current account facts");
                const auto l = unit.read_local_credential("I", "A");
                require(facts(l).local.installation_ref == "I" &&
                            facts(l).local.credential.account_ref == "A" &&
                            facts(l).local.credential.revision_ref == "L" &&
                            facts(l).local.credential.encoded_verifier == h0() &&
                            facts(l).account.revision_ref == "K" &&
                            facts(l).installation.revision_ref == "J" &&
                            facts(l).snapshot_ref == snapshot(),
                        "All raw local/source fences and exact upstream verifier");
                s.commit();
                return 1;
            });
        });
    }
    // AS02/03: O(1) current fences leave all session captures unchanged.
    void epochAndAccessChangesRetainCapturedRows() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                auto next = account();
                next.current.session_epoch_ref = "G";
                next.revision_ref = "M";
                require(unit.replace_account(account(), next) == staged,
                        "Exact current account epoch CAS stages");
                require(unit.replace_installation_epoch(installation(), "H", "N") == staged,
                        "Exact singleton epoch CAS stages");
                complete(facts(unit.read_selected("I", "A", "S")), record(), "G", "H");
                auto disabled = next;
                disabled.current.application_access_enabled = false;
                disabled.revision_ref = "O";
                require(unit.replace_account(next, disabled) == staged,
                        "Access may change independently");
                complete(facts(unit.read_selected("I", "A", "S")), record(), "G", "H", false);
                Clock clock;
                const auto resolution =
                    resolve_current_account_session({"I", digest()}, false, unit, clock);
                require(std::get<SessionFailure>(resolution) == SessionFailure::revoked &&
                            clock.calls == 0,
                        "Complete epoch/access mismatch is supplier revoked before clock");
                s.commit();
                return 1;
            });
            Observer observer(db.path);
            require(observer.rows("SELECT "
                                  "hex(installation_epoch_ref),hex(account_epoch_ref),hex(record_"
                                  "revision_ref),state,lifetime_started_utc_ms,authenticated_utc_"
                                  "ms,expires_utc_ms FROM main.subathon_account_sessions") ==
                        SqlRows{{QString("45"), QString("46"), QString("52"), 0, qint64(1000),
                                 qint64(1000), qint64(86401000)}},
                    "Epoch/access replacement never rewrites sessions");
        });
    }
    // AS04: storage returns complete valid revoked/expired data; supplier owns clock.
    void revokedAndExpiredStayComplete() {
        scenario([] {
            for (const auto revoked : {false, true}) {
                Database db;
                if (revoked)
                    corrupt(db.path, {"UPDATE main.subathon_account_sessions SET state=1"});
                (void)run(db.writer, [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    SqliteAccountSessionUnit unit(s, scope());
                    auto expected = record();
                    if (revoked)
                        expected.state = SessionRecordState::revoked;
                    complete(facts(unit.read_selected("I", "A", "S")), expected);
                    const auto actor = unit.read_lifecycle_actor({"I", digest()}, {});
                    complete(facts(actor).current, expected);
                    require(facts(actor).retention == SessionRetention::normal,
                            "Stored retention preserved");
                    Clock clock;
                    clock.now = 86401000;
                    const auto r =
                        resolve_current_account_session({"I", digest()}, false, unit, clock);
                    require(std::get<SessionFailure>(r) ==
                                    (revoked ? SessionFailure::revoked : SessionFailure::expired) &&
                                clock.calls == (revoked ? 0 : 1),
                            "Actual supplier expiration/revocation ordering");
                    s.commit();
                    return 1;
                });
            }
        });
    }
    // AS05/06: hidden physical fields/links validated before effective state.
    void corruptSelectedClosureNeverBecomesMissingOrRevoked() {
        scenario([] {
            for (const auto *sql :
                 {"DELETE FROM main.subathon_accounts",
                  "DELETE FROM main.subathon_account_installation",
                  "UPDATE main.subathon_account_sessions SET digest_format=2",
                  "UPDATE main.subathon_account_sessions SET retention=2",
                  "UPDATE main.subathon_account_sessions SET state=2",
                  "UPDATE main.subathon_account_sessions SET expires_utc_ms=1000",
                  "UPDATE main.subathon_account_sessions SET authenticated_utc_ms=1000.5",
                  "UPDATE main.subathon_account_sessions SET installation_epoch_ref=X''",
                  "UPDATE main.subathon_account_sessions SET account_epoch_ref=1.5",
                  "UPDATE main.subathon_accounts SET revision_ref=X''",
                  "UPDATE main.subathon_accounts SET application_access_enabled=1.5",
                  "UPDATE main.subathon_account_installation SET singleton=2",
                  "UPDATE main.subathon_account_installation SET revision_ref=17"}) {
                Database db;
                corrupt(db.path, {QString::fromLatin1(sql)});
                (void)run(db.writer, [](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    SqliteAccountSessionUnit unit(s, scope());
                    failure(unit.read_selected("I", "A", "S"),
                            AccountSessionReadFailure::invalid_data);
                    require(std::holds_alternative<SessionSourceInvalidData>(
                                unit.load_current({"I", digest()}, {})),
                            "Selected invalid closure maps explicitly to source invalid_data");
                    bool refused = false;
                    try {
                        (void)unit.revoke_session(record(), "V");
                    } catch (const AccountSessionStorageFailure &error) {
                        refused = true;
                        require(
                            error.code() == AccountSessionReadFailure::invalid_data &&
                                std::string(error.what()) ==
                                    "Account session storage operation failed",
                            "Mutation corruption has exact redacted storage failure, not stale");
                    }
                    require(refused, "Full selected corruption preflight precedes mutation CAS");
                    s.commit();
                    return 1;
                });
            }
        });
    }
    void selectedDigestClassAndLengthAreExact() {
        scenario([] {
            for (const auto *sql :
                 {"UPDATE main.subathon_account_sessions SET digest=zeroblob(31)",
                  "UPDATE main.subathon_account_sessions SET "
                  "digest='0000000000000000000000000000000000000000000000000000000000000000'"}) {
                Database db;
                corrupt(db.path, {QString::fromLatin1(sql)});
                (void)run(db.writer, [](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    SqliteAccountSessionUnit unit(s, scope());
                    failure(unit.read_selected("I", "A", "S"),
                            AccountSessionReadFailure::invalid_data);
                    require(std::holds_alternative<SessionMissing>(
                                unit.load_current({"I", digest()}, {})),
                            "Wrong-key corrupt digest is outside exact actor key; no global scan");
                    s.commit();
                    return 1;
                });
            }
        });
    }
    // AS16/21/24: complete semantic variants, including fixed snapshot and duplicates.
    void completeSelectedAndSupplierBudgetsUseLiteralBytes() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                AccountSessionStoreLimits limits;
                limits.selected_bytes = 114;
                SqliteAccountSessionUnit exact(s, scope(), limits);
                complete(facts(exact.read_selected("I", "A", "S")));
                limits.selected_bytes = 113;
                SqliteAccountSessionUnit short_unit(s, scope(), limits);
                failure(short_unit.read_selected("I", "A", "S"),
                        AccountSessionReadFailure::resource_limit);
                require(std::holds_alternative<SessionSourceResourceLimit>(
                            short_unit.load_current({"I", digest()}, {})),
                        "Store bound survives supplier mapping");
                SqliteAccountSessionUnit supplier(s, scope());
                SessionLimits sl;
                sl.owned_bytes = 55;
                require(std::holds_alternative<CompleteSessionFacts>(
                            supplier.load_current({"I", digest()}, sl)),
                        "Literal supplier strings55 fit");
                sl.owned_bytes = 54;
                require(std::holds_alternative<SessionSourceResourceLimit>(
                            supplier.load_current({"I", digest()}, sl)),
                        "Supplier strings54 refuses complete source");
                sl = {};
                sl.reference_bytes = 43;
                require(std::holds_alternative<SessionSourceResourceLimit>(
                            supplier.load_current({"I", digest()}, sl)),
                        "Snapshot44 is a reference with its own bound");
                limits = {};
                limits.selected_bytes = 115;
                SqliteAccountSessionUnit actor(s, scope(), limits);
                require(std::holds_alternative<LifecycleCurrentSession>(
                            actor.read_lifecycle_actor({"I", digest()}, {})),
                        "Retention adds one semantic byte");
                limits.selected_bytes = 114;
                SqliteAccountSessionUnit actor_short(s, scope(), limits);
                failure(actor_short.read_lifecycle_actor({"I", digest()}, {}),
                        AccountSessionReadFailure::resource_limit);
                s.commit();
                return 1;
            });
        });
    }
    void metadataOveragePrecedesMalformedRevokedPayload() {
        scenario([] {
            for (const auto *sql :
                 {"UPDATE main.subathon_accounts SET session_epoch_ref=zeroblob(65);",
                  "UPDATE main.subathon_accounts SET revision_ref=zeroblob(65);",
                  "UPDATE main.subathon_account_installation SET revision_ref=zeroblob(65);"}) {
                Database db;
                QString update = QString::fromLatin1(sql);
                update.chop(1);
                corrupt(db.path,
                        {update, "UPDATE main.subathon_account_sessions SET state=1,retention=2"});
                (void)run(db.writer, [](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    AccountSessionStoreLimits l;
                    l.reference_bytes = 64;
                    SqliteAccountSessionUnit unit(s, scope(), l);
                    failure(unit.read_selected("I", "A", "S"),
                            AccountSessionReadFailure::resource_limit);
                    require(std::holds_alternative<SessionSourceResourceLimit>(
                                unit.load_current({"I", digest()}, {})),
                            "Complete linked overage before malformed/revoked semantics");
                    s.commit();
                    return 1;
                });
            }
            Database db;
            corrupt(db.path, {"UPDATE main.subathon_local_credentials SET "
                              "encoded_verifier=zeroblob(128),verifier_format=2"});
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                failure(unit.read_local_credential("I", "A"),
                        AccountSessionReadFailure::resource_limit);
                s.commit();
                return 1;
            });
        });
    }
    // AS17/18: storage keeps raw fitting bytes; helper/actual source bridge remain owners.
    void localRawRecordsMissingAndOrphansAreDistinct() {
        scenario([] {
            for (const std::string raw : {"", "$",
                                          "$argon2i$v=19$m=4882,t=2,p=1$bA81arsiXysd3WbTRzmEOw$"
                                          "Nm8QBM+7RH1DXo9rvp5cwKEOOOfD2g6JuxlXihoNcpE"}) {
                Database db;
                (void)run(db.writer, [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    (void)s.execute(
                        {"UPDATE main.subathon_local_credentials SET encoded_verifier=?",
                         {blob(raw)}});
                    SqliteAccountSessionUnit unit(s, scope());
                    const auto selected = unit.read_local_credential("I", "A");
                    require(facts(selected).local.credential.encoded_verifier == raw,
                            "Raw fitting malformed/unsupported PHC is retained exactly, with "
                            "constraints ON");
                    s.commit();
                    return 1;
                });
            }
            for (const auto orphan : {false, true}) {
                Database db;
                corrupt(db.path, {orphan ? "DELETE FROM main.subathon_accounts"
                                         : "DELETE FROM main.subathon_local_credentials"});
                (void)run(db.writer, [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    SqliteAccountSessionUnit unit(s, scope());
                    failure(unit.read_local_credential("I", "A"),
                            orphan ? AccountSessionReadFailure::invalid_data
                                   : AccountSessionReadFailure::missing);
                    s.commit();
                    return 1;
                });
            }
        });
    }
    void completeLocalBudgetAndHiddenFormatAreChecked() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                AccountSessionStoreLimits l;
                l.selected_bytes = 152;
                SqliteAccountSessionUnit exact(s, scope(), l);
                require(facts(exact.read_local_credential("I", "A"))
                                .local.credential.encoded_verifier == h0(),
                        "Literal local success B152");
                l.selected_bytes = 151;
                SqliteAccountSessionUnit short_unit(s, scope(), l);
                failure(short_unit.read_local_credential("I", "A"),
                        AccountSessionReadFailure::resource_limit);
                s.commit();
                return 1;
            });
            corrupt(db.path, {"UPDATE main.subathon_local_credentials SET verifier_format=2"});
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                failure(unit.read_local_credential("I", "A"),
                        AccountSessionReadFailure::invalid_data);
                s.commit();
                return 1;
            });
        });
    }
    void installationAndAccountInsertAndFullCas() {
        scenario([] {
            Database db(false);
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                require(unit.insert_installation(installation()) == staged &&
                            unit.insert_installation(installation()) == collision,
                        "Installation absent insert never adopts row");
                require(unit.insert_account(account()) == staged &&
                            unit.insert_account(account()) == collision,
                        "Account key collision never means registration accepted");
                auto wrong = installation();
                wrong.revision_ref = "x";
                require(unit.replace_installation_epoch(wrong, "H", "N") == stale,
                        "Full installation revision CAS");
                wrong = installation();
                wrong.current.session_epoch_ref = "x";
                require(unit.replace_installation_epoch(wrong, "H", "N") == stale,
                        "Full installation epoch CAS");
                auto replacement = account();
                replacement.current.application_access_enabled = false;
                replacement.revision_ref = "M";
                auto wrong_account = account();
                wrong_account.current.application_access_enabled = false;
                require(unit.replace_account(wrong_account, replacement) == stale,
                        "Access participates in account CAS");
                wrong_account = account();
                wrong_account.current.session_epoch_ref = "x";
                require(unit.replace_account(wrong_account, replacement) == stale,
                        "Epoch participates even with same revision");
                require(unit.replace_account(account(), replacement) == staged &&
                            unit.replace_account(account(), replacement) == stale,
                        "Single winner for full expected account");
                s.commit();
                return 1;
            });
        });
    }
    // AS13/14: exact bytes/revisions/fences only; no DTO accepted-login claim.
    void localReplacementUsesFullSourceAndCurrentFences() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                auto replacement = local();
                replacement.credential.revision_ref = "Z";
                auto wrong = local();
                wrong.credential.encoded_verifier.back() = 'A';
                require(unit.replace_local_credential(wrong, replacement, account(),
                                                      installation()) == stale,
                        "Full verifier bytes required even with same revision");
                auto wrong_account = account();
                wrong_account.revision_ref = "x";
                require(unit.replace_local_credential(local(), replacement, wrong_account,
                                                      installation()) == stale,
                        "Account fence required");
                auto wrong_install = installation();
                wrong_install.revision_ref = "x";
                require(unit.replace_local_credential(local(), replacement, account(),
                                                      wrong_install) == stale,
                        "Installation fence required");
                require(unit.replace_local_credential(std::nullopt, replacement, account(),
                                                      installation()) == collision,
                        "Absence insert cannot adopt existing local record");
                require(unit.replace_local_credential(local(), replacement, account(),
                                                      installation()) == staged,
                        "Exact local CAS stages");
                require(unit.replace_local_credential(local(), replacement, account(),
                                                      installation()) == stale,
                        "Reset loses old lineage");
                const auto current = unit.read_local_credential("I", "A");
                require(facts(current).local.credential.revision_ref == "Z" &&
                            facts(current).local.credential.encoded_verifier == h0() &&
                            facts(current).account.revision_ref == "K" &&
                            facts(current).installation.revision_ref == "J",
                        "Only credential/revision changed");
                s.commit();
                return 1;
            });
        });
    }
    void passwordUpgradeChecksFullExpectedSourceAndRollback() {
        scenario([] {
            Database db;
            const auto result = feature_outcome(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                const auto source = unit.read_local_credential("I", "A");
                PasswordReplacement proposal{"A", "t", "L", h0(), h0()};
                proposal.expected_encoded_verifier.back() = 'A';
                require(unit.replace_password_upgrade(proposal, "Z", facts(source)) == stale,
                        "Mismatched FULL verifier never upgrades");
                proposal.expected_encoded_verifier = h0();
                // Caller already checked actual accepted local login in this scope.
                // This assertion tests only its physical conditional primitive.
                require(unit.replace_password_upgrade(proposal, "Z", facts(source)) == staged,
                        "Physical full-source conditional upgrade stages");
                require(facts(unit.read_local_credential("I", "A")).local.credential.revision_ref ==
                            "Z",
                        "Same unit sees provisional new lineage");
                s.rollback();
                return 1;
            });
            require(result.error == WorkError::not_committed &&
                        result.transaction == TransactionOutcome::rolled_back && !result.value,
                    "No normal result after actual known upgrade rollback");
            Observer observer(db.path);
            require(observer.rows("SELECT hex(revision_ref),encoded_verifier FROM "
                                  "main.subathon_local_credentials") ==
                        SqlRows{{QString("4C"), blob(h0())}},
                    "Known rollback retains literal H0/L");
            require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        db.writer, testing::WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none,
                    "Existing control seam for physical credential unknown outcome");
            const auto unknown = feature_outcome(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                const auto source = unit.read_local_credential("I", "A");
                const PasswordReplacement proposal{"A", "t", "L", h0(), h0()};
                require(unit.replace_password_upgrade(proposal, "Z", facts(source)) == staged,
                        "Physical conditional credential stage before unknown commit");
                s.commit();
                return 1;
            });
            require(unknown.transaction == TransactionOutcome::outcome_unknown &&
                        unknown.error == WorkError::uncertain_outcome && !unknown.value,
                    "Unknown credential commit claims neither normal result nor known rollback");
            require(
                observer.rows("SELECT hex(revision_ref) FROM main.subathon_local_credentials") ==
                    SqlRows{{QString("5A")}},
                "Reached physical credential commit may persist; store never restores old source");
        });
    }
    void physicalSessionInsertAndCollisionNeverAdopt() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                auto candidate = record();
                candidate.digest = digest(1);
                candidate.record_revision_ref = "Q";
                require(unit.insert_session(candidate, SessionRetention::normal, accepted()) ==
                            collision,
                        "Global session identity collision");
                candidate.session_ref = "T";
                candidate.digest = digest();
                require(unit.insert_session(candidate, SessionRetention::normal, accepted()) ==
                            collision,
                        "Installation+digest collision");
                candidate.digest = digest(1);
                require(unit.insert_session(candidate, SessionRetention::normal, accepted()) ==
                            staged,
                        "Absent physical row stages under trusted caller precondition");
                complete(facts(unit.read_selected("I", "A", "T")), candidate);
                s.commit();
                return 1;
            });
            Observer observer(db.path);
            require(observer.rows("SELECT hex(session_ref),hex(record_revision_ref) FROM "
                                  "main.subathon_account_sessions ORDER BY session_ref") ==
                        SqlRows{{QString("53"), QString("52")}, {QString("54"), QString("51")}},
                    "Collision never overwrites/adopts S");
        });
    }
    void issueFencesObserveCurrentEpochAccessAndRevisions() {
        scenario([] {
            for (const auto *sql :
                 {"UPDATE main.subathon_accounts SET session_epoch_ref=X'47',revision_ref=X'4d'",
                  "UPDATE main.subathon_accounts SET "
                  "application_access_enabled=0,revision_ref=X'4d'",
                  "UPDATE main.subathon_accounts SET revision_ref=X'4d'",
                  "UPDATE main.subathon_account_installation SET "
                  "session_epoch_ref=X'48',revision_ref=X'4e'"}) {
                Database db;
                corrupt(db.path, {QString::fromLatin1(sql)});
                (void)run(db.writer, [](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    SqliteAccountSessionUnit unit(s, scope());
                    auto candidate = rotated();
                    candidate.session_ref = "T";
                    require(unit.insert_session(candidate, SessionRetention::normal, accepted()) ==
                                stale,
                            "Store rechecks deterministic current issuance fences");
                    failure(unit.read_selected("I", "A", "T"), AccountSessionReadFailure::missing);
                    s.commit();
                    return 1;
                });
            }
        });
    }
    // AS08: matching revision alone cannot overwrite changed source bytes.
    void rotationUsesFullExpectedRowAndImmutableTimes() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                for (int field = 0; field < 5; ++field) {
                    auto expected = record(), replacement = rotated();
                    if (field == 0)
                        expected.digest = digest(2);
                    if (field == 1) {
                        expected.lifetime_started_utc_ms = 999;
                        replacement.lifetime_started_utc_ms = 999;
                    }
                    if (field == 2) {
                        expected.authenticated_utc_ms = 1001;
                        replacement.authenticated_utc_ms = 1001;
                    }
                    if (field == 3) {
                        expected.expires_utc_ms = 86401001;
                        replacement.expires_utc_ms = 86401001;
                    }
                    if (field == 4) {
                        expected.account_epoch_ref = "G";
                        replacement.account_epoch_ref = "G";
                    }
                    require(unit.replace_session(expected, replacement, SessionRetention::normal,
                                                 nullptr) == stale,
                            "Every expected row component participates with unchanged expected "
                            "revision");
                }
                auto no_new_digest = record();
                no_new_digest.record_revision_ref = "Q";
                require(unit.replace_session(record(), no_new_digest, SessionRetention::normal,
                                             nullptr) == collision,
                        "Own old digest is explicit collision");
                require(unit.replace_session(record(), rotated(), SessionRetention::remembered,
                                             nullptr) == stale,
                        "Token rotation cannot alter persisted retention");
                require(unit.replace_session(record(), rotated(), SessionRetention::normal,
                                             nullptr) == staged,
                        "Exact full-row rotation stages once");
                require(unit.replace_session(record(), rotated(), SessionRetention::normal,
                                             nullptr) == stale,
                        "Stale second CAS changes zero rows");
                require(
                    std::holds_alternative<SessionMissing>(unit.load_current({"I", digest()}, {})),
                    "Old digest disappears in same real unit");
                const auto current = unit.load_current({"I", digest(1)}, {});
                complete(std::get<CompleteSessionFacts>(current), rotated());
                s.commit();
                return 1;
            });
        });
    }
    void reauthenticationChangesOnlyAcceptedTimeTupleAndRetention() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                auto replacement = rotated();
                replacement.lifetime_started_utc_ms = 2000;
                replacement.authenticated_utc_ms = 2000;
                replacement.expires_utc_ms = 2592002000;
                auto authentication = accepted();
                authentication.accepted_utc_ms = 2000;
                require(unit.replace_session(record(), replacement, SessionRetention::remembered,
                                             &authentication) == staged,
                        "Physical reauth stages exact accepted metadata under caller acceptance "
                        "precondition");
                const auto current = unit.read_lifecycle_actor({"I", digest(1)}, {});
                complete(facts(current).current, replacement);
                require(facts(current).retention == SessionRetention::remembered,
                        "Reauth retention persisted");
                s.commit();
                return 1;
            });
        });
    }
    // AS09: independently authorized revocation can target disabled/epoch-mismatched row.
    void revocationPreservesDigestEpochsTimesAndNoReactivation() {
        scenario([] {
            Database db;
            corrupt(db.path,
                    {"UPDATE main.subathon_accounts SET "
                     "application_access_enabled=0,session_epoch_ref=X'47',revision_ref=X'4d'"});
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                require(unit.revoke_session(record(), "V") == staged,
                        "Physical revoke need not authenticate target row");
                auto revoked = record();
                revoked.state = SessionRecordState::revoked;
                revoked.record_revision_ref = "V";
                complete(facts(unit.read_selected("I", "A", "S")), revoked, "G", "E", false);
                require(unit.revoke_session(record(), "W") == stale,
                        "Old active CAS cannot change already-revoked revision");
                s.commit();
                return 1;
            });
        });
    }
    // AS22: full signed SQL domain; unsigned duration calculation is lifecycle's owner.
    void signedExtremesSurviveReadRotationAndReopen() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                (void)s.execute(
                    {"UPDATE main.subathon_account_sessions SET "
                     "lifetime_started_utc_ms=?,authenticated_utc_ms=?,expires_utc_ms=?",
                     {qint64(std::numeric_limits<std::int64_t>::min()),
                      qint64(std::numeric_limits<std::int64_t>::min()),
                      qint64(std::numeric_limits<std::int64_t>::max())}});
                SqliteAccountSessionUnit unit(s, scope());
                auto expected = record(), replacement = rotated();
                expected.lifetime_started_utc_ms = expected.authenticated_utc_ms =
                    std::numeric_limits<std::int64_t>::min();
                expected.expires_utc_ms = std::numeric_limits<std::int64_t>::max();
                replacement.lifetime_started_utc_ms = replacement.authenticated_utc_ms =
                    std::numeric_limits<std::int64_t>::min();
                replacement.expires_utc_ms = std::numeric_limits<std::int64_t>::max();
                complete(facts(unit.read_selected("I", "A", "S")), expected);
                require(unit.replace_session(expected, replacement, SessionRetention::normal,
                                             nullptr) == staged,
                        "No signed duration narrowing in physical rotation");
                complete(facts(unit.read_selected("I", "A", "S")), replacement);
                s.commit();
                return 1;
            });
            SqliteReader reader;
            require(reader.open(db.path) == StorageError::none,
                    "Independent read-only fresh connection");
            const auto seen =
                reader.read({"SELECT lifetime_started_utc_ms,authenticated_utc_ms,expires_utc_ms "
                             "FROM main.subathon_account_sessions",
                             {}});
            require(seen.error == StorageError::none &&
                        seen.rows == SqlRows{{qint64(std::numeric_limits<std::int64_t>::min()),
                                              qint64(std::numeric_limits<std::int64_t>::min()),
                                              qint64(std::numeric_limits<std::int64_t>::max())}},
                    "Fresh reopen keeps literal signed MIN/MIN/MAX");
        });
    }
    // AS10/23: a provisional staged value cannot suppress a reached later SQL failure.
    void lateSqlFailureRollsBackWholeStagedInsert() {
        scenario([] {
            Database db;
            bool reached = false;
            const auto result = feature_outcome(db.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                auto candidate = rotated();
                candidate.session_ref = "T";
                require(unit.insert_session(candidate, SessionRetention::normal, accepted()) ==
                            staged,
                        "Insert provisional before late failure");
                try {
                    (void)s.execute(
                        {"INSERT INTO main.no_such_account_session_table VALUES(1)", {}});
                } catch (const SqliteWorkFailure &e) {
                    reached = e.error() == WorkError::sql_failure;
                    throw;
                }
                return 1;
            });
            require(reached && result.error == WorkError::sql_failure &&
                        result.transaction == TransactionOutcome::rolled_back && !result.value,
                    "Reached real SQL failure retains first sticky error and known whole rollback");
            Observer observer(db.path);
            require(observer.rows("SELECT hex(session_ref),hex(record_revision_ref) FROM "
                                  "main.subathon_account_sessions") ==
                        SqlRows{{QString("53"), QString("52")}},
                    "Fresh query proves no staged T and unchanged S");
        });
    }
    // AS11: seam loses ACK only after successful real SQL COMMIT.
    void unexpectedConstraintFailureIsActualSqlFailure() {
        scenario([] {
            Database db;
            corrupt(
                db.path,
                {"CREATE TRIGGER account_session_fault BEFORE UPDATE ON subathon_account_sessions "
                 "BEGIN SELECT RAISE(ABORT,'synthetic conditional write refusal'); END"});
            bool reached = false;
            const auto result = feature_outcome(db.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                try {
                    (void)unit.replace_session(record(), rotated(), SessionRetention::normal,
                                               nullptr);
                } catch (const SqliteWorkFailure &error) {
                    reached = error.error() == WorkError::sql_failure;
                    throw;
                }
                return 1;
            });
            require(reached && result.error == WorkError::sql_failure &&
                        result.transaction == TransactionOutcome::rolled_back && !result.value,
                    "Unexpected reached SQL constraint is never guessed collision or stale");
            Observer observer(db.path);
            require(observer.rows(
                        "SELECT hex(record_revision_ref) FROM main.subathon_account_sessions") ==
                        SqlRows{{QString("52")}},
                    "Unexpected conditional SQL failure preserves original row");
        });
    }
    void lostCommitAcknowledgementRetainsUnknownWithoutNormalValue() {
        scenario([] {
            Database db;
            require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        db.writer, testing::WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none,
                    "Install existing real-control ACK seam");
            const auto result = feature_outcome(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                require(unit.replace_session(record(), rotated(), SessionRetention::normal,
                                             nullptr) == staged,
                        "Provisional physical rotation");
                s.commit();
                return 1;
            });
            require(result.transaction == TransactionOutcome::outcome_unknown &&
                        result.error == WorkError::uncertain_outcome && !result.value,
                    "Unknown never claims rollback, stale, missing or publication success");
            Observer observer(db.path);
            require(observer.rows("SELECT hex(record_revision_ref),hex(digest) FROM "
                                  "main.subathon_account_sessions") ==
                        SqlRows{{QString("51"), QString(63, '0') + "1"}},
                    "Physical commit may survive lost ACK; no automatic restoration/replay");
        });
    }
    void postCommitExceptionPreservesRowsWithoutNormalValue() {
        scenario([] {
            Database db;
            const auto result = feature_outcome(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                require(unit.replace_session(record(), rotated(), SessionRetention::normal,
                                             nullptr) == staged,
                        "Provisional rotation");
                s.commit();
                throw SyntheticPostCommitFailure();
            });
            require(
                result.transaction == TransactionOutcome::committed &&
                    result.error == WorkError::work_exception && !result.value,
                "Confirmed commit plus exception is neither rollback nor ordinary successful work");
            Observer observer(db.path);
            require(observer.rows(
                        "SELECT hex(record_revision_ref) FROM main.subathon_account_sessions") ==
                        SqlRows{{QString("51")}},
                    "Committed Q stays durable");
        });
    }
    // AS12: each source call actually requeries; an external read snapshot gives no authority.
    void sameSessionFreshReadAndExternalSnapshotAreDistinct() {
        scenario([] {
            Database db;
            Observer observer(db.path);
            (void)observer.rows("BEGIN");
            require(observer.rows(
                        "SELECT hex(record_revision_ref) FROM main.subathon_account_sessions") ==
                        SqlRows{{QString("52")}},
                    "Pin old external WAL snapshot R");
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                const auto first = unit.read_selected("I", "A", "S");
                complete(facts(first));
                require(unit.replace_session(record(), rotated(), SessionRetention::normal,
                                             nullptr) == staged,
                        "Stage Q");
                const auto second = unit.read_selected("I", "A", "S");
                complete(facts(second), rotated());
                require(facts(first).snapshot_ref == snapshot() &&
                            facts(second).snapshot_ref == snapshot(1),
                        "Fresh same-session queries get new snapshots");
                s.commit();
                return 1;
            });
            require(observer.rows(
                        "SELECT hex(record_revision_ref) FROM main.subathon_account_sessions") ==
                        SqlRows{{QString("52")}},
                    "Preexisting external snapshot retains R after writer commit");
            (void)observer.rows("COMMIT");
            require(observer.rows(
                        "SELECT hex(record_revision_ref) FROM main.subathon_account_sessions") ==
                        SqlRows{{QString("51")}},
                    "New external snapshot sees Q");
        });
    }
    void queuedCompetingRotationsHaveExactlyOneWinner() {
        scenario([] {
            Database db;
            std::array<SqliteWorkResult, 2> results;
            std::array<LifecycleWriteResult, 2> writes{};
            int completed = 0;
            bool marker = false;
            for (std::size_t i = 0; i < 2; ++i) {
                require(db.writer.enqueue_work(
                            [&, i](auto context) -> QVariant {
                                try {
                                    const auto s = context.begin();
                                    Cleanup cleanup{s};
                                    SqliteAccountSessionUnit unit(s, scope());
                                    auto next = rotated();
                                    if (i) {
                                        next.record_revision_ref = "V";
                                        next.digest = digest(2);
                                    }
                                    writes[i] = unit.replace_session(
                                        record(), next, SessionRetention::normal, nullptr);
                                    s.commit();
                                    return 1;
                                } catch (const AccountSessionStorageNotImplemented &) {
                                    marker = true;
                                    throw;
                                }
                            },
                            [&, i](auto result) {
                                results[i] = std::move(result);
                                ++completed;
                            }) == StorageError::none,
                        "Both real queued competing commands admitted");
            }
            require(QTest::qWaitFor([&] { return completed == 2; }, 10000),
                    "Both competing commands actually complete");
            if (marker)
                throw AccountSessionStorageNotImplemented();
            require(writes[0] == staged && writes[1] == stale,
                    "Serialized full expected row permits exactly one rotation");
            for (const auto &r : results)
                require(r.transaction == TransactionOutcome::committed &&
                            r.error == WorkError::none && r.value,
                        "Each caller commits its actual completed decision");
            Observer observer(db.path);
            require(observer.rows("SELECT hex(record_revision_ref),hex(digest) FROM "
                                  "main.subathon_account_sessions") ==
                        SqlRows{{QString("51"), QString(63, '0') + "1"}},
                    "Only first candidate Q/D1 remains");
        });
    }
    void facadeDestructionNeverCleansCallerTransaction() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                {
                    SqliteAccountSessionUnit unit(s, scope());
                    require(unit.revoke_session(record(), "V") == staged, "Provisional revoke");
                }
                require(s.execute({"SELECT state FROM main.subathon_account_sessions", {}}) ==
                            SqlRows{{1}},
                        "Destroyed facade did not rollback caller transaction");
                s.commit();
                return 1;
            });
        });
    }
    // AS19/20: page source deliberately includes one validated probe; lifecycle consumes it.
    void pagesKeepRawBlobOrderProbeAndNonexistentBoundary() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                insert_page_rows(s);
                SqliteAccountSessionUnit unit(s, scope());
                const auto first = unit.read_page("I", "A", std::nullopt, 2);
                page(facts(first), {std::string(1, '\0'), "A", std::string("A\0", 2)});
                const auto second = unit.read_page("I", "A", std::optional<std::string>{"A"}, 2);
                page(facts(second), {std::string("A\0", 2), std::string(1, char(0xff))});
                const auto boundary = unit.read_page("I", "A", std::optional<std::string>{"@"}, 2);
                page(facts(boundary), {"A", std::string("A\0", 2), std::string(1, char(0xff))});
                const auto empty = unit.read_page(
                    "I", "A", std::optional<std::string>{std::string(1, char(0xff))}, 2);
                page(facts(empty), {});
                require(facts(first).snapshot_ref == snapshot() &&
                            facts(second).snapshot_ref == snapshot(1) &&
                            facts(boundary).snapshot_ref == snapshot(2) &&
                            facts(empty).snapshot_ref == snapshot(3),
                        "Every successful page is a fresh scoped read");
                s.commit();
                return 1;
            });
        });
    }
    void laterPageGetsNewSnapshotAndInterveningInserts() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                insert_page_rows(s);
                SqliteAccountSessionUnit unit(s, scope());
                page(facts(unit.read_page("I", "A", std::nullopt, 2)),
                     {std::string(1, '\0'), "A", std::string("A\0", 2)});
                s.commit();
                return 1;
            });
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                for (const auto &tuple : std::array<std::pair<QByteArray, int>, 2>{
                         {{QByteArray::fromHex("4000"), 4}, {QByteArray::fromHex("fe"), 5}}}) {
                    const auto d = digest(static_cast<unsigned char>(tuple.second));
                    (void)s.execute(
                        {"INSERT INTO main.subathon_account_sessions "
                         "VALUES(X'49',X'41',?,X'45',X'46',X'52',1,?,0,0,1000,1000,86401000)",
                         {tuple.first, QByteArray(reinterpret_cast<const char *>(d.data()), 32)}});
                }
                AccountSessionReadScope next_scope{};
                next_scope.read_scope_ref.back() = 1;
                SqliteAccountSessionUnit unit(s, next_scope);
                const auto next = unit.read_page("I", "A", std::optional<std::string>{"A"}, 2);
                page(facts(next), {std::string("A\0", 2), std::string(1, char(0xfe)),
                                   std::string(1, char(0xff))});
                auto wanted = snapshot();
                wanted[35] = 1;
                require(facts(next).snapshot_ref == wanted,
                        "New work has distinct caller-provided scope, no request snapshot lease");
                s.commit();
                return 1;
            });
        });
    }
    void malformedProbeFailsWholePageBeforeAnyOutput() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                insert_page_rows(s);
                s.commit();
                return 1;
            });
            corrupt(db.path, {"UPDATE main.subathon_account_sessions SET retention=2 WHERE "
                              "session_ref=X'4100'"});
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                failure(unit.read_page("I", "A", std::nullopt, 2),
                        AccountSessionReadFailure::invalid_data);
                const auto after =
                    unit.read_page("I", "A", std::optional<std::string>{std::string("A\0", 2)}, 2);
                page(facts(after), {std::string(1, char(0xff))});
                require(facts(after).snapshot_ref == snapshot(),
                        "Failed complete probe did not consume successful-read ordinal");
                s.commit();
                return 1;
            });
        });
    }
    void completePageBudgetCountsProbeAndRepeatedSourceReferences() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                // Three ONE-byte session refs: source success B=51+3*63=240 exactly.
                (void)s.execute({"DELETE FROM main.subathon_account_sessions", {}});
                for (int n = 0; n < 3; ++n) {
                    const auto d = digest(static_cast<unsigned char>(n));
                    (void)s.execute(
                        {"INSERT INTO main.subathon_account_sessions "
                         "VALUES(X'49',X'41',?,X'45',X'46',X'52',1,?,0,0,1000,1000,86401000)",
                         {QByteArray(1, char('S' + n)),
                          QByteArray(reinterpret_cast<const char *>(d.data()), 32)}});
                }
                AccountSessionStoreLimits limits;
                limits.selected_bytes = 240;
                SqliteAccountSessionUnit exact(s, scope(), limits);
                page(facts(exact.read_page("I", "A", std::nullopt, 2)), {"S", "T", "U"});
                limits.selected_bytes = 239;
                SqliteAccountSessionUnit short_unit(s, scope(), limits);
                failure(short_unit.read_page("I", "A", std::nullopt, 2),
                        AccountSessionReadFailure::resource_limit);
                s.commit();
                return 1;
            });
        });
    }
    void manyUnselectedRowsStayOutsideFiniteSelectedQueries() {
        scenario([] {
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable query-bound database");
            const auto path = directory.filePath("bounded.sqlite");
            const Data data;
            {
                SqliteWriter setup;
                require(setup.open(path, limits()) == StorageError::none, "Setup writer opens");
                data.schema(setup);
                (void)run(setup, [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    data.populate(s);
                    (void)s.execute({"WITH RECURSIVE n(x) AS (VALUES(0) UNION ALL SELECT x+1 FROM "
                                     "n WHERE x<511) INSERT INTO main.subathon_account_sessions "
                                     "SELECT X'49',X'41',CAST(printf('z%04d',x) AS "
                                     "BLOB),X'45',X'46',X'52',1,CAST(printf('%032d',x) AS "
                                     "BLOB),0,0,1000,1000,86401000 FROM n",
                                     {}});
                    s.commit();
                    return 1;
                });
            }
            corrupt(path, {"UPDATE main.subathon_account_sessions SET retention=2 WHERE "
                           "session_ref=X'7a30353131'"});
            auto bound = limits();
            bound.statements_per_batch = 256;
            bound.result_rows = 256;
            bound.result_value_bytes = 65536;
            SqliteWriter writer;
            require(writer.open(path, bound) == StorageError::none,
                    "Finite-budget selected writer opens");
            (void)run(writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                complete(facts(unit.read_selected("I", "A", "S")));
                page(facts(unit.read_page("I", "A", std::nullopt, 2)), {"S", "z0000", "z0001"});
                require(std::holds_alternative<SessionMissing>(
                            unit.load_current({"I", digest(255)}, {})),
                        "Off-key corrupt tail never converts honest absence");
                s.commit();
                return 1;
            });
            // Result/statement bounds prove no load-all or per-row loop here. Engine
            // range/scratch still requires source/query audit; foundation explains exact index
            // shape.
        });
    }
    void pureLimitOverflowAndInputShapePrecedeExpiredTicket() {
        scenario([] {
            const SqliteWorkSession expired;
            for (int field = 0; field < 4; ++field) {
                AccountSessionStoreLimits l;
                if (field == 0)
                    l.reference_bytes = 0;
                if (field == 1)
                    l.selected_bytes = 0;
                if (field == 2)
                    l.mutation_bytes = 0;
                if (field == 3)
                    l.page_rows = 0;
                input_error([&] { SqliteAccountSessionUnit unit(expired, scope(), l); },
                            AccountSessionInputError::invalid_limits);
            }
            for (const auto huge : {std::numeric_limits<std::uint64_t>::max(),
                                    std::numeric_limits<std::uint64_t>::max() / 3 + 1}) {
                AccountSessionStoreLimits l;
                l.selected_bytes = huge;
                input_error([&] { SqliteAccountSessionUnit unit(expired, scope(), l); },
                            AccountSessionInputError::invalid_limits);
            }
            AccountSessionStoreLimits l;
            l.page_rows = std::numeric_limits<std::uint64_t>::max();
            input_error([&] { SqliteAccountSessionUnit unit(expired, scope(), l); },
                        AccountSessionInputError::invalid_limits);
            SqliteAccountSessionUnit unit(expired, scope());
            input_error([&] { (void)unit.read_account("I", std::string(4097, 'x')); },
                        AccountSessionInputError::resource_limit);
            input_error([&] { (void)unit.read_page("I", "A", std::optional<std::string>{""}, 2); },
                        AccountSessionInputError::invalid_input);
            input_error([&] { (void)unit.read_page("I", "A", std::nullopt, 0); },
                        AccountSessionInputError::invalid_input);
            input_error([&] { (void)unit.read_page("I", "A", std::nullopt, 65); },
                        AccountSessionInputError::invalid_input);
            input_error([&] { (void)unit.read_account("I", ""); },
                        AccountSessionInputError::invalid_input);
            auto wrong = account();
            wrong.current.account_ref = "B";
            input_error([&] { (void)unit.replace_account(account(), wrong); },
                        AccountSessionInputError::invalid_input);
            auto bad_record = record();
            bad_record.state = static_cast<SessionRecordState>(9);
            input_error(
                [&] {
                    (void)unit.insert_session(bad_record, SessionRetention::normal, accepted());
                },
                AccountSessionInputError::invalid_input);
            input_error(
                [&] {
                    (void)unit.insert_session(record(), static_cast<SessionRetention>(9),
                                              accepted());
                },
                AccountSessionInputError::invalid_input);
            auto bad_local = local();
            bad_local.credential.encoded_verifier = "$";
            input_error(
                [&] {
                    (void)unit.replace_local_credential(local(), bad_local, account(),
                                                        installation());
                },
                AccountSessionInputError::invalid_input);
            bad_local = local();
            bad_local.credential.revision_ref = std::string(257, 'x');
            input_error(
                [&] {
                    (void)unit.replace_local_credential(local(), bad_local, account(),
                                                        installation());
                },
                AccountSessionInputError::resource_limit);
            auto bad_authentication = accepted();
            bad_authentication.method = static_cast<AcceptedSignInMethod>(9);
            input_error(
                [&] {
                    (void)unit.insert_session(record(), SessionRetention::normal,
                                              bad_authentication);
                },
                AccountSessionInputError::invalid_input);
            auto bad_times = record();
            bad_times.expires_utc_ms = 1000;
            input_error(
                [&] { (void)unit.insert_session(bad_times, SessionRetention::normal, accepted()); },
                AccountSessionInputError::invalid_input);
            AccountSessionStoreLimits tiny;
            tiny.mutation_bytes = 2;
            SqliteAccountSessionUnit small(expired, scope(), tiny);
            input_error([&] { (void)small.insert_installation({{"oversized", ""}, ""}); },
                        AccountSessionInputError::resource_limit);
            work_error([&] { (void)unit.read_selected("I", "A", "S"); }, WorkError::stale_session);
        });
    }
    void completeMutationTupleBoundsAndPureRefusalPreserveCallerWork() {
        scenario([] {
            Database db(false);
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                AccountSessionStoreLimits l;
                l.mutation_bytes = 3;
                SqliteAccountSessionUnit exact(s, scope(), l);
                require(exact.insert_installation(installation()) == staged,
                        "Literal installation tuple B3 fits");
                l.mutation_bytes = 2;
                SqliteAccountSessionUnit short_unit(s, scope(), l);
                input_error([&] { (void)short_unit.insert_installation(installation()); },
                            AccountSessionInputError::resource_limit);
                require(
                    s.execute(
                        {"SELECT hex(session_epoch_ref) FROM main.subathon_account_installation",
                         {}}) == SqlRows{{QString("45")}},
                    "Pure refusal did not cleanup earlier caller staging");
                s.commit();
                return 1;
            });
            Database local_db;
            (void)run(local_db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                AccountSessionStoreLimits l;
                l.mutation_bytes = 207;
                SqliteAccountSessionUnit exact(s, scope(), l);
                auto replacement = local();
                replacement.credential.revision_ref = "Z";
                require(exact.replace_local_credential(local(), replacement, account(),
                                                       installation()) == staged,
                        "Full expected optional+replacement+account+install tuple B207 fits");
                l.mutation_bytes = 206;
                SqliteAccountSessionUnit short_unit(s, scope(), l);
                input_error(
                    [&] {
                        (void)short_unit.replace_local_credential(local(), replacement, account(),
                                                                  installation());
                    },
                    AccountSessionInputError::resource_limit);
                s.commit();
                return 1;
            });
        });
    }
    void foreignExpiredAndTerminalCapabilitiesKeepActualErrors() {
        scenario([] {
            Database db;
            std::unique_ptr<SqliteAccountSessionUnit> retained;
            (void)run(db.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                retained = std::make_unique<SqliteAccountSessionUnit>(s, scope());
                auto foreign = std::async(std::launch::async, [&] {
                    input_error([&] { (void)retained->read_account("I", ""); },
                                AccountSessionInputError::invalid_input);
                    work_error([&] { (void)retained->read_selected("I", "A", "S"); },
                               WorkError::wrong_thread);
                });
                foreign.get();
                complete(facts(retained->read_selected("I", "A", "S")));
                s.commit();
                return 1;
            });
            work_error([&] { (void)retained->read_selected("I", "A", "S"); },
                       WorkError::stale_session);
            const auto terminal = feature_outcome(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                s.commit();
                work_error([&] { (void)unit.read_selected("I", "A", "S"); },
                           WorkError::session_closed);
                return 1;
            });
            require(terminal.transaction == TransactionOutcome::committed &&
                        terminal.error == WorkError::session_closed && !terminal.value,
                    "Terminal misuse preserves already committed outcome plus sticky error");
        });
    }
    // AS23: public source inability retains real writer failure instead of fabricating missing.
    void sourceFailureAndExistingStickyErrorRemainHonest() {
        scenario([] {
            Database db;
            bool reached = false;
            const auto result = feature_outcome(db.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                try {
                    (void)s.execute({"SELECT value FROM main.no_such_account_session_table", {}});
                } catch (const SqliteWorkFailure &e) {
                    reached = e.error() == WorkError::sql_failure;
                }
                require(std::holds_alternative<SessionSourceUnavailable>(
                            unit.load_current({"I", digest()}, {})),
                        "Sticky SQL failure cannot yield missing or successful source");
                return 1;
            });
            require(reached && result.error == WorkError::sql_failure &&
                        result.transaction == TransactionOutcome::rolled_back && !result.value,
                    "Actual first sticky SQL failure survives source unavailable mapping");
            corrupt(db.path, {"DROP TABLE main.subathon_account_sessions"});
            const auto unavailable = feature_outcome(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                require(std::holds_alternative<SessionSourceUnavailable>(
                            unit.load_current({"I", digest()}, {})),
                        "Missing physical table is unavailable, not absent session");
                return 1;
            });
            require(unavailable.error == WorkError::sql_failure &&
                        unavailable.transaction == TransactionOutcome::rolled_back &&
                        !unavailable.value,
                    "Reached storage failure retains real rollback classification");
        });
    }
    void underlyingSqlBudgetRefusalKeepsFirstWorkError() {
        scenario([] {
            QTemporaryDir directory;
            require(directory.isValid(), "Disposable writer-budget database");
            const auto path = directory.filePath("budget.sqlite");
            const Data data;
            {
                SqliteWriter writer;
                require(writer.open(path, limits()) == StorageError::none, "Setup writer opens");
                data.schema(writer);
                (void)run(writer, [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    Cleanup cleanup{s};
                    data.populate(s);
                    s.commit();
                    return 1;
                });
            }
            auto tiny = limits();
            tiny.result_rows = 1;
            SqliteWriter writer;
            require(writer.open(path, tiny) == StorageError::none,
                    "Constrained actual writer opens");
            const auto result = feature_outcome(writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                const auto seen = unit.load_current({"I", digest()}, {});
                require(std::holds_alternative<SessionSourceResourceLimit>(seen),
                        "Actual cumulative SQL budget maps to source resource refusal");
                return 1;
            });
            require(result.error == WorkError::result_limit &&
                        result.transaction == TransactionOutcome::rolled_back && !result.value,
                    "Actual SQL result budget failure stays sticky and cannot commit");
        });
    }
    void snapshotMaximumOrdinalSucceedsOnceThenRefusesBeforeWrap() {
        scenario([] {
            Database db;
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                SqliteAccountSessionUnit unit(s, scope());
                testing::SqliteAccountSessionTestAccess::ordinal(
                    unit, std::numeric_limits<std::uint64_t>::max());
                const auto last = unit.read_selected("I", "A", "S");
                const auto wanted =
                    std::string("SAS1", 4) + std::string(32, '\0') + std::string(8, char(0xff));
                require(facts(last).snapshot_ref == wanted,
                        "Literal BE64 maximum ordinal remains representable");
                failure(unit.read_selected("I", "A", "S"),
                        AccountSessionReadFailure::resource_limit);
                require(std::holds_alternative<SessionSourceResourceLimit>(
                            unit.load_current({"I", digest()}, {})),
                        "Exhausted ordinal never wraps or supplies a repeated successful snapshot");
                s.commit();
                return 1;
            });
        });
    }
    void binaryCompositeKeysNeverUseTextOrPrefixEquality() {
        scenario([] {
            Database db(false);
            (void)run(db.writer, [](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                const auto i = std::string("I\0\xff", 3), a = std::string("A\0\x80", 3),
                           key = std::string("S\0\xfe", 3);
                const auto e = std::string("E\0", 2), f = std::string("F\xff", 2),
                           r = std::string("R\0", 2);
                (void)s.execute(
                    {"INSERT INTO main.subathon_account_installation VALUES(1,?,?,X'4a')",
                     {blob(i), blob(e)}});
                (void)s.execute({"INSERT INTO main.subathon_accounts VALUES(?,?,1,?,X'4b')",
                                 {blob(i), blob(a), blob(f)}});
                (void)s.execute(
                    {"INSERT INTO main.subathon_local_credentials VALUES(?,?,X'4c',1,?)",
                     {blob(i), blob(a), blob(h0())}});
                (void)s.execute({"INSERT INTO main.subathon_account_sessions "
                                 "VALUES(?,?,?,?,?,?,1,zeroblob(32),0,0,1000,1000,86401000)",
                                 {blob(i), blob(a), blob(key), blob(e), blob(f), blob(r)}});
                SqliteAccountSessionUnit unit(s, scope());
                const auto selected = unit.read_selected(i, a, key);
                SessionRecordFacts expected{
                    i, a, key, e, f, r, digest(), SessionRecordState::active, 1000, 1000, 86401000};
                same_record(facts(selected).session, expected);
                require(facts(selected).account.installation_ref == i &&
                            facts(selected).account.account_ref == a &&
                            facts(selected).account.session_epoch_ref == f &&
                            facts(selected).installation.installation_ref == i &&
                            facts(selected).installation.session_epoch_ref == e,
                        "Opaque linked keys and epochs survive exactly");
                const auto credential = unit.read_local_credential(i, a);
                require(facts(credential).local.installation_ref == i &&
                            facts(credential).local.credential.account_ref == a,
                        "Password current read retains exact BLOB account key");
                failure(unit.read_selected(i, a, "S"), AccountSessionReadFailure::missing);
                require(
                    std::holds_alternative<SessionMissing>(unit.load_current({"I", digest()}, {})),
                    "No truncated installation prefix lookup");
                require(std::holds_alternative<CompleteSessionFacts>(
                            unit.load_current({i, digest()}, {})),
                        "Exact binary installation digest lookup");
                s.commit();
                return 1;
            });
        });
    }
    void returnedValuesOwnBinaryReferencesAfterFacadeAndWork() {
        scenario([] {
            Database db;
            std::optional<CompleteSessionFacts> retained;
            (void)run(db.writer, [&](auto context) -> QVariant {
                const auto s = context.begin();
                Cleanup cleanup{s};
                (void)s.execute(
                    {"UPDATE main.subathon_account_sessions SET record_revision_ref=X'5100ff'",
                     {}});
                {
                    SqliteAccountSessionUnit unit(s, scope());
                    retained = facts(unit.read_selected("I", "A", "S"));
                }
                s.commit();
                return 1;
            });
            require(
                retained && retained->session.record_revision_ref == std::string("Q\0\xff", 3) &&
                    retained->snapshot_ref == snapshot() && retained->session.digest == digest(),
                "Owned exact opaque bytes outlive facade and actual work");
        });
    }
};
QTEST_GUILESS_MAIN(SqliteAccountSessionsTests)
#include "sqlite_account_sessions_tests.moc"
