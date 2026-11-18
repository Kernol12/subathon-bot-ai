// SPDX-License-Identifier: GPL-3.0-or-later
#include <QCoreApplication>
#include <QEvent>
#include <QMetaObject>
#include <QTest>
#include <QThread>

#include <exception>
#include <subathon/server/milestone_timeline.hpp>
#include <utility>

namespace {
using namespace std::chrono_literals;
using namespace subathon::server;
using subathon::core::MilestoneCompletion;
using Starts = std::vector<std::string>;

class ReleaseRecorder final : public MilestoneGroupRelease {
  public:
    [[nodiscard]] bool review_held(const MilestoneCompletion &achievement) const override {
        return held == achievement.milestone_ref;
    }

    void release(CommittedMilestoneGroup group) override {
        starts.push_back(group.achievement.milestone_ref);
        actions.push_back(std::move(group.actions));
        threads.push_back(QThread::currentThread());
    }

    std::optional<std::string> held;
    Starts starts;
    std::vector<std::vector<MilestoneAction>> actions;
    std::vector<QThread *> threads;
};

CommittedMilestoneGroup group(MilestoneCompletion achievement,
                              MilestoneTimingOverride timing = {}) {
    const auto &ref = achievement.milestone_ref;
    std::vector<MilestoneAction> actions{{MilestoneActionKind::automation, "announce-" + ref, "v1"},
                                         {MilestoneActionKind::bonus, "bonus-" + ref, "v2"}};
    return {std::move(achievement), std::move(actions), timing};
}

CommittedMilestoneGroup group(std::string ref, std::int64_t target,
                              MilestoneTimingOverride timing = {}) {
    return group({std::move(ref), "definition-1", target, 400, "apply-1"}, timing);
}

std::optional<ActiveMilestonePresentation> active(std::string ref,
                                                  std::chrono::milliseconds elapsed,
                                                  std::chrono::milliseconds duration = 10s) {
    return ActiveMilestonePresentation{std::move(ref), elapsed, duration};
}

template <typename Scenario> void check(Scenario scenario) {
    try {
        scenario();
    } catch (const MilestoneTimelineNotImplemented &error) {
        // Ordinary Qt failure. Keep subsequent slots runnable on the NI baseline.
        QFAIL(error.what());
    }
}

struct OutputReader {
    void read(const MilestoneTimeline &timeline) { frame = timeline.presentation(); }

    std::optional<ActiveMilestonePresentation> frame;
};
} // namespace

