# SQLite initial reference capture — physical proposal v1

Companion to [concrete018 proposal](sqlite-initial-captures-v1.md), frozen
[CONTRACT014 consumer](reference-application-adapters-v1.md) and DB-01. First
DOCUMENT stage only. Version7 and ID `reference-initial-captures-v1` below are
PROPOSED coordinates for root/different review, not reserved by this author,
accepted migration source or an installed schema. Root alone allocates the actual
version/order/manifest. No old DDL/API/data/fixture/assertion is changed.

## Immutable catalog prefix and additive inventory

Use unchanged catalogformat1/storeidentity `subathon.sqlite-store` and explicit
migration runner/update/backup prerequisites. The future pure
reference_capture_repository_manifest copies actual
application_hold_repository_manifest() versions1–6 with exact statement order,
content identities and full projections, then appends this proposed step.
Supported starts would be `[0,1,2,3,4,5,6,7]`. No open/constructor/find/retain lazily
creates/inspects/migrates, and a manifest return conveys no update/readiness authority.

| Prefix step | ID | Objects at that head | Canonical payload bytes | Frozen SHA256 |
| --- | --- | --- | --- | --- |
| 1 | retain-config-context-v1 | 7 | 2000 | 0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e |
| 2 | identified-receipts-v1 | 13 | 10535 | 24614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2 |
| 3 | contribution-evidence-v1 | 20 | 18126 | b72249ba22c4838af98b9ae29600e70969cf3295eea958a30074167221d2ad1d |
| 4 | captured-application-plans-v1 | 30 | 29015 | bbb8140119b29f54f419479851d4fdf42b6a928f5451e4de756389d2bc359c50 |
| 5 | atomic-application-bundles-v1 | 66 | 62465 | c9ba98fa4fe2a42cc6ed9776baa011fc2b93cf879e7374b53e9a6128f7d7afbb |
| 6 | durable-application-holds-v1 | 89 | 89890 | 9f5b61d41dc644c417808d781cea73144a8d019ad95646373c30751e71596d0a |

The prior fixture/physical documents remain the authority for exact IDs; this
table is an unchanged-prefix cross-check, not permission to regenerate them.

## Exact one-statement proposal

One single-line UTF-8 SQL value, no embedded newline or trailing semicolon:

```sql
CREATE TABLE subathon_reference_initial_captures(installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND length(installation_ref)>0), provider BLOB NOT NULL CHECK(typeof(provider)='blob' AND length(provider)>0), source_account BLOB NOT NULL CHECK(typeof(source_account)='blob' AND length(source_account)>0), identifier_namespace BLOB NOT NULL CHECK(typeof(identifier_namespace)='blob' AND length(identifier_namespace)>0), opaque_id BLOB NOT NULL CHECK(typeof(opaque_id)='blob' AND length(opaque_id)>0), receipt_id BLOB NOT NULL CHECK(typeof(receipt_id)='blob' AND length(receipt_id)=8 AND receipt_id<>X'0000000000000000'), contribution_id BLOB NOT NULL CHECK(typeof(contribution_id)='blob' AND length(contribution_id)=8), configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND length(configuration_ref)>0), arrival_context_ref BLOB NOT NULL CHECK(typeof(arrival_context_ref)='blob' AND length(arrival_context_ref)>0), event_ref BLOB NOT NULL CHECK(typeof(event_ref)='blob' AND length(event_ref)>0), plan_ref BLOB NOT NULL CHECK(typeof(plan_ref)='blob' AND length(plan_ref)>0), plan_revision BLOB NOT NULL CHECK(typeof(plan_revision)='blob' AND length(plan_revision)=8 AND plan_revision=X'0000000000000001'), execution_context_ref BLOB NOT NULL CHECK(typeof(execution_context_ref)='blob' AND length(execution_context_ref)>0), PRIMARY KEY(installation_ref,provider,source_account,identifier_namespace,opaque_id), UNIQUE(installation_ref,receipt_id), FOREIGN KEY(receipt_id) REFERENCES subathon_receipts(receipt_id), FOREIGN KEY(contribution_id) REFERENCES subathon_contributions(contribution_id), FOREIGN KEY(configuration_ref) REFERENCES subathon_retained_configurations(reference), FOREIGN KEY(arrival_context_ref) REFERENCES subathon_retained_contexts(reference), FOREIGN KEY(plan_ref,plan_revision) REFERENCES subathon_plan_revisions(plan_ref,revision), FOREIGN KEY(execution_context_ref) REFERENCES subathon_retained_contexts(reference))
```

Thirteen columns, ten nonempty opaque references and three BLOB8 uint64 fields.
One empty new table, no seed or allocator. Initial revision is exactly encoded1;
ReceiptId1..MAX; ContributionId0..MAX, with no nullable-ID sentinel. Full source
key uses byte-exact SQL BLOB equality, never TEXT collation/Unicode/decimal/coercion.
Upper byte caps remain configurable runtime limits, not hardcoded4096 constraints.

