# SCHEMATOOL-001 — bounded native fault-harness plan

Author B (`/root/session_supplier_author`), 6 November 2026. **PLAN ONLY, pending
different actual review.** Isolated `task/SCHEMATOOL-001-native-fault-harness-plan`
starts at actual root `aedbec004613b511626304919ed554d2db19d8c3`. ONLY this note
changes. User scope is finish open PR77 then stop; no new PR/task/product scope.

Read root/tools/tests instructions, approved native contract `aec16d6` section 7,
process revision4/installation revision2, C policy48c075/native44adb/interfacec0b,
the actual [74-method map](schematool-001-native-test-mapping.md), and current
header/stub/direct-ABI tests. This plan adds no header/test/fixture/owner/binding,
build/CI/policy edit or runtime observation. Root/C must review this actual plan,
then separately assign/review minimal source and independent tests before execution.

## 1. Keep the owner unimplemented until its negative baseline exists

Compile the SAME approved `tools/schema_validation/native/transport_stub.cpp`
bytes into both production and fault DSOs initially. Its entire body remains
literal `return 31;`, no argument/phase/clock/resource operation. Production keeps
only the exact existing versioned ABI. The fault DSO additionally links the four
test-control exports in native-contract§7; it must not replace the owner entrypoint
with a test driver, decoder, synthesized result or worker implementation.

Before the reviewed new red, declarations and genuinely nonvalidating helper
infrastructure are allowed under a separate assignment. No path admission loop,
configuration reader, two-pipe owner, process launcher/decoder/deadline/reaper
algorithm is implemented behind the production entrypoint. Calling the actual
stub in the fault DSO must reach its actual 31, not a second fake missing-feature
return in a helper. Different source/export/link review proves that identity.

The later production owner is one private C++20 algorithm instantiated with
compile-time `RawOps` or `FaultOps`. One shared entry source plus one common
algorithm source contains ALL admission, handle slots, loops, protocol decoding,
deadline, classification and cleanup. Only the Ops type differs at build time.
No runtime mode, Python/native function-pointer callback, std::function, virtual
dispatch, public option, environment flag or production test detection. The same
algorithm source hash/options must be recorded for both instantiations, along
with the explicit target-only Ops selection. Fault helpers cannot copy its decoder
or implement an alternate successful owner. This later source switch is an
independently reviewed implementation change AFTER negative test review/red; the
initial literal stub is not silently refactored into working behavior beforehand.

## 2. Minimal private interface and proposed file ownership

The following are proposed later assignments, not authored files or permission to
edit them now. Root owns final bounded claims and build/export selection.

| Paths | Proposed role and stage |
| --- | --- |
| Existing `tools/schema_validation/native/transport_v1.h`, `transport_stub.cpp` | Keep exact ABI and inert common entry for both initial artifacts. Any actual change needs its own different source review. |
| NEW `tools/schema_validation/native/transport_core_v1.h` | B minimal private declarations: four borrowed pointer/length pairs, raw syscall result, phase/action types and template algorithm declaration ONLY. No exported helper or runtime owner body before red. |
| NEW `tests/schema_validation/native_fault_v1.h`, `native_fault_fixture.cpp` | B separately assigned minimal test-only four-export definitions/control context; common contract types, bounded map/journal, arm/snapshot/report/fallback and controller lifetime. No accepted result/owner algorithm. |
| NEW `tests/schema_validation/native_fault_ops.h`, `native_fault_child.cpp` | A or separately assigned harness author: fixed native Ops adapter, negative child/reporter. Independently reviewed real helper-source foundations; no schema validator or successful worker. |
| NEW `tests/schema_validation/native_fault_foundation.cpp` | A independent raw helper/controller/child foundations, not owner acceptance. |
| NEW `tests/schema_validation/native_owner_acceptance.py` | A independent direct-CABI negative behavior/phase/oracle tests; real fault DSO only. Natural reviewed inventory, not a fabricated count. |
| Later `tools/schema_validation/native/transport_core_v1_impl.h`, `transport_entry.cpp`, `raw_ops_v1.h` | B production implementation ONLY after actual reviewed/red gates; common algorithm/type-selected trampoline/raw kernel Ops. They replace the initial stub in both target source lists together. |
| Root CMake/presets/export maps/workflows/state/HTML | Root only, separately reviewed before build/execute; exact production sole export and fault ABI+four exports. |

