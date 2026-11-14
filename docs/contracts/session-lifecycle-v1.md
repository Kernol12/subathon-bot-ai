# Account-session lifecycle — document proposal v1

SESSIONCONTRACT-003; specification v0.45 §§10/16, AUTH-01/05/09/10/11/12/15,
DB-01, SERVER-01 and TEST-04/05. Identity
`subathon.account-session-lifecycle.v1`. Base main `ebb4368`, claim `db6fbee`.
This is a proposed internal C++/port contract with literal examples. It supplies
no header, schema, tests, stub, implementation or authentication capability.
Actual independent document review and publication are recorded in the
[task handoff](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/sessioncontract-003-handoff.md); dependent source/test
stages retain their own gates.

Import [supplier v1](session-resolution-v1.md), blob
`c5f5a6e71664042e18475068d3caf577b0c8d009`, especially §3/4, and
[transport v1](session-transport-v1.md), blob
`2c33c2bbe9052455107c3056783d6bb605a5fef2`, especially §3/4, unchanged.
Import [password logical v1](password-hashing-v1.md),
[password API v1](password-hashing-api-v1.md),
[writer work v1](writer-transactions-v1.md) and
[effective permissions](effective-permissions-api-v1.md) unchanged; exact blobs
are in the [author record](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/sessioncontract-003-author.md).
[ADR-0010](../decisions/0010-session-executable-prerequisites.md) preserves
[full SESSION-002](../tasks/EPIC-007/SESSION-002.md) and
[full ACCESS-004](../tasks/EPIC-006/ACCESS-004.md).

## 1. Proposed commands and nonsecret results

Future namespace `subathon::server`, header `session_lifecycle.hpp`. Declarations
below are documentation only. Reuse actual `SqliteWriter`, `SqliteWorkContext`,
`SqliteWorkSession`, `TransactionOutcome`, `WorkError`, `StorageError`, and the
supplier's digest, facts, clock and resolved-session types. All new enums have
underlying `std::uint8_t`, consecutive values from zero in listed order.
References are nonempty opaque exact bytes, including NUL/non-UTF-8; no normalization.
They are correlation/selection/provenance data, never authentication capabilities.

```cpp
enum class SessionRetention : std::uint8_t { normal, remembered };
enum class AcceptedSignInMethod : std::uint8_t { local_password, verified_twitch };
enum class LifecycleFailure : std::uint8_t {
    invalid_limits, resource_limit, invalid_input, authentication_required,
    fresh_authentication_required, not_authorized, stale_authentication,
    stale_session, selected_target_missing, invalid_stored_data,
    unavailable, expiry_not_representable
};
enum class InspectedSessionState : std::uint8_t { active, revoked, expired };
enum class CredentialPublication : std::uint8_t {
    not_requested, suppressed, handed_to_emitter, delivery_unknown
};
struct LifecycleIngressBinding {
    std::string installation_ref, policy_revision_ref;
};
struct LifecycleActor {
    LifecycleIngressBinding ingress;
    AccountSessionDigest original_digest;
};
struct IssueAccountSession {
    LifecycleIngressBinding ingress;
    std::string accepted_attempt_ref;
    SessionRetention retention = SessionRetention::normal;
};
struct RotateAccountSession { LifecycleActor actor; };
struct ReauthenticateAccountSession {
    LifecycleActor actor;
    std::string accepted_attempt_ref;
    SessionRetention retention = SessionRetention::normal;
};
struct LogoutAccountSession { LifecycleActor actor; };
struct RevokeAccountSession {
    LifecycleActor actor;
    std::string target_account_ref, target_session_ref;
};
struct InspectAccountSessions {
    LifecycleActor actor;
    std::string target_account_ref;
    std::optional<std::string> after_session_ref;
    std::uint64_t page_size = 64;
};
using SessionLifecycleCommand = std::variant<IssueAccountSession,
    RotateAccountSession, ReauthenticateAccountSession, LogoutAccountSession,
    RevokeAccountSession, InspectAccountSessions>;

struct SessionLifecycleLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t request_bytes = 65536;
    std::uint64_t source_bytes = 1048576;
    std::uint64_t result_bytes = 1048576;
    std::uint64_t page_rows = 64;
    SessionLimits resolution; // Its unchanged independent string-payload profile.
    subathon::core::EffectivePermissionLimits permission;
};
struct SessionCredentialMetadata {
    std::string installation_ref, account_ref, session_ref, record_revision_ref;
    SessionRetention retention;
    std::int64_t lifetime_started_utc_ms, authenticated_utc_ms, expires_utc_ms;
};
struct SessionRevocationMetadata {
    std::string installation_ref, account_ref, session_ref, record_revision_ref;
};
struct InspectedAccountSession {
    std::string session_ref, record_revision_ref;
    InspectedSessionState state;
    bool is_requesting_session;
    std::int64_t lifetime_started_utc_ms, authenticated_utc_ms, expires_utc_ms;
};
struct SessionInspectionPage {
    std::string installation_ref, account_ref, snapshot_ref;
    std::int64_t observed_utc_ms;
    std::vector<InspectedAccountSession> sessions;
    std::optional<std::string> next_after_session_ref;
};
using SessionLifecycleReply = std::variant<SessionCredentialMetadata,
    SessionRevocationMetadata, SessionInspectionPage>;
struct SessionLifecycleCompletion {
    TransactionOutcome transaction;
    WorkError work_error;
    std::optional<LifecycleFailure> failure;
    std::optional<SessionLifecycleReply> reply;
    CredentialPublication publication;
};
```

