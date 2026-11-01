# Identified-receipt physical schema — revision 1 proposal

Root-owned companion to [receipt SQL revision1](sqlite-receipts-v1.md),
STORAGETEST-013 / STORAGE-013; specification v0.45 §§3.2–3.4 and DB-01.
This is an actual contract-stage proposal for review, not source migration code,
a new acceptance fixture or permission to implement. The initial author owns the
logical adapter contract; root owns this exact additive catalog layout/order.

## Catalog prefix and exact statements

A future pure `receipt_repository_manifest()` must start with the existing
`retained_context_manifest()` step1 bit-for-bit, then append version2,
ID `identified-receipts-v1`, catalogformat1/storeidentity `subathon.sqlite-store`,
supported starting versions `[0,1,2]`. Migration1's ID, two DDL statements,
seven-object projection, 2000-byte canonical payload and
SHA256 `0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e`
remain unchanged. Explicit runner advancement still commits the complete missing
prefix atomically; opening a writer/constructing a repository does no migration.
The caller must inspect/advance before normal use under existing operational gates.
No receipt type or SQL codec enters core.

Version2 executes the following four single statements in this order. Each fenced
line is one exact UTF-8 SQL value with no trailing semicolon or embedded newline;
formatting changes would change reviewed history identity.

Statement1:

```sql
CREATE TABLE subathon_receipt_allocator(singleton INTEGER PRIMARY KEY CHECK(typeof(singleton)='integer' AND singleton=1), last_allocated BLOB CHECK(last_allocated IS NULL OR (typeof(last_allocated)='blob' AND length(last_allocated)=8 AND last_allocated<>X'0000000000000000')))
```

Statement2:

```sql
CREATE TABLE subathon_receipts(receipt_id BLOB NOT NULL PRIMARY KEY CHECK(typeof(receipt_id)='blob' AND length(receipt_id)=8 AND receipt_id<>X'0000000000000000'), provider BLOB NOT NULL CHECK(typeof(provider)='blob' AND length(provider)>0), source_account BLOB NOT NULL CHECK(typeof(source_account)='blob' AND length(source_account)>0), identifier_namespace BLOB NOT NULL CHECK(typeof(identifier_namespace)='blob' AND length(identifier_namespace)>0), opaque_id BLOB NOT NULL CHECK(typeof(opaque_id)='blob' AND length(opaque_id)>0), event_type BLOB NOT NULL CHECK(typeof(event_type)='blob'), schema_version INTEGER NOT NULL CHECK(typeof(schema_version)='integer' AND schema_version BETWEEN 0 AND 4294967295), source_timestamp_utc_ms INTEGER CHECK(source_timestamp_utc_ms IS NULL OR typeof(source_timestamp_utc_ms)='integer'), actor_id BLOB CHECK(actor_id IS NULL OR typeof(actor_id)='blob'), actor_display_name BLOB CHECK(actor_display_name IS NULL OR typeof(actor_display_name)='blob'), money_original_decimal BLOB, money_currency BLOB CHECK((money_original_decimal IS NULL AND money_currency IS NULL) OR (typeof(money_original_decimal)='blob' AND typeof(money_currency)='blob')), payment_namespace BLOB, payment_opaque_id BLOB CHECK((payment_namespace IS NULL AND payment_opaque_id IS NULL) OR (typeof(payment_namespace)='blob' AND typeof(payment_opaque_id)='blob')), activity_namespace BLOB, activity_opaque_id BLOB CHECK((activity_namespace IS NULL AND activity_opaque_id IS NULL) OR (typeof(activity_namespace)='blob' AND typeof(activity_opaque_id)='blob')), server_timestamp_utc_ms INTEGER NOT NULL CHECK(typeof(server_timestamp_utc_ms)='integer'), connection_ref BLOB NOT NULL CHECK(typeof(connection_ref)='blob'), configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND length(configuration_ref)>0) REFERENCES subathon_retained_configurations(reference), modifier_context_ref BLOB NOT NULL CHECK(typeof(modifier_context_ref)='blob' AND length(modifier_context_ref)>0) REFERENCES subathon_retained_contexts(reference), UNIQUE(provider,source_account,identifier_namespace,opaque_id))
```

Statement3:

