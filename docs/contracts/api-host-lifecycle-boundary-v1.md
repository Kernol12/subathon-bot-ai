# API host consumer boundary and baseline examples — proposal v1

APITEST-001; proposed identity `subathon.api-host-lifecycle-boundary.v1`.
Authority: specification v0.45 §§2/10/16/17/18, SERVER-01, DB-01,
API-01/02/03, AUTH-05/12, OPS-02/04 and TEST-04/05.
Source base `654ac016268c6caa9efc0241872ca0b0f88aae60`; correction base
`ce80bbafbb1b678826f596beae65e2f8c350f1b2`. Revised after initial contract
review; separate independent Codex verification and closing Claude review remain.
No headers, tests, observed red or implemented host.

Import [host lifecycle v1](api-host-lifecycle-v1.md), including ALL HL01–17,
[native profile v1](native-websocket-profile-v1.md),
[wire primitives v1](wire-primitives-v1.md), [writer v1](sqlite-writer-v1.md),
[work v1](writer-transactions-v1.md), [catalog v1](migration-catalog-v1.md),
[physical account/session v1](sqlite-account-sessions-v1.md),
[supplier v1](session-resolution-v1.md), [lifecycle v1](session-lifecycle-v1.md)
and [clock v1](session-clock-v1.md) unchanged. API-001 retains actual host
composition, startup/current-source production and downstream clock holds.
APITEST-001 supplies only their independently reviewed executable baseline.

## 1. Actual sources, ordinary activation and forward restore obligation

Read at the source/correction bases above; observations are not executed results.

| Actual source | Available mechanism / limit |
| --- | --- |
| `server/include/subathon/server/sqlite_writer.hpp`, `server/src/sqlite_writer.cpp` | Owner-thread `open`, `enqueue_work`, `shutdown`; `finish()` revokes tickets before completion; accepted work drains in FIFO. A wrapper returning without BEGIN yields `not_started/missing_transaction`; fail-closed dispatch yields `not_started/writer_unavailable`. No arbitrary work preemption or queue-removal API. |
| `server/include/subathon/server/sqlite_migrations.hpp`, catalog contract | Explicit `SqliteMigrationRunner::enqueue` inspection/advancement. Only acknowledged committed, error-free, current-schema inspection supplies the schema condition. Open/fresh/upgrade-required/advancement alone is not operational approval. OPS-02 update/backup/quiescence remains caller-owned. |
| `server/src/sqlite_account_sessions.cpp`, `account_session_repository_manifest()` | Actual compiled prefix1–8, version8 `account-sessions-v1`; no installation replacement/startup journal, auto-migration or operational approval. |
| Same source, `read_installation`, `replace_installation_epoch` | Selected singleton facts through a real revocable caller session; CAS of expected installation/epoch/revision. Neither operation records replacement intent or acknowledges a restore. `AccountSessionReadScope` supplies snapshot provenance, not host membership. |
| `server/include/subathon/server/session_resolution.hpp` | Unchanged `CurrentSessionClock::sample_current()`: sample or unavailable. No concrete trusted native clock/host binding. |
| `server/include/subathon/server/native_websocket_profile.hpp` and source | API-012 `create_profile`, owned `ElapsedSource`, message leases, `start_close`, deadline rechecks and snapshots; install once before native startup. No UTC, host readiness or host shutdown barrier. |
| `tests/native_websocket_profile/`, `cmake/DrogonDependency.cmake`, `tools/native_websocket_ci.py` | Corrected-SDK identity/import/load infrastructure and real Linux child/peer checks. Probe force-close/quit is fixture cleanup, not API-001. Keep all44 reviewed methods and admitted source/patch bytes unchanged. |
| `server/`, `deploy/`, STORAGE-007 and ACCESS-005 briefs | No executable restore/replacement startup protocol or durable pending/acknowledged replacement producer found. Existing application-review holds and captured clock journals are not that protocol. |

Also read the coordinator checkout's selected candidate4 source/installed headers
under `build/deps/`, without loading/executing them. They expose
`disableSigtermHandling`, `registerBeginningAdvice`, actual `getListeners`,
`getIOLoop`, dedicated `addListener(..., nativeProfile)` and `quit`.
Selected Trantor `Socket::bindAddress`/`listen` still call `exit(1)` on failure.
These permit disposable-process tests; they do not prove owner barriers.
No new framework dependency/correction is proposed.

**Ordinary activation is owned by API-001 and is not blocked by restore.**
Use the actual compiled `account_session_repository_manifest()` prefix1–8,
head/version8 `account-sessions-v1`. Open the selected writer on the Qt owner;
submit `SqliteMigrationRunner::enqueue` in `MigrationMode::inspect` through that
SAME live writer. Require its actual once-only completion: transaction committed,
work error none, migration reason none and normal report `current`, version8,
target8, applied0. Fresh/old schemas, a future head9, foreign/wrong/malformed
catalogs or uncertain/failed inspection refuse without advance. Inspection is not
itself readiness and never authorizes setup or an update.

