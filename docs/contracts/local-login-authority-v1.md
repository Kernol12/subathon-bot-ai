# Actual local-login authority — document proposal v1

ACCESSCONTRACT-001; identity `subathon.local-login-authority.v1`.
Specification v0.45 §10, AUTH-01/05/08/09/10/11/12/15, SERVER-01, TEST-04/05.
Reconciled on main `40e06a8` (PR112), coordinator claim `3e4a2aa`.
DOCUMENT ONLY: internal ownership, operations and independent examples for
ACCESSTEST-008; no header, test, native call, login, HTTP route, schema or public
proof. Current review/check receipts belong in the eventual PR linked from the
[task brief](../tasks/EPIC-006/ACCESSCONTRACT-001.md); publication is pending.

Fresh Codex author in the prepared `t3/local-login-authority-contract` checkout
explicitly replaces the historical author assignment. Recover/adapt the valid draft
at `53524fc`, contract blob `e08c61d96a4e90f026cd689d9e31800dd85ad3f3`.
Its author/review/handoff receipts remain at that commit in Git history. The old
independent review's authority, source, lifetime and LA01–30 findings remain useful
context; they predate these imports and the current review cycle, so they do not
approve this revision. Initial Claude, separate Codex verification after fixes,
and closing Claude review remain required, coordinator arranged.

Import [password logical/API v1](password-hashing-api-v1.md), including the
[logical byte/cost rules](password-hashing-v1.md), [supplier v1](session-resolution-v1.md)
§§3–4, [lifecycle v1](session-lifecycle-v1.md),
[physical account storage v1](sqlite-account-sessions-v1.md),
[current clock v1](session-clock-v1.md), [writer work v1](writer-transactions-v1.md),
[transport v1 R2](session-transport-v1.md), [account ingress v1 R2](account-ingress-v1.md)
and [host lifecycle v1](api-host-lifecycle-v1.md), with its current
[consumer boundary](api-host-lifecycle-boundary-v1.md), unchanged.
Password logical, supplier, writer and transport blobs are unchanged since the
old draft; password API, lifecycle, physical and clock changes only relocate
historical evidence links. Ingress and host boundaries are now accepted documents,
not implemented request/clock/host producers. Current import identities and actual
read-only source audits accompany the author handoff/PR; no old evidence files are
restored and no accepted import is edited here.

## 1. Owner, upstream inputs and downstream handoff

ACCESS-008 owns one `LocalLoginOwner` on the authoritative Qt command/state thread,
a private fixed-capacity attempt registry, one password worker and its narrow
request/reply cells. These are proposed internal names, not exported declarations.
The actual host owns their composition with its writer, request lifetimes, account
directory and current clock. No client, connector, arbitrary callback or provider
can construct an owner-bound request, accepted attempt or password source.

| Existing owner | Exact responsibility retained |
| --- | --- |
| ACCESS-001 | Established account identity, current username-to-account directory and its revisions, invitation/bootstrap/local-credential creation and account policy. Initial owner requires the actual single-use 15-minute console code while no owner exists; replacement invalidation, local credentials/recovery codes and registration closure stay here. |
| ACCESS-005 / STORAGE-007 | Actual helper/console recovery and stopped/staged restore; same-commit account-epoch or acknowledged installation-epoch fences, operational readiness and unknown-outcome holds. Recovery preserves permissions and existing protected owner identity. |
| API-001 | Actual host/startup activation, execution and fresh completion/publication membership, elapsed deadlines, stop fences and writer/source lifetime under its accepted host contracts. PR112 supplies a reviewed intended-red baseline, not this production host. |
| SESSIONCONTRACT-005 / SESSION-010 / SESSION-003 | Lossless bounded actual ingress, original source/route binding, prior browser sign-in/CSRF/Origin guards, native guards and complete transport integration. This contract supplies neither a fabricated admission flag nor route/schema selection. |
| SESSION-009 | Actual scope-bound Linux clock and current installation/readiness binding under accepted clock v1, including refusal. |
| STORAGE-020 | Bounded current physical facts, full-source/current-fence CAS on the existing writer. No attempt membership, request association or login decision. |
| SESSION-002 / SESSION-003 | Actual issuance/reauthentication, current ingress/policy/clock rechecks, digest-only persistence, matching commit/completion and at-most-once secret publication. |
| ACCESS-004 | Full local plus optional **verified** Twitch login/linking, stable provider IDs and fresh login-method changes. No verified provider path is supplied here. |

Local acceptance runs before any account session exists. It requires neither
SESSION-002 implementation, ACCESS-004 implementation nor a Twitch callback.
It changes no roles, permissions, ownership, provider link or session. An existing
owner's local credentials use this same path; unavailable Twitch cannot remove
that fallback. A role, selected account, username, provider display name, password
helper result or `application_access_enabled` field cannot grant protected-owner
status. Fresh login also does not replace permissions, two-party transfer acceptance
or the local-credential requirement for its recipient under AUTH-09.

The directory is an actual ACCESS-001 dependency, not a new table here. Its bounded
indexed resolution returns one stable account identity and an exact current binding/
revision, known absence, or typed internal inability. Public username grammar,
normalization and password-strength policy remain its owning contract. Never invent
those policies, truncate an account identity to256 bytes or scan all accounts.
An account outside the password API's256-byte local identity profile is unavailable
for this local path until an explicitly reviewed compatibility change. General
account/session identities retain their wider physical/supplier domain.

## 2. Concrete private operations and identity binding

