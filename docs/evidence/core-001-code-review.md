# Independent review — CORE-001

Implementation author: `/root/numeric_implementer`.
Independent reviewer: `/root/numeric_reviewer`; date: 2026-10-31.
Baseline: specification v0.45 §3.5/NUM-01/02/04; exact-decimal contract revision1,
32-case reviewed/red baseline integrated in PR12 at main `cab329b`.

## Additive regression review before the finalizer fix

Regression author: `/root/numeric_test_author`.
Reviewed actual commit `5f9b3c017a820c9447e0708b8eadf0fc7278463f`;
[regression test](../../tests/acceptance/exact_decimal_regression_tests.cpp) blob
`347a810b1edba2ffc36d66ba44b700092bb2b381` and
[authoring note](core-001-zero-regression.md).
Verdict for this stage: **approved for additive regression baseline**.
This is not approval of the incomplete implementation candidate.

The existing contract checks normalized rational numerator/denominator components.
`Exact(0) / (cpp_int(1) << 4096)` is mathematically zero, whose normalized components
are0/1; its final integer must be the literal0. The test does not assert dependency
raw representation and does not use the production parser to derive the expected
value. It remains valid whether a Boost release canonicalizes the intermediate zero
immediately or preserves a raw denominator. Requiring0 does not weaken the original
nonzero4097-bit denominator rejection or enlarge any resource bound.

This is a focused implementation regression under the existing revision1 requirement,
not a contract change or correction of a wrong original assertion. The reviewed
32-case baseline/header/contract are unchanged. The separately authored test must
receive an actual intended red against candidate `873cc6d` before its corresponding
finalizer fix; missing dependencies/compile failure do not count.

Independent syntax-only compilation:
`c++ -std=c++20 -Icore/include -fsyntax-only tests/acceptance/exact_decimal_regression_tests.cpp`
exited0 with installed headers; `git diff --check` exited0. No new regression body
was executed by the reviewer before this approval. The coordinator owns the test
target, observed-red evidence and integration. Final code/CI review remains pending.

## Candidate implementation inspection

Inspected author commits `4ab8613` and `873cc6d4fba9663f3d6f8d5834d528442222b44b`,
source blob `e9ea7805c04af1bbd30f38946a40f5f483da7589`. This candidate is not the
final approved implementation: its normalized-zero finalizer handling is pending the
reviewed regression/red/fix sequence above.

The bounded ASCII cursor parser, complete-syntax-before-internal-resource precedence,
exponent accumulator (at most2569 before its over-limit sentinel), <=256-digit
coefficient, <=511 decimal power, exact signed construction and post-validation
canonical parserzero are consistent with revision1. No binary floating point, Qt,
network/provider or SQL dependency enters core. Sign/magnitude final rounding and
post-round checked signed64 conversion preserve the required tie/end-point behavior.
The known raw-zero denominator issue is the remaining implementation mismatch.

Independent fresh candidate checks in isolated `local-evidence/worktrees/core001-review`:

| Command | Actual result |
| --- | --- |
| `cmake --preset exact-numeric-debug` | exit0 |
| `cmake --build --preset exact-numeric-debug` | exit0 |
| `ctest --preset exact-numeric-debug -L exact-numeric` | exit0; original32/32 pass |
| `cmake --preset acceptance-debug` | exit0 |
| `cmake --build --preset acceptance-debug` | exit0 |
| `ctest --preset acceptance-debug -L 'acceptance\|regression'` | exit0; credit23/23 pass |

These are local candidate checks only, with installed Boost1.92.0/GCC16.2.1/
GoogleTest1.18.0. No final code verdict, automatic numeric CI result, Ubuntu/Windows
compatibility, application atomicity or provider/platform/live claim follows yet.


### Regression harness approval

