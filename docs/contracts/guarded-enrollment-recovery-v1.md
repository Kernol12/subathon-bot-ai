# Guarded enrollment and recovery host — proposal v1, revision 1

APICONTRACT-003; identity `subathon.guarded-enrollment-recovery.v1`.
Authority: specification v0.45 §10, AUTH-01/02/08/10, SERVER-01, TEST-04/05.
Source base main `55913bc49f67753acdffa642b521db4ff6e5994c`; claim `2b58197`.
DOCUMENT ONLY: additive internal port obligations and independent examples.
No executable API, schema, route, policy producer, tests or runtime approval follows.

## 1. Imports and actual operation audit

Import the COMPLETE [host lifecycle](api-host-lifecycle-v1.md),
[host boundary](api-host-lifecycle-boundary-v1.md),
[host-auth continuations](host-auth-continuations-v1.md),
[account ingress R2](account-ingress-v1.md),
[transport R2](session-transport-v1.md),
[writer](sqlite-writer-v1.md), [writer work](writer-transactions-v1.md),
[account store](sqlite-account-sessions-v1.md),
[password logical](password-hashing-v1.md), [password API](password-hashing-api-v1.md),
[session supplier](session-resolution-v1.md),
[session lifecycle](session-lifecycle-v1.md), [clock](session-clock-v1.md)
and [local-login authority](local-login-authority-v1.md) unchanged. Their failure,
secret/resource, revocation, real-outcome and platform limits remain mandatory.
API-014 extends API-013; it does not extend the lifetime of an API-001 execution.

The following observations are from actual source at the source base:

| Source | Actual surface / missing capability |
| --- | --- |
| `server/include/subathon/server/api_host.hpp`, `server/src/api_host.cpp` | Host/transaction/elapsed readiness remain explicit unimplemented boundaries. The guarded account facade exposes installation read/epoch replacement only; no enrollment or recovery ports exist. |
| `server/include/subathon/server/sqlite_account_sessions.hpp`, `server/src/sqlite_account_sessions.cpp` | `SqliteAccountSessionUnit` has installation/account/local-credential reads and account insert/full replacement/local-credential replacement; one borrowed writer session, no commit. Schema8 has no username, protected-owner, invitation, setup/reset, recovery-code, permissions or security-audit relations. |
| `server/include/subathon/server/password_hashing.hpp`, `server/src/password_hashing.cpp` | Exactly three synchronous raw primitives; create/verify each has one expensive call, upgrade can have two. Protected helper copy/lock/wipe exists; neither permit acquisition nor workflow authorization exists. |
| `server/` producer inventory and ACCESS-001/005 briefs | No actual indexed directory, bootstrap/owner singleton, invitation/recovery record, console entitlement or current workflow authorization producer is present. A reference, enum or absent registration cannot supply one. |

ACCESS-001 owns actual directory, owner/bootstrap/invitation and initial access
facts plus INITIAL recovery-set/control creation and separately reviewed logical/
physical extensions. ACCESS-005 owns SUBSEQUENT recovery policy, code/reset/set
lifecycle facts and console/helper entitlement. The same abstract recovery-control
ports bind ACCESS-001's initial producer for enrollment, ACCESS-005's for later
recovery; enrollment must not depend on ACCESS-005. Their integrations
must supply the named ports below. `read_bound_access` uses ACCESS-001's owner/role
storage and ACCESS-003's actual current delegation evaluator; ACCESS-005 composes
them for recovery. ACCESS-001/005 each own their same-transaction security-audit
adapter and must split any substantial physical prerequisite before implementation.
These are required named producers, not capabilities already supplied by STORAGE-020.
API-014 owns abstract port types, guards and
bounded forwarding only; it compiles/tests with unavailable production bindings
or explicitly simulated producers. No reverse dependency on ACCESS-001/005 or
SESSION-002/003 is introduced. No physical schema extension is implicit. Substantial
storage prerequisites must be split/reviewed before consumer implementation.

## 2. Original association and compiled registrations

Composition, before serving, registers one compiled dispatcher per permitted purpose:
`initial_owner`, `invitation_registration`, `recovery_code_reset`, `helper_reset`,
`console_owner_reset`, and the bounded issuer purposes `issue_setup`, `issue_invitation`,
`issue_helper_reset`, `issue_console_reset`, `replace_recovery_codes`,
`revoke_invitation`, `revoke_helper_reset`, `revoke_console_reset`. The two reset
issuer purposes include explicit replacement of their original target's offer;
they cannot change helper/owner purpose or target. Each registers
exact phase transitions, permitted named operations, complete bounded payload/result
shapes, source/mutation checker, producer bindings and response emitter. Duplicate,
unknown or incomplete registration refuses before listening/secret ownership; there
is no runtime script, arbitrary callback/SQL registration or client-selected purpose.
A dispatcher/producer is trusted compiled application composition, not client data.