A `LocalLoginRequest` is one server-owned object registered by the actual host for
one live original request/continuation and installation generation. Its constructor
is private to host composition. It retains bounded actual ingress provenance and
policy revision, original request identity, intended sign-in/reauthentication
handoff and retention choice. No secret, caller `trusted` bit, old session resolution
or serialized attempt is authority. Retain the original username selection and its
bounded exact directory key; a later command cannot substitute a different selection.
Its owning ingress adapter performs the actual pre-session guard before routed entry; absence/unavailability
refuses entry. A browser guard adopts no identity from an existing cookie (expired/
revoked cookies do not block fresh sign-in). Native sign-in keeps its own guards.
A later boundary still performs its own current checks. ACCESS-008's private host
association is upstream of full SESSION-003 route integration: it neither requires
that downstream service to accept a local login nor enables a route without actual
SESSION-010 ingress and the owning route's reviewed finite body/secret profile.
The reference ingress body's4096 limit and this owner's metadata65536 limit do not
choose a login wire/body format. No downstream SESSION-003 callback is needed by
the source checks or accepted-attempt registry.

The following table specifies the complete auth-specific operation surface for
later minimal API review. Calls other than admission/cancellation entry marshal to
or execute on the Qt owner. There is no generic executor, arbitrary queued user
function, proof import or accepted-attempt enumeration operation.

| Operation | Required behavior |
| --- | --- |
| `try_admit(request, password_view)` | On the live host's admission side, validate complete bounded input and the request's actual registration; acquire the sole pipeline slot and one registry slot before making another password copy or queued event. Return a private job owner or a fixed refusal. Admission establishes only resource ownership. |
| `start(job)` | Submit one owned start message; on the authoritative queue resolve the original directory binding and read complete local/account/install facts in one actual host-bound writer work. Known absence/refusal/inability never starts crypto. |
| `verification_finished(job, observation)` | Accept only the private matching worker completion of this job/phase. Native success permits the queue recheck in §4; a public `PasswordVerification::verified` argument alone is not this operation's provenance. |
| `make_password_source(job)` | Construct at one stable address one private noncopyable/nonmovable `PasswordLoginSource` for this actual job/attempt, lasting through its synchronous upgrade call and every pending cell. No public factory returns it. |
| `with_accepted(request, work_identity, attempt_ref)` | Private host binding for lifecycle's `load_accepted`: find the actual retained object, require this original request association/current generation, and reserve it for this exact matching body/completion pair after §5 checks. An exact copied reference/DTO cannot establish membership. |
| `settle_mint(reservation, actual_work_result)` | On the owner, settle once from the actual matching `SqliteWorkResult`, including commit despite a later body exception; §5 defines release/consume/poison. |
| `save_upgrade(job, proposal)` | On the queue, recheck actual accepted membership/lineage and current exact facts, then one conditional physical save and actual outcome settlement under §7. The raw proposal has no write authority. |
| `cancel(request)` / `shutdown()` | Close admission and mark matching jobs/attempts unusable, wake worker-side waits and drain safely under §3. Cancellation neither rolls back a known commit nor frees in-flight inputs. |

Private identity is the actual retained object plus owner generation and phase,
never pointer text from a caller. Nonwrapping internal job/command/work-generation
counters refuse at exhaustion; a retired identity cannot be deliberately reused.
Registry slots are not reused while an old message, source or reservation can still
refer to them. A shared control object keeps such messages safe, but never current.
Retained stale handles fail membership/generation checks without touching SQL.

The nonsecret `attempt_ref` and `acceptance_revision_ref` use one32-byte candidate
each from the already accepted initialized random-byte producer, with no timestamp,
constant or deterministic fallback. A known retained collision refuses without a
redraw loop. Randomness is not a proof of historical uniqueness. Both fit the
existing256-byte password-reference ceiling. These are correlation values only;
there is no external attempt bearer, transferable certificate, public result factory
or durable acceptance reconstruction from these strings.

The actual acceptance retains the exact `AcceptedSessionAuthentication` tuple from
lifecycle v1: installation/account/attempt, installation/account epochs, account/
authenticator/acceptance revisions, method=`local_password`, original accepted UTC.
Its private frame additionally retains full current encoded verifier, credential
revision/format, full account/install fences including installation revision,
original directory binding/revision, actual request and source-object identity,
generation, cancellation/deadline state, mint state and optional matching work
identity. A snapshot reference identifies a read only. Neither this tuple nor the
private frame is accepted from a client or reconstructed from a stored session.

## 3. Finite admission, secrets, worker and queue lifetimes

V1 uses ONE active password pipeline and ZERO waiting password jobs. It includes
initial verification, upgrade re-verification/hash and the intervening owner reads.
The host's simple installation password-work permit exists independently of a
local-login owner/acceptance; setup/reset acquire it too, without an upstream
dependency on ACCESS-008. Their authorization and persistence remain their owners.
This permit is a narrow owned resource counter, not a shared executor/service.
Busy admission refuses;
it does not queue another password, spin, replace an admitted job or reduce cost.
The actual ingress source separately bounds its earlier copies and concurrent
requests before allocation. This gate cannot retrospectively bound framework input.

Engineering defaults:64 registry slots; at most65536 semantic metadata bytes per
frame; at most eight active-job metadata frames; password0..1024 raw bytes;
reference cap4096 except existing password account/revision/attempt caps256 and
verifier127. Limits/counts are positive checked configuration; zero is not unlimited.
Check products/sums, destination `size_t`/`max_size`/terminators before allocation.
An admitted job reserves its registry slot even before acceptance. Never evict a
live/reserved attempt to make space. Expired unreserved entries are retired by
bounded slot inspection; all64 is a finite refusal if none can be reclaimed.
These are service-resource limits, not product account/session-count limits.

Registry membership and authority state are read/written ONLY on the Qt owner.
Admission uses a short mutex-protected count/free-slot credit and actual request
closure state; a reserved credit is not an accepted registry entry. Return that
credit only after the owner's slot and every old message have safely retired.
The worker sees immutable owned snapshots, its own secret/source and atomic cancel
state; it never reads a mutable registry frame. Cell arguments/replies move under
the cell mutex, and its once-only terminal flag precedes wakeup. No lock spans
crypto, SQL, a queued call or a wait for the other thread.

