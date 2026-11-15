# Reference authority registry v1

ACCESS-011, contract revision3 (supplemental selected historical-reader limits).
Public API detail: `server/include/subathon/server/sqlite_reference_authority.hpp`.
This is the bounded original-author producer required by CONTRACT-014 section4.
It does not complete ACCESS-003, login, HTTP, linked identity, UI or delivery.

## Ownership and admission

Restricted reference-profile choices, not additions to full ACCESS-003 policy:
`overrides.edit(E)` gates enrollment; only the owner authorizes/revokes; an
explicit authorization survives session revocation and owner transfer (current
owner/author/subject facts are still rechecked); every registry mutation invalidates
all capability fingerprints via the global head. Bootstrap provenance is deliberately
omitted from the head; enrollment/transition provenance IS durable. This limited
bootstrap audit omission is not a general audit guarantee or full #249 design.

The authoritative command/state queue owns this registry on its existing controlled
SQLite writer. No other connection or writer is a production mutation path.
The composition owner passes a trusted CurrentSessionClock; it is never obtained
from request data. The internal digest lookup is prepared from the actual bearer
secret by the trusted session boundary; no public client supplies an account,
stored digest, resolved-session object or clock sample as authority. Every mutation freshly resolves the supplied digest through
SqliteAccountSessionUnit and resolve_current_account_session(require_fresh=true)
in the SAME active write scope. A supplied ResolvedAccountSession, author string,
allow bool, principal, retained rule/document or prior decision is not accepted
as authentication. Missing/revoked/expired/not-fresh/disabled/epoch-mismatched
sessions deny. Unavailable clock/source refuses unavailable; corrupt source
refuses invalid_data. Actual SqliteWorkFailure propagates unchanged.

Bootstrap is a deliberately restricted trusted owner command, not public signup.
The installation, authenticated owner account, complete permission head, exact
trusted reference catalog and owner facts must already exist. Bootstrap requires
that authenticated account equals the CURRENT protected owner and has current
access.manage management USE. Only an absent registry head can be initialized,
at revision1; a present head collides, including a repeated identical bootstrap.
It creates no accounts, credentials, graph, sessions, rule, capability or grant.
No actor is implicitly authorized by migration or registry absence.

After input/ticket/authentication/complete-graph validation and expected-head CAS
comparison, an existing enrollment key returns collision before proposed capability
checks; it cannot overwrite even with a different otherwise unauthorized binding.
Enrollment binds author to the actual authenticated account and creates PENDING
state only. It requires current overrides.edit management USE for the explicit
execution-account resource plus BOTH author USE and owner-configured GRANT for
ALL THREE complete singleton capabilities derived from this immutable binding.
Execution account is explicit, current, enabled, same installation; it is never
inferred from principal_ref. Its current USE must fit each capability too.
All references and amount are immutable after creation. New rule/action/version,
source, timer, amount, author or execution subject requires a NEW capability ref.
Reusing a capability ref collides; enrollment never overwrites or resets history.

Only the freshly authenticated CURRENT owner may authorize or revoke an existing
binding. Both changes require access.manage USE and current owner GRANT covering
ALL THREE complete immutable capabilities. This applies to restrictive revocation
too; no implicit owner GRANT. Authorizing also revalidates original-author dual
channels and execution account USE. Revocation does not require the original
author still to have USE/GRANT. No third party can change GRANT via this registry.
set_state admits authorized/revoked only; pending is not a transition target.
Authorized-to-authorized and revoked-to-revoked deliberately advance the revision.
Untouched bindings are not reauthorized, rewritten or pruned by any mutation.
Only provider synthetic.reference.v1 is supported; other well-formed enrollment
providers refuse unavailable after current authentication/graph/CAS checks and
before capability checks. No live provider registration.
These restricted paths do not authorize arbitrary graph edits; ACCESS-003 keeps
its full management/delegation/impact rules and other edit flows.

Each mutation CASes the installation registry head. Successful bootstrap is1;
every successful subsequent mutation increments once (including a no-op state
assignment); stale/collision/denied/missing/resource refusal writes NOTHING.
UINT64_MAX cannot advance: resource_limit, zero writes, never wrap/reuse.
Enrollment appends an immutable pending event at the newly allocated revision.
Transitions append immutable events. Global revision ordering is unsigned.
Initial event actor MUST equal original_author, state pending and revision equal
created_revision. Latest event must be >=created and <=head; missing/inconsistent
initial/latest rows are invalid_data, never default pending or known-empty.
Authentication session refs/revisions/timestamp are audit provenance. They are not
foreign keys to ephemeral sessions and later session revocation/expiry does not
rewrite authorship or implicitly revoke an explicitly authorized automation.
Disabling/removing its author or execution account DOES refuse current execution.

