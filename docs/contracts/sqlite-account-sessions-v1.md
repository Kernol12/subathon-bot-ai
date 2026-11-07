# SQLite account and current-session facts — proposal v1

STORAGECONTRACT-002; identity `subathon.sqlite-account-sessions.v1`.
Specification v0.45 §§10/16, AUTH-01/05/11/12, DB-01, SERVER-01, OPS-02.
DOCUMENT ONLY, based on actual main `e4d46d3` and claim `db09a6c`.
The [handoff](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/storagecontract-002-handoff.md) records actual review and
publication; this document does not establish source, tests, a red baseline or login.

Import [supplier v1](session-resolution-v1.md) §§2–4,
[lifecycle v1](session-lifecycle-v1.md), [transport v1](session-transport-v1.md),
[password logical v1](password-hashing-v1.md) and
[password API v1](password-hashing-api-v1.md),
[writer work v1](writer-transactions-v1.md),
[SQLite writer v1](sqlite-writer-v1.md) and
[migration catalog v1](migration-catalog-v1.md) unchanged. Exact imported blobs and
limited author checks are in the [author record](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/storagecontract-002-author.md).

## 1. Scope, ownership and compatibility

The proposed store persists installation/account application-access facts, the ONE
current optional local credential per account, and hash-only account sessions.
Facts and source references confer no accepted login, roles, permission, ownership,
provider authentication, setup/reset entitlement or operational authority.
No usernames, roles, recovery codes, invitation/setup/reset offers, provider tokens,
linked-user tables, secret response cache, audit policy or clock policy are added.

| Owner | Required handoff to/from these primitives |
| --- | --- |
| Installation/setup and invitation/account owner | Authorize creation of immutable installation/account identities, initial access/epochs/revisions and optional local credential in its actual transaction. First-owner/setup-code/single-use/role/recovery completion is its own atomic composition; these rows alone cannot complete it. |
| Local login ACCESSCONTRACT-001/ACCESS-008 | Resolve a real account, admit/verify password off writer, retain actual accepted attempt/request association, then recheck exact credential/account/install facts on the queue. Supply/reserve actual accepted attempt for lifecycle; own its accepted lineage for conditional rehash. |
| Permission/access owner | Supply current operation/target disclosure and mutation permission in the same nonyielding scope. Access-enabled is not a permission grant. Permission edits do not derive from this schema. |
| Recovery ACCESS-005 | Change credential, account epoch and associated recovery facts atomically, preserving permissions/owner identity. An epoch-only primitive does not implement recovery. |
| Restore STORAGE-007 | Replace installation epoch after stopped/staged validated restoration, before any restored access; keep normal startup epoch unchanged. Other restore/clock/action holds remain mandatory. |
| Provider ACCESS-004 | Supply actual verified stable link/authenticator revision and accepted sign-in. A method tag/link/account reference is insufficient; unavailable provider support cannot issue sessions. |
| Lifecycle SESSION-002 | Compose accepted-attempt reservation, current ingress/authority/clock/policy, these reads/CAS, actual commit, normal completion and one secret emission exactly as lifecycle §§4–8. |
| Operations/integrator | Own explicit migration/update/backup/readiness, retention, orphan reconciliation and actual manifest/schema/build/CI changes. |

Every operation below uses ONLY its caller's existing actual `SqliteWorkSession`.
No open/connection/thread/writer, implicit BEGIN/COMMIT/rollback, nested work,
SAVEPOINT, event pump, wait, network, password hash or native handle is supplied.
The caller begins, commits and performs RAII `cleanup()`; store destruction releases
its ticket only. The ticket remains revocable after commit/rollback/body exit.
All store facts/results are provisional until actual matching normal completion.
Existing `SqliteReader`, reference/application finite readers and opaque captured
contexts gain no implicit table access or authentication authority.

The current actual factory `reference_capture_repository_manifest()` has immutable
steps1–7, ending in `reference-initial-captures-v1`, full92 objects. Existing step7
canonical payload is71601 bytes, SHA256
`efe4d5927002c1849ee51b9c3a988ea5b8bcab9eb8547ad5f12387b27c33b218`;
steps1–6 retain the literal prefix identities in the
[initial capture schema](sqlite-initial-captures-schema-v1.md).
A future account-session manifest copies those compiled steps/statements/full
projections EXACTLY and appends the five statements in §2. Proposed coordinate8,
ID `account-sessions-v1`, starts `[0,1,2,3,4,5,6,7,8]`, is for integrator review,
not an allocation by this author. Root rechecks the then-current prefix/order.
No old table/data/constraint, catalog format1 or store identity
`subathon.sqlite-store` changes. No automatic constructor/open/startup migration,
`IF NOT EXISTS` repair, catalog shortcut, downgrade or FK-off normal path exists.
An old executable's narrower manifest must refuse the newer schema/version rather
than silently operate it. Explicit migration inspection/advance and OPS-02 backup/
maintenance/readiness gates remain; restoration is not a migration operation.

## 2. Exact physical proposal and codecs

Each fenced single line is ONE exact UTF-8 SQL value with no trailing semicolon.
Create order is installation, account, local credential, session, page index.
All four tables start empty, without seeded accounts/epochs/session/credentials.

```sql
CREATE TABLE subathon_account_installation(singleton INTEGER NOT NULL PRIMARY KEY CHECK(typeof(singleton)='integer' AND singleton=1), installation_ref BLOB NOT NULL UNIQUE CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), session_epoch_ref BLOB NOT NULL CHECK(typeof(session_epoch_ref)='blob' AND length(session_epoch_ref)>0), revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND length(revision_ref)>0))
```