Private interface intent (document notation): `Request` is exactly four `PathView`
values, each const unsigned-char pointer plus unsigned-int length; no owned string,
caller graph, result object or new public ABI. `RawResult` carries native signed
return/errno without Python allocation. `Ops` supplies only:

- A closed, statically checked raw Linux syscall operation with its native fixed
  scalar/pointer arguments/result. Its permitted calls are exactly those required
  by the approved owner (cfg IO/stat, pipe2/fcntl, clone3, dup/close-range/exec/exit,
  poll/read/write, signal setup, pidfd signal/wait and monotonic clock). No numeric
  PID kill or arbitrary user callback. Exact typed declarations receive actual
  minimal-header/source review; this is not a generic injected validation service.
- Native monotonic observation. RawOps reads the real clock; FaultOps delegates
  to that clock plus the one permitted checked offset. The independent watchdog
  always uses the saved real clock, never the offset clock.
- One selected before/after checkpoint and fixed continuation enum: continue,
  known-resource refusal or interrupted. It cannot return a domain/accepted code.
  RawOps has no test state. FaultOps can trigger only the admitted fixed plan;
  AFTER acquisition preserves real returns/adoption before a negative continuation.
- Fixed native observation hooks. The fault adapter independently records actual
  successful kernel FD/PID acquisitions and real close/wait results, including both
  pipe endpoints; it does not depend on the owner claiming that bookkeeping/cleanup
  succeeded. Owner-slot adoption remains in the common algorithm and source review.

No production algorithm currently calls those declarations. The first real owner
implementation must demonstrate that all its operations traverse this interface,
with no hidden alternate raw calls escaping the test observations. Pre-exec Ops
operations are raw syscall/inline fixed-store only; no controller thread creation,
mutex/allocator/loader/Python call is allowed in the child. Compiler/source/linked
call-graph audits remain required, including inline lock-free map publication.

## 3. Exact fixture contract and an idle-stub-safe lifecycle

Keep native-contract§7's exact four C exports, unchanged signatures:
`subathon_schema_fault_arm_v1`, `subathon_schema_fault_snapshot_v1`,
`subathon_schema_fault_report_v1`, `subathon_schema_fault_fallback_v1`.
No fifth export or success factory. Keep the 128 eight-int64 records, parent64/
child32/controller32 partitions and maxima36/27/24; one 12288-byte anonymous
MAP_SHARED region contains the 8192-byte journal plus fixed counts/latches/capture.
Preserve its writer IDs, publication, poison/overflow behavior, test-owned six-FD
limit and separate native owner eight-FD/32768-byte ceiling. Fresh process/state
per case, never reset poisoned evidence into a passing run.

`arm` validates exactly one closed case/phase/errno and prepares ONLY test-owned
mapping/controller/coordination state. It MUST return without waiting for core
entry, birth, readiness, A, outcome or any owner checkpoint. A controller may wait
on its native test latch with saved-real-clock bounds; it may act only AFTER an
actual relevant owner phase/child acknowledgement. Arm never creates a substitute
owner child or advances the test clock merely because it was called. A failed
map/controller setup is -1/fixture failure, not a future 31 attribution.

The source-reviewed test call sequence is:

1. Enter unconditional fixture cleanup scope BEFORE calling arm, so Python
   cancellation after an arm call cannot strand its test resources.
2. Arm, then invoke the REAL eight-argument ABI from the actual fault DSO using
   the reviewed immutable bounded buffers/builtin ctypes binding.
3. If and ONLY if that real call returns 31, immediately raise exact
   `SchemaTransportNotImplemented`. Do not wait/assert a phase/birth/readiness that
   an inert stub cannot emit. This is an ERROR/red, never a successful bypass.
4. For any ordinary result or expected real SIGINT, require actual planned trigger,
   child/phase identities and every applicable raw outcome/FD/reap oracle BEFORE
   fallback. An early arbitrary failure with no trigger cannot pass a later-phase
   cleanup test. Unknown exceptions fail rather than masquerade as expected SIGINT.
5. Finally call fallback. It signals terminal-cancel to an idle controller, wakes
   it, joins/disposes test resources, and only AFTER oracles kills/reaps/closes
   leaked actual identities. It may neither repair the earlier verdict nor count
   its own reap as owner cleanup. A watchdog may kill only, never reap, and firing
   always fails. No fallback waits indefinitely for an unentered stub phase.

