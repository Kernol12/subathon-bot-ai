# ACCTCONTRACT-001 — independent application-service logical review

Author: bundle_implementer; planning/integration author: root coordinator.
Reviewer: bundle_reviewer, separate agent and new isolated
`task/ACCTTEST-004-contract-review` worktree based on integrated main8c7f71f.
Date: 2026-11-03. Verdict: **approved for the exact logical DOCUMENT contract and
prerequisite decomposition below**. This is not API/schema/test-baseline/red/source
or provider/platform approval. No immediate owner decision/access is required for
this document task; explicitly unresolved policy paths remain mandatory later gates.

## Actual reviewed revision and scope

Read initial9614dd4, requested changes, then actual17c0b22 and finalc6052e8.
Root froze the actual final document and planning as clean e0affbd. Review covered
the complete412-line logical contract, author evidence, ADR-0003, ACCTCONTRACT-001
brief, concrete planned hold016/claim017 briefs, revised service prerequisites,
epic/index references, state dependency graph, and CORE-008/ACCT-005/ACCT-006
follow-up briefs/notes. Actual root files, not hypothetical future artifacts, were
used for this review; reviewer owns only this evidence file.

| Artifact | Exact Git blob |
| --- | --- |
| application-service-v1.md |6d0cd67f487dae92a17d41eb07dda5b407c78a56 |
| Author evidence |8f87e0f3d6d4f318c001b0f453bb6d46d9f7a31d |
| ADR-0003 |30353c8ebcdc68845aed3422a88a9a26614bac41 |
| ACCTCONTRACT-001 brief |b7625f7da3f54e2437f4b56a975bd45814cbc03f |
| STORAGE-016 brief |377856ee57e76b6c9e93e8ebb0662ae3b00b9189 |
| STORAGE-017 brief |3857bfb25d2865dd1c31b334f128055c5e60e1e3 |

Authority examined: specification v0.45 §§3–5/NUM-01–04/DB-01, existing event/state
boundaries, CORE-005 debt/deficit and CORE-002 lifecycle contracts, current independent
contribution conditions, captured plans, accepted logical/concrete bundle and writer
transaction contracts. Existing implementations/ports were inspected read-only to
check the actual two prerequisite gaps; no old behavior/test was changed or replayed.

## Material findings and actual corrections

Initial961 inferred that timer RequireReview left the COMPLETE atomic operation
pending with no other fitting effects. Existing CORE-005 and bundle AT09 expressly
state that timer-only numeric hold does not select a global application/review
result. That inference was not an agreed default. Actual17 removes it: whole-input
overflow still permits no subset, whereas timer-only deficit keeps actual0/full
unapplied provenance and requires an explicitly reviewed mixed-result/global-status/
authorized-continuation policy. No default between recording a complete bundle with
a timer hold and whole-input deferral is selected. Unknown mapping stays not-ready.
Actualc605 fixes residual port/protocol shorthand that otherwise contradicted this
separation. CORE-008/ACCT-005/ACCT-006 explicitly retain their respective unresolved
original/manual/financial mapping and continuation obligations. A later policy may
need an owner decision, but this document neither guesses it nor needs it settled
to describe the bounded prerequisite work.

Initial ordering also allowed a paused admission hold to skip the hard-end observation
asserted by AS16, and an unconditional reducer call could admit Ready incorrectly.
Actual17 classifies lifecycle first, never passes Ready to CORE-002, and uses known
valid segments to observe enabled hard end before final application admission.
Refusal does not commit/publish that private clock proposal; the authoritative
independent clock command retains durable expiry/finish integration. Finished mixed
time/score paths require explicit reviewed policy AND a complete retained
representation: no fake cap-limited/unapplied component is invented for lifecycle
denial. These remain explicit required follow-ups, not permanent product omissions.
AS16/17 now match those boundaries. No unresolved material logical finding remains
within this document's declared scope.

## Positive contract and decomposition checks

- Current execution authority precedes operation/claim handling; current disclosure
  access is separately checked against the ACTUAL historical source/result. A grant
  to apply does not grant unrestricted historic data. Denial returns no protected
  payload or clock/arithmetic/token/action work. Typed ports state current binding,
  trust and missing-adapter gates; opaque refs/documents prove no authority/policy.
- Complete original command equality is distinct from immutable physical
  ApplicationIntent equality. Preview mode/token and stable caller are retained;
  revalidation uses actual current cap/conditions/head, without rewriting a previous
  command or comparing an old physical expected token with today's head. Concrete
  canonical codec/input-mode/bounds/error tests still require their own review.
