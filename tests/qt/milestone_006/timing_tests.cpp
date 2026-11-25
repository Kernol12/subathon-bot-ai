// SPDX-License-Identifier: GPL-3.0-or-later
#include "timeline_boundary.hpp"

#include <QTest>

#include <algorithm>

namespace {
using namespace std::chrono_literals;
using namespace milestone_006_stage1;
using Sample = std::chrono::milliseconds;
using Releases = std::vector<MilestoneOccurrenceKey>;

const MilestoneTrackKey track{"synthetic-install", "timeline-track"};
const Occurrence a{track, {"occurrence-A"}, ApplicationOperationKey{"scope", "application-A"}};
const Occurrence b{track, {"occurrence-B"}, MilestoneEditKey{track, "edit-B"}};
const Occurrence c{track, {"occurrence-C"}, ApplicationOperationKey{"scope", "application-C"}};
const ScopedCause h1{track, "interruption-hold-1"};
const ScopedCause h2{track, "interruption-hold-2"};
const MilestoneHoldKey defer{MilestoneHoldCause::deferred_acceptance, "defer-B"};
const MilestoneHoldKey review{MilestoneHoldCause::refund_review, "refund-B"};

CommittedMilestoneGroup group(const Occurrence &exact, std::string ref, int threshold,
                              MilestoneTimingOverride timing = {}) {
    std::optional<MilestoneEditMembership> edit;
    if (const auto *origin = std::get_if<MilestoneEditKey>(&exact.origin))
        edit = MilestoneEditMembership{*origin, {{*origin, exact.occurrence, 1}}};
    return {{std::move(ref), "definition", threshold, 300, "achievement-context"},
            {{MilestoneActionKind::automation, "same-action", "same-version"}},
            timing,
            exact.occurrence,
            edit};
}

class ReleasePort final : public MilestoneGroupRelease {
  public:
    bool review_held(const subathon::core::MilestoneCompletion &) const override {
        throw std::logic_error("A timing fixture must select an exact occurrence");
    }

    std::optional<MilestoneHeadDisposition>
    occurrence_disposition(const MilestoneOccurrenceKey &exact) const override {
        if (exact == b.occurrence) {
            if (real_skip)
                return MilestoneHeadDisposition::skipped;
            if (!b_holds.empty())
                return MilestoneHeadDisposition::held;
        }
        return MilestoneHeadDisposition::ready;
    }

    void release(CommittedMilestoneGroup value) override {
        if (!value.occurrence)
            throw std::logic_error("An exact release must carry its occurrence");
        if (*value.occurrence == b.occurrence &&
            (!value.edit_membership ||
             value.edit_membership->origin != std::get<MilestoneEditKey>(b.origin)))
            throw std::logic_error("B must retain its exact edit origin on release");
        releases.push_back(*value.occurrence);
    }

    // Supplied #236 dispositions only. Stop and interruption Holds must be
    // applied at the timeline seam, never supplied as #236 Skip/manual pause.
    std::vector<MilestoneHoldKey> b_holds;
    bool real_skip{false};
    Releases releases;
};

struct TimelineFixture {
    ReleasePort release;
    // An explicit captured gap for A differs from the default.
    MilestoneTimeline timeline{{10s, 1700ms, false}, release};

    void start(Sample gap = 1s, bool same_milestone = false) {
        timeline.accept_committed({group(a, "A", 100, {10s, gap}),
                                   group(b, same_milestone ? "A" : "B", 200), group(c, "C", 300)},
                                  0ms);
    }

