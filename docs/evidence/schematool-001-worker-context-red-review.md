# SCHEMATOOL-001 — actual new context regression red attribution

Different observer/reviewer `/root`,6 November2026, after actual new test approval
9aedf4f. Author `/root/session_supplier_review` observed ONLY new regression6894a22
against unchanged worker9f88767 at12:11:07.766722Z. Root actually read the native
stdout/stderr/receipt from `/tmp/schematool001-worker-context-first.{stdout,stderr,json}`;
checked exact4158-byte stderr SHA256
`f93e8925f83b0925c11e41eee10131f33833f8dbe8c1078db498f9c893cb4cbb` and empty stdout.
Verdict: observed intended reached-bug red APPROVED; no replay or production change
was run by this reviewer. Raw artifacts are preserved byte-for-byte as [stderr](schematool-001-worker-context-first-red.stderr.txt), [stdout](schematool-001-worker-context-first-red.stdout.txt) and [receipt](schematool-001-worker-context-first-red.receipt.json).

Native exit1,3 methods,1.453s suite /1.515838s external wall,3 assertion failures,
zero errors/skips. Four positive literal observations passed: dynamic property
name, raw DATA, standalone unknown keyword, and required[] raw sibling control.
Three negative observations reached exact outcome-line assertions after actual A,
activation EOF and outcome EOF: raw allOf/required sibling returned ok instead of
invalid_schema; OAS Schema x-field returned ok instead of unsupported_feature;
Parameter Reference Object x-field returned invalid_schema instead of the earlier
unsupported_feature profile gate. These are the source-reviewed defects, not
missing input/dependency/setup/import/worker failures. Expected outcomes derive
from the frozen native semantic/profile contract and pinned SDK source.

The negative assertions fail before their following status assertion, so this
receipt does not claim that assertion executed on those three observations. Child
wait completed before the protocol/outcome checks; the helper's finally/ExitStack
owns fallback process/descriptor cleanup. Positive observations reached their final
normal status checks. No fake successful validator or alternate backend supplied
any observation. No original64 method or historical FIRST was replayed.

Correct owned production code after this recorded red, preserve all tests/contracts
and bytes, then run the new module on actual corrected source. Actual different
context/source audit, full public64, other new regressions, both current hosted
lanes and normal merge gates remain. This is red attribution, not code approval or
implementation completion.
