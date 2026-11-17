# Coordinator checkpoint

Owner: Kernol12. Issues own brief/status/prerequisites; PR comments own reviews.
Complete specification v0.45, all seven integrations and all three FX providers
remain release scope. This checkpoint grants no new authorization.

## Handover (2026-11-16)

API-016 is merged (PR #664, `4184c9fc`). The coordinator thread that ran it stopped
there by owner instruction. The handover comment lists open owner decisions, PR
states, the next step with its gates, deferred items, planned issues and leftovers:
[#278 handover](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6930279142). A new coordinator starts nothing until the owner gives
the go-ahead named there. The owner gave those decisions on 2026-11-17, as recorded
below; the handover's technical gates remain binding.

## Authorized path

Authorized on 2026-11-16, amended by the owner on 2026-11-17:

1. **API-016 [#616](https://github.com/Kernol12/subathon-bot-ai/issues/616)**: bounded HTTP transport foundation. **Done** (PR #664,
   `4184c9fc`, proof `progress/proofs/API-016.json`).
2. **APITEST-015 [#613](https://github.com/Kernol12/subathon-bot-ai/issues/613)**: revised host baseline (tests stage of API-001), tier A.
   **Done**: PR #667 merged as `60d7730d`; final proof `c8762280` and all seven
   final-head checks passed. All 87 Qt selectors and 30 native methods reached
   intended NI only, under the owner-approved admission exceptions. No host
   implementation or serving behavior was delivered.
   **APITEST-017 [#672](https://github.com/Kernol12/subathon-bot-ai/issues/672)**: authorized native-only scope, tier A, separate worktree/branch.
   Acceptance and final source reviews are complete on `6bd2f27b`; source-head
   CI has all seven checks passing. PR #690 contains its single final proof/policy
   commit and this batched checkpoint. Latest proof-head CI and protected merge
   are the remaining gates at this pre-merge checkpoint.
   Historical `0e774a51` and suspend/admit `8fbc0ee8` gates are satisfied; all five
   current subcases passed 20 unloaded and 20 loaded repetitions. Frozen34 and
   native44 each passed 20 complete repetitions. Former HTTP scope/seam stays
   planned, unauthorized #689 and is not an API-001 prerequisite.
   Shared-file exceptions remain limited to serial coordinator CI ownership.
3. **API-001 [#278](https://github.com/Kernol12/subathon-bot-ai/issues/278)**: next authorized issue after both tests tasks' protected merges, tier A.
   Read its rewritten issue on then-current `main` and use a fresh implementation
   session. No API-001 product work started during these baseline stages; no
   authority or evidence carries over from closed PR #121.

Report each stage when it finishes or needs the owner. Filing the API-016
follow-ups as planned epic-linked board issues is authorized; doing their work is not.

## Current gates (2026-11-17, pre-merge snapshot)

- **APITEST-015 is merged.** PR #667 / `60d7730d`, proof `c8762280`; 87 Qt
  selectors and 30 native methods all intended NI. Native 68 children / 44 subtest
  rows; no unclassified failure. NI-preempted maps are not passes or serving proof.
  [Native receipt](https://github.com/Kernol12/subathon-bot-ai/pull/667#issuecomment-6949622237).
  Sixteen finished clean review/investigation worktrees were removed after merge,
  with private notes archived and hash-checked. Retain source refs and pending
  SDK/runtime artifacts; never force-delete an active or evidence-bearing tree.
- **Provider-proof gate resolved:** bounded WORKFLOW-008 (#698), PR #699, merged
  as `b8e30e86`. A supported provider differing from the phase default needs an
  `owner_authorization` HTTPS URL; absent it, the rejection stays. Actual Codex
  initial receipts on #667/#690 link the temporary-rule owner decision; no provider
  relabeling or completed-review redo. #619/#620 remain frozen. Default provider
  split and its 2026-11-17 revocation record below remain unchanged.
- **APITEST-017 historical gates complete.** The owner corrected attribution:
  #616 reproduction2 targets the **NEW upgraded peer16**, not established peers0–15.
  Ten NEW `0e774a51`+`0059feaf` trials therefore give 10/10 attributable PRODUCT
  failures, valid same-batch witnesses and Legacy passes, zero setup misses.
  Raw former exclusions are preserved; [correction](https://github.com/Kernol12/subathon-bot-ai/issues/672#issuecomment-6953588437)
  changes attribution, not data. Original fa9 trial01 stays unattributed/uncredited.
  Exact `8fbc0ee8` with its own SDK exposed both 1/9-peer post-admit PRODUCT
  failures in one CLI; real Legacy passed. Incompatible `0e` SDK was rejected.
  Existing exact8f Git/CMake preparation sufficed, with no tooling/source change.
  [Historical8f receipt](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6955030669).
- **APITEST-017 current and frozen sampling passed.** Runtime source `0059feaf`,
  current merge `06910d49`: each N1–N5 passed 20 unloaded and 20 loaded runs;
  each window 100 product subcase outcomes / 60 CLIs / 60 actual LegacyControls.
  N5 each window 20 attempts, zero misses/failures. Loaded sampling had no observed
  competing heavy work under the original bounded stress envelope. Main merge
  `07bad67e` preserved actual source/compiled inputs; fresh admissions and frozen34
  ×20 (680 method runs +20 Legacy) / native44 ×20 (880 runs) passed without
  failures/errors/skips. All postseals matched and owned jobs were reaped.
  [Unloaded](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6954291103),
  [loaded](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6954515937),
  [frozen](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6955755341).
- **Final source `6bd2f27b` is published and approved:**
  [Codex verification](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6955970368) /
  [fresh Claude closing](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6956066089).
  Only nine task-identity documentation lines followed `07bad`; old admission
  document hashes remain old, final CI freshly admitted the new source/doc seal.
  Run `44140403788` attempt2 passed all seven source-head checks, including ordinary
  foundation37/native44. Attempt1 native stopped on a 300s APT timeout before build
  or tests; [cause and one bounded rerun](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6956319913)
  are recorded, not hidden by green. Old `6d` run `43985035962` failed the stale
  9920/34 registration guard; reviewed counts fix it, no rerun. The single proof
  binds all five final paths at `6bd`; latest proof-head CI and protected merge
  remain mandatory. No unresolved owner contract choice is being bypassed.

Board: #613 and #698 Done; #672 In progress pending final proof/CI/merge.
API-001 remains next after #690's protected merge. Board dependency readiness
never expands the authorized path.

SLICE-001 (PR660, 579afcb3) and the owner checkpoint on #278 are done. PR #121 is
closed; API-001 restarts from its rewritten issue on current `main` in a fresh
session, and none of PR #121's reviews, approvals or CI results carries over.

## Planned, not authorized

- SESSION-013 [#662](https://github.com/Kernol12/subathon-bot-ai/issues/662), SESSION-009 [#582](https://github.com/Kernol12/subathon-bot-ai/issues/582), API-010 [#560](https://github.com/Kernol12/subathon-bot-ai/issues/560).
- APITEST-018 [#689](https://github.com/Kernol12/subathon-bot-ai/issues/689): large buffered HTTP input with more than eight connections and its
  bounded observation seam, moved from APITEST-017 by the owner. Planned under
  EPIC-008; not an API-001 prerequisite. Historical `f8be1aec` coverage is deferred here.
- APITEST-019 [#692](https://github.com/Kernol12/subathon-bot-ai/issues/692): unexplained unloaded legacy-control exit90/SDK diagnostic. Planned under
  EPIC-008, not authorized; not a prerequisite of APITEST-017 or API-001.
- API-016 follow-ups: V2, C2, C4, C6, I14, I15, I16, I19, I20, O5; see the handover
  comment. Filed as [#678–#687](https://github.com/Kernol12/subathon-bot-ai/issues/273)
  (API-017 through API-026), planned under EPIC-008; none is authorized for work.
- Process/tooling #619/#620 is frozen; lifting the freeze is the owner's decision.

Dependency readiness authorizes nothing beyond the path above. No live chat or
payment side effects are authorized. Keep credentials and supporter data out of
source, fixtures, reports and HTML; synthetic or redacted fixtures only.

## ADR-0012 decision for APITEST-015 (2026-11-16)

**Option 1.** The host baseline is revised so that the host runs the path that
already runs in `subathon-dev-server`. Groups A to E are approved exactly as
[itemized](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6907983078) ([decision](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6908067929)). Nothing beyond that list may be removed or replaced.
If a review changes an item of the list, bring only that item to the owner.

Owner amendments, all recorded on #278:

- [O1/O3](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6913620654): the epoch fence is named in two parts: the comparison in
  `resolve_current_account_session` (`server/src/session_resolution.cpp`) for units
  that authenticate, and `current()`/`accepted()` in
  `server/src/sqlite_account_sessions.cpp` for units that insert or replace a session.
  The host has no generation fence. `local-login-authority-v1.md`,
  `session-lifecycle-v1.md` and `guarded-enrollment-recovery-v1.md` get pointer-only
  changes.
- [Two changed list terms and the §5 pointer](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6914837587) (`known_rollback_preserves_source_and_generation`,
  `busy_begin_is_actual_failure`; pointer-only change in §5 of the listener contract).
- [Reply rule, final form](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6916926039): the host withholds a reply whenever a unit's outcome is
  unknown or something was committed without ending normally; otherwise the
  dispatcher may reply. A rollback the writer performs after an error counts as a
  known rollback. A unit body that throws before anything is committed may be
  answered. Reference units report refusals as rollbacks, and those must be answered.
- API-010 (#560), the first replying dispatcher, carries the criterion "a command with
  a unit that ended without a commit never yields a success reply" ([widened](https://github.com/Kernol12/subathon-bot-ai/issues/613#issuecomment-6919654379)).
  #560 and the matching reviewed, published APITEST-015 boundary sentence are updated.
- #278 acceptance criterion: SIGTERM leads to an orderly stop; the host installs its
  own handler, because Drogon's default one terminates the process.

Owner conditions met before any removal: C, #662 (SESSION-013) exists; E, both clock
guards are acceptance criteria of SESSION-009 (#582).

APITEST-015 step-1 instruction (still binding for the rest of the task): no authored
tests, stubs or header changes until API-016 is merged (now met). The gates for the
next step are on [#613](https://github.com/Kernol12/subathon-bot-ai/issues/613#issuecomment-6917314751) and in the handover comment.

## Standing owner rules for this path

**Contract corrections.** Every correction to a task's contract is a numbered item in
the PR description and is named explicitly in each tier A review packet. It goes to
the owner first if it adds more than one SDK function or touches a first-party header
([rule](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6909553282)). Where no contract sentence governs a behaviour that a fix must decide,
bring the owner a numbered correction before the fix. A recorded reason for a
correction states the design reason; do not claim "matches earlier behaviour" without
measuring it.

**Rulings cover one case.** A coordinator or owner ruling applies to the case at
hand; it grants no class of future corrections or permissions.

**Class audit** ([rule](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6912622673)). When a review finding is one case of a class of defect,
the author audits the whole change for that class in the same fix round and reports
the sites checked. A later finding of the same class is a non-blocking follow-up, not
a new round. Put this rule into every review packet and fix assignment.

**Failed CI runs and re-runs** ([rule](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6921048499)). An author's handback lists every failed
CI run on its branch with the cause. No author re-runs a failed required check
without the coordinator's say. The review packets name those runs. Never merge on a
green re-run of a failed check; find the cause first. A change to a frozen test is
the owner's decision. (Coordinator practice, not an owner rule: a setup failure whose
cause lies outside the change, such as an apt mirror timeout on the checkpoint PR
#674, was re-run by the coordinator after recording the cause on the PR.)

**Fix rounds.** At most two fix rounds per review; ask the owner before a third. After
a fix round the verification is a changed-part re-check: the changed commit, the
findings and their resolutions; no new whole verification per round. (Coordinator
commitment to the owner: a new defect found after a fix round goes to the owner; it
is not folded into the running round.)

**Stop rule of a last pass** (API-016 third pass, [decision](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6924951572)). When the owner
grants a third and last author pass: the author first lists what the fix must cover
and the correction is limited to that list; an independent investigator then repeats
the whole measurement; any further regression means no new author pass, and the
coordinator brings the owner the fallback options instead. The verification and
closing re-check cover everything since the last approved head and name every defect
the measurement found that no frozen test covers.

**Measurement before merge** (API-016 fix round 2, [decision](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6921048499)). A defect that appears only intermittently in CI is
investigated, not re-run away. After a transport fix the investigator measures the
fixed build with the same method and sample size as the investigation, and the
unmodified frozen test runs at least 20 times locally; a non-zero failure rate stops
the task and goes to the owner.

**Provider flags.** If a Codex author run is flagged for cybersecurity risk, save the
uncommitted work as a patch and ask the owner; do not retry or switch providers on
your own. If a Codex verification is flagged, stop and bring it to the owner.

**Roles and providers — default restored (2026-11-17).**
The owner revoked the temporary 2026-11-16 split. Codex authors; Claude provides
initial and closing reviews for tier A and reviews for tier B; Codex verifies.
Investigations and measurements stay with Codex. Completed or running reviews
under the temporary rule remain valid and are not redone. The next new review
uses the default. Every review/investigation is a fresh session that did not
author or fix the work under review. If the required reviewer is unavailable,
leave the gate pending and continue other authorized work; do not substitute.
A provider flag stops for owner. Record actual provider/model in every review
comment; an author never approves its work. Any other provider or model switch
needs the owner. Other standing rules remain unchanged.

## Project board maintenance (2026-11-16)

The "Subathon bot ai development" board follows the issues' native "blocked by"
links. The enabled "Item closed → Status Done" workflow sets Done on close for
issues and pull requests. The coordinator archives board items closed as not
planned without changing the issue, sets Status when claiming or blocking an issue,
and moves issues to Ready when a close unblocks them.

Open non-epic issues are Ready when every native blocker is closed, otherwise
Planned; claimed issues are In progress and issues awaiting the owner are Blocked.
The owner's API-016 follow-ups are explicitly Planned and not authorized for work.
Epics keep their status. Board Ready shows dependency order; it does not authorize
work. The checkpoint's authorized path does.

## Working rules from the owner (2026-11-15)

- `/tmp` is RAM-backed on this machine. Sessions put build directories on disk, in an
  ignored directory of their worktree (`build/TASK-ID/<session>/`) with `TMPDIR` set
  there, and follow the "Temporary files" section of the workflow. Name that
  directory in every assignment and review packet.
- Before the first tests review the author runs only the cases that call a
  not-implemented stub and reports the fixture check in the handoff; it is never red
  evidence. Unreviewed test source stays local.
- Tier A proof record: `progress/proofs/TASK-ID.json` (and its policy entry if the task
  is not enrolled yet), in one commit after the final closing approval, immediately
  before merge. `tools/check_publication.py` requires `origin/main` as an ancestor of
  the head, so merge `main` into the branch before the proof commit.

## Bounded owner authorizations (2026-11-17)

- The coordinator is the sole, serial writer of `.github/workflows/ci.yml` for
  APITEST-017 and APITEST-015. Keep separate commits on their task branches and
  independent review before publication. Reviewed foundation registration is
  37 methods / 297 maximum children / 10158s after N5 correction 1; host registration
  is 87 Qt selectors / 72 native children / 1788s. The workflow edits are separate
  commits on their task branches. Other shared-file conflicts still stop for owner.
- APITEST-015 correction 11 is the THIRD AND LAST contract pass: only the
  phase/thread of `command_admitted` as approved on #613. It is synchronous on Qt
  after retained admission, before entry checks/execution/writer scope, without a
  host lock; a bounded pause leaves IO ingress independent. Any further contract
  gap goes to the owner with fallback options, not a fourth pass. Fresh changed-part
  Codex verification and Claude closing remain required; NI is not runtime proof.
- The owner accepts the APITEST-017 unloaded sample (0/20) as bounded
  non-reproduction. The earlier exit90/SDK diagnostic stays unexplained: one
  failure in 21 unloaded runs across the original and amended consumers. Keep
  the reviewed logging amendment permanently in the frozen foundation test.
  Planned, unauthorized #692 owns that failure; it blocks neither #672 nor API-001.
- The diagnosis-only stage is complete: 20 loaded N5 attempts passed, with no
  acceptance credit; the separate fresh read-only answer was **no deterministic
  overlap using existing gates without timing/load** ([combined report](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6940815717)). The owner then
  resumed acceptance. The later `343414cb` loaded repetition 17 missed its witness
  with `(10,1,0,True)`; the original loaded repetition 12 tuple remains unknown.
  Both are recorded setup misses, not red; do not backfill the earlier tuple/cause.
- **APITEST-017 correction1:** at most three TOTAL fresh-child setup attempts;
  product bodies are never retried. Final optionA permits only a received,
  well-formed unequal-epoch witness to print "scenario not established", pass
  normal stop/reap0/empty-stderr/maps/seals/canaries, then try a fresh child.
  All three misses are a FIXTURE failure; every other failure propagates.
  This supersedes the old9+1 pending101 tuple criterion, whose evidence is retained.
  C017-V1 cleanup-loss finding was resolved and reviewed at `d9875341`
  ([closing](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6943014022)).
- Loaded sampling restarts from the beginning after an approved amendment and
  fresh admission. **No builds or other heavy work anywhere on the machine during
  loaded sampling**; earlier concurrent CPU2/3 build permission is superseded.
  Keep inner setup attempts/misses separate from established product outcomes.
  Any product assertion or exhausted-witness failure stops for owner; no outer retry.
- Historical gates are `0e774a51` and, for suspend/admit, `8fbc0ee8`, as #672 states.
  The owner's earlier `f8be1aec` wording was a copied mistake; it belongs to #689.
  Existing Git/CMake construction is allowed. Anything beyond it (script, route or
  new tooling) comes to owner with options **before authoring**.
- The control-only launcher is ignored evidence, never committed and unused by
  CI. Retain its hash-bound bytes outside #690 to preserve the completed sample's
  evidence. From now on, ANY new tooling, launcher or harness goes to the owner
  BEFORE authoring, even when it serves an approved step.
- The owner authorized the completed fresh N5 scenario/sampling investigation
  and unchanged 20-run historical diagnosis in parallel, with paired delivery.
  These diagnostic results do not resume acceptance or authorize the proposed
  sampled gate. The APITEST-015 existing-facility assessment was read-only; its
  result does not authorize a new capture procedure or identity-only mode.
- **APITEST-017 correction2 / optionA:** existing `boundedEpoch()` only, 16
  established native peers without binary prewarming, seventeenth Upgrade and
  ordinary accept in the same active batch. Active callback origins and same IO0
  supplement equal epochs; one frame each, new peer16 first. No SDK/producer
  change was needed. Foundation pass cannot prove the old faulty branch occurred.
  The historical gate is ten `0e` runs with at least one attributable PRODUCT
  failure, not the rejected 60-sample proposal. Setup max3/product-never-retry holds.
- For APITEST-015, first-real-run retrospective admission was an explicit owner
  exception to prior pre-run receipt/maps sequencing. The owner later authorized
  full Qt selection with logs and full native selection with reused exact first
  admission, unchanged supervisor, no debug variables and before/after seals.
  Any object/hash mismatch would void that run as admission failure, never NI red.
  No staged external capture or identity-only mode is authorized by that exception.
- **APITEST-017 correction3:** optionA's sole source fix1/1 adds only native
  message failure peer/checks-passed diagnostics. Claude diff review approved
  `0059feaf`; no assertion/setup/predicate/timeout/retry change. No source fix
  remains. Original fa9 trial01 stays unattributed/uncredited/no retry.
- **APITEST-017 correction4:** the owner corrected the old peers0–15 criterion:
  NEW peer16 is #616 reproduction2's target. The ten new historical peer16 PRODUCT
  failures satisfy the gate10/10; raw evidence and old reports stay unchanged.
  Continue remaining authorized gates. Any peer16 failure on the current foundation
  is a fixture defect: stop/report, no fix round. All current samples passed.
- Batch checkpoint updates into one documentation PR per stage, not per decision.

## What SLICE-001 delivered

`subathon-dev-server` (`server/dev/`, suite `dev-server` in the `qt` job) starts the
Qt loop, opens the single writer, advances the catalog to version 10, initializes one
timer and applies synthetic contributions through API-011's reference path. The
contract is on #642 (revisions 1 to 4). Owner decisions of 2026-11-15: development
session clock, development provisioning and `--controlled-clock` live only in that
executable; `encode_reference_execution_ref` (SRF1) is public in the server library.

It is a development walking skeleton. Limits: no login or session renewal (the
development session lasts 24 hours), no restart of a live clock anchor (an existing
database is read-only), no HTTP, no production host. SESSION-009, API-010, setup and
API-001 keep their ownership and release requirements.

## Live assignments

- APITEST-015 (#613), tier A, **Done**: PR #667 merged `60d7730d`, final proof
  `c8762280`. Retained source worktree `t3-0dc4e70e`, branch
  `task/APITEST-015-host-baseline-revision`, preserves source/evidence refs.
  Contract/header `9d9587e9` and tests/integration `b8df1ba1` source approvals
  remain. All six final corrections and corrections7–11 are unchanged;
  correction11 is THIRD/LAST. No runtime host implementation or serving proof.
  The owner decided all six numbered
  C2 corrections and then revised 1 and 6 ([latest record](https://github.com/Kernol12/subathon-bot-ai/issues/613#issuecomment-6932687162)); PR #667 and every tier A packet must name all six with these revisions.
  Execute may prequeue units, preserving the three-accepted-unit fail-close oracle.
  Forward-from-body still ends without reply; forwarding is only from execute or a
  supplied unit completion. Payload is a borrowed, charged host-owned input view.
  Unit captures follow existing intake resource rules unchanged; the compile-time
  capture limit is dropped. No new producer prerequisite task.
- APITEST-017 (#672), tier A: retained author `t3-aa9893a6`, branch
  `task/APITEST-017-transport-regressions`, PR #690, approved published source
  `6bd2f27b`. Final source CI passed; one proof/policy/checkpoint commit is prepared
  for required latest-head CI and protected merge. All acceptance gates above
  passed; no active build/stress/test worker remains. Preserve historical
  `t3-415a156d`/`0e` and `t3-24f198e1`/`8f` assemblies/SDK receipts, old fa9
  evidence, current/frozen artifacts and source refs until evidence handoff.
  Old setup misses and9+1 passes stay separate. HTTP stays #689, unknown legacy
  exit90 stays #692, both planned/not authorized/not API-001 prerequisites.
- Independent Codex default writer-budget measurement passed two current-v10
  inspections ([receipt](https://github.com/Kernol12/subathon-bot-ai/pull/667#issuecomment-6932486629)); it establishes no default-budget advancement, installation read or host activation.
- Coordinator owns shared build/CI integration and checkpoint/proof records. Any
  file needed by both tests tasks stops for the owner, except the expressly
  authorized serial coordinator edits of `ci.yml` above.

Builds and scratch use each worktree's ignored `build/TASK-ID/session/` on disk.
Verify actual checkout, worktrees, PR heads and sessions before resuming; these
assignments never substitute for that check.

## API-011 capability map (the reference path SLICE-001 hosts)

Read `server/include/subathon/server/reference_configuration.hpp`,
`reference_runtime.hpp`, `reference_intake.hpp`, issue #562 and PR653.
Entry points: complete immutable `ReferenceConfigurationArchive`; typed
`encode_reference_state`/`decode_reference_state`; real `SystemReferenceClockSampler`
and `OsReferenceEntropy`; five-port `ReferenceApplicationAdapters` with bounded
reserve/settle/reconcile journal; `ReferenceIntake::make_initial_work`,
`make_receipt_work`, `make_evidence_work`, `make_capture_work`, `make_snapshot_work`.

This is component composition, NOT a running host/login/provisioning/recovery or
public synthetic endpoint. Startup needs actual stored fresh owner/session/graph
and complete configuration; no owner boolean or anchor reconstructed from raw SAS1.
Only normal acknowledged startup publishes the live process anchor. Supported
runtime is Active/Main-positive, no wait/debt/balances/deadline, with pause/recovery
known false ONLY from the live journal. Unknown data never proves empty actions or
provider/payment/activity knowledge. API010/login, recovery/restart, full249 and
host/API001/SLICE001 keep their ownership and complete release requirements.

Four DISTINCT transactions: adopted receipt commit; adopted evidence commit;
caller-owned plan/SRX/index commit; actual application-service commit. No second
writer/timer write or commit after adoption. Snapshot authenticates first, requires
current ordinary timer read USE (no owner shortcut), then publishes coherent normal
committed state. Automation rechecks original-author USE+GRANT and execution USE;
historical disclosure checks EVERY actual hold. Unknown stops; real application
postcommit error needs matching independent committed reconciliation, not replay.

Preserve B04 dual SAS1 echoes: facade uses stored previous basis (or initial token),
old service binds CURRENT head; no SAS1/SAR1/SAA1 byte changes. N11 compares complete
duplicate output including optional/nullness. N12 bounds spent RRF1 even for direct
issue; full domain/purpose/suffix identity, no probability/eviction/reuse shortcut.
Every reserve draws one RCO1 including reuse; absent-basis examples use3draws and
after03, known-rollback retry advanced05/after06. No entropy retry/fallback.

Reviewed tests/fixtures/public semantics remain fixed. Existing Qt CI runs exact
32+51methods/59+78invocations and61+80passes with hooks,0skips/warnings, ordinary
acceptance (no WILL_FAIL/inversion). Existing selector name
`SUBATHON_BUILD_REFERENCE_INTAKE_BASELINE=ON` is retained with `reference-rule`
selection. Use docs/TEST_STRATEGY.md, CI and local CMakeUserPresets for SDK paths;
no personal paths are required here.

Local Qt6.12/GCC16.2/Boost1.92 differs from hosted pinned Boost1.83. ASan/UBSan runs
have leaks disabled; LSan fails under ptrace and external libraries are uninstrumented.
No leak proof, allocator-fault/performance theorem, one aggregate process heap
ceiling, live provider, host/HTTP, platform deployment or recovery verification.

## Settled policy and prerequisites

[Owner decisions249](https://github.com/Kernol12/subathon-bot-ai/issues/249#issuecomment-6879902355): restrictive edits need management USE+GRANT scope;
untouched out-of-scope grants may remain, changed/newly exposed rights and every
shared-role account are checked. Automation needs author USE+GRANT and separate
sender checks; recheck at execution and hold after revocation. Full249 remains open;
identity251, delivery208/232 and UI424/473 retain their scope. Prefix1–10 migration
bytes are frozen; no new migration is assigned.

Completed protected merges: ACCESS-009 (#645), PR648, 1fd11c3b;
ACCESS-010 (#646), PR650, 65936bcb; ACCESS-011 (#649), PR651, d7d64f25;
RULES-006 (#558), PR652, 03e52e70; API-011 (#562), PR653, f7921b95. Their
issues, PRs and proofs own the evidence. Do not reverify pre-MIGRATE-004 records.