Reviewed coordinator `bd9c43ba0f243d9cc5c64a460b59b8ade727ffa4`, including the
separate regression target/linkage and `exact-numeric;regression;core` discovery
labels. This target runs the exact reviewed test against the intended numeric
library without skipping, failure inversion or special expected-failure behavior.
Independent `cmake --preset exact-numeric-debug` and
`cmake --build --preset exact-numeric-debug` exited0;
`ctest --preset exact-numeric-debug -N -R ExactDecimalRegression` exited0 and
listed exactly the one new case, without executing it. The coordinator may now
record its genuine intended red against candidate `873cc6d` before authorizing the
finalizerzero fix. This harness approval leaves final implementation/workflow
approval and actual CI observation pending.


## Final code and workflow configuration verdict

Verdict: **approved for code integration**, with actual latest-head PR CI and
repository merge gates still required before task completion/merge.
Reviewed exact final author `74f3594d352b5e1ef1b7817a4d0d0330932ec1bc`, final
source blob `a1ddb4283292a5da0a81a0f4e8255fffc19cafd7`, integrated coordinator
`a58dfd5` and the complete shared harness/workflow/configuration through coordinator
`cd96d7c1cfa3fe158f23e436a768bbe60f5705c5`.

Inspected the actual [one-case red log](core-001-zero-red-ctest.txt) and author note:
coordinator `6224ac3` follows the separate regression/harness approvals above, and its
successfully built test body throws the expected incorrect `length_error` rather
than returning zero. The observed exit8 precedes authorization of the finalizer fix.
The final source returns0 immediately when the exact numerator is zero, before reading
or rejecting its raw denominator. This implements mathematical normalization0/1 and
preserves the original nonzero oversized-component failures. Parserzero canonicalization
was already exercised by the unchanged original factor/signed-zero assertions.

Reviewed the complete implementation beyond those assertions. Every input cursor read
is guarded, ASCII digits do not depend on locale, and malformed full input is rejected
before internal mantissa/exponent resource errors. Exponent accumulation stops growing
once beyond256 while scanning the remaining bounded grammar; it cannot overflow a
built-in integer. Resource acceptance precedes coefficient/power construction, which
is bounded by256 mantissa digits and at most511 decimal power. Source spelling is
owned independently of exact signed calculation; no floating-point shortcut enters.
Final rounding uses nonnegative magnitude/remainder and parity, restores the sign,
and only then checks signed64 bounds before conversion, avoiding signed-minimum
negation or unsafe integral intermediates. Zero is handled before `msb`, and nonzero
4096-bit limits are applied before quotient/remainder rounding. The function does not
claim to bound earlier caller arithmetic or implement application transactions.
No further blocking code, ownership or provenance issue was found.

The original header, revision1 contract, all32 acceptance assertions and existing
credit source/tests/fixtures have an empty diff from main `cab329b`. The one additive
regression remains exact blob `347a810b1edba2ffc36d66ba44b700092bb2b381`.

Independent final-source checks:

| Check | Actual result |
| --- | --- |
| Exact numeric configure/build and label-selected CTest | exit0;33/33 pass, including unchanged32 plus the reviewed zero regression |
| Full `ctest --preset exact-numeric-debug` | exit0;36/36 pass, including three infrastructure checks |
| Credit configure/build and acceptance/regression CTest | exit0;23/23 pass; numeric-only finalizer fix does not change credit artifacts |
| Final reconfigure after shared config revision `cd96d7c` | exit0; explicitly logs Boost1.92.0 at `/usr/include`; build and33-case replay pass |
| Final project/render/Python tooling/diff checks | exit0; current snapshot and19/19 tooling cases pass |

Full-suite output was inspected; the final label-selected replay is retained locally
in ignored `build/exact-numeric-debug/reviewer-final-ctest.txt`. Local environment
remains Boost1.92.0, GCC16.2.1 and GoogleTest1.18.0; no packages were installed.

### Automatic CI configuration review

