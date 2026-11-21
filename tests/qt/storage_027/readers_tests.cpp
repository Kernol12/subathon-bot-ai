// SPDX-License-Identifier: GPL-3.0-or-later
#include "../storage_026/receipt_fixture.hpp"
#include "../storage_026/transition_fixture.hpp"
#include "../support/credential_key_fixture.hpp"
#include "../support/sqlite_live_financial_refunds_fixture.hpp"
#include "../support/sqlite_modifier_activations_fixture.hpp"
#include "../support/sqlite_rate_override_writer_fixture.hpp"

#include "command_fixture.hpp"
#include <subathon/server/sqlite_delivery_catalog.hpp>
#include <subathon/server/sqlite_reference_capture.hpp>

namespace {
using namespace storage_027_fixture;
using predecessor::mutate;

// A diagnostic can distinguish a broken protected fixture from a missing
// successor selector even when the owning exception text is generic.
struct ReaderPhase {
    const char *current{"protected v20 fixture"};
};

template <class Body> void reader_check(Body body) {
    ReaderPhase phase;
    try {
        body(phase);
    } catch (const SqliteBundleStorageFailure &e) {
        QFAIL(qPrintable(QString("%1: bundle storage reason %2 (%3)")
                             .arg(phase.current)
                             .arg(int(e.reason()))
                             .arg(e.what())));
    } catch (const SqliteModifierStorageFailure &e) {
        QFAIL(qPrintable(QString("%1: modifier storage reason %2 (%3)")
                             .arg(phase.current)
                             .arg(int(e.reason()))
                             .arg(e.what())));
    } catch (const SqliteWorkFailure &e) {
        QFAIL(qPrintable(QString("%1: writer error %2 (%3)")
                             .arg(phase.current)
                             .arg(int(e.error()))
                             .arg(e.what())));
    } catch (const std::exception &e) {
        QFAIL(qPrintable(QString("%1: %2").arg(phase.current).arg(e.what())));
    }
}

void reopen_financial(finance::File &file, StorageLimits storage_limits) {
    auto drained = std::make_shared<bool>(false);
    require(file.writer->shutdown([drained] { *drained = true; }) == StorageError::none &&
                QTest::qWaitFor([drained] { return *drained; }, 10000),
            "Financial writer drains before reopening with the selected profile");
    file.observer.reset();
    file.writer.reset();
    file.open(storage_limits);
}

enum Entry {
    pending_queue,
    acceptance_decision,
    bundle_lookup,
    original_claim,
    reversal_basis,
    target_edit,
    target_preview,
    edit_job
};

void selected(CatalogFile &db, Populated &f, int entry) {
    db.write([&](const auto &s) {
        const SqliteApplicationBundleStore bundles(s);
        switch (entry) {
        case pending_queue: {
            const auto pending = SqliteMilestoneGroupStore(s).pending();
            require(pending.size() == 2 && pending[0].group.occurrence == f.app_occurrence &&
                        pending[1].group.occurrence == f.edit_occurrence &&
                        pending[1].group.edit_membership &&
                        pending[1].group.edit_membership->jobs == std::vector{f.job},
                    "Mixed pending queue retains the SAR2 application before the exact edit");
            break;
        }
        case acceptance_decision: {
            const SqliteMilestoneGroupDecisions decisions(s);
            const auto a = decisions.acceptance_audit(f.app_occurrence);
            require(a && a->choice == MilestoneActionChoice::run &&
                        a->evidence.decision_ref == "milestone-decision" &&
                        decisions.pending_disposition(f.edit_occurrence) ==
                            MilestonePendingDisposition::ready,
                    "Audit and pending decisions retain their actual origin and decision");
            break;
        }
        case bundle_lookup: {
            const auto retained = bundles.lookup(f.original);
            require(retained && retained->effects.size() == 1 &&
                        retained->effects[0].request_ordinal == 2 &&
                        retained->result.document.endsWith(
                            QByteArray::fromHex("0000000300000002010002")),
                    "Exact selected SAR2 read keeps its two no-effect terminal ordinals");
            break;
        }
        case original_claim: {
            const auto claim = bundles.lookup_original_claim({ApplicationSubjectKind::contribution,
                                                              f.selected.selector.receipt,
                                                              f.selected.selector.cid});
            require(claim && finance::same(claim->existing.intent.operation, f.original),
                    "Original-claim lookup returns only the actual retained SAR2 origin");
            break;
        }
        case reversal_basis: {
            const auto basis = bundles.read_reversal_basis({f.original, 2, finance::CK::score});
            require(
                basis.basis && basis.basis->actual_credit_units == 15 &&
                    !bundles.read_reversal_basis({f.original, 1, finance::CK::main_delta}).basis &&
                    !bundles.read_reversal_basis({f.original, 3, finance::CK::whole_count}).basis,
                "Only physical score is retained as a reversible effect");
            break;
        }
        case target_edit: {
            const auto read =
                read_milestone_target_edit(s, f.edit_authority, edit::actor, f.edit_intent);
            require(read && read->achievements.size() == 1 && read->achievements[0].order == 2,
                    "Read bound edit with its actual application prefix and order");
            break;
        }
        case target_preview: {
            const auto preview = preview_milestone_target_edit(s, f.edit_authority, edit::actor,
                                                               edit::single("keep", {"B", 104}));
            require(preview.score == 105 && preview.previous_targets.size() == 2,
                    "Target preview follows both committed origins against the same score head");
            break;
        }
        case edit_job: {
            const auto job = lookup_milestone_edit_job(s, f.job);
            require(job && job->revision == 1 && job->state == DeliveryJobState::queued &&
                        job->action_document == QByteArray::fromHex("0081") &&
                        job->capture.sender_ref == "edit-sender",
                    "Historical edit job retains exact capture and sender");
            break;
        }
        }
    });
}

struct ReleaseAuthority final : MilestoneEditReleaseAuthority {
    int calls{};
    bool allows(const SqliteWorkSession &, const MilestoneTrackKey &track,
                const MilestoneOccurrenceKey &occurrence) override {
        ++calls;
        return track == edit::track && occurrence == edit::b;
    }
};

struct DispatchAuthority final : MilestoneEditDispatchAuthority {
    bool allows(const MilestoneEditJob &value) override {
        return value.key.occurrence == edit::b && value.capture.sender_ref == "edit-sender" &&
               value.action_document == QByteArray::fromHex("0081");
    }
    std::optional<std::string> retry_policy(const MilestoneEditJob &) override {
        return "retry-edit-1";
    }
};

MilestoneReleaseCompletion release(Populated &f, bool edit_lane,
                                   const std::shared_ptr<ReleaseAuthority> &authority) {
    return finance::completed<MilestoneReleaseCompletion>([&](auto done) {
        if (edit_lane)
            return release_milestone_head(f.db.writer(), f.edit_occurrence, authority,
                                          std::move(done));
        return release_milestone_head(f.db.writer(), f.app_occurrence, std::move(done));
    });
}

void still_unreleased(Populated &f) {
    require(f.db.rows("SELECT released FROM main.subathon_milestone_groups") == SqlRows{{0}} &&
                f.db.rows("SELECT released FROM main.subathon_milestone_edit_occurrences") ==
                    SqlRows{{0}},
            "No caller falls back or skips the protected earlier occurrence");
}

void recover_review_result(Populated &f) {
    auto review = f.service();
    const auto lookup = await<ContributionReview::Result>([&](auto done) {
        review.lookup(f.review_command.key, principal, "synthetic", std::move(done));
    });
    const auto &rows = f.reply(lookup).rows;
    require(rows.size() == 1 && rows[0].outcome && rows[0].outcome->facts &&
                rows[0].outcome->facts->bundle &&
                finance::same(*rows[0].outcome->facts->bundle, f.original),
            "Normal Review lookup selects the original journal and committed SAR2");
}
} // namespace