Each command is a different trusted route/operation binding; clients cannot choose
an internal variant to bypass transport or authentication. The original digest is
the digest admitted from that actual request, never a client-supplied digest or a
later lookup by session identity. Ingress binding comes from the actual trusted
host/transport, including the pre-session guard for browser sign-in. Native sign-in
retains its owning route's actual ingress checks. No password or raw bearer is
captured by these commands. Optional cursor absent differs from present-empty.

The final service proposal is:

```cpp
class SessionCredentialEmitter {
public:
    virtual ~SessionCredentialEmitter() = default;
    // Synchronous borrow, at most once; no queue, retained view or secret logging.
    virtual CredentialPublication emit(
        std::span<const char, 64> token_hex,
        const SessionCredentialMetadata&, SessionClockSample publication_time) = 0;
};
struct SessionLifecyclePreparedWork {
    SqliteWriter::Work body;
    SqliteWriter::WorkCompletion completion;
};
class SessionLifecycleService final {
public:
    using Completion = std::function<void(SessionLifecycleCompletion)>;
    SessionLifecycleService(SqliteWriter&, SessionLifecyclePorts&,
                            SessionLifecycleLimits = {});
    SessionLifecyclePreparedWork make_work(
        const SessionLifecycleCommand&, SessionCredentialEmitter*, Completion) const;
    StorageError enqueue(
        const SessionLifecycleCommand&, SessionCredentialEmitter*, Completion);
};
```

The constructor is inert. `SessionLifecyclePorts` is the trusted, owned dependency
bundle in §2; emitter is required exactly for issue/rotate/reauthenticate and absent
for the other commands. Body/completion form one matching pair, submitted once to
the real writer. Host, service, writer, ports, emitter and completion dependencies
outlive every prepared pair, including unsubmitted pairs, through completion or
destruction. Foreign calls reject `wrong_thread` before copying input or touching
ports/SQL. Missing completion or invalid emitter presence rejects `invalid_work`.
`make_work` throws fixed `SessionLifecyclePreparationFailure(StorageError)` for
these rejections; `enqueue` returns that error. Owner-thread input/limit errors
throw fixed `SessionLifecycleInputFailure(LifecycleFailure)` before admission.
Neither exception carries input bytes. Rejected admission invokes no body or
completion. Accepted admission means queued only; no automatic replay occurs.
Preparation allocation failure is `SessionLifecycleInputFailure(unavailable)`.

## 2. Trusted current sources and conditional persistence

Every port below is bound by the actual host to this work identity and its ONE
actual active `SqliteWorkSession`. Bindings are synchronous and revocable; retaining
a facade cannot extend the transaction, work, thread or writer lifetime. No port
opens another connection/writer, queues nested work, pumps events, performs network
I/O, hashes a password, or treats a request reference as current authority.

```cpp
struct LifecycleCurrentSession {
    CompleteSessionFacts current;
    SessionRetention retention;
};
struct AcceptedSessionAuthentication {
    std::string installation_ref, account_ref, attempt_ref;
    std::string installation_epoch_ref, account_epoch_ref, account_revision_ref;
    std::string authenticator_revision_ref, acceptance_revision_ref;
    AcceptedSignInMethod method;
    std::int64_t accepted_utc_ms;
};
struct CurrentAcceptanceFacts {
    AcceptedSessionAuthentication accepted;
    CurrentAccountFacts account;
    CurrentInstallationFacts installation;
    std::string account_revision_ref, authenticator_revision_ref;
    std::string acceptance_revision_ref, snapshot_ref;
};
struct SessionLifetimePolicy {
    std::string revision_ref;
    std::uint64_t normal_ms = 86400000, remembered_ms = 2592000000;
};
struct LifecycleRecordIdentity {
    std::string session_ref, record_revision_ref;
};
struct LifecyclePageFacts {
    CurrentAccountFacts account;
    CurrentInstallationFacts installation;
    std::string snapshot_ref;
    std::vector<SessionRecordFacts> sessions; // At most page_size+1, no digest export.
};
enum class LifecycleAccess : std::uint8_t { inspect_account, revoke_session };
struct LifecycleAccessRequest {
    std::string_view actor_account_ref, target_account_ref;
    LifecycleAccess action;
};
enum class LifecycleSourceFailure : std::uint8_t {
    missing, denied, stale, unavailable, invalid_data, resource_limit
};
template<class T> using LifecycleRead = std::variant<T, LifecycleSourceFailure>;
enum class LifecycleWriteResult : std::uint8_t { staged, stale, collision };
class SessionLifecycleUnit;
class SessionLifecyclePorts {
public:
    virtual ~SessionLifecyclePorts() = default;
    virtual std::unique_ptr<SessionLifecycleUnit> bind(
        const SqliteWorkSession&, const SessionLifecycleLimits&) = 0;
    virtual CurrentSessionClock& clock() = 0;
};
```

Forward declarations may order these document declarations in a future header;
all proposed wrappers have public destructors and no secret accessor. The actual
clock returned by ports is bound to this authoritative execution/publication
scope, not a supplied timestamp or a client trust flag.
LifecycleAccessRequest is a synchronous borrow of the already-owned command/
resolved references, never a captured/queued DTO or an extra owning input clone.

Proposed `SessionLifecycleUnit` methods below borrow arguments until return and
return one owned bounded value, or fixed failure. `SessionLifecyclePorts::bind(
const SqliteWorkSession&, const SessionLifecycleLimits&)` supplies one owner-thread
RAII `unique_ptr<SessionLifecycleUnit>`.
The unit cannot call begin/commit itself; destruction releases borrows, while actual
session cleanup/finalization remains §7. Type names in method arguments above are
fully defined; the method table is part of the C++ proposal, not arbitrary SQL.

