# STORAGE-002 — independent review of returned-byte regression

Reviewer: root coordinator; author: bundle_reviewer. 2026-11-03. APPROVED exact
additive test commit291047f18b084019b85d5e5907ace7d8d45dba47/testCPPfadaed4 and
its independently derived oracle; runtime/code approval remains separate.

Root inspected the full53-line additive method and public DTOs, frozen input
INSERTs and rollback helper. No original32 assertion/fixture/port changes. New
public read_inputs test requires a complete ready record at1105 logical bytes and
result_limit at1104, actual marker/whole-unit rollback, and unchanged physical
snapshot. Finite ample writer budgets isolate this from writer consumption limits.

Root independently parsed ONLY frozen JSON input literals with Python stdlib (no
production helper, UUT output or SQL). Independently summed actual DTO members:
identity8/facts77/arrival22/receipt115/event134/capturedstate33/valuation53/
requests63+63+63+61/rules64/modifier46/capture588/currentstate59/currentrecord192/
supportConditions32/anchor190/snapshot112. Complete total:
1 subject+(6+8) PlanKey+8 head+588+112+192+190 = **1105**.
The Conditions member lacks a payment-evidence enum; its observation already has
one. Existing uint32 schema_version counts4, enums/bools1, domain int64/uint64 8.
Raw independently reproduced arithmetic: local-evidence/storage002-independent-byte-review.json.

This is actual separate TEST review before the new test body runs or helper
correction is integrated. Root must compile/register33 and observe the targeted
1105 erroneous result_limit against helper824 plus actual SQL source BEFORE held
helper159 correction; neither compiler failure nor missing-feature stub is that
regression red. Original32 historical red90fa remains unchanged evidence.
Full source/config review, relevant33/full207 and actual changed official CI still
remain required before task completion or normal merge.

## Actual reached red, correction and pass

At CLEANe6bc06d7db5eabc5bcb6968a412a44089a71d4d4 after successful root configure/
warning-free build, root ran ONLY:
`ctest --preset sqlite-bundle-debug -R '^sqlite-bundle.supportAnchorConditionsUseExactReturnedByteBoundary$' --verbose`.
UTC2026-11-03T05:26:55.343043Z, actual exit8/CTest0.04s. Init/cleanup bothPASS,
new bodyFAIL at required committed normal work;0skips/warnings/crashes/setup errors.
[Raw first targeted regression run](storage-002-byte-regression-red.txt) SHA256
3267a29d1daea47333bd16c4deaf9c496d5780d694b78e2b3f81b909cc14df5b.
Source is actual SQLf969 with helper824, not the historical missing-feature stub.

Supplemental debugger inspection by bundle_implementer (following debugger skill,
CLion router unavailable→read-only GDB fallback) at exact root test2159 confirmed
actual1105 limit, storage=result_limit, transaction=rolled_back,
work error=work_exception, no value/problem. First generic ok breakpoint hit fixture
setup and was not the claimed defect; exact refined breakpoint supplied attribution.
No raw GDB logfile retained; no production logging/source instrumentation added.

ONLY THEN root integrated isolated correction159850c as55c059f: count the actual
Conditions member, removing the fabricated payment-evidence byte. Build succeeded
without warnings, SAME targeted command nowPASS1/1 (Qt3PASS0FAIL0SKIP,31ms/CTest0.04s).
Raw green/build logs retained local-evidence/storage002-byte-{regression-green,
corrected-build}.txt. No original red preparation repeated. Actual full source/
configuration review and remaining focused regressions/full acceptance/official CI
remain separate gates.
