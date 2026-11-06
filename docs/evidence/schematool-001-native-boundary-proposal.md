# SCHEMATOOL-001 — proposed native resource boundary

Author B (`/root/session_supplier_author`),6 November2026. **DOCUMENT PROPOSAL ONLY:
no native code, build, interface/stub, test, probe, contract or policy change is
approved or executed by this note.** Freeze Python4cc464a/source3438a92 and retain
all original baselines, reached reds, raw evidence and valid scoped reviews.
Different root/C review and the explicit gates below precede implementation.

## Decision proposed and policy authority

Move ONLY process/descriptor/deadline ownership into one first-party C++20 Linux
component called through stdlib ctypes. Keep exact18 public enum/calls/CLI/inputs/
output/limits, actual pinned Python standards worker, complete offline/context/
example semantics and all product scope. This is no schema engine, server/client
library or new third-party dependency. Existing CMake>=3.25/Ninja/C++20 and optional
GoogleTest infrastructure suffice; no Qt/server/client target linkage is proposed.

Read tools/AGENTS and accepted ADR0008. Their current text requires Python>=3.10
stdlib developer tools and separately admits reviewed standards libraries in the
isolated schema environment. It does NOT already approve this compiled artifact.
Requested root-owned decision: admit a narrowly scoped FIRST-PARTY developer native
transport, with reviewed tools/ADR/installation/private-process/test-plan revisions.
The Python entrypoint/progress tools remain stdlib; other tools do not acquire a
native requirement. Existing standing C++20/product authority makes a separate
owner decision unnecessary in principle, but neither that fact nor this note grants
its own policy exception. Root/C must explicitly approve the concrete reconciliation
before code. Escalate only an actual unresolved policy conflict; do not silently
edit AGENTS or infer permission to add Qt/new libraries/runtime product dependencies.

This adds a developer build/install prerequisite and Linux syscall capabilities.
Import/help must still work without native loading/building. Missing/wrong native
artifact or unsupported facilities return environment_unavailable before package
work; they are NOT a green baseline or a passing supported-host result. Both current
positive CI hosts must actually support the selected backend before taskdone.

## Alternatives assessed

| Option | Assessment |
| --- | --- |
| Current Python Popen/thread/proc/pidfd repair | Historical partial proofs stay valid. Python return-boxing, constructor/start acknowledgement, recovery allocation and cleanup interruption windows remain; more Python callbacks do not remove that boundary. |
| Selected Qt6.12 QProcess behind a native ABI | Existing-library option explicitly considered. Qt documents native process/start/wait/kill facilities and requires QtCore. It is not currently admitted in this isolated tooling lane; its partial-start/allocation/FD/pidfd/cancellation invariants need selected-source/failure proof. Forcing QtCore setup into both schema lanes has no demonstrated advantage for this tiny ownership-only job. |
| First-party fixed POSIX/Linux owner behind ctypes | Recommended review candidate: small inspectable resource lifetime, native integer PID/FD ownership, no callback/return into Python from the owner while it holds resources, no new third-party selection. It still requires real kernel/source/build/test proof. |

