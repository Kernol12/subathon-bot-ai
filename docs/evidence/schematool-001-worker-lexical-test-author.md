# Worker lexical-boundary regression authoring — SCHEMATOOL-001

Author `/root/session_plan_review`,6 November2026. **AUTHORED ONLY; NOT INDEPENDENTLY
REVIEWED OR EXECUTED.** Root explicitly assigned this separate test-author role
after a concrete source-review finding. Root must provide DIFFERENT actual source/
oracle review before any run; the author cannot approve this test gate.

NEW isolated branch `task/SCHEMATOOL-001-worker-lexical-tests-author`, worktree
`/tmp/subathon-schematool001-worker-lexical-tests-author`, from actual root
`9004833abbf96160f2b850b6e2a7f0d396dcb99c`. Own ONLY
[worker_lexical_regression.py](../../tests/schema_validation/worker_lexical_regression.py)
and this note. No production/shared state/HTML/contracts/CI/frozen baseline edit.

## Requirement and independent byte oracle

Authority: root/tests instructions, specificationv0.45 §§17/18 API01/02/03 TEST04,
accepted logical tool§3, concrete API§§3/5 and paired SCHEMATOOL001 brief. UTF8/BOM
checks precede lexical-resource scan, which precedes strict complete JSON. Raw
quoted tokens have a65536-byte ceiling; the scan may refuse a real excess before
a later grammar defect, but cannot count a nonexistent byte after end-of-file.

Actual `_schema_package.py` bloba2507d2419828cd477a19621ab5847d963ac3ee7
advances by two for any backslash, including the LAST available byte, then compares
that synthetic cursor distance with the ceiling. A65536-byte unterminated token
ending in a backslash becomes cursor65537 and incorrectly resource_limit. This
is a source finding, not an observed result or an already approved correction.

Each literal is one opening quote, byte_count-2 ASCII x bytes, and one final
backslash with NO closing quote. Total raw size is EXACT byte_count. UTF8 is valid,
there is no BOM/numeric/deep/value-count issue, and JSON grammar is incomplete.

| Exact raw bytes | Independent expected outcome | Purpose |
| --- | --- | --- |
|65535 |invalid_json |Below-limit incomplete escape control |
|65536 |invalid_json |At-limit incomplete escape must reach grammar refusal |
|65537 |resource_limit |Real raw-token excess precedes incomplete grammar |

The complete literal manifest declares one Draft2020-12 schema and one external
value/root binding. Schema is a complete empty admitted Draft schema. All actual
files are ordinary owned temporary files; each is far below1MiB and their complete
raw sum below8MiB. Manifest shape/paths/counts/dialect and example target are valid.
Only the intended value-file token/grammar differs. No fixture expected result is
generated from project scanning, JSON or validation code.

## Real public path and preserved scope

THREE ordinary undecorated unittest methods invoke the real public validate_package
through one fixture-writing helper. No Popen/decoder/validator/SDK/backend/factory
is patched or replaced; actual parent/worker control and strict decoding are used.
No successful fake or private parser invocation supplies an outcome. Exact enum
identity and empty public stdout/stderr are required. Target import is lazy trusted
local setUpClass; module import creates only constants/definitions, no fixture or
worker/test/SDK call. Temporary directories close on exception or normal return.

Production parent remains1579604c8d50f08e7320c9d7723246fd2ddd3df7; package scanner
remainsa2507d2419828cd477a19621ab5847d963ac3ee7. Frozen original64 source remains
64f4911edae1e6ec1546777661f199252d06131c. The original FIRST63/261 and additive64th
records remain untouched; this is a separate NEW regression module, no replay.

After DIFFERENT actual review, root may observe the first reached-bug run with:

```sh
env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8 /tmp/subathon-schematest001-py314/bin/python -I -B /tmp/subathon-schematool001-worker-lexical-tests-author/tests/schema_validation/worker_lexical_regression.py
```

Cwd `/tmp/subathon-schematool001-worker-lexical-tests-author`. Retain actual raw
bytes/exit/time/code+test blobs. Planned old-source symptom is the65536 case's
resource_limit instead of invalid_json while the two controls pass; this is NOT
an observation. Setup/import/dependency/worker/protocol failures cannot substitute.
Then A may correct its owned preflight AFTER actual attributable red. Preserve all
assertions; passing regression/current source review and ordinary CI remain required.

Actual checks: Python3.10-feature ast.parse/compile WITHOUT exec, exact three names/
literal size+outcome arguments, complete ordinary JSON fixture shapes/byte totals
and unchanged source/baseline hashes PASS. Repository consistency426/122, render
--check and diff--check PASS. No candidate/test/helper/parent/worker/SDK import or
body, native child, clone, installation, live operation or GitHub write ran here.
All implementation/source/integration/security/offline/platform/live/full-release
gates remain. This test-author note grants no independent approval or production fix.