Then enqueue the configured installation read through that SAME writer, using
one real work transaction and `SqliteAccountSessionUnit::read_installation` with
bounded scope/limits. Require the selected identity and valid current epoch and
revision, actual acknowledged COMMIT and normal error-free completion. The host
privately matches both completions to this live startup attempt/owner/store;
request IDs, matching DTOs and a fixture flag cannot supply this chain. No unrelated
writer access or source mutation may interleave this startup chain. Stop/failure
at either step prevents activation. Bind bounded current facts and the initial
live generation only after this chain and before protected entry. Do not create
an installation, migrate/advance, rotate epochs or alter session times. Listener
readiness/serving and each operation's other actual sources remain separate gates.
Q AND N must prove this positive chain and preservation of I/E/J and session
facts; absence of SESSION-009 still refuses trusted UTC and credentials.

The baseline at schema8 has NO supported restore producer. An out-of-band database
copy or coherent administrator/VM rollback is the explicit clock §1 host trust
limit; ordinary startup does not invent detection of that history. Unknown or
malformed CURRENT facts and any actually known unresolved startup/recovery fence
still refuse. This is not a new `normal_startup` flag, reusable issuer or public
restore-cleared input. No new prerequisite/task is allocated, no dependency graph
is changed and no full restore acceptance is waived. In particular, waiting for
STORAGE-007 here would create the existing graph's cycle:
STORAGE-007 depends on SESSION-009, which depends on API-001.

**Binding forward obligation in the existing restore task:** the shared
[STORAGE-007 / STORAGETEST-007 brief](../tasks/EPIC-009/STORAGE-007.md) requires
that NO supported restore crash point leave the selected path acceptable as an
ordinary store to a host lacking a restore consumer. Interrupted supported restore
must either leave the original ordinary installation intact or require guarded
restart/refusal before any restored credential acceptance/publication. For example,
staging an epoch/Recovery/outbound-fenced state before atomic installation, or a
durable intent plus a future catalog that older hosts refuse, may satisfy parts
of that obligation; these are examples ONLY, not chosen mechanisms or proof of
sufficiency. STORAGE-007 must choose/review its actual protocol and extend the real
guarded restart startup, including interrupted replacement and acknowledged epoch
fence, with its existing test-first gates. Recovery review, invalidated sessions,
outbound holds, hard-end enforcement and no replay all remain mandatory. API-001
need not implement that future restore consumer now.

**Live source ownership:** API-001 owns writer/catalog lifetime, nonreentrant
execution/publication membership and bounded current installation storage.
Every admitted transaction uses the host finalization wrapper in §2. No raw
writer/ticket escape permits untracked commits. Completion borrows live bounded
host storage, never a dead repository unit or revoked ticket. Uncertainty revokes
it; future adapters must maintain the same invariant or refuse publication.

## 2. Minimal proposed C++ consumer boundary

Signature/ownership proposal for later headers, subject to independent contract
approval; not a compilable stub. Namespace `subathon::server::api_host`,
C++20/Qt and owned PIMPLs. Supporting types are bounded as described below.

```cpp
class Host; class Ingress; class Execution; class Publication; class PeerLease;
class Transaction; class AccountSessions;
struct NotImplemented {};
enum class State { cold, starting, serving, stopping, stopped, failed };
enum class Failure {
    invalid_options, wrong_thread, already_started, stopping, unavailable,
    resource_limit, expired, disconnected, cancelled, invalid_completion,
    elapsed_unavailable, incomplete_shutdown
};
template<class T> using Result = std::variant<T, Failure, NotImplemented>;

class CommandDispatcher {
public:
    virtual ~CommandDispatcher() = default;
    virtual OwnedDomainResult execute(Execution&, CommandView) = 0;
    virtual OwnedReply complete(Publication&, const OwnedDomainResult&) = 0;
};
class Transport {
public:
    virtual ~Transport() = default;
    virtual Result<StartAccepted> start(Ingress&,
        std::unique_ptr<native_ws::Profile>, StartupObserver) = 0;
    virtual void handoff(ResponseLease) = 0;
    virtual void stop(StopDeadline, NetworkBarrierObserver) = 0;
};
class Host { // owner-thread construction/use/destruction, except stop ingress
public:
    Host(Options, std::unique_ptr<Transport>,
         std::unique_ptr<CommandDispatcher>);
    Result<StartAccepted> start(StartupObserver);
    void request_stop() noexcept;
    Inspection inspect() const; // nonsecret owner-only observation, no permit
};
class Ingress { // synchronized network ingress, never domain execution
public:
    Result<Admitted> try_submit(const PeerLease&, CorrelationView,
                               RouteId, std::span<const std::byte>);
    void disconnected(const PeerLease&) noexcept;
};
class Execution { // host-only constructor, borrowed within execute() only
public:
    Transaction& begin_transaction(); // host owns the one actual transaction
    std::shared_ptr<CurrentSessionClock> clock() const;
    CurrentInstallationView current_installation() const;
};
class AccountSessions { // host-only constructor; checks live execution/transaction
public:
    AccountSessionRead<StoredInstallationFacts> read_installation(std::string_view);
    LifecycleWriteResult replace_installation_epoch(const StoredInstallationFacts&,
        std::string_view new_epoch, std::string_view new_revision);
};
class Transaction { // host-only constructor; borrowed during execute() only
public:
    AccountSessions& accounts(); // guarded facade, not a raw session/unit callback
    void commit();              // host precommit read + actual COMMIT
    void rollback();            // actual rollback, with unchanged uncertainty
};
class Publication { // fresh host-only constructor, no writer accessor
public:
    std::shared_ptr<CurrentSessionClock> clock() const;
    CurrentInstallationView current_installation() const;
};
```

