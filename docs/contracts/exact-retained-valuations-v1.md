# Exact retained valuations — logical bridge revision 1

Task: [RATECONTRACT-001](../tasks/EPIC-017/RATECONTRACT-001.md).
Authority: specification v0.45 [§3.5](../specification.md#35-numerical-precision-and-validation)
and [§9](../specification.md#9-currency-conversion-and-reporting), NUM-04, FX-03/06.
This is a new logical contract and proposed examples, not an amended existing ABI,
SQL/byte format, authored executable baseline or runtime capability. The reviewed
[rate API](rate-snapshots-api-v1.md), [plan](application-plans-v1.md),
[plan store](sqlite-application-plans-v1.md) and
[application service](application-service-api-v1.md) keep their existing meanings.

## 1. Gap and affected surfaces

The exact inverse of the observed EUR→USD `1.20` is USD→EUR `5/6`.
`HistoricalValuation.effective_rate_decimal` cannot represent that inverse as a
finite decimal. An available exact pair must reach accounting without rounding
it, replacing it with a current quote, or labelling it missing. Original fetched
values remain lossless decimal data; only a *derived effective value* needs the
explicit rational representation below.

The audit at main `c70fbec` identifies these concrete downstream obligations:

| Existing surface | Required bridge work / owner |
| --- | --- |
| `core/include/subathon/core/rate_snapshot.hpp`, `core/src/rate_snapshot.cpp` | RATES-001 remains unchanged. Adapt its owned `RatePairResult` and actual immutable snapshot; a pair's three provenance strings alone are not retained evidence |
| `core/include/subathon/core/contribution.hpp` / contribution stores | `ContributionValuation` carries resolution identity/missing-rate state, not a rate body. Preserve its existing meaning; RATECONTRACT-002 pins consistency with the new plan alternative |
| `core/include/subathon/core/application_plan.hpp`, `core/src/application_plan.cpp` | RATES-009 adds a versioned exact valuation alternative to capture/revise histories; ACCT-003's existing legacy transformation assertions remain intact |
| `server/src/sqlite_application_plans.cpp`, `sqlite_application_bundle_access.hpp` | RATES-009 must update both plan and bundle hydration/validation paths and their selected/full closure rules consistently, through an approved additive storage design |
| `server/src/sqlite_application_bundle_values.hpp` | Complete capture/bundle copy and byte accounting must include the new alternative and retained provenance; the existing decimal-field sum is insufficient |
| `server/src/application_service_command.cpp`, `application_service.cpp` | SAC1 references a plan key; SAA1's Plan is also a key, not an embedded valuation. SAR1/SAH2 retain command/result/failure facts. Preserve their frozen bytes where semantics stay the same; audit selected-plan/duplicate/hold closure and budgets rather than blindly versioning every envelope |
| `server/src/sqlite_reference_capture.cpp`, `reference_capture_values.cpp` | STORAGE-018 selects legacy plan columns and counts valuation bytes. Its actual SRC1/SRA1/SRX1/SRI1 path requires no money, not-applicable valuation and one fixed request. Preserve that scope; exact monetary support cannot be inferred or added to those v1 bodies |
| `subathon_plan_revisions` and catalog factories | Migration4 embeds the finite-decimal group. All shipped migration SQL, IDs, framing, digests and prefix expectations stay unchanged; RATES-009 needs a separately approved additive representation and full compiled-prefix tests |
| [retained configuration/context](retained-context-v1.md) | Opaque retention is not typed rate decoding, exact valuation support or proof that a referenced snapshot exists |

Before RATESTEST-009, a separate **RATECONTRACT-002** must freeze the concrete C++
alternative, codec dispatch/byte framing, field/aggregate ownership profiles,
selected-read semantics and additive SQL/catalog/compatibility plan for these
surfaces. It must identify any further independently executable prerequisites as
dependency-linked tasks before expanding implementation. No new migration number,
existing-format suffix or ABI is allocated by this document. RATESTEST-009 then
supplies reviewed literal tests and intended behavioral red; RATES-009 implements
that baseline. Existing reviewed bodies remain executable and unchanged unless a
separate explicit baseline-revision approval is obtained under the workflow.

## 2. New exact valuation meaning

A new resolved capture has logical version `exact-retained-valuation.v1` and owns:

| Field | Meaning |
| --- | --- |
| resolution_ref | Nonempty immutable valuation decision/version identity |
| original_money | Complete original decimal and currency bytes, exactly equal to the immutable source event's money; numeric value is parsed exactly |
| comparison_currency | Destination currency of this accounting valuation, not the current display preference |
| effective_numerator / effective_denominator | Strictly positive coprime integers in canonical ASCII base10, no sign, whitespace, exponent, leading zeros or separators; denominator is never zero |
| basis | Exactly `identity`, `snapshot_pair` or `manual_valuation`, with the distinct obligations below |

Orientation is always units of comparison currency for **one** unit of original
currency. `N/D` describes the effective rate, never a monetary amount, effect,
provider decimal lexeme or rounded display. An accepted record has `gcd(N,D)=1`;
`5/6` is represented by strings `"5"`, `"6"`. On decode, reject noncanonical
encodings rather than normalizing stored bytes and concealing corruption.

For `snapshot_pair`, original and comparison currency differ. Own the selected
snapshot's full model value, including its actual snapshot/parent identities,
optional construction time, selected complete observations, overrides, active
constraints and components. Also retain the exact `(snapshot_ref, from, to)` pair
key. It must match the owned snapshot and both currencies, and exact pair lookup
must return `available` with rate `N/D`. Since the current query accepts
`EffectiveRateSnapshotInput`, RATECONTRACT-002 must define bounded reconstruction/
revalidation from the retained owned snapshot, never a mutable current input. Missing or contradictory evidence cannot
be replaced by the supplied numerator/denominator alone. There is no separately
editable singular fetched-rate/source/timestamp slot: a cross-rate may use several
observations/providers and pins. Keep each original quote spelling, orientation,
ordinal, observation/configuration/request/evidence identity and all separate
fetch/source-rate/publication time fields through the snapshot model.

For `identity`, the currency bytes are equal and the rate is exactly `1/1`.
There is no snapshot, provider quote or fabricated source/time/override evidence.
This is the rate model's arithmetic identity, including for an unrepresented key;
actual currency identification and support admission remain upstream duties.
A caller may not use equal unknown tokens as proof of a supported real currency.

For `manual_valuation`, original and comparison currency must differ, as must
the two entered currency keys. Equal-currency valuations must use `identity`
with exactly `1/1`; manual input cannot override arithmetic identity. Retain an
authorized historical rate for this one contribution without modifying the table. Own the entered strictly
positive decimal spelling, entered from/to currencies, nonempty decision/actor/
reason identities and optional recorded UTC time. The entered pair must match the
original/comparison pair in either direction; parse the exact decimal and invert
only when entered backwards. Its exact result must equal `N/D`. No unrelated
cross path, snapshot, provider/source/fetch/publication facts, temporary/pinned
kind or table-wide inverse/cross update is implied. Preserve the entered spelling
and direction independently from the canonical effective fraction.

RATES-009 owns this value/capture alternative and its pure consistency checks.
RATES-008 owns actual historical/manual-rate resolution; ACCT-005 owns authorized
held-contribution review/continuation; API-002 owns transport authorization and
revision checks. Those still-planned capabilities require their own contracts,
baselines and implementation before use. Their absence cannot be replaced by
DTO strings. A manual resolution of a held contribution follows the audited
append-only plan-revision path with actual current permission/head checks; it
neither clears other holds nor reapplies prior committed effects.

All three basis kinds retain the resolution identity and original source precision.
`1.20`, `1.2000`, `1.2e0` remain different original evidence even when numerically
equal. New captures use the existing exact decimal grammar/domain/resource rules
and currency-byte semantics; this does not retroactively narrow receipt storage.
A manual-only snapshot is valid evidence without an invented provider observation.
Decision/actor/reason strings are provenance, never execution permission.

Not-applicable and missing-historical remain distinct quote-free states. A missing
foreign pair produces unresolved valuation and no dependent finalized request;
it cannot use `1/1`, a current rate, a decimal approximation or a best-effort prefix.
Unknown provider metadata remains absent, including original-text-only dates whose
UTC instant is unknown. Known UTC zero and present-empty optional source text
retain those distinct meanings. Fetch, rate and publication times never fill one
another. An exact pair exceeding a downstream resource limit is **available but
refused by that limit**, not missing historical evidence.

## 3. Durable ownership and unchanged history

RATES-005 owns durable fetched-observation/cache/sync retention; RATES-007 owns
durable effective-table/override revisions and their editing lifecycle. Those
future stores retain the rate model's source-evidence retention-policy obligation;
the opaque capture classification below neither verifies nor waives it. Those
stores must retain original source observations separately from overrides
and must not mutate old identities. Their planning briefs must assign concrete
storage/baseline gates when refined; pure RATES-001 supplies no store.

RATES-009 owns the **capture's durable closure**: one immutable plan revision must
reopen with the exact valuation and complete selected snapshot value above,
without a live provider, current cache, current pins or mutable configuration.
It may store a shared immutable snapshot record instead of repeating its bytes in
every row, provided typed identity/content checks and transactionally enforced
retention make the selected result indistinguishable from owning that value.
The concrete layout belongs to RATECONTRACT-002. Duplicate identity with identical
content is reusable; different content is a conflict, never an overwrite.

The capture closure has a fixed per-field classification; it does not assume
future provider/cache stores exist:

| Selected data / reference | RATES-009 retention and target-check obligation |
| --- | --- |
| Full selected observations, quotes, time fields, overrides/actor/reason values, constraints, components and their order | Own the complete immutable model value; it must validate without external provider/cache lookup |
| Snapshot identity and pair key; shared snapshot row if used | Typed exact identity/content match, validated in the same caller session before success; a missing/wrong-namespace shared target refuses capture |
| Observation provider/configuration/request/source-evidence refs; override decision ref; optional parent snapshot ref | Retain exact immutable identity bytes as declared by the model. They do not certify a retained target, raw provider evidence, permission or an available ancestry chain. No external-target lookup is required by this v1 capture alternative |
| Manual entered pair/spelling, actor/decision/reason and optional time | Own the entered values and audited resolution identity; actual authority is a separate current producer. No table-override or provider target is fabricated |
| Existing plan/event configuration, arrival context, decision and revision links | Preserve the existing owning plan/application store's typed validation and retention obligations; a provider configuration identity is not automatically in the application configuration namespace |

RATES-005/007 must later provide their own typed source/request/evidence/decision/
snapshot stores and admission/retention baselines. RATECONTRACT-002 must freeze
this per-field classification, not silently require those unimplemented producers.
A future adapter claiming typed external-target closure needs explicit reviewed
ports, dependency-linked store prerequisites and admission checks before that
claim is enabled; an opaque retained identity is never advertised as verified
source evidence. Parent identity retention does not promise recursive ancestry
materialization. Required selected snapshot rows and existing typed plan links
must remain retained while the capture refers to them, even after cache deletion
or pin removal. No unbounded lookup or provider request repairs history.

Caller-owned atomic commit includes the capture and any newly required closure
rows. All store results remain provisional until actual COMMIT and normal work
completion. Failure cannot publish a new plan head, success, credited effect or
job. Existing committed closure may be reused without a second write or a second
writer. A SQL/commit failure remains a storage/work outcome; unknown commit must
remain unknown and use the existing idempotent retry/reconciliation path, never a
blind second award. Pure parsing failures do not themselves persist review holds.

Later relevant sync, pin removal, override expiry or reporting-currency changes
never rewrite a captured valuation, exact request, final credited effect, rounding
residual, cap-limited actual amount, milestone/action or reversal reference.
Explicit authorized revaluation appends an audited plan revision using existing
identity/arrival/head/permission rules; it does not edit prior revisions or reapply
an already applied contribution. Reporting-only historical conversions are separate
provenance-bearing display records under RATES-008. A missing display conversion
leaves visible unresolved/partial amounts; it changes no accounting valuation.
Refunds use original committed effects and their allocation/reversal provenance,
not the refund-date rate or the current reporting currency.

## 4. Exact arithmetic, bounds and failures

Compute original exact money × exact effective rate × exact selected rule/modifier
factors and unit scaling; round **once per final requested effect**, half to even,
under CORE-001. No intermediate exchange-rate, amount or effect rounding is
permitted. Requested rational/finalized units/residual and cap-limited actual
components remain different facts. Existing evaluator/application ownership and
whole-input overflow handling remain; this bridge does not calculate permissions,
refund entitlements, rule selections or actual timer capacity.

The new bridge's rate components are individually at most 8192 bits, matching the
model's derived pair ceiling. Their canonical positive strings are at most 2467
bytes each; check byte length/ASCII shape and incremental bit bounds before big-
integer growth or gcd. The selected snapshot keeps all rate-model item/string/
component bounds, including 8MiB owned bytes. A new valuation's complete logical
owned payload (including its snapshot copy, original money and references) has a
16MiB ceiling. These local maxima do not enlarge any existing storage/service
limit. Active transport/storage/statement/result/intermediate limits may refuse a
candidate earlier; every actual owning copy and returned collection must be
charged under the concrete finite profile from RATECONTRACT-002.

Preflight complete field lengths/counts and checked aggregate size arithmetic
before copying/allocating attacker-sized inputs. Validate rate/snapshot consistency
before accepting the capture. Bounded streaming/borrowed validation is permitted;
partial accepted objects and unchecked multiply-then-measure are not. Arithmetic
construction has a fixed 16384-bit ceiling for each temporary integer, with checked
bit-size bounds and cross-cancellation before products. Refuse before growth when
a safe bound cannot be proved. These are arithmetic resource limits, not numeric
approximations or claims of unbounded arbitrary-precision processing.

CORE-001's existing finalizer remains limited to 4096-bit normalized numerator and
denominator. After exact cancellation/scaling a larger still-normalized final
request is a resource refusal; this contract does not change the finalizer. Signed
64-bit rounded range overflow holds the whole affected input without effects/jobs;
record the attributable failure/provenance through the owning reviewed application
workflow. Other resource/allocation/invalid/unsupported/storage failures retain
their distinct outcomes and commit no partial award. A preserved source receipt
need not be discarded because its valuation cannot be accepted locally.

The concrete gate must map, without conflation: missing pair; malformed known
format/nonpositive or inconsistent rate; unsupported/unknown version tag; missing
retained target; identity-content conflict; stale plan/current authority refusal;
resource/allocation refusal; numeric range overflow; SQL failure; and unknown
commit. This document freezes those distinctions, not C++ error enum values or
public response codes. Resource admission never certifies provider support or
financial confirmation.

## 5. Legacy coexistence and explicit version dispatch

Existing HistoricalValuation is a **legacy decimal capture**, not this new logical
version. Read/preserve its original fields, optional presence and immutable bytes
through existing readers. A well-formed stored v1 record may contain opaque strings
that the structural store never certified as rates. Do not newly label that record
corrupt merely because it fails a new semantic parser, or silently upgrade it.
An exact calculation adapter may explicitly interpret a positive legacy decimal
within the existing parser bounds; if it cannot, return a distinct unsupported/
unusable legacy-valuation outcome and preserve its bytes, never guess a rate.

A valid legacy effective decimal `1.2000` has mathematical rate `6/5`; its original
spelling stays `1.2000`. It does not prove a reconstructible rate-model snapshot,
source publication time or missing provenance. No migration invents those facts.
Legacy already-finalized requests/effects remain usable under their existing
contract without revaluation; unchanged duplicate results remain exact historical
results. The new exact writer never squeezes `5/6` into a legacy decimal field,
using `0.833333…`, a fraction string, NULL, zero or an empty placeholder.

RATECONTRACT-002 must specify explicit new/legacy dispatch and the additive schema
role that makes it representable. Old clients/binaries must refuse unsupported
schema/format through the selected catalog/format gates, with no writes, rather
than misread the new alternative. Frozen known v1 document grammar gets no trailing
field. Preserve old prefix SQL/digests and existing assertions; separately review
any revised baseline. Test populated legacy upgrade, new write/read, unknown
version, exact retry and fresh-process reopening, including malformed new data.
There is no authorized implementation shortcut through an untyped opaque blob.

## 6. Independently derived proposed examples

EV01–19 are authored logical examples, **not executed tests or observed outcomes**.
Fractions follow integer place value and multiplication; half ties use the even
integer. No production parser/bridge generates these expected values.

| ID | Given / operation | Required result |
| --- | --- | --- |
| EV01 | Snapshot S1 has EUR→USD observed `1.20` (`6/5`); capture original USD `1.20` to EUR | Preserve both independent `1.20` lexemes and directions; effective `5/6`, exact converted amount `1`; reopen identically |
| EV02 | S1 additionally has EUR→CHF `0.90` (`9/10`); USD→CHF | Exact `(5/6)×(9/10)=3/4`; both selected observations/constraints remain, no fabricated singular fetched cross quote |
| EV03 | Original USD `0.60`, USD→EUR `5/6`, coefficient1 millisecond/EUR | Exact `1/2` millisecond, final0; with original `1.80` exact `3/2`→2 and `3.00` exact `5/2`→2 |
| EV04 | Authorized signed correction USD `-1.80` with rate `5/6`, coefficient1 | Exact `-3/2` units→-2; positive rate remains `5/6`; monetary admission is separately checked |
| EV05 | USD `0.60` ×`5/6` × coefficient3 | Exact `3/2`→2; rounding converted amount `1/2` first would produce0 and is forbidden |
| EV06 | USD `11068046444225730969` ×`5/6`, coefficient1 | Exact `9223372036854775807.5`; upper half tie rounds to even `9223372036854775808`, overflow; no prefix effects/jobs |
| EV07 | Same currency bytes on a valid identified money value, no snapshot | Identity `1/1`, original spelling unchanged, absent provider provenance; unsupported currency admission is not supplied |
| EV08 | Distinct currencies in disconnected components; or a valid available pair refused by a storage byte bound | First is missing-historical; second is resource refusal. Neither may become zero, identity or an approximate accepted decimal |
| EV09 | New encoded numerator/denominator `10/12`, `0/1`, `5/0`, `+5/6` or `05/6` | Reject noncanonical/domain-invalid new capture; never silently normalize stored bytes |
| EV10 | S1 fetched time known0, source text `2026-11-04`/UTC absent, publication wholly absent | Preserve known0 versus unknown; no midnight/publication/fetch substitution; same facts after reopen |
| EV11 | Old effective `1.2000`, optional source absent, no reconstructible snapshot body | Preserve v1 bytes/absence; exact legacy interpretation `6/5` when admitted, no generated S1/source/time; original awards unchanged |
| EV12 | Old structurally valid effective string `not-a-rate`; unknown new logical version | Preserve old record with unusable legacy-conversion outcome; unknown new version refuses unsupported, neither is silently rewritten |
| EV13 | Same snapshot identity S1 with changed quote `1.25`; missing selected shared-snapshot target; wrong-namespace target | Conflict or explicit missing-target failure, no new plan head; preexisting committed S1 unchanged |
| EV14 | Temporary override later expires, pin removed, display currency changed to CHF | Original S1/capture/request/effect remain; a new snapshot/revaluation/display revision is separate, no original refund recalculation |
| EV15 | Request10ms, actual cap-limited credit3ms, later rate doubles and full refund authorized | Reversal basis is original actual3ms under owning refund policy; no second request10ms or new-rate6ms reversal |
| EV16 | Accepted pair `1/2^4096` reaches final request unchanged | Pair components fit8192; finalizer denominator has4097bits, so resource refusal even though rounded numeric value would be0 |
| EV17 | Purely proposed new closure+capture, staging/COMMIT failure; or commit outcome unknown | Known rollback publishes nothing; unknown stays unknown and uses original operation identity for reconciliation, never a new blind award |
| EV18 | New S1 capture committed, process/store reopened while current provider/pins/cache differ | Read original exact `5/6`, source spelling `1.20`, all selected provenance/unknown fields and original finalized/actual effects; current values never repair or replace history |
| EV19 | No historical USD→EUR quote; authorized one-off manual entry EUR→USD `1.20`, original USD `1.20` | `manual_valuation` retains entered `1.20` and direction, actor/decision/reason and optional time; effective `5/6`, exact amount `1`; audited new revision, no temporary/pinned override or table mutation; same-currency USD→USD entry `1.20`/effective `6/5` rejects, never bypassing identity `1/1` |

EV16 is reachable under the current potential limit: weights `2^4095/1` and
`1/2` yield `1/2^4096`; each potential component fits 4096 bits. The executable
baseline must construct actual admitted snapshots, not an unreachable pair DTO.

Future acceptance needs literal independent codec/SQL/oracle fixtures, a reviewed
behavioral red before implementation, source/resource review, real SQLite/reopen
and applicable service/regression CI. Document approval alone supplies none of
those. All three exchange providers, schedules, override editing, historical
reporting, authority, platforms and full-release gates remain required.
