# SCHEMATOOL-001 — active normalization-close interruption fix

Author B, 6 November 2026. Root explicitly released this fix AFTER the reviewed
new regression reached its intended real behavior failure. Fresh isolated branch
`task/SCHEMATOOL-001-native-normalization-fix`, worktree
`/tmp/subathon-schematool001-native-normalization-fix`, starts at actual
`ae97dea38fee530b5f7ba6bc2d7e81c2dcc9ac7f`. Only the common-owner implementation
header and this note change. PR77 only, then stop.

## Existing test-first evidence, not a replay

Read actual root/tools instructions, native R3, the reviewed normalization profile
and [actual regression source review](schematool-001-native-normalization-close-regression-review.md),
then C's [matching local artifact admission](schematool-001-native-owner-generated-review.md).
That admission covers production `e5579584...`, fault
`e1bb573d0ca3a6d771a2644838113c343b3600bd468a97edad36436483a64f76`, matching
metadata and the ordered prerequisite stages; it explicitly withholds approval
of the known normalization bug or a future fix.

Root reports the strict one-case XML bootstrap and full56 prerequisites passed
before the one-method regression. I independently read the preserved original
`/tmp/schematool001-normalization-first-real/{receipt.json,stdout,stderr}` and
matched both stream lengths/hashes to the receipt. The correct CLI includes
`--artifact`; revision ae97dea, UTC `2026-11-06T21:55:52.494014+00:00`, exit1,
stdout0 bytes, stderr1087 bytes. It reports exactly one test/one failure in0.180s
at line89, `AssertionError: 0 != 18`. All preceding assertions for genuine
close0/release-before-AFTER41, owner-FD EBADF and own/group P_ALL cleanup passed.
This is a reached classification failure, with no fixture/import/dependency/
missing-marker cause. Root separately identified its earlier missing-`--artifact`
argparse exit2/zero-test invocation as NOT a red; it is not used to authorize this
fix. No historical or new test was replayed by this author.

Original raw SHA256 values:

* receipt: `cc179aebb871f9f32b6f01afa4b8ded96e8dde7ad7451dc7a7a8716839b61e1d`
* stdout: `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`
* stderr: `5f4706f7965452458f2cef31a9c2781d15b5f939707bcfc05131af4fb9870c51`

Root owns durable raw/result attribution, build/CI/test registration and integration.

## Exact bounded correction

`tools/schema_validation/native/transport_core_v1_impl.h`, blob
`e89a093c781136cac5e16e55ba98b97d4cc746c0`, changes ONLY the three normalization
work-context literals from false to true: BEFORE40 checkpoint, genuine original
close, and AFTER41 checkpoint. Its adjacent comment now accurately describes
active work instead of instructing preservation of the already-observed bug.
The R3 actual-close/invalidation/release-ACK ordering is unchanged, as is the
source-duplicate AFTER hook before the close. The existing classifier can now
retain native interrupted18 for both a real active-close EINTR and its reviewed
AFTER interrupted continuation. The duplicate is still moved into its owned slot
before the existing decision check, then normal cleanup runs; no child is needed.

`close_bare`'s cleanup default remains false. Every other type, statement, method,
cleanup call, ownership transition, raw result, resource-priority rule, phase,
deadline and final-commit path is exact. FaultOps/fixture, original57 plus new1,
worker/public API, build/pins and all contracts are untouched. No test or oracle
is weakened to obtain a pass.

## Actual checks and remaining gates

A stdlib source-inverse check restores the exact previous header after reversing
only those three booleans and the comment. It also confirms the cleanup default
false remains. Original red stream/receipt hash checks pass. Clang-format
dry-run/Werror, project426/122, current progress render and staged diff check PASS.
No compiler/configure/link/load, candidate/test/SDK import, native call, thread,
child or signal action occurred in this assignment.

Different actual delta plus whole-source review must precede root rebuild and
matching generated/combined-static admission. Root must then observe this SAME
regression pass and all applicable unchanged acceptance/facility/hosted gates.
This authored source makes no prospective pass, task-done or merge claim.