```sql
CREATE TABLE subathon_accounts(installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), account_ref BLOB NOT NULL CHECK(typeof(account_ref)='blob' AND length(account_ref)>0), application_access_enabled INTEGER NOT NULL CHECK(typeof(application_access_enabled)='integer' AND application_access_enabled IN (0,1)), session_epoch_ref BLOB NOT NULL CHECK(typeof(session_epoch_ref)='blob' AND length(session_epoch_ref)>0), revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND length(revision_ref)>0), PRIMARY KEY(installation_ref,account_ref), FOREIGN KEY(installation_ref) REFERENCES subathon_account_installation(installation_ref))
```

```sql
CREATE TABLE subathon_local_credentials(installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), account_ref BLOB NOT NULL CHECK(typeof(account_ref)='blob' AND length(account_ref)>0 AND length(account_ref)<=256), revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND length(revision_ref)>0 AND length(revision_ref)<=256), verifier_format INTEGER NOT NULL CHECK(typeof(verifier_format)='integer' AND verifier_format=1), encoded_verifier BLOB NOT NULL CHECK(typeof(encoded_verifier)='blob' AND length(encoded_verifier)<=127), PRIMARY KEY(installation_ref,account_ref), FOREIGN KEY(installation_ref,account_ref) REFERENCES subathon_accounts(installation_ref,account_ref))
```

```sql
CREATE TABLE subathon_account_sessions(installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), account_ref BLOB NOT NULL CHECK(typeof(account_ref)='blob' AND length(account_ref)>0), session_ref BLOB NOT NULL PRIMARY KEY CHECK(typeof(session_ref)='blob' AND length(session_ref)>0), installation_epoch_ref BLOB NOT NULL CHECK(typeof(installation_epoch_ref)='blob' AND length(installation_epoch_ref)>0), account_epoch_ref BLOB NOT NULL CHECK(typeof(account_epoch_ref)='blob' AND length(account_epoch_ref)>0), record_revision_ref BLOB NOT NULL CHECK(typeof(record_revision_ref)='blob' AND length(record_revision_ref)>0), digest_format INTEGER NOT NULL CHECK(typeof(digest_format)='integer' AND digest_format=1), digest BLOB NOT NULL CHECK(typeof(digest)='blob' AND length(digest)=32), state INTEGER NOT NULL CHECK(typeof(state)='integer' AND state IN (0,1)), retention INTEGER NOT NULL CHECK(typeof(retention)='integer' AND retention IN (0,1)), lifetime_started_utc_ms INTEGER NOT NULL CHECK(typeof(lifetime_started_utc_ms)='integer'), authenticated_utc_ms INTEGER NOT NULL CHECK(typeof(authenticated_utc_ms)='integer'), expires_utc_ms INTEGER NOT NULL CHECK(typeof(expires_utc_ms)='integer'), UNIQUE(installation_ref,digest), CHECK(lifetime_started_utc_ms<=authenticated_utc_ms AND authenticated_utc_ms<expires_utc_ms), FOREIGN KEY(installation_ref,account_ref) REFERENCES subathon_accounts(installation_ref,account_ref))
```

```sql
CREATE INDEX subathon_account_sessions_page_idx ON subathon_account_sessions(installation_ref,account_ref,session_ref)
```

Proposed complete head: unchanged92 prefix objects PLUS these10, total102.
Table SQL is its exact statement above. Every generated-index SQL is SQL NULL.
No trigger/view/other object is implied. Actual selected Qt/SQLite schema projection
must be independently observed before future fixture freeze.

| type | name | tbl_name | sql |
| --- | --- | --- | --- |
| index | sqlite_autoindex_subathon_account_installation_1 | subathon_account_installation | NULL |
| index | sqlite_autoindex_subathon_accounts_1 | subathon_accounts | NULL |
| index | sqlite_autoindex_subathon_local_credentials_1 | subathon_local_credentials | NULL |
| index | sqlite_autoindex_subathon_account_sessions_1 | subathon_account_sessions | NULL |
| index | sqlite_autoindex_subathon_account_sessions_2 | subathon_account_sessions | NULL |
| index | subathon_account_sessions_page_idx | subathon_account_sessions | Statement5 |
| table | subathon_account_installation | subathon_account_installation | Statement1 |
| table | subathon_accounts | subathon_accounts | Statement2 |
| table | subathon_local_credentials | subathon_local_credentials | Statement3 |
| table | subathon_account_sessions | subathon_account_sessions | Statement4 |

Opaque references bind as actual nonnull `QByteArray` BLOBs and decode to exact
`std::string` bytes, including NUL/non-UTF-8. Empty required references are invalid;
no TEXT conversion, collation, trimming, case folding, username interpretation or
C-string equality applies. Configurable reference caps are not arbitrary4096 SQL
constraints. The local credential's account/revision caps256 import the password
API; a longer general account reference can have no local credential under that
API until a separately reviewed compatibility change, but remains representable
in general session/account facts. Session identity is globally unique in this one
store; installation+digest is additionally unique. Keys never change or move.

Times are exact SQL INTEGER / checked signed64, never REAL, text, unsigned cast or
local time. Every int64 value, including both extremes, is representable; only
`started<=authenticated<expires` constrains a complete session tuple. Enum mappings
are state active0/revoked1 and retention normal0/remembered1; access is INTEGER0/1.
Do not accept arbitrary QVariant conversions or infer NULL as zero. `digest_format=1`
means supplier's EXACT domain-separated BLAKE2b-256 format; `verifier_format=1`
means password API's self-contained encoded-record container. Digests are EXACT32
BLOB bytes, never hex text. No raw bearer/password/encrypted bearer column exists.
Verifier salt/parameters/digest remain in the one original sensitive encoded record;
no duplicated cost columns or reconstructed current-policy record.

Verifier container bounds precede copying. Raw fitting PHC bytes are retained exactly
on reads, including an empty or malformed record for the password helper's own
`invalid_record`/`unsupported_record`/cost preflight. A container format other than1,
wrong SQL class or broken account link is storage invalid_data. A raw verifier of
128+ bytes is resource_limit before ownership. Credential creation/replacement must
receive a complete supported well-framed record from the real password owner and
perform bounded cheap record preflight; native verification is never performed here.
Permitting an empty BLOB physically does not authorize writing an empty credential.