Network originals use the SAME actual guarded RequestIngressScope, immutable policy
producer/snapshot, route-specific browser/native guard and typed secret admission
as host-auth §§2/3/6. Neither a setup code nor an old cookie bypasses the route guard.
Actual AUTH budget/edge admission remains SESSION-012 and DEPLOYMENT-007/008;
unknown route grammar, producer or budget refuses. Setup/reset code syntax, username
normalization, invitation roles, recovery entropy/format and abuse limits remain
consumer contracts before enablement, not inferred from the engineering bounds here.

Console issuance has a distinct compiled, host-only `ConsoleIssuerScope`: created
only by the actual trusted server-console adapter after current OS/installation
entitlement, bound to this running host/installation/one invocation. During ONE
synchronous admission call, the host validates it and creates the same bounded
original record with inert privately matched console provenance. The borrowed scope
revokes on admission return; it is NEVER retained through either queued stage.
That return does not end the separately owned console invocation/sink lease. The
actual adapter keeps a bounded invocation record alive to final response/drain,
exposes current entitlement/liveness through its registered producer, and cancels
the original on actual invocation end, sink loss, entitlement revocation or stop.
No retained scope/facade supplies later authority: each stage/completion checks the
inert original against that actual current producer and lease. Refused admission
transfers nothing and retains no invocation credit.
The scope is not constructible from an account ID, client
flag or IPC payload. ACCESS-001/005 must supply the actual console provenance and
bounded private output adapter. An external CLI may request issuance through that
reviewed adapter, but neither accepts a password nor invokes crypto in its own
process. No actual console source means unavailable, not initial-owner entitlement.
The host reserves the same original/stage/control/secret/permit capacities and
nonrenewing local deadlines for console issuance; no network budget is fabricated
for this local scope. Console output has its own actual bounded sink lease.

Every original privately binds purpose, installation, immutable original selection
(username/workflow/code selector/target), policy or console source, cancellation,
nonwrapping job/stage/native ordinals and exact expected source lineage. Public IDs,
digests and DTOs are equality data after membership, never constructors or authority.
A later request cannot adopt an old worker result or rebind a reset to another account.

The reference stage registration is finite:

| Purpose | Legal ordered stages / native work between stages |
| --- | --- |
| Initial owner / invitation | `select_enrollment` committed positive code/token/status/UTC/entitlement read; password creation + bounded recovery-code batch; `commit_enrollment`; final response. |
| Recovery-code reset | `select_recovery` committed read; verify ONE selected recovery code, create new password and replacement code batch; `commit_reset`; final response. No scan/try-every-code verification. |
| Helper / console-owner reset redemption | `select_recovery` committed positive token/status/UTC/entitlement read; create new password + replacement code batch; `commit_reset`; final response. The single-use reset token is checked by its registered bounded format/digest mechanism, not treated as an accepted login. |
| Issuers | `select_issue` committed read; bounded token/code candidate preparation; `commit_issue`; final response. Invitation/helper issuance additionally requires current actor/delegation authorization; console/setup requires actual console scope. |
| Revocations | `select_issue` committed current entitlement/original-record read; ZERO candidate/native work; `commit_issue` conditionally revokes only the selected invitation or same-purpose reset offer; final nonsecret response. |
| Replace own recovery codes | `select_recovery_set` committed read; bounded replacement batch; `commit_recovery_set`; final response. Actual current account/required fresh authentication and policy remain consumer-owned. |

Only ONE writer stage is outstanding per original. At most TWO accepted stages or
admission attempts per original, each using the inherited16-credit ceiling without
expanding it. ANY refusal/failure, including known rollback, terminally ends this
original and wipes its candidates after actual drain. No same-original resubmission,
repeated native work or candidate replay; retry requires a fresh guarded request
and newly checked current facts. This consumer profile is stricter than the host's
generic ceiling. Stages
never wait for crypto/IO; no BEGIN, transaction, ticket, facade, proof or source
borrow crosses a yield. Workers retain only inert handles and owned bounded values.
The same host elapsed source is retainable read-only scheduling input; UTC is a new
revocable execution/publication facade per actual scope, never retained as authority.

## 3. Closed named port surface

All names below are PROPOSED internal operations, not existing declarations.
`EnrollmentExecution` contains only guarded named facades in the host's actual
caller-owned transaction. Host-only construction; noncopyable synchronous borrows.
Before every call check owner/thread/nonreentrancy, actual invocation/original/job/
purpose/phase, policy/console membership, generation/readiness, elapsed and stop.
Operations bind original keys; a caller cannot replace them with another selection.
Every port read is bounded/indexed and on the SAME active writer transaction;
no scan, cached success, separate connection or network/worker call is allowed.

