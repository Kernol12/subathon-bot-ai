# API host consumer boundary and baseline examples — proposal v1

APITEST-001; proposed identity `subathon.api-host-lifecycle-boundary.v1`.
Authority: specification v0.45 §§2/10/16/17/18, SERVER-01, DB-01,
API-01/02/03, AUTH-05/12, OPS-02/04 and TEST-04/05.
Base `654ac016268c6caa9efc0241872ca0b0f88aae60`. Authored document awaiting
independent contract review; no headers, tests, observed red or implemented host.

Import [host lifecycle v1](api-host-lifecycle-v1.md), including ALL HL01–17,
[native profile v1](native-websocket-profile-v1.md),
[wire primitives v1](wire-primitives-v1.md), [writer v1](sqlite-writer-v1.md),
[work v1](writer-transactions-v1.md), [catalog v1](migration-catalog-v1.md),
[physical account/session v1](sqlite-account-sessions-v1.md),
[supplier v1](session-resolution-v1.md), [lifecycle v1](session-lifecycle-v1.md)
and [clock v1](session-clock-v1.md) unchanged. API-001 retains actual host
composition, startup/current-source production and downstream clock holds.
APITEST-001 supplies only their independently reviewed executable baseline.

## 1. Actual sources and prerequisite finding

Read at the base above; source observations are not executed results.

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

**Ordinary host composition:** API-001 can own selected writer/catalog lifetime,
bounded current installation storage and nonreentrant membership/invalidation;
no reusable capability issuer is necessary. Startup must inspect the selected
current schema, read the configured installation through that same actual writer,
acknowledge normal completion and bind facts to its live owner/generation.
Missing/mismatched installation refuses; no setup, auto-advance or epoch rotation.
Execution uses actual fresh selected sources inside work. Completion may borrow
only live host-owned current storage maintained by EVERY admitted mutation, not
a dead `SqliteAccountSessionUnit` or revoked ticket. Uncertain mutations revoke
that storage. No external raw-writer access permits untracked changes. A later
adapter must maintain this invariant or invalidate/refuse publication. These
lifetime mechanisms belong to API-001, not another issuer task.

**Genuine unresolved prerequisite:** these mechanics cannot exclude an unresolved
KNOWN stopped-server replacement across process loss. Two literal histories can
have identical valid version8 rows `installation=I, epoch=E, revision=J`:
(A) ordinary unchanged restart; (B) the owning restore workflow replaced data
with that snapshot, then crashed before replacing/acknowledging E. Inspection
and `read_installation("I")` return identical observations. Process-local holds
vanish. Rereading E, a request reference or a new clock monitor cannot distinguish
A/B. Host §3 and clock §4 require B held across restart. This differs from
undetectable out-of-band administrator/VM rollback under clock §1's trust limit.

No supported restore producer exists yet: this is missing evidence for the FULL
acceptance obligation, not a claim that a restore occurred or a STORAGE-020 bug.
Calling every open store ordinary silently assumes that obligation away; a public
`normal_startup=true` does the same. A real source owner cannot clear unknown
recovery by accepting a request flag or rereading a plausible epoch.

**Stop before headers/test authoring.** Smallest proposed split, for coordinator
allocation under STORAGE-007 ownership: contract/examples for a durable
startup/replacement handoff, then its independently reviewed baseline and
implementation before API-001 protected-ready acceptance. Define ordinary-store
recognition; intent durable BEFORE destructive replacement; identity binding
outside the rollbackable restored snapshot; crash/reopen classification; and
acknowledgment of the actual committed epoch fence without clearing independent
Recovery review/outbound/clock holds. Unknown/missing/corrupt evidence refuses.
No marker location, schema, filesystem atomicity or recovery algorithm is chosen
here. This needs neither a reusable membership issuer nor full backup decryption;
STORAGE-007 retains its complete staged restore scope. No task IDs are allocated
by this author. Resolve this finding through contract review/decomposition before
releasing executable ownership. Permanently unavailable protected operations
cannot replace the required positive normal-startup gate.

## 2. Minimal proposed C++ consumer boundary

Signature/ownership proposal for later headers, conditional on §1 resolution;
not a compilable stub. Namespace `subathon::server::api_host`, C++20/Qt and owned
PIMPLs. Supporting types are bounded as described below.