A session's captured `installation_epoch_ref` and `account_epoch_ref` deliberately
have NO FK to the current epoch columns. They are immutable captures, not aliases.
Changing one current account/install epoch is O(1) selected-row mutation; old session
rows remain intact and fail resolution by mismatch. No cascade, session-table scan,
per-session epoch rewrite, silent revocation flag repair or deletion is needed.
FKs enforce identity existence only, with default NO ACTION; runtime also validates
selected links/types/epochs/format. Disabled, expired and revoked rows retain links.

## 3. Concrete internal API proposal and finite records

Future namespace `subathon::server`, header `sqlite_account_sessions.hpp`.
Documentation declarations only. Reuse actual supplier/password/writer types and
lifecycle's accepted authentication, retention, read/write enums and page facts.
New enum underlying types are uint8, consecutive from zero in listed order.

```cpp
enum class AccountSessionReadFailure : std::uint8_t {
    missing, invalid_data, unavailable, resource_limit
};
enum class AccountSessionInputError : std::uint8_t {
    invalid_limits, resource_limit, invalid_input
};
template<class T> using AccountSessionRead =
    std::variant<T, AccountSessionReadFailure>;
struct AccountSessionStoreLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t selected_bytes = 1048576;
    std::uint64_t mutation_bytes = 1048576;
    std::uint64_t page_rows = 64;
};
struct StoredInstallationFacts {
    CurrentInstallationFacts current;
    std::string revision_ref;
};
struct StoredAccountFacts {
    CurrentAccountFacts current;
    std::string revision_ref;
};
struct StoredLocalCredential {
    std::string installation_ref;
    PasswordCredential credential;
};
struct CompleteLocalCredentialFacts {
    StoredLocalCredential local;
    StoredAccountFacts account;
    StoredInstallationFacts installation;
    std::string snapshot_ref;
};
struct AccountSessionReadScope {
    std::array<std::uint8_t, 32> read_scope_ref;
};
class SqliteAccountSessionUnit final : public CurrentSessionSource {
public:
    SqliteAccountSessionUnit(const SqliteWorkSession&, AccountSessionReadScope,
                            AccountSessionStoreLimits = {});
    ~SqliteAccountSessionUnit(); // releases ticket/borrows; no transaction cleanup
    SqliteAccountSessionUnit(const SqliteAccountSessionUnit&) = delete;
    SqliteAccountSessionUnit& operator=(const SqliteAccountSessionUnit&) = delete;
    SessionLookup load_current(const SessionLookupRequest&,
                               const SessionLimits&) override;
    // Remaining signatures are the method table below.
};
```

`AccountSessionInputFailure(AccountSessionInputError)` has fixed text
`Account session storage input rejected`, no caller/SQL/verifier/digest text.
Its sole payload is that enum; thrown only for pure preflight errors.
Construction validates pure store limits and stores one inert session ticket,
fixed scope identity and fixed per-unit read ordinal, without SQL. No ownership of
writer, connection or caller transaction is taken. Explicitly retained unit/ticket
cannot survive its originating real work's validity. Foreign/expired/terminal use
must preserve actual `SqliteWorkFailure` wrong_thread/stale_session/session_closed;
it cannot become missing or stale CAS. Inherited source mapping is specified in §4.

The following complete method table is part of the C++ proposal. All arguments
borrow until synchronous return, values move out, and every source success includes
only the fields declared here/imported. There is no generic SQL/CRUD endpoint.

| Unit method (all public) | Return; exact scope |
| --- | --- |
| `read_installation(std::string_view installation)` | `AccountSessionRead<StoredInstallationFacts>`; exact installation key, required singleton1 validation. |
| `read_account(std::string_view installation, std::string_view account)` | `AccountSessionRead<StoredAccountFacts>`; exact composite key and linked installation. |
| `read_local_credential(std::string_view installation, std::string_view account)` | `AccountSessionRead<CompleteLocalCredentialFacts>`; current local row, complete linked account/install and snapshot, with password-specific256/127 bounds. No attempt reference invented. |
| `read_selected(std::string_view installation, std::string_view account, std::string_view session)` | `AccountSessionRead<CompleteSessionFacts>`; exact selected session and linked current account/install, never a login lookup. |
| `read_lifecycle_actor(const SessionLookupRequest&, const SessionLimits&)` | `AccountSessionRead<LifecycleCurrentSession>`; same complete supplier projection plus stored retention, no extra authority. |
| `read_page(std::string_view installation, std::string_view account, const std::optional<std::string>& after, std::uint64_t page_size)` | `AccountSessionRead<LifecyclePageFacts>`; one selected account/install and at most page_size+1 complete SessionRecordFacts; digest stays internal. |
| `insert_installation(const StoredInstallationFacts&)` | `LifecycleWriteResult`; expects installation table empty, inserts singleton1. Existing row is collision, not adoption. |
| `replace_installation_epoch(const StoredInstallationFacts& expected, std::string_view epoch, std::string_view revision)` | `LifecycleWriteResult`; compare complete expected singleton/key/epoch/revision, replace only epoch/revision. |
| `insert_account(const StoredAccountFacts&)` | `LifecycleWriteResult`; require current selected installation, absent account key; collision never means accepted registration. |
| `replace_account(const StoredAccountFacts& expected, const StoredAccountFacts& replacement)` | `LifecycleWriteResult`; same identities; CAS exact current access/epoch/revision, replace access/epoch/revision only. |
| `replace_local_credential(const std::optional<StoredLocalCredential>& expected, const StoredLocalCredential& replacement, const StoredAccountFacts& account, const StoredInstallationFacts& installation)` | `LifecycleWriteResult`; absent means INSERT only if absent; present means CAS exact full current credential/revision/container plus exact account/install fences. No implicit account creation. |
| `replace_password_upgrade(const PasswordReplacement&, std::string_view new_revision, const CompleteLocalCredentialFacts& expected)` | `LifecycleWriteResult`; exact account/source revision/FULL source verifier, actual bound accepted local login checked by host as below, current account/install and acceptance lineage; no setter authorized by DTO alone. |
| `insert_session(const SessionRecordFacts&, SessionRetention, const AcceptedSessionAuthentication&)` | `LifecycleWriteResult`; lifecycle issue current/reserved actual acceptance and uniqueness/fence rules below. |
| `replace_session(const SessionRecordFacts& expected, const SessionRecordFacts& replacement, SessionRetention, const AcceptedSessionAuthentication* accepted)` | `LifecycleWriteResult`; lifecycle rotation with absent accepted, reauth with present actual reserved accepted; exact full source-row CAS and current fences. |
| `revoke_session(const SessionRecordFacts& expected, std::string_view replacement_revision)` | `LifecycleWriteResult`; exact selected row active→revoked/revision change, with current authority owned by caller. |

