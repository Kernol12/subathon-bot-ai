# MILESTONETEST-001 test author candidate

Author: service_author, 2026-11-04. Dedicated
`task/MILESTONETEST-001-contract-author`, isolated worktree
`local-evidence/worktrees/milestonetest001-contract-author`. The coordinator released
TEST AUTHORING ONLY in `4f8f3e1` after actual minimal source/harness approval
`a2f5d358` / integration `6d1fa3e` and corrected installed-GTest note `53bdf04`.
Inspected a clean tree, then normally merged actual prerequisite branch
`task/MILESTONETEST-001-progression-baseline` as `22ee34d`.

Owns only [literal vectors](../../tests/fixtures/milestone_progression_vectors.hpp),
[feature tests](../../tests/acceptance/milestone_progression_tests.cpp),
[primitive foundation](../../tests/acceptance/milestone_progression_environment.cpp)
and this note. Root owns CMake/presets/workflows/pins/state/HTML/maps/JSON/checkpoint.
No approved assertion, contract, header/stub, production algorithm, schema or shared
build configuration is changed. Root/tests instructions were read before editing.

## Exact source and proof status

| New artifact | Git blob |
| --- | --- |
| milestone_progression_vectors.hpp |409be19d57b2cba4025e236a67a539af63b08be4|
| milestone_progression_tests.cpp |019973844dceb289aa82ec8aabd3d9274faef98e|
| milestone_progression_environment.cpp |7f8c3f9466a0e3b313aa7e3f492b7ee82b91513d|

Frozen model/API/header/TU remain respectively `0115a5cc`, `58b8e129`, `9a167813`
and `4e3291f0`. The new operation remains its single immediate missing-feature
throw. No test body, new foundation main/require body, feature, marker, provider,
action or authoritative-state operation was invoked. Compilation/discovery are not
green feature acceptance, a green native foundation or an intended red baseline.
Actual different test/foundation/harness review must precede first invocation.

## Independent literals and requirement map

The fixture uses standard-library PODs, constexpr arrays/spans and explicit signed
micro-point values. It includes no project header or calculator. A label-only
PrintTo supplies stable discovery/failure metadata, not pointer dumps. Actual inputs
are constructed by copying literal fields into the public raw aggregates; expected
history/projection records come from independent fixture literals, never the SUT.
Fractions use literal unsigned numerator/denominator constants and selected Boost
construction, not project parsing/progression helpers or floating-point math.

Twenty parameter instances cover MP01–18 and MP21–22, inspecting EVERY returned
context/score/history/range/projection field and unchanged caller input. Current
deficit targets and immutable captured target/score/revision/context are checked
separately. Each jump records the actual same post-effect score for every new row.
All optional presence states and exact normalized fraction components are asserted.

| Authority / model examples | Actual test home |
| --- | --- |
| §8 empty/first/threshold/last states; MP01–03/13/21/22 | Named IndependentMP rows, no first-origin invention or fabricated next target |
| §8 ordered crossing; MP08/09 | Full literal new/history suffix, order, source capture, new begin/count |
| §8 reversal/deficit recovery/no repeat; MP04–07/10/11 | Literal score 800 below 1000, deficit 200, next 2000 with remaining 1200/progress 0; individual deficits versus track maximum; legitimate manual-completed prefix |
| Signed/zero targets and NUM-04; MP12–16/18 | Exact negative/zero/fraction/extreme rows plus two explicit signed endpoint cases |
| Current versus captured corrections; MP17 | Current threshold projection retains original completion evidence |
| Malformed order/prefix/reference; MP19 | Six validation cases, three identity cases and first-failure table |
| Resource/preflight assumptions; MP20 and API limits | Eighteen resource cases, exact/one-over profiles and invalid option domains |
| Owned results/input preservation | Four ownership cases plus table/error-helper input snapshots |

