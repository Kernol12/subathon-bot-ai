// SPDX-License-Identifier: GPL-3.0-or-later
#include "../storage_026/receipt_fixture.hpp"
#include "../storage_026/transition_fixture.hpp"
#include "../support/sqlite_live_financial_refunds_fixture.hpp"
#include "../support/sqlite_modifier_activations_fixture.hpp"
#include "../support/sqlite_rate_override_writer_fixture.hpp"

#include "command_fixture.hpp"
#include <subathon/server/sqlite_delivery_catalog.hpp>

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

    void editAttemptAcrossReopen() {
        reader_check([](ReaderPhase &phase) {
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
