// SPDX-License-Identifier: GPL-3.0-or-later
// Public caller-session index acceptance. Setup/oracles use frozen OLD primitives only.
#include "support/sqlite_reference_capture_fixture.hpp"
#include <QTemporaryDir>
#include <array>
#include <exception>
#include <limits>
#include <optional>
#include <subathon/server/sqlite_reference_capture.hpp>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>
#include <thread>

using namespace reference_capture_fixture;
namespace core = subathon::core;
namespace {
using Disposition = ReferenceCaptureRetainDisposition;
using Reason = ReferenceCaptureStorageReason;
using Input = ReferenceInputError;
constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
ReferenceInitialCapture record(bool second = false) {
    return {{"I", {{"synthetic.reference.v1", "A"}, "N", second ? std::string("E\0", 2) : "E"}},
            {second ? maximum : 7},
            {second ? maximum : 0},
            second ? "C2" : "C",
            second ? "X2" : "X",
            second ? "V2" : "V",
            {second ? "P2" : "P", 1},
            second ? "Y2" : "Y"};
}
void same(const ReferenceInitialCapture &a, const ReferenceInitialCapture &b) {
    require(
        a.key.installation_ref == b.key.installation_ref && a.key.source == b.key.source &&
            a.receipt_id == b.receipt_id && a.contribution_id == b.contribution_id &&
            a.configuration_ref == b.configuration_ref &&
            a.arrival_context_ref == b.arrival_context_ref && a.event_ref == b.event_ref &&
            a.plan.plan_ref == b.plan.plan_ref && a.plan.revision == b.plan.revision &&
            a.execution_context_ref == b.execution_context_ref,
        "Complete independently expected first capture, including real CID0 and exact opaque key");
}
template <class Body> void scenario(Body body) {
    try {
        body();
    } catch (const SqliteReferenceCaptureNotImplemented &) {
        QTest::qFail("MISSING_REFERENCE_STORE: SqliteReferenceCaptureNotImplemented", __FILE__,
                     __LINE__);
    } catch (const ReferenceCaptureValuesNotImplemented &) {
        QTest::qFail("MISSING_REFERENCE_CODEC: ReferenceCaptureValuesNotImplemented", __FILE__,
                     __LINE__);
    } catch (const std::exception &e) {
        QTest::qFail(e.what(), __FILE__, __LINE__);
    }
}
template <class Body> void input_failure(Body body, Input expected) {
    bool caught = false;
    try {
        body();
    } catch (const ReferenceInputFailure &e) {
        require(e.code() == expected &&
                    std::string(e.what()) == "Reference application input failed",
                "Exact pure input error and fixed message");
        caught = true;
    }
    require(caught, "Expected pure ReferenceInputFailure");
}
template <class Body> void storage_failure(Body body, Reason expected) {
    bool caught = false;
    try {
        body();
    } catch (const ReferenceCaptureStorageFailure &e) {
        require(e.reason() == expected &&
                    std::string(e.what()) == "Reference initial capture did not commit",
                "Exact known local storage reason and fixed message");
        caught = true;
    }
    require(caught, "Expected known ReferenceCaptureStorageFailure");
}
template <class Body> void work_failure(Body body, WorkError expected) {
    bool caught = false;
    try {
        body();
    } catch (const SqliteWorkFailure &e) {
        require(e.error() == expected, "Exact original writer capability/unknown error");
        caught = true;
    }
    require(caught, "Expected original SqliteWorkFailure");
}
struct DeclaredBodyFailure : std::runtime_error {
    DeclaredBodyFailure() : std::runtime_error("Synthetic postcommit body failure") {}
};
} // namespace

class SqliteReferenceCaptureTest : public QObject {
    Q_OBJECT
    std::unique_ptr<QTemporaryDir> directory_;
    std::unique_ptr<SqliteWriter> writer_;
    std::unique_ptr<Data> data_;
    QString path_;
    // Preserve the real work outcome and rethrow actual unexpected body exceptions,
    // especially markers; never turn setup/work_exception into intended feature red.
    SqliteWorkResult exercise(SqliteWriter::Work body) {
        std::exception_ptr error;
        auto result = outcome(*writer_, [&](auto context) {
            try {
                return body(context);
            } catch (const DeclaredBodyFailure &) {
                throw;
            } catch (...) {
                error = std::current_exception();
                throw;
            }
        });
        if (error)
            std::rethrow_exception(error);
        return result;
    }
    void committed(const SqliteWorkResult &r) {
        require(r.transaction == TransactionOutcome::committed && r.error == WorkError::none &&
                    r.value,
                "Actual acknowledged normal caller commit only");
    }
    void rolled_back(const SqliteWorkResult &r) {
        require(r.transaction == TransactionOutcome::rolled_back && !r.value,
                "Actual whole rollback and no normal published value");
    }
    void raw(const QString &sql) {
        run(*writer_, [&](auto c) {
            const auto s = c.begin();
            (void)s.execute({sql, {}});
            s.commit();
            return QVariant(1);
        });
    }
    void full(bool second = false) {
        run(*writer_, [&](auto c) {
            const auto s = c.begin();
            if (second)
                data_->intake(s, "B");
            data_->stage_capture(s, second ? "B" : "A");
            data_->index(s, second ? "B" : "A");
            s.commit();
            return QVariant(1);
        });
    }
    qlonglong count(const QString &table, const QString &where = {}) const {
        Observer o(path_);
        return o
            .rows("SELECT count(*) FROM main." + table +
                  (where.isEmpty() ? QString{} : " WHERE " + where))[0][0]
            .toLongLong();
    }
    void marker(const SqliteWorkSession &s) {
        (void)s.execute({"INSERT INTO main.fixture_capture_markers VALUES(1)", {}});
    }
    void no_lock_or_marker() {
        Observer o(path_);
        require(o.rows("SELECT count(*) FROM main.fixture_capture_markers")[0][0].toLongLong() == 0,
                "Whole rollback removed prior marker");
        (void)o.rows("BEGIN IMMEDIATE");
        (void)o.rows("ROLLBACK");
    }
  private slots:
    void init() {
        try {
            directory_ = std::make_unique<QTemporaryDir>();
            require(directory_->isValid(), "Disposable directory");
            path_ = directory_->filePath("capture.sqlite");
            data_ = std::make_unique<Data>();
            writer_ = std::make_unique<SqliteWriter>();
            require(writer_->open(path_, limits()) == StorageError::none, "Existing writer opens");
            data_->schema(*writer_);
            run(*writer_, [&](auto c) {
                const auto s = c.begin();
                data_->intake(s, "A");
                (void)s.execute({"CREATE TABLE fixture_capture_markers(value INTEGER)", {}});
                s.commit();
                return QVariant(1);
            });
        } catch (const std::exception &e) {
            QTest::qFail(e.what(), __FILE__, __LINE__);
        }
    }
    void cleanup() {
        writer_.reset();
        data_.reset();
        directory_.reset();
    }

