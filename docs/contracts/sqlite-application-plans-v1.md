# Captured application-plan SQL persistence — revision 1 proposal

STORAGETEST-015 / STORAGE-015. Authority: specification v0.45 §§3.2–3.5/6/7,
NUM-01/DB-01, [slice brief](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-009/STORAGE-015.md),
[owned core plan contract](application-plans-v1.md),
[event/state boundary](event-state-boundaries-v1.md),
[writer work](writer-transactions-v1.md),
[retention](retained-context-v1.md), [receipt SQL](sqlite-receipts-v1.md) and
[contribution SQL](sqlite-contributions-v1.md).

This is an actual logical contract proposal for separate review, not C++/SQL
implementation, a finalized API, authored tests, a wire format or provider proof.
Root owns exact additive migration4/schema/order/content identity after this
proposal; immutable migrations1/2/3 and all completed assertions remain unchanged.

## Existing core model and bounded responsibility

Persist existing `core::ApplicationPlan`, `CapturedPlanRevision`, `PlanCapture`
and their complete owned members. The original22 core cases remain pure
transformation evidence, not Qt/SQL durability. Storage neither invokes rule or
modifier sources nor recalculates requests, parses rates, rounds amounts,
checks live permissions, resolves current capacity or applies effects/jobs.

Core capture creates revision1 without parent/override. Every later revision is
previous+1, has that immediate parent and an authorized complete override decision.
Every revision retains the SAME complete PlanEvent: event/root/parent refs,
ReceiptId, full original identity/facts/arrival and optional ContributionId.
Replacement contribution state, valuation, requested effects and captured source
selections may differ; old revisions never change. These invariants are retained,
not relaxed to make a tiny high-number history appear valid.

Already resolved rule/modifier selections and requests retain their exact supplied
order, including duplicates and excluded rules. Accepting review does not resolve
missing evidence/valuation; a resolved valuation does not authorize application.
Preview freshness remains a separate core operation and cannot authorize commit.
No authoritative state token, cap, effect or job is added to a captured plan.

## Session ownership and proposed primitive roles

Propose an inert value store over an already active existing SqliteWorkSession,
like retained-context storage. New server ports do not yet exist; concrete names,
result structures, signatures and exact error priority require the next actual
API review. There is no need to invent an owning transaction facade merely for
storage of values returned by the existing pure core functions.

| Logical primitive | Required behavior |
| --- | --- |
| Retain initial plan reference and revision | Only revision1/no parent/no override; stage complete immutable event/header/revision/ordered children, or exact duplicate/conflict |
| Find complete plan | Owned complete ApplicationPlan, or absent; validate full selected history and materialize it wholly within configured read bounds, never truncate |
| Find one revision | Owned optional CapturedPlanRevision for a positive typed revision; selected snapshot/local links only, explicitly not certification of a complete ApplicationPlan |
| Append with expected head | Atomically stage exactly one parented next revision and advance head if expected matches actual validated head; missing/stale are explicit no-own-write results |

The caller performs the real BEGIN and COMMIT and owns cleanup/publication. Store
construction/destruction does no SQL/commit/rollback or cleanup transfer. The caller
can compose earlier valid staged work and several plan primitives in the same
unit. Return values are STAGED/speculative until actual physical commit and normal
work completion. Abandonment and uncaught failures use existing owner finalization;
there is no new destructor promise, nested BEGIN, enqueue, private connection,
raw handle, event-loop wait, automatic retry or deferred commit vote.

An exact repeated initial retain is already-present, not another row. Same plan
reference with different initial data is conflict. Compare the complete original
revision and ordered children, not a hash alone; a referenced later head does not
make an identical initial retain overwrite history. A duplicate append with an old
expected head is stale even if its candidate bytes equal an already stored row.
A missing plan or stale expected head makes no primitive writes and does not undo
earlier caller work. The caller's enclosing atomic policy determines whether to
commit other work, return a diagnostic or roll back that unit.

New initial and append operations must finish a COMPLETE revision before returning
success: header, scalar snapshot, all three ordered child collections and head.
Allocate/prepare bookkeeping and bindings safely; any failure after partial plan
writes establishes actual rollback before a known storage error or propagates
actual uncertainty. A caught allocation exception must not strand a partial new
revision that the caller could then commit. No partial row/head success is exposed.

## Complete field retention

All fields below are actual current core members, not proposed evaluator output.
`std::string` fields use exact-length BLOB with no QString/Unicode conversion,
trimming, case folding, NUL termination or decimal normalization. Present empty
BLOB differs from SQLNULL; optional absence stays absent. Unknown provenance is
never replaced by zero, empty text, a current source or an invented default.