- `Options` owns selected store path/installation selector, existing writer/store
  limits and host/native resource/deadline limits. Selection is DATA, never ready/trusted/current
  invocation/generation/restore-cleared input. Startup composition stays internal,
  not an arbitrary injectable ready-result port. Host owns its writer, not a
  borrowed writer admitting unrelated source mutations.
- `RouteId` is a finite compiled trusted dispatch table, not a wire schema or
  client callback. It declares diagnostic/protected work and mandatory trusted
  publication UTC. Bounded bytes/correlation cannot select authority/membership.
  `CommandView` borrows admitted owned bytes during its synchronous body only.
- `OwnedDomainResult`/`OwnedReply` hold bounded nonsecret bytes/domain status.
  Host wraps normal body return in a PRIVATE typed envelope matched to the actual
  once-only writer completion. Client correlation/QVariant shape cannot forge
  it; errors cannot reconstruct it from captured intent. Session candidates stay
  inside their owning reviewed lifecycle envelope, never these generic bytes.
  This proposal does not implement session issuance or accepted login.
- `PeerLease` is opaque and issued/revoked only by transport connect/close events;
  fake transport uses those same entry points. It grants association/capacity,
  never protected readiness. `Ingress` reserves complete lengths/slots/copy costs
  BEFORE ownership/copy/queueing; no arbitrary captured lambda. Its borrows stay
  alive through the accounted transport barrier.
- Execution/publication cannot be publicly constructed/copied. Retainable clock
  handles own revocable facade state, not raw pointers to expired scopes. Guards
  run before underlying samples. Installation views borrow bounded current
  source storage only during a live scope. Execution revokes on every body exit;
  fresh publication revokes before completion returns, including exceptions.
  Absent SESSION-009 always means unavailable, not a substitute native clock.
- `ResponseLease` is move-only, correlated, once-only, IO-owner-bound, budgeted,
  with original elapsed expiry. Inert completion records may survive disconnect;
  late callbacks cannot revive response authority. Handoff never blocks Qt or
  confirms receipt. No writer ticket/source borrow crosses to IO.
- Bounded observers/inspection expose actual state, separate host cancellation,
  writer outcomes, reservations/peaks and cleanup acknowledgments. They cannot
  change readiness. A nonreturning work body cannot service owner inspection;
  the external child supervisor observes the missing barrier, not fake stopped.

### Host-controlled transaction and source finalization

`Execution::begin_transaction()` performs the one actual `SqliteWorkContext::begin`
after entry/cancellation/deadline/generation gates. Host-private transaction state
owns that context, its revocable session and the actual `SqliteAccountSessionUnit`.
Neither context, session, writer, connection, generic SQL executor, commit-capable
callback nor raw unit escapes to the dispatcher, fixtures or publication. Existing
public writer/store APIs are unchanged. The host constructs their real objects
internally rather than subclassing or modifying them.

`AccountSessions` is a host facade over that actual unit; the two methods above
are the minimal selected baseline surface. Its named bounded read/mutation
methods forward the existing unit operations, including the REAL
`replace_installation_epoch(expected_facts, new_epoch, new_revision)`: the first
argument contains the complete expected current facts; the remaining arguments
are the replacement epoch and revision. It preserves the same inputs/results; it grants no independent authority. Every method checks
live execution/transaction/owner/nonreentrancy before accessing the unit. Borrowed
facades cannot construct/copy a raw ticket or finalize a transaction; retained
clock handles remain revocable as above. Any later repository needs an explicitly
owned guarded accessor; no escape hatch returning `SqliteWorkSession`,
`SqliteWorkContext` or a lambda receiving either is permitted.

