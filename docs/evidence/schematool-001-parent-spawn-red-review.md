# SCHEMATOOL-001 — actual constructor ownership red attribution

Different reviewer/observer `/root`,6 November2026. B ran only reviewed NEW spawn
candidatea230265/test777d907, parent1579604 at12:37:06.899997Z, after actual private
r3 C93a5e1 and different root57a79f1 source/oracle approvals. Root actually read
native stdout/stderr/receipt, verified exact bytes/hash and both reached paths;
no helper/test/worker rerun or production change by this reviewer.

Actual native exit1,2tests/.019s suite/.113703s observer,2FAIL/0ERROR, no timeout,
stdout empty. Both constructor cases reached native child-ownership assertion with
child still running, factory_returned empty and all4 parent descriptors CLOSED.
Earlier native self/PID/child-created/readiness, handler/controller/watchdog and
expected cancellation or resource enum guards passed. The tests retain actual
native self so destructor polling cannot establish reap. This is the confirmed
post-spawn/pre-return ownership gap, not an input/dependency/protocol/setup failure.
Both scripts used genuine blocked fault children and never certified validation.
Test fallback killed/reaped children only AFTER those observations.

Exact [stderr](schematool-001-parent-spawn-first-red.stderr.txt)3138bytes SHA256
14fe4719a6cdab8638829dd30a830c74e873c2d07f1dec09d6e1890022c70797;
[stdout](schematool-001-parent-spawn-first-red.stdout.txt) empty;
[receipt](schematool-001-parent-spawn-first-red.receipt.json) preserves command/cwd/
minimal environment/timing/source/review identities. Copied byte-for-byte into
repo evidence; no original64 or historical FIRST replay. Observer source review is
separate from authoring; attribution does not approve any future implementation.

B may now correct actual ownership with both cases preserved, no global handler
mutation, test-detection branch, frozen seam bypass or helper-thread-only safety
claim. Known post-spawn allocation and real main-thread signal paths both require
cleanup before synchronous return/rethrow. Actual independent corrected-code audit,
new regressions/full public64, fresh hosted lanes and normal merge remain gates.