| Owned value | Fields retained exactly |
| --- | --- |
| Plan identity | Nonempty opaque plan_ref; no numeric surrogate or implicit plan-per-receipt uniqueness |
| PlanEvent | Nonempty event_ref/root_event_ref; optional parent_event_ref; nonzero ReceiptId; complete ReceiptIdentity, ReceiptFacts and ReceiptArrival; optional full-range ContributionId including present0 |
| ContributionState | Payment evidence and every independent ContributionConditions field: valuation monetary/original money/resolution ref/missing-rate; review; evidence policy/policy ref/optional decision choice+decision ref+actor ref; application/reversal/settlement |
| PlanValuation | Known state not_applicable0/missing_historical1/resolved2 and optional complete HistoricalValuation group |
| HistoricalValuation | resolution_ref; original_money original_decimal+currency; comparison_currency; effective_rate_decimal; optional fetched_rate_decimal; snapshot_ref; optional source_ref; optional signed64 rate_timestamp_utc_ms/fetch_timestamp_utc_ms; optional manual_decision_ref |
| RuleSelection, ordered | rule_ref, version_ref, selected bool, explanation |
| ModifierSelection, ordered | definition_version_ref, instance_revision_ref, target_ref, group_ref, group_operator, target_operator, original_factor_decimal |
| RequestedEffect, ordered | action_ref/action_version_ref/rule_ref/target_ref; unit; requires_historical_valuation bool; exact_requested_units rational; signed64 requested_units; calculation_explanation; reversal_behavior_ref |
| CapturedPlanRevision | Nonzero full uint64 revision; optional nonzero parent_revision; complete capture/rules/modifiers; optional complete PlanOverrideDecision |
| PlanOverrideDecision | Authority unknown0/denied1/authorized2; actor_ref/reason/decision_ref; persisted later revision requires authorized and all three nonempty |

ContributionState remains a historical snapshot, not a lookup of the current
contribution row. Confirmation/review changes in STORAGE-014 cannot rewrite any
captured state or quote. Initial event provenance is immutable even when an
override supplies new valuation/requests/selections. Ordered duplicates remain
separate child rows; no sorting/map/content uniqueness substitutes for position.

All enum/boolean SQL values require actual INTEGER storage class and exact known
vocabulary. RequestedUnit mapping is milliseconds0/micro_points1/whole_count2.
Contribution dimension mappings remain identical to contribution SQL revision1.
SQL integers store signed64 requested units and timestamp extrema exactly, and
uint32 receipt schema values without narrowing. No ID/revision/money/rate/rational
component is bound via REAL or a signed conversion of an unsigned identifier.

Core validate_capture rules remain: resolved valuation requires a complete quote,
actual event money and byte-exact original money equality; other valuation states
have no quote. Missing_historical forbids requests marked valuation-dependent,
while independent requests marked false remain retainable. This does not infer
eligibility or monetary policy from other independent state dimensions.

The core does NOT validate numeric correctness, rate/factor syntax, rule outputs
or the meaning of semantic refs. Storage preserves those trusted supplied values
and does not introduce semantic nonempty constraints on quote/rule/modifier/action/
policy strings that core never required. In particular a present empty optional
source/fetched/manual ref is not absence. Override audit strings, plan/event/root
and required actual receipt/retained identities have their already agreed nonempty
rules. Parent-event presence and its string bytes remain independent from absence;
this slice has no finalized ancestry namespace/cycle evaluator to invent more.

## Historical valuation and exact rational storage

This slice supplies the actual rate-field storage explicitly deferred from013/014.
Preserve effective and fetched spellings independently, including source precision,
comparison currency, snapshot, known source, source/fetch timestamps and manual
provenance. `1.25000` is not rewritten to `1.25`; `1.249900` fetched may differ from
effective `1.25000`; absent fetched/source/time/manual values stay SQLNULL. A later
manual resolution appends a new revision rather than filling old missing fields.
Neither timestamp substitutes for event source/arrival UTC. No current FX fetch,
provider/quote validation, or permission claim follows from retained strings.

`Exact` is existing Boost.Multiprecision cpp_rational VALUE, not a source rational
spelling. Persist its mathematical numerator and denominator as two required
canonical ASCII integer BLOBs: signed numerator with optional leading `-`, positive
denominator, no leading plus/zeros/whitespace/exponent/locale formatting. Numerator
zero is exactly `0` and requires denominator `1`; otherwise both are reduced with
gcd(abs(numerator),denominator)=1 and denominator>0. Thus the value produced by
Exact(2)/4 stores `1` and `2`; there is no lost original rational expression because
the core type never retained one. Original decimal spellings remain separate
HistoricalValuation/Modifier/Receipt string fields and are NEVER canonicalized.

On reading, strict BLOB/class/byte bounds and canonical grammar precede exact
integer construction. Reject zero/negative denominator, negative zero, nonreduced
pair or malformed component as stored corruption rather than silently fixing it.
Reconstruct by exact integer arithmetic only. Never pass through floating point,
qint64, parse_decimal, round_effect_units or decimal approximation. Signed64
requested_units is retained separately; storage does not recompute it or certify
that it is the correct half-even result of exact_requested_units.