```cpp
class Host; class Ingress; class Execution; class Publication; class PeerLease;
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
    virtual void start(Ingress&, StartupObserver) = 0;
    virtual void handoff(ResponseLease) = 0;
    virtual void stop(StopDeadline, NetworkBarrierObserver) = 0;
};
class Host { // owner-thread construction/use/destruction, except stop ingress
public:
    Host(Options, std::unique_ptr<Transport>,
         std::unique_ptr<CommandDispatcher>,
         std::shared_ptr<const native_ws::ElapsedSource>);
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
    SqliteWorkContext writer_context() const;
    std::shared_ptr<CurrentSessionClock> clock() const;
    CurrentInstallationView current_installation() const;
};
class Publication { // fresh host-only constructor, no writer accessor
public:
    std::shared_ptr<CurrentSessionClock> clock() const;
    CurrentInstallationView current_installation() const;
};
```

- `Options` owns selected store path/installation selector, existing writer/store
  limits and host/native profiles. Selection is DATA, never ready/trusted/current
  invocation/generation/restore-cleared input. The prerequisite's actual consumer
  stays internal, not an arbitrary injectable ready-result port. Host owns its
  writer, not a borrowed writer admitting unrelated source mutations.
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

For Q's nonvacuous clock-guard checks, propose an acceptance-only
`testing::HostAccess` construction hook for a counting synthetic raw sampler,
behind the SAME actual host scope/source guards. It cannot set membership,
generation, activation, restore status or a trusted result. A valid real source
lifetime and actual execution/publication entry must precede any sampler call.
This is simulated downstream clock behavior, never a production clock choice or
trusted UTC evidence. The stock host remains unimplemented even with the hook.
No test may assign writer results or bypass the private completion match; the
existing writer fault seam is sufficient for actual lost-ack paths.

**Explicit stock stub:** after contract approval only, new host source remains
unimplemented: construct/destruct without framework/writer/native clock;
start/submission return `NotImplemented`; inspection stays cold with no successful
readiness/barriers; stop inert. No fake serving, SQL or publication implementation.
Tests assert required behavior, never accept `NotImplemented` as success. Q and
corrected-framework child fixtures must compile/run to explicit host-unimplemented
assertion failures. A working real framework foundation is setup, not a second
production host. Dependency/ABI/build/fixture watchdog failures are not red.

## 3. Independent literal examples

Planned oracles derive from imported requirements, not proposed algorithms.
`Q` = Qt Test, actual disposable writer/store, fake transport and controlled
elapsed observations. `N` = actual corrected-framework Linux children/peers.
Supplied time is simulation; N also needs real native elapsed wiring. Protected
positive/source cases remain gated by §1. Synthetic clock probes test guard
lifetimes only, never trusted UTC/SESSION-009 acceptance. No public setter creates
membership/readiness. Each table row names required independently asserted cases.

