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
   Scope items 1 to 4 (baseline revision, additions, listeners, header and stubs)
   are authorized under the handover gates. Source reviews and intended NI are
   complete. PR #667 owns the final proof, latest-head CI and protected merge.
   Current main, including the bounded tooling exception, was merged first.
   The owner-approved full selections reached intended NI only: 87 Qt selectors
   and all 30 native methods. Final proof encoding is unblocked as recorded below.
   **APITEST-017 [#672](https://github.com/Kernol12/subathon-bot-ai/issues/672)** is also authorized, tier A, in parallel in its own worktree and branch.
   The owner narrowed it to native peers (former scope 2): suspend/admit and more
   than eight peers, using existing observations. Tests must fail on `0e774a51`
   (the suspend/admit case also on `8fbc0ee8`) and pass on the merged foundation.
   Former HTTP scope 1 and its observation seam are deferred to #689, not an
   API-001 prerequisite and not authorized.
   The tasks own separate files. If both need the same file, stop and ask the owner.
3. **API-001 [#278](https://github.com/Kernol12/subathon-bot-ai/issues/278)**: the production host, tier A. Prerequisites: APITEST-015 and
   APITEST-017 (#672). The path remains both tests tasks, then API-001.

Report each stage when it finishes or needs the owner. Filing the API-016
follow-ups as planned epic-linked board issues is authorized; doing their work is not.

## Current owner holds (2026-11-17)

- **APITEST-015 full-selection NI stage is complete.** All 87 Qt selectors and 30
  native methods ran once and reached only intended NI; no other/unclassified
  failure. Native: 68 actual children/44 subtest rows, 67 `drogon_factory` and one
  `elapsed_factory`. Exit 42 cleanup assertions were secondary to those same NI
  exits. Exact first-native admission was reused under explicit owner authority;
  all 1,740 source/dependency/artifact seals matched. Native initial 6/runtime 45 map
  comparisons were NI-preempted; 62 initial/nine runtime comparisons completed.
  Serving/stopped were never observed. Qt's disclosed pre-execution cold-arm NI
  boundary prediction error remains recorded, without source/oracle change/rerun.
  [Full native receipt](https://github.com/Kernol12/subathon-bot-ai/pull/667#issuecomment-6949622237).
- **APITEST-015 proof-provider gate is resolved.** Owner-authorized bounded
  WORKFLOW-008 (#698), PR #699, merged as `b8e30e86`: a supported provider that
  differs from the phase default needs an `owner_authorization` HTTPS URL;
  without it the provider rejection stays. Exactly two focused tests, Claude
  tests/code approvals, observed assertion red/green, 164 tooling tests, three
  real-SDK schema checks and seven final-head CI checks passed. #698 is Done;
  #619/#620 remain frozen. No task/receipt allowlist or other validator change.
  The actual initial Codex receipt on #667 retains its provider/model and links
  the original temporary-rule owner decision. Earlier Claude contract review is
  not substituted for the tests review; completed reviews are not redone.
  The final proof is prepared once; #667 owns latest-head CI and protected merge.
  Source bindings remain exact to approved `9d9587e9` and `b8df1ba1`. No host tests
  are rerun. Finished review/investigation worktree cleanup is after merge only,
  preserving pending artifacts and active worktrees.
- **APITEST-017 is stopped after ten NEW historical trials.** Source `0059feaf`
  adds only peer/checks-passed diagnostics to approved option A; Claude approved
  the diff ([receipt](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6951191480)). Single fix round 1/1 is used; any further source change needs owner.
  Fresh historical `0e774a51` plus four exact overlays built and passed passive
  and runtime admission. Ten trials: ten valid same-batch witnesses, ten real
  LegacyControl passes, zero setup misses, zero N5 passes. All ten PRODUCT
  message failures were new peer 16 / checks_passed 0. Failure counts on each
  peer 0–15 are zero because their checks were NOTREACHED, not successful.
  Latest owner definition is PRODUCT failure after valid same-batch witness on
  established peer 0–15; no extra branch trace is required. There were zero such
  failures, so **the historical gate is NOT satisfied**. Stopped after ten, no
  extension, replacement, retry or later stage
  ([full counts](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6951857099)). All seals matched, jobs reaped. The original fa9 trial 01 stays
  separate, unattributed, no credit/no retry. Metadata-path/source-provisioning
  errors and their coordinator resolutions remain preserved, not product reds.
  Same-batch proof uses existing `boundedEpoch()` and active callbacks on IO0;
  no SDK/producer change. Equal epochs or a foundation pass cannot prove the
  faulty suppression branch occurred. Current-code loaded/unloaded, `8fbc0ee8`
  suspend/admit, frozen34/native44, final-source CI/proof/merge remain open.
  Remote PR #690 stays `6d558798`; local `0059feaf` is unpublished. No 60-sample
  gate or further runtime/editing until owner disposition.

Board: #613 In progress for proof/CI/merge; #698 Done; #672 Blocked on owner
historical-gate disposition. API-001 remains behind both open prerequisites.

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
- **APITEST-017 numbered correction 1**, owner authorized: N5 may discard only a
  received tuple that misses exact `(10,1,1024,True)`, print it as "scenario not
  established", pass normal stop/reap/empty-stderr/maps/cleanup, then use a fresh
  child. At most **three total attempts**; product assertions are never retried.
  All three misses are a FIXTURE failure; diagnostic/event/admission/cleanup or
  assertion failures propagate. No seam, scheduling, predicate or producer change.
  Fresh Codex verification and Claude closing approved `d9875341` ([closing](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6943014022)); fresh admission
  and restarted loaded sampling passed. This does not waive the historical gate.
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
- Latest owner APITEST-017 revision2 replaces N5's old9+1 pending101 case with
  the16+1 same-batch target above. New observation/seam needs owner before edits;
  candidate code and all old results remain preserved. Revised gate is10oldruns,
  not60. Product assertions are never retried; setup max3 remains unchanged.
- For APITEST-015, first-real-run retrospective admission was an explicit owner
  exception to prior pre-run receipt/maps sequencing. The owner later authorized
  full Qt selection with logs and full native selection with reused exact first
  admission, unchanged supervisor, no debug variables and before/after seals.
  Any object/hash mismatch would void that run as admission failure, never NI red.
  No staged external capture or identity-only mode is authorized by that exception.
- Latest APITEST-017 owner decision used option A's sole fix round only for
  diagnostic peer/count context, with Claude diff review. Original fa9 trial 01
  remains unattributed, no credit/no retry. Ten NEW historical runs completed:
  attribution requires a PRODUCT failure after valid same-batch witness on an
  established peer 0–15. All failures were peer 16, so the owner-required stop
  applies after ten; no extension/retry/current-code progression is authorized.
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

- APITEST-015 (#613), tier A: retained Codex author worktree `t3-0dc4e70e`,
  branch `task/APITEST-015-host-baseline-revision`, PR #667. NI observed on
  `b5bc9340`; current main (including #699) merged before proof. Contract/header last `9d9587e9`
  and full tests/integration `b8df1ba1` have approved source review chains
  ([closing](https://github.com/Kernol12/subathon-bot-ai/pull/667#issuecomment-6941482947)). All six final corrections and corrections 7–11 remain unchanged;
  correction 11 is THIRD/LAST. All 87 Qt selectors and 30 native methods reached
  intended NI as above. No host runtime implementation. Final proof encoding
  is unblocked by merged #699; #667 owns final proof/CI/protected merge.
  The owner decided all six numbered
  C2 corrections and then revised 1 and 6 ([latest record](https://github.com/Kernol12/subathon-bot-ai/issues/613#issuecomment-6932687162)); PR #667 and every tier A packet must name all six with these revisions.
  Execute may prequeue units, preserving the three-accepted-unit fail-close oracle.
  Forward-from-body still ends without reply; forwarding is only from execute or a
  supplied unit completion. Payload is a borrowed, charged host-owned input view.
  Unit captures follow existing intake resource rules unchanged; the compile-time
  capture limit is dropped. No new producer prerequisite task.
- APITEST-017 (#672), tier A: retained Codex author worktree `t3-aa9893a6`,
  branch `task/APITEST-017-transport-regressions`; reviewed local `0059feaf`,
  remote draft PR #690 still `6d558798`. Remote native failed the stale
  registration guard; other six passed, no rerun. Option A source is approved
  ([option A closing](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6948896774), [diagnostic review](https://github.com/Kernol12/subathon-bot-ai/pull/690#issuecomment-6951191480)). Historical worktree `t3-415a156d` has exact `0e774a51` plus four
  reviewed `0059feaf` overlays. Fresh admission and ten NEW trials completed;
  peer16-only stop as above. Old `t3-e94838fb`/fa9 evidence remains separate.
  Older N5 misses
  and old 9+1 historical passes remain separate evidence. HTTP scope/seam stays
  deferred to #689. The logging amendment stays permanently; unexplained legacy
  exit 90 remains planned #692, not a blocker or authorized work.
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