No delete, account directory, total count, history enumeration, provider link write,
permission loader, arbitrary inspect/read connection or transaction method exists.
`read_installation/read_account` omit a snapshot field because they are raw facts,
not supplier/acceptance certificates. Their caller can compose them only within the
same real active session. Neither a `Stored*` value nor scope identity is a permit.
Lifecycle wraps these primitives in its actual `SessionLifecycleUnit`; policy,
clock, `check_ingress`, `check_access`, `load_accepted`, lifetime policy and identity
production remain trusted owning ports, not invented SQL tables here.

## 4. Current sources, coherent snapshots and password handoff

All reads are main-qualified fixed positional SQL on the bound session, indexed by
the exact keys described in §2. Exact lookup cardinality is at most2 metadata rows
(the second is an ambiguity guard); two is invalid_data. No selected-row absence is
inferred using an INNER JOIN that silently removes a dangling linked record.
Session lookup first selects by installation+digest WITHOUT a format/state/epoch/
expiry predicate. It then LEFT JOINs or separately selects that record's exact
account and installation. All six session refs, digest, format, state, retention,
three times, current account's three refs/access/revision and current installation's
two refs/revision are checked even if some are not returned by supplier. A corrupt
retention/hidden format/revision is not concealed by a valid public projection.
The foreign account/install keys must match exactly. Missing linked rows or a
partial projection are invalid_data, including for revoked/expired records.
No scan outside selected keys claims to find every possible corrupt wrong-key row.

Snapshot reference is exactly44 bytes:
`ASCII "SAS1" || read_scope_ref[32] || BE64(read_ordinal)`.
Read ordinal starts0 and increments once for each successful snapshot-producing
read; MAX is representable, and a subsequent read fails resource_limit before
wrap or payload allocation. Scope identity belongs to ONE actual bound work/read
facade and is never accepted from a client/command/prior snapshot. The host supplies
fresh fixed32 bytes using the already selected libsodium1.0.22
`randombytes_buf(...,32)` after successful initialization, retains no bearer,
and must not deliberately reuse a scope for another real work. This is the same
accepted random-byte primitive, not another dependency/crypto mechanism. It has no
ordinary recoverable return code; initialization/unavailable actual runtime refuses,
with no zero/timestamp/deterministic/cached fallback. Repeated independent256-bit
values remain probabilistically possible: this is not a historical uniqueness proof.
No persistent snapshot counter/service is added; actual fresh entropy/restart/restore
and source-lifetime audit is a STORAGETEST-020 gate. Same scope/ordinal is assembled only AFTER all complete
facts have been validated from this actual session. Snapshot refs identify reads;
they encode no permission, lease, clock sample or promise of future equality. Every
later load executes actual queries again, including within the same unit.

`load_current` uses its unchanged supplier limits IN ADDITION to store limits.
Its success has exactly the twelve string occurrences of CompleteSessionFacts;
snapshot44 replaces the supplier's illustrative one-byte snapshot, not its API.
Selected record absent alone gives SessionMissing. Complete malformed/broken/unknown
format gives SessionSourceInvalidData; preownership overage gives
SessionSourceResourceLimit; unclassified inability gives SessionSourceUnavailable.
A disabled account, revoked record or differing nonempty captured/current epochs
is a VALID complete source result; supplier §3 applies revoked before clock/expiry.
Expiry/freshness and current-clock trust belong to supplier/lifecycle, not SQL.
The adapter cannot return missing for an epoch mismatch or cache an earlier grant.
Actual SQL/work failures retain their sticky writer error/outcome even when the
source facade exposes unavailable/resource to its consumer; they cannot be caught
and turned into a subsequently successful work.

`read_selected` returns valid disabled/epoch-mismatched/expired facts as well. Revoke
can target such a row after current actor authorization; actor lookup always uses
the actual original request digest. The lifecycle wrapper translates selected/page
missing only under the accepted closed method-specific rules.

The actual `PasswordLoginSource` host is bound to ONE real account/login attempt,
stable source-object address and request lifetime. It calls `read_local_credential`
through its existing authoritative owner bridge, not from the crypto worker via a
foreign ticket. Each bridge request/reply is finite owned metadata. It waits only
on the worker side; the writer never waits for hashing, worker completion or another
work. No active transaction/read snapshot is held across crypto. Subsequent source
checks take fresh owner-queue reads, so credential resets/epoch/access changes
between calls are observed. This document supplies no new bridge/executor/runtime;
unavailable actual host/attempt integration must return unavailable.

The source preflights account/revision/attempt <=256, verifier<=127, complete
PasswordCredentialRead before copying. A present result copies the EXACT current
credential and actual host-bound attempt_ref, never a synthesized verifier/default
cost. A missing credential/account has no credential and EMPTY attempt. A selected
local row with absent account/install is invalid_data internally, never missing.
`read_current`'s fixed public API has no corrupt/resource variants: these internal
failures map explicitly to unavailable with no credential/attempt; retain the typed
internal result without secret diagnostics. Do not invent a new enum, malformed DTO
or exception protocol to force helper source_invalid. Actual source-allocation and
port exceptions keep the unchanged password API mappings. The narrower mapping
makes no wrong-password claim and does not erase real writer outcome/errors.