| Mapping / home | Literal stimulus | Required result |
| --- | --- | --- |
| HL01 / Q, N ingress | Global=2; A/B outstanding, one completing; submit C | Resource refusal before copy/domain entry; count2. Release A once, then D eligible. Also reference128/129 with small bodies. |
| HL02 / Q | Global=4, per-peer=2; P owns2; submit P3 then Q1 | P3 refuses; Q1 eligible, total3. Disconnect P releases its terminal leases once, not Q's. Reference8/9 same oracle. |
| HL03 / Q | Admission0; queue budget5000ms; separate dispatch4999/5000/5001ms | 4999 may enter once if current gates pass; equality/later zero body/BEGIN/facade. No callback-driven extension. |
| HL04 / Q | Order disconnect before entry; separately after entry before handoff | First zero body/BEGIN. Second true commit/outcome retained with response revoked; no automatic replay. Owner ordering chooses one winner. |
| HL05 / Q | Retain A's facade; call after exit/while B queued, foreign-thread, recursively during producer sample | Unavailable before additional underlying reads; no cached fallback. In-scope counting probe establishes connection, not trusted UTC. Never call a dangling Execution reference. |
| HL06 / Q | Work reads I/E/J, commits normally; retain ticket/execution facade into matching completion | Ticket stale, execution facade unavailable. Only fresh publication with actual current live source/generation can invoke nonsecret completion once. Trusted emission still needs real clock/login owners. Required normal-startup positive is not replaced by permanent refusal. |
| HL07 / Q | Separate wrong/missing private envelope, throw after COMMIT, lost commit acknowledgment, stop fence, invalidated publication source | Zero protected reply/bearer. First two retain committed truth; lost ack retains actual outcome_unknown/uncertain_outcome. Existing writer fault seam models ack loss, never assigning WorkResult. No rollback/replay invented. |
| HL08 / Q, N | Listener/writer open, stored I/E/J valid, operational source unknown | Diagnostics may serve; protected work/sample unavailable before underlying clock read. Separate actual normal-startup positive must use §1 producer. |
| HL09 / Q plus prerequisite restore integration | A queued at generation7; real owning fence E→F/rebind8; A now contains request text I/F | A and old facades/source borrows still refuse. Fresh-generation work requires actual activation; no public generation setter. Simulated guards are not restored-startup proof. |
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
| Owned input / Q | Envelope4, aggregate8; own two4-byte inputs; attempt5-byte input or another1-byte copy | Refuse before copy. Exact4 eligible when aggregate permits. Another retained4-byte copy costs4; move adds no second payload. Declare/account actual overhead separately before executable review. |
| Response / Q, N | Single4, aggregate8; two4-byte leases; third lease, duplicate completion, disconnect, expiry at5ms, delayed IO continuation | Bounded control refusal or close; no uncharged copy/duplicate/late handoff. Release once, current totals0 after barriers. No raw secret in observers. |
| Configuration / Q | Zero duration, incoherent counts/capacities, UINT64_MAX milliseconds | Fail startup with no framework activation; separately check duration conversion and deadline addition, never float/wrap. |
| Elapsed conversion / Q | sec18446744073 with nsec709551615, then709551616 | First exactly UINT64_MAX ns; second overflow refuses. Per-owner nonregression must not falsely order simultaneous reads by other threads. |
| Thread/source / Q, N | IO submit, Qt execute/commit/complete, IO handoff; accepted cancelled wrapper retains dependencies | Actual SQL/source mutation/clock guards on Qt owner; sockets/response teardown on IO owner. Record actual thread IDs/destructors. No network wait in state or state code in IO. Source review supplements counters. |
| Activation failures / Q | Fresh/old/wrong catalog, missing/wrong installation, unknown replacement evidence, writer full/fail-close | No implicit migration/setup/readiness. Ordinary activation preserves epoch/revision/session times. Actual positive chain required; DTO equality/request reference cannot enable it. |
| Publication invalidation / Q | Destroy/hold source or change installation/account epoch through real owner mutation before old publication | No stale borrow/sample/emission. Account recovery cannot clear global holds. Every future mutation updates/revokes bounded source storage; no unobserved writer access. |
| Stop barriers / Q, N | Stop1000ms, drain5000ms, Close1000ms; traffic/repeat stop1999/5999ms | Drain stays6000ms; Close retains its own first-Close deadline. Revoke/drain actual IO callbacks before network acknowledgment/dependency destruction. Writer ack follows all accepted completions and connection removal. Keep Qt dispatcher through owner cleanup; IO join follows socket/callback cleanup. |
| Startup cancellation / Q, N | Stop before start, during inspection, late listener-ready after stop, startup callback exception | No transient admission, one cleanup, no restart. Bind/listen exit remains distinct process-terminal category. |

## 4. Harness gates and proposed next ownership

Q uses Qt Test and existing real SQLite APIs. Runner may pump events BETWEEN
callbacks, never within domain work. Explicit disposable migration/seeding BEFORE
host acquisition is fixture setup, not setup entitlement/readiness. Protected
positive fixtures must also exercise the actual prerequisite producer.

N is a NEW host child and Python unittest peer driver using corrected API-012,
not production reuse of the old probe. Test persistent independent correlations,
idle/partial/uncooperative peers, early signal installation, startup races,
occupied-port exit, fixed deadlines and actual teardown. Reply order beyond
correlation is not promised. Control channels only observe/synchronize events;
they cannot set readiness/generation/invocation. Assign finite pipe/log/event/
command/peer budgets, watchdogs and cleanup attempts before independent fixture
review. Fixture overflow/timeout is failure, not red or safe destruction.
Supervisor wall time never becomes the product elapsed domain.

Proposed EXACT ownership AFTER §1 resolution and independent contract initial
Claude → separate Codex verification/fixes → closing Claude approval:

- Author: `server/include/subathon/server/api_host.hpp`,
  `server/include/subathon/server/testing/api_host_observers.hpp`,
  `server/src/api_host.cpp` (explicit stock stub ONLY),
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

Current author owns ONLY this file and `docs/tasks/EPIC-008/API-001.md` and stops
at document handoff. No state/HTML/executable/build/CI/GitHub changes. Diagnostics
or simulated guards cannot close real protected-ready/session-clock acceptance.
SESSIONTEST-009/SESSION-009 retain ready/refusal/native-clock-hold gates; restore,
login/permission/disclosure, API-003 sync, TLS/Caddy ingress, deployment/platform
and full release remain separate. Preserve all seven integrations, three FX
providers, English/German, appearance modes and both renderers.