The 33 focused cases cover embedded NUL/high bytes and numeric rather than ID order;
full signed endpoint achievement; every required reference; duplicate/nonascending/
skipped/reversed/long prefixes; nonascending historical captures/manual deficits;
copy/move/input destruction/later-result/refusal independence; and all validation
phases in caller-labelled rows. Wrong exceptions cause NONFATAL failures with their
actual dynamic type/message; the marker is never accepted as MilestoneProgressionFailure
or caught as success. EXPECT_NO_THROW success rows likewise fail if the
marker appears. Table failures retain caller SCOPED_TRACE labels and continue to
later rows. No skips, failure inversion, WILL_FAIL or fake successful output exists.

Large resource input builders only synthesize bounded input IDs/ascending integers;
their expected counts and sampled outputs are literal. They do not calculate an
expected progression. All original captured rows are also compared unchanged.
The count/reference ceilings bound actual values below the default 16384-item and
8MiB ceilings; those upper option domains are tested at max/max+1, while reachable
actual resource boundaries use LOWER profiles. Input/output items are tested at
3/2 and 4/3, input bytes at 56/55, empty output 36/35 and full interval 118/117. Smaller
reference/target/completion ceilings and exact 256/257-byte, 4096/4097-row boundaries
are exercised. No artificial giant row bypasses an earlier bound to pretend the
default global maxima are reachable.

Runtime tests check acceptance/refusal and all-or-none results, NOT allocation
ordering. The later production source audit must separately verify before-copy/
before-growth complete output projections, scratch bounds and checked arithmetic.

## Independent old-primitive foundation

The standalone foundation has its own main, includes only std/selected Boost and
links no project library/header/fixture/symbol. Its 25 literal require calls cover
signed64 width/extrema, widening before subtraction/negation, full unsigned64
conversion/decimal spelling, unsigned gcd, direct unsigned64 rational construction,
normalized full-width/third/tenth/zero fractions, literal deficit/remaining distances
and independently stated byte/item sums. It does not call or certify progression.
It is COMPILED ONLY; no fact's runtime success is claimed. Different actual review
and selected-environment GREEN invocation remain mandatory before FIRST feature red.

## Actual compile/list checks

Linux x86-64, GNU C++ 16.2.1/C++20, host Boost 1.92 and installed GoogleTest 1.18.0,
clang-format 23.1.4. No dependency was installed/downloaded. Exact commands:

```sh
clang-format -i tests/fixtures/milestone_progression_vectors.hpp tests/acceptance/milestone_progression_tests.cpp tests/acceptance/milestone_progression_environment.cpp
c++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Icore/include tests/acceptance/milestone_progression_tests.cpp core/src/milestone_progression.cpp -lgtest_main -lgtest -pthread -o /tmp/subathon-milestone-progression-tests
c++ -std=c++20 -Wall -Wextra -Wpedantic -Werror tests/acceptance/milestone_progression_environment.cpp -o /tmp/subathon-milestone-progression-environment
/tmp/subathon-milestone-progression-tests --gtest_list_tests
```

All exited 0. Compile/link emitted no warnings. LIST ONLY discovered exactly 53
unique cases: 20 literal instances + 33 focused cases. GoogleTest registration,
parameter naming/printing metadata ran; no test setup/body or new feature ran.
No CTest registration/full-preset execution is claimed: root still owns that harness.
Saved list `/tmp/subathon-milestone-progression-list.txt` SHA256
`2f26d0857cdcc11b83eac203c1fc42760207a77eccdf85253662344c18f1572d`.

`nm -C /tmp/subathon-milestone-progression-environment` and source include inspection
confirm no subathon symbols/project includes. Read-only inspection counts 25 require
calls; the foundation program itself was NOT run. Repository clang-format dry-run,
`python3 tools/check_project.py` (396 task records/122 decisions),
`python3 tools/progress.py render --check` and whitespace checks pass. Staged
four-owned-path verification accompanies commit. No selected Boost 1.83 CI, Windows,
feature acceptance or FIRST behavioral red is claimed.

