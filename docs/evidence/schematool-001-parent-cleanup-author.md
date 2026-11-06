# Parent cancellation/allocation regression authoring — SCHEMATOOL-001

Author `/root/session_supplier_author`,6 November2026. **AUTHORED; NOT EXECUTED OR
INDEPENDENTLY REVIEWED.** Existing isolated parent branch/worktree
`task/SCHEMATOOL-001-parent-author` / `/tmp/subathon-schematool001-parent-author`,
starting at actual parent3e3f4f350b78f7cfcc11e2d55a6ee1b8adb7b5c4. Only NEW
parent_cleanup_regression.py and this evidence note change in this test commit.
Parent source remains EXACT `a25d531ea70271a8434f2a4206b5bff25f444d45`;
all frozen64 tests remain EXACT `64f4911edae1e6ec1546777661f199252d06131c`.
No production correction or weakened old assertion precedes reached-bug red.

## Actual different source findings and contract oracle

Different source reviewer C found:

1. _worker_result's finally loop closes descriptors BEFORE child kill/wait without
   an encompassing cleanup mechanism. A KeyboardInterrupt/BaseException during a
   close aborts remaining closes and skips kill/reap. API§4 expressly requires the
   same cleanup on cancellation while Python can unwind, with no fabricated result.
2. validate_package's first _bounded_path/UTF8 allocation occurs OUTSIDE its
   MemoryError handler. A known initial allocation refusal escapes rather than
   returning the API/logical resource_limit. The ordinary exact path input need
   not reach installation, package access or a worker for this refusal.

Root explicitly authorized these TWO narrow regression seams and required different
actual test review and observed reached-bug red BEFORE parent correction. Read
scoped tests instructions. C and author coordinated source design read-only: real
Linux close MUST happen BEFORE the same one-shot cancellation; record only the two
parent pipe pairs before entering Popen (native Popen may use its own pipe).
This is no new public callback/option/factory, standards-engine or successful worker.
Root retains contract/shared-test/configuration/progress ownership.

## Actual two-case source and independent literal expectations

The new stdlib-only module defines ParentCleanupRegression with TWO ordinary named
methods, lazy explicit trusted-target loading in setUpClass and no module-load
fixtures/processes/SDK activity. It works with isolated -I -B absolute-file execution.
No baseline test/helper is imported or changed.

- test_cancelled_descriptor_close_reaps_and_closes_before_propagating records REAL
  os.pipe pairs, then intercepts the frozen Popen argv to launch a REAL tiny isolated
  fault child. It writes malformed activation B, acknowledges test readiness, and
  blocks on a separate held pipe while keeping both protocol writes open. It never
  emits ok, a domain result or a fake validating callback. A kernel select bounds
  readiness; a real5-second timer kills ONLY and marks failure if fired, never reaps.
  A root-authorized one-shot os.close interposer calls saved real_close on the first
  parent READ descriptor, THEN raises the SAME KeyboardInterrupt object. Native
  Popen's internal pipes and test synchronization pipes cannot trigger it.
- After the public call, capture real waitpid(WNOHANG) and ALL4 parent fstat states
  BEFORE test poll/wait/close/kill cleanup. Require original cancellation identity,
  one actual triggered close, watchdog not fired, child state parent-reaped (precise
  ChildProcessError), all4FDs closed and no public logs. A still-running or waitable
  unreaped child fails. Capture both child and FD evidence before assertion so the
  genuine old failure can show the remaining descriptor too. Finally fallback
  cancels/joins timer, closes owned test/remaining parent descriptors and kills/waits
  a child only AFTER the oracle. Test cleanup cannot manufacture parent success.
- test_initial_path_allocation_refusal_is_resource_limit_without_setup injects one
  specific MemoryError only at existing private _bounded_path for an ordinary exact
  builtin manifest string. Require the exact resource enum and one initial helper
  call, with installation/Popen/builtin/io/os open guards untouched and no logs.
  These guards raise AND assert_not_called after return, preventing swallowed guard
  exceptions from masquerading as success. This is allocation containment, not
  successful validation, a different number algorithm or child resource proof.

Expected OLD-source failures are an actual reached cancellation/cleanup assertion
(child still running/FD left open) and the actual specific
MemoryError INITIAL_ADMISSION_ALLOCATION_REFUSAL from initial admission. Those are
PLANNED oracles, not observations. If readiness/setup/import/source mismatch or
another cause fails, preserve it and do not call it the intended bug red.

## Exact intended first reached-bug command and gates

After DIFFERENT C actual source/oracle review, root/author may run exactly:

```sh
env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8 /tmp/subathon-schematest001-py314/bin/python -I -B /tmp/subathon-schematool001-parent-author/tests/schema_validation/parent_cleanup_regression.py
```

