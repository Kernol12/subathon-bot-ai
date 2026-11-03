# STORAGE-002 — application-bundle SQLite store

Coordinator root; SQL author bundle_implementer; pure helper author bundle_validation;
independent code/config reviewer bundle_reviewer. 2026-11-03, dedicated
`task/STORAGE-002-application-bundle` from integrated main995e0fa. [PR43](https://github.com/Kernol12/subathon-bot-ai/pull/43).
Specification v0.45 §3/§16 DB-01/OPS-02; concrete logical/physical revision1.

All seven formerly stubbed functions now implement caller-session initial/read/
input/complete lookup/selected-basis/stage and the frozen migration5 manifest.
One real caller owns commit; the facade never commits/adopts/destructor-cleans.
Exact uint64/rational values, immutable state/provenance, separate current dimensions,
identity claims, supplied component conservation, reversal progress and real whole-
unit rollback/unknown distinctions are retained. Sibling metadata is paged64/65,
with actual full cardinality and separate materialization/writer budgets. Head is
advanced last; result/job documents remain storage envelopes rather than semantic
permission, recovery or delivery certificates.

## Preserved baseline and focused corrections

[Original reviewed baseline](storagetest-002-handoff.md), originalCPP4b60 and red90fa/
rawSHA1e677e remain historical evidence; no completed preparation was repeated.
All32 original cases are retained with ONE separately reviewed correction:
terminal-ticket misuse must preserve completion session_closed, while physical
rollback, absent payload and capability assertions remain unchanged. Root authored
[reason](storage-002-terminal-baseline-correction.md), different reviewer
[approval](storage-002-terminal-baseline-review.md) preceded commit945e. Automatic
review initially rejected the premature commit; required independent review
resolved that objection without an override or weakening writer behavior.

Three additive slots (one has3 data rows) have actual separate test review and
reached-red evidence before root correction integration. The [byte regression](storage-002-regression-review.md)
proves exact1105/1104 returned quota; the [quota/metadata regressions](storage-002-metadata-regression.md)
prove independent returned-vs-input quotas and detection of transition0/progress0.
The statekind1 row already passed before correction and is not claimed red.
Frozen fixtures, public header8e24, physical contractf13 and prior ports/schemas/
assertions remain unchanged. [Independent full source/config review](storage-002-code-review.md)
5b71576a accepts sourcea905/helperb0ca/test8bf/QtCmake0e0/WF783/presetsb9.

## Actual local checks

Clean root6a6a200: configure/build sqlite-bundle-debug PASS, final3-step build has
0warnings; `ctest --preset sqlite-bundle-debug -L '^sqlite-bundle$'` PASS35/35,2.33s;
`ctest --preset sqlite-bundle-debug` PASS209/209,7.07s. Different reviewer fresh
20-step warning-free build,35PASS2.27s/full209PASS7.06s. Exact35 compiled/enabled
registrations,3data rows within one slot,206Qt totals0fail/0skip/0blacklisted.
Host GCC16.2.1/Boost1.92/Qt6.12.0/SQLite3.54.0; raw commands/hashes in
[CI evidence](storage-002-ci.md). Existing174 suites/diagnostics retained.

## Publication and remaining gates

Local source/config review and acceptance are complete. Actual automatic
[SQL41585131851](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41585131851)
at published6133afe SUCCESS: fresh92/0warnings/GNU13.3/pinnedBoost1.83/
verifiedQt6.12SQLite3.53.4/old164/mandatorygreens/exact35guard/UNCONDITIONAL35PASS
5.26s, all11PR checksSUCCESS. Pins/check name/permissions unchanged. No manual
input/skip/inversion. Task is DONE but PR43 is UNMERGED at this checkpoint;
completion-metadata head must receive its own latestCI. Actual readiness,
reviews/discussions/effective rules/mergeability must be rechecked before normal
exact-head merge. No independent review is extrapolated over changed behavior.

This is real synthetic disposable-file component acceptance, not the application
orchestrator, semantic rules/caps/permissions, provider/FX/refund allocation,
delivery/recovery/backup/UI/renderers/platform or complete release verification.
All seven integrations/three FX remain mandatory. No live effects or credentials
used. Next dependent slice after integration: ACCTTEST-004; no dependent edits yet.

[Independent final attribution review](storage-002-final-review.md)876cd975 verifies
actual source6133 officialCI and completed-but-unmerged task metadata. Finalhead
CI/normal merge remains an actual later observation, never prospective approval.


## Actual integration

PR43 normally MERGED2026-11-03T05:49:50Z at exact final34ed520ed3c32aeafa897650631f2898b7899159
as main8c7f71f424126727f1a846edc01541d0f091529f. All11 latest checksSUCCESS,
including final SQL41585878746, ready/CLEAN/MERGEABLE, zero unresolved threads,
effective26170263 requiredGHapprovals0 and unchanged reviewed code rechecked.
Normal merge with exact-head guard; no bypass/force/branch deletion. Localmain
fast-forwarded clean; only then ACCTTEST-004 claimed on a new dedicated branch.
Earlier UNMERGED paragraphs describe their actual checkpoints, now superseded by
this observed merge. Postmain workflows are separate observations, not invented.
