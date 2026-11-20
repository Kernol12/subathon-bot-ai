// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/scheduled_pause.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <set>
#include <string_view>
#include <utility>

namespace subathon::server {
namespace {

using Instant = std::int64_t;
using Limits = ScheduledPauseLimits;
using Failure = ScheduledPauseCoverageFailure;
using Refusal = ScheduledPauseRefusal;
using ZoneError = ScheduleZoneUnavailable;
using Knowledge = ScheduledPauseKnowledgeUnavailable;
using Lifecycle = core::TimerLifecycle;
using Run = core::ScheduleFoldRun;
using namespace std::chrono;

constexpr auto first_ms = std::numeric_limits<Instant>::min();
constexpr auto last_ms = std::numeric_limits<Instant>::max();
constexpr auto day_ms = milliseconds{days{1}}.count();
constexpr auto padding = 2 * day_ms;
constexpr auto max_duration = milliseconds{Limits::max_duration}.count();
constexpr auto core_span = milliseconds{core::schedule_preview_max_span}.count();
constexpr auto scan_span = milliseconds{Limits::max_progression_span}.count();

std::optional<Instant> add(Instant point, Instant delta) {
    if ((delta > 0 && point > last_ms - delta) || (delta < 0 && point < first_ms - delta)) {
        return {};
    }
    return point + delta;
}

std::uint64_t distance(Instant lower, Instant upper) {
    return static_cast<std::uint64_t>(upper) - static_cast<std::uint64_t>(lower);
}

struct Bytes {
    std::size_t left;