[Qt6.12 QProcess documentation](https://doc.qt.io/qt-6/qprocess.html) supports the
Qt comparison; no undocumented Qt safety failure or native alternative's prospective
success is asserted. Choosing the small owner is an inference about proof/setup
scope, not a measured performance claim or blanket rejection of existing libraries.

## One C ABI and bounded allocation

Proposed versioned exported function (declaration is DOCUMENT notation, not a header):

```c
int subathon_schema_transport_v1(
    const unsigned char *manifest, unsigned int manifest_bytes,
    const unsigned char *interpreter, unsigned int interpreter_bytes,
    const unsigned char *worker, unsigned int worker_bytes,
    const unsigned char *sdk, unsigned int sdk_bytes);
```

Freeze Linux x86-64 C ABI widths before authoring:32-bit unsigned lengths/int result,
64-bit pointers; no C++ object/exception, Python object, callback, function pointer,
PID/FD, owned result buffer or allocator crosses this boundary. The C++ definition
is extern C/noexcept and exports only this versioned symbol. Wrapper owns private
UTF8/NUL-terminated buffers for the four already admitted paths; lengths1..4096,
no embedded NUL, trusted paths absolute. Construct ctypes buffers/prototype/load
BEFORE entry. Native rechecks bounds and copies into four4097-byte stack arrays
BEFORE creating resources; it borrows no mutable caller data during ownership.

Return a fixed integer ONLY AFTER definitive child reap (or separately proved
prior reap by another waiter, refusing success) and all native-owned handles released. Explicit private mapping follows the public
enum order0..17 (resource_limit is native5, whose CLI exit remains4; environment is
native4, whose CLI exit remains3). usage_error/native1 is never a worker result.
The remaining explicit native values are0ok,1usage_error(reserved),2invalid_path,
3io_unavailable,4environment_unavailable,5resource_limit,6invalid_json,
7invalid_manifest,8unsupported_dialect,9unsupported_format,10dependency_unavailable,
11invalid_schema,12unresolved_reference,13reference_cycle,14invalid_inventory,
15unsupported_feature,16invalid_instance,17validation_unavailable. These are ABI
values, NOT CLI exits. Setup inability, known allocation/deadline refusal and
unexpected transport failure must retain their existing closed classifications;
none is a schema/instance error manufactured by the transport.

Private18 means interrupted transport: after native cleanup, actual pending Python
SIGINT may raise its original KeyboardInterrupt; absent a Python exception, map18
to validation_unavailable, never invent KeyboardInterrupt/success. Unknown values
refuse. A separately reviewed inert native stub may reserve31 for a NAMED native
unimplemented marker solely during the new baseline; never map it to product ok.

Only fixed std::array/scalar ownership records and bounded native buffers while
resources live; no new/vector/string/iostream/filesystem/logging, Python C API or
exception allocation. Proposed scratch bound32KiB is an implementation review/check,
not a new package ceiling. FD/PID/pidfd slots start invalid and destructors are
noexcept; every path has explicit cleanup status. Native allocator/call-graph tests
must establish this rather than infer it from C++ or RAII names.

Move project-controlled pyvenv.cfg open/read/fstat/close into this SAME native call:
Python derives/checks runtime/sysconfig/prefix strings without an os.open-owned
handle, preserving the three BEFORE-sysconfig environment guards. Native derives
the executable's root and bounds the regular cfg read4096+1. Loading the library
and Python/ctypes conversions may fail BEFORE resource entry; return boxing or
Python handling may fail AFTER cleanup. Neither may expose a live native PID/FD.
Native artifact provenance is trusted local build state, not a package-selected URL.

### CDLL versus PyDLL

[ctypes documentation](https://docs.python.org/3.10/library/ctypes.html#loading-shared-libraries)
distinguishes CDLL's GIL release from PyDLL's held-GIL/error-flag behavior.
Recommend CDLL with builtin argtypes/c_int restype, no errcheck/custom converters/
CFUNCTYPE/py_object. The native function NEVER calls back into Python while resources
live. Other Python threads may run, including a real test SIGINT controller or
reaper; atomic identity/cleanup must remain sound under them. For a call on main, its Python
signal handler executes only after its native call returns, with resources gone.
For a call on another Python thread, ordinary main-thread signal semantics remain;
no new global handler/mask is installed. Native EINTR handling supplies cleanup.

PyDLL is an explicit alternative, not necessary ownership proof: it holds the GIL
for up to the work deadline/cleanup and blocks Python controller threads, requiring
an external sender/native OS-thread fixture. Neither binding alone proves resource
ownership. ABI conversions occur before entry and result conversion after native
return in either case; no callback-capable binding is permitted. These claims need
actual selected3.10/3.12/3.14 boundary foundations before execution is advertised.

## Native birth, channels and cleanup candidate

Prefer an atomic birth primitive: Linux clone3 with CLONE_PIDFD, exit_signal SIGCHLD
and NO VM/thread/parent-sharing flags. The kernel supplies native PID plus pidfd in
an int output slot; no PyLong creation is interposed after birth/FD creation.
Require source/negative tests that failed atomic creation leaves no child/pidfd.
[clone3/CLONE_PIDFD](https://man7.org/linux/man-pages/man2/clone.2.html) documents
this interface; [kernel fork source](https://raw.githubusercontent.com/torvalds/linux/v6.12/kernel/fork.c)
shows pidfd preparation. This is a CANDIDATE requiring actual minimum/selected-host
kernel/seccomp tests, not a claim clone3 works in every container. No runtime fallback
to fork+pidfd_open is selected. A reviewed raw clone(CLONE_PIDFD) atomic alternative
could be assessed if clone3 is blocked; unsupported is not task acceptance.

Before birth: copy argv/env, prepare exactly two CLOEXEC protocol pipes plus owned
/dev/null, normalize sources above reserved child descriptors, and establish one
absolute CLOCK_MONOTONIC deadline immediately before birth. All syscalls return
native integers into native owner slots without Python allocation/cancellation.
Check native SIGCHLD disposition/no-autoreap requirements explicitly; inability
to establish an approved configuration refuses before package work. No parent
signal handler/mask mutation, helper thread/start acknowledgement or traceback
recovery is needed in this design.

Child path NEVER returns to Python/C++ runtime. Map /dev/null to0/1/2 and the two
protocol WRITE ends to3/4, close unrelated descriptors>=5 with close_range, then
execve the exact trusted interpreter with -I -S and the unchanged four ordered
option/value pairs (--manifest, --sdk-dir, --activation-fd3, --outcome-fd4).
Environment remains ONLY PATH=/usr/bin:/bin, LC_ALL=C.UTF-8, LANG=C.UTF-8. No shell,
Python preexec_fn, allocator, logger, exception, at-fork Python callback or event
loop. Raw clone3 avoids libc fork-handler invocation; child uses only reviewed
raw Linux syscalls or POSIX async-signal-safe operations. Eager symbol binding and
actual generated call-graph/source audit must verify that statement.
[Async-signal-safety](https://man7.org/linux/man-pages/man7/signal-safety.7.html)
and [close_range](https://man7.org/linux/man-pages/man2/close_range.2.html) are the
primary constraints, not permission to call arbitrary libc/C++ code after birth.

Parent closes WRITE/null handles immediately, polls two nonblocking READ ends and
pidfd, retains only2 activation bytes/257 outcome bytes, and matches exact complete
line+EOF+normal status. Existing narrow empty-A/env35LF/normal3 setup refusal stays;
no other result/success without A. Real worker still verifies512MiB/no-site/ABI
before package reads/SDK imports. Any child pre-exec setup failure uses only that
nonvalidating refusal or an invalid protocol; no fake A/backend.

Use atomic pidfd identity, not numeric PID kill. Verify/reap with the reviewed
same-birth-thread waitid(P_PIDFD,...WEXITED/WNOHANG/WNOWAIT/__WNOTHREAD) semantics;
[source wait rules](https://man7.org/linux/man-pages/man2/waitid.2.html) and actual
selected kernel details need separate audit. pidfd_send_signal kills ONLY the pinned
child on deadline/protocol/interruption, then finish reaping and close. Another
thread can steal exit status: refuse success if status/ownership is unavailable.
ECHILD/ESRCH alone is NOT proof a child is gone; combine with pinned exit observation
and valid wait semantics. Never signal a possibly reused raw PID.
[Pidfd signalling](https://man7.org/linux/man-pages/man2/pidfd_send_signal.2.html)
provides identity, not an automatic reap guarantee.

EINTR during native work triggers cleanup/refusal (or continued required reaping),
not Python callbacks/unwinding midway through ownership. Default SIGINT remains
pending until after return; no new handler is fabricated. Linux close errors do
not cause a second close of a reused FD. The native nonthrowing interval removes
Python pre-close, PID/pidfd-boxing, constructor and Thread.start gaps together.

### Post-birth failure and unavoidable platform limits

Atomic birth is recommended precisely to AVOID a later pidfd_open allocation/failure
path. If reviewers instead select POSIX fork+pidfd_open, they must FIRST supply a
proved bounded cleanup path when pidfd_open fails after birth: no numeric kill based
on a stale identifier. A child-only default/unblocked remaining-wall timer before
exec is a possible backstop, but survival across exec, async-safety, precision,
worker preservation and failed setup/refusal need primary proof and real tests.
That alternative is NOT approved by listing it here. Do not silently add it.

No ordinary result is accepted past the same10-second deadline; on expiry request
termination then complete cleanup. Linux is not real-time: uninterruptible kernel
states/scheduling can delay birth, signal delivery or reap. No API can promise an
absolute return<=10s in those states while also promising no live child on return.
Private wording must explicitly reconcile work deadline versus mandatory cleanup;
no hidden relaxed acceptance deadline or premature return. Fatal process death,
hostile native hooks/kernel faults are not fabricated successful observations.
No native safety certificate is inferred before source/facility/negative tests.

## Explicit private revisions required BEFORE code

The current r3 production Popen/os.pipe/Python-clock lookup, returned-Popen and
constructor-fixture rules do NOT authorize this path. A new reviewed private
revision must replace those MECHANISMS with the native ABI/owner/clock and native
fault seam; keep their worker argv/two-channel/EOF/status/reap/refusal invariants.
Fixed child3/4 remains within the existing descriptor domain. Installation policy
must admit the trusted local artifact and native cfg ownership; preserve the actual
20 pins/metadata checks and3 sysconfig guards. No new public call/option/outcome.

Native clone3/CLONE_PIDFD/P_PIDFD/signal facilities and close_range imply a candidate
Linux5.9+ capability profile, subject to ACTUAL kernel/seccomp proof on BOTH positive
hosts. A version number alone is not proof. No legacy fork+pidfd_open fallback or
unsupported-positive-host skip may obtain green. Parent resource/Python-private
fixtures must be ported under an exact reviewed map before retirement; a C ABI
callable substituted with a successful Python result is never an admissible test.

## Build, setup and exact test-first split

Propose root-owned opt-in SUBATHON_BUILD_SCHEMA_TRANSPORT plus schema-transport-debug
preset/SHARED target subathon_schema_transport_v1 using existing C++20/Ninja options,
no Qt/server/client links. Deterministic artifact
<repo>/build/schema-transport-debug/lib/libsubathon_schema_transport_v1.so is loaded
by ONE trusted module-relative absolute path, never cwd/package/env/library search.
No build/download on import/help/validation. Missing/ABI-wrong artifact refuses
cleanly. Export only the versioned C symbol, use eager binding, record compiler/
libc/kernel/artifact/source identity and preserve GPL-3.0-or-later. A new component
uses existing system libc/C++ runtime/Python ctypes support, no new package pin.
[Existing CMake SHARED targets](https://cmake.org/cmake/help/v3.25/command/add_library.html)
fit the selected build system; actual build/install profile still needs review.

Both Ubuntu22.04/Python3.10 and Ubuntu24.04/Python3.12 lanes must build actual native
bits, establish syscall/ctypes/ABI foundations, then run SDK14,20 metadata checks,
FULL64 public acceptance plus all currently reviewed focused oracles and new native
faults unconditionally. Selected local3.14 remains additional evidence. Native
GoogleTest can reuse the already selected>=1.17 mechanism; no fetch/new version is
implicitly authorized. Qt lanes/current job/check protection stay untouched.

| Proposed executable work package (root owns actual IDs/claims) | Required gate/result |
| --- | --- |
| Native contract/policy/installation/test-map | Different actual proposal review; root narrows tools/ADR/private transport/fault contracts and artifact/kernel profile. Freeze ABI/status/clock/identity and test-only seam. No code yet. |
| Native baseline/interface/harness | Minimal reviewed declarations/named inert stub plus separately authored synthetic native/ctypes tests. Real facility/build/loader foundations GREEN, then intended native-unimplemented RED; missing .so/kernel/GTest is not red. |
| Native resource owner | Implement SAME reviewed core; separate RAII/async-safety/allocation/identity/deadline audit and passing genuine native negatives. No test-detection or production fault knobs. |
| Python binding/oracle port | Keep public behavior/worker; construct buffers before entry, use real native call, exact status mapping after cleanup. Different source review; full real package/SDK acceptance. |
| Root CI/completion | Unconditional actual bodies on both supported hosts, full logs/latest head/rules/reviews and normal merge; preserve every historical red/review and explicit remaining release gate. |

A's actual companion test mappingbe753ee identifies74 existing methods (public64,
cleanup2, context3, lexical3, spawn2); integrate/link that document after review.
Old Popen/os.pipe/time/_execute_child mocks CANNOT certify this transport. Port
EVERY refusal/FD/reap/deadline/allocation oracle to real native owner tests, retaining
literal outcomes and pre-fallback native waitpid/fcntl evidence. Native fault workers
remain real blocked/malformed children; default package cases use production native
bits plus genuine Python SDK. Use separately compiled negative-only syscall/phase
instrumentation on the SAME core, never a successful fake/production option. Real
native-thread/external SIGINT controllers, deterministic native deadline phases,
pre/post-allocation boundaries and child/FD checks need actual different review.

Any old mechanism retirement requires a written PRIVATE contract-migration reason,
exact oracle-to-oracle map, independent revised tests/red and reviewed green—not
skip/delete/weaken to pass. Original sources/receipts remain historical evidence;
do not rename/replay FIRST63/261 or its additive red. Existing worker/context/lexical
oracles not tied to transport stay ordinary checks. Counts are inventories, not
passing acceptance. No native implementation is authorized by this proposal.


Actual author work was repository/primary-source reading and this document ONLY.
No native/Python source/interface/stub, test, CMake, policy, contract or pin changed;
no SDK/candidate import, compile/build/facility probe, fork/child/signal or acceptance
run occurred. Repository project/render/whitespace consistency checks accompany the
commit and do not establish any native capability. Root/C different actual review
is the next gate; all design choices above remain proposals, not safety approval.
