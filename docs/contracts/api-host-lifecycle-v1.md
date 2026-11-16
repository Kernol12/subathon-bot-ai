# Authoritative API host lifecycle — proposal v1

APICONTRACT-001; identity `subathon.api-host-lifecycle.v1`. Document proposal,
not an implemented host, readiness source or clock binding. Base main
`1dcf2b61f65a4d87a29a0ac52e68d7c8b0d20c90`; claim `806dba7`.
Authority: specification v0.45 §§2/10/16/17/18, SERVER-01, API-01/02/03,
AUTH-05/12 and TEST-04/05. This is API-001's host boundary, not a second host
allocated to SESSION-009.

Import unchanged [wire primitives v1](wire-primitives-v1.md), [writer
v1](sqlite-writer-v1.md), [writer work v1](writer-transactions-v1.md),
[physical account/session v1](sqlite-account-sessions-v1.md), [session
lifecycle v1](session-lifecycle-v1.md) and [clock v1](session-clock-v1.md),
especially clock §§2/4/5. Clock policy remains unchanged. The paired [native WebSocket
profile](native-websocket-profile-v1.md) owns frame/control behavior and
native connection buffers. No new endpoint, JSON schema, permission, accepted
login, recovery record or durable timer is defined here.

[Listeners](api-host-listeners-v1.md) §§1–6 specify isolated ordinary/native
endpoints in ONE host lifetime, actual combined readiness, buffer/thread ownership,
role refusals/private scratch, serialization, observer failure and signal quiescence.
[Boundary revision2](api-host-lifecycle-boundary-v1.md) implements approved groups
A–E and owner amendments, qualifying this lifecycle contract. Bridge charges and
all rules outside the approved list remain binding. Neither contract nor header
approval establishes runtime support.

## 1. Owners, lifetime and state

One process has one Qt application/authoritative queue/controlled writer and
Drogon lifetime. Qt owns writer/private SQL construction, use and destruction.
IO owns sockets/parsers/replies and submits bounded owned commands, never state
work, SQL or waiting for its result. State never waits for network callbacks.
Independent real readers retain their own connection/thread rules.

States: cold→starting→serving→stopping→stopped, failed terminal. Stop in cold or
starting prevents serving admission. Only serving admits; it is transport service,
not protected readiness (§3). Owner-queue startup/stop transitions synchronize
admission observed by IO; IO cannot set it.

ONE start attempt/framework lifetime per process: failure/stop cannot restart;
retry needs a fresh process/owners. Pinned Trantor bind/listen exit1 is terminal,
not recoverable or destructor proof; occupied-port retry uses another child.
Recoverable in-process binding would need an explicit source/contract prerequisite.

Install signal bridge before framework threads; async handler only records/wakes,
never allocates/calls Qt/Drogon/SQL/destroys owners. Qt consumes stop. Repeated
signals are idempotent, cannot restart deadlines or admit work. External kill
during startup proves neither graceful cleanup nor response delivery.

## 2. Actual execution and asynchronous completion

An outer association owns correlation, command, cancellation and response
lease while queued/active/completing. It is bookkeeping, not a permit.
Client-supplied installation references and matching DTOs cannot grant
authority; correlations follow wire v1 and identify transport requests, not
domain retry authority.

