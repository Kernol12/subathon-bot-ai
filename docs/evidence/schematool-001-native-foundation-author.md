# SCHEMATOOL-001 native foundation source

Author stage only, from root `075f3567da0893265a1afac828171ae62959ec8b` on isolated
branch `task/SCHEMATOOL-001-native-foundation-author`. This follows the actual
policy approval `48c075` and native-contract approval `44adb`; the current
native/process-test/installation contracts and ADR-0009 remain unchanged.
The user asked to finish the existing PR77 and stop. No other task is claimed.

## Owned source and exact inventory

Only these files are authored here: `tests/schema_validation/native_foundation.cpp`,
`native_foundation_child.h`, `native_foundation_child.cpp`,
`native_loader_foundation.py`, and this note. The ABI header/stub are the separate
author's `22e0df49f26fa607b7118166619c8a50b8c7ff5b`, not my implementation.
Root owns the build packet, CI, state, contracts and publication. The unchanged
worker and all prior baseline tests/reviews/red receipts are preserved.

Six GoogleTest cases, all in `SchemaNativeFoundation`:

| Case | Independent literal/control oracle |
| --- | --- |
| `InertBaselineLiteral31` | Actual linked ABI symbol, four readable NUL-terminated literal buffers, integer 31. **Temporary inert-stage control**, never product success. |
| `SigchldProfileQueryWithoutBirth` | Initial SIGCHLD is default without SA_NOCLDWAIT; actual raw queries observe separately installed ignored/no-wait states, then restore before assertions. No child/owner is invoked in this control. |
| `AtomicPidfdSignalsAndCloseRange` | Actual native positive PID plus atomic pidfd, copied-stack child report, live wait result 0/si_pid 0, pidfd kill, CLD_KILLED/SIGKILL, actual reap, stale pidfd ESRCH/ECHILD, no own-thread children, all six actual acquired descriptor numbers EBADF. |
| `ActualExecveNonvalidatingProbe` | The same child sends a pre-exec report, performs actual execve of the fixed reporter, then sends a second report with the same PID, default dispositions, empty mask and inherited channel. Kill/reap identities remain literal. |
| `ForeignThreadAndCompetingReapKeepPinnedIdentity` | A native controller exists before birth. Unflagged wait admits the live sibling-thread child; __WNOTHREAD returns ECHILD. The controller actually reaps the killed child; the still-open pidfd thereafter gives ESRCH/ECHILD, never a numeric-PID signal. |
| `ControlledCloneErrorsRollbackWithoutGuessingOutput` | Real short clone-argument size gives EINVAL; real PROT_NONE pidfd-output memory gives EFAULT. A bounded before/after /proc/self/fd inventory matches and own-thread wait gives ECHILD. Output-cell contents are diagnostic only, never adopted or used as a cleanup target after failure. |

Two Python `NativeLoaderFoundation` methods:
`test_builtin_abi_profile` checks the fixed Linux/x86-64/little-endian builtin
widths and Python >=3.10; `test_actual_inert_artifact_returns_literal_31` repeats
those admission assertions, loads only the exact trusted build path with real
CDLL immediate/local binding, freezes eight builtin pointer/unsigned-int types
and c_int, and calls the actual inert symbol with four private builtin arrays.
No custom converter, callback, errcheck, py_object, SDK or validation is involved.
Import is definition-only; the artifact is inspected/loaded only during actual
test execution. The two literal-31 controls require an explicit written,
different-reviewed stage migration when the real owner replaces the stub.

## Harness scope, ownership and finite failure

This is a fixed nonvalidating facility harness, not a transport owner, manifest
validator, syscall fault adapter or port of the 74 behavioral methods. All native
resources and signal fixture changes end before GoogleTest assertions, formatting
or diagnostics. A controller thread is constructed before FD/birth acquisition
and joined before assertions. The copied child enters a raw-syscall-only noreturn
path immediately after successful clone3; it cannot return through parent C++
destructors or GoogleTest. Its prebuilt argv/environment and fixed-size reports
need no child allocation or formatting. The separately executed reporter's main
also uses only the same fixed raw-syscall operations; normal executable startup
after exec is distinct from the pre-exec copied-stack path. Generated-code and
linked-call review remain mandatory before execution/admission.

