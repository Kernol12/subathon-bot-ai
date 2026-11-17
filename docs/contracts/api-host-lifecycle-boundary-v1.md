# API host consumer boundary and baseline examples — proposal v1, revision 2

APITEST-015; identity `subathon.api-host-lifecycle-boundary.v1`. Specification
v0.45 §§2/10/16/17/18: SERVER-01, DB-01, API-01/02/03, AUTH-05/12,
OPS-02/04, TEST-04/05. This is a reviewed-baseline proposal, not host support.
The [approved groups A–E](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6907983078),
[decision](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6908067929),
[epoch amendments](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6913620654),
[changed terms](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6914837587)
and [final reply rule](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6916926039)
govern the revised baseline; later owner handle decisions on #613 govern its header.
All assertions outside that approved revision remain binding.

Import [lifecycle](api-host-lifecycle-v1.md), [listeners](api-host-listeners-v1.md)
(including §6 amendments), [native profile](native-websocket-profile-v1.md),
[wire](wire-primitives-v1.md), [writer](sqlite-writer-v1.md),
[work](writer-transactions-v1.md), [catalog](migration-catalog-v1.md),
[account/session storage](sqlite-account-sessions-v1.md),
[resolution](session-resolution-v1.md), [session lifecycle](session-lifecycle-v1.md)
and [clock](session-clock-v1.md). Their independent requirements remain binding.
Historical receipts are in PR112, current work in #613/PR667; neither proves unrun tests.

## 1. Actual sources, ordinary activation and forward restore obligation

The Qt owner privately owns the actual writer/store/catalog and startup attempt. First submit
`SqliteMigrationRunner::enqueue(MigrationMode::inspect)` with the actual compiled
`reference_authority_repository_manifest()` through that writer. Require once-only acknowledged
committed/none/value completion, migration reason none, report current/version10/target10/applied0 today
(compiled head is authoritative). Fresh, old (including complete version8), future head11,
wrong/foreign/malformed or uncertain catalogs refuse without setup, advance or operational approval. Then
use the SAME writer for one actual work transaction reading the configured installation through
`SqliteAccountSessionUnit::read_installation`; require its selected identity, valid current
epoch/revision and acknowledged normal COMMIT. No unrelated writer access/mutation interleaves the chain.
Match actual completions to this attempt/owner/store; DTO equality, request IDs, a fixture flag or
listener readiness is insufficient. Stop/failure prevents activation. Activate storage only after both
completions, before protected entry; preserve I/E/J and all session rows. No installation creation, epoch
rotation, session-time change or auto-migration. OPS-02 update/backup/quiescence remains caller-owned.