One command is active at a time, including diagnostics. Before dispatch check stop
(including native signal), disconnect, queue deadline, elapsed and activation;
refused entry submits no unit or writer completion. `api_host.hpp` defines the
move-only CommandHandle: execute may prequeue units; forward only from execute or
supplied completion, never a running body; finish once with optional reply.
[Boundary §2](api-host-lifecycle-boundary-v1.md#2-minimal-proposed-c-consumer-boundary)
fixes the owner-approved handle rules, true writer results, drain and callback
exception behavior. No next command enters before accepted-unit/completion drain.
Writer/store APIs remain unchanged, with no host Transaction/AccountSessions,
generation, precommit reread, current-installation, private envelope or Publication.
Payload is borrowed charged host-owned input; existing intake capture resource
rules remain unchanged. No compile-time capture restriction or new producer API.

The host has no generation fence. Authenticating units use
`resolve_current_account_session`'s same-transaction epoch comparison (revoked on
mismatch); insertion/replacement uses current()/accepted() in sqlite_account_sessions.cpp.
[Boundary §1](api-host-lifecycle-boundary-v1.md#1-actual-sources-ordinary-activation-and-forward-restore-obligation)
binds those sources; SESSION-013 (#662) verifies every session-bearing unit before
first rotation. Account recovery cannot clear independent global clock holds.

The host withholds a reply whenever a unit's outcome is unknown or something
was committed without ending normally; otherwise the dispatcher may reply.

Normal committed work has no error and a value; unknown or abnormal commit
withholds, latched across later units. Every known noncommit permits reply under
this unit-outcome rule, as does no-unit work; hidden exceptions get no new result.
The [boundary's exhaustive table and separate gates](api-host-lifecycle-boundary-v1.md#host-reply-eligibility-after-prepared-units)
and `CommandHandle` comments govern callback exceptions, accepted drain, known
rollback/busy replies, retained no-handoff cases, stop/lease/elapsed and once-only.
The host never judges content or invents/relabels SQL truth, retry or replay.

The session clock is not part of this host boundary. SESSION-009 (#582) owns
both removed guards: revocation, thread and reentrancy; wall-clock jumps do
not move deadlines. The owning operation decides whether trusted publication
time is required. For bearer and session-credential emission, clock/lifecycle
v1 REQUIRE a fresh clock sample immediately before the synchronous emitter;
execution time is never reused. Secret emission remains at most once with
existing exact metadata/expiry/wipe rules. Ordinary nonsecret replies acquire
no session-UTC requirement. Local handoff is not confirmed receipt;
post-commit refusal/exception preserves actual truth.

## 3. Readiness comes from actual owning sources

Protected readiness is a CLOSED conjunction of actual owner state: serving
admission; live selected storage/catalog activation; the authoritative installation
identity/epoch bound from its real selected current store; no unresolved known
startup/restore fence; and the specific operation's current sources. Unknown,
absent, malformed, stale or unavailable evidence refuses. Listener readiness,
writer-open, a successful migration, a stored DTO and a caller boolean each fail
to establish this conjunction. Clock trust additionally requires SESSION-009's
actual Linux observations; it never follows merely from host readiness.

Ownership is explicit:

| Fact or transition | Actual producer and gate |
| --- | --- |
| Running queue/listener/owner lifetimes and live membership | API-001 host itself; native startup/stop barriers plus Qt owner execution. |
| Catalog/schema activation and actual installation selection | API-001's real startup composition using selected migration/writer/store APIs. `SqliteAccountSessionUnit::read_installation` supplies bounded facts inside an actual work session, not operational approval. Missing activation provenance refuses. |
| Ordinary startup may expose protected storage | API-001 must specify and independently test its ACTUAL normal-startup activation producer, including how unresolved known replacement is excluded. No existing probe or STORAGE-020 DTO supplies it. |
| Known restore/replacement/recovery review | STORAGE-007's stopped/staged restore and acknowledged installation-epoch fence. Before its real protocol establishes safety, restored access, accepted login and bearer publication remain held. |
| Account recovery and account/session facts | ACCESS-005's actual account fence plus the current account/session source; account recovery cannot clear installation/clock holds. |
| Current clock, accepted sign-in, policy and permissions | SESSION-009, actual local/provider authentication owners, owner-policy producer and current access owner respectively; their independent gates remain required. |

Startup activation follows [boundary
§1](api-host-lifecycle-boundary-v1.md#1-actual-sources-ordinary-activation-and-forward-restore-obligation):
inspect the head of `reference_authority_repository_manifest()` (version10
today) and read the configured installation with acknowledged normal commits
through the same private writer. A complete version8 store refuses as old,
without advance. Readiness is checked at command entry, never captured at
ingress. A request flag or plausible reread cannot clear an unknown outcome.

The normal-startup producer remains mandatory API-001 acceptance, not
capability delivered here. Reusable issuers, durable replacement markers,
recovery journals or new protocols beyond this composition require a split
prerequisite with its contract/test/implementation before code.
STORAGE-007/ACCESS-005 retain full scope. Missing actual sources refuse; a
real listener with diagnostic commands, simulated readiness or a native clock
behind a fake host cannot close clock §2 or SESSION-009's actual ready-host
binding acceptance.

## 4. Finite admission, cancellation and reply ownership

At startup the host selects finite limits and checked byte arithmetic. The
following is the concrete reference/test profile, in bytes and milliseconds:

| Limit | Reference value |
| --- | ---: |
| Admitted command associations, queued plus active plus completing | 128 |
| Associations for one transport connection | 8 |
| Simultaneous authoritative execution callbacks | 1 |
| One owned opaque command envelope | 1048576 |
| All owned command-envelope copies retained by the bridge | 8388608 |
| One response lease / all response-lease payload copies | 1048576 / 8388608 |
| Admission-to-execution queue deadline | 5000 |
| Completed response's local network handoff deadline | 5000 |
| Stop network drain / WebSocket close handshake | 5000 / 1000 |

These are configurable bridge resource limits, not accounting/reference/retained-
configuration domain limits or a new public message schema. The owning endpoint
must admit a compatible finite profile before shipping larger valid domain
values; refuse resource exhaustion explicitly instead of truncating or changing
their domain type. Invalid/overflowing/incoherent configuration fails startup.
Client input cannot raise limits. Tests use reduced literal limits to exercise
boundaries without large allocations.

All queue, response, upgrade, frame/message, Close and drain deadlines share ONE
API-001-owned elapsed-time domain across the authoritative thread and IO loops.
The selected Linux profile reads
[`CLOCK_MONOTONIC`](https://man7.org/linux/man-pages/man2/clock_gettime.2.html)
through a read-only host-owned adapter; it excludes suspended time. This is
elapsed resource scheduling, not SESSION-009's trusted UTC or a session-age source.
Wall-clock corrections cannot
move these deadlines. No request timestamp, client deadline or previous process's
anchor supplies this domain. Exact native implementation/source observations are
downstream gates; no native call is made by this document.

Represent elapsed instants/deadlines as checked unsigned64 nanoseconds in that
same domain. Native observations require nonnegative seconds and normalized
nanoseconds `0 <= nsec < 1000000000`; convert `sec*1000000000+nsec` exactly and
range-check the final value, without floating point or overflowing intermediates.
Convert a configured millisecond duration and add it to the actual start instant
without overflow; never wrap, saturate or turn failure into an
unlimited timeout. A malformed/unavailable/regressing observation or unrepresentable
deadline refuses new admission, cancels not-yet-entered domain work, revokes the
affected response lease, or tears down the affected network peer as appropriate.
Regressions are checked against that owner's prior accepted observation, without
misordering simultaneous reads from different threads. A network owner that
cannot obtain time force-closes its affected peer on that turn rather than waiting
on an unmeasurable timeout. There is no session-UTC or wall-time fallback.
Elapsed-time failure cannot fabricate a SQL outcome or preempt executing work.
Invalid duration configuration fails startup. Timer callbacks only wake their owner:
sample/recheck `now >= deadline` at execution entry, local handoff and every IO
continuation, before accepting more input/output. Equality is expired. A delayed
callback never extends the original deadline, and expiry wins over entry/handoff
when that owner checks both in the same turn. Absolute instants are not persisted
or compared across host processes/domains. An external supervisor's wall-time
termination policy remains separate from this suspend-excluding profile.

Before taking ownership, reserve association and cumulative payload/response
capacity, including every actual retained copy and its declared overhead. Check
whole lengths before copy/parse/materialization. Native frame/control buffers use
the separate native profile; ordinary HTTP uses the API-016 foundation under the
listener contract, including explicit native pre-Upgrade compatibility review.
These bridge numbers alone cap neither. No unbounded lambda capture,
pending-completion list, response callback or
unaccounted retry queue may escape the budgets. Release each reservation exactly
once on every terminal path. Fixed control/error reserves must still permit a
bounded refusal when ordinary response capacity is exhausted; otherwise close
the connection without inventing an application result.

Full/expired/stopping admission refuses before execution. On disconnect or
queue deadline, an association not yet executing is cancelled and must never
invoke its command. Race resolution occurs on the owner queue: exactly one of
cancellation or execution entry wins after the current deadline/fence check. A
waiting command has no accepted writer item: it is cancelled by the host
without execution or writer completion. Accepted units retain their
dependencies/reservations and drain with real FIFO outcomes. A fail-closed
writer supplies actual ordered `not_started/writer_unavailable`; the host
never relabels outcomes, erases accepted units or retries them. Ordinary
already-admitted writer batches keep their existing execution/drain semantics;
host cancellation cannot erase or rewrite them. Ingress reservation is not
writer acceptance.

Once actual non-yielding domain work starts, disconnect/timeouts only revoke the
response lease; they cannot preempt SQL or imply rollback.
Retain the true committed/rolled-back/uncertain outcome internally, suppress an
unavailable response, and use only the operation's reviewed retry policy. A
cancelled response is not a new operation, authenticated retry or delivery proof.

Correlated results are moved to the owning network loop through a once-only
bounded lease. No raw QObject, stack reference, writer ticket or secret borrowed
from completed work crosses to that loop. Expiry/disconnect makes a delayed
network callback inert. Persistent sockets may carry multiple independent
requests, with no promise of reply ordering beyond their correlations. The
native profile and API-003 still own persistent frame handling and authorized
snapshot/sync; a transport connection grants no current account authority.

## 5. Stop ordering and honest deadlines

The first consumed stop request closes application admission and increments
the admission fence before draining. Already admitted queued commands are
cancelled; already-accepted prepared units drain/complete in FIFO under §4.
Waiting commands never enter and produce no writer completion. Requesting
writer shutdown drains ALL accepted items and completions under its unchanged
contract. The one actual executing callback, if any, finishes under that
contract. Its completion may record truth but cannot start new work or publish
a new bearer after the stop fence. Already handed-off nonsecret responses may
drain within the remaining network deadline. New/partial HTTP requests are
refused or closed without a state command; idle/partial/uncooperative sockets
cannot extend the deadline. Persistent WS receives the native profile's
bounded server Close.

At the earlier of drain completion or the stop network deadline, force-close
remaining peers on their network loops. Do not reset the deadline on traffic,
Pong, repeated signals or another drain callback. Account for actual
outstanding network callbacks, revoke their leases, and pass the network
barrier before destroying callback dependencies. Request writer shutdown on
its owner thread, observe its actual completion and connection removal, and
destroy writer and dispatcher owners there only after their work and cleanup.
Stop/join IO loops after their socket/callback cleanup; retain the Qt
dispatcher until its owner-thread cleanup acknowledgments complete. A late
callback cannot touch a destroyed host, socket owner or SQL connection.

The finite deadlines bound APPLICATION admission and network waiting; they do
not make arbitrary synchronous SQLite/native calls preemptible or guarantee OS
scheduling. If owner work fails to return, the graceful owner barrier has not
passed. Report incomplete shutdown rather than claim `stopped`, rollback or safe
destruction; a process supervisor can enforce its separately configured hard
termination deadline. Tests of that process-terminal path run in disposable
children. A future hard in-process SQL deadline/recovery mechanism is a separate
prerequisite, not an implicit timer around this API.

## 6. Independent examples and remaining gates

Synthetic literal examples below are contract oracles, not executed tests.

| ID | Given / event | Required result |
| --- | --- | --- |
| HL01 | Admission 128 occupied; a 129th otherwise valid request arrives | Resource refusal; no extra command/copy or execution. At a reduced limit 2, request 3 has the same oracle. |
| HL02 | One connection has 8 associations, global count 8 | Its ninth refuses; a different connection can still use remaining global capacity. |
| HL03 | Queue deadline 5000; entry at 4999 versus 5000 after admission | First may execute if all current gates pass; second never enters execution. Scheduling must not silently extend the deadline. |
| HL04 | Disconnect races execution entry | Cancel wins: no invocation. Entry wins: work outcome remains actual, response lease revoked. Neither path assumes rollback from disconnect. |
| HL05 | Host clock facade removed | Both clock guards are SESSION-009 (#582) acceptance criteria; boundary §1 names them. |
| HL06 | Current-catalog activation, prepared unit commits normally | Committed/none/value completion, revoked ticket; startup order and I/E/J/session rows unchanged. One correlated nonsecret reply; downstream credential gates remain. |
| HL07 | Body throws after commit; lost acknowledgment/fail-close; unit completion throws; stop | No handoff or replay; actual committed/unknown truth retained. Body exception reaches completion as committed/work_exception/no value. No envelope/publication stage. |
| HL08 | Listener running, writer open and valid old installation DTO; operational source unknown | Diagnostic service may run; protected work and session clock sample are unavailable. This does not pass real-ready clock acceptance. |
| HL09 | Host generation/rebind cases removed | Authenticating units use resolve_current_account_session; session insertion/replacement uses current()/accepted(). No host generation fence; SESSION-013 (#662) must verify every session-bearing unit before first rotation. |
| HL10 | Stop during startup, then three more signals | Never admit serving work; one deadline/cleanup sequence. A later start call in this process refuses. |
| HL11 | Bound port unavailable in fresh child | Process-terminal bind failure; no same-process retry/destructor-success claim. A NEW child may retry after releasing the port. |
| HL12 | Stop with idle socket, partial frame, queued request and already executing write | Cancel queue, let actual write finish, close/force-close peers by fixed network deadline, then require actual owner barriers. Unreturned write prevents a graceful-stopped claim. |
| HL13 | Normal work commits; emitter locally hands off; peer disappears before receipt | Committed and local handoff only; no confirmed client delivery, no automatic operation replay. |
| HL14 | Admit W1–W3, W1 commits, stop before W2 entry | W2/W3 cancelled by host without entry/unit; one execution and writer_completed=1. W3 admission < stop < cancellation; W1 completion and stop precede writer acknowledgment. Accepted units retain true FIFO results, including fail-close. |
| HL15 | Monotonic start 1000ms and queue budget 5000ms; wall UTC jumps forward or backward; elapsed entry 5999 versus 6000ms | Original elapsed deadline stays 6000ms. First may enter if current gates pass; equality refuses. A delayed wake at 6001 does not grant another 5000ms. |
| HL16 | Valid configured duration would add beyond unsigned64 nanoseconds, or elapsed observation becomes unavailable | Refuse affected admission/entry/handoff and tear down affected peers as applicable; no wrapped/unlimited deadline or invented SQL result. Already executing work keeps its actual outcome. |
| HL17 | Ordinary nonsecret diagnostic/error reply, session-clock producer unavailable | No implicit session-UTC dependency; current transport/lifecycle and elapsed lease gates still apply. Session credential emission under the same outage remains suppressed by its mandatory fresh trusted publication sample. |

Contract/examples, independent baseline review and admitted actual intended red
precede implementation. Missing libraries, compile failures/skips are not red;
source/ABI/build/license admission, current CI, review and protected merge remain.
SESSION-009 tests actual API-001 ready AND refusal paths. Restore/login/permissions/
delivery/native/container/platform/full release remain separate; all7 integrations
and all3 FX providers remain release scope.
