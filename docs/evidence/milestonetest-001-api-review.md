# MILESTONETEST-001 independent concrete API document review

Reviewer: codec_author, different from author service_author. Date: 2026-11-04.
Ownership is only this new note on `task/MILESTONETEST-001-contract-review`.
Root/core instructions and the contract-first review partition apply.

Reviewed author `c89776c5c07252339e1cd85f8c627c3ffaee7b24`, imported by root as
`9d8ba9ec5bebcf512648be274a6a78dc87372b28`. After confirming the review tree was
clean, merged `task/MILESTONETEST-001-progression-baseline` normally as
`6c1b522c4573345794dc23660db8fffb2de53dc0`. Prior logical approval `8338981`
remains unchanged. Exact inspected artifacts:

| Artifact | Git blob |
| --- | --- |
| [API proposal](../contracts/milestone-progression-api-v1.md) | `58b8e129815c0525176cc6c1b6830a7e6f0a9fdb` |
| [API author evidence](milestonetest-001-api-author.md) | `9a9f3e5905565e3d51947e00abfcebf6ba5716d4` |
| [Frozen logical model](../contracts/milestone-progression-v1.md) | `0115a5cc3cdc4fb1ebb8c9739716a27cb3379933` |
| [Logical author evidence](milestonetest-001-contract-author.md) | `67593a9ebabafc44976fb1c5f4260f2951b11baa` |
| [Prior logical review](milestonetest-001-contract-review.md) | `f163cac8fc582a4fa4ad300fbc7fe30cffe84b79` |

Authority remains specification v0.45 §8/MILE-01/NUM-04, MILESTONE-001,
event/state ownership, exact-decimal revision 1/public types and the accepted
logical review. The API does not amend those frozen semantics.

## Verdict

**Approved for the concrete API document stage.** No substantive defect,
requirement conflict or additional owner decision was found. This verdict covers
only the actual API document and author evidence above. It does not approve a
future header, source, stub, harness, test baseline, first red or implementation.
MILESTONETEST-001 remains incomplete; MILESTONE-001 remains planned.

## Direct input and immutable ownership

One raw-input operation is sufficient for both progression and preview. Ordinary
owned aggregates with literal target/completion data reach its future stub without
first calling another unimplemented feature. There is no public validated-result
factory, test-only constructor or caller-supplied validity/authority flag.

The result owns exactly its data aggregate. Private construction, public defaulted
copy/move and const noexcept data access preserve the logical value boundary;
borrow lifetime and caller copy/assignment synchronization are explicit. No result
field refers to caller storage. A suffix start/count indexes its own full prefix,
avoiding a second vector while retaining every captured completion field unchanged.

Projection uses current target definitions, separately from immutable historical
targets/scores/revisions/contexts. Each new record captures the same actual supplied
score. Manual/corrected history need not satisfy current target values or historical
threshold ordering. Structural acceptance proves neither manual/reset authority
nor that an old completion omitted from an unseen parent can be forgotten.

## Numeric and projection semantics

All current and captured scores/targets use the full signed64 domain. The boundary
requires an already authoritative score; it neither calculates score plus delta
nor repeats parsing/final rounding. Earlier conversion from untrusted numbers
must be checked by its owning boundary. A typed field cannot recover information
already narrowed before the call; no fake checked-caller certificate is added.

Distances use uint64 through UINT64_MAX, and comparisons establish direction before
widened subtraction. At most 65 signed bits suffice for a difference; a valid
opposite-extreme pair does not produce a false signed overflow. Between targets,
span is positive and earned is smaller than span. Unsigned gcd normalization gives
a reduced exact fraction, including zero as 0/1. Its components need at most 64
bits/20 decimal digits each; complete fraction size is measurable before returning
an owned Exact. There is no unbounded Exact input or new numeric dependency.

Empty, before-first, between and all-completed states match the logical model.
Previous/next are actual current target copies; deficits are per-completed values
with the last as track maximum. Remaining is present and positive only when next
exists. Interval exists only with both previous and next; it stays absent before
the first completion and after all completion. Equality advances first; no next
target remains at 100% waiting for action/presentation execution.

## Complete budgets and validation order

The field walks count each actual string/scalar/Exact and repeated owning copy:
captured records, current deficit targets, optional previous/next and repeated
revision/context strings. Suffix scalars add 16 bytes without a second collection.
The result omits a hidden full raw-input/target-list copy. Recursive items are
exactly input targets plus prefix, and output prefix plus deficit entries.

Input and result budgets apply independently, including the selected complete
output prefix ceiling. Lowered byte/item/count limits are meaningful even when
default aggregate ceilings exceed reachable combinations of other maxima.
Input counts/reference lengths precede indexing or copies. Borrowed scratch is
bounded at 4096 records. The complete prospective result, including normalized
fraction size, must fit before any output vector/string/Exact growth. Input/caller
copies and allocator overhead remain honestly outside the semantic field metric.

The seven phases are unambiguous: all options; complete input resource preflight;
all required references; duplicate target IDs; ascending current thresholds;
prefix shape; complete output preflight/materialization. This preserves later
resource failures ahead of earlier empty references, duplicate/order/prefix
precedence, and legitimate captured history. References remain opaque data.
Exact fixed error text leaks no input. Allocation failure remains bad_alloc and
does not synthesize a valid projection or malformed-input classification.

Independently checked 16 standard-Python integer/Fraction facts, with no project
import/model implementation. The empty input/result are 11/36 bytes. The listed
two-target input/result are 50/118 bytes, projection 70, interval 18 and items 3/2.
Two newly completed targets require four result collection items. Extreme distance
is 18446744073709551615; the MP14 reduced ratio is
9223372036854775808/18446744073709551615, with 64-bit components. Zero normalizes
to 0/1 and a bounded fraction needs at most 40 canonical numerator/denominator
bytes. All assertions pass. These are document checks, not feature/test execution
or implementation-generated expected values.

## Inert source and remaining gates

The future single stub must immediately throw the distinct
MilestoneProgressionNotImplemented with the exact documented text. No validation,
successful value, helper/parser call or arithmetic may precede it. Type/accessor/
exception definitions may be inert. Build/discovery must not invoke the entrypoint
or marker, and no generated expected value/data provider may call it for setup.
Selected std/Boost literal foundation checks are separate from the model.

Actual minimal-source/harness review, independent literal test authorship/review,
selected green foundation and real named-marker behavioral red remain future
gates before production. Skips, compile/dependency/setup failures, marker-as-pass
catches and exit inversion cannot substitute. Edit/manual/reset authority, action
release/presentation/holds, integration/atomic commit, UI and platform work retain
their required separate owners. No prospective source/test approval is given here.

## Actual checks

- Exact Git-object inspection and clean normal prerequisite merge: done.
- Read-only standard-Python integer/Fraction audit: exit 0, 16 assertions pass.
- `python3 tools/check_project.py`: exit 0; 396 task records, 122 decisions and
  valid local links/presets/current HTML in this isolated review tree.
- `python3 tools/progress.py render --check`: exit 0, current snapshot.
- Final staged-path/whitespace checks accompany this note's commit.

No feature source/harness/test, native executable, provider operation or model
entrypoint was created or invoked. Only this review note is authored; frozen
logical/API documents and prior reviews remain intact. Root owns shared state,
build/CI/checkpoint, publication and subsequent stage releases.
