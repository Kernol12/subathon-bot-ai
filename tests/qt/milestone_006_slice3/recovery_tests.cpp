// SPDX-License-Identifier: GPL-3.0-or-later
#include "application_fixture.hpp"

namespace {
using namespace milestone_006_slice3;
using State = DeliveryJobState;
using Release = MilestoneReleaseDisposition;

struct ExactEditRelease final : MilestoneEditReleaseAuthority {
    bool allows(const SqliteWorkSession &, const MilestoneTrackKey &track,
                const MilestoneOccurrenceKey &occurrence) override {
        return track == edit::track && occurrence == edit::b;
    }
};

void release_edit(Populated &f) {
    const auto app = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
        return release_milestone_head(f.db.writer(), f.app_occurrence, std::move(done));
    });
    finance::normal(app.work);
    const auto edit = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
        return release_milestone_head(f.db.writer(), f.edit_occurrence,
                                      std::make_shared<ExactEditRelease>(), std::move(done));
    });
    finance::normal(edit.work);
    require(app.disposition == Release::released && edit.disposition == Release::released,
            "Both normal release entries commit in order");
}

struct StrandedReplacement {
    Occurrence target;
    SelectedVersion version;
    qint64 sending_revision;
};

struct ReplacementRecoveryPage {
    std::vector<StrandedReplacement> attempts;
    std::optional<QByteArray> next_after;
};

class RecoveryMissing final : public std::logic_error {
  public:
    using std::logic_error::logic_error;
};

ReplacementRecoveryPage stranded_replacements(const SqliteWorkSession &session,
                                              const MilestoneTrackKey &track,
                                              const std::optional<QByteArray> &after,
                                              std::size_t limit) {
    auto page = stranded_replacement_attempts(session, track, after, limit);
    ReplacementRecoveryPage result{{}, std::move(page.next_after)};
    for (auto &attempt : page.attempts)
        result.attempts.push_back({std::move(attempt.target), std::move(attempt.version),
                                   attempt.sending_revision});
    return result;
}

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

std::vector<Replacement> patch(bool edit_origin) {
    using Kind = MilestoneActionKind;
    if (edit_origin) {
        const MilestoneAction action{Kind::automation, "announce", "replacement-v2"};
        return {{1, action}, {2, action}, {3, action}};
    }
    const Replacement audio{1, {Kind::automation, "audio", "replacement-v2"}};
    const Replacement chat{3, {Kind::automation, "chat", "replacement-v2"}};
    const Replacement in_flight{4, {Kind::automation, "in-flight", "replacement-v2"}};
    return {audio, chat, in_flight};
}

bool same_origin(const Occurrence &a, const Occurrence &b) {
    if (a.track != b.track || a.occurrence != b.occurrence)
        return false;
    if (const auto *key = std::get_if<ApplicationOperationKey>(&a.origin)) {
        const auto *other = std::get_if<ApplicationOperationKey>(&b.origin);
        return other && finance::same(*key, *other);
    }
    const auto *other = std::get_if<MilestoneEditKey>(&b.origin);
    return other && std::get<MilestoneEditKey>(a.origin) == *other;
}

template <class Db>
void start_exact_attempts(Db &db, const Occurrence &target, const Decision &selected) {
    require(selected.disposition == Disposition::committed && selected.versions.size() == 3,
            "Actual confirmation writes three exact successors");
    const auto &versions = selected.versions;
    require(!versions[0].version.isEmpty() && !versions[1].version.isEmpty() &&
                !versions[2].version.isEmpty() && versions[0].version != versions[1].version &&
                versions[0].version != versions[2].version &&
                versions[1].version != versions[2].version,
            "Every successor has a distinct nonempty key");
    SuccessorAuthority dispatch;
    db.write([&](const auto &s) {
        for (std::size_t i : {0U, 1U})
            require(claim_replacement(s, target, versions[i], 1, dispatch) ==
                        Disposition::committed,
                    "Exact normal claim commits each sending revision");
        require(finish_replacement(s, target, versions[1], 2, DeliveryEvidence::delivered,
                                   "settled-before-recovery") == Disposition::committed,
                "Second successor settles and third remains queued");
    });
    require(dispatch.calls == 2, "Exactly two successors entered dispatch authority");
}

