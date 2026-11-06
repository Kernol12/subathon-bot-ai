# SCHEMATOOL-001 — hosted read-only comparison control review

Reviewer C independently reviewed root's actual
`7a1f2b7`, control source `7d7e945`, and written reason on a fresh isolated
note-only branch. **APPROVED TEST SOURCE; release ONLY the new named literal
control's first run against the unchanged auditor.** No checker correction,
artifact/native execution or future pass is approved here.

I independently queried exact published ba5366e native run42304624411, jobs
128190707206/128190707589, and downloaded the full log196595 bytes/SHA256
`c4305e83a2a974c292622ec831867ad87da2dc8574d869e2fb98cf8b1014274c`.
Both selected hosts build and pass preceding literal controls, then fail at the
production owner fixed_rsp guard: `cmp %r11,%rsp` is classified as a write.
Every subsequent bootstrap/discovery/facility/full66 step is skipped by the failed
prerequisite, so NONE of those native stages ran. This is an actual static guard
classification error, not a kernel/product failure or hosted acceptance.

The new method uses independent instruction literals. Exact CMP reads both
register operands and changes flags; its explicit second register is not written.
It must therefore accept both the reached stack-pointer comparison and a frame
comparison. Five negative rows preserve rejection of register MOV/SUB/XCHG/
CMPXCHG to RSP and MOV to RBP. Exact CMP is distinguished from writing CMPXCHG;
no opcode-prefix allowance is implied. Existing stack size/metadata/profile and
all default transfer/TLS/child constraints remain unchanged.

I independently removed ONLY the new undecorated method from its AST and obtained
the complete previous module AST, including all five existing tests/helpers and
assertions. The auditor file itself is byte-identical. load_tool imports only its
reviewed non-main definitions; this test calls only the pure fixed_rsp function.
It cannot run artifact main, binutils, proc inspection, native code, SDK or child.
The intended new red is the RuntimeError at the exact positive CMP call; a fixture
or import error cannot count. After that actual red, a separately reviewed exact
CMP-only correction and all six control passes still precede artifact checks.

Selected command scope: ordinary minimal env3 Python `-I -S -B`
`tests/schema_validation/native_owner_audit_controls.py`
`NativeOwnerAuditControls.test_stack_pointer_comparison_is_read_only_and_writes_still_refuse`.
No old test or hosted run is replayed by this approval. Actual checks performed:
full source/diff/literal/oracle and hosted failure analysis, AST3.10 plus
compile-without-execution, old-module/auditor preservation, project426/122,
render/diff PASS. No candidate import/test/checker, compiler, ELF load/native
call, SDK, thread or child executed by this reviewer. PR77 only, then stop.

## Actual reached control red and exact CMP-only correction

Reviewer C independently reviewed root's actual `6fcfcd0`, checker blob
`5c1ebe2`, written reason and full original selected-control receipt/raw.
At clean85c8952132f327ab364e0c3cd45ba896f9fd965a, UTC22:29:11.608675,
the one approved named method executes/.008s/exit1/stdout0. Raw1819 bytes/
SHA256c555966f17254b2e47ceb7ec18e729e27d4745b42d81c60e269088edfd235d90
matches receipt. It reaches the actual pure fixed_rsp at its first literal
positive CMP and raises the exact hosted unknown-stack-write RuntimeError.
Imports and definition loading succeed; no artifact/helper/setup cause is used.
This is a genuine reached auditor bug, not a native feature red or a retry of
hosted code. Later negative rows were not reached in this first error and must
run after the correction.

**APPROVED SOURCE.** Exactly two added lines skip stack-write classification
ONLY for mnemonic equality `cmp`. I independently removed them and obtained the
entire previous checker byte-for-byte. Full six-control source is unchanged.
CMP reads its explicit operands and writes flags, whereas CMPXCHG and every
forbidden MOV/SUB/XCHG/RBP/RSP write still traverse the existing checks. No broad
prefix, new base opcode, variable-stack form, budget or instruction/transfer/TLS/
child guard is allowed. No native source, ABI, compile flag or dependency changes.

Root may rerun the SAME selected regression, then the FULL six controls; only if
all pass, run both reviewed combined static artifact gates on matching current
outputs. Preserve actual results and stop on any failure. On passing these and
applicable repository checks, ordinary diagnostic draft publication is cleared
to obtain fresh both-host CI; ba5366e's skipped native stages are not promoted
into success. Actual latest-hosted static/bootstrap/full56/full66 and SDK14
results remain required before normal merge77 and stop.

Actually performed: complete diff/context/source inverse, actual raw/count/cause/
hash analysis, AST3.10/compile-without-execution, unchanged control-source check,
project426/122/render/diff PASS. No candidate/checker/test import or execution,
compiler, ELF load, worker, SDK, thread, child or probe ran in this review.