| Proposed operation | Exact obligation |
| --- | --- |
| `read_bound_account()` | Forward `read_account(installation, account)` unchanged after original target is privately resolved. Preserve healthy absence versus invalid/resource/unavailable; no directory lookup is inferred. |
| `read_bound_local_credential()` | Forward API-013's existing bound full credential read unchanged, including complete account/install closure, metadata-first preflight, exact verifier and full expected facts. Known missing credential is distinct from missing producer or corrupt closure. |
| `insert_bound_account(new_account)` | Forward STORAGE-020 `insert_account(const StoredAccountFacts&)` unchanged; only current enrollment authority, newly reserved original identity and same transaction. Collision never adopts an existing account. |
| `replace_bound_account(expected, replacement)` | Forward `replace_account(expected,replacement)` unchanged: immutable installation/account keys, full access/epoch/revision CAS and new revision. Reset preserves enabled/access facts, changes account epoch/revision, and cannot rewrite permissions/owner. |
| `replace_bound_local_credential(expected_optional,new_local,expected_account,expected_installation)` | Forward `replace_local_credential` unchanged with all FOUR actual arguments. Compare full old revision/verifier and current account/install fences; absence may insert only if still absent. New credential revision differs. If account changed earlier in the same transaction, supply its exact newly staged facts, not a stale pre-write account. No replacement can create accepted login or rehash lineage. |
| `read_original_binding()` | Forward API-013 `resolve_original_username()` unchanged for the privately bound original username, returning exact key/binding/account/revision or known absence. The compiled ACCESS-001 producer must establish actual indexed absence; unbound producer returns unavailable before lookup. No account-keyed reverse lookup is implied. |
| `compare_exchange_original_binding(expected_optional,replacement)` | New bounded directory port: compare ALL original key/normalization-version/binding/account/revision facts and uniqueness in this transaction; insert only on healthy absence, replace only on exact current match/new revision. Preserve installation/key and permitted enrollment semantics; no arbitrary username reassignment. |
| `read_enrollment_control()` | New ACCESS-001 port: one exact installation-scoped bootstrap/protected-owner/registration control record, current setup-code selection or invitation record, full revisions/status/deadline/digest/roles/delegation source and original target. Healthy no-owner is an actual singleton fact, not an empty account lookup. Return complete closure or typed absence/invalid/resource/unavailable. |
| `compare_exchange_enrollment_control(expected,transition)` | Named transitions only: issue/replace setup code while no owner; issue/revoke invitation under current delegation; consume original live code/invitation; atomically install the initial protected owner AND close bootstrap. Full singleton/code/invitation CAS; setup replacement invalidates old code in same commit. No generic owner-transfer or ordinary role edit operation. |
| `read_bound_access()` | New consumer-owned port: full current protected-owner binding, target permission/role facts and their revisions, and actor/delegation/fresh-auth source where applicable. Missing producer refuses. This port supplies facts, not permission merely by returning them. |
| `install_enrollment_access(expected_absence,new_access)` | New ACCESS-001 port: initial owner or invitation's currently authorized initial access only, full expected absence/control/delegation CAS. No merge with unknown existing roles and no rights beyond current inviter delegation. Initial owner status is established through the SAME enrollment control transition. |
| `read_recovery_control()` | New purpose-bound abstract port: ACCESS-001 supplies initial-set/control absence for enrollment; ACCESS-005 supplies subsequent lifecycle facts. For recovery, return exact original reset offer OR selected recovery-code record plus current recovery-set generation/revision, original account, expiry/status/digest/verifier and current permission/owner closure. One indexed candidate only; no enumeration of all account codes. Also supports current set metadata for authorized replacement/issuance. |
| `compare_exchange_recovery_control(expected,transition)` | Same purpose-bound producers: ACCESS-001 installs initial finite set only against actual absence; ACCESS-005 issues/replaces/revokes bound reset offers, consumes selected code/reset offer, replaces a finite recovery set and invalidates ALL former generation codes/offers for the target. Full original record AND set-generation CAS. Set replacement is bounded generation fencing, not an unbounded row walk. Physical implementation must prove old codes cannot be reused. No target/ownership/permission change. |
| `append_security_audit(event)` | New bounded compiled audit port: same-commit fixed category, original operation identity, actor/target opaque references and revisions/outcome; no raw code/password/hash/token or arbitrary exception text. Missing/full/failing required audit prevents successful domain commit. Exact schema/retention producer remains consumer/storage-owned. |

The following allowlist is closed; absent operations/transition kinds REFUSE before
port access. Abbreviations expand to exact names above: `a`=read_bound_account,
`c`=read_bound_local_credential, `d`=read_original_binding,
`e`=read_enrollment_control, `p`=read_bound_access, `r`=read_recovery_control.
`I`=insert_bound_account, `A`=replace_bound_account,
`C`=replace_bound_local_credential, `D`=compare_exchange_original_binding,
`E`=compare_exchange_enrollment_control, `P`=install_enrollment_access,
`R`=compare_exchange_recovery_control, `L`=append_security_audit.
Host-only installation/finalization reads remain inherited, never consumer escapes.