    void pending() {
        timeline.restore_committed({group(a, "A", 100, {10s, 1s}), group(b, "B", 200)});
    }
};

std::optional<ActiveMilestonePresentation> window(std::string ref, Sample elapsed) {
    return ActiveMilestonePresentation{std::move(ref), elapsed, 10s};
}

PresentationObservation active_a(Sample sample) {
    // Supplied exact owner evidence for A's one live generation. Neither
    // the release log nor an output/adapter acknowledgement grants Stop.
    return {a,
            "presentation-A",
            "generation-A-1",
            "generation-A-1",
            "owner-revision-1",
            0,
            10000,
            sample.count(),
            "owner-revision-1",
            0};
}

CommittedStop normal_stop(const Occurrence &target, QByteArray operation,
                          std::optional<PresentationObservation> presentation = {},
                          std::optional<ScopedCause> hold = {}) {
    const auto choice = hold ? std::nullopt : std::optional{QueueChoice::continue_queue};
    return {{target, choice},
            std::move(operation),
            StopProof::normal_commit,
            std::move(presentation),
            std::move(hold)};
}

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const TimelineUpdateMissing &error) {
        QFAIL(error.what());
    }
}
} // namespace

class Milestone006TimingTests final : public QObject {
    Q_OBJECT

  private slots:
    void s2AcknowledgedStop_data() {
        QTest::addColumn<bool>("output_receives");
        QTest::addColumn<bool>("adapter_confirms");
        QTest::addColumn<int>("first_advance");
        QTest::newRow("output-and-adapter-later") << true << true << 3000;
        QTest::newRow("lost-output") << false << true << 3000;
        QTest::newRow("adapter-failed") << true << false << 3000;
        QTest::newRow("first-eligible-turn-later") << true << true << 3100;
    }