The existing parser's512-byte/256-digit rules and finalizer's4096-bit bound belong
to those core operations, not this storage format. Proposed independent positive
component_bytes default16384 bounds EACH stored canonical component and is
configurable. Values above signed64 and above4096 bits must round-trip if within
this bound; arbitrary precision does not promise infinite resources or override
actual writer budgets. Input Exact may already own a large value; materialization
and conversion checks must avoid overflow and fail explicitly, not truncate.

## Actual source and retained-reference resolution

Before initial retention compare the supplied PlanEvent identity/facts/arrival
against the actual immutable receipt original by ReceiptId, not latest attempt.
Require actual typed retained configuration/context/known-config targets with the
same configured reference/document bounds. A missing actual receipt is explicit;
a different original projection is mismatch; corrupt/dangling selected source is
stored-data failure. Current SELECT proves existence/bytes, not prior commitment,
normalization/conflict-free status or caller classification. Those remain honest
upstream preconditions; fixtures claiming them arrange a prior actual commit.

Persist one immutable event/header per plan. Its receipt reference can hydrate
identity/facts/arrival from that immutable original instead of duplicating growing
history. Every reconstructed revision receives that same exact event. Candidate
append compares EVERY event member with the stored header/original, including
optional ancestry/contribution presence; a changed event is an explicit event-
conflict refusal after stored comparison, with no own writes. It is not labelled
a pre-SQL failure or a new revision of historical source facts.

A present ContributionId must resolve in the contribution namespace with exact
BLOB8 including0; absence remains SQLNULL. It may refer to a contribution linked
through a confirmation or another legitimate event relationship. Do NOT require
contribution.original_receipt_id == PlanEvent.receipt_id, require a matching support
kind, force current state to equal captured state, or infer a provider correlation.
The agreed relationship beyond actual typed existence is supplied upstream.
Selected malformed/dangling actual target fails; a same-byte receipt/config key
cannot satisfy a missing contribution. No new public lookup helper is invented
only to expose implementation; actual private read layering remains API review.

Event parent/root refs and rule/action/version/target/modifier/group/operator/
quote/snapshot/resolution/actor/decision/reversal refs have retained semantic meaning
under their owning caller/source contracts, but no finalized physical namespaces
or resolver format here. Do not relabel them as configurations or create fake
rows merely to satisfy FKs. Actual arrival refs and ReceiptId/ContributionId have
physical SQL resolution; those other versioned semantic-resolution/permission/
evaluator gates remain explicit future product obligations, including API-008.
The caller/source must already retain and resolve their meanings; nonempty bytes
are not proof. This bounded storage proposal does not claim those gates complete
or remove the full specification's durable-resolvability requirements.

## Revision domain, targeted access and honest MAX evidence

The pure uint64 byte codec covers0..UINT64_MAX. Persisted/head/parent revision
values cover1..UINT64_MAX; initial is1 and NULL alone means no parent. ReceiptId
remains1..MAX and optional ContributionId remains0..MAX. All actual unsigned SQL
fields require typeof BLOB/length8 with exact big-endian bytes, including values
above signed64; never TEXT/INTEGER/REAL even when conversion appears equivalent.

| Numeric value | Exact codec BLOB hex |
| --- | --- |
| 0, codec only for revision | 0000000000000000 |
| 1 | 0000000000000001 |
| 9007199254740993 | 0020000000000001 |
| 9223372036854775807 | 7fffffffffffffff |
| 9223372036854775808 | 8000000000000000 |
| 18446744073709551615 | ffffffffffffffff |

Full history must be nonempty, start1 and be contiguous with immediate parents,
authorized audited later rows, immutable stable event and valid capture shapes.
Head metadata must equal the greatest canonical actual revision. Row count equals
that numeric head under this no-deletion revision. Check types/cardinality before
converting counts; count overflow/engine limits are explicit failures. No seed-
from-high-revision/import/reset/repair operation is introduced.

A full find returns a complete ApplicationPlan only after those conditions and
all selected bodies/ordered collections pass. Before materialization, inconsistent
count/min/max/type/head metadata is corruption; a valid complete history over the
configured full-read bound is result_limit, never a truncated partial plan.

Targeted revision lookup returns only ONE existing CapturedPlanRevision, not an
ApplicationPlan and not a claim that all intervening history is valid. Validate
selected row/body/children/source targets and its local role: revision1 has no
parent/decision; a later revision has parent=revision-1, an existing same-plan
parent target and authorized complete decision. Validate typed header/head/max
markers and an actual structurally valid initial anchor (revision1/no parent/
override and its core capture-valuation shape/source targets). Unselected initial
child payloads are not certified by that anchor check. Do not recursively traverse/materialize
all ancestors or pretend that this certifies full continuity. Concrete return-type
wording must make this scope clear; this snapshot cannot be passed off as a valid
one-element ApplicationPlan or used as permission for append/application.