class MilestoneTimelineTests final : public QObject {
    Q_OBJECT
  private slots:
    void committedCrossingUsesExactDefaultTimeline() {
        check([] {
            // The upstream reducer supplies a proposal. This fixture stands in
            // for the owner after normal commit of the proposal's achievements.
            const auto progression =
                subathon::core::advance_milestone_progression({"track",
                                                               "definition-1",
                                                               "apply-1",
                                                               300,
                                                               {{"A", 100}, {"B", 200}, {"C", 300}},
                                                               {}});
            const auto &achieved = progression.data().completed_prefix;
            QCOMPARE(achieved.size(), 3U);
            QCOMPARE(achieved[0].milestone_ref, "A");
            QCOMPARE(achieved[1].milestone_ref, "B");
            QCOMPARE(achieved[2].milestone_ref, "C");

            ReleaseRecorder release;
            MilestoneTimeline timeline({}, release);
            timeline.accept_committed({group(achieved[0]), group(achieved[1]), group(achieved[2])},
                                      0ms);
            timeline.advance(0ms);
            QCOMPARE(timeline.presentation(), active("A", 0ms));
            QCOMPARE(release.starts, (Starts{"A"}));
            using Kind = MilestoneActionKind;
            QCOMPARE(release.actions[0],
                     (std::vector<MilestoneAction>{{Kind::automation, "announce-A", "v1"},
                                                   {Kind::bonus, "bonus-A", "v2"}}));

            timeline.advance(9999ms);
            QCOMPARE(timeline.presentation(), active("A", 9999ms));
            timeline.advance(10000ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(10999ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A"}));

            timeline.advance(11000ms);
            timeline.advance(11000ms);
            QCOMPARE(timeline.presentation(), active("B", 0ms));
            QCOMPARE(release.starts, (Starts{"A", "B"}));
            QCOMPARE(release.actions[1],
                     (std::vector<MilestoneAction>{{Kind::automation, "announce-B", "v1"},
                                                   {Kind::bonus, "bonus-B", "v2"}}));

            timeline.advance(21000ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(21999ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A", "B"}));
            timeline.advance(22000ms);
            QCOMPARE(timeline.presentation(), active("C", 0ms));
            QCOMPARE(release.starts, (Starts{"A", "B", "C"}));
            QCOMPARE(release.actions[2],
                     (std::vector<MilestoneAction>{{Kind::automation, "announce-C", "v1"},
                                                   {Kind::bonus, "bonus-C", "v2"}}));
            timeline.advance(32000ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(33000ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A", "B", "C"}));
        });
    }

    void globalAndPerMilestoneTimingResolveEachField() {
        check([] {
            ReleaseRecorder release;
            MilestoneTimeline timeline({.presentation = 4s, .gap = 700ms}, release);
            timeline.accept_committed(
                {group("A", 100), group("B", 200, {.presentation = 2s, .gap = {}}),
                 group("C", 300, {.presentation = {}, .gap = 250ms}), group("D", 400)},
                0ms);
            timeline.advance(0ms);
            QCOMPARE(timeline.presentation(), active("A", 0ms, 4s));
            timeline.advance(4000ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(4699ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A"}));

            timeline.advance(4700ms);
            QCOMPARE(timeline.presentation(), active("B", 0ms, 2s));
            timeline.advance(6700ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(7399ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A", "B"}));

            timeline.advance(7400ms);
            QCOMPARE(timeline.presentation(), active("C", 0ms, 4s));
            timeline.advance(11400ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(11649ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A", "B", "C"}));

            timeline.advance(11650ms);
            QCOMPARE(timeline.presentation(), active("D", 0ms, 4s));
            QCOMPARE(release.starts, (Starts{"A", "B", "C", "D"}));
        });
    }

    void queuePauseIsIndependentByDefault() {
        check([] {
            ReleaseRecorder release;
            MilestoneTimeline timeline({}, release);
            timeline.accept_committed({group("A", 100), group("B", 200)}, 0ms);
            timeline.advance(0ms);
            timeline.set_countdown_paused(true, 1000ms);
            timeline.advance(3000ms);
            QCOMPARE(timeline.presentation(), active("A", 3000ms));

            timeline.set_queue_paused(true, 3000ms);
            timeline.advance(9999ms);
            QCOMPARE(timeline.presentation(), active("A", 9999ms));
            timeline.advance(10000ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A"}));

            timeline.set_queue_paused(false, 10400ms);
            timeline.advance(10999ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A"}));
            timeline.advance(11000ms);
            QCOMPARE(timeline.presentation(), active("B", 0ms));
            QCOMPARE(release.starts, (Starts{"A", "B"}));
        });
    }

    void queuePauseLetsGapElapseBeforeResume() {
        check([] {
            ReleaseRecorder release;
            MilestoneTimeline timeline({}, release);
            timeline.accept_committed({group("A", 100), group("B", 200)}, 0ms);
            timeline.advance(0ms);
            timeline.advance(10000ms);
            QVERIFY(!timeline.presentation());
            timeline.set_queue_paused(true, 10400ms);
            timeline.advance(11000ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A"}));
            timeline.advance(19999ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A"}));

            timeline.set_queue_paused(false, 20000ms);
            timeline.advance(20000ms);
            QCOMPARE(timeline.presentation(), active("B", 0ms));
            QCOMPARE(release.starts, (Starts{"A", "B"}));
        });
    }

    void countdownLinkKeepsManualAndCountdownPausesIndependent() {
        check([] {
            ReleaseRecorder release;
            MilestoneTimeline timeline({.follow_countdown_pause = true}, release);
            timeline.accept_committed({group("A", 100), group("B", 200), group("C", 300)}, 0ms);
            timeline.advance(0ms);
            timeline.set_countdown_paused(true, 2000ms);
            timeline.advance(4000ms);
            QCOMPARE(timeline.presentation(), active("A", 4000ms));

            timeline.set_queue_paused(true, 4000ms);
            timeline.set_queue_paused(false, 5000ms);
            timeline.advance(9999ms);
            QCOMPARE(timeline.presentation(), active("A", 9999ms));
            timeline.advance(10000ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(11000ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A"}));

            timeline.set_countdown_paused(false, 12500ms);
            timeline.advance(12500ms);
            QCOMPARE(timeline.presentation(), active("B", 0ms));
            QCOMPARE(release.starts, (Starts{"A", "B"}));

            timeline.set_countdown_paused(true, 13500ms);
            timeline.set_queue_paused(true, 14500ms);
            timeline.set_countdown_paused(false, 15500ms);
            timeline.advance(22499ms);
            QCOMPARE(timeline.presentation(), active("B", 9999ms));
            timeline.advance(22500ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(23500ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A", "B"}));

            timeline.set_queue_paused(false, 25000ms);
            timeline.advance(25000ms);
            QCOMPARE(timeline.presentation(), active("C", 0ms));
            QCOMPARE(release.starts, (Starts{"A", "B", "C"}));
        });
    }

    void existingReviewHoldBlocksLaterGroupsAcrossQueueResume() {
        check([] {
            ReleaseRecorder release;
            MilestoneTimeline timeline({}, release);
            timeline.accept_committed({group("A", 100), group("B", 200), group("C", 300)}, 0ms);
            timeline.advance(0ms);
            QCOMPARE(timeline.presentation(), active("A", 0ms));
            QCOMPARE(release.starts, (Starts{"A"}));
            release.held = "B";
            timeline.advance(10000ms);
            timeline.advance(11000ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A"}));

            timeline.set_queue_paused(true, 12000ms);
            timeline.set_queue_paused(false, 14000ms);
            timeline.advance(50000ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A"}));
        });
    }

    void serverTurnsAdvanceWithoutReadersAndReconnectReadsCurrentPosition() {
        check([] {
            QObject owner;
            ReleaseRecorder release;
            MilestoneTimeline timeline({}, release);
            timeline.accept_committed({group("A", 100), group("B", 200)}, 0ms);
            timeline.advance(0ms);
            timeline.advance(2500ms);

            std::optional<OutputReader> desktop{std::in_place};
            std::optional<OutputReader> browser{std::in_place};
            desktop->read(timeline);
            browser->read(timeline);
            QCOMPARE(desktop->frame, active("A", 2500ms));
            QCOMPARE(browser->frame, desktop->frame);
            const auto last_desktop_frame = desktop->frame;
            desktop.reset();
            browser.reset();

            // Run the same owner turns with both outputs absent. Capture NI
            // inside the event dispatch so it never unwinds through Qt.
            std::exception_ptr turn_error;
            bool ran = false;
            QVERIFY(QMetaObject::invokeMethod(
                &owner,
                [&] {
                    ran = true;
                    try {
                        timeline.advance(10000ms);
                        timeline.advance(11000ms);
                        timeline.advance(13500ms);
                    } catch (...) {
                        turn_error = std::current_exception();
                    }
                },
                Qt::QueuedConnection));
            QCoreApplication::sendPostedEvents(&owner, QEvent::MetaCall);
            QVERIFY(ran);
            if (turn_error)
                std::rethrow_exception(turn_error);
            QCOMPARE(release.starts, (Starts{"A", "B"}));
            QCOMPARE(release.threads, (std::vector<QThread *>{owner.thread(), owner.thread()}));

            desktop.emplace().read(timeline);
            browser.emplace().read(timeline);
            QCOMPARE(desktop->frame, active("B", 2500ms));
            QCOMPARE(browser->frame, desktop->frame);
            QCOMPARE(last_desktop_frame, active("A", 2500ms));
            QCOMPARE(release.starts, (Starts{"A", "B"}));

            timeline.advance(21000ms);
            desktop->read(timeline);
            browser->read(timeline);
            QVERIFY(!desktop->frame);
            QVERIFY(!browser->frame);
            QCOMPARE(release.starts, (Starts{"A", "B"}));
        });
    }

    void laterCommittedBatchAppendsWithoutChangingExistingTiming() {
        check([] {
            ReleaseRecorder release;
            MilestoneTimeline timeline({}, release);
            timeline.accept_committed({group({"A", "definition-1", 100, 200, "apply-1"}),
                                       group({"B", "definition-1", 200, 200, "apply-1"},
                                             {.presentation = 3s, .gap = 400ms})},
                                      0ms);
            timeline.advance(0ms);
            QCOMPARE(timeline.presentation(), active("A", 0ms));
            QCOMPARE(release.starts, (Starts{"A"}));

            timeline.accept_committed({group({"C", "definition-1", 300, 400, "apply-2"}),
                                       group({"D", "definition-1", 400, 400, "apply-2"})},
                                      3500ms);
            timeline.advance(3500ms);
            QCOMPARE(timeline.presentation(), active("A", 3500ms));
            QCOMPARE(release.starts, (Starts{"A"}));
            timeline.advance(9999ms);
            QCOMPARE(timeline.presentation(), active("A", 9999ms));
            timeline.advance(10000ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(10999ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A"}));

            timeline.advance(11000ms);
            QCOMPARE(timeline.presentation(), active("B", 0ms, 3s));
            QCOMPARE(release.starts, (Starts{"A", "B"}));
            timeline.advance(13999ms);
            QCOMPARE(timeline.presentation(), active("B", 2999ms, 3s));
            timeline.advance(14000ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(14399ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A", "B"}));

            timeline.advance(14400ms);
            QCOMPARE(timeline.presentation(), active("C", 0ms));
            QCOMPARE(release.starts, (Starts{"A", "B", "C"}));
            timeline.advance(24400ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(25399ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A", "B", "C"}));
            timeline.advance(25400ms);
            QCOMPARE(timeline.presentation(), active("D", 0ms));
            QCOMPARE(release.starts, (Starts{"A", "B", "C", "D"}));
        });
    }

    void laterBatchStartsAtSubmissionAfterQueueHasDrained() {
        check([] {
            ReleaseRecorder release;
            MilestoneTimeline timeline({}, release);
            timeline.accept_committed({group({"A", "definition-1", 100, 100, "apply-1"})}, 0ms);
            timeline.advance(0ms);
            QCOMPARE(timeline.presentation(), active("A", 0ms));
            timeline.advance(10000ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(11000ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(39999ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A"}));

            timeline.accept_committed({group({"B", "definition-1", 200, 300, "apply-2"}),
                                       group({"C", "definition-1", 300, 300, "apply-2"})},
                                      40000ms);
            timeline.advance(40000ms);
            QCOMPARE(timeline.presentation(), active("B", 0ms));
            QCOMPARE(release.starts, (Starts{"A", "B"}));
            timeline.advance(49999ms);
            QCOMPARE(timeline.presentation(), active("B", 9999ms));
            timeline.advance(50000ms);
            QVERIFY(!timeline.presentation());
            timeline.advance(50999ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(release.starts, (Starts{"A", "B"}));
            timeline.advance(51000ms);
            QCOMPARE(timeline.presentation(), active("C", 0ms));
            QCOMPARE(release.starts, (Starts{"A", "B", "C"}));
        });
    }
};

QTEST_GUILESS_MAIN(MilestoneTimelineTests)
#include "milestone_timeline_tests.moc"
