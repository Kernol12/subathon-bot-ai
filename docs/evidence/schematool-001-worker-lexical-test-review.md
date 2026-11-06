# SCHEMATOOL-001 — actual lexical regression source review

Reviewer `/root`, different from author `/root/session_plan_review`,6 November2026.
Actual authored revision91ce5da189fcca90876ac63242d23b704f42a5d4,
test blobfc826843aa64e5e5d1917403bd0f612e2f9f9240. APPROVED for first new-only
reached-bug run against unchanged scanner a2507d2419828cd477a19621ab5847d963ac3ee7
and parent1579604c8d50f08e7320c9d7723246fd2ddd3df7. Actual source inspected before
execution; no candidate/helper/parent/worker/SDK import or body ran in this review.

The complete literal manifest, empty admitted Draft schema and external value
binding select the intended parse phase. Raw value is exactly opening quote +
(n-2) ASCII x bytes + final backslash, with no closing quote. These bytes are valid
UTF8 and incomplete JSON. Accepted API §3 raw quoted-token ceiling counts actual
bytes, including quotes/escapes; ordering is UTF8/BOM, resource scan, strict JSON.
Therefore65535/65536 bytes reach invalid_json;65537 actual bytes cross the ceiling
and return resource_limit first. The expected outcome does not use production
scanning, decoding, arithmetic or native validation to construct its oracle.

Three separate ordinary methods preserve both controls even if the middle fails.
They use the real public validate_package, real owned synthetic temp files and
fresh actual worker; no Popen/backend/SDK/decoder patch, fake success, alternate
limit or private parser call exists. Exact enum identity and no public output
are checked. The lazy setUpClass trusted file import has no invocation on module
import; TemporaryDirectory owns cleanup. Data size is below every earlier package/
file/number/depth/count limit; earlier setup/dependency errors cannot supply valid
red attribution. Tests are undecorated and use ordinary unittest lifecycle.

Only new file/note were authored; original64 source64f4911, every contract, pin,
fixture and FIRST artifact remains unchanged. Actual root AST/compile admission,
repository/current-HTML and whitespace checks pass without executing methods.
Next root observes ONLY this3-method module on the exact author worktree/source,
preserves native bytes/exit/time and verifies the reached65536 assertion plus two
controls before A corrects its owned scanner. No production fix is approved by
this test review; actual code review/pass/full public acceptance/CI remain gates.