| Unit method | Return and required meaning |
| --- | --- |
| `check_ingress(const LifecycleIngressBinding&)` | `LifecycleRead<std::monostate>`; require same actual current installation/policy revision. A changed revision is unavailable, requiring a fresh actual request under transport §3. |
| `load_actor(const SessionLookupRequest&)` | `LifecycleRead<LifecycleCurrentSession>`; exact installation+original digest index, coherent complete supplier facts plus stored retention. Missing is only absent selected row. |
| `load_accepted(std::string_view attempt_ref)` | `LifecycleRead<CurrentAcceptanceFacts>`; actual accepted sign-in owned by the real local/provider authentication owner, current linked facts and source-bound attempt reservation described below. |
| `load_lifetime_policy()` | `LifecycleRead<SessionLifetimePolicy>`; current trusted owner configuration. Both positive U64 durations and nonempty revision are validated, even if only one duration is selected. |
| `new_record_identity(std::optional<std::string_view> session)` | `LifecycleRead<LifecycleRecordIdentity>`; absent for issue gives a proposed new identity/revision; present keeps that exact session and supplies a different fresh revision. Nonempty checked references, no guaranteed uniqueness until the conditional write. |
| `check_access(const LifecycleAccessRequest&)` | `LifecycleRead<std::monostate>`; current exact actor/operation/target permission and current disclosure/workflow guards, not a supplied previous permit. `denied` means evaluated refusal; unknown/partial facts are unavailable. |
| `load_selected(std::string_view installation, std::string_view account, std::string_view session)` | `LifecycleRead<CompleteSessionFacts>`; one exact linked selected record after authorization. Missing does not disguise a bad link or source failure. |
| `load_page(std::string_view installation, std::string_view account, const std::optional<std::string>& after, std::uint64_t page_size)` | `LifecycleRead<LifecyclePageFacts>`; bounded indexed keyset read with one probe, strict length/type preflight and §6 ordering. |
| `insert_session(const SessionRecordFacts&, SessionRetention, const AcceptedSessionAuthentication&)` | `LifecycleWriteResult`; unique installation+digest and session identity, current epochs/accepted bindings, no existing-row adoption. |
| `replace_session(const SessionRecordFacts& expected, const SessionRecordFacts& replacement, SessionRetention, const AcceptedSessionAuthentication* accepted)` | `LifecycleWriteResult`; accepted is absent for token-only rotation and present for fresh reauth; CAS of exact source record/revision/digest/state/epochs, immutable identities and current account/install/acceptance fences. Collision never overwrites another row. |
| `revoke_session(const SessionRecordFacts& expected, std::string_view replacement_revision)` | `LifecycleWriteResult`; same conditional source/fence binding, active → revoked with changed revision, exact row only. |

For every read, unavailable→unavailable, invalid_data→invalid_stored_data, and
resource_limit→resource_limit. Other source failure translations are closed:
load_actor permits only missing→authentication_required; load_accepted permits
missing/denied/stale→stale_authentication; check_access permits denied→not_authorized;
load_selected permits missing→selected_target_missing; load_page permits
missing→selected_target_missing only for a nonexistent selected account. A missing
account already linked by actor/selected facts is invalid_data, never missing.
The remaining methods permit none of missing/denied/stale; an unexpected variant
is unavailable. This is not permission to hide corruption as unavailable: actual
known malformed/dangling source facts must return invalid_data. A changed ingress
policy returns unavailable, never stale. Successful monostate reads have no payload.

The unit is configured with the complete validated limits and actual per-call read
scope; it must reject a source projection before allocating an oversized value.
Selected reader and writer physical codecs, identities, revisions and indexes are
[STORAGECONTRACT-002](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/STORAGECONTRACT-002.md), not chosen here. No added
SQL permission, table, migration, proof serialization or revision-generation
algorithm is implied. Fresh nonempty session/revision identities are supplied by
that trusted store owner; session identity collision is unavailable. Identity
production cannot use a timestamp alone or silently reuse an existing identity.

`load_accepted` cannot be a wrapper over `verify_password`, `PasswordVerification`,
`PasswordVerifiedSource`, `PasswordUpgradeResult`, a Twitch name, client flag or a
caller-constructed principal. Its real owner must retain this exact attempt's
successful authentication and all actual acceptance conditions. It must bind this
command to the real server-owned login handoff and request association; selecting
another accepted_attempt_ref cannot borrow another person's successful login.
For local sign-in,
off-writer verification must match the real current account's COMPLETE credential
and exact revision, account access, installation and epochs when acceptance returns
to the queue; reset, account change or stale credential work cannot become accepted.
The source checks the full source verifier/binding, without exporting it here.
A legitimate conditional post-login rehash must preserve that same acceptance's
lineage through the real source owner; arbitrary revision mismatch is never ignored.
This boundary works with no preexisting account session. Password-helper success
alone authenticates no account and cannot authorize credential persistence.

Local accepted-attempt ownership/lifetime/revocation and the actual password source
remain [ACCESSCONTRACT-001](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-006/ACCESSCONTRACT-001.md)/ACCESS-008.
The local path depends on neither ACCESS-004 nor Twitch. The second method tag is
reserved for actual verified Twitch evidence under ACCESS-004's separate reviewed
baseline: stable linked user/account identity, verified current authentication,
current link/authenticator revision, account/install fences and replay protection.
An unimplemented provider path returns unavailable; the tag supplies no provider
support. App sign-in is distinct from broadcaster/bot authorization. Issuance never
changes permissions, local credentials, protected owner or provider links.