| Compiled purpose | Selection reads (also reread at final stage) | Final-stage mutations ONLY |
| --- | --- | --- |
| initial_owner | a,c,d,e,p,r | I,C,D,E(initial-owner consume+close),P,R(initial set),L |
| invitation_registration | a,c,d,e,p,r | I,C,D,E(consume invitation),P,R(initial set),L |
| recovery_code_reset / helper_reset / console_owner_reset | a,c,p,r; d ONLY if original selection includes username | A,C,R(consume selected code/offer+replace set/invalidate old),L |
| issue_setup | e,p | E(issue/replace setup while no owner),L |
| issue_invitation | e,p | E(issue invitation),L |
| revoke_invitation | e,p | E(revoke original invitation),L |
| issue_helper_reset / issue_console_reset | a,p,r | R(issue/replace same-purpose original offer),L |
| revoke_helper_reset / revoke_console_reset | a,p,r | R(revoke same-purpose original offer),L |
| replace_recovery_codes | a,p,r | R(replace own set/invalidate old generation),L |

Each row has only its §2 select/final stage pair; selection stages are READ ONLY.
Within a shared row the original purpose remains exact: helper cannot target owner,
console reset cannot target helper, and a code consumer cannot consume a reset offer.
The bound original record determines each transition; no arbitrary transition enum
can expand this matrix. In particular issuers/revokers cannot change credentials,
and resets cannot insert accounts, write directory/access or alter ownership.

Current permission/delegation and workflow policy checks are compiled CONSUMER
operations using these current facts. The registration supplies `check_current`
within the live synchronous body, immediately before each permitted mutation and
again after staging before finalization; its inputs are actual fresh source reads
and private job/candidate provenance. It cannot accept a caller boolean, old verified
enum, public capability DTO or stale selection. Required fresh authentication uses
the actual current supplier/owner path and UTC, not the reset token's creation time.
Initial enrollment/reset may be entitled without an existing session/accepted login.
Helper reset must refuse the protected owner; console owner recovery targets the
actual existing owner only. Missing authority source refuses before disclosure/write.

BEFORE any native call or candidate generation, the selection body must check
current entitlement, complete original source/control closure, trusted UTC expiry,
live/unconsumed/unrevoked status and setup/invitation/reset token format+digest
match using the owning reviewed bounded mechanism. Only its actual normal committed
matching positive completion, with fresh host and consumer guards, releases worker
work. Wrong/expired/replaced setup, invitation or reset token means ZERO expensive
calls and ZERO output candidates. Recovery-code redemption first validates the ONE
indexed selector/status/expiry/set/entitlement; its secret match necessarily costs
ONE permitted native verification. A negative verification permits no password or
replacement-code creation. No scan, batch of verification attempts or caller-provided
verified flag substitutes. All checks repeat at final mutation; selection is not a
durable reservation or future grant.

Abstract producer construction/binding is private trusted composition: the host
binds actual concrete adapters to its SAME private writer session for one execution.
The consumer sees ONLY these named guards, never SqliteWorkContext/Session,
SqliteAccountSessionUnit, raw SQL/query/transaction callback or a commit escape.
Producers are independently reviewed trusted storage adapters, cannot begin/commit,
and obey cumulative writer SQL/row/payload budgets. API-014 cannot fabricate their
physical implementation. Missing registration/binding is unavailable BEFORE port
access; known missing is emitted only by a successfully bound healthy producer.
Exceptions retain actual sticky writer errors; no catch converts corruption or
SQL failure into absence/stale or subsequent successful work.

## 4. Atomic compositions and actual settlement

Each initial selection is a read transaction and grants no durable reservation or
success. Only its normal actual COMMITTED, privately matching completion plus fresh
original/policy/phase/elapsed guards can release positive metadata to the worker.
Final mutation rereads FULL source/control/binding/account/install/access/recovery
facts and current authority; exact original selection/revision/verifier comparison
prevents ABA-by-revision-only, account rebinding and adoption of replacement codes.
Expiry uses the same actual revocable trusted UTC: `now >= expires` is expired.
A read sample cannot authorize the later commit. Failed/unknown clock refuses;
no elapsed/UTC fallback, cached trust, persisted anchor or clearance of other holds.

The final no-yield transaction stages one complete operation:

- Enrollment: verify current no-owner/setup code OR live original invitation and
  current delegation; prove original directory key absent; insert account; conditionally
  insert local credential against actual staged account/install; insert directory and
  access facts; install finite recovery set; consume code/invitation; initial setup
  installs protected owner/closes bootstrap; append required audit.
- Reset (ACCESS-005 recovery policy, guarded here): verify selected live code/offer, original target and current entitlement;
  preserve exact existing permissions/protected owner and account access; replace
  account epoch/revision and local credential with full CAS; consume code/offer and
  invalidate old recovery-set/reset generations, stage offered replacement codes;
  append audit. Directory remains byte-for-byte unchanged. If the original selected
  a username, freshly compare that binding through `read_original_binding`; token/
  console/account-selected reset instead compares its original target/control/full
  account closure, without inventing an account-keyed directory read.
  New account epoch revokes ALL existing application sessions for that account via
  their unchanged captured-epoch supplier fence, without an unbounded session scan
  or a false claim that physical session rows were rewritten. Other accounts remain
  unchanged. Actual authentication owners invalidate outstanding old acceptance/
  password jobs; every later use also reads the new full source/epoch. No reset is
  the local-login owner's rehash lineage handoff.
