# Authoritative API host lifecycle — proposal v1

APICONTRACT-001; identity `subathon.api-host-lifecycle.v1`. Document proposal,
not an implemented host, readiness source or clock binding. Base main
`1dcf2b61f65a4d87a29a0ac52e68d7c8b0d20c90`; claim `806dba7`.
Authority: specification v0.45 §§2/10/16/17/18, SERVER-01, API-01/02/03,
AUTH-05/12 and TEST-04/05. This is API-001's host boundary, not a second host
allocated to SESSION-009.

Import unchanged [wire primitives v1](wire-primitives-v1.md),
[writer v1](sqlite-writer-v1.md), [writer work v1](writer-transactions-v1.md),
[physical account/session v1](sqlite-account-sessions-v1.md),
[session lifecycle v1](session-lifecycle-v1.md) and
[clock v1](session-clock-v1.md), especially clock §§2/4/5. The corresponding
base-main blobs are `11eeb5f422a174e042143df6b476ae1d717eeacf`,
`4d280461966f3fd9fcb13ab6d054c88228ca05b8`,
`4ba20506db9a858450099b82888a22c8f6dbd761`,
`3b2b87ce4457046b762a04ca1ea99b868e52cbd3`,
`636174bc76b14fa64c362e542817cd34a0184551` and
`9b95f11fce95ca268399bcbae6eba89817665fb7`. The paired
[native WebSocket profile](native-websocket-profile-v1.md) owns frame/control
behavior and native connection buffers. No new endpoint, JSON schema, permission,
accepted login, recovery record or durable timer is defined here.

## 1. Owners, lifetime and state

One process has one Qt application, one authoritative command/state queue, one
controlled SQL writer and one Drogon application lifetime. Construct, use and
destroy the writer and its private connections on the authoritative Qt thread.
Network IO loops own sockets, parsers and response callbacks. They submit owned,
bounded command envelopes; they never execute a state command, query SQL or wait
for its completion. State work never waits for a network callback. Separate actual
readers retain their own connection/thread rules; they are not another state writer.

The host states are `cold -> starting -> serving -> stopping -> stopped`, with
`failed` terminal. A stop requested during `cold/starting` prevents subsequent
serving admission. Only `serving` admits new application work. `serving` means
the bridge can receive work; protected operational readiness is a separate
predicate in §3. Startup/stop transitions run on the owner queue; network loops
observe synchronized admission state and cannot set it.

This profile permits ONE start attempt and ONE framework lifetime per process.
Failed or stopped hosts cannot restart in that process; retry means a fresh
process with fresh owners. Pinned Trantor calls `exit(1)` on bind/listen failure;
that path is a process-terminal startup failure, not a recoverable host result or
proof that C++ destructors ran. A repeated bind attempt is tested in a separate
disposable process. Same-process recoverable binding would require an explicit
source/contract prerequisite before implementation.

The signal bridge is installed before starting framework threads. Its asynchronous
handler only records/wakes through an appropriate async-signal-safe mechanism;
it cannot allocate, invoke Qt/Drogon/SQL or destroy owners. The owner queue consumes
the stop request. Repeated signals are idempotent stop requests, do not restart
drain timers, and cannot admit new work. A process externally killed during startup
has no graceful-cleanup or response-delivery guarantee.

## 2. Actual execution and separate publication membership

An outer work association owns a correlation, command, installation-generation
capture, cancellation state and response lease while queued/active/completing.
It is bookkeeping, not a permit. Neither its address/identifier nor a client-supplied
installation reference can enter a scope. Correlations follow wire v1 and identify
transport requests only; operation identity/retry semantics remain domain-owned.

The host alone creates an execution binding when an admitted command actually
enters its non-yielding authoritative execution callback. It binds this host,
owner thread, unique live invocation, actual installation generation and current
source lifetime. Foreign-thread, reentrant, stale-generation, expired or missing
membership refuses without reading the native session clock. No public ready,
trusted, current-invocation or installation setter is permitted. Implementations
must not reconstruct membership by looking up a supplied ID or comparing DTOs.

The binding and its borrowed facades are revoked before execution callback exit,
including exception paths. No Qt event pumping, await, nested command dispatch,
network wait or retained callback extends it. A queue wait has no execution
binding. The existing `CurrentSessionClock::sample_current()` signature is
unchanged; its host-owned facade uses this actual membership, not a new timestamp
parameter or externally usable clock-service method.

Writer work retains its own smaller ticket lifetime. Its `finish()` revokes the
`SqliteWorkSession` before calling completion. Neither that ticket nor a
`SqliteWorkResult`, `QVariant` envelope, committed result or source DTO is a
publication binding. The host matches the actual once-only completion to the
outstanding association, verifies the required actual commit/work/domain envelope,
and only then may create a FRESH publication binding on its owner thread. It
revalidates current generation, actual source lifetime and §3 readiness at that
point. An earlier execution facade stays revoked.

Publication membership covers only the synchronous final gate and emitter call,
with no yielding or later command interleaving. Sources used there must be valid
current host-owned bindings at completion, not a dead repository unit or a copied
previous permit. If the host cannot establish them, suppress publication. The
clock is sampled anew immediately before the emitter, under clock/lifecycle v1;
execution time is not reused. Secret emission remains at most once and follows
the existing exact metadata/expiry/wipe contract. Local handoff is not confirmed
network receipt. A post-commit refusal/exception leaves committed truth intact.

