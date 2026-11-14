# Bounded host authentication continuations — proposal v1, revision 1

APICONTRACT-002; identity `subathon.host-auth-continuations.v1`.
Authority: specification v0.45 §10, AUTH-01/08/10/11/12, SERVER-01, TEST-04/05.
Source base main `7df5d51448221b03fdee9d49e0ef7d9ce05a402e`; prepared author
head `6b8e69f41bf489c06ff477f59124e9746d7c70da` includes coordinator claims.
DOCUMENT ONLY: proposed internal operations, ownership and independent oracles.
No header, test, runtime, route, schema, producer or gate approval is supplied.

## 1. Frozen imports and observed capability

Import the COMPLETE following contracts unchanged, including their failure,
resource, native, trust and release limitations, rather than just their DTOs:

| Import | Responsibility retained |
| --- | --- |
| [Host lifecycle v1](api-host-lifecycle-v1.md) and [consumer boundary v1](api-host-lifecycle-boundary-v1.md) | API-001 owns actual startup, private writer, selected installation, execution/fresh completion scopes, finalization, one elapsed source and shutdown. APITEST-001 remains frozen. |
| [Native profile v1](native-websocket-profile-v1.md) and [native boundary v1](native-websocket-boundary-v1.md) | Separate framing/IO/callback/output ownership, capacities and deadlines; no account Upgrade/auth authority. |
| [Account ingress v1 R2](account-ingress-v1.md) and [transport v1 R2](session-transport-v1.md) | SESSION-010 actual policy producer/RequestIngressScope; SESSION-011 native capture; admitted external edge, prior browser/native guards, no raw-request replay. |
| [Local-login authority v1](local-login-authority-v1.md) | ACCESS-008 owns request-bound attempts, six-work password pipeline, actual acceptance, two upgrade checks, lineage, reservation/consume/poison and its one worker/cell. |
| [Writer v1](sqlite-writer-v1.md) and [writer work v1](writer-transactions-v1.md) | One Qt SQL writer, FIFO, synchronous real commit, revoked tickets, all actual outcomes and cumulative SQL budgets. |
| [Account store v1](sqlite-account-sessions-v1.md) | STORAGE-020 supplies bounded physical reads/full CAS; no username directory, accepted login or readiness. |
| [Password logical v1](password-hashing-v1.md) and [password API v1](password-hashing-api-v1.md) | Three synchronous primitives, exact bytes/preflight/costs, protected helper buffer, source borrows, noncooperative native lifetime; no acquired work permit. |
| [Supplier v1](session-resolution-v1.md), [lifecycle v1](session-lifecycle-v1.md), [clock v1](session-clock-v1.md) | Actual current auth/permission/UTC, lifecycle mutation/reservation/publication, epochs and restore holds; this extension creates none of them. |

Read-only source audit at the source base:

| Actual path/symbol | Observation, not an invented API |
| --- | --- |
| `server/include/subathon/server/api_host.hpp`, `server/src/api_host.cpp` | `Ingress::try_submit`, `CommandDispatcher::execute/complete`, `Execution`, `Transaction`, `Publication` are explicit unimplemented host stubs. `AccountSessions` exposes ONLY read_installation/replace_installation_epoch. No continuation, password permit or directory operation exists. |
| `server/include/subathon/server/api_host_elapsed.hpp`, `server/src/api_host_elapsed_linux.cpp` | Factory and converter remain explicit stubs. No consumer can currently obtain the proposed host elapsed observation. |
| `server/include/subathon/server/native_websocket_profile.hpp`, `server/src/native_websocket_profile.cpp` | Existing read-only `ElapsedSource`, native message leases and finite profile; no auth-continuation producer. Do not bypass host elapsed ownership by constructing another source. |
| `server/include/subathon/server/sqlite_writer.hpp`, `server/src/sqlite_writer.cpp` | `enqueue_work` rejects without completion; `finish` revokes before completion; accepted wrappers drain. Returning without BEGIN yields missing_transaction, fail-closed dispatch writer_unavailable. |
| `server/include/subathon/server/sqlite_account_sessions.hpp`, `server/src/sqlite_account_sessions.cpp` | `read_local_credential` metadata-preflights full closure; `replace_password_upgrade(proposal,new_revision,expected)` compares full credential/account/install facts. Its comment explicitly leaves still-accepted actual login to caller. It never commits. Schema8 has no username/setup/reset directory or restore journal. |
| `server/include/subathon/server/password_hashing.hpp`, `server/src/password_hashing.cpp` | Actual protected password copy/explicit lock/RAII wipe and synchronous source calls. `propose_password_upgrade` re-verifies and checks twice. Its input span remains caller-owned throughout. `admitted_memory_bytes` is a ceiling, not mutual exclusion. |
| `server/` source inventory; SESSION-010 and ACCESS-001 briefs | No actual RequestIngressScope/policy-generation, LocalLoginOwner or indexed username producer is implemented. A similarly named contract value cannot supply one. |

API-013 proposes an ADDITIVE auth-only host surface. API-001's synchronous dispatcher
never becomes a continuation callback, and no frozen assertion is relaxed. API-013
can implement guarded ports with unavailable production bindings and simulated
port tests; it must not depend on its future ACCESS-001/005/008 or SESSION-002/003
consumers. Missing consumer-specific facts refuse. Real login/setup/reset/session
acceptance remains with those owners after their own gates.

## 2. One original request, distinct bounded stages

The host privately registers one move-only `AuthRequest` from ONE actual guarded
ingress scope. It owns original host/peer/request membership, trusted compiled
purpose (sign-in/reauthentication; setup/reset need §9's follow-up), original username selection or owning workflow
selection, installation generation, immutable policy owner/snapshot/reference,
cancellation and absolute deadlines. Internal U64 request/stage/native-job counters
never wrap/reuse; exhaustion refuses. IDs and correlations are equality data AFTER
actual membership, never constructors, lookup authority or replay tokens. Nothing
is serialized, persisted or recreated by supplying the same bytes after restart.

The ORIGINAL association consumes one of host128 global /8 per-peer association
slots through waiting, crypto, stages, final response and inert cleanup. It is
not released/recreated between stages. Auth requests also count against the same
input/response/peer budgets as ordinary host commands. One association has at most
one outstanding writer stage, one source BridgeCell, one pending owner notification
and one worker-result notification; cancellation is a coalesced flag/wake, not a
new message per call. The local owner keeps its separate64-slot registry and eight
metadata-frame budget. An accepted attempt cannot keep a peer/request alive after
its lease. Termination invalidates all request-bound attempts; terminal records
stay charged until no message/reservation/native borrow can address them.

Reference extension profile (trusted configuration, positive checked limits):

| Resource | Bound / meaning |
| --- | --- |
| Live original auth requests | At most32, included in host128; per-peer included in8. Auth alone leaves96 association slots for ordinary commands; this is no latency/fairness guarantee. |
| Outstanding auth writer stages | One per request;32 globally, active+queued+completing including cancelled wrappers. Each substitutes for that original's writer slot, never an additional ordinary dispatch. Total ordinary+auth outstanding writer wrappers<=128; host writer pending_batches>=128. |
| Lifetime stage admission attempts |16 per original request, including refusals/explicit known-rollback resubmission; nonwrapping ordinals. No internal automatic retry. |
| Stage command / stage reply | Each complete semantic payload<=65536 bytes. |
| Aggregate retained stage command/reply payload copies |8388608 bytes TOGETHER, also charged against host input/response pools by direction; duplicate payloads consume duplicate reservations. |
| Added control storage | Fixed32 request records and32 stage records, each charged256 bytes in addition to inherited host overhead; at most16384 added semantic control bytes. Concrete C++/Qt allocation overhead requires later audit. |
| Password pipelines / waiting password jobs | ONE installation-wide pipeline; ZERO waiting jobs. |
| Pipeline duration | min(admission+10000ms, effective original deadline); this earlier host cancellation preserves the upstream30000ms maximum, never extends it. |
| Original auth response/request lease | min(admission+12000ms, admitted transport response deadline minus2000ms margin); equality expires. No renewal by acceptance, stages, retries or helper checks. Missing trustworthy transport budget refuses admission. |
| One stage admission-to-entry |5000ms, clipped to original request AND pipeline deadline while password pipeline is live. |
| Final response local handoff |5000ms from completed response, clipped to original request deadline and inherited stop/peer deadline. |
| Earlier auth capture concurrency |8 host-wide, one raw scope per peer; credits after request-line route binding and before method/target/field/body capture, including rejected requests until wiped/drained; peer receive credits precede every read. |

These are engineering profiles, not new public body formats or account/session
quotas. The proposed12000ms original lease may refuse before local acceptance's
own300000ms retention cutoff, which starts at acceptance. It never extends that
retention or inclusive UTC freshness. A pending response uses bounded capacity
reservation/control only, not a preallocated password/raw-body copy. Ordinary
API-001 requests keep their frozen deadlines; the new finite original lease applies
only to this extension. Downstream lifecycle dispatch after plaintext drain still
uses this ORIGINAL membership/lease, its own current checks and remaining stage
credits. No session service is needed to hold or cancel the association.

Admission projects whole values using lifecycle's B (string length, tag/bool1,
u64/i64=8, optional1+content, variant1+selected, structs sum), including hidden
fields, repeated refs and collection probes. Check products/sums, size_t/max_size,
terminators and all imported per-value limits BEFORE copying. Project actual
capacity separately from logical length; a move/shared handle adds no payload
copy but pays bounded handle/control overhead. The local owner's existing eight
frames must account for these SAME auth metadata objects, not hide a ninth bridge
clone. Host's simultaneous stage copies remain cumulatively charged even when
already counted in an owner's per-value peak. Response data separately fits host
single/aggregate response limits; secret output uses lifecycle's reviewed owner,
never generic OwnedReply. Refuse exhaustion without eviction or truncation.