    bool take(std::size_t amount) {
        if (amount > left) {
            return false;
        }
        left -= amount;
        return true;
    }
};

bool reference(Bytes &bytes, std::string_view value) {
    return value.size() <= Limits::max_reference_bytes && bytes.take(value.size());
}

std::size_t config_strings(const ScheduledPauseConfiguration &config) {
    auto bytes = config.configured_zone.size();
    for (const auto &rule : config.rules) {
        bytes += rule.schedule_id.size();
    }
    return bytes;
}

bool bounded(Bytes &bytes, const ScheduledPauseConfiguration &config) {
    return config.rules.size() <= Limits::max_rules &&
           bytes.take(config.rules.size() * sizeof(ScheduledPauseRule)) &&
           reference(bytes, config.configured_zone) &&
           std::ranges::all_of(config.rules, [&](const auto &rule) {
               return reference(bytes, rule.schedule_id) && rule.duration.count() <= max_duration;
           });
}

bool bounded(Bytes &bytes, const ScheduledPauseState &state) {
    if (state.epochs.size() > Limits::max_epochs ||
        (state.ledger && state.ledger->deliveries.size() > Limits::max_deliveries)) {
        return false;
    }
    if (!bytes.take(sizeof(state) + state.epochs.size() * sizeof(ScheduledPauseEpoch)) ||
        !reference(bytes, state.snapshot_ref) || !reference(bytes, state.timer_lineage) ||
        !reference(bytes, state.clock_lineage) || !reference(bytes, state.state_revision) ||
        !reference(bytes, state.configuration_revision) ||
        !reference(bytes, state.cursor.last_observation.observation_ref) ||
        state.pause.reasons.size() > 6 ||
        !bytes.take(state.pause.reasons.size() * sizeof(core::PauseReason))) {
        return false;
    }
    for (const auto &epoch : state.epochs) {
        if (!bounded(bytes, epoch.configuration) ||
            (epoch.closed_zone_rules_revision &&
             !reference(bytes, *epoch.closed_zone_rules_revision))) {
            return false;
        }
    }
    if (!state.ledger) {
        return true;
    }
    const auto &ledger = *state.ledger;
    if (!bytes.take(sizeof(ledger) + ledger.deliveries.size() * sizeof(DeliveredScheduledPause)) ||
        !reference(bytes, ledger.timer_lineage) || !reference(bytes, ledger.revision)) {
        return false;
    }
    return std::ranges::all_of(ledger.deliveries, [&](const auto &delivery) {
        return reference(bytes, delivery.key.timer_lineage) &&
               reference(bytes, delivery.key.schedule_id) &&
               (!delivery.stop || reference(bytes, delivery.stop->reconciled_at.observation_ref));
    });
}

bool bounded(Bytes &bytes, const ScheduledPauseClockInput &clock) {
    return clock.cuts.size() <= Limits::max_clock_cuts &&
           bytes.take(sizeof(clock) + clock.cuts.size() * sizeof(ScheduledPauseElapsedCut)) &&
           reference(bytes, clock.source_snapshot_ref) && reference(bytes, clock.clock_lineage) &&
           reference(bytes, clock.anchor.observation_ref) &&
           std::ranges::all_of(clock.cuts, [&](const auto &cut) {
               return cut.independent_pause.reasons.size() <= 6 &&
                      bytes.take(cut.independent_pause.reasons.size() *
                                 sizeof(core::PauseReason)) &&
                      reference(bytes, cut.observation.observation_ref);
           });
}

bool valid_timer(const core::TimerState &timer, const core::TimerConfig &config) {
    using enum core::TimerEndMode;
    using Phase = core::CountdownPhase;
    return (timer.lifecycle == Lifecycle::Ready || timer.lifecycle == Lifecycle::Active ||
            timer.lifecycle == Lifecycle::Finished) &&
           (timer.phase == Phase::Main || timer.phase == Phase::Grace) &&
           (config.end_mode == Immediate || config.end_mode == Grace ||
            config.end_mode == Confirmation) &&
           (config.grace_reuse == core::GraceReuse::Fresh ||
            config.grace_reuse == core::GraceReuse::Shared) &&
           config.grace_duration_ms >= 0 && timer.main_remaining_ms >= 0 &&
           timer.grace_remaining_ms >= 0 &&
           (timer.phase != Phase::Grace || timer.main_remaining_ms == 0) &&
           (!timer.awaiting_confirmation ||
            (timer.lifecycle == Lifecycle::Active && timer.phase == Phase::Main &&
             timer.main_remaining_ms == 0 && config.end_mode == Confirmation));
}

bool valid_pause(const core::PauseState &pause, bool independent) {
    using enum core::PauseReason;
    const auto valid_pending = [&](std::optional<Instant> duration, core::PauseReason reason) {
        return !duration || (*duration >= 0 && !pause.reasons.contains(reason));
    };
    return std::ranges::all_of(pause.reasons,
                               [&](auto reason) {
                                   return reason >= Manual && reason <= StreamStatusUnknown &&
                                          (!independent || reason != Schedule);
                               }) &&
           valid_pending(pause.stream_offline_pending_ms, StreamOffline) &&
           valid_pending(pause.client_disconnect_pending_ms, DesignatedClientDisconnected) &&
           valid_pending(pause.stream_unknown_pending_ms, StreamStatusUnknown);
}

bool valid_config(const ScheduledPauseConfiguration &config) {
    if (config.configured_zone.empty() && !config.rules.empty()) {
        return false;
    }
    std::set<std::string_view> ids;
    return std::ranges::all_of(config.rules, [&](const auto &rule) {
        const auto &recurrence = rule.recurrence;
        return !rule.schedule_id.empty() && ids.insert(rule.schedule_id).second &&
               rule.duration > milliseconds::zero() &&
               recurrence.local_time >= milliseconds::zero() && recurrence.local_time < days{1} &&
               (recurrence.gap_policy == core::ScheduleGapPolicy::ShiftForward ||
                recurrence.gap_policy == core::ScheduleGapPolicy::Skip) &&
               (recurrence.fold_policy == core::ScheduleFoldPolicy::RunOnce ||
                recurrence.fold_policy == core::ScheduleFoldPolicy::RunTwice);
    });
}

using Slot = std::pair<core::ScheduleLocalTime, Run>;
using Keys = std::map<std::string_view, std::set<Slot>, std::less<>>;

Slot slot(const core::ScheduleOccurrenceId &id) { return {id.nominal_local_start, id.run}; }

bool valid_basis(const ScheduledPauseState &state) {
    if (state.snapshot_ref.empty() || state.timer_lineage.empty() || state.clock_lineage.empty() ||
        state.state_revision.empty() || state.configuration_revision.empty() ||
        state.cursor.last_observation.observation_ref.empty() ||
        state.cursor.evaluated_through_utc_ms > state.cursor.last_observation.utc_ms ||
        (state.cursor.hold &&
         state.cursor.hold->from_utc_ms > state.cursor.last_observation.utc_ms) ||
        !valid_pause(state.pause, false) || !valid_timer(state.timer, state.timer_config)) {
        return false;
    }
    for (std::size_t i = 0; i < state.epochs.size(); ++i) {
        const auto &epoch = state.epochs[i];
        const bool closed = i + 1 < state.epochs.size();
        if (!valid_config(epoch.configuration) || closed != epoch.through_utc_ms.has_value() ||
            (epoch.closed_zone_rules_revision &&
             (!closed || epoch.closed_zone_rules_revision->empty())) ||
            (epoch.through_utc_ms &&
             (*epoch.through_utc_ms > state.cursor.last_observation.utc_ms ||
              (epoch.after_utc_ms && *epoch.after_utc_ms >= *epoch.through_utc_ms))) ||
            (i > 0 &&
             (!epoch.after_utc_ms || state.epochs[i - 1].through_utc_ms > epoch.after_utc_ms ||
              (state.epoch_history_complete &&
               state.epochs[i - 1].through_utc_ms != epoch.after_utc_ms))) ||
            (epoch.after_utc_ms && *epoch.after_utc_ms > state.cursor.last_observation.utc_ms) ||
            (i == 0 && state.epoch_history_complete && epoch.after_utc_ms)) {
            return false;
        }
    }
    if (!state.ledger) {
        return true;
    }
    if (state.ledger->timer_lineage != state.timer_lineage || state.ledger->revision.empty()) {
        return false;
    }
    Keys seen;
    return std::ranges::all_of(state.ledger->deliveries, [&](const auto &delivery) {
        const auto &key = delivery.key;
        const auto &window = delivery.window;
        if (key.timer_lineage != state.timer_lineage || key.schedule_id.empty() ||
            (key.occurrence.run != Run::First && key.occurrence.run != Run::Second) ||
            window.start_utc_ms >= window.end_utc_ms ||
            distance(window.start_utc_ms, window.end_utc_ms) > max_duration ||
            window.start_utc_ms > state.cursor.last_observation.utc_ms ||
            !seen[key.schedule_id].insert(slot(key.occurrence)).second) {
            return false;
        }
        if (!delivery.stop) {
            return true;
        }
        const auto &stop = *delivery.stop;
        using enum ScheduledPauseStopCause;
        return !stop.reconciled_at.observation_ref.empty() &&
               stop.reconciled_at.utc_ms <= state.cursor.last_observation.utc_ms &&
               ((stop.cause == expired && stop.effective_utc_ms == window.end_utc_ms &&
                 stop.reconciled_at.utc_ms >= stop.effective_utc_ms) ||
                ((stop.cause == deleted || stop.cause == disabled) &&
                 stop.effective_utc_ms >= window.start_utc_ms &&
                 stop.effective_utc_ms < window.end_utc_ms &&
                 stop.effective_utc_ms == stop.reconciled_at.utc_ms));
    });
}

std::optional<Refusal> validate_clock(const ScheduledPauseState &state,
                                      const ScheduledPauseClockInput &clock,
                                      const ScheduledPauseConfiguration *proposed, Bytes &bytes) {
    if (!bounded(bytes, clock) || !bounded(bytes, state) ||
        (proposed && (!bytes.take(sizeof(*proposed)) || !bounded(bytes, *proposed)))) {
        return Refusal::limit_exceeded;
    }
    if (clock.cuts.empty() || !valid_basis(state) || (proposed && !valid_config(*proposed))) {
        return Refusal::invalid_input;
    }
    const auto &anchor = state.cursor.last_observation;
    if (clock.source_snapshot_ref != state.snapshot_ref ||
        clock.clock_lineage != state.clock_lineage ||
        clock.anchor.observation_ref != anchor.observation_ref ||
        clock.anchor.utc_ms != anchor.utc_ms) {
        return Refusal::stale_observation;
    }
    std::set<std::string_view> observations{anchor.observation_ref};
    auto previous = anchor.utc_ms;
    for (const auto &cut : clock.cuts) {
        if (cut.observation.observation_ref.empty() || !valid_pause(cut.independent_pause, true) ||
            (cut.elapsed_since_previous && *cut.elapsed_since_previous < milliseconds::zero())) {
            return Refusal::invalid_input;
        }
        if (cut.observation.utc_ms <= previous ||
            !observations.insert(cut.observation.observation_ref).second) {
            return Refusal::stale_observation;
        }
        previous = cut.observation.utc_ms;
    }
    return {};
}

const ScheduledPauseRule *find_rule(const ScheduledPauseConfiguration &config,
                                    std::string_view id) {
    const auto it = std::ranges::find(config.rules, id, &ScheduledPauseRule::schedule_id);
    return it == config.rules.end() ? nullptr : &*it;
}

bool can_start(const ScheduledPauseRule &rule) {
    return rule.enabled &&
           std::ranges::any_of(rule.recurrence.weekdays, [](bool day) { return day; });
}

bool has_starts(const ScheduledPauseConfiguration &config) {
    return std::ranges::any_of(config.rules, can_start);
}

auto offset_at(const ScheduleZoneFacts &facts, Instant time) {
    const auto it = std::ranges::upper_bound(facts.transitions, time, {},
                                             &core::ScheduleZoneTransition::at_utc_ms);
    return it == facts.transitions.begin() ? facts.offset_at_start_seconds
                                           : (it - 1)->offset_after_seconds;
}

bool agree(const ScheduleZoneFacts &left, const ScheduleZoneFacts &right) {
    const auto from = std::max(left.from_utc_ms, right.from_utc_ms);
    const auto through = std::min(left.through_utc_ms, right.through_utc_ms);
    if (from > through) {
        return true;
    }
    const auto same_changes = [&](const auto &a, const auto &b) {
        return std::ranges::all_of(a.transitions, [&](const auto &change) {
            return change.at_utc_ms <= from || change.at_utc_ms > through ||
                   offset_at(b, change.at_utc_ms) == change.offset_after_seconds;
        });
    };
    return offset_at(left, from) == offset_at(right, from) && same_changes(left, right) &&
           same_changes(right, left);
}

core::ScheduleZoneRules core_rules(const ScheduleZoneFacts &facts) {
    return {facts.from_utc_ms, facts.through_utc_ms, facts.offset_at_start_seconds,
            facts.transitions};
}

std::size_t fact_bytes(const ScheduleZoneFacts &facts) {
    return sizeof(ScheduledPauseFactBinding) + facts.configured_zone.size() +
           facts.rules_revision.size() +
           facts.transitions.size() * sizeof(core::ScheduleZoneTransition);
}

// Validate even a short source prefix that cannot yet be passed to CORE-004.
bool supported_offsets(const ScheduleZoneFacts &facts) {
    const auto valid_offset = [](auto offset) { return offset >= -86'400 && offset <= 86'400; };
    if (!valid_offset(facts.offset_at_start_seconds)) {
        return false;
    }
    std::array<std::pair<Instant, Instant>, core::schedule_zone_max_transitions + 1> intervals;
    std::size_t count = 0;
    auto from = facts.from_utc_ms;
    auto offset = facts.offset_at_start_seconds;
    const auto append = [&](Instant through) {
        const auto shift = milliseconds{seconds{offset}}.count();
        const auto start = std::max(from, first_ms - std::min(shift, Instant{0}));
        const auto end = std::min(through, last_ms - std::max(shift, Instant{0}));
        if (start <= end) {
            intervals[count++] = {start + shift, end + shift};
        }
    };
    for (const auto &change : facts.transitions) {
        if (change.at_utc_ms <= from || change.at_utc_ms > facts.through_utc_ms ||
            !valid_offset(change.offset_after_seconds) || change.offset_after_seconds == offset) {
            return false;
        }
        append(change.at_utc_ms - 1);
        from = change.at_utc_ms;
        offset = change.offset_after_seconds;
    }
    append(facts.through_utc_ms);
    const std::span local{intervals.data(), count};
    return std::ranges::none_of(local, [&](const auto &segment) {
        return std::ranges::count_if(local, [&](const auto &other) {
                   return other.first <= segment.first && segment.first <= other.second;
               }) > 2;
    });
}

struct View {
    const ScheduledPauseConfiguration &config;
    std::optional<std::size_t> epoch;
    const std::string *revision = nullptr;
    bool archived = false;
};

View old_view(const ScheduledPauseEpoch &epoch, std::size_t index) {
    return {epoch.configuration, index,
            epoch.closed_zone_rules_revision ? &*epoch.closed_zone_rules_revision : nullptr,
            epoch.through_utc_ms.has_value()};
}

class ZoneReads {
  public:
    ZoneReads(ScheduleZoneSource &source, Bytes input, Bytes *bindings = nullptr)
        : source_(source), input_(input), bindings_(bindings) {}