- Issuance/set replacement: current console/actor/account/delegation/fresh-auth
  checks; exact bound control/set CAS and required audit. Issuing a reset link does
  not itself replace a password, grant a session, change permissions/ownership or
  claim completed recovery. Setup/reset links are copied/shared manually only.

Any stale/collision/refusal/required port failure prevents the complete domain
mutation commit; rollback every staged change. One caller owns BEGIN/COMMIT/cleanup.
The frozen host performs its final selected installation read AFTER all mutations
and immediately before real COMMIT; reserve that SQL/read budget, block later facade
calls and stage actual installation facts privately. Execution revokes BEFORE the
writer completion. A transaction's outcome is actual writer truth, not domain intent.

| Actual outcome | Settlement and response |
| --- | --- |
| Writer admission refused | No body/completion/result exists; release unaccepted resources only. No consumption, returned code batch or hidden retry. |
| not_started / known rolled_back | No new state/success is claimed. Prior current code/offer may remain usable only under a fresh request/current checks; no same-request retry or second hash is automatic. Wipe abandoned generated secrets. |
| normal committed with matching private final envelope | Settle source/generation invalidation from actual committed facts; only then permit one fresh guarded publication of the intended result/code batch. Consumer current-source/expiry/publication checks still apply. |
| committed with later exception, missing/wrong envelope, timeout or lost output | Preserve ALL committed changes/consumption/session fences; suppress positive response and erase unpublished codes. No rollback claim, old verifier restoration, automatic reissue, secret-response cache or bearer recovery. User can invoke a separately authorized NEW issuance/recovery operation. |
| outcome_unknown / cleanup uncertainty / impossible completion | Fence affected protected sources and secret output, poison candidates/old attempts, retain actual writer fail-closed/drain behavior. No guessed rollback/consumption result, retry or plausible later read clearing uncertainty; owning recovery/reconciliation is required. |

Every accepted stage has one actual settlement callback even when cancelled. It
matches host/original/job/stage/purpose/phase and the writer's actual body/completion
pair; duplicate/foreign completions cannot settle another job. Actual host installation
truth updates even after domain cancellation; independent account/directory/workflow
source owners settle/invalidate their own affected facts before new use. Positive
worker notification and public emission are separate once-only latches. Cancellation
never turns committed into rolled_back or releases a native borrow early.

## 5. Finite work, deadlines and secret copies

Use the SAME single installation-wide PasswordWorkPermit from API-013, including
setup, invitation, code/reset issuance/redemption and code replacement. Zero waiting
password jobs; no extra worker/permit per consumer and no out-of-process password
path. No expensive call runs on Qt/IO. Creation/verification use unchanged primitives,
not the login-only upgrade API or invented accepted-login proofs.
Taking this permit even for zero-Argon issuers/revokers is a conservative additional
profile choice: one secret workflow/drain at a time, including console issuance.
It provides no fairness or availability guarantee; an active login may refuse console
issuance. ACCESS-001/005's abuse/admission gates must assess this before enablement;
any reserved/fair issuance path requires separate review, never a second executor or
password bypass. Revocation likewise cannot claim success on a busy refusal.

Reference profile: one recovery set has exactly8 codes; each raw generated code
is at most256 bytes and each encoded verifier at most127. Eight is a finite engineering
profile, not a new public recovery policy; another batch size needs a separately
reviewed profile preserving every limit before enablement. Consumer formats/entropy/
selector/digest rules remain to be reviewed; unknown format is unavailable. All input
passwords retain exact0..1024 bytes and unchanged password API cost/preflight rules.

| Purpose | Maximum expensive native calls / other candidate work per original |
| --- | --- |
| Initial owner / invitation |1 password create +8 sequential recovery-code creates =9. |
| Recovery-code reset |1 verify of ONE indexed code +1 password create +8 code creates =10. |
| Helper / console reset redemption |1 password create +8 code creates =9; one bounded reset-token check. |
| Replace recovery set |8 code creates. |
| Setup/invitation/reset issuer / revoker |ZERO password primitives; issuer at most one bounded token candidate generation/digest, revoker ZERO candidates. Same permit prevents a second code/password pipeline. |

Generate each candidate once with the owning reviewed selected random primitive;
no collision redraw/retry loop. At most8 raw code candidates plus one issuer token,
no more than10 expensive calls, checked nonwrapping ordinals. Preflight a whole batch
and reserve ALL capacities before generation/copy. Recovery-code selectors locate
one record; verification failure does not try another. Any helper error, failed
candidate/hash/entropy, malformed output, mismatch, timeout/cancellation or over-budget
result ends the pipeline, erases all candidates and admits no further native call.
All8 replacement records must be valid before atomic staging; no partial code set.
At most one expensive call at a time, max268435456 admitted Argon bytes per unchanged
helper rules, never a product of10. Scalar admitted_memory_bytes is not the permit.

