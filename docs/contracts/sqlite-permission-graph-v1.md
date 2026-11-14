# SQLite permission graph, physical boundary v1

ACCESS-010 #646; tier A; **authored, not reviewed/implemented**. Public API:
`server/include/subathon/server/sqlite_permission_graph.hpp`. Core supplied-fact
semantics remain in `effective_permissions.hpp` and `delegated_permissions.hpp`.
This contract owns the physical cross-module schema/codec, not authenticated saves.

## 1. Ownership and bootstrap

One existing `SqliteWriter`, real `SqliteWorkSession`, existing installation/accounts
and migration runner. No new connection, thread, BEGIN, COMMIT, ROLLBACK, savepoint,
network or nested queue submission in this adapter. Borrow immutable trusted catalog
and management mapping for the unit lifetime. No callback or Qt SQL handle escapes.
Every public method checks a live ticket even if its result could be cached;
construction only preflights borrowed inputs and performs no SQL/ticket access.
A foreign/expired/closed/sticky ticket preserves the original `SqliteWorkFailure`.

The server composition root owns `PermissionGraphTrust`, never an edit payload.
`reference_permission_graph_trust()` is the compiled producer for #649's bounded
synthetic reference profile: catalog revision `permission-reference-v1`; exact
ten-operation/domain/mapping inventory follows in stored vector order (names are reference-profile
identifiers, not the full product mapping of effective-permissions-v1 §5):

| Operation | Class | Resource slots | Quantities / helper target |
| --- | --- | --- | --- |
| reference.time-add.v1 | ordinary | installation,timer,source | adjustment/ms integer0..INT64_MAX, modes15 |
| reference.application-read.v1 | ordinary | installation,timer,source | none |
| reference.hold-read.v1 | ordinary | installation,timer,source | none |
| roles.edit | ordinary | role | none |
| members.edit | ordinary | account | none |
| overrides.edit | ordinary | account | none |
| access.manage | protected_access_management | none | none |
| owner.recover | protected_recovery | none | none |
| helper.reset | helper_password_reset | target | helper target=target |
| timer.snapshot.read | ordinary | timer | none |

Bindings role/membership/override are respectively roles.edit/role,
members.edit/account, overrides.edit/account. The three reference operations match
CONTRACT-014 §4; timer.snapshot.read matches CONTRACT-013 §2. Domain endpoints
are reduced rational spellings 0/1 and 9223372036854775807/1; modes15 permits all
four quantity modes. No aliases. Full product coverage remains release work;
unlisted operations stay unsupported, never Any.
Catalog revision identifies its **entire** catalog and mapping. Reusing a revision
with different classes, definitions or mappings is invalid_data. A valid persisted
catalog with a different revision is unavailable to normal loads/writes until the
trusted deployment owner explicitly synchronizes it by CAS. Synchronization validates
both stored channels against the new catalog; incompatibility is invalid_data,
with no writes/discards. Same-revision corrupt catalog is never repaired by sync.
The store contains raw facts, not a permit/authentication token.

Trusted setup may bootstrap exactly once, with an existing installation, existing
owner account, complete USE and complete owner-configured GRANT graphs. Persist owner
separately; roles never establish/remove ownership. Bootstrap is a storage primitive:
#249 owns setup-code consumption, credentials, fresh authentication and any future
transfer. No owner transfer API is supplied here. Owner exists even with no grants;
owner USE guards do not imply an implicit GRANT bypass. Only the separate internal
owner-service primitive replaces GRANT. Its caller must authenticate the owner;
its name/type cannot confer authority. Ordinary replace_use cannot edit owner,
GRANT or catalog. Neither mutation accepts an actor, operation class or management
request supplied by an untrusted client.

