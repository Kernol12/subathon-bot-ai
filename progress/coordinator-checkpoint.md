# Coordinator checkpoint

Owner: Kernol12. Issues own brief/status/prerequisites; PR comments own reviews.
Complete specification v0.45, all seven integrations and all three FX providers
remain release scope. This checkpoint grants no new authorization.

## Owner scope: API-016 → APITEST-015 → API-001

Authorized on 2026-11-16, in this order and nothing else:

1. **API-016 [#616](https://github.com/Kernol12/subathon-bot-ai/issues/616)**: bounded HTTP transport foundation. Claimed, tier A.
2. **APITEST-015 [#613](https://github.com/Kernol12/subathon-bot-ai/issues/613)**: revised host baseline (tests stage of API-001), tier A.
3. **API-001 [#278](https://github.com/Kernol12/subathon-bot-ai/issues/278)**: the production host, tier A.

SLICE-001 (PR660, 579afcb3) and the owner checkpoint on #278 are done.

**Owner decision under ADR-0012 (2026-11-16): option 1.** The host baseline is
revised so that the host runs the path that already runs in `subathon-dev-server`.
Groups A to E are approved exactly as [itemized](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6907983078); the decision is
[recorded here](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6908067929). Nothing beyond that list may be removed or replaced.
If the tier A review changes an item, bring only that item back to the owner.

Both owner conditions are met before any removal:

- C: [#662](https://github.com/Kernol12/subathon-bot-ai/issues/662) (SESSION-013) exists. The host has no generation fence; the store's
  epoch check is the fence, and APITEST-015 names it in the boundary contract.
- E: both clock guards are acceptance criteria of SESSION-009 [#582](https://github.com/Kernol12/subathon-bot-ai/issues/582).

The briefs of #613 and #278 are rewritten for this decision. SESSION-013 and
SESSION-009 are planned, not authorized.

PR #121 is closed, not parked. API-001 restarts from its rewritten issue on current
`main` in a fresh session; the PR #121 code is reference material only and none of
its reviews, approvals or CI results carries over.

Process/tooling #619/#620 stays frozen; lifting that freeze is the owner's decision.
Dependency readiness authorizes nothing beyond the path above. No live chat or
payment side effects are authorized.

## Working rules from the owner (2026-11-15)

- After a fix round the verification is a changed-part re-check, as
  `docs/AGENT_WORKFLOW.md` describes: the changed commit, the findings and their
  resolutions. Do not order a new whole verification per round.
- `/tmp` is RAM-backed on this machine. Sessions put build directories on disk, in an
  ignored directory of their worktree (`build/TASK-ID/<session>/`), and follow the
  "Temporary files" section of the workflow. Name that directory in every assignment
  and review packet.
- Before the first tests review the author runs only the cases that call a
  not-implemented stub and reports the fixture check in the handoff; it is never red
  evidence.

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

API-016 (#616) is claimed by the coordinator on branch
`task/API-016-bounded-http-foundation`. Its sessions and stage are recorded on the
issue and its PR. Verify the actual checkout, worktrees and PR state before acting;
never infer a live session from this file.

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