```sql
CREATE TABLE subathon_receipt_attempts(receipt_id BLOB NOT NULL CHECK(typeof(receipt_id)='blob' AND length(receipt_id)=8 AND receipt_id<>X'0000000000000000') REFERENCES subathon_receipts(receipt_id), ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal>0), event_type BLOB NOT NULL CHECK(typeof(event_type)='blob'), schema_version INTEGER NOT NULL CHECK(typeof(schema_version)='integer' AND schema_version BETWEEN 0 AND 4294967295), source_timestamp_utc_ms INTEGER CHECK(source_timestamp_utc_ms IS NULL OR typeof(source_timestamp_utc_ms)='integer'), actor_id BLOB CHECK(actor_id IS NULL OR typeof(actor_id)='blob'), actor_display_name BLOB CHECK(actor_display_name IS NULL OR typeof(actor_display_name)='blob'), money_original_decimal BLOB, money_currency BLOB CHECK((money_original_decimal IS NULL AND money_currency IS NULL) OR (typeof(money_original_decimal)='blob' AND typeof(money_currency)='blob')), payment_namespace BLOB, payment_opaque_id BLOB CHECK((payment_namespace IS NULL AND payment_opaque_id IS NULL) OR (typeof(payment_namespace)='blob' AND typeof(payment_opaque_id)='blob')), activity_namespace BLOB, activity_opaque_id BLOB CHECK((activity_namespace IS NULL AND activity_opaque_id IS NULL) OR (typeof(activity_namespace)='blob' AND typeof(activity_opaque_id)='blob')), server_timestamp_utc_ms INTEGER NOT NULL CHECK(typeof(server_timestamp_utc_ms)='integer'), connection_ref BLOB NOT NULL CHECK(typeof(connection_ref)='blob'), configuration_ref BLOB NOT NULL CHECK(typeof(configuration_ref)='blob' AND length(configuration_ref)>0) REFERENCES subathon_retained_configurations(reference), modifier_context_ref BLOB NOT NULL CHECK(typeof(modifier_context_ref)='blob' AND length(modifier_context_ref)>0) REFERENCES subathon_retained_contexts(reference), disposition INTEGER NOT NULL CHECK(typeof(disposition)='integer' AND disposition IN (0,1,2)), PRIMARY KEY(receipt_id,ordinal))
```

Statement4:

```sql
INSERT INTO subathon_receipt_allocator(singleton,last_allocated) VALUES (1,NULL)
```

## Exact resulting projection

The full projection has13 objects. Retain all seven version1 objects unchanged.
Add the following three tables with SQL exactly equal to their CREATE statements
above, plus three engine-generated index objects with SQL NULL:

| type | name | tbl_name | sql |
| --- | --- | --- | --- |
| index | sqlite_autoindex_subathon_receipt_attempts_1 | subathon_receipt_attempts | NULL |
| index | sqlite_autoindex_subathon_receipts_1 | subathon_receipts | NULL |
| index | sqlite_autoindex_subathon_receipts_2 | subathon_receipts | NULL |
| table | subathon_receipt_allocator | subathon_receipt_allocator | Statement1 |
| table | subathon_receipt_attempts | subathon_receipt_attempts | Statement3 |
| table | subathon_receipts | subathon_receipts | Statement2 |

Catalog sorting/framing remains migration revision1 unsigned UTF-8 tuple order.
The INTEGER PRIMARY KEY singleton adds no index. Statement4 initializes exactly
one `(1,NULL)` allocator row; NULL means no allocated receipt. No `sqlite_sequence`,
AUTOINCREMENT, trigger, view, JSON/REAL ID or second catalog is introduced.

All byte-bearing columns use BLOB and explicit storage-class constraints. Every
optional pair requires both SQL NULL or both BLOB, preserving present-empty values.
INTEGER fields store signed64 timestamps, uint32 schema version and private
positive signed64 attempt ordinals; original receipt IDs use nonzero eight-byte
big-endian BLOBs and never pass through signed SQLite INTEGER. Pure codec zero
is valid, while domain ID0 is rejected by these constraints. Foreign keys link
receipt/attempt arrivals to both typed retained namespaces and attempts to their
original. Foreign-key enforcement stays enabled by the existing writer.

No SQL immutable trigger is needed: trusted primitives expose no update/delete
except allocator advance. Arbitrary trusted direct SQL is not sandboxed by this
adapter. Runtime selected validation additionally covers structural/history,
strict actual-type/bounds, retained-document/known-link and allocator consistency;
DDL alone never establishes completeness of receipt history or exact port errors.
No history repair, absent-link enrichment or crosslink-equality policy is added.

## Independent proposal observations and remaining gates

Root's disposable Python stdlib SQLite probe created metadata, unchanged retained
DDL and these four exact statements without invoking any future product factory.
It observed the proposed13-object schema and byte-exact stored CREATE values.
The separate actual local Qt6.12/private-writer probe also observed the exact13
objects, NULL autoindex SQL and allocator `(1,NULL)` on SQLite3.54.0, with one
real BEGIN/COMMIT. [Probe evidence](../evidence/storagetest-013-probe.md) records
actual commands and limits. Different contract review and official CI fixture
proof remain required before test freeze. This is diagnostic environment work,
not an authored feature test or intended red.

Independent Python length-framing/SHA256 calculation over this actual proposed
version2 SQL and full13-object projection yields:

- Payload bytes: `10535`.
- SHA256: `24614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2`.

These values are proposal oracles, not values from production. The future test
author must independently verify the literal full payload/projection/digest and
immutability of prefix1, then supply a selected-Qt green environment fixture.
Actual API/stubs, independent SQL integration cases and separate full test review,
successful compilation and genuine missing-behavior red still precede STORAGE-013.
Complete app transactions, downstream014/015 codecs/rates, effects/jobs, backup,
update authority, recovery and platform/provider proofs remain mandatory later.