Admission also acquires the actual host association/cumulative capacity before
posting; it adds no second uncharged queue. Each stage gets its own once-only host
work association within the same original request. Host queue-entry (default5000ms)
and response-lease deadlines apply independently and may refuse earlier than the
whole password pipeline deadline; neither restarts that deadline.

Each plaintext job has one locked guarded owner of at most1024 bytes (one byte for
empty), acquired only after the permit. Explicit lock success precedes copying;
allocation/lock/runtime failure refuses without a plaintext fallback. Preserve all
bytes and length, including NUL/non-ASCII/whitespace. Never use QString, QByteArray,
logs, metrics, QVariant, CLI arguments or a callback capture containing plaintext.
An event may copy a shared handle to the ONE private job owner, never clone its
secret storage. After transfer, only the worker borrows the bytes; the admission
side erases/releases its earlier owned copy under the ingress contract.

The accepted password helper takes its own checked protected copy synchronously.
Thus at most TWO application plaintext buffers coexist: job1024 + helper1024;
empty input uses two1-byte allocations. Keep the job buffer through optional
post-acceptance upgrade; erase it immediately after the last helper call or any
terminal/cancelled stage. Helper cleanup/record scratch remains its unchanged RAII
obligation. Clear the entire owned allocations using the selected non-elidable
primitive on every release. Verifier/replacement metadata is also private material:
no diagnostic/public reply contains it; release it at last use and clear
owner-controlled mutable scratch. Imported aggregate/string/result lifetimes do
not promise erasure of every encoded-record copy. This is no claim about unknown
framework copies, compiler
registers, native Argon scratch, dumps, swap or guarded-page overhead.