`Transaction::commit()` ALWAYS rereads the selected bounded installation facts
through its private actual unit inside the SAME active SQL transaction, AFTER all
unit mutations and immediately BEFORE actual `session.commit()`. Require valid
identity/epoch/revision and budget for this final read within the existing work
SQL/row/value limits; read failure cannot be bypassed by committing elsewhere.
Block further facade operations while finalizing and after either terminal call.
Stage those facts only in a private host record tied to this work, never in the
returned domain envelope. A failed precommit read cleans up the active transaction;
no COMMIT follows. Real rollback/cleanup uncertainty revokes the live source.

Only the matched actual writer completion confirming `transaction=committed`
can bind the staged candidate; no queued work can interleave before this owner
completion. Revoke the execution binding before leaving the body and retain no
unit/ticket as a publication source. Unknown outcome, lost acknowledgement,
missing candidate or impossible completion revokes protected source availability;
a plausible reread alone cannot clear uncertainty. A normal commit with unchanged
facts does not rotate the generation. Known rollback leaves the
previous committed facts intact. An acknowledged commit binds even if the body
then threw: retain committed truth, suppress that operation's publication because
its normal error-free private envelope is absent. An ordinary committed normal
completion may publish only through the separate current-source checks.

On a changed selected installation/epoch, the OWNER revokes old queued work,
facades and source borrows and advances its internal generation before any new
publication/admission; no public setter exists. Revision-only changes refresh
bounded facts and revoke old borrows as appropriate. Exhaustion refuses/terminates
rather than reusing a generation. A rebind cannot clear independent recovery or
clock holds. The changing operation's old-generation publication may refuse;
the next eligible fresh publication must use the newly bound facts or refuse.
Even transactions thought to be unrelated/read-only take the final read: actual
mutations cannot evade tracking through a dispatch label. Exceptions after COMMIT,
no-BEGIN cancellation and FIFO work outcomes retain the imported writer semantics.

### Shipped transport, signal and elapsed composition

The production `make_drogon_transport` declaration/source are
`server/include/subathon/server/api_host_drogon.hpp` and
`server/src/api_host_drogon.cpp`. N links THESE pieces with the production host,
signal bridge and elapsed adapter below. Its probe provides finite dispatch data,
disposable paths and observation/synchronization only; it never implements an
alternative listener, signal handler, readiness/transaction bridge or shutdown.
Q may inject fake transport through the same `Transport` boundary.

Exactly one process main thread constructs/runs `QCoreApplication`; that SAME Qt
thread owns Host, dispatcher, authoritative queue, writer/SQL, current-source and
execution/publication lifetimes through destruction. A distinct owned framework
main thread configures and runs the single `drogon::app().run()` lifetime and its
main-loop accept/listen work; select exactly ONE Drogon IO thread for this baseline.
That IO loop owns established peer sockets/parsers, callbacks, handoff, Close and
force-close; any framework-main-loop accept/partial connection resources retain
their own loop ownership and are included in the network barrier. Qt never calls
socket teardown directly. IO/main-loop callbacks enqueue bounded ingress/events;
Qt never waits for a response or executes domain work on those threads. Actual
listener readiness uses corrected beginning/listener observations, not a timer.
Late readiness after stop cannot admit work. Disable Drogon's own SIGTERM handling
with the selected `disableSigtermHandling` before framework startup.

Production `StopSignalBridge` declaration/source are
`server/include/subathon/server/api_host_signal.hpp` and
`server/src/api_host_signal_linux.cpp`. Install it on the Qt main thread BEFORE
creating framework/IO threads. Its SIGINT/SIGTERM handler only records a pending
`sig_atomic_t` flag and wakes a preopened nonblocking close-on-exec pipe with
async-signal-safe `write` (preserve errno; full pipe already supplies a wake).
No Qt/Drogon/SQL, allocation, locking or `Host::request_stop()` runs in the handler.
A Qt-owner notifier drains/consumes the pending request and calls `request_stop`.
That method is thread-safe for ordinary callers, coalescing requests into at most
one pending owner notification; it is NOT declared async-signal-safe. Keep handler
state/descriptors valid through framework/IO exit; restore/block signal delivery
and quiesce handlers before closing descriptors. Repeated signals cannot reset
first-stop/drain/Close deadlines. Stop before/during startup follows the same fence.