Consequently the exact current stub produces a prompt named 31 red even for the
deadline/SIGINT/reaper plans: no child/phase is fabricated to obtain it. A separate
green helper test must exercise arm→cancel/fallback with NO owner entry, proving
this idle-controller path terminates and disposes resources. That helper green is
not an owner cleanup observation. Actual bounded idle/controller source and tests
need review before their first run; no passing helper behavior is asserted here.

## 4. Preserve the literal protocol families

Case IDs retain frozen source order. `LF` below means ONE byte 0x0a; `empty` means
zero bytes plus channel EOF. All standard malformed rows expect native17, no logs,
and actual owner termination/reap/core-FD closure BEFORE fallback.

| Cases | Exact original activation/outcome/termination | Expected native value |
| --- | --- | ---: |
| 1..3 | Activation empty / AA / B respectively; outcome `{"code":"ok"}`+LF; normal0 | 17 |
| 4 | A; 257 literal x bytes; normal0 | 17 |
| 5 | A; `{"code":"unknown"}`+LF; normal0 | 17 |
| 6 | A; `{"code":"ok"}` WITHOUT LF; normal0 | 17 |
| 7 | A; `{"code":"ok"}`+LF+`extra`; normal0 | 17 |
| 8 | A; `{"code":"usage_error"}`+LF; normal64 | 17 |
| 9 | A; empty outcome; normal0 | 17 |
| 10 | A; `{"code":"invalid_json","secret":"PRIVATE"}`+LF; normal2 | 17 |
| 11 | A; `{"code":"invalid_json"}`+LF; wrong normal0 | 17 |
| 12 | A; empty outcome; real self SIGKILL | 17, never inferred OOM |
| 13 | Empty activation; exact35-byte `{"code":"environment_unavailable"}`+LF; normal3 | 4 ONLY after both EOFs/normal3/reap |
| 14 | Empty activation; same environment line; wrong normal0 | 17 |
| 15 | Empty activation; empty outcome; normal3 | 17 |
| 16 | Empty activation; environment line+`extra`; normal3 | 17 |
| 17 | Empty activation; environment JSON WITHOUT LF; normal3 | 17 |
| 18 | Empty activation; one leading space then environment line; normal3 | 17 |
| 19..20 | B / AA activation respectively; environment line; normal3 | 17 |
| 21 | Empty activation; `{"code":"ok"}`+LF; normal0 | 17 |
| 22 | Empty activation; `{"code":"invalid_json"}`+LF; normal2 | 17 |
| 23 | Actual ready/blocked pre-exec child; one offset advance beyond original t0+10s | 5 plus real kill/reap, no watchdog-fired pass |

These are the original 3 activation, 7 outcome, 2 status/death, 10 setup and 1
deadline observations, not newly invented passing worker responses. Capture raw
bytes without newline normalization; do not pair the extra-key case with wrong
normal0 or the forbidden usage_error with wrong normal0, which would mask the
intended refusal. A synthetic ok line is never a successful validity case.

## 5. Required additional ownership observations

Cases24..28 and the 19 phase bases/BEFORE=2*base, AFTER=2*base+1 remain exactly
native-contract§7. One-shot errno choices and impossible-plan rejection remain;
do not claim a full Cartesian test matrix. A's actual independent test source
must enumerate its precise natural rows and coverage before review/run.