class Storage027ReadersTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { qt_test_support::fail_on_sql_warnings(); }

    void existingSelectors_data() {
        QTest::addColumn<int>("entry");
        QTest::newRow("pending-order") << int(pending_queue);
        QTest::newRow("acceptance-audit") << int(acceptance_decision);
        QTest::newRow("sar2-bundle-lookup") << int(bundle_lookup);
        QTest::newRow("original-claim") << int(original_claim);
        QTest::newRow("reversal-basis") << int(reversal_basis);
        QTest::newRow("target-edit-read") << int(target_edit);
        QTest::newRow("target-edit-preview") << int(target_preview);
        QTest::newRow("exact-edit-job") << int(edit_job);
    }

    void existingSelectors() {
        QFETCH(int, entry);
        reader_check([&](ReaderPhase &phase) {
            Populated f;
            phase.current = "protected v20 selected entry";
            selected(f.db, f, entry);
            phase.current = "literal successor advance and reopened inspect";
            advance_fixture(f.db);
            phase.current = "post-successor selected entry";
            selected(f.db, f, entry);
        });
    }

    void sar2HeadCannotBeBypassed_data() {
        QTest::addColumn<int>("damage");
        QTest::newRow("protected-sar2") << 0;
        QTest::newRow("missing-terminal-ordinal") << 1;
        QTest::newRow("wrong-terminal-disposition") << 2;
        QTest::newRow("lookalike-marker") << 3;
        QTest::newRow("missing-successor-shape") << 4;
    }

    void sar2HeadCannotBeBypassed() {
        QFETCH(int, damage);
        reader_check([&](ReaderPhase &phase) {
            Populated f;
            phase.current = "protected v20 pending SAR2 and edit witness";
            selected(f.db, f, pending_queue);
            phase.current = "literal successor advance and reopened inspect";
            advance_fixture(f.db);
            phase.current = "post-successor selected entry";
            const auto authority = std::make_shared<ReleaseAuthority>();
            if (damage == 0) {
                phase.current = "post-successor application release";
                const auto first = release(f, false, authority);
                finance::normal(first.work);
                require(first.disposition == MilestoneReleaseDisposition::released,
                        "Normal application release validates SAR2 and consumes the first head");
                phase.current = "post-successor edit release";
                const auto second = release(f, true, authority);
                finance::normal(second.work);
                require(second.disposition == MilestoneReleaseDisposition::released &&
                            authority->calls == 1,
                        "The authorized edit release consumes only the next head");
                return;
            }
            // Credit these corruption controls only after the unmodified successor
            // admits both release overloads. An unknown catalog masks the fault.
            constexpr const char *faults[]{
                "DELETE FROM main.subathon_review_terminal_requests WHERE request_ordinal=1",
                "UPDATE main.subathon_review_terminal_requests SET disposition=2 WHERE "
                "request_ordinal=1",
                "UPDATE main.subathon_migration_history SET "
                "migration_id='Milestone-slot-interruption-storage-v1'"
                " WHERE version=(SELECT max(version) FROM main.subathon_migration_history)",
                "DROP TABLE main.subathon_milestone_playback_evidence"};
            mutate(f.db, faults[damage - 1]);
            for (const auto edit_lane : {true, false}) {
                phase.current = edit_lane ? "damaged successor edit release"
                                          : "damaged successor application release";
                const auto blocked = release(f, edit_lane, authority);
                require(blocked.work.transaction == TransactionOutcome::rolled_back &&
                            !blocked.disposition,
                        "Both release overloads fail closed on the exact damaged selection");
                still_unreleased(f);
            }
            require(authority->calls == 0,
                    "Damaged catalog or SAR2 evidence reaches no edit authority");
        });
    }

    void partitionedOriginDistinctEntry_data() {
        QTest::addColumn<int>("entry");
        QTest::addColumn<int>("fault");
        const std::array entries{"group-stage", "record-acceptance", "track-initialize", "pending",
                                 "retained-edit-prefix"};
        const std::array faults{"clean", "missing-ordinal", "wrong-terminal-disposition"};
        for (int i = 0; i < int(entries.size()); ++i)
            for (int j = 0; j < int(faults.size()); ++j)
                QTest::newRow((QByteArray(entries[i]) + "/" + faults[j]).constData()) << i << j;
    }

    void partitionedOriginDistinctEntry() {
        QFETCH(int, entry);
        QFETCH(int, fault);
        reader_check([&](ReaderPhase &phase) {
            const std::array through{SetupThrough::bundle, SetupThrough::group,
                                     SetupThrough::decision, SetupThrough::both_origins,
                                     SetupThrough::both_origins};
            Populated f({.through = through.at(entry)});
            const MilestoneTrackSeed seed{
                edit::track,
                "cfg",
                "partition-control-definition",
                "partition-control-prefix",
                {{"threshold-100", 100}, {"B", 200}},
                std::vector<MilestoneTrackSeed::Member>{
                    {f.app_group.group.achievement, f.original, f.app_occurrence}}};
            const auto invoke = [&](const SqliteWorkSession &s) {
                switch (entry) {
                case 0:
                    SqliteMilestoneGroupStore(s).stage(f.original, {f.app_group});
                    return true;
                case 1:
                    SqliteMilestoneGroupDecisions(s).record_acceptance(
                        f.original, {PausedAcceptanceMode::normal,
                                     {"operator", "protected-audit"},
                                     {{f.app_occurrence, MilestoneActionChoice::defer}}});
                    return true;
                case 2:
                    return initialize_milestone_track(s, seed);
                case 3: {
                    const auto groups = SqliteMilestoneGroupStore(s).pending();
                    return groups.size() == 2 && groups[0].group.occurrence == f.app_occurrence &&
                           groups[1].group.occurrence == f.edit_occurrence;
                }
                case 4: {
                    const auto retained =
                        read_milestone_target_edit(s, f.edit_authority, edit::actor, f.edit_intent);
                    return retained && retained->achievements.size() == 1 &&
                           retained->achievements[0].order == 2;
                }
                }
                throw std::logic_error("Unlisted partitioned-origin entry");
            };
            phase.current = "protected v20 partitioned entry with controlled rollback";
            const auto protected_result = predecessor::read_missing(f.db, invoke);
            require(protected_result, "Each real protected entry reaches the exact SAR2 origin");
            phase.current = "literal successor partitioned-entry advance";
            advance_fixture(f.db);
            if (fault == 0) {
                phase.current = "post-successor clean named partitioned entry";
                f.db.write([&](const auto &s) {
                    require(invoke(s), "Clean successor enters the exact partitioned owner");
                });
                return;
            }
            phase.current = "post-successor damaged named partitioned entry";
            mutate(
                f.db,
                fault == 1
                    ? "DELETE FROM main.subathon_review_terminal_requests WHERE request_ordinal=1"
                    : "UPDATE main.subathon_review_terminal_requests SET disposition=2 WHERE "
                      "request_ordinal=1");
            const auto before = snapshot(f.db, fixture_manifest().steps.back().expected_schema);
            bool port_refused = false;
            const auto result = qt_test_support::run_work(
                f.db.writer(),
                [&](auto context) -> QVariant {
                    const auto s = context.begin();
                    try {
                        port_refused = !invoke(s);
                        s.rollback();
                    } catch (const SqliteBundleStorageFailure &e) {
                        port_refused = e.reason() == SqliteBundleStorageReason::invalid_stored_data;
                        throw;
                    }
                    return {};
                },
                10000);
            require(port_refused && result.transaction == TransactionOutcome::rolled_back &&
                        !result.value &&
                        before == snapshot(f.db, fixture_manifest().steps.back().expected_schema),
                    "Damaged terminal evidence cannot create or expose a selected successor entry");
        });
    }

    void freshSar2OwningStageAndReplay() {
        reader_check([](ReaderPhase &phase) {
            Populated origin({.through = SetupThrough::bundle});
            std::optional<RecordedApplicationBundle> recorded;
            origin.db.write([&](const auto &s) {
                recorded = SqliteApplicationBundleStore(s).lookup(origin.original);
                require(recorded && recorded->contribution && recorded->reversals.empty() &&
                            recorded->result.document.startsWith("SAR2"),
                        "Protected Review owns the exact independently staged SAR2 witness");
            });
            predecessor::Populated target(99, 7);
            install_protected(target);
            const auto selected =
                target.seed("terminal",
                            {finance::request(finance::Unit::milliseconds, 20, "main"),
                             finance::request(finance::Unit::micro_points, 15, "score"),
                             finance::request(finance::Unit::whole_count, 1, "count")},
                            false, true);
            require(selected.selector == origin.selected.selector,
                    "Fresh independent prerequisites retain the same precise owner scope");
            phase.current = "protected v20 separate accepted contribution preparation";
            finance::normal(qt_test_support::run_work(
                target.db.writer(),
                [&](auto context) {
                    SqliteContributionRepository repo(context);
                    auto unit = repo.begin();
                    const auto current = unit->find_activity(
                        {{"review-fixture", "account"}, "fixture-support", "terminal"});
                    require(current && current->state.conditions.review ==
                                           core::ContributionReview::pending,
                            "Owning evidence retains a still-pending source");
                    auto accepted = current->state;
                    accepted.conditions.review = core::ContributionReview::accepted;
                    unit->update_state(selected.selector.cid, accepted);
                    unit->commit();
                    return 1;
                },
                10000));
            const PreparedApplicationBundle bundle{
                recorded->intent,      recorded->advanced_before,
                recorded->after,       recorded->advance_explanation,
                recorded->effects,     recorded->jobs,
                recorded->transitions, {},
                recorded->result,      recorded->audit};
            const auto stage = [&](const auto &s) {
                return SqliteApplicationBundleStore(s).stage(bundle).disposition;
            };
            phase.current = "protected v20 fresh SAR2 stage with controlled rollback";
            require(predecessor::read_missing(target.db, stage) ==
                            SqliteBundleStageDisposition::staged &&
                        target.db.rows("SELECT count(*) FROM main.subathon_app_claims") ==
                            SqlRows{{0}},
                    "Valid independent producer reaches the actual partition stage without "
                    "consuming it");
            phase.current = "literal application partition successor advance";
            advance_fixture(target.db);
            phase.current = "post-successor fresh owning SAR2 stage";
            target.db.write([&](const auto &s) {
                require(stage(s) == SqliteBundleStageDisposition::staged,
                        "Exact successor admits the formerly proven normal SAR2 stage");
            });
            const auto retained = target.db.rows(
                "SELECT request_ordinal,disposition FROM main.subathon_review_terminal_requests "
                "ORDER BY request_ordinal");
            require(retained == SqlRows{{1, 1}, {3, 2}},
                    "Exact independent mask persists atomically with the one real score effect");
            target.db.reopen(profile());
            phase.current = "post-successor cold SAR2 exact owning replay";
            target.db.write([&](const auto &s) {
                const auto again = SqliteApplicationBundleStore(s).stage(bundle);
                require(again.disposition == SqliteBundleStageDisposition::already_present &&
                            again.existing && again.existing->effects.size() == 1 &&
                            again.existing->effects[0].request_ordinal == 2 &&
                            again.existing->result.document == recorded->result.document,
                        "Exact replay keeps one score effect with both original terminal ordinals");
            });
            require(
                target.db.rows("SELECT count(*) FROM main.subathon_app_claims") == SqlRows{{1}} &&
                    target.db.rows(
                        "SELECT request_ordinal,disposition "
                        "FROM main.subathon_review_terminal_requests ORDER BY request_ordinal") ==
                        retained,
                "Reopened replay creates no claim or partition rows");
        });
    }

    void denseLegacyAndSuccessorTerminalGate_data() {
        QTest::addColumn<int>("fault");
        QTest::newRow("genuine-older-dense-and-clean-successor") << 0;
        QTest::newRow("malformed-sar2-tag") << 1;
        QTest::newRow("extra-dense-terminal-ordinal") << 2;
    }

    void denseLegacyAndSuccessorTerminalGate() {
        QFETCH(int, fault);
        reader_check([&](ReaderPhase &phase) {
            phase.current = "genuine admitted pre-v19 dense read";
            finance::File older;
            older.write([](const auto &s) {
                require(SqliteApplicationBundleStore(s).lookup(finance::original).has_value(),
                        "A genuine older catalog independently admits dense historical records");
            });
            predecessor::Populated f;
            install_protected(f);
            const auto source = f.seed("dense-selected", Case::positive());
            auto review = f.service();
            const auto command = f.bind("dense-selected", {source}, f.preview(review, {source}));
            phase.current = "protected v20 real dense Review commit";
            const auto executed = f.execute(review, command);
            expected_row(f.reply(executed), 1, ReviewDisposition::committed, ReviewStatus::applied);
            const auto dense = *f.reply(executed).rows[0].outcome->facts->bundle;
            const auto scope = QByteArray::fromStdString(dense.operation_scope);
            const auto key = QByteArray::fromStdString(dense.operation_ref);
            const auto body =
                f.db.rows("SELECT format_ref,substr(document,1,4) FROM main.subathon_app_results "
                          "WHERE operation_scope=? AND operation_ref=?",
                          {scope, key});
            require(body == SqlRows{{QByteArray("subathon.internal.application-result.v1"),
                                     QByteArray("SAR1")}},
                    "Real owner commits exact dense SAR1 without terminal ordinals");
            f.db.write([&](const auto &s) {
                require(SqliteApplicationBundleStore(s).lookup(dense).has_value(),
                        "Exact dense history resolves at protected v20");
            });
            phase.current = "literal successor dense advance";
            advance_fixture(f.db);
            if (fault == 1) {
                mutate(f.db,
                       "UPDATE main.subathon_app_results SET format_ref=?,document=? "
                       "WHERE operation_scope=? AND operation_ref=?",
                       {QByteArray("subathon.internal.application-result.v2"), QByteArray("SAR2x"),
                        scope, key});
            } else if (fault == 2) {
                mutate(
                    f.db,
                    "INSERT INTO main.subathon_review_terminal_requests "
                    "SELECT operation_scope,operation_ref,plan_ref,plan_revision,1,1 "
                    "FROM main.subathon_app_operations WHERE operation_scope=? AND operation_ref=?",
                    {scope, key});
            }
            phase.current = fault == 0 ? "post-successor dense SAR1 lookup"
                                       : "post-successor damaged dense/tagged selected gate";
            if (fault == 0) {
                f.db.write([&](const auto &s) {
                    require(SqliteApplicationBundleStore(s).lookup(dense).has_value(),
                            "Exact current reader preserves dense SAR1 selected history");
                });
                return;
            }
            bool refused = false;
            try {
                f.db.write(
                    [&](const auto &s) { (void)SqliteApplicationBundleStore(s).lookup(dense); });
            } catch (const SqliteBundleStorageFailure &e) {
                refused = e.reason() == SqliteBundleStorageReason::invalid_stored_data;
            }
            require(refused, "Tagged malformed result or extra dense terminal row fails closed");
        });
    }

    void editAttemptAcrossReopen_data() {
        QTest::addColumn<bool>("recovery");
        QTest::newRow("finish-and-retry") << false;
        QTest::newRow("exact-typed-recovery") << true;
    }

    void editAttemptAcrossReopen() {
        QFETCH(bool, recovery);
        reader_check([&](ReaderPhase &phase) {
            Populated f;
            const auto authority = std::make_shared<ReleaseAuthority>();
            phase.current = "protected v20 release and exact edit claim";
            for (const auto edit_lane : {false, true}) {
                const auto entry = release(f, edit_lane, authority);
                finance::normal(entry.work);
                require(entry.disposition == MilestoneReleaseDisposition::released,
                        "Owning release precedes the protected attempt");
            }
            DispatchAuthority dispatch;
            f.db.write([&](const auto &s) {
                const auto started = claim_milestone_edit_job(s, f.job, 1, dispatch);
                require(started.disposition == DeliveryDisposition::changed && started.job &&
                            started.job->attempt == 2,
                        "First real edit sending attempt was committed under protected v20");
            });
            phase.current = "literal successor advance and reopened inspect";
            advance_fixture(f.db);
            phase.current = "post-successor selected entry";
            f.db.write([&](const auto &s) {
                phase.current = "post-successor edit stranded selection";
                const auto page = stranded_milestone_delivery(s, {}, 1);
                require(page.attempts.size() == 1 && page.attempts[0].revision == 2 &&
                            std::get_if<MilestoneEditJobKey>(&page.attempts[0].key) &&
                            std::get<MilestoneEditJobKey>(page.attempts[0].key) == f.job,
                        "Stranded traversal after reopen preserves the exact typed attempt");
                if (recovery) {
                    phase.current = "post-successor selected edit recovery";
                    const auto recovered = recover_milestone_edit_job(s, f.job, 2);
                    require(recovered.disposition == DeliveryDisposition::changed &&
                                recovered.job &&
                                recovered.job->state == DeliveryJobState::outcome_unknown &&
                                recovered.job->revision == 3 && recovered.job->attempt == 2,
                            "Cold selected edit recovery retains the same exact started attempt");
                    return;
                }
                phase.current = "post-successor edit finish";
                const auto done = finish_milestone_edit_job(
                    s, f.job, 2, 2, DeliveryEvidence::known_undelivered, "attempt-2-failed");
                require(done.disposition == DeliveryDisposition::changed && done.job &&
                            done.job->state == DeliveryJobState::failed && done.job->revision == 3,
                        "Matching finish can append exact known-undelivered evidence");
                phase.current = "post-successor edit retry";
                const auto retried = retry_milestone_edit_job(s, f.job, 3, dispatch);
                require(
                    retried.disposition == DeliveryDisposition::changed && retried.job &&
                        retried.job->state == DeliveryJobState::queued &&
                        retried.job->revision == 4,
                    "Authorized retry preserves old attempt history and adds queued revision 4");
            });
        });
    }

    void independentV11DeliveryAcrossReopen() {
        reader_check([](ReaderPhase &phase) {
            phase.current = "fresh protected v11 migration";
            QTemporaryDir dir;
            require(dir.isValid(), "Disposable independent delivery store");
            const auto path = dir.filePath("independent.sqlite");
            auto writer = std::make_unique<SqliteWriter>();
            require(writer->open(path, finance::limits()) == StorageError::none,
                    "Single legacy writer opens");
            report(migrate(*writer,
                           {MigrationMode::advance, delivery_jobs_repository_manifest(), {}, {}}),
                   CatalogState::current, 11, 11, 11);
            const auto write = [&](auto body) {
                std::exception_ptr error;
                const auto result = qt_test_support::run_work(
                    *writer,
                    [&](auto c) -> QVariant {
                        const auto s = c.begin();
                        try {
                            body(s);
                            s.commit();
                        } catch (...) {
                            error = std::current_exception();
                            throw;
                        }
                        return 1;
                    },
                    10000);
                if (error)
                    std::rethrow_exception(error);
                finance::normal(result);
            };
            const auto reopen = [&] {
                auto drained = std::make_shared<bool>(false);
                require(writer->shutdown([drained] { *drained = true; }) == StorageError::none &&
                            QTest::qWaitFor([drained] { return *drained; }, 10000),
                        "Independent owner drains and reopens the exact same database");
                writer = std::make_unique<SqliteWriter>();
                require(writer->open(path, finance::limits()) == StorageError::none,
                        "One current owner reopens the legacy job file");
            };
            phase.current = "protected v11 retained sources and genuine receipt";
            write([](const auto &s) {
                SqliteRetainedContextStore retained(s);
                require(retained.retain({{RetainedKind::configuration, "cfg"}, "legacy-cfg", {}}) ==
                                RetainDisposition::inserted &&
                            retained.retain({{RetainedKind::arrival_context, "ctx"},
                                             "legacy-ctx",
                                             ConfigurationReference{"cfg"}}) ==
                                RetainDisposition::inserted &&
                            SqliteApplicationBundleStore(s).retain_initial_state(
                                {"legacy-head",
                                 finance::document(),
                                 "cfg",
                                 finance::timer(core::TimerLifecycle::Active, 1000),
                                 0,
                                 {}}) == SqliteBundleInitialDisposition::inserted,
                        "Protected stores create real retained records and initial head");
            });
            const core::ReceiptInput input{
                {{"producer", "account"}, "legacy-test", "independent-receipt"},
                {"test", 1, 1, {}, {}, {}, {}, {}},
                {100, "connection", "cfg", "ctx"}};
            std::optional<core::ReceiptId> id;
            finance::normal(qt_test_support::run_work(
                *writer,
                [&](auto c) -> QVariant {
                    SqliteReceiptRepository repo(c);
                    const auto r = ReceiptIntake(repo).ingest(input);
                    require(r.disposition == core::ReceiptDisposition::created,
                            "v11 receipt creates a real record");
                    id = r.receipt_id;
                    return 1;
                },
                10000));
            require(id.has_value(), "Receipt owner returns actual scoped ID");
            const DeliveryJobKey key{{"test", "independent"}, "job"};
            const auto doc = QByteArray::fromHex("0081");
            phase.current = "protected v11 independent delivery enrollment";
            write([&](const auto &s) {
                const ContributionReceipt receipt{*id, input.identity, input.facts, input.arrival};
                auto bundle = finance::File::receipt_application(s, receipt, "independent");
                finance::File::retain_plan(s, bundle.intent.plan,
                                           finance::File::plain_capture(receipt, {}, {}));
                bundle.jobs.push_back({"job", {}, {}, {}, {delivery_job_format, doc}});
                const DeliveryCapture capture{
                    "job",           "local-route", DeliverySenderSelection::explicit_identity,
                    "legacy-sender", "announce",    "announce-v1"};
                require(SqliteDeliveryJobStore(s).stage(bundle, {capture}).disposition ==
                            SqliteBundleStageDisposition::staged,
                        "Real v11 owner commits one exact job without a group or occurrence");
                const auto exact = SqliteDeliveryJobStore(s).lookup(key);
                require(exact && exact->revision == 1 && exact->action_document == doc,
                        "Protected v11 reads queued capture and binary action");
            });
            phase.current = "compiled protected v20 advance and legacy job read";
            report(
                migrate(*writer,
                        {MigrationMode::advance, connector_intake_repository_manifest(), {}, {}}),
                CatalogState::current, 20, 20, 9);
            reopen();
            write([&](const auto &s) {
                const auto job = SqliteDeliveryJobStore(s).lookup(key);
                require(job && job->state == DeliveryJobState::queued &&
                            job->capture.sender_ref == "legacy-sender",
                        "Protected v20 retains the independent v11 lane");
            });
            phase.current = "literal successor legacy job advance";
            auto manifest = fixture_manifest();
            const auto end = manifest.steps.back().version;
            MigrationLimits limits;
            limits.schema_objects = manifest.steps.back().expected_schema.size();
            report(migrate(*writer, {MigrationMode::advance, manifest, limits, {}}),
                   CatalogState::current, end, end, 1);
            reopen();
            report(migrate(*writer, {MigrationMode::inspect, manifest, limits, {}}),
                   CatalogState::current, end, end, 0);
            phase.current = "post-successor independent v11 lookup and guarded dispatch";
            write([&](const auto &s) {
                const auto job = SqliteDeliveryJobStore(s).lookup(key);
                require(job && job->state == DeliveryJobState::queued && job->revision == 1 &&
                            job->capture.sender_ref == "legacy-sender" &&
                            job->action_document == doc,
                        "Exact independent v11 capture stays readable after the successor");
            });
            struct Authority final : DeliveryDispatchAuthority {
                bool allows(const DeliveryJob &job) override {
                    return job.key.job_ref == "job" && job.capture.sender_ref == "legacy-sender";
                }
            };
            struct Adapter final : DeliveryAdapter {
                std::vector<DeliveryJob> started;
                void start(DeliveryJob job) override { started.push_back(std::move(job)); }
            };
            const auto adapter = std::make_shared<Adapter>();
            const auto start = finance::completed<DeliveryDispatchCompletion>([&](auto done) {
                return dispatch_delivery_job(*writer, key, 1, std::make_shared<Authority>(),
                                             adapter, std::move(done));
            });
            finance::normal(start.work);
            require(start.result && start.result->disposition == DeliveryDisposition::changed &&
                        adapter->started.size() == 1 && adapter->started[0].attempt == 2 &&
                        adapter->started[0].action_document == doc,
                    "Only the normally acknowledged v11 claim reaches the fake adapter");
            phase.current = "post-successor cold independent v11 stranded read and recovery";
            reopen();
            write([&](const auto &s) {
                const SqliteDeliveryJobStore store(s);
                const auto page = store.stranded({}, 1);
                require(page.attempts.size() == 1 && page.attempts[0].sending_revision == 2 &&
                            finance::same(page.attempts[0].key.operation, key.operation) &&
                            page.attempts[0].key.job_ref == key.job_ref,
                        "Exact cold page can recover its independent retained attempt");
                const auto recovered = store.recover(key, 2);
                require(recovered.disposition == DeliveryDisposition::changed && recovered.job &&
                            recovered.job->state == DeliveryJobState::outcome_unknown &&
                            recovered.job->revision == 3 && recovered.job->attempt == 2 &&
                            adapter->started.size() == 1,
                        "Recovery records uncertainty and never starts the adapter again");
            });
        });
    }

    void milestoneApplicationDelivery_data() {
        QTest::addColumn<QString>("entry");
        for (const auto *name : {"lookup", "unreleased-dispatch", "released-dispatch", "stranded"})
            QTest::newRow(name) << QString(name);
    }

    void milestoneApplicationDelivery() {
        QFETCH(QString, entry);
        reader_check([&](ReaderPhase &phase) {
            finance::File f(finance::OriginalCredit{.milestone = true});
            report(migrate(
                       *f.writer,
                       {MigrationMode::advance, contribution_review_repository_manifest(), {}, {}}),
                   CatalogState::current, 19, 19, 4);
            phase.current = "compiled v20 application-job upgrade";
            report(
                migrate(*f.writer,
                        {MigrationMode::advance, connector_intake_repository_manifest(), {}, {}}),
                CatalogState::current, 20, 20, 1);
            reopen_financial(f, finance::limits());
            phase.current = "protected v20 selected application job";
            struct Ports final : DeliveryDispatchAuthority, DeliveryAdapter {
                unsigned decisions{};
                std::vector<DeliveryJob> started;
                bool allows(const DeliveryJob &job) override {
                    ++decisions;
                    require(job.key.job_ref == "original-job" &&
                                job.capture.route_ref == "local-fixture" &&
                                job.action_document == "synthetic-action",
                            "Exact captured job enters local authority");
                    return true;
                }
                void start(DeliveryJob job) override { started.push_back(std::move(job)); }
            };
            const DeliveryJobKey key{finance::original, "original-job"};
            auto ports = std::make_shared<Ports>();
            const auto dispatch = [&](DeliveryDisposition expected) {
                const auto result = finance::completed<DeliveryDispatchCompletion>([&](auto done) {
                    return dispatch_delivery_job(*f.writer, key, 1, ports, ports, std::move(done));
                });
                finance::normal(result.work);
                require(
                    result.result && result.result->disposition == expected &&
                        !result.adapter_threw,
                    "Dispatch obeys exact occurrence release and returns an acknowledged outcome");
                const bool started = expected == DeliveryDisposition::changed;
                require(ports->started.size() == std::size_t(started),
                        "Only a committed sending attempt reaches the bounded fake adapter");
                if (started)
                    require(ports->started[0].state == DeliveryJobState::sending &&
                                ports->started[0].revision == 2 && ports->started[0].attempt == 2,
                            "Adapter sees the committed immutable job at sending attempt 2");
            };
            const auto lookup = [&](DeliveryJobState expected, int revision) {
                f.write([&](const auto &s) {
                    const auto saved = SqliteDeliveryJobStore(s).lookup(key);
                    require(saved && saved->state == expected && saved->revision == revision &&
                                saved->action_document == "synthetic-action" &&
                                saved->capture.sender_ref == "fixture-sender",
                            "Exact enrolled job lookup validates its real occurrence membership");
                });
            };
            const auto release = [&] {
                const auto result = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
                    return release_milestone_head(*f.writer, finance::achievement, std::move(done));
                });
                finance::normal(result.work);
                require(result.disposition == MilestoneReleaseDisposition::released,
                        "Real application head is released through its owning writer");
            };
            const auto scan = [&] {
                f.write([&](const auto &s) {
                    const auto page = SqliteDeliveryJobStore(s).stranded({}, 1);
                    require(page.attempts.size() == 1 &&
                                finance::same(page.attempts[0].key.operation, key.operation) &&
                                page.attempts[0].key.job_ref == key.job_ref &&
                                page.attempts[0].sending_revision == 2,
                            "Bounded stranded scan returns exactly the committed sending attempt");
                });
            };
            lookup(DeliveryJobState::queued, 1);
            if (entry == "unreleased-dispatch" || entry == "released-dispatch")
                dispatch(DeliveryDisposition::refused);
            if (entry == "stranded") {
                release();
                dispatch(DeliveryDisposition::changed);
                scan();
            }
            phase.current = "literal successor application-job advance";
            auto manifest = fixture_manifest();
            const auto head = manifest.steps.back().version;
            MigrationLimits limits;
            limits.schema_objects = manifest.steps.back().expected_schema.size();
            reopen_financial(f, predecessor::migration_profile());
            report(migrate(*f.writer, {MigrationMode::advance, manifest, limits, {}}),
                   CatalogState::current, head, head, 1);
            reopen_financial(f, finance::limits());
            report(migrate(*f.writer, {MigrationMode::inspect, manifest, limits, {}}),
                   CatalogState::current, head, head, 0);
            phase.current = "post-successor protected application delivery entry";
            if (entry == "lookup")
                lookup(DeliveryJobState::queued, 1);
            if (entry == "unreleased-dispatch")
                dispatch(DeliveryDisposition::refused);
            if (entry == "released-dispatch") {
                release();
                dispatch(DeliveryDisposition::changed);
                lookup(DeliveryJobState::sending, 2);
            }
            if (entry == "stranded")
                scan();
        });
    }

    void legacyAndLiveFinancialEntries() {
        reader_check([](ReaderPhase &phase) {
            namespace live = live_financial_refund_fixture;
            const live::OriginalCredit credited{.amount = "10",
                                                .requested = 10,
                                                .main = 10,
                                                .cap_tail = 0,
                                                .score = 100,
                                                .count = 8};
            phase.current = "protected v19 live financial fixture";
            live::Fixture f(credited);
            auto &file = f.file;
            phase.current = "compiled v20 financial migration";
            file.reopen(predecessor::migration_profile());
            report(
                migrate(*file.writer,
                        {MigrationMode::advance, connector_intake_repository_manifest(), {}, {}}),
                CatalogState::current, 20, 20, 1);
            phase.current = "protected v20 financial reopen with owning profile";
            reopen_financial(file, finance::limits());
            phase.current = "protected v20 fund legacy refund";
            f.main("fund-mixed-prefix", 10);
            f.input.config = {live::End::Grace, 19, live::Reuse::Fresh, {}};
            phase.current = "protected v20 retain legacy refund receipt";
            const auto first = f.command("legacy-first", "4", 0);
            live::Effects legacy;
            legacy.apply(-4, 0, {4});
            phase.current = "protected v20 legacy refund and allocation";
            const auto prior = file.accept(
                first,
                live::quote(0, 40, 100, false,
                            live::time(0, {0, 0}, {4, 0}, finance::fraction(4), -4, 0),
                            {live::link(finance::CK::score, 2, 2, 0, 0, 40, finance::fraction(2, 5),
                                        40, false),
                             live::link(finance::CK::whole_count, 3, 3, 0, 0, 0,
                                        finance::fraction(2, 5), finance::fraction(16, 5), false)}),
                legacy);
            constexpr auto by_key =
                "SELECT substr(document,1,8) FROM main.subathon_financial_refunds "
                "WHERE operation_scope=? AND operation_ref=?";
            const auto prefix = [&](const auto &key) {
                return file.rows(by_key, {finance::bytes(key.operation_scope),
                                          finance::bytes(key.operation_ref)});
            };
            require(prefix(first.operation) == SqlRows{{QByteArray::fromHex("0000000000000001")}} &&
                        prior.payment_position == 1 &&
                        prior.application.after.timer.main_remaining_ms == 6,
                    "Protected legacy writer commits the v1 result and exact credited allocation");
            phase.current = "protected v20 restore coherent owner clock";
            f.move("restore-coherent-owner-clock", file.head().timer);

            auto manifest = fixture_manifest();
            const auto target = manifest.steps.back().version;
            MigrationLimits limits;
            limits.schema_objects = manifest.steps.back().expected_schema.size();
            phase.current = "literal successor financial migration";
            reopen_financial(file, predecessor::migration_profile());
            report(migrate(*file.writer, {MigrationMode::advance, manifest, limits, {}}),
                   CatalogState::current, target, target, 1);
            phase.current = "post-successor financial reopen and inspect";
            reopen_financial(file, finance::limits());
            report(migrate(*file.writer, {MigrationMode::inspect, manifest, limits, {}}),
                   CatalogState::current, target, target, 0);
            phase.current = "post-successor selected legacy financial v1 lookup";
            file.write([&](const auto &s) {
                const auto retained = SqliteFinancialRefundStore(s).lookup(first.operation);
                require(retained && retained->payment_position == 1 &&
                            retained->timer == prior.timer &&
                            retained->application.after.timer.main_remaining_ms == 6,
                        "Selected exact v1 stays valid after advancement and reopen");
            });

            phase.current = "post-successor retain live refund receipt";
            const auto next = f.command("live-next", "6", 1);
            const auto entitlement = live::quote(
                40, 100, 100, true, live::time(1, {4, 0}, {10, 0}, finance::fraction(10), -6, 0),
                {live::link(finance::CK::score, 2, 2, 1, 40, 100, 1, 100, true),
                 live::link(finance::CK::whole_count, 3, 3, 1, 0, 8, 1, 8, true)});
            phase.current = "post-successor live financial v2 entry";
            const auto result = live::accepted(f.refund(next, entitlement));
            require(
                result.payment_position == 2 &&
                    result.application.after.timer.grace_remaining_ms == 19 &&
                    result.application.transitions == std::vector{live::Transition::MainExpired,
                                                                  live::Transition::GraceStarted} &&
                    prefix(next.operation) == SqlRows{{QByteArray::fromHex("0000000000000002")}},
                "The #201 live entry validates the mixed prefix and retains real v2 Grace");
            phase.current = "post-successor cold financial replay";
            reopen_financial(file, finance::limits());
            const auto replay = live::accepted(f.refund(next, entitlement), finance::D::existing);
            require(replay.payment_position == 2 &&
                        replay.application.after.token == result.application.after.token &&
                        replay.timer == result.timer,
                    "Authorized exact retry after reopen selects the one committed v2 result");
            phase.current = "post-successor financial history traversal";
            file.write([&](const auto &s) {
                const auto history = SqliteFinancialRefundStore(s).history(file.payment, 2);
                require(history && history->position == 2 && history->accepted_money == 10 &&
                            history->refunds.size() == 2,
                        "Financial traversal retains precisely both adjacent committed records");
            });
        });
    }

    void financialAndOrdinaryHoldClosureAfterAdvance() {
        reader_check([](ReaderPhase &phase) {
            namespace live = live_financial_refund_fixture;
            const live::OriginalCredit credited{.amount = "10",
                                                .requested = 10,
                                                .main = 10,
                                                .cap_tail = 0,
                                                .score = 100,
                                                .count = 8};
            phase.current = "protected v19 financial hold fixture";
            live::Fixture f(credited);
            auto &file = f.file;
            phase.current = "compiled v20 financial hold migration";
            report(
                migrate(*file.writer,
                        {MigrationMode::advance, connector_intake_repository_manifest(), {}, {}}),
                CatalogState::current, 20, 20, 1);
            reopen_financial(file, finance::limits());
            phase.current = "protected v20 real refund and active ordinary/financial holds";
            f.main("active-main-two", 2);
            const auto original = f.command("held-refund", "4", 0);
            const auto entitlement = live::quote(
                0, 40, 100, false, live::time(0, {0, 0}, {4, 0}, finance::fraction(4), -4, 0),
                {live::link(finance::CK::score, 2, 2, 0, 0, 40, finance::fraction(2, 5), 40, false),
                 live::link(finance::CK::whole_count, 3, 3, 0, 0, 0, finance::fraction(2, 5),
                            finance::fraction(16, 5), false)});
            const auto first = live::accepted(f.refund(original, entitlement));
            require(first.timer == RefundTimerOutcome{finance::TD::held, {0, 0, 0, 4, 0}},
                    "The real refund commits score reversal and leaves its own 4-ms time hold");
            const auto hold = live::require_hold(first, live::HoldReason::deficit, 1, -4, 0);
            const auto ordinary = file.detect(file.ordinary_detection("ordinary-mixed", false));
            const auto both = file.continuation("mixed-refused", hold, {ordinary, hold.locator});
            const ApplicationHoldResolutionBatch batch{
                "ordinary-clear",
                {file.cid},
                both.expected_active,
                {{ordinary.hold_ref, ordinary.version_ref, "v2", finance::document(), {}}},
                "tester",
                "ordinary-clear",
                "Independent ordinary acceptance",
                finance::document(),
                file.ordinary_link(false, ordinary)};
            const auto active = [&] {
                const auto kept = file.financial_active();
                require(kept.size() == 1 && finance::same(kept[0].locator, hold.locator),
                        "Selected active financial hold retains its exact original locator");
            };
            active();
            phase.current = "literal successor mixed-hold advance";
            reopen_financial(file, predecessor::migration_profile());
            const auto manifest = fixture_manifest();
            const auto head = manifest.steps.back().version;
            MigrationLimits limits;
            limits.schema_objects = manifest.steps.back().expected_schema.size();
            report(migrate(*file.writer, {MigrationMode::advance, manifest, limits, {}}),
                   CatalogState::current, head, head, 1);
            reopen_financial(file, finance::limits());
            report(migrate(*file.writer, {MigrationMode::inspect, manifest, limits, {}}),
                   CatalogState::current, head, head, 0);
            phase.current = "post-successor active financial hold and mixed-set gate";
            active();
            require(f.resolve(both).disposition == finance::D::active_hold,
                    "An independent ordinary hold prevents financial-only resolution");
            phase.current = "post-successor linked ordinary hold resolution";
            const auto linked = file.run_reply(
                [&](const auto &s) { return stage_financial_aware_linked_resolution(s, batch); });
            require(linked.disposition == finance::D::ordinary_resolved,
                    "Exact linked resolution changes only the ordinary hold");
            active();
            file.write([&](const auto &s) {
                const auto found = SqliteApplicationHoldStore(s).find(ordinary.hold_ref);
                require(found && found->current_version.locator.version_ref == "v2" &&
                            found->current_version.state == ApplicationHoldState::resolved,
                        "Independent ordinary closure retains its exact resolved version");
            });
            phase.current = "post-successor live financial hold closure";
            f.main("fund-after-linked-resolution", 10);
            const auto final = file.continuation("financial-exact", hold, {hold.locator});
            const RefundTimerOutcome expected{finance::TD::applied, {4}};
            const auto saved = live::resolved(f.resolve(final), final, expected);
            require(saved.after.timer.main_remaining_ms == 6 && file.financial_active().empty(),
                    "Credit-backed closure removes four once from the exact funded head");
            phase.current = "post-successor cold selected resolution and immutable refund";
            reopen_financial(file, finance::limits());
            file.write([&](const auto &s) {
                const SqliteFinancialRefundStore refunds(s);
                const auto read = refunds.lookup_resolution(final.operation);
                const auto initial = refunds.lookup(original.operation);
                require(read && read->after.token == saved.after.token &&
                            finance::same(read->held.locator, hold.locator),
                        "Exact selected closure survives a cold read");
                require(initial && initial->timer == first.timer &&
                            initial->payment_position == first.payment_position,
                        "Resolution preserves the immutable held result and payment position");
            });
            const auto replay =
                live::resolved(f.resolve(final), final, expected, finance::D::existing);
            require(replay.after.token == saved.after.token,
                    "Authorized cold replay retains the exact saved resolution and effects");
        });
    }

    void stoppedOccurrenceExcludesNewRefundPolicy_data() {
        QTest::addColumn<bool>("edit_origin");
        QTest::addColumn<bool>("stopped");
        QTest::addColumn<int>("policy");
        for (bool edit : {false, true})
            for (bool stop : {false, true})
                for (int policy = 0; policy != 3; ++policy) {
                    const auto name = QByteArray(edit ? "edit-" : "application-") +
                                      (stop ? "stopped-" : "eligible-") +
                                      std::array{"continue", "review", "skip"}[policy];
                    QTest::newRow(name.constData()) << edit << stop << policy;
                }
        QTest::newRow("application-replace-defer-with-real-refund-review") << false << false << 3;
        QTest::newRow("edit-replace-defer-with-real-refund-review") << true << false << 3;
    }

    void stoppedOccurrenceExcludesNewRefundPolicy() {
        QFETCH(bool, edit_origin);
        QFETCH(bool, stopped);
        QFETCH(int, policy);
        reader_check([&](ReaderPhase &phase) {
            namespace live = live_financial_refund_fixture;
            const live::OriginalCredit credit{.amount = "400",
                                              .requested = 0,
                                              .main = 0,
                                              .cap_tail = 0,
                                              .score = 400,
                                              .count = 8000};
            phase.current = "protected v19 score-only financial owner";
            live::Fixture f(credit, {.main_before = 15, .other_score = 700, .omit_time = true},
                            false);
            auto &file = f.file;
            f.input.selected_track = edit::track;
            phase.current = "compiled v20 stopped-refund fixture migration";
            report(
                migrate(*file.writer,
                        {MigrationMode::advance, connector_intake_repository_manifest(), {}, {}}),
                CatalogState::current, 20, 20, 1);
            reopen_financial(file, finance::limits());
            const MilestoneAction action{MilestoneActionKind::bonus, "award", "v1"};
            const MilestoneOccurrenceKey first = edit::a, second = edit::b;
            constexpr auto deferred_ref = "adjust-decision";
            const MilestoneEditKey edit_key{edit::track, "adjust"};
            const ApplicationOperationKey app_key{"test", "application-A"};
            const Occurrence target{
                edit::track, first,
                edit_origin ? std::variant<ApplicationOperationKey, MilestoneEditKey>{edit_key}
                            : std::variant<ApplicationOperationKey, MilestoneEditKey>{app_key}};
            phase.current = "protected v20 creates two ordered application or edit occurrences";
            file.write([&](const auto &s) {
                require(SqliteAccountSessionUnit(s, {}).insert_installation(
                            {{"install", "epoch"}, "revision"}) == LifecycleWriteResult::staged,
                        "The owning account store establishes the exact command installation");
            });
            if (edit_origin) {
                edit::Authority owner;
                const auto head = file.head();
                owner.facts.lifecycle.timer = head.timer;
                owner.facts.captures = {{"A", first, {action}, {}, {}}, {"B", second, {}, {}, {}}};
                const MilestoneEditIntent intent{
                    edit_key,
                    {edit::actor.actor_ref, deferred_ref},
                    {{"A", 1000}, {"B", 1050}},
                    {{first, MilestoneActionChoice::defer}, {second, MilestoneActionChoice::run}}};
                file.write([&](const auto &s) {
                    require(
                        initialize_milestone_track(s, {edit::track,
                                                       "cfg",
                                                       "initial",
                                                       "prefix",
                                                       {{"A", 1200}, {"B", 1300}},
                                                       std::vector<MilestoneTrackSeed::Member>{}}),
                        "Shipped track owner records exact empty completion history");
                    const auto preview =
                        preview_milestone_target_edit(s, owner, edit::actor, intent);
                    edit::File::confirmable(preview);
                    const auto confirmed = confirm_milestone_target_edit(
                        s, owner, edit::actor, intent, *preview.confirmation);
                    require(confirmed.result && confirmed.result->achievements.size() == 2 &&
                                confirmed.result->achievements[0].order == 1 &&
                                confirmed.result->achievements[1].order == 2,
                            "Real edit commits exact distinct Defer A and ready following B");
                });
            } else {
                std::vector<MilestoneTrackSeed::Member> prefix;
                for (const auto label : {std::string("A"), std::string("B")}) {
                    const bool a = label == "A";
                    const auto &key = a ? first : second;
                    const auto ref = "application-" + label;
                    const auto r = file.intake(ref, {}, "synthetic-milestone-owner");
                    const core::MilestoneCompletion achievement{label, "initial", a ? 1000 : 1050,
                                                                1100, "ctx"};
                    prefix.push_back({achievement, {"test", ref}, key});
                    file.write([&](const auto &s) {
                        auto bundle = finance::File::receipt_application(s, r, ref);
                        bundle.after =
                            file.encoded(std::move(bundle.after), bundle.advanced_before.token);
                        finance::File::retain_plan(s, bundle.intent.plan,
                                                   finance::File::plain_capture(r, {}, {}));
                        require(SqliteApplicationBundleStore(s).stage(bundle).disposition ==
                                    SqliteBundleStageDisposition::staged,
                                "Actual application anchors the occurrence");
                        SqliteMilestoneGroupStore(s).stage(
                            bundle.intent.operation,
                            {{{achievement,
                               a ? std::vector{action} : std::vector<MilestoneAction>{},
                               {},
                               key},
                              {}}});
                        SqliteMilestoneGroupDecisions(s).record_acceptance(
                            bundle.intent.operation, {PausedAcceptanceMode::normal,
                                                      {"operator", deferred_ref},
                                                      {{key, a ? MilestoneActionChoice::defer
                                                               : MilestoneActionChoice::run}}});
                    });
                }
                file.write([&](const auto &s) {
                    require(initialize_milestone_track(s, {edit::track,
                                                           "cfg",
                                                           "initial",
                                                           "prefix",
                                                           {{"A", 1000}, {"B", 1050}},
                                                           prefix}),
                            "Exact application members become the authoritative prefix");
                });
            }
            const auto score = [&] {
                return file.rows(
                    "SELECT sum(b.units) FROM subathon_app_head h "
                    "JOIN subathon_app_balances b ON h.token=b.token WHERE balance_kind=2");
            };
            require(score() == SqlRows{{1100}},
                    "Physical score is exact before the first recovery cut");
            f.move("score-before-earlier-refund", file.head().timer, {}, -80);
            const auto entitlement = [](qint64 prior, qint64 accepted, qint64 position) {
                const auto fraction = finance::fraction(accepted, 400);
                return live::quote(prior * 10, accepted * 10, 4000, false, {},
                                   {live::link(finance::CK::score, 1, 1, position, prior, accepted,
                                               fraction, finance::fraction(accepted), false),
                                    live::link(finance::CK::whole_count, 2, 2, position, 0, 0,
                                               fraction, fraction * 8000, false)});
            };
            const auto advance_financial = [&] {
                phase.current = "literal successor stopped-refund advance and reopen";
                reopen_financial(file, predecessor::migration_profile());
                const auto manifest = fixture_manifest();
                const auto end = manifest.steps.back().version;
                MigrationLimits limits;
                limits.schema_objects = manifest.steps.back().expected_schema.size();
                report(migrate(*file.writer, {MigrationMode::advance, manifest, limits, {}}),
                       CatalogState::current, end, end, 1);
                reopen_financial(file, finance::limits());
                report(migrate(*file.writer, {MigrationMode::inspect, manifest, limits, {}}),
                       CatalogState::current, end, end, 0);
            };
            if (policy == 3) {
                advance_financial();
                const Replacement changed{1, {MilestoneActionKind::bonus, "award", "v2"}};
                const Command cmd{edit::track, "refreshed-bonus", edit::actor,
                                  ReplaceIntent{target, {changed}}};
                Authority authority;
                authority.captures = {{changed, {}, {}, {}, 20000}};
                const auto bind = [&] {
                    std::shared_ptr<const ReplacePreview> preview;
                    file.write([&](const auto &s) {
                        const auto selected = preview_replace(s, authority, cmd);
                        const auto *bound =
                            std::get_if<std::shared_ptr<const ReplacePreview>>(&selected);
                        require(bound && *bound,
                                "STORAGE-027 NI: exact pre-change refund-aware preview");
                        preview = *bound;
                    });
                    return preview;
                };
                phase.current = "post-successor preview under unchanged Defer";
                const auto old_preview = bind();
                file.write([&](const auto &s) {
                    require(SqliteMilestoneGroupDecisions(s).resolve(
                                first, {MilestoneHoldCause::deferred_acceptance, deferred_ref},
                                MilestoneHoldResolution::continue_actions,
                                {"operator", "exact-defer-closed"}),
                            "The prior Defer is independently resolved before the new refund hold");
                });
                phase.current = "post-successor real refund installs replacement Review gate";
                f.input.milestone_policy = MilestoneRefundPolicy::review;
                const auto refund = f.command("swap-refund-review", "25", 0);
                const auto actual = live::accepted(f.refund(refund, entitlement(0, 25, 0)));
                require(score() == SqlRows{{995}} && actual.milestone_observations.size() == 1,
                        "Exact live refund is the owner of the replacement Review gate");
                file.write([&](const auto &s) {
                    const auto active = SqliteMilestoneGroupDecisions(s).active_holds(first);
                    require(active ==
                                    std::vector{MilestoneHoldKey{MilestoneHoldCause::refund_review,
                                                                 "swap-refund-review"}} &&
                                stage_command(s, authority, cmd, old_preview.get()).disposition ==
                                    Disposition::refused,
                            "Changing the held cause stales the unused earlier preview atomically");
                });
                require(file.rows("SELECT count(*) FROM main.subathon_milestone_command_journal") ==
                            SqlRows{{0}},
                        "Stale confirmation reserves no operation key");
                const auto fresh = bind();
                file.write([&](const auto &s) {
                    const auto selected = stage_command(s, authority, cmd, fresh.get());
                    require(selected.disposition == Disposition::committed &&
                                selected.versions.size() == 1 &&
                                SqliteMilestoneGroupDecisions(s).pending_disposition(first) ==
                                    MilestonePendingDisposition::held,
                            "Fresh exact preview selects replacement and leaves the new "
                            "independent Review active");
                });
                return;
            }
            phase.current = "protected v20 real first crossing Review from 1020 to 995";
            require(score() == SqlRows{{1020}}, "First refund begins at retained score 1,020");
            f.input.milestone_policy = MilestoneRefundPolicy::review;
            const auto older = f.command("earlier-refund", "25", 0);
            const auto early = live::accepted(f.refund(older, entitlement(0, 25, 0)));
            require(score() == SqlRows{{995}} && early.milestone_observations.size() == 1 &&
                        early.milestone_observations[0].occurrence == first,
                    "Only first occurrence crosses target 1,000 and gets a real refund Review");
            const MilestoneHoldKey defer{MilestoneHoldCause::deferred_acceptance, deferred_ref};
            const MilestoneHoldKey review{MilestoneHoldCause::refund_review, "earlier-refund"};
            const auto original_holds = [&] {
                file.write([&](const auto &s) {
                    const SqliteMilestoneGroupDecisions gates(s);
                    const auto old = gates.refund_audit(first, "earlier-refund");
                    require(old && old->deficit_micro_points == 5 &&
                                old->current_target_micro_points == 1000 &&
                                gates.active_holds(first) == std::vector{defer, review} &&
                                gates.pending_disposition(second) ==
                                    MilestonePendingDisposition::ready,
                            "Both original causes remain independently active and B stays ready");
                });
            };
            original_holds();

            advance_financial();
            Authority auth;
            if (stopped) {
                phase.current = "post-successor Stop Continue on doubly-held earlier group";
                const auto cmd = stop("stop-before-refund", target, QueueChoice::continue_queue);
                file.write([&](const auto &s) {
                    const auto selected = stage_command(s, auth, cmd);
                    require(
                        selected.disposition == Disposition::committed &&
                            selected.cancellations.size() == 1 &&
                            selected.cancellations[0].ordinal == 1 && selected.added_causes.empty(),
                        "STORAGE-027 NI: stopped unreleased earlier group before second refund");
                });
                reopen_financial(file, finance::limits());
                phase.current = "post-successor cold original Defer and Review readback";
                original_holds();
            }

            phase.current = "post-successor real second refund from 1010 to 990";
            f.move("score-recovered-for-second-crossing", file.head().timer, {}, 15);
            require(score() == SqlRows{{1010}}, "Second refund starts above A and below B");
            f.input.milestone_policy = static_cast<MilestoneRefundPolicy>(policy);
            const auto new_refund = f.command("later-refund", "20", 1);
            const auto later = live::accepted(f.refund(new_refund, entitlement(25, 45, 1)));
            require(score() == SqlRows{{990}} &&
                        later.milestone_observations.size() == (stopped ? 0U : 1U),
                    "Exact credit reversal persists, and only eligible A receives a new decision");
            file.write([&](const auto &s) {
                const SqliteMilestoneGroupDecisions gates(s);
                const auto older_audit = gates.refund_audit(first, "earlier-refund");
                const auto fresh_audit = gates.refund_audit(first, "later-refund");
                require(older_audit && older_audit->deficit_micro_points == 5 &&
                            older_audit->current_target_micro_points == 1000 &&
                            !gates.refund_audit(second, "later-refund"),
                        "Saved earlier deficit and unrelated B remain exact");
                if (stopped) {
                    require(!fresh_audit && gates.active_holds(first) == std::vector{defer, review},
                            "Stopped A receives no decision and both real causes still block");
                } else {
                    const auto expected = f.input.milestone_policy.value();
                    require(fresh_audit && fresh_audit->policy == expected &&
                                fresh_audit->deficit_micro_points == 10 &&
                                fresh_audit->current_target_micro_points == 1000,
                            "Unstopped A records the explicit selected policy and exact deficit");
                    if (expected == MilestoneRefundPolicy::skip) {
                        require(gates.active_holds(first).empty() &&
                                    older_audit->ended_by_skip ==
                                        MilestoneSkipProvenance{MilestoneSkipOrigin::refund_policy,
                                                                "later-refund"},
                                "Actual #236 Skip alone ends prior Review with exact provenance");
                    } else {
                        const auto expected_count =
                            expected == MilestoneRefundPolicy::review ? 3U : 2U;
                        require(gates.active_holds(first).size() == expected_count,
                                "Continue leaves two holds and Review adds exactly one");
                    }
                }
            });
            const auto replay =
                live::accepted(f.refund(older, entitlement(0, 25, 0)), finance::D::existing);
            require(
                replay.payment_position == 1 &&
                    replay.application.after.token == early.application.after.token &&
                    score() == SqlRows{{990}},
                "Exact authorized older retry reads the immutable result with no repeated effect");

            if (!stopped)
                return;
            const auto later_release = [&] {
                return finance::completed<MilestoneReleaseCompletion>([&](auto done) {
                    if (edit_origin)
                        return release_milestone_head(*file.writer, second,
                                                      std::make_shared<ReleaseAuthority>(),
                                                      std::move(done));
                    return release_milestone_head(*file.writer, second, std::move(done));
                });
            };
            const auto blocked = [&] {
                const auto outcome = later_release();
                finance::normal(outcome.work);
                require(outcome.disposition == MilestoneReleaseDisposition::refused,
                        "Later ready occurrence cannot pass a stopped but still-held head");
            };
            const auto resolve = [&](const MilestoneHoldKey &cause, MilestoneHoldResolution how,
                                     const char *evidence) {
                file.write([&](const auto &s) {
                    require(SqliteMilestoneGroupDecisions(s).resolve(first, cause, how,
                                                                     {"operator", evidence}),
                            "Owning #236 primitive resolves the exact named active cause");
                });
            };
            phase.current = "post-successor stopped head gates cold release";
            blocked();
            if (static_cast<MilestoneRefundPolicy>(policy) == MilestoneRefundPolicy::skip) {
                phase.current = "post-successor actual explicit Defer Skip provenance";
                resolve(defer, MilestoneHoldResolution::skip, "explicit-stop-head-skip");
                file.write([&](const auto &s) {
                    const SqliteMilestoneGroupDecisions decisions(s);
                    const auto old = decisions.refund_audit(first, "earlier-refund");
                    require(old &&
                                old->ended_by_skip ==
                                    MilestoneSkipProvenance{
                                        MilestoneSkipOrigin::deferred_resolution, deferred_ref} &&
                                decisions.active_holds(first).empty(),
                            "Only the real explicit Skip ends the earlier Refund Review");
                });
            } else {
                phase.current = "post-successor exact Defer and Review Continue resolutions";
                resolve(defer, MilestoneHoldResolution::continue_actions, "clear-exact-defer");
                blocked();
                resolve(review, MilestoneHoldResolution::continue_actions, "clear-exact-review");
            }
            phase.current = "post-successor passable stopped head and retained refund replay";
            const auto released = later_release();
            finance::normal(released.work);
            require(released.disposition == MilestoneReleaseDisposition::released,
                    "Only the later occurrence releases once the stopped head becomes passable");
            const QString table =
                edit_origin ? "subathon_milestone_edit_occurrences" : "subathon_milestone_groups";
            require(file.rows("SELECT released FROM main." + table + " ORDER BY queue_order") ==
                        SqlRows{{0}, {1}},
                    "Actually skipped or passable A never acquires a release bit");
            const auto again =
                live::accepted(f.refund(older, entitlement(0, 25, 0)), finance::D::existing);
            require(again.application.after.token == early.application.after.token &&
                        score() == SqlRows{{990}},
                    "Authorized older replay remains exact after independent hold resolution");
        });
    }

    void modifierSelectedRevisions() {
        reader_check([](ReaderPhase &phase) {
            namespace m = modifier_activation_fixture;
            predecessor::Populated f;
            install_protected(f);
            f.db.write([](const auto &s) {
                const SqliteModifierActivationStore store(s);
                require(store.append_definition(m::old_definition, {}) ==
                                SqliteModifierDefinitionRevision{m::old_definition, 1} &&
                            store.activate("D", "z-old", {}, {"I", "j0", m::moderator, {}})
                                    .revision_to_append == m::initial_instance &&
                            store.checkpoint_remaining({"I", "j0"}, "j1", 180000).instance ==
                                m::checkpointed_instance &&
                            store.append_definition(m::new_definition, "z-old") ==
                                SqliteModifierDefinitionRevision{m::new_definition, 2},
                        "Protected modifier owner commits real active and predecessor revisions");
            });
            const auto read = [](const auto &s) {
                const SqliteModifierActivationStore store(s);
                require(store.current_definition("D") ==
                                SqliteModifierDefinitionRevision{m::new_definition, 2} &&
                            store.definition_revision("z-old") ==
                                SqliteModifierDefinitionRevision{m::old_definition, 1},
                        "Selected head and retained definition preserve exact captured factors");
                require(
                    store.live_instance("D") ==
                            SqliteModifierInstanceRevision{m::checkpointed_instance,
                                                           2,
                                                           ModifierInstanceOperation::checkpoint,
                                                           {},
                                                           ModifierInstanceLifetime::live,
                                                           {}} &&
                        store.instance_revision("j0") ==
                            SqliteModifierInstanceRevision{m::initial_instance,
                                                           1,
                                                           ModifierInstanceOperation::activation,
                                                           "z-old",
                                                           ModifierInstanceLifetime::live,
                                                           {}},
                    "Selected live and prior instance preserve exact duration and provenance");
            };
            phase.current = "protected v20 selected entry";
            f.db.write(read);
            phase.current = "literal successor advance and reopened inspect";
            advance_fixture(f.db);
            phase.current = "post-successor selected entry";
            f.db.write(read);
        });
    }

    void rateSavedSelection() {
        reader_check([](ReaderPhase &phase) {
            namespace rate = rate_fixture;
            predecessor::Populated f;
            install_protected(f);
            const auto saved = rate::fetched_preview();
            rate::Authority authority;
            f.db.write([&](const auto &s) {
                const auto choice = SqliteRateOverrideUnit(s).save(
                    authority, rate::literal_rate_trust(), saved.edit, saved, saved.basis,
                    [] { return std::optional<std::int64_t>{3}; });
                const auto *receipt = std::get_if<RateReceiptSelection>(&choice);
                require(receipt && receipt->origin == RateReceiptOrigin::newly_staged &&
                            receipt->receipt.successor == "S",
                        "Protected authorized save stages a real fetched rate snapshot");
            });
            const auto read = [&](const auto &s) {
                SqliteRateOverrideUnit unit(s);
                rate::Authority current;
                const auto result =
                    unit.recover(current, rate::literal_rate_trust(), saved.edit, saved);
                const auto *selection = std::get_if<RateReceiptSelection>(&result);
                require(selection && current.reads == 1,
                        "Exact retained selection rechecks authority");
                rate::expect_receipt(*selection, RateReceiptOrigin::retained, saved);
                for (const auto &lookup : {unit.head(), unit.revision("S")}) {
                    const auto *snapshot = std::get_if<core::EffectiveRateSnapshot>(&lookup);
                    require(
                        snapshot &&
                            rate::snapshot_bytes(*snapshot) ==
                                rate::snapshot_bytes(saved.proposal.candidate) &&
                            snapshot->data().components.front().members.back().weight ==
                                rate::ratio(6, 5),
                        "Exact saved head and revision retain value 6/5 and complete provenance");
                }
            };
            phase.current = "protected v20 selected entry";
            f.db.write(read);
            phase.current = "literal successor advance and reopened inspect";
            advance_fixture(f.db);
            phase.current = "post-successor selected entry";
            f.db.write(read);
        });
    }

    void realSavedRateHeadAndNonHead() {
        reader_check([](ReaderPhase &phase) {
            namespace rate = rate_fixture;
            predecessor::Populated f;
            install_protected(f);
            const auto first = rate::first_preview();
            const auto second = rate::replacement_preview();
            phase.current = "protected v20 both real rate saves";
            f.db.write([&](const auto &s) {
                SqliteRateOverrideUnit unit(s);
                rate::Authority authorized;
                const auto save = [&](const RateEditPreview &preview, qint64 now) {
                    const auto next =
                        unit.save(authorized, rate::literal_rate_trust(), preview.edit, preview,
                                  preview.basis, [=] { return std::optional<std::int64_t>{now}; });
                    const auto *staged = std::get_if<RateReceiptSelection>(&next);
                    require(staged && staged->origin == RateReceiptOrigin::newly_staged &&
                                staged->receipt.successor == preview.edit.successor,
                            "Each exact rate revision source commits through owning save");
                };
                save(first, 3);
                save(second, 4);
            });
            const auto select = [&](const auto &s) {
                SqliteRateOverrideUnit unit(s);
                const auto now = unit.head();
                const auto retained = unit.revision("S");
                const auto *head = std::get_if<core::EffectiveRateSnapshot>(&now);
                const auto *previous = std::get_if<core::EffectiveRateSnapshot>(&retained);
                require(head && previous && head->data().snapshot_ref == "T" &&
                            previous->data().snapshot_ref == "S" &&
                            rate::snapshot_bytes(*head) ==
                                rate::snapshot_bytes(second.proposal.candidate) &&
                            rate::snapshot_bytes(*previous) ==
                                rate::snapshot_bytes(first.proposal.candidate),
                        "Stored head T and non-head S preserve distinct exact provenance");
                rate::Authority authority;
                const auto found =
                    unit.recover(authority, rate::literal_rate_trust(), first.edit, first);
                const auto *receipt = std::get_if<RateReceiptSelection>(&found);
                require(receipt && authority.reads == 1,
                        "Prior exact operation is selected under fresh authority");
                rate::expect_receipt(*receipt, RateReceiptOrigin::retained, first);
            };
            phase.current = "protected v20 rate head and retained predecessor";
            f.db.write(select);
            phase.current = "literal successor real rate advance";
            advance_fixture(f.db);
            phase.current = "post-successor real rate head, prior revision and receipt";
            f.db.write(select);
        });
    }

    void earlierCredentialSelectedScope() {
        reader_check([](ReaderPhase &phase) {
            namespace credential = credential_test_support;
            namespace bytes = credential_vectors;
            predecessor::Populated f;
            install_protected(f);
            credential::KeyFixture owned;
            const auto path = owned.file();
            credential::KeyFixture::write(path);
            auto installed = credential::provision_path(path);
            require(std::holds_alternative<CredentialKey>(installed),
                    "Protected credential owner provisions only its disposable synthetic fixture");
            const auto &key = std::get<CredentialKey>(installed);
            phase.current = "protected v20 real credential stage";
            f.db.write([&](const auto &s) {
                const auto staged =
                    CredentialRepository(s, key).stage_put(bytes::exact, bytes::plaintext);
                require(std::holds_alternative<CredentialStaged>(staged),
                        "Owning credential storage stages its selected synthetic value");
            });
            const auto exact_read = [&] {
                std::optional<CredentialLoadResult> loaded;
                const auto reply = qt_test_support::run_work(
                    f.db.writer(),
                    [&](auto context) {
                        const auto s = context.begin();
                        CredentialRepository repo(s, key);
                        loaded = repo.load(bytes::exact);
                        auto unrelated = bytes::exact;
                        unrelated.slot_ref = "different-slot";
                        require(std::holds_alternative<CredentialMissing>(repo.load(unrelated)),
                                "Wrong exact scope never opens a neighboring selected record");
                        s.commit();
                        return 1;
                    },
                    10000);
                finance::normal(reply);
                const auto value = finish_credential_load(std::move(loaded), reply);
                require(value && std::ranges::equal(value->bytes(), bytes::plaintext),
                        "Only the normally acknowledged exact selected load publishes the "
                        "synthetic bytes");
            };
            phase.current = "protected v20 exact credential load";
            exact_read();
            const auto before =
                f.db.rows("SELECT scope,record FROM main.subathon_provider_credentials");
            phase.current = "literal credential successor advance";
            advance_fixture(f.db);
            phase.current = "post-successor exact credential load";
            exact_read();
            require(
                before.size() == 1 &&
                    before ==
                        f.db.rows("SELECT scope,record FROM main.subathon_provider_credentials"),
                "Reopened selection preserves ciphertext and the opaque scoped key byte-for-byte");
        });
    }

    void initialCaptureAfterUpgrade() {
        reader_check([](ReaderPhase &phase) {
            predecessor::Populated f;
            install_protected(f);
            const core::ReceiptInput input{
                {{"synthetic.reference.v1", "A"}, "N", "E"},
                {"synthetic.reference.support.v1", 1, {}, {}, {}, {}, {}, {}},
                {0, "Q", "C", "X"}};
            const auto complete = ReferenceFactsState::complete;
            const ReferenceConfiguration config{
                "C",
                "W",
                input.identity.origin,
                "N",
                predecessor::capture_rule(),
                {},
                {core::TimerEndMode::Grace, 120000, core::GraceReuse::Fresh, {}},
                complete,
                complete,
                complete,
                complete,
                complete};
            f.install("I");
            const ReferenceInitialCapture expected{
                {"I", input.identity}, {1}, {0}, "C", "X", "V", {"P", 1}, "Y"};
            f.db.write([&](const auto &s) {
                const SqliteRetainedContextStore store(s);
                require(
                    store.retain({{RetainedKind::configuration, "C"},
                                  encode_reference_configuration(config).document,
                                  {}}) == RetainDisposition::inserted &&
                        store.retain({{RetainedKind::arrival_context, "X"},
                                      encode_reference_arrival(
                                          {"X", "V", 0, config, {"Q", "E", {}, QByteArray("", 0)}})
                                          .document,
                                      ConfigurationReference{"C"}}) == RetainDisposition::inserted,
                    "Shipped retained owner stores typed configuration and arrival");
            });
            const auto receipt = qt_test_support::run_work(
                f.db.writer(),
                [&](auto ctx) {
                    SqliteReceiptRepository repo(ctx);
                    const auto result = ReceiptIntake(repo).ingest(input);
                    require(result.receipt_id == expected.receipt_id &&
                                result.disposition == core::ReceiptDisposition::created,
                            "Shipped receipt owner commits the first exact synthetic receipt");
                    return 1;
                },
                10000);
            finance::normal(receipt);
            core::ContributionState support;
            support.payment_evidence = core::PaymentEvidence::not_applicable;
            support.conditions.valuation = {false, {}, {}, false};
            support.conditions.review = core::ContributionReview::not_required;
            support.conditions.evidence_admission = {
                core::EvidencePolicy::require_evidence_review, "C", {}};
            support.conditions.settlement = core::SettlementGate::clear;
            const auto evidence = qt_test_support::run_work(
                f.db.writer(),
                [&](auto ctx) {
                    SqliteContributionRepository repo(ctx);
                    const auto linked = ContributionEvidenceService(repo).observe(
                        {{expected.receipt_id, input.identity, input.facts, input.arrival},
                         ContributionObservationKind::support,
                         support.payment_evidence,
                         {},
                         support.conditions});
                    require(linked.disposition == ContributionLinkDisposition::created &&
                                linked.contribution_id == expected.contribution_id,
                            "Shipped evidence owner commits present CID zero");
                    return 1;
                },
                10000);
            finance::normal(evidence);
            const core::CapturedPlanRevision plan{
                1,
                {},
                {{"V",
                  {},
                  "V",
                  expected.receipt_id,
                  input.identity,
                  input.facts,
                  input.arrival,
                  expected.contribution_id},
                 support,
                 {core::PlanValuationState::not_applicable, {}},
                 {{"a", "b", "R", "T", finance::Unit::milliseconds, false,
                   finance::fraction(600000), 600000, "reference.integer-ms.v1",
                   "reference.original-time.v1"}}},
                {{"R", "r", true, "reference.single-rule.match.v1"}},
                {},
                {}};
            f.db.write([&](const auto &s) {
                require(SqliteApplicationPlanStore(s).retain_initial("P", plan) ==
                                SqlitePlanInitialDisposition::inserted &&
                            SqliteRetainedContextStore(s).retain(
                                {{RetainedKind::arrival_context, "Y"},
                                 encode_reference_execution(
                                     {"Y", "X", "I", input.identity, expected.receipt_id,
                                      expected.contribution_id, expected.plan,
                                      predecessor::capture_rule()})
                                     .document,
                                 ConfigurationReference{"C"}}) == RetainDisposition::inserted &&
                            SqliteReferenceCaptureStore(s).retain_initial(expected) ==
                                ReferenceCaptureRetainDisposition::inserted,
                        "Normal plan, retained context and initial capture owners commit complete "
                        "closure");
            });
            const auto read = [&](const auto &s) {
                const auto found = SqliteReferenceCaptureStore(s).find(expected.key);
                require(found && found->key.installation_ref == "I" &&
                            found->key.source == input.identity &&
                            found->receipt_id == core::ReceiptId{1} &&
                            found->contribution_id == core::ContributionId{0} &&
                            found->configuration_ref == "C" && found->arrival_context_ref == "X" &&
                            found->event_ref == "V" && found->plan.revision == 1 &&
                            found->plan.plan_ref == "P" && found->execution_context_ref == "Y",
                        "Normal reader preserves complete selected initial capture");
            };
            phase.current = "protected v20 selected initial capture and plan";
            f.db.write(read);
            const auto media = f.db.rows("SELECT * FROM main.subathon_reference_initial_captures");
            phase.current = "literal successor capture advance and reopen";
            advance_fixture(f.db);
            phase.current = "post-successor normal initial capture reader";
            f.db.write(read);
            require(media.size() == 1 &&
                        f.db.rows("SELECT * FROM main.subathon_reference_initial_captures") ==
                            media,
                    "Initial capture index survives the explicit upgrade and reopen byte for byte");
        });
    }

    void freshReviewPreviewExecuteFinalRead() {
        reader_check([](ReaderPhase &phase) {
            predecessor::Populated f;
            install_protected(f);
            const auto control = f.seed("old-dense", Case::positive());
            const auto wanted = f.seed("new-dense", Case::positive());
            auto old = f.service();
            const auto previous = f.bind("older-committed", {control}, f.preview(old, {control}));
            expected_row(f.reply(f.execute(old, previous)), 1, ReviewDisposition::committed,
                         ReviewStatus::applied);
            const auto before = f.db.rows(
                "SELECT * FROM main.subathon_review_entries ORDER BY work_scope,work_ref,ordinal");
            require(before.size() == 1, "Protected owner provides an independent retained peer");
            phase.current = "literal successor Review input advance";
            advance_fixture(f.db);
            f.trace = {};
            auto live = f.service();
            phase.current = "post-successor fresh Review selected preview";
            const auto preview = f.preview(live, {wanted});
            const auto command = f.bind("newly-selected", {wanted}, preview);
            phase.current = "post-successor fresh Review execute and executor final reconciliation";
            const auto result = f.execute(live, command);
            expected_row(f.reply(result), 1, ReviewDisposition::committed, ReviewStatus::applied);
            const auto final = f.trace.ordinals(ReviewUnit::final_read, ReviewUnitPhase::began);
            require(final == std::vector{0} &&
                        f.db.rows("SELECT count(*) FROM main.subathon_review_entries") ==
                            SqlRows{{2}},
                    "Fresh Review publishes only after its exact executor final read");
            f.trace = {};
            const auto replay = f.execute(live, command);
            expected_row(f.reply(replay), 1, ReviewDisposition::existing, ReviewStatus::applied);
            require(f.trace.ordinals(ReviewUnit::row, ReviewUnitPhase::began).empty() &&
                        f.db.rows("SELECT count(*) FROM main.subathon_review_entries") ==
                            SqlRows{{2}},
                    "Exact retry adds no row or re-execution beside the protected peer");
        });
    }

    void sixteenEntryReviewKeepsTheProtectedProfile() {
        reader_check([](ReaderPhase &phase) {
            predecessor::Populated f;
            install_protected(f);
            std::vector<ReviewEntryIntent> entries;
            for (int i = 16; i > 0; --i)
                entries.push_back(f.seed("bounded-" + std::to_string(i), {}, true, true));
            constexpr StorageLimits review_profile{64, 768, 65536, 8585216, 768, 16105472};
            f.db.reopen(review_profile);
            phase.current = "protected v20 Review preview and bound command";
            auto old = f.service();
            const auto command = f.bind("sixteen", entries, f.preview(old, entries));
            f.trace.control = [](const auto &event) {
                if (event.unit == ReviewUnit::row && event.ordinal == 16 &&
                    event.phase == ReviewUnitPhase::before_commit)
                    throw std::runtime_error("Known sixteenth row rollback");
            };
            phase.current = "protected v20 Review executes the fifteen-row prefix";
            const auto first = f.execute(old, command);
            for (int ordinal = 1; ordinal < 16; ++ordinal)
                expected_row(f.reply(first), ordinal, ReviewDisposition::committed,
                             ReviewStatus::accepted_pending);
            expected_row(f.reply(first), 16, ReviewDisposition::write_failed);
            const auto prefix =
                f.db.rows("SELECT * FROM main.subathon_review_entries ORDER BY ordinal");
            require(prefix.size() == 15 &&
                        f.db.rows("SELECT next_ordinal FROM main.subathon_review_work") ==
                            SqlRows{{16}},
                    "Protected v20 leaves exactly fifteen committed ordinals");
            phase.current = "literal successor advance and reopened inspect";
            advance_fixture(f.db);
            phase.current = "post-successor selected entry";
            phase.current = "post-successor Review ordinal 16 and final read at shipped profile";
            f.db.reopen(review_profile);
            f.trace = {};
            auto review = f.service();
            const auto result = f.execute(review, command);
            for (int ordinal = 1; ordinal < 16; ++ordinal)
                expected_row(f.reply(result), ordinal, ReviewDisposition::existing,
                             ReviewStatus::accepted_pending);
            expected_row(f.reply(result), 16, ReviewDisposition::committed,
                         ReviewStatus::accepted_pending);
            require(
                f.trace.ordinals(ReviewUnit::row, ReviewUnitPhase::began) == std::vector{16} &&
                    f.trace.ordinals(ReviewUnit::final_read, ReviewUnitPhase::began) ==
                        std::vector{0} &&
                    f.db.rows("SELECT next_ordinal FROM main.subathon_review_work") ==
                        SqlRows{{17}} &&
                    f.db.rows("SELECT * FROM main.subathon_review_entries ORDER BY ordinal")
                            .mid(0, 15) == prefix &&
                    f.db.rows("SELECT count(*) FROM main.subathon_app_claims") == SqlRows{{0}},
                "The same 768-row profile admits ordinal 16 and final read with no extra claims");
            f.head(95, 0, 90);
        });
    }

    void reviewCatalogCorruption_data() {
        QTest::addColumn<int>("fault");
        QTest::newRow("protected-prefix-digest") << 0;
        QTest::newRow("successor-digest") << 1;
        QTest::newRow("extra-history") << 2;
        QTest::newRow("review-schema") << 3;
    }

    void reviewCatalogCorruption() {
        QFETCH(int, fault);
        reader_check([&](ReaderPhase &phase) {
            Populated f;
            recover_review_result(f);
            phase.current = "literal successor advance and reopened inspect";
            advance_fixture(f.db);
            phase.current = "post-successor selected entry";
            const auto version = fixture_manifest().steps.back().version;
            if (fault < 2)
                mutate(
                    f.db,
                    "UPDATE main.subathon_migration_history SET content_sha256=? WHERE version=?",
                    {QByteArray(32, 'z'), fault == 0 ? 1 : version});
            else if (fault == 2)
                mutate(f.db, "INSERT INTO main.subathon_migration_history VALUES(?,?,?)",
                       {version + 1, QString("overflow-witness"), QByteArray(32, 'z')});
            else
                mutate(f.db, "DROP INDEX main.subathon_review_entries_cid");
            const auto journal =
                f.db.rows("SELECT * FROM main.subathon_review_entries ORDER BY ordinal");
            const auto history = f.db.rows(history_query);
            const auto schema = f.db.rows(schema_query);
            auto review = f.service();
            const auto result = await<ContributionReview::Result>([&](auto done) {
                review.lookup(f.review_command.key, principal, "synthetic", std::move(done));
            });
            require(
                std::get_if<ReviewRefusal>(&result) &&
                    std::get<ReviewRefusal>(result) == ReviewRefusal::unavailable &&
                    journal ==
                        f.db.rows("SELECT * FROM main.subathon_review_entries ORDER BY ordinal") &&
                    history == f.db.rows(history_query) && schema == f.db.rows(schema_query),
                "Malformed catalog discloses no retained Review and leaves all evidence exact");
        });
    }

    void protectedIntakePageAndAttemptReopen_data() {
        QTest::addColumn<int>("damage");
        QTest::newRow("exact-successor") << 0;
        QTest::newRow("lookalike-successor-marker") << 1;
        QTest::newRow("missing-successor-shape") << 2;
    }

    void protectedIntakePageAndAttemptReopen() {
        QFETCH(int, damage);
        reader_check([&](ReaderPhase &phase) {
            namespace intake = subathon::server::connector_intake;
            predecessor::Populated f;
            install_protected(f);
            f.install();
            predecessor::stage_scope(f.db);
            auto input = predecessor::managed("page-receipt", 8000, "exact-owner-capture");
            input.admission = predecessor::ManagedInput::Admission::audit_only;
            input.history = predecessor::ManagedInput::PageSlot{"O", 1, 1};
            const auto operation = predecessor::import_intent();
            auto page = predecessor::page_admission(1, {predecessor::normalized_item(18)});
            page.binding.operation_revision = 1;
            page.binding.coverage = predecessor::PageBinding::Coverage::unproved;
            page.binding.frontier_after = page.binding.frontier_before;
            page.binding.qualified_proof = {};
            const auto qu = predecessor::QuarantineInput{"install",    "P",    "A",    "S",
                                                         "quarantine", "norm", "proof"};

            phase.current = "protected v20 binds the exact import intent";
            f.db.write([&](const auto &s) {
                require(predecessor::bind_history_operation(s, operation) ==
                            predecessor::Stop::committed,
                        "Protected history writer binds its first exact import operation");
                const auto before = predecessor::bind_page(s, page);
                require(before.stop == predecessor::Stop::committed && before.committed_prefix == 0,
                        "Protected page owner binds one exact item at prefix zero");
            });
            phase.current = "protected v20 adopted receipt commit";
            const auto [accepted, work] = predecessor::manage(f.db, input);
            finance::normal(work);
            require(accepted.stop == predecessor::Stop::committed && accepted.attempt &&
                        accepted.attempt->ordinal == 1,
                    "Real adopted receipt owner commits exactly the first attempt");
            const auto locator = *accepted.attempt;
            phase.current =
                "protected v20 item disposition, committed prefix and page finalization";
            f.db.write([&](const auto &s) {
                require(
                    intake::stage_intake_disposition(s, predecessor::managed_input(input), locator,
                                                     intake::IntakeDisposition::observed_only) ==
                        intake::HistoryStatus::staged,
                    "Audit-only disposition qualifies exactly one committed prefix member");
                const auto bound = predecessor::bind_page(s, page);
                require(bound.stop == predecessor::Stop::exact_replay &&
                            bound.committed_prefix == 1,
                        "Exact bound page read distinguishes allocated from committed prefix");
                require(predecessor::finalize_page(s, page.binding) == predecessor::Stop::committed,
                        "Finalization consumes the exact complete page with no coverage assertion");
            });
            phase.current = "protected v20 quarantine admission";
            f.db.write([&](const auto &s) {
                require(predecessor::admit_quarantine(s, qu) == predecessor::Stop::committed,
                        "Exact synthetic quarantine record commits through its owning path");
            });
            const auto page_rows = [&] {
                return f.db.rows("SELECT page,item_count,committed_prefix,outcome "
                                 "FROM main.subathon_intake_pages ORDER BY ordinal");
            };
            require(page_rows() == SqlRows{{page.binding.page, 1, 1, 1}},
                    "Independent committed page oracle contains exactly one final prefix member");
            const auto select = [&] {
                phase.current = "reopened selected intake attempt and exact page replay";
                f.db.write([&](const auto &s) {
                    const auto attempt = predecessor::read_attempt(s, input, locator);
                    require(attempt.stop == predecessor::Stop::exact_replay &&
                                attempt.attempt == locator &&
                                attempt.selected ==
                                    core::ReceiptAttempt{input.receipt.arrival, input.receipt.facts,
                                                         core::ReceiptDisposition::created},
                            "Exact retained attempt preserves immutable facts and its own arrival");
                    require(
                        predecessor::finalize_page(s, page.binding) ==
                            predecessor::Stop::exact_replay,
                        "Exact final page CAS reads its recorded successor without another write");
                    const auto selected =
                        intake::inspect(s, {{"install", "P", "A", "S"}, qu.key, true, true});
                    require(selected.status == intake::SelectionStatus::selected &&
                                selected.entries.size() == 1 && selected.entries[0].ref == qu.key &&
                                selected.entries[0].document == qu.normalized,
                            "Exact quarantine selection retains normalized bytes and scope");
                });
            };
            f.db.reopen(profile());
            select();
            phase.current = "literal successor intake advance and reopen";
            advance_fixture(f.db);
            select();
            if (damage != 0) {
                phase.current = damage == 1 ? "damaged intake successor marker"
                                            : "damaged intake successor shape";
                if (damage == 1)
                    mutate(
                        f.db,
                        "UPDATE main.subathon_migration_history "
                        "SET migration_id='Milestone-slot-interruption-storage-v1' "
                        "WHERE version=(SELECT max(version) FROM main.subathon_migration_history)");
                else
                    mutate(f.db, "DROP TABLE main.subathon_milestone_playback_evidence");
                const auto before = f.db.rows("SELECT * FROM main.subathon_intake_entries "
                                              "ORDER BY intake_ref");
                const auto stopped = qt_test_support::run_work(
                    f.db.writer(),
                    [&](auto context) -> QVariant {
                        const auto s = context.begin();
                        const auto exact = predecessor::read_attempt(s, input, locator);
                        const auto quarantine =
                            intake::inspect(s, {{"install", "P", "A", "S"}, qu.key, true, true});
                        s.rollback();
                        require(exact.stop != predecessor::Stop::exact_replay && !exact.selected &&
                                    quarantine.status != intake::SelectionStatus::selected &&
                                    quarantine.entries.isEmpty(),
                                "Incompatible catalog must not disclose either retained record");
                        return {};
                    },
                    10000);
                require(
                    stopped.transaction == TransactionOutcome::rolled_back &&
                        (stopped.error == WorkError::not_committed ||
                         stopped.error == WorkError::sql_failure) &&
                        !stopped.value &&
                        before == f.db.rows("SELECT * FROM main.subathon_intake_entries "
                                            "ORDER BY intake_ref"),
                    "Incompatible intake selector refuses without protected disclosure or writes");
                return;
            }
            phase.current = "post-successor exact quarantine resolution and replay";
            f.db.write([&](const auto &s) {
                require(
                    predecessor::resolve_quarantine(s, {qu, "authority"}) ==
                            predecessor::Stop::committed &&
                        predecessor::resolve_quarantine(s, {qu, "authority"}) ==
                            predecessor::Stop::exact_replay,
                    "Exact quarantine resolution releases capacity once and retains its record");
            });
            require(page_rows() == SqlRows{{page.binding.page, 1, 1, 1}} &&
                        f.db.rows("SELECT phase,disposition,typeof(quarantine_bytes) "
                                  "FROM main.subathon_intake_entries WHERE intake_ref=?",
                                  {qu.key}) == SqlRows{{3, 2, QString("null")}},
                    "Quarantine resolution preserves exact page and frees only its owned charge");
        });
    }

    void protectedIntakeLongExactAttempt() {
        reader_check([](ReaderPhase &phase) {
            predecessor::Populated f;
            install_protected(f);
            f.install();
            predecessor::stage_scope(f.db);
            f.db.reopen(finance::limits());
            const auto input = predecessor::managed("long-legacy", 8000, "exact-provenance");
            phase.current = "protected v20 real legacy receipt and long immutable attempt prefix";
            const auto receipt = predecessor::legacy_receipt(f.db, input, 1025);
            auto arrival = input.receipt.arrival;
            arrival.server_timestamp_utc_ms = 9024;
            const auto select = [&] {
                const auto result = predecessor::read_missing(f.db, [&](const auto &s) {
                    return predecessor::read_attempt(s, input, {receipt, 1025});
                });
                require(
                    result.stop == predecessor::Stop::exact_replay &&
                        result.attempt == predecessor::AttemptLocator{receipt, 1025} &&
                        result.selected == core::ReceiptAttempt{arrival, input.receipt.facts,
                                                                core::ReceiptDisposition::retry},
                    "Exact selected ordinal fits the profile and retains its own original facts");
            };
            f.db.reopen(profile());
            phase.current = "protected v20 one exact attempt at 768 rows";
            select();
            phase.current = "literal successor long receipt advance";
            advance_fixture(f.db);
            phase.current = "post-successor one exact attempt at 768 rows";
            select();
        });
    }

    void protectedIntakeAdoptionReopen() {
        reader_check([](ReaderPhase &phase) {
            predecessor::Populated f;
            install_protected(f);
            f.install();
            predecessor::stage_scope(f.db);
            const predecessor::SourceChange adoption{"install",
                                                     "P",
                                                     "A",
                                                     "S",
                                                     1,
                                                     "g3",
                                                     9,
                                                     1,
                                                     "g4",
                                                     "P2",
                                                     "new-only-member",
                                                     "new-cut",
                                                     "adopt",
                                                     "current-decision",
                                                     "history-v2",
                                                     "outside-old-award-envelope",
                                                     predecessor::Purpose::adoption};
            phase.current = "protected v20 commits explicit adoption";
            f.db.write([&](const auto &s) {
                require(predecessor::switch_source_policy(s, adoption) ==
                            predecessor::Stop::committed,
                        "Protected transition executes against exact old source and policy fence");
            });
            const auto oracle = [&] {
                const predecessor::HistoricalHead prior{
                    1, 9, 1, "g3", QByteArray("cut"), QByteArray("F")};
                const predecessor::HistoricalHead now{2, 10, 2, "g4", QByteArray("new-cut"), {}};
                require(predecessor::matches_sql_rows(
                            f.db.rows("SELECT purpose,epoch,policy_revision,generation,intent,"
                                      "outcome,prior_cut,selected_cut "
                                      "FROM main.subathon_intake_operations WHERE operation_ref=?",
                                      {QByteArray("adopt")}),
                            SqlRows{{3, 2, 2, QByteArray("g4"),
                                     predecessor::source_history(prior, now, "history-v2",
                                                                 "current-decision",
                                                                 "outside-old-award-envelope"),
                                     1, QByteArray("cut"), QByteArray("new-cut")}}),
                        "Exact predecessor and selected cuts, whole retained typed intent, and "
                        "the committed adoption disposition survive");
                require(f.db.rows("SELECT epoch,revision,policy_revision,generation,cut "
                                  "FROM main.subathon_intake_sources") ==
                            SqlRows{{2, 10, 2, QByteArray("g4"), QByteArray("new-cut")}},
                        "Exact selected source head agrees with retained adoption evidence");
            };
            oracle();
            phase.current = "literal successor adoption advance and reopen";
            advance_fixture(f.db);
            phase.current = "post-successor adoption evidence and exact source readback";
            oracle();
            f.db.write([&](const auto &s) {
                require(predecessor::switch_source_policy(s, adoption) == predecessor::Stop::stale,
                        "The consumed predecessor fence cannot publish a duplicate adoption");
            });
            oracle();
        });
    }

    void reviewJournalReopen() {
        reader_check([](ReaderPhase &phase) {
            Populated f;
            phase.current = "protected v20 Review journal lookup";
            recover_review_result(f);
            const auto old = f.db.rows("SELECT * FROM main.subathon_review_entries "
                                       "ORDER BY work_scope,work_ref,ordinal");
            phase.current = "literal successor advance and reopened inspect";
            advance_fixture(f.db);
            phase.current = "post-successor selected entry";
            phase.current = "post-successor Review journal lookup";
            recover_review_result(f);
            phase.current = "post-successor exact Review retry";
            auto review = f.service();
            const auto result = f.execute(review, f.review_command);
            const auto &reply = f.reply(result);
            require(reply.rows.size() == 1 && reply.rows[0].outcome &&
                        reply.rows[0].outcome->facts && reply.rows[0].outcome->facts->bundle &&
                        finance::same(*reply.rows[0].outcome->facts->bundle, f.original),
                    "Authorized exact retry selects the protected retained result");
            require(old == f.db.rows("SELECT * FROM main.subathon_review_entries "
                                     "ORDER BY work_scope,work_ref,ordinal"),
                    "Readback and retry leave exact protected journal rows unchanged");
            f.head(99, 7, 105);
        });
    }
};

QTEST_GUILESS_MAIN(Storage027ReadersTests)
#include "readers_tests.moc"