template <class Db>
void assert_stranded_and_settle(Db &db, const Occurrence &target, const Decision &selected) {
    const auto &versions = selected.versions;
    db.write([&](const auto &s) {
        const auto sending = lookup_replacement_attempt(s, target, versions[0], 2);
        const auto settled = lookup_replacement_attempt(s, target, versions[1], 3);
        const auto queued = lookup_replacement_attempt(s, target, versions[2], 1);
        require(sending && sending->state == State::sending && settled &&
                    settled->state == State::delivered && queued && queued->state == State::queued,
                "Normal exact v21 reads prove one stranded, one settled and one queued successor");
        const auto original = stranded_milestone_delivery(s, {}, stranded_delivery_page_limit);
        require(original.attempts.empty(),
                "Protected original-job pager cannot see the stranded successor");
    });
    std::optional<QByteArray> cursor;
    std::vector<StrandedReplacement> found;
    std::vector<QByteArray> visited;
    for (std::size_t page_index = 0; page_index < 4; ++page_index) {
        std::optional<ReplacementRecoveryPage> page;
        db.write([&](const auto &s) { page = stranded_replacements(s, target.track, cursor, 1); });
        require(page && page->attempts.size() <= 1,
                "Each bounded page returns at most one attempt");
        found.insert(found.end(), page->attempts.begin(), page->attempts.end());
        if (!page->next_after) {
            cursor.reset();
            break;
        }
        require(std::ranges::find(visited, *page->next_after) == visited.end() &&
                    std::ranges::any_of(
                        versions, [&](const auto &v) { return v.version == *page->next_after; }),
                "Continuation advances through actual version keys without repeats");
        visited.push_back(*page->next_after);
        cursor = page->next_after;
    }
    require(!cursor && visited.size() == versions.size() && found.size() == 1 &&
                same_origin(found.front().target, target) && found.front().version == versions[0] &&
                found.front().sending_revision == 2,
            "Discovery returns precisely the stopped v21 sending successor and its typed origin");
    const auto recovered = found.front();
    db.write([&](const auto &s) {
        require(finish_replacement(s, recovered.target, recovered.version,
                                   recovered.sending_revision, DeliveryEvidence::indeterminate,
                                   "stranded-outcome") == Disposition::committed,
                "Known exact discovered attempt settles through existing normal entry");
        const auto last = lookup_replacement_attempt(s, target, recovered.version, 3);
        require(last && last->state == State::outcome_unknown &&
                    last->evidence == "stranded-outcome",
                "Recovery adds one outcome-unknown event for the exact stopped attempt");
    });
    db.write([&](const auto &s) {
        const auto after = stranded_replacements(s, target.track, {}, stranded_delivery_page_limit);
        require(after.attempts.empty(), "Settled attempt is no longer discoverable");
    });
}

void assert_catalog_read_exceeds_budget(SqliteWriter &writer, const Occurrence &target,
                                        const SelectedVersion &version) {
    bool entered = false;
    const auto result = qt_test_support::run_work(
        writer,
        [&](auto ctx) -> QVariant {
            const auto session = ctx.begin();
            entered = true;
            // The catalog preflight must return two integer cells. Each costs
            // eight bytes, so an eight-byte work cannot reach version lookup.
            const auto attempt = lookup_replacement_attempt(session, target, version, 2);
            session.commit();
            return attempt.has_value();
        },
        10000);
    require(entered && result.transaction == TransactionOutcome::rolled_back &&
                result.error == WorkError::result_limit && !result.value,
            "Catalog preflight exceeds eight bytes and publishes no work result");
}

} // namespace

class Milestone006RecoveryTests final : public QObject {
    Q_OBJECT

  private slots:
    void strandedReplacementPaging_data() {
        QTest::addColumn<bool>("edit_origin");
        QTest::newRow("application") << false;
        QTest::newRow("edit") << true;
    }

    void strandedReplacementPaging() {
        QFETCH(bool, edit_origin);
        check([&] {
            Authority owner;
            if (edit_origin) {
                Populated f({.edit_slots = 3});
                release_edit(f);
                advance_fixture(f.db);
                const auto target = edited(f);
                const auto cmd = replacement(owner, target, "recover-edit", patch(true));
                const auto preview = bind_replace(f.db, owner, cmd);
                const auto selected =
                    commit(f.db, owner, cmd, Disposition::committed, preview.get());
                start_exact_attempts(f.db, target, selected);
                const auto stopped =
                    commit(f.db, owner, stop("fence-edit", target, QueueChoice::continue_queue),
                           Disposition::committed);
                require(stopped.cancellations == std::vector{selected.versions[2]},
                        "Only the never-started selected version is cancelled");
                f.db.reopen(profile());
                assert_stranded_and_settle(f.db, target, selected);
                return;
            }
            ApplicationFixture f;
            require(f.release(f.a) == Release::released, "Application group starts normally");
            const auto cmd = replacement(owner, f.a, "recover-application", patch(false));
            const auto preview = f.preview(owner, cmd);
            const auto selected = f.command(owner, cmd, preview.get());
            start_exact_attempts(f.file, f.a, selected);
            const auto stopped =
                f.command(owner, stop("fence-app", f.a, QueueChoice::continue_queue));
            require(stopped.disposition == Disposition::committed, "Application Stop commits");
            require(
                stopped.cancellations ==
                    std::vector<SelectedVersion>{{2, {}}, selected.versions[2], {5, {}}, {6, {}}},
                "Stop cancels the selected versions in action order");
            reopen_financial(f.file);
            assert_stranded_and_settle(f.file, f.a, selected);
        });
    }

