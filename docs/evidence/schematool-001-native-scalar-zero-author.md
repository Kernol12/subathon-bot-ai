# SCHEMATOOL-001 — scoped scalar zero-initialization correction

Author B, 6 November 2026. Root released SOURCE ONLY on a fresh isolated branch
`task/SCHEMATOOL-001-native-scalar-zero`, worktree
`/tmp/subathon-schematool001-native-scalar-zero`, from actual regression/hook
`e1fabcca75b0733ccac997c3a125b5ad36a29636`. Only the two test-policy/fixture source
paths below and this note change. PR77 only, then stop.

Read root/tools/tests instructions, actual R3/profile, prior codegen source/reason,
and C's actual `18381930abc26d484790d47ff63cb71eb3d04776` review from root. That
review permits a root rebuild/static inspection of its exact earlier delta; it
does not approve this new source, owner runtime or the normalization behavior fix.

## Actual input and bounded delta

Read the preserved compile-only archive, without executing an input ELF:
`schematool-001-native-owner-compile-only.tar.gz`, member
`schematool001-owner-static-stage/fault.disassembly`, 574476 bytes, SHA256
`44b0714b9f2a1d42bebea2534caeb07c523930099b7a69de1156a7b1d80bacd1`.
The actual old constructor at `0x2692` contains `pxor` at `0x29d2` and three
`movups` at object offsets `0x1e0/0x1f0/0x200`: the 48-byte six-pointer argv
initializer. The old `wait_pinned` at `0x89ec` uses eight `movups` to clear all
128 `siginfo_t` bytes. The strict owner opcode profile continues to refuse SIMD.

* `tests/schema_validation/native_fault_ops.h`, blob
  `2f772f7be7d36552b6e53613e2d329d3f3e6a97e`: remove ONLY `fixed_argv_[6]{}`'s
  implicit zero initializer. In the existing constructor, before any array use,
  write its complete `sizeof(fixed_argv_)` representation through volatile
  unsigned-char elements. On the selected 64-bit-pointer ABI this reproduces
  the actual old 48 zero bytes. The same six typed assignments immediately
  follow. Handles' negative/default fields, all other scalar and array defaults,
  parent/mode strings, constructor inputs and existing getpid call remain exact.
* `tests/schema_validation/native_fault_fixture.cpp`, blob
  `3e5edd8546f031b969d849bfbbc5c387c890a1d6`: replace ONLY `info = {}` with
  volatile unsigned-char stores for `sizeof(info)` bytes in the same do/while
  body. The selected native ABI requires 128-byte `siginfo_t`; every byte is
  zeroed before each unchanged real waitid attempt, including EINTR retries.

The [C++ object-representation definition](https://eel.is/c++draft/basic.types.general#2)
describes an object's bytes as unsigned-char objects. The
[volatile observable-behavior rule](https://eel.is/c++draft/intro.abstract#8.1)
requires accesses through volatile glvalues to follow the abstract machine.
These sources justify the explicit representation stores, not any unobserved
compiler output. The actual archived old stores establish the selected ABI's
prior zero bytes; this is no portable all-architectures null-pointer claim.

No helper function, allocation, TLS, callback, new syscall or ownership stage is
added. The new locals/loop bounds stay in the same existing functions; rebuilt
`.su`, complete call graphs, scalar instructions and total stack bounds still
require actual inspection. No global compiler flag, SSE allowance or checker
change is made. Atomic instructions and their separately scoped checker review
are untouched.

## Preservation and actual checks

An independent stdlib text-inverse check restores exactly the previous two
source files after removing only these two loop blocks and restoring their
initializers. Literal 6-by-8-byte and 128-byte zero sequences compare exactly;
no candidate is imported or executed. R3 core/header, old57 module and the new
normalization regression are byte-exact against e1fabcc. The three normalization
`work=false` contexts remain unchanged: the known bug is deliberately UNFIXED
pending separate actual regression review/generated admission/reached-bug red.

Actual checks: text/literal preservation above, clang-format dry-run/Werror,
`python3 tools/check_project.py` (426 task records/122 decisions),
`python3 tools/progress.py render --check`, and final staged diff check PASS.
No configure/compiler/link/load, native/test/control/SDK/child/thread execution,
candidate import or historical replay occurred. Different actual source review
must precede root rebuild; generated approval and the new-only red remain gates.
