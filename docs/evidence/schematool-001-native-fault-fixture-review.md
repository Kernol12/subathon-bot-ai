# SCHEMATOOL-001 nonvalidating fault fixture — independent body review

Reviewer C (`session_plan_review`), 6 November 2026, PR77 only. Isolated review
branch from root `c7f3977`; I read B's actual original body832/rootc02 and the
subsequent immutable corrections through
`ffbf4063a70ab5937e7aea3766e93bb765263123` using Git source reads. I authored no
fixture, child, Ops or owner body. Only this review note is owned here.

**APPROVED: the actual corrected nonvalidating fixture SOURCE for assembly of the
separately reviewed helper/test/build packet. No configure/compile/link/load,
arm/control call, controller/thread/child/signal/probe or test execution is
released.** A's actual different helper-foundation test source, complete build
packet and generated-code/resource audit must pass before execution. This is not
approval of B's separate negative child source, future FaultOps/shared owner,
accepted validator result, complete negative baseline, hosted runtime or merge.

| Actual source | Git blob |
| --- | --- |
| Corrected `tests/schema_validation/native_fault_fixture.cpp` | `8d53bc685c77ced6b4ed611fcef6e33ace0b5059` |
| `tests/schema_validation/native_fault_state.h` | `fe3a326b01f9b181a10a1a2b1689670763b6a32a` |
| B's fixture author note | `9dd159ccec294e77ca395afa34228693c572c421` |

Authority: native contract revision2/kind8 clarification34 with different Cac750
review, accepted C44/C48 policy/native scope, C265 fixture plan, reviewed typed
declarations and unchanged57-row matrix/74-oracle map. The source has exactly the
four declared test C exports and hidden fixed C++ hooks. It defines no production
ABI entry and returns no31/accepted validation substitute. Both later DSOs must
retain the exact inert `transport_stub.cpp` until the genuine test/red gate.

## Actual findings and corrections reviewed

Original fixture `3d819105` / state header `843dabf` had three real pre-execution
issues, sent promptly to root/B. No passing-source approval was issued for that
original body:

1. Case23 `pin_birth` acknowledged only the guard bind. The parent could compute
   its timeout before genuine child readiness and offset advancement. Actual
   `016b217d8541c56ef684753b0b18ed908f8066c4` additionally waits for action_done
   **only** for case23, after birth_bound. The header requires the child's
   independent pre-remap ready/hold entry, avoiding a circular parent-return
   dependency. No repeated1ms sleep or hypothetical race fix is accepted.
2. After proven child disposal and FD closes, a failed munmap retained a map
   whose next fallback tried an already-closed guard and could not retry unmap.
   Actual ffbf406 records completed child and descriptor disposal in two fixed
   map-tail words, then skips completed stages on retry. Failure poison persists;
   it never revives a green result or re-closes a released descriptor.
3. The initial fallback kill did not latch assistance before a controller could
   finish its reap during join. A later pinned-reaped check could return without
   poisoning that assistance. Actual ffbf406 poisons **before** the initial help
   kill. Concurrent completion cannot erase that evidence.

I read the exact corrected source/diffs. These findings are resolved within the
fixture's documented fixed-process/caller model. Their runtime paths are not yet
dynamically verified; future helper controls/source audit remain necessary.

## Ownership, observation and synchronization assessment

Plan admission matches all23 original protocol tuples,29 exact case24 phase/errno
tuples, two SIGINT tuples and cases26/27/28:57. Unknown/repeated arm returns-1 and
cannot reset the one-use process into green. Arm maps one12288-byte shared region,
prepares exactly six stable test FD numbers, then creates one fixed pthread
controller without waiting for any owner birth/phase. Partial resource acquisition
is tracked before later fallible steps; failed pipe outputs are never adopted.
No owner/worker/manifest/protocol implementation or substitute child is created.

The journal is exactly128x8 int64 cells=8192 bytes, with4096 bytes remaining for
fixed control state,64/32/32 partitions and release-published complete counts.
Each partition has one writer at a time: arm→controller→joined-fallback handoff
for the controller partition, actual owner caller for parent, actual copied child
for child. Snapshot copies only committed prefixes and refuses poison/overflow;
it does not invent a total chronological order. Actual controller lifecycle
accounting is at most17 in-map records/18 including terminal map release, below
the24/32 bound. Terminal munmap cannot be journalled after destroying the map;
its real result and later mincore/FD observations must remain independent.

