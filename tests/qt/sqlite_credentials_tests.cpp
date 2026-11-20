// SPDX-License-Identifier: GPL-3.0-or-later
#include "../support/credential_native_controls.hpp"
#include "support/credential_key_fixture.hpp"

#include <QFile>
#include <QTemporaryDir>
#include <algorithm>
#include <cerrno>
#include <exception>
#include <memory>
#include <subathon/server/sqlite_migrations.hpp>
#include <subathon/server/sqlite_modifier_activations.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
#include <sys/stat.h>
#include <tuple>
#include <unistd.h>
#include <vector>

using namespace subathon::server;
using qt_test_support::fail_on_sql_warnings;
using qt_test_support::run_work;
namespace native = credential_native;
namespace v = credential_vectors;

namespace {
using credential_test_support::blob;
using credential_test_support::checked;
using credential_test_support::expect_error;
using credential_test_support::KeyFixture;
using credential_test_support::provision_path;
using credential_test_support::require;
constexpr int timeout = 5000;
constexpr auto query_row = "SELECT record FROM main.subathon_provider_credentials WHERE scope=?";
constexpr auto upsert_row = "INSERT INTO main.subathon_provider_credentials(scope, record) "
                            "VALUES (?,?) ON CONFLICT(scope) DO UPDATE SET record=excluded.record";

CredentialKey key_from(const QByteArray &path) {
    auto result = provision_path(path);
    if (const auto *error = std::get_if<CredentialError>(&result))
        require(false, QString("Provision expected a move-only key, got category %1")
                           .arg(static_cast<int>(*error)));
    return std::get<CredentialKey>(std::move(result));
}

void expect_value(const CredentialLoadResult &result,
                  std::span<const unsigned char> expected = v::plaintext) {
    const auto *value = std::get_if<CredentialValue>(&result);
    require(value && std::ranges::equal(value->bytes(), expected),
            "Expected exact owned plaintext");
}

void expect_staged(const CredentialPutResult &result) {
    require(std::holds_alternative<CredentialStaged>(result), "Expected exactly one staged put");
}

void expect_work(const SqliteWorkResult &result,
                 TransactionOutcome transaction = TransactionOutcome::committed,
                 WorkError error = WorkError::none) {
    require(result.transaction == transaction && result.error == error,
            QString("Writer transaction %1 and error %2")
                .arg(static_cast<int>(result.transaction))
                .arg(static_cast<int>(result.error)));
}

class Database final {
  public:
    Database() : path(dir.filePath("credentials.sqlite")) {
        require(dir.isValid(), "Disposable SQLite directory exists");
        // The full v14 migration projects every prefix in one transaction.
        StorageLimits migration_limits;
        migration_limits.statements_per_batch = 16384;
        migration_limits.statement_utf8_bytes = 4194304;
        migration_limits.bound_value_bytes = 33554432;
        migration_limits.result_rows = 65536;
        migration_limits.result_value_bytes = 33554432;
        open(migration_limits);
        auto done = std::make_shared<std::optional<MigrationResult>>();
        const auto admission = SqliteMigrationRunner::enqueue(
            *writer, {MigrationMode::advance, modifier_activations_repository_manifest(), {}, {}},
            [done](auto result) { done->emplace(std::move(result)); });
        require(admission.error == StorageError::none && admission.reason == MigrationReason::none,
                "Selected v14 migrator accepts test setup");
        require(QTest::qWaitFor([done] { return done->has_value(); }, timeout),
                "Migration finishes");
        const auto &result = done->value();
        expect_work(result.work);
        require(result.reason == MigrationReason::none && result.report &&
                    result.report->version == 14,
                "Selected v14 exists");
        open();

        // Fixture-only DDL. No production catalog reservation. Deliberately
        // allow TEXT and NULL to reach selected-row corruption checks.
        commit([](const auto &s) {
            (void)s.execute({"CREATE TABLE main.subathon_provider_credentials "
                             "(scope BLOB PRIMARY KEY NOT NULL, record BLOB)",
                             {}});
        });
    }

    void open(StorageLimits limits = {}) {
        if (writer) {
            const auto done = std::make_shared<bool>(false);
            require(writer->shutdown([done] { *done = true; }) == StorageError::none,
                    "Writer accepts close");
            require(QTest::qWaitFor([done] { return *done; }, timeout),
                    "Writer drained and closed");
            writer.reset();
        }
        writer = std::make_unique<SqliteWriter>();
        require(writer->open(path, limits) == StorageError::none, "Writer opens disposable file");
    }

