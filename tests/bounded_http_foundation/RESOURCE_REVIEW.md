<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
# APITEST-016 authored packet: review before execution

This packet includes the ORIGINAL author's corrections and subsequent coordinator
oracle/classification fixes. It is not independent approval, artifact admission
or observed red. Initial review:
[Claude Opus5.5 report](https://github.com/Kernol12/subathon-bot-ai/pull/124#issuecomment-6823972291),
authoritative T3 identity apitest016-initial-review-20261112-v1. f25c690c is only
the report's finding namespace, not an authenticated native reviewer identity.
Authority is the COMPLETE accepted APICONTRACT-005 R3 source
`cee1845c847389697bea0097e94685d721d213e5`, with its imported closure and six
closing a8d495e8 advisories. Historical pending/R2 prose does not supersede R3.
The JSON inventory is static, not test discovery. It names every selector, literal
wire row (including request hash/length), split position and maximum child count.
No old tests, native/host sources, SDK inputs or assertions are changed.

## First missing boundary and failure classification

A legacy child installs the genuine old native profile once, starts ONE ordinary
listener with the public API, observes its actual kernel port after the real IO0
listen marker, dispatches `/legacy-positive`, and receives a real 200/body. It
proves fixture/framework/socket setup, not foundation resource properties. Its
own quit/join is fixture cleanup, not a foundation stop barrier.
Diagnostics are installed BEFORE profile construction, not just before run.

Every feature child installs ONE genuine native profile, then requires Foundation
availability BEFORE configuring listeners or running the framework. The NI target
returns NI at `foundation.create.before_listeners_run` and exits 40 after flushing
that explicit event. All subsequent wire assertions are PRESENT BUT UNEXECUTED in
the baseline. Configuration children have the analogous create boundary; the pure
arithmetic child first requires `project_storage`. None accepts NI as success.
The parent stores terminal events instead of throwing past maps: every complete
reached phase must be admitted, all pipes reach EOF, stderr must be empty and the
exact named mode/boundary must reap40 (NI) or a named assertion must reap42 BEFORE
reporting a product failure. Any mismatch is SETUP/FIXTURE. Initial/serving/cleanup
maps cannot be manufactured for unreached phases. Legacy runs first as a separate
positive gate. If it succeeds, ALL foundation selectors run without failfast;
no all-refusal/NI-only control substitutes for it. No admission document is supplied.
No feature wire red, successful registration, serving map, cleanup map or graceful
drain can be attributed to an NI child. Only reached phases have map records.

Exit 42 is an explicit assertion failure. Exit 90, missing symbols/headers,
compiler/dependency/admission problems, SDK diagnostics, malformed/truncated
protocol, event overflow, missing read witnesses, watchdog/timeout, failed reap,
scratch residue or unexpected process death are SETUP/FIXTURE failures, never red.
In particular, inability to arm or release F08's actual-node gate stays fixture
failure (exit90), as the accepted contract requires. Ordinary producer violations
after fixture availability is established retain their product assertions.
The two void wrong-thread children first observe request and a flushed operation
marker immediately preceding the foreign call, then require exact SIGABRT (-6); an arbitrary nonzero
exit does not satisfy them. No WILL_FAIL, skips or expected failures are used.

## Finite caller/fixture bounds

* Suite: 35 selectors (34 foundation methods and ONE legacy positive method),
  at most 290 sequential child lifetimes. With the current NI producer and a
  passing legacy gate, all34 methods run:97+112 subtest children +32 other first
  boundaries +1 legacy =242 expected NI-baseline child attempts (not an observation). 97 ordinary/native wire rows, 112
  separately observed CR/LF split children. Contract F13 permits 3 fresh peers
  for each of reference/tiny/P2 (9 complete children maximum); unwitnessed attempts are fixture
  failures/reaped retries, not product observations. No in-process reinstall.
  Controlled identity is a separate prospective admission invocation, not one
  of those suite children.
* Each child: 30s watchdog (300 x 100ms), main command loop <=6000 poll turns x
  5ms, <=256 commands x 512 bytes, one partial 513-byte command array and one
  512-byte read array. The main loop parses at most 512 bytes per turn. Fixture
  command queue copies total <=131072 bytes; the Python driver serializes ordinary
  commands, INCLUDING stop, by ack. One actual future timer readiness and an
  end-of-active-batch IO0 marker provides a positive barrier after delivery;
  <=256 such one-shot timers total, <=1 outstanding timer per serialized barrier.
  Timer/queue allocations are finite caller fixture costs, never foundation mailbox
  capacity. Timer wakes do not change the shared synthetic product clock. These are fixture
  queue allocations, NOT bounded producer mailbox evidence.
* Each step <=7s, parent child horizon 30s, then at most TERM/wait2s and
  KILL/wait2s. Proposed CTest watchdog: 290*(30+2+2)+60 = **9920s**.
  The coordinator-integrated CMake uses9920s for those290 children. This is a failure
  watchdog, not OS preemption or a filesystem/allocator latency guarantee.
* Writer: one 2097152-byte immutable arena and 2048 offset/length records (8 bytes
  each), total 2113536 bytes before synchronization/thread/serializer overhead.
  No callback waits on a pipe. <=2048 events, each <=65536 bytes, cumulative
  <=2097152 bytes. Single writer makes <=256 write/poll attempts per event,
  polls <=10ms/attempt; fatal NI/assertion flush waits <=3s. Overflow is fixture
  failure. JSON serialization owns a finite temporary whose inputs are bounded;
  this is not an exact malloc/ABI bound. SDK diagnostics fail via stderr; they
  cannot silently redirect request logs to files or claim erasure.
* Maps: <=256 lines/phase x <=4096 bytes, initial/serving/cleanup only when actually
  reached. Complete raw lines, including anonymous/virtual mappings, are retained
  within the SAME event/log caps; simultaneous maxima may exceed the cumulative
  cap and must fail admission/fixture honestly. File-backed maps must be absolute,
  non-deleted and hash-bound by the external admission inventory. No missing file
  or truncated map is treated as zero. Admission document <=65536 bytes, <=128
  files, regular files only, each <=2GiB hashed in <=32768 data chunks x65536
  plus one EOF read (32769 iterations), with inode/size/time stability checks. No test authenticates the
  human/source review by reading its URL: coordinator must actually perform it.
* Caller contexts: 128 fixed Peer records, at most 128 peer sockets per child;
  each record owns optional actual handles/Connection/weak/lease and exactly two
  precharged 64-byte ReadSample latches (16384 bytes across the caller table).
  Compile-only size guards enforce Peer<=512 and Rig<=66560 bytes (128*512+1024);
  these caller records are separate from producer States. No payload history.
  Only actual callbacks issue handles/context correlation.
  The largest authored connection scenario uses 82 peer sockets. Parent uses
  <=3 child pipes+128 sockets+one hash FD (132 owned FDs, plus its inherited stdio).
  Child RLIMIT_NOFILE=512 and RLIMIT_CORE=0, applied in main; pre-main SDK bootstrap
  remains separate admission. Actual SDK FD/channel layout requires source proof.
* At most six intended child threads simultaneously: creating main, framework
  main, ONE IO0, event writer, watchdog and one joined foreign worker. No Qt,
  SQL, database/Redis clients, plugins, file/access logger or runtime package
  subsystem. Framework internals remain part of selected-source admission.
* Client payload/read cap 1048576+40961+128 =1089665 bytes. Writes <=8192 per call
  (<=134 chunks), reads <=8192 per call and <=8192 iterations. Event pump reads
  <=4096, <=4096 newline scans per block, <=8192 wait iterations. F07 cumulative
  waits <=32 inspections/phase, up to four phases (<=128), under reference5000ms.
  Foundation time is a shared synthetic source; supervisor time supplies no permit.
* Full-receive peer remains actually suspended; IO0 is NEVER blocked. Each
  actual listening socket sets SO_RCVBUF262144 before listen and emits readback
  >=131072. Client SIOCOUTQ must reach0 after65536 bytes (<=700 x10ms/7s), then
  a positive timer/IO0 barrier and reads=bytes_read=0 precede actual admission.
  First actual syscall MUST witness65536; after request1 an additional8192 bytes
  are positively delivered, then another IO barrier. Reads=1/bytes_read65536,
  requests1, reserved33101824/mapped29888512 must remain unchanged, batches0.
  No TCP packet boundary is assumed. F08's distinct SDK gate is cold/once,
  actual application node only, 128 bytes inside G65536, never controls/101.
* Scratch: one exclusively created mode0700 private parent per child (path<=512),
  cwd there; at most 260 directories including root/empty/upload/tmp/256 SDK
  UPPERCASE leaves (00..FF), four exact16-byte canaries =64 file bytes,
  no other files or symlinks. `rglob` excludes root:259 dirs+4 files=263 entries.
  Before run, NI/identity cleanup sees only the authored subset (8 entries).
  Document/upload paths are absolute actual-cwd paths; getUploadPath/getDocumentRoot
  are checked. The canary is inside the SDK's ACTUAL upload/tmp/00 leaf.
  Public enableReusePort(true) bypasses `/tmp/drogon.lock`; each actual listener
  clears/readbacks SO_REUSEPORT=0 and SO_REUSEADDR=1. No guessed fixed port.
  Successful/terminal child cleanup closes/reaps immediately, checks the finite
  subtree and removes only that root. Cleanup is idempotent. At most 112 dormant
  cleanup registrations can remain within a method; buffers/FDs are cleared.
  No scan/removal of unrelated temp entries.
* Native registration is process-global in historical SDK. The single Rig/Profile/
  Clock/callback target is intentionally retained through process exit, with an
  explicit `native_registration_retained_until_process_exit` record. There is NO
  claim of unregistering/reclaiming those global owners. Feature `finish` proves
  only its actual network/handle barrier; failed/NI/aborted children rely on reap.

## Literal and ownership review mapping

F01–F05/F05P: explicit Python bytes, statuses and declared route indices, method
and target/body/field occurrence checks. F05P is B8192 only: header431 before
body413 before total431 before syntax400, with a real route3 positive. It is NOT
Cookie/auth/representation/account capture. The accepted SESSION-011/reference
Cookie, credential and 415/406/400 priorities are retained, not reimplemented.

F06: one coalesced pair, first held, actual wire release/raw/current-handle drop,
remaining suspension, explicit admit and repeated admit with exactly two requests.
F07: actual read callback before parse, signed result/ordinal/cumulative accounting,
two write-once latches; 16+16/10+10 and 32/20 use reference5000ms. Pure 8192/8191
and 65536/65537 guards supplement actual read witnesses. Observer throw must
prevent dispatch of that read's bytes; established native Session is excluded.
F08: local IO0 sequence starts only when both real HTTP requests arrived; W132+
node128=260 held after first handle drop, second reserve resource refusal BEFORE
fill, actual release, response right retry; separate ungated 66-byte positive.
Deadline expiry during that sequence is FIXTURE failure. Cancel/stop/expiry override
the actual held node in separate children; no kernel EAGAIN/peer-receipt claim.
F09: P2 roles, reference80 and a native reserved opportunity, input8/9 before read,
transient peak=1, backlog connects never imply accepts/construction.
F10: accept equality, request/trickle/commit equality, idle and output anchors,
first stop earlier/later repeat on an actually observed outstanding large-output node
(SO_SNDBUF4096 is fixture setup, not proof by itself), cross-owner preflight/IO nonordering, unavailable/
regressing/overflowing samples, established native/lifetime stop barriers.
F11: independent literal storage plans, L+X boundary, W overflow, duration boundary/
overflow and huge-plan epoll narrowing with coherent C1/U1 native profile.
Overflowing next-P under mismatched reference-native C32/quota1 MUST be arithmetic
failure before native/epoll refusal. Allocation uses a current /proc/self/statm
observation, checked RLIMIT_AS=current VM+67108864 headroom, and coherent B=E=1GiB:
requested L=17192980480 exceeds that finite headroom; expected allocation_failure.
Unable to establish the limit is fixture failure; no forced allocator hook exists.
F12: once-only reserve/fill/commit, wrong fill count, before-commit drop/cancel,
committed ownership, explicit End0/2/6, actual EOF read-result0 with body3/4, callback/read throw, wrong-thread Result/void paths,
reentrant and revoked visits, exclusive foreign moves, inert foreign drops after
join and incomplete finish, wipe quarantine. End precedes actual removed.
F13: exact key/accept/101, Host256/257, missing/duplicate framing/negotiation,
Cookie excluded from projection, CL0/00, native role refusal, read-ahead capacity
65544 vs empty0 with actual same-read witness; strong-without-lease, weak-only and
message-lease retention are SEPARATE. Native receives only Peer/Wake.
After closed and positive IO0 progress, strong/weak cases require actual
native_retires1 while sdk_retained_peers remains1; the live message-lease case
instead requires native_retires0 until release. Removed must remain absent for
that peer in the parent's events. An IO0 assertion immediately before the real
foreign reset also rejects any earlier removed callback, closing the buffered
event race. The eventual once-only removed event carries the actual retirement
counter and must report1 in all three cases. Stop-retention/post-after-stop cases
use the same checks before drain/join. Retirement polling is finite (at most32
barrier/inspect pairs,64 commands) within existing256-command/30s/event budgets;
it issues no new public authority, native fault setter or manufactured SDK fence.

## Mandatory source/ABI oracles and unresolved testability limits

Output values cannot alone establish all requirements. These are explicit
SOURCE/RESOURCE gates for independent review and the later producer, not skipped
passing tests or asserted facts. None is implemented by NI.

1. Closing a8d495e8 A1: ALL retained facade methods off IO0/after close must guard before ANY
   allocation/enqueue/native access. Negative send/shutdown/forceClose cases are
   present, but unchanged metadata counters do NOT detect a hidden stock queue
   allocation. Review actual WebSocket/TCP call paths and allocation requests.
2. Closing a8d495e8 A2/A3: sdk_objects_released runs on IO0 after the cleanup fence (fixture's
   explicit producer assumption, MUST be ratified in API016 implementation source
   review under initial advisory A14; not an amendment or an executed observation), Transport is INVALID after socket_removed. Native build-copy
   Session retains actual PeerStorage; installed Context lookup includes terminal
   owners for closed before detach/retire. No public connection identity ABI.
   Separate actual custom deleters/control-block allocators, two deletion records,
   inflight/weak pins and IO0 destruction must be proved at actual SDK sites;
   snapshots/zero leases/native closed/use_count cannot prove that fence.
3. Closing a8d495e8 A4–A6: observer scope is ordinary/pre-Upgrade reads only; thrown observer blocks
   THAT read's parse/dispatch. Pure helpers have ordinary-thread/no-authority rules;
   native cases issue no imaginary Request/Reply. F07/F08 timing rules above apply.
4. Checked incarnation/request/read ordinal exhaustion, EAGAIN/EINTR eventfd errors,
   precommit mailbox lock failure, stop-before-construction FD races, allocation
   failures and read syscall faults have normative refusal/no-wrap/no-fallback
   source oracles. Initial Claude explicitly accepted these as NI-baseline source
   gates, not executed fault coverage. The accepted first-party API has NO fault/generation setter.
   The NI packet cannot deterministically inject those private SDK conditions.
   Same for busy control storage, partial-emitted application-error output and
   forced poller/allocator capacity extremes. These are accepted NI-baseline
   source-only gates, NOT executed feature coverage. Allocation's finite RLIMIT_AS
   case and EOF0 are authored executable witnesses. Linux RST does not guarantee
   readFd returns -1/ECONNRESET: selected Channel handles HUP/ERR before reads,
   and SO_ERROR consumption/close ordering can change the syscall outcome.
   Negative syscall/error propagation therefore stays source-reviewed, rather
   than becoming a flaky expected errno or an excuse for a new fault seam.
   No SDK/public authority hook, fabricated flag, skip or fault execution claim
   is introduced. Partial-emitted output followed by terminal close already has
   a one-response-prefix oracle in test_stop_first_deadline.
5. Source/ABI inventory must prove all removed stock maps/pipeline/deque/send
   fallback/timing wheel/queue nodes, fixed reads/compaction wipe ranges, actual
   pre-read guard, per-turn8 accepts/continuations and8192 byte turns, fixed poller
   records and bootstrap G, one W/node, 4x128 State slots and per-peer16384 SDK
   objects/control blocks/2x128 deletion records. Pure requested reservations are
   NOT ABI/heap/RSS measurements. A build that cannot fit must refuse admission.
6. These are later mandatory producer source/ABI gates, accepted as source-only
   for the NI baseline by initial Claude. They do not block authoring NI source
   solely because no setter exists, and do not authorize skipping feature gates
   or implementing API016 before reviewed red. This author approves no stage.

## Integration/admission handoff (prospective, not executed)

The subdirectory CMake uses existing `subathon_target_options`,
`subathon_native_websocket_profile`, Drogon::Drogon, Threads::Threads and
OpenSSL::Crypto. Coordinator commits0d829be/e2d2428/934d4e5 (imported unchanged) own selector closure,
three-attempt F13 bound, CACHE receipt default, preset and tooling/strategy work.
B8/A13 are coordinator-authored, independently source-verified by Codex01a1cffc
and01a1d00e, and retained by the first Claude closing review. That closing found
a different blocker; it did not close the whole tests stage. No real
admission content is generated by CACHE default; runtime refuses missing receipt.
Coordinator retains shared build/preset/CI ownership; the current290-child timeout
is integrated. No replacement standalone project.
NI contains ONLY bounded_http.cpp; NEVER bounded_http_sdk_adapter.cpp or successor
vendor symbols. Later implementation selects a different producer target, rather
than link both NI and implementation definitions.

Before any load: exact selected candidate4 upstream/patch/manifest/source inputs,
compiler/stdlib/C++20/OpenSSL/shared recipe, passive COMPLETE ELF/interpreter/
DT_NEEDED/VERNEED/symbol/RTTI/ABI/transitive/license closure and reviewed source/
fixture/build packet for THIS consumer. Then controlled scrubbed identity invocation
`bounded_http_probe '{"mode":"identity"}'` in a disposable private cwd, check
initial/cleanup maps and actual binary/DSO SHA256 seals. No parent probe receipt
admits this executable. Runtime admission JSON has `source_commit`, actual HTTPS
`review_receipt`, complete `files` mapping absolute regular-file paths to lowercase
SHA256, exact13-path `sources` mapping every reviewed test/probe/NI/header/inventory/
integration input to lowercaseSHA256, and scrubbed string-valued `environment`
limited to PATH/LD_LIBRARY_PATH/LANG/LC_ALL/TZ.
Only that admitted environment reaches the child. Passive ELF/interpreter/DSO
review must supply any necessary loader search path there (including
LD_LIBRARY_PATH if required); CTest does not inherit one into the child. Missing
loader setup is SETUP failure of the legacy positive, never behavioral red.
The test consumes that admission; exact keys/types/hex/duplicate-key checks and explicit fail-closed
`require` gates never disappear under -O. Optimized Python is refused before launch.
The external coordinator MUST authenticate review and commit-to-source binding;
a syntactically valid URL/commit is not authentication. Sources are sealed before
and after EACH child as well as at driver entry. No receipt is invented here. Keep matching seals at all
reached map phases and before/after each child. Synthetic only.

Only after independent Claude initial -> author fixes/separate Codex verification
until no blockers -> fresh Claude closing, reviewed shared integration and artifact
admission, coordinator may execute:
`python3 tests/bounded_http_foundation/test_bounded_http.py --probe <absolute-probe> --admission <reviewed-json>`.
First legitimate baseline evidence is legacy positive followed by explicit NI
availability failure, with downstream bodies UNEXECUTED. Further execution/real
producer source/resource review, successor native44 and all host Qt75/native18/SQL,
account/TLS/provider/platform/security/deployment/release gates remain separate.
All seven integrations, three FX providers, English/German, Light/Dark/System,
separate renderers and full desktop editor remain in scope.

## Corrected witness details and accepted source dispositions

First closing f1a7beb9 C1: the first required false wipe may occur at release_raw
AFTER commit. Quarantine cannot retract bytes already in the kernel. The existing
wipe child therefore reads to EOF/reset and requires an empty/full/prefix of the
ONE literal DIAG response; a second head or any other bytes fail. Reads are bounded
to67 received bytes and68 attempts within the existing step/child horizons.
fixture_wipes>0, output_slots0 and finish->wipe_failed remain mandatory. This
changes an unsupported pre-approval oracle, not an observed/approved red baseline.

Closing CA1: retained-native backlog checks now require no extra accept_attempts
and no increase in constructed, matching F09 without assuming the latter counts
only cumulative or only live SDK objects. CA2's stale B8/A13 status is reconciled
above. CA3's admitted loader environment remains an explicit admission gate, not
an inherited CTest environment. Optional CA4 is authored in this same wipe child:
null owner/wipe/revoked each invalid_config BEFORE valid installation; a repeated
valid install then invalid_state. No extra child/selector/watchdog or authority
seam is added. All these downstream assertions remain unexecuted behind NI.

Initial B5's suggested `read_guard_pauses>=1` would overconstrain R3. Section3,
"Body, coalescing and raw hooks" (lines630–637) requires reads disabled after one
complete request until raw/wire/current handles/admission permit a successor;
section3 "Fixed storage" (lines552–558) checks writable only BEFORE another actual
read. A conforming implementation reaches the one-current-request suspension
first and never asks check_read_window again. The full-receive case discriminates
with actual65536 syscall + positively delivered extra8192 + a future IO turn,
unchanged syscall/byte/storage/request counts, and stock_batches0; the pure8192/
8191 case independently fixes the guard boundary. It does NOT manufacture a pause.

IO barriers are finite fixture timer events, followed by queueInLoop markers at
that batch's end. Selected Trantor EventLoop.cc:221–239 polls, dispatches ALL active
channels, THEN doRunInLoopFuncs (337–360). A command queue ack by itself is not a
network opportunity because that function drains nested queued functions too.
Before absence claims, send one byte to a full-role backlog peer and observe
SIOCOUTQ0 (ACKed data), then wait for the future real IO timer/batch marker.
SDK source/resource review must retain equivalent barrier semantics in successor.

B7 adds explicit end reasons, HTTP retained-Peer/Wake quota pin/reuse, native idle
beyond HTTP deadlines followed by binary41, native callback/read-counter exclusion,
control_capacity0 after101, gate refusing to capture400/101 but retaining the next
actual app node, post-stopping refusal, and a LIVE uncommitted foreign Request drop.
The stop-gate case uses one fixture command to record a5s stop and advance to5s,
below the6s output bound: earlier or deadline terminal drain is allowed; actual
release must be once, prefix at most the one66-byte response, with End drained
or stopped (a drained End requires the complete literal). No required timing pick.

Candidate4 source-review sites (read-only basis, not executed source coverage):
WebSocketConnectionImpl.cc send/shutdown/forceClose and setPingMessage paths;
TcpConnectionImpl.cc:413–458 runInLoop facade captures, :545–620 queued send copies;
WebSocket destructor/connPtr owns TCP; TcpConnectionImpl constructor/parser/channel
ownership sites and control-block deletion requests; native first-party
native_websocket_profile.cpp:150 Owner weak SDK references and :927 Context::retire
(terminal/detached/leases/work) are separate from actual SDK destruction.
Read callback TcpConnectionImpl.cc:129–153 initializes saved errno and calls
readFd; Channel.cc:78–101 handles HUP/ERR/READ separately. Actual retained
Connection/weak tests cannot prove guard-before-allocation or deletion-thread
behavior. A14 requires explicit source-review ratification of the IO0 cleanup
callback assumption before any implementation approval.
