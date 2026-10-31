# ACCT-002 independent implementation/configuration review

Date: 2026-10-31. Implementer: `/root/contribution_implementer`.
Reviewer: `/root/baseline_reviewer`, separate from source/test authors.
Verdict: **source, configuration and final integration approved** at the actual
revisions below. Latest-head PR/CI/discussion/merge eligibility remains the
coordinator's responsibility. Verification is pure assessment plus
primitive-repository simulation, not SQL, provider, Qt or application support.

## Exact actual candidate and baseline integrity

Reviewed source author `3e76ce9`, integrated as `49112b3`; coordinator configuration
`1beb71f09c2228229c33a0e8a05c53936d21fb0a`. Actual complete reviewer checkout:
`a579e38ba7f132e283ec5bbacd37e8be2308b458`, reconciled with integrated timer
main `0700d5d`. Isolated branch/worktree: `task/ACCT-002-code-review`,
`local-evidence/worktrees/acct002-review`. This reviewer owns this evidence document
only and has changed no source, interfaces, fixtures, tests, fake, build or progress.

Authority: specification v0.45 §§3.2–4,5,14,18 and settlement boundary §8;
[contribution eligibility revision1](../contracts/contribution-eligibility-v1.md),
[event/state boundaries](../contracts/event-state-boundaries-v1.md),
[receipt intake revision1](../contracts/receipt-intake-v1.md) and
[ACCT-002 brief](../tasks/EPIC-002/ACCT-002.md). Root/scoped instructions and
workflow/test strategy were read. The [baseline handoff](accttest-002-baseline-handoff.md)
retains separate review `a561508`, actual21-case stub-red at `4d1400f`, final review
`3ffb71d` and shared-artifact review `4e614fe`, before production implementation.

| Reviewed artifact | Exact Git blob |
| --- | --- |
| Implemented core assessor | `c3b2ea0d8449fd3752512b35eac052b2f300ece7` |
| Implemented server observation service | `975067bb952383e08c015d87139d6a806240ecb0` |
| Preserved core interface | `65ad81ac0dc3fc808421258ab89b30ff610be085` |
| Preserved server interface/port | `82c2e5f7315b3f2a9ab15fdec87104f6456c970a` |
| Preserved21 reviewed tests | `702a7bf1d8162ba748ee9477ea48ffa73047ef65` |
| Preserved primitive fake | `8d35e502a9656d0e65555efb48b3bd8650bd0c5e` |
| New contribution workflow | `df69ccc3bf4455a156375f251b876b743bce1491` |
| Root `CMakeLists.txt` | `e398b8cb52088f1b6c494db00d5a2041ce6be1d2` |
| `CMakePresets.json` | `3b2f4dc345b22c7f5dac924e6e8fb67306ebe0eb` |
| `tests/CMakeLists.txt` | `40eaeb1bb979bd89ac3992dd55fe8014e3657357` |

Diff against main0700d5d confirms no header/test/fake/behavioral-contract or selected
GoogleTest dependency change. Historical reviewed assertions are preserved. Current
contract status prose still describes explicit stubs at this source/config checkpoint;
the coordinator was notified to update that framing in final completion evidence.
That prose update must not change normative behavior or historical baseline identities.

## Source assessment beyond the existing assertions

No blocking source findings in this bounded implementation.

- The assessor reads a const state and independently adds every applicable blocker.
  Captured record-only always blocks. Unknown/unconfirmed evidence under review policy
  requires the separately scoped evidence decision; ordinary acceptance cannot replace
  it. Explicit provider-report admission retains the original evidence label. Confirmed
  and not-applicable remain distinct caller-supplied facts. Defaults stay conservative.
- Monetary assessment separately tests source money, marked missing rate and unresolved
  valuation. A resolution reference cannot invent amount/zero or clear a missing-rate
  flag. Explicit nonmonetary inputs avoid valuation holds; review/application/settlement
  checks still run. Review rejection/pending cannot be bypassed by evidence or rate
  resolution. Applied inputs block even after reversal; never-applied reversal or an
  unresolved settlement stays held for ACCT-006. No dimension/reference is mutated.
- The service rejects support without conditions, confirmation with conditions or
  non-confirmed evidence, and unknown observation kinds. A supplied verified key must
  match receipt origin and contain all source/namespace/opaque/proof fields. These
  checks validate shape/presence only; they do not establish provider proof. Committed
  receipt validity and caller-established source evidence remain explicit preconditions.
