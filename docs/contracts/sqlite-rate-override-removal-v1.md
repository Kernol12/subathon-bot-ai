# #733 Standalone rate override removal — Tier A draft r2

Protected `e451293a4bc9a6faf8b11bfaa74714f15e85d9c4`, tree `34367e45f5f1073f10607a769ec31e644f3c8d01`.
Spec v0.45 §9 / FX-06 and 164 A: fresh absence refuses without writes. Replay requires an exact committed retry.
[Brief](https://github.com/Kernol12/subathon-bot-ai/issues/733) ·
[164 A](https://github.com/Kernol12/subathon-bot-ai/issues/733#issuecomment-7083771820) ·
[#406 rulings](https://github.com/Kernol12/subathon-bot-ai/pull/724#issuecomment-7008906120) ·
[#727 retained contract](https://github.com/Kernol12/subathon-bot-ai/blob/e451293a4bc9a6faf8b11bfaa74714f15e85d9c4/docs/contracts/sqlite-rate-overrides-v1.md)

## 1. Public types and ownership

In namespace `subathon::core`, in `rate_override.hpp`:

```cpp
struct RateOverridePair { std::string low_currency, high_currency; };
struct RateOverrideRemovalProposal {
    EffectiveRateSnapshotInput candidate;
    std::vector<RatePairChange> affected_rates;
    std::string removed_override_ref;
};
[[nodiscard]] RateOverrideRemovalProposal preview_rate_override_removal(
    const EffectiveRateSnapshot& current, const RateOverridePair& pair,
    std::string_view removed_override_ref, std::span<const RateOverrideFetchedEdge> fetched_edges,
    std::string_view candidate_ref);
```

Pure refusals use `RateSnapshotFailure`: invalid pair shape → `invalid_currency`, no exact active match →
`invalid_override`, empty ref or successor equal to base → `invalid_reference`, invalid ordinal →
`unresolved_basis`. Use existing #406 snapshot validation and `resource_limit` for exceeding ceilings.

In namespace `subathon::server`, in `sqlite_rate_overrides.hpp`:

```cpp
struct RateOverrideRemovalEdit {
    std::string installation, operation, actor, decision, base, successor;
    core::RateOverridePair pair;
    std::string removed_override_ref;
};
struct RateRemovalPreview {
    RateOverrideRemovalEdit edit;
    RateSourceBasis basis;
    core::RateOverrideRemovalProposal proposal;
};
struct RateRemovalReceipt {
    std::string decision, base, successor;
    core::RateOverridePair pair;
    std::string removed_override_ref;
    std::vector<core::RatePairChange> changes;
};
struct RateRemovalReceiptSelection { RateReceiptOrigin origin; RateRemovalReceipt receipt; };
using RateRemovalPreviewResult = std::variant<RateRemovalPreview, RateStoreRefusal>;
using RateRemovalSaveResult = std::variant<RateRemovalReceiptSelection, RateStoreRefusal>;
using RateRemovalRecoveryResult =
    std::variant<RateRemovalReceiptSelection, RateStoreRefusal, RateAttemptAbsent>;
// Public const members added to SqliteRateOverrideUnit:
[[nodiscard]] RateRemovalPreviewResult preview_removal(RateAuthoritySource&,
    const PermissionGraphTrust&, const RateOverrideRemovalEdit&, const RateSourceBasis&) const;
[[nodiscard]] RateRemovalSaveResult save_removal(RateAuthoritySource&,
    const PermissionGraphTrust&, const RateOverrideRemovalEdit&, const RateRemovalPreview&,
    const RateSourceBasis& current, const std::function<std::optional<std::int64_t>()>& sample) const;
[[nodiscard]] RateRemovalRecoveryResult recover_removal(RateAuthoritySource&,
    const PermissionGraphTrust&, const RateOverrideRemovalEdit&, const RateRemovalPreview&) const;
// Namespace-level completion gate:
[[nodiscard]] std::optional<RateRemovalReceiptSelection> finish_rate_removal_receipt(
    std::optional<RateRemovalReceiptSelection>, const SqliteWorkResult&);
```

#733 owns the new API, removal branches and frozen boundary contract proposed at
`docs/contracts/sqlite-rate-override-removal-v1.md`. It amends these shared #406/#727 paths:

| Shared paths | Amendment |
| --- | --- |
| `core/include/subathon/core/rate_override.hpp`, `core/src/rate_override.cpp` | Add the pure entry point, reusing selection and preview rules. Preserve add/replace and update semantics. |
| `server/include/subathon/server/sqlite_rate_overrides.hpp`, `server/src/sqlite_rate_overrides.cpp`, `server/src/sqlite_rate_override_codec.hpp` | Add removal types, work methods, completion and paired codecs. Extend shared decision dispatch and closure checks. An authorized legacy add/replace encountering a valid removal under its operation key now gets `identity_conflict` after retained verification. Legacy formats and same-kind results hold. |
| `docs/contracts/sqlite-rate-overrides-v1.md` | At publication link the #733 contract from scope and qualify Decision/Occurrence and key conflict clauses for removal. Keep the #727 grammar, catalog and introducing-edit contract. |
| `tests/acceptance/rate_override_tests.cpp`, `tests/qt/sqlite_rate_override_writer_tests.cpp`, `tests/qt/support/sqlite_rate_override_writer_fixture.hpp` | Later test-stage amendments own the removal cases and necessary fixture additions. Preserve shipped controls. |

## 2. Selection, authority and intent

Remove one temporary or pinned override by exact active ref and unordered pair. Canonicalize direction
by sorting the distinct 1–64 byte currency tokens by unsigned bytes, preserving spelling. The typed pair
requires that order. Scope is lowercase hex of `B(low) B(high)` with U32 big-endian byte lengths.
EUR/USD is `0000000345555200000003555344`. An old occurrence or equal rate cannot select a different revision.

Use #727's ordinary `rates.override.edit.v1` on `permission-rates-v1`: one pair
resource, no quantities or helper. For preview, save, retry and recovery resolve
current installation, enabled account, session and coherent graph in scope.
The submitted `edit.actor` must equal the authenticated account. Mismatch is
`denied` before any protected rate lookup, including lookup of a used key.
Validate USE and GRANT channels and typed pair scopes. Allows enumerate pairs;
ordinary Any denies are admissible. A matching individual tier controls scope
without role fallback. Outside it role deny wins over role allow then default
deny. Require one whole USE witness; owner or GRANT alone cannot permit. Refuse
unknown/incompatible authority and contradictory overlapping individual entries.

Authorized preview selects the verified committed head, requires exact base and retained occurrence
match, and returns an owned edit, basis and proposal for equality checks. A spent base gives
`stale`. No matching target gives `invalid_proposal` with zero writes.

Apply surviving manuals first with identical bodies, then compatible non-self
edges in supplied trace order. Earlier compatible choices win conflicts. Use
only indexed edges, including formerly rejected ones. Keep complete selected
observation metadata and original quote ordinals and retain all basis docs
and trace for audit. Canonical RSV1 sorting (fetched constraints before
manuals) cannot select winners. The successor has the old head as parent and
absent construction time. No new manual, accepted update or incidental expiry.

Over the union of old and new members, emit distinct directed changes in unsigned from/to order only
when exact reduced `weight(to)/weight(from)` or availability changes. Missing has absent value and
provenance. Identity is one with absent provenance and omitted from deltas. Available quotes bind
snapshot/from/to with auditable snapshot support, even at equal rates.

In the authorized save unit after structural admission:

1. Look up the installation-scoped key. If present, verify the committed
   closure, then compare kind and full bound intent, including preview.
   A coherent difference gives `identity_conflict`. Exact match returns the
   original result after head or live policy changes, including after removal
   of the target, with no resampling, audit rewrite or head publication.
   Denial discloses neither existence nor receipt. Invalid/unsupported/
   unavailable retained data is a storage refusal, never absence.
2. For an absent key, verify head, reject stale base, reject unmatched
   active target under 164 A, then reject successor or decision collisions.
3. Require `preview.edit.actor == edit.actor ==` authenticated account.
   If the earlier submitted-actor check passed but the fresh preview's actor
   differs, give `preview_changed`. Recheck the full preview against current
   policy identity, ordered observations and indexed edges and independently
   recompute the canonical candidate, target and ordered changes. Any
   difference requires a fresh preview even at equal rates. Sample once on
   the new staging path, with unknown time absent and known time only in audit.

## 3. Durable decision and verified read

Reuse the #727 schema and RIN1/RDC1 unchanged for introducing edits. Use
paired formats for removal in the existing columns. #727 B, O, Basis, Witness,
required refs and per-field rules apply, with pair tokens at most 64 bytes
and target and actor/account refs at most 256 bytes.

```text
subathon.internal.rate-override-removal-intent.v1
RRI1 B(installation) B(operation_key) B(decision) B(base) B(successor)
     B(low_currency) B(high_currency) B(removed_override) B(actor) Basis
subathon.internal.rate-override-removal-decision.v1
RRD1 B(installation) B(operation_key) B(decision) B(base) B(successor)
     B(low_currency) B(high_currency) B(removed_override) B(account)
     B(session) B(session_record_revision) B(session_snapshot)
     B(permission_catalog_revision) U64(permission_graph_revision)
     Witness O(I64(accepted_at_utc_ms))
```

Encoded caps include four magic bytes. The common maximum is
`4 + (4 + 4096) + 6 × (4 + 256) + 2 × (4 + 64) = 5,800` bytes.
RRI1 adds `260 + (4 + 16 × 260) + (4 + 16,384 × 12)` = **206,836 bytes total**.
RRD1 adds `3 × 4,100 + 260 + 8 + (1 + 3 × 4,100) + (1 + 8)` for session
refs, catalog, graph revision, role witness and present UTC time =
**30,678 bytes total**. Both fit the existing 229,376/32,768 column caps.
Preflight known envelope sizes and declared counts before materializing or
allocating. Over-cap sizes are `resource_limit`; malformed within-cap retained
type/framing/link or mixed known formats is `invalid_retained_data`. Unknown
well-bounded format gives `unsupported_retained_data`. Keep all #406/#727
numeric, logical, encoded, source trace, selected SQL and writer preflights.

Intent includes RRI1, canonical successor RSV1 and complete ROB1 in Basis order.
Recompute ordered changes. RRD1 shares exact intent and row identity, requires
byte-exact `RRI1.actor == RRD1.account` and records the initial current witness.
Retained mismatch is `invalid_retained_data`. Match exact format IDs and magic.

Reuse identical ROB1 or stage already-admitted new evidence. Same ref with different bytes gives
`identity_conflict`. Append one successor and one removal decision and CAS H from the exact base,
requiring one changed row. All other prior rows stay byte-identical, including the target occurrence
and its original introduction links. No new occurrence, catalog step, provider sync or historical valuation.

Shared head, revision, key and recovery readers dispatch the removal closure.
Verify base/successor and parent, exact target and occurrence body in base
only, byte-identical survivors, and zero occurrences attributed to removal.
Select distinct base/successor occurrences, introducing decision links and
source evidence within #727 limits. Recompute from retained basis and require
canonical successor equality. Historical verification uses neither the live
head nor current source policy and does not expand unrelated history. Existing
add/replace can use a verified removal head and replay its own old receipts.

C733-05 remains a limit of the shipped decision-less update reader. If the
removal decision is missing but the successor remains, head/revision can
accept its subset shape under that branch. Such a state proves neither a
removal receipt nor verified absence. This contract adds no rule to that branch.

Only acknowledged physical COMMIT and normal completion release the provisional
receipt and publish once. Late SQL/CAS failure rolls back the whole unit with
its writer outcome. Confirmed COMMIT followed by work error has no normal
payload. Unknown COMMIT fails admission closed. No automatic repetition.
Authorized coherent recovery after reopen accepts only a full matching committed
closure, with no republish. Verified absence requires missing operation and
proposed successor/decision, no attributed occurrence or head reference, and
coherent historical base, preview and observations (exact already-retained
bytes count as reuse). Partial/conflicting/unavailable is unresolved. Known
rollback or verified absence permits a new authorized attempt under the usual
freshness and intent rules.
[Writer completion rules](https://github.com/Kernol12/subathon-bot-ai/blob/e451293a4bc9a6faf8b11bfaa74714f15e85d9c4/docs/contracts/writer-transactions-v1.md)

## 4. Shipped starts and independent examples

Each start uses #727 `RateFile`, explicit catalog setup and committed E. Use `literal_rate_trust`, fixture
`Authority` and named whole grants at the existing typed authority port. For each add/replace use shipped
`SqliteRateOverrideUnit::preview/save` with direct `RateSourceBasis` parameters and normal writer
completion. Verify `head/revision` and capture independent committed `rate_image` before removal,
without SQL seeding. Session resolution and graph loading own production facts; use fixture facts here.
[#727 writer fixture](https://github.com/Kernol12/subathon-bot-ai/blob/e451293a4bc9a6faf8b11bfaa74714f15e85d9c4/tests/qt/support/sqlite_rate_override_writer_fixture.hpp)

Session/graph/trust feed admission and the first USE witness feeds audit. Pair, target and base feed
selection. Ordered basis feeds validation, freshness, the solver and ROB1, including unused facts.
The snapshot factory, RSV1 and quotes use selected constraints, full selected docs and weights. Decision
verification consumes both revisions, occurrences and retained basis. Retry uses no current policy or clock.
The writer consumes candidate, decision and expected head. Completion consumes the physical outcome.
The independent reader and reopen check durability. Fixtures need no provider, reporting currency or history.

### A. Pinned removal restores a fetched constraint; edge order decides

Start E. Under actor A's EUR/USD permit, save pin O EUR→USD `1.20` (decision D)
to S. Pass `RateSourceBasis` B directly with observations in order F, W, G.
F is the shipped `selected_f()` with #1 EUR→GBP `0.90` and #2 EUR→USD `1.30`;
W has USD→GBP `0.70` at #1 (opaque context refs `p/c/q/e`, times absent);
G is shipped `unused_g` CAD→JPY `2.0` with no indexed edge. Trace is
F#1, F#2, W#1. Then under a CAD/JPY permit use a second shipped save to add
pin Q CAD→JPY `3.00` to T with the same basis. Both commits precede removal.
T has weights EUR=1, GBP=9/10, USD=6/5 and CAD=1, JPY=3. O wins over F#2;
W disagrees with USD→GBP `(9/10)/(6/5) = 3/4`. T embeds F, retains all three
ROB1 documents, and its constraints are F#1, O and Q.

Remove O from T, giving U. F#1 and F#2 now select USD weight 13/10.
W loses because `(9/10)/(13/10) = 9/13 ≠ 7/10`. U has F#1, F#2, Q, and
identical F and Q bodies. G remains unselected despite its CAD/JPY quote.
Expected changes in order:

| Pair | T | U |
| --- | --- | --- |
| EUR→USD | 6/5 | 13/10 |
| GBP→USD | 4/3 | 13/9 |
| USD→EUR | 5/6 | 10/13 |
| USD→GBP | 3/4 | 9/13 |

EUR↔GBP is 9/10 and 10/9; CAD↔JPY is 3 and 1/3 throughout. Across components
rates stay missing. For changed quotes, provenance moves from T to U with
the same query endpoints and U's fetched constraints replacing O's support.
F's UTC0 fetch, original-only source time and present-empty publication text
remain exact. W and G remain present as whole standalone ROB1 facts.

Repeat from E with F#1, W#1, F#2 for both setup saves and removal. T has the
same rates and selected constraints. U selects F#1 and W#1: EUR→USD is
`(9/10)/(7/10) = 9/7`. F#2 now loses against that path. The U column becomes
9/7, 10/7, 7/9, 7/10 in the same order. U embeds complete F and W, including
F's rejected #2. Canonical sorting of U cannot recover the policy trace.

### B. Temporary removal disconnects a pair and its cross rates

Start E with empty basis B. Save temporary O USD→EUR `1.25` to S, then save
pin Q EUR→GBP `0.50` to T under distinct EUR/USD and EUR/GBP grants.
T weights are EUR=1, GBP=1/2, USD=4/5. Remove O from T with the canonical
EUR/USD scope and empty basis. U preserves the EUR↔GBP 1/2 and 2/1 and Q's
complete provenance. Removed changes in order are EUR→USD 4/5, GBP→USD 8/5,
USD→EUR 5/4, USD→GBP 5/8, all to missing with absent value and provenance.
USD→USD remains identity 1. This start needs neither a sync witness nor expiry.

### C. Same rates, different evidence; exactly one durable transition

Start E and save O EUR→USD `1.20` to S using only F#2 with the F body from
`selected_f("1.20")`. F's unused #1 EUR→GBP stays in the full document.
Both O and F#2 constrain S. Remove O to U. EUR↔USD stays 6/5 and 5/6,
EUR↔GBP stays missing, and the delta list is empty. U's support is F#2
alone and quotes reference U. Changes in provenance and override membership
require the same single removal transition as A and B.

Each successful A–C changes the committed image once: one successor with one fewer override, one decision
and one H update. Every preexisting revision, decision, occurrence and observation row is byte-identical.
These reuse starts add zero observations and occurrences. Independently verify the row diff, both revision contents
and receipt. Reopen and verify active U, historical S or T with original snapshot provenance and constraints, and the exact receipt.

### D. Save, failure and retry controls

Recreate A or C for each control and compare independent images, including after reopen where specified.

| Control | Stimulus and required result |
| --- | --- |
| Authority | Matching individual deny over role allow at save after preview. Repeat with the individual scope moved outside the pair so a role deny beats a role allow; a whole unsuppressed role allow is the positive counterpart. Revoke session or remove USE after commit before retry; owner and GRANT-only cases also give no permit. A submitted actor unequal to the resolved account is denied before protected lookup. An authorized submitted actor with a different preview actor on a fresh current base receives `preview_changed`. None of these refusals sample or write. |
| Retained actor link (C733-04) | Test-stage corruption control from committed C: change only RRD1 account to a different valid ref; an authorized exact retry refuses `invalid_retained_data`, with no repair, receipt or rate writes. |
| Fresh target | From v17's empty E, or from U produced by C under a fresh key and current base, target O: refuse without writes. In A target Q with the EUR/USD pair, or an unknown ref. Replace O via a shipped same-value edit and request old O against the new head: retained O cannot match. |
| Stale head/facts | In A save a disjoint shipped CHF/SEK pin after preview, then submit the spent T preview; refuse stale even if the quoted removal effects are unchanged. Separately change the current basis after preview by swapping F#2 and W#1 or by changing only G's opaque time metadata. In C change only policy identity B→B2 with identical facts and rates. Each source-only variation returns `preview_changed`. The spent-base and source-only controls all leave the independent rate image without removal rows. |
| Exact replay and conflict | Commit A or C once. Retry with refreshed authorized session; return the original receipt and audit without new samples or writes. Advance U through a shipped add in a disjoint pair and retry again; the later head stays active. Vary target, authorized pair, actor (authenticate another permitted account and submit that actor), base, decision or bound source/preview with the same key; coherent changed intent conflicts. Reuse a key across removal and legacy add/replace in either direction; it conflicts after authority and retained verification. |
| Observation admission (C733-06) | Start A twice. Change only the body of retained G (e.g. set fetch time to UTC0) in both fresh preview and current basis: `identity_conflict`, no sampling or writes. Separately append a new observation X, copied from G with ref X and no indexed edge, to both bases: same rate oracle and exactly one additional ROB1 row with X's full body, no new occurrence. Reopen and verify exact replay with X retained. |
| Competing work | Queue identical authorized removal submissions from the same base: one acknowledged commit and one retained result. Repeat with distinct keys and distinct successor/decision identities: first commits, second is stale. Exactly one committed image delta and one acceptance sample. |
| Known rollback | Reach the H write after staging successor and decision, then force late SQL failure or zero-row CAS using #727's existing fault pattern. No normal receipt. Preserve the SQL versus work-failure outcome, verify the unchanged pre-attempt image through reopen, and verify the attempted removal is absent. An explicit subsequent authorized attempt with the same still-current base can commit once. |
| Unknown / post-commit failure | Apply the #727 outcome fault after physical commit, and separately throw after confirmed COMMIT. No normal payload. On authorized reopen verify the single exact committed closure; no new publication or write. A partial or corrupted closure cannot prove replay or verified absence. |

## 5. Independent verification handoff

Verify C733-01–04 (typed signatures, shared paths/placement, capped frames and
actor precedence) and C733-05–07 (recorded reader limit, observation controls
and direct basis parameter). Verify against the protected tree above, not the
older local main. This round is source-traced; fixture execution remains for the
test stage. Independent Codex verification and fresh Claude closing precede freeze.