`describe_use_edit` derives one request per changed role definition, per account
with changed membership set and per account with changed override entries. Additions
and removals count, empty role creation/deletion counts, ordering alone does not.
Each mapping names exactly one resource slot and no quantity slots in its catalog
operation. Targets are actual role/account identities, including self and every
changed target; sort/deduplicate by unsigned (operation,slot,target) bytes.
No-op yields no requests. #249 binds actor from a resolved current session, checks
management USE for **all** these requests plus GRANT/whole edit impact using the pure
APIs, and conditionally writes within that same serialized transaction. These
facts are insufficient to bypass that service. Probes and client previews confer
no save authority. Restrictive edits still require both channels (#249 owner policy).

ACCESS-011's exact prerequisite is `load()` on its caller's live serialized work:
complete current owner/catalog, both editable channels, graph revision and bounded
existing installation/account facts. It binds/derives original-author and explicit
execution-subject policies there. Principal_ref is not implicitly account_ref.
Original-author/capability registry bootstrap, immutable binding, revocation revision
and finite callback-safe read capability remain #649; none exists because of this
API declaration. Account disable/epoch/revision checks remain account-source facts,
separate from graph revision. Consumers must revalidate both; head alone is not an
authenticated session fence or combined automation revision.

## 2. Additive schema inventory

Coordinator reserves step **9**, ID `permission-graph-v1`, on the actual compiled
`account_session_repository_manifest()` prefix1-8. Preserve every previous version,
ID, SQL byte, catalog identity and expected-schema tuple. Supported starts0..9.
No data inserts or changes to previous tables/indexes. No trigger/view/extra index.
Three new ordinary rowid tables and exactly their three automatic PK indexes.
Complete expected_schema is previous head's projection plus these six objects,
sorted (type,name); autoindex SQL is NULL. sqlite_schema CREATE SQL is the exact
statement text without a trailing semicolon. The independently authored C++ literal
`step9` in `tests/qt/support/sqlite_permission_graph_fixture.hpp` fixes whitespace.
Below are the identical statements, displayed one statement per line.

```sql
CREATE TABLE subathon_permission_head(installation_ref BLOB NOT NULL PRIMARY KEY CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), revision BLOB NOT NULL CHECK(typeof(revision)='blob' AND length(revision)=8 AND revision<>X'0000000000000000'), owner_ref BLOB NOT NULL CHECK(typeof(owner_ref)='blob' AND length(owner_ref)>0), catalog BLOB NOT NULL CHECK(typeof(catalog)='blob' AND length(catalog)>0), FOREIGN KEY(installation_ref) REFERENCES subathon_account_installation(installation_ref), FOREIGN KEY(installation_ref,owner_ref) REFERENCES subathon_accounts(installation_ref,account_ref))
CREATE TABLE subathon_permission_roles(installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), channel INTEGER NOT NULL CHECK(typeof(channel)='integer' AND channel IN (0,1)), role_ref BLOB NOT NULL CHECK(typeof(role_ref)='blob' AND length(role_ref)>0), entries BLOB NOT NULL CHECK(typeof(entries)='blob' AND length(entries)>=8), PRIMARY KEY(installation_ref,channel,role_ref), FOREIGN KEY(installation_ref) REFERENCES subathon_permission_head(installation_ref))
CREATE TABLE subathon_permission_accounts(installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), channel INTEGER NOT NULL CHECK(typeof(channel)='integer' AND channel IN (0,1)), account_ref BLOB NOT NULL CHECK(typeof(account_ref)='blob' AND length(account_ref)>0), memberships BLOB NOT NULL CHECK(typeof(memberships)='blob' AND length(memberships)>=8), entries BLOB NOT NULL CHECK(typeof(entries)='blob' AND length(entries)>=8), PRIMARY KEY(installation_ref,channel,account_ref), FOREIGN KEY(installation_ref) REFERENCES subathon_permission_head(installation_ref), FOREIGN KEY(installation_ref,account_ref) REFERENCES subathon_accounts(installation_ref,account_ref))
```

All refs are byte-exact BLOBs; no UTF-8 conversion/coercion. Channel0 is USE,1 GRANT.
`revision` is nonzero U64 big-endian;1 is bootstrap. No INTEGER narrowing.
Physical references link existing installation/account facts. Per-channel roles
are independent, including membership. Head absence plus no child rows is missing;
orphan child rows, missing parents/owner or malformed selected facts are invalid_data.
Existing healthy head plus empty role/account tables is complete known-empty policy.
Load enumerates the bounded actual installation account roster, synthesizing an
empty account record in each channel only when its exact sparse row is proven absent.
A channel account row stores all memberships and individual entries; an omitted
row means both lists empty. Writes MUST omit rows with both lists empty; reads
accept explicit empty rows. All supplied accounts must
exactly equal the current roster, including empty ones, with no extras/duplicates.
New accounts with no graph rows have known no rights, not an unknown policy. Account
creation/deletion/access facts retain their owning source revisions; graph head
covers explicit owner/catalog/policy rows. Account owners compose both fences.
Read roles/accounts in unsigned reference order; preserve entry and nested vector
order as encoded. Inputs may reorder sets/vectors where core semantics permit;
management diffs compare those sets semantically. Every membership must resolve.

## 3. Exact structural codec (no opaque policy/capture shortcut)

Dedicated tables store typed policy fields, never retained-context/capture objects.
Catalog BLOB is encoding of `PermissionGraphTrust`. Entries BLOB is encoding of
`vector<PermissionPolicyEntry>`; memberships BLOB is `vector<string>`. No QDataStream,
JSON, platform ABI, native endian, compression, floating point or lossy number parse.

Encoding recursively, with no padding, prefix magic or trailing bytes:

| Type | Bytes |
| --- | --- |
| enum / allowed_modes | One byte, public enum declaration ordinal / exact mask |
| U64 / vector count / string length | Eight bytes unsigned big-endian |
| string | U64 length followed by exactly that many raw bytes |
| optional | 00 absent;01 followed by value present; other tags invalid |
| vector | U64 count followed by each value in order |
| struct | Fields in public declaration order, expanded below |

rational=numerator,denominator. ResourceConstraint=slot,mode,allowed.
QuantityConstraint=slot,unit,mode,lower,upper.
PolicyEntry=origin,source,role,operation,effect,grant,resources,quantities.
QuantityDefinition=slot,unit,kind,minimum,maximum,allowed_modes.
OperationDefinition=operation,classification,resource_slots,quantity_slots,helper_target_slot.
Catalog=state,revision,operations. ManagementBinding=operation,target_slot.
Trust=catalog,role_edit,membership_edit,override_edit.
No graph/role/account wrapper or state tag is stored in the row payloads. Sparse
absence supplies complete state only after healthy head/closure validation.

Check cursor subtraction, tags, counts, all lengths and projected decoded items/
semantic bytes before reserve/resize/copy. Counts exceeding items return resource_limit
even if the body is truncated; fitting truncated strings/records, trailing bytes,
invalid enums/numbers/catalog/policy/origin/source/grant shape return invalid_data.
Opaque refs permit NUL/high bytes; required refs nonempty. Exact rationals use core's
canonical reduced spellings and domains, no conversion to decimal/double. Validate
ALL rows, including unassigned roles and unrelated accounts; duplicate identities,
unresolved memberships and opposing individual scopes follow core contracts.
Malformed supplied graphs/trust are invalid_input, not storage corruption. Shape
validation supplies no authorization; structurally valid protected-operation grants
remain raw facts and cannot override protected core guards.

Independent framing examples: empty vector=`0000000000000000`; vector<string>{R}
=`0000000000000001 0000000000000001 52`; present string A=`01 0000000000000001 41`;
revision1=`0000000000000001`, MAX=`ffffffffffffffff`.

## 4. Complete reads, preownership and finite limits

All limits positive and fit SIZE_MAX, PTRDIFF_MAX, qsizetype and SQL signed LIMIT;
reference/component ceilings also leave one terminator byte. Actual destination
projections fit their max_size; a ceiling does not allocate a limit-sized buffer. Check rows+1 and items+1, terminator space, and checked aggregate
`3*selected_bytes+mutation_bytes`; impossible limits are invalid_limits. Priority:
limits, complete borrowed input size/items, shape, ticket, SQL metadata bounds,
complete class/codec/link/semantic validity, current-head CAS. Oversize input beats
malformed input. Constructor borrows trust, validates it within mutation_bytes/items
without copying; selected_bytes applies to reads, not this trusted input preflight.
Expected0, empty installation/owner and unknown/malformed input graphs are invalid_input.
Constructor installation >reference_bytes is resource_limit before copying.
Absent installation or protected/requested bootstrap owner in storage is
invalid_data (load or mutation), not a known-empty roster; a different existing installation is a distinct graph scope.
After borrowed shape checks, complete current-source validation precedes comparison
of supplied accounts to the actual roster; roster mismatch alone is invalid_input.

Each graph/trust/input/result uses recursive semantic B: string=size, enum/bool1,
U64=8, optional=1+present value, struct=sum fields, vector=sum elements. I counts
EVERY vector element recursively. Include installation, revision, owner, catalog,
BOTH graphs, current account/install facts and repeated management/after fields in
complete returned values. Per-method borrowed charges (B sums, I recursively):
constructor=(installation,trust); load=0; describe_use_edit=(installation,after);
bootstrap=(installation,owner,use,grant); replace_use/replace_grant=(installation,
expected U64,after); synchronize_catalog=(installation,expected U64,trust).
Trust is also independently preflighted on construction. I counts only vector
members, never structs/strings/options. Returned-value budget failure is typed
resource_limit; borrowed-size failure throws input resource_limit. No input/result cross-sum is required;
each whole projection fits independently. Rows is the sum of head + both channels'
role/account rows + actual installation/account closure, once per physical row;
metadata/payload repeats do not multiply it. Sparse writes omit empty accounts.
No overhead/RSS claim follows from semantic bytes. References/components get their
separate field caps. Encoded row payloads AND their decoded complete output each
independently fit selected_bytes/items; complete borrowed mutations fit
mutation_bytes/items before copying. Encoded write bindings independently fit
selected_bytes/items (framing overhead is not semantic mutation B). Trust preflight is separate
from method input; source review must count simultaneous ownership and scratch.

Use explicit bounded column lists; never SELECT *. Metadata pass returns only
rowids/fixed integers, typeof/NULL and BLOB lengths, no variable payload. Enumerate
head, role/account rows in this installation and actual account/install closure,
with total rows budget plus **one sentinel**. Check all metadata resource bounds
before any class/codec invalid_data refusal, including across channels. No unbounded COUNT/materialize/sort-all
in C++. Validate every metadata field and checked aggregate physical byte sum
(including duplicate occurrences) before a driver-owned payload SELECT. Payload
queries repeat admitted rowid/type/length guards and actual Qt types/lengths/keys
must match. Metadata/driver payload/decoded result each independently fit selected
bounds; maximum three such payload projections coexist, with one preflighted
mutation projection. Scalar/index scratch, SQL engine allocation and Qt overhead
need source audit, not an invented exact heap bound. Cumulative writer budgets
remain additional limits and their sticky errors propagate unchanged. A graph that
fits store bounds need not fit arbitrary smaller writer budgets.

A complete load never truncates/skips malformed or excess rows, never returns empty
on failure. Allocation refusal is typed resource_limit; catalog revision mismatch
is typed unavailable. Actual SQL errors (including missing tables), prior sticky
errors and cumulative writer budget failures throw original SqliteWorkFailure from
reads AND mutations; never relabel them unavailable or storage invalid_data. Source proof must show metadata BEFORE
payload, decoded projection BEFORE ownership, borrowed preflight BEFORE copies,
bounded row enumeration/codec loops and no cached current facts. Output tests cannot
prove these obligations. No L^4/tight complexity or hard deadline is promised here.

## 5. Whole-mutation CAS and completion

Bootstrap collision is proven existing healthy head; malformed existing state is
not collision. Missing head on replacement is stale only after absence is proved
with no orphan rows. Compare expected to a healthy current revision BEFORE any
DELETE/INSERT/UPDATE. Old head mismatch is stale with zero writes. Every accepted
mutation, even no-op/synchronization, stages current+1 (MAX refuses resource_limit).
Replace all rows of exactly one channel, retain other channel/owner/catalog; catalog
synchronization changes only exact trusted catalog/mapping. Use conditional head
UPDATE WHERE installation+revision and immediate same-session changes()==1; require
the complete validated mutation in the single existing BEGIN IMMEDIATE transaction.
No UPSERT/adoption, caller-picked next revision, wrap, retry or stale-as-SQL-error.

All validation/codec/binding projections precede first mutation; source failures
throw typed storage failure without writing. A later SQL/native failure retains
writer error and requires whole caller-owned transaction rollback; no caller may
catch that failure and commit a subset. Store never autonomously cleans up another
owner's session. Staged results and same-session reads are provisional. Confirm
only real commit + normal work completion/value; rollback publishes no mutation,
committed+exception publishes no success, uncertain outcome claims neither rollback
nor success. Replays with an old head are stale, not idempotent success. #249 owns
session/current-auth binding, audit and client publication; this CAS is not a save
service and does not implement original-author capability lifecycle.