Source storage/native exceptions must preserve the actual writer outcome and
first sticky error. The write result enum reports only a completed conditional
decision; it cannot hide a SQL/commit/cleanup failure as stale/collision.

The authentication owner reserves one actual accepted attempt for this issuance/
reauthentication work. A reference cannot replay a completed attempt to mint more
credentials. The owner keeps the reservation through completion: committed or
unknown outcome makes it unusable; known rollback may release it only if its
original acceptance is still current and fresh. Reservation/consumption must agree
with the actual transaction outcome; no early consume followed by a false success,
rollback-induced resurrection, or unknown-outcome retry. This is a logical handoff
obligation, not a new durable attempt table or transferable proof format. The actual
producer contract must choose/review how its retained state provides this guarantee.

## 3. Finite ownership and arithmetic

All five lifecycle limits are positive, <=SIZE_MAX and <=PTRDIFF_MAX, and fit their
destination containers' `max_size` with required terminator arithmetic. Check
`page_rows+1`, row-count conversions and `request_bytes+5*source_bytes+2*result_bytes`
without overflow before work. Also validate the nested unchanged supplier and
effective-permission profiles by their own rules. Invalid → invalid_limits.
There is no account/session-count quota, fixed reference format or lifetime ceiling
below positive U64. A larger checked page/profile is allowed by configuration.

For this document's DTO budget B, string contributes its byte length; enum/bool
contributes1; u64/i64 contributes8; digest contributes32; struct sums all declared
fields; optional contributes1 plus the present value; vector sums elements;
variant contributes1 plus its selected value. No native padding/capacity/container
header is counted. Request B is for the complete command including its variant
tag. A source value B includes its success/failure variant tag. Reply B includes
its variant tag. Limits and native object overhead are outside B. Only the page
has a variable collection, at most `page_size+1` source rows and `page_size` result
rows; no recursive values or omitted losing strings exist.

Before copying, inspect borrowed complete request B and every reference against
request_bytes/reference_bytes. Sources preflight every selected reference/type/
aggregate before construction against source_bytes/reference_bytes. Validation
borrows source values; retain at most FOUR full source success values (reauth's
actor, acceptance, policy and new identity), plus ONE source-sized proposed record,
fixed resolved actor metadata and one output. Other operations use fewer values;
there is only one page read, never a page collection. The fixed actor's five
references must fit supplier result limits, and its complete B must fit result_bytes.
Before growing output,
project COMPLETE reply B, including repeated references and cursor, against
result_bytes. Any additional owning DTO copy requires its own prior projection;
move bounded handoffs and store expected CAS bindings in already budgeted values.
At most request_bytes + 5*source_bytes + 2*result_bytes semantic DTO bytes coexist.
The replacement record is separately projected against source_bytes before growth;
it is not an uncharged clone of loaded facts. The second result allowance covers the
fixed resolved actor while the reply is materialized. Writer SQL row/binding/
statement cumulative limits and imported permission-producer work budgets remain
additional independent bounds; none becomes a process-memory or hard CPU claim.

Full supplier facts retain their original independent per-value STRING-payload
limits as well as these lifecycle DTO bounds. The permission source uses the
selected `permission` profile to bound its actual coherent loading/evaluation
before materialization, not merely its tiny returned monostate. Malformed or
oversized hidden policy facts cannot be silently omitted. Sources have no
unbounded history/table scan or speculative retry loop. Allocation/unclassified
native exceptions become unavailable, except actual writer errors keep §7 truth.

Token generation/hash/encoding reuse ONLY the accepted supplier's selected
primitives/format. A candidate has one private fixed32-byte raw buffer, fixed60-byte
frame, fixed32-byte digest and fixed64-byte lowerhex output:188 application byte
slots at peak, plus fixed counters/owner state and separately audited native
scratch. Wipe raw/frame on every path immediately after their last use; wipe the
64-byte bearer on destruction or after emitter return. No dynamic token string,
raw secret command, source return, public reply, audit, log or retry cache exists.
No more than one candidate is generated per work; collision refuses rather than
looping. A private copyable Qt envelope may share ONE fixed secret owner to satisfy
QVariant mechanics, but copies must not clone its secret buffer. The later source/
native review must bound envelope/control-block/framework copies and cleanup;
188 is not a total Qt/libsodium/RSS claim. Import sodium erasure/platform limitations.

Compute signed expiry as mathematical start + positive U64 duration, then check
INT64_MIN..INT64_MAX; never narrow duration to i64 first, overflow, saturate or use
floating point. Full-domain nonnegative time differences fit U64 when the two i64
times are ordered, including INT64_MIN..INT64_MAX. Fresh is inclusive age<=300000;
expiry is exclusive now<expires. No local timezone, calendar-month or DST arithmetic.

## 4. Exact staged decision priority

Finish a stage before proceeding; stop at its first applicable failure. No later
producer is called after refusal. Within a source's complete DTO: byte/row budgets
precede enum/required-field/link shape, which precedes epoch/state/freshness checks.
An unexpected source failure variant for that operation is unavailable. Fixed
failure enums contain no token/digest/account/source diagnostic. Actual writer
unknown/sticky errors supersede any local intended refusal under §7.

1. Owner/thread/callback/emitter checks and pure limits, complete request size,
   then all command tag/reference/cursor/page shape checks. Too large →
   resource_limit; invalid tag, empty required field, present-empty cursor,
   page_size=0 or page_size>page_rows → invalid_input. No SQL/port call occurs.
