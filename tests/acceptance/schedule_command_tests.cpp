// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <map>
#include <stdexcept>
#include <tuple>
#include <utility>

#include <subathon/core/timer_editorial_commands.hpp>
#include <subathon/server/schedule_command.hpp>

namespace {
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace subathon::core;
using namespace subathon::server;
using Kind = ScheduleCommandKind;
using Refusal = ScheduleRefusal;
using Outcome = ScheduleWriteOutcome;
using ZoneError = ScheduleZoneUnavailable;
using Instant = std::int64_t;

constexpr Instant day = duration_cast<milliseconds>(sys_days{2026y / January / 8}
                                                      .time_since_epoch()).count();
constexpr Instant at(milliseconds time) { return day + time.count(); }
constexpr auto noon = at(12h);
constexpr auto h1 = at(24h + 11h + 55min);
constexpr auto h2 = at(48h + 11h + 55min);
constexpr auto dense_h = noon + 1;
const PauseState manual{{PauseReason::Manual}};
const PauseReasons manual_schedule{PauseReason::Manual, PauseReason::Schedule};

ScheduledPauseObservation observation(Instant t) { return {std::to_string(t), t}; }
TimerState active_timer() { return {.lifecycle = TimerLifecycle::Active, .main_remaining_ms = 900'000}; }
ScheduledPauseRule rule(std::string id, milliseconds local, milliseconds duration, bool enabled = true) {
    return {std::move(id), {.local_time = local}, duration, enabled};
}
const auto rule_a = rule("A", 11h + 45min, 30min);
const auto rule_b = rule("B", 11h + 50min, 20min);
const ScheduledPauseConfiguration old_rules{"Fixed/Zero", {rule_a, rule_b}};
const ScheduledPauseConfiguration keep_b{"Fixed/Zero", {rule_b}};
const ScheduledPauseConfiguration empty{"Fixed/Zero", {}};
const ScheduledPauseConfiguration dense{"Dense/Two", {rule("long", 12h + 1ms, days{366})}};

template<class Expected, class... T>
void expect(const std::variant<T...> &reply, Expected expected) {
    const auto *value = std::get_if<Expected>(&reply);
    ASSERT_NE(value, nullptr) << "variant index " << reply.index();
    EXPECT_EQ(*value, expected);
}

enum class DenseTail { complete, cap, terminal };
struct Zone final : ScheduleZoneSource {
    std::string fixed_revision = "Z1", dense_revision = "r1";
    DenseTail tail = DenseTail::complete;
    bool fail_zero = false;
    Instant other_end = at(days{7});
    int reads = 0;
    std::vector<ScheduleZoneRequest> pages;
    std::variant<ScheduleZoneFacts, ZoneError> read(const ScheduleZoneRequest &r) override {
        ++reads;
        if (r.configured_zone == "Fixed/Zero" && fail_zero) {
            return ZoneError::insufficient_coverage;
        }
        const auto revision = r.configured_zone == "Fixed/Other" ? "Other1" :
                              r.configured_zone == "Dense/Two" ? dense_revision : fixed_revision;
        if (r.required_rules_revision && *r.required_rules_revision != revision) {
            return ZoneError::insufficient_coverage;
        }
        const auto end = r.configured_zone == "Fixed/Other" ? other_end : r.through_utc_ms;
        return ScheduleZoneFacts{r.configured_zone, revision, r.from_utc_ms,
                                 std::min(end, r.through_utc_ms), 0, {}};
    }
    std::variant<ForecastPage, ZoneError> read_forecast_page(const ScheduleZoneRequest &r) override {
        pages.push_back(r);
        if (r.configured_zone != "Dense/Two") {
            return ScheduleZoneSource::read_forecast_page(r);
        }
        if (r.required_rules_revision && *r.required_rules_revision != dense_revision) {
            return ZoneError::insufficient_coverage;
        }
        constexpr auto first = at(12h + days{3});
        constexpr auto spacing = duration_cast<milliseconds>(2h).count();
        std::vector<ScheduleZoneTransition> rows;
        for (int i = 0; i < 4097; ++i) {
            const auto t = first + spacing * i;
            if (t <= r.from_utc_ms || t > r.through_utc_ms) {
                continue;
            }
            if (i == 4096 && tail == DenseTail::terminal) {
                return ForecastPage{{r.configured_zone, dense_revision, r.from_utc_ms, t - 1,
                                     0, std::move(rows)}, TerminalPrefix{ZoneError::insufficient_coverage}};
            }
            if (i == 4096 && tail == DenseTail::complete) {
                break;
            }
            if (rows.size() == 64) {
                return ForecastPage{{r.configured_zone, dense_revision, r.from_utc_ms, t - 1,
                                     0, std::move(rows)}, TransitionCap{}};
            }
            rows.push_back({t, i % 2 == 0 ? 1 : 0});
        }
        return ForecastPage{{r.configured_zone, dense_revision, r.from_utc_ms, r.through_utc_ms,
                             0, std::move(rows)}, ReachedRequestEnd{}};
    }
};

using Scope = std::tuple<std::string, std::string, std::string, std::string>;
Scope scope(const ScheduleAdmission &a) {
    return {a.request.context.installation, a.request.context.timer, a.admitted.actor, a.admitted.grant};
}

struct Rig final : ScheduleAdmissionPort, ScheduleAccessPort, ScheduleClockPort,
                   SchedulePreviewPort, ScheduleWriterPort {
    Zone zone;
    SchedulePorts ports{*this, *this, *this, zone, *this, *this};
    ScheduleContext context{"installation", "timer", "session"};
    SchedulePermit permit{"X", "G", "permission-1"};
    ScheduleAccess use = permit, snapshot_use = permit;
    std::map<std::string, ScheduleAdmission> admissions;
    std::map<std::string, Scope> selectors;
    std::map<std::string, SchedulePreviewRecord> issues;
    std::map<std::string, SchedulePresentedRecord> shown;
    std::vector<SchedulePublication> publications;
    std::vector<std::string> calls;
    unsigned serial = 0;
    bool fail_release = false;
    bool deny_candidate = false;
    Outcome write_result = Outcome::acknowledged;
    int writer_calls = 0, candidate_checks = 0;
    std::optional<ScheduleCandidate> candidate;
    std::vector<ScheduledPauseEpoch> epochs;
    ScheduledPauseLedger ledger{"L", "ledger", {}};
    ScheduledPauseState state;
    ScheduleClock clock;
    PauseState independent;
    explicit Rig(ScheduledPauseConfiguration config = old_rules, PauseState pause = manual)
        : epochs{{std::move(config), {}, {}, {}}},
          state{.snapshot_ref = "ready", .timer_lineage = "L", .clock_lineage = "L",
                .state_revision = "r-ready", .configuration_revision = "config",
                .timer_config = {.end_mode = TimerEndMode::Immediate, .hard_end_utc_ms = {}},
                .timer = {.lifecycle = TimerLifecycle::Ready, .main_remaining_ms = 900'000},
                .pause = pause, .cursor = {observation(at(11h)), {}, {}},
                .epochs = epochs, .epoch_history_complete = true, .ledger = &ledger},
          independent(std::move(pause)) {}
    std::string next(std::string prefix) { return prefix + std::to_string(++serial); }
    void target(Instant t, bool entry = false) {
        const auto anchor = state.cursor.last_observation;
        clock = {state.snapshot_ref, state.clock_lineage, anchor, observation(t), {}};
        if (!entry) {
            clock.cuts.push_back({clock.target, milliseconds{t - anchor.utc_ms}, independent});
        }
    }
    void install(const ScheduledPauseProgress &p) {
        state.timer = p.timer;
        state.pause = p.pause;
        state.cursor = p.cursor;
        ledger.deliveries.insert(ledger.deliveries.end(), p.new_deliveries.begin(), p.new_deliveries.end());
        for (const auto &stop : p.stops) {
            const auto i = std::ranges::find(ledger.deliveries, stop.key, &DeliveredScheduledPause::key);
            if (i == ledger.deliveries.end()) {
                throw std::logic_error("stop without delivery in fixture");
            }
            i->stop = stop.stop;
        }
        state.snapshot_ref = next("snapshot");
        state.state_revision = next("revision");
        if (!p.new_deliveries.empty() || !p.stops.empty()) {
            ledger.revision = next("ledger");
        }
    }
    void enter(Instant t) {
        install(std::get<ScheduledPauseProgress>(enter_scheduled_pauses(
            state, {state.snapshot_ref, observation(t), active_timer()}, zone)));
    }
    void advance(Instant t) {
        target(t);
        install(std::get<ScheduledPauseProgress>(progress_scheduled_pauses(
            state, {state.snapshot_ref, state.clock_lineage, clock.anchor, clock.cuts}, zone)));
    }
    void basis_s() {
        enter(at(11h + 40min));
        advance(at(11h + 45min));
        advance(at(11h + 50min));
        target(noon);
        EXPECT_EQ(state.timer.main_remaining_ms, 900'000);
        EXPECT_EQ(state.pause.reasons, manual_schedule);
        EXPECT_EQ(ledger.deliveries.size(), 2U);
    }
    void basis_e() {
        enter(noon - 1);
        target(noon);
        EXPECT_TRUE(ledger.deliveries.empty());
        EXPECT_EQ(state.timer.main_remaining_ms, 900'000);
    }
    std::variant<ScheduleCommandRef, Refusal> admit(const ScheduleRequest &r) override {
        ScheduleCommandRef ref{next("command")};
        std::optional<TimerState> entry;
        if (r.kind == Kind::start || r.kind == Kind::reopen) {
            entry = active_timer();
        }
        admissions.emplace(ref.value, ScheduleAdmission{ref, r, permit, clock.target, entry});
        return ref;
    }
    std::optional<ScheduleAdmission> active(const ScheduleCommandRef &ref) override {
        calls.push_back("active");
        const auto it = admissions.find(ref.value);
        return it == admissions.end() ? std::nullopt : std::optional{it->second};
    }
    ScheduleAccess command(const ScheduleRequest &, const ScheduleCandidate *p) override {
        calls.push_back(p ? "candidate USE" : "command USE");
        candidate_checks += p != nullptr;
        return p && deny_candidate ? ScheduleAccess{ScheduleDenied{}} : use;
    }
    ScheduleAccess snapshot(const ScheduleContext &) override {
        calls.push_back("snapshot USE");
        return snapshot_use;
    }
    std::variant<ScheduleClock, Refusal> observe(const ScheduleAdmission &, const ScheduledPauseState &) override {
        calls.push_back("clock");
        return clock;
    }
    std::variant<ScheduleIssuedRef, Refusal> issue(SchedulePreviewRecord r) override {
        calls.push_back("issue");
        const ScheduleIssuedRef ref{next("issued")};
        r.issued = ref;
        selectors.emplace(ref.value, scope(r.issuer));
        issues.emplace(ref.value, std::move(r));
        return ref;
    }
    bool scoped(const ScheduleAdmission &a, const std::string &ref) const {
        const auto i = selectors.find(ref);
        return i != selectors.end() && i->second == scope(a);
    }
    Lookup<SchedulePreviewRecord> issued(const ScheduleAdmission &a, const ScheduleIssuedRef &r) override {
        calls.push_back("issued scope");
        if (!scoped(a, r.value)) {
            return ScheduleScopeUnproved{};
        }
        calls.push_back("issued payload");
        const auto i = issues.find(r.value);
        if (i == issues.end()) {
            return ScheduleProofMissing{};
        }
        return i->second;
    }
    Lookup<SchedulePresentedRecord> presented(const ScheduleAdmission &a, const ScheduleConfirmation &c) override {
        calls.push_back("presented scope");
        if (!scoped(a, c.issued.value) || (c.shown && !scoped(a, c.shown->value))) {
            return ScheduleScopeUnproved{};
        }
        calls.push_back("presented payload");
        if (!c.shown || !shown.contains(c.shown->value)) {
            return ScheduleProofMissing{};
        }
        return shown.at(c.shown->value);
    }
    std::variant<SchedulePublication, Refusal> release(const ScheduleAdmission &a,
            const SchedulePreviewRecord &r, SchedulePublication publication) override {
        calls.push_back("release");
        if (fail_release) {
            return Refusal::fresh_preview_required;
        }
        publication.confirm.shown = SchedulePresentedRef{next("shown")};
        selectors.emplace(publication.confirm.shown->value, scope(a));
        shown.emplace(publication.confirm.shown->value, SchedulePresentedRecord{r, publication});
        publications.push_back(publication);
        return publication;
    }
    ScheduledPauseState live(const ScheduleContext &) override {
        calls.push_back("live");
        return state;
    }
    ScheduleWriteOutcome commit(const ScheduleAdmission &, const ScheduleCandidate &p) override {
        calls.push_back("commit");
        ++writer_calls;
        candidate = p;
        if (write_result != Outcome::not_committed) {
            if (const auto *edit = std::get_if<ScheduledPauseCutover>(&p)) {
                epochs.back().through_utc_ms = edit->cutover.utc_ms;
                epochs.back().closed_zone_rules_revision = edit->closed_zone_rules_revision;
                epochs.push_back({edit->configuration, edit->cutover.utc_ms, {}, {}});
                state.epochs = epochs;
                state.configuration_revision = next("config");
                install(edit->progress);
            } else {
                install(std::get<ScheduledPauseProgress>(p));
            }
        }
        return write_result;
    }
    ScheduleCommandRef command_ref(Kind kind, decltype(ScheduleRequest::intent) intent) {
        return std::get<ScheduleCommandRef>(admit(
            {context, kind, {state.snapshot_ref, state.state_revision}, std::move(intent)}));
    }
    ScheduleCommandRef preview(ScheduleEdit edit) { return command_ref(Kind::preview, std::move(edit)); }
    ScheduleCommandRef save(ScheduleEdit edit, ScheduleConfirmation confirm) {
        return command_ref(Kind::save, ScheduleSave{std::move(edit), std::move(confirm)});
    }
    std::optional<SchedulePublication> publish(const ScheduleEdit &edit) {
        const auto ref = preview(edit);
        const auto issued_reply = issue_schedule_preview(ref, ports);
        const auto *issued_ref = std::get_if<ScheduleIssuedRef>(&issued_reply);
        if (!issued_ref) {
            ADD_FAILURE() << "issue refused before presentation, variant " << issued_reply.index();
            return {};
        }
        const auto release_reply = present_schedule_preview(ref, *issued_ref, ports);
        const auto *publication = std::get_if<SchedulePublication>(&release_reply);
        if (!publication) {
            ADD_FAILURE() << "presentation refused";
            return {};
        }
        EXPECT_EQ(publication->confirm.issued.value, issued_ref->value);
        EXPECT_TRUE(publication->confirm.shown);
        EXPECT_EQ(publication->through_utc_ms, edit.through_utc_ms);
        return *publication;
    }
};

auto apply(Rig &r, Kind kind, const ScheduleCommandRef &ref) {
    return kind == Kind::save ? apply_schedule_save(ref, r.ports)
                              : apply_schedule_entry(kind, ref, r.ports);
}

void call_refusal(Rig &r, Kind kind, const ScheduleCommandRef &ref, Refusal refusal) {
    if (kind == Kind::preview) {
        expect(issue_schedule_preview(ref, r.ports), refusal);
    } else {
        expect(apply(r, kind, ref), refusal);
    }
}

auto image(const Rig &r) {
    // Snapshot values plus the identity of every owned revision and publication.
    return std::tuple{r.state.snapshot_ref, r.state.state_revision, r.state.configuration_revision,
                      r.ledger.revision, r.state.timer.lifecycle, r.state.timer.main_remaining_ms,
                      r.state.cursor.last_observation.observation_ref, r.state.cursor.last_observation.utc_ms,
                      r.state.cursor.evaluated_through_utc_ms, r.state.cursor.hold.has_value(),
                      r.state.pause.reasons, r.epochs.size(), r.ledger.deliveries.size(),
                      r.publications.size(), r.shown.size(), r.issues.size()};
}

void refuse_save(Rig &r, const ScheduleCommandRef &ref, Refusal refusal) {
    const auto before = image(r);
    expect(apply_schedule_save(ref, r.ports), refusal);
    EXPECT_EQ(r.writer_calls, 0);
    EXPECT_EQ(image(r), before);
}

void expect_publication_s(const SchedulePublication &p, Instant horizon, std::size_t rows) {
    EXPECT_FALSE(p.incomplete);
    EXPECT_FALSE(p.notice);
    EXPECT_EQ(p.certified_through_utc_ms, horizon);
    EXPECT_EQ(p.as_of.utc_ms, noon);
    EXPECT_EQ(p.as_of.observation_ref, observation(noon).observation_ref);
    ASSERT_TRUE(p.pause_after_cutover);
    EXPECT_EQ(p.pause_after_cutover->reasons, manual_schedule);
    ASSERT_EQ(p.upcoming.size(), rows);
    for (std::size_t i = 0; i < rows; ++i) {
        const auto start = at(days{i + 1} + 11h + 50min);
        const auto end = start + 1'200'000;
        const auto nominal = ScheduleLocalTime{milliseconds{start}};
        const ScheduledPauseKey key{"L", "B", {nominal, ScheduleFoldRun::First}};
        EXPECT_EQ(p.upcoming[i].key, key);
        EXPECT_EQ(p.upcoming[i].window,
                  (ScheduledPauseOccurrence{{key.occurrence, start, nominal}, end,
                                             ScheduleLocalTime{milliseconds{end}}}));
    }
    ASSERT_EQ(p.effects.size(), 2U);
    EXPECT_EQ(p.effects[0].key.schedule_id, "A");
    EXPECT_EQ(p.effects[0].captured.start_utc_ms, at(11h + 45min));
    EXPECT_EQ(p.effects[0].captured.end_utc_ms, at(12h + 15min));
    EXPECT_EQ(p.effects[0].stop_at_save, ScheduledPauseStopCause::deleted);
    EXPECT_EQ(p.effects[1].key.schedule_id, "B");
    EXPECT_EQ(p.effects[1].captured.start_utc_ms, at(11h + 50min));
    EXPECT_EQ(p.effects[1].captured.end_utc_ms, at(12h + 10min));
    EXPECT_FALSE(p.effects[1].stop_at_save);
}

TEST(ScheduleCommand, B1eAbsentInactiveAndWrongKindPrecedeAllWork) {
    Rig r;
    r.target(noon, true);
    const auto preview = r.preview({keep_b, h1});
    const auto save = r.save({keep_b, h1}, {{"issued"}, {}, "basis"});
    const auto start = r.command_ref(Kind::start, ScheduleEntrySelection{"start-intent"});
    const auto reopen = r.command_ref(Kind::reopen, ScheduleEntrySelection{"reopen-intent"});
    const auto inactive = r.preview({keep_b, h1});
    r.admissions.erase(inactive.value);
    for (const auto &bad : {ScheduleCommandRef{"absent"}, inactive, start}) {
        call_refusal(r, Kind::preview, bad, Refusal::not_admitted);
        expect(present_schedule_preview(bad, {"unused"}, r.ports), Refusal::not_admitted);
    }
    for (const auto kind : {Kind::save, Kind::start, Kind::reopen}) {
        const auto wrong = kind == Kind::start ? reopen : kind == Kind::reopen ? start : preview;
        const auto inactive_kind = kind == Kind::start ? start : kind == Kind::reopen ? reopen : save;
        for (const auto lifecycle : {TimerLifecycle::Ready, TimerLifecycle::Finished}) {
            r.state.timer.lifecycle = lifecycle;
            call_refusal(r, kind, {"absent"}, Refusal::not_admitted);
            call_refusal(r, kind, wrong, Refusal::not_admitted);
        }
        const auto saved = r.admissions.extract(inactive_kind.value);
        call_refusal(r, kind, inactive_kind, Refusal::not_admitted);
        r.admissions.emplace(inactive_kind.value, saved.mapped());
    }
    EXPECT_EQ(r.calls.size(), 21U);
    EXPECT_TRUE(std::ranges::all_of(r.calls, [](const auto &c) { return c == "active"; }));
    EXPECT_EQ(r.zone.reads, 0);
    EXPECT_TRUE(r.zone.pages.empty());
}

TEST(ScheduleCommand, B1gDenialAndUnknownWinAtIssuePresentAndSave) {
    for (const auto &access : {ScheduleAccess{ScheduleDenied{}}, ScheduleAccess{ScheduleUnknown{}}}) {
        Rig r;
        r.basis_s();
        const auto issue_ref = r.preview({keep_b, h1});
        const auto proof = r.publish({keep_b, h1});
        const auto save = r.save({keep_b, h2}, proof ? proof->confirm :
                                 ScheduleConfirmation{{"unused"}, {}, "wrong"});
        r.use = access;
        r.state.snapshot_ref = "stale";
        r.zone.fail_zero = true;
        r.calls.clear();
        r.zone.reads = 0;
        r.zone.pages.clear();
        expect(issue_schedule_preview(issue_ref, r.ports), Refusal::access_denied);
        expect(present_schedule_preview(issue_ref, {"foreign"}, r.ports), Refusal::access_denied);
        refuse_save(r, save, Refusal::access_denied);
        EXPECT_EQ(r.calls, (std::vector<std::string>{"active", "command USE", "active",
                                                   "command USE", "active", "command USE"}));
        EXPECT_EQ(r.zone.reads, 0);
        EXPECT_TRUE(r.zone.pages.empty());
    }
}

class ScheduleHorizonCase : public ::testing::TestWithParam<bool> {};
TEST_P(ScheduleHorizonCase, B1fExactHorizonAndCrossedSelectors) {
    const bool same_day = GetParam();
    Rig r;
    r.basis_s();
    const ScheduleEdit first{keep_b, same_day ? noon + 60'000 : h1};
    const ScheduleEdit second{keep_b, same_day ? noon + 120'000 : h2};
    const auto p = r.publish(first);
    const auto q = r.publish(second);
    ASSERT_TRUE(p && q);
    expect_publication_s(*p, first.through_utc_ms, same_day ? 0 : 1);
    expect_publication_s(*q, second.through_utc_ms, same_day ? 0 : 2);
    EXPECT_NE(p->confirm.issued.value, q->confirm.issued.value);
    ASSERT_TRUE(p->confirm.shown && q->confirm.shown);
    EXPECT_NE(p->confirm.shown->value, q->confirm.shown->value);
    refuse_save(r, r.save(first, q->confirm), Refusal::fresh_preview_required);
    refuse_save(r, r.save(first, {p->confirm.issued, q->confirm.shown, q->confirm.basis}),
                Refusal::fresh_preview_required);
}
INSTANTIATE_TEST_SUITE_P(ScheduleCommand, ScheduleHorizonCase,
                         ::testing::Values(false, true));

TEST(ScheduleCommand, StaleIssueRetainsNoProof) {
    Rig r;
    r.basis_s();
    const auto ref = r.preview({keep_b, h1});
    r.state.snapshot_ref = "new-snapshot";
    expect(issue_schedule_preview(ref, r.ports), Refusal::stale_observation);
    EXPECT_TRUE(r.issues.empty());
    EXPECT_TRUE(r.publications.empty());
}

enum class PresentationChange { basis, projection, foreign, evicted };
class SchedulePresentationCase : public ::testing::TestWithParam<PresentationChange> {};
TEST_P(SchedulePresentationCase, RechecksIdentityAndNeverPublishesRefusal) {
    const auto change = GetParam();
    Rig r;
    r.basis_s();
    const ScheduleEdit edit{keep_b, h1};
    const auto ref = r.preview(edit);
    const auto issuance = issue_schedule_preview(ref, r.ports);
    const auto *issued = std::get_if<ScheduleIssuedRef>(&issuance);
    ASSERT_NE(issued, nullptr);
    if (change == PresentationChange::basis) {
        r.advance(at(11h + 51min));
        r.target(noon);
    }
    if (change == PresentationChange::projection) r.zone.fixed_revision = "Z2";
    if (change == PresentationChange::evicted) r.issues.erase(issued->value);
    r.calls.clear();
    const bool foreign = change == PresentationChange::foreign;
    expect(present_schedule_preview(r.preview(edit), foreign ? ScheduleIssuedRef{"foreign"} : *issued,
                                    r.ports),
           foreign ? Refusal::access_denied : Refusal::fresh_preview_required);
    if (foreign) {
        EXPECT_EQ(std::ranges::find(r.calls, "issued payload"), r.calls.end());
    }
    EXPECT_TRUE(r.shown.empty());
    EXPECT_TRUE(r.publications.empty());
    EXPECT_EQ(r.writer_calls, 0);
}
INSTANTIATE_TEST_SUITE_P(ScheduleCommand, SchedulePresentationCase,
                         ::testing::Values(PresentationChange::basis, PresentationChange::projection,
                                           PresentationChange::foreign, PresentationChange::evicted));

TEST(ScheduleCommand, ProvenanceAndEviction) {
    Rig r;
    r.basis_s();
    const ScheduleEdit edit{keep_b, h1};
    const auto ref = r.preview(edit);
    const auto result = issue_schedule_preview(ref, r.ports);
    const auto *issued = std::get_if<ScheduleIssuedRef>(&result);
    ASSERT_NE(issued, nullptr);
    const ScheduleConfirmation unshown{*issued, {}, r.issues.at(issued->value).basis};
    refuse_save(r, r.save(edit, unshown), Refusal::fresh_preview_required);
    r.fail_release = true;
    EXPECT_TRUE((std::holds_alternative<Refusal>(present_schedule_preview(ref, *issued, r.ports))));
    EXPECT_TRUE(r.shown.empty());
    EXPECT_TRUE(r.publications.empty());
    refuse_save(r, r.save(edit, unshown), Refusal::fresh_preview_required);
    r.fail_release = false;
    const auto p = r.publish(edit);
    ASSERT_TRUE(p && p->confirm.shown);
    const auto good = p->confirm;
    for (const std::string field : {"actor", "grant", "timer", "proposal", "forged", "key loss"}) {
        SCOPED_TRACE(field);
        auto c = good;
        auto proposed = edit;
        const auto saved_scope = r.permit;
        if (field == "actor") r.permit.actor = "Y";
        if (field == "grant") r.permit.grant = "G2";
        if (field == "timer") r.context.timer = "other timer";
        if (field == "proposal") proposed.proposed = empty;
        if (field == "forged") c.shown = SchedulePresentedRef{"forged"};
        if (field == "key loss") r.selectors.clear();
        r.use = r.permit;
        r.calls.clear();
        refuse_save(r, r.save(proposed, c), Refusal::access_denied);
        if (field != "proposal") {
            EXPECT_EQ(std::ranges::find(r.calls, "presented payload"), r.calls.end());
        }
        r.permit = saved_scope;
        r.use = saved_scope;
        r.context.timer = "timer";
    }
    r.selectors.emplace(good.issued.value, scope(r.admissions.at(ref.value)));
    r.selectors.emplace(good.shown->value, scope(r.admissions.at(ref.value)));
    r.shown.erase(good.shown->value);
    refuse_save(r, r.save(edit, good), Refusal::fresh_preview_required);
}

TEST(ScheduleCommand, B1hStaleClockAndLaterInvalidProof) {
    Rig r;
    r.basis_s();
    const ScheduleEdit edit{keep_b, h1};
    const auto p = r.publish(edit);
    ASSERT_TRUE(p);
    const auto save = r.save(edit, p->confirm);
    const auto trusted = r.clock;
    r.clock.anchor.observation_ref = "wrong anchor";
    refuse_save(r, save, Refusal::stale_observation);
    r.clock = trusted;
    r.clock.target.observation_ref = "wrong target";
    refuse_save(r, save, Refusal::stale_observation);
    r.clock = trusted;
    r.clock.target = r.clock.anchor;
    refuse_save(r, r.save(edit, p->confirm), Refusal::stale_observation);
    r.advance(at(11h + 51min));
    r.target(noon);
    refuse_save(r, save, Refusal::stale_observation);
    r.use = ScheduleDenied{};
    refuse_save(r, save, Refusal::access_denied);
    r.use = r.permit;
    refuse_save(r, r.save(edit, p->confirm), Refusal::fresh_preview_required);
}

TEST(ScheduleCommand, B1hChangedFactsAndExpiredRetainedEffect) {
    Rig r;
    r.basis_s();
    const ScheduleEdit edit{keep_b, h1};
    const auto p = r.publish(edit);
    ASSERT_TRUE(p);
    r.zone.fixed_revision = "Z2";
    refuse_save(r, r.save(edit, p->confirm), Refusal::fresh_preview_required);
    r.zone.fixed_revision = "Z1";
    r.target(at(12h + 10min));
    refuse_save(r, r.save(edit, p->confirm), Refusal::fresh_preview_required);
    EXPECT_FALSE(r.ledger.deliveries[1].stop);
}

class ScheduleAtomicCase : public ::testing::TestWithParam<std::pair<milliseconds, Outcome>> {};
TEST_P(ScheduleAtomicCase, AtomicCutoverAndWriterCertainty) {
    const auto [offset, outcome] = GetParam();
    Rig r;
    r.basis_s();
    const ScheduleEdit edit{keep_b, h1};
    const auto p = r.publish(edit);
    ASSERT_TRUE(p);
    const auto cut = noon + offset.count();
    r.target(cut);
    const auto save = r.save(edit, p->confirm);
    const auto competing = r.save(edit, p->confirm);
    const auto before = image(r);
    r.write_result = outcome;
    expect(apply_schedule_save(save, r.ports), outcome);
    ASSERT_EQ(r.writer_calls, 1);
    ASSERT_TRUE(r.candidate);
    EXPECT_GT(r.candidate_checks, 0);
    EXPECT_EQ(r.publications.size(), 1U);
    if (outcome == Outcome::not_committed) {
        EXPECT_EQ(image(r), before);
        EXPECT_FALSE(r.ledger.deliveries[0].stop);
        return;
    }
    ASSERT_EQ(r.epochs.size(), 2U);
    EXPECT_EQ(r.epochs[0].through_utc_ms, cut);
    EXPECT_EQ(r.epochs[0].closed_zone_rules_revision, "Z1");
    EXPECT_EQ(r.epochs[1].after_utc_ms, cut);
    ASSERT_EQ(r.epochs[1].configuration.rules.size(), 1U);
    EXPECT_EQ(r.epochs[1].configuration.rules[0].schedule_id, "B");
    EXPECT_EQ(r.state.timer.main_remaining_ms, 900'000);
    EXPECT_EQ(r.state.cursor.evaluated_through_utc_ms, cut);
    EXPECT_EQ(r.state.pause.reasons, manual_schedule);
    ASSERT_EQ(r.ledger.deliveries.size(), 2U);
    ASSERT_TRUE(r.ledger.deliveries[0].stop);
    const auto &stop = *r.ledger.deliveries[0].stop;
    EXPECT_EQ(stop.cause, ScheduledPauseStopCause::deleted);
    EXPECT_EQ(stop.effective_utc_ms, cut);
    EXPECT_EQ(stop.reconciled_at.observation_ref, observation(cut).observation_ref);
    EXPECT_FALSE(r.ledger.deliveries[1].stop);
    EXPECT_EQ(r.ledger.deliveries[1].window.end_utc_ms, at(12h + 10min));
    const auto &progress = std::get<ScheduledPauseCutover>(*r.candidate).progress;
    EXPECT_TRUE(progress.new_deliveries.empty());
    EXPECT_TRUE(progress.timer_transitions.empty());
    expect(apply_schedule_save(competing, r.ports), Refusal::stale_observation);
    EXPECT_EQ(r.writer_calls, 1);
}
INSTANTIATE_TEST_SUITE_P(ScheduleCommand, ScheduleAtomicCase,
                         ::testing::Values(std::pair{0ms, Outcome::acknowledged},
                                           std::pair{1000ms, Outcome::acknowledged},
                                           std::pair{0ms, Outcome::not_committed},
                                           std::pair{0ms, Outcome::outcome_unacknowledged}));

class ScheduleSameCutCase : public ::testing::TestWithParam<Outcome> {};
TEST_P(ScheduleSameCutCase, SameCutDueDeliveryAndDisableStop) {
    const auto outcome = GetParam();
    Rig r({"Fixed/Zero", {rule("A", 12h, 20min)}}, {});
    r.basis_e();
    const ScheduleEdit edit{{"Fixed/Zero", {rule("A", 12h, 20min, false)}}, at(36h)};
    const auto p = r.publish(edit);
    ASSERT_TRUE(p);
    ASSERT_EQ(p->effects.size(), 1U);
    EXPECT_EQ(p->effects[0].stop_at_save, ScheduledPauseStopCause::disabled);
    EXPECT_EQ(p->effects[0].captured.start_utc_ms, noon);
    EXPECT_EQ(p->effects[0].captured.end_utc_ms, at(12h + 20min));
    r.write_result = outcome;
    const auto before = image(r);
    expect(apply_schedule_save(r.save(edit, p->confirm), r.ports), outcome);
    if (outcome == Outcome::not_committed) {
        EXPECT_EQ(image(r), before);
        return;
    }
    ASSERT_EQ(r.ledger.deliveries.size(), 1U);
    ASSERT_TRUE(r.ledger.deliveries[0].stop);
    EXPECT_EQ(r.ledger.deliveries[0].stop->cause, ScheduledPauseStopCause::disabled);
    EXPECT_EQ(r.ledger.deliveries[0].stop->effective_utc_ms, noon);
    EXPECT_EQ(r.state.timer.main_remaining_ms, 899'999);
    EXPECT_TRUE(r.state.pause.reasons.empty());
    ASSERT_TRUE(r.candidate);
    EXPECT_TRUE(std::get<ScheduledPauseCutover>(*r.candidate).progress.timer_transitions.empty());
}
INSTANTIATE_TEST_SUITE_P(ScheduleCommand, ScheduleSameCutCase,
                         ::testing::Values(Outcome::acknowledged, Outcome::not_committed));

class ScheduleEntryCase : public ::testing::TestWithParam<Kind> {};
TEST_P(ScheduleEntryCase, EntryRetainsExactWindowsAndChecksHardEndBeforeZone) {
    const auto kind = GetParam();
    Rig r;
    const auto deadline = at(12h + 4min);
    r.state.timer_config.hard_end_utc_ms = deadline;
    if (kind == Kind::reopen) {
        r.basis_s();
        r.advance(deadline);
        ASSERT_EQ(r.state.timer.lifecycle, TimerLifecycle::Finished);
        ASSERT_EQ(r.state.cursor.evaluated_through_utc_ms, at(11h + 50min));
    }
    r.target(at(12h + 5min), true);
    const auto entry = [&] { return r.command_ref(kind, ScheduleEntrySelection{"owning-intent"}); };
    r.zone.reads = 0;
    const auto permit = r.use;
    r.use = ScheduleDenied{};
    r.calls.clear();
    call_refusal(r, kind, entry(), Refusal::access_denied);
    EXPECT_EQ(r.calls, (std::vector<std::string>{"active", "command USE"}));
    r.use = permit;
    call_refusal(r, kind, entry(), Refusal::hard_end_reached);
    EXPECT_EQ(r.zone.reads, 0);
    EXPECT_EQ(r.writer_calls, 0);
    if (kind == Kind::reopen) {
        const auto revision = r.state.state_revision;
        const auto removal = project_timer_editorial_command({
            {revision, {r.state.timer, 0, {}, r.state.timer_config}},
            {"remove deadline", revision, "X", {}, TimerDeadlineReplacement{}},
            {TimerEditorialAccessState::allowed, "permission-1"},
            {{1, deadline + 1, true}}, deadline + 1});
        ASSERT_EQ(removal.data().disposition, TimerEditorialDisposition::accepted);
        ASSERT_TRUE(removal.data().accepted_after);
        r.advance(deadline + 1);
        r.state.timer = removal.data().accepted_after->timer;
        r.state.timer_config = removal.data().accepted_after->config;
    }
    r.state.timer_config.hard_end_utc_ms.reset();
    r.state.snapshot_ref = "deadline-removed";
    r.state.state_revision = "deadline-state";
    r.state.configuration_revision = "deadline-config";
    r.target(at(12h + 5min), true);
    for (const auto &access : {ScheduleAccess{ScheduleDenied{}}, ScheduleAccess{ScheduleUnknown{}}}) {
        r.use = access;
        const auto reads = r.zone.reads;
        r.calls.clear();
        call_refusal(r, kind, entry(), Refusal::access_denied);
        EXPECT_EQ(r.calls, (std::vector<std::string>{"active", "command USE"}));
        EXPECT_EQ(r.zone.reads, reads);
        EXPECT_EQ(r.writer_calls, 0);
    }
    r.use = permit;
    r.deny_candidate = true;
    const auto candidate_checks = r.candidate_checks;
    call_refusal(r, kind, entry(), Refusal::access_denied);
    EXPECT_GT(r.candidate_checks, candidate_checks);
    EXPECT_EQ(r.writer_calls, 0);
    r.deny_candidate = false;
    expect(apply(r, kind, entry()), Outcome::acknowledged);
    ASSERT_TRUE(r.candidate);
    const auto &p = std::get<ScheduledPauseProgress>(*r.candidate);
    EXPECT_EQ(p.timer.lifecycle, TimerLifecycle::Active);
    EXPECT_EQ(p.timer.main_remaining_ms, 900'000);
    EXPECT_TRUE(p.pause.reasons.contains(PauseReason::Schedule));
    EXPECT_EQ(p.next_known_expiry_utc_ms, at(12h + 10min));
    EXPECT_EQ(p.new_deliveries.size(), kind == Kind::start ? 2U : 0U);
    EXPECT_TRUE(p.timer_transitions.empty());
    ASSERT_EQ(r.ledger.deliveries.size(), 2U);
    EXPECT_EQ(r.ledger.deliveries[0].window.start_utc_ms, at(11h + 45min));
    EXPECT_EQ(r.ledger.deliveries[0].window.end_utc_ms, at(12h + 15min));
    EXPECT_EQ(r.ledger.deliveries[1].window.start_utc_ms, at(11h + 50min));
    EXPECT_EQ(r.ledger.deliveries[1].window.end_utc_ms, at(12h + 10min));
}
INSTANTIATE_TEST_SUITE_P(ScheduleCommand, ScheduleEntryCase,
                         ::testing::Values(Kind::start, Kind::reopen));

TEST(ScheduleCommand, HeldEntryNoticeRequiresSnapshotUseAndSnapshotUseCannotPreview) {
    Rig r(old_rules, {});
    const auto cut = at(12h + 5min);
    r.target(cut, true);
    r.zone.fail_zero = true;
    expect(apply(r, Kind::start, r.command_ref(Kind::start, ScheduleEntrySelection{"start-intent"})),
           Outcome::acknowledged);
    EXPECT_EQ(r.state.timer.main_remaining_ms, 900'000);
    EXPECT_FALSE(r.state.cursor.evaluated_through_utc_ms);
    EXPECT_TRUE(r.ledger.deliveries.empty());
    const auto notice = disclose_schedule_notice(r.context, r.ports);
    ASSERT_TRUE(notice);
    expect(notice->hold.cause, ZoneError::insufficient_coverage);
    EXPECT_EQ(notice->hold.from_utc_ms, cut);
    EXPECT_EQ(notice->observed_at.observation_ref, observation(cut).observation_ref);
    r.calls.clear();
    r.snapshot_use = ScheduleDenied{};
    EXPECT_FALSE(disclose_schedule_notice(r.context, r.ports));
    EXPECT_EQ(r.calls, (std::vector<std::string>{"snapshot USE"}));
    r.snapshot_use = r.permit;
    r.use = ScheduleDenied{};
    expect(present_schedule_preview(r.preview({keep_b, h1}), {"unused"}, r.ports),
           Refusal::access_denied);
}

TEST(ScheduleCommand, DistinguishesUnavailableOldEffectsFromFutureOnlyGap) {
    Rig r;
    r.basis_s();
    const ScheduleEdit deletion{empty, h1};
    const auto gone = r.publish(deletion);
    ASSERT_TRUE(gone);
    r.zone.fail_zero = true;
    refuse_save(r, r.save(deletion, gone->confirm), Refusal::cutover_uncertified);
    r.zone.fail_zero = false;
    const ScheduleEdit other{{"Fixed/Other", {rule_b}}, h1};
    const auto complete = r.publish(other);
    ASSERT_TRUE(complete);
    r.zone.other_end = at(60h);
    refuse_save(r, r.save(other, complete->confirm), Refusal::forecast_incomplete);
    const auto prefix = r.publish(other);
    ASSERT_TRUE(prefix);
    EXPECT_TRUE(prefix->incomplete);
    EXPECT_TRUE(prefix->notice);
    EXPECT_EQ(prefix->certified_through_utc_ms, noon);
    refuse_save(r, r.save(other, prefix->confirm), Refusal::forecast_incomplete);
    r.zone.other_end = h1 + duration_cast<milliseconds>(48h).count();
    const auto restored = r.publish(other);
    ASSERT_TRUE(restored);
    EXPECT_EQ(restored->certified_through_utc_ms, h1);
}

struct DenseCase {
    DenseTail shown, replay;
    const char *revision;
    Refusal refusal;
    bool snapshot_allowed = true, old_fails = false;
};
constexpr auto complexity = Refusal::beyond_supported_timezone_transitions;
constexpr auto incomplete = Refusal::forecast_incomplete;
class DenseSave : public testing::TestWithParam<DenseCase> {};
TEST_P(DenseSave, ExactHorizonAndAuthenticatedSupportBreach) {
    const auto [shown_tail, replay, rev, refusal, allowed, old_fails] = GetParam();
    Rig r(old_fails ? ScheduledPauseConfiguration{"Fixed/Zero", {rule("old", 15h, 1ms)}} : empty, {});
    r.basis_e();
    r.zone.tail = shown_tail;
    r.zone.dense_revision = shown_tail == DenseTail::complete ? "r1" : "r2";
    if (!allowed) r.snapshot_use = ScheduleDenied{};
    const ScheduleEdit edit{dense, dense_h};
    const auto p = r.publish(edit);
    ASSERT_TRUE(p);
    EXPECT_EQ(p->incomplete, shown_tail != DenseTail::complete);
    EXPECT_EQ(p->notice.has_value(), allowed && p->incomplete);
    if (p->incomplete) EXPECT_EQ(p->certified_through_utc_ms, allowed ? std::optional{noon} : std::nullopt);
    else EXPECT_EQ(p->certified_through_utc_ms, dense_h);
    r.zone.tail = replay;
    r.zone.dense_revision = rev;
    r.zone.fail_zero = old_fails;
    r.zone.pages.clear();
    refuse_save(r, r.save(edit, p->confirm), refusal);
    if (!old_fails && shown_tail != DenseTail::terminal) {
        EXPECT_TRUE(r.zone.pages.size() == 64U || r.zone.pages.size() == 128U);
    }
    EXPECT_LE(r.zone.pages.size(), 128U);
    for (std::size_t i = 0; i < r.zone.pages.size(); ++i) {
        const auto expected_start = i % 64 == 0 ? at(-36h) : at(12h + days{3} + hours{128 * (i % 64)}) - 1;
        EXPECT_EQ(r.zone.pages[i].from_utc_ms, expected_start);
        EXPECT_EQ(r.zone.pages[i].through_utc_ms, at(12h + days{366}) + 1);
    }
}
INSTANTIATE_TEST_SUITE_P(Owner179AndFrozenSave, DenseSave, testing::Values(
    DenseCase{DenseTail::cap, DenseTail::cap, "r2", complexity},
    DenseCase{DenseTail::cap, DenseTail::cap, "r2", complexity, false},
    DenseCase{DenseTail::cap, DenseTail::terminal, "r2", incomplete},
    DenseCase{DenseTail::terminal, DenseTail::cap, "r2", incomplete},
    DenseCase{DenseTail::cap, DenseTail::complete, "r1", incomplete},
    DenseCase{DenseTail::complete, DenseTail::cap, "r2", complexity, false},
    DenseCase{DenseTail::complete, DenseTail::terminal, "r2", incomplete},
    DenseCase{DenseTail::complete, DenseTail::cap, "r1", incomplete},
    DenseCase{DenseTail::complete, DenseTail::cap, "r2", Refusal::cutover_uncertified, true, true},
    DenseCase{DenseTail::complete, DenseTail::complete, "r3", Refusal::fresh_preview_required}));

TEST(ScheduleCommand, CrossedHorizonRefusesBeforeProjectionAndEqualityReachesProjection) {
    Rig r(empty, {});
    r.basis_e();
    const ScheduleEdit edit{dense, dense_h};
    const auto p = r.publish(edit);
    ASSERT_TRUE(p);
    r.target(dense_h + 1);
    r.zone.pages.clear();
    r.zone.reads = 0;
    refuse_save(r, r.save(edit, p->confirm), Refusal::fresh_preview_required);
    EXPECT_TRUE(r.zone.pages.empty());
    EXPECT_EQ(r.zone.reads, 0);
    r.target(dense_h);
    refuse_save(r, r.save(edit, p->confirm), Refusal::fresh_preview_required);
    EXPECT_FALSE(r.zone.pages.empty());
    r.target(noon);
    r.deny_candidate = true;
    refuse_save(r, r.save(edit, p->confirm), Refusal::access_denied);
    EXPECT_GT(r.candidate_checks, 0);
    r.deny_candidate = false;
    expect(apply_schedule_save(r.save(edit, p->confirm), r.ports), Outcome::acknowledged);
}
} // namespace