    template <class Body> void commit(Body body) {
        auto cause = std::make_shared<std::exception_ptr>();
        const auto work = run_work(
            *writer,
            [body = std::move(body), cause](const auto &context) -> QVariant {
                try {
                    const auto s = context.begin();
                    body(s);
                    s.commit();
                    return 1;
                } catch (...) {
                    *cause = std::current_exception();
                    throw;
                }
            },
            timeout);
        if (*cause)
            std::rethrow_exception(*cause);
        expect_work(work);
    }

    void seed(QVariant record = blob(v::record), QByteArray scope = blob(v::scope)) {
        commit([&](const auto &s) { (void)s.execute({upsert_row, {scope, record}}); });
    }

    SqlRows read(const SqlStatement &query) const {
        SqliteReader observer;
        require(observer.open(path) == StorageError::none, "Independent reader opens database");
        const auto result = observer.read(query);
        require(result.error == StorageError::none, "Independent reader completes");
        return result.rows;
    }

    QVariant record(const QByteArray &scope = blob(v::scope)) const {
        const auto rows = read({query_row, {scope}});
        require(rows.size() == 1 && rows.front().size() == 1, "Exact raw row exists");
        return rows.front().front();
    }

    QTemporaryDir dir;
    QString path;
    std::unique_ptr<SqliteWriter> writer;
};

struct ScopeChange {
    CredentialScope scope;
    QByteArray encoded;
};

auto alternate_scopes() {
    auto installation = v::exact;
    installation.installation_ref = "I";
    auto provider = v::exact;
    provider.provider = ConnectorProvider::patreon;
    auto connector = v::exact;
    connector.connector_ref = std::string_view("\xff"
                                               "D",
                                               2);
    auto account = v::exact;
    account.subject = AccountSubject{"B"};
    auto subject = v::exact;
    subject.subject = InstallationSubject{};
    auto slot = v::exact;
    slot.slot_ref = "t";
    return std::array{
        ScopeChange{installation, QByteArray::fromHex("000149010002ff430200014100027400")},
        ScopeChange{provider, QByteArray::fromHex("00024900030002ff430200014100027400")},
        ScopeChange{connector, QByteArray::fromHex("00024900010002ff440200014100027400")},
        ScopeChange{account, QByteArray::fromHex("00024900010002ff430200014200027400")},
        ScopeChange{subject, QByteArray::fromHex("00024900010002ff430100027400")},
        ScopeChange{slot, QByteArray::fromHex("00024900010002ff4302000141000174")}};
}

void expect_nonce(const QVariant &stored, const native::Nonce &nonce) {
    require(stored.metaType() == QMetaType::fromType<QByteArray>() &&
                stored.toByteArray().mid(20, 24) == blob(nonce),
            "Public record header reflects exact application nonce");
}

class MessageCapture final {
  public:
    explicit MessageCapture(const QByteArray &scope)
        : scope_hex(QString::fromLatin1(scope.toHex())) {
        active = this;
        previous = qInstallMessageHandler(capture);
    }
    ~MessageCapture() {
        qInstallMessageHandler(previous);
        active = nullptr;
    }
    bool contains_secret() const { return leaked; }

  private:
    static void capture(QtMsgType, const QMessageLogContext &, const QString &message) {
        // Report only a boolean; never print captured material in an assertion.
        static const std::array needles{QStringLiteral("synthetic-key-path-marker"),
                                        QStringLiteral("scope-disclosure-marker"),
                                        QStringLiteral("00ff4100800a7f"),
                                        QString::fromLatin1(blob(v::key).toHex()),
                                        QString::fromLatin1(blob(v::id).toHex()),
                                        QString::fromLatin1(blob(v::nonce).toHex()),
                                        QString::fromLatin1(blob(v::record).toHex()),
                                        QString::fromLatin1(blob(v::plaintext)),
                                        QString::fromLatin1(blob(v::key)),
                                        QString::fromLatin1(blob(v::record))};
        if (active) {
            active->leaked |= message.contains(active->scope_hex, Qt::CaseInsensitive);
            active->leaked |= std::ranges::any_of(needles, [&](const auto &needle) {
                return message.contains(needle, Qt::CaseInsensitive);
            });
        }
    }

