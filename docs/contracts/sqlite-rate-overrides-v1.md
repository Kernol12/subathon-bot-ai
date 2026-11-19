# SQLite rate overrides and effective revisions — revision 1

RATES-010 [#727](https://github.com/Kernol12/subathon-bot-ai/issues/727), Tier A.
Specification v0.45 §9 / FX-06. Owner decisions
[42A–46B](https://github.com/Kernol12/subathon-bot-ai/issues/727#issuecomment-7020229285)
and the retained [#406 rules](https://github.com/Kernol12/subathon-bot-ai/pull/724#issuecomment-7008906120)
govern this boundary. This is a draft for independent contract review.

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
  A standalone removal command belongs to unqueued #733.
- The only source inputs here are current observations and ordered edges supplied
  by the owning authority. This contract defines the persistence shape for
  successors produced with those facts. Provider calls, successful-sync
  determination, source selection policy, scheduling, reporting currency and
  historical valuation/application capture wiring stay with their owning tasks.
  There is no inference from provider identity, an unaccepted fetch, a client DTO,
  or absence of a field.

## Preview, permission and intent

1. The request names two nonempty distinct currency byte strings. Resolve the
   existing current session and effective USE graph in the writer unit. Request
   one ordinary override-edit operation with one pair resource and no quantity
   or kind dimension. The resource is the injective encoding of the two currency
   byte strings sorted in unsigned byte order. Encode the bridge's length-framed
   pair `B(low) B(high)` as lowercase hexadecimal for the permission resource.
   Preserve entered direction for the rate.
   The existing evaluator must find one complete covering grant under its
   ordinary precedence. A grant covering only a different pair, combined
   currency fragments, delegation authority, and owner identity alone give no
   permit. Temporary and pinned use the same grant. Refuse unavailable,
   inconsistent, or stale authority facts. Recheck on every save and retry
   before protected operation, head or evidence lookup.
2. Bind the preview to the exact committed base identity, a distinct proposed
   successor identity, authenticated installation/account, entered pair and
   spelling, kind, reason, override/decision identities and the optional named
   replacement. Bind the current source-policy basis identity and its ordered
   (observation identity, one-based quote ordinal) input and the observation
   identities in the basis. Within a basis all
   observation identities denote immutable complete observations. Retain the
   exact candidate and all ordered before/after exact pair changes, including
   availability changes and the replaced override identity. The command owner
   must bind the operator's submitted intent to this preview; a supplied candidate
   is only an equality operand. Server-generated IDs and any preview metadata
   remain stable in that binding across retries.
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

The request carries a stable operation key scoped to the installation. Its
canonical intent includes the authenticated account and complete preview binding,
including the proposed identities, exact model content and replacement
optionality. Compare complete byte/typed identity, not just number equality or a
digest. The live permission revision/session/time is rechecked separately from
stable intent. Record its original accepted evidence once. No credentials,
token digest, or unverified actor/time can enter the retained decision.

For a structurally fitting and authorized request, use this order.

| Existing operation / base | Outcome |
| --- | --- |
| Operation present, exact intent, complete retained closure | Return its immutable original receipt, even after head or live source-policy advance. No writes, new decision or second head publication. |
| Operation present, changed intent | Identity conflict without revealing retained contents or writing. Changing original spelling, entered direction, actor, target, base, source binding or candidate identity is a change even at an equal rate. |
| Operation absent, base spent | Stale. Minting a new operation key does not permit reusing the preview. |
| Operation absent, base current | Recheck current source inputs and exact preview, reject revision/override/decision identity collisions, then attempt the head CAS and atomic write. |

An unavailable operation lookup or invalid selected closure gives a typed
storage refusal, not absence. Never let current facts repair the old result.
A refreshed session of the same account may authorize an exact retry; its
metadata never replaces the original audit. Keep distinct authorization,
stale, intent/identity conflict, proposal/resource, invalid/unsupported
retained data and writer outcomes. Protected details are returned only through
the applicable authorized operation.

## Retained values and SQLite boundary

Use one additive migration placeholder **RATES-010 step** after the actual
protected prefix in the posted night order. Version, migration ID and literal
DDL/projection are for the reviewed catalog assignment. Copy all shipped
catalog SQL, IDs, projections and digests exactly. Migration creates and seeds
the new relations in the controlled catalog transaction. Opening the store
does not advance it. Apply the existing [migration rules](migration-catalog-v1.md),
including populated-prefix and older-manifest refusal coverage.

| Logical relation | Durable identity and invariants |
| --- | --- |
| Fetched observation | Immutable observation ref to complete context and ordered quotes; store original spelling, provider/config/request/evidence refs and both independent optional time evidences, fetch time and snapshot ref. Identical exact bytes reuse an observation; a changed byte under the same ref conflicts. Retain admitted current observations in the accepted input basis even when no edge survives. |
| Effective revision | Immutable snapshot ref to optional parent and complete canonical RSV1 known-format document, with an owned whole collection of active overrides, selected sources, constraints and rational components. Parent is a real earlier revision in this store. Require one successor at most per parent. Check key/parent against the body. A complete collection may shrink, including to zero. |
| Override occurrence | Permanent unique override ref bound to the revision/decision that introduced it and the exact manual data in that revision. Later references keep that same occurrence unchanged. The first revision, replaced/expired occurrences and redeemed identities remain readable. Never reuse an old ref for a new occurrence. |
| Override acceptance | Immutable operation key, exact canonical intent, unique decision ref and successor, base, introduced override and optional replaced occurrence, ordered source input basis, plus original actor/session provenance, current graph/witness and acceptance time if known. Receipt references this original closure. No in-place update or deletion. |
| Active pointer | Exactly one checked singleton for this server, referencing a real effective revision with no successor. Seed it to reserved revision `E` with absent parent/time and all collections empty. No fetched observation, last-success time, or decision is implied. All subsequent revisions have a parent. A missing or malformed singleton/root in a verified current store is invalid data. |

Use byte-exact keys and envelope payloads (SQLite BLOB), explicit type/length
checks, typed local foreign keys and unique constraints. Audit links that can
outlive live identity/session rows are retained immutable evidence. Provider refs,
external source evidence and metadata stay opaque; do not claim verification of
their external targets. The effective revision retains #406's complete selected
observation closure, including quote rows rejected as constraints. Separate
observation evidence also retains the unselected sources named by accepted
input facts. Validate cross-references and complete canonical bytes before
admitting or returning a selected result. One selected operation read uses its
base, successor, decision, introduced/replaced occurrences and retained supplied
observations in a coherent committed view. Check ordered edges against their
original quote ordinals. Do not read unrelated histories or use the latest head
as part of the old receipt. Render the receipt and its exact changes from the
retained accepted values (retain exact deltas or reconstruct them from the
two selected immutable revisions with the same ordering). Use bounded selected
reads and reject incomplete or conflicting closure.

Use the [bridge RSV1 grammar and profile](exact-valuation-storage-v1.md#5-complete-canonical-byte-grammars),
including original decimals, rational potentials, distinct optional absence,
canonical collection order, type tags, limits and known/unknown format handling.
The standalone observation record uses the same Observation body with an
independently identified envelope. New decision/intent envelopes must have
explicit internal format identity, byte-exact length framing and bounded
fields. Compare or validate full values, not just hashes or SQL affinity. Use
no SQL REAL, currency normalization, rounded quotes, invented timestamp, or
reporting-currency-relative interpretation. An equal-currency query remains
arithmetic identity; an unavailable foreign pair remains missing.

Each complete raw snapshot and parsed snapshot separately fits 24,576 logical
bytes and the bridge count/field/bit limits. Each complete canonical RSV1 document
separately fits 32,768 actual bytes. Apply the same local byte ceilings to
standalone observation values/envelopes; bill selected observation copies in
the snapshot again. Preflight collection counts, logical sizes and measured
framed sizes before copying/materialization at the storage boundary. Reuse
#406's fixed proposal/affected-pair budgets as a separate bound; give decision
metadata and ordered input edges bounded framing and enforce actual cumulative
writer statement, row, bound/result byte limits plus enclosing owned-value
limits before staging. Select at most 16 input observations and 16,384 edges
and use checked arithmetic before allocation. Refuse over-budget inputs whole.
Never discard evidence, select a smaller provider table, or enlarge a bridge or
writer default to make a save fit. The linked bridge supplies the format
contract; a codec is a later implementation dependency, not assumed shipped.
Historical plan layouts, captures and migrations remain with their own owners.

## Write, commit and reopen

The command owner starts one real writer transaction. Validate the complete
selected current data, operation identity and candidate before staging.
Stage needed observations, new effective revision/override occurrence, exact
decision and receipt binding, then conditionally update the singleton from the
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
| R02 — Scope and denial | The EUR/USD grant covers USD→EUR, both kinds, but not EUR/GBP. No grant, an applicable deny, unrelated/incomplete grants, revoked session, or unknown current graph yields no access to protected results or writes. Change graph after preview, then independently revoke it after commit and retry. |
| R03 — Named replacement and exact graph | Base has EUR/USD pin 6/5 and source rows EUR→GBP `0.90`, EUR→USD `1.20` in that policy order. Replace the named pin with USD→EUR `0.80` using the same source. EUR/USD becomes 5/4, USD/EUR 4/5, USD/GBP becomes (9/10)/(5/4)=18/25, inverse 25/18. Old USD/GBP is 3/4, inverse 4/3. The EUR/GBP pair stays 9/10 and 10/9. Preserve both original source rows and their ordinals; EUR→USD is excluded as a constraint. Reject absent, wrong or another pair's replacement, zero/negative rate, and a manual USD→GBP `0.72000000000000000001` when the other two legs are pinned. |
| R04 — Facts, replay and collision | Save K from E; a named same-value replacement advances the head with an empty affected-rate list. Authorized K returns the first receipt. New K2 from E is stale. Changed K (e.g. `1.20` → `1.200`, changed actor or replacement) conflicts. A new operation using a retained override, decision or snapshot ref conflicts. Even with unchanged head and values, a changed current source-policy basis/edge trace needs fresh preview; no overwrite or double publication. |
| R05 — Observation fidelity | Selected F has a conflicting second row and unselected G has no accepted edge. Reopen both full facts from the decision basis, with F's source time original-only, fetch UTC0 and present-empty publication text distinct from absence. Same F identity with a changed lexeme or time presence conflicts before any durable changes. |
| R06 — Smaller successor | Temporary USD→EUR 7/5, pinned EUR→GBP 1/2, unrelated temporary JPY→CAD 3/250. Successful supplied USD→CHF `0.70`, CHF→EUR `2.0` imply USD→EUR 7/5, so only the first override expires. Use compatible fetched edges and derive USD→GBP 7/10. Repeat with direct same-value fetched USD→EUR, unrelated-only quotes, and no accepted observation. Reopen both override counts and original provenance. |
| R07 — Race and late failure | Two fresh commands share E. First commits, second is stale, and two identical K submissions yield one commit and one retained receipt. Force a reached late write/CAS failure after staging and verify from an independent committed view and reopen that the old head and every prior row survive and all staged rows are absent. |
| R08 — Uncertain and post-commit error | Lose acknowledgement after physical commit and separately fail the body after confirmed commit. Both lack a normal payload. The former is unknown and fail-closed; the latter is committed with a work error. Fresh selected read proves one exact committed closure. A partial closure or wrong intent cannot prove acceptance; full absence or known rollback still requires the ordinary authorization and freshness gates. |
| R09 — Catalog and corruption | Keep populated protected-prefix rows unchanged across additive advance and reopen. Refuse old manifest on the newer store. Missing head/decision links, a pointer to a nonterminal revision, wrong physical type, key/body mismatch, invalid ordinal, duplicate active pair, noncanonical rational, malformed known format, and unknown format must never cause fallback to E, current repair, or success. |

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
