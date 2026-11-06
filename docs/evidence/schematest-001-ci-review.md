# Independent CI configuration review — SCHEMATEST-001 library foundations

**APPROVED for actual CI configuration and bounded local-evidence description.**
Reviewer `/root/session_plan_review`, different from configuration/evidence author
`/root` and foundation author/reviewer roles,6 November2026. Actual candidate
`e0406e08694f6f8e05d1ee3c2e3795c279fae261` examined in NEW isolated branch
`task/SCHEMATEST-001-ci-review`, worktree `/tmp/subathon-schematest001-ci-review`.
Only this review note changes. This is not hosted CI/minimum-Python compatibility,
wrapper behavior, baseline completion or future source/API/merge approval.

| Actual artifact | Reviewed Git blob |
| --- | --- |
| [New workflow](../../.github/workflows/schema-library-foundation.yml) | 87aa2e6d07738675f2e4b38085bbf614d3ec867f |
| [Corrected native evidence](schematest-001-foundation-native.md) | 5f96cb21a0f8a50b8ba448d0a8aadd2f671f2f3f |
| [Corrected native receipt](schematest-001-foundation-corrected.receipt.json) | cf6464f400dfa1478d1d646b9ce3e39c7ac17149 |
| [Actual separate source/correction review](schematest-001-foundation-review.md) | 373ef35b8b51356279d8f833b71e7feb14b6adaf |
| [Unchanged corrected source](../../tests/schema_validation/library_foundation.py) | d2df9b986892d9eee8074678c2c2aa3de35b55b5 |

Read root/tools/tests instructions, admitted dependency records, current logical
contract, actual foundation source, original failure/corrected receipts and source
review. Existing independent dependency admissionee5/fadd and source/correction
approval21f/import1aa remain separate; this note does not replace them.

## Workflow and isolation audit

The workflow runs on every PR to main and main push, with manual dispatch as an
additional trigger. Its two ordinary matrix entries select Ubuntu22.04/system
Python3.10 and Ubuntu24.04/systemPython3.12. The selected `/usr/bin/python3` minor
is asserted before setup, avoiding an ambient runner Python alias. Both matrix
variants run normally with a15-minute job timeout; fail-fast false lets both produce
evidence and does not ignore a failed job. No body-execution switch, skip/failure
inversion, continue-on-error or path filter suppresses either lane.

Checkout retains the existing pinned action SHA, read-only contents permission and
persist-credentials false. Dependencies install only into a fresh RUNNER_TEMP venv
with the exact admitted requirements.lock, pip isolated/no-cache, require-hashes
and binary-only selection from explicit PyPI. All20 direct/transitive requirements
remain exact. No global/server/client dependency, optional extra or source build
is introduced. Network access here is dependency acquisition, not a claimed offline
validator run. Actual hosted installation/availability still must be observed.

The narrowly conditional removal of bootstrap setuptools handles its possible
presence in a Python3.10-created venv. It removes only setuptools if present, then
unconditionally runs pip-check. The foundation still asserts the COMPLETE exact20
installed distributions after excluding only bootstrap pip; no assertion is changed
to tolerate unexpected extras. Metadata/setup faults cannot become passing feature
evidence merely because this removal is conditional.

The real standard-library AST guard requires exactly one LibraryFoundation class,
the exact seven unique reviewed test method names and no decorators on them. The
subsequent invocation is unconditional and uses the actual checked-in source under
`env -i`, minimal PATH/UTF8 locale, venv Python `-I -B`. No environment-selected
backend/cache or user PYTHONPATH can replace the program through inherited values.
All current seven methods execute through unittest's normal failure exit behavior.
The exact source retains its persistent network poison and actual selected-library
calls; there is no fake validator or expected-success substitute.

Every existing workflow file is byte-identical to the pre-change699943b tree;
only this new workflow is added. Exact hashes of requirements.lock, wheels.json,
THIRD_PARTY_NOTICES and tools policy equal the admittedee5aeb8 artifacts. No pin,
license or standard-library-tool isolation claim changes here. New job names are
recorded without claiming repository required-check enforcement. Current hosted
results and any later protection configuration remain coordinator gates.

## Corrected local receipt and preserved invalid attempt

Rehashed committed corrected stderr against the receipt: SHA256
`5ee713ab6df0a7a68d0afd55b4419e9ae807309e59e083c8e9a5adf3b1440737`, seven
named ok records, Ran7tests/0.115s and final OK. Receipt records direct exit0,
elapsed0.201s and the SHA256 of empty stdout. Its exact source blob equals the
currentd2 source approved by a different reviewer BEFORE execution. This verifies
root's retained CPython3.14 receipt; it is not a new run on3.10/3.12 or hosted CI.

Original4f37 FIRST attempt/raw evidence remains present, exit1, and explicitly
invalid as green-foundation proof. The selected native reader reached its own
bundled metaschema through a file URI, which an overbroad initial guard classified
as network. Different actual correction review documents the specification-based
reason and exact admitted34260-byte resource/hash. The narrow correction supplies
only those verified real bytes, preserving the original seven semantic methods,
network poison and adding its trusted-read ledger assertion. No failure is renamed
wrapper red, no broad file/network exemption or fake validating callback appears.

Corrected success therefore supports only the fixed real-library recipes on the
actual3.14 environment and no intercepted network attempt under those tripwires.
It is not an OS network sandbox, complete offline/package validator, arbitrary input
proof or future wrapper resource/process guarantee. The remaining adapter/context/
reference/example/format/numeric/API/test/red gates remain explicit; SCHEMATEST001
stays in_progress and SCHEMATOOL001 is not released.

## Actual independent checks and remaining gates