Six occupied targets predate owner entry. Guard/capture dup3 replaces existing
slots atomically; sources remain owned until ACK, preventing a recycled FD number
from masquerading as a closed-owner identity. For case28, clearing CLOEXEC on the
pre-birth >=5 guard placeholder creates a meaningful inherited sentinel. Parent
guard replacement cannot alter the child's private copy; absent child close_range
therefore cannot be hidden by exec's normal CLOEXEC cleanup. No seventh FD/channel
or accepted data is introduced. Actual flag/private-table/closure controls still
need the different helper tests and generated/runtime evidence.

The controller uses shared futex words with absolute real-monotonic deadlines and
15s fixture limits, separate from the10s owner clock. Real R readiness precedes
the selected offset/SIGINT/resource action. Case25 targets the recorded calling
main thread; no parent disposition/mask or Python callback is changed. Case27's
explicit controller consumes actual pinned status; watchdog never reaps. Normal
idle terminal cancellation wakes and joins without awaiting a nonexistent owner
phase, so unchanged stub31 can reach its genuine marker before fallback.

Kind8 records only the caller-supplied actual final observation under reviewed
revision2; its errno is-1. This generic recorder is not an independent proof of
quiescence or of truthful future caller metadata. The actual shared owner/FaultOps
source must place it after the real final decision/cleanup; no helper computes or
overrides a successful ABI integer. Parent PID/FD adoption, required phase and
independent pre-fallback FD/reap oracles remain mandatory.

Case28 capture tees actual core reads into257 fixed bytes. The retained
nonblocking duplicate reads only the unread suffix; exact248+EOF is necessary.
Wrong size, surplus, missing EOF/EAGAIN or poison refuses, and fields are copied
raw for independent literal assertions. It cannot create a report or change the
core decoder. The separately authored report writer still needs its own review;
this source verdict does not prove its write-before-activation-EOF ordering.

## Required caller and failure boundaries

All controls are serialized in a fresh fixture process. Real saved kernel
PID/pidfd/source identities are preconditions, not untrusted external inputs.
The caller registers cleanup before arm, holds source FDs through ACK and saves
all real oracles before fallback; it must create no new free FD between owner
return and descriptor disposal. Record/snapshot/capture callers may not introduce
extra writers or reuse an invalidated process/state. Actual test/source review
must enforce those conditions rather than infer them from helper status.

Failed/unacknowledged binding retains-1 state; it is not a clean or universally
recoverable disposal certificate. The original caller must retain and dispose
its actual source identity, and a failed fresh test process must be handled as
failure/disposable state. Join/quiescence failure cannot release the last unproved
identity/map. Tests must not assert that every such failed state returns0 or
silently retry it as green. Proven child/FD stages do survive a later failed-unmap
retry, as corrected above. Fallback help poisons before cleanup, never repairs the
saved oracle, and never sends a numeric-PID signal. No blanket allocation,
asynchronous cancellation or arbitrary-platform recovery guarantee is claimed.

I checked primary [absolute shared futex waits](https://man7.org/linux/man-pages/man2/FUTEX_WAIT_BITSET.2const.html),
[atomic dup replacement and shared open-file status](https://man7.org/linux/man-pages/man2/dup.2.html),
and [join termination semantics](https://man7.org/linux/man-pages/man3/pthread_join.3.html).
They support these source obligations, not actual generated-code/host capability.
Raw recorder/futex/clock helpers use narrow no-stack-protector attributes and
lock-free-size assertions; actual compiled child paths, atomics/TLS/call targets,
export map and final helper resource behavior remain separate gates.

Actually performed: full729-line original body/header/contract/matrix/reason read;
actual016/ffbf source correction review; independent literal29-tuple/57-plan,
map/journal/export/count checks; project426/122, render--check, diff--check PASS.
No source import, compile/link/load, control call, thread/child/signal, probe,
test/SDK execution or GitHub write was performed. Original Python/public/SDK/red
evidence remains unchanged. Finish PR77 then stop remains the scope.