The child installs a five-second SIGALRM watchdog, independent of the parent
protocol. That timer cannot reap and its SIGALRM status fails the SIGKILL oracle;
it never makes a failed observation green. Parent polls and controller futex
waits have fixed three-second deadlines; no stress, allocation exhaustion,
catastrophic expression or ENOSYS skip is used. A failed normal observation
sets `fallback=true` before cleanup, and successful tests explicitly reject it.
The actual native pidfd is retained through kill/wait; every parent FD is closed
before fcntl observations and assertions. Unexpected successful births during
negative clone controls immediately raw-exit in the child and are really reaped
in the parent, while the literal expected errno assertion still fails.

The signal fixture observes a real caught SIGUSR1 disposition, initially ignored
SIGPIPE and blocked SIGUSR2. CLONE_CLEAR_SIGHAND is expected to clear the caught
handler while preserving ignore; explicit raw reset/unmask then clears the
ignored disposition and inherited mask. The fixture restores parent state.
The SIGCHLD query control mutates only this disposable test process, with no child
present, and restores it; it does not prove that the future owner admission guard
is implemented or permit production global signal mutation.

**Only one live extra FD is closed** by the pre-exec close_range operation.
The actual parent copy remains open, proving the FD tables are not shared.
The after-exec reporter receives that already-closed number and asserts EBADF;
its idempotent close_range is not a second live-FD close proof. Full production
all>=5 closure/remapping, ignored-signal case28, owner rollback at every phase,
allocation/cancellation/deadline behavior, callback-free native ownership and all
74 behavior ports remain separate gates. Kernel/seccomp/libc/uname metadata are
recorded only as diagnostics during future runs, never assumed capability.

## Primary-source basis reviewed before authoring

