// SPDX-License-Identifier: GPL-3.0-or-later
#include "../milestone_006_slice2/refund_fixture.hpp"

namespace {
using namespace milestone_006_slice2;
using Kind = MilestoneActionKind;

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

std::shared_ptr<const ReplacePreview> bind(finance::File &file, CommandAuthority &owner,
                                           const Command &command) {
    std::shared_ptr<const ReplacePreview> result;
    file.write([&](const auto &s) {
        const auto preview = preview_replace(s, owner, command);
        const auto *bound = std::get_if<std::shared_ptr<const ReplacePreview>>(&preview);
        require(bound && *bound, "Real v21 preview binds the exact eligible patch");
        result = *bound;
    });
    return result;
}

Decision confirm(finance::File &file, CommandAuthority &owner, const Command &command,
                 const ReplacePreview *proof) {
    std::optional<Decision> decided;
    file.write([&](const auto &s) { decided = stage_command(s, owner, command, proof); });
    return *decided;
}

std::vector<SelectedVersion> read_versions(finance::File &file, CommandAuthority &owner,
                                           const Occurrence &target,
                                           const std::vector<MilestoneHoldKey> &holds = {}) {
    std::optional<OccurrenceSelection> selected;
    file.write([&](const auto &s) {
        selected = read_occurrence_selection(s, owner, stop("selection-read", target));
        require(selected && selected->target.occurrence == target.occurrence &&
                    selected->target.track == target.track &&
                    selected->target.origin.index() == target.origin.index() &&
                    !selected->released && !selected->skipped && selected->stop.isEmpty() &&
                    selected->independent_holds == holds && selected->interruption_holds.empty(),
                "Normal exact readback retains original release, identity and independent gates");
        if (const auto *key = std::get_if<ApplicationOperationKey>(&target.origin))
            require(finance::same(std::get<ApplicationOperationKey>(selected->target.origin), *key),
                    "Readback retains the exact application operation");
        else
            require(std::get<MilestoneEditKey>(selected->target.origin) ==
                        std::get<MilestoneEditKey>(target.origin),
                    "Readback retains the exact edit operation");
    });
    return selected->selected_versions;
}

const Replacement shared_next{1, {Kind::automation, "achievement-action", "shared-v2"}};
const Replacement aux_next{3, {Kind::automation, "aux", "aux-v2"}};

struct CrossOriginFixture {
    finance::File file{finance::OriginalCredit{.milestone = true}};
    Occurrence b{edit::track, finance::achievement, finance::original};
    MilestoneEditIntent edit_a =
        edit::single("earn-A", {"A", 36000000}, {{edit::a, MilestoneActionChoice::run}});
    Occurrence a{edit::track, edit::a, edit_a.key};
    DeliveryJobKey app_job{finance::original, "original-job"};
    MilestoneEditJobKey edit_job{edit_a.key, edit::a, 1};
    Authority owner;