2. Admit once to the real writer. Admission error is not a lifecycle success.
   Body calls context.begin ONCE, binds that actual session, then check_ingress
   ONCE. Unknown or changed policy is unavailable, no authentication fallback.
3. For every command except issue: load_actor ONCE by original digest; perform
   unchanged supplier §3 resolution/clock/error priority, including malformed
   facts before revoked, then clock, expiry and required freshness. This lifecycle
   profile itself adds no fresh-auth requirement to rotation/logout/inspection/
   individual revocation; sensitive enclosing workflows keep their own guards.
   Missing/revoked/expired → authentication_required; supplier freshness refusal →
   fresh_authentication_required; unavailable/corrupt/resource remain distinct
   unavailable/invalid_stored_data/resource_limit. Retention enum is validated
   with all actor facts before state checks. Do not refresh identity by session_ref.
4. Issue/reauthenticate: load_accepted ONCE; missing/denied/stale →
   stale_authentication. Validate all current/accepted bindings and revisions.
   Broken links/enum/empty data → invalid_stored_data. Different requested
   installation/attempt, disabled account, unequal account/installation epochs,
   account/authenticator/acceptance revisions, or a different reauth account →
   stale_authentication. An actual valid login for B cannot refresh A. For issue
   sample trusted current clock once here; reauth uses stage3's same sample.
   now<accepted_utc_ms → unavailable; age>300000 → stale_authentication. The
   acceptance source must establish actual fresh sign-in, not relabel a cached
   password result with a newer time. Reserve current acceptance through §7/8.
5. For revoke/inspect, resolve target authority BEFORE any selected-session/page
   lookup. Exact target_account==resolved actor.account permits the intrinsic
   self operation. Others require current check_access for this exact account
   and inspect/revoke intent. `denied` → not_authorized; unavailable/resource/
   invalid data remain source failures. An Administrator/Owner label or older
   evaluation is insufficient. Protected-owner classification follows the real
   trusted operation catalog; this proposal assigns no editable role shortcuts.
6. Issue/reauthenticate load/validate complete lifetime policy; zero duration or
   malformed configuration → invalid_stored_data. Compute checked chosen expiry;
   out of signed range → expiry_not_representable. Rotate uses its recorded
   policy/times and does not load current lifetime configuration. Revoke loads
   selected after authorization; absent → selected_target_missing. Inspect
   loads one bounded page after authorization. Validate complete loaded facts;
   broken links or out-of-scope/duplicate/unsorted rows → invalid_stored_data.
   A mutating active-row operation obtains new_record_identity once. Validate
   all its lengths, required references, preserved selected identity and different
   revision before result projection; invalid identity facts are invalid_stored_data.
7. Project complete result/expected-write materialization before generating a
   token or staging mutation. Over budget → resource_limit. Stage the exact
   conditional mutation (§5) once. `stale` → stale_session; any collision →
   unavailable. There is no identity adoption, automatic retry or partial page.
8. Recheck the actual source-bound acceptance/current account/install/permission
   fences as part of the conditional mutation/transaction's serialized authority
   scope, commit synchronously, and return the exact private typed envelope
   normally. For inspection commit its read-only transaction after the same
   current disclosure checks. Do not yield between authority and commit.

The execution sample is fixed for this indivisible work. If a source/operation
waits or releases this scope, discard the proposal and re-resolve current authority;
retaining a result does not keep rights. Publication has a separate sample (§8).
Invalid source data is never hidden by a deny or an expired timestamp. Error
distinctions here are internal; transport §4's non-enumerating errors/challenge/
freshness headers remain unchanged. This document assigns no HTTP route, body
schema or new public mapping for lifecycle conflicts/unknown writes.

## 5. Exact mutation behavior

Issue: create one distinct session with immutable installation/account identity,
current epochs, fresh identity/revision, new random credential and digest-only
record. Set lifetime_started=authenticated=execution now and expires=now+chosen
duration. Default durations are exactly86400000 and2592000000ms; owner configuration
is positive U64. Existing sessions keep recorded maximums when configuration changes.
Unrelated sessions remain active; multiple intentional sign-ins create individually
inspectable/revocable sessions. No permission, ownership or recovery state changes.

Rotate: require a currently valid original digest and CAS the same selected session.
Preserve session/account/installation identity, both epochs, lifetime_started,
authenticated, expires and retention EXACTLY. Change only digest and record revision
to fresh values. Reject even a generated candidate equal to its own old digest as
a collision. Atomic replacement removes old digest acceptance at the same commit;
there is no overlap, sliding renewal, timed automatic rotation or freshness upgrade.

Reauthenticate: require the current original session AND a separately accepted fresh
sign-in to that same account. A stale/expired/revoked original session instead uses
ordinary new sign-in/issue; it is not revived through its identity. CAS replacement
keeps this session's identity/account/installation and current epochs, replaces
digest/revision and retention, and sets all three lifetime/authentication times from
this accepted sign-in exactly as issue does. No second active old credential remains.
This is the explicit fresh-sign-in renewal; token-only rotation cannot invoke it.
Other sensitive operations still require current permission and their own workflow,
local credentials where required, and single-use/expiry/two-party conditions.

