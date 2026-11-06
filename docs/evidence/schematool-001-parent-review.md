# Independent parent audit — bounded two-fix verdict, remaining spawn gate

Reviewer `/root/session_plan_review`, DIFFERENT from parent author
`/root/session_supplier_author` and integrator `/root`,6 November2026.
**APPROVED exact TWO corrected paths and reached-bug attribution ONLY**, actual
candidate `7850a7106fb83931942b520602002698b9938ab5`, parent blob
`1579604c8d50f08e7320c9d7723246fd2ddd3df7`. Overall parent/worker implementation
approval is PENDING the explicit constructor-ownership gate and real integration.

NEW original review branch `task/SCHEMATOOL-001-parent-review`, worktree
`/tmp/subathon-schematool001-parent-review`, from root23b6700/author3e3f4f3. Read full
actual initial320-line parenta25, contracts/source, actual785 correction diff/author
evidence and preserved regression artifacts through immutable git/root900 objects.
Do not mistake the preserved review-tree sourcea25 for this exact verdict target.
ONLY this note changes for this verdict; CI has its separate bounded review.

## Source findings, independent tests and actual reached red

Initiala25 had TWO concrete defects reported before correction:

1. _worker_result finally drained descriptors THEN killed/waited without an
   encompassing cleanup mechanism. A close-time KeyboardInterrupt could skip the
   remaining FD and live child. Frozen API§4 requires cancellation cleanup.
2. validate_package's first bounded-path/UTF8 admission was OUTSIDE its MemoryError
   handler, so a known initial allocation refusal escaped rather than resource_limit.

Root assigned the new separate parent_cleanup_regression.py, exact testacde40d,
with actual different reviewadc171654905a40335c410dba2d3d0ad620b3ed7 BEFORE execution.
The author used the frozen Popen/real-child boundary plus root-authorized one-shot
real-close-FIRST interruption, captured waitpid/all4fstat BEFORE fallback, and the
narrow initial private-allocation fault. No successful validating fake exists.

Independently read COMPLETE original stderr/stdout/receipt committed by root900:
stderr2827bytes, SHA256
`2041dcb227306e66accca6c80886d94d7779709dc48af174cb6f8731c0912cb2`, stdoutempty.
Native2 tests0.012s, actual exit1, ONE failure and ONE error, no timeout. Actual
receipt starts12:09:59.871683Z at clean8460c85, parenta25/testacde, AFTER test review
adc12:08:02Z. No original64 or historical FIRST was replayed.

The cancellation case passed SAME-object/one-trigger/watchdog checks, then its
actual assertion reported child_state running rather than parent-reaped and captured
FD states ['closed','closed','open','closed']. This is the reached parent cleanup
defect, not test cleanup manufacturing success. The allocation ERROR contains exact
MemoryError INITIAL_ADMISSION_ALLOCATION_REFUSAL through public validate_package
line289 and the intentional mock failure point. It is the missing containment
boundary, not spontaneous setup/import/dependency/allocator exhaustion. No readiness
or other cause substitutes. Raw summary/hash/source/review fields independently match.

Red was recorded in3d6bed7 at12:12:14Z BEFORE production correction785 at12:14:14Z.
Both NEW assertions and frozen64 source are byte-identical through the correction.
These artifacts satisfy the focused reached-bug gate; no replay ran in this review.

## Exact correction and broader source audit

The corrected public try now encloses initial path/type/UTF8/URI admission as well
as installation/worker setup. Known initial MemoryError returns the preexisting
resource_limit enum before setup/IO, preserving exact invalid-path admission and
callback refusal. No alternate success path, profile or expected result changes.

Descriptor draining records a close-time BaseException and continues through other
owned FDs. Enclosing finally reaches child kill/wait even when cleanup is interrupted.
It preserves an already unwinding exception, otherwise raises the SAME deferred
interruption after cleanup. Linux close removes ownership before the syscall and
does not retry a possibly released FD number. The reviewed one-shot real-close
cancellation therefore cannot skip the other FD or returned child's reap.

Source-only AST comparison finds ONLY _worker_result and validate_package changed;
all other functions and public signatures remain exact. The author reports unchanged
new2 PASS0.009s/exit0 and unchanged selected parent11 PASS0.223s/exit0 at exact157.
Those are AUTHOR targeted observations, not independent full64 acceptance, actual
constructor proof or a test rerun by this reviewer.

The original whole-parent audit also examined exact builtin path/argv checks before
callbacks/copying, help without feature operations, enum/closed CLI/status mapping,
lazy sysconfig three-key guard, actual ordinary venv/ABI/one-SDK binding and no parent
package/SDK imports. Protocol owns TWO-byte activation and257-byte outcome buffers;
readv fills borrowed memoryviews, rejects excess/prefix/unknown/mismatched statuses,
waits for both EOFs and normal child exit, and allows only the exact empty-A env3
nonvalidating exception. Deadline is from immediately before Popen through reads/
exit; unknown signals do not imply memory exhaustion. These checks do not prove
an arbitrary worker's standards behavior or every unsupported host primitive.

Primary [Python readv](https://docs.python.org/3.10/library/os.html#os.readv),
[poll](https://docs.python.org/3.10/library/select.html#poll-objects),
[memoryview](https://docs.python.org/3.10/library/stdtypes.html#memoryview) and
[Linux close](https://man7.org/linux/man-pages/man2/close.2.html) support the selected
bounded POSIX approach; archive/installation admissions remain their separate reviews.
No package bytes, arbitrary exception text or diagnostic payload is returned.

## Explicit unresolved constructor window and integration gates

A DIFFERENT source-backed caveat remains OPEN: Popen can raise after OS spawn but
BEFORE assignment returns a handle, leaving caller process=None. Read exact
[CPython3.10.12 subprocess](https://raw.githubusercontent.com/python/cpython/v3.10.12/Lib/subprocess.py)
and [3.14.9 subprocess](https://raw.githubusercontent.com/python/cpython/v3.14.9/Lib/subprocess.py):
constructor exception cleanup of its streams is not a promise to kill/reap an
already spawned process. The two approved fixes cover an owned/RETURNED child,
not that constructor window or arbitrary constructor MemoryError. Root has assigned
separately reviewed native-spawn/SIGINT/allocation regressions before any constructor
production change. This verdict MUST NOT be expanded into all-cancellation approval.

Current parent/worker source review, real full64 plus NEW regression passing
acceptance, final ordinary CI registration/promotion and fresh selected-host results
remain required before taskdone. Missing worker/full-suite setup failures are not
acceptance; no full64 was run for this source-only audit. Worker resource/ABI/no-site/
secure-read/offline/graph-key/allocation and source/integration gates remain. No HTTP/
CONTRACT013/provider/live/platform/full-release capability is claimed;7 integrations
and3 FX providers remain.

Actual checks: Python3.10-feature AST/compile WITHOUT exec, two-function delta,
unchanged regression/frozen64 hashes, original raw/receipt/frame/diagnostic/time
attribution and exact diff check PASS. Earlier actual project426/122/render checks
were green. No source/SDK/test/helper import or execution, fault child/clone/install,
historical replay or GitHub write occurred; only this bounded review note changes.
