# Test strategy and first baseline

| Suite | Responsibility | Current state |
| --- | --- | --- |
| GoogleTest | Core calculations/rules/accounting/reversals/milestones | 19 reviewed credit-boundary cases; intended red observed, behavior unimplemented |
| GoogleMock | Meaningful adapter/service interface interactions | Planned |
| Qt Test | Qt signals/models/event loops | Optional environment probe only |
| Qt Quick Test | QML components/behavior | Planned |
| CTest | Native suite runner | Local scaffold 3/3 and Qt probe 4/4 passed; acceptance 19/19 intended failures |
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
[baseline handoff](evidence/time-credit-baseline-handoff.md). No algorithm is supplied.
The retained CTest `draft` label is historical suite metadata; the linked review and
progress evidence identify the exact approved baseline. It is not a passing product suite.

Before implementation: review contract/oracles; decide/add invalid input, integer-boundary
and interaction tests through the proper contracts; independently review; run and record
red tests caused by missing behavior. Compilation errors do not count. Do not mark
unknown provider capabilities as supported using a fictional fixture.

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
not-run and unsupported. Default scaffold CI excludes unreviewed feature tests explicitly;
manual acceptance CI genuinely fails while the stub remains. It must not turn a missing
dependency or failing test into a passing feature claim.

Individual test-drafting tasks may be marked authored/done when their draft artifacts
are ready for review; they do not complete the reviewed/red-baseline gate. The separate
TEST-002 task represents the handoff-ready baseline after review and observed failure.