Use one dedicated Qt worker object/thread for password calls. The Qt owner never
waits for crypto, a worker, a condition variable or a later writer work, never
hashes/verifies a password and never pumps events. Worker start/result notifications
use explicit queued calls with owned private job handles. No AutoConnection may
silently run crypto on the owner. Qt documents queued invocation/argument copying
and thread affinity; those facilities alone provide neither bounds nor authority.
[Qt6.12 invocation](https://doc.qt.io/qt-6/qmetaobject.html),
[thread affinity](https://doc.qt.io/qt-6/qobject.html).

`PasswordLoginSource` is synchronous, so only its worker side may wait for a bounded
owner reply. Use one auth-specific shared `BridgeCell`: fixed command identity/kind,
owned bounded arguments/reply, completed/abandoned state and mutex/condition variable.
At most one cell is outstanding. Post one owner event; its handler validates actual
job/generation/phase and submits one matching host-controlled writer pair.
The matching completion stores the reply once and wakes the worker. Each dispatched
stage must enter a NEW actual host execution binding; no execution facade exists
while queued or survives a worker wait. Reads/conditional saves use that binding's
one real writer transaction and host finalization wrapper, including its precommit
current-installation read and unknown-outcome invalidation. This is an auth-specific
adapter to API-001, not raw writer access bypassing its current-source producer.
Before ANY directory/credential lookup or private reply in any stage, require the
same live request/host generation, current ingress-policy revision/readiness and
unexpired original pipeline deadline. The stage queue deadline is checked at actual
execution entry; it does not preempt an executing transaction. Policy change is
unavailable and requires a fresh actual client request, never rebinding an old guard.
Full/closed/failed invocation or writer admission makes unavailable; no implicit
resubmission.

The host's current baseline facade exposes only installation operations. The later
ACCESSTEST-008 minimal API review must specify the named guarded local-credential,
conditional-upgrade and directory accessors needed here; ACCESS-008 composes them
with that same host transaction/finalization path. No raw unit, writer ticket,
context or lambda receiving one may escape. Existing writer/store signatures stay
unchanged. This bounded accessor adaptation is part of the auth slice, not a
second host or a prerequisite supplied by an unimplemented facade.

No BlockingQueuedConnection, nested event loop, queued `PasswordVerifiedSource`,
borrowed view/reference, foreign SQL ticket or caller-supplied callback is used.
A reply contains a completed current observation, never a live writer session.

An admitted job has a30-second resource deadline in API-001's ONE host-owned
elapsed domain (checked U64 nanoseconds; Linux CLOCK_MONOTONIC, suspend excluded).
Check before posting/starting each stage and after every native return; equality
is expired. Worker waits use the remaining ORIGINAL deadline, checked when waking,
not a fresh30 seconds per call. Unavailable/regressing elapsed time or overflow
refuses/cancels affected work; no independent QDeadlineTimer time authority or UTC
fallback. Cancellation/deadline atomically marks the cell abandoned, wakes a wait
and makes later results unusable. Completion still settles the actual transaction
and releases its owned cell; it never writes into a returned stack variable. Do not
release the pipeline/slot while native work or a late cell still uses its state.
This elapsed cutoff supplies neither UTC nor authentication freshness.

Native password work has no cooperative hard deadline. A deadline/disconnect can
refuse acceptance promptly, but in-flight inputs/source stay alive until native
return and cell drain, then are erased. The30 seconds are a logical work cutoff,
not a guaranteed erasure/CPU/shutdown latency. No thread termination, unsafe free,
second worker or new admission hides a stuck call. Qt interruption is advisory.
[Qt6.12 worker/interruption](https://doc.qt.io/qt-6/qthread.html).

Shutdown follows the API-001 stop fence: close admission, cancel requests/
acceptances, abandon/wake cells and forbid new stages or bearer publication. Already
accepted writer wrappers remain in FIFO through their actual completion. A wrapper
cancelled before execution creates no binding, calls no domain body/BEGIN and
retains actual `not_started/missing_transaction` (or writer-unavailable) truth;
it is never erased from the queue or called rolled back. An already executing
transaction completes truthfully. Outstanding crypto returns into cancellation;
it must not enqueue a new source read after stop. Keep the Qt dispatcher, writer
and cell dependencies alive until all accepted work settles and worker borrows
drain, then join/destroy the worker/source and pass the host's writer/network
cleanup barriers. Do not block the owner in `wait()` while a worker still needs it.
Network deadlines may force-close peers earlier; they cannot certify safe native
termination, erasure or graceful stop. A stuck call leaves shutdown incomplete.
A dropped queued event or dead worker is unavailable, never a successful absent
credential. Abrupt process loss supplies no completion guarantee and destroys all
in-memory acceptance; restart cannot reconstruct/replay it. No durable attempt
journal is needed for this model.

## 4. Initial verification and actual queue-time acceptance

The start work uses the host-bound real writer: one host-owned transaction
(internally one `context.begin()`), bounded directory resolution plus guarded
`read_local_credential`, readonly real commit and matching normal
completion. It returns only private exact typed owned facts for this job. No active
transaction, writer ticket, read snapshot or SQL facade crosses the native call.
Physical corruption/resource/SQL failure is not directory/credential absence.
No account, absent local credential or disabled access does not start verification;
all public outcomes use §8. Other setup/recovery rules are not inferred from rows.

The worker calls the actual `verify_password` with this immutable exact credential
and the admitted raw password, using the stored cost and unchanged preflight.
Invalid/unsupported records, cost refusal, unavailable protection/runtime and native
`not_successful` produce no acceptance or upgrade. Native −1 cannot establish why
verification was unsuccessful. A worker return is matched to its private job,
source snapshot, phase and live request; accepting an externally supplied enum or
calling the helper directly cannot produce this completion.

Only real native verified0 schedules the acceptance work. On queue execution:

1. Require actual registered live request, correct job/source/generation/phase,
   uncancelled30-second deadline and current ingress/installation readiness.
   An old policy/source guard never becomes current by copying it. Compare actual current ingress policy; a changed revision is
   unavailable and requires a fresh actual client request under transport v1.
   Never re-admit from a prepared guard or invent missing source facts.
2. Begin one actual host-owned writer transaction; re-resolve the original
   directory selection/binding/revision and read complete current local/account/
   installation facts. No lookup by a later substituted account. Preflight complete
   metadata before allocation; validate all shapes/links/format/hidden revisions. Source inability/corruption/resource refusal
   stops before an authentication decision, with actual writer errors preserved.
3. Compare byte-for-byte the original installation/account, full verifier/container
   and credential revision, entire captured account access/epoch/revision, and
   installation identity/epoch/revision and original directory binding/revision.
   Require current application access enabled.
   A reset, absent credential/account, directory rebinding, access or revision/epoch
   change loses even when a revision string alone matches. Never accept the latest
   credential merely because the worker verified some earlier one.
4. After those current checks, sample the actual clock once in this no-yield scope.
   Unavailable is unavailable. This establishes the original `accepted_utc_ms`; no
   request/worker timestamp or clock at admission can establish it. Prepare the
   bounded private tuple/frame and fresh candidate refs before commit, including
   a checked retention deadline from the actual host elapsed observation. Elapsed
   failure/overflow creates no acceptance; it cannot substitute for the UTC sample.
5. Commit the readonly work synchronously and install acceptance only from its
   matching committed/error-free/normal exact completion while the same
   nonreentrant owner turn is preserved. No state command/event pump may interleave
   final checks and registry installation. Wrong envelope, body exception, unknown
   outcome or lost scope creates no usable acceptance, even if native verified0.
   Completion also checks actual request/job cancellation and the original job
   deadline before installation; a disconnect or elapsed deadline during the work
   suppresses acceptance despite its known readonly commit. Execution facades and
   writer tickets are already revoked. Use only the host's fresh matched completion
   binding/current bounded source storage, rechecking current generation/readiness
   and stop fence; a retained ticket, envelope or execution clock cannot supply it.
   No event/state command may interleave the final read and this installation.

The exact private completion carries matching job/phase/facts, not an externally
constructible proof. Provisional accepted time/reference fields captured in a body
cannot certify success if the writer has no normal result. Completion/allocation
failure never creates a fallback accepted entry. This owner provides no session
bearer and no successful-login HTTP response. Its successful observation means a
real current accepted local sign-in is retained for its actual host continuation.

## 5. Retained membership, freshness, reservation and settlement

Each slot has independent login validity and mint disposition. Validity is live,
invalidated or poisoned. Mint is available, reserved for one actual work, or consumed.
Poison/invalidated is terminal for that object; retiring frees it only after every
message/source/reservation releases its lifetime. Mint-consumed does not by itself
invalidate the accepted local login for its already-started post-login rehash.
It never permits another mint.

Only the real owner creates/locates these objects. Lifecycle's private unit binds
to the actual original request/continuation and matching writer work. A supplied
attempt_ref is merely an equality check AFTER this binding. Verify actual registry
membership, object generation, method local_password, request identity/liveness,
current host generation/readiness, all captured fences and current full credential
lineage. The `CurrentAcceptanceFacts` tuple is then projected under both owners'
limits, never fabricated from a DTO provided to physical storage.

Sample the real current clock in the same work: now before accepted time is
unavailable; otherwise ordered full-domain age<=300000ms is fresh, inclusive.
Age300001 refuses as stale authentication. In addition, each entry has a finite
host-elapsed300000ms retention cutoff from acceptance; expiry can refuse earlier but
never extend UTC freshness. Readiness/clock failure makes it unusable while unknown;
no cached sample authorizes it. Lookup and current checks repeat on each separate
work and at conditional mutation. If scope yields/waits, re-resolve all authority.
A retained available entry cannot keep the original request alive indefinitely;
request termination invalidates it. Registry cleanup is bounded by64 slots.
Elapsed failure refuses retained use too; retention expiry includes equality.
The30-second pipeline deadline still governs already-started rehash/source work;
a drained acceptance handed to lifecycle retains its own five-minute/request/host
limits. Draining releases plaintext and the pipeline, not the original host request
association. The host continuation remains bounded until completion/cancellation;
retained source identity is only a retired generation after source destruction,
never a borrow that may be called again.

Reserve only after actual membership/current eligibility is established, at most
one work per available attempt. Retain the reservation through the actual matching
completion, including thrown bodies, rejected candidates and publication failure.
This lifecycle binding uses the ALREADY active host work/guarded transaction;
it never queues a source callback, waits on the worker or begins nested work.
Physical store DTOs are comparison data; `insert_session` cannot reserve an attempt.
Current fences/full source are checked again in lifecycle's nonyielding conditional
mutation scope. Its own freshness/revision/ingress/clock/policy checks remain intact.

| Actual matching work outcome | Owner settlement |
| --- | --- |
| Rejected before admission / no body | No reservation created, or undo a private provisional reservation with no accepted use. Existing deadlines remain; no caller replay is inferred. |
| `not_started` with proved no mutation | Release only after current/fresh original request/acceptance is re-established on the owner; otherwise invalidate. No automatic retry. |
| Known `rolled_back` | Release only if the SAME original acceptance/request is still live/current/fresh and no other use completed. A new explicit internal dispatch from that original host continuation gets a new work identity; all checks rerun. |
| `committed`, including later exception, bad/missing envelope, suppressed publication or response loss | Consume for minting exactly once. No reference replay, raw-token recovery, rollback claim or retry of the old handoff. |
| `outcome_unknown` / inability to establish settlement | Poison, suppress further mint/rehash use and retain actual unknown truth. Writer fail-close/readiness rules apply. Never release on timeout or assume rollback. |

The original host may explicitly resubmit after a known rollback while its original
request remains live; a new HTTP request cannot claim that continuation using a
reference. A separate external retry begins a new admitted password login. The
owner never silently reassigns a reservation or refreshes acceptance time. A stale
completion for a reused slot cannot settle a newer work/object.

Account credential replacement/reset/access/revision/epoch change and installation
epoch/readiness change invalidate incompatible retained acceptances. Actual committed
recovery invalidation is ordered on the queue; full fresh reads also enforce it if
no eager slot sweep ran. Unknown recovery/storage outcome fences all affected use.
A known rollback does not manufacture a reset. Normal unchanged startup preserves
stored epochs/sessions but starts with ZERO accepted attempts. Known restore cannot
accept any restored credential until its owning epoch/readiness fence is acknowledged.

## 6. The real PasswordLoginSource callbacks

One private source instance belongs to one admitted job/attempt, is never moved,
and outlives the whole synchronous `propose_password_upgrade` call and all cells.
It has no acceptance setter, configurable accepted boolean or alternate crypto.
It runs on the worker and marshals only metadata; every read/check is a fresh real
owner-queue work with terminal completion, not a cached snapshot across crypto.

`read_current(account_ref)` first verifies this source/job/request membership and
phase. It reads the actual bound installation/account using the physical reader;
it cannot switch accounts. A complete present read returns exact current
`PasswordCredential` and this actual attempt_ref, bounded256/256/127/256 BEFORE
copies. Missing means proven absent account/local credential, with no credential
and EMPTY attempt. An orphan, unsupported container format, internal corruption,
resource refusal or unavailable real owner maps to the existing public
`unavailable` state with neither credential nor attempt. Preserve the typed
internal reason and actual writer truth privately. Do not extend the header enum,
manufacture malformed DTOs to trigger source_invalid, or silently call absence.
The unchanged helper still handles actual source allocation/port exceptions.

`check_current_successful_login(verified)` first calls
`verified.belongs_to(*this)` synchronously; false is denied without queuing or
retaining the proof. Check bounded argument fields, exact bound account/attempt
and source/job identity. Copy only owned needed credential/attempt metadata into
the private cell while the borrow is valid. Never queue/retain the proof address,
its credential/view, a stack destination or a future reference to it.

On the owner, require actual registry membership, accepted local validity and
request/source/generation association; exact proof source credential/revision must
equal this acceptance's legitimate current lineage AND fresh actual stored bytes.
Recheck account/install full fences, access, clock/freshness and request/deadline.
Known absent/never-accepted/revoked login or evaluated policy refusal is denied;
changed source/binding/revision/epoch, retired stale generation or expired acceptance
is stale; inability/unknown is unavailable. A still-bound explicitly cancelled
request is denied. No complete positive observation returns before normal committed
readonly completion AND fresh matched host completion checks for the live request,
original deadline, current generation/readiness and stop fence. Cancellation during
that work cannot return accepted just because the earlier body check succeeded.
These decisions accept/issue no login, acquire no new mint
reservation, persist no credential and extend no deadline/authenticated instant.

The password helper freshly verifies the source, checks once before deciding/hash,
and checks again after generating a replacement. Native unsuccessful never calls
either successful-login check. Both checks must reach this real owner; a fake
source proves only helper behavior. A valid private proof from this source still
cannot substitute for the owner's real accepted-login membership/current checks.

## 7. Post-login upgrade, exact conditional save and lineage

The normal local pipeline is: initial verify → actual acceptance → optional
`propose_password_upgrade` while its job password is available → conditional save
if ready → erase plaintext → downstream handoff if still eligible. No upgrade
runs before actual acceptance. Admission covers the complete sequence: at most
initial verify + upgrade verify + one joined-cost hash, sequentially. This extra
work belongs in supported-server measurements; do not advertise one-hash latency.

Upgrade preserves the accepted password API: full original source snapshot,
componentwise maximum costs, no downgrade, both real source checks and owned
`PasswordReplacement`. `not_needed` leaves exact bytes alone. Any hashing/resource/
protection/allocation refusal produces no upgrade success. An already accepted
login never becomes a wrong-password event because later rehash failed or source
changed. The downstream handoff still requires current acceptance independently.

`save_upgrade` does not trust replacement_ready or a copied replacement. It must
be this actual job's bounded returned proposal, with exact account/attempt/source
revision/FULL verifier equal to retained accepted lineage. On the queue begin one
actual writer work, check actual membership/request/freshness/current ingress and
full account/install/credential fences, then invoke guarded physical
`replace_password_upgrade` with its complete fresh expected facts and one fresh
credential revision. Selected corruption/SQL errors retain their actual categories;
healthy missing/stale source causes no overwrite. Do not hold a transaction during
rehash, restore an old verifier, change epochs/permissions/owner or invent a setter.

At the conditional mutation, all full expected record/fence comparisons apply even
if references/revisions happen to match. Reset/revocation/change between either
callback and save wins. Before committing, prepare one exact private normal result
binding actual staged replacement/new revision, originating job/source/attempt and
work identity. Generate/project the fresh credential and acceptance revisions
before staging/commit under the same32-byte/nonreuse rules; failed generation or
allocation cannot leave a partially advanced lineage. Actual settlement is:

| Actual save outcome | Credential and retained lineage |
| --- | --- |
| Healthy stale/missing CAS or known rollback | No upgrade success; keep the old lineage only while still current. A newer reset remains untouched; stale acceptance is invalidated. No automatic rehash/retry. |
| Confirmed commit + error-free normal exact matching result, unchanged authority turn | Advance ONLY this acceptance's lineage to the exact committed new verifier/revision, with a fresh acceptance revision. Preserve original accepted time, request, account/install fences, epochs and mint disposition. |
| Confirmed commit but exception/wrong or absent envelope/lost authority scope | Preserve committed truth; invalidate this acceptance conservatively. No lineage reconstructed from intent or a later matching row, no rollback/upgrade-success claim or old-source restoration. |
| Unknown commit/cleanup or settlement | Poison this acceptance and refuse later mint/rehash; actual credential may be old or new. No blind retry, guessed upgrade success or assumed rollback. |

Advance only the same real accepted object's lineage after known commit. Other
accepted attempts based on the old credential do not inherit it, even if they
verified the same password; they must fail a later current-source check. Already
issued unrelated sessions retain their recorded times/epochs: opportunistic rehash
is not account recovery. A mint-consumed but otherwise live accepted object may
finish its already-bound rehash; it can never mint a second session. The exact
normal path orders upgrade before mint, while the separate callback contract also
covers this consumed state if an enclosing reviewed composition orders them so.

No reset is a lineage handoff. ACCESS-005's actual recovery transaction replaces
credential/account epoch and invalidates old acceptance/session authority while
preserving permissions/owner identity. Any additional recovery records/protocols
remain that owner's reviewed prerequisites.

## 8. Truthful failures, current clock and external boundaries

Use fixed internal categories: denied authentication, stale authentication,
resource refusal, unavailable, invalid stored/source data and cancelled. Preserve
actual `StorageError`, `WorkError` and `TransactionOutcome` separately; do not expose
source text or sensitive bytes. Public mapping belongs to the existing owning
transport: no account, absent credential, disabled access and unsuccessful password
must not disclose account/local-method existence. No per-account detail, verifier,
revision, attempt, permission or owner classification appears in responses/logs.
This proposal makes no constant-time whole-login claim; admitted stored costs,
preflight and actual missing paths differ. Abuse/timing/rate-limit/transport review
remains mandatory; no dummy verifier or account-enumeration guarantee is invented.

Unavailable storage/runtime/protection/clock, malformed stored data, resource
refusal, cancelled work and unknown outcome cannot be reported as wrong password
or nonexistent account. Native −1 is only verification not successful, covering
mismatch or unclassified failure. A generic authentication refusal may communicate
no success without asserting its cause. Actual service inability keeps the agreed
unavailable envelope; it cannot become a fresh-auth challenge or blanket-success
retry. Preserve committed truth when a later callback/response fails.

Current time uses EXACTLY the accepted clock v1 Linux host model: administrator-
maintained UTC/discipline, trusted kernel/libc/hardware/hypervisor and unchanged
time namespace, actual synchronization observations plus continuity monitor and
current operational readiness. `TIME_OK` is no authenticated absolute-UTC proof.
Unknown coherent VM/process rollback or privileged false status is outside that
model. The clock loses its in-memory floor at process loss; healthy cold startup
establishes fresh live observations rather than historical continuity. An old
checkpoint, worker duration, caller flag or accepted timestamp cannot create trust.
Known restore stays held until actual installation-epoch acknowledgment. No new
time authority, durable clock fact or independent recovery service is implied.

Lifecycle rechecks accepted membership/current source on execution and mutation,
uses its own exact positive-U64 policy and signed expiry arithmetic, and separately
samples publication time only after normal committed completion. This contract
neither sets session expiry nor publishes a bearer. Browser pre-session guards,
cookies/CSRF/Origin/CORS, desktop headers, response schemas/status selection and
DPAPI/QtKeychain remain SESSION-003/010 and their API/client owners. No session URL,
browser local-storage/password persistence, project-hosted authorization service,
provider shortcut or email service is added.

## 9. Finite work arithmetic and independent literal examples

Semantic byte accounting counts every string occurrence by length, enum/bool/tag
by1 and i64/u64 by8; optional adds1 plus present content; structs sum their fields.
Fixed job/control/Qt objects, pointers/counters and library/SQL/native overhead
are additional audited storage. Every frame projects its COMPLETE payload before
copy/growth, including duplicate refs and hidden fields. No uncharged metadata
clone/collection, retained history or speculative retry loop is permitted.

The64 registry frames each fit65536 bytes, including reserved/terminal-but-referenced
slots. The active pipeline has at most eight additional frames: request metadata,
original source facts, current owner facts, bridge command, bridge reply/helper
read, replacement/helper result, staged commit envelope and completion projection.
A frame moves at handoff or immutable owners are shared without cloning payloads.
One outstanding cell and one ordinary writer pair per stage are the only bridge
backlog; cancellation keeps the pipeline occupied until it drains. Existing source/
writer/lifecycle per-value and cumulative budgets still apply independently.

Default peak charged metadata is `(64+8)*65536 = 4718592` bytes. Two application
password payloads add at most2048, giving4720640 before native Argon memory. One
active admitted native call has at most268435456 Argon bytes under password v1;
the simple sum is273156096 bytes. This excludes guarded-page/allocator/thread/
framework/driver/native scratch, ingress and ordinary application RAM, so it is
neither a measured RSS bound nor a sufficient deployment RAM requirement.
One normal pipeline has at most SIX writer works: initial read, acceptance, upgrade read,
first check, second check and conditional save. No-upgrade/no-replacement/refused
paths use fewer. Host finalization's precommit installation reads occur inside
those same works and count against writer budgets, not as extra transactions.
Downstream lifecycle work and explicit known-rollback resubmission
have their own fresh admission/bounds. At most three sequential expensive calls
are made; no input-dependent work loop or hard native CPU-time claim follows.

With single-byte synthetic account/revision/attempt, H0 length96 and actual
snapshot44, `PasswordCredentialRead` is `1+1+(1+1+96)+1 = 101` bytes. Complete local
facts are151, or152 including the success variant, as in physical v1. A replacement
with a127-byte new verifier is `1+1+1+96+127 = 226`; its three reference fields are
account, attempt and expected revision, so the maximum is `3*256+2*127 = 1022`.
Exact101/226 budgets fit;100/225 refuse before extra ownership.
These literal examples do not use generated hashes as expected output.

All examples below are synthetic proposed acceptance oracles, never executed tests.
Base: real-source-simulated registered request Q for installation i/account a,
directory binding d, enabled account/account revision k, installation revision j,
epochs e, credential revision r/exact96-byte upstream H0 (empty password), and
admitted job/source S. Native success is actual selected-library behavior only in
the later foundation/feature harness; a test flag must not manufacture acceptance.
Accepted time1000ms, fresh current clock/readiness unless varied. UTC-boundary
rows keep host-elapsed retention unexpired to isolate the UTC comparison; a separate
elapsed cutoff always may refuse earlier. Reference letters
are symbolic legal byte references; actual new owner candidates are32 bytes.
Reset-race rows may prepare the separately authorized replacement hash before this
pipeline's admission, then commit the reset on the queue while verification/rehash
runs; they require no second overlapping crypto pipeline.

| ID | Independent input/order | Required oracle |
| --- | --- | --- |
| LA01 | No preexisting session; H0/empty password; all queue rechecks equal; normal readonly completion | One real accepted local object for Q/a at1000; no session/permission/owner change and no bearer. |
| LA02 | Directory absent, account absent, local credential absent, or access disabled | No acceptance/rehash; same non-enumerating public authentication refusal, no invented account creation. |
| LA03 | H0 with NUL byte instead of empty; or helper native −1 | No acceptance/check/hash/save; unsuccessful observation does not diagnose every cause as wrong password. |
| LA04 | Empty/unsupported/malformed fitting verifier,128-byte verifier, excessive stored cost, or protection/runtime failure | Typed internal record/resource/unavailable refusal; no native work where preflight forbids it, no acceptance and no public account detail. |
| LA05 | Native verified0, then r/H0→z/H1 reset before acceptance work | Full-source mismatch loses; no acceptance or rehash; H1 remains. Equal r with different bytes also loses. |
| LA06 | Verification pending; directory selection changes a→b, account access/revision changes, or either epoch/install revision changes | Refuse stale current authentication; never accept b or ignore full fences because a reference still matches. |
| LA07 | Verified0 plus cancelled/deadline-expired Q, wrong job/phase/source, foreign generation or body failure | No acceptance; late result cannot revive Q or create an object from helper success. |
| LA08 | Verified0; clock unavailable or known restore fence unacknowledged | Unavailable, no accepted timestamp. Caller trusted=true/checkpoint1000 cannot rescue it. |
| LA09 | Q's accepted t; independent request X supplies exact t and copied tuple | Membership/request association fails; no reservation or session, even if every DTO byte matches. |
| LA10 | Q reserves t for W; second W2 attempts t before W completes | At most W owns reservation; W2 cannot mint. A stale W completion cannot settle W2/new slot. |
| LA11 | W rolled back, Q still live/current, now301000 vs301001 | Explicit new internal dispatch may reserve at age300000; age300001 fails stale. No automatic retry or refreshed accepted time. |
| LA12 | W physically commits then ACK lost | outcome_unknown, poison t, no bearer/success/replay. Actual row may exist; neither rollback nor safe retry is assumed. |
| LA13 | W confirmed commit then body throws, wrong envelope or response loss | Mint consumed despite no success response; t cannot create a second session or restore the old digest. |
| LA14 | Reuse matching t after request end/process restart; normal stored epochs unchanged | No actual member exists/current; refuse. Restart preserves valid stored sessions but reconstructs zero attempts. |
| LA15 | Actual source callback receives proof belonging to S2; or valid S proof but no actual accepted login | denied; source identity/native proof alone never establishes acceptance. No retained proof/view. |
| LA16 | First real upgrade check accepted; reset/account/epoch/access changes during new hash | Second fresh check stale/denied, replacement discarded; new state preserved, original login not relabeled wrong password. |
| LA17 | Both checks accepted; reset changes full r/H0 before conditional save | Physical full CAS/current-fence and host membership refuse; reset wins, no old credential restoration. |
| LA18 | Ready replacement r/H0→z/H2; actual save known rollback | No upgrade success, old lineage retained only if still current; no lost-newer-credential overwrite or automatic retry. |
| LA19 | Same save known committed with normal exact matching completion | Only this accepted object's lineage becomes z/H2/fresh acceptance revision; accepted time1000, epochs and mint disposition stay unchanged. Lifecycle must load current lineage again. |
| LA20 | Same save physically committed with lost ACK, or known committed followed by body exception | Unknown poisons; known committed abnormal invalidates. Neither adopts lineage from intent/snapshot nor calls old-source retry safe. |
| LA21 | Actual accepted login; upgrade hash allocation/native generation unavailable, no source change | No replacement persisted; accepted login remains eligible only after normal current checks. No upgraded or wrong-password claim. |
| LA22 | Two intentional admitted logins Q then X, both optional upgrades fail as in LA21 without source change; both accepted before issue | Distinct actual objects/request bindings may each mint once; unrelated sessions are not silently revoked. Pipeline native calls still never overlap. |
| LA23 | X accepts r/H0, its optional upgrade generation/allocation fails (LA21), then its pipeline drains while request/acceptance stay live; Q next accepts r/H0 and legitimately commits z/H2 | Advance Q only; X's old lineage is stale for later mint/save. Existing issued sessions retain epoch/times. |
| LA24 | One active pipeline or all64 slots unavailable; input1025 bytes; message budget100 for the101-byte read | Refuse before another secret copy/queue/native call or oversized return ownership respectively; no eviction, partial success or lower cost. |
| LA25 | Cancel while worker waits in source; queued work later runs/completes | Wake worker with unavailable/cancelled, retain owned cell until safe drain; late completion settles actual truth, no stack write or acceptance. |
| LA26 | Deadline/disconnect occurs during native call | Logical refusal; no second admission or unsafe destruction. Buffer/source remain through native return, then are erased. No30-second erasure promise. |
| LA27 | accepted1000, trusted now999/301000/301001 | unavailable / fresh at age300000 / stale at300001. Monotonic retention may refuse earlier, never extend these UTC decisions. |
| LA28 | Foreign-thread store/session use or shutdown while source callback pending | Reject without foreign SQL; drain owner completions before joining/destroying worker/writer. No owner blocking on worker and no event pump. |
| LA29 | Recovery commits account epoch e→f for a; b unchanged; installation restore unresolved | a's old attempts/sessions invalid; b's account facts unchanged. Installation-wide restore hold still refuses both; recovery cannot clear clock/readiness hold. |
| LA30 | Accepted t mint-consumed, still-live source does a separately bound rehash check; later mint replay | Rehash can observe the same actual accepted lineage, subject to every current check; replay cannot reserve consumed t. No attempt revival. |
| LA31 | Host admits a stage at elapsed1000ms; queue deadline6000, pipeline deadline31000; dispatch at5999 vs6000 | First may enter with all current gates; equality cancels before binding/domain/BEGIN, preserves actual writer completion. No fresh5000/30000 deadline on wake; wall UTC changes neither cutoff. |
| LA32 | Stop/disconnect arrives during readonly source/acceptance work; body commits normally, or host's current-installation finalization becomes unknown | No accepted registry installation/positive source reply or later stage after stop/cancellation. Normal readonly commit stays committed; unknown invalidates host current sources and poisons affected use. Fresh completion membership is mandatory; dead execution facades/tickets cannot rescue it. |

## 10. Review and remaining gates

Different actual review must check the complete owner/request/registry membership,
source bridge, secret/thread/deadline/finite-frame arithmetic, all LA01–32 and exact
compatibility with the accepted imports. This document does not approve its own
independent review or establish an executable baseline. ACCESSTEST-008 must provide
a minimal reviewed API/harness, real disposable store/worker/queue integration,
independent selected-library password oracles, denial/race/failure/replay/lineage
assertions and intended explicit missing-behavior red after working foundations.
No new native/helper/foundation body runs in this document stage.

ACCESS-008 then implements only this narrow auth-specific composition and passes
its reviewed integration plus affected password/storage regressions, with separate
actual authority/secret/thread/resource/source review and selected CI. Original
receipts/assertions remain. The private one-cell bridge/worker is part of that
allocated slice; it grants no reusable server runtime. API-001's production host,
ACCESS-001's actual directory and SESSION-009's actual clock remain named prerequisites
in the existing graph (the latter depends on API-001), not implemented mechanisms
inherited from their DTOs. Ordinary startup uses the current host consumer boundary's
actual schema8 inspection/read chain; no STORAGE-007 runtime prerequisite is added
upstream. Supported restore must later satisfy that owner's guarded-restart/fence
obligation. SESSION-009 binds the actual host, never a ready flag issued by this
login owner; API-001 does not wait for ACCESS-008 or SESSION-009 to start serving.

If implementation needs a general executor/bridge beyond the accepted host boundary,
durable attempt survival/outcome journal or stronger UTC assurance, root must split
an explicit contract/test/implementation prerequisite with owner, acceptance and
focused PR before code. For example API-001 owns a generic bridge whose acceptance
must prove bounded admission, owner-only dispatch and shutdown/drain; a durable
acceptance owner would require separate crash/restart/replay/settlement tests and
reviewed persistence. None is silently absorbed here or required by this in-memory
request-scoped model. No new dependency, authorization service or provider behavior
is selected. Full setup/invitation/recovery/transfer, lifecycle/transport/ingress,
restore/reconciliation, supported Linux/Windows/client/package/platform/live and
hardware-cost gates remain open with their owners. All seven integrations and
all three FX providers remain required for release.
