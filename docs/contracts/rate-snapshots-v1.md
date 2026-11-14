# Lossless fetched observations and effective rate snapshots — proposal v1

RATESTEST-001 document stage, from integrated main 0b631dc and claim 6338966.
Authority: specification v0.45 §§3.5,9,15, NUM-01/02/04 and FX-06;
[RATES-001](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-017/RATES-001.md),
[record boundaries](event-state-boundaries-v1.md) and
[exact numeric revision 1](exact-decimal-v1.md).
This defines a small pure value-model contract and independent examples, not a
C++ API, schema, store, generator, test baseline or implementation. Different
actual document review precedes those stages. Production RATES-001 remains planned.

## 1. Responsibility and exact source data

The model preserves original money and positive rate spellings, supplied time/
source evidence, and an internally consistent effective table with explicit basis
provenance. Core uses selected Boost.Multiprecision integers/rationals, C++20 values
and owned data. It has no Qt, network, provider SDK, database, clock or UI dependency.
Existing parse_decimal and parse_positive_rate are the selected decimal boundary;
no new numeric library or binary floating-point conversion is proposed.

CurrencyRef is a nonempty OPAQUE byte identifier, at most64 bytes. Equality/order
is exact unsigned-byte equality/ordering. No ISO-three-letter requirement, registry,
uppercasing, case folding, Unicode normalization or alias inference is imposed.
Thus USD and usd remain different keys until an independently reviewed upstream
mapping establishes otherwise. A valid key is not proof that a provider supports
it or that an unrecognized source label has known currency meaning. Keys are data,
not permission/provider capabilities. Larger identifiers are an explicit local
resource refusal; original receipt fields remain retained and are never truncated.
This is a bounded new model, not a retrofit of ReceiptMoney's existing storage.

OriginalMoney preserves the COMPLETE existing ReceiptMoney original_decimal and
currency bytes and the exact numeric value obtained from parse_decimal. Preserve
trailing zeros, exponent spelling and signed zero. The currency satisfies the
CurrencyRef bound; the original spelling remains unchanged. Zero and signed amounts
are representable data; support/refund/correction admission is an upstream policy,
not inferred from a sign. Absent money stays absent, not a zero-valued OriginalMoney.
The model does not round, scale to micro-points or change the original amount to a
reporting-currency amount.

PositiveRate preserves an original decimal spelling and its strictly positive
Exact value, tied by parsing. It follows the EXISTING ASCII JSON-number grammar,
512-byte/256-mantissa-digit/absolute-exponent256 limits and error priority in the
exact-numeric contract. Numerically equal spellings such as 1.20 and 1.2000 remain
distinct evidence. Zero, negative, NaN/Infinity, fractions in a decimal field,
whitespace, locale separators and trailing junk are rejected. Do not infer a
provider's payload number format from this internal grammar: adapters must verify
and preserve their real source lexeme before supplying this value.

## 2. Fetched observation and honest time provenance

A FetchedObservation owns the following logical fields:

| Field | Meaning |
| --- | --- |
| observation_ref | Nonempty local immutable observation identity |
| provider_ref | Nonempty source/provider identity supplied by the actual adapter |
| configuration_ref | Retained provider/plan/configuration revision identity, without credentials |
| request_ref | Nonempty local request/attempt provenance identity |
| source_evidence_ref | Nonempty retained evidence identity; it must resolve under the caller's retention policy |
| provider_snapshot_ref | Optional exact provider-supplied version/identity; absent when unavailable |
| fetched_at_utc_ms | Optional actually known fetch observation UTC milliseconds |
| source_rate_time | Independent TimeEvidence for the source's as-of/rate time |
| publication_time | Independent TimeEvidence for source publication time |
| quotes | Ordered owned ObservedRate values, preserving this normalized observation's quote order |

Each ObservedRate has from/to CurrencyRef plus PositiveRate. Its orientation is
EXACTLY "units of to for one unit of from". No base currency, inverse, unit or
source date is inferred from a provider name. A quote has a stable one-based ordinal
inside this immutable observation; ordinals identify evidence, not a new global
rate ID. Duplicate directed from/to keys inside one observation are invalid even
if values/spellings agree; do not silently overwrite/deduplicate them. Cross-
observation repetitions are separate evidence.

