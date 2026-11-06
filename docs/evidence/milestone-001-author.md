# MILESTONE-001 implementation author handoff

Author: `/root/session_plan_review`, assigned implementation role `milestone_author`,
6 November 2026. Branch `task/MILESTONE-001-progression-author`, isolated worktree
`/tmp/subathon-milestone001-author`, from actual coordinator claim
`420d4847fc04e7c6c5e69b2a4fb7f641fbfd103a` on integrated main `e2cd5eb`.
Only `core/src/milestone_progression.cpp` and this author note change.
Implementation source Git blob: `d79b77cad26bf876fd4bd89038161d4fc0c1b809`.

## Contract and preserved baseline

Authority: specification v0.45 §8/MILE-01/NUM-04, the reviewed
[logical contract](../contracts/milestone-progression-v1.md) blob0115a5cc and
[API contract](../contracts/milestone-progression-api-v1.md) blob58b8e129.
Read root/core instructions, both full contracts, the complete53-test source and
literal fixture, plus actual logical/API/minimal-source/full-test/foundation/current
harness reviews. [Baseline handoff](milestonetest-001-handoff.md) and
[FIRST attribution](milestonetest-001-first-red-review.md) retain the original
53 genuine missing-behavior failures on clean c2bcb088 before this implementation.
Those completed reviews and historical FIRST red were not replayed or replaced.

Frozen public header9a167813, test019973844dceb289aa82ec8aabd3d9274faef98e,
fixture409be19d57b2cba4025e236a67a539af63b08be4 and both contracts remain exact.
No assertion, fixture, public interface, dependency pin, target, preset, test label,
workflow, schema or shared progress/map file changes. No new dependency is added.

## Implementation and allocation audit

The existing one raw-input operation now returns a complete owned proposal:
immutable captured prefix, consecutive newly achieved suffix, current deficits,
previous/next, remaining distance and reduced exact interval. All new completions
capture the same supplied actual score/current revision/context. It retains
historical targets/scores/revisions/contexts even when current definitions differ.
The first interval remains absent; equality advances before projection; empty and
all-completed remain distinct. There is no score aggregation, rounding or authority
lookup and no action/state/transport/provider/UI dependency.

The seven normative phases are explicit in source:

1. All five limit options are checked in declaration order, positive and no larger
   than their frozen defaults.
2. Input target/prefix counts and combined items, every reference byte ceiling in
   depth-first declaration order, then complete input bytes are checked before
   semantic reference checks or index creation.
3. Every required reference is checked nonempty in that same field order.
4. Duplicate IDs are detected in target order with one fixed array of4096 borrowed
   string_view slots; no owning key, heap index, target reorder or normalized ID.
   Exact bytes after NUL and high bytes participate in both hash and equality.
5. Adjacent current targets must increase by comparison, without subtraction.
6. Prefix count and ordered identities must match first-k current targets. Captured
   historical fields impose no fabricated score-sufficiency/order/authority rule.
7. Derive the full prefix and scalar projection, check selected output-prefix limit,
   complete output items and every output field/copy byte before constructing any
   output string/vector/Exact; only then materialize the complete result.

`ByteBudget` maintains used<=ceiling and tests each amount against ceiling-used,
so byte sums cannot overflow. Combined item checks likewise subtract only after
the first count fits. All output counts are bounded by4096; suffix size/index
conversion to uint64 is therefore representable. Output field accounting includes
each unchanged or new completion's three strings/two integers, every current
deficit target/integer, optional previous/next copies, suffix16, state1,
track-deficit8, optional remaining8 and interval16 plus its canonical ratio digits.
Input and result bytes/items are independent; no fitting partial prefix escapes.

The fixed borrowed-ID table has4096 slots and no dynamic allocation. All referenced
strings have been checked nonempty, so an empty slot is unambiguous. Before each
insert, fewer than4096 slots are occupied: input target count is at most4096.
Thus collision probing always reaches a free slot or equal prior ID. Target order
and opaque bytes are preserved. Numeric scratch is fixed uint64/size_t scalar
state, not speculative history or an output-sized arithmetic matrix.

