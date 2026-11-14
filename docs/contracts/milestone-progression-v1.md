# Sequential milestone progression — proposal v1

MILESTONETEST-001 logical contract/examples stage, from integrated main `3217c21`
and explicit claim `f04e833`. Authority: specification v0.45
[§8](../specification.md#8-score-milestones-and-reversals), MILE-01, NUM-04,
[MILESTONE-001](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-005/MILESTONE-001.md),
[record boundaries](event-state-boundaries-v1.md) and
[exact numeric revision 1](exact-decimal-v1.md).
This proposes pure values, invariants and independent examples, not a C++ API,
header, schema, store, fixture, executable baseline or implementation. Different
actual document review precedes those stages. Production MILESTONE-001 remains
planned.

## 1. Responsibility and score domain

One sequential track consumes an already authoritative post-effect score and an
active completed prefix. It derives every newly achieved target in ascending order,
preserves earlier achievement evidence, and projects current deficits, the next
remaining amount and an exact interval fraction. Its result is an owned proposal;
calling a pure calculation does not record a durable achievement or release actions.

Score and configured targets use signed 64-bit MICRO-POINTS. One point is exactly
1,000,000 micro-points, following NUM-04. The full range
`[-9223372036854775808, 9223372036854775807]` is accepted, including zero, negative
score and negative/zero targets. No positivity rule, artificial minimum score,
implicit score reset or hidden starting origin is introduced. Targets must be
STRICTLY ascending. Equal targets are invalid even when their identities differ.

Upstream exact calculation/round-half-to-even has already produced each credited
effect and checked its aggregate score. This model does not parse money, apply a
delta, sum effects, round score again or recompute an old award under new rules.
Requested score from a captured plan is not a substitute for the actual resulting
authoritative balance. A caller must reject/hold an unrepresentable aggregate
under NUM-04 without committing any effects/jobs from that input. No overflowing
score may be wrapped/clamped into this boundary.

The caller binds that score's actual balance/target identity and this track's
definition revision to the same authoritative state used for application. This
model does not select among several balances or prove that mismatched snapshots
belong together merely because their opaque references are nonempty.

Core uses C++20 standard-library values and selected Boost.Multiprecision exact
arithmetic, with no new dependency. There is no Qt, network/provider, SQL, clock,
permission engine, action executor or UI dependency.

## 2. Track, completed evidence and owned proposal

The logical input has these distinct owned values:

| Value | Fields and meaning |
| --- | --- |
| Track context | `track_ref`, current `definition_revision_ref`, and the incoming `progression_context_ref` |
| Target list | Ordered entries with unique `milestone_ref` and current signed64 `target_micro_points` |
| Completed prefix | Ordered immutable records described below, matching exactly the first k target identities |
| Current score | One signed64 `score_micro_points`, already resolved by the authoritative caller |

References are nonempty opaque byte strings, each at most 256 bytes. Compare their
bytes exactly; no case/Unicode normalization, provider registry or lexical identity
inference is performed. Milestone identities are unique within the track, distinct
from their editable target values or positions. Different tracks may reuse a local
milestone identity. The current definition revision must resolve to the caller's
retained ordered configuration. The progression context resolves to its retained
application/command/preview evidence; it is not permission or proof of a commit.
The pure model neither generates random identities nor consults wall-clock time.

Each completed-prefix record owns:

- milestone_ref;
- definition_revision_ref at its completion;
- captured target_micro_points at that completion;
- actual score_micro_points at that completion;
- completion_context_ref identifying its retained source command/application.

No source/provider timestamp, event identity, financial state or actor is invented
from those references. Full action/audit/valuation/event evidence remains separately
retained and resolvable under CONTRACT-002. The record is a small progression
projection, not a replacement for the owning audit history.

Completed-prefix length k is between zero and target count inclusive; its identities
must equal the first k target identities in order, exactly once. A missing earlier
completion followed by a later one is invalid. Do not reorder, deduplicate, truncate
or repair it. Empty tracks require an empty completed prefix. A supplied prefix is
DATA: the caller must establish its authority and retained history independently.
The helper cannot detect an unauthorized omitted completion from an unseen parent.

Captured historical target/score/revision/context fields remain byte/value identical
in every returned proposal. They need not equal CURRENT definitions or score.
Manual completion can have actual score below its captured target; permitted target
corrections can leave captured targets different from current ones. Historical
captured target values therefore need not themselves form a newly sorted sequence.
Validate the current target order and active prefix identities, not a fabricated
historical score sufficiency/positivity rule.

The result owns the unchanged track/current-definition context and score, a complete
proposed completed prefix, its ordered newly achieved suffix, and the projection
in §4. The suffix can be identified by start/count into the result's own prefix;
no borrow into caller input is needed. Concrete C++ types/field walks/lifetimes are
the separately reviewed API stage. All actual returned records/strings are owned;
later input mutation/destruction or another call cannot change an earlier result.

## 3. Immediate ordered achievement proposal

Starting at index k, append each consecutive current target whose value is LESS
THAN OR EQUAL TO the supplied score. Stop at the first strictly higher target or
the end. Ascending targets make this exactly the newly achieved suffix; there is
no sorting, provider selection or policy decision during the calculation.

Each appended record captures that milestone identity, the current definition
revision/target, the SAME actual supplied post-effect score and the supplied
progression context as its completion context. A jump to 2500 through 1000/2000
records both achievements with actual score 2500. Never invent intermediate
observed scores 1000/2000, timestamps or partially applied source effects merely
to mimic overlay animation. Return the complete ordered proposal or reject it;
no fitting prefix may escape a resource/validation failure.

Already-completed prefix entries are never appended again, removed, reopened or
rewritten. A refund may lower score below any number of them; recovery changes
deficits/progress without re-earning those achievements or repeating their rewards.
Reaching a genuinely unfinished later target still appends it normally. Operation
deduplication, state-revision conflict checks and the source's actual entitlement
remain caller obligations; a context reference alone proves none of them.

On actual application, CONTRACT-002 requires one transaction to commit the score,
all these achievements, application/result/audit and required jobs together, then
publish committed state. No action/presentation delay postpones achievement
recording. This pure proposal supplies no writer or such transaction, and does not
claim that the current application-service/state interfaces already invoke it.

This is one score observation, not an unordered batch of effects. If the owning
application produces several ordered authoritative score observations, compose the
proposed prefixes in that order and commit them with the whole application. Do not
invent intermediate score samples or assume a final net score reproduces every
earlier observation. Defining that application's actual effect/observation sequence
and rejecting its entire input on a later failure remain integration obligations.
Nothing in this helper authorizes publishing an intermediate proposal.

## 4. Deficit, next remaining and exact progress

Projection uses the proposed completed prefix AFTER the append step. For every
completed identity i, let Ti be its CURRENT configured target, and let S be current
score. Its current deficit is `max(Ti - S, 0)`. Return those projections in prefix
order, separately from immutable completion records. The track deficit is the
last completed milestone's deficit, or zero when none is completed. It is the
largest current deficit, not the sum of overlapping deficits.

Let P be the current target of the last completed milestone, when one exists;
let N be the first unfinished target, when one exists. In particular, P is not
silently taken from an older captured target after an authorized target correction.
Such a correction can change current progress/deficit without changing completion
evidence or replaying actions. Pending-action refund decisions still need their
captured group definitions and separate policy; this projection grants no hold/
release decision.

| State after progression | Previous / next | Remaining and progress |
| --- | --- | --- |
| No configured targets | Both absent | Deficit 0; next remaining absent; interval fraction absent |
| Before the first achievement | P absent; N first target | Deficit 0; exact remaining N−S; interval fraction absent |
| At least one completed, one unfinished | P and N present, P<N | Remaining N−S; exact interval fraction below |
| All configured targets completed, nonempty track | P last target; N absent | Deficit may remain positive; next remaining and interval fraction absent |

These states distinguish an empty track from a nonempty completed track. No absent
next target is fabricated as zero. All-completed does not mean zero deficit or
that actions have finished. This helper's next target is not the action group
currently displayed during a multi-achievement animation.

For P/N present, use exact arithmetic:

~~~text
span = N - P                         (strictly positive)
earned = max(S - P, 0)
interval_fraction = earned / span    (reduced rational)
~~~

After progression S<N, so `0 <= earned < span`. Below P, deficit is positive and
progress is exactly zero; at P deficit/progress are zero. At N, achievement is
recorded FIRST, so projection moves to the following interval or all-completed.
Do not leave a completed next target at 100% awaiting its presentation/actions.
Display percentages/rounding are separate; for example fraction 1/3 remains exact
and cannot alter score/accounting.

The specification supplies the interval between a PREVIOUS completed milestone
and the next, but supplies no numeric origin before any completion. The explicit
engineering choice here is an absent first-interval fraction, with the first target
and exact remaining amount still available. It is not zero progress, an invented
previous milestone at 0, a configurable-origin feature or a restriction to positive
first targets. A later presentation requirement for a numeric first-interval origin
needs an explicit reviewed rule; callers must not silently infer one. First-target
achievement itself is fully defined for every signed target/score.

### Exact distance and overflow boundary

Every nonnegative difference of two signed64 scores/targets is at most
`18446744073709551615 = 2^64−1`. Deficits, remaining, span and earned must support
that COMPLETE unsigned64 magnitude. Widen/check before subtraction; never negate
INT64_MIN or subtract in signed64 and inspect an already overflowing result.
Using selected Boost integers for these bounded temporaries is permitted; standard
checked unsigned arithmetic also suffices. Any signed difference intermediate
needs at most 65 signed bits. Ratio numerator/denominator have at most 64 bits,
including before reduction. No unbounded arithmetic chain or floating point occurs.

A valid score/target pair at opposite signed extremes is not an overflow error
merely because its display distance does not fit signed64. Distances are derived
projections, not additional credited score or SQLite signed64 balance fields.
Future API/transport/storage must preserve that magnitude without a narrowing cast;
this document defines no new SQL column or wire encoding. Values outside the agreed
signed64 score/target domain reject before calculation; they are never coerced.
An upstream aggregate overflow still invokes NUM-04 with no partial application.

## 5. Local validation and resource policy

Proposed local engineering ceilings, not score maxima or provider capabilities:

| Resource | Maximum |
| --- | ---: |
| Each opaque track/definition/milestone/context reference |256 bytes|
| Configured targets / active completed-prefix records |4096 each|
| Recursive collection items in a complete input/result |16384|
| Owned logical bytes in a complete input/result |8MiB|

Count each actual owned string's bytes, each signed64/unsigned64 scalar as 8,
each boolean/tag as 1 and each Exact as its canonical signed numerator/positive
denominator decimal bytes. An absent optional contributes 0, a present optional its
value. Collections count their actual elements recursively; no allocator/padding
or global lifetime-history quota is implied. An index/range identifying the new
suffix does not charge a second copy of its records. Any real copied target/record
inside a projection DOES count. The API stage must freeze exact field walks,
exception/error priority and scratch bounds before tests/interfaces are authored.

Check counts/reference byte ceilings, checked total items/bytes and signed numeric
domains before materializing output copies. Then validate nonempty references,
unique identities, strictly ascending current targets and completed-prefix shape.
Project the complete result before growing its owning collections. Fail the entire
proposal on invalid input or resource refusal, preserving caller data and previous
results. No truncation, partial completion or automatic removal of audit history is
allowed to fit a limit. Global durable reference uniqueness/resolution and older
inactive reset history remain caller/store retention obligations.

## 6. Independent literal examples

The ordinary rows below use POINTS for readability; each supplied score/target is
exactly that number times 1,000,000 micro-points. Rows explicitly marked micro-points
use raw integer units. A/B/C denote distinct stable milestone identities. Completion
records/context references are retained independently; no provider evidence is
implied. Fractions/distances are manual arithmetic, not generated feature output.

| ID | Input / prior completed prefix | Required proposal and projection |
| --- | --- | --- |
| MP01 | Empty track, score −5, empty prefix | No new achievements; empty state; no previous/next/remaining/fraction; deficit 0 |
| MP02 | Targets A=1000/B=2000, score 800, no prefix | None new; next A; remaining 200; no previous/fraction; deficit 0 |
| MP03 | Same targets, score 1000, no prefix | Append A at actual score 1000; previous 1000, next 2000, remaining 1000, fraction 0 |
| MP04 | Same targets, A complete, score 1500 | None new; deficit 0, remaining 500, fraction 1/2 |
| MP05 | Same targets, A complete; 1100−300 gives score 800 | A remains byte/value identical; deficit 200, remaining 1200, fraction 0 |
| MP06 | Recover score 800→1000 with A complete | No new A/reward; deficit 0, remaining 1000, fraction 0 |
| MP07 | Then score 1100 with A complete | None new; remaining 900, fraction 1/10 |
| MP08 | A=1000/B=2000/C=3000, no prefix, score 2500 | Append A then B, BOTH with actual score 2500; previous 2000/next 3000, remaining 500, fraction 1/2 |
| MP09 | Same targets, A complete, score 3500 | Append B then C, preserve A; all-completed; previous 3000; next/remaining/fraction absent |
| MP10 | Same targets, A/B complete, refund to score 800 | No new rows; A deficit 200, B/track deficit 1200; next C, remaining 2200, fraction 0 |
| MP11 | A=1000/B=2000, A manually complete at actual score −50 | Accept existing prefix/evidence without adding points; deficit 1050, remaining 2050, fraction 0 |
| MP12 | Targets A=−100/B=0/C=100, score −50, no prefix | Append A at actual score −50; next B, remaining 50, fraction 1/2 |
| MP13 | Targets A=0/B=100, score 0, no prefix | Append A; zero target is not disabled; next B, remaining 100, fraction 0 |
| MP14 | MICRO-POINT targets A=INT64_MIN/B=INT64_MAX, score 0, no prefix | Append A; span 18446744073709551615, earned 9223372036854775808, fraction 9223372036854775808/18446744073709551615; remaining 9223372036854775807 |
| MP15 | MICRO-POINT target A=INT64_MAX already complete; score INT64_MIN | Retain A; all-completed with deficit 18446744073709551615; no next/remaining/fraction; no false signed overflow |
| MP16 | MICRO-POINT first target INT64_MAX, no prefix, score INT64_MIN | None new; remaining 18446744073709551615; previous/fraction absent |
| MP17 | A completed under captured target 1000; accepted current revision A=1200/B=2000; score 1100 | Historical target 1000 retained; current previous 1200, deficit 100, remaining 900, fraction 0; no replay |
| MP18 | MICRO-POINT targets A=0/B=3, A complete, score 1 | Remaining 2; exact fraction 1/3, no intermediate/display rounding |
| MP19 | Duplicate identity; targets 1000/1000; descending targets; prefix B without A; prefix longer than targets; empty required reference | Reject respective malformed input, no repair/partial proposal |
| MP20 | 4097 targets or a 257-byte reference | Resource refusal before output copying; no fitting prefix or discarded history |
| MP21 | First target A=−100, next B=0, score −150, no prefix | None new; next A, remaining 50; previous/fraction absent; no hidden zero origin |
| MP22 | Sole target A=0, score 0, no prefix | Append A; all-completed, deficit 0; next/remaining/fraction absent |

## 7. Adjacent mandatory operations and next gates

MILESTONE-002 owns authorized target edits/completed-target corrections, preview,
revision conflicts, original/revised audit and action choices for newly achieved
targets. MILESTONE-003/004 own durable jobs, uncertain outcomes, sequential action
release and the independent server presentation timeline. MILESTONE-005 owns
Run/Defer/Skip, independent review holds and refund-before-start Continue/Review/
Skip policy, with Review default. MILESTONE-006 owns definition replacement and
group interruption without automatic effect rollback. MILESTONE-007 owns manual
completion permission/reason and preserving actual score. MILESTONE-008 owns the
exceptional reset of a suffix, score-below-reset-target check, history retention,
cancellation and separately authorized replay. No ordinary refund performs reset.

This calculation does not grant any of those authorities, choose pending actions,
cancel effects, clear holds, deliver chat, fabricate time or decide post-finish/
paused/recovery applicability. The caller must select an eligible current score
transition and retain all independent gates. Core's acceptance of a supplied
completed prefix cannot authorize a manual completion/reset or prove that old
performed actions are safe to repeat. Reset archive history remains retained even
when a different authorized active prefix is later supplied.

The next stages require different actual logical review, concrete API/ownership/
error/resource review, minimal explicit marker interfaces and inert build harness,
independent literal tests/review and an observed intended behavioral red. Only
then may production MILESTONE-001 implement this behavior, followed by separate
source review and relevant ordinary automatic acceptance. No new feature/provider/
platform test or executable behavior is claimed by this document.