## Current facts and composition

resolve reads current registry head/binding/initial/latest event, actual immutable
capture closure and COMPLETE SqlitePermissionGraphUnit graph on the same session.
It verifies reference_permission_graph_trust(), current owner, catalog, all graph
roles/memberships/overrides and actual current account/install fences. Missing
installation/owner/account closure is invalid_data. A wholly absent graph (no
head or children) is missing; an absent head with orphaned children is invalid_data.
A corrupt, stale or unsupported stored reference catalog is invalid_data, not an
implicitly supported catalog. No sparse graph omission is
interpreted as an unknown account with an empty policy; known existing accounts
use ACCESS-010's complete, deterministically ordered sparse-row semantics.

Captured principal must be automation; installation/principal/capability/rule,
action and both versions/timer/amount/source scope must equal the registry binding.
Source scope is provider/source_account/namespace, byte-exact, not an event ID or
transport connection. Each event still must resolve its actual retained initial
capture/index, SRX1/SRA1/configuration and plan closure. All duplicated receipt,
CID (including real0), plan/version/source/arrival/execution bindings must agree.
An enrollment does not authenticate a historical retained document's creator.
After shape preflight, check equal principal/captured/rule installation fields,
then lookup the capability by installation and capability ref; absent registry or
capability is missing. Known binding scope mismatches are invalid_data. Then select
the capture index by the source's full event key: absent index/selected document
is missing; an existing index/document whose execution/arrival/receipt/CID/plan or
other duplicated field differs is invalid_data. Inactive/current-policy failure
denies. Non-automation principal OR rule automation kind is an input-shape failure
ReferenceAuthorityInputFailure(invalid_data), not an authorization refusal.
Enrollment execution-account absence is missing; a binding/session installation
mismatch denies. set_state on an unknown capability is missing; pending target
throws input failure(invalid_data). No fallback aliases.
Only the three ordinary reference operations from CONTRACT-014 are supported.

For execute, build ONE concrete singleton complete capability using the FULL
captured requested_ms (0..INT64_MAX), never an actual/cap-limited credit. Run
revalidate_permission_automation for ORIGINAL author with its separately bound
current USE and GRANT. A failed action is denied, not replaced by another author.
Also evaluate the actual execution account's own concrete request/policy using
ACCESS-002. Any deny refuses denied; unavailable/invalid calculation facts refuse
unavailable/invalid_data, limits refuse resource_limit. No policy intersection,
synthetic role, merged grant, sampled range or persisted permit is constructed.
The result retains BOTH raw original_author_input and separate permission_input.
The consumer verifies the complete binding and shared snapshot/catalog/owner,
recomputes original-author dual channels AND execution-subject permission, then
maps success to ApplicationAccessAllowed(permission_revision_ref). Facts are DATA;
neither the source, facts DTO nor old consumer result authorizes a later attempt.
Only the owning consumer can hold/deny work; this reader does not create holds.

Read operations use their own quantity-free singleton action, requiring current
original-author USE+GRANT AND execution-subject USE for that read. They never
reuse execute authority. disclose_application requires historical_operation,
loads the ACTUAL full stored bundle by that key, decodes its SAC1 and single SRF1,
and validates that record's source/plan/principal/capture/timer against the query.
disclose_holds requires historical_operation AND historical_hold, loads the actual
full hold cause/current version and its command/source/captured plan, and performs
the same binding checks. For whole-set disclosure the owning consumer must call
for EVERY actual hold and disclose none if ANY member refuses. Execute forbids
both historical fields; application disclosure forbids historical_hold. Missing
required historical keys are invalid input, never an empty disclosure permit.
requested_operation identifies today's request; it never replaces historical keys.
No read discloses details before its separate authorization has succeeded.
Consumer mapping: denied -> ApplicationAccessDenied; missing/unavailable/resource
refusal -> ApplicationAccessUnknown. invalid_data is a known-semantic failure
requiring existing whole-unit failure handling, never a convenient unknown/deny.
stale/collision are mutation-only, never valid successful resolve alternatives.

## Finite callback capability