Import ALL host-auth deadlines unchanged for network originals: remaining edge
budget must be actually known; original lease is
`min(admission+12000ms, transport_deadline-2000ms)`; pipeline cutoff is
`min(admission+10000ms, original_lease)`; queue-entry bound5000ms clipped to both;
response handoff5000ms clipped to original/transport/stop/peer. A5000ms entry bound
is not a new mid-body timeout. Console uses the same12000/10000/5000ms local caps
clipped to its actual sink/stop lease. No stage/native call/code batch renews them.
Budget check precedes EACH new call; equality expires. Two stages and ten calls
are MAXIMA, not guarantees that they can finish before any edge deadline. Supported
hardware/profile benchmark plus admitted bundled AND alternative AUTH-edge budget
must prove practical completion before public enablement, or keep this route
unavailable/pursue a separately reviewed profile; never silently extend frozen caps.

Use EXACTLY the host's API-001/native ElapsedSource instance for these scheduling
checks and the SAME current session UTC facade for expiry/freshness/publication.
Missing/regressing/overflow elapsed cancels/closes affected IO; failed UTC refuses
operation-specific authority. Neither source replaces the other. A code lifetime
is900000ms from trusted issuance for setup/reset, invitations default172800000ms
with authorized configured expiry. Checked arithmetic, no saturation/wrap. Consuming
at equality is expired. Local pipeline cancellation never extends/shortens the
persisted code's specification lifetime or implies code consumption without commit.

Complete typed-owner secret profile, overlapping imported job/helper caps and charged
to the SAME host input/response pools (no independent pool per profile):

| Owner | Maximum live semantic capacity / last-use obligation |
| --- | --- |
| Earlier network receive/capture/body/decoder | EXACT host-auth §6 owners/caps/whole-capacity wipes and zero generic host copies. The route's4096 raw-body reference cap includes code/password and needs actual grammar/complete-request admission; it is not automatically inherited from another route. Console has no raw-password input. |
| Job input | ONE checked guarded/locked password allocation<=1024 plus ONE checked guarded/locked code/token allocation<=256; lock BEFORE direct decode/copy. No ordinary string/DOM, raw secret in queue payload or decoded intermediate. Empty password uses one-byte allocation, length0. Keep through final synchronous borrower return. |
| Generated recovery/issuer output | ONE guarded/locked fixed4096-byte arena, at most8*256=2048 raw recovery bytes or one<=256 issuer token plus fixed lengths/offsets. No growing vector/string/secret lambda; no duplicate batch while preparing verifiers. Wipe entire arena on rejection/error or after actual output borrow drains. |
| Password helper | ONE additional guarded allocation<=1024 per call, unchanged helper whole-allocation wipe; code-hash calls borrow one<=256 slice from arena. Record/native/decode scratch retains upstream separate cleanup. |
| Stage commands/replies/cells | ZERO plaintext. Only bounded sensitive verifier/digest/selection/provenance metadata, complete command/reply each<=65536; imported aggregate retained stage copies<=8388608 and original control caps apply, never one pool per purpose. Imported string/driver verifier copies have no complete-erasure claim. |
| Final typed output and IO | ONE separate guarded/locked fixed4096-byte serialized output owner (whole response incl framing must fit) plus at most ONE bounded4096-byte project plaintext IO owner while handing off. Reserve BOTH before copying. No generic OwnedReply/QByteArray/framework response clones, cache, log, password echo, automatic delivery/email or repeat emission. Adapter with unbounded/unknown copies is unavailable. Final output/IO buffers wipe on suppression, exception, disconnect, stop, timeout and real final borrower return/destruction. |

Conservative live capacity for these typed owners is1024+256+4096+1024+4096+4096=14592 bytes
at these job/helper/output owners, besides imported raw/native scratch and sensitive
nonplaintext stage payloads. The1024 job and1024 helper owners are the SAME owners
already counted in host-auth §6, not two new allocations or an additive14592 on top
of those owners. One active pipeline; retained output from a finished
pipeline remains charged until drained. Reserve one installation-wide14592 secret
credit across that entire lifecycle, so a still-live output cannot overlap another
pipeline under an uncounted budget. Password permit additionally lasts through all
native/cell/stage/secret drains, as upstream; neither credit releases at network
close alone. Overflow/capacity/lock failure refuses before any new copy/native work.
OS/kernel/TLS/Go/Caddy/library/register/swap/dump copies remain the imported limits;
these capacity sums are not whole-process RSS or whole-edge heap-erasure proof.

