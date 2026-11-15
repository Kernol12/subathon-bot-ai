# Coordinator checkpoint

Owner: Kernol12. Issues own brief/status/prerequisites; PR comments own reviews.
Complete specification v0.45, all seven integrations and all three FX providers
remain release scope. This checkpoint grants no new authorization.

## Owner boundary: finish ONLY API-011, then STOP

Finish API-011#562/PR653 through proof/publication validation, final-head required
CI, protected merge, merge read-back, close562 and safe cleanup. Then STOP. Do not
claim/start SLICE-001 or any other task even when dependency-ready. Before handback
verify no owned delegate/pending child work remains and clear all PR watches.
Final owner message: merge SHA, next candidate, retained/uncarried work.

Next candidate is **SLICE-001 #642, tier B**, requiring explicit owner resume or
new assignment. Historical remaining path642→API-016#616→APITEST-015#613→API-001#278
is suspended. Process/tooling619/620 stays frozen until a server starts, runs a
timer and accepts a fake contribution. Draft121 remains parked. PR656/657 were
separately owned and merged; their workflow integration authorizes no new task.

## Current task and merge gate

- [API-011 #562](https://github.com/Kernol12/subathon-bot-ai/issues/562) owns acceptance and prerequisites.
- [PR653](https://github.com/Kernol12/subathon-bot-ai/pull/653) owns live head/CI/merge. Branch `task/API-011-reference-intake`.
- Reviewed source **2b609a17d0ab83041ac8097ca2889ec65a5e24d9**, main/base2271a4ba,
  original base03e52e70. Recheck actual checkout/remote/main/rules; no stale CI.
- Both stages are tier A and reviews are now approved. The ONE final proof/policy
  commit includes this checkpoint, `progress/proofs/API-011.json` and policy entry;
  it changes no reviewed source/test/build/contract. Required final-head CI and
  protected merge/read-back/close562 remain external gates until actually observed.
- If PR653 is unmerged, finish ONLY these gates. If merged and562 closed, this
  assignment is over; do not resume the standing loop. No merge SHA can be written
  into this pre-merge checkpoint; the PR merge record/final report supplies it.
- No owner/product-policy decision remains open for API-011. No seventh source
  round or baseline change is implied. A new blocker goes to owner resolution,
  subject only to the explicit scoped exception below; never bypass protection.

## Evidence and decisions a fresh coordinator needs

- [Durable evidence index](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6895564949)
  preserves all8 tests receipts/hashes, complete137-case NI classifications,
  original135NI+2SETUP failures, source probes and limitations. Tests closed at
  d7155b42; then137 exact first-NI failures+4hooks,0setup/skips/warnings and152
  dependency nodes. Earlier setup failures remain invalid red where setup failed.
- Public contracts configuration2/intake3/runtime3 and [examples4](https://github.com/Kernol12/subathon-bot-ai/issues/562#issuecomment-6892590521).
  Five test/fixture files remain byte-exact; implementation-private header changes
  are covered by whole implementation review. No baseline assertion weakened.
- [Fifth class-wide handoff](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6899622439)
  publishes all C01–C38 sites/dispositions, original transaction/resource audit and
  reproducible diagnostics. [Independent fifth WHOLE rejection](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6899914593)
  accepted I10/I11/I12/I13 and all sites except C05/I14. Every older rejection
  remains recorded in the proof and PR; none was retroactively made approved.
- [Owner's widened fifth rule](https://github.com/Kernol12/subathon-bot-ai/issues/562#issuecomment-6899304385):
  if verification still finds the SAME preownership-limit class with NO authority
  bypass AND a CORRECT refusal result, record a follow-up issue, ask for nonblocking
  disposition and continue to closing; other blockers require owner resolution.
  No remaining qualifying finding/follow-up was identified by the final reviewers.
- I14 incorrectly required a150-byte future decoder wrapper for a135-byte encoder
  result. [Owner approved ONLY sixth I14 correction](https://github.com/Kernol12/subathon-bot-ai/issues/562#issuecomment-6900143851),
  with CHANGED-PART Codex re-check, then fresh WHOLE Claude closing. Do not order
  another whole Codex verification or reinterpret the narrow receipt as one.
- [Sixth source/handoff](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6900254824):
  full SAS1 scan remains unconditional; encoder skips only hypothetical decoder
  output admission, decoder keeps its150-byte preownership guard. Encoder134
  refuses/135 fits; decoder149 refuses/150 fits, exact68-byte literal preserved.
  Startup separately bounds complete encoded committed state and anchor.
- [Changed-part Codex APPROVED](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6900387974)
  at2b609a17:221 diagnostics+seven selected methods ordinary/ASan/UBSan. Own session
  directory deleted and clean checkout confirmed; no whole verification claimed.
- [Fresh WHOLE Claude CLOSING APPROVED](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6900536055)
  at SAME2b609a17: no blockers/new findings,29-path inventory matches. Its direct
  traces and justified reliance on unchanged fifth independent evidence are
  explicitly distinguished. Own full154/154,137invocations+4hooks ordinary and
  sanitized, strict/static pass. Own1.2GB session directory deleted before reporting.
- [Actual source-head hosted CI](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6900376199):
  7/7 at2b609a17, run43754335035/Qt132935003631, pinned Boost1.83, actual61/80passes.
  This is NOT evidence that the later proof head has passed; read that separately.
- Proof serializes actual8tests+8implementation receipts and binding scopes. Tests
  paths bind immutable baseline files; whole implementation binds28 nonbookkeeping
  paths. Final closing/verification are source2b609a17; proof-only head is separate.
  Validate BOTH stages with tools/check_publication.py using isolated schema SDK.

## Cleanup and private artifacts

Latest owner cleanup instruction and main PR657 Temporary files rule apply:
`build/API-011/<session>/` contains new per-session scratch; root-sixth uses an
absolute workspace TMPDIR to avoid documented /tmp quota. All new delegates delete
OWN session directory after publishing commands/results and BEFORE reporting.
No active owned delegate remains at this proof checkpoint; verify again at stop.

AFTER protected merge read-back, delete `/tmp/api011*` entries and task scratch.
Remove worktrees assigned for API-011 and already merged ACCESS-009/010/011 and
RULES-006 ONLY if no active/pending session uses them and no uncommitted or unpushed
work exists. Never force removal. Keep unsafe worktrees and list paths/reasons in
final report. Do not delete unrelated `/tmp` data, especially not shared
`/tmp/claude-1000` as a whole. Selected external SDK/dependency caches are not this
task's disposable scratch. Legacy root task build products may be removed only
when ownership and no pending need are established. Do not remove arbitrary builds.

All essential scope/decisions/evidence/diagnostic sources are public in the linked
receipts/proof. Raw local XML/logs/auxiliary scripts/build products are disposable,
not unrecorded approvals. If cleanup retains unpublished work, report it explicitly;
no essential decision should depend on this conversation or /tmp alone.

## Capability map and constraints for a later coordinator

Read `server/include/subathon/server/reference_configuration.hpp`,
`reference_runtime.hpp`, `reference_intake.hpp` and the issue/PR links above.
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
no personal paths are required here. Validate proof with tools/proofs.py and
check_publication.py for BOTH tests/implementation, using the isolated interpreter
from docs/schema-validation.md; never enroll a pending proof.

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

Completed protected merges: ACCESS-009 (#645), PR648,1fd11c3b;
ACCESS-010 (#646), PR650,65936bcb; ACCESS-011 (#649), PR651,d7d64f25;
RULES-006 (#558), PR652,03e52e70. Their issues/PRs/proofs own
evidence. Do not reverify pre-MIGRATE004 historical records.

## Future tests-stage instruction (API-011 explicitly unchanged)

From the NEXT tests stage, before first review author runs ONLY cases calling NI
stubs, checking setup up to that call; fix setup defects and report the fixture
check in the handoff. It is NEVER red evidence: observe red after approval. Exclude
no-stub cases and TEST_STRATEGY review/admission-before-execution exceptions
(including bounded HTTP/schema), with explanation in handoff. Permanent wording
is now merged from separately owned PR656 (c3445c91) in AGENT_WORKFLOW.md and
COORDINATOR_PROMPT.md. This changes no current API-011 stage and grants no permission
to start a new task after API-011.