    void manifestMatchesFrozenPrefixAndStepSeven() {
        scenario([&] {
            const auto actual = reference_capture_repository_manifest(),
                       expected = data_->manifest();
            require(
                actual.catalog_format == 1 && actual.store_identity == "subathon.sqlite-store" &&
                    actual.steps.size() == 7 &&
                    actual.supported_starting_versions == QList<qint32>({0, 1, 2, 3, 4, 5, 6, 7}),
                "Exact complete manifest coordinates");
            for (qsizetype n = 0; n < 7; ++n) {
                const auto &a = actual.steps[n], &b = expected.steps[n];
                require(
                    a.version == b.version && a.id == b.id && a.sql == b.sql &&
                        a.expected_schema == b.expected_schema,
                    "All immutable prefix statements/projections and literal additive step exact");
                const auto m = n < 6 ? data_->prefixes[n] : data_->frozen["manifest"].toObject();
                require(migration_content_sha256(a).toHex() ==
                            m["sha256_hex"].toString().toLatin1(),
                        "Independent frozen per-step digest");
            }
            require(actual.steps.back().sql.size() == 1 &&
                        actual.steps.back().expected_schema.size() == 92 &&
                        canonical_migration_payload(actual.steps.back()).size() == 71601,
                    "Observed literal one-statement92-object step71601 bytes");
        });
    }
    void absentLookupIgnoresUnindexedAndOffScopeData() {
        scenario([&] {
            // An unrelated complete B exists but its closure is deliberately malformed;
            // A still has no index. Absence is not a database-wide semantic audit.
            full(true);
            raw("UPDATE main.subathon_plan_requests SET requested_units=600001 WHERE "
                "plan_ref=X'5032'");
            committed(exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                SqliteReferenceCaptureStore store(s);
                require(!store.find(record().key),
                        "Absent exact A index despite unrelated corrupted B");
                auto key = record(true).key;
                key.installation_ref = "other";
                require(!store.find(key), "Installation remains part of complete exact key");
                s.commit();
                return QVariant(1);
            }));
            require(count("fixture_capture_markers") == 1, "Absence preserves caller transaction");
        });
    }
    void insertAndSameSessionReadAreProvisionalUntilCommit() {
        scenario([&] {
            std::optional<ReferenceInitialCapture> owned;
            committed(exercise([&](auto c) {
                const auto s = c.begin();
                data_->stage_capture(s, "A");
                SqliteReferenceCaptureStore store(s);
                require(store.retain_initial(record()) == Disposition::inserted,
                        "New complete first capture staged once");
                owned = store.find(record().key);
                require(owned.has_value(), "Own staged capture readable");
                same(*owned, record());
                require(count("subathon_reference_initial_captures") == 0 &&
                            count("subathon_plan_headers") == 0 &&
                            count("subathon_retained_contexts", "reference=X'59'") == 0,
                        "External reader sees none of three staged groups");
                s.commit();
                return QVariant(1);
            }));
            same(*owned, record());
            Observer o(path_);
            data_->verify(o, "A");
        });
    }
    void exactDuplicateReusesImmutableFirstMeaning() {
        scenario([&] {
            full();
            committed(exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                SqliteReferenceCaptureStore store(s);
                require(store.retain_initial(record()) == Disposition::already_present,
                        "Exact fully validated duplicate");
                const auto a = store.find(record().key);
                require(a.has_value(), "Duplicate retains original");
                same(*a, record());
                s.commit();
                return QVariant(1);
            }));
            require(count("subathon_reference_initial_captures") == 1 &&
                        count("fixture_capture_markers") == 1,
                    "No duplicate own rows or cleanup");
            Observer o(path_);
            data_->verify(o, "A");
        });
    }
    void existingConflictPrecedesProposedTargetResolution() {
        scenario([&] {
            full();
            committed(exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                auto candidate = record();
                candidate.plan.plan_ref = "absent-plan";
                candidate.execution_context_ref = "absent-context";
                candidate.configuration_ref = "absent-config";
                require(SqliteReferenceCaptureStore(s).retain_initial(candidate) ==
                            Disposition::conflict,
                        "Existing source meaning wins before new references");
                s.commit();
                return QVariant(1);
            }));
            require(count("fixture_capture_markers") == 1 &&
                        count("subathon_reference_initial_captures") == 1,
                    "Healthy conflict does not clean caller unit");
        });
    }
    void receiptAliasConflictsBeforeCandidateResolution() {
        scenario([&] {
            full();
            committed(exercise([&](auto c) {
                const auto s = c.begin();
                auto candidate = record();
                candidate.key.source.opaque_id = "other";
                candidate.plan.plan_ref = "absent";
                require(SqliteReferenceCaptureStore(s).retain_initial(candidate) ==
                            Disposition::conflict,
                        "Installation+receipt uniqueness protects FIRST source binding");
                s.commit();
                return QVariant(1);
            }));
            require(count("subathon_reference_initial_captures") == 1, "No alias row");
        });
    }
    void bothSelectedIndexesValidateBeforeConflict() {
        scenario([&] {
            full();
            full(true);
            raw("UPDATE main.subathon_plan_requests SET requested_units=600001 WHERE "
                "plan_ref=X'5032'");
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                auto candidate = record();
                candidate.receipt_id = {maximum};
                storage_failure(
                    [&] { (void)SqliteReferenceCaptureStore(s).retain_initial(candidate); },
                    Reason::invalid_stored_data);
                no_lock_or_marker();
                return QVariant(1);
            });
            rolled_back(r);
        });
    }
    void candidateMissingRootsRemainExplicit() {
        scenario([&] {
            const auto result = exercise([&](auto c) {
                const auto s = c.begin();
                data_->stage_capture(s, "A");
                SqliteReferenceCaptureStore store(s);
                auto v = record();
                v.receipt_id = {8};
                require(store.retain_initial(v) == Disposition::missing_reference,
                        "Direct absent receipt");
                v = record();
                v.contribution_id = {1};
                require(store.retain_initial(v) == Disposition::missing_reference,
                        "Direct absent CID including distinct zero");
                v = record();
                v.plan.plan_ref = "missing";
                // Ordered §4 resolves retained SRX first: Y still binds P, so this
                // candidate is a different binding before any missing-plan lookup.
                require(store.retain_initial(v) == Disposition::source_mismatch,
                        "Retained SRX plan mismatch precedes unresolved new plan");
                v = record();
                v.execution_context_ref = "missing";
                require(store.retain_initial(v) == Disposition::missing_reference,
                        "Direct absent execution context");
                require(count("subathon_reference_initial_captures") == 0,
                        "No failed candidate owns an index");
                // Host cleans all attempted plan/SRX staging before reporting capture refusal.
                s.rollback();
                return QVariant(1);
            });
            rolled_back(result);
        });
    }
    void healthyDifferentMeaningIsSourceMismatch() {
        scenario([&] {
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                data_->stage_capture(s, "A");
                SqliteReferenceCaptureStore store(s);
                auto v = record();
                v.key.source.opaque_id = "different";
                require(store.retain_initial(v) == Disposition::source_mismatch,
                        "Present receipt describes different complete identity");
                v = record();
                v.event_ref = "different";
                require(store.retain_initial(v) == Disposition::source_mismatch,
                        "Present initial plan describes different captured event");
                s.rollback();
                return QVariant(1);
            });
            rolled_back(r);
            require(count("subathon_plan_headers") == 0 &&
                        count("subathon_retained_contexts", "reference=X'59'") == 0,
                    "Host removes refused candidate staging");
        });
    }
    void unindexedPlanCanonicalStructurePrecedesCapturedComparison_data() {
        QTest::addColumn<bool>("malformed");
        QTest::newRow("noncanonical-equivalent-request") << true;
        QTest::newRow("canonical-different-request") << false;
    }
    void unindexedPlanCanonicalStructurePrecedesCapturedComparison() {
        scenario([&] {
            QFETCH(bool, malformed);
            // OLD literals/primitives only: commit a plan/SRX closure WITHOUT an
            // index. Both candidates retain the unchanged captured600000 rule.
            run(*writer_, [&](auto c) {
                const auto s = c.begin();
                data_->stage_capture(s, "A");
                (void)s.execute({"UPDATE main.subathon_plan_requests SET exact_numerator=?, "
                                 "exact_denominator=X'31',requested_units=? WHERE plan_ref=X'50' "
                                 "AND revision=X'0000000000000001' AND ordinal=1",
                                 {QByteArray(malformed ? "0600000" : "600001"),
                                  qlonglong(malformed ? 600000 : 600001)}});
                s.commit();
                return QVariant(1);
            });
            require(count("subathon_reference_initial_captures") == 0,
                    "No indexed binding can turn candidate comparison into indexed corruption");
            const auto result = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                if (malformed) {
                    bool caught = false;
                    try {
                        (void)SqliteReferenceCaptureStore(s).retain_initial(record());
                    } catch (const ReferenceCaptureStorageFailure &error) {
                        require(error.reason() == Reason::invalid_stored_data &&
                                    std::string(error.what()) ==
                                        "Reference initial capture did not commit",
                                "Present plan's noncanonical0600000/1 is structural corruption "
                                "before comparison");
                        // Actual observer BEGIN and marker absence INSIDE catch
                        // prove whole cleanup happened before error publication.
                        no_lock_or_marker();
                        caught = true;
                    }
                    require(caught, "Malformed canonical structure must throw instead of healthy "
                                    "source_mismatch");
                } else {
                    require(SqliteReferenceCaptureStore(s).retain_initial(record()) ==
                                Disposition::source_mismatch,
                            "Healthy canonical600001/1 differs from captured600000 and remains a "
                            "refusal");
                    const auto rows =
                        s.execute({"SELECT count(*) FROM main.fixture_capture_markers", {}});
                    require(rows.size() == 1 && rows[0].size() == 1 && rows[0][0].toLongLong() == 1,
                            "Healthy source mismatch preserves active caller marker before host "
                            "rollback");
                    require(count("fixture_capture_markers") == 0,
                            "Active marker is provisional to outside readers");
                    s.rollback();
                    no_lock_or_marker();
                }
                return QVariant(1);
            });
            rolled_back(result);
            require(count("subathon_reference_initial_captures") == 0 &&
                        count("subathon_plan_headers") == 1 &&
                        count("subathon_retained_contexts", "reference=X'59'") == 1,
                    "No index inserted; earlier committed selected plan/SRX remain after caller "
                    "rollback");
        });
    }
    void unindexedPlanValuationStructurePrecedesBinding_data() {
        QTest::addColumn<bool>("resolved");
        QTest::newRow("complete-resolved-quote-without-event-money") << true;
        QTest::newRow("missing-historical-with-dependent-request") << false;
    }
    void unindexedPlanValuationStructurePrecedesBinding() {
        scenario([&] {
            QFETCH(bool, resolved);
            run(*writer_, [&](auto c) {
                const auto s = c.begin();
                data_->stage_capture(s, "A");
                if (resolved) {
                    // Complete mandatory quote group and fitting optional fields;
                    // the intrinsic defect is the actual original receipt's NULL money.
                    (void)s.execute(
                        {"UPDATE main.subathon_plan_revisions SET valuation_state=2,"
                         "historical_resolution_ref=X'71',historical_original_decimal=X'312e3030',"
                         "historical_currency=X'555344',comparison_currency=X'455552',"
                         "effective_rate_decimal=X'302e39303030',fetched_rate_decimal=X'"
                         "302e39303031',"
                         "snapshot_ref=X'73',rate_source_ref=X'66',rate_timestamp_utc_ms=0,"
                         "fetch_timestamp_utc_ms=0,manual_decision_ref=NULL WHERE plan_ref=X'50' "
                         "AND revision=X'0000000000000001'",
                         {}});
                    const auto quote = s.execute(
                        {"SELECT valuation_state,typeof(historical_resolution_ref),"
                         "typeof(historical_original_decimal),typeof(historical_currency),"
                         "typeof(comparison_currency),typeof(effective_rate_decimal),typeof("
                         "snapshot_ref) "
                         "FROM main.subathon_plan_revisions WHERE plan_ref=X'50' "
                         "AND revision=X'0000000000000001'",
                         {}});
                    require(quote.size() == 1 && quote[0].size() == 7 &&
                                quote[0][0].toLongLong() == 2,
                            "OLD setup retained resolved state and all six mandatory quote fields");
                    for (qsizetype field = 1; field < 7; ++field)
                        require(quote[0][field].toString() == "blob",
                                "Complete quote fields are actual non-NULL BLOBs");
                } else {
                    (void)s.execute({"UPDATE main.subathon_plan_revisions SET valuation_state=1 "
                                     "WHERE plan_ref=X'50' AND revision=X'0000000000000001'",
                                     {}});
                    (void)s.execute(
                        {"UPDATE main.subathon_plan_requests SET requires_historical_valuation=1 "
                         "WHERE plan_ref=X'50' AND revision=X'0000000000000001' AND ordinal=1",
                         {}});
                }
                s.commit();
                return QVariant(1);
            });
            require(count("subathon_reference_initial_captures") == 0,
                    "Selected malformed plan has no index");
            const auto result = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                bool caught = false;
                try {
                    (void)SqliteReferenceCaptureStore(s).retain_initial(record());
                } catch (const ReferenceCaptureStorageFailure &error) {
                    require(error.reason() == Reason::invalid_stored_data &&
                                std::string(error.what()) ==
                                    "Reference initial capture did not commit",
                            "Intrinsic capture valuation failure precedes reference-profile "
                            "binding mismatch");
                    no_lock_or_marker();
                    caught = true;
                }
                require(caught, "Malformed valuation must throw after whole cleanup, not return "
                                "source_mismatch");
                return QVariant(1);
            });
            rolled_back(result);
            require(count("subathon_reference_initial_captures") == 0 &&
                        count("subathon_plan_headers") == 1,
                    "No capture inserted and earlier committed selected plan remains");
        });
    }
    void unindexedImportedOpaqueRefsStayHealthyMismatches_data() {
        QTest::addColumn<bool>("parent");
        QTest::newRow("generic-rule-ref-present-empty") << false;
        QTest::newRow("generic-parent-event-present-empty") << true;
    }
    void unindexedImportedOpaqueRefsStayHealthyMismatches() {
        scenario([&] {
            QFETCH(bool, parent);
            run(*writer_, [&](auto c) {
                const auto s = c.begin();
                data_->stage_capture(s, "A");
                (void)s.execute(
                    {parent ? "UPDATE main.subathon_plan_headers SET parent_event_ref=X'' WHERE "
                              "plan_ref=X'50'"
                            : "UPDATE main.subathon_plan_rules SET rule_ref=X'' WHERE "
                              "plan_ref=X'50' AND revision=X'0000000000000001' AND ordinal=1",
                     {}});
                s.commit();
                return QVariant(1);
            });
            require(count("subathon_reference_initial_captures") == 0,
                    "Healthy generic plan remains unindexed");
            const auto result = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                require(SqliteReferenceCaptureStore(s).retain_initial(record()) ==
                            Disposition::source_mismatch,
                        "Imported present-empty opaque value is generic valid data, incompatible "
                        "with reference profile");
                const auto rows =
                    s.execute({"SELECT count(*) FROM main.fixture_capture_markers", {}});
                require(rows.size() == 1 && rows[0].size() == 1 && rows[0][0].toLongLong() == 1,
                        "Healthy incompatible generic value preserves active caller marker");
                require(count("fixture_capture_markers") == 0, "Caller marker stays provisional");
                s.rollback();
                no_lock_or_marker();
                return QVariant(1);
            });
            rolled_back(result);
            require(count("subathon_reference_initial_captures") == 0 &&
                        count("subathon_plan_headers") == 1,
                    "Host rollback adds no index or loss of prior generic plan");
        });
    }
    void unindexedGenericChildrenDistinguishMismatchFromCorruption_data() {
        QTest::addColumn<bool>("gap");
        QTest::newRow("two-valid-contiguous-ordered-rule-duplicates") << false;
        QTest::newRow("one-rule-at-invalid-ordinal-two") << true;
    }
    void unindexedGenericChildrenDistinguishMismatchFromCorruption() {
        scenario([&] {
            QFETCH(bool, gap);
            run(*writer_, [&](auto c) {
                const auto s = c.begin();
                data_->stage_capture(s, "A");
                (void)s.execute(
                    {gap ? "UPDATE main.subathon_plan_rules SET ordinal=2 WHERE plan_ref=X'50' AND "
                           "revision=X'0000000000000001'"
                         : "INSERT INTO main.subathon_plan_rules SELECT "
                           "plan_ref,revision,2,rule_ref,version_ref,selected,explanation FROM "
                           "main.subathon_plan_rules WHERE plan_ref=X'50' AND "
                           "revision=X'0000000000000001' AND ordinal=1",
                     {}});
                s.commit();
                return QVariant(1);
            });
            require(count("subathon_reference_initial_captures") == 0,
                    "Generic child classification has no index");
            const auto result = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                if (gap) {
                    bool caught = false;
                    try {
                        (void)SqliteReferenceCaptureStore(s).retain_initial(record());
                    } catch (const ReferenceCaptureStorageFailure &error) {
                        require(
                            error.reason() == Reason::invalid_stored_data &&
                                std::string(error.what()) ==
                                    "Reference initial capture did not commit",
                            "Missing generic ordinal1 is intrinsic selected structure corruption");
                        no_lock_or_marker();
                        caught = true;
                    }
                    require(caught, "Noncontiguous generic rules must throw after whole cleanup");
                } else {
                    require(SqliteReferenceCaptureStore(s).retain_initial(record()) ==
                                Disposition::source_mismatch,
                            "Generic contiguous ordered duplicates are valid but incompatible with "
                            "exactly-one-rule reference profile");
                    const auto rows =
                        s.execute({"SELECT count(*) FROM main.fixture_capture_markers", {}});
                    require(rows.size() == 1 && rows[0].size() == 1 && rows[0][0].toLongLong() == 1,
                            "Healthy two-rule mismatch preserves active caller marker");
                    require(count("fixture_capture_markers") == 0,
                            "Healthy mismatch does not publish caller marker");
                    s.rollback();
                    no_lock_or_marker();
                }
                return QVariant(1);
            });
            rolled_back(result);
            require(count("subathon_reference_initial_captures") == 0 &&
                        count("subathon_plan_headers") == 1,
                    "No index inserted or earlier selected plan removed");
        });
    }
    void selectedCorruptionRollsBackWholeUnit_data() {
        QTest::addColumn<QString>("sql");
        QTest::newRow("SRC1") << "UPDATE main.subathon_retained_configurations SET document=X'00' "
                                 "WHERE reference=X'43'";
        QTest::newRow("SRA1")
            << "UPDATE main.subathon_retained_contexts SET document=X'00' WHERE reference=X'58'";
        QTest::newRow("SRX1")
            << "UPDATE main.subathon_retained_contexts SET document=X'00' WHERE reference=X'59'";
        QTest::newRow("original-facts") << "UPDATE main.subathon_receipts SET event_type=X'78' "
                                           "WHERE receipt_id=X'0000000000000007'";
        QTest::newRow("support") << "UPDATE main.subathon_contribution_observations SET "
                                    "support_application=1 WHERE receipt_id=X'0000000000000007'";
        QTest::newRow("request")
            << "UPDATE main.subathon_plan_requests SET requested_units=600001 WHERE plan_ref=X'50'";
        QTest::newRow("rule")
            << "UPDATE main.subathon_plan_rules SET explanation=X'78' WHERE plan_ref=X'50'";
        QTest::newRow("index-event") << "UPDATE main.subathon_reference_initial_captures SET "
                                        "event_ref=X'78' WHERE plan_ref=X'50'";
        QTest::newRow("missing-rule")
            << "DELETE FROM main.subathon_plan_rules WHERE plan_ref=X'50'";
        QTest::newRow("rule-ordinal")
            << "UPDATE main.subathon_plan_rules SET ordinal=2 WHERE plan_ref=X'50'";
        QTest::newRow("missing-request")
            << "DELETE FROM main.subathon_plan_requests WHERE plan_ref=X'50'";
        QTest::newRow("request-ordinal")
            << "UPDATE main.subathon_plan_requests SET ordinal=2 WHERE plan_ref=X'50'";
        QTest::newRow("noncanonical-exact")
            << "UPDATE main.subathon_plan_requests SET exact_numerator=X'30363030303030' WHERE "
               "plan_ref=X'50'";
        QTest::newRow("extra-modifier")
            << "INSERT INTO main.subathon_plan_modifiers "
               "VALUES(X'50',X'0000000000000001',1,X'64',X'69',X'54',X'67',X'6d',X'6d',X'31')";
        QTest::newRow("extra-rule")
            << "INSERT INTO main.subathon_plan_rules SELECT "
               "plan_ref,revision,2,rule_ref,version_ref,selected,explanation FROM "
               "main.subathon_plan_rules WHERE plan_ref=X'50'";
        QTest::newRow("extra-request")
            << "INSERT INTO main.subathon_plan_requests SELECT "
               "plan_ref,revision,2,action_ref,action_version_ref,rule_ref,target_ref,unit,"
               "requires_historical_valuation,exact_numerator,exact_denominator,requested_units,"
               "calculation_explanation,reversal_behavior_ref FROM main.subathon_plan_requests "
               "WHERE plan_ref=X'50'";
    }
    void selectedCorruptionRollsBackWholeUnit() {
        scenario([&] {
            QFETCH(QString, sql);
            full();
            raw(sql);
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                storage_failure([&] { (void)SqliteReferenceCaptureStore(s).find(record().key); },
                                Reason::invalid_stored_data);
                no_lock_or_marker();
                return QVariant(1);
            });
            rolled_back(r);
        });
    }
    void structurallyValidDocumentsNeedCompleteMatchingClosure_data() {
        QTest::addColumn<int>("change");
        QTest::newRow("unknown-category") << 0;
        QTest::newRow("embedded-config-mismatch") << 1;
        QTest::newRow("execution-receipt-mismatch") << 2;
        QTest::newRow("execution-cid-mismatch") << 3;
        QTest::newRow("execution-rule-mismatch") << 4;
    }
    void structurallyValidDocumentsNeedCompleteMatchingClosure() {
        scenario([&] {
            QFETCH(int, change);
            full();
            run(*writer_, [&](auto c) {
                const auto s = c.begin();
                const auto docs = data_->capture("A")["documents"].toObject();
                auto src = QByteArray::fromHex(
                    docs["SRC1"].toObject()["document_hex"].toString().toLatin1());
                auto sra = QByteArray::fromHex(
                    docs["SRA1"].toObject()["document_hex"].toString().toLatin1());
                auto srx = QByteArray::fromHex(
                    docs["SRX1"].toObject()["document_hex"].toString().toLatin1());
                if (change == 0) {
                    src[111] = char(0);
                    sra[26 + 111] = char(0);
                } else if (change == 1)
                    src[13] = 'Z';
                else if (change == 2)
                    srx[67] = char(8);
                else if (change == 3)
                    srx[75] = char(1);
                else
                    srx[93] = 'S';
                // All edits retain exact lengths, known tags and framed domains.
                // Unknown0 facts are structural DATA but fail admitted index meaning.
                (void)s.execute({"UPDATE main.subathon_retained_configurations SET document=? "
                                 "WHERE reference=X'43'",
                                 {src}});
                (void)s.execute(
                    {"UPDATE main.subathon_retained_contexts SET document=? WHERE reference=X'58'",
                     {sra}});
                (void)s.execute(
                    {"UPDATE main.subathon_retained_contexts SET document=? WHERE reference=X'59'",
                     {srx}});
                s.commit();
                return QVariant(1);
            });
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                storage_failure([&] { (void)SqliteReferenceCaptureStore(s).find(record().key); },
                                Reason::invalid_stored_data);
                no_lock_or_marker();
                return QVariant(1);
            });
            rolled_back(r);
        });
    }
    void corruptOriginalOutranksReplacementConflict() {
        scenario([&] {
            full();
            raw("UPDATE main.subathon_plan_requests SET requested_units=600001 WHERE "
                "plan_ref=X'50'");
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                auto v = record();
                v.plan.plan_ref = "missing";
                storage_failure([&] { (void)SqliteReferenceCaptureStore(s).retain_initial(v); },
                                Reason::invalid_stored_data);
                no_lock_or_marker();
                return QVariant(1);
            });
            rolled_back(r);
        });
    }
    void presentIntrinsicMalformedLinkIsNotMissingCandidate() {
        scenario([&] {
            run(*writer_, [&](auto c) {
                const auto s = c.begin();
                data_->stage_capture(s, "A");
                s.commit();
                return QVariant(1);
            });
            raw("UPDATE main.subathon_retained_contexts SET configuration_ref=NULL WHERE "
                "reference=X'59'");
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                storage_failure(
                    [&] { (void)SqliteReferenceCaptureStore(s).retain_initial(record()); },
                    Reason::invalid_stored_data);
                no_lock_or_marker();
                return QVariant(1);
            });
            rolled_back(r);
        });
    }
    void zeroAndMaxIdsAndNulKeysRemainDistinct() {
        scenario([&] {
            full();
            full(true);
            committed(exercise([&](auto c) {
                const auto s = c.begin();
                SqliteReferenceCaptureStore store(s);
                for (bool second : {false, true}) {
                    const auto v = record(second);
                    const auto a = store.find(v.key);
                    require(a.has_value(), "Exact source exists");
                    same(*a, v);
                }
                auto key = record().key;
                key.source.origin.source_account = std::string("A\0", 2);
                require(!store.find(key), "Account NUL suffix is distinct");
                s.commit();
                return QVariant(1);
            }));
            require(count("subathon_reference_initial_captures") == 2,
                    "CID0 and CIDMAX are independent real rows");
        });
    }
    void currentDimensionsRetriesAndLaterPlanHistoryAreUnselected() {
        scenario([&] {
            full();
            run(*writer_, [&](auto c) {
                const auto s = c.begin();
                (void)s.execute({"UPDATE main.subathon_contributions SET "
                                 "payment_evidence=0,review=0,application=1 WHERE "
                                 "contribution_id=X'0000000000000000'",
                                 {}});
                auto later = statement(data_->capture("A")["capture"].toArray()[1]);
                later.bindings[1] = QByteArray::fromHex("0000000000000002");
                later.bindings[2] = QByteArray::fromHex("0000000000000001");
                later.bindings[30] = 2;
                later.bindings[31] = QByteArray("actor");
                later.bindings[32] = QByteArray("reason");
                later.bindings[33] = QByteArray("decision");
                (void)s.execute(later);
                (void)s.execute({"UPDATE main.subathon_plan_headers SET "
                                 "head_revision=X'0000000000000002' WHERE plan_ref=X'50'",
                                 {}});
                auto attempt = statement(data_->capture("A")["intake"].toArray()[3]);
                attempt.bindings[1] = 2;
                attempt.bindings[2] = QByteArray("unselected-different-facts");
                (void)s.execute(attempt);
                s.commit();
                return QVariant(1);
            });
            committed(exercise([&](auto c) {
                const auto s = c.begin();
                SqliteReferenceCaptureStore store(s);
                const auto v = store.find(record().key);
                require(v.has_value(), "Initial selected historical binding remains");
                same(*v, record());
                require(store.retain_initial(record()) == Disposition::already_present,
                        "Duplicate retains plan1 independent of current head/dimensions/retry");
                s.commit();
                return QVariant(1);
            }));
        });
    }
    void pureInputPriorityPrecedesStaleCapability() {
        scenario([&] {
            SqliteWorkSession stale;
            for (auto bad : {qsizetype(0), std::numeric_limits<qsizetype>::max()}) {
                for (int field = 0; field < 3; ++field) {
                    ReferenceCaptureLimits l;
                    if (field == 0)
                        l.reference_bytes = bad;
                    else if (field == 1)
                        l.document_bytes = bad;
                    else
                        l.owned_bytes = bad;
                    auto v = record();
                    v.receipt_id = {0};
                    v.key.installation_ref.clear();
                    input_failure(
                        [&] { (void)SqliteReferenceCaptureStore(stale, l).retain_initial(v); },
                        Input::invalid_limits);
                    input_failure([&] { (void)SqliteReferenceCaptureStore(stale, l).find(v.key); },
                                  Input::invalid_limits);
                }
            }
            auto v = record();
            v.key.source.opaque_id = std::string(4097, 'x');
            v.receipt_id = {0};
            input_failure([&] { (void)SqliteReferenceCaptureStore(stale).retain_initial(v); },
                          Input::input_limit);
            v = record();
            v.key.installation_ref.clear();
            v.receipt_id = {0};
            input_failure([&] { (void)SqliteReferenceCaptureStore(stale).retain_initial(v); },
                          Input::invalid_reference);
            for (auto revision : {std::uint64_t(0), std::uint64_t(2), maximum}) {
                v = record();
                v.plan.revision = revision;
                input_failure([&] { (void)SqliteReferenceCaptureStore(stale).retain_initial(v); },
                              Input::invalid_value);
            }
            v = record();
            v.receipt_id = {0};
            input_failure([&] { (void)SqliteReferenceCaptureStore(stale).retain_initial(v); },
                          Input::invalid_value);
            work_failure([&] { (void)SqliteReferenceCaptureStore(stale).find(record().key); },
                         WorkError::stale_session);
        });
    }
    void pureRefusalPreservesEarlierCallerWork() {
        scenario([&] {
            committed(exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                auto v = record();
                v.event_ref.clear();
                input_failure([&] { (void)SqliteReferenceCaptureStore(s).retain_initial(v); },
                              Input::invalid_reference);
                (void)s.execute({"SELECT 1", {}});
                s.commit();
                return QVariant(1);
            }));
            require(count("fixture_capture_markers") == 1, "Pure failure did no SQL/cleanup");
        });
    }
    void validForeignAndExpiredTicketsPreserveWriterErrors() {
        scenario([&] {
            std::optional<SqliteReferenceCaptureStore> saved;
            committed(exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                saved.emplace(s);
                std::exception_ptr error;
                std::thread foreign([&] {
                    try {
                        work_failure([&] { (void)saved->find(record().key); },
                                     WorkError::wrong_thread);
                    } catch (...) {
                        error = std::current_exception();
                    }
                });
                foreign.join();
                if (error)
                    std::rethrow_exception(error);
                (void)s.execute({"SELECT 1", {}});
                s.commit();
                return QVariant(1);
            }));
            work_failure([&] { (void)saved->find(record().key); }, WorkError::stale_session);
        });
    }
    void activeTerminalTicketAndFacadeDestructionAreHonest() {
        scenario([&] {
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                {
                    SqliteReferenceCaptureStore facade(s);
                    require(!facade.find(record().key), "Healthy absent read");
                }
                marker(s);
                s.commit();
                work_failure([&] { (void)SqliteReferenceCaptureStore(s).find(record().key); },
                             WorkError::session_closed);
                return QVariant(1);
            });
            require(r.transaction == TransactionOutcome::committed && !r.value,
                    "Terminal protocol call grants no normal completion");
            require(count("fixture_capture_markers") == 1,
                    "Facade destructor did not clean caller work");
        });
    }
    void selectedDocumentsAtBoundAndOneUnder() {
        scenario([&] {
            full();
            committed(exercise([&](auto c) {
                const auto s = c.begin();
                ReferenceCaptureLimits l;
                l.document_bytes = 157;
                const auto v = SqliteReferenceCaptureStore(s, l).find(record().key);
                require(v.has_value(), "Largest selected A document157 fits");
                same(*v, record());
                s.commit();
                return QVariant(1);
            }));
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                ReferenceCaptureLimits l;
                l.document_bytes = 156;
                storage_failure([&] { (void)SqliteReferenceCaptureStore(s, l).find(record().key); },
                                Reason::result_limit);
                no_lock_or_marker();
                return QVariant(1);
            });
            rolled_back(r);
        });
    }
    void completeIndexProjectionPrecedesSemanticMismatch() {
        scenario([&] {
            full();
            run(*writer_, [&](auto c) {
                const auto s = c.begin();
                (void)s.execute({"UPDATE main.subathon_reference_initial_captures SET event_ref=? "
                                 "WHERE plan_ref=X'50'",
                                 {QByteArray(4096, 'x')}});
                s.commit();
                return QVariant(1);
            });
            for (bool fits : {false, true}) {
                const auto r = exercise([&](auto c) {
                    const auto s = c.begin();
                    marker(s);
                    ReferenceCaptureLimits l;
                    l.owned_bytes = fits ? 4150 : 4149;
                    storage_failure(
                        [&] { (void)SqliteReferenceCaptureStore(s, l).find(record().key); },
                        fits ? Reason::invalid_stored_data : Reason::result_limit);
                    no_lock_or_marker();
                    return QVariant(1);
                });
                rolled_back(r);
            }
        });
    }
    void selectedFieldCeilingIsCheckedBeforeOwnership() {
        scenario([&] {
            full();
            committed(exercise([&](auto c) {
                const auto s = c.begin();
                ReferenceCaptureLimits l;
                l.reference_bytes = 26;
                const auto value = SqliteReferenceCaptureStore(s, l).find(record().key);
                require(
                    value.has_value(),
                    "Selected reversal reference26 and every healthy A reference fit ceiling26");
                same(*value, record());
                s.commit();
                return QVariant(1);
            }));
            const auto selected = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                ReferenceCaptureLimits l;
                l.reference_bytes = 25;
                storage_failure([&] { (void)SqliteReferenceCaptureStore(s, l).find(record().key); },
                                Reason::result_limit);
                no_lock_or_marker();
                return QVariant(1);
            });
            rolled_back(selected);
            committed(exercise([&](auto c) {
                const auto s = c.begin();
                ReferenceCaptureLimits l;
                l.reference_bytes = 21;
                input_failure([&] { (void)SqliteReferenceCaptureStore(s, l).find(record().key); },
                              Input::input_limit);
                s.commit();
                return QVariant(1);
            }));
            run(*writer_, [&](auto c) {
                const auto s = c.begin();
                (void)s.execute({"UPDATE main.subathon_reference_initial_captures SET event_ref=? "
                                 "WHERE plan_ref=X'50'",
                                 {QByteArray(23, 'x')}});
                s.commit();
                return QVariant(1);
            });
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                ReferenceCaptureLimits l;
                l.reference_bytes = 22;
                storage_failure([&] { (void)SqliteReferenceCaptureStore(s, l).find(record().key); },
                                Reason::result_limit);
                no_lock_or_marker();
                return QVariant(1);
            });
            rolled_back(r);
            // Isolate an oversized selected index field above the otherwise
            // healthy closure ceiling26, preserving the original23/22 case above.
            run(*writer_, [&](auto c) {
                const auto s = c.begin();
                (void)s.execute({"UPDATE main.subathon_reference_initial_captures SET event_ref=? "
                                 "WHERE plan_ref=X'50'",
                                 {QByteArray(27, 'x')}});
                s.commit();
                return QVariant(1);
            });
            const auto isolated = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                ReferenceCaptureLimits l;
                l.reference_bytes = 26;
                storage_failure([&] { (void)SqliteReferenceCaptureStore(s, l).find(record().key); },
                                Reason::result_limit);
                no_lock_or_marker();
                return QVariant(1);
            });
            rolled_back(isolated);
        });
    }
    void explicitRollbackRemovesPlanExecutionIndexTogether() {
        scenario([&] {
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                data_->stage_capture(s, "A");
                require(SqliteReferenceCaptureStore(s).retain_initial(record()) ==
                            Disposition::inserted,
                        "Capture staged");
                s.rollback();
                return QVariant(1);
            });
            rolled_back(r);
            require(count("subathon_reference_initial_captures") == 0 &&
                        count("subathon_plan_headers") == 0 &&
                        count("subathon_retained_contexts", "reference=X'59'") == 0,
                    "All three groups removed");
            require(count("subathon_receipts") == 1 && count("subathon_contributions") == 1,
                    "Earlier intake/support commit remains");
            committed(exercise([&](auto c) {
                const auto s = c.begin();
                data_->stage_capture(s, "A");
                require(SqliteReferenceCaptureStore(s).retain_initial(record()) ==
                            Disposition::inserted,
                        "Known rollback permits later explicit healthy retry");
                s.commit();
                return QVariant(1);
            }));
        });
    }
    void hostCleansCompetingStagingAfterConflict() {
        scenario([&] {
            full();
            run(*writer_, [&](auto c) {
                const auto s = c.begin();
                data_->intake(s, "B");
                s.commit();
                return QVariant(1);
            });
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                data_->stage_capture(s, "B");
                auto v = record();
                v.plan = {"P2", 1};
                v.execution_context_ref = "Y2";
                require(SqliteReferenceCaptureStore(s).retain_initial(v) == Disposition::conflict,
                        "Original binding prevails despite competing staging");
                s.rollback();
                return QVariant(1);
            });
            rolled_back(r);
            require(count("subathon_plan_headers") == 1 &&
                        count("subathon_retained_contexts", "reference=X'5932'") == 0 &&
                        count("subathon_reference_initial_captures") == 1,
                    "No competing plan/SRX/index orphan committed");
        });
    }
    void cumulativeSqlBudgetsRequireActualWholeRollback_data() {
        QTest::addColumn<bool>("resultBudget");
        QTest::newRow("statements") << false;
        QTest::newRow("result-bytes") << true;
    }
    void cumulativeSqlBudgetsRequireActualWholeRollback() {
        scenario([&] {
            QFETCH(bool, resultBudget);
            full();
            writer_.reset();
            auto l = limits();
            if (resultBudget)
                l.result_value_bytes = 100;
            else
                l.statements_per_batch = 3;
            writer_ = std::make_unique<SqliteWriter>();
            require(writer_->open(path_, l) == StorageError::none,
                    "Reopen disposable state with selected actual SQL budget");
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                if (resultBudget) {
                    // OLD writer contract: each cell costs base8 + BLOB payload.
                    // Payload92 therefore consumes the exact100-byte result budget.
                    const auto exhausted = s.execute({"SELECT zeroblob(92)", {}});
                    require(exhausted.size() == 1 && exhausted[0].size() == 1 &&
                                exhausted[0][0].metaType().id() == QMetaType::QByteArray &&
                                !exhausted[0][0].isNull() &&
                                exhausted[0][0].toByteArray() == QByteArray(92, '\0'),
                            "OLD primitive returned complete92-byte BLOB within exact100-byte cell "
                            "budget");
                } else {
                    (void)s.execute({"SELECT 1", {}});
                    (void)s.execute({"SELECT 1", {}});
                }
                storage_failure([&] { (void)SqliteReferenceCaptureStore(s).find(record().key); },
                                resultBudget ? Reason::result_limit : Reason::resource_limit);
                no_lock_or_marker();
                return QVariant(1);
            });
            rolled_back(r);
        });
    }
    void missingSqlTableMapsOnlyAfterKnownRollback() {
        scenario([&] {
            raw("DROP TABLE main.subathon_reference_initial_captures");
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                storage_failure([&] { (void)SqliteReferenceCaptureStore(s).find(record().key); },
                                Reason::sql_failure);
                no_lock_or_marker();
                return QVariant(1);
            });
            rolled_back(r);
        });
    }
    void lostCommitAcknowledgementNeverPublishesBinding() {
        scenario([&] {
            require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        *writer_, testing::WorkControlFault::lose_next_commit_ack) ==
                        StorageError::none,
                    "Inject only actual future successful COMMIT acknowledgement loss");
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                data_->stage_capture(s, "A");
                require(SqliteReferenceCaptureStore(s).retain_initial(record()) ==
                            Disposition::inserted,
                        "Staged capture");
                work_failure([&] { s.commit(); }, WorkError::uncertain_outcome);
                return QVariant(1);
            });
            require(r.transaction == TransactionOutcome::outcome_unknown &&
                        r.error == WorkError::uncertain_outcome && !r.value,
                    "Actual unknown/no normal publication/no replay");
            require(count("subathon_reference_initial_captures") == 1,
                    "Physical COMMIT occurred despite lost acknowledgement");
        });
    }
    void lostCleanupAcknowledgementOutranksLocalCorruption() {
        scenario([&] {
            full();
            raw("UPDATE main.subathon_plan_requests SET requested_units=600001 WHERE "
                "plan_ref=X'50'");
            require(testing::SqliteWriterWorkAccess::inject_next_control_ack_loss(
                        *writer_, testing::WorkControlFault::lose_next_rollback_ack) ==
                        StorageError::none,
                    "Inject actual future cleanup acknowledgement loss");
            const auto r = exercise([&](auto c) {
                const auto s = c.begin();
                marker(s);
                work_failure([&] { (void)SqliteReferenceCaptureStore(s).find(record().key); },
                             WorkError::uncertain_outcome);
                return QVariant(1);
            });
            require(r.transaction == TransactionOutcome::outcome_unknown &&
                        r.error == WorkError::uncertain_outcome && !r.value,
                    "Uncertain actual cleanup overrides known local reason");
            require(count("fixture_capture_markers") == 0 &&
                        count("subathon_reference_initial_captures") == 1,
                    "Physical rollback preserves previously committed index");
        });
    }
    void postCommitBodyFailureRetainsRowsWithoutNormalResult() {
        scenario([&] {
            const auto r = exercise([&](auto c) -> QVariant {
                const auto s = c.begin();
                data_->stage_capture(s, "A");
                require(SqliteReferenceCaptureStore(s).retain_initial(record()) ==
                            Disposition::inserted,
                        "Staged capture");
                s.commit();
                throw DeclaredBodyFailure();
            });
            require(r.transaction == TransactionOutcome::committed &&
                        r.error == WorkError::work_exception && !r.value,
                    "Real durable commit followed by body exception, no normal binding");
            require(count("subathon_reference_initial_captures") == 1,
                    "Acknowledged physical commit is never relabeled rollback");
        });
    }
    void queuedCompetingCandidatesPublishOnlyFirstCommit() {
        scenario([&] {
            run(*writer_, [&](auto c) {
                const auto s = c.begin();
                data_->intake(s, "B");
                s.commit();
                return QVariant(1);
            });
            std::array<SqliteWorkResult, 2> results;
            std::array<std::exception_ptr, 2> errors;
            std::array<bool, 2> done{false, false};
            for (size_t n = 0; n < 2; ++n) {
                const auto admission = writer_->enqueue_work(
                    [&, n](auto c) -> QVariant {
                        try {
                            const auto s = c.begin();
                            data_->stage_capture(s, n == 0 ? "A" : "B");
                            auto v = record();
                            if (n == 1) {
                                v.plan = {"P2", 1};
                                v.execution_context_ref = "Y2";
                            }
                            const auto expected =
                                n == 0 ? Disposition::inserted : Disposition::conflict;
                            require(SqliteReferenceCaptureStore(s).retain_initial(v) == expected,
                                    "Serialized complete first candidate prevails");
                            if (n == 0)
                                s.commit();
                            else
                                s.rollback();
                            return QVariant(1);
                        } catch (...) {
                            errors[n] = std::current_exception();
                            throw;
                        }
                    },
                    [&, n](auto result) {
                        results[n] = std::move(result);
                        done[n] = true;
                    });
                require(admission == StorageError::none,
                        "Both candidates admitted before event-loop dispatch");
            }
            require(QTest::qWaitFor([&] { return done[0] && done[1]; }, 10000),
                    "Both actual queued units completed");
            for (const auto &error : errors)
                if (error)
                    std::rethrow_exception(error);
            committed(results[0]);
            rolled_back(results[1]);
            require(count("subathon_reference_initial_captures") == 1 &&
                        count("subathon_plan_headers") == 1 &&
                        count("subathon_retained_contexts", "reference=X'5932'") == 0,
                    "Second candidate leaves no competing plan/SRX/index");
        });
    }
    void returnedCaptureOwnsAllReferencesBeyondFacadeAndWork() {
        scenario([&] {
            full();
            std::optional<ReferenceInitialCapture> value;
            committed(exercise([&](auto c) {
                const auto s = c.begin();
                auto key = record().key;
                {
                    SqliteReferenceCaptureStore store(s);
                    value = store.find(key);
                }
                key.source.opaque_id = "changed";
                s.commit();
                return QVariant(1);
            }));
            require(value.has_value(), "Owned capture returned");
            writer_.reset();
            same(*value, record());
        });
    }
};
QTEST_GUILESS_MAIN(SqliteReferenceCaptureTest)
#include "sqlite_reference_capture_tests.moc"