- Receipt lookup occurs before activity linkage. Exact equality compares the complete
  immutable observation, excluding growing transport attempts by construction. Changed
  receipt projection/classification/key/proof/conditions rejects without replacing
  history. Exact repeats return their current stored association and commit before
  returning, including an initially unresolved confirmation linked by later support.
- Only verified provider/account/namespace/opaque activity identity selects an existing
  contribution; verification reference is retained observation evidence, not a key.
  No raw payment reference, name, amount, source timestamp or arrival resemblance is
  used for correlation. Missing verified identity cannot attach supplementary proof.
- Existing-activity observations append full audit data and preserve original support
  facts/context/conditions. Only explicit confirmed evidence promotes aggregate payment
  evidence, using a copied current state with all other fields intact. Weaker delayed
  evidence cannot downgrade it and remains auditable with its real label. Application,
  reversal, settlement, review and valuation are never overwritten from late support.
- Confirmation without matching support persists unresolved and creates no contribution.
  New support uses its own original snapshot/conditions even when matching confirmations
  arrived first. Code considers every returned pending observation, promotes aggregate
  evidence from confirmed proof and links every pending receipt inside the same
  transaction. One verified activity retains one contribution; this is not an actual
  one-application transaction or candidate-delivery implementation.
- Every return follows a successful commit. Owned transaction lifetime unwinds on
  lookup/create/append/update/link/commit exceptions; no exception is swallowed and
  there is no hidden retry, cache, pre-commit publication or independent writer.
  The fake supplies primitive staged storage/failure injection, not these service
  decisions. Real transaction isolation, uniqueness and unknown commit outcomes remain
  adapter/storage obligations. No effects, jobs, candidates or financial computation
  are performed by either behavior function.

## CI/build configuration assessment

The new contribution workflow runs PRs to main, main pushes and manual dispatch
without path filters. It retains the selected immutable checkout/GoogleTest pins,
disables persisted checkout credentials, grants only contents:read and uses an
Ubuntu24.04 job with a15-minute timeout. Commands use the same explicit preset and
primary contribution-evidence label as local verification. No new dependency,
package installation, provider credential, live state, skip, failure inversion or
continue-on-error is introduced. Existing timer/receipt/numeric/credit/scaffold
workflows remain intact. Local labels now accurately read
`contribution-evidence;acceptance;simulation;server`.

Approval of configuration is not observation of hosted CI. Actual remote output and
latest-revision checks must be inspected before completion/merge; local extra labels
must not be asserted as observed remote metadata without evidence. Adding a workflow
also does not configure required-status branch protection.

## Independent verification

Commands below ran independently in the reviewer worktree at actuala579e38.
Linux x86-64; GCC16.2.1, Python3.14.8, CMake4.4.5, installed GoogleTest1.18.0;
the exact-numeric lane selected installed Boost1.92.0. No download/install or live
operation occurred; the explicit GoogleTest fetch flag used the installed package.

| Exact commands | Observed result |
| --- | --- |
| `cmake --preset contribution-evidence-debug -DSUBATHON_FETCH_GOOGLETEST=ON` then `cmake --build --preset contribution-evidence-debug` | exit0;12 build/discovery steps complete |
| `ctest --preset contribution-evidence-debug -L contribution-evidence` | exit0;21/21 pass with local simulation/server metadata |
| `cmake --preset timer-lifecycle-debug` then `cmake --build --preset timer-lifecycle-debug` | exit0 |
| `ctest --preset timer-lifecycle-debug -L timer-lifecycle` | exit0;39/39 pass |
| `cmake --preset receipt-intake-debug` then `cmake --build --preset receipt-intake-debug` | exit0 |
| `ctest --preset receipt-intake-debug -L receipt-intake` | exit0;15/15 pass |
| `cmake --preset exact-numeric-debug` then `cmake --build --preset exact-numeric-debug` | exit0;Boost1.92.0 selected |
| `ctest --preset exact-numeric-debug -L exact-numeric` | exit0;33/33 pass |
| `cmake --preset acceptance-debug` then `cmake --build --preset acceptance-debug` | exit0 |
| `ctest --preset acceptance-debug -L 'acceptance\|regression'` | exit0;23/23 pass |
| `cmake --preset scaffold-debug` then `cmake --build --preset scaffold-debug` | exit0 |
| `ctest --preset scaffold-debug` | exit0;3/3 pass, including19 tooling tests/project consistency |
| `python3 tools/progress.py render --check` | exit0; current snapshot |
| Artifact-specific `git rev-parse` and preserved-baseline `git diff` | exit0; exact identities/no reviewed baseline changes |
| `git diff --check` and `git diff --cached --check` | exit0 before evidence commit |