Stop/disconnect/policy/source revocation first cancels membership, forbids new
stages/calls and abandons waiting cells. Drain actual accepted writer completions
and raw/IO callbacks on their owners. A running synchronous native call may continue
past10s/12s/network shutdown and can never be force-freed, detached, preempted or
replaced by another worker. Keep permit, buffers and stable inert dependencies until
return and final settlement/wipe; then join without Qt waiting on a worker that may
need Qt. `stopped` requires ALL inherited plus new barriers. A never-returning native
call leaves graceful shutdown incomplete; a supervisor process kill has no destructor/
erasure/rollback/response guarantee. A restart recreates no original/permit/job from
records and cannot clear durable unknown-outcome or independent restore/clock holds.

## 6. Independent literal examples

These specification/import-derived oracles are authored document examples, not tests
or observed behavior. IDs name separate future test scenarios; real SQL/source/secret/
thread/edge instrumentation is required, not a mock's success flag.
The sole permit serializes these workflows through actual drain. Source/CAS fault
injections below test refusal with deliberately changed simulated producer/storage
facts; they do NOT demonstrate two concurrently admitted password/issuer pipelines.
Other authorized non-password state operations may independently change access/
installation facts while the permit is held; those retain their own gates.

| ID | Direct setup / order | Literal expected observation |
| --- | --- | --- |
| ER01 | Healthy bootstrap has no owner, setup c1 expires900000; now899999, original c1; native succeeds; final facts unchanged | Atomic account/credential/directory/access/recovery-set/owner+closed-bootstrap/consume/audit commit; one post-commit response. No session inferred. |
| ER02 | Same c1 at now900000; alternatively UTC unavailable | Expired / unavailable respectively, zero native calls/candidates/mutation/public success; no elapsed fallback. |
| ER03 | Two raw initial-owner contenders; first holds permit through commit/drain, second attempts admission | Second refuses busy before selection/native. Fresh second request after drain reads existing owner and refuses setup; no second owner/account adoption/partial directory. In a separate injected control-CAS race, competing owner installation makes final enrollment roll back atomically. |
| ER04 | c1 redemption holds permit; console replacement attempts; then redemption cancels and fully drains without commit; fresh console issuance replaces c1→c2 | First console attempt busy, no replacement. Fresh committed replacement invalidates c1; later original c1 refuses before native. Separately inject c2 or same revision/different bytes after selection: final full comparison loses, never adopts/overwrites c2. |
| ER05 | Owner a already exists; actual console requests initial setup | Refuse initial-owner entitlement; no code/account/ownership rewrite. Console owner-reset may separately target a only. |
| ER06 | Actual directory producer absent vs bound healthy original username absent | Unavailable versus known absence; former performs no account lookup/crypto, latter alone may proceed under enrollment authority. |
| ER07 | Invitation i expires172800000; now172799999; inviter delegated Observer originally but current grant revoked before final stage | Current delegation refusal; no account/roles/code consumption. A still-authorized unexpired invitation can consume once; replay loses. |
| ER08 | Invitation redemption holds permit; revocation attempts; separately inject consumed/revoked invitation or directory a→b after selection | Real revoker busy before selection; fresh authorized revocation after cancel/drain makes later redemption refuse before native. Injected full-source mismatch refuses final mutation with no adoption of b or credential overwrite. |
| ER09 | Reset holds permit with a/r/H0; competing reset attempts; separately inject z/H1 or same r/different verifier before final stage | Real competitor busy before selection/native; fresh reset only after drain/current checks. Injected full-CAS mismatch retains H1 with zero partial epoch/recovery change; no simultaneous accepted reset claimed. |
| ER10 | Actual separately authorized non-password access/install/permission change; separately inject owner binding/account epoch/recovery-set generation change during crypto | Fresh current comparison refuses; no cached selection/entitlement authorizes mutation. Fault-injected cases establish mismatch handling only, not producer concurrency or an extra permit. |
| ER11 | Valid helper reset of helper h with permissions P; sessions capture epoch e; code set G1 | One ACCESS-005 policy commit keeps P/access/identity, epoch e→f, replaces credential, consumes offer and G1→G2; all old-epoch sessions and G1 codes refuse. Other account b unchanged. |
| ER12 | Administrator attempts helper reset targeting protected owner a | Denied before secret disclosure/mutation. Actual console reset of existing a preserves SAME owner binding/permissions, never transfers to a new account. |
| ER13 | Code selector k identifies one recovery record; verify not_successful | No other code tried, no password/code creation, no mutation. It is not definitive wrong-password proof or accepted login. |
| ER14 | Recovery reset selected8 replacement codes; all operations succeed | Exactly1+1+8=10 maximum expensive calls; one sequential native call at any instant, one permit. Failure on code4 discards whole batch; no partial commit. |
| ER15 | Set size9 under reference8, password1025, response4097, projected stage65537 or exhausted retained pool | Refuse before respective ownership/native/mutation; no truncation, spill allocation or lowered crypto cost. |
| ER16 | Login holds permit; initial setup/reset/console issuance arrives, or output buffer from previous job still drains | Refuse busy/resource before new secret copy/post; no extra executor/permit or waiting password job. |
| ER17 | Admission at elapsed1000; proved transport deadline16000 | Original lease13000, pipeline11000;5000ms queue-entry clipped accordingly. At11000 no new native call/stage under live pipeline; no batch deadline renewal. |
| ER18 | Actual edge closes7000 while pipeline cutoff11000, native call still executing | Cancel output/new calls immediately; keep permit/input/helper until native return and cleanup. A committed earlier mutation stays committed. |
| ER19 | Elapsed read fails/regresses or arithmetic overflows; separate UTC failure at final source check | Cancel unavailable/close affected network on that IO turn; no scheduled-time trust or prior UTC reuse. Existing actual outcome still settles. |
| ER20 | Writer enqueue refuses queue_full; alternatively accepted wrapper cancelled before entry | First has no completion/body. Second drains its real not_started completion without BEGIN; neither consumes workflow or emits codes. |
| ER21 | Final transaction writes account then directory/audit CAS or SQL fails, known rollback | ALL staged changes undone; old current code/epoch/credential remain. New candidate bytes wiped; no automatic retry/hash. |
| ER22 | Reset actual COMMIT succeeds, then envelope invalid/emitter throws/output lease expires | Keep reset/session/code invalidation; suppress/erase unpublished replacements, never restore H0 or claim rollback/delivery. |
| ER23 | Cleanup/outcome unknown despite proposed successful reset | Source/output fenced; no guessed consumption/rollback, duplicate issuance, retry or later-read clearance. Reconciliation remains owning gate. |
| ER24 | Duplicate/foreign old job result after cancellation and new original admitted | Cannot settle/mutate/respond for new original; old actual outcome drains once; no double permit release. |
| ER25 | Canaries in malformed body/target/header, partial decode, allocation/lock failure, stop during native/output | Inspect each real owner before release/reuse: full required raw/job/helper/arena/output wipe after final borrower; zero generic queue/response clones. No whole-edge erasure claim. |
| ER26 | Console adapter missing/current source revoked; console client supplies password or trusted=true | Unavailable/invalid route; no direct primitive invocation or synthetic issuer scope. Genuine console issuance returns only specified setup/reset code with900000ms lifetime. |
| ER27 | Normal unchanged restart vs known restore/unknown earlier outcome | No in-memory job/original/code plaintext resurrects; ordinary epochs unchanged. Restore/unknown holds remain; this contract supplies no startup acknowledgment or clock binding. |
| ER28 | Setup c issued UTC1000; network original expires elapsed12000 before redemption commits | c's stored expiry901000 is unchanged/unconsumed absent commit; a NEW actual request may use current c before expiry. Old continuation is terminal and cannot replay. |
| ER29 | Wrong setup/invitation/reset-token digest, revoked token, or expiry before selection; separately positive selection result with rolled-back/mismatched completion | ZERO native calls/candidates; no workflow consumption. Positive DTO/domain intent cannot release work without actual matching committed selection. |
| ER30 | Original indexed live recovery-code record selected; first verify fails; separately selector absent or expired | First: exactly ONE verification, ZERO password/code creates. Second: ZERO native calls. Neither searches another code or mutates state. |
| ER31 | Authorized revoke_invitation commits; alternatively issue_helper_reset explicitly replaces offer x with y | Revoked invitation refuses redemption; x refuses and y retains its own15-minute lifetime. Each uses only its allowlisted control CAS+audit, ZERO password primitives. |
| ER32 | Issuer calls credential replacement, reset calls account insert/access install, or selection calls any mutation | Guard refuses BEFORE producer access/SQL. No generic transition enum widens the compiled allowlist. |
| ER33 | Known rollback on final stage, then repeat same original | Original terminally invalidated; candidates erased after drain; repeat refuses with no native restart/third stage. A fresh original is required even if the old code remains current. |