Production `make_linux_monotonic_elapsed` declaration/source are
`server/include/subathon/server/api_host_elapsed.hpp` and
`server/src/api_host_elapsed_linux.cpp`: a read-only `native_ws::ElapsedSource`
adapter using actual `clock_gettime(CLOCK_MONOTONIC)`, exact checked normalized
sec/nsec→U64ns conversion and unavailable on syscall/malformed/range failure.
The HOST constructs and retains ONE shared source instance from that factory,
then calls `native_ws::create_profile(limits, shared_source)` itself before native
startup and transfers the owned profile/configuration to transport. Host queue,
response/drain deadlines AND native upgrade/frame/message/Close use that SAME
shared instance; options/transport cannot inject an independently built profile
or another clock. Per-owner nonregression state avoids ordering simultaneous
cross-thread samples. There is no UTC/SESSION-009 or supervisor-time fallback.
Q's counting-source seam checks shared identity and actual host/profile reads
(queue admission/entry, response handoff/drain and native deadline rechecks);
N must exercise this real factory/host/transport wiring and teardown, supplemented
by source review, never treating a fixture clock as production elapsed evidence.

Stop joins these concrete owners: revoke/drain all peer and framework-main-loop
callbacks/leases on their owning loops before the network acknowledgment; request
writer shutdown and observe every accepted completion plus connection removal on
Qt; only then destroy source/dispatcher/writer there. Quit/join framework/IO after
socket/callback cleanup and keep the Qt dispatcher alive through owner cleanup.
The final stopped barrier requires BOTH real writer and network acknowledgments
and joins. Network deadlines never certify a blocked writer body as stopped.
Bind/listen exit(1) remains process-terminal without a destructor/barrier claim.

### Acceptance-only hooks and concrete accounting

The `testing::HostAccess` class definition and synthetic sampler construction
API live ONLY in
`server/include/subathon/server/testing/api_host_observers.hpp`; the production
header contains only its forward declaration and narrow friendship, matching the
existing `testing::SqliteWriterWorkAccess` pattern. Its bounded construction hook
may select a counting synthetic raw session sampler or a shared test elapsed
source, behind ALL actual host activation/scope/source guards. Production never
constructs this testing helper or chooses a synthetic sampler. The hook cannot
set membership, generation, activation, restore status or trusted results. A real
source lifetime and actual execution/publication must precede the raw sampler.
Counting proves guard connection, never trusted UTC/SESSION-009 acceptance. The
stock host remains unimplemented even through the hook.

The same testing header may forward one pending commit/rollback ack-loss request
to the existing `SqliteWriterWorkAccess` seam while the private writer is idle;
it returns no writer/ticket and cannot assign a `SqliteWorkResult`. No hook may
bypass the private completion match. Observers contain bounded nonsecret events.

Before test AUTHORING, select these concrete accounting quantities:

| Owned bridge storage | Reservation rule / reference bound |
| --- | --- |
| Each retained input or response payload copy | Its whole allocated usable capacity C, plus ONE 128-byte metadata slot; payload length ≤ C. Reference payload capacity totals remain 8388608 input and 8388608 response. |
| Copy metadata | 128 slots per input pool and 128 per response pool, each charged128 bytes (16384 per pool). Slot exhaustion refuses even if payload bytes fit. A move transfers the existing slot/capacity without another charge. |
| Association bookkeeping | 128 slots charged512 bytes each (65536 total): inline256 bytes for the unchanged wire correlation bound plus256 metadata; per-peer cap8. Copy metadata above is separate. |
| Correlation / other variable data | One inline correlation in the association slot, borrowed by pinned execution/response records through their terminal barrier; any retained duplicate needs another charged slot. Other variable data uses the capacity/copy ledger; no uncharged string/lambda copy. |
| Refusal/control fallback | One preallocated1024-byte bridge control slot per admitted peer, at most128 peers (131072), respecting the unchanged CommonFailure bound; otherwise close without allocating a reply. Native control/output buffers retain their separate native-profile budgets. |

Reserve slots, capacity AND metadata BEFORE allocating/copying; capacity growth
requires reservation of the full replacement while the old buffer remains alive.
An allocator requiring extra usable capacity must secure that charge before
retention or refuse. Bridge reference charges total
`16777216 + 32768 + 65536 + 131072 = 17006592` bytes across these pools; native
buffers, selected installation/source storage (bounded by the existing selected
store limits), preallocated owner/event tables and framework/allocator overhead
are additional separately reviewed bounds, not hidden in the payload total.
128/256/512/1024 are engineering accounting charges with fixed bounded slot
representations, NOT exact platform allocator/RSS ceilings. Later header layout
must fit each declared metadata slot (checked on selected targets); a larger
representation requires a reviewed contract revision, not a silent undercharge.
No per-copy heap metadata/closures outside those slots. Actual buffer capacities,
slot counts, current/peak charges and one-time release are observable; source/
resource review must account fixed tables and platform overhead before fixture
approval. This proposal claims no exact process-memory ceiling.

