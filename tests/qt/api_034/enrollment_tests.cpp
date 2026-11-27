// SPDX-License-Identifier: GPL-3.0-or-later

#include "enrollment_candidate.hpp"

namespace {
using namespace api_034_fixture;

const QString source_table = "subathon_intake_sources";
const QString policy_table = "subathon_intake_policies";
const QString member_table = "subathon_intake_members";
const Scope installation_scope{a.installation, {}, {}, {}};
const Fence before_page{2, 4, 2, "g3"};
const Fence after_page{2, 5, 2, "g3"};

EnrollmentRequest request(Fence fence = before_page) {
    return {a, std::move(fence), 2,
            {member(b), "mirror-lane", "g4", "gB", profile, cursor, true}, "enroll-cfg"};
}

class EnrollmentFixture {
  public:
    HistoryFixture history;
    QByteArray permit{"enrollment-decision"};
    bool allowed{true};
    QList<std::tuple<EnrollmentAction, Scope, qint64>> calls;

    EnrollmentFixture() {
        history.open_catch_up();
        history.startup.configuration.revision = "enroll-cfg";
        history.startup.configuration.lanes.append(request().incoming);
        require(store().rows("SELECT account FROM main.subathon_intake_sources") ==
                    SqlRows{{a.account}}, "A is the only source before enrollment");
    }

    Store &store() { return history.startup.store; }

    StartupResult submit(const EnrollmentRequest &intent, EnrollmentAction action) {
        calls.clear();
        return enroll(history.startup, intent, action,
                      [this](const auto &, EnrollmentAction current, const Scope &scope, qint64 n)
                          -> std::variant<StartupDenied, StartupUnavailable, EnrollmentPermit> {
                          calls.append({current, scope, n});
                          if (!allowed)
                              return StartupDenied{};
                          return EnrollmentPermit{permit};
                      });
    }

    bool resolved_both(EnrollmentAction action) const {
        return calls.size() == 2 && calls.contains({action, b, 3}) &&
               calls.contains({action, installation_scope, 3});
    }

    void advance_page() {
        const auto next = QByteArray("U");
        const auto proof = QByteArray("fixture-qualified-contiguous-range");
        const PageBinding page{a, before_page, "O", 1, 1, "T",
                               QByteArray("P\0\0\0\0", 5) + f(next) + f(proof),
                               1, 0, next, "11", "12", proof, Coverage::contiguous, "14"};
        store().work([&](const auto &unit) -> QVariant {
            require(stage_page(unit, {page, {}}).status == HistoryStatus::staged,
                    "Real writer binds the synthetic empty page");
            require(stage_page_finalization(unit, page) == HistoryStatus::staged,
                    "Real writer advances A's frontier through page finalization");
            return true;
        });
        require(store().rows("SELECT revision,policy_revision,frontier "
                             "FROM main.subathon_intake_sources WHERE account=X'41'") ==
                    SqlRows{{after_page.revision, after_page.policy_revision, QByteArray("12")}},
                "A advances its source revision and frontier on policy 2");
    }
};

void exact_enrollment_image(EnrollmentFixture &fixture, Image before) {
    const auto b_member = member(b);
    before[policy_table].append(SqlRow{a.installation, qint64(3),
                                      f("review") + f("enroll-cfg") + f("enrollment-decision")});
    const auto a_member = member(a);
    before[member_table].append(SqlRow{a.installation, qint64(3), a.provider, a.account,
                                     a.stream, a_member.award_class, a_member.effect_scope,
                                     a_member.overlap_set, qint64(1), a_member.document});
    before[member_table].append(SqlRow{b.installation, qint64(3), b.provider, b.account,
                                     b.stream, b_member.award_class, b_member.effect_scope,
                                     b_member.overlap_set, QVariant{}, b_member.document});
    before[source_table].append(SqlRow{b.installation, b.provider, b.account, b.stream,
                                      qint64(1), QByteArray("g4"), qint64(1), qint64(3), qint64(0),
                                      QVariant{}, QVariant{}, cursor});
    require(fixture.store().image() == before,
            "Only the next policy, copied member, new non-awarding member and pending source exist");
}

class LaterEnrollmentTest final : public QObject {
    Q_OBJECT

  private slots:
    void authorizedEnrollmentPreservesAAndReturnsExactReplayUnderFreshAuthority() {
        EnrollmentFixture fixture;
        const auto before = fixture.store().image();
        const auto exact = request();
        fixture.allowed = false;
        const auto denied_write = fixture.submit(exact, EnrollmentAction::enroll_source);
        QCOMPARE(denied_write.status, IntakeStatus::denied);
        QVERIFY(!denied_write.projection);
        QVERIFY(fixture.store().image() == before);

        fixture.allowed = true;
        const auto written = fixture.submit(exact, EnrollmentAction::enroll_source);
        QCOMPARE(written.status, IntakeStatus::committed);
        QVERIFY(fixture.resolved_both(EnrollmentAction::enroll_source));
        QVERIFY(written.projection);
        exact_enrollment_image(fixture, before);
        const auto enrolled = fixture.store().image();

        fixture.permit = "current-inspect-decision";
        const auto replay = fixture.submit(exact, EnrollmentAction::inspect);
        QCOMPARE(replay.status, IntakeStatus::exact_replay);
        QCOMPARE(replay.readback, std::optional{StartupReadback::applied});
        QVERIFY(fixture.resolved_both(EnrollmentAction::inspect));
        QVERIFY(replay.projection == written.projection);
        QVERIFY(fixture.store().image() == enrolled);
        fixture.allowed = false;
        const auto denied = fixture.submit(exact, EnrollmentAction::inspect);
        QCOMPARE(denied.status, IntakeStatus::denied);
        QVERIFY(!denied.projection);
        QVERIFY(fixture.store().image() == enrolled);
    }

    void bothAProgressOrdersProduceTheSameEnrollmentAndReplay() {
        EnrollmentFixture enroll_first;
        EnrollmentFixture page_first;
        page_first.advance_page();
        QCOMPARE(enroll_first.submit(request(), EnrollmentAction::enroll_source).status,
                 IntakeStatus::committed);
        enroll_first.advance_page();
        QCOMPARE(page_first.submit(request(after_page), EnrollmentAction::enroll_source).status,
                 IntakeStatus::committed);
        const auto converged = enroll_first.store().image();
        QVERIFY(page_first.store().image() == converged);
        QCOMPARE(enroll_first.submit(request(after_page), EnrollmentAction::enroll_source).status,
                 IntakeStatus::exact_replay);
        QVERIFY(enroll_first.resolved_both(EnrollmentAction::enroll_source));
        QCOMPARE(page_first.submit(request(), EnrollmentAction::enroll_source).status,
                 IntakeStatus::exact_replay);
        QVERIFY(page_first.resolved_both(EnrollmentAction::enroll_source));
        auto changed = request();
        changed.incoming.enrollment_generation = "another-generation";
        enroll_first.history.startup.configuration.lanes.last() = changed.incoming;
        QCOMPARE(enroll_first.submit(changed, EnrollmentAction::enroll_source).status,
                 IntakeStatus::conflict);
        QVERIFY(enroll_first.store().image() == converged);
        QVERIFY(page_first.store().image() == converged);
    }
};
} // namespace

QTEST_GUILESS_MAIN(LaterEnrollmentTest)
#include "enrollment_tests.moc"
