# CORE-008 timer editorial implementation handoff

2026-11-05, author `/root/access_reviewer` in its assigned CORE008 source-author
role; isolated `/tmp/subathon-core008-author`, `task/CORE-008-resumed-author`,
base57b977e. Original preserved claim3f81224 was refreshed with current main by
the coordinator; no prior implementation/agent was assumed active. Owns only
`core/src/timer_editorial_commands.cpp` and this note. No header, contract,
specification, assertion,44ED inventory, build/CI or shared progress file changes.

## Scope and original gates

Read root/core instructions, workflow/brief, complete logical/API/header,27-test
source and44ED inventory, owner decisions/specification§4, CORE002/005 contracts
and actual helper implementations. The [original reviewed baseline/red](coretest-008-handoff.md)
from integrated PR52 remains intact; no unchanged stub-red was replayed. This
implements only a pure command proposal, not trusted permission/clock loading,
actual warning UI, authoritative queue, persistence, deduplication or CAS.

All stage priorities follow the frozen API. Borrowed complete input B/string/step
preflight occurs before owned cloning, helper metadata or trace growth. Full-width
existing enum comparisons reject256/-1 instead of narrowing to u8. Snapshot,
command, access and observation validation complete in category order before
access/refusal/revision checks. Required refs and opaque optional-empty actor/reason
retain their declared meaning. Valueless payload counts only its discriminator
in preflight, then rejects as invalid_command without visiting nonexistent data.
Preview shape binds to expected revision; old-cap/current-cap consistency waits
until after actual matching revision, so stale/denied input gets the required
refusal rather than a dependent preview error.

Admitted observations use every supplied OLD-setting segment in order. Ready is
explicitly retained; Active/Finished use existing CORE002. No UTC subtraction,
Main+Grace sum or aggregate signed elapsed sum is used. One fixed batch per input
step is retained, including zero-event batches, with only count used slots meaningful
and unused slots left at MainExpired. Old hard-end/natural-expiry transitions
cannot be erased by an editorial change. After-observation review/refusal outcomes
retain the complete old observed state/trace rather than silently dropping clock facts.

Nonnegative deltas reuse CORE005 debt-first/current-cap planning and pass only
actual Main credit to CORE002, once. Above-cap Main is valid; debt repayment can
occur without rescue or new Main headroom. Negative deltas preserve signed minimum,
require explicit selected policy, use CORE005 and compose its actual balance through
CORE002 zero-elapsed ending behavior. An overflow returns numeric_overflow_review
with full request unapplied and no partial accepted balance; timer-only RequireReview
and missing policy remain distinct. Ready/Finished delta prerequisites are explicit.
Private ephemeral CORE005 reference fields both name this MANUAL command where
needed solely for numeric reuse; no financial original effect/entitlement is created
or returned, and helper owning metadata is released after numeric scalar import.

Cap attempts compare actual observed Main. Missing choice yields choice-required;
Keep accepts cap without reducing Main; Reduce accepts cap plus an audited exact
nonpositive Main correction, never adds time after countdown removed the excess;
Abort accepts neither. Warnings retain current old/proposed caps and reduction,
separately from any historical preview. Debt/Grace/pause facts are not cleared.
Active zero-Main reduction composes existing Immediate/Confirmation/Fresh/Shared
ending behavior. Ready/Finished cap edits retain their lifecycle without transitions.
Deadline edits zero-observe only observed Active state under the new deadline;
Ready/Finished are not started/reopened. The first-Start check returns resolved
Ready deadline-clear/blocked DATA with its prerequisite, not an actual Start.

## Resource and allocation strategy for different review

The first compute pass owns only fixed scalar/optional state/audit fields. It does
not construct the public DTO, clone strings/input or accumulate an output vector.
CORE002 temporaries have at most4 transitions; their fixed-array batch is bounded
before copying. No variable-size list caches every observation. Complete helper
request+result metadata is preflighted from operation length L: addition2L+56,
reversal4L+58, within checked derived8*reference_bytes+512 before helper reference
copies. It is private bounded scratch, not incorrectly charged to the selected
result byte ceiling or interpreted as financial provenance.

Limits validate positive representability and derived6N+4,22N,8R+512 before input
work. All count additions compare remaining allowance; validated22N proves17N
input-step and5N output-batch products representable. No new independent work/item
limit is invented. Actual input/output container max_size checks precede their
copies/reserve. Result projection starts with complete retained input B and adds
actual optional observed/after values, both traces, six-component adjustment and
warning, including every tag/fixed5-byte batch. Only after complete result B passes
does materialization create/copy the owning DTO; a second deterministic OLD-state
observation pass fills its preflighted batch vector. It does not reevaluate or
reapply the editorial effect. Failure during either pass destroys partial local
ownership and returns no proposal; input remains caller-owned. All evaluator-local
std::bad_alloc/std::length_error map to allocation-free fixed TimerEditorialFailure.
Physical allocator capacity/padding/RSS and unknown caller lifetimes are not falsely
reported as semantic bytes or solved by this primitive.

## Actual checks

Host Linux x86-64/GNU16.2.1/CMake4.4.5/Ninja1.13.2/GTest1.18.0/Python3.14.9.
No additional dependency/Boost SDK is used by this std-only module/helper lane.

```sh
cmake --preset timer-editorial-debug
cmake --build --preset timer-editorial-debug
ctest --preset timer-editorial-debug --no-tests=error \
  --output-junit /tmp/core008-author-native101.xml
clang-format --dry-run --Werror core/src/timer_editorial_commands.cpp
git diff --check
```

All exit0. Fresh18-step build/no warnings; full101/101PASS,0failures/disabled/skips,
CTest0.68s. Includes unchanged27 groups/all44ED,32 existing debt,39 existing
lifecycle and3 infrastructure cases. Discovery independently matches the exact27
fixture names/101 total. Author branch retains original intentional-red labels;
coordinator owns reviewed unconditional acceptance-CI promotion. This passing
implementation receipt does not relabel/replay the original FIRSTred or establish
any UI, clock host, persistent review or full timer capability. JUnit/hash and
actual source blob are recorded below from existing artifacts, never fabricated.

## Remaining gates

Different actual source/resource/error/allocation review, applicable integration,
selected ordinary Ubuntu27 acceptance, latest-head CI, metadata/rules/discussions
and normal focused PR merge remain. Coordinator owns build/CI/state/map/HTML/pins/
checkpoint/publication. Current account/whole-command authority and actual clock
segmentation/CAS, warning UI, API007 Start/absolute Main, CORE007 reopen/post-finish,
explicit debt correction, ACCT005/006 mixed/global review/continuation/financial
reversal, persistence/recovery/platform/provider/release gates remain. A refusal
proposal supplies no independent clock-only commit authority. No live provider,
chat/payment/deployment side effect or production state write was performed.

Actual source blob `45e817d55aab8fcba59bb9b04a63b94615f91bd0`. JUnit SHA256 `6c594700c475b022b6f64fd9c391c95196a97b4b2710e359b3b447f26a78bb75`; tests101/failures0/disabled0/skipped0.