    CrossOriginFixture() {
        report(migrate(*file.writer,
                       {MigrationMode::advance, contribution_review_repository_manifest(), {}, {}}),
               CatalogState::current, 19, 19, 4);
        report(migrate(*file.writer,
                       {MigrationMode::advance, connector_intake_repository_manifest(), {}, {}}),
               CatalogState::current, 20, 20, 1);
        reopen_financial(file);
        edit::Authority edit_owner;
        auto capture = edit::capture_b(true);
        capture.milestone_ref = "A";
        capture.occurrence = edit::a;
        capture.actions = {{Kind::automation, "achievement-action", "achievement-v1"},
                           {Kind::bonus, "award", "award-v1"},
                           {Kind::automation, "aux", "aux-v1"}};
        auto auxiliary = capture.jobs.front();
        auxiliary.ordinal = 3;
        auxiliary.capture.action_ref = "aux";
        auxiliary.capture.action_version_ref = "aux-v1";
        capture.jobs.front().capture.action_ref = "achievement-action";
        capture.jobs.front().capture.action_version_ref = "achievement-v1";
        capture.jobs.push_back(std::move(auxiliary));
        edit_owner.facts.lifecycle.timer = file.head().timer;
        edit_owner.facts.captures = {std::move(capture)};
        file.write([&](const auto &s) {
            require(SqliteAccountSessionUnit(s, {}).insert_installation(
                        {{"install", "epoch"}, "revision"}) == LifecycleWriteResult::staged,
                    "Normal account store creates the exact installation");
            const subathon::core::MilestoneCompletion app{"target-35", "milestone-v1", 35000000,
                                                          40000000, "ctx"};
            require(initialize_milestone_track(
                        s, {edit::track,
                            "cfg",
                            "milestone-v1",
                            "original-prefix",
                            {{"target-35", 35000000}, {"A", 50000000}},
                            std::vector<MilestoneTrackSeed::Member>{
                                {app, finance::original, finance::achievement}}}),
                    "Real track binds application B and the unearned edit target");
            const auto preview = preview_milestone_target_edit(s, edit_owner, edit::actor, edit_a);
            edit::File::confirmable(preview);
            const auto saved = confirm_milestone_target_edit(s, edit_owner, edit::actor, edit_a,
                                                             *preview.confirmation);
            require(saved.result && saved.result->achievements.size() == 1 &&
                        saved.result->achievements.front().order == 2,
                    "Real target edit earns exact A and captures its mixed slots");
        });
        advance_to_protected_successor(file);
        owner.historical_bonuses = {{a, {2, {Kind::bonus, "award", "award-v1"}}, 30000}};
        set_default_captures();
        require_originals();
    }

    void set_default_captures() {
        owner.captures = {{shared_next, QByteArray::fromHex("0081"), "local/next", "S"},
                          {aux_next, QByteArray::fromHex("00ff"), "local/aux", "S"}};
    }

    void require_originals() {
        file.write([&](const auto &s) {
            const auto app = SqliteDeliveryJobStore(s).lookup(app_job);
            const auto edited = lookup_milestone_edit_job(s, edit_job);
            require(app && edited && app->capture.action_ref == "achievement-action" &&
                        edited->capture.action_ref == "achievement-action" &&
                        app->capture.action_version_ref == "achievement-v1" &&
                        edited->capture.action_version_ref == "achievement-v1" &&
                        app->state == DeliveryJobState::queued && app->revision == 1 &&
                        edited->state == DeliveryJobState::queued && edited->revision == 1,
                    "Each typed origin retains an independently enrolled unstarted shared action");
        });
    }

    Command command(const Occurrence &target, QByteArray operation,
                    std::vector<Replacement> patch = {shared_next}) const {
        return {edit::track, std::move(operation), edit::actor,
                ReplaceIntent{target, std::move(patch)}};
    }

    std::array<SqlRows, 9> snapshot() {
        return {
            file.rows("SELECT * FROM main.subathon_milestone_command_journal ORDER BY operation"),
            file.rows("SELECT * FROM main.subathon_milestone_slot_versions ORDER BY version"),
            file.rows(
                "SELECT * FROM main.subathon_milestone_slot_attempts ORDER BY version,revision"),
            file.rows("SELECT * FROM main.subathon_milestone_groups ORDER BY queue_order"),
            file.rows(
                "SELECT * FROM main.subathon_milestone_edit_occurrences ORDER BY queue_order"),
            file.rows("SELECT * FROM main.subathon_delivery_captures ORDER BY "
                      "operation_scope,operation_ref,job_ref"),
            file.rows("SELECT * FROM main.subathon_delivery_events ORDER BY "
                      "operation_scope,operation_ref,job_ref,revision"),
            file.rows("SELECT * FROM main.subathon_milestone_edit_actions ORDER BY "
                      "occurrence_ref,action_ordinal"),
            file.rows("SELECT * FROM main.subathon_milestone_edit_events ORDER BY "
                      "occurrence_ref,action_ordinal,revision")};
    }