Reduced Q copy-pool example (association/correlation slots accounted separately):
payload cap8, copy slots2, metadata256. Two capacity4 copies
reserve8+256=264; a third capacity1 copy refuses on both payload and slot limits.
A single5-byte input refuses the separate envelope4 cap. After releasing one
copy once, usage is4+128=132 and another capacity4 copy is eligible; moving it
adds0. With payload cap12 but the SAME two slots, a third copy still refuses at
264. Response pools use the identical capacity/slot arithmetic independently.

**Explicit stock stubs:** after contract approval only, new host, production
Drogon transport, signal bridge and elapsed adapter sources remain explicitly
unimplemented: construct/destruct without framework/writer/native clock;
host/transport/signal start and elapsed factory return their explicit
`NotImplemented` alternatives; submission does too; inspection stays cold with
no successful readiness/barriers; stop inert. No fake serving, SQL or publication implementation.
Tests assert required behavior, never accept `NotImplemented` as success. Q and
corrected-framework child fixtures must compile/run to explicit production-piece-
unimplemented assertion failures. N never fills a production stub with fixture
logic. A working real framework foundation is setup, not a second production host. Dependency/ABI/build/fixture watchdog failures are not red.

## 3. Independent literal examples

Planned oracles derive from imported requirements, not proposed algorithms.
`Q` = Qt Test, actual disposable writer/store, fake transport and controlled
elapsed observations. `N` = actual corrected-framework Linux children/peers.
Supplied time is simulation; N also needs real native elapsed wiring. Ordinary
positive/source cases use the actual §1 chain, with no prerequisite-producer gate.
Synthetic clock probes test guard lifetimes only, never trusted UTC/SESSION-009 acceptance. No public setter creates
membership/readiness. Each table row names required independently asserted cases.

| Mapping / home | Literal stimulus | Required result |
| --- | --- | --- |
| HL01 / Q, N ingress | Global=2; A/B outstanding, one completing; submit C | Resource refusal before copy/domain entry; count2. Release A once, then D eligible. Also reference128/129 with small bodies. |
| HL02 / Q | Global=4, per-peer=2; P owns2; submit P3 then Q1 | P3 refuses; Q1 eligible, total3. Disconnect P releases its terminal leases once, not Q's. Reference8/9 same oracle. |
| HL03 / Q | Admission0; queue budget5000ms; separate dispatch4999/5000/5001ms | 4999 may enter once if current gates pass; equality/later zero body/BEGIN/facade. No callback-driven extension. |
| HL04 / Q | Order disconnect before entry; separately after entry before handoff | First zero body/BEGIN. Second true commit/outcome retained with response revoked; no automatic replay. Owner ordering chooses one winner. |
| HL05 / Q | Retain A's facade; call after exit/while B queued, foreign-thread, recursively during producer sample | Unavailable before additional underlying reads; no cached fallback. In-scope counting probe establishes connection, not trusted UTC. Never call a dangling Execution reference. |
| HL06 / Q, N | Actual §1 version8 ordinary activation, then wrapped work reads I/E/J and commits normally; retain execution clock facade into matching completion | Real normal-startup positive, unchanged I/E/J/session facts; retained clock facade unavailable after exit, internal ticket revoked (observer only), never call a dangling Execution reference. Fresh publication with actual current live source/generation invokes nonsecret completion once. Trusted emission still needs real clock/login owners. |
| HL07 / Q | Separate wrong/missing private envelope, throw after COMMIT, lost commit acknowledgment, stop fence, invalidated publication source | Zero protected reply/bearer. First two retain committed truth; lost ack retains actual outcome_unknown/uncertain_outcome. Existing writer fault seam models ack loss, never assigning WorkResult. No rollback/replay invented. |
| HL08 / Q, N | First listener/writer open without completed activation; separately exact normal version8 inspection + configured read on same writer with normal acknowledged commits | First diagnostics only and zero protected/sample work; second real storage-ready execution/nonsecret publication eligible, preserving I/E/J/session facts. Clock/login owners remain required for credentials. No restore prerequisite. |
| HL09 / Q actual wrapper | Admit B then A at owner generation7 before either enters; B executes first while A stays queued. B invokes facade→real `replace_installation_epoch` E→F and wrapper precommit read/COMMIT; completion owner rebinds8; A contains request text I/F | Observe actual SQL/CAS and acknowledged commit, automatic rebind8; A/old facades/source borrows refuse, zero A body/BEGIN. Next fresh publication reads F or refuses. No raw ticket/public generation setter. This is live mutation/rebind evidence, not supported restore proof. |
| HL10 / Q, N | Stop before serving during startup, then three signals; attempt start again | No serving admission; one fence/deadline/cleanup; restart refuses. Handler only records/wakes. Fixture barriers synchronize events, never set host state. |
| HL11 / N | Parent holds loopback listening port; child starts there. Release port; NEW child starts | First exits1 on selected bind failure; no destructor/graceful-stop claim. New child may start; no in-process framework retry. Reap both. |
| HL12 / Q, N | Stop with idle TCP, partial HTTP/Upgrade, partial WS frame, queued command and finishing executing write | Queue cancelled; executing actual outcome retained; Close once; peers close/force-close by original network bound. Actual writer/network barriers precede stopped/destruction. Separate nonreturning-body child never proves stopped; watchdog kill is process-terminal only. |
| HL13 / Q, N | Commit then local handoff; peer disappears before receipt | One commit, at most one local handoff, no confirmed-delivery claim or automatic replay/old-digest restoration. Correlation is not retry authority. |
| HL14 / Q actual writer | Accept W1,W2,W3; W1 commits/completes, consume stop before W2 | Completions1,2,3,shutdown; W2/W3 wrappers dispatch once, zero BEGIN/body/facade, actual not_started/missing_transaction, no value. Record stop separately. Real fail-close variant retains ordered not_started/writer_unavailable. No accepted item erased; old WriteBatch semantics intact. |
| HL15 / Q, N elapsed | Start1000ms, budget5000ms; UTC jumps both ways; elapsed5999/6000/6001ms | Deadline6000ms; first eligible, equality/later refuse without reset. Host/native profile share ONE elapsed source/domain. Real N wiring does not imply privileged wall-clock-change capability. |
| HL16 / Q | Now18446744073709551615ns plus1ms; separate sec=-1, nsec=1000000000, unavailable, per-owner100→99ns | Overflow/malformed/unavailable/regressing observation refuses affected entry/handoff and tears down affected peer on IO turn. No wrap/saturation/unlimited timeout/SQL invention; executing work retains truth. |
| HL17 / Q, N | Clock unavailable; nonsecret diagnostic with valid elapsed lease; separately credential-requiring route | Diagnostic needs no session sample, but all transport/lifecycle/elapsed gates. Credential publication suppressed; no invented login or actual secret issuance. |

