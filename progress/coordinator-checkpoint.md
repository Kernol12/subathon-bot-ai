# Coordinator checkpoint

Owner: Kernol12. Issues own brief/status/prerequisites; PR comments own reviews.
Complete specification v0.45, all seven integrations and all three FX providers
remain release scope. This checkpoint grants no new authorization.

## Current work

Use the [Subathon bot ai development board](https://github.com/users/Kernol12/projects/1)
for the queue, status and owner holds. [AGENTS.md](../AGENTS.md) defines daytime
selection, night prep and slot limits. Issue comments retain task-specific
decisions and review chronology. The handoff below records historical pins.

## Standing authorization — continuous coordination (2026-11-18)

The owner replaced the former “merge, then STOP” instruction. Continue through
both stages, reviews, observed intended red, implementation, checks, proof and
protected merge without routine permission pauses. After every protected merge,
close the issue, update its board status, and select the next task under
[AGENTS.md](../AGENTS.md).
Apply the workflow tier; report each merge briefly and continue without awaiting
a reply. The original [#278 handover](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6930279142)
remains historical technical context, not a continuing stop directive.

Since 2026-11-23, questions go to the advisor under
[docs/ADVISOR.md](../docs/ADVISOR.md), which lists what still reaches the owner.
An explicit owner stop always applies. Ask with options and a recommendation,
while continuing independent work not blocked. Pre-build/test CI infrastructure
allows one recorded rerun; routine in-scope choices, self-caught mistakes and
filing planned issues are not stops.

**Standing test-binary admission.** Apply the procedure to every authorized Qt
or GoogleTest binary using its own closure. Record passive ELF/ABI and dependency
hashes for the exact binary. Run the first real test with bounded loader and
plugin logs, reconcile all loaded objects and their hashes before crediting the
result, then check hashes again. Any mismatch voids the run. Changes to the procedure, tooling, modes or
native-supervisor debug variables require the owner under rule 4(e).

Before the first review packet for either stage, admit each candidate binary
and run its suite once. In the tests stage, fix setup failures before review.
In implementation, run the frozen tests and fix any failures before review.
These runs earn no test evidence. Admit each reviewed binary again before the
credited run. Stages that already sent their first review packet keep their
existing gates.

Apply log size caps only to captured log files, including admission logs.
Test processes and their children run without a process file-size limit.
The owner’s BUILD-002 diagnostic reproduced SIGXFSZ at a 1,048,576-byte
SQLite WAL under the inherited cap. The original
`example8_twoEvents` observation remains in the ledger as a harness fault.

Record `readelf` checks of both `.dynsym` and `.symtab` for Drogon
`native_profile` Session symbols. The consumer Session RTTI export check applies
if either table contains a defined or imported Session symbol. Retain all other
checks, including imports, versions, SONAMEs, selected SDK identity and exports,
the SDK's own Session RTTI and marker, hashes and loader reconciliation. External
compiler and system-header pre-build hashes are outside this procedure.

### Structure review of `199dc02` (owner decision 2026-11-18)

- [API-008 #292](https://github.com/Kernol12/subathon-bot-ai/issues/292) remains
  planned. Its brief now calls for one contribution operation shared by the dev
  CLI and HTTP host, by default in `subathon_reference_intake`. Keep the existing
  receipt-to-snapshot transaction boundaries, return a typed clock-reservation
  refusal, and test both callers. The CLI builds a synthetic event, calls the
  operation and prints its outcome.
- Move encode/decode code out of `application_service.cpp` or
  `sqlite_application_bundle_access.hpp` only within an authorized task that
  already changes the respective file. Use a separate commit and name it in
  that task's review packet.

## Historical continuation after ACCT-007 (2026-11-21)

- #545 completed in [PR #768](https://github.com/Kernol12/subathon-bot-ai/pull/768).
  #201 completed in [PR #773](https://github.com/Kernol12/subathon-bot-ai/pull/773)
  at f63a418b. Their evidence worktrees and build folders are retained.
- #760 keeps migration v20 under 117A. The separately reviewed [131A correction](https://github.com/Kernol12/subathon-bot-ai/issues/760#issuecomment-7066483187)
  changes only four frozen expected enums. [Stage 2 credited storage and #201 regressions](https://github.com/Kernol12/subathon-bot-ai/issues/760#issuecomment-7067157033)
  passed on exact 7dc09e93. Use the issue and Stage 2 PR for integration and merge
  receipts. #288 follows protected #760. Migrations proceed #760 → #761 → #318.
- #588 took #201's slot. The [verified Caddy readback](https://github.com/Kernol12/subathon-bot-ai/issues/588#issuecomment-7066452752)
  identifies the pinned artifact. Exact isolated composition and Stage 1 follow
  the night order. Preserve any malformed-input stop without retry.
- #761's [advance closing counts at the cutoff](https://github.com/Kernol12/subathon-bot-ai/issues/761#issuecomment-7067055232).
  After #760 merges, re-pin and freeze if only pins and predecessor evidence
  change. Ask before any other change. Catalog assignment is at implementation entry.
- [#398](https://github.com/Kernol12/subathon-bot-ai/issues/398#issuecomment-7066591056),
  [#400](https://github.com/Kernol12/subathon-bot-ai/issues/400#issuecomment-7066934561),
  [#543](https://github.com/Kernol12/subathon-bot-ai/issues/543#issuecomment-7066691816)
  and [#210](https://github.com/Kernol12/subathon-bot-ai/issues/210#issuecomment-7066812204)
  have frozen contracts. #398 has the lowest contract preparation priority.
  #223 follows 133A with #777 gating live names. #212 can draft against frozen
  #210 and re-pins after its merge. #320 follows 127B, remains last, and needs
  both the 23:15 freeze and an exclusive storage slot. #162 is contract-only;
  #221 stays Planned outside the night list. #774–#777 are Planned and unqueued.

## Handover (2026-11-21)

This coordinator stopped at the end of the current review steps. No new task,
stage, fix round or delegate was started for the handover. The successor
uses the board and current rules above.

| Issue | Stage and exact commit or producer pin | Branch | Worktree | Next step |
| --- | --- | --- | --- | --- |
| [#760 STORAGE-026](https://github.com/Kernol12/subathon-bot-ai/issues/760) | Stage 1 round-two verification stopped at `b4e8080a13d643115d4d6c001e97b4a8a2718006` | `author/storage-026-stage1-760-fix2` | `/home/archnoob/.t3/worktrees/subathon-bot-ai/t3-fff400f8` | Apply [115A](https://github.com/Kernol12/subathon-bot-ai/issues/760#issuecomment-7062716319) in the next free slot, against the [two remaining findings](https://github.com/Kernol12/subathon-bot-ai/issues/760#issuecomment-7062685185). Verify the delta, then use the separate closing and binary admission gates. |
| [#545 RELEASE-002](https://github.com/Kernol12/subathon-bot-ai/issues/545) | Stage 2 verification stopped at `e977e06bf97e3f4594d68191808931ca40f6288f` | `author/release-002-stage2-diagnostics` | `/home/archnoob/.t3/worktrees/subathon-bot-ai/t3-c49bd267` | Read the [generated-header finding and stop receipt](https://github.com/Kernol12/subathon-bot-ai/issues/545#issuecomment-7063223279). Runtime admission, acceptance, closing review and merge remain pending. |
| [#201 ACCT-007](https://github.com/Kernol12/subathon-bot-ai/issues/201) | Stage 1 initial review stopped at `cc9dd9704d396774146672fcb5caf7b8354efa18` | `author/acct-007-stage1-a696a2e6` | `/home/archnoob/.t3/worktrees/subathon-bot-ai/t3-f789ca27` | Fix round 1 awaits resumption. Start with the [three blockers](https://github.com/Kernol12/subathon-bot-ai/issues/201#issuecomment-7063223070), then relink, reseal and verify. |
| [#588 SESSION-010](https://github.com/Kernol12/subathon-bot-ai/issues/588) | Ready after the prerequisite merge. Frozen contract pin `814e9ca76514fab0dd6b59eb3aeab4fb796c4921` | None | None | Resume in a normal slot after #760 and before new tasks. Use the [conditional freeze](https://github.com/Kernol12/subathon-bot-ai/issues/588#issuecomment-7050096465) and [resume receipt](https://github.com/Kernol12/subathon-bot-ai/issues/588#issuecomment-7062933062). |
| [#288 API-006](https://github.com/Kernol12/subathon-bot-ai/issues/288) | [Contract r3 frozen](https://github.com/Kernol12/subathon-bot-ai/issues/288#issuecomment-7060454151), pin `c88adbcd3650b1136c4a363b1e30a73c025337b7` | None | None | Stage 1 follows the merge of #760. |
| [#761 STORAGE-027](https://github.com/Kernol12/subathon-bot-ai/issues/761) | Planned Tier A prerequisite for #238. No candidate commit. | None | None | Open after #760 merges. Assign the catalog number at implementation entry. |
| [#238 MILESTONE-006](https://github.com/Kernol12/subathon-bot-ai/issues/238) | [Contract r2 frozen](https://github.com/Kernol12/subathon-bot-ai/issues/238#issuecomment-7060752381), pin `c88adbcd3650b1136c4a363b1e30a73c025337b7` | None | None | Stage 1 follows the merge of #761. |
| [#318 STORAGE-003](https://github.com/Kernol12/subathon-bot-ai/issues/318) | [Contract r3 frozen](https://github.com/Kernol12/subathon-bot-ai/issues/318#issuecomment-7062576817), pin `a696a2e6a7494a4f7a959672164438bbeb380b78` | None | None | Carry forward the gates in the freeze and [114A](https://github.com/Kernol12/subathon-bot-ai/issues/318#issuecomment-7062461621). Its migration opens after #761 merges. |
| [#272 SESSION-007](https://github.com/Kernol12/subathon-bot-ai/issues/272) | Planned, conditional 237-line draft pinned to `c88adbcd3650b1136c4a363b1e30a73c025337b7` | None | None | Resume only under [113B](https://github.com/Kernol12/subathon-bot-ai/issues/272#issuecomment-7061402909) and the [#509 deployment gate](https://github.com/Kernol12/subathon-bot-ai/issues/509#issuecomment-7061016580). |

**Provider safeguards.** The standing provider rules below apply. #752's
one-case owner track is complete. Claude sessions covered the test stage.
Codex covered the authorized implementation source reviews only, in the
temporary third slot. The owner published and merged [#763](https://github.com/Kernol12/subathon-bot-ai/pull/763);
that slot is closed. For #588, if the author is stopped on malformed-input tests
again, park and report the missing cases without retrying. The
[#588 resume receipt](https://github.com/Kernol12/subathon-bot-ai/issues/588#issuecomment-7062933062)
records that limit.

**Local evidence.** The three kept worktrees own their current candidates and
review artifacts. The ignored `build/post-night-2026-11-21/` tree retains the
open contract packets. The #272 draft is
`build/post-night-2026-11-21/SESSION-007/issue-draft-r2.md` with SHA-256
`40000547d18cd1cf78cefee1e0eb314e1eb8b690238d5a2f85d3e956d4864c8b`.
The repository bundle, dirty-worktree patches and copied handover evidence are
under `/home/archnoob/coordinator-backups/subathon-bot-ai-2026-11-21-84f0e4z2/`.
Shared SDKs and the proof environment are under `build/deps/` and
`build/workflow005-schema-venv/`.

The owner stopped #715 after its [one hosted experiment](https://github.com/Kernol12/subathon-bot-ai/issues/715#issuecomment-7004781214).
#715 remains Planned, uses no parking slot, and its retry change was never enabled.

### Continuation after the RELEASE-002 merge

- [#545](https://github.com/Kernol12/subathon-bot-ai/issues/545) completed in
  [PR #768](https://github.com/Kernol12/subathon-bot-ai/pull/768), protected merge
  `2629e9dedc45fa04fa60a3ce05513f70387f6f10`. The seven required checks passed,
  and the admitted diagnostics fixture passed the producer control and 26 methods.
  The issue is closed and the build evidence is retained.
- [#760](https://github.com/Kernol12/subathon-bot-ai/issues/760) resumed under
  [bounded 116A](https://github.com/Kernol12/subathon-bot-ai/issues/760#issuecomment-7064212263).
  Diagnose R1 and R3 on the admitted `288057faa72af99e26ff810d6cd2e39fa995434e`
  runner and record the exact causes before starting the split. Preserve the
  [incomplete run](https://github.com/Kernol12/subathon-bot-ai/issues/760#issuecomment-7064099749).
- [#201](https://github.com/Kernol12/subathon-bot-ai/issues/201) Stage 1 round one
  [verification approved](https://github.com/Kernol12/subathon-bot-ai/issues/201#issuecomment-7064314623)
  `ef5a2db3595a2baf439e8e04fe682fe7123eb747`. Fresh closing and runtime admission
  remain pending. The worktree and branch in the handover still apply.

## Parallel work and standing CI rerun rule

- The board carries work status and the [root instructions](../AGENTS.md) set
  slot limits. Keep concurrent tasks out of each other's owned files. The
  coordinator owns shared CMake, CI, acceptance map and checkpoint.
- Serialize local test execution and exact-binary admission. Builds and
  reviews may overlap. At a provider limit, pause and retry the same provider
  on the next active turn.
- Main CI stops new work on a build or test failure. A failure in
  **Prepare authenticated Ubuntu package acquisition** before build or test
  does not stop new work. The authorized [main rerun](https://github.com/Kernol12/subathon-bot-ai/pull/714#issuecomment-6993941079)
  passed after the original APT timeout on `c8674545`.
- Until the owner reopens [#715](https://github.com/Kernol12/subathon-bot-ai/issues/715),
  a failure in that exact setup step may be rerun up to twice on fresh
  runners (three attempts total). Record each rerun on the PR. Park after
  the third setup failure.

Keep credentials and supporter data out of source, fixtures, reports and HTML.
Use synthetic or redacted fixtures. Live chat and payment side effects need
separate owner authorization.

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

**Fix rounds (owner clarification, 2026-11-18).** Each separately reviewed revision,
including a coordinator integration revision, has its own two fix rounds. After a
fix round, verify the changed commit and the dispositions of the findings. Bring
new defects found after a fix round on that same revision to the owner before
folding them into the round. Findings from the first review of a new revision use
the new revision's fix rounds. Ask the owner before a third round. The other stop
rules continue to apply.

**C++ compile gate (owner decision, 2026-11-18).** Compile each C++ source or
test revision before sending it for review. This gate is build-only, with no
test or product execution. Report compile failures to the author as ordinary
findings. They earn no intended-red credit and do not consume a fix round.

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

**Codex model — owner decision (2026-11-18).** The coordinator and every new
Codex child use `gpt-6.2-sol`, including authors, investigators and verifiers.
The provider stays Codex, so this is within the standing authorization. Existing
running sessions, completed reviews and receipts retain their original models.

## Working rules from the owner (2026-11-15)

- `/tmp` is RAM-backed on this machine. Sessions put build directories on disk, in an
  ignored directory of their worktree (`build/TASK-ID/<session>/`) with `TMPDIR` set
  there, and follow the "Temporary files" section of the workflow. Name that
  directory in every assignment and review packet.
- Unreviewed test source stays local.
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
  That gate allowed existing Git/CMake construction. Other scripts, routes or
  tooling required a separate owner decision before authoring.
- The control-only launcher is ignored evidence, never committed and unused by
  CI. Retain its hash-bound bytes outside #690 to preserve the completed sample's
  evidence. The advisor may approve a run script for one task's run with
  independent review before execution. It must stay in that task's ignored
  build folder, remain uncommitted and unused by CI, derive from a reviewed
  script or be small glue, and preserve every check, isolation boundary and
  assertion. Name it in the report for the advisor's digest. Escalate reusable
  tools, anything under `tools/`, committed scripts or tooling, and CI changes
  to the owner before authoring. See [the advisor rule](../docs/ADVISOR.md).
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

Coordinator owns any future shared CI/proof/checkpoint integration only within
new owner authorization. Build/scratch remain on disk under owned ignored task
roots; verify actual checkout/PR/session before any future authorized task.

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

Do not reverify pre-MIGRATE-004 records.