- State token covers only declared snapshot fields. All15 current contribution
  dimensions and actual plan head are rechecked separately; support provenance and
  evidence/valuation/review/reversal/settlement stay independent. Arrival-time rule/
  quote/request history is not silently reevaluated using current rules/rates.
- Existing bundle stage requires a valid computed proposal before its early claims.
  Query-only STORAGE-017 is therefore an actual missing prerequisite, not reuse by
  raw SQL/dummy proposal or weakened old equality. It must return full validated
  selected records with normal limits/errors/provisional semantics, no authority.
- Existing success-bundle store cannot enforce standalone durable NUM-04 holds.
  STORAGE-016 retains stable installation/tagged ORIGINAL subject gating independent
  of operation key/planrevision/confirmation, immutable reason/source/history and
  current version/authorized resolution. Whole-input hold creates no success claim;
  explicitly approved timer-only composition preserves the existing original claim
  and cannot replay already-applied other components. Resolve+apply is one unit;
  clearing one hold never clears another independent condition.
- One real writer unit owns BEGIN/COMMIT. Normal executed denial/conflict/duplicate
  may use an acknowledged read-only COMMIT; existing writer completion still requires
  normal body return before external publication. Known B08 rollback, commit/cleanup
  uncertainty, fail-close/no blind retry, committed body failure and later response
  failure remain distinct. No owning old adapter, nested transaction or event pump
  is introduced by the proposed borrowed ports.
- Exact current debt-first/cap/lifecycle/counter composition retains per-occurrence
  provenance and zero components; only actual Main addition rescues Grace/wait.
  Signed original corrections do not manufacture financial reversal identity.
  Versioned command/state/result/audit/job meanings are required future codecs,
  not semantic guarantees inferred from storage-only envelopes or fake providers.

Actual state graph has396 records: ACCTCONTRACT-001 depends on integrated ACCT-003,
STORAGE-002 and CORE-005. Separate STORAGETEST-016→STORAGE-016 and
STORAGETEST-017→STORAGE-017 depend on the reviewed logical contract/bundle, not the
future service. ACCTTEST-004 depends on BOTH implemented stores, then ACCT-004 on
its own reviewed service baseline. No epic/child cycle or aggregate-red shortcut.
The same focused PR's historical branch name preserves work; the task split does
not complete ACCTTEST-004 or either new storage slice. Shared schema/order/build/
pins/metadata remain root-owned. Original release requirements are retained.

## Independently derived AS examples and actual checks

Reviewed all21 AS examples against the cited contracts. Independent standard-library
integer/Fraction diagnostic (no production helper, source result, SQL or native
case) reproduced these literal (MainAdded,DebtRepaid,CapLimited) partitions:

- 3480000/0 +600000 cap3600000 → (120000,0,480000);
 cap3540000 → (60000,0,540000).
- 55/2 +10 cap60 → (5,2,3);100/5 +8 cap60 → (0,5,3).
- 0/5 +5 cap0 → (0,5,0);0/5 +7 uncapped → (2,5,0).
- Main/debt/request9223372036854775807 uncapped → (0,9223372036854775807,0).

Each exact sum equals the request. AS06 sequential10 then4 gives(5,2,3) then(0,0,4)
and final60/0. AS07's fitting time55+5=60 cannot survive whole-input score
9223372036854775807+1 overflow. AS17 request-150 partitions as-100-50, debt7+50=57,
or timer-only actual0/full-150 held without selecting global disposition. For AS18,
INT64_MIN magnitude9223372036854775808 minus Main1 is signedMAX; Main0 would require
unrepresentable debt9223372036854775808. No signed64 negation is used. Grace/wait,
deadline, identity/access/preview, B08/unknown/publication and hold examples were
checked against explicit contract semantics, not labeled observed service passes.

Actual read-only checks on root's final planning/document artifacts:
`python3 tools/check_project.py` PASS396 tasks/122 decisions/local links/presets/
current HTML; `python3 tools/progress.py render --check` PASS/current;
`git diff --check` PASS. A nonblocking old inline baseline counter in README was
reported separately; summary package counts are not verification percentages.
No C++/DDL/stub/harness/fixture/test was authored here. No native/old-baseline run
or provider/live side effect occurred. Existing209 passes establish no new service
acceptance or intended red.

Next gates remain concrete hold/query contracts+interfaces+independent real-store
fixtures/tests/review/compiled discovery/intended red, then implementation/code
review/actual ordinary CI/integration; only then service API/stubs/harness, its own
independent baseline and real missing-behavior red. Actual policy/access/segmented-
clock/action adapters and unresolved mixed/post-finish/continuation semantics remain
mandatory. All seven integrations/three FX, both renderers/UI/locales/themes,
platforms/packages/security/backup/recovery/live gates stay in full release scope.
