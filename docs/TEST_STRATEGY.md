# Test strategy and first baseline

| Suite | Responsibility | Current state |
| --- | --- | --- |
| GoogleTest | Core calculations/rules/accounting/reversals/milestones | Credit19+4, exact-decimal32+1, timer39, receipt15 and contribution21 simulation cases pass; bounded slices only |
| GoogleMock | Meaningful adapter/service interface interactions | Planned |
| Qt Test | Qt signals/models/event loops | Optional environment probe only |
| Qt Quick Test | QML components/behavior | Planned |
| CTest | Native suite runner | Local scaffold 3/3 and Qt probe 4/4 passed; credit23/23, exact-numeric33/33 timer39/39, receipt15/15 and contribution21/21 simulation pass |
| Vitest | Browser unit/component behavior | Planned |
| Playwright | Browser workflows | Planned; separate OBS checks still required |
| Python unittest | Development dashboard/consistency tools only | Scaffold tooling, not product test coverage |

## Initial behavior

`calculate_time_credit` covers a narrow valid-input active-main-timer planning case.
It has no persistence, refunds, debt, grace, hard end, input review or concurrency behavior.
Read docs/contracts/time-credit-v0.md (revision 1). The original six fixtures/assertions
are preserved among 19 cases with independent REVIEW-001 approval. TEST-002 observed
19/19 failures from FeatureNotImplemented after successful compilation. See
[the review](evidence/time-credit-independent-review.md) and
[baseline handoff](evidence/time-credit-baseline-handoff.md) for that historical red run.
IMPL-001 implements this numeric contract and preserves all 19 cases. Four additional
reviewed regression cases cover signed-minimum validation, above-cap error precedence,
a full maximum cap and the largest uncapped request from zero. They also received
an intended red run before implementation. Passing checks and independent code
review are recorded in the [implementation handoff](evidence/impl-001-handoff.md).
The retained CTest `draft` label is historical suite metadata; the linked review and
progress evidence identify the exact approved baseline. It does not establish any
timer/service, refund, persistence or UI behavior.

Before implementation: review contract/oracles; decide/add invalid input, integer-boundary
and interaction tests through the proper contracts; independently review; run and record
red tests caused by missing behavior. Compilation errors do not count. Do not mark
unknown provider capabilities as supported using a fictional fixture.

## Exact numeric baseline

CORETEST-001 adds [exact decimal revision1](contracts/exact-decimal-v1.md): owned
lossless decimals, positive rates/nonnegative factors and signed64 half-even final
conversion. Its32 independently reviewed cases first failed on explicit stubs;
[baseline evidence](evidence/coretest-001-baseline-handoff.md) retains exact artifacts
and actual exit8. CORE-001 implements that revision; all32 unchanged cases plus one
separately reviewed normalized-zero regression pass, with independent code review
and [Ubuntu/Boost1.83 CI evidence](evidence/core-001-ci.md). See the
[implementation handoff](evidence/core-001-handoff.md).

Run the explicit lane with `cmake --preset exact-numeric-debug`,
`cmake --build --preset exact-numeric-debug` and
`ctest --preset exact-numeric-debug -L exact-numeric`. Installed Boost headers and
GoogleTest and Boost>=1.83 are required. The numeric, credit and scaffold workflows
run automatically on relevant PRs/main; numeric uses the pinned Ubuntu package
and actual compiler/header/config verification.
Per-input atomic rejection, aggregates, modifier combination, refunds and all platform
behavior remain their own baselines and are not covered by these primitive cases.

## Identified receipt service

ACCTTEST-001 supplied15 independently reviewed cases for stable receipt identity,
retry/conflict audit and per-attempt provenance through an atomic repository port.
[Original red evidence](evidence/accttest-001-baseline-handoff.md) retains all15 stub
failures before ACCT-001 implementation. The unchanged cases now pass against the
real service decision logic and a primitive disposable store; separate code review
and [automatic Ubuntu simulation CI](evidence/acct-001-ci.md) passed.
[Implementation handoff](evidence/acct-001-handoff.md) records exact source/checks.

Run configure/build `receipt-intake-debug`, then CTest with `-L receipt-intake`.
The receipt/numeric/credit/scaffold lanes all run automatically on relevant PRs/main.
The fake does not classify retries/conflicts or enforce uniqueness; passing this
suite remains simulation evidence. SQL durability/uniqueness, Qt ownership, live
identifiers, source admission, contribution creation and actual effects remain
separate acceptance/platform/provider gates.

## Contribution eligibility and linkage

ACCTTEST-002 supplied21 independently reviewed cases and the original intended stub-red;
[baseline handoff](evidence/accttest-002-baseline-handoff.md) retains that sequence.
ACCT-002 implements the pure assessor and writer-port linkage service. All21 unchanged
cases pass with separate code review and [observed automatic simulation CI](evidence/acct-002-ci.md).
Use configure/build `contribution-evidence-debug`, then CTest `-L contribution-evidence`;
that same stable lane runs automatically on relevant PRs/main. The primitive fake
provides simulation storage, not SQL/Qt/provider/application/refund verification;
[handoff](evidence/acct-002-handoff.md) records exact artifacts and remaining gates.

## Priorities after first baseline

- Duplicate receipts/commands, atomic effects/state/jobs and restart recovery.
- Exact half-even rounding and signed 64-bit overflow bounds, partial/full refunds,
  actual cap-limited credit, refund debt and preserved completed milestones.
- Main/grace/pause/recovery/hard-end and scheduled pause/DST combinations.
- Arrival-time rules/modifiers, exclusions and generated-event bounds.
- Permission checks, stale revisions, token/session and redaction boundaries.
- All provider adapters: evidence, identifiers, overlap, gaps and capability limits.
- Independent native/browser scene reference behavior and actual OBS/Wayland/Windows output.

Use controllable clocks rather than sleeps; deterministic fake adapters and reviewed
synthetic/redacted fixtures. Expected values must be independently derived, not computed
by the code under test. Each behavior has one primary testing home. No blanket coverage
percentage or number of green tests replaces the acceptance map and review.

## Required evidence

Use docs/templates/VERIFICATION.md. Record expected red separately from pass, fail,
not-run and unsupported. Scaffold CI excludes unreviewed feature tests explicitly.
Credit acceptance/regression CI runs automatically for PRs targeting main and pushes
to main, with manual dispatch retained. It executes the real reviewed cases without
failure inversion. It must not turn a missing dependency or failing test into a
passing feature claim. Extend CI with executable product capabilities according to
[the workflow policy](AGENT_WORKFLOW.md#evolving-continuous-integration); record gaps
and actual results separately from live/platform evidence.

Individual test-drafting tasks may be marked authored/done when their draft artifacts
are ready for review; they do not complete the reviewed/red-baseline gate. The separate
TEST-002 task represents the handoff-ready baseline after review and observed failure.

## Timer lifecycle implementation

CORETEST-002 supplied39 independently reviewed cases and observed intended stub-red;
[baseline evidence](evidence/coretest-002-baseline-handoff.md) retains the original run.
CORE-002 implements the bounded pure Main/Grace reducer: all39 unchanged cases now
pass with separate code review and [observed automatic timer CI](evidence/core-002-ci.md).
Run configure/build `timer-lifecycle-debug`, then CTest `-L timer-lifecycle`; that same
lane runs automatically on relevant PRs/main. [Implementation handoff](evidence/core-002-handoff.md)
records scope and exact artifacts. Pause reasons, queue/state transactions, start/
reopen, recovery and schedule/DST remain separate contracts and acceptance gates.
