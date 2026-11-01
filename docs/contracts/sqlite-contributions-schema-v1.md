# Contribution evidence physical schema — revision 1 proposal

Root-owned companion to [contribution SQL revision1](sqlite-contributions-v1.md),
STORAGETEST-014 / STORAGE-014; specification v0.45 §§3.2–3.5/14 and DB-01.
This exact additive migration3 proposal is for separate review, not production
source or an authored acceptance fixture. Immutable retention1 and receipt2 remain
byte-for-byte unchanged. No additional observation ordinal, data duplicate or
semantic reference namespace is introduced.

## Immutable prefix and exact migration statements

The future pure contribution_repository_manifest() copies the actual
receipt_repository_manifest() prefix1/2 and appends version3,
ID contribution-evidence-v1, catalogformat1/storeidentity subathon.sqlite-store,
supported starting versions `[0,1,2,3]`. Prefix1 retains its2000-byte payload/
SHA0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e;
prefix2 retains10535/SHA24614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2
and exact13-object projection. No opening/constructor auto-migrates or grants
operational update authority. Existing explicit runner commits the missing whole
prefix atomically under its original caller preconditions.

Execute these four exact single UTF-8 statements in this order. Each fenced line
is one SQL value with no trailing semicolon or embedded newline; whitespace or
DDL changes alter the reviewed migration identity.

Statement1:

```sql
CREATE TABLE subathon_contribution_allocator(singleton INTEGER PRIMARY KEY CHECK(typeof(singleton)='integer' AND singleton=1), last_allocated BLOB CHECK(last_allocated IS NULL OR (typeof(last_allocated)='blob' AND length(last_allocated)=8)))
```

Statement2:

```sql
CREATE TABLE subathon_contributions(contribution_id BLOB NOT NULL PRIMARY KEY CHECK(typeof(contribution_id)='blob' AND length(contribution_id)=8), original_receipt_id BLOB NOT NULL UNIQUE CHECK(typeof(original_receipt_id)='blob' AND length(original_receipt_id)=8 AND original_receipt_id<>X'0000000000000000') REFERENCES subathon_receipts(receipt_id), activity_provider BLOB, activity_source_account BLOB, activity_namespace BLOB, activity_id BLOB CHECK((activity_provider IS NULL AND activity_source_account IS NULL AND activity_namespace IS NULL AND activity_id IS NULL) OR (typeof(activity_provider)='blob' AND length(activity_provider)>0 AND typeof(activity_source_account)='blob' AND length(activity_source_account)>0 AND typeof(activity_namespace)='blob' AND length(activity_namespace)>0 AND typeof(activity_id)='blob' AND length(activity_id)>0)), payment_evidence INTEGER NOT NULL CHECK(typeof(payment_evidence)='integer' AND payment_evidence IN (0,1,2,3)), monetary INTEGER NOT NULL CHECK(typeof(monetary)='integer' AND monetary IN (0,1)), original_decimal BLOB, currency BLOB CHECK((original_decimal IS NULL AND currency IS NULL) OR (typeof(original_decimal)='blob' AND typeof(currency)='blob')), resolution_ref BLOB CHECK(resolution_ref IS NULL OR typeof(resolution_ref)='blob'), missing_rate INTEGER NOT NULL CHECK(typeof(missing_rate)='integer' AND missing_rate IN (0,1)), review INTEGER NOT NULL CHECK(typeof(review)='integer' AND review IN (0,1,2,3)), evidence_policy INTEGER NOT NULL CHECK(typeof(evidence_policy)='integer' AND evidence_policy IN (0,1,2)), policy_ref BLOB NOT NULL CHECK(typeof(policy_ref)='blob'), decision_choice INTEGER, decision_ref BLOB, decision_actor_ref BLOB CHECK((decision_choice IS NULL AND decision_ref IS NULL AND decision_actor_ref IS NULL) OR (typeof(decision_choice)='integer' AND decision_choice IN (0,1) AND typeof(decision_ref)='blob' AND typeof(decision_actor_ref)='blob')), application INTEGER NOT NULL CHECK(typeof(application)='integer' AND application IN (0,1)), reversal INTEGER NOT NULL CHECK(typeof(reversal)='integer' AND reversal IN (0,1,2)), settlement INTEGER NOT NULL CHECK(typeof(settlement)='integer' AND settlement IN (0,1)), UNIQUE(activity_provider,activity_source_account,activity_namespace,activity_id))
```