This permits a meaningful typed-MAX diagnostic without falsifying core semantics:
create a deliberately sparse disposable SQL fixture with header/headMAX and actual
rows1,MAX-1,MAX, with complete selected MAX body/local parent/physical source refs.
Targeted lookup must return actual numeric MAX and parentMAX-1 exactly, with raw
BLOB8/type/hex checks. That fixture is NOT a valid complete core plan. Full find
and append MUST reject its count/head gap as invalid stored history; never call
it valid exhaustion or append success. Similar signed-boundary selected rows prove
the actual typed field, in addition to independent pure codec vectors.

A logically valid headMAX cannot append without overflow, wrap, gap search or
reuse. Reachable ordinary tests prove valid1→2→3, stale/duplicate/noncontiguous
refusals and typed high values separately. They must state that constructing an
entire1..MAX history is physically infeasible; no tiny fixture or mocked count
establishes successful real-chain exhaustion. A selected MAX predicate/diagnostic
is numeric storage evidence only. Actual configured/database/row-count resource
limits may prevent reaching such a history long before numerical MAX.

## Append validation without a read-materialization quota

The full-plan read cap is NOT a product limit on retained revisions or append.
Append accepts one bounded owned candidate and compares its parent/number with
actual current head under the same active writer transaction. A correct next
revision is head+1 only after the checked no-overflow condition. Same-session
checking/staging is atomic under the authoritative writer; other commands cannot
interleave a head replacement. No RETURNING-specific driver behavior is required.
Advance head only with all new child rows complete. Earlier rows remain immutable.

Append validates old STRUCTURAL history independently from full-plan loading:
strict ID types/cardinality/min/max/count/head and bounded-page traversal of the
revision metadata checks every immediate parent/audit and core capture-valuation
shape, including missing-historical versus any valuation-dependent request flags.
Resolved old quotes require comparison of actual original-money bytes with the
immutable event money, complete quote presence/types and valid dependency flags;
a valuation enum alone is not that check. The shared immutable event/header is
validated once. This audit inspects the required scalar revision/capture metadata
in bounded pages but does not materialize all historical
rule/modifier/request payloads as ApplicationPlan vectors or apply the full-read
cap. Every query/result still consumes the existing cumulative session budgets;
resource/result exhaustion is an honest known failure, not a stale conflict or
permission to reset budgets, skip metadata or silently stop checking.

This is explicitly NOT a global audit of unselected historical payload fields:
old rational numerators/denominators, rule/modifier strings and collection payloads
are strictly validated when selected/materialized, not certified by a metadata-only
append audit. Core validate_history checks event/chain/decision/capture valuation,
not evaluator/numeric correctness of those payloads. Immutable prior writes and
trusted SQL boundaries remain prerequisites. Do not claim a validator examined
unselected bodies or repairs arbitrary external corruption. Full find validates
all bodies it returns; targeted find validates its selected body/local links.

## Proposed logical rows and configurable bounds

Propose five logical main-schema tables: immutable plan/event header with head;
immutable revisions with complete state/valuation/override scalar groups; ordered
rule selections; ordered modifier selections; ordered requested effects including
canonical exact components. Root chooses exact physical layout/constraints after
actual review. Revision key is(plan_ref,revision); each child uses that key and a
positive signed64 ordinal1..N. Ordinal is collection position, not a new domain ID.
All-empty collections are legitimate. Selected count/order gaps, duplicates or
wrong ordinal types fail; exact duplicate content at different ordinals remains.

Actual schema needs relational uniqueness and typed FKs for receipt/contribution,
revision membership and same-plan parent/children where feasible. Header/head-to-
actual-max agreement, complete-unit creation and unsigned immediate-parent rules
remain explicit runtime invariants; avoid inventing cyclic/deferred FK behavior
without an actual root proposal. No migration4 source, DDL or manifest is authored
in this initial document. Earlier migration identity/payload/projections stay fixed.

Proposed trusted defaults: reference_bytes4096, value_bytes262144,
component_bytes16384, revisions_per_full_find1024, items_per_vector1024 and
items_per_full_find4096 total across all materialized children. Each complete
initial/new revision must fit the per-vector/input byte bounds; the full-find
limits apply only to materialization, not appends or total retained history.
These are configurable engineering bounds, not scope reductions or product quotas.
References include all opaque identity/version/source/action/audit/operator refs;
value bytes include event actor/event/money strings, explanations/reasons and
original/effective/fetched/factor decimal spellings and retained documents.
Canonical exact integer components have their own bound. Required actual source
identity/arrival refs retain receipt13's reference/value roles. The full concrete
field-to-bound mapping/overflow-safe count+1 representability/error priority is an
actual API precision gate before tests, not an implementation guess.

## Failure truth and future acceptance examples

