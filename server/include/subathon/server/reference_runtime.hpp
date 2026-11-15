// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <subathon/server/reference_configuration.hpp>

namespace subathon::server {
// API-011 public contract revision 1. Borrowed ports outlive all prepared pairs;
// drain writer before destruction. No callback opens SQL, writes, commits, pumps
// events, resamples time, queues work or caches authority. All DATA is bounded.
struct ReferenceClockSample {
    std::int64_t monotonic_ms = 0, utc_ms = 0;
};
using ReferenceClockSampleResult = std::variant<ReferenceClockSample, ReferenceUnavailable>;
class ReferenceClockSampler {
  public:
    virtual ~ReferenceClockSampler() = default;
    virtual ReferenceClockSampleResult sample() = 0;
};
// Actual production sampler independently reads steady_clock then system_clock;
// checked conversion to integer milliseconds, no UTC-difference elapsed, no host
// stub/fallback. Unsupported range/clock refuses not_ready/reference.clock.v1.
class SystemReferenceClockSampler final : public ReferenceClockSampler {
  public:
    ReferenceClockSampleResult sample() override;
};
enum class ReferenceEntropyError : std::uint8_t { none, short_read, unavailable };
struct ReferenceEntropyRead {
    std::array<std::uint8_t, 32> bytes{};
    std::uint64_t received = 0;
    ReferenceEntropyError error = ReferenceEntropyError::unavailable;
};
class ReferenceEntropy {
  public:
    virtual ~ReferenceEntropy() = default;
    virtual ReferenceEntropyRead read32() = 0;
};
// Linux: ONE getrandom(...,32,0), no EINTR/short retry, fallback, loop or weak seed.
// Exact32->none; 0..31->short_read; negative->unavailable. Never expose partial
// bytes: non-success zeroes array, received retains nonnegative short count/0.
// Other platforms explicit unavailable until their separate adapter is reviewed.
class OsReferenceEntropy final : public ReferenceEntropy {
  public:
    ReferenceEntropyRead read32() override;
};
struct ReferenceCommittedState {
    ApplicationStateSnapshot snapshot;
    std::optional<std::string> basis;
    ApplicationClockObservation observation;
    std::string policy_revision_ref;
};
// Typed bounded SAS1 facade, byte-identical to service grammar, NOT readiness.
// Decode verifies ALL snapshot duplicates/config/clock/policy/timer/debt/balances;
// optional basis absent is valid only for initial state. Known malformed ->
// ReferenceInputFailure(invalid_document/binding); healthy unknown format ->
// unsupported_format. Envelope B includes format AND document; exact codec scratch
// <=64 borrowed fields/512 bytes/depth3, projections checked before materializing.
[[nodiscard]] ReferenceCommittedState decode_reference_state(const ApplicationStateSnapshot &,
                                                             const ReferenceAdapterLimits & = {});
[[nodiscard]] ApplicationStateSnapshot encode_reference_state(const ReferenceCommittedState &,
                                                              const ReferenceAdapterLimits & = {});
struct ReferenceClockAnchor {
    ReferenceCommittedState committed;
    ReferenceClockSample sample;
};
// Anchor DATA must come from actual acknowledged fresh startup in THIS process.
// Tests may separately seed literal committed rows + controlled clock evidence.
// Raw reopen/decode is NOT authority to construct a live lineage. No public route
// accepts anchors. Only trusted queue composition supplies these dependencies.
enum class ReferenceZeroStep : std::uint8_t { reuse_unchanged, fresh };
struct ReferenceRuntimeLimits {
    ReferenceAdapterLimits values{};
    ReferenceAuthorityLimits authority{};
    ApplicationServiceLimits service{};
    std::uint64_t pending_reservations = 16;
};
// Concrete FIVE unchanged ACCT-004 ports. Constructor inertly stores dependencies
// and optional LIVE anchor; absent anchor never becomes ready by reading SAS1.
// reserve occurs on owner queue immediately BEFORE service execution, after FIFO
// wait; current head/config/policy must equal anchor. Whole supported runtime:
// actual SAS1 Active/Main>0, !wait, debt0, balances empty, no deadline, known false
// pause/recovery, same live lineage. Unknown/expiry/discontinuity -> not_ready.
// Sample UTC and elapsed separately once; elapsed checked >=0 and <Main.
// Keep current anchor + every outstanding reservation: no eviction, wrap or reuse.
// capacity refusal is not_ready/reference.journal-capacity.v1 BEFORE sampling or
// entropy; capacity includes anchor, reservations and spent observations. Released
// reservations remain tombstoned until lineage ends; capacity is a process bound.
// zero/unchanged bindings+UTC: reuse mode releases unused fresh reservation and
// returns old observation/segments{}; fresh mode emits one {0,UTC,false}. Changed
// UTC ALWAYS explicit zero/fresh. Nonzero emits ONE {elapsed,UTC,false}. Callbacks
// consume reserved sample unchanged, never sample after permission/policy work.
// settle only accepts matching pending work: committed/none/present response may
// promote the independently decoded committed head; rollback retains old anchor.
// Unknown stops permanently; committed/work_exception blocks until independent
// authorized committed read reconciles via reconcile, never blind replay/reset.
// Missing/malformed normal response never promotes/publishes. Mismatched binding
// is invalid_binding, no journal mutation. No guessed current UTC on any read.
// check calls actual authority source EVERY attempt for original-author whole
// USE+GRANT and separate execution USE, full captured request including actual0.
// disclose_application re-resolves ACTUAL record; disclose_holds resolves EVERY
// actual member, no partial output or prior execute-permit reuse. Preserve original
// callback/view/writer/ReferenceExecutionFailure and service cleanup ownership.
// resolve loads complete captured+CURRENT head config, echoes every binding and
// exact current cap/TimerConfig/policy; captured amount/rule stay immutable.
// prepare rechecks BOTH configurations and whole numeric proposal before returning
// jobs{} / explanation reference.no-external-actions.v1. Missing source!=empty.
// issue: ONE entropy read -> RRF1 + U8(purpose) +32 (37 bytes); echo whole binding.
// reserve uses RCO1+32 (36). Repeated journal bytes refuse conflict/reference.collision.v1;
// issue purpose/binding may be consumed once, no retry. State/hold/version tokens
// use ACTUAL store uniqueness/conflict, never an in-memory probability assertion.
// Clock annotations remain reserved/tombstoned across known rollback; no nonreuse
// claim from SQL, since observations aren't independently unique SQL keys.
// Resources ALL methods: structural B/I from configuration header applied to ALL
// arguments and nested producer results, each whole result/intermediate separately.
// Full imported command/input/policy/proposal/record/hold/authority DTOs charged in
// every occurrence under their own limits too; no fields omitted on losing paths.
// Scalars sample B16/I0; entropy B41/I0 (array32+received8+enum1); limits uncharged.
// current input=snapshot. Anchor B=committed+16; committed B=snapshot+optional
// basis+observation+policy. reserve input=head+whole command+zero-mode1; settle
// input=completion+optional committed1+body; reconcile input=committed; vectors recursively count
// elements; callback objects uncharged. journal_entries bounds anchor+pending+tombstones;
// pending_reservations bounds prepared queue reservations. All positive/range/container/n+1 checks,
// checked arithmetic and pre-copy/output projections follow configuration priority.
// Invalid input wins before affinity/view/dependencies. Affinity wrong_thread;
// valid expired/foreign finite views preserve original errors. Whole output/nested
// resource refusal never yields partial permit. Pure input errors ReferenceInputFailure;
// semantic service callback errors use ReferenceExecutionFailure, never fake rollback.
// Source audit REQUIRED for preallocation, nested minima, current whole graph,
// SQL metadata projection, bounded loops, capability revocation and nonreuse.
class ReferenceApplicationAdapters final : public ExecutionAccess,
                                           public CurrentApplicationPolicy,
                                           public SegmentedApplicationClock,
                                           public RequiredApplicationActions,
                                           public FreshApplicationReferences {
  public:
    ReferenceApplicationAdapters(ReferenceConfigurationSource &, ReferenceAuthoritySource &,
                                 ReferenceClockSampler &, ReferenceEntropy &,
                                 std::optional<ReferenceClockAnchor>, ReferenceRuntimeLimits = {});
    // Readiness from live owned journal plus ACTUAL supplied head, without clock
    // sampling/entropy; no raw-reopen reanchor. Pure whole input priority applies.
    [[nodiscard]] std::variant<ReferenceCommittedState, ReferenceUnavailable>
    current(const ApplicationStateSnapshot &) const;
    [[nodiscard]] std::optional<ReferenceUnavailable>
    reserve(const ReferenceCommittedState &, const OriginalApplicationCommand &,
            ReferenceZeroStep = ReferenceZeroStep::reuse_unchanged);
    void settle(const ApplicationServiceCompletion &,
                const std::optional<ReferenceCommittedState> &);
    void reconcile(const ReferenceCommittedState &);
    AccessDecision check(const ApplicationReferenceReadView &,
                         const OriginalApplicationCommand &) override;
    bool disclose_application(const ApplicationReferenceReadView &,
                              const OriginalApplicationCommand &,
                              const RecordedApplicationBundle &) override;
    bool disclose_holds(const ApplicationReferenceReadView &, const OriginalApplicationCommand &,
                        const ApplicationHoldActiveSet &) override;
    PolicyResolution resolve(const ApplicationReferenceReadView &,
                             const OriginalApplicationCommand &, const ApplicationBundleInputs &,
                             const AccessDecision &) override;
    ClockResolution observe(const ApplicationReferenceReadView &,
                            const OriginalApplicationCommand &, const ApplicationBundleInputs &,
                            const BoundApplicationPolicy &) override;
    ActionResolution prepare(const ApplicationReferenceReadView &,
                             const OriginalApplicationCommand &, const ApplicationBundleInputs &,
                             const BoundApplicationPolicy &,
                             const ApplicationCalculatedProposal &) override;
    ReferenceResolution issue(ApplicationReferencePurpose,
                              const ApplicationReferenceBinding &) override;

  private:
    ReferenceConfigurationSource &configuration_;
    ReferenceAuthoritySource &authority_;
    ReferenceClockSampler &clock_;
    ReferenceEntropy &entropy_;
    std::optional<ReferenceClockAnchor> anchor_;
    ReferenceRuntimeLimits limits_;
};
} // namespace subathon::server