`check_current_successful_login` first checks `verified.belongs_to(*this)` without
retaining the proof, then exact real bound account/attempt/full source revision/
verifier, ACTUAL accepted local-login lineage and current account access/install/
account epochs and revisions. No owned/queued proof borrow is permitted; a bridge
copies only its already bounded needed fields during the call. Absent acceptance
or a real policy refusal is denied; changed full source/bindings is stale; inability
to establish current facts is unavailable. Only a still-accepted current actual
local login returns accepted. Both helper checks are fresh, read-only observations;
they accept/issue no login, reserve no new issuance and persist no credential.

`replace_password_upgrade` additionally requires that same actual owner to check
still-accepted account/attempt/source conditions in this same no-yield transaction.
The raw replacement/source DTO has no authorization power. Compare account,
expected_revision_ref AND FULL expected_encoded_verifier, plus all current account/
install fences; reject stale/missing without restoring an old credential. A successful
conditional rehash updates only the local credential/revision, preserving account
identity/permissions/session epochs and the acceptance's original authenticated
instant. The owner advances its retained accepted lineage to the exact new revision/
verifier only after known commit; this is the sole legitimate source-revision handoff
for lifecycle's later current authenticator check. Known rollback retains the old
lineage; unknown poisons it. No arbitrary revision mismatch or external DTO is
accepted as lineage. A reset instead invalidates acceptance and uses its own recovery
transaction. An accepted attempt consumed for session minting may still identify
the already accepted login for this rehash; it cannot mint another session.

## 5. Conditional mutations, immutability and attempt replay

Pure preflight (§6) precedes SQL. Runtime corruption is distinct from a CAS miss.
Each mutation validates the selected complete current facts BEFORE testing expected
values, so malformed selected rows are invalid_data rather than stale. Methods
with this return enum throw fixed `AccountSessionStorageFailure` carrying only
AccountSessionReadFailure for invalid_data/unavailable/resource_limit, with text
`Account session storage operation failed`; missing expected target is stale.
Actual SqliteWorkFailure is never wrapped into stale/collision or lost.

Every changed fact has a fresh nonempty revision; identities are immutable. Revision
meaning/CAS is scoped to its exact relation and installation/account/session key;
there is no cross-record UNIQUE revision constraint. Within that identity a revision
reference denotes one exact fact value, cannot be deliberately reused for different
bytes (including ABA), and is not derived from time alone. Fresh identity/
revision production uses the same already accepted initialized
`randombytes_buf(...,32)` for each nonsecret candidate, not a client-selected revision
or a new subsystem. A host-created session/revision/scope candidate is32 opaque bytes;
the store still accepts other owners' legal opaque byte references unchanged.
`new_record_identity` supplies one candidate only: issue a new session_ref/revision, mutations the exact existing
session_ref and a different revision. Validate candidates before staging. Actual
current uniqueness/CAS still decides; no probabilistic generator output is a proof
that no historical value ever existed. Equality with its current expected revision
or a known retained identity in that same namespace is refused unavailable/collision without another draw;
new session_ref colliding with any current stored session is collision. Snapshot
reuse never makes a prior DTO authoritative: all current fact/owner checks still run.
Full expected-record/fence equality is required even when revisions happen to match;
no claim of deterministic historical ABA protection follows from32 random bytes.
A later concrete implementation must audit freshness/nonreuse across restart/restore
together with epoch fences; this document
provides no historical uniqueness index or scan.

No mutable reference/key, account deletion, hard session deletion, reactivation or
bulk update exists. Installation replace preserves installation identity. Account
replace preserves both key components, requires a changed revision and compares
all expected access/epoch/revision bytes. Access/epoch replacement can be combined
with other owner facts in the same caller transaction. Local replacement preserves
key/format, compares full original revision AND encoded bytes; absent expected
cannot adopt an existing credential. Credential change does not invent an owner,
local login, fresh-auth sample or permission change. New revision differs from old.

Lifecycle insert requires matching installation/account, current enabled account,
current epochs/account revision and the actual source-bound accepted authenticator/
acceptance revision. For local acceptance, exact current full credential/source
lineage is additionally rechecked by authentication owner; verified Twitch uses its
actual provider owner, unavailable until implemented. Session insert stores only
passed digest/format1/metadata and cannot infer acceptance from a reference.

Rotation CAS compares ALL supplied SessionRecordFacts fields: install/account/session,
captured epochs, revision, digest_format1, digest, state and three times. Persisted
retention is validated too, and token-only rotation requires the passed retention
to equal the current stored retention. The imported expected DTO has no retention
member; a legal retention change also changes revision and therefore loses this CAS.
Replacement
preserves immutable bindings, epochs, state active and all three times/retention;
only digest/revision change. Reauth preserves identity/bindings/current captured
epochs, changes digest/revision/retention and times exactly under lifecycle accepted
fresh-login rules. Source must currently resolve active/unexpired under caller's
trusted execution sample. Store rechecks current enabled account/install/epochs;
local/provider acceptance fences are also required for issue/reauth. Revocation
compares all expected row fields and preserves digest/epochs/retention/times,
changing only active→revoked and revision. Selected account may be disabled or epoch-
mismatched; this must not prevent authorized revocation by a different valid actor.
Its actual actor/current permission is rechecked by host before staging/commit.
Already-revoked target is handled as lifecycle's read-only committed metadata path.

