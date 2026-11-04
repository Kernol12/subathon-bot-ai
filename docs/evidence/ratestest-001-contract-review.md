# RATESTEST-001 independent logical document review

Reviewer: codec_author, a different agent from author service_author. Date:
2026-11-04. Review branch `task/RATESTEST-001-contract-review`; ownership is this
note only. No contract, source, fixture, test, state, build or CI file is edited.

Reviewed initial integrated candidate `f4f321a470f886e44932a1bb0209dd80d7a31ca9`
(author commit `af2f9a9`):

- [Rate snapshot proposal](../contracts/rate-snapshots-v1.md), Git blob
  `36291ce33dc27242f6ab6ace29b776175422831a`.
- [Author evidence](ratestest-001-contract-author.md), Git blob
  `46acb47b410d286cdbb7e52f66cc3d237813a455`.
- Root's required [RATES-009 bridge brief](../tasks/EPIC-017/RATES-009.md),
  Git blob `a5a80ee93b5e3ca1ec030d5f217a253b749a54f3`, its planned baseline/
  implementation dependencies in state, and EPIC-017's required children.

Authority read: root AGENTS, architecture/workflow/test strategy, RATES-001/007/009,
specification v0.45 §§3.5/9/15 and NUM-04/FX-06, exact-decimal revision 1/header,
event/state boundaries and captured-plan contract/header, plus STORAGE-015's
historical valuation representation. There is no narrower docs AGENTS file.

## Initial verdict: request changes

RS10 needs one explicit retention distinction. Section 3 includes an observation
inside an EffectiveSnapshot only when at least one fetched quote from it remains
an active constraint. Under RS09, pins imply USD→EUR 5/4 and USD→GBP 5/8. O1 has
only 6/5 and 3/5, so neither quote can remain active. RS10 must say O1 remains
unchanged in retained history **outside** that EffectiveSnapshot, as section 3
already permits. Otherwise the example appears to describe an impossible accepted
snapshot. Preserve the selected-observation invariant and the original evidence;
no new product choice or discarded pin is needed.

Also requested a bounded clarity correction: RS15's 256-currency boundary is the
table's 256 **effective component members**. Unselected retained quote rows may
mention other currency tokens within their own quote/byte limits. The example
must not invent a tighter bound on all fetched currencies.

Both findings were sent to root and author before approval. The coordinator
authorized these routine document corrections. This initial request preceded the
actual revised-artifact review recorded below.

## Findings from the full initial review

OriginalMoney retains source spelling, source currency and exact parsed value;
PositiveRate reuses the reviewed decimal grammar/domain/bounds. Opaque byte keys
do not assert ISO identity or provider support. Independent optional source,
fetch, publication and raw-time fields preserve unknown/present-empty/epoch-zero
states without inferred chronology or date-midnight conversion.

Positive source observations remain immutable evidence even when rounded cycles
disagree. Effective components have canonical anchors and positive reduced exact
potentials. Resolving constraints, checking their equality and undirected
connectivity, and retaining every active manual reference are sufficient to reject
unbacked values, contradictory cycles and omitted supplied pins without inventing
a graph solver. Temporal pin removal, relevant-update expiry, authorization and
selection policy remain RATES-007. Reporting changes cannot mutate old evidence,
credited effects, milestones or reversal provenance.

RS01–17 were checked independently against those rules. A read-only Python
`fractions.Fraction` calculation, importing no project module, asserted 17 literal
place-value, ratio and bounded-product facts: 2469/2000; rate 6/5; RS03 weights
EUR=1, GBP=1/2, USD=5/6; inverse/cross 5/6, 1/2, 5/3, 3/5; inconsistent cycle
3/5≠7/10; RS09 weights EUR=1, GBP=1/2, USD=4/5 and cross 5/8; both O1/pin
mismatches; RS16 product 7407/5000; and two 4096-bit components' product can
require 8192 bits.
All assertions passed. Other examples' identity/missing, unknown-time, provenance,
immutability and boundary claims were checked by document inspection. These are
arithmetic/document checks, not executable feature tests or baseline acceptance.

The 4096-bit potential/8192-bit pair bounds do not relax CORE-001's 4096-bit finalizer
limit. Concrete DTO ownership walks, intermediate comparisons, exceptions and
error priority explicitly await separate API review. No unbounded graph arithmetic,
decimal approximation or fitting-prefix result is authorized by this proposal.

The existing HistoricalValuation decimal-only field genuinely cannot represent 5/6.
Required RATESTEST-009/RATES-009 and EPIC-017 dependencies track a versioned exact
valuation/capture/command/codec/durable-plan bridge with compatibility/migration,
separate reviewed tests and observed red. The brief forbids silently rounding or
reporting an available recurring rate as missing. The pure model is not an award
adapter; all three providers, history conversion, scheduler/cache, permissions,
UI and actual platform/provider verification retain their separate gates.

## Checks and remaining gates

- Git/status, exact blob reads and contract/header/specification inspection: done.
- Independent Fraction-only arithmetic command: exit 0, 17 assertions passed.
- `git diff --check` before authoring this note: exit 0.
- No project feature, new API/stub, test suite, provider, network request or native
  behavior was invoked. No feature baseline, observed red, implementation or CI
  acceptance is certified.

## Revised artifact: approved for logical document integration

Re-reviewed author correction `ce1b9269bd7f80a942c93d289c1a259c33ca114f`,
imported by root as `0c9769192fba795512e74ce1242d08a18c33d066`:

- Contract blob `76d2d9b3e542c56b7475e99defd717d9902885a5`.
- Author evidence blob `c5e87c1888bbdb78a9d7e6a1dffebe1824cf8b9d`.

Read the exact `git diff f4f321a 0c97691` for both authored documents and resolved
those blobs with `git rev-parse`. The complete change is two corrected example
rows and the author's truthful review/correction history. RS10 now explicitly
retains O1 outside the pin-only snapshot and fabricates no active fetched edge.
RS15 now applies the 256-member ceiling to effective components, preserving the
separate observation limits. Both findings are resolved. All other source/rate
literals, invariants, provenance requirements and the required bridge gate remain
unchanged. No additional substantive document issue was found.

Final verdict: **approved for this logical document stage only**. This does not
approve a future C++ API, baseline fixture/test, stub, implementation or usable
valuation adapter. RATESTEST-001 remains incomplete until its remaining reviewed
API/test/red gates; RATES-001 remains planned. No additional owner decision is
required by this review.

Checks in the isolated review worktree (base `f4f321a`, corrected candidate read
by exact Git objects): `python3 tools/check_project.py` passed with 398 task records,
122 decisions and local links/presets/current HTML; `python3 tools/progress.py
render --check` passed. These are documentation checks only. The final owned-note
whitespace/staged-path checks are recorded with its commit handoff.

Next gate: a separately reviewed concrete API, independent literal fixtures/tests
and their review/intended behavioral red remain required before RATES-001
implementation. Root owns shared progress, publication and task completion evidence.
