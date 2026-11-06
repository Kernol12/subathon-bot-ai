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
