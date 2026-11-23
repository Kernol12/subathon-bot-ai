# SQLite rate overrides and effective revisions — revision 2

RATES-010 [#727](https://github.com/Kernol12/subathon-bot-ai/issues/727), Tier A.
Specification v0.45 §9 / FX-06. Owner decisions
[42A–46B](https://github.com/Kernol12/subathon-bot-ai/issues/727#issuecomment-7020229285)
and the retained [#406 rules](https://github.com/Kernol12/subathon-bot-ai/pull/724#issuecomment-7008906120)
govern this boundary. Review receipts and freeze status are recorded in the PR.

## Scope and entry points

- The authoritative Qt command/state owner makes previews from committed rates and
  supplied current source facts. One controlled SQLite writer work item handles
  save revalidation and atomic persistence. A typed repository borrows a revocable
  [work session](writer-transactions-v1.md); it opens no connection, begins no
  transaction, commits nothing itself and retains no session across calls.
- Use the frozen [rate model](rate-snapshots-api-v1.md) and
  [#406 header](../../core/include/subathon/core/rate_override.hpp) directly.
  A preview is owned data, never authority. A manual command adds one override
  or explicitly replaces the active override for the same unordered pair.
  The standalone removal operation is specified by [RATES-013](sqlite-rate-override-removal-v1.md).
- The only source inputs here are current observations and ordered edges supplied
  by the owning authority. This contract defines the persistence shape for
  successors produced with those facts. Provider calls, successful-sync
  determination, source selection policy, scheduling, reporting currency and
  historical valuation/application capture wiring stay with their owning tasks.
  There is no inference from provider identity, an unaccepted fetch, a client DTO,
  or absence of a field.

## Preview, permission and intent

1. Resolve the current installation/account/session and effective USE graph on
   the authoritative path, again in the writer unit on every save or retry before
   protected lookup. Evaluate ordinary `rates.override.edit.v1`, exactly one
   resource slot `pair`, zero quantity slots, absent helper target. For two
   distinct nonempty currency byte strings of at most 64 bytes, the typed scope
   is `lower_hex(B(low) B(high))` (maximum 272 bytes), sorted by unsigned bytes.
   Decode the entire scope, require canonical lowercase hex and strict low < high.
   EUR/USD in either direction is `0000000345555200000003555344`.
   Use this codec for every rate `allowed_set` value in USE and GRANT. Rate allows
   must enumerate pair scopes. An ordinary Any deny is still valid. Apply the
   existing precedence and require one whole USE witness for the requested pair.
   No delegation or owner shortcut. Both override kinds use this same operation.
2. Bind the preview to installation, authenticated account, committed base,
   proposed successor, new decision and override, the entered direction, rate
   spelling, kind, reason, and optional named replacement. The new override's
   actor is the authenticated account, and its `recorded_at_utc_ms` is absent.
   The #406 candidate has absent construction time. Preserve the exact candidate,
   replacement and ordered directed changes in the preview binding. Bind current
   source input as `Basis` below plus its complete ordered observations. The
   command owner binds the submitted edit to that preview. IDs and provenance
   remain stable across retries. A supplied candidate is an equality operand.
3. Recompute a new intent in the write unit from the actual base and current
   supplied authoritative inputs using `preview_rate_override`. Check the
   identity of the base with `check_rate_preview_base`. Changed base or changed
   source facts behind the preview require a fresh preview, even when every
   displayed rate compares equal. Refuse if the preview and recomputation differ
   in complete typed meaning, including provenance and unset versus set fields.
   Apply manuals first, select compatible fetched edges in supplied order,
   preserve full observations, and compare the normalized candidate in canonical
   model order. Retain source selection order separately from that canonical
   storage order. Named replacement must match the active pair in either
   orientation; reject a wrong or absent target and an add with a spurious target.
   Rate positivity, contradiction, bounds, exact inverses and path consistency
   follow the pure rules. The durable active set also enforces one override per
   unordered pair, including when reading back otherwise valid legacy-model DTOs.

The complete trusted rate catalog is `permission-rates-v1`. It retains the
[reference trust](sqlite-permission-graph-v1.md#1-ownership-and-bootstrap)
inventory in order and its three management bindings, then appends the rate
operation above. The composition root supplies this separate exact trust value.
For an existing installation the trusted deployment owner uses
`synchronize_catalog(expected_graph_revision)` on that trust, validating both
stored channels, including the typed pair scopes, before the existing CAS. No
auto-sync at save or retry. Missing, unsynchronized or inconsistent authority
refuses under the existing graph rules. Rate policy admission validates typed
scopes before they can authorize. Shared integration owns these profile producers
and adoption. The fixed `permission-reference-v1` producer and consumers retain
their exact-match behavior. An installation advanced to the rate profile requires
compatible composition for consumers sharing that graph.

The operation key is stable and installation-scoped. Compare complete canonical
intent (defined below), including immutable content, spelling and optionality.
Session and graph revision can change between attempts. Store the first accepted
audit once, with acceptance time exclusively in the decision. Credentials and
token digests are outside all retained envelopes.

For a structurally fitting and authorized request, use this order.

| Existing operation / base | Outcome |
| --- | --- |
| Operation present, exact intent, complete retained closure | Return its immutable original receipt, even after head or live source-policy advance. No writes, new decision or second head publication. |
| Operation present, other operation kind | Verify the retained closure, then return identity conflict without disclosing the retained payload or writing. |
| Operation present, changed intent | Identity conflict without revealing retained contents or writing. Changing original spelling, entered direction, actor, target, base, source binding or candidate identity is a change even at an equal rate. |
| Operation absent, base spent | Stale. Minting a new operation key does not permit reusing the preview. |
| Operation absent, base current | Recheck current source inputs and exact preview, reject revision/override/decision identity collisions, then attempt the head CAS and atomic write. |

An unavailable operation lookup or invalid selected closure gives a typed
storage refusal, not absence. Never let current facts repair the old result.
A refreshed session of the same account may authorize an exact retry; its
metadata never replaces the original audit. Keep distinct authorization,
stale, intent/identity conflict, proposal/resource, invalid/unsupported
retained data and writer outcomes. Under current authority the receipt returns
decision, base, successor, introduced override and optional replaced refs, plus
ordered directed changes.
A denied retry discloses neither operation existence nor receipt and writes zero
rows. Session, graph, witness and source-basis evidence stay internal.

## Canonical envelopes and bounds

Use the [bridge RSV1 grammar and profile](exact-valuation-storage-v1.md#5-complete-canonical-byte-grammars)
unchanged. Here `B` is U32 length and exact bytes, `O` has tags 0/1,
`V` is U32 element count, and integers are big-endian (I64 two's complement).
`Override` and `Observation` below are their exact RSV1 bodies. Each row-carried
format ID is an exact BLOB of 1–128 bytes, paired with the literal four ASCII
magic bytes. The bridge defines the format. The shared RSV1 codec is still open
in [#553](https://github.com/Kernol12/subathon-bot-ai/issues/553).

| Envelope | Format ID |
| --- | --- |
| RIN1 | `subathon.internal.rate-override-intent.v1` |
| RDC1 | `subathon.internal.rate-override-decision.v1` |
| ROB1 | `subathon.internal.rate-observation.v1` |

```text
RIN1 B(installation) B(operation_key) B(base) B(successor)
     Override O(B(replaced_override)) Basis
Basis = B(source_policy_basis) V(B(observation_ref)) V(Edge)
Edge = U32(observation_index) U64(quote_ordinal)
RDC1 B(installation) B(operation_key) B(decision) B(base) B(successor)
     B(introduced_override) O(B(replaced_override)) B(account)
     B(session) B(session_record_revision) B(session_snapshot)
     B(permission_catalog_revision) U64(permission_graph_revision)
     Witness O(I64(accepted_at_utc_ms))
Witness = U8(0) B(source) B(grant)                           # individual allow
        | U8(1) B(source) B(grant) B(role)                   # role allow
ROB1 Observation
```

- Both envelopes bind the same decision ref through `Override` and `RDC1`.
  Required refs and present optional refs are nonempty. Installation, session,
  session record revision, session snapshot and witness source/grant/role refs
  are at most 4,096 bytes each. Other envelope refs are at most 256 bytes each.
  Nested fields follow the bridge (currency 64, decimal/time text 512, reason
  4,096 bytes). Preserve original text, including present-empty time text.
  Graph revision is positive. Session fields come from current session resolution.
  Copy the original evaluator permit witness, including role only for a role
  allow. Its operation/pair are implicit from the fixed catalog and bound edit.
  The witness is audit evidence, with no retained live authorization. Decision
  time is the authoritative acceptance sample if known, otherwise absent.
- The source port supplies one basis identity, at most 16 observations in
  supplied order with unique refs, and at most 16,384 edges in supplied order.
  Both vectors may be empty. The index is zero-based in that observation
  vector, and the ordinal is one-based in its full quote vector. Bounds and
  membership are checked even for edges unused by #406. Preserve edge order
  and multiplicity. Identity binds the complete facts, including unused rows.
- RIN1 is at most 229,376 bytes and RDC1 at most 32,768 bytes, including magic
  and framing. RSV1 and ROB1 are each at most 32,768 encoded bytes. Each raw
  and parsed revision and each raw and parsed standalone observation separately
  fits 24,576 logical bytes and the bridge count/field/bit limits. With at most
  16 ROB1 documents, the input basis has at most 524,288 encoded bytes total.
  Copies embedded in RSV1 are charged again against that document's limits.
- Scan declared counts and sizes before copying or allocating, with checked
  arithmetic and the bridge's framing/value admission sequence. Limit overflow
  is a resource refusal. Bad physical types, magic, tags, truncation, trailing
  bytes and noncanonical known contents are invalid data. Unknown well-bounded
  format IDs are unsupported, with no fallback.
  #406's combined raw proposal/replacement/affected output budget of 8,388,608
  logical bytes and 32,768 items still applies. Preflight the selected closure
  and actual cumulative writer statements, rows and bound/result bytes as well
  as enclosing owned-value limits. Refuse the entire input on overflow. Keep
  original evidence and existing writer/bridge defaults.

## Logical rows and selected receipt

Use one additive **RATES-010 step** after the protected prefix in the posted
night order. Version, ID, DDL and projection follow the reviewed assignment.
Preserve shipped bytes and digests. Seed in the controlled catalog transaction.
Opening the store cannot advance it. Apply [migration rules](migration-catalog-v1.md)
including populated-prefix and old-manifest refusal.

| Relation | Key in this server's rate store | Other logical columns and constraints |
| --- | --- | --- |
| Observation | observation ref | Format + ROB1. Complete immutable context and ordered quotes. Reuse only identical canonical bytes. Retain every admitted basis observation, including those with no selected edges. |
| Revision | snapshot ref | Nullable parent FK to Revision, unique among non-null parents. Format + complete RSV1. All nonroot revisions have a real earlier parent. |
| Decision | decision ref | Installation ref and operation key form a unique composite. Successor is separately unique. Base and successor are Revision FKs. Introduction and replacement use paired RIN1/RDC1 envelopes. Standalone removal uses paired RRI1/RRD1 envelopes under RATES-013. All envelope refs agree with their columns and each other. |
| Occurrence | override ref | Introducing decision FK (unique), introducing revision FK and complete canonical RSV1 Override body (no envelope). The body matches the introduced override in that revision and the bound edit. Later active references keep those same bytes. Retain replaced, removed and expired occurrences permanently. A standalone removal introduces no occurrence. |
| Head | one-byte `H` (0x48) | Active revision FK. Exactly one head, at a terminal revision. Seed with reserved one-byte ref `E` (0x45), absent parent/time and empty vectors. Reserve E from successor generation. A missing or malformed head or root is invalid data. |

Keys and payloads are byte-exact BLOBs. Rate rows share the single-server
namespace. Only operation lookup is installation-scoped. Seed E and H without
requiring a preexisting account installation. Use explicit physical type/length
checks and local FKs on relational references.
All rows except Head are immutable and retained. An Occurrence body is at most
5,535 bytes under the new-override field and provenance rules.
Validate envelope-only links explicitly. For RIN1/RDC1, select the introduced
occurrence by its unique introducing decision and match the RDC1 ref. An
RRI1/RRD1 decision cannot introduce an occurrence. Its target is an existing
occurrence retained under the original introducing decision. A
replacement names an existing occurrence present in the base and absent from
the successor, on the same unordered pair. Its stored body must equal the base's
override. Keep the revision store separate from valuation snapshot identities.
Audit refs can outlive live account/session/graph rows. Source refs stay opaque
outside this store. Neither SQL affinity nor hashes establish exact identity.

The canonical intent is RIN1 plus the exact successor RSV1 and complete ROB1
documents in Basis order, resolved by those exact typed keys. RIN1 refers to
those documents by key. Compare submitted preview material to the whole
intent, including unused observations. Matching reference bytes alone cannot
prove a match. A changed same-ref body conflicts with a new submission; an
internally inconsistent retained closure is invalid data. Check the retained
candidate against the captured edit and ordered source input under #406, using
only those retained facts. Check selected observations byte-for-byte with the
basis documents, including original quote rows excluded as constraints.

Read the decision, both immutable revisions, each distinct occurrence keyed by
either revision's overrides, and basis observations in one coherent view. Check
exact keys, bodies, parent and typed links against both revisions. Never consult
ancestry, unrelated history, the current rate head or the live source policy.
Preflight row counts and lengths before fetching BLOBs. Enumerate distinct pairs
over the unsigned-byte-sorted member union, from then to. Return reduced exact
`weight(to)/weight(from)` when both endpoints share a component and absence
otherwise. Emit only changes in exact value or availability. Equal-currency
identity is omitted. Rebuild the list under the same proposal budget and compare
any supplied preview changes. No persisted delta list or reporting-currency
interpretation is needed.

## Write, commit and reopen

The command owner starts one real writer transaction. Check every active base
override against its exact occurrence key and body before staging. Charge direct
selected reads to the same cumulative writer budget on save and receipt lookup.
Stage needed observations, the new effective revision, then its exact decision
and introduced occurrence. Conditionally update the singleton from the
exact preview base to the candidate. Check exactly one row changed. A late
zero-row CAS or failed write after staging rolls back the entire unit; invalid
cardinality/closure is a storage failure. A healthy refusal stages nothing.
Concurrent submissions with the same base serialize; only the first new intent
can advance the pointer. The same operation and intent can only receive the
original committed receipt. Any reached SQL/limit failure preserves the
underlying writer error and rollback classification.

A staged result or active-session read is provisional. Only normal completion
of acknowledged physical COMMIT can publish a newly accepted revision/receipt.
A committed transaction that then fails its work body has no normal success
payload. An indeterminate COMMIT is outcome unknown and closes writer admission
under the existing contract. Neither completion publishes success or starts an
automatic replay. Resolve through a fresh authoritative, authorized, committed
read after reopen, with one coherent selected closure and exact operation
identity/intent. A full matching closure establishes the first committed
outcome even if newer revisions are active. A conclusively known rollback or
verified absence of the entire attempted write permits only a newly authorized
attempt under normal base/intent rules. Partial, conflicting, unsupported or
unavailable evidence remains unresolved. Recovery of a historical receipt never
publishes that revision again as active.

For a separately supplied accepted-update witness, apply
`propose_rate_update` over the preceding revision and the accepted observation's
own quotes, before manuals. A direct or derived fetched rate for a temporary
override's pair expires it even at the same value. Unrelated temporaries and
all pins survive. No accepted observation yields no successor. The revision
shape and head rules can store a valid shorter successor; an override-edit
grant confers no authority to certify a successful fetch or invoke that path.

## Independent examples for review

| Case | Inputs and expected evidence |
| --- | --- |
| R01 — Root and first addition | Reopen verified root E before a successful fetch. RSV1 has length 27 (4 magic + 5 ref + 2 absent tags + 4 empty vector counts). EUR/USD missing, EUR/EUR 1. Add EUR→USD pinned `1.20` with no source. New revision parent E, EUR/USD 6/5, USD/EUR 5/6, exactly two ordered availability changes, one decision and one override. Reopen exactly; no implied fetch or timestamp. |
| R02 — Scope and denial | The EUR/USD grant covers either direction and both kinds. EUR/GBP needs its own pair scope. Deny on no complete grant, individual deny, revoked session, unavailable graph, or unsynchronized catalog, without protected lookup or writes. Independently revoke the grant after preview and between a committed save and exact retry. |
| R03 — Named replacement and exact graph | Base has EUR/USD pin 6/5 and source rows EUR→GBP `0.90`, EUR→USD `1.20` in that policy order. Replace the named pin with USD→EUR `0.80` using the same source. EUR/USD becomes 5/4, USD/EUR 4/5, USD/GBP becomes (9/10)/(5/4)=18/25, inverse 25/18. Old USD/GBP is 3/4, inverse 4/3. The EUR/GBP pair stays 9/10 and 10/9. Preserve both full source rows and their ordinals. Exclude EUR→USD as a constraint. Reject absent, wrong or other-pair replacement refs and zero/negative rates. In a separate base with EUR/USD pinned at 5/4 and EUR/GBP pinned at 9/10, USD→GBP `0.72000000000000000001` contradicts the exact cross rate 18/25. |
| R04 — Facts, replay and collision | K adds EUR/USD `1.20` from E to S. K2 from S explicitly replaces K's override at the same rate and advances to T with zero rate changes. An authorized K retry returns S and its two original changes, with T still active. Fresh K3 from E is stale. A changed K lexeme, actor, decision or replacement conflicts. On the current base, any new operation reusing an existing override, decision or successor ref conflicts. A changed current source-policy basis or edge trace on a fresh operation needs a new preview even when the head and rates agree. |
| R05 — Observation fidelity | Selected F has a conflicting second row and unselected G has no accepted edge. Reopen both full facts from the decision basis, with F's source time original-only, fetch UTC0 and present-empty publication text distinct from absence. Same F identity with a changed lexeme or time presence conflicts before any durable changes. |
| R06 — Smaller successor | Temporary USD→EUR 7/5, pinned EUR→GBP 1/2, unrelated temporary JPY→CAD 3/250. Successful supplied USD→CHF `0.70`, CHF→EUR `2.0` imply USD→EUR 7/5, so only the first override expires. Use compatible fetched edges and derive USD→GBP 7/10. Repeat with direct same-value fetched USD→EUR, unrelated-only quotes, and no accepted observation. Reopen both override counts and original provenance. |
| R07 — Race and late failure | Two fresh commands share E. First commits, second is stale, and two identical K submissions yield one commit and one retained receipt. Force a reached late write/CAS failure after staging and verify from an independent committed view and reopen that the old head and every prior row survive and all staged rows are absent. |
| R08 — Uncertain and post-commit error | Lose acknowledgement after physical commit and separately fail the body after confirmed commit. Both lack a normal payload. The former is unknown and fail-closed; the latter is committed with a work error. Fresh selected read proves one exact committed closure. A partial closure or wrong intent cannot prove acceptance; full absence or known rollback still requires the ordinary authorization and freshness gates. |
| R09 — Catalog and corruption | Keep the populated prefix intact across advance and reopen. Refuse an older manifest. For K1 adding pin A and K2 adding pin B, remove A's occurrence or change its spelling from `1.20` to `1.200` without changing either embedded revision. A fresh save and an exact K2 retry both refuse retained data. Missing links, a nonterminal head, invalid type/ordinal, duplicate active pair, noncanonical rational and malformed or unsupported format also refuse without repair. |

## Bridge pressure witnesses

These are synthetic full-table shapes admissible to the pure rate model.
They exercise complete evidence storage, independently of provider behavior.

- Four ordered observations use bases CHF, EUR, GBP and USD, each with 210
  quotes to the same 210 distinct three-byte tokens `x00` through `xD1` (hex).
  All spellings are `1.20`; the four base potentials are 1/1 and all targets
  6/5. Use unit-byte snapshot S, parent E, five unit-byte required context refs
  per observation with distinct observation refs, and all optional fields absent.
  The 840 fetched constraints are compatible even though redundant. Raw model
  bytes are 17,892, parsed 19,572, encoded 34,054. This shape crosses only the
  encoded cap. Reducing to 202 quotes per observation gives encoded 32,766.
  Add two bytes to one source evidence ref for exactly 32,768, then one more
  for refusal. Neither step approaches the model cap.
- One 220-quote USD-base observation with the same context/ref profile and
  targets `x00` through `xDB` uses `1.234567890123456789` for each.
  Its reduced target potential has two 19-digit components. Raw model bytes
  are 16,952, parsed 25,312, encoded 23,188. This shape crosses only the
  parsed-model cap. For 213 targets, a 69-byte present source-rate original
  text gives exactly 24,576 parsed bytes; adding one byte fails that ceiling.

Both crossings are resource refusals under the owner-set boundary. They show
what to raise for a decision if a required current source policy needs these
shapes. They establish no need to widen a default and no live-provider limit.
