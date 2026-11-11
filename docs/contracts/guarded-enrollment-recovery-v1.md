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
facts plus separately reviewed logical/physical extensions. ACCESS-005 owns actual
recovery policy, code/reset facts and console/helper entitlement. Their integrations
must supply the named ports below. API-014 owns abstract port types, guards and
bounded forwarding only; it compiles/tests with unavailable production bindings
or explicitly simulated producers. No reverse dependency on ACCESS-001/005 or
SESSION-002/003 is introduced. No physical schema extension is implicit. Substantial
storage prerequisites must be split/reviewed before consumer implementation.

## 2. Original association and compiled registrations

Composition, before serving, registers one compiled dispatcher per permitted purpose:
`initial_owner`, `invitation_registration`, `recovery_code_reset`, `helper_reset`,
`console_owner_reset`, and the bounded issuer purposes `issue_setup`, `issue_invitation`,
`issue_helper_reset`, `issue_console_reset`, `replace_recovery_codes`. Each registers
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
entitlement, bound to this running host/installation/one invocation and revoked on
return/disconnect/stop. The scope is not constructible from an account ID, client
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
| Initial owner / invitation | `select_enrollment` committed read; password creation + bounded recovery-code batch; `commit_enrollment`; final response. |
| Recovery-code reset | `select_recovery` committed read; verify ONE selected recovery code, create new password and replacement code batch; `commit_reset`; final response. No scan/try-every-code verification. |
| Helper / console-owner reset redemption | `select_recovery` committed read; create new password + replacement code batch; `commit_reset`; final response. The single-use reset token is checked by its registered bounded format/digest mechanism, not treated as an accepted login. |
| Issuers | `select_issue` committed read; bounded token/code candidate preparation; `commit_issue`; final response. Invitation/helper issuance additionally requires current actor/delegation authorization; console/setup requires actual console scope. |
| Replace own recovery codes | `select_recovery_set` committed read; bounded replacement batch; `commit_recovery_set`; final response. Actual current account/required fresh authentication and policy remain consumer-owned. |

Only ONE writer stage is outstanding per original. At most TWO real stages for a
successful operation; failures terminally end it. Explicit known-rollback resubmission
may use remaining host lifetime credits (16 total admission attempts), but has no
automatic retry and no repeated native work or reused consumed candidate. Stages
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
| `read_original_binding()` | API-013 directory read of original username/selection, returning exact key/binding/account/revision or known absence. The compiled ACCESS-001 producer must establish actual indexed absence; unbound producer returns unavailable before lookup. |
| `compare_exchange_original_binding(expected_optional,replacement)` | New bounded directory port: compare ALL original key/normalization-version/binding/account/revision facts and uniqueness in this transaction; insert only on healthy absence, replace only on exact current match/new revision. Preserve installation/key and permitted enrollment semantics; no arbitrary username reassignment. |
| `read_enrollment_control()` | New ACCESS-001 port: one exact installation-scoped bootstrap/protected-owner/registration control record, current setup-code selection or invitation record, full revisions/status/deadline/digest/roles/delegation source and original target. Healthy no-owner is an actual singleton fact, not an empty account lookup. Return complete closure or typed absence/invalid/resource/unavailable. |
| `compare_exchange_enrollment_control(expected,transition)` | Named transitions only: issue/replace setup code while no owner; issue/revoke invitation under current delegation; consume original live code/invitation; atomically install the initial protected owner AND close bootstrap. Full singleton/code/invitation CAS; setup replacement invalidates old code in same commit. No generic owner-transfer or ordinary role edit operation. |
| `read_bound_access()` | New consumer-owned port: full current protected-owner binding, target permission/role facts and their revisions, and actor/delegation/fresh-auth source where applicable. Missing producer refuses. This port supplies facts, not permission merely by returning them. |
| `install_enrollment_access(expected_absence,new_access)` | New ACCESS-001 port: initial owner or invitation's currently authorized initial access only, full expected absence/control/delegation CAS. No merge with unknown existing roles and no rights beyond current inviter delegation. Initial owner status is established through the SAME enrollment control transition. |
| `read_recovery_control()` | New ACCESS-005 port: exact original reset offer OR selected recovery-code record plus current recovery-set generation/revision, original account, expiry/status/digest/verifier and current permission/owner closure. One indexed candidate only; no enumeration of all account codes. Also supports current set metadata for authorized replacement/issuance. |
| `compare_exchange_recovery_control(expected,transition)` | Named transitions only: issue/replace/revoke bound reset offer, consume selected code/reset offer, install/replace a finite recovery set and invalidate ALL former generation codes/offers for the target. Full original record AND set-generation CAS. Set replacement is bounded generation fencing, not an unbounded row walk. Physical implementation must prove old codes cannot be reused. No target/ownership/permission change. |
| `append_security_audit(event)` | New bounded compiled audit port: same-commit fixed category, original operation identity, actor/target opaque references and revisions/outcome; no raw code/password/hash/token or arbitrary exception text. Missing/full/failing required audit prevents successful domain commit. Exact schema/retention producer remains consumer/storage-owned. |

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
- Reset: verify selected live code/offer, original target and current entitlement;
  preserve exact existing permissions/protected owner and account access; replace
  account epoch/revision and local credential with full CAS; consume code/offer and
  invalidate old recovery-set/reset generations, stage offered replacement codes;
  append audit. Directory remains byte-for-byte unchanged and freshly compared.
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
| Setup/invitation/reset issuer |ZERO password primitives; at most one bounded token candidate generation/digest. Same permit prevents a second code/password pipeline. |

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

Additional typed secret profile, charged also to imported host input/response pools:

| Owner | Maximum live semantic capacity / last-use obligation |
| --- | --- |
| Earlier network receive/capture/body/decoder | EXACT host-auth §6 owners/caps/whole-capacity wipes and zero generic host copies. The route's4096 raw-body reference cap includes code/password and needs actual grammar/complete-request admission; it is not automatically inherited from another route. Console has no raw-password input. |
| Job input | ONE checked guarded/locked password allocation<=1024 plus ONE checked guarded/locked code/token allocation<=256; lock BEFORE direct decode/copy. No ordinary string/DOM, raw secret in queue payload or decoded intermediate. Empty password uses one-byte allocation, length0. Keep through final synchronous borrower return. |
| Generated recovery/issuer output | ONE guarded/locked fixed4096-byte arena, at most8*256=2048 raw recovery bytes or one<=256 issuer token plus fixed lengths/offsets. No growing vector/string/secret lambda; no duplicate batch while preparing verifiers. Wipe entire arena on rejection/error or after actual output borrow drains. |
| Password helper | ONE additional guarded allocation<=1024 per call, unchanged helper whole-allocation wipe; code-hash calls borrow one<=256 slice from arena. Record/native/decode scratch retains upstream separate cleanup. |
| Stage commands/replies/cells | ZERO plaintext. Only bounded sensitive verifier/digest/selection/provenance metadata, complete command/reply each<=65536; imported aggregate retained stage copies<=8388608 and original control caps apply, never one pool per purpose. Imported string/driver verifier copies have no complete-erasure claim. |
| Final typed output and IO | ONE separate guarded/locked fixed4096-byte serialized output owner (whole response incl framing must fit) plus at most ONE bounded4096-byte project plaintext IO owner while handing off. Reserve BOTH before copying. No generic OwnedReply/QByteArray/framework response clones, cache, log, password echo, automatic delivery/email or repeat emission. Adapter with unbounded/unknown copies is unavailable. Final output/IO buffers wipe on suppression, exception, disconnect, stop, timeout and real final borrower return/destruction. |

Conservative additional live capacity is1024+256+4096+1024+4096+4096=14592 bytes
at these job/helper/output owners, besides imported raw/native scratch and sensitive
nonplaintext stage payloads. One active pipeline; retained output from a finished
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

| ID | Direct setup / order | Literal expected observation |
| --- | --- | --- |
| ER01 | Healthy bootstrap has no owner, setup c1 expires900000; now899999, original c1; native succeeds; final facts unchanged | Atomic account/credential/directory/access/recovery-set/owner+closed-bootstrap/consume/audit commit; one post-commit response. No session inferred. |
| ER02 | Same c1 at now900000; alternatively UTC unavailable | Expired / unavailable respectively, zero mutation/public success; no elapsed fallback. |
| ER03 | Two originals selected no-owner/c1; both generated distinct candidates; first commits | Second rereads owner/control and refuses stale; no second owner/account adoption or partial directory row. |
| ER04 | Select c1; console replaces with c2 before final commit | c1 stale, unchanged c2. Replacement CAS atomically invalidates c1; equal revision with different full bytes still loses. |
| ER05 | Owner a already exists; actual console requests initial setup | Refuse initial-owner entitlement; no code/account/ownership rewrite. Console owner-reset may separately target a only. |
| ER06 | Actual directory producer absent vs bound healthy original username absent | Unavailable versus known absence; former performs no account lookup/crypto, latter alone may proceed under enrollment authority. |
| ER07 | Invitation i expires172800000; now172799999; inviter delegated Observer originally but current grant revoked before final stage | Current delegation refusal; no account/roles/code consumption. A still-authorized unexpired invitation can consume once; replay loses. |
| ER08 | Invitation consumed/revoked or original directory key becomes bound a→b during crypto | Final full selection/control/binding mismatch; no adoption of b, no credential overwrite. |
| ER09 | Reset selects account a/r/H0; credential becomes z/H1 or same r/different verifier before final stage | Full CAS refusal; H1 retained, zero partial epoch/recovery change. |
| ER10 | Account access/revision/epoch, install epoch, permission revision, owner binding or recovery-set generation changes during crypto | Fresh current comparison refuses; no cached selection/entitlement authorizes mutation. |
| ER11 | Valid helper reset of helper h with permissions P; sessions capture epoch e; code set G | One commit keeps P/access/identity, epoch e→f, replaces credential, consumes offer and G→H; all old-epoch sessions and G codes refuse. Other account b unchanged. |
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

## 7. Remaining gates and dependency direction

APITEST-014 must first refine minimal additive types and independently reviewed
literal tests on working API-013 foundations; intended red must be missing behavior,
not compilation/dependency/skip failure. API-014 gets a fresh implementer and complete
separate review/check/CI/normal-merge cycle, preserving ALL frozen assertions.
Port simulations establish only host guard/forwarding behavior. Actual source/schema/
policy/atomicity/current authorization/console/randomness and typed-output adapters
require their owning ACCESS-001/005 and session/ingress/edge gates before enablement.
No ownership transfer, provider method, route/OpenAPI/JSON, auto-login/session mint,
password strength or email delivery is selected here. Full initial-owner/invitation/
recovery/permissions and both deployment-edge release scope remains intact.

Task graph direction stays APICONTRACT-003 → APITEST-014/API-014 → consumer
baselines/implementations; API-014 also depends on API-013. There is no dependency
from this host contract/extension back to its future consumers. API-001 implementation
and actual session-clock binding remain unauthorized by this task. This document
requires independent Claude initial, separate Codex verification and Claude closing
review; authored examples/source observations alone satisfy none of those gates.