    bool query() {
        if (remaining_ == 0) {
            return false;
        }
        --remaining_;
        return true;
    }

    using Answer = std::variant<const ScheduleZoneFacts *, ZoneError>;

    const ScheduleZoneFacts *known(const View &view, Instant from) const {
        const ScheduleZoneFacts *best = nullptr;
        for (const auto &binding : facts) {
            const auto &prior = binding.facts;
            if (binding.old_epoch_index == view.epoch && prior.from_utc_ms <= from &&
                from <= prior.through_utc_ms &&
                (!best || best->through_utc_ms < prior.through_utc_ms)) {
                best = &prior;
            }
        }
        return best;
    }

    Answer read(const View &view, Instant from, Instant through) {
        if (view.archived && !view.revision) {
            return ZoneError::insufficient_coverage;
        }
        if (const auto *prior = known(view, from); prior && prior->through_utc_ms >= through) {
            return prior;
        }
        const auto request_bytes = sizeof(ScheduleZoneRequest) +
                                   view.config.configured_zone.size() +
                                   (view.revision ? view.revision->size() : 0);
        if (!query() || !input_.take(request_bytes)) {
            return ZoneError::limit_exceeded;
        }
        auto answer = source_.read({view.config.configured_zone, from, through,
                                    view.revision ? std::optional{*view.revision} : std::nullopt});
        if (const auto *error = std::get_if<ZoneError>(&answer)) {
            return *error;
        }
        auto &reply = std::get<ScheduleZoneFacts>(answer);
        if (reply.transitions.size() > core::schedule_zone_max_transitions ||
            !input_.take(sizeof(reply) +
                         reply.transitions.size() * sizeof(core::ScheduleZoneTransition)) ||
            !reference(input_, reply.configured_zone) || !reference(input_, reply.rules_revision) ||
            facts.size() >= Limits::max_result_rows) {
            return ZoneError::limit_exceeded;
        }
        if (reply.configured_zone != view.config.configured_zone || reply.rules_revision.empty() ||
            (view.revision && *view.revision != reply.rules_revision) || reply.from_utc_ms > from ||
            reply.through_utc_ms < from) {
            return ZoneError::insufficient_coverage;
        }
        if (!supported_offsets(reply)) {
            return ZoneError::unsupported_rules;
        }
        if (!view.archived) {
            const auto bound = current_revisions_.find(view.config.configured_zone);
            if (bound != current_revisions_.end() &&
                facts[bound->second].facts.rules_revision != reply.rules_revision) {
                return ZoneError::unsupported_rules;
            }
        }
        for (const auto &binding : facts) {
            const auto &prior = binding.facts;
            if (prior.configured_zone == reply.configured_zone &&
                prior.rules_revision == reply.rules_revision && !agree(prior, reply)) {
                return ZoneError::unsupported_rules;
            }
        }
        if (bindings_ && !bindings_->take(fact_bytes(reply))) {
            return ZoneError::limit_exceeded;
        }
        if (!view.archived) {
            current_revisions_.try_emplace(view.config.configured_zone, facts.size());
        }
        facts.push_back({view.epoch, std::move(reply)});
        return &facts.back().facts;
    }

    std::optional<std::string> revision(std::size_t epoch) const {
        const auto it = std::ranges::find(facts, std::optional{epoch},
                                          &ScheduledPauseFactBinding::old_epoch_index);
        return it == facts.end() ? std::nullopt : std::optional{it->facts.rules_revision};
    }

