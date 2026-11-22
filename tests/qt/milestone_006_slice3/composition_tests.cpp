// SPDX-License-Identifier: GPL-3.0-or-later
#include "application_fixture.hpp"

#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace {
using namespace milestone_006_slice3;
using namespace subathon::server::testing;
using namespace std::chrono_literals;
using Sample = std::chrono::milliseconds;

class OwnerEntryMissing final : public std::logic_error {
  public:
    using std::logic_error::logic_error;
};

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

// Both completions come from the real writer. A staged Decision is never fed
// to the owner. A replay from a readback may be the first acknowledged Stop.
enum class StopCompletionSource { command, readback };
void adopt_stop_completion(MilestoneTimeline &, Sample, const Command &, StopCompletionSource,
                           const SqliteWorkResult &, const std::optional<Decision> &) {
    throw OwnerEntryMissing(
        "T1/T2: adopt the exact acknowledged writer Stop result on the warm timeline");
}

Preview preview_replacement_owner(const SqliteWorkSession &, CommandAuthority &, const Command &) {
    throw OwnerEntryMissing(
        "R7: preview the exact authorized application replacement at the owner entry");
}

struct Completion {
    SqliteWorkResult work;
    std::optional<Decision> published;
};

template <class OwnerCall>
Completion complete(SqliteWriter &writer, OwnerCall call, bool rollback = false) {
    std::optional<Decision> provisional;
    auto work = qt_test_support::run_work(
        writer,
        [&](auto ctx) -> QVariant {
            const auto session = ctx.begin();
            provisional = call(session);
            if (rollback)
                session.rollback();
            else
                session.commit();
            return 1;
        },
        10000);
    require(provisional.has_value(), "Real operation executed inside its writer session");
    std::optional<Decision> published;
    if (work.transaction == TransactionOutcome::committed && work.error == WorkError::none &&
        work.value)
        published = *provisional;
    return {work, std::move(published)};
}

struct ActualReleasePort final : MilestoneGroupRelease {
    ApplicationFixture &fixture;
    std::vector<MilestoneOccurrenceKey> releases;
    explicit ActualReleasePort(ApplicationFixture &f) : fixture(f) {}

    bool review_held(const subathon::core::MilestoneCompletion &) const override {
        throw std::logic_error("Actual warm timeline requires exact identities");
    }

    std::optional<MilestoneHeadDisposition>
    occurrence_disposition(const MilestoneOccurrenceKey &key) const override {
        if (key == first || key == later)
            return MilestoneHeadDisposition::ready;
        return {};
    }

    void release(CommittedMilestoneGroup group) override {
        require(group.occurrence == first || group.occurrence == later,
                "Timeline releases an exact application origin");
        const auto &target = group.occurrence == first ? fixture.a : fixture.b;
        require(fixture.release(target) == MilestoneReleaseDisposition::released,
                "Normal release acknowledgement precedes presentation");
        releases.push_back(target.occurrence);
    }
};

std::vector<CommittedMilestoneGroup> pending(ApplicationFixture &f) {
    std::vector<CommittedMilestoneGroup> result;
    f.file.write([&](const auto &s) {
        for (const auto &row : SqliteMilestoneGroupStore(s).pending())
            result.push_back(row.group);
    });
    require(result.size() == 2 && result[0].occurrence == first && result[1].occurrence == later,
            "Exact durable pending order is A before B");
    return result;
}

std::optional<ActiveMilestonePresentation> window(std::string ref, Sample elapsed) {
    return ActiveMilestonePresentation{std::move(ref), elapsed, 10s};
}

} // namespace

class Milestone006CompositionTests final : public QObject {
    Q_OBJECT

