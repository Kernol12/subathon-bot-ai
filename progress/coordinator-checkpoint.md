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
parked. Separately owned PR656 was merged by its owner atc3445c91; integrating
that documentation-only main update grants no new process/task assignment.

## Current boundary: I14 requires owner resolution before further source work

Source63fffbaa's [NEW fifth-round WHOLE rejection](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6899914593)
resolves I13 and confirms I10/I11/I12, but NEW I14 blocks C05: encode_reference_state
returns a135-byte ApplicationStateSnapshot yet its final shared scanner requires
a hypothetical150-byte decoded ReferenceCommittedState. Actual input82/document68/
envelope106/I0 all fit135;134 correctly refuses,135/149 wrongly refuse,150 succeeds
with the same independent literal. Root freshly reproduced using its own library.
No authority bypass, but INCORRECT refusal: the owner's same-class follow-up
exception does NOT qualify. Do not waive it or start another material source round.

[Concrete UNAPPLIED proposal](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6899963288):
separate encoded syntax/binding validation from decoder-result admission. Keep
encoder's own135-byte output guard, decoder's actual150-byte guard BEFORE ownership,
startup's full committed-state/anchor guards, every format/header/test unchanged.
The private two-path patch passes apply-check only; no fix is applied or tested.
Owner approval is required for further material source work; no sixth source round
or fifth baseline correction is authorized. All prior receipts stay recorded.

Verifier completed without child work. No owned delegate remains active; unwatch
653 before owner handback. Source63fffbaa has actual hosted7/7 and independent
154/154+137invocations/4hooks normal/sanitized PASS, but review is REJECTED. The
checkpoint-only update has separate pending CI and changes no runtime behavior.
After owner resolution, NEW independent WHOLE verification and NEW fresh WHOLE
Claude closing, proof/publication/final-head CI/protected merge/read-back/close562
remain. Finish only API-011 then STOP; next candidate642 still needs owner resume.
Essential reproduction/proposal/source audit/review is public via these links and
[the fifth-round handoff](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6899622439);
no decision depends only on thread or /tmp. Raw logs/build products may expire.

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
That round applied audit-reason document measurement to both
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