Observed positive quotes may be mutually inconsistent because of source rounding,
different times or malformed upstream data. Preserve them as fetched DATA rather
than rewriting their values to create an apparent consistent observation. A positive
self quote other than1 can likewise be retained as supplied data; it cannot become
an active effective constraint. Model validity is not adapter/sync success and does
not certify that an inconsistent source is usable. Provider parsing/response policy
can separately reject its use without destroying retained observation evidence.

TimeEvidence has two independent optional fields: original_text (exact supplied
bytes, including a present empty value) and utc_ms (a genuinely known signed64 UTC
instant). Both absent means unknown. Text alone, such as a provider's date-only
"2026-11-04", does NOT establish midnight, timezone or publication instant. utc_ms0
is a known epoch instant, not absence. A known instant may have no textual source
when its provenance is already represented elsewhere. UTC normalization/unit/date
interpretation belongs to the verified adapter and retained evidence, not this model.

Fetch, rate-as-of and publication times never fill each other's missing fields.
No fetch=publication fallback, current-clock substitution, date-to-midnight guess
or successful-sync timestamp is synthesized. This model checks types/bounds and
preserves supplied values; it does not impose fetch>=publication or another guessed
ordering in the presence of clock skew/imported history. A caller may separately
flag questionable metadata. Freshness warnings/scheduling must use the actual
known fetch/publication instants, or explicitly report unknown age. A nonempty
raw date/label alone cannot prove a 72-hour publication age.

provider_ref is an opaque identity, not an enum of supported capabilities. Frankfurter,
ExchangeRate-API and Open Exchange Rates all remain REQUIRED release integrations.
No endpoint/auth/base-symbol/history/quota/update claim is made here or inferred
from §15's documentation baseline. Their current primary documentation and real
adapter evidence belong to RATES-002/003/004/090. Secrets, API keys and full credential-
bearing URLs have no model field; retain sanitized evidence under private policy.

## 3. One effective table with selected, retained evidence

EffectiveSnapshot owns snapshot_ref, optional parent_snapshot_ref, optional
constructed_at_utc_ms, source observations, active manual override evidence, active
constraint references and canonical connected components. References are opaque
immutable identities, not timestamps, sequence arithmetic or commit certificates.
An empty table may honestly represent no available foreign conversions; it cannot
by itself advance a successful-sync/cache pointer. Manual-only snapshots are allowed:
no fetched observation or imaginary provider timestamp is fabricated for them.

Source observations are complete owned COPIES of the selected FetchedObservations,
kept distinct from effective values; their original quotes/times are unchanged.
Every included source observation is referenced by at least one active fetched
constraint. Unselected quotes WITHIN that source observation remain preserved.
Observations not selected at all remain separately retainable outside this snapshot;
this value is not the entire provider cache/history.

Each active ManualOverrideEvidence owns override_ref, kind(temporary or pinned),
from/to CurrencyRef, PositiveRate, decision_ref, optional actor_ref, nonempty reason
and optional recorded_at_utc_ms. from and to are distinct. The original decimal,
selected mode, actor absence and decision provenance remain exact. Presence of a
decision_ref is DATA, never proof of permission. This slice accepts supplied evidence;
authorized creation/preview/expiry/removal needs RATES-007. A temporary override is
not silently given a timer expiry; its relevant-successful-update rule is a later
explicit operation. A pin is never converted to temporary by normalization.

An active constraint is EXACTLY one of:

- Fetched: observation_ref plus quote ordinal, resolving to that exact retained
  directed PositiveRate; no replacement decimal is stored in the reference.
- Manual: override_ref, resolving to that exact active override evidence.