No supported restore/replacement producer or restore fence exists in versions9/10:
Q/N invent no stimulus/cleared flag. Out-of-band copying/VM rollback is clock §1's
trust limit; unknown CURRENT facts refuse. [STORAGE-007](https://github.com/Kernol12/subathon-bot-ai/issues/326)
requires every restore crash point to retain the original installation or require
guarded restart/refusal before restored credentials, extending startup for interrupted
replacement/acknowledged epoch fencing, recovery/outbound holds, hard-end and no replay.
No mechanism is selected; waiting would cycle through SESSION-009/API-001.
No restore acceptance or prerequisite is waived.

**Epoch fence:** no host generation/mutation facade/precommit reread. Authenticating units use
`resolve_current_account_session`'s epoch comparison (revoked on mismatch); session insert/replace uses
`current()`/`accepted()` in `sqlite_account_sessions.cpp`. SESSION-013 (#662) verifies every
session-bearing unit before first rotation; scope/provenance tickets never imply host membership. The
resolver compares before sampling the clock; its reference current-session source uses
`read_lifecycle_actor` on the active unit session, and `accepted()` checks authentication bindings. This
does not claim every unit has been traced.

**Clock ownership:** no host session clock/sampler/facade. SESSION-009 (#582) owns
revocation/thread/reentrancy and wall-jump guards; absent clock means UTC unavailable.
Credentials require a fresh operation-owned sample immediately before synchronous
emission, never execution time; metadata/expiry/wipe/once-only/global holds remain.

## 2. Minimal proposed C++ consumer boundary

The public declarations and handle semantics are in
`server/include/subathon/server/api_host.hpp`; these replace the historical
Execution/Transaction/AccountSessions/Publication/complete surface. Handles are
host-issued, move-only and cannot reconstruct membership from IDs. `Options`
owns data/limits, never a ready/trusted/source port; Host owns its private writer.
State/Failure/Result meanings and finite RouteId dispatch stay unchanged.
`credential`/`account` require unavailable downstream owners, not a routing shortcut.
`PeerLease` comes only from actual connect/close; ingress reserves complete
association, capacity, copy and correlation costs BEFORE copy/ownership/queueing.
No arbitrary ingress callback; IO never waits for Qt. Response leases remain
move-only/IO-owned/correlated/once-only/budgeted; copy pays another reservation but
shares authority. Recheck expiry/disconnect before borrow; no writer/source/secret
crosses to IO. Handoff neither blocks Qt nor confirms receipt; inert records stay inert.

Writer defaults: pending128 (standalone64), statements64/rows1024; pending>=associations.
[Observed](https://github.com/Kernol12/subathon-bot-ai/pull/667#issuecomment-6932486629): two normal current-v10 inspections under actual host defaults, not advancement,
populated scans, installation reads or activation. Fixture/dev explicit16384/65536 stays.

### Diagnostic route execution

Diagnostics synchronously execute bounded nonsecret data on Qt OUTSIDE writer
work: no handle/BEGIN/source/session clock/protected entry. No storage activation
or session UTC is required; serving/transport/stop/elapsed/response gates still are.
Exceptions suppress reply or use the bounded nonsecret reserve, never domain truth.
Queued callbacks cancel before entry on disconnect/expiry/stop. Active callbacks
are non-yielding; retain dependencies through drain/owner cleanup. No network wait
or cross-route reply ordering is promised.

### Prepared units and asynchronous command completion

One command is active at a time, including diagnostics between protected commands.
Before execute, check stop INCLUDING pending native signal, disconnect, original
queue deadline, elapsed and activation. Refused entry submits no unit and emits no
writer completion. Consume pending signal before forward, finish and reply handoff,
even if recorded inside a body. A signal handler only records/wakes; stop cannot
preempt a running callback but prevents reply and further forwarding.

`CommandHandle` declares input(), forward(existing Work,WorkCompletion) and
finish(optional reply); execute returns void and may prequeue multiple units.
Only execute/completions forward; body forward ends without reply. Existing
writer/store APIs, one-BEGIN, real results and Qt completion order remain unchanged.
Units own BEGIN/COMMIT/rollback; the three-prequeued-unit fail-close oracle stays.
Payload is a borrowed charged host input view; unit captures follow existing intake
resource rules UNCHANGED. No new capture bound, charging requirement or producer API.

### Host reply eligibility after prepared units

The host withholds a reply whenever a unit's outcome is unknown or something
was committed without ending normally; otherwise the dispatcher may reply.

| Actual unit outcome | Unit-outcome reply side |
| --- | --- |
| outcome_unknown, any error/value presence | Withheld |
| committed, WorkError::none, value present | Dispatcher may reply |
| committed, any other error or absent value | Withheld |
| rolled_back, any error/value presence | Dispatcher may reply |
| not_started, any error/value presence | Dispatcher may reply |

This exhaustive partition includes writer-established rollback after error; hidden
exceptions get no new observation under first-error priority. Withholding latches
across later normal units. SQL truth is never relabelled, retried or replayed.
The header fixes known rollback/busy replies, normal domain-failure values and
zero-unit reply eligibility. API-010 (#560) owns content: a command with a unit
ending without commit never yields success; host does not inspect content.
Keep commit-then-throw committed/work_exception/no-value/no-handoff. Prequeue3:
outcome_unknown/uncertain_outcome then two not_started/writer_unavailable, storage
inactive, no handoff, all completions before writer ack; the FIRST result withholds.
Callback exceptions/stop remain separate gates. Retain reply within budget until
normal callback return, accepted drain and lease/elapsed/authority/once-only gates;
never hand off inside that callback.

### Admitted-command handle rules outside the normal path

`CommandHandle` comments fix exact signatures, failures, ownership and C2 decisions.
| Rule | Required behavior |
| --- | --- |
| (a) | Owner-only operations; foreign call rejects before host/writer access or any command mutation. Reentrant forward/finish refuses invalid_completion without state change. |
| (b) | Terminal handle inert, no resurrection. No active-command timer; response elapsed/disconnect revokes reply only. Consumed stop includes pending signal; forward or finish(reply) returns stopping and ends without reply. Finish(no reply) may end subject to (e). Abandonment, including foreign-thread destruction, records bounded lifetime-safe cancellation for owner cleanup; cleanup submits no new work; an outliving handle never dereferences Host. |
| (c) | Escaping execute/completion exception ends without reply/replay. All accepted bodies and once-only completions drain with actual truth; completion throw is not retried. No release/next entry before they return; stopped/unavailable admits no next command. |
| (d) | Actual writer admission only. A refused admission invokes no body/completion or writer_completed and ends without reply. Pending units and host records stay bounded by pending_batches and declared host accounting; no secondary pending list. Accepted completions drain but cannot submit after terminal refusal. |
| (e) | Body forward or finish, or finish while accepted units remain pending, refuses invalid_completion and ends without reply. Last unit completion may finish after pending count drops, but handoff/next entry awaits normal callback return. Execute prequeue remains allowed. |
| (f) | Return from execute/completion unfinished with no accepted unit pending ends without reply on that owner turn. No idle continuation service, timeout or scheduling API. Nonreturning callbacks prevent graceful owner barrier. |
| (g) | Stop cancels waiting commands and cuts active forwarding between units; accepted bodies/completions drain unchanged. Barrier waits for cleanup/drain, never external handle finish. |
| (h) | Host input storage/capacity/copy/association charges survive execute and all accepted bodies/completions until no borrow can use them; release once. Payload remains a borrowed charged host input view. Unit captures follow existing intake resource rules unchanged. Inert handles cannot pin payloads; response charges survive their separate transport barrier. |
| (i) | No other command, including diagnostic, enters between active command units. Ingress/disconnect/stop may progress between callbacks; no event pumping within them or second writer. |
| (j) | Actual unit-entered is host-emitted before body, actual writer_completed after ticket revocation and before supplied completion. A disconnect there precedes reply. Distinguish admission/entry/unit/end/cancel/forward-refused/finish-refused; non-writer events carry no invented result. Second finish refuses invalid_completion without changing a chosen finish/reply. W3 admission < stop < cancellation; all accepted completions precede writer acknowledgment. |

### Shipped transport, signal and elapsed composition

N links shipped Host/Drogon/signal/elapsed sources and matching public headers; Q injects fake transport
only through the same boundary. Process-main Qt owns Host/dispatcher/queue/writer/SQL through
destruction. One framework-main runs Drogon; exactly one IO0 listens/accepts and owns
sockets/parsers/callbacks/handoff/Close on selected Linux; startup resources retain their loop owners.
Dual readiness, scratch, endpoints/socket options follow listeners §§1–6, never a timer/bind-only
observation. No further Trantor event loop is constructed while the foundation is installed. The N probe
supplies finite dispatch data, disposable paths and observation/synchronization only; it never implements
an alternate listener, signal handler, readiness/transaction bridge or shutdown, fills production stubs
with fixture logic, or sets readiness/invocation through control channels.

Signal implementation requirements (production StopSignalBridge):

| Phase | Required ownership/safety |
| --- | --- |
| Install | Qt before framework/IO; disable Drogon's SIGTERM handler. SIGINT/SIGTERM use proven lock-free atoms, preopened nonblocking CLOEXEC pipe and async-signal-safe write preserving errno; a full pipe already wakes. |
| Handler | Record/wake only, never allocation, locks, Qt/Drogon/SQL or Host. Ordinary request_stop is thread-safe, coalescing at most one owner notification, NOT signal-safe. |
| Teardown | Keep state/descriptors valid through framework/IO exit and late entries; gate entry plus in-flight count BEFORE descriptor borrow, close gate, wait every allowed borrower before close/reuse. Quiesce ALL threads. One-thread masking, volatile sig_atomic_t or zero sampled once is insufficient; only separately reviewed equally safe replacement is allowed. |

Host owns ONE read-only Linux CLOCK_MONOTONIC source and builds the native profile with it;
options/transport inject no other profile/clock. The production pure converter handles exact nonnegative
normalized sec/nsec→U64ns, unavailable for syscall/malformed/overflow; no
float/wrap/saturation/UTC/supervisor fallback. Lifecycle §4 fixes same-domain deadlines, equality expiry,
no extension and per-owner nonregression without ordering cross-thread reads. Q calls the real converter;
N proves wiring, not syscall failure, trusted UTC, wall changes or scheduling bounds. The production
monotonic adapter MUST call `convert_monotonic_observation` after its real syscall; source review
verifies that reuse.

Lifecycle §5's stop barriers require owner-loop IO/framework callback/lease drain before network
acknowledgment, Qt writer completion/connection removal before owner destruction, then framework/IO joins
and Qt cleanup. BOTH acknowledgments are required; a blocked body means incomplete shutdown, never
deadline-certified stopped. Bind/listen exit1 is process-terminal, not destructor proof; startup callback
exceptions require the same cleanup even in failed state.

### Acceptance-only hooks and concrete accounting

`testing/api_host_observers.hpp` has bounded synchronous nonsecret observations,
no host lock held, no payload/authority setters. Only request_stop reentry is
allowed; never pump/wait for the emitting thread. Empty hooks use production.
No session sampler/envelope seam. writer_completed is COMMAND units only; actual
startup inspection/read observations may separately contain real writer truth.
One cold/idle ack-loss fault forwards ONCE to SqliteWriterWorkAccess after open,
before inspection if cold; no tickets/results escape. Unknown inspection stops
BEFORE installation enqueue; diagnostics may run, protected work cannot.
Diagnostic queued is after actual dependency retention on Qt, before entry;
refused ingress and later owner operation_refused are distinct actual phases.

| Owned bridge storage | Reservation rule / reference bound |
| --- | --- |
| Each retained input or response payload copy | Its whole allocated usable capacity C, plus ONE 128-byte metadata slot; payload length ≤ C. Reference payload capacity totals remain 8388608 input and 8388608 response. |
| Copy metadata | 128 slots per input pool and 128 per response pool, each charged128 bytes (16384 per pool). Slot exhaustion refuses even if payload bytes fit. A move transfers the existing slot/capacity without another charge. |
| Association bookkeeping | 128 slots charged512 bytes each (65536 total): inline256 bytes for the unchanged wire correlation bound plus256 metadata; per-peer cap8. Copy metadata above is separate. |
| Correlation / other variable data | One inline correlation in the association slot, borrowed by pinned execution/response records through their terminal barrier; any retained duplicate needs another charged slot. Other variable data uses the capacity/copy ledger; no uncharged string/lambda copy. |
| Refusal/control fallback | One preallocated1024-byte bridge control slot per admitted peer, at most128 peers (131072), respecting the unchanged CommonFailure bound; otherwise close without allocating a reply. Native control/output buffers retain their separate native-profile budgets. |

Reserve full usable capacity, slots and metadata BEFORE host allocation/copy; growth reserves full
replacement while old lives. Moves transfer a charge; copies pay again. Bridge pool sum is
16777216+32768+65536+131072=17006592 bytes, excluding separate native buffers, bounded startup-store
reads, fixed owner/event tables and allocator/framework overhead. Slots must fit selected-target
representations; larger metadata needs reviewed revision, not hidden heap metadata. These are engineering
charges, not exact allocator/RSS or one aggregate process ceiling. OwnedReply is already-allocated
trusted bounded result, not retroactive allocation enforcement; capture rules remain the intake's.
Inspect capacity/copies/current/peak charges and one-time release; source/resource review remains
mandatory. Reduced input/response oracle: cap8/slots2, capacities4+4 charge264; third1 and
envelope4/payload5 refuse. Release one→132; move+0. Cap12/slots2 still refuses third.

## 3. Independent literal examples

Independent planned oracles: Q=Qt, real disposable writer/store, fake transport and
controlled elapsed; N=real corrected-framework Linux children/peers with native
elapsed wiring. Supplied time is simulation. Ordinary positives use actual §1;
no public setter creates readiness and no prerequisite producer is invented.

| Mapping / home | Literal stimulus | Required result |
| --- | --- | --- |
| HL01 / Q, N ingress | Global=2; A/B outstanding, one completing; submit C | Resource refusal before copy/domain entry; count2. Release A once, then D eligible. Also reference128/129 with small bodies. |
| HL02 / Q | Global=4, per-peer=2; P owns2; submit P3 then Q1 | P3 refuses; Q1 eligible, total3. Disconnect P releases its terminal leases once, not Q's. Reference8/9 same oracle. |
| HL03 / Q | Admission0; queue budget5000ms; separate dispatch4999/5000/5001ms | 4999 may enter once if current gates pass; equality/later zero execution/unit/writer completion. No callback-driven extension. |
| HL04 / Q | Order disconnect before entry; separately after entry before handoff | First zero body/BEGIN. Second true commit/outcome retained with response revoked; no automatic replay. Owner ordering chooses one winner. |
| HL05 / downstream | Host session-clock facade removed | Both guards belong to SESSION-009 (#582); see Clock ownership in §1. |
| HL06 / Q, N | Actual §1 current-catalog activation, then a prepared unit reads I/E/J and commits | Normal committed/none/value completion, ticket revoked; startup chain order and I/E/J/session rows unchanged. One correlated nonsecret reply. |
| HL07 / Q | Commit then body exception; lost acknowledgment/fail-close; completion exception; stop | Actual unit results unchanged, no handoff/replay; see §2 prepared units. No envelope or publication stage. |
| HL08 / Q, N | Listener/writer without activation; separately current catalog inspection + configured read on same writer | First diagnostics only; second storage-ready execution/reply eligible with I/E/J/session facts preserved. Credentials still need downstream clock/login owners; no restore prerequisite. |
| HL09 / downstream | Host generation/mutation/rebind cases removed | Authenticating units use resolve_current_account_session; session insertion/replacement uses current()/accepted(). No host generation fence; SESSION-013 (#662) verifies every session-bearing unit before first rotation (§1). |
| HL10 / Q, N | Stop before serving during startup, then three signals; attempt start again | No serving admission; one fence/deadline/cleanup; restart refuses. Handler only records/wakes. Fixture barriers synchronize events, never set host state. |
| HL11 / N | Parent holds loopback listening port; child starts there. Release port; NEW child starts | First exits1 on selected bind failure; no destructor/graceful-stop claim. New child may start; no in-process framework retry. Reap both. |
| HL12 / Q, N | Stop with idle TCP, partial HTTP/Upgrade, partial WS frame, queued command and finishing executing write | Queue cancelled; executing actual outcome retained; Close once; peers close/force-close by original network bound. Actual writer/network barriers precede stopped/destruction. Separate nonreturning-body child never proves stopped; watchdog kill is process-terminal only. |
| HL13 / Q, N | Commit then local handoff; peer disappears before receipt | One commit, at most one local handoff, no confirmed-delivery claim or automatic replay/old-digest restoration. Correlation is not retry authority. |
| HL14 / Q actual writer | Admit commands W1–W3; W1 commits; stop before W2 entry | W2/W3 never enter, each cancelled by host; one execution, writer_completed=1; admission of W3 < stop < cancellation. W1 completion and stop precede writer acknowledgment. Accepted units still drain with unchanged writer truth. |
| HL15 / Q, N elapsed | Start1000ms, budget5000ms; elapsed5999/6000/6001ms | Deadline6000ms; first eligible, equality/later refuse without reset. Host/native profile share ONE elapsed source/domain. The removed UTC-jump stimulus belongs to SESSION-009 (#582), as specified in §1. Real N wiring does not imply privileged wall-clock-change capability. |
| HL16 / Q | Now18446744073709551615ns plus1ms; separate sec=-1, nsec=1000000000, unavailable, per-owner100→99ns | Overflow/malformed/unavailable/regressing observation refuses affected entry/handoff and tears down affected peer on IO turn. No wrap/saturation/unlimited timeout/SQL invention; executing work retains truth. |
| HL17 / Q, N | Clock unavailable; nonsecret diagnostic with valid elapsed lease; separately credential-requiring route | Diagnostic needs no session sample, but all transport/lifecycle/elapsed gates. Credential publication suppressed; no invented login or actual secret issuance. |

Handle examples:

| Given / call | Required result |
| --- | --- |
| Unit BODY calls forward; separately calls finish | invalid_completion, command ends without reply; accepted units drain. |
| Reentrant operation or second finish | invalid_completion, no state change or change to chosen finish/reply. |
| Pending native signal before forward; separately finish(reply) | stopping, command ends without reply; accepted truth/completions unchanged. |
| Entered command disconnects between units | Later forward remains allowed, reply refuses disconnected; host input charges survive drain. |

Additional required failure/resource/ownership families:

| Family / home | Literal boundary/event | Required observation |
| --- | --- | --- |
| Owned input / Q | Envelope4, aggregate8; own two4-byte inputs; attempt5-byte input or another1-byte copy | Refuse before copy. Exact4 eligible when aggregate permits. Another retained4-byte copy costs4; move adds no second payload. Charge capacity4+metadata128 per copy; two copies264, move adds0; see §2 before authoring. |
| Response / Q, N | Single4, aggregate8; two4-byte leases; third lease, duplicate completion, disconnect, expiry at5ms, delayed IO continuation | Bounded control refusal or close; no uncharged copy/duplicate/late handoff. Release once, current totals0 after barriers. No raw secret in observers. |
| Configuration / Q | Zero duration, incoherent counts/capacities, UINT64_MAX milliseconds | Fail startup with no framework activation; separately check duration conversion and deadline addition, never float/wrap. |
| Elapsed conversion / Q | sec18446744073 with nsec709551615, then709551616 | First exactly UINT64_MAX ns; second overflow refuses. Per-owner nonregression must not falsely order simultaneous reads by other threads. |
| Thread/source / Q, N | IO submit, Qt entry/unit body/unit completion, IO handoff | Entry, writer_completed, writer removal, dispatcher destruction and owner cleanup on Qt; unit body and completion on owner; handoff/teardown on IO. Record thread IDs/destructors; source review supplements counters. No network wait on state thread or state work on IO. |
| Activation failures / Q, N source cases; Q startup uncertainty | Fresh/old (including complete version8), future head11, wrong/malformed catalog, missing/wrong/malformed installation; startup inspection lost acknowledgment | Refuse without advance/setup/rotation. Unknown inspection completion stops before installation read, storage activation or protected reply; diagnostics still work. Current catalog succeeds through §1. Startup private queue is initially empty with coherent nonzero capacity and no unrelated writer access, so no manufactured queue-full fault. Ordered fail-closed COMMAND completions remain the separate HL07/14 test, not startup evidence. Genuine enqueue/open failures refuse; no arbitrary writer-status setter is added. Unknown current facts refuse; DTO equality/request reference cannot enable activation. The lost acknowledgment fail-closes the writer: stop the chain rather than enqueueing a second read to manufacture writer_unavailable. Host startup stop fencing is covered below. No restore-fence producer/stimulus is invented; STORAGETEST-007 retains supported restore tests. |
| Downstream authority / Q | Stop or actual storage uncertainty; absent account source | No protected reply after stop/uncertainty. The minimal host surface supplies no account source. Account-dependent operations refuse; account recovery and epoch invalidation remain with their owning source/ACCESS-005. Its eventual guarded source must invalidate at real mutation and cannot clear independent global holds; no unobserved writer access. This remains a downstream integration gate, not an authored account-recovery claim here. Host has no source-rebind/publication facade. |
| Stop barriers / Q, N | Stop1000ms, drain5000ms, Close1000ms; traffic/repeat stop1999/5999ms | Drain stays6000ms; Close retains its own first-Close deadline. Revoke/drain actual IO callbacks before network acknowledgment/dependency destruction. Writer ack follows all accepted completions and connection removal. Keep Qt dispatcher through owner cleanup; IO join follows socket/callback cleanup. |
| Startup cancellation / Q, N | Stop before start, during inspection, late listener-ready after stop, startup callback exception | No transient admission, one cleanup, no restart. Bind/listen exit remains distinct process-terminal category. |

## 4. Harness gates and proposed next ownership

Q real Qt/SQLite/writer/store seeds BEFORE acquisition, never host entitlement;
pump BETWEEN callbacks only. Frozen profile installs once permanently: fresh process
per literal method[:row], one Rig/host; no reset/replacement/multirow/bespoke runner.
Compare `tests/qt/api_host_cases.cmake` to admitted data-tag discovery. Historical
75Qt/18N map: remove9Qt/replace14/E-drop8/rename11/untouched33; native0/5/4/9.
Declare additive final counts before tests review; static counts are not execution.

Q proves shared-source identity/attributable host reads, not native reads through
fake transport. Per-thread probe: Qt100 diagnostic→IO99 take→another IO99 take
succeeds; same-owner100→99 refuses. N empty hooks uses actual monotonic factory and
profile with200ms frame budget: complete frame succeeds; real partial masked frame
closes by7s without stop/fixture close/domain entry; another peer still succeeds.
Supervisor rejects premature close but is NEVER product time. Retained IO lease→
disconnect→delayed take refuses, no local handoff; repeated reset releases all
capacity/copy/metadata/association charges once; committed truth survives.
Global-exhaustion recovery releases only A while B stays outstanding. Idle/partial
HTTP/Upgrade stop may give bounded HTTP refusal/EOF; WS Close needs actual upgrade.

N fixture limits remain: inherited FD, separate stdout/stderr; serialized1MiB arena/
256descriptors,256events×128KiB,64commands×4096bytes,8peers/4096byte-operation, 1MiB total logs;
step7s/child20s, TERM then KILL reap2s each. Loaded-map cap128, a candidate bound, not measured; complete
initial/post-QSQLITE/serving/stopped admission; hash-cache size/mtime/ctime.
Watchdog/overflow/setup/ABI failure is not red or graceful cleanup proof. Before
bodies require source/resource/fixture and passive ELF/ABI/identity/seal/map gates
for THIS consumer, not old probe approval. Unreviewed tests stay local; independent
tier A oracle/stub/fixture/resource/build/CI review precedes intended red. All pieces
stay explicit NI (cold inspection, inert stop, no callbacks/writer/framework/clock);
NI is never success, historical red never revised-feature evidence.
API-001 needs fresh implementation/review, passing acceptance/regressions and protected
merge. SESSION-009/login/restore/permission/sync/TLS/platform/release stay separate;
all7 integrations,3FX providers, English/German, appearance and both renderers remain.