The ordinary rowid table has TWO generated autoindexes (both SQL NULL). The full
proposed head adds these three tuples to ALL89 prefix6 tuples, giving92:

| type | name | table_name | sql |
| --- | --- | --- | --- |
| index | sqlite_autoindex_subathon_reference_initial_captures_1 | subathon_reference_initial_captures | NULL |
| index | sqlite_autoindex_subathon_reference_initial_captures_2 | subathon_reference_initial_captures | NULL |
| table | subathon_reference_initial_captures | subathon_reference_initial_captures | Exact statement above |

The PK index serves exact installation+ALL source components. The second unique
index serves exact installation+ReceiptId and detects another source binding
without scanning all initial captures. No extra prefix-only/hash/nonunique index
is needed. Within a selected installation, a source and actual receipt cannot
acquire competing first meanings. A different installation must still match its
actual retained SRC1/SRX1 automation installation; a changed installation string
cannot evade the original source's closure. There is no invented cross-installation
alias service or second per-installation database in this adapter.

All table/index SQL projections must be observed on selected Qt/SQLite before
fixture freeze; this document's predicted autoindex names/counts are not an
executed driver proof. No trigger/view/ALTER, schema/catalog write shortcut,
foreign-keys-off path, SAVEPOINT, ATTACH, raw handle or IF NOT EXISTS repair.
No SRI1 document is stored redundantly: the thirteen typed columns are its exact
logical members. SRI1 remains a canonical external/internal envelope codec when
explicitly requested; key discovery uses columns/indexes, not document scans.

## Field mapping, query bounds and strict selected validation

| Logical member | SQL fields / representation |
| --- | --- |
| key.installation_ref | installation_ref BLOB |
| key.source.origin | provider/source_account BLOB |
| key.source identity | identifier_namespace/opaque_id BLOB |
| receipt_id, contribution_id | existing positive receipt/full-CID big-endian8 codecs unchanged |
| configuration/arrival/event | configuration_ref/arrival_context_ref/event_ref BLOB |
| plan | plan_ref BLOB + plan_revision existing BE8 exact1 |
| execution binding | execution_context_ref BLOB in existing retained-context namespace |

Actual selected row decoding queries typeof/length/null before payload copy and
uses bounded length/projection guards. No numeric ID passes through SQLite INTEGER
or REAL; binding and decoding are eight actual BLOB bytes. Projection calculations
sum all ten actual reference lengths plus24 with checked arithmetic. Strict class/
required presence/key/ID/revision checks precede equality and return; a larger
well-typed selected field or whole record gives result_limit, not guessed corruption.
A fitting malformed/crosslinked indexed record gives invalid_stored_data. SQL
predicate matching does not certify types or the contents of other selected rows.
No global scan promises to detect wrong-key data outside the exact predicates.

Runtime queries are only fixed adapter-owned statements, main-qualified and bound
through the existing public session.execute. Each source/install+receipt lookup
uses its respective index and bounded LIMIT2 ambiguity guard; no LIMIT1 hides a
second row. Related lookups use existing exact receipt/CID/config/context/plan1
keys and indexed child `(plan_ref,revision,ordinal)` ranges. Fixed cardinality
preflight counts selected rule/request/modifier rows before their bounded payload
materialization, never applies a full-growing-history quota. No index exists for
a generic arbitrary rule lookup, and none is needed by this selected closure.
The consumer cannot inject table/column/SQL text through an identifier field.

`execute` itself owns returned QVariant buffers. Accordingly a payload query that
returns a large document and its length together is already too late for018's
preflight. First issue fixed metadata-only selections for typeof/null/length and
fixed-width scalar checks, with bounded rows/columns and writer accounting. Sum
the WHOLE selected semantic DTO, validate field/document/container ceilings, and
only then select its payloads. Guard payload selection by the validated exact key,
classes and lengths; any unexpectedly changed projection is stored corruption,
not permission to copy an unchecked value. No external writer can interleave
within this writer transaction; these private synchronous reads do not yield or
invoke a callback that can mutate the unit. Metadata/guard-query costs still count
against the same cumulative session budgets. Index lookup metadata must likewise
bound the complete thirteen-column result before fetching its reference payloads.

The FKs enforce existence only. Runtime independently checks:

- receipt identity and original arrival/facts against the complete scoped key and
  typed retained configuration/arrival; no latest retry/attempt substitution;
- contribution ID/original support receipt/observation association, including CID0,
  captured immutable support conditions rather than current mutable dimensions;
- initial plan1/no parent/override and its full original event, exactly1 rule,
 0 modifiers and1 canonical integer request bound to the captured rule;