No bare rate, unspecified source or hash replaces full evidence. Duplicate constraint
references are rejected. Every active manual override appears exactly once; otherwise
a supplied pin/temporary override could silently stop constraining the table. Each
selected fetched reference resolves to an included source observation and valid
ordinal. These constraints have distinct from/to keys; fetched self quotes remain
unselected observation data. All source/override identities are locally unique.
Global uniqueness/durable resolution/immutable historical retention are caller/store
obligations under CONTRACT-002, not guaranteed by a pure in-memory reference.

A component is a sorted nonempty set of distinct CurrencyRefs and positive Exact
potentials w(currency). Its anchor is its lexicographically smallest raw-byte key,
with weight EXACTLY1. Components sort by anchor and never share a currency. Each
weight is a reduced rational with positive denominator, not a decimal approximation.
For different currencies in one component:

~~~text
R(from -> to) = w(to) / w(from)
~~~

This representation guarantees inverse and cross consistency algebraically. It
stores no independently editable matrix cells. Reporting currency is NOT a stored
anchor/field: selecting another reporting currency cannot rebase or rewrite this
snapshot, original observations or overrides.

Validation of a SUPPLIED candidate requires:

1. Every active constraint has endpoints inside the SAME component and its exact
   rate equals w(to)/w(from). Check equality exactly; no epsilon/rounding tolerance.
2. Ignoring edge direction for connectivity, active constraints connect every
   component member to its anchor. A one-currency component needs no edge and
   provides identity only. No arbitrary unbacked foreign weight is accepted.
3. Anchor/order/positive-weight and local identity/reference invariants above hold.
   Redundant consistent cycles are allowed and retain ALL their evidence. A cycle
   conflicting by even a small exact amount rejects the entire candidate.

The validator does not solve a graph, choose a provider, discard a quote, prefer an
edge, move a pin or repair a conflicting candidate. RATES-007/provider normalization
must propose a consistent candidate and record why fetched observations were not
selected. This separates a checkable value from editing/merge policy. Fetched
source rounding does not entitle this model to round away a contradictory pin.

Pair lookup distinguishes identity, available and missing. Equal valid CurrencyRefs
have exact arithmetic identity1, including when no foreign quote covers that key;
this is not a claim of provider support/known currency identity. Different members
of one component yield the exact ratio and provenance pair(snapshot_ref,from,to),
whose full retained meaning resolves through this snapshot. Different components
or an unrepresented endpoint yield missing, NEVER zero/one/current rates or an
implicit provider request. Upstream unknown/unsupported currency identification is
still an adapter/review gate even when token bytes happen to match.

All values are owned immutable proposals. Copying/mutating/destroying input buffers
must not change accepted snapshots/observations. A changed quote/pin/time/spelling
requires a new observation/snapshot identity; never mutate a retained old value.
A reporting-currency selection or presentation precision change constructs no new
rate evidence and has no effect on historical credited time/score, milestones,
performed actions or original-effect refund references.

## 4. Resource/error policy and adjacent-operation boundary

These are explicit local engineering bounds, not provider limits or maximum legal
money/count values:

| Resource | Proposed ceiling |
| --- | ---: |
| CurrencyRef bytes |64|
| Identity/configuration/evidence/decision/actor references |256 bytes each|
| Optional raw time text |512 bytes each|
| Reason/explanation text |4096 bytes each|
| Source observations in one effective snapshot |16|
| Quotes in one observation / total included quotes |4096 /16384|
| Active manual overrides / active constraints |1024 /4096|
| Currency members across all components |256|
| Total collection items across the complete owned value |32768|
| Total owned logical bytes across the complete value |8MiB|
| Each normalized effective potential numerator/denominator |4096 bits|
| A derived pair numerator/denominator before/after reduction |8192 bits|

Logical owned bytes count each actual string's bytes, each Exact's canonical
signed-numerator/positive-denominator decimal bytes, each int64/uint64 as8 and each
tag/bool as1. An absent optional adds0; a present value contributes its value.
Collections count their actual elements recursively across source observations,
quotes, overrides, constraints, components and members. Every real owning copy
inside the complete value counts; a reference does not charge a second copy of
the separately owned target. Exact concrete DTO field walks are pinned in the next
API stage before baseline tests; this metric is not sizeof or a lifetime-history
quota. Pair-query ratio output is at most8192 bits per normalized component from
two4096-bit potentials; it does not relax CORE-001's separate4096-bit FINALIZER
contract. Later valuation/rounding integration must handle its own resource refusal
with retained input/provenance, never an approximate rate or partial credited effect.

