# SCHEMATOOL-001 — missing ABI alias literal control review

Reviewer C independently reviewed author A's actual
`ab7c731336a56fc94ffa8cea6431468a85f2ee57`, test blob
`361a1bfc4a5b8c944ff857e779a24224735a6c4a`, author note and root's actual
dea9324 written bridge clarification. Fresh isolated note-only branch.
**APPROVED TEST SOURCE and narrow clarification. Release ONLY this new named
method's first static-control run against the unchanged auditor.** No parser
implementation, artifact proof or native runtime is approved prospectively.

I independently observed exact0b6 hosted native run42306061441: both selected
hosts stop before native execution and report the same three missing identities,
RawOps C1, Owner<RawOps> C1 and Owner<RawOps> D1. Complete log198777 bytes/SHA256
`789b97a9ee629b1613b301a2ac7bb81e58661efa895642462a91817ab01f0f8e`.
Naming identifies a possible ABI-alias category, not actual code/frame equality.

The [primary ABI naming specification, section5.1.4.3](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling-special-ctor-dtor)
distinguishes C1/C2 and D1/D2 complete/base variants; it does not certify that
these variants share instructions or a stack frame. The required additional proof
remains BOTH real ELF symbols at the SAME address and a real matched CI/SU
counterpart definition, with no missing/unknown/zero frame substituted.

The constructor fixture removes ONLY the existing triangle C1 node, retaining
entry→C1 and C1→C2 edges. The independent destructor fixture likewise retains
entry→D1 and D1→D2 with its real framed counterpart. Literal symbol maps require
addresses0x100/0x200 and exact frames16/24; expected canonical graph entry→body
and total48=16+8+24 are independent of the audited implementation. The literal
addresses test proof requirements; they do NOT prove any hosted ELF alias.
Both original edges are retained as input evidence. Only a physically proved
same-body alias bridge may canonicalize without inventing another call/frame.

Root's actual dea9324 note explicitly resolves the earlier missing-caller
shorthand: the missing alias MAY be source only of its exact C1→C2/D1→D2 bridge.
An ordinary missing caller or other missing-alias outgoing edge still refuses.
This is a coherent compiler-representation refinement within the existing fixed
proof, with no public/native ABI or scope change; no further owner decision or
public contract revision is needed. The future parser must enforce that precise
bridge rule. Generic orphan acceptance or dropping an unproved edge is not
approved; actual binary recursion must still fail the union-graph cycle check.

Each family has ten independent refusal mutations: unequal or absent ELF symbols,
absent counterpart CI/SU, mismatched SU, ordinary missing caller, unknown target,
non-_ZN prefix and multiple ambiguous ABI tokens. Expected values are literal;
no candidate callback, successful native result or verifier-generated oracle is
used. After removing only this one undecorated method, I obtain the entire old
module AST, including all six existing methods/helpers/assertions. Auditor091f
is byte-identical; load_tool loads definitions only and the selected test calls
pure metadata/graph/stack functions, never main/artifacts/proc/binutils/native.

The selected command is minimal env3 Python -I -S -B
`tests/schema_validation/native_owner_audit_controls.py`
`NativeOwnerAuditControls.test_missing_abi_alias_node_requires_same_elf_counterpart_frame`.
One method can report TWO positive subtest errors because subTest continues
between constructor/destructor; the current expected cause is the actual parser's
missing-node RuntimeError at those positives. Import/fixture errors cannot count.
Negative cases require full execution after any later separately reviewed fix.

Actually performed: complete actual source/fixture/contract/clarification and
primary-ABI reading, exact old-AST/auditor preservation, literal edge/48 arithmetic,
AST3.10/compile-without-execution, project426/122/render/diff PASS. No test/checker
import or execution, compiler, ELF load, SDK, worker, thread, child or probe ran.
Actual reached red, separate parser source review and current matching local/
hosted proofs remain required; only PR77 is in scope, then stop.