Logout targets exactly the requesting current session. Individual revoke targets
the authorized selected session; it cannot revoke an entire account implicitly.
For an active selected row CAS active→revoked and change revision, preserving
identity, digest and all times. An already revoked selected row may return its
unchanged revocation metadata after authorized current reading and normal read-only
commit; it never returns a bearer. Expired or epoch-invalid active rows may still
be explicitly marked revoked. A request authenticated by an already revoked/expired
session fails stage3, including repeated logout. A still-valid OTHER session can
revoke/inspect the old row. This keeps authority ahead of idempotent disclosure.

Recovery changes the account session epoch in the same recovery commit, preserving
permissions/owner identity. Restoration replaces the installation epoch before any
restored access; normal unchanged startup preserves it. These operations remain
ACCESS-005/STORAGE-007. The store/CAS checks current epochs so an attempt captured
before either fence cannot issue/replace a current session afterward. Logout,
revocation and expiry affect application access; the subathon process/timer/
integrations and independent configured disconnect policy remain their own scope.

## 6. Inspection semantics and bounded pages

Each request inspects ONE exact installation/account, authorized at execution.
The current reader orders session_ref by unsigned-byte lexicographic order,
shorter exact prefixes first, and returns strictly greater than after_session_ref
when present. A cursor is only a nonsecret boundary, not a permission, trusted
database cursor, page number, snapshot lease or requirement that that row exists.
Use a bounded index, not load-all then sort/truncate. Select at most page_size+1
metadata rows. The probe is validated/preflighted too; a malformed probe is a
source failure rather than a silently concealed row.

Result has the first at most page_size rows. next_after_session_ref is present
exactly if the probe establishes another row, and then equals the LAST emitted
session_ref. Empty/end page has no next cursor. No total-count query, raw token,
digest, accepted-attempt/credential record or permission explanation is exported.
All listed rows retain valid time order lifetime_started<=authenticated<expires
and exact account/installation bindings. Snapshot reference identifies this page's
coherent actual read; it cannot authorize the next request.

Use the same trusted execution time and linked current account/install facts for
every emitted row. Effective state is revoked if row revoked, account disabled, or
either epoch differs; otherwise expired if now>=expires; otherwise active.
For an otherwise active future-authenticated row, now<authenticated or lifetime start
is unavailable, not guessed active. `is_requesting_session` is true exactly when
both installation and session identity equal the currently resolved actor; it
does not reveal a digest. Pages include active/revoked/expired sessions without
silently hiding another device. No device/IP/User-Agent/name fields are invented.

Each page is independently current, including its disclosure permission. Insertions
or revocations between page requests can change later results; no multi-request
snapshot, exhaustive one-time export or retained permission is promised. An insertion
at/below the previous boundary may require restarting enumeration. Metadata retention/
eventual cleanup needs its own storage policy; a page ceiling is never a quota on
the number of valid sessions. No account-wide admin directory is added by this API.

## 7. One real work, commit and outcome truth

One admitted pair → one real writer work → at most one real BEGIN IMMEDIATE and
one actual synchronous commit attempt. Unit reads, acceptance reservation, current
permission checks, CAS and hash-only persistence compose on that SAME session.
No deferred outer commit/vote, second connection, nested transaction or next work
runs inside the body. All read and mutation results are provisional until actual
terminal completion. Body rejection rolls back or remains not_started, carrying
only its fixed lifecycle failure to matching completion; it is never a success.
Nonthrowing RAII cleanup calls actual session cleanup only while needed; committed/
terminal cleanup is a no-op. Owner finalization still protects abandoned wrappers.

Private writer value must be the EXACT registered lifecycle envelope type containing
the actual reply and, for credential operations only, its private secret owner.
Validate its disposition, originating work identity and selected-command reply
kind without QVariant coercion. Expected replies are credential metadata for
issue/rotate/reauth, revocation metadata for logout/revoke, page for inspect.
A missing/wrong/invalid envelope is unavailable with no reply/secret publication,
even if physical commit is known. Never reconstruct it from captured intent,
accepted flags, staged rows, an old reply or another work's secret. The matching
completion receives the real SqliteWorkResult; the host cannot synthesize it.

| Actual writer outcome | Lifecycle completion and secret |
| --- | --- |
| not_started admission/begin failure | No reply/publication; actual admission/work error retained; no implicit replay. |
| Known rollback after domain refusal or SQL/budget/commit failure | No reply/publication; fixed lifecycle failure if applicable plus actual first work error. Candidate erased; retry only a separately admitted still-current accepted attempt/request. |
| COMMIT failed but no commit plus rollback established | rolled_back, actual commit/work error; never claim that an arbitrary COMMIT error proves rollback. |
| Rollback/commit/cleanup cannot establish outcome | outcome_unknown/uncertain_outcome, no reply/publication; writer fail-closes and queued work fails not_started in order. Attempt/secret not replayed. |
| Confirmed COMMIT, then body throws or protocol fails | committed plus actual work error, no reply/publication; no rollback/retry claim; candidate erased. |
| Confirmed COMMIT and normal body result, wrong/missing private envelope | committed, bridge unavailable, no reply/publication; orphan handling required. |
| Confirmed COMMIT, normal exact envelope, publication gate fails | committed; no credential reply or bearer publication; failure unavailable and publication suppressed. State remains committed. |
| Confirmed COMMIT, normal exact envelope and successful required publication gate | Exact reply; credential emission at most once, otherwise not_requested. |

Unknown transaction truth has first priority, then the writer's first sticky work
error, then an uncaught body exception, then local domain/bridge/publication refusal.
Preserve both transaction and error fields; no source exception can change a real
committed fact to rollback. Known domain failure does not mask failed cleanup.
Externally thrown completion/emitter code cannot undo a transaction. `failure`
contains the applicable internal lifecycle refusal or unavailable for a bridge/
publication failure; reply is absent on all those paths. A normal completed revoke/
inspect has no failure and publication=not_requested. Other failed credential
operations use publication=suppressed. Work-error-only failures need not invent
an additional lifecycle diagnosis.