| Family | Required real observation; cannot be replaced by an early refusal |
| --- | --- |
| 24 pre-birth facilities/resources | Native pipe2 ENOSYS→4/no birth/no logs; known resource exhaustion→5. Raw acquisitions before the selected failure and every later release are journalled and independently fstat-checked. Config, pipe, null/normalization failure boundaries remain source/test gates. |
| 24 after successful acquisition / transport IO | Real syscall FIRST, record both actual pipe ends or successful atomic PID+pidfd; only after adoption may negative continuation occur. Unexpected EIO→17; known ENOMEM→5. No false kernel rollback or fabricated post-birth test. Cover read/poll/wait/kill/close error handling by reached phase and actual disposal, not return code alone. |
| 25 real SIGINT | Real native controller targets the actual calling main thread at the admitted pre-close AND after-real-close/ownership phases; handlers/masks unchanged. Actual native interruption and Python propagation only after cleanup, with real pre-fallback FD/reap evidence. Do not fabricate a Python exception/identity at a native signal boundary. |
| 26 post-birth known resource refusal | Actual committed PID+pidfd/blocked child before injected known refusal→5, with child already reaped and all recorded core FDs closed. Pre-entry failure cannot satisfy this. |
| 27 competing reaper | Real controller deliberately consumes the pinned child's status; record that separate actor/actual status. Core must observe pinned exit plus fresh same-birth ECHILD and refuse17. This deliberate reaper is not fallback/watchdog; ordinary cases still require CORE reap. No numeric-PID kill or stress attempt to force PID reuse. Kernel identity foundations/call graph supply the non-retarget proof. |
| 28 child FD/signal state | Exact248-byte invalid NTF1 report and independent raw masks/sets/flags, plus public17/no logs and real termination/reap/FD disposal. Follow the reviewed gather-open3/4→close-only-scan→full atomic write4 while3open→close3/noA→close4/attempt3 order. Parent-only capture duplicate collects committed suffix only; no extra inherited test FD or normal3 requirement. |

Raw kernel return/identity observations come from FaultOps around actual native
operations, not an owner's clean boolean. Record actual parent birth PID and
atomic pidfd only on successful clone3; failed output cells stay UNOWNED. Child
pre-exec observations use the shared mapping, never its private post-fork copy.
After exec the map is gone; case28 uses existing outcome4 and a bounded separate
raw report assertion, never diagnostic bytes in ordinary protocol/priority cases.

Before fallback: saved raw waitpid/waitid proves no live child/zombie and fcntl/
fstat checks every actual recorded owned FD, including config/null/duplicates/
pidfd, with no new FD-opening operation that could reuse a closed number before
that check. Direct-core tests own no unrelated child and do not let poll/wait/
test disposal manufacture the evidence. A failed oracle still runs unconditional
real disposal afterwards. Source audits cover unreachable allocation sites and
every untested security/resource branch explicitly, not a blanket coverage claim.

The 74-map remains mandatory: port all five fault callers/helper together; retain
cleanup2/spawn2 exception/FD/reap literals with the written mechanism reason;
strengthen help/env/allocation guards for native entry; move private context3
observations through real public integration; keep lexical3/normal64/SDK14 and
20 distribution identities. This plan does not edit/retire any old assertion or
approve public binding behavior. Direct-core negatives supplement those later
ports; passing them cannot stand in for the whole integrated inventory.

## 6. GREEN fixture foundations versus the missing-owner RED

After actual source/build/oracle review, fixture foundations independently use
real kernel primitives and fixed nonvalidating children to establish: exact four
export types; map publication/segments/poison/overflow; fresh-state rejection;
idle arm→terminal cancel; controller readiness/signal/watchdog disposal; literal
child bytes; bounded report capture/EOF; actual raw FD/wait observations. Helper
controls may own disposable resources to test THEMSELVES, but do not call a copied
owner/decoder or classify raw output as an accepted product result. They run a
small independent fixture driver, not a successful schema backend. No library,
setup, kernel/seccomp, child readiness or helper failure is feature red.

Only after actual BOTH-host C++/ABI/loader/kernel/helper and pinned-SDK foundations
are GREEN does the independently reviewed negative behavior suite invoke the
shared actual owner stub. Its named 31 errors are the NEW missing-owner red, with
exact actual source/export hashes, raw output/status/time and different attribution.
The 12-method/22-observation direct behavior baseline retains its separate scope;
neither it nor fixture green already establishes the negative owner baseline.

Then root may separately release implementation of common owner/RawOps and the
binding. Independent source/call-graph/allocation/async-safety/security review,
all required negative cases and preserved/ported acceptance, unconditional actual
both-host CI/latest-head rules/normal merge remain. No native owner capability,
new red/pass, task completion or permission to implement is claimed by this plan.

## Actual author verification

This assignment performs source/document reads and ordinary repository/link/render/
whitespace checks ONLY. `python3 tools/check_project.py` PASS426/122/current HTML;
`python3 tools/progress.py render --check` PASS; `git diff --check` PASS. Only this
note changes; parent3438/header04e8/stube04d/direct-tests90c54b5 remain identical.
No new header,
helper/fixture/test source, native/Python owner or binding, build/policy/CI/state
edit; no candidate/SDK import, compile/link/load/call, probe, child/thread/signal,
test execution or installation. All source and previous evidence remain frozen.
