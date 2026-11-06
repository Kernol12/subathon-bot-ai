# Schema validation native process contract — v1, revision3 (normalization-close profile)

SCHEMATOOL-001, private developer-tool contract, original author B. Revision2 has
actual different document approval C44adb36c. Revision3 adds ONLY the bounded
normalization-close profile below, authored root and independently approved
C99d244e for interface/test authoring. No document verdict approves future code. Planning approval is C `f94b1ef`; the Python
transport stays frozen at `4cc464a`, source `3438a92c` until the replacement gates
below pass. Root owns the matching policy, installation and private-process edits.

Authority remains specification v0.45 §§17/18, API-01/02/03 and TEST-04, the
[logical](schema-validation-tool-v1.md), [public API](schema-validation-tool-api-v1.md),
[context](schema-validation-context-v1.md) and
[installation](schema-validation-installation-v1.md) contracts. This replaces only
the explicitly reconciled private process mechanisms. The public two calls, exact
18 enum values, CLI, input/error priority, package limits, real pinned Python SDK,
worker isolation and all seven integrations/three exchange providers remain.

## 1. Profile and trusted artifact

The component is first-party C++20, Linux x86-64, loaded by the existing Python
developer entrypoint through stdlib ctypes. It is not a standards engine, product
runtime dependency, Qt/server/client target or third-party selection. Existing
CMake >=3.25/Ninja and selected GoogleTest infrastructure are reused; configuration
must never silently download a dependency. GPL-3.0-or-later remains.

Root's reviewed opt-in build profile shall be `SUBATHON_BUILD_SCHEMA_TRANSPORT`,
preset `schema-transport-debug`, shared target `subathon_schema_transport_v1`.
The one production artifact is
`<repository>/build/schema-transport-debug/lib/libsubathon_schema_transport_v1.so`,
derived from the trusted entrypoint's repository location, independent of cwd,
manifest, environment or library search. No automatic build/download, alternative
artifact search or package-selected native path is permitted. A trusted build is
bound to reviewed source/build configuration, compiler/libc/architecture and
recorded whole-artifact SHA256; this is local build provenance, not protection
against an administrator replacing both code and its provenance.

Import and exact help must neither load the artifact nor inspect dependencies,
open project/package files, create descriptors or start a child. Public input and
the three pre-sysconfig environment-presence guards precede native loading/entry.
Missing/unloadable/wrong-architecture artifact or missing versioned symbol is
environment_unavailable, without exposing loader/path/exception text. Known Python
MemoryError remains resource_limit. An unrecognized returned integer is
validation_unavailable. A hostile/binary-incompatible local library is not safely
probed by calling arbitrary code; only the trusted admitted build is callable.

Production ELF constructors/destructors and initialization shall own no resource,
spawn no thread/child, register no callback/handler and perform no validation.
No resource-owning static or thread-local object survives a call. Eager binding
and hidden visibility except the ABI symbol are required, with actual generated
symbols/call graph reviewed before admission. Loading may occur lazily after
admission; no successful package observation is cached.

## 2. ABI 1, data and return values

The sole production export is this C ABI, defined `extern "C"` and `noexcept` in
C++; the following is document notation, not an authored header:

```c
int subathon_schema_transport_v1(
    const unsigned char *manifest, unsigned int manifest_bytes,
    const unsigned char *interpreter, unsigned int interpreter_bytes,
    const unsigned char *worker, unsigned int worker_bytes,
    const unsigned char *sdk, unsigned int sdk_bytes);
```

ABI 1 requires 8-bit bytes, 32-bit int/unsigned int, 64-bit pointers, Linux x86-64
System V calling convention. There are no optional arguments, callbacks, output
buffers, allocator hooks, Python/C++ objects, resource handles or exception objects.
All four lengths are 1..4096 UTF8 payload bytes, excluding a required trailing NUL;
no embedded NUL or invalid UTF8 is admitted. The manifest retains public relative/
absolute path semantics. The other three paths are trusted absolute installation
paths; interpreter symlink spelling is preserved. Invalid manifest spelling is
invalid_path; invalid trusted configuration is environment_unavailable. These
checks do not enlarge or replace public path admission.

The Python wrapper prepares private immutable-lifetime buffers, exact builtin
ctypes pointer/unsigned-int argument types and `c_int` result type BEFORE entry.
The native function rejects null pointers/invalid lengths before dereferencing,
then copies into four 4097-byte native arrays before acquiring any resource. The
trusted caller must provide readable buffers for their declared lengths plus NUL;
the ABI is not a memory-safety validator for arbitrary foreign pointers. No caller
borrow or request state is retained after return. No other thread mutates these
private buffers while the GIL is released.

