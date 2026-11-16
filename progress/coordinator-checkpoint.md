# Coordinator checkpoint

Owner: Kernol12. Issues own brief/status/prerequisites; PR comments own reviews.
Complete specification v0.45, all seven integrations and all three FX providers
remain release scope. This checkpoint grants no new authorization.

## Handover (2026-11-16)

API-016 is merged (PR #664, `4184c9fc`). The coordinator thread that ran it stopped
there by owner instruction. The handover comment lists open owner decisions, PR
states, the next step with its gates, deferred items, planned issues and leftovers:
[#278 handover](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6930279142). A new coordinator starts nothing until the owner gives
the go-ahead named there.

## Authorized path

Authorized on 2026-11-16, in this order and nothing else:

1. **API-016 [#616](https://github.com/Kernol12/subathon-bot-ai/issues/616)**: bounded HTTP transport foundation. **Done** (PR #664,
   `4184c9fc`, proof `progress/proofs/API-016.json`).
2. **APITEST-015 [#613](https://github.com/Kernol12/subathon-bot-ai/issues/613)**: revised host baseline (tests stage of API-001), tier A.
   Step 1 (contract revision for groups A to E) is reviewed: PR #667 at `bc9b33c4`,
   draft, not merged. The next step (scope items 1 to 4: test-suite baseline revision,
   additions, listeners, header and stubs) waits for the owner's go-ahead.
3. **API-001 [#278](https://github.com/Kernol12/subathon-bot-ai/issues/278)**: the production host, tier A. Prerequisites: APITEST-015 and
   APITEST-017 (#672, not authorized).

SLICE-001 (PR660, 579afcb3) and the owner checkpoint on #278 are done. PR #121 is
closed; API-001 restarts from its rewritten issue on current `main` in a fresh
session, and none of PR #121's reviews, approvals or CI results carries over.

## Planned, not authorized

- APITEST-017 [#672](https://github.com/Kernol12/subathon-bot-ai/issues/672): reviewed tests for the defects of API-016 that no frozen test
  covers (large input with more than 8 connections; established native peers after
  suspend/admit and beyond 8 peers). Prerequisite of API-001.
- SESSION-013 [#662](https://github.com/Kernol12/subathon-bot-ai/issues/662), SESSION-009 [#582](https://github.com/Kernol12/subathon-bot-ai/issues/582), API-010 [#560](https://github.com/Kernol12/subathon-bot-ai/issues/560).
- API-016 follow-ups (not filed): V2, C2, C4, C6, I14, I15, I16, I19, I20, O5; see
  the handover comment.
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
  #560 and the one boundary-contract sentence that quotes it are updated at the next
  step of APITEST-015; until then both carry the narrower "rolled-back unit" wording.
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

**Roles and providers — temporary owner rule (2026-11-16, until revoked).**
Codex for authoring, tier A initial reviews, tier A verification, tier B reviews,
and investigations or measurements. Every review or investigation is a fresh
session that did not author or fix the work under review.
Claude only for the tier A closing review, and for anything the owner asks for by
name. If Claude is unavailable for a closing review, leave that gate pending and
continue other authorized work; do not substitute Codex for it.
If a Codex review or investigation is flagged by the provider, stop and bring it to
the owner; do not move it to Claude independently.
Record the actual provider and model in every review comment. An author never
approves its own work. This temporary rule replaces the default provider split;
all other standing owner rules remain unchanged.

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

None. API-016 is merged. APITEST-015 (#613) is claimed for step 1 only; its branch
`task/APITEST-015-host-baseline-revision` (PR #667) has no running session. Verify
the actual checkout, worktrees and PR state before acting; never infer a live session
from this file.

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
