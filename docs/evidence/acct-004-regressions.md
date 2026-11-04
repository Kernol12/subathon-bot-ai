# ACCT-004 additive source-review regressions

Author/coordinator root,2026-11-04, task/ACCT-004-application-service.
The existing ACCTTEST-004 baseline, all35 original test methods/224 scenarios,
independent fixtures and first stub-red remain unchanged. Eight additive slots
(nine scenarios) address actual source-review findings in first implementation722e11b,
importede4089bd, service blobc25beb6d03bb23f2edd47db9491659baedae05de.
These are reached behavior failures, distinct from the original missing-feature red.

Authority: [concrete contract](../contracts/application-service-api-v1.md) A/C/D/E/F,
[logical contract](../contracts/application-service-v1.md), specificationv0.45
sections3–5/NUM-04/B08. No requirement narrowing or existing assertion correction.
Root owns additive test methods, registration/count guards, raw evidence and shared
progress. Service author owns production corrections; codec author owns the private
retained-command decoder seam. Actual different reviewer service_reviewer approved
EVERY addition before its first invocation; [review](acct-004-code-review.md).

| Added test | Independent expected behavior |
| --- | --- |
| referenceViewRetainsSelectedKeyLimit | Current callback view selected reference limit1 survives as data after callback expiry; cfg rejected invalid_reference before expiry, valid c still expired/no authority |
| retainedStateBudgetPrecedesPolicy | Frozen supported SAS1 has1segment+2balances=3 actual items; limit2 fails before current policy, even though each vector alone fits |
| historicalBeforePolicyMatchesAudit | Only advanced-before B(policy) changes policy to policX; typed state/clock/after/audit unchanged, so retained_semantics contradictory_document/rollback before revaluation |
| retainedClockFinalUtcMatchesObservation | Only retained Clock observedUTC becomes1001 while last segment remains1000; malformed known document/rollback before resolver |
| historicalJobMeaningIsNotOnlyAnEcho | SAJ1 magic becomesXAJ1 in BOTH retained job and its exact SAR1 echo; matching duplicate bytes cannot legitimize malformed job grammar |
| explicitUnresolvedPolicyRecordsKnownHold | Bound admission unresolved_with_reason retains exact durable policy hold with known Policy/Clock/reason; existing frozen dActionsMissing annotation applies unchanged because Policy grammar omits admission enum |
| retainedCommandBoundIsNotMalformedData | New input clears choices to fit choice_bytes1; old valid six-byte stored choice exceeds selected internal bound and yields known result_limit, not malformed bytes |
| negativeCounterRequiresCorrectionPurpose | Two independently modified exact-1/1 selected requests, score/count, are valid old-store data; ordinary captured purpose fails existing correction gate before clock/arithmetic/actions, with no counter deficit policy invented |

## Actual chronology and commands

First complete implementation passed all35 unchanged slots and full300 before
source audit. Root full300PASS24.50s and author full300PASS24.66s are bounded
first-candidate results, NOT approval of the uncovered defects. Different reviewer
also independently passed original35 while withholding full approval.

Each additive test revision was compiled successfully with actual preset target;
no missing dependency, skip, compile failure or missing marker is a behavioral red.
Root ran the following selections ONCE each on the UNCHANGED production blob above:

```sh
ctest --preset application-service-debug -R '^application-service\.(referenceViewRetainsSelectedKeyLimit|retainedStateBudgetPrecedesPolicy|historicalBeforePolicyMatchesAudit|retainedClockFinalUtcMatchesObservation|historicalJobMeaningIsNotOnlyAnEcho)$' --verbose
ctest --preset application-service-debug -R '^application-service\.(explicitUnresolvedPolicyRecordsKnownHold|retainedCommandBoundIsNotMalformedData)$' --verbose
ctest --preset application-service-debug -R '^application-service\.negativeCounterRequiresCorrectionPurpose$' --verbose
```

- First5: reviewer approved CPP1b7d73ca/Qt a4993752/CI cee1d8ab; rootHEAD95fa152.
  Exit8, five exact behavior assertion failures,10initcleanupPASS,0skip/setup failure,
  0.16s. [Canonical per-invocation raw](acct-004-five-regression-red.txt).
- Next2: approved CPP8ab5dba0/Qt f5f08ab1/CI0700f15e; rootHEADba2e90f (only review
  evidence added). Exit8, wrong disposition and wrong bounded category,4initcleanup
  PASS/0skip/setup failure. [Canonical raw](acct-004-two-regression-red.txt).
- Last slot/two rows: approved CPP7329b24f/Qt8e9d85ee/CIc63ee177; same production
  and rootHEAD. Exit8, both exact before-clock ordering assertions fail;2initcleanup
  PASS/0skip/setup failure. [Canonical raw](acct-004-negative-regression-red.txt).

The reviewed additive files/guards were uncommitted while those genuine red commands
ran; exact blob pins above identify each actual revision. No clean-head claim is
made for these additional regressions. The resulting task commit freezes them and
the original raw logs without replaying the completed red. Original ACCTTEST-004
same-clean-head baseline and chronological proof remain untouched.

Native environment GNU16.2.1/Qt6.12/Boost1.92. Raw stdout/build logs and canonical
LastTest captures remain under ignored local-evidence. Assertions after each first
failing check remain unexecuted until corrections. Allocation ordering requires
actual source audit in addition to observable quota/call-order assertions; none of
these tests pretends to instrument allocation or rewrite writer transaction truth.

## Pending corrections and gates

All eight correction groups are released after actual different review/reached red.
Corrected source, private retained-codec helper, private cached-view limits, complete
preallocation projection and current-source semantics require actual re-review.
Final target has43 enabled slots/233scenarios/full308 registrations. Every case
must execute in ordinary selectedQt CI, with exact-registration guard and unchanged
old fixtures/dependency pins. No final pass/review/CI/merge is claimed by this note.