    void s2AcknowledgedStop() {
        QFETCH(bool, output_receives);
        QFETCH(bool, adapter_confirms);
        QFETCH(int, first_advance);
        check([&] {
            TimelineFixture f;
            f.start();
            QCOMPARE(f.timeline.presentation(), window("A", 0ms));
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            f.timeline.advance(1800ms); // Stop request observed, no committed result.
            QCOMPARE(f.timeline.presentation(), window("A", 1800ms));

            apply_owner_update(f.timeline, 2000ms, normal_stop(a, "stop-A", active_a(2000ms)));
            QVERIFY(!f.timeline.presentation());
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));

            f.timeline.advance(2400ms);
            if (output_receives) {
                const auto output_snapshot = f.timeline.presentation();
                QVERIFY(!output_snapshot);
            }
            f.timeline.advance(2999ms);
            QVERIFY(!f.timeline.presentation());
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));

            f.timeline.advance(Sample{first_advance});
            QCOMPARE(f.timeline.presentation(), window("B", 0ms));
            QCOMPARE(f.release.releases, (Releases{a.occurrence, b.occurrence}));
            // External observation on a later turn. Failure and confirmation
            // both leave the already accepted Stop and B's start in force.
            const auto adapter_outcome = adapter_confirms ? "confirmed" : "failed";
            f.timeline.advance(4000ms);
            QVERIFY2(f.timeline.presentation() == window("B", Sample{4000 - first_advance}),
                     adapter_outcome);
            QCOMPARE(f.release.releases, (Releases{a.occurrence, b.occurrence}));
        });
    }

    void s3IndependentGates_data() {
        QTest::addColumn<bool>("stop_first");
        QTest::addColumn<bool>("manual_pause");
        QTest::newRow("stop-and-manual-pause") << true << true;
        QTest::newRow("stop-without-manual-pause") << true << false;
        QTest::newRow("natural-gap-gate-control") << false << true;
        QTest::newRow("natural-gap-without-manual-pause-control") << false << false;
    }

    void s3IndependentGates() {
        QFETCH(bool, stop_first);
        QFETCH(bool, manual_pause);
        check([&] {
            TimelineFixture f;
            f.release.b_holds = {defer, review};
            f.start();
            f.timeline.set_queue_paused(manual_pause, 1800ms);
            QCOMPARE(f.timeline.presentation(), window("A", 1800ms));
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            if (stop_first) {
                apply_owner_update(f.timeline, 2000ms, normal_stop(a, "stop-A", active_a(2000ms)));
                QVERIFY(!f.timeline.presentation());
            }

            const auto boundary = stop_first ? 3000ms : 11000ms;
            f.timeline.advance(boundary - 1ms);
            QVERIFY(!f.timeline.presentation());
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            QCOMPARE(std::erase(f.release.b_holds, defer), 1U);
            f.timeline.advance(boundary);
            QVERIFY(!f.timeline.presentation());
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            QCOMPARE(std::erase(f.release.b_holds, review), 1U);
            f.timeline.advance(boundary + 100ms);
            if (manual_pause) {
                QVERIFY(!f.timeline.presentation());
                QCOMPARE(f.release.releases, (Releases{a.occurrence}));
                f.timeline.set_queue_paused(false, boundary + 200ms);
                f.timeline.advance(boundary + 200ms);
            }
            QCOMPARE(f.timeline.presentation(), window("B", 0ms));
            QCOMPARE(f.release.releases, (Releases{a.occurrence, b.occurrence}));
        });
    }

    void s4ExactTargetAndNaturalBoundary_data() {
        QTest::addColumn<int>("sample_ms");
        QTest::addColumn<bool>("stop_a");
        QTest::addColumn<bool>("real_skip_control");
        QTest::newRow("same-ref-queued-B-during-A") << 2000 << false << false;
        QTest::newRow("queued-B-during-natural-gap") << 10500 << false << false;
        QTest::newRow("A-at-exact-expiry-with-queued-slot") << 10000 << true << false;
        QTest::newRow("natural-boundary-with-real-skip") << 10000 << false << true;
    }

    void s4ExactTargetAndNaturalBoundary() {
        QFETCH(int, sample_ms);
        QFETCH(bool, stop_a);
        QFETCH(bool, real_skip_control);
        check([&] {
            TimelineFixture f;
            f.start(1s, !stop_a && !real_skip_control);
            auto &timeline = f.timeline;
            const Sample sample{sample_ms};
            timeline.advance(sample);
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            const auto before = sample < 10s ? window("A", sample) : std::nullopt;
            QCOMPARE(timeline.presentation(), before);
            if (real_skip_control) {
                f.release.real_skip = true;
            } else {
                // A's captured queued slot qualifies Stop at expiry without
                // an active observation. B is keyed independently of A.
                apply_owner_update(timeline, sample, normal_stop(stop_a ? a : b, "exact-stop"));
                QCOMPARE(timeline.presentation(), before);
            }

            if (sample < 10s) {
                timeline.advance(9999ms);
                QCOMPARE(timeline.presentation(), window("A", 9999ms));
                timeline.advance(10000ms);
                QVERIFY(!timeline.presentation());
            }
            timeline.advance(10999ms);
            QVERIFY(!timeline.presentation());
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            timeline.advance(11000ms);
            QCOMPARE(timeline.presentation(), window(stop_a ? "B" : "C", 0ms));
            QCOMPARE(f.release.releases,
                     (Releases{a.occurrence, stop_a ? b.occurrence : c.occurrence}));
        });
    }

    void s5ZeroGapAndIndependentHolds_data() {
        QTest::addColumn<int>("gap_ms");
        QTest::addColumn<bool>("prior_hold");
        QTest::addColumn<bool>("new_hold");
        QTest::newRow("zero-gap-continue") << 0 << false << false;
        QTest::newRow("zero-gap-continue-preserves-H1") << 0 << true << false;
        QTest::newRow("zero-gap-resume-H2-then-H1") << 0 << true << true;
        QTest::newRow("single-Hold-clears-after-gap") << 1000 << false << true;
    }

    void s5ZeroGapAndIndependentHolds() {
        QFETCH(int, gap_ms);
        QFETCH(bool, prior_hold);
        QFETCH(bool, new_hold);
        check([&] {
            TimelineFixture f;
            f.start(Sample{gap_ms});
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            f.timeline.advance(1000ms);
            QCOMPARE(f.timeline.presentation(), window("A", 1000ms));
            if (prior_hold) {
                // C is pending with an unstarted slot. Its committed Hold
                // gates future starts without interrupting the active A.
                apply_owner_update(f.timeline, 1000ms, normal_stop(c, "stop-C", {}, h1));
                QCOMPARE(f.timeline.presentation(), window("A", 1000ms));
            }

            apply_owner_update(f.timeline, 2000ms,
                               normal_stop(a, "stop-A", active_a(2000ms),
                                           new_hold ? std::optional{h2} : std::nullopt));
            QVERIFY(!f.timeline.presentation());
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            f.timeline.advance(2000ms);
            if (prior_hold || new_hold) {
                QVERIFY(!f.timeline.presentation());
                QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            }
            if (new_hold) {
                if (gap_ms == 1000) {
                    f.timeline.advance(2999ms);
                    QVERIFY(!f.timeline.presentation());
                    f.timeline.advance(3000ms);
                    QVERIFY(!f.timeline.presentation());
                }
                const auto resume_at = prior_hold ? 2100ms : 3500ms;
                apply_owner_update(f.timeline, resume_at, CommittedResume{{h2}});
                QVERIFY(!f.timeline.presentation());
                f.timeline.advance(resume_at);
                if (prior_hold) {
                    QVERIFY(!f.timeline.presentation());
                    QCOMPARE(f.release.releases, (Releases{a.occurrence}));
                }
            }
            if (prior_hold) {
                apply_owner_update(f.timeline, 2200ms, CommittedResume{{h1}});
                QVERIFY(!f.timeline.presentation());
                f.timeline.advance(2200ms);
            }
            QCOMPARE(f.timeline.presentation(), window("B", 0ms));
            QCOMPARE(f.release.releases, (Releases{a.occurrence, b.occurrence}));
        });
    }

    void s8ReleaseRaceTiming_data() {
        QTest::addColumn<bool>("stop_first");
        QTest::newRow("committed-stop-before-release") << true;
        QTest::newRow("released-and-active-before-stop") << false;
    }

    void s8ReleaseRaceTiming() {
        QFETCH(bool, stop_first);
        check([&] {
            TimelineFixture f;
            f.pending();
            QVERIFY(!f.timeline.presentation());
            QVERIFY(f.release.releases.empty());
            if (!stop_first) {
                f.timeline.advance(0ms);
                QCOMPARE(f.timeline.presentation(), window("A", 0ms));
                QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            }

            apply_owner_update(
                f.timeline, 2000ms,
                normal_stop(a, "stop-race",
                            stop_first ? std::nullopt : std::optional{active_a(2000ms)}));
            QVERIFY(!f.timeline.presentation());
            if (stop_first) {
                QVERIFY(f.release.releases.empty());
                f.timeline.advance(2000ms);
                QCOMPARE(f.release.releases, (Releases{b.occurrence}));
            } else {
                f.timeline.advance(2999ms);
                QVERIFY(!f.timeline.presentation());
                QCOMPARE(f.release.releases, (Releases{a.occurrence}));
                f.timeline.advance(3000ms);
                QCOMPARE(f.release.releases, (Releases{a.occurrence, b.occurrence}));
            }
            QCOMPARE(f.timeline.presentation(), window("B", 0ms));
        });
    }

    void t1KnownRollbackKeepsNaturalBoundary() {
        TimelineFixture f;
        f.start();
        f.timeline.advance(2000ms);
        // The rolled-back writer turn publishes no owner update.
        QCOMPARE(f.timeline.presentation(), window("A", 2000ms));
        f.timeline.advance(9999ms);
        QCOMPARE(f.timeline.presentation(), window("A", 9999ms));
        f.timeline.advance(10000ms);
        QVERIFY(!f.timeline.presentation());
        f.timeline.advance(10999ms);
        QVERIFY(!f.timeline.presentation());
        QCOMPARE(f.release.releases, (Releases{a.occurrence}));
        f.timeline.advance(11000ms);
        QCOMPARE(f.timeline.presentation(), window("B", 0ms));
        QCOMPARE(f.release.releases, (Releases{a.occurrence, b.occurrence}));
    }

    void t1UncertaintyGatesSchedulingWithoutFreezingElapsedTime() {
        check([] {
            TimelineFixture f;
            f.start();
            f.timeline.advance(1800ms);
            QCOMPARE(f.timeline.presentation(), window("A", 1800ms));
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            apply_owner_update(f.timeline, 2000ms, UncertainStop{a, "uncertain-stop"});
            QCOMPARE(f.timeline.presentation(), window("A", 2000ms));
            f.timeline.advance(9999ms);
            QCOMPARE(f.timeline.presentation(), window("A", 9999ms));
            f.timeline.advance(10000ms);
            QVERIFY(!f.timeline.presentation());
            f.timeline.advance(11000ms);
            QVERIFY(!f.timeline.presentation());
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            apply_owner_update(f.timeline, 11500ms,
                               CommittedStop{{a, QueueChoice::continue_queue},
                                             "uncertain-stop",
                                             StopProof::normal_readback,
                                             {},
                                             {}});
            QVERIFY(!f.timeline.presentation());
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            f.timeline.advance(11500ms);
            QCOMPARE(f.timeline.presentation(), window("B", 0ms));
        });
    }

    void t1UncertainStopReadbackWhileActive() {
        check([] {
            TimelineFixture f;
            f.start();
            f.timeline.advance(1800ms);
            apply_owner_update(f.timeline, 2000ms, UncertainStop{a, "uncertain-stop"});
            f.timeline.advance(2300ms);
            QCOMPARE(f.timeline.presentation(), window("A", 2300ms));
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));

            apply_owner_update(f.timeline, 2600ms,
                               CommittedStop{{a, QueueChoice::continue_queue},
                                             "uncertain-stop",
                                             StopProof::normal_readback,
                                             active_a(2600ms),
                                             {}});
            QVERIFY(!f.timeline.presentation());
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            for (const auto sample : {3000ms, 3599ms}) {
                f.timeline.advance(sample);
                QVERIFY(!f.timeline.presentation());
                QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            }
            f.timeline.advance(3600ms);
            QCOMPARE(f.timeline.presentation(), window("B", 0ms));
            QCOMPARE(f.release.releases, (Releases{a.occurrence, b.occurrence}));
        });
    }

    void t1FirstReadbackUsesReconciliationSample_data() {
        QTest::addColumn<int>("sample_ms");
        QTest::addColumn<int>("next_start_ms");
        QTest::newRow("still-active") << 2600 << 3600;
        QTest::newRow("in-natural-gap") << 10500 << 11000;
        QTest::newRow("natural-gap-already-elapsed") << 11500 << 11500;
    }

    void t1FirstReadbackUsesReconciliationSample() {
        QFETCH(int, sample_ms);
        QFETCH(int, next_start_ms);
        check([&] {
            TimelineFixture f;
            f.start();
            f.timeline.advance(2000ms);
            QCOMPARE(f.timeline.presentation(), window("A", 2000ms));
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));

            // Supply the readback as an independent entry witness. The only
            // intervening scheduler turn is at 2,000. The integrated
            // uncertainty fence is exercised in its own selector above.
            const Sample readback{sample_ms};
            const Sample next_start{next_start_ms};
            const auto observation =
                readback < 10s ? std::optional{active_a(readback)} : std::nullopt;
            apply_owner_update(f.timeline, readback,
                               CommittedStop{{a, QueueChoice::continue_queue},
                                             "uncertain-stop",
                                             StopProof::normal_readback,
                                             observation,
                                             {}});
            QVERIFY(!f.timeline.presentation());
            QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            if (readback < next_start) {
                f.timeline.advance(next_start - 1ms);
                QVERIFY(!f.timeline.presentation());
                QCOMPARE(f.release.releases, (Releases{a.occurrence}));
            }
            f.timeline.advance(next_start);
            QCOMPARE(f.timeline.presentation(), window("B", 0ms));
            QCOMPARE(f.release.releases, (Releases{a.occurrence, b.occurrence}));
        });
    }
};

QTEST_GUILESS_MAIN(Milestone006TimingTests)
#include "timing_tests.moc"