- SRC1/SRA1/SRX1 exact role/magic/format/known links, byte/value-equal captured rule,
  every duplicated key/ID/ref and exact installation automation binding.

No FK is invented for opaque event/rule/principal names. A composite receipt
identity FK would require altering old receipt unique definitions, which is
forbidden; the existing receipt FK plus complete runtime comparison supplies that
obligation. No additional plan-per-receipt constraint is added to old generic
plans. This reference index is narrower than their supported data and does not
change targeted/full-plan semantics or deny legitimate old records.

## Literal boundary observations proposed for later fixture authors

After independently arranging the complete healthy A closure in the concrete
companion, the one index row has these literal13 values, in column order:

```text
X'49', X'73796e7468657469632e7265666572656e63652e7631', X'41', X'4e', X'45',
X'0000000000000007', X'0000000000000000', X'43', X'58', X'56', X'50',
X'0000000000000001', X'59'
```

These are expected values derived from literal names/BE8, not output of a future
store. Check every value with actual typeof/length/hex at the SQL boundary; ten
reference lengths sum31, plus three8-byte scalars gives record B55. The SAME exact
source predicate and `(installation_ref=X'49',receipt_id=X'0000000000000007')`
predicate must each select precisely this one row, including CId0. A different
opaque ID X'4500' or source account X'4100' is a distinct key; no C-string prefix
comparison is allowed. Corresponding valid separate closures are still required
before inserting those rows. A raw SQL shape test does not establish closure.

Later real-driver primitive diagnostics should separately demonstrate PK rejection
of a second row with the five same key values, unique-key rejection of another
source with the same I/Receipt7, all six FK target obligations, rejection of TEXT
or INTEGER IDs, Receipt0 and revision2, and acceptance of BLOB8 CId0/MAX with real
matching parents. Do not disable FKs in normal setup to make these pass. Deliberate
post-setup corruption belongs to a separately controlled diagnostic arrangement;
its raw writes are never the production adapter. SQL constraints do not validate
synthetic support/rule/principal meaning; actual facade cases must separately
observe source_mismatch/missing_reference and indexed corruption/whole rollback.

The A SRI1 document's independently framed99 bytes, grouped only for reading,
are literal (concatenate groups; spaces/newlines are not bytes):

```text
53524931 00000001 49 00000016 73796e7468657469632e7265666572656e63652e7631
00000001 41 00000001 4e 00000001 45 0000000000000007 0000000000000000
00000001 43 00000001 58 00000001 56 00000001 50 0000000000000001
00000001 59
```

The expected outer format is exactly subathon.reference.initial-capture.v1,
37 bytes; complete envelope B136. This annotation is not an existing stored
payload or an observed codec/SQL execution. The schema stores the typed columns,
and later codecs/fixtures must agree with the independent literal bytes rather
than generate their expected value through the implementation under test.

## Staging, manifest oracles and future evidence

Plan1 rows/children, retained SRX1 and index INSERT occur after ONE real BEGIN on
the same session, with immediate FKs satisfied by staging parents first. None is
acknowledged durable before real COMMIT and normal work completion. The unit's
conflict/refusal/exception cleanup removes new orphan plan/SRX/index together;
committed receipt/support units remain. No release/reset/repair of the original
first binding exists. Known failure must have actual whole rollback, while real
unknown remains unknown/fail-closed/no replay under the original writer contract.

Independent F framing uses the existing catalog grammar without calling a future
factory: version7, proposed ID reference-initial-captures-v1, sql_count1, the exact
statement, full object_count92 and all sorted89+3 tuples. Prior version6 should
first reframe to the unchanged89890/9f5b61d4 value. The proposed new statement byte
length/payload/hash are recorded below after an independent stdlib text-only
calculation. They are DOCUMENT oracles only; no SQL engine/probe/schema/native
program is run by this assignment. Later independent fixture authors must verify
all bytes/NULL-index tuples/prefixes again, then demonstrate selected real Qt
setup separately before genuine feature red. Root may change the PROPOSED7
coordinate only through explicit reviewed artifact/baseline updates, never by
silently rewriting immutable1–6 or claiming an unobserved digest/driver result.

Canonical payload is PER migration step: only that step's SQL statement list,
then the full schema projection at that head. Step6 frames its own12 statements;
step7 frames this ONE1977-byte statement plus the full92-object projection.
Consequently step7's payload can be smaller than step6's89890; it is neither a
concatenated prefix digest nor a payload containing all earlier SQL lists. Each
old step and its independent digest remains exact and is copied unchanged.

Text-only independent proposal calculation: statement UTF-8 bytes **1977**; complete step7 canonical payload **71601 bytes**, SHA256
`efe4d5927002c1849ee51b9c3a988ea5b8bcab9eb8547ad5f12387b27c33b218`. Unchanged prefix6 reframe PASS89890/9f5b61d4; predicted full92 objects, no SQL executed.