Use fixed `UPDATE ... WHERE`/`INSERT ... SELECT ... WHERE` predicates for full
expected equality, current links/fences and uniqueness, followed immediately by
`SELECT changes()` on the SAME session to require exactly1. Zero is stale only after
healthy selected facts and known expected mismatch/missing fence; no mutation occurs.
Absence insert prechecks exact unique keys; identity or installation+digest candidate
collision is collision with no adoption. Own old digest is explicitly collision.
The one BEGIN IMMEDIATE writer serializes these checks with mutation; no other
writer/host callback can interleave. A surprise SQL constraint/SQL failure retains
actual sticky WorkError and rollback truth, never a guessed collision category.
No `REPLACE`, update-on-conflict, retry loop, broad account revocation SQL or
ignored failure converts another row into the proposed record.

Accepted-attempt reservation is NOT a durable table or transferable proof added by
this contract. `load_accepted`/real authentication owner retains the exact tuple
attempt/account/install/captured epochs/account revision/authenticator revision/
acceptance revision/method/accepted time AND actual command/request association.
A random string or copied AcceptedSessionAuthentication cannot reserve it. The
owner reserves at most once for this work, keeps it reserved through actual matching
completion, checks freshness/current lineage in the serialized mutation scope and
settles it according to actual outcome:

| Actual outcome | Reservation/replay consequence |
| --- | --- |
| Not admitted/no work began | No store mutation or fabricated acceptance; owner's prior acceptance lifetime still applies. |
| Known rollback | Release only if ORIGINAL actual acceptance remains current/fresh and no other completed use; retry requires a new explicit admitted request. |
| Committed, including later exception/publication loss | Consumed for minting; no replay by reference, no bearer recovery or rollback claim. |
| outcome_unknown/uncertain cleanup | Unusable/poisoned; no automatic release/retry. Fence unresolved access and use owning reconciliation/recovery process. |

The primitive supplies provisional staged/stale/collision only; the host supplies
this outcome binding with the existing real work completion. Process exit cannot
resurrect an in-memory acceptance because none survives restart. Durable acceptance
survival, if later wanted, needs a separately reviewed producer contract; it is not
assumed or silently implemented with a new journal here. This exactly preserves
lifecycle replay obligations without choosing ACCESSCONTRACT-001's retained-state
mechanism, proof protocol or provider behavior.

## 6. Preownership bounds, exact queries and pages

Store limits are positive U64 and fit SIZE_MAX/PTRDIFF_MAX/container max_size with
terminator allowance. Also check page_rows+1 fits SQL signed LIMIT, qsizetype and
writer row counters, and `3*selected_bytes+mutation_bytes` without overflow. No U64
domain value is narrowed merely to fit SQL INTEGER. Durations remain positive FULL
U64 in the lifecycle policy, with exact mathematical signed-start+duration before
checking int64 expiry. This schema persists final signed times, not a narrowed
signed duration. Ordinal BE64 maps 0→0000000000000000, MAX→ffffffffffffffff;
opaque refs/epochs/revisions have no numeric ordering or zero sentinel.

Priority: validate limits; inspect COMPLETE borrowed input field lengths/counts/B;
then enum/reference/cursor/optional/key/time/operation shape; then active ticket/SQL;
then selected metadata bounds; then selected class/shape/link validity; then CAS/
epoch/state semantics owned by caller. Excess input is resource_limit; empty refs,
present-empty cursor, page_size0/>page_rows, invalid enums or wrong replacement
shape are invalid_input. Oversized beats malformed shape. Per-input limit errors
occur even on a foreign/expired ticket without touching it; valid ticket calls keep
writer foreign/stale/terminal behavior. No automatic cleanup is triggered by a pure
input refusal. Caller still owns cleanup of an already-active unit.

Use lifecycle §3's DTO byte function B: string length, enum/bool1, signed/unsigned8,
digest32, optional1+present, variant1+selected, struct sum, vector elements. Do not
charge native padding as semantic bytes or omit duplicate references. Each complete
source variant fits selected_bytes; each complete borrowed mutation argument tuple
(including expected+replacement/fences/optional/tags) fits mutation_bytes BEFORE
creating binding/capture copies. Supplier string-only and password fixed ceilings
remain additional independent limits; lifecycle source/result limits apply to its
full returned variants. Any additional owning copy gets its own projection first.

| Complete source projection | Finite payload/count |
| --- | --- |
| StoredInstallationFacts | 3 refs; B=sum refs. |
| StoredAccountFacts | 4 refs+bool; B=sum refs+1. |
| StoredLocalCredential | install/account/revision/verifier; B=sum4 fields. |
| CompleteLocalCredentialFacts | local4 + account4 + install3 + snapshot1 =12 byte strings, bool1. Success variant adds1. |
| CompleteSessionFacts | supplier12 refs + digest32+state1+times24+access1; B=sum12 refs+58. Success variant adds1. |
| LifecycleCurrentSession | CompleteSessionFacts+retention1; success variant adds1. |
| LifecyclePageFacts | account3 refs+access1, install2 refs, snapshot1, N SessionRecordFacts each6 refs+digest32+state1+times24; N<=page_size+1. Success variant adds1. |

A bounded scalar metadata pass precedes ANY driver-owned variable payload result.
For each selected root/linked column query SQL typeof(), NULL/presence and BLOB
length only, plus bounded fixed scalar values and private rowid locators. Never
`SELECT blob,length(blob)` and then claim the already materialized blob was bounded.
Validate ALL metadata (including hidden format/revisions/retention and probe), project
all source/output/SQL-binding lengths with checked sums, then execute a guarded
payload SELECT for only those selected locators/keys. The guards repeat admitted
classes/lengths/fixed values; independently check actual Qt QVariant types, lengths
and key equality before copying into strings. Payload/projection mismatch is
invalid_data, not a repair or another unchecked fetch. Metadata failures/overages
return no partial value. SQL engine/index scratch and Qt/container overhead require
separate source audit; these B limits are not process-memory/hard-deadline claims.

