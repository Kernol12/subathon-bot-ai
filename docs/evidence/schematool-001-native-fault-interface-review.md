# SCHEMATOOL-001 — private fault interface source review

Reviewer root, different from author B, 6 November 2026. **APPROVED exact minimal
declaration source only** at dce1b5625d81e715d9ef44946ab0e1a9b9091600. No fixture
body, owner algorithm, native capability or execution is approved by this note.
User scope remains finish PR77 then stop.

Actual transport_core_v1.h blob3165820afd976a3af079f65c8781eab5ff13811a and
native_fault_v1.h blob105bb1df026021f2cecec69d887d3724d8c33b06 were read in full
through immutable git show. Compared against actual native contract aec16d6,
independent C44/C48 document decisions, plan587 and its different C26530af review.

PathView is one borrowed const-byte pointer/unsigned length; Request contains
exactly manifest/interpreter/worker/SDK views. NativeWord/RawResult are fixed
64-bit scalar representations, not resource/Python/validation objects. The closed
KernelCall tags cover only required raw operations. All19 phase bases, two edges,
three continuations and ten observation tags match the reviewed contract. No
domain outcome or accepted-result injection was added.

The TransportOps concept checks unevaluated noexcept static-policy call shapes;
it provides no method body or runtime dispatch. The constrained common owner is
DECLARED ONLY. No resource acquisition, constructor, callback, allocator, decoder,
trampoline or substitute owner is implemented. Future source must separately
prove every actual operation traverses the approved policy and every used tag
specialization matches; checking read alone does not establish that proof.

The test header contains exactly the four C exports, precise pointer/int/uint
types, C11/C++ width assertions, default symbol visibility and conditional
extern-C/noexcept. No fifth helper or public outcome exists. It is test-only;
production retains the exact v1 ABI and literal31 source unchanged.

This source review permits separately assigned nonvalidating harness and test
SOURCE AUTHORING against these types, under the already approved plan. Actual
body/oracle/build review still precedes any execution. Real fixture foundations
and reviewed intended missing-owner red precede owner implementation. It does
not waive the current foundation-source cleanup and generated-code audit findings
from C's independent299 review, nor the later74 ports/source/full CI/merge gates.

Actual verification: complete source/contract comparison and existing project/
render/whitespace consistency checks. No C++ compilation, candidate import, load,
probe, test, child/thread/signal, installation or live operation occurred.