    void require_no_selection_change() {
        require(read_versions(file, owner, a) ==
                        std::vector<SelectedVersion>{{1, {}}, {2, {}}, {3, {}}} &&
                    read_versions(file, owner, b) == std::vector<SelectedVersion>{{1, {}}},
                "Neither exact origin acquired a selected successor");
        require_originals();
    }
};

struct CurrentAuthority final : CommandAuthority {
    Authority &trusted;
    bool available{true};
    int reads{};

    explicit CurrentAuthority(Authority &current) : trusted(current) {}

    std::optional<Facts> select(const SqliteWorkSession &s, const Command &command) override {
        ++reads;
        return available ? trusted.select(s, command) : std::nullopt;
    }

    std::optional<qint64> historical_bonus_amount(const SqliteWorkSession &s,
                                                  const Occurrence &target,
                                                  const Replacement &definition) override {
        return trusted.historical_bonus_amount(s, target, definition);
    }
};

Decision confirm_at_owner(const SqliteWorkSession &, CommandAuthority &, const Command &,
                          const ReplacePreview &) {
    throw std::logic_error("R7: confirm exact cross-origin command under current authority");
}

} // namespace

class Milestone006ReplaceTests final : public QObject {
    Q_OBJECT

  private slots:
    void r5ApplicationRefundReviewInvalidatesDeferPreview() {
        check([] {
            RefundFixture f(false);
            advance_to_protected_successor(f.file);
            const Occurrence a{edit::track, edit::a,
                               ApplicationOperationKey{"test", "application-A"}};
            Authority owner;
            const Replacement new_bonus{1, {Kind::bonus, "award", "bonus-v2"}};
            owner.captures = {{new_bonus, {}, {}, {}, 20000}};
            owner.historical_bonuses = {{a, {1, bonus}, 30000}};
            const Command command{edit::track, "replace-held-application", edit::actor,
                                  ReplaceIntent{a, {new_bonus}}};
            require(read_versions(f.file, owner, a, {defer}) ==
                        std::vector<SelectedVersion>{{1, {}}},
                    "Original unstarted bonus is selected under the real Defer");
            const auto historical = f.file.rows("SELECT * FROM main.subathon_milestone_actions "
                                                "ORDER BY occurrence_ref,action_ordinal");
            const auto old = bind(f.file, owner, command);

            const MilestoneDecisionEvidence resolved{"operator", "r5-continue-defer"};
            f.file.write([&](const auto &s) {
                require(
                    SqliteMilestoneGroupDecisions(s).resolve(
                        a.occurrence, defer, MilestoneHoldResolution::continue_actions, resolved),
                    "Normal #236 path settles the original exact Defer");
            });
            f.fixture.input.milestone_policy = Policy::review;
            const auto refund = f.fixture.command("r5-refund-review", "25", 0);
            const auto applied = live::accepted(f.fixture.refund(refund, f.entitlement(0, 25, 0)));
            require(applied.milestone_observations.size() == 1 &&
                        applied.milestone_observations.front().occurrence == a.occurrence,
                    "Owning live refund introduces only A's Review");
            f.require_score(995);
            f.reopen();
            const MilestoneHoldKey hold{MilestoneHoldCause::refund_review, "r5-refund-review"};
            const auto audit = [&] {
                f.file.write([&](const auto &s) {
                    const SqliteMilestoneGroupDecisions gates(s);
                    const auto acceptance = gates.acceptance_audit(a.occurrence);
                    const auto review_row = gates.refund_audit(a.occurrence, hold.source_ref);
                    require(acceptance &&
                                acceptance->resolution ==
                                    MilestoneHoldResolution::continue_actions &&
                                acceptance->resolution_evidence == resolved &&
                                !acceptance->ended_by_skip && review_row &&
                                review_row->policy == Policy::review &&
                                review_row->current_target_micro_points == 1000 &&
                                review_row->deficit_micro_points == 5 && !review_row->resolution &&
                                !review_row->ended_by_skip &&
                                gates.pending_disposition(a.occurrence) ==
                                    MilestonePendingDisposition::held,
                            "Defer history and the exact active refund Review survive");
                });
            };
            const auto durable = [&] {
                return std::array{
                    f.file.rows("SELECT * FROM main.subathon_milestone_command_journal"),
                    f.file.rows("SELECT * FROM main.subathon_milestone_slot_versions"),
                    f.file.rows("SELECT * FROM main.subathon_milestone_slot_attempts"),
                    f.file.rows("SELECT * FROM main.subathon_milestone_bonus_receipts"),
                    f.file.rows("SELECT * FROM main.subathon_milestone_refund_decisions"),
                    f.file.rows("SELECT * FROM main.subathon_milestone_acceptance_decisions"),
                    f.file.rows("SELECT released FROM main.subathon_milestone_groups ORDER BY "
                                "queue_order")};
            };
            const auto before = durable();
            audit();
            require(
                before[0].empty() && before[1].empty() && before[2].empty() && before[3].empty() &&
                    before[6] == SqlRows{{0}, {0}},
                "No replacement, attempt, bonus receipt or release exists before stale confirm");
            require(
                confirm(f.file, owner, command, old.get()).disposition == Disposition::refused &&
                    durable() == before &&
                    read_versions(f.file, owner, a, {hold}) ==
                        std::vector<SelectedVersion>{{1, {}}},
                "Changed exact cause stales the old token without changing selection or history");

            const auto fresh = bind(f.file, owner, command);
            require(durable() == before, "Fresh preview reserves no versions or journal keys");
            const auto selected = confirm(f.file, owner, command, fresh.get());
            require(
                selected.disposition == Disposition::committed && selected.versions.size() == 1 &&
                    selected.versions.front().ordinal == 1 &&
                    !selected.versions.front().version.isEmpty() &&
                    read_versions(f.file, owner, a, {hold}) == selected.versions,
                "A fresh token selects the only exact slot while A remains held and unreleased");
            const auto saved = durable();
            require(saved[0].size() == 1 && saved[1].size() == 1 && saved[2].empty() &&
                        saved[3].empty() && saved[4] == before[4] && saved[5] == before[5] &&
                        saved[6] == before[6] &&
                        f.file.rows("SELECT * FROM main.subathon_milestone_actions ORDER BY "
                                    "occurrence_ref,action_ordinal") == historical,
                    "The only persistent additions are one replacement and operation");
            auto replay = selected;
            replay.disposition = Disposition::replay;
            require(confirm(f.file, owner, command, fresh.get()) == replay && durable() == saved,
                    "Exact replay cannot append a duplicate version, attempt or receipt");
            audit();
            f.require_score(995);
        });
    }