Decimal input bounds are the existing CORE-001 limits above. Empty quote/source/
override/table collections are valid where the structural invariants allow them;
resource ceilings are not minimum counts. Optional text absence differs from a
present empty raw field. Required references/reason are nonempty; optional actor/
provider version refs, when present as identities, are nonempty. Time raw text is
DATA and may be empty. Signed64 timestamps preserve negative/zero values distinctly
from unknown. No direct wall clock is consulted.

Validate collection ceilings and bounded source/reference/decimal strings before
materializing copies or constructing large numbers, then decimal grammar/domain,
reference/shape, exact component bounds and consistency. Exact inputs already
constructed by a caller cannot retroactively have their construction bounded;
the concrete later API must check components before copying/new arithmetic or
accept bounded canonical component strings where appropriate. Derived ratios use
only two bounded potentials, not an unbounded multiplication chain. Fixed products
used to compare a constraint must have checked representability/resource bounds;
the concrete API will pin temporary/ownership/error details before any tests/code.

Rejection categories are logical invalid reference/currency/decimal/rate/time,
duplicate identity/quote/constraint, unresolved basis, noncanonical/unbacked component,
contradictory constraint and resource limit. No partially valid snapshot or fitting
prefix is returned; input evidence is unchanged. Missing pair lookup is an honest
result, distinct from invalid/zero rates or a malformed snapshot. C++ public types,
precise exceptions/error order and storage/transport encodings are the NEXT separately
reviewed API stage, not supplied here. Local errors do not persist a NUM-04 review
hold or authorize a cache/state update.

RATES-007 must compare parent/current active pins during actual updates, preserve
pins until explicit authorized removal, reject contradictory pins BEFORE save,
preview affected inverse/cross rates and expire temporary overrides only on a
successful RELEVANT update. A snapshot by itself cannot detect an omitted pin from
an unseen parent; it does not certify these temporal operations. In particular,
changing reporting currency cannot remove/rewrite either override mode.

RATES-002/003/004 retain response parsing/metadata/capability verification; RATES-005
retains caching, retries, request accounting and 12-hour/24-hour/72-hour behavior;
RATES-006 retains ordered fallback and actual replacement source; RATES-008 retains
historical display retrieval/manual valuation and visible partial/unconverted totals.
Do not silently use a current snapshot for absent historical data. Backend financial
valuation/refund/rounding/application/permission and persistence remain their existing
tasks. All seven integrations and all three FX providers remain required.

There is a concrete EXISTING integration gap: HistoricalValuation in
core/include/subathon/core/application_plan.hpp stores effective_rate_decimal;
STORAGE-015 and retained plan/service contracts currently preserve that finite-
decimal projection. Exact inverse1.20 is5/6, which has NO finite decimal spelling.
Do not round it, write "5/6" into a decimal-only field or claim this model plugs into
that old projection. RATESTEST-009/RATES-009 now explicitly own the dependency-tracked
exact valuation/plan/command/storage bridge, its compatibility/migration review and
its own independent tests/red. No header/schema/codec changes occur here. Snapshot
model acceptance cannot finish that bridge or claim actual credited-rate adoption.

## 5. Independent literal examples and next gates

All examples are synthetic. No named provider was contacted and none of its payload,
metadata or capabilities is inferred. Fractions come from place value and exact
rational algebra, not an implementation under test.