ApplicationReferenceReadView gains only
resolve_reference_authority(const ReferenceAuthorityRequest&,
const ReferenceAuthorityLimits&) -> ReferenceAuthorityResolution.
SqliteReferenceAuthoritySource delegates to that method, without owning SQL/session.
Preflight selected limits, then complete borrowed input, then check callback
lifetime/thread, then privately construct the unit on the EXACT existing session.
No public view constructor, raw SQL, session, writer, transaction, nested work,
event pumping, network or arbitrary repository operation is exposed. Method
limits cannot raise the enclosing view's reference/document/result ceilings;
use minima with service reference/storage_document/intermediate bytes/items.
Default/retained/copied/returned/terminal capabilities cannot extend callback
lifetime: expired_callback0; live foreign thread: wrong_thread1, before Qt SQL.
Copies WITHIN the active owner callback remain usable. Actual underlying writer
ticket/SQL/cumulative/terminal failures propagate unchanged. A failed call cannot
become unavailable/empty policy or silently commit/cleanup a healthy refusal.
The callback must revoke on every normal/exception exit as existing call() does.

## Revisions and physical representation

Step10 `reference-authority-v1` appends to ACTUAL compiled prefix1-9 from
permission_graph_repository_manifest(). Preserve every shipped ID, ordered SQL,
canonical payload and expected schema byte; supported starts are exactly0..10.
Opening a database never migrates/bootstrap-authorizes. Explicit migration leaves
new registry tables empty, preserving singleton account installation semantics.
Separate installation scenarios require separate disposable databases.

The three tables below use BLOB references (nonempty, typeof='blob'), u64 heads
and created/event revisions as EXACTLY8 big-endian BLOB bytes, nonzero. INTEGER
fields require typeof='integer'. No TEXT coercion or signed-u64 workaround.
Primary-key column order and append-only semantics are contract, not ABI.
Exact additive DDL and full sqlite_master projection are independently literal in
the test fixture's step10; implement the manifest from this schema, not an altered
prefix or a dynamically observed/normalized schema.

| Table | Columns in physical order / constraints |
| --- | --- |
| subathon_reference_authority_head | installation_ref PK, revision; installation FK to account_installation |
| subathon_reference_authority_bindings | installation_ref, capability_ref PK pair; principal_ref, rule_ref, rule_version_ref, action_ref, action_version_ref, timer_ref, provider, source_account, identifier_namespace, requested_ms INTEGER>=0, original_author, execution_account, created_revision; installation FK to registry head; (installation,author) and (installation,execution) FKs to accounts |
| subathon_reference_authority_events | installation_ref, capability_ref, revision, state INTEGER IN(0,1,2), actor_ref, session_ref, session_revision_ref, authenticated_utc_ms INTEGER; PK(installation_ref,revision); (installation,capability) FK to bindings; (installation,actor) FK to accounts |
| subathon_reference_authority_events_capability_idx | CREATE INDEX on subathon_reference_authority_events(installation_ref,capability_ref,revision); required for finite latest-per-capability seek |

Enums are frozen explicit bytes in the header: use execute0/application1/holds2;
state pending0/authorized1/revoked2; failure denied0/missing1/invalid_data2/
unavailable3/resource_limit4/stale5/collision6. SQL states store the same numbers.
Rows are never physically deleted through this API. Latest selection is bounded
ordered revision seek, not materialization of history; unrelated registry entries
or unselected intermediate history are not globally audited by a selected read.
No new registry payload codec, random alias or retained-policy envelope exists.

Permission source is EXACT `RSS1` + B(provider)+B(source_account)+B(namespace),
where B is u32be byte length + bytes. Preflight16+sum(component lengths) against
BOTH selected reference and permission reference ceilings BEFORE construction.

Automation revision is `RAA1` + SHA-256 of this canonical preimage:
`RAA1` + B(installation)+u64(head)+B(capability)+
u64(created)+u64(latest event)+u8(state)+B(original author)+B(execution account).
Combined permission revision is `RAR1` + SHA-256 of this canonical preimage:
`RAR1` + B(installation)+u64(graph head)+
B(catalog revision)+B(current owner)+B(installation epoch)+B(installation revision)+
u32(account count)+each account in unsigned byte identity order:
B(account)+B(epoch)+B(revision)+u8(enabled), then B(automation revision).
Fingerprints are36 raw bytes (4-byte tag plus32-byte digest), not hex or permits.
Stream canonical preimages through the selected SHA-256; no unbounded buffer.
All graph accounts are included, not only the author/subject. Counters never wrap.
Both input snapshots equal combined permission revision. Catalog remains the ACTUAL
current catalog. Valid catalog/head/owner/account/install changes must invalidate
combined revisions; even a restored identical policy has a new head. Catalog byte
changes require the graph/catalog owner's versioned trust migration, not an
ACCESS-011 alternate catalog. Unknown/stale catalog never becomes supported.
Neither revision claims authenticated client session authority at execution.