    void r7SharedRefDoesNotAuthorizeCrossOriginOrChangedBytes() {
        check([] {
            CrossOriginFixture f;
            const auto edit_cmd = f.command(f.a, "replace-edit-A");
            const auto app_cmd = f.command(f.b, "preview-application-B");
            const auto bound = bind(f.file, f.owner, edit_cmd);
            require(bool(bind(f.file, f.owner, app_cmd)),
                    "B is independently eligible for the same patch");
            const auto before = f.snapshot();

            auto wrong = edit_cmd;
            std::get<ReplaceIntent>(wrong.intent).target = f.b;
            require(confirm(f.file, f.owner, wrong, bound.get()).disposition ==
                            Disposition::refused &&
                        f.snapshot() == before,
                    "An otherwise eligible application B cannot use edit A's exact confirmation");

            f.owner.captures[0].payload = QByteArray::fromHex("0082");
            require(
                confirm(f.file, f.owner, edit_cmd, bound.get()).disposition ==
                        Disposition::refused &&
                    f.snapshot() == before,
                "Changed authority bytes at identical refs and revisions stale the exact token");
            f.set_default_captures();
            f.owner.captures[0].route = "local/different-current-route";
            require(confirm(f.file, f.owner, edit_cmd, bound.get()).disposition ==
                            Disposition::refused &&
                        f.snapshot() == before,
                    "A different route cannot substitute for the bound current capture");
            f.set_default_captures();
            auto claimed_actor = edit_cmd;
            claimed_actor.actor.actor_ref = "caller-asserted-actor";
            require(confirm(f.file, f.owner, claimed_actor, bound.get()).disposition ==
                            Disposition::denied &&
                        f.snapshot() == before,
                    "A client-asserted identity cannot authorize the exact edit token");
            f.require_no_selection_change();
            f.owner.revision = "fresh-after-parity";
            const auto fresh = bind(f.file, f.owner, edit_cmd);
            require(f.snapshot() == before, "Fresh current preview reserves no operation");
            const auto selected = confirm(f.file, f.owner, edit_cmd, fresh.get());
            require(
                selected.disposition == Disposition::committed && selected.versions.size() == 1 &&
                    selected.versions.front().ordinal == 1 &&
                    !selected.versions.front().version.isEmpty() &&
                    read_versions(f.file, f.owner, f.a) ==
                        std::vector<SelectedVersion>{selected.versions.front(), {2, {}}, {3, {}}} &&
                    read_versions(f.file, f.owner, f.b) == std::vector<SelectedVersion>{{1, {}}},
                "Authorized exact edit A changes only its intended slot");
            f.require_originals();
        });
    }