    void replacementCatalogBudgetRefusesWholeWork() {
        check([] {
            Populated f({.edit_slots = 3});
            release_edit(f);
            advance_fixture(f.db);
            Authority owner;
            const auto target = edited(f);
            const auto cmd = replacement(owner, target, "recover-under-budget", patch(true));
            const auto preview = bind_replace(f.db, owner, cmd);
            const auto selected = commit(f.db, owner, cmd, Disposition::committed, preview.get());
            start_exact_attempts(f.db, target, selected);
            const auto history = [&] {
                return f.db.rows(
                    "SELECT version,revision,state FROM main.subathon_milestone_slot_attempts "
                    "ORDER BY version,revision");
            };
            const auto before = history();
            auto budget = profile();
            budget.result_value_bytes = 8;
            f.db.reopen(budget);
            assert_catalog_read_exceeds_budget(f.db.writer(), target, selected.versions[0]);
            f.db.reopen(profile());
            require(history() == before, "Refused writer work leaves exact attempt history intact");
            f.db.write([&](const auto &s) {
                const auto sending = lookup_replacement_attempt(s, target, selected.versions[0], 2);
                require(
                    sending && sending->revision == 2 && sending->state == State::sending,
                    "The same normal lookup succeeds on the exact version at the ordinary budget");
            });
        });
    }

    void s7EditInitiallySkippedAndFullyHistorical_data() {
        QTest::addColumn<bool>("skipped");
        QTest::newRow("initial-Skip") << true;
        QTest::newRow("fully-historical") << false;
    }

    void s7EditInitiallySkippedAndFullyHistorical() {
        QFETCH(bool, skipped);
        check([&] {
            Populated f({.third_target = true});
            // C is independently created with a terminal or empty Run choice.
            const auto choice = skipped ? MilestoneActionChoice::skip : MilestoneActionChoice::run;
            const auto intent = edit::single("historical-C", {"C", 105}, {{edit::c, choice}});
            f.edit_authority.facts.captures = {edit::empty_c};
            f.db.write([&](const auto &s) {
                const auto preview =
                    preview_milestone_target_edit(s, f.edit_authority, edit::actor, intent);
                edit::File::confirmable(preview);
                const auto confirmed = confirm_milestone_target_edit(
                    s, f.edit_authority, edit::actor, intent, *preview.confirmation);
                require(confirmed.result && confirmed.result->achievements.size() == 1,
                        "Real origin publishes the exact edit C");
            });
            if (skipped) {
                f.db.write([&](const auto &s) {
                    require(SqliteMilestoneGroupDecisions(s).pending_disposition(edit::c) ==
                                MilestonePendingDisposition::skipped,
                            "C is initially skipped by its actual edit choice");
                });
            } else {
                release_edit(f);
                struct ReleaseC final : MilestoneEditReleaseAuthority {
                    bool allows(const SqliteWorkSession &, const MilestoneTrackKey &track,
                                const MilestoneOccurrenceKey &key) override {
                        return track == edit::track && key == edit::c;
                    }
                };
                const auto result = finance::completed<MilestoneReleaseCompletion>([&](auto done) {
                    return release_milestone_head(f.db.writer(), edit::c,
                                                  std::make_shared<ReleaseC>(), std::move(done));
                });
                finance::normal(result.work);
                require(result.disposition == Release::released, "Empty C is wholly historical");
            }
            advance_fixture(f.db);
            Authority owner;
            const Occurrence target{edit::track, edit::c, intent.key};
            const auto before =
                f.db.rows("SELECT occurrence_ref,released FROM "
                          "main.subathon_milestone_edit_occurrences ORDER BY queue_order");
            const auto cmd = stop("refused-edit-C", target);
            (void)commit(f.db, owner, cmd, Disposition::refused);
            require(
                journal(f.db).empty() && causes(f.db).empty() &&
                    cancellations(f.db, cmd.operation).empty() &&
                    f.db.rows("SELECT occurrence_ref,released FROM "
                              "main.subathon_milestone_edit_occurrences ORDER BY queue_order") ==
                        before,
                "Initial Skip or fully historical edit refuses without any Stop writes");
        });
    }
};

QTEST_GUILESS_MAIN(Milestone006RecoveryTests)
#include "recovery_tests.moc"
