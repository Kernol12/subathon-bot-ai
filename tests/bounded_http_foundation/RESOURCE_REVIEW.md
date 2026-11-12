<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
# APITEST-016 authored packet: review before execution

This is an unreviewed test author packet, not artifact admission or observed red.
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

Every feature child installs ONE genuine native profile, then requires Foundation
availability BEFORE configuring listeners or running the framework. The NI target
returns NI at `foundation.create.before_listeners_run` and exits 40 after flushing
that explicit event. All subsequent wire assertions are PRESENT BUT UNEXECUTED in
the baseline. Configuration children have the analogous create boundary; the pure
arithmetic child first requires `project_storage`. None accepts NI as success.
No feature wire red, successful registration, serving map, cleanup map or graceful
drain can be attributed to an NI child. Only reached phases have map records.

Exit 42 is an explicit assertion failure. Exit 90, missing symbols/headers,
compiler/dependency/admission problems, SDK diagnostics, malformed/truncated
protocol, event overflow, missing read witnesses, watchdog/timeout, failed reap,
scratch residue or unexpected process death are SETUP/FIXTURE failures, never red.
The two void wrong-thread children require actual SIGABRT; an arbitrary nonzero
exit does not satisfy them. No WILL_FAIL, skips or expected failures are used.

## Finite caller/fixture bounds

* Suite: 31 selectors (30 foundation methods and ONE legacy positive method),
  at most 279 sequential child lifetimes. 93 ordinary/native wire rows, 112
  separately observed CR/LF split children. Contract F13 permits 3 fresh peers
  for each of reference/tiny/P2 (9 complete children maximum); unwitnessed attempts are fixture
  failures/reaped retries, not product observations. No in-process reinstall.
  Controlled identity is a separate prospective admission invocation, not one
  of those suite children.
* Each child: 30s watchdog (300 x 100ms), main command loop <=6000 poll turns x
  5ms, <=256 commands x 512 bytes, one partial 513-byte command array and one
  512-byte read array. The main loop parses at most 512 bytes per turn. Fixture
  command queue copies total <=131072 bytes; the Python driver serializes ordinary
  commands by ack. Stop can leave one extra pending command/ack. These are fixture
  queue allocations, NOT bounded producer mailbox evidence.
* Each step <=7s, parent child horizon 30s, then at most TERM/wait2s and
  KILL/wait2s. CTest watchdog: 279*(30+2+2)+60 = **9546s**. This is a failure
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
  files, each hashed in <=32768 chunks x65536 =2GiB. No test authenticates the
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
* Full-receive case uses a finite caller gate in connected before enabling reads:
  <=5000 x1ms waits; main command releases it AFTER 65536 bytes were written.
  The actual read latch MUST witness 65536, otherwise FIXTURE failure. This gate
  does not change SDK state or forge a read. F08's distinct SDK gate is cold/once,
  actual application node only, 128 bytes inside G65536, never controls/101.
* Scratch: one exclusively created mode0700 private parent per child (path<=512),
  cwd there; at most 260 directories including root/empty/upload/tmp/256 SDK
  leaves, four 16-byte canaries =64 file bytes, no other files or symlinks.
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
transient peak<=1, backlog connects never imply accepts/construction.
F10: accept equality, request/trickle/commit equality, idle and output anchors,
first stop earlier/later repeat on an actually observed outstanding large-output node
(SO_SNDBUF4096 is fixture setup, not proof by itself), cross-owner preflight/IO nonordering, unavailable/
regressing/overflowing samples, established native/lifetime stop barriers.
F11: independent literal storage plans, L+X boundary, W overflow, duration boundary/
overflow and huge-plan epoll narrowing before listener/native coherence.
F12: once-only reserve/fill/commit, wrong fill count, before-commit drop/cancel,
committed ownership, EOF3/4, callback/read throw, wrong-thread Result/void paths,
reentrant and revoked visits, exclusive foreign moves, inert foreign drops after
join and incomplete finish, wipe quarantine. End precedes actual removed.
F13: exact key/accept/101, Host256/257, missing/duplicate framing/negotiation,
Cookie excluded from projection, CL0/00, native role refusal, read-ahead capacity
65544 vs empty0 with actual same-read witness; strong-without-lease, weak-only and
message-lease retention are SEPARATE. Native receives only Peer/Wake.

