// SPDX-License-Identifier: GPL-3.0-or-later
#include "../support/sqlite_live_financial_refunds_fixture.hpp"
#include "../support/sqlite_modifier_activations_fixture.hpp"
#include "../support/sqlite_rate_override_writer_fixture.hpp"
#include "command_fixture.hpp"

namespace {
using namespace storage_027_fixture;
using predecessor::check;
using predecessor::mutate;

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
        check([&] {
            Populated f;
            selected(f.db, f, entry);
            advance_fixture(f.db);
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
        check([&] {
            Populated f;
            selected(f.db, f, pending_queue);
            advance_fixture(f.db);
            const auto authority = std::make_shared<ReleaseAuthority>();
            if (damage == 0) {
                const auto first = release(f, false, authority);
                finance::normal(first.work);
                require(first.disposition == MilestoneReleaseDisposition::released,
                        "Normal application release validates SAR2 and consumes the first head");
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
        check([] {
            Populated f;
            const auto authority = std::make_shared<ReleaseAuthority>();
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
            advance_fixture(f.db);
            f.db.write([&](const auto &s) {
                const auto page = stranded_milestone_delivery(s, {}, 1);
                require(page.attempts.size() == 1 && page.attempts[0].revision == 2 &&
                            std::get_if<MilestoneEditJobKey>(&page.attempts[0].key) &&
                            std::get<MilestoneEditJobKey>(page.attempts[0].key) == f.job,
                        "Stranded traversal after reopen preserves the exact typed attempt");
                const auto done = finish_milestone_edit_job(
                    s, f.job, 2, 2, DeliveryEvidence::known_undelivered, "attempt-2-failed");
                require(done.disposition == DeliveryDisposition::changed && done.job &&
                            done.job->state == DeliveryJobState::failed && done.job->revision == 3,
                        "Matching finish can append exact known-undelivered evidence");
                const auto retried = retry_milestone_edit_job(s, f.job, 3, dispatch);
                require(
                    retried.disposition == DeliveryDisposition::changed && retried.job &&
                        retried.job->state == DeliveryJobState::queued &&
                        retried.job->revision == 4,
                    "Authorized retry preserves old attempt history and adds queued revision 4");
            });
        });
    }

    void legacyAndLiveFinancialEntries() {
        check([] {
            namespace live = live_financial_refund_fixture;
            const live::OriginalCredit credited{.amount = "10",
                                                .requested = 10,
                                                .main = 10,
                                                .cap_tail = 0,
                                                .score = 100,
                                                .count = 8};
            live::Fixture f(credited);
            auto &file = f.file;
            const auto reopen = [&](StorageLimits storage_limits = finance::limits()) {
                auto drained = std::make_shared<bool>(false);
                require(file.writer->shutdown([drained] { *drained = true; }) ==
                                StorageError::none &&
                            QTest::qWaitFor([drained] { return *drained; }, 10000),
                        "Protected financial writer drains before the next reader attaches");
                file.observer.reset();
                file.writer.reset();
                file.open(storage_limits);
            };
            file.reopen(predecessor::migration_profile());
            report(
                migrate(*file.writer,
                        {MigrationMode::advance, connector_intake_repository_manifest(), {}, {}}),
                CatalogState::current, 20, 20, 1);
            f.main("fund-mixed-prefix", 10);
            f.input.config = {live::End::Grace, 19, live::Reuse::Fresh, {}};
            const auto first = f.command("legacy-first", "4", 0);
            live::Effects legacy;
            legacy.apply(-4, 0, {4});
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
            f.move("restore-coherent-owner-clock", file.head().timer);

            auto manifest = fixture_manifest();
            const auto target = manifest.steps.back().version;
            MigrationLimits limits;
            limits.schema_objects = manifest.steps.back().expected_schema.size();
            report(migrate(*file.writer, {MigrationMode::advance, manifest, limits, {}}),
                   CatalogState::current, target, target, 1);
            reopen();
            report(migrate(*file.writer, {MigrationMode::inspect, manifest, limits, {}}),
                   CatalogState::current, target, target, 0);
            file.write([&](const auto &s) {
                const auto retained = SqliteFinancialRefundStore(s).lookup(first.operation);
                require(retained && retained->payment_position == 1 &&
                            retained->timer == prior.timer &&
                            retained->application.after.timer.main_remaining_ms == 6,
                        "Selected exact v1 stays valid after advancement and reopen");
            });

            const auto next = f.command("live-next", "6", 1);
            const auto entitlement = live::quote(
                40, 100, 100, true, live::time(1, {4, 0}, {10, 0}, finance::fraction(10), -6, 0),
                {live::link(finance::CK::score, 2, 2, 1, 40, 100, 1, 100, true),
                 live::link(finance::CK::whole_count, 3, 3, 1, 0, 8, 1, 8, true)});
            const auto result = live::accepted(f.refund(next, entitlement));
            require(
                result.payment_position == 2 &&
                    result.application.after.timer.grace_remaining_ms == 19 &&
                    result.application.transitions == std::vector{live::Transition::MainExpired,
                                                                  live::Transition::GraceStarted} &&
                    prefix(next.operation) == SqlRows{{QByteArray::fromHex("0000000000000002")}},
                "The #201 live entry validates the mixed prefix and retains real v2 Grace");
            reopen();
            const auto replay = live::accepted(f.refund(next, entitlement), finance::D::existing);
            require(replay.payment_position == 2 &&
                        replay.application.after.token == result.application.after.token &&
                        replay.timer == result.timer,
                    "Authorized exact retry after reopen selects the one committed v2 result");
            file.write([&](const auto &s) {
                const auto history = SqliteFinancialRefundStore(s).history(file.payment, 2);
                require(history && history->position == 2 && history->accepted_money == 10 &&
                            history->refunds.size() == 2,
                        "Financial traversal retains precisely both adjacent committed records");
            });
        });
    }

    void modifierSelectedRevisions() {
        check([] {
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
            f.db.write(read);
            advance_fixture(f.db);
            f.db.write(read);
        });
    }

    void rateSavedSelection() {
        check([] {
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
            f.db.write(read);
            advance_fixture(f.db);
            f.db.write(read);
        });
    }

    void sixteenEntryReviewKeepsTheProtectedProfile() {
        check([] {
            predecessor::Populated f;
            install_protected(f);
            std::vector<ReviewEntryIntent> entries;
            for (int i = 16; i > 0; --i)
                entries.push_back(f.seed("bounded-" + std::to_string(i), {}, true, true));
            constexpr StorageLimits review_profile{64, 768, 65536, 8585216, 768, 16105472};
            f.db.reopen(review_profile);
            auto old = f.service();
            const auto command = f.bind("sixteen", entries, f.preview(old, entries));
            f.trace.control = [](const auto &event) {
                if (event.unit == ReviewUnit::row && event.ordinal == 16 &&
                    event.phase == ReviewUnitPhase::before_commit)
                    throw std::runtime_error("Known sixteenth row rollback");
            };
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
            advance_fixture(f.db);
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
        check([&] {
            Populated f;
            recover_review_result(f);
            advance_fixture(f.db);
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

    void reviewJournalReopen() {
        check([] {
            Populated f;
            recover_review_result(f);
            const auto old = f.db.rows("SELECT * FROM main.subathon_review_entries "
                                       "ORDER BY work_scope,work_ref,ordinal");
            advance_fixture(f.db);
            recover_review_result(f);
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