- The [Linux clone manual](https://man7.org/linux/man-pages/man2/clone.2.html)
  permits stack NULL/size zero without CLONE_VM, with copied address space/stack.
  The selected flags have no CLONE_VM, CLONE_VFORK, CLONE_THREAD or stack change.
  [glibc x86-64 syscall assembly](https://raw.githubusercontent.com/bminor/glibc/master/sysdeps/unix/sysv/linux/x86_64/syscall.S)
  shows the generic syscall wrapper's success `ret` on the unchanged stack.
  This is the bounded reason a C-call return from this fork-like clone is usable;
  it does not approve a different-stack or shared-VM clone recipe.
- The [syscall ABI tables](https://man7.org/linux/man-pages/man2/syscall.2.html)
  and that assembly support the private x86-64 raw register mapping. Child raw
  results remain negative errno without libc TLS conversion. The
  [x86 signal UAPI](https://raw.githubusercontent.com/torvalds/linux/v5.15/arch/x86/include/uapi/asm/signal.h)
  fixes the four-word sigaction layout; the mask size is eight bytes for the raw
  kernel operations, not sizeof glibc sigset_t. Compile-time layout assertions
  retain these exact x86-64 assumptions.
- [Linux 5.15 fork.c](https://raw.githubusercontent.com/torvalds/linux/v5.15/kernel/fork.c)
  `copy_sighand`, `copy_process` and rollback labels show caught-handler reset,
  pidfd reservation and put_user before publication, and fput/put_unused_fd on
  failure. Its possible post-output failure also explains why an unchanged
  output-cell sentinel is not an ownership/rollback oracle. The PROT_NONE case
  deliberately exercises the real failed output write rather than artificial
  allocation exhaustion. A newer kernel/source comparison and actual selected
  host observations remain the independent review/runtime gates.
- [Linux exit.c](https://raw.githubusercontent.com/torvalds/linux/v6.12/kernel/exit.c)
  `is_effectively_child` distinguishes current real parent from same-thread-group
  waiting with __WNOTHREAD; the test observes both results and a real competing
  reap. [waitid](https://man7.org/linux/man-pages/man2/waitpid.2.html),
  [pidfd_send_signal](https://man7.org/linux/man-pages/man2/pidfd_send_signal.2.html)
  and [close_range](https://man7.org/linux/man-pages/man2/close_range.2.html)
  define the selected status/signal/closure operations.
- [Python 3.10 ctypes](https://docs.python.org/3.10/library/ctypes.html)
  provides the builtin CDLL, pointer/unsigned-int and c_int binding used here.
  Loader success does not prove the implemented owner's cleanup or GIL-boundary
  guarantees; this actual source calls only the resource-free inert stub.

## Build handoff and remaining gates

Root's planned target `schema_transport_native_foundation` links the actual
`subathon_schema_transport_v1`, `GTest::gtest_main` and `Threads::Threads`; include
the native ABI-header directory. Build the separate
`schema_transport_native_foundation_child` from the reporter .cpp and define
`SUBATHON_NATIVE_FOUNDATION_CHILD_PATH` as its actual absolute target path, with
an explicit dependency. Discover/register only the six actual cases, prefix and
label `schema-native-foundation`, with finite CTest timeout. The loader's exact
argv is `native_loader_foundation.py --artifact ABS_TARGET_PATH`, with the selected
Python `-I -S -B` and minimal environment. It refuses alternate artifact paths.

This author has performed **no configure, C++ compile, native loading, candidate
import, kernel probe, child/thread creation or test execution**. Independent
actual source/oracle review and root build/CI-packet review must precede the
root's explicit execution release. Actual green facility/build/loader receipts
on both selected hosts precede intended red of the separate behavioral baseline.
No feature, native owner, all-controls coverage or done status is claimed.

Static author checks (all exit 0, no candidate import):

- A stdlib Python here-document read/parsed the loader with
  `ast.parse(..., feature_version=(3, 10))`, then `compile` without `exec`; it
  independently enumerated two test methods and six unique C++ TEST names.
  Literal checks established 16 x 8 = 128 report bytes and path payload lengths
  13/16/20/14. This is syntax/literal inspection, not Python test execution or
  C++ compilation.
- `clang-format --dry-run --Werror tests/schema_validation/native_foundation.cpp tests/schema_validation/native_foundation_child.cpp tests/schema_validation/native_foundation_child.h`: PASS after formatting with the repository style.
- `python3 tools/check_project.py`: PASS, 426 tasks / 122 decisions; the command
  explicitly reports that draft GoogleTest cases are not compiled/run/approved.
- `python3 tools/progress.py render --check`: current.
- `git diff --cached --check`: PASS before the source commit.

## Pre-execution interrupted-wait correction

Different reviewer C's actual source review of `8e20a76` / root build-base
`29902d4` found that disposal used one unchecked blocking wait. Real EINTR there
could leave a zombie, despite the note's child-gone claim. The foreign reaper and
unexpected-success negative controls had the same fixture defect; the writable
short-size control also did not close its committed pidfd on an unexpected
successful clone. Root explicitly released this bounded correction on fresh
`task/SCHEMATOOL-001-native-foundation-cleanup-correction` from `29902d4`, preserving
the original author and matrix trees. This is a pre-execution harness correction,
not a new feature/test case, implementation change or replayed failing baseline.

The raw waitid/wait4 helpers now retry only EINTR. Disposal of an actual successful
birth retains its known pidfd, retries interrupted pidfd signalling, and requires
an actual terminal reap of that native PID before releasing the identity. A lost
status is accepted for disposal only with pinned exit readiness and a fresh
same-birth-thread ECHILD under the checked default/no-auto-reap SIGCHLD profile.
The only other thread here is the recorded controller, which neither births nor
changes SIGCHLD. Saved-native-PID wait4 with __WNOTHREAD is a wait-only disposal
fallback: this still-live birth thread has no intervening other birth/reparenting,
so it cannot reap another thread's reused-PID child. No numeric-PID signal or
failed-output-cell adoption is introduced. The controller retries interrupted
wait4, retains an explicit failure on any nonmatching reap, and leaves the pinned
identity for the birth thread's actual cleanup. `fallback=true` remains latched
before disposal and its existing assertion still fails; helper recovery cannot
rescue the original observation.

An unexpected positive short-size clone alone adopts the actual successful
writable output and closes that pidfd once after definitive child disposal. A
negative clone return still never reads/adopts/signals/closes its output cell.
The PROT_NONE unexpected-positive branch can reap the actual child but cannot
read a committed output identity; it terminates the fixture with raw nonzero121
instead of guessing a descriptor or returning to ordinary assertions. Likewise
an impossible missing successful output, unprovable child cleanup or failed
success-only descriptor disposal is a fatal nonreturning fixture failure
(119/120/121), never a child-gone claim or passing observation. Parent process
death is not relabeled as a successful cleanup proof. All normal expected paths
retain the original literal oracles, six C++/two Python names and temporary31.

Only `native_foundation.cpp` and this note change. Child .cpp/.h and loader bytes
stay identical. Actual independent correction review is required before root's
separately permitted compilation/static disassembly; runtime, discovery/loading,
probes and all tests remain held. Static correction checks are recorded below
before handoff; this author has performed no compilation or execution.
Root separately compiled the old `29902d4` packet under C's compile-only release
and reported one dangling-else macro warning around the inventory comparison.
This correction adds explicit braces to that unchanged EXPECT_EQ loop; it removes
the ambiguous macro binding without deleting or weakening an inventory assertion.
That old build receipt is not execution or a build result for this correction.

Actual correction checks, all exit0: stdlib source-only comparison with preserved
`8e20a76` author tree proves all six C++ TEST names/order and every EXPECT
invocation unchanged, the three ancillary source files byte-identical, two loader
methods by Python3.10 AST, fallback failure still latched and both raw wait helpers
retry EINTR. `clang-format --dry-run --Werror tests/schema_validation/native_foundation.cpp`
PASS; `python3 tools/check_project.py` PASS426/122;
`python3 tools/progress.py render --check` current; `git diff --cached --check`
PASS. No candidate import, compile, load, thread/child/probe or test body ran here.

## Narrow raw-child instrumentation correction

Root's actual GNU16.2.1 compile/static-disassembly of the old packet found a
generated `__stack_chk_fail@plt` branch in `schema_foundation::observe` and a TLS
canary check in the fixed exec reporter main. Even a failure branch is outside
the admitted raw-only copied-child call graph. C then explicitly released a
function-level annotation correction; this is not permission for target-wide
hardening changes or a PLT/TLS exception. This isolated branch starts at root
`3837fbb`, containing the separately reviewed `264c522` wait-cleanup correction.
Only child .h/.cpp and this note change; every other source/config remains exact.

The seven bounded header functions `raw`, `address`, `finish`, `observe`,
`publish`, `hold` and `child`, plus reporter `literal`, `descriptor` and `main`,
receive only `gnu::no_stack_protector`. Bodies, fixed report128, literal parser
bounds, parameters, syscalls and all six native/two loader oracles remain exact.
Comments explain why this fixed raw child graph needs the narrow exclusion.
The [GCC function-attribute documentation](https://gcc.gnu.org/onlinedocs/gcc/Common-Attributes.html)
defines the function-local effect and GNU standard attribute spelling; this is
not a claim that every compiler/selected binary implements it as intended.

Parent GoogleTest/probe/controller/disposal functions retain compiler hardening;
there is no global or target `-fno-stack-protector`, no sanitizer/profile exception,
no `noplt` allowance, changed decoder or new behavior. Generated-code audit on
each actual selected host remains mandatory: the complete reachable raw graph
must contain no TLS access, indirect/external call, stack-check helper, profiling,
sanitizer or other compiler runtime helper. Source annotation alone is not proof.
Actual different source review, generated-checker review and artifact audit must
precede runtime. This author has not configured/compiled/run or loaded a candidate.
Static annotation/preservation and ordinary repository checks are recorded below
before handoff; no native/runtime/source-implementation capability is claimed.

Actual static checks, all exit0: stdlib source comparison strips only the new
attributes/comments/whitespace and proves header/reporter declarations, bodies,
parameters and literals unchanged; exactly7 header/3 reporter attributes;
parent `native_foundation.cpp` and loader byte-identical to `264c522`; report128
assertion retained. `clang-format --dry-run --Werror tests/schema_validation/native_foundation_child.h tests/schema_validation/native_foundation_child.cpp`
PASS; `python3 tools/check_project.py` PASS426/122;
`python3 tools/progress.py render --check` current; `git diff --cached --check`
PASS. No compiler, candidate import/loading, disassembler/checker, native probe,
thread/child or test body executed in this author assignment.