    std::vector<ScheduledPauseFactBinding> facts;

  private:
    ScheduleZoneSource &source_;
    Bytes input_;
    Bytes *bindings_;
    std::map<std::string_view, std::size_t, std::less<>> current_revisions_;
    std::size_t remaining_ = Limits::max_projection_queries;
};

struct Candidate {
    const ScheduledPauseRule *rule;
    core::ScheduledPauseOccurrence occurrence;
};

struct Starts {
    Instant through;
    std::optional<Failure> failure{};
    std::vector<Candidate> rows{};
};

struct RuleRange {
    const ScheduledPauseRule *rule;
    Instant after;
};

std::vector<RuleRange> ranges(const ScheduledPauseConfiguration &config, Instant after) {
    std::vector<RuleRange> result;
    for (const auto &rule : config.rules) {
        if (can_start(rule)) {
            result.push_back({&rule, after});
        }
    }
    return result;
}

class DeliveredKeys {
  public:
    explicit DeliveredKeys(const ScheduledPauseLedger *ledger) {
        if (ledger) {
            count = ledger->deliveries.size();
            for (const auto &delivery : ledger->deliveries) {
                keys_[delivery.key.schedule_id].insert(slot(delivery.key.occurrence));
            }
        }
    }

    std::vector<core::ScheduleOccurrenceId> between(std::string_view id, Instant after,
                                                    Instant through) const {
        std::vector<core::ScheduleOccurrenceId> result;
        const auto it = keys_.find(id);
        if (it != keys_.end()) {
            // CORE-004 considers only nominal slots within one day of the scan.
            const Slot lower{core::ScheduleLocalTime{milliseconds{after - day_ms}}, Run::First};
            const auto upper = core::ScheduleLocalTime{milliseconds{through + day_ms}};
            for (auto k = it->second.lower_bound(lower); k != it->second.end() && k->first <= upper;
                 ++k) {
                result.push_back({k->first, k->second});
            }
        }
        return result;
    }

    void add(const Candidate &row) {
        keys_[row.rule->schedule_id].insert(slot(row.occurrence.start.id));
        ++count;
    }

    std::size_t count = 0;

  private:
    Keys keys_;
};

Starts scan_rule(ZoneReads &zone, const View &view, const RuleRange &range, Instant through,
                 bool local_ends, const DeliveredKeys &keys) {
    const auto after = range.after;
    const auto duration = range.rule->duration.count();
    const auto shoulder = local_ends ? std::max(padding, duration) : padding;
    const auto lower = add(after, -padding);
    auto end = std::min({through, last_ms - shoulder, last_ms - duration});
    if (!lower || end <= after) {
        return {after, ZoneError::limit_exceeded};
    }

    const ScheduleZoneFacts *facts = nullptr;
    for (;;) {
        const auto reply = zone.read(view, *lower, end + shoulder);
        if (const auto *available = std::get_if<const ScheduleZoneFacts *>(&reply)) {
            facts = *available;
            break;
        }
        const auto error = std::get<ZoneError>(reply);
        const auto *prior = zone.known(view, *lower);
        if (prior && prior->through_utc_ms > after + shoulder) {
            facts = prior;
            break;
        }
        if (error != ZoneError::limit_exceeded || end - after <= 1) {
            return {after, error};
        }
        // Retry with a smaller padded range. A dense mandatory shoulder may
        // itself exceed the source or CORE-004 limit.
        end = after + (end - after) / 2;
    }
    const auto upper = add(facts->through_utc_ms, -shoulder);
    if (!upper || *upper <= after) {
        return {after, ZoneError::insufficient_coverage};
    }
    end = std::min(end, *upper);
    if (!zone.query()) {
        return {after, ZoneError::limit_exceeded};
    }
    Starts result{end};
    try {
        const auto delivered = keys.between(range.rule->schedule_id, after, end);
        const auto rules = core_rules(*facts);
        for (const auto &occurrence : core::preview_schedule_occurrences(
                 range.rule->recurrence, rules, {after, end}, delivered)) {
            const auto utc_end = occurrence.start_utc_ms + duration;
            core::ScheduleLocalTime local_end{};
            if (local_ends) {
                const auto offset = milliseconds{seconds{offset_at(*facts, utc_end)}}.count();
                const auto projected = add(utc_end, offset);
                if (!projected) {
                    result.through = occurrence.start_utc_ms - 1;
                    result.failure = ZoneError::limit_exceeded;
                    std::erase_if(result.rows, [&](const auto &prior) {
                        return prior.occurrence.start.start_utc_ms > result.through;
                    });
                    break;
                }
                local_end = core::ScheduleLocalTime{milliseconds{*projected}};
            }
            result.rows.push_back({range.rule, {occurrence, utc_end, local_end}});
        }
    } catch (const std::invalid_argument &) {
        return {after, ZoneError::unsupported_rules};
    }
    return result;
}

Instant start_of(const Candidate &row) { return row.occurrence.start.start_utc_ms; }

// Advance only as far as every rule in the tile was resolved. Rows are retained
// in UTC order and an oversized boundary group is removed as a whole.
Starts scan(ZoneReads &zone, const View &view, std::span<const RuleRange> input, Instant after,
            Instant through, bool local_ends, const DeliveredKeys &keys) {
    if (input.empty()) {
        return {through};
    }
    Starts result{after};
    while (result.through < through) {
        const auto lower = result.through;
        auto frontier = std::min(through, add(lower, core_span).value_or(last_ms));
        std::optional<Failure> failure;
        for (const auto &range : input) {
            const auto from = std::max(lower, range.after);
            if (from >= frontier) {
                continue;
            }
            auto resolved = scan_rule(zone, view, {range.rule, from}, frontier, local_ends, keys);
            if (resolved.through < frontier) {
                frontier = resolved.through;
                failure = resolved.failure;
            }
            const auto suffix = std::ranges::upper_bound(result.rows, frontier, {}, start_of);
            result.rows.erase(suffix, result.rows.end());
            const auto middle = result.rows.size();
            result.rows.insert(result.rows.end(), resolved.rows.begin(), resolved.rows.end());
            std::inplace_merge(result.rows.begin(),
                               result.rows.begin() + static_cast<std::ptrdiff_t>(middle),
                               result.rows.end(), [](const auto &left, const auto &right) {
                                   return start_of(left) < start_of(right);
                               });
            if (result.rows.size() > Limits::max_result_rows) {
                frontier = start_of(result.rows[Limits::max_result_rows]) - 1;
                result.rows.erase(std::ranges::upper_bound(result.rows, frontier, {}, start_of),
                                  result.rows.end());
                failure = ZoneError::limit_exceeded;
            }
        }
        result.through = frontier;
        if (failure || frontier == lower) {
            result.failure = failure.value_or(Failure{ZoneError::insufficient_coverage});
            break;
        }
    }
    return result;
}

std::size_t key_strings(std::string_view lineage, std::string_view id) {
    return lineage.size() + id.size();
}

std::size_t key_strings(const ScheduledPauseKey &key) {
    return key_strings(key.timer_lineage, key.schedule_id);
}

std::size_t delivery_bytes(const DeliveredScheduledPause &delivery) {
    return sizeof(delivery) + key_strings(delivery.key) +
           (delivery.stop ? delivery.stop->reconciled_at.observation_ref.size() : 0);
}

std::size_t group_end(std::span<const Candidate> rows, std::size_t first) {
    auto end = first + 1;
    while (end < rows.size() && start_of(rows[end]) == start_of(rows[first])) {
        ++end;
    }
    return end;
}

struct ActiveDelivery {
    const DeliveredScheduledPause *prior;
    std::size_t new_index = 0;

