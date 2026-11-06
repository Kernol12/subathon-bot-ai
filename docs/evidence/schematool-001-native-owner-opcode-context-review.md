# SCHEMATOOL-001 — optional base audit opcode predicate

Reviewer C independently reviewed root's actual
`3631b70b815567d5227ebfc217b0ec4eff94a488`, base auditor blob
`248796f1782e37eed774b7b0e0394f7d6408ebd0`, and its written scope note.
**APPROVED SOURCE: the optional predicate API is bounded as proposed.**
This change alone admits no new instruction, artifact, owner behavior or runtime.

`extra_instruction=None` preserves the old default. Only a supplied predicate's
truth value can additionally satisfy the existing opcode-admission clause.
The exact closed-function gate, nonempty instruction inventory, instruction
boundaries and TLS/segment rejection remain independent; TLS is checked before
the predicate. Direct target resolution, instruction-interior rejection,
closed-function traversal, indirect/unresolved transfer rejection and trap
rejection still follow. The predicate cannot short-circuit any of those gates.
The existing exact shared-wake LOCK-add exception is unchanged. No concrete CAS
context, broad LOCK/SIMD allowance or user configuration is added here.

I independently restored the previous complete file byte-for-byte by removing
ONLY the added argument, optional predicate assignment/comment and its one
opcode-clause alternative. No unrelated parser/ELF/libc/export/stack logic changed.
Actual future owner-specific predicates require different source/oracle review
with exact function/width/register/address contexts and negative controls;
this base approval cannot stand in for that review. All prior mandatory controls
and actual per-artifact generated/resource gates remain required.

Actually performed: full actual delta and surrounding control-flow inspection;
stdlib AST3.10/compile-without-execution and complete old-file byte-preservation
check; project426/122, render check and diff check PASS. I did not import/call a
checker, control test, candidate module, artifact, worker, SDK or native function;
no compiler, binutil, thread, child or probe ran. This source can integrate with
the separately reviewed owner checker/control packet. Execution permission remains
subject to that complete packet's actual independent review; native runtime and
PR77 completion/merge gates remain open.