## Mandatory source/ABI oracles and unresolved testability limits

Output values cannot alone establish all requirements. These are explicit
SOURCE/RESOURCE gates for independent review and the later producer, not skipped
passing tests or asserted facts. None is implemented by NI.

1. A1: ALL retained facade methods off IO0/after close must guard before ANY
   allocation/enqueue/native access. Negative send/shutdown/forceClose cases are
   present, but unchanged metadata counters do NOT detect a hidden stock queue
   allocation. Review actual WebSocket/TCP call paths and allocation requests.
2. A2/A3: sdk_objects_released runs on IO0 after the cleanup fence (chosen producer
   assumption), Transport is INVALID after socket_removed. Native build-copy
   Session retains actual PeerStorage; installed Context lookup includes terminal
   owners for closed before detach/retire. No public connection identity ABI.
   Separate actual custom deleters/control-block allocators, two deletion records,
   inflight/weak pins and IO0 destruction must be proved at actual SDK sites;
   snapshots/zero leases/native closed/use_count cannot prove that fence.
3. A4–A6: observer scope is ordinary/pre-Upgrade reads only; thrown observer blocks
   THAT read's parse/dispatch. Pure helpers have ordinary-thread/no-authority rules;
   native cases issue no imaginary Request/Reply. F07/F08 timing rules above apply.
4. Checked incarnation/request/read ordinal exhaustion, EAGAIN/EINTR eventfd errors,
   precommit mailbox lock failure, stop-before-construction FD races, allocation
   failures and read syscall faults have normative refusal/no-wrap/no-fallback
   source oracles. The accepted first-party API has NO fault/generation setter.
   The NI packet cannot deterministically inject those private SDK conditions.
   Same for busy control storage, partial-emitted application-error output and
   forced poller/allocator capacity extremes. These are outstanding deterministic
   fault-testability limitations, NOT observed feature coverage. Coordinator/review
   must resolve whether source proof plus naturally reached integration witnesses
   is sufficient, or explicitly approve a private bounded fault seam before such
   assertions can be executable. Do not silently add an SDK/public authority hook,
   fabricate flags, skip cases or claim this author packet closes those gates.
5. Source/ABI inventory must prove all removed stock maps/pipeline/deque/send
   fallback/timing wheel/queue nodes, fixed reads/compaction wipe ranges, actual
   pre-read guard, per-turn8 accepts/continuations and8192 byte turns, fixed poller
   records and bootstrap G, one W/node, 4x128 State slots and per-peer16384 SDK
   objects/control blocks/2x128 deletion records. Pure requested reservations are
   NOT ABI/heap/RSS measurements. A build that cannot fit must refuse admission.
6. Remaining source limits are deliberate review blockers for any FULL feature
   acceptance claim, not grounds to invent an intended NI run or begin API-016.
   This author has not independently approved even the executable subset.

## Integration/admission handoff (prospective, not executed)

The subdirectory CMake uses existing `subathon_target_options`,
`subathon_native_websocket_profile`, Drogon::Drogon, Threads::Threads and
OpenSSL::Crypto. Coordinator must select this under a bounded-http-foundation
acceptance selector, provide the admission FILE PATH (its future content may be
created AFTER compilation), preserve the original dependency recipe and review
root/server/selector/preset/CI integration. No replacement standalone project.
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
`review_receipt`, complete `files` mapping absolute paths to SHA256 and scrubbed
`environment` limited to PATH/LD_LIBRARY_PATH/LANG/LC_ALL/TZ. The test consumes
that admission; it cannot grant or authenticate it. Keep matching seals at all
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