    const DeliveredScheduledPause &get(const ScheduledPauseProgress &result) const {
        return prior ? *prior : result.new_deliveries[new_index];
    }
};

bool reached(const core::TimerConfig &config, Instant when) {
    return config.hard_end_utc_ms && when >= *config.hard_end_utc_ms;
}

class Progression {
  public:
    Progression(const ScheduledPauseState &state, ZoneReads &zone, Bytes &output)
        : result{.source_snapshot_ref = state.snapshot_ref,
                 .timer = state.timer,
                 .pause = state.pause,
                 .cursor = state.cursor,
                 .starts_resolved_through_utc_ms = {},
                 .next_known_expiry_utc_ms = {},
                 .notice = {},
                 .new_deliveries = {},
                 .stops = {},
                 .timer_transitions = {}},
          keys(state.ledger), state_(state), zone_(zone), output_(output) {
        // The existing basis already passed preflight. Keep space for new
        // ledger contents so the proposal can serve as a subsequent basis.
        (void)bounded(retention_, state);
        if (state.ledger) {
            for (const auto &delivery : state.ledger->deliveries) {
                if (!delivery.stop) {
                    active_.emplace(delivery.window.end_utc_ms, ActiveDelivery{&delivery});
                }
            }
        }
    }

    void run(const ScheduledPauseClockInput &clock);
    void enter(const ScheduledPauseEntry &entry);
    std::optional<Failure> edit(const ScheduledPauseConfiguration &proposed,
                                std::vector<ScheduledPauseCutoverEffect> *effects);
    void probe_new(const ScheduledPauseConfiguration &proposed);
    void archive_old();
    bool retain_epoch(const ScheduledPauseConfiguration &, const std::optional<std::string> &);

    ScheduledPauseProgress result;
    DeliveredKeys keys;

  private:
    void hold(Failure failure, Instant from) {
        if (!result.cursor.hold) {
            result.cursor.hold = ScheduledPauseCoverageHold{failure, from};
        }
    }

    void set_independent(const core::PauseState &pause) {
        result.pause =
            core::set_pause_reason(pause, core::PauseReason::Schedule,
                                   result.pause.reasons.contains(core::PauseReason::Schedule));
    }

    void reconcile() {
        // The crossing is computed from captured endpoints, with a single
        // union for all starts and stops at this observation.
        if (state_.ledger) {
            const auto now = result.cursor.last_observation.utc_ms;
            const auto next = active_.upper_bound(now);
            const auto window = next == active_.end()
                                    ? std::span<const core::ScheduledPauseWindow>{}
                                    : std::span{&next->second.get(result).window, std::size_t{1}};
            result.pause = core::reconcile_scheduled_pause(result.pause, window, now);
        }
    }

    void finish_observation() {
        const auto next = active_.upper_bound(result.cursor.last_observation.utc_ms);
        // On a finishing cut, an expiry at the cut has intentionally not
        // been reconciled. It remains in the ledger until a later operation.
        result.next_known_expiry_utc_ms =
            next == active_.end() ? std::nullopt : std::optional{next->first};
        result.notice.reset();
        if (result.cursor.hold) {
            result.notice =
                ScheduledPauseCoverageNotice{*result.cursor.hold, result.cursor.last_observation};
        }
    }

    void advance(Instant elapsed) {
        const auto &at = result.cursor.last_observation;
        auto next = core::advance_timer(result.timer, state_.timer_config,
                                        {elapsed, at.utc_ms, !result.pause.reasons.empty()});
        result.timer = next.state;
        if (!next.transitions.empty()) {
            result.timer_transitions.push_back({at, std::move(next.transitions)});
        }
    }

    std::optional<Failure> expire();
    std::optional<Failure> admit(std::span<const Candidate> group);
    std::optional<Failure> eligibility();
    Starts ordinary(Instant after, Instant through);
    void cleared() { result.cursor.hold.reset(); }