Additional required failure/resource/ownership families:

| Family / home | Literal boundary/event | Required observation |
| --- | --- | --- |
| Owned input / Q | Envelope4, aggregate8; own two4-byte inputs; attempt5-byte input or another1-byte copy | Refuse before copy. Exact4 eligible when aggregate permits. Another retained4-byte copy costs4; move adds no second payload. Charge capacity4+metadata128 per copy; two copies264, move adds0; see §2 before authoring. |
| Response / Q, N | Single4, aggregate8; two4-byte leases; third lease, duplicate completion, disconnect, expiry at5ms, delayed IO continuation | Bounded control refusal or close; no uncharged copy/duplicate/late handoff. Release once, current totals0 after barriers. No raw secret in observers. |
| Configuration / Q | Zero duration, incoherent counts/capacities, UINT64_MAX milliseconds | Fail startup with no framework activation; separately check duration conversion and deadline addition, never float/wrap. |
| Elapsed conversion / Q | sec18446744073 with nsec709551615, then709551616 | First exactly UINT64_MAX ns; second overflow refuses. Per-owner nonregression must not falsely order simultaneous reads by other threads. |
| Thread/source / Q, N | IO submit, Qt execute/commit/complete, IO handoff; accepted cancelled wrapper retains dependencies | Actual SQL/source mutation/clock guards on Qt owner; sockets/response teardown on IO owner. Record actual thread IDs/destructors. No network wait in state or state code in IO. Source review supplements counters. |
| Activation failures / Q, N | Fresh/old schema, unsupported future head9, wrong/malformed catalog, missing/wrong/malformed installation, known unresolved fence, writer full/fail-close/lost ack | Refuse without advance/setup/epoch rotation. Unchanged schema8 ordinary activation succeeds through actual positive §1 chain; DTO equality/request reference cannot enable it. Unknown current facts refuse; no invented out-of-band restore detector. |
| Publication invalidation / Q | Destroy/hold the installation source or change installation epoch through the actual guarded mutation before old publication | No stale installation borrow/sample/emission. The minimal surface supplies no account source: account-dependent operations refuse. Actual account-epoch mutation/invalidation is a retained downstream integration gate for the owning account source/ACCESS-005, not an authored account-recovery claim here. Its eventual guarded source must invalidate at real mutation and cannot clear independent global holds; no unobserved writer access. |
| Stop barriers / Q, N | Stop1000ms, drain5000ms, Close1000ms; traffic/repeat stop1999/5999ms | Drain stays6000ms; Close retains its own first-Close deadline. Revoke/drain actual IO callbacks before network acknowledgment/dependency destruction. Writer ack follows all accepted completions and connection removal. Keep Qt dispatcher through owner cleanup; IO join follows socket/callback cleanup. |
| Startup cancellation / Q, N | Stop before start, during inspection, late listener-ready after stop, startup callback exception | No transient admission, one cleanup, no restart. Bind/listen exit remains distinct process-terminal category. |