## 7. Remaining gates and dependency direction

APITEST-014 must first refine minimal additive types and independently reviewed
literal tests on working API-013 foundations; intended red must be missing behavior,
not compilation/dependency/skip failure. API-014 gets a fresh implementer and complete
separate review/check/CI/normal-merge cycle, preserving ALL frozen assertions.
Port simulations establish only host guard/forwarding behavior. Actual source/schema/
policy/atomicity/current authorization/console/randomness and typed-output adapters
require their owning ACCESS-001/005 and session/ingress/edge gates before enablement.
Twitch-only invitation registration (AUTH-01) and ownership transfer
(AUTH-09/ACCESS-006) remain preserved outside this local-password host surface;
neither is removed from release scope. No ownership transfer, provider method,
route/OpenAPI/JSON, auto-login/session mint, password strength or email delivery is
selected here. Full initial-owner/invitation/
recovery/permissions and both deployment-edge release scope remains intact.

Task graph direction stays APICONTRACT-003 → APITEST-014/API-014 → consumer
baselines/implementations; API-014 also depends on API-013. There is no dependency
from this host contract/extension back to its future consumers. API-001 implementation
and actual session-clock binding remain unauthorized by this task. This document
requires independent Claude initial, separate Codex verification and Claude closing
review; authored examples/source observations alone satisfy none of those gates.