    const ScheduledPauseState &state_;
    ZoneReads &zone_;
    Bytes &output_;
    Bytes retention_{Limits::max_input_bytes};
    std::multimap<Instant, ActiveDelivery> active_;
    bool finished_on_last_cut_ = false;
};

std::optional<Failure> Progression::expire() {
    const auto &at = result.cursor.last_observation;
    while (!active_.empty() && active_.begin()->first <= at.utc_ms) {
        const auto end = active_.begin()->first;
        const auto until = active_.upper_bound(end);
        std::size_t bytes = 0;
        std::size_t count = 0;
        for (auto it = active_.begin(); it != until; ++it) {
            bytes += sizeof(ScheduledPauseDeliveryStop) + key_strings(it->second.get(result).key) +
                     at.observation_ref.size();
            ++count;
        }
        const auto retained = count * at.observation_ref.size();
        if (retained > retention_.left) {
            return Knowledge::ledger;
        }
        if (count > Limits::max_result_rows - result.stops.size() || !output_.take(bytes)) {
            return ZoneError::limit_exceeded;
        }
        (void)retention_.take(retained);
        for (auto it = active_.begin(); it != until; ++it) {
            result.stops.push_back(
                {it->second.get(result).key, {ScheduledPauseStopCause::expired, end, at}});
        }
        active_.erase(active_.begin(), until);
    }
    return {};
}

std::optional<Failure> Progression::admit(std::span<const Candidate> group) {
    if (group.empty()) {
        return {};
    }
    if (!state_.ledger || group.size() > Limits::max_deliveries - keys.count) {
        return Knowledge::ledger;
    }
    std::size_t bytes = 0;
    for (const auto &row : group) {
        bytes += sizeof(DeliveredScheduledPause) +
                 key_strings(state_.timer_lineage, row.rule->schedule_id);
    }
    if (bytes > retention_.left) {
        return Knowledge::ledger;
    }
    if (group.size() > Limits::max_result_rows - result.new_deliveries.size() ||
        !output_.take(bytes)) {
        return ZoneError::limit_exceeded;
    }
    (void)retention_.take(bytes);
    for (const auto &row : group) {
        const auto &occurrence = row.occurrence;
        active_.emplace(occurrence.end_utc_ms,
                        ActiveDelivery{nullptr, result.new_deliveries.size()});
        result.new_deliveries.push_back(
            {{state_.timer_lineage, row.rule->schedule_id, occurrence.start.id},
             {start_of(row), occurrence.end_utc_ms},
             {}});
        keys.add(row);
    }
    return {};
}

Starts Progression::ordinary(Instant after, Instant through) {
    if (state_.epochs.empty() ||
        (state_.epochs.back().after_utc_ms && after < *state_.epochs.back().after_utc_ms)) {
        return {after, Knowledge::history};
    }
    const auto &epoch = state_.epochs.back();
    const auto rules = ranges(epoch.configuration, after);
    const auto limit = add(state_.cursor.last_observation.utc_ms, scan_span).value_or(last_ms);
    const auto end = rules.empty() ? through : std::clamp(limit, after, through);
    auto resolved =
        scan(zone_, old_view(epoch, state_.epochs.size() - 1), rules, after, end, false, keys);
    if (!state_.ledger && !resolved.rows.empty()) {
        resolved.through = start_of(resolved.rows.front()) - 1;
        resolved.failure = Knowledge::ledger;
        resolved.rows.clear();
    }
    if (!resolved.failure && end < through) {
        resolved.failure = ZoneError::limit_exceeded;
    }
    return resolved;
}

std::optional<Failure> Progression::eligibility() {
    const auto at = result.cursor.last_observation.utc_ms;
    if (state_.epochs.empty()) {
        return Knowledge::history;
    }
    const auto &current = state_.epochs.back().configuration;
    std::map<std::string_view, std::size_t, std::less<>> enabled_since;
    bool missing_history = false;
    for (const auto &rule : current.rules) {
        if (!rule.enabled) {
            continue;
        }
        auto oldest = state_.epochs.size() - 1;
        while (oldest > 0) {
            const auto &prior = state_.epochs[oldest - 1];
            if (prior.through_utc_ms != state_.epochs[oldest].after_utc_ms) {
                const auto lower = *state_.epochs[oldest].after_utc_ms;
                missing_history |= distance(lower, at) < static_cast<std::uint64_t>(max_duration);
                break;
            }
            const auto *previous = find_rule(prior.configuration, rule.schedule_id);
            if (!previous || !previous->enabled) {
                break;
            }
            --oldest;
        }
        if (oldest == 0 && !state_.epoch_history_complete) {
            const auto first = state_.epochs.front().after_utc_ms;
            missing_history |=
                !first || distance(*first, at) < static_cast<std::uint64_t>(max_duration);
        }
        enabled_since.emplace(rule.schedule_id, oldest);
    }
    if (enabled_since.empty()) {
        result.pause = core::set_pause_reason(result.pause, core::PauseReason::Schedule, false);
    }
    // An observed disable/delete ends eligibility even for a captured window.
    std::erase_if(active_, [&](const auto &item) {
        const auto &delivery = item.second.get(result);
        const auto enabled = enabled_since.find(delivery.key.schedule_id);
        if (enabled == enabled_since.end()) {
            return true;
        }
        const auto index = enabled->second;
        const auto &boundary = state_.epochs[index].after_utc_ms;
        const bool known_enable = index > 0 && state_.epochs[index - 1].through_utc_ms == boundary;
        return known_enable && boundary && delivery.window.start_utc_ms <= *boundary;
    });
    if (!state_.ledger && !enabled_since.empty()) {
        return Knowledge::ledger;
    }
    if (missing_history) {
        return Knowledge::history;
    }

    for (std::size_t i = 0; i < state_.epochs.size(); ++i) {
        const auto &epoch = state_.epochs[i];
        const auto through = std::min(at, epoch.through_utc_ms.value_or(at));
        std::vector<RuleRange> rules;
        auto after = through;
        for (const auto &rule : epoch.configuration.rules) {
            const auto enabled = enabled_since.find(rule.schedule_id);
            if (!can_start(rule) || enabled == enabled_since.end() || enabled->second > i) {
                continue;
            }
            const auto lookback = add(at, -rule.duration.count());
            if (!lookback && !epoch.after_utc_ms) {
                return ZoneError::limit_exceeded;
            }
            const auto lower =
                std::max(lookback.value_or(first_ms), epoch.after_utc_ms.value_or(first_ms));
            if (lower < through) {
                rules.push_back({&rule, lower});
                after = std::min(after, lower);
            }
        }
        if (rules.empty()) {
            continue;
        }
        const auto starts = scan(zone_, old_view(epoch, i), rules, after, through, false, keys);
        const std::span rows{starts.rows};
        result.starts_resolved_through_utc_ms = starts.through;
        for (std::size_t first = 0; first < rows.size();) {
            const auto end = group_end(rows, first);
            if (const auto failure = admit(rows.subspan(first, end - first))) {
                result.starts_resolved_through_utc_ms = start_of(rows[first]) - 1;
                return failure;
            }
            first = end;
        }
        if (starts.failure) {
            return starts.failure;
        }
    }
    return {};
}

void Progression::run(const ScheduledPauseClockInput &clock) {
    std::optional<Starts> plan;
    std::size_t row = 0;
    // A terminal cut consumes none of its incoming interval. There is no
    // need to resolve scheduled starts at or after that cut.
    auto scan_through = clock.anchor.utc_ms;
    for (const auto &cut : clock.cuts) {
        if (reached(state_.timer_config, cut.observation.utc_ms)) {
            break;
        }
        scan_through = cut.observation.utc_ms;
    }

    for (const auto &cut : clock.cuts) {
        const auto previous = result.cursor.last_observation.utc_ms;
        const auto now = cut.observation.utc_ms;
        result.cursor.last_observation = cut.observation;
        set_independent(cut.independent_pause);
        finished_on_last_cut_ = false;
        if (result.timer.lifecycle == Lifecycle::Active && reached(state_.timer_config, now)) {
            advance(0);
            cleared();
            finished_on_last_cut_ = true;
            continue;
        }
        if (result.timer.lifecycle != Lifecycle::Active) {
            if (const auto failure = expire()) {
                hold(*failure, previous);
            }
            reconcile();
            continue;
        }
        if (result.cursor.hold) {
            const auto prior_frontier = result.starts_resolved_through_utc_ms;
            if (!expire() && !eligibility()) {
                cleared();
            }
            result.starts_resolved_through_utc_ms =
                std::max(prior_frontier, result.starts_resolved_through_utc_ms);
            reconcile();
            plan.reset();
            continue;
        }

        if (!plan) {
            plan = ordinary(previous, scan_through);
            result.starts_resolved_through_utc_ms = plan->through;
            row = 0;
        }
        const auto expiry = active_.lower_bound(previous);
        const bool missed_expiry = expiry != active_.end() && expiry->first < now;
        const bool missed_start = row < plan->rows.size() && start_of(plan->rows[row]) < now;
        if (now > add(plan->through, 1).value_or(last_ms)) {
            hold(plan->failure.value_or(Failure{ZoneError::insufficient_coverage}), previous);
        } else if (!state_.ledger && result.pause.reasons.contains(core::PauseReason::Schedule)) {
            hold(Knowledge::ledger, previous);
        } else if (!cut.elapsed_since_previous || missed_start || missed_expiry) {
            hold(Knowledge::elapsed_split, previous);
        } else {
            advance(cut.elapsed_since_previous->count());
            result.cursor.evaluated_through_utc_ms = now;
            if (result.timer.lifecycle == Lifecycle::Finished) {
                finished_on_last_cut_ = true;
                continue;
            }
        }

        if (const auto failure = expire()) {
            hold(*failure, now);
        }
        if (!result.cursor.hold && now <= plan->through && row < plan->rows.size() &&
            start_of(plan->rows[row]) == now) {
            const auto end = group_end(plan->rows, row);
            if (const auto failure = admit(std::span{plan->rows}.subspan(row, end - row))) {
                result.starts_resolved_through_utc_ms = now - 1;
                hold(*failure, now);
            }
            row = end;
        }
        if (now > plan->through) {
            hold(plan->failure.value_or(Failure{ZoneError::insufficient_coverage}), now);
        }
        reconcile();
    }
    finish_observation();
}

void Progression::enter(const ScheduledPauseEntry &entry) {
    result.timer = entry.admitted_timer;
    result.cursor.last_observation = entry.observation;
    cleared();
    if (const auto failure = expire()) {
        hold(*failure, entry.observation.utc_ms);
    } else if (const auto eligibility_failure = eligibility()) {
        hold(*eligibility_failure, entry.observation.utc_ms);
    }
    reconcile();
    finish_observation();
}

std::optional<Failure> Progression::edit(const ScheduledPauseConfiguration &proposed,
                                         std::vector<ScheduledPauseCutoverEffect> *effects) {
    if (finished_on_last_cut_) {
        return {};
    }
    if (result.cursor.hold) {
        return result.cursor.hold->cause;
    }
    if (!state_.ledger) {
        return Knowledge::ledger;
    }

    const auto &at = result.cursor.last_observation;
    struct Edit {
        decltype(active_)::iterator delivery;
        std::optional<ScheduledPauseStopCause> cause;
    };
    std::vector<Edit> edits;
    std::size_t bytes = 0;
    std::size_t stops = 0;
    for (auto it = active_.upper_bound(at.utc_ms); it != active_.end(); ++it) {
        const auto &delivery = it->second.get(result);
        const auto *rule = find_rule(proposed, delivery.key.schedule_id);
        std::optional<ScheduledPauseStopCause> cause;
        if (!rule || !rule->enabled) {
            cause = rule ? ScheduledPauseStopCause::disabled : ScheduledPauseStopCause::deleted;
            bytes += sizeof(ScheduledPauseDeliveryStop) + key_strings(delivery.key) +
                     at.observation_ref.size();
            ++stops;
        }
        if (effects) {
            bytes += sizeof(ScheduledPauseCutoverEffect) + delivery_bytes(delivery) -
                     sizeof(delivery) + (cause ? at.observation_ref.size() : 0);
        }
        edits.push_back({it, cause});
    }
    const auto retained = stops * at.observation_ref.size();
    if (retained > retention_.left) {
        return Knowledge::ledger;
    }
    if (stops > Limits::max_result_rows - result.stops.size() ||
        (effects && edits.size() > Limits::max_result_rows) || !output_.take(bytes)) {
        return ZoneError::limit_exceeded;
    }
    (void)retention_.take(retained);

    for (const auto &edit : edits) {
        const auto &delivery = edit.delivery->second.get(result);
        std::optional<ScheduledPauseStop> stop;
        if (edit.cause) {
            stop = ScheduledPauseStop{*edit.cause, at.utc_ms, at};
            result.stops.push_back({delivery.key, *stop});
        }
        if (effects) {
            effects->push_back({delivery, std::move(stop)});
        }
        if (edit.cause) {
            active_.erase(edit.delivery);
        }
    }
    reconcile();
    finish_observation();
    return {};
}

void Progression::probe_new(const ScheduledPauseConfiguration &proposed) {
    const auto at = result.cursor.last_observation.utc_ms;
    if (result.timer.lifecycle == Lifecycle::Active && has_starts(proposed)) {
        const auto next = add(at, 1);
        const auto rules = ranges(proposed, at);
        const auto failure =
            next ? scan(zone_, View{proposed, {}}, rules, at, *next, false, keys).failure
                 : std::optional<Failure>{ZoneError::limit_exceeded};
        if (failure) {
            hold(*failure, at);
        }
    }
    finish_observation();
}

void Progression::archive_old() {
    if (state_.epochs.empty()) {
        return;
    }
    const auto index = state_.epochs.size() - 1;
    const auto &epoch = state_.epochs[index];
    if (!has_starts(epoch.configuration) || zone_.revision(index)) {
        return;
    }
    const auto at = result.cursor.last_observation.utc_ms;
    const auto view = old_view(epoch, index);
    auto after = at;
    for (const auto &rule : epoch.configuration.rules) {
        if (can_start(rule)) {
            const auto lower = std::max(add(at, -rule.duration.count()).value_or(first_ms),
                                        epoch.after_utc_ms.value_or(first_ms));
            after = std::min(after, lower);
        }
    }
    const auto from = add(after, -padding);
    const auto through = add(at, padding);
    if (from && through) {
        (void)zone_.read(view, *from, *through);
    }
    // A later entry may need a wider interval than the source supplies now.
    // A coherent point at cutover can still bind the closing rules revision.
    if (!zone_.revision(index)) {
        (void)zone_.read(view, at, at);
    }
}

bool Progression::retain_epoch(const ScheduledPauseConfiguration &proposed,
                               const std::optional<std::string> &revision) {
    return retention_.take(sizeof(ScheduledPauseEpoch) +
                           proposed.rules.size() * sizeof(ScheduledPauseRule) +
                           config_strings(proposed) + (revision ? revision->size() : 0));
}

constexpr std::size_t progress_bytes =
    sizeof(ScheduledPauseProgress) + 9 * Limits::max_reference_bytes +
    6 * sizeof(ScheduledPauseTransitions) + 12 * sizeof(core::TimerTransition) +
    6 * sizeof(core::PauseReason);

} // namespace

std::variant<ScheduledPausePreview, ScheduledPauseRefusal> project_scheduled_pause_edit(
    const ScheduledPauseState &state, const ScheduledPauseClockInput &clock,
    const ScheduledPauseConfiguration &proposed, Instant through, ScheduleZoneSource &source) {
    Bytes input{Limits::max_input_bytes};
    if (const auto refusal = validate_clock(state, clock, &proposed, input)) {
        return *refusal;
    }
    const auto &at = clock.cuts.back().observation;
    if (through < at.utc_ms) {
        return Refusal::invalid_input;
    }
    if (distance(at.utc_ms, through) > milliseconds{Limits::max_preview_span}.count()) {
        return Refusal::limit_exceeded;
    }

    Bytes output{Limits::max_result_bytes - progress_bytes - sizeof(ScheduledPausePreview) -
                 7 * Limits::max_reference_bytes - 6 * sizeof(core::PauseReason) -
                 proposed.rules.size() * sizeof(ScheduledPauseRule) - config_strings(proposed)};
    ZoneReads zone(source, input, &output);
    Progression progression(state, zone, output);
    progression.run(clock);

    ScheduledPausePreview result{.source_snapshot_ref = state.snapshot_ref,
                                 .timer_lineage = state.timer_lineage,
                                 .clock_lineage = state.clock_lineage,
                                 .state_revision = state.state_revision,
                                 .configuration_revision = state.configuration_revision,
                                 .delivery_revision =
                                     state.ledger ? state.ledger->revision : std::string{},
                                 .as_of = at,
                                 .proposed = proposed,
                                 .requested_through_utc_ms = through,
                                 .resolved_through_utc_ms = {},
                                 .incomplete = {},
                                 .facts = {},
                                 .upcoming = {},
                                 .cutover_effects = {},
                                 .pause_after_cutover = {}};
    const auto old_failure = progression.edit(proposed, &result.cutover_effects);
    if (!old_failure) {
        result.pause_after_cutover = progression.result.pause;
    }
    result.incomplete = old_failure;

    const auto rules = ranges(proposed, at.utc_ms);
    auto starts = scan(zone, View{proposed, {}}, rules, at.utc_ms, through, true, progression.keys);
    result.resolved_through_utc_ms = starts.through;
    if (!result.incomplete) {
        result.incomplete = starts.failure;
    }

    result.facts = std::move(zone.facts);
    const auto old_nominal_limit = add(at.utc_ms, day_ms).value_or(last_ms);
    const std::span rows{starts.rows};
    for (std::size_t first = 0; first < rows.size();) {
        const auto end = group_end(rows, first);
        std::size_t bytes = 0;
        std::optional<Failure> row_failure;
        for (const auto &row : rows.subspan(first, end - first)) {
            bytes += sizeof(ScheduledPausePreviewRow) +
                     key_strings(state.timer_lineage, row.rule->schedule_id);
            if (!state.ledger) {
                row_failure = Knowledge::ledger;
            } else if (old_failure &&
                       row.occurrence.start.id.nominal_local_start.time_since_epoch().count() <=
                           old_nominal_limit &&
                       (state.epochs.empty() ||
                        find_rule(state.epochs.back().configuration, row.rule->schedule_id))) {
                row_failure = old_failure;
            }
        }
        if (row_failure || !output.take(bytes)) {
            result.incomplete = row_failure.value_or(Failure{ZoneError::limit_exceeded});
            result.resolved_through_utc_ms = start_of(rows[first]) - 1;
            break;
        }
        for (const auto &row : rows.subspan(first, end - first)) {
            result.upcoming.push_back(
                {{state.timer_lineage, row.rule->schedule_id, row.occurrence.start.id},
                 row.occurrence});
        }
        first = end;
    }
    return result;
}

std::variant<ScheduledPauseCutover, ScheduledPauseCoverageFailure, ScheduledPauseRefusal>
project_scheduled_pause_cutover(const ScheduledPauseState &state,
                                const ScheduledPauseClockInput &clock,
                                const ScheduledPauseConfiguration &proposed,
                                ScheduleZoneSource &source) {
    Bytes input{Limits::max_input_bytes};
    if (const auto refusal = validate_clock(state, clock, &proposed, input)) {
        return *refusal;
    }
    if (state.epochs.size() == Limits::max_epochs) {
        return Failure{Knowledge::history};
    }
    Bytes output{Limits::max_result_bytes - progress_bytes - sizeof(ScheduledPauseCutover) -
                 2 * Limits::max_reference_bytes -
                 proposed.rules.size() * sizeof(ScheduledPauseRule) - config_strings(proposed)};
    ZoneReads zone(source, input);
    Progression progression(state, zone, output);
    progression.run(clock);
    if (const auto failure = progression.edit(proposed, nullptr)) {
        return *failure;
    }
    progression.probe_new(proposed);
    progression.archive_old();
    auto revision = state.epochs.empty() ? std::nullopt : zone.revision(state.epochs.size() - 1);
    if (!progression.retain_epoch(proposed, revision)) {
        return Failure{Knowledge::history};
    }
    return ScheduledPauseCutover{clock.cuts.back().observation, proposed, std::move(revision),
                                 std::move(progression.result)};
}

std::variant<ScheduledPauseProgress, ScheduledPauseRefusal>
enter_scheduled_pauses(const ScheduledPauseState &state, const ScheduledPauseEntry &entry,
                       ScheduleZoneSource &source) {
    Bytes input{Limits::max_input_bytes};
    if (!bounded(input, state) || !input.take(sizeof(entry)) ||
        !reference(input, entry.source_snapshot_ref) ||
        !reference(input, entry.observation.observation_ref)) {
        return Refusal::limit_exceeded;
    }
    if (!valid_basis(state) ||
        (state.timer.lifecycle != Lifecycle::Ready &&
         state.timer.lifecycle != Lifecycle::Finished) ||
        entry.source_snapshot_ref.empty() || entry.observation.observation_ref.empty() ||
        entry.admitted_timer.lifecycle != Lifecycle::Active ||
        !valid_timer(entry.admitted_timer, state.timer_config)) {
        return Refusal::invalid_input;
    }
    if (entry.source_snapshot_ref != state.snapshot_ref ||
        entry.observation.utc_ms <= state.cursor.last_observation.utc_ms ||
        entry.observation.observation_ref == state.cursor.last_observation.observation_ref) {
        return Refusal::stale_observation;
    }
    if (reached(state.timer_config, entry.observation.utc_ms)) {
        return Refusal::hard_end_reached;
    }
    Bytes output{Limits::max_result_bytes - progress_bytes};
    ZoneReads zone(source, input);
    Progression progression(state, zone, output);
    progression.enter(entry);
    return std::move(progression.result);
}

std::variant<ScheduledPauseProgress, ScheduledPauseRefusal>
progress_scheduled_pauses(const ScheduledPauseState &state, const ScheduledPauseClockInput &clock,
                          ScheduleZoneSource &source) {
    Bytes input{Limits::max_input_bytes};
    if (const auto refusal = validate_clock(state, clock, nullptr, input)) {
        return *refusal;
    }
    Bytes output{Limits::max_result_bytes - progress_bytes};
    ZoneReads zone(source, input);
    Progression progression(state, zone, output);
    progression.run(clock);
    return std::move(progression.result);
}

} // namespace subathon::server