    void r7ShapeRefusalsPreserveBothOrigins_data() {
        QTest::addColumn<int>("shape");
        QTest::newRow("application-empty") << 0;
        QTest::newRow("application-duplicate") << 1;
        QTest::newRow("application-nonexistent") << 2;
        QTest::newRow("application-no-change") << 3;
        QTest::newRow("edit-valid-plus-automation-to-bonus") << 4;
        QTest::newRow("edit-valid-plus-bonus-to-automation") << 5;
    }

    void r7ShapeRefusalsPreserveBothOrigins() {
        QFETCH(int, shape);
        check([&] {
            CrossOriginFixture f;
            const auto before = f.snapshot();
            const auto positive = shape < 4 ? f.command(f.b, "shape-positive")
                                            : f.command(f.a, "shape-positive", {aux_next});
            require(bool(bind(f.file, f.owner, positive)),
                    "The unchanged same-kind counterpart is genuinely eligible");
            auto invalid = positive;
            auto &patch = std::get<ReplaceIntent>(invalid.intent).patch;
            if (shape == 0)
                patch.clear();
            else if (shape == 1)
                patch = {shared_next, shared_next};
            else if (shape == 2) {
                const Replacement nonexistent{5, shared_next.action};
                patch = {nonexistent};
                f.owner.captures.push_back({nonexistent, QByteArray("valid"), "local/next", "S"});
            } else if (shape == 3) {
                const Replacement original{
                    1, {Kind::automation, "achievement-action", "achievement-v1"}};
                patch = {original};
                f.owner.captures = {
                    {original, QByteArray("synthetic-action"), "local-fixture", "fixture-sender"}};
            } else if (shape == 4) {
                const Replacement conversion{1, {Kind::bonus, "award", "award-v2"}};
                patch.push_back(conversion);
                f.owner.captures.push_back({conversion, {}, {}, {}, 20000});
            } else {
                const Replacement conversion{2, {Kind::automation, "achievement-action", "v2"}};
                patch.push_back(conversion);
                f.owner.captures.push_back({conversion, QByteArray("valid"), "local/next", "S"});
            }
            f.file.write([&](const auto &s) {
                const auto reply = preview_replace(s, f.owner, invalid);
                const auto *refusal = std::get_if<Disposition>(&reply);
                require(refusal && *refusal == Disposition::refused,
                        "Invalid complete patch receives no opaque preview");
            });
            require(
                f.snapshot() == before,
                "Invalid patch does not reserve a key, append a version or start either origin");
            f.require_no_selection_change();
        });
    }

