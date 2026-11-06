# SCHEMATOOL-001 — owner proof integration in both static gates

Reviewer C independently reviewed root's actual
`b55972ea36d2926c985fe1842e7599898d191e94`, on a fresh isolated note-only branch.
**APPROVED SOURCE; root may execute the two combined static auditors on the
matching build outputs.** No candidate ELF or native runtime is released here.

Exact reviewed blobs: base auditor `230406964f63ecc1641ab746dd54bab8083a38ff`,
fault auditor `3ec958f662a950fd1b58b2b922ed8d6573a97420`, workflow
`b756e408defb77b607b29c6d6722bf6cfc6e75b5` and the written integration note.
The independently root-reviewed owner checker/control refinements remain their
separate source admission; this review checks the actual integration edges.

Each former literal-entry-only clause is replaced by the correct per-DSO owner
proof: production for the production artifact, fault for the fault artifact.
The fixed sibling audit_native_owner.py is loaded under a non-main runpy name.
Only the matching audit_artifact API is then called, with the actual artifact's
resolved build root. That API requires the selected role's exact filename and
fixed compiler-TU CI/SU inventory. Its own lazy base load defines functions under
another non-main name; no main/owner_proof runs while definitions load, so this
is not recursive proof execution. Missing file/API/role/metadata and proof
exceptions propagate normally. No candidate artifact is dlopened or executed.

I inspected the entire call chain. The matching owner proof retains closed
reachability, actual ELF-versus-compiler edges, fixed stack forms and GNU CI/SU
agreement, acyclic inherited chain bound, actual clone zero/return edges,
child-only closure, exact exports, eager binding and C++ initializer checks.
It supplies a structured result, rather than accepting a badge/string. A
standalone invocation of either old auditor therefore cannot omit owner proof.

All former helper/primitive/negative-child/raw-success/probe/PLT/GOT/libc checks
remain before or after that proof. I independently compared ASTs: every old
function except the intended main callsite is unchanged; only owner_proof is
new. The existing reachable default and strict TLS/target/trap checks are exact.
All additional export/eager/no-initializer checks in the old scripts remain.

The workflow adds one unconditional minimal-env `-I -S -B` five-control step
before the first owner proof. Independent removal of that exact four-line block
restores the entire previous workflow byte-for-byte. Existing eight/eight/five
controls, both combined static gates, exact one-case XML bootstrap, strict
inventories, environments and subsequent unconditional commands are preserved.
This is source approval of the integration, not permission to publish/run those
later native stages while the remaining runtime gates are open.

Actually performed: complete actual diff/surrounding definitions/loader callchain,
matching-role/metadata-path and CI-order audit; AST3.10/compile-without-execution,
old-definition and whole-workflow preservation checks; added command bash-n;
project426/122/render/diff PASS. No checker/control or candidate import/call,
compiler, ELF load, worker, SDK, thread, child or probe executed by this reviewer.
Actual both-artifact combined proof, hashes, no-child bootstrap and narrow
new-normalization reached red still need their own observed evidence/admission.
Whole implementation approval remains withheld for the known work=false defect;
no task completion, hosted capability or PR77 merge approval is implied.