    static inline thread_local MessageCapture *active{};
    QtMessageHandler previous{};
    QString scope_hex;
    bool leaked{};
};

enum class Finish { committed, rolled_back, unknown, post_commit_failure, missing };
} // namespace

class SqliteCredentialsTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { fail_on_sql_warnings(); }

    void exactRecordAndRestart_SC01_SC04() {
        checked([&] {
            KeyFixture keys;
            const auto path = keys.file();
            KeyFixture::write(path, blob(v::key_file), 0400);
            Database db;
            db.seed();
            {
                const auto key = key_from(path);
                db.commit([&](const auto &s) {
                    expect_value(CredentialRepository(s, key).load(v::exact));
                });
                db.commit([](const auto &s) {
                    (void)s.execute({"DELETE FROM main.subathon_provider_credentials WHERE scope=?",
                                     {blob(v::scope)}});
                });
                QVERIFY(db.read({query_row, {blob(v::scope)}}).isEmpty());
                QVERIFY(native::prepare_sdk());
                const std::array<native::Nonce, 1> nonces{v::nonce};
                const native::Controls controls(native::Fault::none, 0, nonces);
                db.commit([&](const auto &s) {
                    expect_staged(CredentialRepository(s, key).stage_put(v::exact, v::plaintext));
                });
                QCOMPARE(controls.observation().nonce_draws, 1U);
            }
            QVERIFY(db.record().metaType() == QMetaType::fromType<QByteArray>());
            QVERIFY(db.record().toByteArray() == blob(v::record));
            QVERIFY(db.read({"SELECT typeof(scope),typeof(record),length(record) "
                             "FROM main.subathon_provider_credentials",
                             {}}) ==
                    (SqlRows{{QStringLiteral("blob"), QStringLiteral("blob"), 71}}));
            db.open();
            const auto fresh_key = key_from(path);
            db.commit([&](const auto &s) {
                expect_value(CredentialRepository(s, fresh_key).load(v::exact));
            });
            QVERIFY(db.record().toByteArray() == blob(v::record));
        });
    }

    void exactSelectionAndPlacementAuthentication_SC02_SC03() {
        checked([&] {
            KeyFixture keys;
            const auto path = keys.file();
            KeyFixture::write(path);
            const auto key = key_from(path);
            Database db;
            db.seed();
            const auto alternatives = alternate_scopes();
            db.commit([&](const auto &s) {
                const CredentialRepository repo(s, key);
                expect_value(repo.load(v::exact));
                for (const auto &alternate : alternatives)
                    require(std::holds_alternative<CredentialMissing>(repo.load(alternate.scope)),
                            "Adjacent scope remains absent while exact scope is present");
            });
            for (const auto &alternate : alternatives)
                db.seed(blob(v::record), alternate.encoded);
            db.commit([&](const auto &s) {
                const CredentialRepository repo(s, key);
                for (const auto &alternate : alternatives)
                    expect_error(repo.load(alternate.scope),
                                 CredentialError::authentication_failed);
            });
            for (const auto &alternate : alternatives)
                QVERIFY(db.record(alternate.encoded).toByteArray() == blob(v::record));

            // Reset fixture-only corrupt row before using the same alternate slot as
            // a positive independent record. Other transplanted rows stay inert.
            const auto &slot = alternatives.back();
            db.commit([&](const auto &s) {
                (void)s.execute({"DELETE FROM main.subathon_provider_credentials WHERE scope=?",
                                 {slot.encoded}});
            });
            constexpr std::array<unsigned char, 1> other{0x80};
            constexpr std::array<unsigned char, 1> replacement{0x81};
            db.commit([&](const auto &s) {
                expect_staged(CredentialRepository(s, key).stage_put(slot.scope, other));
            });
            db.commit([&](const auto &s) {
                const CredentialRepository repo(s, key);
                expect_value(repo.load(slot.scope), other);
                expect_staged(repo.stage_put(slot.scope, replacement));
            });
            db.commit([&](const auto &s) {
                const CredentialRepository repo(s, key);
                expect_value(repo.load(slot.scope), replacement);
                expect_value(repo.load(v::exact));
            });
            QVERIFY(db.record().toByteArray() == blob(v::record));
        });
    }

    void rejectsMismatchedKeyAndAuthenticatedIdEdit_SC05_data() {
        QTest::addColumn<int>("change");
        QTest::newRow("same-id-wrong-key") << 0;
        QTest::newRow("changed-file-id") << 1;
        QTest::newRow("file-and-record-id-changed") << 2;
    }

    void rejectsMismatchedKeyAndAuthenticatedIdEdit_SC05() {
        checked([&] {
            QFETCH(int, change);
            KeyFixture keys;
            const auto path = keys.file();
            auto key_file = blob(v::key_file);
            auto record = blob(v::record);
            if (change == 0)
                key_file[20] = static_cast<char>(0x81);
            else
                key_file[4] = 0x20;
            if (change == 2)
                record[4] = 0x20;
            KeyFixture::write(path, key_file);
            const auto key = key_from(path);
            Database db;
            db.seed(record);
            db.commit([&](const auto &s) {
                const CredentialRepository repo(s, key);
                const auto expected = change == 1 ? CredentialError::key_unavailable
                                                  : CredentialError::authentication_failed;
                expect_error(repo.load(v::exact), expected);
                expect_error(repo.stage_put(v::exact, v::plaintext), expected);
            });
            QVERIFY(db.record().toByteArray() == record);
        });
    }

    void selectedSqlPreflight_SC07_data() {
        QTest::addColumn<QVariant>("record");
        QTest::addColumn<int>("expected");
        QTest::newRow("text-before-length")
            << QVariant(QString(65601, 'x')) << static_cast<int>(CredentialError::invalid_record);
        QTest::newRow("null-before-length")
            << QVariant() << static_cast<int>(CredentialError::invalid_record);
        QTest::newRow("blob-over-ceiling") << QVariant(QByteArray(65601, 'x'))
                                           << static_cast<int>(CredentialError::resource_limit);
    }

    void selectedSqlPreflight_SC07() {
        checked([&] {
            QFETCH(QVariant, record);
            QFETCH(int, expected);
            KeyFixture keys;
            const auto path = keys.file();
            KeyFixture::write(path);
            const auto key = key_from(path);
            Database db;
            db.seed(record);
            StorageLimits preflight;
            preflight.statements_per_batch = 1;
            preflight.result_value_bytes = 128;
            db.open(preflight);
            db.commit([&](const auto &s) {
                const CredentialRepository repo(s, key);
                expect_error(repo.load(v::exact), static_cast<CredentialError>(expected));
            });
            QVERIFY(db.record() == record || (!record.isValid() && db.record().isNull()));

            // Exhaust the budget independently of the repository query shape.
            // The writer error must reach completion unchanged.
            db.open();
            db.seed();
            db.open(preflight);
            const auto exhausted = run_work(
                *db.writer,
                [&](const auto &context) {
                    const auto s = context.begin();
                    (void)s.execute({"SELECT 1", {}});
                    (void)CredentialRepository(s, key).load(v::exact);
                    s.commit();
                    return 1;
                },
                timeout);
            expect_work(exhausted, TransactionOutcome::rolled_back, WorkError::resource_limit);
            QVERIFY(!exhausted.value.has_value());
            QVERIFY(db.record().toByteArray() == blob(v::record));
        });
    }

    void inputPreflight_SC08_data() {
        QTest::addColumn<int>("input");
        QTest::newRow("empty-ref-oversized-value") << 0;
        QTest::newRow("oversized-ref-unknown-provider") << 1;
        QTest::newRow("empty-value") << 2;
        QTest::newRow("empty-ref") << 3;
        QTest::newRow("unknown-provider") << 4;
    }

    void inputPreflight_SC08() {
        checked([&] {
            QFETCH(int, input);
            KeyFixture keys;
            const auto path = keys.file();
            KeyFixture::write(path);
            const auto key = key_from(path);
            Database db;
            StorageLimits limits;
            limits.statements_per_batch = 1;
            db.open(limits);
            const std::string huge_ref(4097, 'c');
            const std::vector<unsigned char> huge_value(65537, 0x81);
            db.commit([&](const auto &s) {
                auto changed = v::exact;
                const CredentialRepository repo(s, key);
                switch (input) {
                case 0:
                    changed.slot_ref = "";
                    expect_error(repo.stage_put(changed, huge_value),
                                 CredentialError::resource_limit);
                    break;
                case 1:
                    changed.connector_ref = huge_ref;
                    changed.provider = static_cast<ConnectorProvider>(99);
                    expect_error(repo.load(changed), CredentialError::resource_limit);
                    break;
                case 2:
                    expect_error(repo.stage_put(changed, {}), CredentialError::invalid_input);
                    break;
                case 3:
                    changed.subject = AccountSubject{""};
                    expect_error(repo.load(changed), CredentialError::invalid_input);
                    break;
                case 4:
                    changed.provider = static_cast<ConnectorProvider>(99);
                    expect_error(repo.load(changed), CredentialError::invalid_input);
                    break;
                }
                require(s.execute({"SELECT 1", {}}).size() == 1,
                        "Input preflight leaves the sole available SQL statement unused");
            });
            QVERIFY(
                db.read({"SELECT scope FROM main.subathon_provider_credentials", {}}).isEmpty());
        });
    }

    void valueBounds_SC08_data() {
        QTest::addColumn<int>("size");
        QTest::newRow("minimum") << 1;
        QTest::newRow("maximum") << 65536;
    }

    void valueBounds_SC08() {
        checked([&] {
            QFETCH(int, size);
            KeyFixture keys;
            const auto path = keys.file();
            KeyFixture::write(path);
            const auto key = key_from(path);
            Database db;
            const std::vector<unsigned char> value(static_cast<std::size_t>(size), 0x81);
            db.commit([&](const auto &s) {
                expect_staged(CredentialRepository(s, key).stage_put(v::exact, value));
            });
            const auto envelope_size = db.record().toByteArray().size();
            QCOMPARE(envelope_size, size == 1 ? 65 : 65600);
            db.commit([&](const auto &s) {
                expect_value(CredentialRepository(s, key).load(v::exact), value);
            });
        });
    }

    void rollbackReopenAndFreshNoncePerAttempt_SC04_SC09() {
        checked([&] {
            KeyFixture keys;
            const auto path = keys.file();
            KeyFixture::write(path);
            Database db;
            QVERIFY(native::prepare_sdk());
            constexpr auto fourth_nonce =
                v::hex("c0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7");
            const std::array nonces{v::nonce, v::second_nonce, v::third_nonce, fourth_nonce};

            const auto stage = [&](const auto &key, bool commit, std::size_t index) {
                auto cause = std::make_shared<std::exception_ptr>();
                const native::Controls controls(native::Fault::none, 0,
                                                std::span(nonces).subspan(index, 1));
                const auto work = run_work(
                    *db.writer,
                    [&](const auto &context) {
                        try {
                            const auto s = context.begin();
                            expect_staged(
                                CredentialRepository(s, key).stage_put(v::exact, v::plaintext));
                            const auto raw = s.execute({query_row, {blob(v::scope)}});
                            require(raw.size() == 1 && raw.front().size() == 1,
                                    "One provisional row");
                            expect_nonce(raw.front().front(), nonces[index]);
                            if (commit)
                                s.commit();
                            else
                                s.rollback();
                            return 1;
                        } catch (...) {
                            *cause = std::current_exception();
                            throw;
                        }
                    },
                    timeout);
                if (*cause)
                    std::rethrow_exception(*cause);
                expect_work(
                    work, commit ? TransactionOutcome::committed : TransactionOutcome::rolled_back,
                    commit ? WorkError::none : WorkError::not_committed);
                require(controls.observation().nonce_draws == 1,
                        "One post-readiness application draw");
            };

            {
                const auto key = key_from(path);
                stage(key, false, 0);
            }
            db.open();
            QVariant original_record;
            {
                const auto key = key_from(path);
                db.commit([&](const auto &s) {
                    require(std::holds_alternative<CredentialMissing>(
                                CredentialRepository(s, key).load(v::exact)),
                            "Known rolled back insert stays absent after reopen");
                });
                stage(key, true, 1);
                original_record = db.record();
                expect_nonce(original_record, nonces[1]);
                stage(key, false, 2);
            }
            db.open();
            const auto key = key_from(path);
            QVERIFY(db.record() == original_record);
            db.commit(
                [&](const auto &s) { expect_value(CredentialRepository(s, key).load(v::exact)); });
            stage(key, true, 3);
            expect_nonce(db.record(), nonces[3]);
            db.commit(
                [&](const auto &s) { expect_value(CredentialRepository(s, key).load(v::exact)); });
        });
    }

    void guardedMemoryFailures_SC09_data() {
        QTest::addColumn<int>("allocation_size");
        QTest::addColumn<bool>("lock");
        for (const auto &[size, failure, lock_failure] :
             {std::tuple{53, "provision-allocation", "provision-lock"},
              std::tuple{65536, "put-allocation", "put-lock"}}) {
            QTest::newRow(failure) << size << false;
            QTest::newRow(lock_failure) << size << true;
        }
    }

    void guardedMemoryFailures_SC09() {
        checked([&] {
            QFETCH(int, allocation_size);
            QFETCH(bool, lock);
            KeyFixture keys;
            const auto path = keys.file();
            KeyFixture::write(path);
            Database db;
            db.seed();
            const auto fault = lock ? native::Fault::lock : native::Fault::allocation;
            const bool key_file = allocation_size == 53;
            const auto check = [&](const auto &controls) {
                const auto o = controls.observation();
                require(o.allocations == 1 && o.locks == (lock ? 1U : 0U) &&
                            o.releases == (lock ? 1U : 0U),
                        "Guarded allocation/lock refusal releases any allocation");
            };
            if (key_file) {
                const native::Controls controls(fault, 53);
                expect_error(provision_path(path), CredentialError::crypto_unavailable);
                check(controls);
            } else {
                const auto key = key_from(path);
                const native::Controls controls(fault, static_cast<std::size_t>(allocation_size));
                const std::vector<unsigned char> bytes(65536, 0x81);
                auto scope = v::exact;
                scope.slot_ref = "s";
                db.commit([&](const auto &s) {
                    expect_error(CredentialRepository(s, key).stage_put(scope, bytes),
                                 CredentialError::crypto_unavailable);
                });
                check(controls);
                QVERIFY(
                    db.read({query_row, {QByteArray::fromHex("00024900010002ff4302000141000173")}})
                        .isEmpty());
            }
            QVERIFY(db.record().toByteArray() == blob(v::record));
            const auto key = key_from(path);
            db.commit(
                [&](const auto &s) { expect_value(CredentialRepository(s, key).load(v::exact)); });
        });
    }

    void lostFileHasNoFallbackAndRestores_SC10() {
        checked([&] {
            KeyFixture keys;
            const auto path = keys.file();
            KeyFixture::write(path);
            Database db;
            db.seed();
            {
                (void)key_from(path);
            }
            QVERIFY(::unlink(path.constData()) == 0);
            expect_error(provision_path(path), CredentialError::key_unavailable);
            struct stat info{};
            QVERIFY(::lstat(path.constData(), &info) == -1 && errno == ENOENT);
            QVERIFY(db.record().toByteArray() == blob(v::record));
            KeyFixture::write(path, blob(v::key_file), 0400);
            const auto restored = key_from(path);
            db.commit([&](const auto &s) {
                expect_value(CredentialRepository(s, restored).load(v::exact));
            });
        });
    }

    void pathCeilingPrecedesShape_SC11() {
        checked([&] {
            KeyFixture keys;
            std::string path(4096, 'x');
            path[0] = '/';
            path[1] = '\0';
            expect_error(provision({path}), CredentialError::resource_limit);
            path.pop_back();
            expect_error(provision({path}), CredentialError::key_file_rejected);
            for (const auto malformed :
                 {"relative", "/double//slash", "/dot/./key", "/dotdot/../key", "/trailing/"})
                expect_error(provision({malformed}), CredentialError::key_file_rejected);
        });
    }

    void nativeProtectionAndFileFraming_SC11_SC12_data() {
        QTest::addColumn<QString>("scenario");
        for (const auto *name : {"read-only",
                                 "read-write",
                                 "group-read",
                                 "other-read",
                                 "write-only",
                                 "set-id",
                                 "group-write-parent",
                                 "other-write-parent",
                                 "group-write-grandparent",
                                 "inaccessible-parent",
                                 "final-link",
                                 "parent-link",
                                 "fifo",
                                 "directory",
                                 "non-directory-parent",
                                 "protection-before-size-and-magic",
                                 "short",
                                 "long",
                                 "bad-magic",
                                 "newline"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }

    void nativeProtectionAndFileFraming_SC11_SC12() {
        checked([&] {
            QFETCH(QString, scenario);
            KeyFixture keys;
            auto path = keys.file();
            const auto parent = keys.file("parent");
            const auto real = keys.file("real");
            auto contents = blob(v::key_file);
            mode_t mode = 0600;
            auto expected = CredentialError::key_file_rejected;
            bool accepted = false;

            if (scenario == "read-only" || scenario == "read-write") {
                mode = scenario == "read-only" ? 0400 : 0600;
                accepted = true;
            } else if (scenario == "group-read") {
                mode = 0640;
            } else if (scenario == "other-read") {
                mode = 0644;
            } else if (scenario == "write-only") {
                mode = 0200;
            } else if (scenario == "set-id") {
                mode = 04600;
            } else if (scenario == "protection-before-size-and-magic") {
                mode = 0644;
                contents = "bad";
            } else if (scenario == "short" || scenario == "long" || scenario == "bad-magic" ||
                       scenario == "newline") {
                expected = CredentialError::invalid_key_file;
                if (scenario == "short")
                    contents.chop(1);
                if (scenario == "long")
                    contents.append('X');
                if (scenario == "bad-magic")
                    contents[3] = '2';
                if (scenario == "newline")
                    contents.append('\n');
            } else if (scenario == "group-write-grandparent") {
                QVERIFY(::mkdir(parent.constData(), 0700) == 0);
                const auto child = parent + "/child";
                QVERIFY(::mkdir(child.constData(), 0700) == 0);
                path = child + "/key";
                KeyFixture::write(path);
                QVERIFY(::chmod(parent.constData(), 0720) == 0);
                expect_error(provision_path(path), expected);
                return;
            } else if (scenario == "group-write-parent" || scenario == "other-write-parent" ||
                       scenario == "inaccessible-parent") {
                QVERIFY(::mkdir(parent.constData(), 0700) == 0);
                path = parent + "/key";
                KeyFixture::write(path);
                const mode_t parent_mode = scenario == "group-write-parent"   ? 0720
                                           : scenario == "other-write-parent" ? 0702
                                                                              : 0000;
                QVERIFY(::chmod(parent.constData(), parent_mode) == 0);
                const auto result = provision_path(path);
                QVERIFY(::chmod(parent.constData(), 0700) == 0);
                expect_error(result, scenario == "inaccessible-parent"
                                         ? CredentialError::key_unavailable
                                         : CredentialError::key_file_rejected);
                return;
            } else if (scenario == "final-link") {
                KeyFixture::write(real);
                QVERIFY(::symlink(real.constData(), path.constData()) == 0);
            } else if (scenario == "parent-link") {
                QVERIFY(::mkdir(real.constData(), 0700) == 0);
                KeyFixture::write(real + "/key");
                QVERIFY(::symlink(real.constData(), parent.constData()) == 0);
                path = parent + "/key";
            } else if (scenario == "fifo") {
                QVERIFY(::mkfifo(path.constData(), 0600) == 0);
            } else if (scenario == "directory") {
                QVERIFY(::mkdir(path.constData(), 0700) == 0);
            } else if (scenario == "non-directory-parent") {
                KeyFixture::write(parent);
                path = parent + "/key";
            }

            if (scenario == "read-only" || scenario == "read-write" || scenario == "group-read" ||
                scenario == "other-read" || scenario == "write-only" || scenario == "set-id" ||
                scenario == "protection-before-size-and-magic" ||
                expected == CredentialError::invalid_key_file) {
                KeyFixture::write(path, contents, mode);
            }

            if (accepted) {
                const native::Controls controls(native::Fault::none, 53);
                (void)key_from(path);
                const auto observed = controls.observation();
                QVERIFY(observed.allocations == 1 && observed.locks == 1 && observed.releases == 1);
            } else {
                expect_error(provision_path(path), expected);
            }
        });
    }

    void typedDelivery_SC13_data() {
        QTest::addColumn<int>("finish");
        QTest::newRow("normal-commit") << static_cast<int>(Finish::committed);
        QTest::newRow("known-rollback") << static_cast<int>(Finish::rolled_back);
        QTest::newRow("unknown-commit") << static_cast<int>(Finish::unknown);
        QTest::newRow("post-commit-failure") << static_cast<int>(Finish::post_commit_failure);
        QTest::newRow("committed-missing") << static_cast<int>(Finish::missing);
    }

    void typedDelivery_SC13() {
        checked([&] {
            QFETCH(int, finish);
            const auto outcome = static_cast<Finish>(finish);
            KeyFixture keys;
            const auto path = keys.file();
            KeyFixture::write(path);
            const auto key = key_from(path);
            Database db;
            db.seed();
            if (outcome == Finish::unknown) {
                using namespace subathon::server::testing;
                QVERIFY(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                            *db.writer, WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none);
            }

            std::optional<CredentialLoadResult> slot;
            std::optional<CredentialValue> delivered;
            std::exception_ptr loading_failure;
            std::optional<SqliteWorkResult> done;
            QVERIFY(db.writer->enqueue_work(
                        [&](const auto &context) -> QVariant {
                            const auto s = context.begin();
                            try {
                                auto scope = v::exact;
                                if (outcome == Finish::missing)
                                    scope.slot_ref = "unoccupied";
                                slot.emplace(CredentialRepository(s, key).load(scope));
                                if (outcome == Finish::missing)
                                    require(std::holds_alternative<CredentialMissing>(*slot),
                                            "Typed absence");
                                else
                                    expect_value(*slot);
                                // Use a public side effect to distinguish rollback
                                // from committed storage independently of delivery.
                                (void)s.execute(
                                    {upsert_row,
                                     {QByteArray("completion-marker"), QByteArray("public")}});
                            } catch (...) {
                                loading_failure = std::current_exception();
                                throw;
                            }
                            if (outcome == Finish::rolled_back)
                                s.rollback();
                            else
                                s.commit();
                            if (outcome == Finish::post_commit_failure)
                                throw std::runtime_error("synthetic post-commit exception");
                            return 19;
                        },
                        [&](auto completion) {
                            delivered = finish_credential_load(std::exchange(slot, std::nullopt),
                                                               completion);
                            done.emplace(std::move(completion));
                        }) == StorageError::none);
            QVERIFY(QTest::qWaitFor([&] { return done.has_value(); }, timeout));
            if (loading_failure)
                std::rethrow_exception(loading_failure);
            const bool normal = outcome == Finish::committed || outcome == Finish::missing;
            if (outcome == Finish::rolled_back)
                expect_work(*done, TransactionOutcome::rolled_back, WorkError::not_committed);
            else if (outcome == Finish::unknown)
                expect_work(*done, TransactionOutcome::outcome_unknown,
                            WorkError::uncertain_outcome);
            else if (outcome == Finish::post_commit_failure)
                expect_work(*done, TransactionOutcome::committed, WorkError::work_exception);
            else
                expect_work(*done);
            QVERIFY(done->value.has_value() == normal);
            if (normal)
                QVERIFY(*done->value == QVariant(19));
            QVERIFY(delivered.has_value() == (outcome == Finish::committed));
            if (delivered)
                QVERIFY(std::ranges::equal(delivered->bytes(), v::plaintext));
            QVERIFY(db.read({query_row, {QByteArray("completion-marker")}}).isEmpty() ==
                    (outcome == Finish::rolled_back));
            QVERIFY(db.record().toByteArray() == blob(v::record));
        });
    }

    void selectedRefusalKeepsDiagnosticsSecretFree_SC13() {
        checked([&] {
            KeyFixture keys;
            const auto path = keys.file();
            KeyFixture::write(path);
            const auto key = key_from(path);
            Database db;
            auto changed = v::exact;
            changed.slot_ref = "scope-disclosure-marker";
            const auto scope = QByteArray::fromHex(
                "00024900010002ff4302000141001773636f70652d646973636c6f737572652d6d61726b6572");
            db.seed(blob(v::record), scope);
            bool leaked = false;
            {
                MessageCapture capture(scope);
                // Prove the sink detects forbidden sentinels without emitting them.
                qWarning("scope-disclosure-marker");
                QVERIFY(capture.contains_secret());
            }
            {
                // The private sink replaces QtTest's warning handler in this window.
                MessageCapture capture(scope);
                expect_error(provision_path(keys.file("missing-synthetic-key-path-marker")),
                             CredentialError::key_unavailable);
                db.commit([&](const auto &s) {
                    expect_error(CredentialRepository(s, key).load(changed),
                                 CredentialError::authentication_failed);
                });
                leaked = capture.contains_secret();
            }
            QVERIFY(!leaked);
            QVERIFY(db.record(scope).toByteArray() == blob(v::record));
        });
    }
};

QTEST_GUILESS_MAIN(SqliteCredentialsTests)
#include "sqlite_credentials_tests.moc"