    void r7ApplicationRetryAndDisclosureCheckRevocation() {
        check([] {
            CrossOriginFixture f;
            const auto command = f.command(f.b, "application-retry-guard");
            const auto preview = bind(f.file, f.owner, command);
            const auto selected = confirm(f.file, f.owner, command, preview.get());
            require(selected.disposition == Disposition::committed &&
                        selected.versions.size() == 1 && selected.versions.front().ordinal == 1 &&
                        read_versions(f.file, f.owner, f.b) == selected.versions &&
                        read_versions(f.file, f.owner, f.a) ==
                            std::vector<SelectedVersion>{{1, {}}, {2, {}}, {3, {}}},
                    "Real authorized application confirmation selects only its exact slot");
            const auto saved = f.snapshot();
            auto replay = selected;
            replay.disposition = Disposition::replay;
            require(
                confirm(f.file, f.owner, command, preview.get()) == replay && f.snapshot() == saved,
                "Authorized exact application replay returns the durable result without writes");
            f.owner.disclosure = false;
            f.file.write([&](const auto &s) {
                require(read_command(s, f.owner, command).disposition == Disposition::denied &&
                            !read_occurrence_selection(s, f.owner, stop("withheld-selection", f.b)),
                        "Revoked current disclosure reveals neither the result nor selection");
            });
            require(confirm(f.file, f.owner, command, preview.get()).disposition ==
                            Disposition::denied &&
                        f.snapshot() == saved,
                    "Revoked retry writes nothing on either origin");
        });
    }

    void r7ConfirmationAuthorityParity_data() {
        QTest::addColumn<bool>("edit_origin");
        QTest::addColumn<int>("gate");
        QTest::newRow("edit-revoked") << true << 0;
        QTest::newRow("edit-absent") << true << 1;
        QTest::newRow("edit-current") << true << 2;
        QTest::newRow("application-revoked") << false << 0;
        QTest::newRow("application-absent") << false << 1;
        QTest::newRow("application-current") << false << 2;
    }

    void r7ConfirmationAuthorityParity() {
        QFETCH(bool, edit_origin);
        QFETCH(int, gate);
        check([&] {
            CrossOriginFixture f;
            const auto &target = edit_origin ? f.a : f.b;
            const auto &other = edit_origin ? f.b : f.a;
            auto expected_selection = read_versions(f.file, f.owner, target);
            const auto other_before = read_versions(f.file, f.owner, other);
            const auto command = f.command(target, "authorize-confirm");
            CurrentAuthority owner(f.owner);
            const auto before = f.snapshot();
            const auto preview = bind(f.file, owner, command);
            require(f.snapshot() == before, "Exact protected preview leaves both origins intact");
            // Facts selected through this owner port model the three current
            // authority observations, without persisting any synthetic grant.
            f.owner.command = gate != 0;
            f.owner.disclosure = gate != 0;
            owner.available = gate != 1;
            if (gate == 0)
                require(confirm(f.file, owner, command, preview.get()).disposition ==
                                Disposition::denied &&
                            f.snapshot() == before,
                        "Protected confirmation refuses known revocation without writes");
            const auto previous_reads = owner.reads;
            const auto expected = gate == 0   ? Disposition::denied
                                  : gate == 1 ? Disposition::unavailable
                                              : Disposition::committed;
            std::optional<Decision> decided;
            f.file.write([&](const auto &s) {
                decided = confirm_at_owner(s, owner, command, *preview);
                require(decided->disposition == expected && owner.reads > previous_reads,
                        "Owner confirmation distinguishes current denial, missing facts and grant");
            });
            if (gate != 2) {
                require(decided->versions.empty() && f.snapshot() == before,
                        "Neither denial nor unavailable facts change either origin");
            } else {
                require(!expected_selection.empty() && decided->versions.size() == 1 &&
                            decided->versions.front().ordinal == 1 &&
                            !decided->versions.front().version.isEmpty(),
                        "Authorized owner returns one exact nonoriginal selection");
                expected_selection.front() = decided->versions.front();
                require(read_versions(f.file, f.owner, target) == expected_selection &&
                            read_versions(f.file, f.owner, other) == other_before,
                        "Authorized owner persists only the exact selected slot");
                f.require_originals();
            }
        });
    }
};

QTEST_GUILESS_MAIN(Milestone006ReplaceTests)
#include "replace_tests.moc"