[Owner approved ONE bounded fourth source round](https://github.com/Kernol12/subathon-bot-ai/issues/562#issuecomment-6897511532),
accepting the [published proposal](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6896984370).
At this publication it corrects20 imported DATA fields across15 DTOs, including
hold/resolution/override reasons, with unchanged field order, charges, optional
semantics and equality. Hold/bundle document ceilings stay separate.
Unused application-codec limits do not narrow read-only store snapshots; port
DTOs still enforce that codec's limits at their actual boundary. Exact size
measurement borrows the existing backends; metadata rejects clearly oversized
components before ownership. A separately bounded base-2^32 working buffer counts
decimal digits without an Exact/text clone. First traversals use owning Exact
component caps, independently of raw permission-source component budgets.
No public header/test/fixture, schema, build, CI or numeric format changed.

Root final154/154, exact137invocations+4hooks in ordinary AND fresh ASan/UBSan,
actual4096/4097 persisted-hold/service disclosure, prior commit/reconcile controls,
20 text-role pairs,72 large-number boundaries, zero/sign and original/fallback
diagnostic controls pass. Huge numerator/denominator/sign probes observe0-byte
new allocations in64-byte private preflight and largest56bytes in public prepare,
not the former1048584-byte clone. Separate producer budget controls pass.
Sanitizers have leaks disabled/external libraries uninstrumented. An initial
sanitizer compilation hit scratch quota; obsolete generated root products were
cleaned, sources/receipts/logs preserved, and a -g1 rebuild/check succeeded.
These are author checks, not independent approval or a general allocator proof.

[NEW WHOLE rejectione1e0a0ed](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6898478039)
resolves I11/I12 and identifies NEW I13: shared startup/snapshot `authorize` copies
policy/expanded roles/catalog/owner/actor BEFORE its prospective calculator input
is checked against the independent permission limits. With only permission
input_owned_bytes1, the graph read is admitted but an observed1280-byte catalog
clone atauthorize:93 precedes original EffectivePermissionFailure(input_limit),
actual rollback/work_exception/no snapshot. The semantic catalog exceeds B1;
this is not comparing ABI allocation bytes with the semantic charging formula.
The receipt includes the minimal real-SQL reproduction and stack attribution.

[Owner approved ONE bounded fifth source round](https://github.com/Kernol12/subathon-bot-ai/issues/562#issuecomment-6898949641),
accepting the [published proposal](https://github.com/Kernol12/subathon-bot-ai/pull/653#issuecomment-6898635596).
That round adds a complete borrowed prospective permission-input projection BEFORE
any assigned-role, flattened-entry, catalog, owner, actor or request copy. It uses
the existing reviewed authority/graph declaration-order traversal, now shared in a
private server header, with a prospective flattened-vector max_size guard. All
nested rows, catalog operations (including unrequested ones), optional-presence,
reference and numerator/denominator component charges are included. Adapter B/I/ref
and calculator B/I/ref/component budgets stay separate; adapter refusal retains its
priority and calculator refusal remains original EffectivePermissionFailure(input_limit).
Graph/account/session reads retain their independent producer budgets and fresh
checks. Existing materialization/evaluation follows only after both projections fit.
The shared extraction changes no SQL, public interface, schema, policy or test.

Root fresh154/154 nodes, all137invocations+4hooks with exact discovery equality in
ordinary AND separate ASan/UBSan builds, strict six-TU/format/project/diff checks,
16 independently hand-counted individual/role/deny/quantity fit-and-one-less cases,
8 actual startup/snapshot nested-field refusals, role/scope/owner-deny controls and
adapter-first error controls pass. Allocation traces have sensitive fitting controls:
8 direct authorize allocations each on startup/snapshot, ZERO such >=128-byte traces
on all8 narrowed profiles; the previously observed1280-byte catalog clone is absent
before refusal. Legitimate graph producer allocations remain. These are author
checks, not independent approval or a general heap/work proof. Sanitizer leaks are
disabled and external libraries uninstrumented. Public contracts and all approved
137 cases/fixtures remain byte-exact. The two existing private authority paths add
to the whole PR review scope (29 paths from current main), not a new task.

[Owner widened this SAME fifth round](https://github.com/Kernol12/subathon-bot-ai/issues/562#issuecomment-6899304385):
search the WHOLE API-011 producer for borrowed data copied before its own limit,
fix all found sites and give the verifier each site/disposition. The author audited
all intake/configuration/runtime/reference-application functions and private helpers.
In addition to I13, the round now projects configuration read keys, complete
startup/encoded state+anchor, normalized receipts, conditions/evidence observations,
capture keys/bindings/index values and retained envelopes/rows before copying.
Reservation observations and complete pending values are projected before entropy
or cloning; added journal elements are charged before insertion. Settlement and
reconciliation use complete borrowed comparisons and preflight replacement anchors.
Snapshot/receipt wrappers move admitted values. Historical authority keys stay
borrowed until their complete OWN request budget fits; Allowed revision output is
checked before copying. Prospective explanation DATA roles match owning DTOs.
Head configuration metadata admits both reference and whole-value bytes before
payload read. No reviewed test/public contract, schema, format or policy changed.
The PR handoff publishes the complete C01–C38 site inventory and dispositions;
remaining independently bounded transfers/dependency returns are distinguished.

Normal154/154 and exact137invocations+4hooks, fresh ASan/UBSan inventories and
focused probes pass on the final widened source; builds reuse only this session's
own fresh fifth-round outputs and selected external SDK. Scratch build/harness
failures are preserved as failures, not red. New independent literal controls pin
retained-key65/64, retained-row5/4 and pending318/317 or2311/2310 boundaries before
view/session access or entropy. All original I13 controls/source-order checks stay.
These are author observations; no independent fifth-round approval is inferred.

PR653 stays draft/unmerged and562 open. NEW independent WHOLE Codex verification
must check this class across the whole producer using the site inventory, then NEW
fresh WHOLE Claude closing. If verification still finds a site of THIS SAME class
with NO authority bypass and a CORRECT refusal result, create a follow-up issue,
ask the reviewer for a non-blocking disposition and continue through closing and
normal merge gates WITHOUT asking for a sixth round. ANY OTHER blocker stops for
owner resolution. This is the owner's explicit scoped exception, not a blanket
review waiver. No fifth baseline correction or downstream task is authorized.

Preserved fourth-round independent final154/154, exact137invocations+4hooks ordinary/sanitized
and hosted7/7 passed but did not waive I13. Reviewer's initial /tmp-backed full151/154 and
direct49pass/12fail were quota/setup failures, preserved, never intended red or
passing evidence. Unchanged code passed with build AND TMPDIR on the workspace
filesystem. Continuation should use an ignored workspace build path and absolute
workspace TMPDIR for compiler/test databases; -g1 alone did not cure that quota.
Keep failed logs/receipts/source; generated products may expire.

Remaining gates: NEW independent WHOLE Codex verification, NEW
fresh WHOLE Claude closing, one final proof/policy commit, both-stage publication
validation, final-head CI, protected merge/read-back, close562 and STOP. Preserve
every rejection. The current fifth-round author checks do not replace either required review;
current-head hosted CI must be read back independently before merge.

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
