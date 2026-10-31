# CORETEST-002 independent draft review

Date: 2026-10-31. Author: `/root/numeric_test_author` (separate prior author
session). Reviewer: `/root/baseline_reviewer`. Verdict: **request changes**;
this is a partial review of an authored draft, not approval of the test baseline.
The 35-case follow-up below closes the two test-coverage findings; the unresolved
confirmation-credit policy remains the only blocking finding in this review.

## Actual revision and review scope

Reviewed the preserved author commits `68e0119` and `3ccc52d`, integrated by the
coordinator through `3059ed89aa7025661073b11e64cad810afff4378`, including coordinator
build-harness commit `2c4456710a1c750e48de3b47f6b4120016a2ca99`. The review branch
`task/CORETEST-002-timer-review` starts from main `806f69f`; it owns this evidence
only and does not author or change the contract, tests, stubs or shared build files.

Authority: specification v0.45 §4 and §18, TIMER-01/02, the reviewed
[event/state boundaries revision1](../contracts/event-state-boundaries-v1.md), and
the actual [timer lifecycle draft revision1](../contracts/timer-lifecycle-v1.md).
Read the root/module instructions, coordinator instructions, architecture,
workflow, test strategy, first-assignment preparation, CORE-002 task brief and
author evidence before reviewing.

| Reviewed artifact | Actual Git blob |
| --- | --- |
| `core/include/subathon/core/timer_lifecycle.hpp` | `69cad7023b9928371877d45955b800aed9996cd5` |
| `core/src/timer_lifecycle.cpp` | `5584f845faef1dff68377363ec04c7b2effc4cae` |
| `tests/acceptance/timer_lifecycle_tests.cpp` (33 cases) | `5fdcde0c4a5fecb6d3b7ea7b63ecbb89a9517f1f` |
| `docs/contracts/timer-lifecycle-v1.md` | `b88657ce48447e7d509d683a0b5c89cea567ac00` |
| `docs/tasks/EPIC-001/CORE-002.md` | `1b6be5b7f2c926d4e3e7ec7ca9c9b9c0409e237f` |

## Findings requiring resolution

1. **Blocking product decision — positive credit during confirmation wait.** The
   specification chooses an authorized-confirmation ending mode but does not settle
   whether accepted positive time automatically resumes Main or leaves an explicit
   pending decision. The draft correctly identifies this gap and has no invented
   oracle. An explicit owner decision, recorded contract/examples and actual tests
   for the chosen behavior are required before full baseline approval. Repeated
   requests to resume coordination are not an answer. No timer test bodies or
   intended red run are approved by this partial review.
2. **Focused validation coverage gap (closed by follow-up below).** The contract promises rejection of
   incoherent snapshots, but the tests check negative values and invalid command
   lifecycle/preconditions without exercising the distinct state-consistency
   branches (actual contract lines139–146; existing negative tests lines336–344).
   Add specification-linked contract cases for Grace with positive Main,
   confirmation wait with positive Main or Grace phase, and waiting state advanced
   with a non-Confirmation mode. These are declared local API invariants, not new
   product policies. Exercise the relevant public boundaries that accept each
   malformed state; group closely related invalid states rather than adding broad
   repetitive smoke coverage.
3. **Focused shared-budget coverage gap (closed by follow-up below).** The contract states that an exhausted
   shared budget enters and expires Grace without an extra tick (lines60–64). Existing zero
   Grace coverage uses Fresh mode with configured duration zero; it does not cover
   Shared with retained budget zero and a positive configured duration. Add that
   exact Main-expiry example and assert Finished/Grace0 with the ordered four-event
   trace. This protects TIMER-02's distinct retained-budget path and its zero
   boundary rather than duplicating the Fresh configuration case.

The final authored revision needs a separate review after these changes and the
owner answer. Nothing here authorizes changing or weakening existing assertions.

## Findings checked without further objection

- The 33 authored cases use explicit synthetic values and manually derived
  subtraction/trace oracles; they never use the production reducer to generate
  expected values. The helpers only construct snapshots.
- Default 120000ms Grace, exact boundaries, immediate/confirmation ending,
  positive/zero rescue, Fresh/Shared reuse, pause freezing, overshoot through both
  phases and signed64 limits match the bounded contract. The one-step full trace
  preserves MainExpired → GraceStarted → GraceExpired → Finished.
- Elapsed milliseconds and UTC observation remain separate. Pause is constant over
  an already segmented interval. Hard-end observation priority explicitly retains
  the pre-call snapshot without inventing within-interval UTC transition times or
  cross-domain interpolation. This claims neither wall-clock chronology nor
  recovery/skew correctness.