  private slots:
    void t1T2WarmWriterCompletion_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("known-rollback") << 0;
        QTest::newRow("lost-ack-readback-and-replay") << 1;
        QTest::newRow("lost-ack-fences-expired-gap") << 2;
    }

    void t1T2WarmWriterCompletion() {
        QFETCH(int, mode);
        const bool rollback = mode == 0;
        check([&] {
            ApplicationFixture f(false);
            ActualReleasePort port(f);
            MilestoneTimeline timeline({10s, 1s, false}, port);
            timeline.accept_committed(pending(f), 0ms);
            require(timeline.presentation() == window("A", 0ms) &&
                        port.releases == std::vector{first},
                    "Exact A starts on a normal durable release at zero");
            timeline.advance(1800ms);
            require(timeline.presentation() == window("A", 1800ms), "Request sample never stops A");
            Authority owner;
            owner.presentation = PresentationObservation{f.a,
                                                         "presentation-A",
                                                         "generation-A",
                                                         "generation-A",
                                                         "revision-A",
                                                         0,
                                                         10000,
                                                         2000,
                                                         "revision-A",
                                                         1800};
            const auto cmd = stop("warm-stop-A", f.a, QueueChoice::continue_queue);
            if (!rollback)
                require(SqliteWriterWorkAccess::inject_next_control_ack_loss(
                            *f.file.writer, WorkControlFault::lose_next_commit_ack) ==
                            StorageError::none,
                        "Select loss of real Stop COMMIT acknowledgement");
            const auto attempt = complete(
                *f.file.writer,
                [&](const auto &s) {
                    const auto staged = stage_command(s, owner, cmd);
                    require(staged.disposition == Disposition::committed &&
                                staged.cancellations == std::vector<SelectedVersion>{{1, {}}},
                            "The exact active Stop reached the writer transaction");
                    return staged;
                },
                rollback);
            require(!attempt.published && !attempt.work.value &&
                        attempt.work.transaction == (rollback
                                                         ? TransactionOutcome::rolled_back
                                                         : TransactionOutcome::outcome_unknown) &&
                        attempt.work.error ==
                            (rollback ? WorkError::not_committed : WorkError::uncertain_outcome),
                    "No owner success comes from rollback or lost acknowledgement");
            adopt_stop_completion(timeline, 2000ms, cmd, StopCompletionSource::command,
                                  attempt.work, attempt.published);
            require(timeline.presentation() == window("A", 2000ms) &&
                        port.releases == std::vector{first},
                    "The sampled timeline advances without ending A on a nonacknowledged result");
            if (rollback) {
                require(f.file.rows("SELECT operation FROM main.subathon_milestone_command_journal")
                            .empty(),
                        "Rollback retains no Stop");
                timeline.advance(10999ms);
                require(!timeline.presentation() && port.releases == std::vector{first},
                        "Natural gap is intact until 11,000");
                timeline.advance(11000ms);
                require(timeline.presentation() == window("B", 0ms) &&
                            port.releases == std::vector{first, later},
                        "Known rollback leaves the natural boundary intact");
                return;
            }

            reopen_financial(f.file);
            const auto acknowledge = [&](Sample now, StopCompletionSource source) {
                owner.presentation->observed_ms = now.count();
                const auto completion = complete(*f.file.writer, [&](const auto &s) {
                    return source == StopCompletionSource::readback ? read_command(s, owner, cmd)
                                                                    : stage_command(s, owner, cmd);
                });
                finance::normal(completion.work);
                require(completion.published &&
                            completion.published->disposition == Disposition::replay &&
                            completion.published->cancellations ==
                                std::vector<SelectedVersion>{{1, {}}},
                        "Real exact readback and authorized replay retain the same Stop");
                adopt_stop_completion(timeline, now, cmd, source, completion.work,
                                      completion.published);
            };
            if (mode == 2) {
                timeline.advance(11000ms);
                require(!timeline.presentation() && port.releases == std::vector{first},
                        "Actually committed but unresolved Stop fences an otherwise eligible B");
                acknowledge(11500ms, StopCompletionSource::readback);
                require(!timeline.presentation() && port.releases == std::vector{first},
                        "Late readback applies no invented gap or immediate release");
                timeline.advance(11500ms);
                require(
                    timeline.presentation() == window("B", 0ms) &&
                        port.releases == std::vector{first, later},
                    "First advance after late readback starts B at the elapsed natural boundary");
                return;
            }
            acknowledge(2600ms, StopCompletionSource::readback);
            require(!timeline.presentation() && port.releases == std::vector{first},
                    "First acknowledgement interrupts A on the warm 2,600 sample");
            acknowledge(3100ms, StopCompletionSource::command);
            timeline.advance(3599ms);
            require(!timeline.presentation() && port.releases == std::vector{first},
                    "The original interruption's 3,600 boundary still holds");
            timeline.advance(3600ms);
            require(timeline.presentation() == window("B", 0ms) &&
                        port.releases == std::vector{first, later},
                    "Replay neither restarts the gap nor starts another presentation");
            acknowledge(3700ms, StopCompletionSource::command);
            require(timeline.presentation() == window("B", 100ms) &&
                        port.releases == std::vector{first, later},
                    "Exact old Stop replay cannot interrupt currently presenting B");
        });
    }

    void r7PositiveOwnerPreviewIsConfirmable() {
        check([] {
            ApplicationFixture f(false);
            Authority owner;
            const Replacement patch{1, {MilestoneActionKind::automation, "audio", "v2"}};
            const auto cmd = replacement(owner, f.a, "positive-owner-preview", {patch});
            require(bool(f.preview(owner, cmd)), "Positive protected preview proves normal setup");
            std::shared_ptr<const ReplacePreview> bound;
            try {
                f.file.write([&](const auto &s) {
                    const auto result = preview_replacement_owner(s, owner, cmd);
                    const auto *preview =
                        std::get_if<std::shared_ptr<const ReplacePreview>>(&result);
                    require(preview && *preview, "Authorized owner returns an opaque real preview");
                    bound = *preview;
                });
            } catch (const OwnerEntryMissing &) {
                throw;
            }
            require(f.file.rows("SELECT operation FROM main.subathon_milestone_command_journal")
                        .empty(),
                    "Owner preview does not reserve a durable command");
            const auto confirmed = f.command(owner, cmd, bound.get());
            require(confirmed.disposition == Disposition::committed &&
                        confirmed.versions.size() == 1 && confirmed.versions.front().ordinal == 1 &&
                        !confirmed.versions.front().version.isEmpty(),
                    "Actual confirmation accepts the exact owner-produced preview");
        });
    }
};

QTEST_GUILESS_MAIN(Milestone006CompositionTests)
#include "composition_tests.moc"