## Actual compiled case inventory

```text
MilestoneIdentity.PreservesOpaqueNulAndHighBytesWithoutSorting
MilestoneIdentity.BytesAfterNulDistinguishMilestones
MilestoneIdentity.ExactOpaqueDuplicateIsRejected
MilestoneEndpoints.MinimumScoreEqualsMinimumTarget
MilestoneEndpoints.MaximumScoreCrossesBothSignedExtremes
MilestoneValidation.EveryRequiredReferenceMustBeNonempty
MilestoneValidation.DuplicateIdsAreRejectedEvenWithDifferentTargets
MilestoneValidation.CurrentTargetsMustBeStrictlyAscending
MilestoneValidation.CompletedIdentitiesMustBeExactlyTheFirstK
MilestoneValidation.HistoricalTargetsAndScoresNeedNotBeAscendingOrSufficient
MilestoneValidation.FirstFailureOrderAppliesAcrossAllInputAndOutputPhases
MilestoneResources.EveryReferenceAccepts256Bytes
MilestoneResources.EveryReferenceRejects257Bytes
MilestoneResources.ReferenceBudgetCountsBytesAfterNul
MilestoneResources.SmallerReferenceCeilingAcceptsExactAndRefusesOneOver
MilestoneResources.SmallerTargetCeilingAcceptsExactAndRefusesOneOver
MilestoneResources.Accepts4096TargetsWithoutFabricatingInitialProgress
MilestoneResources.Rejects4097Targets
MilestoneResources.Accepts4096CompletedRowsWithoutRepeatingThem
MilestoneResources.Rejects4097CompletedRowsBeforePrefixShape
MilestoneResources.SmallerCompletedCeilingAcceptsExactOutputAndRefusesOneOver
MilestoneResources.SmallerCompletedCeilingRefusesInputOneOver
MilestoneResources.ItemBudgetMeasuresInputIndependently
MilestoneResources.ItemBudgetMeasuresWholeOutputBeforeGrowth
MilestoneResources.ByteBudgetMeasuresExactInput56AndOneOver
MilestoneResources.ByteBudgetMeasuresExactEmptyOutput36AndOneOver
MilestoneResources.ByteBudgetIncludesAllIntervalCopiesAt118
MilestoneResources.DeclaredMaximumOptionsAreAccepted
MilestoneResources.EveryOptionRejectsZeroMaxPlusOneAndSizeMaximum
MilestoneOwnership.RetainsAllFieldsAfterInputMutationAndDestruction
MilestoneOwnership.LaterProposalDoesNotRewriteEarlierResult
MilestoneOwnership.CopyAndMoveKeepOwnedPrefixRangeAndProjection
MilestoneOwnership.RefusalDoesNotChangeEarlierResult
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP01_Empty
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP02_BeforeFirst
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP03_ExactThreshold
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP04_IntervalHalf
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP05_RefundDeficit
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP06_RecoveryAtPrevious
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP07_RecoveryAbovePrevious
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP08_OrderedJump
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP09_JumpPreservesOldPrefix
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP10_MultipleDeficitsAreNotSummed
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP11_ManualCompletionWithDeficit
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP12_NegativeTargets
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP13_ZeroTarget
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP14_FullWidthInterval
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP15_FullWidthCompletedDeficit
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP16_FullWidthFirstRemaining
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP17_CurrentVersusCapturedTarget
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP18_ExactThird
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP21_BelowNegativeFirstTarget
IndependentMP/MilestoneVectors.MatchesIndependentLiteral/MP22_SoleZeroTarget
```

Next gates are actual different test/foundation/harness review, selected green setup,
FIRST intended marker-red and separate attribution. Only then may production
MILESTONE-001 implement the reviewed behavior. Root owns publication/PR/CI/state
and releases subsequent work. This author makes no push, PR or main merge.