Metadata scope is finite: installation4 physical columns, account5, local5,
session13; root session closure22 columns (13+account access/epoch/revision/key refs
as needed +installation facts), not growing history. Each pass has a fixed column
list. Never SELECT *, COUNT all sessions, load/sort-all, strlen an unbounded value,
or call a consumer before complete source preflight. At most one bounded metadata
set, one bounded driver payload projection and one bounded constructed output
coexist per primitive; cap each separately at selected_bytes. With the
borrowed input projected before bindings, maximum dynamic store-owned semantic
payload is `3*selected_bytes+mutation_bytes`; fixed scalar/locator/view scratch,
QVariant bookkeeping and host retained lifecycle values are separately audited.
All metadata/payload/binding fields, repeated occurrences and later changes() count
against actual writer cumulative statement/result/binding budgets as well.
No finite default guarantee promises that arbitrarily chosen small writer budgets
can accommodate a legitimate page: exhaustion preserves actual sticky writer error.

Page query shape is exactly indexed `(installation_ref=?,account_ref=?)`, optional
`session_ref>?` BLOB boundary, `ORDER BY session_ref ASC LIMIT ?`, bounded at
page_size+1; two fixed statement shapes avoid an OR that defeats the key range.
The explicit page index uses raw unsigned-byte lexicographic order, exact prefixes
first. Cursor absent differs from empty; cursor need not identify an existing row.
Preflight metadata for EVERY selected row and linked account/install BEFORE owning
any page strings/digests, including probe; then check strictly ordered unique refs
and exact scope. Budget complete returned source (including probe) before payload,
and lifecycle reply (including repeated cursor) before output. No truncation to fit.
Store returns complete internal SessionRecordFacts including digest for validated
composition, but lifecycle exports NONE of those digests. Returning this imported
DTO does not add a public digest export method.

Consumer emits at most page_size, next cursor iff validated probe exists, equal to
last emitted ref; empty/end page has no cursor. Same current account/install and
trusted execution sample classify all rows: explicit revoked/disabled/either epoch
mismatch→revoked; otherwise now>=expires→expired; otherwise active (future auth/start
is unavailable). Shape/time corruption precedes effective state. Each later page
performs fresh authority+queries with a new snapshot; no multi-request snapshot
lease, account quota or total-session ceiling follows. Inserts at/below the cursor
may require restarting. Cleanup/retention is a separate authorized policy.

## 7. Independently derived literal SQL/logical examples

Proposed disposable-fixture oracles only, NOT executed SQL/feature tests. D0 is
32 zero bytes, D1 is 31 zero bytes then01; these synthetic digest bytes require no
bearer/hash call. They do not certify supplier hashing/randomness. All letter
references below are exact one-byte ASCII BLOBs. Use separate disposable cases;
ordinary setup keeps FKs/checks ON and creates real parents first. Corruption cases
require a separately reviewed isolated fixture mechanism after healthy setup;
the product never opens a writable corruption connection or disables constraints.
No expected value is generated by a future implementation.

Base SQL values (column order §2); H0 is the exact independent upstream verifier in
password logical §6 and the existing frozen password fixture, never one generated
by this store. For physical-container-only observations use X'24' (`$`) instead:
it fits storage but is malformed PHC and must not be accepted as a new credential.

```text
$argon2id$v=19$m=4882,t=2,p=1$bA81arsiXysd3WbTRzmEOw$Nm8QBM+7RH1DXo9rvp5cwKEOOOfD2g6JuxlXihoNcpE
```

H0 is96 ASCII bytes. CompleteLocalCredentialFacts with the other eleven string
occurrences totalling54 (ten one-byte references plus snapshot44) has B151,
success variant152; PasswordCredentialRead with account/revision/attempt each
one byte has96+3+state1+optional1=101. Store/password limits apply independently.

```text
installation: (1,X'49',X'45',X'4a')                       # I,E,J
account:      (X'49',X'41',1,X'46',X'4b')                 # I,A,enabled,F,K
local:        (X'49',X'41',X'4c',1,exact ASCII H0 bytes)   # I,A,L
session:      (X'49',X'41',X'53',X'45',X'46',X'52',1,
               X'0000000000000000000000000000000000000000000000000000000000000000',
               0,0,1000,1000,86401000)                  # I,A,S,E,F,R,D0
```

The snapshot example scope32 zero bytes/ordinal0 is exactly
`53415331 || 0000000000000000000000000000000000000000000000000000000000000000 || 0000000000000000`
(44 bytes). This synthetic scope is permitted only in direct future source-fixture
construction; it proves no production identity freshness. Ordinal1 changes only
last byte to01. Full supplier string payload with all other refs one byte is55
(11+44); full CompleteSessionFacts B113, success variant114. With owned_bytes55 it
fits;54 fails source resource before payload. Setting reference_bytes43 fails the
snapshot field even if aggregate is large. Resolved result strings total48
(four one-byte references plus44), independently within result limits.