These passing cases and code review support the bounded pure-unit/service simulation.
They establish no SQL uniqueness/durability/restart/concurrency or indeterminate
commit handling, Qt writer/queue ownership, live identifiers/proof/correlation,
permissioned review operations, effect application or refund settlement. Those
storage/API/connector/accounting and release/platform gates remain required. Final
current-status prose/map/state, hosted CI and publication/merge outcomes are not
approved in advance by this source/configuration review.

## Final implementation/status integration review

Reviewer `/root/baseline_reviewer`, 2026-10-31. Actual completion candidate:
`60b1f8af7f997030caa5e1e27ac1416f68c52384`; isolated review checkout before this
append was `c8d4a7428aff564e1633779112141c5b276fdb9f`, with an identical candidate
tree. Verdict: **approved for integration** of this bounded implementation and
completion evidence, subject to final published-head CI and PR/merge gates. No
latest-head merge eligibility or merge outcome is claimed by this approval.

Compared the completion diff against approveda579e38: core source
`c3b2ea0d8449fd3752512b35eac052b2f300ece7`, service source
`975067bb952383e08c015d87139d6a806240ecb0`,21 tests
`702a7bf1d8162ba748ee9477ea48ffa73047ef65`, both interfaces, primitive fake,
workflow `df69ccc3bf4455a156375f251b876b743bce1491`, selected dependency configuration
and all build/preset files are unchanged. Contract/brief changes update current
implementation status, historical ownership and evidence links only; no normative
eligibility, identity, error or transaction semantics changed. No reviewed assertion
or fixture was weakened or skipped. Prior independent native passes remain valid.

Independently queried actual [run41235930193](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41235930193).
GitHub reports successful completion of the pull_request job at exact head
`a579e38ba7f132e283ec5bbacd37e8be2308b458`, with configure, build and test steps all
successful. The actual log identifies GNU13.3.0 and all21 reviewed case names passing,
with0 failures. Its label summary establishes contribution-evidence only; local
additional simulation/server/acceptance labels are correctly kept separate in the
new CI note. Hosted execution remains evidence of the same pure/fake-backed suite,
not a database, provider, Qt writer or application environment.

Final README, architecture and Test Strategy describe the implemented assessor and
repository-port linkage with their bounded verification limits. `ACCT-002` is done
for its implementation acceptance/regression/review/observed-CI gates; SQL/Qt/provider
and platform/release verification remain not_run. The acceptance-map scenario is
passed with `verification_level: simulation`, preserved original red evidence and
separate code review. No provider proof, application/effect/job, authorization/review
operation or refund/settlement gate is upgraded. The checkpoint retains known main
0700d5d/merged timer work, current contribution PR19, and separate proposed pause
work with two explicitly unanswered owner questions. No pause implementation or
policy answer is inferred from that concurrent drafting work.

Independent final checks:

| Exact command / check | Observed result |
| --- | --- |
| `git diff a579e38 60b1f8a -- <source/interface/test/fake/workflow/build/dependency paths>` and artifact-specific `git rev-parse` | exit0; all approved behavioral/configuration artifacts preserved |
| `git diff --exit-code 60b1f8a HEAD` before this appended review | exit0; reviewer checkout tree matches exact completion candidate |
| `gh run view 41235930193 --repo Kernol12/subathon-bot-ai --json headSha,event,status,conclusion,jobs` | exit0; successful pull_request run at the exact reviewed source head, all steps successful |
| `gh run view 41235930193 --repo Kernol12/subathon-bot-ai --log` with focused `rg` filtering | exit0; GNU13.3.0,21 named passes,0 failures and actual primary-label summary inspected |
| `python3 -m unittest discover -s tests/tooling -v` | exit0;19/19 pass |
| `python3 tools/check_project.py` | exit0;377 tasks,122 decisions, links/presets/current HTML pass |
| `python3 tools/progress.py render --check` | exit0; current generated snapshot |
| `git diff --check` and `git diff --cached --check` | exit0 before final review commit |

No new findings. This final evidence pass did not repeat the unchanged six native
lanes or the coordinator's DOM simulation. It performed no live provider/credential/
supporter operation, deployment or external message. Final completion/review commits
must be published and their actual latest-head CI/discussions/required-review/ruleset
conditions rechecked by the coordinator before merge. Full release scope and all
remaining storage/API/connector/accounting/platform gates remain required.
