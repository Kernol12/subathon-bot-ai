# SCHEMATOOL-001 — native private contract author

Author B (`/root/session_supplier_author`), 6 November 2026. Owned paths ONLY:
`docs/contracts/schema-validation-native-process-v1.md` and this note. Author base
`46a464aa1621f6eee3fceceb5cb3312a746dcdb4` on the preserved isolated parent-author
branch/tree. Root assigned concrete DOCUMENT authoring after different planning
approval C `f94b1ef1e061110b62e4fd05c248473a4e430a03`; this is not that reviewer's
approval of this new contract, a native source/interface approval or a capability.

Read root/tools/tests AGENTS, ADR0008, implementation brief, logical/API/context/
installation/private-process contracts, selected CMake/presets/GoogleTest setup,
actual proposal46a/blob4240, C planning reviewf94 and A complete mappingbe753ee/
blob96130822. The map accounts for all 74 existing methods and explicit native
oracle gaps; original sources/reviews/red/functional evidence are not repeated or
relabelled. Root's newer functional pass does not close native resource gates.

## Concrete choices and boundaries

The contract freezes versioned C ABI/path and integer mapping, CDLL without Python
callbacks, bounded native copies/scratch/handle count, native parent-cfg ownership,
atomic clone3 PID+pidfd ownership commit/rollback, child3/4 and exact exec/env,
signal state, deadline/mandatory cleanup and closed failure classifications.
CLONE_CLEAR_SIGHAND is selected alongside CLONE_PIDFD so caught Python handlers
reset at birth; child ignored dispositions/mask reset through raw kernel setup.
Parent-global signal state remains untouched. This is source-backed design, not
an executed signal/facility claim.

Error pidfd cells are explicitly UNOWNED: selected kernels can write the number
before a later failure releases it. Only positive successful birth commits owner
slots; no blind close of a reused number. Other-reaper handling requires pinned
whole-process exit plus fresh same-birth-thread ECHILD under the admitted profile,
refuses normal success without status, and still requires actual selected-kernel
proof. Eight native descriptors/32768 automatic bytes are reviewable ceilings,
not measured implementation observations. Deadline starts before native acquisition;
no ordinary result commits at/after 10 seconds, and mandatory quiescent cleanup
cannot return early merely to meet a non-real-time wall promise.

The negative-only seam is a separate DSO using the same owner/core bytes/options,
native syscall/phase/clock adapter, real negative children, bounded raw identity
ledger and pre-fallback FD/reap observations. No callable/result replacement or
production fault option. The contract fixes the three test-only C helper signatures,
64-record/eight-int64 layout, 28 closed case IDs and 19 phase bases; their actual
minimal header/source still needs separate review, and no header is authored here. Normal/domain
checks remain real production owner+worker+SDK. The 74-map preserves literal
oracles while requiring mechanism ports and stronger native no-call guards.

Root separately owns tools/ADR/private-process/installation reconciliation. Both
ordinary hosted targets must prove the conditional atomic backend before release;
unavailable facilities are not a positive pass or missing-feature red. No unproved
fork/pidfd_open/raw-clone fallback, new dependency or reduced product scope.

## Primary evidence and actual work

Primary web reads: Python3.10 ctypes documentation and tagged3.10.12 callproc.c;
Linux man-pages clone/wait/execve/close/close_range/pidfd_send_signal; Linux5.15 and
6.12 fork.c,6.12 exit.c/pidfs.c. Direct links sit beside the contract claims.
These support interface/ordering design; they do NOT establish selected hosted
kernel/seccomp/ABI/build/signal behavior. In particular no assumption that a
successful pidfd output cell is owned after an error, or that POLLIN alone reaps.

Commands are repository/source reads and primary web reads only before authoring.
Post-author checks: `python3 tools/check_project.py` PASS (426 task records,
122 decisions/current HTML), `python3 tools/progress.py render --check` PASS,
`git diff --check` PASS, and changed-path/frozen-source identity inspection.
Parent source remains `3438a92c9a98a4aa326ed733abffeee6c26c7502`.
No production Python/C++, header/stub/test, CMake, progress/HTML, policy/AGENTS,
contract owned by root, dependency, wheel or CI changed. No native probe, compile,
build, candidate/SDK import, test/helper/worker body, clone/child/signal or install
was executed. Different actual contract and shared-policy reviews are next; later
minimal harness/test review, real green foundations, new red, code review/full
acceptance/hosted receipts remain mandatory separate gates.