| ID | Direct setup/order | Literal expected observation |
| --- | --- | --- |
| AS01 | Base row; exact I+D0 lookup, then different D1 | First complete I/A/S/E/F/R active plus current A/F enabled and I/E; second missing. No time check or native crypto in storage. |
| AS02 | Replace account current epoch F→G/revision K→M only | Session captured F and revision R/digest D0/times unchanged; complete source succeeds with F/G mismatch; supplier revoked. Any issue using acceptance captured F is stale; no session scan. |
| AS03 | Restore install E→H/revision J→N | Session captured E retained; source complete with E/H mismatch, supplier revoked. Normal unchanged reopen preserves H, never regenerates E. |
| AS04 | Access0 or session state1; healthy links | Complete source; supplier revoked before clock. Expired healthy row at86401000 similarly stays complete and supplier expired. |
| AS05 | Physically selected D0 row has missing account/install, wrong digest format2, retention2, or bad times | invalid_data before revoked/expiry; no fabricated missing, partial facts or default retention. Real DDL refuses ordinary bad inserts first. |
| AS06 | Digest TEXT64/31-byte BLOB; epoch NULL/empty; INTEGER/REAL reference | Ordinary constraints reject. Deliberately corrupt row selected by session identity is invalid_data (or resource first if an independent complete bound is exceeded). An unmatched corrupt digest outside the exact actor key is not discovered by a global scan. No coercion or prefix token lookup. |
| AS07 | Insert same S with D1; insert new T with same I+D0; candidate D0 equals own old digest | collision, no row adoption/change; entire lifecycle rolls back. Unexpected native constraint failure instead retains actual sql_failure, not forged collision. |
| AS08 | Base rotate expected R/D0→Q/D1 committed, then stale expected R/D0 CAS | Exactly S/Q/D1 remains, all three times1000/1000/86401000; stale second change affects0. D0 lookup is missing, D1 complete. No revision-only overwrite. |
| AS09 | Authorized revoke after AS08 from another valid actor | S becomes state1/new revision V, keeps D1/times/epochs. Already-revoked read returns unchanged V; logout using that revoked actor cannot authenticate. |
| AS10 | SQL stage insert T, then real later statement failure/known rollback | T absent after fresh reopen; all prior rows unchanged, no success/bearer. Staged result was provisional. |
| AS11 | Physical commit then lost acknowledgment; or confirmed commit then body throws | First outcome_unknown/fail-close/no publication/no replay; second committed/work_exception/no publication. Reopen may reveal new digest; old digest is never blindly restored. |
| AS12 | Staged replace R→Q observed through SAME unit, external reader before/after commit | Same unit fresh query sees Q; preexisting external read snapshot sees R; new postcommit reader sees Q. Explicit rollback instead leaves R. No external reader supplies authority. |
| AS13 | Base local L/H0; raw upgrade expected L but one different encoded byte; or reset L→Z while crypto runs | stale, no overwrite. Matching expected L AND FULL H0 plus actual still-accepted local login/fences may stage new verifier/revision; accepted helper flag alone cannot. |
| AS14 | New verifier was staged but commit rolls back/unknown | Known rollback keeps H0/L/old lineage; unknown marks attempt unusable and claims neither old nor new durable value. Confirmed rehash commit updates lineage only to exact committed new source. |
| AS15 | Accepted attempt t reserved for issue; committed or unknown completion, then request t again | No second mint. Known rollback alone may release, only with original acceptance still current/fresh/request-bound. Picking someone else's actual t is not membership. |
| AS16 | An oversized linked account epoch/revision or selected verifier length128; state revoked/expired simultaneously | resource_limit from complete metadata preflight before payload/copy/native work; no truncation or revoked/missing concealment. |
| AS17 | Raw selected credential BLOB empty/`$`, format1, healthy links | Internal bounded raw read; helper invalid_record. Unsupported but fitting PHC remains original and helper unsupported_record; container format2/broken links are storage invalid_data mapped unavailable by fixed password port. |
| AS18 | Account A absent credential vs orphan local row with absent A | First missing with no attempt from password source; second invalid_data internally and unavailable with no credential/attempt externally. No joined-away orphan. |
| AS19 | Session refs X'00',X'41',X'4100',X'ff', page_size2 | First emits00,41, cursor41 using4100 probe. Next emits4100,ff/no cursor. after X'40' emits41,4100/cursor4100; boundary row need not exist. BLOB NUL/prefix/high byte order is exact. |
| AS20 | Same first page then insert X'4000' and X'fe'; request after41 | Next page4100,fe/cursorfe;4000 requires restart. New snapshot/current permission every page. Bad4100 probe on first page fails whole read, not partial00,41. |
| AS21 | N records with single-byte refs and snapshot44; other store/SQL budgets sufficiently large | Page-source success B=51+63N; page_size2 with probe N3 costs240. Lifecycle source_bytes240 admits that DTO projection,239 refuses before payload; no total-account scan/count quota. |
| AS22 | Full signed time tuple MIN,MIN,MAX; positive U64 duration MAX | Stored times exact -9223372036854775808/-9223372036854775808/9223372036854775807; mathematical MIN+18446744073709551615=MAX. No narrowed -1 duration. MAX+1 expiry refuses before staging. |
| AS23 | Corrupt/missing source + database unavailable or writer sticky failure | Missing only proven absent selected root; inability unavailable with actual writer error/outcome retained. Unknown never becomes known rollback/stale/missing. |
| AS24 | Exact source per-value fits, but complete mutation/returned page incl cursor exceeds bound; valid operation on expired/foreign ticket | Resource refusal before ownership at its defined stage, no partial mutation/output. Valid ticket use preserves stale_session/wrong_thread, no SQL/cleanup on another work. |

Page arithmetic AS21: current account3 refs+bool=4, installation2, snapshot44,
variant1 →51; each record six refs+digest32+state1+times24=63. Internal returned
retention/format/revision metadata checks need their own projections; they are not
hidden free payload. No actual native/Qt allocation claim follows from this arithmetic.

## 8. Acceptance and downstream gates

This bounded proposal supplies STORAGETEST-020's physical/API contract only after a
DIFFERENT actual review. No new executable feature body is run here, and no historical
red is replayed. Root must settle additive version/full schema projection and actual
selected-driver observations, then separately review minimal concrete headers/stubs,
independent literal SQL/codec/resource/current-source/CAS/replay fixtures/tests and
build/CI harness before FIRST intended missing-behavior red. Compilation failure,
missing dependency, skipped/inverted test or an old component's green results cannot
establish it. STORAGE-020 implementation follows that reviewed red, with independent
source/resource audit and ordinary real-SQL acceptance/selected CI.

Actual source/identity generation, retained local-attempt owner and bridge, permission/
clock/transport/lifecycle host and commit/publication/reconciliation composition,
account setup/invitation/roles/recovery/ownership/provider facts, stopped restoration/
backup/update/readiness, retention and Linux/Windows/native/browser/platform/live
checks remain separate owning gates. No new substantial architecture or product
behavior is selected to fill them. If concrete ownership/lifetime/replay/identity
integration needs a new subsystem, split and review it before implementation.
Keep full release scope, all seven integrations and all three FX providers.