Statement3:

```sql
CREATE TABLE subathon_contribution_observations(receipt_id BLOB NOT NULL PRIMARY KEY CHECK(typeof(receipt_id)='blob' AND length(receipt_id)=8 AND receipt_id<>X'0000000000000000') REFERENCES subathon_receipts(receipt_id), contribution_id BLOB CHECK(contribution_id IS NULL OR (typeof(contribution_id)='blob' AND length(contribution_id)=8)) REFERENCES subathon_contributions(contribution_id), kind INTEGER NOT NULL CHECK(typeof(kind)='integer' AND kind IN (0,1)), evidence INTEGER NOT NULL CHECK(typeof(evidence)='integer' AND evidence IN (0,1,2,3)), activity_provider BLOB, activity_source_account BLOB, activity_namespace BLOB, activity_id BLOB, verification_ref BLOB CHECK((activity_provider IS NULL AND activity_source_account IS NULL AND activity_namespace IS NULL AND activity_id IS NULL AND verification_ref IS NULL) OR (typeof(activity_provider)='blob' AND length(activity_provider)>0 AND typeof(activity_source_account)='blob' AND length(activity_source_account)>0 AND typeof(activity_namespace)='blob' AND length(activity_namespace)>0 AND typeof(activity_id)='blob' AND length(activity_id)>0 AND typeof(verification_ref)='blob' AND length(verification_ref)>0)), support_monetary INTEGER, support_original_decimal BLOB, support_currency BLOB, support_resolution_ref BLOB, support_missing_rate INTEGER, support_review INTEGER, support_evidence_policy INTEGER, support_policy_ref BLOB, support_decision_choice INTEGER, support_decision_ref BLOB, support_decision_actor_ref BLOB, support_application INTEGER, support_reversal INTEGER, support_settlement INTEGER, CHECK((kind=0 AND typeof(support_monetary)='integer' AND support_monetary IN (0,1) AND typeof(support_missing_rate)='integer' AND support_missing_rate IN (0,1) AND typeof(support_review)='integer' AND support_review IN (0,1,2,3) AND typeof(support_evidence_policy)='integer' AND support_evidence_policy IN (0,1,2) AND typeof(support_application)='integer' AND support_application IN (0,1) AND typeof(support_reversal)='integer' AND support_reversal IN (0,1,2) AND typeof(support_settlement)='integer' AND support_settlement IN (0,1) AND ((support_original_decimal IS NULL AND support_currency IS NULL) OR (typeof(support_original_decimal)='blob' AND typeof(support_currency)='blob')) AND (support_resolution_ref IS NULL OR typeof(support_resolution_ref)='blob') AND typeof(support_policy_ref)='blob' AND ((support_decision_choice IS NULL AND support_decision_ref IS NULL AND support_decision_actor_ref IS NULL) OR (typeof(support_decision_choice)='integer' AND support_decision_choice IN (0,1) AND typeof(support_decision_ref)='blob' AND typeof(support_decision_actor_ref)='blob'))) OR (kind=1 AND evidence=2 AND (support_monetary IS NULL AND support_original_decimal IS NULL AND support_currency IS NULL AND support_resolution_ref IS NULL AND support_missing_rate IS NULL AND support_review IS NULL AND support_evidence_policy IS NULL AND support_policy_ref IS NULL AND support_decision_choice IS NULL AND support_decision_ref IS NULL AND support_decision_actor_ref IS NULL AND support_application IS NULL AND support_reversal IS NULL AND support_settlement IS NULL))))
```

Statement4:

```sql
INSERT INTO subathon_contribution_allocator(singleton,last_allocated) VALUES (1,NULL)
```

## Exact projection and relational boundaries

The full projection has20 objects. Keep all13 prefix2 objects unchanged and add:

| type | name | tbl_name | sql |
| --- | --- | --- | --- |
| index | sqlite_autoindex_subathon_contribution_observations_1 | subathon_contribution_observations | NULL |
| index | sqlite_autoindex_subathon_contributions_1 | subathon_contributions | NULL |
| index | sqlite_autoindex_subathon_contributions_2 | subathon_contributions | NULL |
| index | sqlite_autoindex_subathon_contributions_3 | subathon_contributions | NULL |
| table | subathon_contribution_allocator | subathon_contribution_allocator | Statement1 |
| table | subathon_contribution_observations | subathon_contribution_observations | Statement3 |
| table | subathon_contributions | subathon_contributions | Statement2 |

Sort/frame the complete projection using unchanged migration revision1 rules.
The INTEGER singleton adds no index; Statement4 initializes exactly `(1,NULL)`.
Contribution IDs are BLOB8 including zero; NULL means absent/no allocation.
Receipt IDs remain nonzero BLOB8. Foreign keys reference actual receipt originals
and contributions; UNIQUE protects contribution identity/original support and
one observation per receipt. Every present activity/proof group is exact nonempty
BLOB; current policy/decision strings can be present empty. All enum/bool fields
use explicit INTEGER storage class/known values. Optional money/decision groups
are wholly absent or correctly typed, never partly NULL. Observation support
conditions are present for support; confirmation has entirely absent conditions
and explicit confirmed evidence, matching the existing service's shape validation.
Nullable observation association is independent of optional absence: ID0 is present.

Canonical ReceiptId ordering supplies deterministic unresolved collection order;
there is no timestamp identity or new private ordinal/SQLite random-overflow policy.
Original facts/arrival hydrate from immutable referenced receipts. Runtime strict
original-only decoding validates every supplied/selected projection and actual
retained arrival targets. The SQL constraints establish byte/type/cardinality/
referential structure. Runtime checks additionally enforce association key/no-key
compatibility, no retarget, selected ambiguity/corruption/limits and complete new
support linkage before commit. Trusted arbitrary SQL is not sandboxed by triggers;
no policy/evidence/authorization/semantic resolver is silently added.

Support-condition/current-state dimensions remain independent: nonmonetary does
not erase original money, a resolution ref does not clear missing_rate, and
application/reversal/settlement fields are recorded projection values rather than
proof of effects/permission. No CHECK infers such truth. Policy/actor/decision/
resolution/verification meaning and a receipt's prior commit remain the explicit
upstream preconditions in the logical contract; SQL existence/equality is not
invented proof of those. No new namespace or public receipt helper is needed.

## Independent proposal observations and remaining gates

Root's independent Python stdlib SQLite diagnostic executed metadata, exact
unchanged prefixes1/2 and the four proposed SQL statements without a future
production factory/codec. It observed exactly20objects, exact stored CREATE values,
all prior13 tuples unchanged and allocator `(1,NULL)`. The separate actual local Qt6.12/SQLite3.54 private-writer diagnostic also
matched20objects/exactSQL/NULLindexSQL/unchangedprefixes and initialallocatorNULL.
It bound ContributionId0 into actual new rows and a NULL association separately,
observed BLOB8zero versus SQLNULL and selected only the truly unassociated row.
[Probe evidence](../evidence/storagetest-014-probe.md) pins scope/commands.
Different contract review and later selected official-Qt fixture proof remain required.
These are local environment/proposal observations, not feature acceptance/red.
Independent literal decimal-length F framing and hashlib SHA256 over the actual
proposed step3 SQL/full20-object projection yield:

- Payload bytes: `18126`.
- SHA256: `b72249ba22c4838af98b9ae29600e70969cf3295eea958a30074167221d2ad1d`.

Expected values were not generated by production. The later actual fixture author
must independently verify full payload/projection/typed boundary vectors, then
provide selected official-Qt green setup. Different actual contract review precedes
minimal interfaces/stubs; full test/oracle/harness review +successful compilation
and genuine intended red precede STORAGE-014 production. Existing passing118,
all earlier reviews/reds/prefixes and all seven integrations/three rate providers/
application/operational/platform release scope remain preserved.