## Resource and failure obligations

All ceilings positive and below SIZE_MAX/PTRDIFF_MAX, with checked sums and
container max_size preflight, including nested graph/account/capture/core limits.
Invalid limits/input throw ReferenceAuthorityInputFailure(invalid_data);
whole borrowed input overage throws that class(resource_limit), BEFORE copying,
SQL or numeric work. Valid input with missing/denied/corrupt/limited selected facts
returns the distinct typed failure; real SqliteWorkFailure always escapes unchanged.
No warnings/skips/truncation/partial facts. Unexpected allocation failure refuses
resource_limit, never success. Stale head is not SQL unavailable or corruption.

Semantic B/I: string bytes/0; enum,bool1/0; i64,u648/0; optional1+present B/I;
vector sum B, size+recursive I; structs sum EVERY declared field. Limits uncharged.
input_bytes covers entire request/read keys or mutation arguments, including
SessionLookupRequest's installation+32 digest bytes and expected_head. Clock and
session/limits objects are borrowed dependencies, not input payloads.
result_bytes/result_items cover the WHOLE Facts or Record, preprojected before
materialization. Successful staged output is8 bytes/0 items, projected before
writes. Source and both revision blobs are charged in EVERY occurrence,
including both snapshots. A failure enum is the bounded refusal, not partial facts.
Nested producers/calculators retain their OWN limits in addition to these totals.

For authority historical disclosure, `captures` bounds the COMPLETE selected
historical source record as well as the capture closure; these are separate
bounded materializations, not charges against registry_selected_bytes. Let L be
ReferenceAuthorityLimits after callback minima. Derive SqliteBundleLimits from
defaults with reference_bytes=min(L.reference_bytes,L.captures.reference_bytes),
value_bytes=L.captures.document_bytes, owned_bytes=L.captures.owned_bytes,
component_bytes=min(default,L.permissions.component_bytes) and
result_items=min(default,L.result_items). Derive SqliteHoldLimits from defaults:
references=that bundle profile, owned_bytes=L.captures.owned_bytes and
result_items=min(default,L.result_items). All other owning ceilings remain defaults.
The owning decoded-record byte/item charging rules apply, including repeated
returned ownership. Checked positive/range conversion precedes use. This mapping
is normative for private selected readers; it grants no permission or larger SQL
budget. Returned authority Facts/Record ALSO retain result_bytes/result_items;
these do not replace the historical source profile. Enclosing callback minima
still apply BEFORE deriving it. This reference-profile mapping is coordinator
accepted; broader public storage defaults and failure policies are unchanged.

registry_selected_bytes counts raw projected head/binding/initial/latest rows,
all reference/blob bytes, each SQL INTEGER8 except state1; repeated identical
initial/latest row is charged ONCE. Schema/length/type metadata must be checked
BEFORE payload materialization, including unbounded stored BLOBs. Source closure
uses captures; complete graph uses graph; authentication uses accounts/sessions.
mutation_bytes counts EVERY new/replaced registry row, including full head; no
charge for old unchanged rows. Preflight full write set before staging ANY row.
For the independent I/K/U/R/r/a/b/T/synthetic.reference.v1/A/N/600000/A/E fixture:
head9, binding50, each event23; authorized selected105; pending selected82;
bootstrap mutation9; enrollment mutation82; transition mutation32. Exact fit
succeeds; one less refuses without writes. Base semantic request B/I=88/0,
Record=79/0, whole Facts=2440/131 (including both complete policy inputs and36-byte
fingerprints). Tests independently reproduce these with practical small ceilings.

Publication requires actual commit AND normal SqliteWorkResult completion/value.
CAS and same-session reads are provisional; rollback cancels all staged changes.
After an actual SQL/cumulative failure later unit calls report session_closed,
while completion preserves the ORIGINAL failure. Commit ack loss is unknown,
not committed success and not an automatic retry under fresh refs.

Mandatory source audit: whole-input and output preflight BEFORE copies/expansion;
checked RSS1/framing/revision arithmetic; complete current graph validation; exact
account binding and independent policy channels; only finite indexed selected
registry/capture/historical reads with metadata-first projection, no history scan;
CAS/event atomicity and no writes on healthy refusal; current session source and
trusted clock freshness ordering; revocable same-session owner/thread checks on
ALL paths including defaults, copies, exceptions and terminal callbacks; unchanged
writer error propagation; no cached allow, public constructor, SQL/transaction/
network escape or suppressed warning. Tests alone cannot establish these costs,
capability confinement, trusted composition wiring or supported-platform behavior.