Every distance first establishes high>=low by comparisons, converts both operands
to uint64 BEFORE subtraction, and obtains the exact unsigned mathematical distance
through UINT64_MAX. No signed addition/subtraction/negation can overflow, including
INT64_MIN. Between targets span>0 and earned<span. Unsigned gcd normalizes the
bounded components, including zero0/1; decimal digit counting uses division on
uint64 without allocated strings. Only after complete output preflight is the
returned Boost Exact constructed from those <=64-bit reduced components. There is
no arbitrary-size Exact input or unbounded arithmetic chain.

The call borrows immutable input and only mutates a new local result after all
validation/preflight. Every returned string/record is owned; prefix/deficit vector
reservations use the already checked complete counts. No catch changes bad_alloc
or unexpected programming errors into fake domain results. RAII discards a partially
materialized local on allocation failure, leaving caller input/prior results intact.
These are semantic resource budgets, not global allocator/CPU quotas.

## Actual commands and results

Fresh Linux build: GNU16.2.1, CMake4.4.5, Ninja1.13.2, Python3.14.9,
installed GoogleTest1.18.0. Configure reports authentic selected Boost1.83.0 at
`/tmp/subathon-boost-1.83-sdk/prefix/usr/include`; no Qt is required by this lane.

```sh
cmake --preset milestone-progression-debug -DSUBATHON_FETCH_GOOGLETEST=ON -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset milestone-progression-debug
ctest --preset milestone-progression-debug --show-only=json-v1
ctest --preset milestone-progression-debug -L '^milestone-progression$' --no-tests=error --output-on-failure --output-junit /tmp/subathon-milestone001-author-targeted.xml
ctest --preset milestone-progression-debug --no-tests=error --output-on-failure --output-junit /tmp/subathon-milestone001-author-full.xml
clang-format --dry-run --Werror core/src/milestone_progression.cpp
python3 tools/check_project.py
git diff --check
```

All exit0. Fresh configure/build completes18 steps with zero compiler warnings.
Parsed discovery has exactly90 total/53 milestone cases. First implementation
acceptance run: unchanged53/53 PASS, CTest0.10s, no failures/disabled/skips.
Full preset90/90 PASS0.72s, no failures/disabled/skips, including33 old numeric,
three infrastructure and the independently reviewed25-fact SDK foundation.
The label `intentional-red` is still historical baseline metadata in this author
branch; no exclusion was used in either acceptance/full command. Source format,
whitespace and project consistency pass422 tasks/122 decisions/current HTML.

Byte-safe subprocess capture retains stdout/stderr and native JUnit in
`/tmp/subathon-milestone001-author-{targeted,full}.{stdout,stderr,xml}`.
Discovery is `/tmp/subathon-milestone001-author-discovery.json`.
Native JUnit SHA256:

- targeted53: `d586a1c9e43e9c29a525b4002b86ff6adc9964e8b26e196a9d67f5ef903f238e`;
- full90: `5ea2d5b15bda08c3717ded4b696a27bf27fedf14e195e08492db7887a534ac72`.

## Remaining gates

Different actual source/configuration review remains required; this author does
not approve its own implementation. Root owns ordinary-CI promotion, final progress/
acceptance evidence, current selected Ubuntu execution, metadata review and normal
PR/check/rules/discussion/merge gates. No defect-driven regression was needed during
this initial implementation. If actual independent review finds an uncovered defect,
preserve the candidate and obtain separately authored/reviewed regression with an
observed behavior-red before its bounded fix; do not weaken the reviewed baseline.

This is a pure component, not authoritative score/history/application/queue/action
integration or manual/edit/reset permission. Those requirements remain in their
own tasks. No Qt/SQL/UI/provider/platform/package/live/OBS/recovery/full release
verification follows. All seven integrations, three FX providers, both renderers,
localization/themes/platforms and full agreed release scope remain required. Only
synthetic fixtures were used; no live operation or GitHub write occurred.
