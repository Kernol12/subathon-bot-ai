# Coordinator checkpoint

Owner: Kernol12. Full specification v0.45, all seven integrations and all three
FX providers remain release scope. Issues own status/prerequisites; PRs own reviews.

## Latest owner boundary: finish API-011, then STOP

Owner instruction2026-11-15 overrides the old standing loop. Finish ONLY API-011
(#562/PR#653) through reviews, proof, final-head CI, protected merge, merge read-back
and closing562. Then stop. Do not claim/start SLICE-001 or any other task even if
ready. Before handback verify this coordinator's delegates finished/cancelled and
unwatch its PRs; report merge SHA, next candidate and anything not carried over.
If merging needs an owner decision, stop there and ask; never waive a gate.

Next candidate is **SLICE-001 #642 (tier B)**, requiring explicit owner resume/new
assignment. Historical remaining path is642→API-016#616→APITEST-015#613→API-001#278;
all is suspended by this stop. Process/tooling619/620 stays frozen until a server
actually starts, runs a timer and accepts a fake contribution. Draft121 stays
parked. PR656 is separately owned, not ours to review/merge without assignment.

## Task, checkout and authoritative evidence

- [API-011 #562](https://github.com/Kernol12/subathon-bot-ai/issues/562) owns the full brief and acceptance.
- [PR653](https://github.com/Kernol12/subathon-bot-ai/pull/653) owns current head, reviews, CI and eventual protected merge.
- Branch `task/API-011-reference-intake`, original base03e52e70. Recheck actual
  checkout/main/PR/rules before acting; no stale-head CI or inferred live sessions.
- If653 is unmerged, finish only this task. If merged and562 closed, the assignment
  is over. A future merge SHA cannot be embedded in this pre-merge checkpoint;
  the PR merge record/final owner report supply it. No follow-up task is authorized.
- Final source/review bindings live in `progress/proofs/API-011.json` and
  `progress/proof-policy.json` once the one final proof commit exists. Until then
  PR receipts/status are authoritative. Never infer approval from the notes below.
- [Durable handoff](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6895564949)
  publishes all8 tests receipts/hashes, full137-case red classifications, the old
  setup failures, diagnostic sources, artifact reading order and limitations.
  Essential knowledge no longer depends on this thread or `/tmp`. Raw local logs,
  auxiliary audit scripts and build products may expire; outcomes/source bindings
  are public. Historical worktrees/assignments are not running sessions.

## API-011 stage evidence and gates

Both stages are tier A. Public contracts: configuration2, intake/runtime3;
[examples4](https://github.com/Kernol12/subathon-bot-ai/issues/562#issuecomment-6892590521).
Tests review closed atd7155b42, then actual137/137 exact first-NI failures,
4passing hooks,0setup/skips/Qt warnings and152/152 dependency nodes. Original
fe14d2bb135NI+2SETUP failures remain historical and invalid where setup failed.
Bounded third/fourth test corrections were owner-approved; no fifth authorized.

- [Author implementation/audit7e9f648b](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6894300934).
- [WHOLE Claude rejectiond69f2617](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6894462643): I01 snapshot session must precede head/config query.
- [Fix1 bf0c4ab1](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6894871585): auth ordering, integer clock, complete binding/policy, dangling-head and scoped body-error handling.
- [WHOLE Codex rejectionbf0c4ab1](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6895075222): I01–I07 accepted; I10 losing-result reference ceiling and I11 command-reason document ceiling are blockers.
- [Fix2 a135f48f](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6896077228)
  repairs runtime refusal projection and command-reason measurement, but is NOT
  approved. Tests/public contracts remain fixed.
- [NEW WHOLE Codex rejectiona135f48f](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6896259943):
  I11 still treats `ApplicationAudit::reason` as a reference. A valid4097-byte
  reason commits in the real service, then `settle` rejects it before journal
  promotion; historical disclosure has the same walker. I10 intake `refuse` and
  result-limit fallback still construct owned reasons before checking limits.
  At reference ceiling31, the32-byte runtime refusal is built before omission.
  The receipt includes minimal reproductions independent of local scratch files.
  Fresh154/154,137directinvocations+4hooks, ASan/UBSan (leaks off) and hosted7/7
  pass at that source; none waives the rejection.

[Owner approved ONE bounded third source round](https://github.com/Kernol12/subathon-bot-ai/issues/562#issuecomment-6896696424),
accepting the [published proposal](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6896336588).
At this publication that round applies audit-reason document measurement to both
command and persisted audit, preserving every field/charge/optional/equality rule.
All intake literal/forwarded refusals and result-limit fallback are projected before
owning their reasons. An over-limit refusal carries no owned diagnosis and retains
the same real rollback/error; the complete wrapper remains separately bounded.
Tests/public contracts are unchanged. Root full154/154, exact137directinvocations
plus4hooks, real4096/4097 commit/settle/journal/disclosure and refusal-limit controls,
strict compilation, format/project/diff checks pass. New independent reviews and
hosted CI must bind to the actual new source; old green checks grant no approval.

[NEW WHOLE rejection261d8003](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6896940946)
resolves I10 but keeps I11 open: valid persisted4097-byte hold reasons are still
treated as references, so actual whole-hold disclosure and the real service fail.
The same field-role error affects resolution/plan-override reasons structurally.
NEW I12: free Boost numerator/denominator accessors clone before checking limits;
the reviewer observed1048584bytes allocated before refusal, including public
prepare's losing proposal path. Minimal reproductions and source mechanisms are
public in the receipt. Fresh154/154,137invocations+4hooks, ASan/UBSan (leaks off)
and hosted7/7 at261d8003 pass but do not waive either blocker.

**STOP: owner approval required for ONE bounded fourth source round.** No fourth
implementation round or fifth baseline correction is authorized or applied.
[Unapplied proposal](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6896984370): complete the API-011 walker DATA/reference field-role audit for
hold/resolution/override reasons with unchanged complete charges/equality, and
borrow Exact component backends for metadata preflight before any size-dependent
copy/conversion. Preserve the working I10/command-audit fixes and all baselines.
No other product-policy decision has been identified. PR653 is draft/unmerged;
562 remains open. The last verifier is complete with no child work; no owned
delegate is active. Unwatch653 before owner handback; start no downstream task.

Only after owner approval and corrections: NEW independent WHOLE Codex verification,
NEW fresh WHOLE Claude closing, one final proof/policy commit, both-stage publication
validation, current required CI, protected merge/read-back, close562 and STOP.
Preserve every rejection. This checkpoint-only amendment changes no source, test,
contract or CI; its pending CI is separate from the reviewed green source above.

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
is separately owned in PR656 (owner reports approval7acad387). This grants no656
merge assignment and no permission to start a new task after API-011.