Pure input/option/enum/group/bound errors precede SQL/capability access and make no
writes/cleanup if caught, following existing storage conventions. The new primitive
preflight is distinct from the unchanged core transformation's exception ordering;
no persistence path calls evaluators/sources merely to validate an argument.
After pure shape checks, validate existing selected header/structural history before
classifying duplicate/stale/missing. A healthy stale/conflict result precedes
resolution of any newly proposed physical targets; it cannot mask corrupt stored
metadata. Exact concrete categories/precedence await separate actual API review.

Missing actual source, mismatched original, malformed selected row/group/type/
canonical component/history, dangling physical target or selected valid data over
bounds is an explicit failure. Do not fabricate a partial/default record. Known
storage exceptions require confirmed actual rollback; actual capability/protocol/
invalid-statement errors remain original WorkFailure, and unknown commit/cleanup
outranks every local cause/fail-closes. No auto-replay, fake known rollback from an
inactive cleanup no-op, repair or outcome reconciliation. Domain no-own-write
refusals do not imply previous caller writes were undone.

These are future specification-derived oracles, NOT authored/approved tests:

| ID | Required independent observations |
| --- | --- |
| SP01 | Literal six pure uint64 vectors; actual selected positive revision fields above2^53/signed64/MAX, raw BLOB8/hex/class; zero only codec, malformed/type-equivalent TEXT/7/9-byte values refused |
| SP02 | Initial1/no parent/no audit, valid1→2→3 full history/stable complete event; immutable old snapshots after input/source/current-state changes and reopen |
| SP03 | Exact duplicate initial versus different conflict; stale/duplicate append with no own writes and no evaluator invocation; caller can commit prior unrelated work after a caught domain refusal |
| SP04 | Two revisions racing expected1 under queue/real writer produce one2 and one stale, no lost history or partial head; caller commit/publication remains real |
| SP05 | Full history read cap exact/one-over; append past that read cap still works with adequate cumulative writer budgets, while full find refuses rather than truncates |
| SP06 | Explicit sparse MAX diagnostic targeted read only; full find/append reject count/head gap, no forged valid history/exhaustion claim; no wrap/reuse |
| SP07 | Canonical huge coprime Exact numerator/denominator, negative/zero fractions and signed64 requested extrema round-trip; >4096bit within component bound retained; 2/4 construction stores1/2, decimals remain original spellings |
| SP08 | Wrong component class/grammar/nonreduced/zero-negative denominator or bound+1 fail exact category/known rollback; expected values independent of production codec |
| SP09 | Complete HistoricalValuation effective/fetched difference, snapshot/currency/source/time/manual optional-null/present-empty and signed timestamp extrema; later resolution appends without filling original missing quote |
| SP10 | All independent contribution states and valuation shapes, explicit missing-historical independent requests; inconsistent quote/original money/dependent requests rejected, no inferred evidence/application permission |
| SP11 | Ordered duplicate rules/exclusions/modifiers/request actions and every provenance/operator/factor/explanation field; exact collection bounds/counts/ordinals survive reopen/owned-result lifetimes |
| SP12 | Actual original receipt projection and typed cfg/context/knowncfg/optionalCID0 targets; unequal knowncfg allowed, same-byte wrong namespaces/missing/dangling/corrupt targets refuse; confirmation-linked CID accepted without original-support equality |
| SP13 | Complete ancestry/optional presence and every event member immutable across append; forbidden event rewrite cannot publish partial scalar/child/head changes |
| SP14 | Fault after observed+1 header/revision/child rows or actual head change rolls back complete snapshot/earlier adopted work; external flags prove stage, retry recomputes from durable head |
| SP15 | Selected malformed scalar/child type/group/enum/ordinal/history ambiguity versus well-typed materialization bounds; inside-catch independent BEGIN IMMEDIATE while store alive proves synchronous rollback/no normal result |
| SP16 | Two individually fitting selected results exhaust cumulative budget after proved staged writes; no hidden per-query reset, truncation or full-read quota substituted for append validation |
| SP17 | Foreign/stale/terminal tickets, inert store lifetime and owned results preserve existing capability truth; physical commit/cleanup acknowledgment loss is unknown/no durable payload/one body/no replay |

Actual different logical plus root physical review precedes minimal API/stubs.
Then actual API/test/fixture/harness reviews, successful compilation, selected
official Qt green proof and genuine first missing-behavior red precede STORAGE-015.
No old red, native feature body, rate provider, live credential/action, application
bundle, permission/evaluator, effect/job, backup/update/recovery/power-loss or
platform/UI/packaging release proof is supplied. All seven integrations and all
three exchange-rate providers remain intended release scope.

Root now supplies the actual [migration4 physical proposal](sqlite-application-plans-schema-v1.md)
(five tables/30 total objects/29015-byte payload); full different logical/physical
review must inspect that exact companion before minimal API/stub authorization.