Approved workflow configuration blob `886f435f1fe0826f01e873fd697d9e1637ad3daf`
(`.github/workflows/exact-numeric.yml`) and core CMake blob
`a9c394fd98a7b08dbe515fe983c74eb3721b6be5`. The new automatic PR/main job runs the
same explicit numeric preset and all `exact-numeric` acceptance/regression cases,
without path filters, skipped failures or expected-failure inversion. Checkout and
GoogleTest references retain immutable pins, permissions remain contents:read, and
the job has a15-minute budget. Ubuntu installs the selected Boost package at an exact
version, explicitly chooses its1.83 CMake/header config directories and independently
checks compiler `BOOST_VERSION108300`. Thus installation alone is not mistaken for
proof of which headers the build consumes. CMake local discovery remains explicit
`Boost >=1.83 CONFIG` with `Boost::headers`, and the preset display/CTest labels
accurately reflect the now-passing suite. No implicit local dependency fetch is added.

This is static workflow/configuration approval plus independent local replay, not an
observed Ubuntu/Boost1.83 workflow result. The coordinator must observe that new job
and all other latest-head checks, record actual dependency/test results and reassess
any changed revision before marking CORE-001 done or merging. Required-status-check
configuration remains the separately documented repository-settings gap. Windows,
release packaging, application-level aggregate/review/atomicity, Qt server behavior,
providers, renderers/UI/localization and live/OBS validation remain outside this
bounded primitive approval.


## Final completion-record integration review

Reviewer `/root/numeric_reviewer`, 2026-10-31.
Verdict: **approved for integration** of coordinator
`39cb9a6d5c13b6a493129515a41d0fa15d5b3559`, including completion records from
`b71a72c`. This addendum changes only this review evidence document.

Inspected README/architecture/test-strategy/dependency status, the implementation
and CI handoffs, consolidated coordinator checkpoint, done-state progress/generated
HTML and acceptance map. CORE-001 completes only its bounded numeric primitive
slice; done is explicitly distinguished from PR publication/merge. NUM-01/02/04
remain partial, while the narrowly named primitive scenario is passed. Application
review/atomic aggregates, modifier combinations/clamp, refunds, providers, UIs,
platforms and release work remain explicit. No unsupported completion claim found.

The final source still has blob `a1ddb4283292a5da0a81a0f4e8255fffc19cafd7`;
original header/tests and additive regression still equal their approved hashes.
Workflow, core CMake and test harness have no diff from the reviewed `cd96d7c`.
The contract diff has only updated initial status and a paragraph framing historical
red harness/CI text; API, resource limits, error precedence, rounding and all expected
examples are unchanged. Current contract blob `f82710e7885223ec4e71fcbfd2ea7542ddbb59e4`
is approved for that status-only update; original behavioral snapshot `59d334d...`
remains identified in historical reviewed/red evidence. No assertion was changed.

Independently queried GitHub using `gh run view 41183487457 --json ...` and
`gh run view 41183487457 --log`. The actual pull_request
[run41183487457](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41183487457)
completed successfully for head `fd86a6b9e8546bf5308a26e5561be0faf49e35fd`.
The log confirms exact package1.83.0-2.1ubuntu3, compiler `BOOST_VERSION108300`,
GCC13.3.0, CMake-selected Boost1.83.0 and33/33 passing numeric cases. That head's
source/workflow blobs match the final approved source/configuration. This resolves
the previously pending actual Ubuntu/Boost1.83 primitive workflow observation; it
establishes no full Ubuntu application deployment or Windows/release support.

Independent done-state `python3 tools/check_project.py`,
`python3 tools/progress.py render --check`,
`python3 -m unittest discover -s tests/tooling -q`,
`node tools/check_progress_ui.mjs` and `git diff --check` all exited0;19 tooling
cases pass and real-browser/OBS checks remain explicitly unrun. No native test rerun
was needed for these status-only changes after the reviewed final33/36/23 passes.

At inspection PR13 was still draft at head `39cb9a6`, with new latest-head checks
pending. Final publication/review-evidence commits require another latest-head CI,
review-thread/ruleset/required-approval and actual GitHub mergeability check before
merge. The previous successful run does not waive those final-revision gates.