Installation generation is an internal live-host identity, distinct from stored
installation/account epochs, correlations and request references. An actual
installation rebind revokes old queued work, facades and source borrows before a
new generation can serve. Generations never wrap or get reused; exhaustion is
terminal. Rebinding an old request reference cannot move its authority into the
new installation. Account-specific recovery preserves the independent global
clock boundaries in clock §4.

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

Actual installation facts may be retained only in bounded host-owned source storage
whose provenance, generation and invalidation are maintained by ALL actual owner
mutations. Merely retaining the output of a store read does not implement that
producer. Readiness is rechecked at execution/publication, not captured at ingress.
An operational source cannot clear its own unknown outcome by accepting a request
flag or rereading a plausible old epoch.

The normal-startup producer and same-process current-source publication binding
are mandatory API-001 acceptance gates, not capabilities delivered by this document.
If implementing them needs a reusable issuer, durable replacement marker, recovery
journal or a new operational protocol beyond the existing host composition, root
must split that prerequisite and its contract/test/implementation before code.
STORAGE-007/ACCESS-005 keep their full existing scope. Until a required actual
producer exists, protected operations return unavailable. A real listener with
diagnostic commands, simulated readiness or a native clock behind a fake host
cannot close clock §2 or SESSION-009's actual ready-host binding acceptance.

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

Before taking ownership, reserve association and cumulative payload/response
capacity, including every actual retained copy and its declared overhead. Check
whole lengths before copy/parse/materialization. Network receive/parser/output
buffers use the separate native profile; these bridge numbers alone do not cap
them. No unbounded lambda capture, pending-completion list, response callback or
unaccounted retry queue may escape the budgets. Release each reservation exactly
once on every terminal path. Fixed control/error reserves must still permit a
bounded refusal when ordinary response capacity is exhausted; otherwise close
the connection without inventing an application result.

Full/expired/stopping admission refuses before execution. On disconnect or queue
deadline, an association not yet executing is cancelled and must never invoke its
command. Race resolution occurs on the owner queue: exactly one of cancellation
or execution entry wins. Once actual non-yielding work starts, disconnect/timeouts
only revoke the response lease; they cannot preempt SQL or imply rollback.
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

The first consumed stop request closes application admission and increments the
admission fence before draining. Already admitted queued commands are cancelled;
the one actual executing callback, if any, finishes under the existing writer
contract. Its completion may record truth but cannot start new work or publish a
new bearer after the stop fence. Already handed-off nonsecret responses may drain
within the remaining network deadline. New/partial HTTP requests are refused or
closed without a state command; idle/partial/uncooperative sockets cannot extend
the deadline. Persistent WS receives the native profile's bounded server Close.

At the earlier of drain completion or the stop network deadline, force-close
remaining peers on their network loops. Do not reset the deadline on traffic,
Pong, repeated signals or another drain callback. Account for actual outstanding
network callbacks, revoke their leases, and pass the network barrier before
destroying callback dependencies. Request writer shutdown on its owner thread,
observe its actual completion and connection removal, and destroy writer/source/
clock owners there only after their work and facade revocations. Stop/join IO
loops after their socket/callback cleanup; retain the Qt dispatcher until its
owner-thread cleanup acknowledgments complete. A late callback cannot touch a
destroyed host, socket owner or SQL connection.

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
| HL05 | Retained execution clock facade used while queued, in a later callback, from a foreign thread or reentrantly | Unavailable before native observation; no last-time fallback. |
| HL06 | Real COMMIT and normal private envelope; completion receives revoked writer ticket | Ticket stays unusable. Only a freshly matched publication binding with CURRENT facts/readiness may sample/emit. |
| HL07 | Same completion has wrong envelope, body exception, uncertain commit, stop fence or unavailable publication source | No bearer/reply publication; committed/unknown truth remains exactly as observed, no synthetic rollback/retry. |
| HL08 | Listener running, writer open and valid old installation DTO; operational source unknown | Diagnostic service may run; protected work and session clock sample are unavailable. This does not pass real-ready clock acceptance. |
| HL09 | Queue work captured generation 7; actual restore fence/rebind establishes generation 8 | Old work/facades refuse, even if request says the new installation reference. No copy-and-relabel authority. |
| HL10 | Stop during startup, then three more signals | Never admit serving work; one deadline/cleanup sequence. A later start call in this process refuses. |
| HL11 | Bound port unavailable in fresh child | Process-terminal bind failure; no same-process retry/destructor-success claim. A NEW child may retry after releasing the port. |
| HL12 | Stop with idle socket, partial frame, queued request and already executing write | Cancel queue, let actual write finish, close/force-close peers by fixed network deadline, then require actual owner barriers. Unreturned write prevents a graceful-stopped claim. |
| HL13 | Normal work commits; emitter locally hands off; peer disappears before receipt | Committed and local handoff only; no confirmed client delivery, no automatic operation replay. |

Different document/example review precedes APITEST-012/API-012's real native
profile baseline/correction, then APITEST-001's independent Qt/fake AND real
framework lifecycle baseline and API-001 implementation. Missing libraries,
compile errors and skipped tests are not intended red. Selected source/ABI/build/
license admission, current CI, independent code review and normal merge rules
remain required. SESSIONTEST-009/SESSION-009 must use the actual API-001 host and
prove ready as well as refusal paths once its actual source gates exist. Restore,
accepted login, permission/disclosure, delivery, native/container/platform and
full product release verification remain separate. All seven integrations and
all three FX providers remain in release scope.