Extracted all four literal shell blocks from the actual workflow and ran `bash -n`
on each; both embedded Python blocks parsed with standard-library ast. Executed
ONLY the stdlib method-inventory guard against actual source: exactly7PASS.
Independent Git-object, receipt byte/hash/test-name and old-workflow/pin/policy
comparisons passed. No candidate foundation/program/native/library body was replayed
by this configuration review; no venv/dependency/source/test was altered.

`python3 tools/check_project.py` PASS426tasks/122decisions/current links/presets/HTML;
`python3 tools/progress.py render --check` PASS;
`python3 -m unittest discover -s tests/tooling -v` PASS19/19 in0.315s;
`git diff 1aa5cb2..e0406e0 --check` PASS. These checks are static/configuration and
legacy tooling evidence, not hosted/library/product acceptance.

No blocking configuration/evidence issue found. Root must publish and observe BOTH
actual latest-revision hosted matrix jobs and all applicable inherited checks,
record failures honestly, assess required-check configuration and complete normal
review/discussion/rules/merge gates. Concrete wrapper/API, reviewed independent
tests/intended red, later implementation/ordinary CI and full runtime/platform/live/
release requirements remain. All seven integrations and three FX providers remain.

## Actual hosted foundation receipt — separately observed

**APPROVED for this actual fixed-library foundation receipt only.** Same different
reviewer, NEW isolated receipt branch
`task/SCHEMATEST-001-hosted-foundation-review`, worktree
`/tmp/subathon-schematest001-hosted-foundation-review`, based on actual published
`0dfa0de931a42523169418f8792126cbd435a82e`. The preceding configuration verdict is
retained as a prior stage, not rewritten as prospective hosted approval.

Independently queried
[workflow run42178767687](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42178767687)
and downloaded the COMPLETE log with `gh run view 42178767687 --log` to my own
`/tmp/schematest001-hosted-foundation-independent.txt`:111203bytes/585lines, SHA256
`ee79b053ef0a311eac97ad22776911f623dadf96a7d8d6a219b0e5d09a646be1`.
That independently matches root's retained log. Run metadata reports pull_request,
exact head0dfa0de, created07:07:12Z, completed SUCCESS/updated07:07:30Z on6November2026.

Both jobs checked out PR merge commit
`55fed51b07c80f78ab27da081deb3aa61f0def4c`, as their actual checkout logs report.
Independent GitHub Git-object queries confirm parents main14190827 and exact0dfa
and tree `9c76d34c217628ee5c2e0874945f880d26621368`, identical to published0dfa's
tree and my local Git tree. Thus the actual checked-out source/config/dependency
artifacts equal the reviewed head; the PR merge ref is not assumed identical merely
because workflow metadata names its head. Initial Git-object lookup with a short
SHA returned404; using the complete logged SHA resolved it, with no test rerun.

| Actual job | Distribution Python | Observed foundation body | Job result |
| --- | --- | --- | --- |
| [127763232191](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42178767687/job/127763232191), Ubuntu22.04 | 3.10.12/GCC11.4.0 | Seven distinct named ok records, Ran7tests0.158s, final OK | SUCCESS, completed07:07:30Z |
| [127763231560](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42178767687/job/127763231560), Ubuntu24.04 | 3.12.3/GCC13.3.0 | Seven distinct named ok records, Ran7tests0.191s, final OK | SUCCESS, completed07:07:28Z |

Read the full setup/version/install/inventory/body logs, not only status summaries.
Both minor guards actually pass; binary-only/require-hashes/isolated/no-cache
installation runs against the reviewed lock. Independent parsing of each actual
Successfully installed line confirms the exact20 admitted versions. Ubuntu22.04
finds bootstrap setuptools59.6.0 and explicitly uninstalls it; Ubuntu24.04 reports
no such removal. Both unconditionally print No broken requirements found, then
the strict seven-method inventory guard passes and the minimal-env `-I -B` command
actually runs every method. The20+pip runtime inventory assertion passes in both.
There are no skipped tests, fail/error summaries or failed/ignored steps in these
two jobs. No optional backend/extra/source build appears in the installed inventory.

Source d2 and workflow87aa2e6, all dependency/policy/notice artifacts remain EXACT
the prior reviewed versions from e0406e0; intervening0dfa changes only evidence/
handoff/checkpoint. The actual seven passing recipes include their persistent
tripwire cleanup and trusted-resource ledger assertions. This supports the absence
of intercepted attempts during those recipes, not a universal OS network sandbox
or unknown-scheme/default-handler guarantee. Initial invalid4f37 attempt/correction
history remains unchanged and is still not wrapper intended red.

Read-only current ruleset26170263 remains active with0 required GitHub approving
reviews, discussion resolution and no bypass actors; it has NO required-status-check
rule. New job existence and success therefore do not claim enforcement. The written
workflow still requires applicable latest-head checks and normal review/rules/merge
gates, including after later source/config revisions.

Actual independent Git-object/log/installed-version/name/hash comparisons,
`python3 tools/check_project.py` PASS426/122, `python3 tools/progress.py render --check`
PASS and `git diff --check` PASS. No local foundation/native/library replay, install,
venv edit, source/test change or GitHub write occurred in this receipt review.

This establishes these exact ordinary fixed SDK recipes on actual hosted3.10/3.12,
alongside retained local3.14 proof. Complete schema-context traversal/closure,
reference-cycle/inventory/offline adapter, process/resource limits, concrete wrapper
API/tests/review/intended red and later SCHEMATOOL001 source/ordinary CI remain open.
SCHEMATEST001 baseline completion, actual CONTRACT013 package, product/runtime/
platform/live/release capability and future-head checks/merge are not approved.