## 8. Normal-completion-only secret publication

Only committed + WorkError::none + actual normal exact envelope permits credential
publication. The private bearer is inaccessible to ports, public replies and the
host before this check. The authoritative host must preserve one nonreentrant
queue turn from the final authority/transaction checks through matching completion
and synchronous emitter call; no event pumping, await, network wait or later state
command may interleave. If the bridge cannot preserve that scope, suppress/wipe
the bearer and require a new actual sign-in; it cannot claim an old scope current.
Ingress transport revision/authority was checked in the actual work. A suspended
later response cannot recover authority from a session_ref or frozen permit.

Immediately before emitter invocation, sample the actual trusted publication clock
once, independently of execution. Unavailable/unresolved clock, time before the
recorded authenticated/lifetime-start/execution time, or now>=expires suppresses
publication; never extend expires or resurrect an old digest. Clock production/
recovery semantics remain SESSIONCONTRACT-004/SESSION-009. The nonsecret reply
and emitter receive the exact committed metadata, never a new lifetime calculation.
Emitter may return only handed_to_emitter or delivery_unknown; invalid return or
exception is delivery_unknown. It receives one borrowed64-byte lowerhex value,
must not retain the span, and its owning adapter must account for/erase necessary
bounded output copies. Erase the service secret after return even on exception.
handed_to_emitter means local handoff, not browser/network receipt. delivery_unknown
retains committed truth and no successful public reply claim; reply is absent and
failure=unavailable. No second emission is attempted.

Browser emitter imports the exact protected cookie, no JSON bearer, no-store and
finite header/envelope budgets from transport §4. Normal retention omits cookie
Max-Age/Expires. Remembered retention computes ceil((expires-publication_now)/1000)
with checked full-domain arithmetic and no narrowing/round-up overflow. Native
emitter uses its separately reviewed secret response body and finite budget; no
native success schema is chosen here. Desktop persistence remains DPAPI/QtKeychain
with no plaintext/password fallback. Acknowledged logout commits revocation before
the transport deletes its cookie/native token; deletion alone proves no revocation.
Unknown/failing logout never manufactures acknowledgment or clears credentials as
if persistence succeeded. Origin/CSRF failures do not change cookies.

Committed response loss, publication refusal, process exit or post-commit exception
can leave an unreachable valid session/new digest. Server stores no recoverable raw
bearer, old digest is never revived, and an outcome-unknown replacement is never
repeated blindly. Reconciliation must establish actual digest-only durable state
through the store/operational owner, fence access if unresolved, and permit new
fresh sign-in plus authorized inspection/revocation or conservative orphan cleanup.
Cleanup cannot assume the client failed to receive a response or revoke unrelated
sessions. No automatic cleanup algorithm, outcome journal or remote exactly-once
delivery is selected here; the full SESSION-002 tests must exercise these branches.

## 9. Independently derived literal examples