- Ready is represented and rejected by the already-started advance helper as an
  explicit API precondition. This is not a decision about first start, Ready at a
  hard end or starting after a passed deadline; actual coordinator follow-up gates
  remain in API-007 and CORE-008. No scope reduction was inferred.
- Same-instant advance-before-command ordering, Finished rejection and authorized
  confirmation remain explicit. Eligibility, caps/debt, pause reasons, permissions,
  persistence, event ancestry and transaction wiring are external responsibilities.
- Source contains only three unconditional `TimerLifecycleNotImplemented` throws;
  there is no production behavior to approve. The optional CMake lane links the
  correct stub target and GoogleTest, discovers cases, and labels them
  `timer-lifecycle;acceptance;intentional-red`. It uses no failure inversion,
  assertion skipping or changes to stable baseline targets.

## Commands and evidence limits

Commands actually run by this reviewer: `git status --short`, `git worktree list`,
`git branch --show-current`, `git show --stat 3059ed8`, `git show --stat 2c44567`,
`git show --stat 68e0119`, `git log --oneline -7`, artifact-specific `git rev-parse
3059ed8:<path>`, `git show 3059ed8:<follow-up task path>`, and targeted `cat`, `sed`
and `rg` reads. All completed successfully; the actual blobs match the author's
recorded identities. `git diff --check` and `git diff --cached --check` passed
before commit.

No reviewer compilation, discovery execution, timer test body, red run or production
test was performed. Author-reported compilation is author evidence only. After the
actual final baseline receives independent approval, the coordinator must compile
and observe `ctest --preset timer-lifecycle-debug -L timer-lifecycle` failing for
explicit missing behavior, with exact revision/environment/results recorded.

CI assessment: the isolated draft lane is appropriate for intentional-red work;
stable scaffold/credit/numeric suites must remain passing. CORE-002 must introduce
and observe automatic passing lifecycle CI before implementation completion. This
review establishes no Qt/service loop, SQL, platform, renderer, OBS, packaging or
live-provider capability. All downstream and release gates remain open.

## Follow-up review of focused test corrections

Separate correction author: `/root/timer_test_fixes`. Actual author revision:
`ef82bbcf12b9ee8074ab27c18bc41f986a786c8e`, integrated by the coordinator as
`e754278`. Reviewer inspected the full correction diff, then merged the actual
candidate into this isolated review branch for compilation/listing; pre-evidence
review checkout was `136be34fb7933b27c1d0580342adb09775d3399b`. The test blob is now
`7a802f2151da1d62b85f27c357528f63426cf2d0` (35 cases). Header, stub and behavioral
contract are unchanged from the artifact identities above. The change adds two
test cases and author evidence; all original 33 cases and assertions are preserved.

- `IncoherentPhaseAndConfirmationWaitSnapshotsAreRejected` exercises Grace with
  positive Main, wait with positive Main, and wait in Grace through advance,
  applicable-credit and confirmation boundaries. It separately checks a valid
  waiting snapshot against Immediate and Grace configurations. Credit uses zero
  and malformed snapshots; it asserts no policy for valid positive credit during
  confirmation wait. **Finding2 closed.**
- `ExhaustedSharedBudgetExpiresAtMainZeroWithoutAnArtificialDelay` uses Shared,
  configured Grace120000, retained budget0, Main1000 and elapsed1000. Its manually
  derived expected result is Finished/Grace with both durations0 and the exact
  four-transition trace. This distinguishes exhausted Shared from Fresh/config0.
  **Finding3 closed.**

Reviewer commands in `local-evidence/worktrees/coretest002-review`:

| Command | Observed result |
| --- | --- |
| `git diff 3059ed8 e754278 -- tests/acceptance/timer_lifecycle_tests.cpp core/include/subathon/core/timer_lifecycle.hpp core/src/timer_lifecycle.cpp docs/contracts/timer-lifecycle-v1.md` | exit0; exactly the two added cases, no other artifact change |
| `cmake --preset timer-lifecycle-debug` | exit0; GCC16.2.1 and Python3.14.8 |
| `cmake --build --preset timer-lifecycle-debug` | exit0; all10 compile/link/discovery steps complete |
| `ctest --preset timer-lifecycle-debug -N -L timer-lifecycle` | exit0;35 cases listed, no bodies executed |
| `c++ -dumpfullversion`, `cmake --version`, `pkg-config --modversion gtest` | exit0;16.2.1,4.4.5 and1.18.0 respectively |
| `git diff --check` | exit0 before follow-up evidence commit |

This approves these two bounded coverage corrections only. Overall verdict remains
**request changes** until the owner decision and its actual contract/tests receive
separate review. Compilation and listing establish neither red nor passing
behavior. No timer test body, production code or intended-red run was executed or
authorized by this follow-up.