The imported edge has a15s H1 write timeout. It starts after request headers,
not at backend password admission: see the pinned
[Go readRequest source](https://raw.githubusercontent.com/golang/go/6e676ab2b809d46623acb5988248d95d1eb7939c/src/net/http/server.go).
A configured timeout alone is NOT a trustworthy remaining budget. The frozen
SESSION-010 and DEPLOYMENT-007/008 contracts do NOT own or supply this producer.
The gap is explicitly allocated to planned
[SESSIONCONTRACT-007](../tasks/EPIC-007/SESSIONCONTRACT-007.md) →
[SESSIONTEST-012/SESSION-012](https://github.com/Kernol12/subathon-bot-ai/issues/606), downstream of those
existing tasks without changing their edges. That new slice owns the AUTH-route
earlier edge anchor, conservative host-elapsed conversion, transit/body/admission
delay and uncertainty for BOTH bundled and alternative edges, and the additive
binding to the actual010 scope/policy. No client header, cross-process clock
equality or guessed maximum delay supplies it. No such producer exists today:
auth remains unavailable on stock R2 until that separately reviewed slice admits
its AUTH profiles and actual producer. All six ACCESS-001/005/008 consumer baseline/
implementation nodes now additionally wait for SESSION-012; API-013's upstream
host stays independent and may test only unavailable/simulated budget bindings. Direct TLS is unavailable too. This is an
explicit future integration gate, not a claim that a new timeout was deployed.
Given a proven remaining15000ms at reservation, the12000ms local lease leaves
3000ms, exceeding the2000ms required transport margin; the10000ms pipeline leaves
2000ms within that lease for lifecycle/handoff. Shorter proven budgets clip BOTH
lease and pipeline; <=2000ms refuses. Recheck the conservative transport deadline
at every delayed IO handoff. Early actual edge closure always cancels, retains
committed truth and cannot trigger retry. These are local eligibility bounds,
never promises of timely native completion or receipt at the client.

Reserve control/error capacity at admission for one fixed refusal/terminal event;
if an IO owner cannot deliver even that within limits it closes. No unbounded
completion list, callback capture, queued proof, polling/retry backlog or new
executor is provided. Alternative trusted profiles must preserve all inequalities
and independently reviewed consumer bounds; invalid options fail before listening.

### Stage protocol

Internal proposed operations, not declarations in the current header:

Composition registers one compiled `AuthStageDispatcher` per allowed purpose with
bounded stage kinds, phase transitions and payload/result shapes. Its proposed
`execute(AuthExecution&, StageView)` runs synchronously on the actual Qt writer
owner in the fresh stage binding. Every writer-accepted stage also receives
`settle(AuthSettlementView)` once on Qt for its ACTUAL matching outcome, regardless
of cancellation, expiry, stop, policy/source invalidation or envelope failure.
Only then may `complete(AuthCompletion&, StageResultView)` run on Qt after fresh
completion checks. Views borrow matched bounded information owned by the private
host stage record for that call only; no stack/consumer reference escapes.
The consumer dispatcher is supplied by ACCESS-008 (or the later setup/reset
owner); API-013 knows the abstract port, never those implementations. Missing
registration refuses BEFORE reservation/copy/post. No client-selected callback,
lambda or generic SQL body is accepted. Test dispatchers establish simulation only.

`settle` has NO execution, SQL, clock, publication, response or stage-admission
capability. It conveys actual work error/transaction outcome, original stage and
reservation binding, and normal/abnormal/absent-envelope status from host-owned
records. Even absent/incorrect normal envelope cannot suppress settlement; an
unmatchable consumer envelope is not treated as a successful domain result. The
consumer consumes its matching mint reservation on ANY confirmed commit, poisons
unknown outcomes, and invalidates upgrade lineage on committed abnormal/lost
authority; it never guesses rollback. Normal lineage advancement requires the
separate fresh guarded completion scope in the SAME non-yielding owner turn,
before any event/state interleave, with all upstream current checks. This is not
a seventh work or another queued turn. Known-rollback release retains its own
required fresh current checks; settlement alone cannot make an acceptance usable
or release a stale reservation. A settled normal upgrade remains unusable unless
guarded complete installs its entire lineage in that SAME turn. If complete does
not run, treat that as lost authority and invalidate; no partially advanced tuple
may escape. A throwing complete revokes its completion/publication capabilities
before return (including exception unwind), invalidates incomplete lineage, and
receives the same consumer-registration-generation revocation/quarantine as a
throwing settle. Keep actual committed mint consumption and writer truth intact.
A not-started/rolled-back cancellation must still retire or quarantine its bound
consumer record, never resurrect an expired request.

Reserve the settlement record/consumer lifetime BEFORE writer acceptance; retain
it through actual completion/drain, even after deregistration. This is one direct
synchronous call on Qt, no separate unbounded callback queue. Mark its once-only
latch before invocation; duplicate results cannot invoke it twice. The consumer
must be nonthrowing. Catch any unexpected exception at the host boundary: preserve
actual writer truth, suppress positive completion, revoke the entire affected
consumer registration generation and all its outstanding request capabilities,
and quarantine unresolved private records without making them reusable. Every
consumer entry observes that host-owned revocation; missing acknowledgment cannot
reopen it. Continue mandatory writer/native/network drain; no retry of settlement,
secret exception log, early permit release or freeing a live borrow. Re-enable
requires that consumer's separately reviewed reconciliation/restart gate; it is
not an automatic same-process recovery mechanism. Writer admission refusal has
NO actual outcome/settle call; the separate fixed refusal/wake path unwinds only
unaccepted reservations. Accepted stages cannot disappear with a logical response.

The host checks request/phase/stage membership, kind/ordinal, policy/generation,
installation/readiness, elapsed/stop, exact envelope pairing and once-only use.
The registered consumer checks its own actual job/source/proposal provenance,
accepted-object/lineage, domain entitlement and current full facts synchronously
INSIDE the same transaction before any named mutation. Missing consumer authority
refuses there. The host cannot reconstruct those private objects from DTOs.
`AuthExecution::clock()` exposes the SAME revocable CurrentSessionClock binding
as ordinary Execution, sampled only in this execution; no UTC producer is added.
For accept_verified the consumer compares full facts and samples that clock,
stages a private acceptance candidate, then installs it only in matching normal
committed completion after all fresh host AND consumer checks. A result enum or
caller boolean never substitutes for this registered synchronous consumer body.

| Operation | Required behavior |
| --- | --- |
| `try_begin_auth(scope, borrowed_input)` | At §3's exact point atomically reserve original association, permit, registry credit where login needs it, complete initial payload/response/control credits and deadlines; otherwise refuse before stage/secret ownership. |
| `try_stage(request, named_kind, owned_metadata)` | Auth-specific compiled dispatch only; reserve fresh stage record/ordinal, whole command and possible reply capacity, original remaining stage credit, deadline and host writer slot BEFORE posting/copying. No client-supplied function/lambda/body. |
| `stage_complete(actual_result)` | Host privately matches the real body/completion pair and typed phase envelope; settle host truth and unconditionally call consumer settle for accepted work, then open fresh completion scope for any permitted private reply/next-stage decision. |
| `finish_auth(request, owned_result)` | Once-only final publication decision on Qt, then bounded correlated IO handoff. Duplicate/late completion cannot finish again. Finalization does not imply network receipt. |
| `cancel_auth` | Actual disconnect, expiry, stop, policy revocation or consumer cancellation terminally closes response/stage authority; settlement/drain still happens. |

Each stage carries actual host/original object, generation, ordinal, kind, job/source
identity and expected phase through private control storage. Legal login stages
are initial_read, accept_verified, upgrade_read, upgrade_check_before,
upgrade_check_after and save_upgrade (at most SIX real works). Named lifecycle
handoff uses remaining credits; setup/reset require the separately planned API-014 surface (§9);
no generic callback/SQL route is exposed. The helper's checks are separate works,
not suspended executions. Before posting, at actual entry, at every facade call
and at matching completion, check actual request/phase membership, current policy,
installation/readiness, stop/cancel state and relevant elapsed deadlines.
The5000ms queue-entry cutoff applies ONLY at entry. At facade calls and completion
check pipeline (while live), original lease and transport/stop limits; passing the
entry cutoff during a running body alone does not expire it. Pipeline/lease expiry
refuses further facade operations, without preempting SQL or inventing rollback.
Uncertainty
or missing guard refuses before private lookup/disclosure. On a live nonyielding
execution, domain/clock/mutation priority remains the imported owner contract.

No execution facade exists in a queued interval. Each admitted wrapper makes a
NEW nonreentrant owner execution binding only after entry checks; at most one
begin/transaction, and all execution/transaction/source borrows revoke on every
exit, exception, commit/rollback and before the writer's completion. Clock handles
may remain only as inert revocable handles. No Execution reference, raw ticket,
unit, live proof/view or Publication crosses a worker wait, event pump or yield.
A fresh completion scope covers one synchronous owner callback, revoked on exit;
it is not the old execution scope kept alive under another name.

Admission to the host is distinct from `SqliteWriter::enqueue_work` acceptance.
Owner ingress processing submits each reserved stage once, retaining its complete
pair through actual completion. Rejected writer admission (queue_full/not_open/
shutting_down/wrong_thread/invalid_work) has NO body/completion/result; cancel the
stage, wake a waiting cell unavailable and release only its unaccepted resources.
Never queue a hidden retry. Once writer-accepted, a cancelled/expired wrapper stays
in FIFO with dependencies; entry returns before binding/domain/BEGIN. Its actual
result is not_started/missing_transaction, or not_started/writer_unavailable if
fail-closed before invocation. Host cancellation is a SEPARATE field. Started
work is not preempted: cancellation suppresses later authority/reply, not SQL truth.

## 3. Exact pre-queue point, policy and password permit

SESSION-010 captures ONE immutable snapshot at request-start, from its real host
policy producer: installation, canonical origin, trusted route purpose, finite
body/capture/parse limits, actual native/edge artifacts, peer isolation/TLS facts
and log profile. Generation starts1, `g`+canonical U64 decimal<=21 bytes, increments
for ANY effective change; exhaustion refuses. Same generation requires the SAME
live producer/immutable snapshot, not string equality across owners/restarts.
Publish changes serialized on Qt while admission is suspended, coordinate actual
edge/native readiness and drain/close incompatible channels before reopening.
Old scopes keep coherent OLD facts; never mix/rewrite them under a new generation.

The exact pre-queue sequence is:

1. Reserve bounded peer/receive storage BEFORE native reads. After the request
   line binds an AUTH route, reserve one of the8 AUTH capture credits BEFORE
   method/target slot copy, field capture or body ownership. Earlier request-line
   bytes stay only in the already bounded receive owner. Refusal closes/wipes that
   peer after borrower drain. Other routes keep their upstream capture admission;
   this sub-cap does not redefine all account-ingress concurrency. Native supplies
   its CaptureLease; SESSION-010 constructs the actual bound RequestIngressScope.
2. While borrowing that live scope on IO, finish bounded framing/route/media/body
   scans and actual HTTPS/authority/peer/log/policy-source checks. Run the owning
   pre-session browser sign-in guard (POST, exact Origin and marker; no identity
   adopted from an old cookie) or the reviewed native/setup/reset guard. Absence
   of a required guard or unknown route/body grammar refuses. No new endpoint or
   native sign-in syntax is chosen here. Parsing uses borrowed slices/fixed counters,
   NO DOM, decoded password string or generic host command copy.
3. With those checks complete and while the SAME scope is live, validate complete
   decoded lengths/metadata/purpose against the selected reviewed profile without
   owning plaintext. Call `try_begin_auth`: short synchronized reservation of all
   §2 resources, permit and original closure/policy identity. A policy suspension,
   stop/disconnect or busy permit loses before any new stage/password payload copy.
   This point is AFTER ingress guards and BEFORE original continuation/stage
   materialization or a queued notification, not inside start/verification work.
4. Only successful reservation permits the one guarded job allocation/decode (§6).
   Recheck closure/policy on transfer. Erase earlier owned plaintext regions on
   their IO owner, revoke borrowed capture access, and acknowledge raw cleanup
   BEFORE posting the first owned job notification. Any failure unwinds/wipes;
   native/host reservations remain until their real borrowers/callbacks drain.

A lossless bounded two-pass route decoder must validate syntax/decoded length then
write directly to the guarded job allocation without an intermediate string.
That decoder/body grammar remains a separately reviewed consumer adapter, not a
claim that account ingress's reference4096 decoder implements password input.
The extension proposes a4096-byte maximum raw auth body for this profile; it is
NOT inherited from reference routes or local owner's65536 metadata ceiling.
An owning route needing a different body profile requires explicit separate
review before enablement. Unknown username/password encoding/strength policy is
not selected by these engineering limits.

`AuthRequest` retains only bounded nonsecret prepared provenance and a shared
immutable policy snapshot after raw cleanup. SESSION-010 privately establishes
that provenance from the original RequestIngressScope; copying BrowserSignInGuard,
PreparedTransportSession, a policy reference or a trusted flag cannot register it.
At EVERY stage entry, named port use, positive source reply, acceptance installation
and final publication, compare original snapshot/producer against the actual current
producer. Changed/unknown policy yields unavailable (owning HTTP mapping503) and
requires a FRESH actual client request. No replay/re-prepare from erased raw bytes,
new policy on an old continuation, or re-admission using a digest is permitted.
Disconnect/cancellation revokes the request even if its policy still matches.

The host owns one installation-wide `PasswordWorkPermit`, independently of any
login owner, accepted attempt or session. It covers login verify/reverify/hash,
reauthentication, initial-owner/invitation credential creation, recovery-code
hash generation/redemption and helper/reset credential creation. Console commands
only mint the specified setup/reset codes; they do not accept passwords or run
password primitives in a separate process. No production out-of-process bypass is
admitted; a future multi-process design needs a separately reviewed cross-process
permit before enablement. All in-process callers of the three password primitives in production composition
must acquire THIS permit; the primitives remain unchanged and do not enforce it.
No separate permit per owner/account/route/worker or use of admitted_memory_bytes
as a permit is allowed. Setup/reset need their own actual guard/selection but no
accepted login or preexisting session. No reverse task dependency follows.

Acquire atomically at step3 with zero waiting jobs. Retain across all pipeline
owner reads, worker waits, native calls, optional upgrade/save and cancellation
until worker/native return AND all referencing cell/stage settlement/secret erasure.
Do not release on response timeout or native duration timeout. There is at most
one active expensive call and three sequential expensive calls in a normal login.
A cancelled in-flight helper may already have crossed a source check and enter
its internal hash before observing cancellation; its same permit/input stay alive
through the WHOLE synchronous call. No cooperative cancellation between the
helper's instructions is assumed. A new helper invocation must not start after
cancellation. Setup/reset/recovery-code consumers require API-014's separately reviewed finite
stage/native-call counts and guarded write surface before enablement; no silently
unbounded batch of recovery-code hashes fits this single permit. This task does
not implement their entitlement or mutation protocol. An unauthenticated caller
can occupy this permit until full native drain, excluding owner login/setup/reset;
ACCESS-008 and setup/reset owners must review abuse/rate-limit controls before
public enablement. The32-association sub-cap does not solve permit starvation.

## 4. Guarded named directory/credential operations and truth

Proposed `AuthExecution` has only named `directory()` and `credentials()` facades
in its actual host-owned transaction, alongside existing guarded installation
operations. They are host-only constructed, noncopyable synchronous borrows. Every
method first checks actual owner/thread/nonreentrancy/request/stage/transaction,
current policy/generation/readiness and bound purpose. Never return a raw writer,
SqliteWorkContext/Session, SqliteAccountSessionUnit, generic query/CRUD port, or
callback receiving one. No method begins/commits independently or hashes a password.

| Proposed named operation | Exact delegation / limits / missing-producer behavior |
| --- | --- |
| `resolve_original_username()` | Only the original immutable selection held by this request; bound installation, no replacement argument. Returns one owned stable account reference plus exact directory binding and revision, known absence, or invalid_data/resource_limit/unavailable. Each selection/key/binding/revision<=4096; local account<=256, aggregate result including tag<=65536. ACCESS-001 must supply actual bounded indexed facts in the SAME active writer transaction. No scan, normalization guess, table invented here or second connection. Missing producer returns unavailable BEFORE a lookup or crypto, never absence. |
| `read_bound_local_credential()` | Use resolved original account/installation (or same privately bound selection on a later stage); forward actual `read_local_credential(installation,account)` unchanged. Return complete local/account/install facts and snapshot; preserve metadata-before-payload,256-byte account/revision and127 verifier caps, full closure and source variant bounds. Missing/invalid/unavailable/resource distinctions stay real. No attempt reference is invented by storage. |
| `replace_bound_password_upgrade(proposal,new_revision,expected)` | Forward actual `replace_password_upgrade(const PasswordReplacement&,string_view,const CompleteLocalCredentialFacts&)` ONLY within the registered consumer body after that consumer synchronously verifies private bound job/source/proposal provenance, actual accepted-login lineage and full current directory/credential/account/install/freshness. Host independently checks only its own stage/purpose/request/transaction guards. Entire tuple preflight before copies under store mutation limits and frame cap. Both helper checks are required but never authorize this save. Missing registered consumer or actual consumer-side accepted-login producer refuses without mutation; no host import of ACCESS-008 is permitted. |

Directory results do not grant authority; fresh re-resolution at each relevant
stage compares full original binding/key/account/revision. Username grammar,
normalization/uniqueness/rebinding, actual index/revision semantics and any additive
logical/physical storage extension belong to ACCESS-001 and coordinator review.
API-013 defines/guards the port and refuses a missing production binding; its tests
can supply an explicitly simulated bounded directory. This avoids a dependency
cycle through a future consumer. No extension to schema8 is implicit.

The credential read can be used for initial verification before any accepted
attempt exists. The registered consumer must establish upgrade authority from the local owner's actual still-live
request/source/accepted object, not a public DTO, password verified enum or proof.
The source's synchronous `belongs_to` check happens on worker before copying the
bounded needed metadata into its one cell; never queue PasswordVerifiedSource.
Owner rechecks actual membership and current full facts in each fresh work.
No private positive read/check/acceptance reply goes back to the worker before
actual normal committed completion AND fresh completion guards. Cancellation
or a policy change during a read suppresses its positive reply even if committed.

ALL reads and mutations follow the frozen host finalization path: one real BEGIN,
actual bounded unit access, and a final selected installation read INSIDE the
same transaction AFTER all mutations and immediately BEFORE actual COMMIT.
Budget that read in cumulative SQL limits; block further facade calls during
finalization/after terminal calls. No alternate commit escape exists. Stage the
candidate in a private host record, not a returned DTO. Execution revokes before
completion. No state event can interleave final checks and matched completion.

The host completion owns current installation truth, not ACCESS-008:

| Actual result | Source and authority settlement |
| --- | --- |
| Admission refused | No actual writer result; release unaccepted stage only. |
| not_started or known rolled_back | Retain actual error and prior committed installation. No stage-success reply. Local owner releases a mint reservation only after independently establishing SAME original acceptance/request current and fresh; never automatically retry. |
| committed, including later body exception | Bind the staged selected installation facts from this actual work, even with work_exception. Refresh revision-only facts/revoke old borrows without generation rotation; changed identity/epoch revokes old work/requests and advances nonwrapping generation before any new admission/publication. Suppress abnormal domain reply. |
| outcome_unknown, cleanup uncertainty, impossible completion or absent required staged candidate | Revoke current protected source availability and all affected source/attempt uses; actual unknown writer fail-closes/drains. A later plausible read cannot clear uncertainty. No guessed rollback, old epoch restoration or automatic writer takeover. |

A committed normal exact envelope additionally needs fresh original membership,
phase/current policy/generation/installation/readiness/elapsed/stop checks to install
acceptance, return a positive source observation or choose a next stage. A known
commit does not grant those. Duplicate/foreign/old completions cannot settle newer
objects. Retain the real once-only writer completion separately from the once-only
logical cell reply; an abandoned cell cannot be overwritten or write into a
returned stack frame. Actual writer errors/transaction outcome always outrank a
local intended refusal; diagnostics contain only fixed categories, never secret
metadata or arbitrary exception text.

Local-login lineage/reservation settlement remains EXACTLY upstream: normal known
committed upgrade advances ONLY that actual acceptance to exact new credential/
acceptance revision while preserving original accepted time/epochs/mint state;
known rollback keeps old lineage only if still current; unknown poisons; committed
abnormal invalidates rather than reconstructing a lineage. Mint commit consumes
even if publication fails; unknown poisons, known rollback may conditionally release.
Reset changes credential/account epoch and invalidates incompatible attempts and
sessions; it is not a rehash lineage handoff. Actual account source owners must
track every guarded mutation or refuse current use. Installation rebind cannot
clear independent clock/restore/account holds. Ordinary schema8 startup remains
API-001's inspect/read/acknowledgment chain, never waits for ACCESS-001 or restore.
Supported restore retains STORAGE-007's guarded-restart obligation.

## 5. Same elapsed source, completion and final response

Proposed `AuthElapsed::read()` returns one checked U64 nanosecond observation or
unavailable. The host privately gives a retainable READ-ONLY facade to the admitted
job/source/cell, valid on its owning worker and IO/Qt owners as needed. It uses
EXACTLY the API-001 shared `native_ws::ElapsedSource` instance also passed to the
native profile; no factory/domain/client time injection. Check host lifetime,
job membership/cancellation before forwarding, and per-reader-owner nonregression
without falsely ordering concurrent cross-thread reads. Unlike execution/UTC
facades this narrow scheduling facade can span a worker wait; it grants no SQL,
installation facts, session time or authority. Revocation makes it unavailable;
its inert shared control may survive to keep native cleanup safe.

Import checked sec/nsec conversion, ms multiplication, U64 addition, normalized
nonnegative observations and all equality/failure rules. No saturation/wrap,
QDeadlineTimer-derived authority, wall/UTC fallback, persisted anchor or new time
service. Each wait uses REMAINING original pipeline/request deadline; a condition
variable/timer only wakes, then re-read/recheck. At now>=any applicable deadline,
expiry wins. Failure/regression/overflow cancels not-entered work and revokes its
response; IO force-closes its affected peer on that turn when time is unknowable.
It cannot preempt started SQL/native work or create a writer outcome. The worker
may use a bounded periodic wake to observe a newly failed source, but never turn
its local wait clock into authorization or renew the lease.

On normal final completion the host consumes any pending stop first, matches the
actual final envelope and current original lease, then enters a FRESH synchronous
publication scope. All operation-specific current sources remain required. Session
bearers additionally require lifecycle's normal committed exact envelope, exact
metadata, actual accepted reservation settlement and independent current trusted
publication UTC immediately before one synchronous emitter. This extension
neither creates a bearer nor supplies trusted UTC. Nonsecret refusal needs no
session UTC but still needs current elapsed/peer/response gates. No positive
acceptance or stage result is itself a public login response.

Final completion and local handoff share one once-only latch; claiming final
publication consumes it even if emitter/transport fails. Output copy/lease
reservations and erasure are owned by the reviewed emitter/IO adapter. Recheck
elapsed/peer lease at each delayed IO continuation; deadline is
min(effective original deadline, completion+5000ms, current transport deadline minus2000ms margin, applicable stop/peer deadline).
No later queue stage or network callback renews it. Suppressed, exception or
lost response preserves committed truth; no automatic new issuance, raw-token
recovery, old digest restoration or delivery confirmation. End logical request
and invalidate its attempts after final local decision or cancellation; retain
inert charged cleanup state until barriers, not usable continuation authority.

## 6. Complete plaintext ownership audit and adapter extension

Every malformed field/body/target may contain a password. Bounds/erasure must
cover ALL earlier ingress/host owners; the two login/helper allocations do not
bound the entire process. The following is a REQUIRED proposed additive
`auth secret-transfer profile` owned by API-013 at its SESSION-010/011 adapter boundary, separately gated
before real password-bearing admission. Account-ingress R2 already mandates selected-libsodium erasure of field arena,
consumed receive, method/target/body and pending-on-close regions on every exit.
Those obligations remain; this is not a release-only ingress contract. Existing
API-001 opaque vectors/lease release do NOT prove this new typed profile. Preserve frozen upstream source/tests; add adapter-specific baselines,
never silently reinterpret release/free/move/clear as erasure.

| Owner / actual maximum live regions | Bound and transfer / every refusal, cancellation and cleanup obligation |
| --- | --- |
| External client, OS/kernel, edge Go/Caddy/TLS | Outside project auth-buffer count. Import admitted R2 finite head/body/reader/upstream/response quanta, connection/concurrency/process/log constraints. Existing edge does NOT promise Go/TLS heap erasure; no such guarantee is asserted here. DEPLOYMENT-007/008 must admit the actual selected AUTH route/body profile and no secret sinks; unknown path unavailable. Client memory, kernel buffers, compiler/register/native internal copies, swap/dumps/hibernation remain explicit platform limits. |
| Native plain backend receive: ONE owner per admitted peer |65536 payload+8prefix, prior to every read writable>=8192, no spill/growth; wipe consumed ranges before compaction/reuse, old source AND stale destination ranges when relocating, EOF/timeout/refusal/disconnect/stop/destructor wipe full owned capacity before free. Pending later request bytes stay only in this bounded owner until separately scoped; on close wipe them too. Never wipe a concurrently borrowed live slice; cancel/revoke, drain that borrow, then wipe. Native peer cap must be finite BEFORE reads (proposed auth profile<=128, with8 active raw capture credits). |
| Capture fields: ONE arena/64 records per active raw scope |32768-byte arena, method16/target2048 slots, all complete field/route bounds checked before copy. Contains arbitrary header/target secrets, not just tokens. At most8 scopes globally/1 per peer. Wipe entire mutable slots/arena including unused previously written capacity after final guard/scan borrower returns, BEFORE release/reuse and first job notification. On parse/guard/resource failure or exception wipe at scope unwind; on cancel/stop drain synchronous borrower then wipe. No maps/request-pool/full-head clone. |
| Raw AUTH body: ONE route owner per raw scope | Proposed<=4096, complete projected route request also bounded by its separately reviewed profile. Native receive may coexist while copying; both charged. Clear consumed receive body range once no borrowed parser slice remains. After direct validated decode to job erase ENTIRE body capacity before release. Refusal before permit, partial body/EOF, decode/lock failure, exception/disconnect/stop wipe all written/owned ranges and never post. No HttpRequest body clone, generic JSON DOM, spill file or logging. |
| Decoder | Two bounded borrowed scans, fixed non-owning counters/slices; ZERO owning password scratch/string. Direct decode into the one checked guarded job allocation only AFTER permit/reservations. On any partial decode failure erase that full allocation plus earlier raw owners. Any future unavoidable scratch is a separately reviewed extension with its own pre-copy reserve/count/wipe, not an unmentioned third password buffer. |
| Synchronous transport preparation | At most16 borrowed views and fixed counters, no owning header/token list. BrowserSignInGuard copies only its bounded policy reference, no password/token. If the separately reviewed route uses supplier preparation, its ONE32-byte raw token and ONE60-byte framed buffer are wiped on every return/exception; ONE32-byte digest is nonplaintext lookup metadata. The124-byte application scratch excludes native crypto scratch under its own audit. All borrowed capture views revoke after the synchronous call; neither an old account cookie nor its digest identifies the new login. |
| Generic host command/response and framework/Qt queue copies | ZERO raw head/body/password payload copies permitted on this typed path. Do not pass the auth body to existing opaque `Ingress::try_submit`, CommandView/OwnedReply vectors, QVariant/QByteArray/QString or lambdas. The new typed admission/transfer moves bounded nonsecret metadata and shared inert job handles only. Baseline generic vectors promise release, not wipe; retrofitting them would require separate explicit review/tests. Source audit must reject ANY adapter path that clones plaintext into them. |
| Job password | ONE guarded locked allocation max(password_length,1)<=1024, acquired only after permit; lock success BEFORE copying. Direct decode fills it, no string intermediate; preserve NUL/non-ASCII/space/empty length. Admission/transfer failure erases whole allocation immediately once no borrower exists. On transfer worker alone borrows; keep through last synchronous helper return. Cancel/deadline/stop makes it logically unusable but must retain until native return and source/cell drain; then selected non-elidable erase+free. No early force-free. |
| Password helper | ONE additional guarded locked allocation max(length,1)<=1024 per synchronous call, unchanged actual ProtectedPassword RAII. Entire allocation wipes on every return/exception;128-byte native record/output scratch and bounded verifier decode scratch retain upstream cleanup. No concurrent second helper. During upgrade this allocation lives through both synchronous source waits/hash; cancellation cannot free it externally. |
| Sources/stage cells/registry/results | ZERO password payloads. Bounded verifier/replacement/account metadata is sensitive: no logs/public projection. Clear project-owned mutable scratch at last use; imported string/result/driver copies do not promise complete verifier erasure. Source never retains a password/proof borrow. Abandon/wake once on refusal/cancel; real late completion drains owned metadata rather than writing to dead stack storage. |
| Secret response, if later requested by lifecycle | ZERO password copies. Lifecycle's separate fixed token owner/188-byte application scratch and reviewed emitter's bounded output copies retain their own wipe obligations on every suppression/error/emitter return/IO cleanup; no generic response clone or password echo. |

Delta ownership is singular: SESSION-011 retains its existing raw receive/capture
and R2 erasure baseline; SESSION-010 retains policy/scope construction and guarded
borrow provenance. API-013 owns ALL additional auth-only adapter changes: native
whole-capacity wiping of previously written unused ranges, partial-decode unwind,
cleanup acknowledgment and typed transfer with ZERO generic-host plaintext copies.
It also owns the route-specific two-pass decoder. These are proposals, not current
host mechanisms. APITEST-013 must review these additive interfaces and actual early
allocation/copy/erase instrumentation on working010/011 first; its existing010
prerequisite transitively requires011. No reverse dependency from010/011 is added.
If a necessary change is reusable rather than this bounded auth adapter, split it
before implementation; the current task authorizes no such expansion. If release-only ownership or implicit framework
clones cannot meet this table, keep auth admission unavailable and split the
necessary substantial native/adapter prerequisite before code. No upstream baseline
assertion or native profile capacity is removed to enable auth.

At the8-scope capture limit the proposed plain/capture payload capacities sum to
`8*(65536+8+32768+16+2048+4096)=835776` bytes plus fixed records/facts.
This is the active raw-scope boundary only: up to128 admitted peers may each hold
65544 receive bytes (8389632 total), including pending/partial rejected input;
active scope arenas/method/target/body add at most311424, giving8701056 bytes
at these native boundaries. Limits must be admitted before peer/read ownership.
Job+helper add at most2048 password bytes (two1-byte allocations for empty).
One native Argon call may use268435456 admitted memory bytes plus library scratch.
Local registry/frame metadata and host cumulative pools remain separately charged,
with overlapping payload occurrences counted in each relevant budget, not added
as disjoint RSS. None of these sums covers edge/TLS/driver/framework overhead,
unknown native memory, measured RSS or deployment RAM requirements.

Prove transfer cleanup by inspecting bytes BEFORE deallocation/reuse at actual
owner sites, including native compaction source/destination, scope/body unwinds
and job/helper RAII. Canary output absence or RSS alone proves neither bounds nor
erasure. No live credentials/captures in artifacts; use synthetic canaries and
bounded nonsecret owner/count/offset observations. For optional direct native TLS,
no fixed plaintext/BIO capacities or erasure are known: keep it disabled/unavailable
until its own native allocation/copy/lifetime manifest and tests. The required
bundled and alternative proxy gates remain; no whole-edge heap-erasure claim.

## 7. Cancellation, stop, noncooperative native work and teardown

The first consumed stop closes admission and increments the inherited stop fence;
consume pending ordinary/signal stop before any stage entry/completion/publication.
Cancel all original requests/attempts; revoke leases, abandon/wake source cells;
no new stage/helper invocation or bearer publication. Policy/installation rebind,
disconnect and deadline similarly terminally cancel affected requests. Repeated
stop/cancel/late worker result cannot renew deadlines, release a permit twice,
replace a terminal cell reply or attach to another generation.

Queued writer-accepted wrappers still dispatch/complete once in FIFO, without
BEGIN if cancelled; already executing work settles real outcomes and current-source
invalidation. Owner continues servicing their completions and cleanup notifications.
Worker-only waits wake unavailable on abandonment; queued events retain owned cell
state until consumed or explicitly cancelled with acknowledgment. Neither owner
nor IO waits for crypto; no BlockingQueuedConnection, nested event loop or Qt
owner join while a worker may still need it. Stop cannot drop an accepted wrapper
or relabel unknown/committed truth as cancellation/rollback.

Native helper calls have NO cooperative hard deadline.10s local pipeline (within
the upstream30s maximum) and5s network
drain are logical/network limits only. A still-running call retains job/helper
buffers, stable PasswordLoginSource, permit and inert request/cell dependencies.
It may return much later; suppress its result, finish erasure and drain. No unsafe
thread termination/detach, forced-free, second worker/permit or early `stopped`
claim is allowed. If it never returns, graceful shutdown is incomplete. A supervisor
may enforce its separately configured process hard-stop/kill policy; that is
process-terminal, has no C++ destructor/erasure/rollback/response guarantee, and
must be tested in disposable children. This contract does not choose a hard-stop
duration or pretend5s/10s/30s can make native execution preemptible.

Safe graceful barrier order: close admission/cancel; revoke and drain raw scopes
and network callbacks on their IO/framework owners within inherited network drain
(force-close peers as required); keep Qt/current sources/writer/cells alive while
all accepted stages settle and native/source borrows finish; erase secrets and
retire cell/job/permit; join finished worker without a live owner dependency;
request/observe writer shutdown through all accepted completions/connection
removal on Qt; destroy source/dispatcher/writer on Qt, join framework/IO only after
their callback cleanup, keep Qt dispatcher through final acknowledgments. Existing
writer shutdown may begin earlier after no future stage can be admitted, but its
ack alone is not the auth/native barrier. `stopped` requires ALL inherited and
new barriers, zero retained raw/job/helper borrows and no pending callback accessing
a destroyed owner. Late callbacks use inert owned control, never dangling pointers.
Process restart creates no requests, permits-in-use or accepted attempts from disk;
normal epochs/session times stay unchanged. Unresolved durable outcomes still need
their actual owning recovery/reconciliation gates, not in-memory resurrection.

## 8. Independent named acceptance examples

These are independently derived DOCUMENT oracles, not tests, native results or
review approval. Use synthetic request Q/peer P, actual-source-simulated policy
g1/installation I, epoch E/revision J and complete bound credential r/H0 from
password logical §6. Factories/flags cannot manufacture production provenance.
Elapsed examples are exact milliseconds for readability (multiply by1000000 for
stored ns). For these SIMULATED timing examples only, a separately admitted
transport-budget source gives Q a conservative absolute edge deadline16000.
Q admitted at1000: pipeline deadline11000, original lease=min(13000,14000)=13000;
stage admitted1000 has deadline6000. These supplied facts are not stock edge proof. All other current gates hold unless varied.
A real directory/clock/ingress producer and selected crypto success remain distinct
later gates; simulated ports prove only their named component behavior.

| ID / name | Independent stimulus | Required oracle |
| --- | --- | --- |
| HC01 Original survives wait | Initial read commits; verify off writer; source read/check works use new stages | SAME original Q occupies one host slot throughout. Six-work maximum normal password path; distinct stage identities, no live execution/ticket across native work. No bearer/login merely from enum verified. |
| HC02 Queue equality | Stage entry5999/6000/6001 |5999 eligible,6000/6001 cancelled before facade/domain/BEGIN. Accepted wrapper still actual not_started/missing_transaction; refused-before-enqueue has no result. |
| HC03 Whole pipeline equality | Helper returns10999/11000/11001; wakes at last value | Only10999 may schedule next stage if other gates pass. Equality/later refuses with original deadline; never a new10s or30s. Late native cleanup still required. |
| HC04 Original lease equality | Plaintext already drained; downstream completion12999/13000 | First potentially eligible, second expired despite still-fresh accepted UTC. Holding an attempt never extends Q. |
| HC05 Handoff equality | Password pipeline already finished/drained before11000; final downstream result at11000: completion+5000=16000, original13000, edge-margin14000; IO at12999/13000 | Effective deadline13000; first at most one handoff, equality inert/wipe output. No confirmed network receipt. |
| HC06 Elapsed failure | sec=-1, nsec=1000000000, read unavailable, same-reader100→99ns, or U64_MAX+1ms | No wrap/saturation/UTC fallback. Refuse admission/next stage, cancel queued work; IO unknowable-time peer closes. Executing SQL/native outcome remains real. Cross-thread earlier read alone is not regression. |
| HC07 Stage full | Reduced stage cap2 occupied incl completing; third attempts; later release one | Refuse third BEFORE copy/post. No retry on release; only new explicit live internal stage may admit. Exact cap/input65536 fits,65537 refuses before copy. |
| HC08 Writer full/closed | Host reservation exists, actual enqueue returns queue_full/not_open/shutting_down | No body/completion or fabricated WorkResult; cell unavailable, resources unwind, no hidden queue/replay. |
| HC09 Cumulative copies | Reduced shared stage pool8, one command4 plus reply4; attempt another1 or duplicate4 | Refuse before additional ownership. Move4 adds0 payload; real extra copy4 pays4 and refuses here. Terminal-but-referenced payload remains charged. |
| HC10 Stage credit exhaustion | Q has16 stage attempts incl one refused retry; attempt17 | Refuse without post/ID reuse; original response lease not renewed. Normal six-work login leaves bounded credits for later handoff. |
| HC11 Policy changed before entry | Capture/guardg1, publishg2 before initial or later source stage | Unavailable/owning503 before directory/credential lookup/private disclosure; fresh real client request needed. No rebind of old guard/digest. |
| HC12 Policy or request revoked during read | Readonly work commits, then completion observes cancelled peer, pending stop or unavailable policy | Keep committed truth; NO positive worker reply or installed acceptance/next stage. Fresh completion guard required. |
| HC13 Guard provenance | Copied BrowserSignInGuard/g1, wrong owner, expired/reentrant scope, bad native identity or missing policy producer | Cannot register Q or copy password to job; unavailable, raw owners wiped/closed. Canonical bytes do not create membership. |
| HC14 Credential race | Reset changes r/H0→z/H1 while verify/hash runs; separately same r with different bytes, directory a→b, account access/epoch/revision change | Fresh current full comparisons refuse stale local use; no adoption of b/new verifier, no overwrite/reset undo. Existing permissions/owner preserved by recovery owner. |
| HC15 Installation rebind | Real guarded transaction E→F commits; older Q stage queued, old source handle retained | Host binds F, advances generation, revokes Q/old source before new entry. Revision-only J→K refreshes facts/revokes old borrows without generation rotation. Neither clears clock/restore holds. |
| HC16 Missing directory/auth owner | No production directory; separately save with no actual accepted-login owner but matching proposal DTO | Unavailable before lookup/crypto or mutation respectively. Fake directory flag and raw replacement cannot grant authority. |
| HC17 Scope misuse | Retain revocable clock handle after stage; call foreign/reentrant; try facade from worker | Unavailable before underlying UTC/SQL; never call a dangling C++ reference. New stage requires new actual binding/transaction. AuthElapsed alone grants scheduling reads only. |
| HC18 Disconnect before dispatch | Writer already accepted stage; peer disconnect then wrapper enters | Zero BEGIN/body/facade; real missing_transaction (writer_unavailable if fail-closed first), separate disconnected reason. Accepted wrapper/dependencies not erased. |
| HC19 Disconnect during SQL | Entry wins, then cancel before completion | Actual commit/rollback retained; no positive reply/next stage/final secret. No preemption or automatic retry. |
| HC20 Disconnect during crypto | Native call pending; peer closes at2000; native returns60000 | Logical Q cancelled, no acceptance/next call. Permit, source and buffers remain until real return/cell drain then wipe. ZERO30s erasure/shutdown claim. |
| HC21 Stop between helper checks | First source check returned accepted; stop then helper's internal hash starts before second check | Same single permit protects still-running call. Second check unavailable; output discarded. No assumption that cancellation interrupts library/instruction sequence. |
| HC22 Stop during stages | Stop before initial post, queued work, active SQL, worker source wait, native call, or final gate; repeat stop | No new stages/helper entry/bearer. Pre-post has no result; queued/active retain actual truth; wait wakes unavailable; in-flight native retained. One stop fence/drain deadline. |
| HC23 Late/duplicate/foreign completion | Old ordinal/cell completes after cancel or after another Q phase; duplicate worker event | Actual writer truth settled once for its OWN record; no terminal reply overwrite, new acceptance, slot reuse ABA, stack write or second response. |
| HC24 Commit uncertainty | Physical upgrade COMMIT then lost ACK; separately failed rollback cleanup | Real outcome_unknown/uncertain_outcome, writer fail-close; revoke current installation source, poison acceptance, no save/mint success or guessed rollback/retry. Already accepted later wrappers fail in order. |
| HC25 Committed abnormal | Upgrade COMMIT confirmed then body throws, wrong/missing private envelope or completion scope lost | Preserve committed verifier and bind actual staged installation; invalidate local lineage, suppress positive reply. Never recover lineage from intent or restore old verifier. Mint commit similarly consumes despite failed output. |
| HC26 Known rollback vs normal upgrade | Matching ready proposal: known rollback; separately acknowledged normal exact commit | Rollback no upgraded claim, old lineage only if still current. Normal commit advances ONLY bound acceptance's exact lineage/revision, original time/epochs/mint state preserved. Other old-source acceptances stale. |
| HC27 Permit excludes all consumers | Login holds permit while waiting source; setup/invitation/helper reset, reauthentication, recovery-code generation/redemption or second login attempt | ALL refuse busy before new job password copy/post, even for different accounts/owners and no existing session. After full drain one may freshly admit. No per-owner extra permit or lowered cost. |
| HC28 Permit at late return | Deadline expired but native still running; try setup; then return and cell drain/wipe | Setup refuses until FULL release, not merely network close. Release exactly once, no waiting password queue. Native/helper Argon memory never overlaps another pipeline. |
| HC29 Teardown drain | Cancel source wait with one accepted wrapper and one late worker event; network drains before native finishes | Cell wake once unavailable; wrapper completion retained FIFO; no join blocking Qt. Stopped/destruction only after all owner/native/writer/network acknowledgments; counts0 then. |
| HC30 Nonreturning crypto / hard kill | Native never returns beyond logical/network bounds; external supervisor later kills disposable child | Network can close, graceful barrier remains incomplete. No safe forced-free/zeroization/rollback/stopped claim. Process terminal exit is separate evidence, no resurrected attempts on restart. |
| HC31 Before-permit refusal secrets | Synthetic canaries in malformed header/target/body; guard/busy/policy/full refusal | Zero job/helper copy and post. Native receive/capture/body whole-capacity wipe at real owner unwind incl pending-on-close; no generic map/log/host clone. |
| HC32 Decode/transfer failures | Exact password1024 vs1025; guarded allocation/lock failure, partial decode exception, disconnect before transfer acknowledgment |1024 only size-eligible,1025 refuses before job copy/native. Failure wipes partial job and all earlier raw regions; no fallback, post or leaked permit/credit. Empty uses valid length0/one-byte job allocation. |
| HC33 Successful secret transfer | Body and receive contain password61 00 62; decode to job then initial stage posts | All three bytes preserved; body/consumed receive/field+target owners erased BEFORE notification. One shared job handle, zero Qt plaintext copies. Later helper allocation also3 bytes; job/helper both wipe full allocations. |
| HC34 Every helper exit | Native unsuccessful, preflight/cost/runtime/protection refusal, upgrade hash failure/second-check stale/exception, normal success | Job erased after last use/drain; helper RAII wipes any actual allocation and native record/output scratch on every exit. Native−1 is not proof of wrong password; no plaintext diagnostic/output. |
| HC35 Raw cleanup during early transport | Partial body EOF, timeout, receive compaction, parser exception, pool destruction, stop with later pipelined bytes | Before reuse/free inspect complete owned ranges zero incl moved-from/stale regions; close wipes future bytes. No borrowing after scope revocation, no dropped charged capture. Never claim whole TLS/edge heap erasure. |
| HC36 Secret response cleanup | Lifecycle later commits candidate; publication UTC unavailable, emitter throws, IO lease expires/disconnects, or normal emit returns | Each candidate/output owner wipes on its real last-use path, at most one emit. Committed truth remains, no retry/raw-token recovery/password echo. Separate emitter/body/wire gate still required. |
| HC37 Capture arithmetic |8 active raw scopes,128 receive peers; ninth scope or129th peer; whole frame/result overage | Ninth AUTH scope refuses after request-line binding BEFORE capture copies;129th peer refuses before receive/read. Other routes keep their own admission. Literal active835776/native8701056 sums hold at named boundaries plus fixed records, NOT process RSS or edge erasure. |
| HC38 Entry versus running deadline | Stage admitted1000 enters5999, calls facade6001 with pipeline11000 and lease13000 still live; separately call at11000 while pipeline live | First allowed despite passed queue cutoff. Second refuses further operations; actual SQL rollback/commit result still settles, no forced preemption. |
| HC39 Early edge closure | Q has the simulated16000 edge bound, but peer actually closes at7000 while pipeline cutoff11000; earlier upgrade or mint committed | Cancel Q at actual disconnect; suppress results and retain permit until drain. Keep real committed truth; no retry, new issuance or delivery claim. Unknown budget at admission instead refuses before job copy. |
| HC40 Transport scratch and reauthentication | Supplier preparation on a separately admitted reauth route returns, refuses or throws with32-byte raw token/60-byte framed scratch; existing session cookie also present on sign-in | Inspect32/60 buffers wiped before unwind/reuse on every exit. Reauth's registered consumer requires its own actual current-session guard plus fresh password; sign-in runs pre-session guard and never adopts the cookie's identity. Neither path exposes plaintext to a generic command. |
| HC41 Compiled consumer ownership | No dispatcher; foreign phase; dispatcher without actual accepted owner; accept_verified with unavailable clock | No registration refuses before job ownership/post. Host phase rejects before execute. Consumer fails lineage/UTC inside its fresh execution without mutation/acceptance; no DTO creates authority, no reverse host dependency. |
| HC42 Reserved ordinary capacity |32 auth originals still referenced during drain; attempt33 and one ordinary request with other pools free | Auth33 refuses before materialization; ordinary request can reserve one of remaining96 slots under its own gates. Busy password permit still excludes new password work independently. |
| HC43 Settlement after disconnect/stop | Mint physically commits; peer disconnects or stop wins before fresh completion; separately upgrade commits with abnormal/missing envelope | Unconditional matched settle still consumes mint or invalidates upgrade lineage. No guarded complete/positive reply/publication. Actual truth and charged drain records remain. |
| HC44 Unknown and callback exception | Actual outcome_unknown after cancellation; separately settle unexpectedly throws after latch claimed | First still settles poison without response authority. Exception preserves actual outcome, revokes consumer registration generation/all request uses and quarantines unresolved records; no retry/false release/new authority, drain continues. |
| HC45 Envelope absent and duplicate | Accepted work commits but normal consumer envelope absent; later duplicate completion after deregistration | Original stage's bound settlement is called once with actual commit and absent-envelope status; mint consumed/upgrade invalidated, no success reconstructed. Retained registration lifetime covers call/drain; duplicate cannot settle another record. |

## 9. Review and downstream gates

The complete proposal, its compatibility/source audit, arithmetic, phase/ownership
and HC01–45 require independent Claude initial review, Codex correction/separate
Codex verification and independent Claude closing review on the exact resulting
revision. The author records no approvals. Coordinator owns revision receipts,
shared metadata/HTML/index/evidence, current CI and normal protected merge.

APITEST-013 follows ONLY after that review cycle. It needs its own independently
reviewed minimal additive API/stubs, finite harness and examples/tests on working
API-001/SESSION-010/writer/native foundations; intended missing-behavior red must
be distinct from dependency/compile/protection failure or a skipped test. Verify
actual early allocation/copy/erase paths, thread/source/once-only/outcome/drain
boundaries, permit exclusion and unchanged host/ingress/writer/native assertions.
Fake directory/login/clock adapters must be labeled simulation; real producers
remain their own acceptance gates. API-013 then requires fresh implementation,
independent code/secret/resource/thread review, relevant passing regressions and
selected CI. No body is run or runtime capability established by this task.

Setup/reset writes are deliberately a separate planned prerequisite:
[APICONTRACT-003](../tasks/EPIC-008/APICONTRACT-003.md) →
[APITEST-014/API-014](https://github.com/Kernol12/subathon-bot-ai/issues/603). That host extension must define
named guarded account insertion/replacement/local-credential replacement (forward
STORAGE-020 unchanged), bounded conditional directory-binding writes and the
workflow-specific recovery/code ports needed for atomic setup/reset, plus explicit
finite stage/native-call profiles. ACCESS-001 owns indexed directory facts and
its separately reviewed logical/physical producer, ACCESS-005 owns recovery policy.
The host depends only on compiled abstract ports, missing registrations refuse;
no dependency on ACCESS-001/005 implementations. API-013 alone enables neither
setup nor reset. Their four baseline/implementation records explicitly depend on
API-014 in addition to API-013; new nodes remain planned/unassigned. This split
preserves full setup/recovery scope without authorizing another executable task.

Open integration boundaries: actual API-001 host and SESSION-010 policy/scope;
separately reviewed API-013 auth erasure/typed transfer/decoder adapter on010/011;
SESSION-012 actual AUTH transport-budget producer and both edge profiles; route grammar/native guard; ACCESS-001 indexed directory and its logical/
physical extension; ACCESS-008 accepted-source; setup/reset entitlement/mutation;
SESSION-009 trusted UTC; SESSION-002/003 lifecycle/egress; restore/reconciliation;
edge/direct TLS/platform/native overhead and supported-hardware cost evidence.
This proposal's raw capture profile is account H1 only. Existing native WebSocket
message leases do not become password inputs: authenticated Upgrade, message-time
authority and any password-bearing WS copy/erase profile need their own review.
Missing pieces refuse, never become assumed existing APIs. No reverse dependency,
actual session-clock binding, API-001 implementation, provider call or next task
is authorized. All seven integrations, three FX providers and full release scope
remain required.