These are proposed oracles, not executed service tests. Synthetic references and
digests only. Base actor: installation `i`, account `a`, session `s`, all current/
row epochs `e`, record revision `r`, snapshot `p`, enabled/active, normal retention,
digest D0 (supplier's all-zero token vector), lifetime_started=authenticated=1000,
expires=86401000. Accepted attempt `t` is REAL-source-simulated local acceptance
for that same account/install, accepted time1000, matching credential/account/
acceptance revisions; current ingress policy matches. Any mutation gets a fresh
revision `q`; candidate D1 is independently distinct from D0. A fixture supplies
the actual source state directly; neither a password helper nor this service
generates an expected result. Fault injection needs later reviewed harness/source.
For the full-domain issuance arithmetic rows, the accepted time/current facts
are varied consistently with the listed execution start; they do not reuse an
ancient acceptance at1000. Rows explicitly testing stale acceptance keep its time.

| ID | Independent input/order | Required oracle |
| --- | --- | --- |
| LC01 | Issue normal at1000 | started=authenticated=1000, expires86401000; new session only, current epochs, hash-only; no secret before normal committed completion. |
| LC02 | Issue remembered at1000 | expires2592001000; other sessions unchanged. |
| LC03 | Owner changes normal duration to2000; old row still exists | New issue at1000 expires3000; old row retains86401000. |
| LC04 | Start=-9223372036854775808, duration18446744073709551615 | Expiry9223372036854775807 is representable; the U64 duration must not narrow to -1. |
| LC05 | Same start, duration18446744073709551614 | Expiry9223372036854775806; duration0 invalid_stored_data. |
| LC06 | Normal start9223372036768375807, then start+1 | First expires9223372036854775807; second expiry_not_representable, no RNG/write. |
| LC07 | Rotate base row at301001 | Same session/times/epochs/retention; age300001 stays nonfresh; D0 invalid immediately after D1 commit, no overlap. |
| LC08 | At301000 versus301001 require freshness in enclosing sensitive workflow | Age300000 fresh, age300001 fresh_authentication_required; rotation cannot repair it. |
| LC09 | Accepted fresh reauth at301001, normal | Same session, started=authenticated301001, expires86701001, new digest/revision; D0 invalid. |
| LC10 | Reauth accepted account b against actor a | stale_authentication before candidate generation; neither session changes. |
| LC11 | Reset changes credential revision or account epoch while password verification runs | Old result is not accepted; if previously accepted tuple becomes stale before work, stale_authentication, no issue/rehash overwrite. |
| LC12 | Installation restored epoch e→f before issue/queued use | Old acceptance fails stale_authentication; old session fails authentication_required; no restored raw credential is accepted. |
| LC13 | Rotate R then revoke V both admitted with original D0, R commits first | R commits D1; V's stage3 original D0 is missing, authentication_required; no rebind by session_ref. |
| LC14 | V is authenticated by independent valid admin session, R then V target s | V reads current D1 row and revokes it when currently authorized; D1 then fails. An adapter's stale CAS r against q returns stale_session, never overwrite. |
| LC15 | Self logout V commits first, then queued rotate R from D0 | R fails authentication_required, no RNG or replacement; subathon continues. |
| LC16 | Any protected operation queued with D0 executes after acknowledged rotation | Reject original D0; its earlier resolved account/session DTO cannot grant access. |
| LC17 | Candidate equals old D0, another existing digest, or new identity collides | unavailable, whole work rolled back, no token publication/adoption/retry loop. |
| LC18 | Statement fails after staged issue, known rollback | No durable new row, no public reply/bearer; actual first writer error retained. |
| LC19 | Inject physical commit then lost acknowledgment | outcome_unknown, no bearer/reply, writer fail-closed; not confirmed rollback and no new token retry. |
| LC20 | Confirmed commit then outer body throws | committed/work_exception, new digest persists, no public reply/bearer; old digest not revived. |
| LC21 | Confirmed commit, normal reply, network response lost | One digest-only active row may remain; emission not retried, new sign-in/authorized inspection/revocation remains possible. No server raw-token recovery. |
| LC22 | Commit at1000; trustworthy publication now86401000 or unavailable | committed, suppressed bearer/no credential reply; maximum never extends. A remembered fixture configured with this same expiry and publication86400999 has remaining1ms, Max-Age1. |
| LC23 | Target account b denied, selected session missing OR present | Same not_authorized; selected lookup/page not called. Guessing session_ref grants no existence disclosure. |
| LC24 | Admin permitted on admission, permission revoked before execution | Current denial before selected lookup, no revocation/list disclosure. Current owner change also invalidates a previously protected source. |
| LC25 | Account epoch e→f on recovery; installation epoch unchanged | Every old session revoked in resolution, even remembered/unexpired; permissions/owner unchanged. Unchanged normal startup keeps e. |
| LC26 | Current actor valid; selected already-revoked row | Authorized metadata returned only after read-only commit; its old revision/times unchanged, no bearer. A repeated logout authenticated by that row fails. |
| LC27 | Page_size2, ordered sessions a,b,c; after absent then b | First emits a,b with next=b; second emits c, absent next. after z returns empty, absent next. |
| LC28 | Same first page, cursor b then inserted aa and d | Next current page c,d; aa requires restart. No retained snapshot/disclosure permit. |
| LC29 | Malformed account link plus revoked/expired selected row; malformed probe row | invalid_stored_data before state classification; no partial page. |
| LC30 | Accepted time1000, issue now301000 versus301001 | First accepted age300000 still fresh; second stale_authentication, no credential minted from old verification. now999 is unavailable. |
| LC31 | Known rollback releases reservation; unknown/committed attempt retried by reference t | Only still-current/fresh original acceptance may retry after known rollback. Unknown/committed t cannot mint another session. |
| LC32 | Page_size3 needs3 metadata rows; selected result budget fits only2 | resource_limit, no partial success. A separate page_size2 reader may inspect exactly3 rows as its probe, irrespective of total stored session count. |

Literal DTO budgets, with every required reference one ASCII byte: issue request
B=1+2+1+1=5, so request_bytes5 fits and4 refuses before admission. Rotate request is
1+2+32=35. Credential reply is1+4+1+24=30. Revocation reply is1+4=5. One inspected
row is2+1+1+24=28. A page reply with one row and absent cursor is
1+3+8+28+1=41; with two rows and present one-byte cursor is1+3+8+56+2=70.
A lifecycle page-source success with N single-byte-reference records is
1+(3+1)+2+1+N*(6+32+1+24)=8+63N;65 rows need4103 bytes.
These full DTO figures are additional to, and do not redefine, supplier's12-byte
complete-facts and5-byte resolved STRING-payload examples. Reference00 is legal;
empty reference is invalid. Enum tags/presence/digest bytes are counted explicitly.

## 10. Acceptance and downstream gates

Proposed finite page/C++/reservation/publication choices require actual independent
review. No requirement conflict or additional substantial subsystem is identified
by this document: ADR-0010 already allocates the physical store/snapshot/revision
producer, actual local acceptance, clock and lossless ingress prerequisites. Those
contracts must make their respective obligations concrete; if their design reveals
a new conflict or substantial prerequisite, record/split it before implementation.
Do not fill it with a client flag, fake provider or silent release-scope reduction.

SESSIONTEST-002 still needs an approved minimal API/harness, specification-linked
tests, different test review and observed intended behavioral red; SESSION-002
then needs real service composition, separate code review, passing relevant
acceptance/regression and selected CI. Accepted SESSIONTEST-008/SESSION-008 supplier
component work cannot establish these lifecycle gates. Physical schema/current
store, actual local/password/permission/clock producers, ingress/egress routes,
reconciliation/cleanup, account setup/recovery/restore, actual verified Twitch and
browser/native/DPAPI/QtKeychain/framework/OS/package support remain separate actual
work. This document enables no new executable CI; documentation/tooling checks
apply. Keep all seven integrations, all three FX providers and full release scope.