## Concrete API precision for separate review

The new internal [API](../../server/include/subathon/server/sqlite_application_plans.hpp)
uses SqliteApplicationPlanStore over a copied SqliteWorkSession and SqlitePlanLimits.
Constructor/copy/destructor are inert. Methods are const, synchronous and repeatable;
there is no mutable single-use repository guard, construction-thread identity,
BEGIN/commit method, authority transfer or store-destructor cleanup. Valid calls
reach the actual ticket, which enforces true owner/work/phase/lifetime. Pure input
checks precede that access even on a stale/foreign ticket. The caller still owns
active-session exclusivity, immutable input buffers through each call, real commit
and publication; returned values own their bytes/Exact data beyond session closure.

### Exact declared functions and staged results

- encode_plan_revision_uint64(uint64) -> QByteArray and its decode counterpart
  cover the FULL numeric0..MAX range. Null or length other than8 is invalid_codec.
  No method treats codec0 as a valid stored revision/head/parent.
- SqlitePlanExactComponents owns numerator/denominator QByteArrays.
  encode_plan_exact(Exact,component_bytes=16384) and decode_plan_exact(components,
  component_bytes=16384) operate only on canonical rational VALUE described above.
- application_plan_repository_manifest() is pure description of exact immutable
  prefix1/2/3 plus approved step4. No SQL/open/operational authority.
- retain_initial(plan_ref,initial CapturedPlanRevision) -> inserted, already_present
  or conflict. inserted means complete staged revision1, never durable success.
- find(plan_ref) -> optional complete owned ApplicationPlan.
- find_revision(plan_ref,positive revision) -> optional SqlitePlanRevisionSnapshot,
  containing owned plan_ref and ONE CapturedPlanRevision. The distinct snapshot
  type deliberately makes no complete-history/current-preview certification.
- append(plan_ref,expected_head,next CapturedPlanRevision) -> appended,
  missing_plan, stale_head, revision_conflict or event_conflict. appended is staged.
  revision_conflict means a locally valid candidate whose number/parent does not
  follow the actual matching head; event_conflict means an event rewrite. Both,
  like stale/missing and initial conflict, make no primitive writes/cleanup.

expected_head is an optimistic token, not a persisted domain value. ALL uint64
tokens, including0/MAX, are valid arguments: expected0 is stale against every
valid stored head. Do not reject it as a zero revision, bind it through signed64,
or calculate expected+1 before reading/validating actual head. Targeted revision0
is invalid_revision. No preview/current authoritative state token is introduced.

### Options and complete field-to-bound mapping

SqlitePlanLimits has exactly the six proposed fields/defaults. reference_bytes,
value_bytes and component_bytes must be positive representable qsizetype values.
For each count reject n<1 before any unsigned conversion; using U(nonnegative)
as std::uintmax_t, require strict n<qsizetypeMAX and n<qint64MAX. Additionally:

| Count | Further exact representability predicate |
| --- | --- |
| revisions_per_full_find | U(n) < U(std::vector<core::CapturedPlanRevision>{}.max_size()) |
| items_per_vector | U(n) strictly below EACH max_size of std::vector<core::RuleSelection>, std::vector<core::ModifierSelection>, std::vector<core::RequestedEffect> |
| items_per_full_find | U(n) < U(std::numeric_limits<std::size_t>::max()); it is a checked TOTAL across separate vectors, not capacity of one invented vector |

These strict comparisons reserve n+1 lookahead without first overflowing signed
arithmetic. No huge vector is required to be allocated by validation, and no memory
availability is promised. Aggregate item counters use checked addition/subtraction;
full-find totals count all three returned child collections across every revision.
The total excludes revision scalars, the shared event and unmaterialized metadata.
Full read fails if either per-vector or total bound would be exceeded. Targeted
read uses the same per-vector bounds for its one selected body and does NOT apply
either full-plan count bound; append/initial input uses per-vector bounds only.

The metadata audit's private page size is64 rows, independently of the full-plan
read bound; page/key arithmetic and all result lookahead still require checked
conversion. Use strict greater-than-last canonical revision key, ordered ascending,
with termination and expected next revision verified. A full page is not permission
to skip another page. This defines bounded page materialization, not a fixed SQL
statement count or hard callback deadline. Existing cumulative statement/binding/
result budgets remain unchanged and may refuse any operation safely.

| Bound | Every covered byte field |
| --- | --- |
| reference_bytes | plan_ref; event_ref/root_event_ref/optional parent_event_ref; receipt origin/provider/account/identifier namespace/opaque ID; receipt optional payment/activity namespace+ID pairs; arrival connection/configuration/modifier-context refs; ContributionState resolution/policy/decision/decision-actor refs; HistoricalValuation resolution/snapshot/optional source/manual-decision refs; rule_ref/version_ref; modifier definition-version/instance-revision/target/group/group-operator/target-operator; request action/action-version/rule/target/reversal refs; override actor/decision refs; actual selected retained keys/known links |
| value_bytes | receipt event_type/optional actor ID/display-name/money decimal+currency; ContributionState original-money decimal+currency; HistoricalValuation original-money decimal+currency/comparison currency/effective-rate/optional fetched-rate spellings; rule explanation; modifier original factor; request calculation explanation; override reason; selected retained document payloads |
| component_bytes | Canonical ASCII numerator and denominator separately, including a numerator minus sign |

