# ACCT-003 independent code and CI review

Implementer: `/root/application_plan_implementer` (`dcf0990`, integrated `df58c84`).
Reviewer: `/root/application_plan_reviewer`, 2026-10-31, separate session.
Coordinator CI/harness: `0e8ebbf`. Reviewed candidate: `df58c84`, based on merged
baseline main `8ed9d45`. Reviewer owns only this note on local branch
`task/ACCT-003-code-review`; no remote publication.

Verdict: **approved for code and CI integration**. No blocking findings. Hosted CI,
final status/evidence reconciliation and latest PR publication/merge remain gates.

| Artifact | Exact reviewed Git blob |
| --- | --- |
| Implementation | `42b47e4ae4c733ba8e0d8f743c32faa15376d425` |
| Preserved interface | `501112d64959d20f682420d33a260e7745f5b378` |
| Preserved22-case baseline | `f93287fa2c7c3ef4753e6bbbedd1d2a54f4529b9` |
| New workflow | `e43e856c3a0db3430f98adb37fb5c0feb1230289` |

## Contract and implementation assessment

Applied root/core/tests instructions and the workflow/test-first requirements.
Reviewed complete source and public header against full application-plans revision1,
ACCT-003 brief, implementation evidence, specification v0.45 §§3/6/7 and the
receipt/contribution/event-state/exact-numeric boundaries used in baseline review.
Baseline36ac654/final9aea69e approval and retained22 intended stub failures at
a524447 precede implementation; the merged baseline evidence remains unchanged.
No assertion, fixture or interface was weakened or rewritten.

Capture validates local identity and valuation shape before resolving sources. It
copies the complete captured input into owned revision1 and passes that owned event
to each source, retaining ordered returned selections. Raw amount/rate spellings,
optional unknown metadata, exact rational requests, signed/zero requested amounts,
action/reversal provenance and all independent contribution conditions are copied;
there is no parsing, floating-point conversion, rerounding or evaluator imitation.
Missing historical valuation rejects only dependent requests, preserves independent
ones, and cannot fetch a current rate or silently create zero valuation. Resolved
quotes must carry exactly the event's original money. Rate syntax/numeric results
and retained-reference resolution remain explicit upstream preconditions.

History validation checks nonempty plan identity/history, initial revision1 with
no parent/audit, every captured shape, whole original event equality, contiguous
increments, immediate parents and complete authorized decisions for every later
revision. Whole PlanEvent comparison covers receipt identity/facts, original arrival,
ancestry and contribution link, beyond individually exercised mutation examples.
The previous-revision maximum guard prevents overflowing the structural increment.

Revision checks a valid history and expected head before any source call, then
requires explicit authorized authority and actor/reason/decision references. Audit
strings alone cannot satisfy it. Replacement capture/event validation and maximum
head check precede source resolution. The increment is checked; practical runtime
exhaustion remains unreachable without an unrealizably large valid contiguous vector,
as documented during baseline review. No synthetic malformed max-head fixture is
misrepresented as overflow coverage.

A copied history receives one owned child revision, leaving every original revision
and caller value untouched on success or exceptions. Sources can have their own
recorded calls; this pure API does not promise rollback of external adapter state.
Caller permission evaluation and atomic expected-head persistence remain mandatory;
the authority enum is an upstream result, not an authentication mechanism.

Preview validates history and nonempty tokens, then returns stale_plan before
stale_state, otherwise current. It changes no plan/contribution and grants no
eligibility, permission or capacity. No timer cap, actual effect, provider/network,
Qt queue, SQL writer, storage transaction or delivery implementation enters core.
The compact validation/resolution helpers keep these boundaries explicit.

## Harness and workflow assessment

The optional build flag stays OFF by default. Changes to option/preset descriptions
and labels correctly replace intentional-red with core for the now-passing suite;
all22 source bodies and expectations are unchanged. The target still reuses selected
Boost::headers and existing GoogleTest resolution; no dependency is added.

New application-plan workflow mirrors the existing exact-numeric workflow:
Ubuntu24.04, read-only contents, checkout pinned to
3d3c42e5aac5ba805825da76410c181273ba90b1 with credentials persistence disabled,
15-minute timeout, exact libboost1.83-dev=1.83.0-2.1ubuntu3 package and compiler
BOOST_VERSION108300 check, explicit Boost/header config directories, and existing
GoogleTest commit063de7e9578f82b369302001269680b4b1553359 when fetch is needed.
PR/main push and manual triggers have no path filters; build/test use the same
application-plan preset and actual application-plan label. No skip, WILL_FAIL,
failure inversion or blanket continue-on-error appears. Static workflow inspection
does not establish actual hosted execution or branch-rule enforcement.