| ID | Input / required result |
| --- | --- |
| RS01 | OriginalMoney decimal `1.234500`, currency `USD`: retain both strings exactly; value2469/2000. No rounding to cents or micro-points |
| RS02 | Rates `1.20`, `1.2000`, `1.2e+00`: exact6/5 each, distinct original strings. `0`, `-0.00`, `-1`, `NaN`, `1/2`, ` 1.2` and trailing junk are not PositiveRate |
| RS03 | Fetched source O1 supplies USD->EUR `1.2000`, USD->GBP `0.6000`. Canonical component anchor EUR: w(EUR)=1, w(GBP)=1/2, w(USD)=5/6. Selected references(O1,1)/(O1,2) satisfy both source rates |
| RS04 | RS03 gives EUR->USD5/6, EUR->GBP1/2, GBP->USD5/3 and USD->GBP3/5. Every inverse/cross is exact; no recurring decimal approximation is stored |
| RS05 | Identity USD->USD=1; unknown-token X->X is arithmetic identity only. USD->JPY missing if JPY is absent. A separate JPY/KRW component does not create USD->JPY |
| RS06 | O1 fetch UTC5000, source raw `2026-11-04` with UTC absent, publication both absent: retain that exact state. Publication age is unknown, not5000 or invented date-midnight |
| RS07 | Known source UTC0 differs from absent; known fetch/publication instants remain independent. Changing only fetched_at does not change the pair rate or rewrite source publication evidence |
| RS08 | Fetched USD->EUR1.2, EUR->GBP0.5, USD->GBP0.7 remains three positive observed data rows. Selecting ALL as active constraints is contradictory: 1.2*0.5=0.6, not0.7. No effective candidate is returned or quote overwritten |
| RS09 | Pins P1 USD->EUR1.25 and P2 EUR->GBP0.5 require canonical EUR1/GBP1/2/USD4/5, hence USD->GBP5/8. Additional pin USD->GBP0.7 conflicts; rejecting the whole candidate preserves all supplied evidence |
| RS10 | In the consistent two-pin RS09 snapshot, O1's only rates (USD->EUR 1.2000 and USD->GBP 0.6000) both disagree with the pinned table (1.25 and 0.625). O1 remains immutable and retained OUTSIDE this EffectiveSnapshot; it is not included as a selected source and no fictitious fetched constraint is added. The snapshot contains P1/P2 and their manual provenance, each active override referenced exactly once. Neither original observation bytes nor pins are rewritten |
| RS11 | Changing reporting currency EUR to GBP changes only a lookup/display request: all snapshot IDs/weights, raw original amounts, original rate evidence, pins and previously credited effects remain identical |
| RS12 | `USD` versus `usd`, absent versus present-empty raw time, and rate spellings1.20 versus1.2000 remain distinguishable. Duplicate directed keys inside O1 are rejected rather than merged |
| RS13 | A manual-only consistent snapshot has no fetched source or provider/fetch/publication time fiction. Invalid/absent decision provenance cannot become an authorized edit merely by constructing a model value |
| RS14 | Scaling all RS03 weights by2 preserves ratios but is noncanonical (anchor2); reject that supplied representation. Unbacked foreign currency weights, overlapping components, missing observation/ordinal or omitted active override reference also reject |
| RS15 | 256 effective component members across all components is the local table boundary;257 is resource refusal before copying that oversized table. Unselected fetched quote keys remain governed by the separate quote/owned-value ceilings, not this component-member count. 4096-bit normalized potential components are the boundary;4097 reject before new ratio arithmetic. Source numeric512/256/256 bounds remain CORE-001's exact rules |
| RS16 | Original money1.234500 USD times RS03 rate6/5 is exact7407/5000 EUR for a later display calculation. Such a display projection does not replace the original USD amount, its captured valuation or any awarded time/score |
| RS17 | The immutable RS04 EUR->USD5/6 cannot be represented by HistoricalValuation.effective_rate_decimal. The separate RATES-009 bridge must carry exact rational/provenance end-to-end; model success is not bridge acceptance |

Next: different actual logical-document review; separately reviewed concrete owned
C++ API/limits/errors; minimal inert types/explicit unimplemented stubs/build harness;
independent literal source/metadata/fraction fixtures and meaningful tests/review;
actual valid missing-feature red; only then RATES-001 implementation and separate
code/CI acceptance. Old decimal tests, existing SQL passes or these written examples
cannot substitute for this new slice's reviewed test/red gate. No live/provider,
platform, history/credit bridge or full release verification is claimed.