All optional-present strings use their field's bound, even when empty. Only the
already agreed required identities and override audit fields require nonempty;
empty policy/decision/actor/resolution/rule/action/quote strings otherwise remain
present exact BLOB. Limits never become a grammar, currency-normalization, semantic
namespace or provider-verification rule. No original receipt schema/timestamp
narrowing or new source version/time-zero restriction is added.

### Pure preflight and exact input categories

All store methods check the complete options predicate first (invalid_options),
then nonempty/bounded plan_ref (invalid_plan_ref). Find_revision then checks its
requested revision>0 (invalid_revision). Retain/append next check this ordering:

1. Numerical role/parent shape: initial revision1 with no parent; next revision>1,
   parent>0/belowMAX and revision=parent+1 after checked arithmetic. Otherwise
   invalid_revision. This is candidate-local shape, not relation to expected_head.
2. Sizes of all three candidate vectors, before walking them: collection_limit if
   any exceeds items_per_vector. Full-find count limits do not limit these inputs.
3. Override role: initial has no override; append has authorized2 and nonempty
   bounded actor/reason/decision. Wrong authority/presence/audit -> invalid_override.
4. ReceiptId0 -> invalid_receipt_id, then full event/source byte/required-ref shape,
   ancestry presence and required nonempty event/root refs -> invalid_event.
   Any optional ContributionId0..MAX is allowed; its real target is checked later.
5. ContributionState known enum/group/member bounds -> invalid_contribution_state.
6. PlanValuation known state/whole quote presence/original-money equality and
   missing_historical versus bounded request dependency flags -> invalid_valuation.
7. Rule fields in order -> invalid_rule, then modifier fields -> invalid_modifier,
   then request known unit/byte fields -> invalid_request. For each request its
   exact rational component bound check follows these request shape checks and
   propagates exact_component_limit; no numeric correctness/rounding is computed.

This new storage preflight is distinct from core revise_application_plan's existing
history/expected-token/decision/source order; no core function is changed, called
with fake sources or relabelled to implement these checks. Pure input failures
are SqlitePlanInputFailure deriving std::invalid_argument; when caught they do no
SQL, rollback or caller-unit mutation. A later stored comparison failure is never
misreported as one of these pre-SQL categories.

Standalone Exact functions check positive component_bytes first (invalid_options).
Encoding rejects components whose canonical ASCII length exceeds it as
exact_component_limit, without invoking parser/finalizer policies. Decoding checks
null/empty components first (invalid_exact), then either component's byte overage
(exact_component_limit), then canonical ASCII grammar/positive denominator/reduced
pair/zero0over1 (invalid_exact). Numerator sign counts toward the same byte bound.
QByteArray is a byte input, not SQL; these functions do not validate QVariant types
or claim stored BLOB proof. Selected SQL component decoding first checks actual
SQL type/presence/declared length, then bounds before expensive grammar/arithmetic:
wrong SQL class/shape -> invalid_stored_data; a larger correctly typed BLOB uses
result_limit before inspecting its unread digits; fitting noncanonical data uses
invalid_stored_data. No silent truncation or normalization. Combined-error priority
is explicit; malformed cases and valid one-over cases must have independent oracles.

### Actual selected scope and no-own-write refusal ordering

Every method first performs the pure checks above, then actual session operations.
Missing header returns absent/missing_plan; it does not trigger a global orphan-row
audit or proposed new-target resolution. For a present header, validate exact
stored identity/ancestry/positive head and actual receipt-original projection with
typed cfg/context/known-config resolution and the same reference/document bounds.
A context's known configuration may differ from arrival configuration.

The present-CID lookup is deliberately a physical ID/REFERENCE projection: select
actual contribution_id and original_receipt_id, strictly decode BLOB8/nonzero
receipt/BLOB8 CID including0, require the matching contribution identity and an
actual correctly typed matching original receipt identity target. It does NOT
materialize/claim validation of that contribution's current state/valuation,
original facts/arrival/context or original support observation. Those unselected
payloads/anchor remain STORAGE-014/application validation obligations. Never require
that support receipt equals this PlanEvent receipt, impose observation kind/linkage
or compare live state with captured historical state. Missing/dangling/wrong-class
selected identity/reference fields fail, not fallback to another namespace.