## 4. Harness gates and proposed next ownership

Q uses Qt Test and existing real SQLite APIs. Runner may pump events BETWEEN
callbacks, never within domain work. Explicit disposable migration/seeding BEFORE
host acquisition is fixture setup, not setup entitlement/readiness. Protected
positive fixtures exercise the actual §1 inspection/read/acknowledgment chain.

N is a NEW host child and Python unittest peer driver using corrected API-012
and linking the SHIPPED production host/Drogon/signal/elapsed pieces in §2,
not a fixture reimplementation or production reuse of the old probe. Test persistent
independent correlations, idle/partial/uncooperative peers, early signal installation, startup races,
occupied-port exit, fixed deadlines and actual teardown. Reply order beyond
correlation is not promised. Control channels only observe/synchronize events;
they cannot set readiness/generation/invocation. Assign finite pipe/log/event/
command/peer budgets, watchdogs and cleanup attempts before independent fixture
review. Fixture overflow/timeout is failure, not red or safe destruction.
Supervisor wall time never becomes the product elapsed domain.

Proposed EXACT ownership AFTER separate independent Codex verification/fixes
and closing Claude contract approval (initial review already returned findings):

- Author: `server/include/subathon/server/api_host.hpp`,
  `server/include/subathon/server/testing/api_host_observers.hpp`,
  `server/include/subathon/server/api_host_drogon.hpp`,
  `server/include/subathon/server/api_host_signal.hpp`,
  `server/include/subathon/server/api_host_elapsed.hpp`,
  `server/src/api_host.cpp`, `server/src/api_host_drogon.cpp`,
  `server/src/api_host_signal_linux.cpp`,
  `server/src/api_host_elapsed_linux.cpp` (ALL explicit unimplemented stubs ONLY),
  `tests/qt/api_host_tests.cpp`, `tests/qt/support/api_host_fixture.hpp`,
  `tests/api_host_lifecycle/api_host_probe.cpp`,
  `tests/api_host_lifecycle/test_api_host_lifecycle.py`.
- Coordinator: `server/CMakeLists.txt`, `tests/CMakeLists.txt`,
  `tests/qt/CMakeLists.txt`, new `tests/api_host_lifecycle/CMakeLists.txt`,
  `cmake/AcceptanceSuites.cmake`, `CMakePresets.json`,
  `tests/acceptance-map.json`, needed root CMake/tooling/CI/admission paths,
  `progress/state.json`, `PROGRESS.html`, `progress/coordinator-checkpoint.md`.
  Existing assertions/native profile source/header/patch/manifest stay frozen.
  Additional shared/support paths require explicit claim.

Proposed selectors/labels `api-host` and `api-host-native`, enabled with
`intentional-red` body labels while unimplemented; these do not exist yet.
Coordinator assigns exact compile/discovery/body commands and dependency closure
after reviewing authored harnesses. No such command is released/run here.
Retain passive source/ABI/build/license admission and complete actual loaded-map
gates for the NEW consumer; previous probe admission alone is insufficient.

Authored test/oracle AND stub/fixture/resource/build/CI stages need their required
independent review cycles before intended red. Selected bodies must execute on
a working foundation; missing/skipped/disabled cases, `WILL_FAIL`, blanket
continue-on-error or failed compilation do not count. Retain exact commands,
environment and explicit unimplemented-host failures separately from setup
failures. Baseline acceptance is reviewed intended red, not production success.
API-001 needs a fresh implementation session, independent code review, passing
acceptance/regression checks and normal protected merge under separate authority.

Current correction author owns ONLY this file, `docs/tasks/EPIC-008/API-001.md`
and `docs/tasks/EPIC-009/STORAGE-007.md` and stops at committed document handoff.
The future path list is a PROPOSAL, not permission to author executables now.
No state/HTML/executable/build/CI/GitHub changes. Diagnostics or simulated guards cannot close real protected-ready/session-clock acceptance.
SESSIONTEST-009/SESSION-009 retain ready/refusal/native-clock-hold gates; restore,
login/permission/disclosure, API-003 sync, TLS/Caddy ingress, deployment/platform
and full release remain separate. Preserve all seven integrations, three FX
providers, English/German, appearance modes and both renderers.
