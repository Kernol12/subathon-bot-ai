# SCHEMATOOL-001 — bounded scalar zero initialization review

Reviewer C independently reviewed author B's exact
`7a1311705ad31fd198bc6778d1dced4fe6edf71d` from e1fabcc, on a fresh isolated
note-only branch. Reviewed FaultOps blob
`2f772f7be7d36552b6e53613e2d329d3f3e6a97e`, fixture blob
`3e5edd8546f031b969d849bfbbc5c387c890a1d6` and its author note.
**APPROVED SOURCE for integration, rebuild and static inspection only.**
No actual generated output or runtime is approved by these source changes.

The sole constructor change removes the six-pointer argv member's implicit
zero initializer and writes all sizeof(array) bytes through a bounded volatile
unsigned-char loop before any use. The same six typed assignments immediately
follow; all other member initialization and constructor actions are exact.
The existing ABI asserts 8-byte pointers. I independently read archived old
fault.disassembly:574476 bytes/SHA256
`44b0714b9f2a1d42bebea2534caeb07c523930099b7a69de1156a7b1d80bacd1`.
It proves the previous compiler actually wrote the same48 zero bytes before
those typed assignments. No portable all-platform null-pointer representation
claim is needed, and the sixth slot is still explicitly assigned nullptr.

The only wait_pinned change writes every sizeof(siginfo) byte to zero in the
same do/while body before each real waitid, including every EINTR retry. The
selected owner profile asserts siginfo_t128; the archived old code clears the
same128 bytes. Both changes use permitted character access to object
representation and preserve volatile observable stores; see the primary
[C++ object representation rule](https://eel.is/c++draft/basic.types.general#2)
and [volatile abstract-machine rule](https://eel.is/c++draft/intro.abstract#8.1).
These language rules support the source operations, not a claim about an
unobserved compiler's instruction selection.

No new helper, allocator, syscall, callback, ownership transition or atomic
policy is introduced. Strict SIMD/TLS/indirect/trap/profile rejection stays in
the generated-code gate. New actual .ci/.su, complete reachable instruction
graphs and total stack bounds remain required after rebuild. This is a scoped
source correction to satisfy that gate, not a relaxation of its opcode policy.

I independently restored both entire previous files byte-for-byte by inverting
ONLY these two initialization changes. R3 header/core, old57 grammar and the new
normalization regression remain exact f986622e/058d9d12/b223e74f/08e3b3da.
The three normalizer work=false contexts and known classification defect remain
unchanged; source admission, new reached red and later behavior fix stay separate.

Actually performed: actual diff/body/order/ABI and archived-disassembly reading,
exact source inverse and four unchanged-blob checks; clang-format dry-run/Werror,
project426/122, render and diff checks PASS. No candidate import, compiler,
checker/control execution, artifact load/call, SDK, worker, thread, child or probe
ran. This releases root's rebuild/static stage, not owner runtime, passing full
acceptance, hosted capability, task completion or PR77 merge.
