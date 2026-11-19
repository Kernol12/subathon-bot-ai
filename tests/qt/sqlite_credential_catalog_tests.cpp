// SPDX-License-Identifier: GPL-3.0-or-later
#include "support/credential_key_fixture.hpp"
#include "support/sqlite_credential_catalog_consumers.hpp"

#include <array>

namespace {
using namespace credential_catalog_fixture;
namespace keys = credential_test_support;

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &failure) {
        QFAIL(failure.what());
    }
}

constexpr auto insert_sql =
    "INSERT INTO main.subathon_provider_credentials(scope,record) VALUES(?,?)";
constexpr auto stored_blobs = "SELECT scope,record,typeof(scope),typeof(record) "
                              "FROM main.subathon_provider_credentials ORDER BY scope";

void add_blob(const SqliteWorkSession &session, const QByteArray &scope, const QByteArray &record) {
    (void)session.execute({insert_sql, {scope, record}});
}

void expect_prefix(const MigrationManifest &candidate, const MigrationManifest &literal) {
    require(candidate.catalog_format == literal.catalog_format &&
                candidate.store_identity == literal.store_identity &&
                candidate.supported_starting_versions == literal.supported_starting_versions &&
                candidate.steps.size() == literal.steps.size(),
            "Producer preserves protected catalog metadata and adds exactly the v16 start");
    for (qsizetype i = 0; i < literal.steps.size(); ++i) {
        const auto &actual = candidate.steps[i];
        const auto &expected = literal.steps[i];
        require(actual.version == expected.version && actual.id == expected.id &&
                    actual.sql == expected.sql &&
                    actual.expected_schema == expected.expected_schema,
                "Producer has the exact ordered statements and projected schema through v16");
    }
}

constexpr CredentialScope exact_scope{std::string_view("I\0", 2), ConnectorProvider::twitch,
                                      std::string_view("\xff", 1), InstallationSubject{}, "S"};
constexpr CredentialScope other_scope{exact_scope.installation_ref, exact_scope.provider,
                                      exact_scope.connector_ref, InstallationSubject{}, "T"};
constexpr std::array<unsigned char, 3> initial{0x00, 0xff, 0x41};
constexpr std::array<unsigned char, 2> replacement{0x80, 0x00};

std::shared_ptr<CredentialKey> provision_key(const QByteArray &path) {
    auto result = keys::provision_path(path);
    require(std::holds_alternative<CredentialKey>(result), "Protected synthetic key provisions");
    return std::make_shared<CredentialKey>(std::get<CredentialKey>(std::move(result)));
}

void expect_credential(CatalogFile &file, std::shared_ptr<CredentialKey> key,
                       std::span<const unsigned char> expected) {
    struct Load {
        std::optional<CredentialLoadResult> pending;
        std::optional<CredentialValue> delivered;
        std::optional<SqliteWorkResult> completion;
    };
    const auto load = std::make_shared<Load>();
    require(file.writer->enqueue_work(
                [load, key = std::move(key)](auto context) {
                    const auto session = context.begin();
                    const CredentialRepository repository(session, *key);
                    require(std::holds_alternative<CredentialMissing>(repository.load(other_scope)),
                            "Neighboring scope remains typed missing");
                    load->pending.emplace(repository.load(exact_scope));
                    require(std::holds_alternative<CredentialValue>(*load->pending),
                            "Selected scope returns a provisional authenticated value");
                    session.commit();
                    return 1;
                },
                [load](SqliteWorkResult result) {
                    load->delivered =
                        finish_credential_load(std::exchange(load->pending, std::nullopt), result);
                    load->completion.emplace(std::move(result));
                }) == StorageError::none,
            "Writer admits a real credential read");
    require(QTest::qWaitFor([load] { return load->completion.has_value(); }, 10000),
            "Caller receives the credential work completion");
    finance::normal(*load->completion);
    require(!load->pending && load->delivered &&
                std::ranges::equal(load->delivered->bytes(), expected),
            "Normal acknowledged delivery contains the expected protected bytes");
}

void expect_record(CatalogFile &file, int expected_size) {
    const auto rows = file.rows("SELECT scope,typeof(scope),typeof(record),length(record) "
                                "FROM main.subathon_provider_credentials");
    require(rows == SqlRows{{QByteArray::fromHex("00024900010001ff01000153"), QString("blob"),
                             QString("blob"), expected_size}},
            "One physical row has the canonical BLOB scope and expected BLOB record size");
}

} // namespace

class SqliteCredentialCatalogTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void upgradeAndReopen_V16_01() {
        check([] {
            CatalogFile file;
            const RetainedDocument configuration{{RetainedKind::configuration, "v16-cfg"},
                                                 QByteArray("synthetic configuration"),
                                                 {}};
            const RetainedDocument sentinel{{RetainedKind::arrival_context, "v16-sentinel"},
                                            QByteArray::fromHex("00ff7316"),
                                            ConfigurationReference{"v16-cfg"}};
            file.write([&](const auto &session) {
                SqliteRetainedContextStore retained(session);
                require(retained.retain(configuration) == RetainDisposition::inserted &&
                            retained.retain(sentinel) == RetainDisposition::inserted,
                        "Commit a known retained context and its parent");
            });
            constexpr auto sentinel_sql =
                "SELECT reference,document,configuration_ref,typeof(reference),typeof(document),"
                "typeof(configuration_ref) FROM main.subathon_retained_contexts";
            const SqlRows expected_sentinel{{QByteArray("v16-sentinel"),
                                             QByteArray::fromHex("00ff7316"), QByteArray("v16-cfg"),
                                             QString("blob"), QString("blob"), QString("blob")}};
            const auto prefix = financial_refunds_repository_manifest();
            SqlRows expected_history;
            for (const auto &step : prefix.steps)
                expected_history.append(history_row(step));
            require(file.rows(sentinel_sql) == expected_sentinel &&
                        file.rows(history_sql) == expected_history && expected_history.size() == 15,
                    "Known retained bytes and all independently framed v15 history are committed");
            const auto before_schema = file.rows(schema_sql);
            const auto producer = provider_credentials_repository_manifest();

            expect_report(migrate(*file.writer, MigrationMode::inspect, producer),
                          CatalogState::upgrade_required, 15, 16, 0);
            require(file.rows(schema_sql) == before_schema &&
                        file.rows(history_sql) == expected_history &&
                        file.rows(sentinel_sql) == expected_sentinel,
                    "Inspection leaves schema, history and retained bytes untouched");

            file.advance_producer();
            expected_history.append(history_row(literal_v16().steps.back()));
            require(file.rows(history_sql) == expected_history &&
                        file.rows(sentinel_sql) == expected_sentinel,
                    "Advance appends just the v16 marker and preserves the committed prefix");
            const auto upgraded_schema = file.rows(schema_sql);
            file.reopen();
            expect_report(migrate(*file.writer, MigrationMode::inspect, producer),
                          CatalogState::current, 16, 16, 0);
            expect_report(migrate(*file.writer, MigrationMode::advance, producer),
                          CatalogState::current, 16, 16, 0);
            expect_migration_refusal(migrate(*file.writer, MigrationMode::inspect, prefix),
                                     MigrationReason::unsupported_version);
            require(file.rows(schema_sql) == upgraded_schema &&
                        file.rows(history_sql) == expected_history &&
                        file.rows(sentinel_sql) == expected_sentinel,
                    "Reopen, no-op advance and older inspection preserve committed state");
        });
    }

    void lookalikeAtV15Refuses_V16_01() {
        check([] {
            CatalogFile file;
            file.write([](const auto &session) { (void)session.execute({credential_ddl, {}}); });
            const auto schema = file.rows(schema_sql);
            const auto history = file.rows(history_sql);
            expect_migration_refusal(migrate(*file.writer, MigrationMode::inspect,
                                             provider_credentials_repository_manifest()),
                                     MigrationReason::schema_mismatch);
            expect_migration_refusal(migrate(*file.writer, MigrationMode::advance,
                                             provider_credentials_repository_manifest()),
                                     MigrationReason::schema_mismatch);
            require(file.rows(schema_sql) == schema && file.rows(history_sql) == history,
                    "A lookalike at v15 is neither adopted nor removed");
        });
    }

    void exactPhysicalSchemaAndRecordReuse_V16_02() {
        check([] {
            CatalogFile file;
            file.advance_producer();
            const auto literal = literal_v16();
            const auto &step = literal.steps.back();
            expect_prefix(provider_credentials_repository_manifest(), literal);
            expect_schema(file.rows(schema_sql), step);
            require(file.rows(history_sql).back() == history_row(step),
                    "Latest history row has exact types, marker bytes and independent digest");

            QCOMPARE(file.rows("SELECT cid,name,type,\"notnull\",pk,hidden "
                               "FROM pragma_table_xinfo(?) ORDER BY cid",
                               {table_name}),
                     (SqlRows{{0, QString("scope"), QString("BLOB"), 1, 1, 0},
                              {1, QString("record"), QString("BLOB"), 1, 0, 0}}));
            QCOMPARE(
                file.rows("SELECT ncol,wr,strict FROM pragma_table_list(?) WHERE schema='main'",
                          {table_name}),
                (SqlRows{{2, 0, 0}}));
            QCOMPARE(file.rows("SELECT name,\"unique\",origin,partial FROM pragma_index_list(?)",
                               {table_name}),
                     (SqlRows{{index_name, 1, QString("pk"), 0}}));
            QCOMPARE(file.rows("SELECT cid,name,\"desc\",coll FROM pragma_index_xinfo(?) "
                               "WHERE key=1 ORDER BY seqno",
                               {index_name}),
                     (SqlRows{{0, QString("scope"), 0, QString("BINARY")}}));

            const auto scope = QByteArray::fromHex("00ff");
            const auto other = QByteArray::fromHex("00fe");
            const auto record = QByteArray::fromHex("ff0041");
            file.write([&](const auto &session) { add_blob(session, scope, record); });
            require(file.rows(stored_blobs) ==
                        SqlRows{{scope, record, QString("blob"), QString("blob")}},
                    "One BLOB row commits");
            file.write([&](const auto &session) { add_blob(session, other, record); });
            require(file.rows(stored_blobs) ==
                        SqlRows{{other, record, QString("blob"), QString("blob")},
                                {scope, record, QString("blob"), QString("blob")}},
                    "A distinct byte scope may carry the same record bytes");
        });
    }

    void rejectsInvalidPhysicalRows_V16_02_data() {
        QTest::addColumn<QVariant>("scope");
        QTest::addColumn<QVariant>("record");
        const auto distinct_scope = QByteArray::fromHex("0100");
        const auto distinct_record = QByteArray::fromHex("8100");
        QTest::newRow("duplicate-scope")
            << QVariant(QByteArray::fromHex("0101")) << QVariant(distinct_record);
        QTest::newRow("scope-text") << QVariant(QString("0100")) << QVariant(distinct_record);
        QTest::newRow("scope-integer") << QVariant(256) << QVariant(distinct_record);
        QTest::newRow("scope-null") << QVariant{} << QVariant(distinct_record);
        QTest::newRow("record-text") << QVariant(distinct_scope) << QVariant(QString("8100"));
        QTest::newRow("record-integer") << QVariant(distinct_scope) << QVariant(33024);
        QTest::newRow("record-null") << QVariant(distinct_scope) << QVariant{};
    }

    void rejectsInvalidPhysicalRows_V16_02() {
        QFETCH(QVariant, scope);
        QFETCH(QVariant, record);
        check([&] {
            CatalogFile file;
            file.advance_producer();
            const auto good_scope = QByteArray::fromHex("0101");
            const auto good_record = QByteArray::fromHex("8000");
            file.write([&](const auto &session) { add_blob(session, good_scope, good_record); });

            const auto caught = std::make_shared<bool>(false);
            const auto result = qt_test_support::run_work(
                *file.writer,
                [=](auto context) -> QVariant {
                    const auto session = context.begin();
                    try {
                        (void)session.execute({insert_sql, {scope, record}});
                    } catch (const SqliteWorkFailure &failure) {
                        *caught = failure.error() == WorkError::sql_failure;
                        throw;
                    }
                    session.commit();
                    return 1;
                },
                10000);
            require(*caught && result.error == WorkError::sql_failure &&
                        result.transaction == TransactionOutcome::rolled_back && !result.value,
                    "Constraint rejects the isolated insertion and rolls back");
            require(file.rows(stored_blobs) ==
                        SqlRows{{good_scope, good_record, QString("blob"), QString("blob")}},
                    "Refused insertion leaves only the original committed row");
        });
    }

    void credentialPutReplaceAndReopen_V16_03() {
        check([] {
            CatalogFile file;
            file.advance_producer();
            keys::KeyFixture fixture;
            const auto path = fixture.file();
            keys::KeyFixture::write(path);
            auto key = provision_key(path);

            file.write([&](const auto &session) {
                const CredentialRepository repository(session, *key);
                require(std::holds_alternative<CredentialMissing>(repository.load(exact_scope)),
                        "Initial production-table read is typed missing");
                require(std::holds_alternative<CredentialStaged>(
                            repository.stage_put(exact_scope, initial)),
                        "Production-table insert RETURNING stages the synthetic value");
            });
            expect_record(file, 67);
            expect_credential(file, key, initial);
            file.write([&](const auto &session) {
                require(
                    std::holds_alternative<CredentialStaged>(
                        CredentialRepository(session, *key).stage_put(exact_scope, replacement)),
                    "Authenticated production-table replacement RETURNING stages the new value");
            });
            expect_record(file, 66);
            expect_credential(file, key, replacement);

            key.reset();
            file.reopen();
            expect_report(migrate(*file.writer, MigrationMode::inspect,
                                  provider_credentials_repository_manifest()),
                          CatalogState::current, 16, 16, 0);
            expect_record(file, 66);
            expect_credential(file, provision_key(path), replacement);
        });
    }

    void genuineV14Control_V16_04() {
        check([] {
            const auto manifest = modifier_activations_repository_manifest();
            CatalogFile file(manifest);
            populate_v14(file);
            expect_report(migrate(*file.writer, MigrationMode::inspect, manifest),
                          CatalogState::current, 14, 14, 0);
            file.write([](const auto &session) {
                const auto pending = SqliteMilestoneGroupStore(session).pending();
                require(pending.size() == 1 &&
                            pending.front().group.occurrence == MilestoneOccurrenceKey{"z-first"} &&
                            pending.front().group.actions.size() == 4,
                        "Genuine v14 retains the selected milestone occurrence and capture");
                expect_v15_evidence(session, Consumer::modifier, {});
            });

            const FinancialPaymentKey payment{
                {"test-provider", "source"}, {"payment-v1", "p1"}, {1}};
            expect_consumer_refusal<SqliteBundleStorageFailure>(
                *file.writer, SqliteBundleStorageReason::invalid_stored_data,
                [=](const auto &session) {
                    (void)SqliteFinancialRefundStore(session).history(payment, 16);
                });
        });
    }

    void selectedConsumerMarkers_V16_04_data() {
        QTest::addColumn<int>("consumer");
        QTest::addColumn<int>("marker_case");
        for (const auto &[name, which] : {std::pair{"financial", Consumer::financial},
                                          {"milestone", Consumer::milestone},
                                          {"modifier", Consumer::modifier}}) {
            QTest::newRow(qPrintable(QString("%1-exact-v16").arg(name)))
                << int(which) << int(MarkerCase::exact_v16);
            QTest::newRow(qPrintable(QString("%1-case-changed-v16").arg(name)))
                << int(which) << int(MarkerCase::case_changed_v16);
            QTest::newRow(qPrintable(QString("%1-exact-id-v17").arg(name)))
                << int(which) << int(MarkerCase::exact_id_v17);
        }
    }

    void selectedConsumerMarkers_V16_04() {
        QFETCH(int, consumer);
        QFETCH(int, marker_case);
        check([&] {
            const auto which = Consumer(consumer);
            const auto marker_case_value = MarkerCase(marker_case);
            finance::File file(finance::OriginalCredit{.milestone = true});
            populate_v15(file);
            const auto read = [which, payment = file.payment](const auto &session) {
                expect_v15_evidence(session, which, payment);
            };
            file.write([payment = file.payment](const auto &session) {
                expect_v15_evidence(session, Consumer::financial, payment);
                expect_v15_evidence(session, Consumer::milestone, payment);
                expect_v15_evidence(session, Consumer::modifier, payment);
            });

            // The literal branch reaches each reader with exact physical v16
            // even while the real producer still targets protected v15.
            expect_report(migrate(*file.writer, MigrationMode::advance, literal_v16()),
                          CatalogState::current, 16, 16, 1);
            if (marker_case_value == MarkerCase::exact_v16) {
                file.write(read);
                return;
            }
            select_invalid_marker(file, marker_case_value);
            if (which == Consumer::modifier)
                expect_consumer_refusal<SqliteModifierStorageFailure>(
                    *file.writer, SqliteModifierStorageReason::invalid_stored_data, read);
            else
                expect_consumer_refusal<SqliteBundleStorageFailure>(
                    *file.writer, SqliteBundleStorageReason::invalid_stored_data, read);
        });
    }
};

QTEST_GUILESS_MAIN(SqliteCredentialCatalogTests)
#include "sqlite_credential_catalog_tests.moc"