Full find validates basic numeric summary (all revision key types/domain,
cardinality/min/max/head/count) before its full-read revision bound. Inconsistent
summary -> invalid_stored_data, not result_limit. A count over the bound then fails
result_limit without claiming unread scalar/child payloads valid. Otherwise decode
all scalar rows/immediate parents/audits/capture valuation and full ordered children
under all selected bounds, yielding a complete ApplicationPlan only on full success.

Find_revision validates header/actual max agreement and initial-anchor scalar/
audit/capture-valuation metadata INCLUDING actual original quote money and all
request dependency flag metadata. No complete child payload/Exact grammar/ordinal
validity is claimed for an unselected initial anchor. A missing requested row then
returns absent even when its absence would be a global gap; a missing/invalid
required initial or head/max marker is invalid_stored_data. A found target validates
its complete selected scalar/body/children and local positive immediate-parent
identity target; it does NOT recurse into that parent's own chain/body. Its distinct
snapshot result never certifies a complete history. This is precisely the limited
scope of the sparse MAX diagnostic, not a corruption-tolerant aggregate import.

For existing retain_initial and every append, perform the complete declared
structural metadata audit independent of full-read caps: numeric summary plus
paged chain, immediate parent, required authorized audit and capture-valuation/
quote-original-money/dependency-flag checks. Old child numeric/string payloads
not selected by that audit are not certified. For existing initial retention,
fully decode/compare the actual initial scalar/ordered bodies and complete event:
exact match -> already_present, difference -> conflict, before checking newly
proposed targets. A selected malformed original cannot be hidden by conflict.

For append, after healthy old structural audit compare expected_head: mismatch
(including expected0) -> stale_head. Only a matching actually valid MAX head can
cause revision_exhausted; a sparse MAX fixture first fails structural history.
For a matching non-MAX head, a candidate number/parent different from actual
head+1/head -> revision_conflict; then any complete event difference -> event_conflict.
Each is a no-own-write result, and none resolves proposed replacement targets first.
Only a successful exact relation stages the complete new scalar/children/head.
No stale token, event conflict or duplicate performs store cleanup or discards
unrelated earlier writes. No duplicate result can turn into an upsert/retarget.

For new initial retention, only after header absence resolve proposed actual
receipt/projection/typed targets, then stage a complete first revision/header/
children. Missing actual receipt -> missing_receipt; missing new CID target ->
missing_contribution; different otherwise valid original -> receipt_mismatch.
Stored malformed/dangling source/header target is invalid_stored_data. Stored
source/selected limits take precedence over a candidate projection mismatch;
never compare guessed/partially decoded data or normalize it to obtain equality.

### Known failure, allocation and uncertainty boundaries

SqlitePlanStorageFailure is the new known-uncommitted port error with exactly
SqlitePlanStorageReason: busy, sql_failure, resource_limit, result_limit,
missing_receipt, missing_contribution, receipt_mismatch, invalid_stored_data and
revision_exhausted. Domain no-own-write dispositions are not storage exceptions.
Stored enum/group/revision/ordinal/chain/ambiguity/canonical component faults and
dangling selected targets use invalid_stored_data; selected byte/component/
collection/full-read bounds use result_limit. Busy/SQL/resource/result failures
map only after the actual writer has established the recorded rollback/outcome.
Existing retention selected failures map their known invalid_stored_data/result_limit
after its real rollback; an inactive second cleanup is not fresh rollback proof.

Local selected failures on an active unit must establish actual rollback before
throwing a known port exception. Capability/phase/protocol/invalid-statement and
other unmapped WorkFailure stay original. Unknown COMMIT/cleanup always stays
SqliteWorkFailure(uncertain_outcome), outranks local reasons, closes admission and
provides no permission to replay. The caller owns real commit/control faults;
the store has no hidden commit path or implicit recovery/resolution. Mapping
inside this facade covers only failures raised by its own primitive calls; a
caller session.commit failure remains that caller's original SqliteWorkFailure,
not a store exception.

Prepare/guard initial+append writes so a caught allocation exception after any own
writes cannot leave a partial head/header/revision/child set. Cleanup covers the
whole existing caller unit, including earlier staged work, or actual unknown
outranks the original allocation exception. std::bad_alloc is not automatically
labelled resource_limit or a known port error. No cleanup is performed merely for
pure input or a no-own-write domain refusal. Store destruction remains inert even
with caller-staged writes; owner session finalization is the actual backstop.

The initial [source](../../server/src/sqlite_application_plans.cpp) contains only
an inert session/options constructor and NINE immediate named
SqliteApplicationPlansNotImplemented logic-error throws: two uint64 codecs, two
Exact codecs, manifest and four store methods. No validator, serialization,
manifest/schema source, SQL/target/metadata audit, counter, write guard or durable
behavior is implemented by these stubs. Invalid calls also reach that explicit
marker; this makes no claim that validation passed or caller writes rolled back.
Different actual API/stub/shared-build precision review precedes any new tests/
fixtures/green diagnostic or production behavior.
