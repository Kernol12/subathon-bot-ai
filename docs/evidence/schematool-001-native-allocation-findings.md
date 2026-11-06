# SCHEMATOOL-001 — native resource boundary investigation

Coordinator `/root`,6 November2026. Read-only primary-source assessments by B/C/A
follow actual reviewed implementation and attributable negative tests. No new
process/FD/kernel probe, test, installation or production code is authorized by
this note. This records why further design review is needed; no fallback or native
replacement is approved prospectively, and no public guarantee is waived.

The approved Python worker8f4 remains a usable reviewed artifact. Actual parent4cc
fixes two published-handle constructor paths, with separate C e0a98d9 scoped review.
Overall parent remains unapproved. [Review](schematool-001-constructor-code-review.md)
records unbounded constructor completion wait, Thread.start pre-acknowledgement,
before-real-close untracking, native unpublished PID and recovery-allocation gates.
These are distinct from the observed successful after-real-close and published-
constructor regressions. Do not project those passes onto the uncovered windows.

Actual selected CPython fork/return source creates an OS child before allocating
its Python PID return value; subprocess.py assigns self.pid afterward. A known
MemoryError there can unwind with a child and no published handle. Exact primary
lines across3.10.12/3.12.3/3.14.9 are in the [parent source assessment](schematool-001-parent-review.md).
The existing known-memory/no-survivor/unwinding API applies. No catastrophic-OOM
exception follows merely because native allocation caused the error.

A independently assessed a preopened per-birth-thread Linux children-file recovery.
Kernel task-owned inode/parent-list/seek semantics support a constrained exclusive
single-child inference while the birth thread is alive. The documentation warns
about incomplete lists generally; thread exit reparents children. Other process
threads can reap children, and autoreap/PID reuse make unchecked numeric kill unsafe.
A observed only actual local procfs open/bounded empty-read/EOF/rewind capability;
NO new thread/child/fork/syscall identity experiment was run. Exact local kernel7.2.14
source identity was unavailable; current master source is not that proof.

Primary references: [Linux proc child-list documentation](https://www.kernel.org/doc/html/latest/filesystems/proc.html#proc-pid-task-tid-children-information-about-task-children),
[task child listing implementation](https://raw.githubusercontent.com/torvalds/linux/master/fs/proc/array.c),
[thread-self binding](https://raw.githubusercontent.com/torvalds/linux/master/fs/proc/thread_self.c),
[fork parent linkage](https://raw.githubusercontent.com/torvalds/linux/master/kernel/fork.c),
[wait/reaper rules](https://man7.org/linux/man-pages/man2/waitpid.2.html).

A further source-assessed stable-identity recipe uses pidfd_open, then STILL-LIVE
birth-thread waitid(P_PIDFD,WEXITED|WNOHANG|WNOWAIT|Linux __WNOTHREAD=0x20000000).
Kernel5.4/6.12 accept this flag and limit eligibility to that calling thread's child;
live own child succeeds without a result, foreign-thread/nonchild rejects ECHILD.
A verified pidfd binds its original kernel PID object, so subsequent reap/reuse
cannot retarget pidfd_send_signal. Empty/stale candidates still require actual
own-thread no-child reconciliation, not assumed completion; alive(0,0) is not reap.
This is source support for an identity STEP, not a complete cleanup implementation.
[Linux5.4 wait implementation](https://raw.githubusercontent.com/torvalds/linux/v5.4/kernel/exit.c),
[Linux6.12 wait implementation](https://raw.githubusercontent.com/torvalds/linux/v6.12/kernel/exit.c),
[wait flags](https://raw.githubusercontent.com/torvalds/linux/master/include/uapi/linux/wait.h),
[pidfd safety conditions](https://man7.org/linux/man-pages/man2/pidfd_open.2.html).

That route still crosses another native allocation boundary: source3.10.20,
3.12.15 and3.14.9 posixmodule.c performs pidfd_open syscall, then PyLong_FromLong(fd),
without closing the real FD if result allocation fails. These are the actual
inspected tags, not a claim of byte identity for every distro/patch. Therefore a
one-shot identity recipe cannot certify all frozen allocation/FD guarantees.
[CPython3.10.20](https://raw.githubusercontent.com/python/cpython/v3.10.20/Modules/posixmodule.c),
[3.12.15](https://raw.githubusercontent.com/python/cpython/v3.12.15/Modules/posixmodule.c),
[3.14.9](https://raw.githubusercontent.com/python/cpython/v3.14.9/Modules/posixmodule.c).
No native stress, actual exhaustion, PID-reuse stress or blanket certificate follows.

Root has frozen further Python resource-recovery patches while B authors a concrete
DOCUMENT-ONLY native C++20 process-owner alternative using the existing selected
toolchain and standard-library ctypes boundary. Candidate design goal: one native
call owns all bounded FDs/PID/protocol/deadline and finishes cleanup before Python
can receive a return allocation or unwind. It must address signal/EINTR/fork/exec/
close-fds/known-allocation/PID-reuse and after-birth pidfd failure paths, with no
Python callback while resources live. A native owner is a hypothesis for review,
not proof that libc/kernel operations are failure-atomic or real-time.

The actual proposal must explicitly assess tools/AGENTS.md's Python standard-library
policy and ADR0008, build/installation/ABI/CI impact, private Popen/fault-seam changes
and independent test-first task split. Preserve18publicAPI, raw validation semantics,
all limits/dependencies/release scope, existing valid worker reviews and historical
reds. Port every applicable protocol/refusal/reap/deadline oracle meaningfully;
no test deletion/weakening or successful fake can hide the architecture transition.
Different actual proposal/contract/test reviews and intended new reds precede any
native production implementation. If an owner policy decision is genuinely needed,
prepare the concrete reviewed result first; routine implementation does not imply
permission to silently relax public guarantees or rewrite AGENTS.