Use `ctypes.CDLL` with immediate/local symbol binding, no Python `errcheck`, custom
converter/restype, `py_object`, CFUNCTYPE or Python callback. Argument conversion
and loading finish before native entry; integer conversion and Python unwinding
occur after native return. Other Python threads may run. Neither the core nor
its syscall/clock functions may call Python while ANY owned resource remains.
No helper Python thread, traceback recovery, Python process constructor or signal
handler/mask mutation participates in native ownership. PyDLL is not this selected
binding; holding the GIL would not independently prove ownership.

[ctypes](https://docs.python.org/3.10/library/ctypes.html#loading-shared-libraries)
defines CDLL's GIL behavior; selected
[CPython call source](https://raw.githubusercontent.com/python/cpython/v3.10.12/Modules/_ctypes/callproc.c)
separates argument conversion, foreign call and result conversion. Actual selected
3.10/3.12/3.14 ABI/signal foundations remain required.

| Native integer | Meaning | CLI exit when published |
| ---: | --- | ---: |
| 0 | ok | 0 |
| 1 | usage_error, reserved to parent CLI; never a worker result | 64 |
| 2 | invalid_path | 2 |
| 3 | io_unavailable | 3 |
| 4 | environment_unavailable | 3 |
| 5 | resource_limit | 4 |
| 6 | invalid_json | 2 |
| 7 | invalid_manifest | 2 |
| 8 | unsupported_dialect | 2 |
| 9 | unsupported_format | 2 |
| 10 | dependency_unavailable | 3 |
| 11 | invalid_schema | 2 |
| 12 | unresolved_reference | 2 |
| 13 | reference_cycle | 2 |
| 14 | invalid_inventory | 2 |
| 15 | unsupported_feature | 2 |
| 16 | invalid_instance | 2 |
| 17 | validation_unavailable | 3 |
| 18 | private interrupted transport; never a worker/public enum | see below |
| 31 | separately reviewed inert native-baseline marker only | never product ok |

Value 1 returned by transport/worker and any value outside this table refuse as
validation_unavailable. Private 18 means native work was interrupted and cleanup
finished. A real pending Python SIGINT may then raise the original Python
KeyboardInterrupt; without such an exception, map 18 to validation_unavailable.
Never manufacture KeyboardInterrupt from an integer or turn cancellation into ok.
The separately admitted inert stub returns only 31, with no file/resource/worker
operation. Its binding raises a distinct named native-unimplemented marker solely
for the new reviewed baseline; 31 is forbidden in completed production behavior.

## 3. One resource lifetime and bounded setup

One synchronous native call owns configuration handles, exactly two protocol
pipes, one `/dev/null` handle, any temporary duplicate handles, one native PID and
its atomic pidfd. All handle slots start invalid. At most eight descriptors may
be owned simultaneously: configuration is closed before pipes; four pipe ends,
null, one temporary normalization duplicate and pidfd fit within eight. Child
remapping inherits copies, not parent-shared tables. No third protocol channel,
successful worker factory, retained cache or background owner is permitted.

The entire core has fixed arrays/scalars and at most 32768 bytes of native
automatic scratch per invocation, including path/argv/environment/config/protocol
buffers and owner records. No heap allocation, container growth, C++ exception,
iostream/filesystem/logging, Python API, dynamic loader or callback occurs between
first resource acquisition and final release. Native kernel allocation failures
are still possible and classified below. Stack/call graph and allocator checks
must establish this bound; the C++/RAII labels alone establish nothing.

The parent still derives and checks its actual CPython ABI, prefix/sysconfig and
one bound SDK directory under the installation contract, with all three override
presence refusals BEFORE sysconfig. The native interval owns the parent's actual
`pyvenv.cfg` open/read/fstat/close: derive its root from the preserved absolute
interpreter spelling; open no-follow/nonblocking, require a regular file, read at
most 4097 bytes for the 4096 ceiling and reject an incomplete/changed snapshot.
No manifest/package file or SDK candidate import belongs in this parent check.
Child recomputation, the 20 exact metadata pairs, hashes and phase-6 imports remain
the worker's responsibilities. The matching root-owned installation revision
must make this split explicit before dependent tests/code.

Build argv/environment, signal actions, descriptor slots and poll records before
birth. Prepare two `pipe2(O_CLOEXEC)` pairs; make only parent read ends nonblocking
and keep outcome/activation writes blocking. Open `/dev/null` read/write with
close-on-exec. Normalize all sources to distinct descriptors >=5 using native
integer results, never assuming descriptors 0/1/2 were originally open. Adopt only
successful syscall-created handles; do not close arbitrary output-slot contents
after a failed multi-output syscall without an actual kernel guarantee.

The only production clock is native CLOCK_MONOTONIC. Capture `t0` before first
native resource acquisition, and the absolute deadline is `t0 + 10 seconds`, with
checked arithmetic. This includes all native preparation and atomic birth; it
never starts afresh after clone/exec/readiness/activation. A failed/unrepresentable
clock is environment_unavailable before ownership, otherwise transport refusal
after cleanup. No public clock, timeout or limit option is added.

## 4. Atomic birth and child instructions

The sole selected candidate is raw Linux `clone3` with flags
`CLONE_PIDFD | CLONE_CLEAR_SIGHAND`, exit_signal SIGCHLD, and every unused clone_args
field zero. No CLONE_VM/FILES/FS/SIGHAND/THREAD/VFORK/PARENT, namespace, ptrace,
set-tid, cgroup or TLS option. The calling native thread remains alive and owns
wait/cleanup synchronously; no Thread.start/acknowledgement interval exists.
Parent SIGCHLD must be SIG_DFL without SA_NOCLDWAIT; explicit SIG_IGN, an installed
handler or inability to query this is environment_unavailable before birth.
The owner queries but never changes any parent disposition or mask.

The native PID/pidfd commit rule is exact:

1. Initialize a separate unowned pidfd output cell to -1; owner PID/pidfd slots
   stay invalid. Invoke raw clone3 with a writable native output cell.
2. On parent return >0, immediately adopt that PID and the successfully committed
   pidfd before any fallible secondary operation. No allocation/callback separates
   syscall return and native-slot adoption. Invalid successful outputs are a fatal
   backend-contract defect, not permission to kill a guessed numeric PID.
3. On child return 0, enter only the child syscall path below; no parent owner
   destructor, Python instruction or C++ return runs there.
4. On error, do NOT adopt/close/signal using the pidfd output cell, even if changed.
   Kernel rollback must leave no installed pidfd/live child. Selected-kernel
   source plus real controlled error foundations must prove this BEFORE the
   backend is admitted. A failure cell may contain a reserved/released number
   that another thread has already reused; blindly closing it is forbidden.

[clone documentation](https://man7.org/linux/man-pages/man2/clone.2.html) defines
the atomic output and child-handler reset. Actual
[5.15 fork source](https://raw.githubusercontent.com/torvalds/linux/v5.15/kernel/fork.c)
contains output writes before later rollback points; this is why output contents
alone are not ownership. No `fork` plus later `pidfd_open`, Python PID/FD boxing,
`/proc` discovery or unreviewed fallback is selected.

CLONE_CLEAR_SIGHAND resets caught handlers at birth, before any child instruction;
the child then resets all remaining ignored catchable dispositions to SIG_DFL and
clears its inherited signal mask using raw kernel operations. SIGKILL/SIGSTOP are
not altered. No Python/native user handler may execute in this pre-exec child.
The child's private signal changes never affect the parent; any setup failure
refuses without A/package work. This includes inherited SIGPIPE/SIGINT/SIGCHLD
and blocked-signal cases; exact raw ABI/source and real signal controls are gates.

The child uses only audited raw syscalls/async-signal-safe operations on prebuilt
buffers: signal setup, dup2/dup3, close/close_range, bounded write, execve and _exit.
No libc atfork handlers, allocator, loader resolution, C++ exception/destructor,
Python callback, preexec_fn or event loop. Copy private null to 0/1/2; activation
WRITE to 3 and outcome WRITE to 4; close every descriptor >=5 with
`close_range(5, UINT_MAX, 0)`. The exact exec plan is:

```
ABS_INTERPRETER -I -S ABS_WORKER --manifest PATH --sdk-dir ABS_SDK_DIRECTORY --activation-fd 3 --outcome-fd 4
```

argv has exactly those 12 non-null entries plus terminating null. Environment has
only `PATH=/usr/bin:/bin`, `LC_ALL=C.UTF-8`, `LANG=C.UTF-8` plus terminating null.
No shell, inherited stdin, extra worker option or unrelated descriptor survives.
Pre-exec failure may publish ONLY the exact nonvalidating environment refusal
below, or terminate with invalid/missing protocol; it must not forge activation.
The pinned Python worker still establishes/verifies 536870912 address-space limits
and ABI/no-site/root admission before package reads/candidate work.

[execve signal inheritance](https://man7.org/linux/man-pages/man2/execve.2.html),
[close_range](https://man7.org/linux/man-pages/man2/close_range.2.html) and the
[async-safety rules](https://man7.org/linux/man-pages/man7/signal-safety.7.html)
constrain the generated child call graph. Eager linking alone is insufficient proof.

## 5. Protocol, absolute deadline and quiescent return

Parent immediately closes its write/null copies and polls only two read ends and
the pidfd. Retain at most 2 activation bytes and 257 outcome bytes; reject excess
instead of truncating/accepting a prefix. POLLHUP is drained through bounded reads
to actual EOF. Nonblocking read EAGAIN returns to bounded polling, not failure or
EOF. Poll errors, partial writes, missing EOF, malformed/extra/unknown
lines, mismatched status and unclassified signal death cannot be ok. No stdout,
stderr or child exception text is interpreted or exposed.

Ordinary received results require exactly one ASCII A followed by activation EOF,
exactly one public constant `{"code":"VALUE"}\n` line followed by outcome EOF,
and definitive normal child status matching the public CLI mapping. Worker
usage_error is forbidden. The sole pre-A exception remains EMPTY activation EOF,
exact 35-byte `{"code":"environment_unavailable"}\n` plus outcome EOF, normal exit
3 AND actual reap. Any other no-A/extra/truncated/whitespace/status-mismatched tuple
is validation_unavailable. All original protocol rows remain literal test oracles.

For exit/status observations use same-birth-thread raw
`waitid(P_PIDFD, pidfd, ..., WEXITED | __WNOTHREAD)`; add WNOHANG for work polling,
and WNOWAIT only for non-consuming observation. A zero si_pid with WNOHANG is live/
not-yet-observed, not cleanup. Consume the actual status before ordinary success.
Never wait/kill a numeric PID discovered from an untrusted/reused number. Native
pidfd integers stay pinned through termination/reap and are closed last.

On deadline/protocol excess/interruption, terminate through
`pidfd_send_signal(pidfd, SIGKILL, NULL, 0)` and finish cleanup. An ESRCH signal
response or ECHILD wait response alone is insufficient. Other caller threads can
reap; they cannot authorize stale PID signalling. If own wait loses status, require
BOTH pinned whole-process exit observation AND a fresh same-birth-thread waitid
ECHILD under the admitted no-reparent/no-autoreap profile before releasing pidfd;
return validation_unavailable, never infer a matching normal status. Actual kernel
identity/wait rules and a real competing-reaper control must prove this combination.
Do not require newer-kernel pidfd POLLHUP behavior on a kernel that proves only
POLLIN; do not equate exit-ready with reap by itself.

[wait rules](https://man7.org/linux/man-pages/man2/waitpid.2.html) distinguish reap
and WNOWAIT, and [pidfd signalling](https://man7.org/linux/man-pages/man2/pidfd_send_signal.2.html)
pins the target. Selected [kernel wait source](https://raw.githubusercontent.com/torvalds/linux/v6.12/kernel/exit.c)
and [pidfd polling source](https://raw.githubusercontent.com/torvalds/linux/v6.12/fs/pidfs.c)
must be compared with the actual hosts; no universal kernel inference is asserted.

The final ordinary result is committed only after EOF/status/reap/all closes and
a final monotonic check strictly before the absolute deadline. At/after deadline,
resource_limit takes precedence; no late ok/domain result is admitted. Known
resource refusal also dominates received data. EINTR during work commits private
18, then cleanup continues through EINTR as necessary; actual Python signal
propagation remains outside the resource interval. Deadline/known-resource refusal
already committed is not turned into a different ordinary result by later cleanup.

Every exit closes config/normalization/null/all four pipe endpoints/pidfd exactly
once as owned. On Linux, close errors must not cause a second close of a number
that may already be reused. Native code executes the actual close before removing
its slot; no Python callback/unwind can split that sequence. Close/transport errors
refuse success and cannot abandon other cleanup. A returned integer certifies no
remaining owned descriptor, live child or unreaped child; if quiescence cannot yet
be established, continue cleanup rather than return a fabricated clean failure.

[Linux close semantics](https://man7.org/linux/man-pages/man2/close.2.html) prohibit
the naive close retry. Kernel scheduling/uninterruptible states may delay signal
delivery, syscall completion and mandatory reap. The ten seconds is the absolute
work/result-acceptance deadline, not a promise to return while a child is alive:
cleanup after expiry remains mandatory and may exceed it under non-real-time OS
conditions. Fatal parent death/kernel failure produces no invented result. The
matching private-process revision must state this explicitly, not silently allow
late accepted work or drop no-surviving-child cleanup.

## 6. Closed transport failures

| Observation | Required native/public classification after cleanup |
| --- | --- |
| Bad manifest / trusted argument admission | invalid_path / environment_unavailable respectively; no birth |
| Missing artifact, unsupported ABI/config/SIGCHLD or unavailable required facility (ENOSYS, policy denial) | environment_unavailable; pre-birth refusal does no package work; post-birth discovery immediately terminates/reaps and accepts no worker result |
| Native known ENOMEM, EMFILE, ENFILE or process-capacity EAGAIN; known Python MemoryError | resource_limit; no inference from an unexplained signal |
| Absolute deadline reached | resource_limit, including before A |
| EINTR while work is active | private 18; finish cleanup, then normal Python signal semantics |
| Config absent/nonregular/oversize/changed or unusable trusted setup | environment_unavailable |
| Exact received activated outcome/status/EOF and own definitive normal reap | that listed outcome, only within deadline |
| Exact pre-A environment tuple | environment_unavailable only under its complete conditions |
| Unknown signal, lost status, unexpected syscall/protocol failure or unknown native value | validation_unavailable |

No general OSError=>resource rule, package-derived error string, inferred OOM or
success-after-skipped-facility is allowed. Unsupported facility in a BOTH-host
positive foundation is a BLOCKED capability, not a passing supported-host check.
If an implementation encounters an additional ambiguous errno/cleanup condition,
refuse and obtain source-based classification review; do not invent domain errors.

## 7. Exact negative-only test seam

Normal positive/domain/SDK/metadata/context examples use the REAL production DSO,
native owner, trusted worker and pinned SDK, unpatched. No public path/options,
environment flag, success factory, callback, injected received result or production
test detection selects a fault. Original Python Popen/os.pipe/time/_execute_child
patches cannot certify this implementation and must not become vacuous green.

The admitted native fault fixture is a SEPARATE test-only shared artifact compiled
from the SAME owner/core source bytes and build options; a native Ops adapter and
driver may differ. Production exports only ABI 1 and has no fault-plan state.
Its loop/ownership/decoder/cleanup/deadline algorithm may not be copied/reimplemented
in the fixture. Record core SHA, exact compile definitions/options and actual
linked symbols for both artifacts; different review verifies common algorithm and
that only the explicit syscall/clock/phase adapter varies.

The fixture exports ABI 1 plus these test-only C functions (document notation):

```c
int subathon_schema_fault_arm_v1(unsigned int case_id,
    unsigned int phase_id, int fault_errno);
int subathon_schema_fault_snapshot_v1(int64_t *cells, unsigned int cell_count);
int subathon_schema_fault_report_v1(unsigned char *bytes, unsigned int byte_count);
int subathon_schema_fault_fallback_v1(void);
```

The arm result is 0 only for an admitted fixed plan before core entry; unknown/
inconsistent plans return -1 and cannot run the core. Snapshot requires exactly
1024 writable int64_t cells (8192 bytes); it returns 0..128 record count, or -1
for invalid state/overflow. Each eight-cell record is sequence, event kind, phase,
fd, PID, native result, errno, monotonic nanoseconds; unavailable fields are -1.
The sixth cell is a kind-specific native result: for syscall-return it is the
actual raw kernel return; for final-commit it is the ACTUAL closed ABI integer
committed by the common owner AFTER its resource cleanup and final decision,
before C return/Python conversion. That event must not precede quiescence or
invent/override an accepted result. In particular it can observe private18 when
pending Python SIGINT later prevents Python from receiving the integer. Its errno
cell is -1 because this is not a syscall. Other events retain only their actual
defined native observations; unavailable result/errno fields stay -1. The final
event is not independent proof of cleanup: raw FD/reap oracles remain mandatory.
Event kinds are 1 syscall-attempt, 2 syscall-return, 3 adopt, 4 release, 5 clock,
6 signal, 7 readiness, 8 final-commit, 9 poison, 10 child-exec-handoff-intent.
Sequence encodes writer_id in its upper 32 bits and that writer's increasing local
index in its lower 32 bits: parent 0, child 1, controller 2. Snapshot concatenates
their committed entries in that order; this is not a total chronological trace.
Two-output acquisitions record both actual parent descriptors separately; no
unrecorded second pipe endpoint is allowed. Handoff intent is NOT a release or
proof exec succeeded; real outcome/activation/exit evidence remains necessary.
Copies occur after core return, never through a callback. Fallback returns 0 only
after actual test-owned/leaked-child termination/reap/close, and runs AFTER the
oracles; its result cannot repair a failed oracle. Every case uses fresh fixture
state/process; unknown-plan/ledger poison cannot be reset and reused as green.
Minimal actual helper headers/source still require separate review. These symbols
are absent from production and may not alter its ABI or invoke Python callbacks.

Revision2 clarifies only that existing final-commit observation's scalar meaning;
the earlier syscall-only cell wording could not represent the already required
actual interrupted18 oracle when ctypes raises before exposing the return. No
record/export/layout/public outcome/resource limit or fake result is added. Its
actual document/type delta needs different review before dependent tests execute.
Python tests may redirect ONLY the private artifact load to the actual
reviewed fault DSO, never substitute a Python callable/accepted integer. The
fixture validates the normal 12-entry argv/3-key environment/2 channels before a
fixed negative case may replace exec with a real tiny isolated fault child.
No fault child can establish a successful domain/SDK result.

The fixed case IDs are 1..3 for the original activation rows, 4..10 for the seven
outcome rows, 11 for status mismatch, 12 for unclassified death, 13..22 for the ten
setup rows, and 23 for readiness/deadline. Their literal source-order values stay
those of frozen `wrapper_acceptance.py`; phase_id/fault_errno are both 0. Case 24
injects one syscall errno at a selected phase; case 25 delivers real SIGINT at that
phase; case 26 records one known native resource refusal AFTER genuine birth;
case 27 runs the real competing reaper; case 28 observes child descriptor/signal
closure using a real nonvalidating child. Cases 25..28 require fault_errno 0.
The independent actual test source fixes each case's literal expected refusal and
real readiness/control, not a success generated by the owner. Additional case IDs
or broader injection need written contract/test review first.

The phase bases, in order, are 1 config-open, 2 config-read, 3 config-close,
4 activation-pipe, 5 outcome-pipe, 6 source-duplicate, 7 null-open, 8 atomic-birth,
9 parent-write-close, 10 child-signal, 11 child-dup, 12 child-close-range,
13 child-exec, 14 protocol-read, 15 protocol-poll, 16 wait/reap, 17 pidfd-kill,
18 pidfd-close, 19 final-commit. Revision3 appends base20 normalization-original-close;
all earlier numeric values remain unchanged. The encoded phase_id is twice the base for BEFORE
and twice the base plus one for AFTER. Repeated operations select their FIRST
occurrence only. For case 24, fault_errno is one of EINTR, ENOSYS, ENOMEM, EMFILE,
ENFILE, EAGAIN, EIO, EACCES or EPERM, only where meaningful under section 6;
an impossible syscall/errno combination is a rejected plan, not a passing case.
An AFTER phase first executes the saved
REAL syscall and records its actual return/identity; it cannot return an error
before birth and claim post-birth coverage. Error injection is one-shot, fixed
negative errno/EINTR or a fixed resource-refusal flag, never an accepted result.
An AFTER acquisition phase preserves the real successful return and native handle
adoption before signalling a negative continuation; it MUST NOT rewrite a
successful pipe/clone result to look like kernel rollback. Likewise an after-close
signal cannot turn an already-released number into an owned retry candidate.
Atomic-birth error fixtures must separately prove real no-child/no-new-fd rollback;
they may not inject failure after successful birth and pretend the kernel rolled
it back. Both original pre-call and after-real-close cancellation oracles remain.

Revision3 admits exactly ONE additional case24 tuple: phase41 (AFTER the first
normalization-original-close), fault_errno EINTR. No new case ID, child mode,
channel, export, public argument or successful test result is added. Phase40
exists as the corresponding BEFORE observation but is not an admitted injection
tuple. The source duplication's original descriptor is closed during ACTIVE
setup work; it is not cleanup. A work close error EINTR and an AFTER-close
interrupted continuation both commit private18, then finish all cleanup without
retrying the released Linux descriptor number. Cleanup-only closes retain their
existing distinct classification.

For this new tuple, execute the actual raw close, invalidate its slot and record
the real release ACK BEFORE the AFTER checkpoint. Preserve the actual syscall
return0 and the actual closed descriptor identity; the adapter may request only
the negative interrupted continuation. It MUST NOT fabricate a negative kernel
return, keep a closed number owned, accept any worker result or report a birth
that did not occur. Expected18 is reached before birth and all remaining owned
descriptors are released before the actual final-commit event. The original
source-duplicate AFTER boundary remains after the genuine F_DUPFD_CLOEXEC adopt
and BEFORE this original close, preserving every existing oracle.

This closes the independently identified75source defect where active normalization
reused cleanup-only close classification. Tests must observe the genuine missing
work interruption behavior before the separately reviewed fix. A minimal
interface/phase hook may be authored with the CURRENT cleanup-only classification
explicitly preserved solely to establish that new red; this is not working
behavior or permission to skip original69/public74/binding9 acceptance. Independent
review must inspect that hook's retained classification and actual release facts.
Journal capacities remain128/36+27+24; the new selected parent observation/release
uses the same bounded resources. No resource budget or ordinary acceptance rule
changes. The original57-row negative inventory remains exact and mandatory; a
separate focused normalization-close regression records this new tuple.

This is a SELECTED bounded journal, not a trace of every syscall, poll iteration,
clock read or individual signal-loop operation. Every actual parent-native FD
adoption/release/identity is mandatory; selected events supplement it. The fixed
128-record allocation has disjoint parent 64, child 32 and controller 32 segments:

| Journal | Bound and required selected scope |
| --- | --- |
| Parent (64) | At most 12 created handles: config 1 + pipe ends 4 + null 1 + source duplicates 5 + pidfd 1. All adoption/release events fit 24 entries. Up to 2 selected fault-phase attempt/return entries, 4 clock checkpoints (t0, ready, offset advance, final decision) and 6 lifecycle/status/poison entries give at most 36. Acquisitions are not retried after interruption; source duplicates are made at most once per source. |
| Child (32) | Five inherited source identities, five actual destination-dup results, five source-close observations after close_range, and five destination handoff-intent entries consume 20. The selected phase pair, signal-setup entry/exit, readiness, exec-failure return and poison add at most 7, total 27. No per-signal loop trace. Real exit/reap and post-exec report, not handoff intent, establish later closure/state. |
| Controller (32) | At most six test-owned descriptors (four coordination ends, one pidfd guard duplicate, one outcome-read capture duplicate) give 12 adoption/release entries. Shared-map acquire/release, selected phase pair, up to four readiness/signal events, two clock events and two poison entries give at most 24. |

If actual source needs more resource operations than these bounds, it must obtain
contract/count review before tests; overflow is latched failure, never truncation
or dropped mandatory evidence. Ordinary repeated reads/polls/clocks are exercised
by the SAME core and source audit, not spuriously declared fully traced. Do not
accept an owner's claimed clean boolean. Test-owned handles/mapping are separately
accounted and closed, outside the production eight-FD/32768-byte scratch bounds.
Ledger poison/unexpected network/phase events never reset to manufacture success.

Because clone3 does NOT share VM, a private child array would be invisible to the
parent. The fixture therefore prepares ONE 12288-byte test-only anonymous
MAP_SHARED read/write mapping before birth: 8192 bytes for these records and the
remaining fixed space for counts/poison/latches/report capture. No fd backs it.
Parent/child/controller each write only their own segment, publish completed
records with native release stores and read committed counts with acquire loads.
The selected compiler/architecture must prove lock-free inline publication and
the generated pre-exec store/call graph; no hidden lock/allocator/Python callback
is admitted. Snapshot copies only committed records. The child mapping persists
across birth but is removed by exec; it supplies PRE-EXEC observations only.
The fixture releases the parent map in unconditional post-oracle cleanup, even
when the owner/test fails. A mapping/setup failure is fixture failure, never red.
[mmap semantics](https://man7.org/linux/man-pages/man2/mmap.2.html) and
[execve mapping removal](https://man7.org/linux/man-pages/man2/execve.2.html)
support this route, not a claim that private child stores become shared.

The deterministic test clock uses the SAME native clock access points: saved real
monotonic time plus a checked fixed offset, initially zero, advanced once beyond
t0+10s ONLY after a genuine fault child acknowledges bounded readiness and blocks.
No sleep/stress/resource exhaustion, restarted interval, external watchdog-fired
pass or Python clock stand-in. An independent real monotonic watchdog may kill
but MUST NOT reap before observations and MUST fail if fired.

Case 23 may use a fixed native negative child phase AFTER real child signal setup
and BEFORE descriptor remapping/exec: raw-write readiness on a separately owned
test coordination pipe, then raw-read a blocked test pipe while retaining outcome.
The native controller observes that readiness before advancing the clock. This
proves a real pre-exec startup hold and cleanup, not successful exec or unrelated-
descriptor closure. Production has no such phase behavior. Case 28 separately
runs actual remap/close_range/exec with NO extra inherited test descriptors and
observes only 0/1/2 plus 3/4 and the required signal state in a real fixed native
nonvalidating reporter. Thus coordination cannot weaken the close-fds oracle.

The Case 28 reporter gathers all raw observations while activation 3 AND outcome 4
are OPEN, closes only its newly opened scan fd, and commits one intentionally
INVALID binary packet on existing outcome 4 while activation 3 remains OPEN.
Only AFTER a successful full write does it close activation 3 WITHOUT A, close
outcome 4, and attempt normal exit 3. It is
never a worker result/controls certificate. Exactly 248 bytes, all integers little
endian, no compiler struct padding:

| Field order | Bytes / actual raw observation |
| --- | --- |
| Magic | 4 bytes `NTF1`, deliberately not a protocol JSON prefix |
| PID, scan-fd, descriptor-count | Three signed int32 values; actual getpid/open/getdents-derived values |
| Descriptor numbers | 17 signed int32 slots, ascending, unused slots -1; count 17 means at least 17 observed and is refusal, never a truncated fitting inventory |
| F_GETFD, then F_GETFL for 0..4 | Ten signed int32 raw returns (negative errno on failure) |
| Blocked mask | Raw 64-bit Linux signal-mask bits |
| SIGINT, SIGPIPE, SIGCHLD, SIGTERM | For each: signed int32 raw sigaction return, uint64 handler, uint64 flags, uint64 action-mask (28 bytes each) |
| Terminal getdents result | Signed int32 raw result, required 0 for complete EOF |

The reporter opens `/proc/self/fd` AFTER exec solely for bounded raw enumeration,
using one 4096-byte getdents64 buffer and at most 17 numeric entries; it closes
this newly opened scan fd before sending. This is not an extra inherited fd. A
correct inherited set yields scan fd 5 and exact observed set 0..5, count 6, EOF;
descriptor flags must independently show 0..4 open without CLOEXEC, null read/write
on 0..2, blocking write channels on 3/4, empty blocked mask and default dispositions.
Tests compare literal masks/sets/handler values, not a reporter's clean boolean.
Missing proc/read/error/overfull/incomplete observation is fixture failure. The
reporter has no Python/SDK/schema engine; actual source/oracle review is mandatory.

The reporter commits ALL 248 bytes in ONE blocking write <=PIPE_BUF on descriptor
4 while descriptor 3 is still OPEN. Any short/error write is fixture failure,
never a committed report. Closing activation early is forbidden: it would both
invalidate the 0..5 descriptor observation and permit refusal before packet commit.
The owner may kill on the invalid packet before the reporter's attempted exit;
the test observes real termination/reap, and does not demand normal 3 or infer a
normal worker result. Only this case's fixture tees actual outcome reads into a fixed 257-byte
capture. A separately tracked parent-only duplicate read handle, acquired before
birth with descriptor >=5 and closed in child close_range, may obtain only the unread committed suffix AFTER the
core returns, using nonblocking bounded reads; it neither reaps nor repairs the
owner. `subathon_schema_fault_report_v1` requires exactly 248 writable bytes and
returns 248 only for the complete packet plus EOF, otherwise -1; it copies bytes,
not a pass/clean status. It does not replace/withhold bytes returned by real core
reads, modify the decoder or suppress its refusal. Packet overflow/capture error
latches fixture failure. Parent/controller identity bounds include this duplicate.
The shared pre-exec map is NOT assumed available in the exec'd reporter.

Assert the raw packet independently AND exact public validation_unavailable/no
logs, actual termination/reap/all core FDs closed before fallback. The intentional bad packet
must not be mixed into normal/domain or the original protocol/error-priority rows;
those retain their original literal bytes and classifications. A refusal code
alone cannot establish child FD/signal correctness. [getdents](https://man7.org/linux/man-pages/man2/getdents.2.html)
and [pipe byte-stream rules](https://man7.org/linux/man-pages/man7/pipe.7.html)
require explicit bounded capture rather than assumed message/read boundaries.

For native-during-ownership SIGINT, a real native thread/external sender delivers
the signal to the actual calling main thread at a recorded phase; handler/mask
remain unchanged. Assert Python propagation only after native cleanup. Preserve
exact exception identity where a preexisting Python exception is actually
observable; a native SIGINT test must not invent such an object. Complementary
pre-entry and post-return Python MemoryError/cancellation cases require separately
reviewed narrow fault sites and prove no entry or already-quiescent native return.
They may not claim a c_int boxing allocation is reachable without source proof.

After the raw public/native call, inspect saved real waitpid/waitid/fcntl states
BEFORE poll/wait/test fallback could reap/close anything. A zombie or a fixture-
reaped child is a failure. Only then may unconditional fallback kill/reap/close
keep failed tests disposable. Real read/FD identities, child readiness, signal
and resource phases must be observed; fixture installation failure is not red.

## 8. Preserved oracles and release sequence

The actual approved mapping is `be753ee`,
`docs/evidence/schematool-001-native-test-mapping.md`, blob `96130822`; root owns
its integration. ALL 74 existing methods remain accounted: public 64 at source
`64f4911`, cleanup 2 `acde40d`, context 3 `b34a564`, lexical 3 `fc82684`, spawn 2
`777d907`. Keep every literal refusal, exact status/line/byte ceiling, poison
ledger, source review and original FIRST/raw receipt. Counts are inventories,
not native passes. Preserve outer real-script launches and real SDK-clone controls.

Port all five public fault callers together with their helper; retain every row
(activation 3, outcome 7, status/signal, setup 10 and deadline). Strengthen help,
environment and pre-entry allocation no-call guards for native load/entry. Port
cleanup/spawn FD/reap/cancellation/allocation oracles under the recorded private
mechanism-change reason, never deletion/skip/weaken to pass. Context helper cases
also run through the real public integration; worker/context/lexical assertions
and SDK 14 remain ordinary checks. Extra native cases cover atomic rollback,
child descriptor/signal state, competing reaper/PID identity, errno/EINTR at the
listed ownership phases, no allocation/callback interval and ctypes boundaries.

Before any dependent header/test/code: different approval of this actual contract,
root policy/tools/ADR/install/private-process reconciliation and actual oracle map.
Then separately review minimal inert ABI/harness and actual independent native/
binding test source. Establish GREEN real build, loader, ABI and kernel foundations
before the NEW named native-unimplemented red; missing library/compiler/GTest/
kernel/child readiness is not feature red. Never replay historical FIRST as new.

Both ordinary Ubuntu22.04/Python3.10 and Ubuntu24.04/Python3.12 CI lanes must prove
actual clone3+CLONE_PIDFD+CLONE_CLEAR_SIGHAND, pidfd signal/poll/same-thread wait,
pipe2/close_range and ctypes behavior under THEIR kernel/seccomp policy. Linux5.9+
is a candidate floor, not evidence. No fork/raw-clone fallback is admitted; an
unavailable atomic backend blocks supported-host acceptance until a separately
reviewed source/test/profile decision. Local CPython3.14 is additional evidence.

Implementation then needs separate native/Python/worker source review (including
generated child call graph, fixed allocation, resource/read-race/offline gates),
all preserved/ported oracles plus new native negatives passing, SDK14/20-package
checks and full public64 unconditionally on both actual hosted lanes. Root owns
build/CI/progress/pins/latest-head checks/current rules/normal merge. Neither this
document nor an old functional Python pass closes those gates or completes the task.
