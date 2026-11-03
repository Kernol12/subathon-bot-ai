# STORAGETEST-016 handoff — durable application-hold baseline

Coordinator/integrator /root,2026-11-03,
`task/STORAGETEST-016-durable-holds`, [PR47](https://github.com/Kernol12/subathon-bot-ai/pull/47).
Baseline acceptance COMPLETE; final metadata CI and normal merge remain pending.
STORAGE-016 implementation is PLANNED and must start from integrated main.

The baseline provides reviewed logical/physical/API contracts, unconditional stubs,
independent frozen native SQL fixtures and35 feature slots/265 scenarios. It proves
an intended missing-feature red, not passing hold behavior. Specificationv0.45
§§3.3–3.5,4,5/NUM-02/03/04/DB-01; [brief](../tasks/EPIC-009/STORAGE-016.md),
[prerequisite ADR](../decisions/0003-application-service-prerequisites.md).

Logical701c710f/physical83e96362/current API46c70297 remain the contracts. Step6
has12 SQL statements/full89 catalog/F89890/SHA9f5b61d4; every old prefix is exact.
Headerfe50fb10/CPP5b10c0e0 still contain six immediate unconditional marker entries.
No production hold/query/migration behavior has been implemented by this task.

Reviewed source0cce9545/author1a65cc9e/harness ea1e3cab+b9529c18 and unchanged
fixture63de09c5/helper7ca946f2/environmenta17312fe are pinned in
[CI/red evidence](storagetest-016-ci.md). [Full review](storagetest-016-review.md)
adb0404/41aa858 plus [separate setup correction](storagetest-016-setup-correction-review.md)
743b87fe establish all current tests; [final attribution](storagetest-016-final-review.md)
8d3a9bc verifies the actual revised valid red.

Exact selected officialSQL41640390883 FOR598a96e succeeds12:59:45–13:01:46Z:
fresh110/zero warnings/GNU13.3/Boost108300/Qt6.12/SQLite3.53.4/new3diagnostic5PASS
0skip188ms/exact35compiled/oldstable/all11. Synthetic checkout tree equals598.
ONLY THEN native `ctest --preset sqlite-hold-debug -L '^sqlite-hold$' --verbose`
at SAME CLEAN598a96e runs13:02:29.325–13:02:35.196Z,exit8:35 failed slots,
265 unique named missing rows,70 init-cleanup passes,0setup/skip failures.
Raw59a4d6d3/LastTestb6892801 retained; do not rerun an already established valid red.

The [initial af310 attempt](storagetest-016-initial-attempt.md) remains incomplete:
261 markers+4 actual setup errors. Genuine physical-schema SQL corrections preserve
all assertions and independent source-mismatch requirements; actual separate review
and native OLD-library setup diagnostics preceded the revised candidate/CI/run.
Earlier UPDATE-only failure injection was also corrected additively before full
review. No production change or baseline weakening obtained red.

Changed paths: three hold contract documents; header/stub CPP and minimal server
library; new JSON/helper/environment/feature tests; root CMake/presets/Qt CMake/CI;
evidence and state/HTML/acceptance-map. Existing implementation/tests/fixtures and
migration prefixes are unchanged. Root owns all shared files/integration.

Checks: native compile0warnings/exact35/global264/format;
`python3 -m unittest discover -s tests/tooling -v`:19PASS0.273s;
project396records122decisions,
progress render/currentHTML and source/metadata diff checks pass. Full-PR whitespace
check reports only byte-exact archived raw logs (original trailing-space error
messages and LastTest EOF blanks); those are preserved with their hashes. No
code-style or test gate is relaxed. New cases are enabled draft-red; ordinary CI
runs mandatory foundation/old acceptance and compiles/discovers the intentional red.
No WILL_FAIL/disabled/ignored failure. Post-stub assertions await implementation.

Next: finish latest metadata CI/rules/thread/readiness gates and normal exact-head
merge; refresh main; claim STORAGE-016 in a dedicated branch before editing. Preserve
source0cce9545/all independent oracles. Implement exact schema/closure/mutation/
metadata/bounds/rollback/unknown semantics, with separate source/allocation-order
review and actual passing acceptance/promoted CI. Do not pretend sparseMAX history
proves healthy lifetime exhaustion or DATA links prove authority/same-unit commit.
Service004, capture producer/evaluator, permissions, mixed timer/continuation/
postfinish/financial entitlement, all seven providers/three FX, both renderers,
locales/themes/platforms/packaging/recovery/security/live/release remain open.
Progress JSON/HTML and acceptance map are updated with baseline-only evidence;
DB-01/NUM-02/NUM-04 remain partial. No owner decision/access is needed now.
