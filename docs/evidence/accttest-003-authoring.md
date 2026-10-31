# ACCTTEST-003 authoring evidence

Author: application_plan_test_author (separate delegated session), 2026-10-31.
Base: main afcf528; coordinator-created branch task/ACCTTEST-003-plan-author.
Coordinator owns progress claim, all build/preset/state/HTML/map/CI/review files.
Author owns only the new core header/stub, acceptance tests, contract, this evidence
and ACCT-003 brief. No server service/fake is necessary for pure owned transforms.

Read root/core/server/tests instructions, FIRST_AGENT_PROMPT, CLION_SETUP,
architecture/workflow/test strategy, current state, ACCT-003 brief, specification
§§3/6/7 and receipt/contribution/numeric/time-credit/event-state contracts.
Environment validation/build harness and review/red are coordinated separately.

First artifact is proposed revision1 contract/interface with unconditional named
ApplicationPlanNotImplemented stubs. This is not implementation or independent
review. Tests are next; no bodies will run until an actual separate approval.

Scope: preserve owned historical input/context/rate/calculation/requested values;
audited already-authorized revisions; explicit historical valuation and preview
freshness. No permissions engine/provider capability validation/full evaluator,
actual cap application, transaction/durable history or wire schema claims.

CI assessment: new intentional-red lane remains isolated; coordinator adds harness.
Later ACCT-003 must add passing stable automatic CI. No dependencies are added;
existing selected Boost exact values and contribution/receipt types are reused.

## Authored examples and oracle boundaries

22 GoogleTest cases in `tests/acceptance/application_plan_tests.cpp` use literal
synthetic event/configuration/rule/modifier/rate/action references. Primitive rule
and modifier adapters copy literal selections and record calls; they contain no
matching/exclusion/combination logic. No repository fake or new dependency.

- §§3.2–5, NUM-01/04: event/receipt/contribution ancestry, unknown source fields,
  exact source amount/rate spelling, independent evidence/review/settlement holds.
- §§6/7: ordered selected/excluded rule explanation, definition/instance revisions,
  group/target combination operators; captured originals survive caller/source edits
  and expiry. The test asserts captured decisions, not evaluator correctness.
- NUM-01/04: already resolved120000ms (60 seconds×2), exact3/2 micro-points with
  requested2, exact−5/2 milliseconds with requested−2, zero whole count. Expected
  final amounts are hand-derived nearest-even examples, not computed by production.
  This slice copies these pairs rather than calculating or verifying them.
- B03/B04:120000 remains requested when a current-state token changes; no actual
  cap result or committed timer amount exists in this interface. Time-credit and
  ACCT-004 separately own actual cap computation/application.
- §§3.3–4: missing historical quote remains explicit; independent nonmonetary
  requests may be retained, valuation-dependent requests require a resolved quote.
  Not-applicable remains distinct. State/quote contradictions and substituted source
  precision fail explicitly; no current rate is fetched or silently substituted.
- §§3.4/7, MOD-04: authorized override appends parented revision2 then revision3,
  preserving original histories. A separately recorded historical/manual resolution
  does not confirm payment or accept review. Unknown/denied permission result is
  rejected despite populated audit references; complete audit and expected head
  are required. The authorization engine and atomic compare-and-commit are outside
  this pure API; the enum is an explicit caller precondition, not verified permission.
- Preview checks distinguish stale plan/current state and reject empty tokens or
  malformed history. A current preview changes no contribution state.

Potential ambiguity resolved locally: override calculation sources can supply
explicit replacement selections while original event arrival references remain
immutable. Referenced durable historical context must be retained by downstream
storage; these pure tests cannot prove resolvability/durability.

## Actual checks so far

- `git diff --check`: exit0 after authoring; whitespace only.
- `rg -c '^TEST' tests/acceptance/application_plan_tests.cpp`:22 authored cases.
- Initial git add/commit failed read-only worktree metadata; authorized local commit
  with sandbox escalation succeeded as d615cb4 (contract/header/stub/brief/evidence).
- Configure/build/list: awaiting coordinator harness; not yet run.
- Test bodies/red: not run; independent review required first. No production
  implementation, Qt/SQL, live/provider/permission engine, platform or API claim.

## Compile/discovery handoff after coordinator harness

Coordinator merged its isolated harness at a5ef409 after author commit6ed861e.
An evidence add/add conflict was resolved by the coordinator keeping the full author
content; no author assertion or behavior was removed. Source formatting uses the
repository clang-format. Scalar defaults explicitly initialize new projection IDs,
versions, flags and requested amounts; an omitted receipt remains invalid0 and
omitted valuation remains missing, never an invented success.

Actual author-worktree commands on Linux x86-64:

| Command | Result |
| --- | --- |
| `cmake --preset application-plan-debug` | exit0; GNU16.2.1, Python3.14.8, Boost1.92.0; CMake4.4.5, installed GTest1.18.0 |
| `cmake --build --preset application-plan-debug` | exit0; linked library/test executable; first build emitted nodiscard warnings in EXPECT_THROW; explicit void-discard fixed those test call sites |
| `cmake --build --preset application-plan-debug` (after correction and final defaults) | exit0;4 incremental steps, no diagnostics |
| `ctest --preset application-plan-debug -N -L application-plan` | exit0;22 cases listed; no bodies executed |
| `clang-format --dry-run --Werror core/include/subathon/core/application_plan.hpp core/src/application_plan.cpp tests/acceptance/application_plan_tests.cpp` | exit0 using clang-format23.1.4 |
| `git diff --check` | exit0 |

Exact final candidate blobs (before independent review):

| Artifact | Git blob |
| --- | --- |
| Core interface | `501112d64959d20f682420d33a260e7745f5b378` |
| Unconditional core stubs | `ac9dcd0a73ec12fffaf3bdb1ff45f9f1f80fc23f` |
|22 authored cases | `f93287fa2c7c3ef4753e6bbbedd1d2a54f4529b9` |
| Normative contract | `5bb9a914f8b80a7ad43cabcccf3905294a9f7a63` |

Separate reviewer must assess this exact interface/contract/test candidate and
harness. Coordinator owns intended runtime red only after actual approval, progress,
acceptance map, HTML, integration checks and PR. Nothing here establishes approved
baseline, implemented behavior, current-state cap correctness, permissions engine,
durable history/Qt ownership, full evaluator or live/platform verification.