cwd `/tmp/subathon-schematool001-parent-author`. Capture actual return/stdout/stderr
and source/test blobs; inspect actual triggered source frames/child/FD diagnostics.
This is TWO NEW focused regressions against actual NEW parent behavior, never an
original baseline replay or an inversion of failure. No successful fake worker or
unconditional green claim. Do not alter production before that actual reached-bug
run. Then fix only owned parent source, retain assertions and run these regressions
plus appropriate parent checks; separate actual code review remains mandatory.

Actual checks before this test commit: stdlib Python3.10-feature ast.parse and
compile WITHOUT exec/import PASS;2 named test definitions. Source/test SHA guards
confirm parenta25 and frozen64 source64f unchanged. No test/helper/candidate import,
regression body, child/readiness/timer/probe, clone/SDK or install was executed.
Project/render/diff checks are recorded with the committed handoff; no shared write.
Full64 real acceptance, worker source/integration/offline/RLIMIT/no-site/read-race/
allocation review and ordinary latestCI/task-completion gates remain explicit.

## FIRST actual two-case reached-bug red, before correction

Different actual C test approval `adc171654905a40335c410dba2d3d0ad620b3ed7` was
read BEFORE execution. At clean8460c85ddac1c03f3c7267deda2e23600e2963d3, actual
parenta25/testacde, run ONLY the exact new-module command above at
2026-11-06T12:09:59.871683+00:00. Native exit1,2 tests0.012s,1failure+1error,
external0.1136749479919672s, no timeout. Original64/FIRST artifacts were not replayed.

Actual cancellation case passed same-object/one-trigger/watchdog checks, then
failed on child_state running instead of parent-reaped; independently captured
FD states were ['closed','closed','open','closed']. Test fallback cleanup ran ONLY
after that real oracle. Actual allocation case escaped MemoryError with exact
INITIAL_ADMISSION_ALLOCATION_REFUSAL; native traceback shows public initial
_bounded_path admission at tools/schema_validate.py line289 outside its handler.
No setup/import/readiness/SDK cause substitutes for either observed failure.

Original bytes/receipt retained BEFORE interpretation at:

- /tmp/schematool001-parent-regression-first-red.stdout.txt (0bytes,
  SHA256e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855)
- /tmp/schematool001-parent-regression-first-red.stderr.txt (2827bytes,
  SHA2562041dcb227306e66accca6c80886d94d7779709dc48af174cb6f8731c0912cb2)
- /tmp/schematool001-parent-regression-first-red.receipt.json (exact argv/cwd/
  minimal environment/source/test/review/time/status/hash record).

The stdlib external observer wrote files directly, imposed30s solely as a test
observation bound, preserved actual native failure1 and did not invert it. This is
an AUTHOR observation pending different attribution/source-fix review. Root may
integrate raw files into its owned durable evidence paths. These actual reached
bugs now authorize parent correction, preserving both regression assertions.
A separately identified possible Popen-constructor cancellation ownership window
remains unresolved for source-based investigation after this coherent fix; this
new-case result proves neither that window nor all cancellation/resource controls.

## Actual correction and targeted passing checks

Only AFTER that actual red was recorded in commit3d6bed7, correct owned parent:
initial exact path/UTF8/URI admission now lies inside the public MemoryError catch.
Owned-descriptor cleanup defers a close-time BaseException, continues draining,
and uses an enclosing finally for child kill/wait. Transient cancellation during
kill/wait is deferred until reaping; preserve an already propagating exception,
otherwise re-raise the SAME deferred interruption after cleanup. OSError cleanup
refusal still cannot become successful validation. No test assertion changed.

Corrected parent blob `1579604c8d50f08e7320c9d7723246fd2ddd3df7`:
actual SAME two-method command PASS2/0.009s/exit0. Actual unchanged eleven parent
methods listed in parent-author note PASS11/0.223s/exit0 with the same selected
CP314 -I -B/minimal environment. No skips/errors/failures. New regression source
remainsacde40d; frozen64 source remains64f4911. These are targeted author checks,
not independent code approval or FULL64 package acceptance. Python3.10-feature
AST/compile WITHOUT exec PASS. Project/render/diff checks accompany handoff.

C's separate constructor-ownership caveat remains OPEN: a KeyboardInterrupt after
OS spawn but before Popen returns/assignment may leave caller process=None. This
coherent two-bug fix does NOT claim that window solved or dynamically tested.
Root explicitly requested source-based investigation after this handoff and a
separately reviewed meaningful regression before any constructor fix. No further
production change, signal handler/mask change or new knob is authorized by these
passing two cases. Worker/full acceptance/source/integration/ordinaryCI gates stay.

Root integration preserves the exact observed raw [stderr](schematool-001-parent-cleanup-first-red.stderr.txt), [stdout](schematool-001-parent-cleanup-first-red.stdout.txt) and [receipt](schematool-001-parent-cleanup-first-red.receipt.json) byte-for-byte. No replay. This integration link does not approve the author's own code or the separate Popen constructor ownership gate.