## Independent commands and limits

Reviewer Linux x86-64, GNU16.2.1, Python3.14.8, installed Boost1.92.0/GTest1.18.0;
local builds use installed dependencies, not the hosted Ubuntu1.83 package.

| Command | Actual result |
| --- | --- |
| `cmake --preset application-plan-debug` | exit0 |
| `cmake --build --preset application-plan-debug` | exit0,10 steps, no diagnostics |
| `ctest --preset application-plan-debug` | exit0,25/25: unchanged22 plan cases plus3 scaffold checks |
| `cmake --preset exact-numeric-debug` | exit0 |
| `cmake --build --preset exact-numeric-debug` | exit0,12 steps, no diagnostics |
| `ctest --preset exact-numeric-debug -L exact-numeric` | exit0,33/33 including numeric regression |
| `clang-format --dry-run --Werror core/src/application_plan.cpp` | exit0 |
| `git diff --check` | exit0 |
| `git ls-files -s` for reviewed artifacts | exact blobs above confirmed |

No additional business behavior or defect was introduced that requires new test
oracles beyond this reviewed baseline. Passing synthetic core cases do not prove
full evaluator correctness, historical persistence, permission enforcement, current
caps/lifecycle/debt, atomic application/effects/jobs, SQL/Qt ownership, concurrent
writers, providers or live/platform/release behavior. All seven integrations and
three rate providers remain required release scope.

Coordinator must update historical unimplemented/intentional-red status prose in
contract/strategy and final progress/acceptance evidence, then obtain actual latest
hosted CI and final integration/status review. This code approval does not claim
those later gates are already complete or authorize bypassing repository rules.

## Final integration and status review — 2026-10-31

Reviewer `/root/application_plan_reviewer` examined final candidate `d2a87b6`
(merged into reviewer worktree at `c479a1f`). Verdict: **approved for integration**;
no blocking findings. Source42b47e4a, interface501112d6, testsf93287fa and workflow
e43e856 remain unchanged from actual code/config approval4425064. Contract changes
are implementation/history status only; revision1 semantics and baseline artifacts
are preserved. No native test repetition was needed for these evidence/status edits.

Read the new implementation handoff/CI note and retained ignored evidence
`local-evidence/coordinator/application-plan-ci-run.log`. The log explicitly shows
PR merge checkout of df58c84 into base8ed9d45, GNU13.3.0, selected Boost1.83.0,
compiler macro BOOST_VERSION108300, successful configure/build and all22 actual
application-plan cases passing. Coordinator records successful pull_request run
41244388435 and all9 checks at df58c84. These observations substantiate that source
head; they do not assert future final-head checks have passed. Main push workflow
remains configured separately from this observed PR execution.

README/architecture/strategy accurately describe owned pure transforms and22 passing
preserved cases. Contract and brief retain reviewed-red history while recording
implementation and separate code review/automatic CI. Task ACCT-003 done and passed
acceptance/regression/code/CI fields are limited to this domain slice; writer,
permission, application, provider and platform/release remain not_run. The acceptance
map's passed deterministic-domain scenario explicitly excludes evaluator/numerical
correctness and real permissions/durable audit/current caps/effects/application.
Preview remains freshness only and cannot grant commit permission. No release scope
or unknown provider capability is upgraded by these status edits.

Reconstructed coordinator checkpoint correctly separates integrated PR20–22 history,
current main8ed9d45 and active PR23 final gates, retaining referenced baseline/review/
implementation artifacts. Cleanup33 completed remote heads is recorded with retained
local archives/support branches; this reviewer did not repeat deletion or publish any
support branch. Final active-head publication, latest9 CI checks, resolved discussions,
current repository rules/mergeability and merge remain coordinator gates.

Actual final reviewer checks: `python3 tools/check_project.py` exit0 (377 tasks,
122 decisions, links/presets/current HTML); `python3 tools/progress.py render --check`
exit0; `git diff --check` exit0. Source/test/workflow comparisons and proof/status
inspection were read-only. Only this independent review evidence was appended.
All previously recorded scope, provider, platform and persistence limits remain.
