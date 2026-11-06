# SCHEMATOOL-001 native-owner negative-oracle matrix

DOCUMENT ONLY. Author A, isolated branch
`task/SCHEMATOOL-001-native-owner-test-matrix`, base
`29902d46b4d1b7ab070ab006f580910276975b00`. Only this note changes. User scope is
finish existing PR77 and stop. This is a proposed exact test-source inventory,
not authored tests, coverage achieved, an implementation release or a run receipt.

Authority is the independently approved
[native contract aec16d6, sections 2 and 4–7](../contracts/schema-validation-native-process-v1.md),
[private process revision4](../contracts/schema-validation-process-test-v1.md),
[installation revision2](../contracts/schema-validation-installation-v1.md),
and the actual [74-method preservation map be753ee](schematool-001-native-test-mapping.md).
B's actual [fixture plan 5871869](schematool-001-native-fault-harness-plan.md)
received different review `26530af` (root import `de0a343`); this matrix does not
stand in for its forthcoming typed helper/source/build review. The actual
[foundation source 8e20a76](schematool-001-native-foundation-author.md) and root
build packet `29902d4` still await actual whole-packet review/runtime gates.

## Executable inventory proposed for separate source review

Proposed file `tests/schema_validation/native_owner_acceptance.py`, class
`NativeOwnerAcceptance`, test-only exact argv `--artifact ABS_TARGET_PATH`.
Root must separately freeze the fault artifact at
`<repository>/build/schema-transport-debug/lib/libsubathon_schema_fault_v1.so`.
There is no artifact search, environment/public fault option, callable replacement,
successful fake validator or new export. Ordinary domain/package/SDK/context
observations use the production DSO and real worker unpatched. Direct negatives
use the actual same-core fault DSO. The eight ABI1 argument types and four
test-control export types remain exactly native-contract section7.

The natural inventory is 12 ordinary methods, 57 selected data rows; these are
planning counts, not registered or observed tests. Each row runs in fresh fixture
state/process. No Cartesian expansion or generated expected outcome is proposed.

| Proposed `test_` suffix | Fixed rows | Case IDs / source of oracle |
| --- | ---: | --- |
| `original_activation_rows` | 3 | 1..3; frozen wrapper activation method |
| `original_outcome_rows` | 7 | 4..10; frozen wrapper outcome method |
| `status_mismatch_and_unclassified_death` | 2 | 11..12; frozen wrapper status/death method |
| `pre_activation_setup_rows` | 10 | 13..22; frozen wrapper setup-refusal method |
| `startup_deadline_from_original_t0` | 1 | 23; frozen wrapper startup deadline |
| `prebirth_facility_and_resource_rows` | 8 | 24; table P below, native sections6/7 |
| `acquisition_and_work_failure_rows` | 18 | 24; table W below, native sections4–7 |
| `cleanup_interrupt_and_release_once_rows` | 3 | 24; table C below, native sections5–7 |
| `real_sigint_before_and_after_close` | 2 | 25; before-close plus original cleanup/spawn cancellation obligations |
| `known_resource_after_atomic_birth` | 1 | 26; original post-birth known-allocation refusal, native sections4/6/7 |
| `competing_reaper_keeps_pinned_identity` | 1 | 27; native whole-process exit plus fresh same-birth-thread ECHILD |
| `raw_child_fd_signal_packet` | 1 | 28; exact native section7 NTF1 packet |

## One call lifecycle; failures cannot masquerade as missing behavior

Before entry, genuine build/ABI/kernel/helper foundations and trusted SDK/worker
binding must be green. Missing library/symbol/config, unsupported kernel policy,
arm/map/controller/child/readiness/capture failure and a skip are fixture errors,
never intended feature red. Tests prepare real readable immutable-lifetime path
buffers, builtin ctypes signatures and independent expected constants before the
owner call. Neither setup nor arm performs validation or starts a substitute owner.

Enter unconditional disposal scope before arm. Arm must return promptly without
waiting for a core phase. Invoke the actual ABI exactly once. If it returns 31,
immediately raise **exact** `SchemaTransportNotImplemented` with message
`Native schema transport is intentionally unimplemented`, before any phase,
readiness, child, result or cleanup assertion that the actual stub cannot satisfy.
This is an error/red, never an accepted result. Finally terminal-cancel the idle
controller and dispose fixture resources; this path must already have its own
independent green helper foundation. No second missing-feature return in a helper
or wait-for-nonexistent-phase timeout may replace the actual stub observation.

For every ordinary returned value or expected real SIGINT, first obtain the exact
raw snapshots/report and inspect **before fallback**:

- Actual selected phase/one-shot trigger and real return/adoption ordering. Both
  endpoints of every acquired pipe, config/null/source duplicates and atomic
  PID/pidfd have actual identities. BEFORE failure has no successful acquisition
  at that phase; AFTER acquisition retains real kernel success and owner adoption
  before its negative continuation. No guessed failed output cell is owned.
- Every recorded core FD is independently EBADF by saved real fcntl/fstat, before
  any new FD-opening operation could reuse a closed number. Ledger release alone
  is insufficient. Owned closed numbers are not retried. Test-owned descriptors
  and the shared map remain separately charged and subsequently disposed.
- For a born child, pinned whole-process exit plus an independent saved raw wait
  observation establishes no live child or zombie. An ordinary core reap must
  precede the observation; a test observation which itself reaps is a failure.
  Case27 alone records the deliberately separate controller reap. A fresh
  same-birth-thread ECHILD and pinned exit are both required there; ESRCH or
  exit-ready alone never certifies cleanup. No numeric-PID signal is permitted.
- No journal poison/overflow, controller error, watchdog firing, fabricated
  readiness or phase mismatch. Snapshot retains every committed event in its
  writer partition; its concatenated order is not a global time ordering.
  Parent/child/controller maxima remain 36/27/24 within 64/32/32 slots. Unknown
  fields stay -1, not invented zero. Any overflow fails rather than truncating.
- Direct CABI negatives produce no stdout/stderr/log/secret text. Public enum,
  CLI-line and exception integration assertions remain separately mandatory.

Only after capturing those observations does fallback kill/reap/close any leaked
actual resources and terminal-cancel/join the controller. A fallback reap cannot
repair the prior verdict. A saved-real-clock watchdog may kill, never reap, and
firing fails. Bounded helper/controller/capture failure is not a native31 red.

## Original 23 literal protocol and deadline rows

These values come directly from
[wrapper source64f4911](../../tests/schema_validation/wrapper_acceptance.py),
methods `worker_activation_missing_extra_and_malformed_refuse_forged_ok`,
`worker_outcome_bounded_exact_closed_protocol`,
`worker_status_mismatch_and_unclassified_signal`,
`worker_setup_refusal_before_activation`, and `worker_startup_deadline_kills_and_reaps`.
All use phase0/errno0. LF is exactly one 0x0a; empty includes actual EOF. Both
channels must reach actual EOF and status must be definitive after real reap.
Activation cap2/outcome cap257 and original absolute t0+10s remain unchanged.

| Case | Activation bytes | Outcome bytes | Termination | Literal native result |
| ---: | --- | --- | --- | ---: |
| 1 | empty | `{"code":"ok"}`+LF | normal0 | 17 |
| 2 | `AA` | `{"code":"ok"}`+LF | normal0 | 17 |
| 3 | `B` | `{"code":"ok"}`+LF | normal0 | 17 |
| 4 | `A` | 257 literal `x` bytes | normal0 | 17 |
| 5 | `A` | `{"code":"unknown"}`+LF | normal0 | 17 |
| 6 | `A` | `{"code":"ok"}` without LF | normal0 | 17 |
| 7 | `A` | `{"code":"ok"}`+LF+`extra` | normal0 | 17 |
| 8 | `A` | `{"code":"usage_error"}`+LF | normal64 | 17 |
| 9 | `A` | empty | normal0 | 17 |
| 10 | `A` | `{"code":"invalid_json","secret":"PRIVATE"}`+LF | normal2 | 17 |
| 11 | `A` | `{"code":"invalid_json"}`+LF | wrong normal0 | 17 |
| 12 | `A` | empty | real self SIGKILL | 17 |
| 13 | empty | exact35-byte `{"code":"environment_unavailable"}`+LF | normal3 | 4 |
| 14 | empty | same environment line | wrong normal0 | 17 |
| 15 | empty | empty | normal3 | 17 |
| 16 | empty | environment line+`extra` | normal3 | 17 |
| 17 | empty | environment JSON without LF | normal3 | 17 |
| 18 | empty | one leading space+environment line | normal3 | 17 |
| 19 | `B` | environment line | normal3 | 17 |
| 20 | `AA` | environment line | normal3 | 17 |
| 21 | empty | `{"code":"ok"}`+LF | normal0 | 17 |
| 22 | empty | `{"code":"invalid_json"}`+LF | normal2 | 17 |
| 23 | no A; real ready/blocked child keeps channels open | no result | actual deadline kill/reap | 5 |

Case10 retains normal2 and case8 normal64, preventing status mismatch from masking
extra-key/forbidden-code checks. Synthetic ok bytes are solely rejection inputs.
Case12's unknown signal is never inferred OOM. Case13 alone accepts no-A, only
with both EOFs/exact line/normal3/reap. Case23 advances the checked native clock
offset once beyond the original t0+10s after actual readiness, at the admitted
pre-remap hold. The independent watchdog uses real time; no sleep/stress or
restarted deadline is a substitute. This row does not prove exec or full FD closure.

## Selected native phase/errno rows

Case24 only. BEFORE=2*base; AFTER=2*base+1, first occurrence only. Each table row
is one exact arm tuple `(24, encoded_phase, errno)` and one independently fixed
native result. Named errnos are Linux x86-64 constants. Each chosen AFTER row
first executes the saved real operation; failed real prerequisites are fixture
failure, not the intended injected path. Acquisitions preserve real returns and
immediate owner adoption; after-close rows retain the real close/release before
negative continuation. These are meaningful selected operation boundaries,
not a claim to test every syscall, every errno or the full phase Cartesian set.

P: eight pre-birth rows, no actual child/PID/pidfd adoption; all earlier handles
must be gone. Trusted setup/facility policy failures are4; known descriptor or
process capacity failures are5, per native section6.

| Row | Base / operation | Encoded phase | Errno | Result |
| --- | --- | ---: | --- | ---: |
| P1 | 1 config-open BEFORE | 2 | EACCES | 4 |
| P2 | 4 activation-pipe BEFORE | 8 | ENOSYS | 4 |
| P3 | 5 outcome-pipe BEFORE | 10 | ENFILE | 5 |
| P4 | 6 source-duplicate BEFORE | 12 | EMFILE | 5 |
| P5 | 7 null-open BEFORE | 14 | EACCES | 4 |
| P6 | 8 atomic-birth BEFORE | 16 | ENOSYS | 4 |
| P7 | 8 atomic-birth BEFORE | 16 | EAGAIN | 5 |
| P8 | 8 atomic-birth BEFORE | 16 | EPERM | 4 |

P3/P4 must independently observe all earlier acquisitions, not just no birth.
P6..P8 are negative syscall injection, **not** proof of kernel rollback after
actual reserve/output paths. Actual atomic rollback is the independently reviewed
foundation's real short-size/EFAULT controls plus selected-kernel source review;
an injected pre-call error cannot replace them.

W: eighteen acquisition/work rows. Unless a child pre-exec facility row specifies
otherwise, the plan keeps the genuine trusted worker/SDK available; valid data is
actually validated and no injected successful result is used to reach read/wait.
For born/blocked rows the fixed native negative child can establish only real
readiness/ownership, not successful schema behavior. Exact child selection for
each fixed case/phase must be present in the separately reviewed helper source;
there is no arbitrary child/path option or new arm field.

| Row | Base / operation | Encoded phase | Errno / negative continuation | Result |
| --- | --- | ---: | --- | ---: |
| W1 | 2 config-read BEFORE | 4 | EINTR | 18 |
| W2 | 3 config-close AFTER real close | 7 | EINTR | 18 |
| W3 | 4 activation-pipe AFTER | 9 | ENOMEM | 5 |
| W4 | 5 outcome-pipe AFTER | 11 | ENOMEM | 5 |
| W5 | 6 source-duplicate AFTER | 13 | ENOMEM | 5 |
| W6 | 7 null-open AFTER | 15 | ENOMEM | 5 |
| W7 | 8 atomic-birth AFTER real adoption | 17 | EINTR | 18 |
| W8 | 9 parent-write-close AFTER real close | 19 | EIO | 17 |
| W9 | 10 child-signal BEFORE | 20 | EPERM | 4 |
| W10 | 11 child-dup BEFORE | 22 | ENOSYS | 4 |
| W11 | 12 child-close-range BEFORE | 24 | ENOSYS | 4 |
| W12 | 13 child-exec BEFORE | 26 | EACCES | 4 |
| W13 | 14 protocol-read BEFORE | 28 | EIO | 17 |
| W14 | 14 protocol-read AFTER actual read | 29 | EINTR | 18 |
| W15 | 15 protocol-poll BEFORE | 30 | EIO | 17 |
| W16 | 15 protocol-poll AFTER actual poll | 31 | EINTR | 18 |
| W17 | 16 wait/reap BEFORE | 32 | EIO | 17 |
| W18 | 16 wait/reap BEFORE | 32 | EINTR | 18 |

W2/W8 must not retry the already-closed number. W3..W6 separately record real
adopted handles (both pipe ends), then the known refusal and cleanup. W7 requires
actual native PID+atomic pidfd adoption before interruption, real child identity,
and actual termination/reap before18; it cannot pass on a pre-birth refusal.
W9..W12 require a real committed birth and reached child operation, no A or package
work, exact environment refusal/cleanup under the existing contract; helper
setup or an impossible errno/operation pair cannot become a passing row. W14/W16
require a successful real read/poll first; a BEFORE error or idle phase cannot
claim after-operation coverage. W17/W18 force a reached work-phase wait; cleanup
must still perform the real termination/status disposal after the one-shot fault.
Private18 is the literal interrupted native value; these injected errno rows do
not invent a Python signal or require KeyboardInterrupt without a real signal.

C: three cleanup rows. For C1/C2 the fixed real negative child emits `B` then
blocks with both writes open, so refusal17 is already committed. The journal must
prove that malformed input, actual child and selected cleanup operation were
reached. These rows test cleanup continuation, not independent classification.

| Row | Base / operation | Encoded phase | Errno | Result |
| --- | --- | ---: | --- | ---: |
| C1 | 17 pidfd-kill BEFORE, one shot | 34 | EINTR | 17 already committed |
| C2 | 17 pidfd-kill AFTER real kill | 35 | EIO | 17 already committed |
| C3 | 18 pidfd-close AFTER real close | 37 | EIO | 17 |

C1 must show a subsequent **real** pidfd kill and actual reap, not success after
skipping the kill. C2 records the successful actual kill first. C3 uses an
otherwise real ordinary result path, rejects success on the reached close error,
and observes the actual closed pidfd without a second close. No row models a
Linux close error by pretending a still-live FD was already released.
Final-commit base19 is source-reviewed for final monotonic acceptance/cleanup;
this selected matrix does not advertise an injected final-commit test or authorize
an impossible errno on a non-syscall checkpoint. Any additional dynamic boundary
test needs its own written fixed plan and review; the deadline/final-check
implementation obligation is retained.

## SIGINT, post-birth resource, competing reap and raw child state

| Case/rows | Exact arm phase/errno | Literal expected result and indispensable observations |
| --- | --- | --- |
| 25a / 25b | BEFORE parent-write-close18 / AFTER parent-write-close19; errno0 | Real native controller sends SIGINT to actual calling main thread, with real ready/blocked child. Require actual pending-signal/interrupted native path and KeyboardInterrupt propagation only after all core cleanup. Handler/mask unchanged. AFTER row records actual close/release before signal; BEFORE row retains ownership through the real close. |
| 26 | AFTER atomic-birth17; errno0 | Native5. Actual committed PID/pidfd adopted and actual blocked-child ownership/readiness precede fixed known resource refusal; no allocator exhaustion or unpublished Python PID substitute. Core reap/FD closure precede return. |
| 27 | phase0/errno0, fixed reaper plan | Native17. Real controller consumes actual status of the pinned child; raw pinned exit plus fresh same-birth-thread ECHILD are both observed. No claimed ordinary status, numeric kill, PID-reuse stress, watchdog/fallback reap or controller-as-core reap. |
| 28 | phase0/errno0, fixed descriptor/signal plan | Native17, no logs; exact248-byte packet and raw literal field checks below, real termination/reap and all core FD checks before fallback. |

For25, an actual CPython pending signal can raise before the C integer is exposed
to Python. Assert interrupted18 from the native final-commit observation when
committed, and the actual exception through the real CDLL call; never fabricate
a received18 or prebuilt exception to stand in for native ownership. No Python
callable/handler/controller participates in the resource interval. Existing
exception-identity assertions remain in complementary genuinely observable
pre-entry/post-return Python cases under separate binding-source review. They
are not silently replaced with an invented native exception object.

The fixed Case28 reporter uses actual remap/close_range/exec, with **no extra
inherited test FD**. While3/4 are open, gather; close only the new scan FD; commit
one full248-byte write on4 while3 stays open; close3 without A; close4; attempt
exit3. Normal3 is not required because refusal can kill it after packet commit.
An incomplete/short write, missing proc, overflow or incomplete EOF is fixture
failure. The parent-only capture duplicate is closed by child close_range and
may collect only unread committed suffix after return; it cannot alter core reads,
withhold protocol bytes, reap, or manufacture a complete packet.

Parse explicit little-endian fields, never native struct padding or a clean bool:

| Packet field | Literal independent oracle |
| --- | --- |
| Magic4 | bytes `NTF1`, intentionally invalid protocol |
| PID / scan-FD / count (3 signed32) | actual born PID / 5 / 6 |
| 17 signed32 descriptor slots | `[0,1,2,3,4,5]` then eleven `-1`; no truncated17-as-fitting inventory |
| F_GETFD0..4 (5 signed32) | all0, all actually open without CLOEXEC |
| F_GETFL0..4 (5 signed32) | access mode2 on0..2; access mode1 on3/4; O_NONBLOCK bit0 on all; preserve other raw bits for diagnostics rather than inventing them |
| Blocked mask64 | 0 |
| SIGINT/SIGPIPE/SIGCHLD/SIGTERM, each result32+handler64+flags64+mask64 | raw result0, handler0, flags0, mask0 for each |
| Terminal getdents signed32 | 0, complete EOF |

Exact size is4+12+68+40+8+112+4=248. Fixed raw enumeration uses4096 bytes and at
most17 numeric entries. Core receives the deliberately invalid original bytes;
ordinary protocol/priority rows never receive this packet. A17 without the full
raw packet and pre-fallback resource observations cannot pass descriptor/signal
state. This is the future full>=5/remap/Case28 oracle, unlike the foundation's
single live extra-FD close.

## Preserve all 74 existing methods and their distinct integration obligations

The approved [per-method map](schematool-001-native-test-mapping.md) remains the
complete74-name authority, with all five source blobs unchanged at this base:
wrapper64f4911(64), cleanupacde40d(2), contextb34a564(3), lexicalfc82684(3),
spawn777d907(2). This direct57-row matrix does **not** supply public64 or full74
proof. Existing valid source reviews and historical FIRST63/261 plus ONLY-new1/10
and subsequent focused FIRST receipts remain preserved; no replay is ordered.

The following exact existing methods/helpers need later separately reviewed
changes under the source-based native-boundary reason, not deletion or weaker
assertions:

- Wrapper `fault` and its five `worker_*` callers: port the real negative child,
  protocol tuples, clock/readiness, deadline, all actual FD/reap and unconditional
  cleanup together. Native direct rows supplement their public enum/CLI boundary;
  old Popen/os.pipe/Python-clock mocks cannot become vacuous green.
- Wrapper `parent_pipe_enosys_refuses_without_worker_or_logs`: replace the private
  Python pipe failure with the actual native facility row/public binding, preserving
  environment_unavailable/no child/no logs; direct native4 alone is insufficient.
- Wrapper `help_static_without_worker_or_package_io` and
  `sysconfig_environment_presence_including_empty_refuses_unchanged`: keep existing
  no-read/no-callback/no-spawn assertions and add actual native load/entry no-call
  proof; all six environment-presence rows and unchanged environment remain.
- Cleanup `cancelled_descriptor_close_reaps_and_closes_before_propagating`:
  retain actual after-close interruption, pre-fallback core reap and four-channel
  FD closure/no logs; use the actual native close boundary. Preserve original
  exception identity wherever a preexisting Python exception actually exists.
  `initial_path_allocation_refusal_is_resource_limit_without_setup`: retain known
  MemoryError/resource_limit and no setup/IO/birth; extend native load/entry guards.
- Spawn `sigint_during_native_constructor_reaps_before_propagating` and
  `post_native_spawn_allocation_refusal_reaps_before_resource_limit`: port the
  real signal/held birth/resource sites and exact pre-fallback disposal, not the
  former Popen publication mechanism. Case25/26 supplements these public assertions;
  pre-entry/post-return Python allocation/exception-identity sites require their
  own precise source-backed binding fixture and different review. No reachable
  ctypes c_int-boxing MemoryError is assumed without actual source evidence.
- Context all three `worker_context_regression.py` methods: preserve the raw
  Schema sibling semantic negative, both OAS extension negatives and all three
  DATA/dynamic-name/standalone positives, and run through the actual unpatched
  public native-owner+worker path. Worker-only helper results are not owner proof.

All remaining wrapper package/path/JSON/numeric/closure/inventory/metadata/SDK
oracles and lexical three retain their bodies/independent literals unless an
actual test defect is written and differently reviewed. Actual SDK-clone positive
controls must pass before whole WHEEL/METADATA corruption/removal can establish
dependency_unavailable; missing native artifact cannot mask them. Outer real
script launches remain real public scripts with exact lines/statuses/redaction.
All14 SDK foundations and20 exact same-artifact distribution identities remain.

## Sequence and actual checks

Root/C review this exact matrix and the separately authored typed declarations.
Only an explicit assignment then releases fixture/helper/test-source authoring;
their actual different source/oracle/build review precedes any execution. Green
real loader/ABI/kernel and independent fixture foundations on both selected hosts
precede the focused actual-stub31 intended red, preserved raw bytes/status/source
and separate attribution. No fixture setup failure may count as that red.

Only after those gates may a separate assignment implement the same common
owner with RawOps/FaultOps and the Python binding. Later checks must include all
reviewed new negatives, all preserved/ported74 methods and14 SDK methods, actual
private resource/async-safety/call-graph/offline source audit, unconditional
both-host CI and latest-head normal PR77 merge gates. This document claims none
of those future results and does not complete SCHEMATOOL or any product feature.

Actual author checks: a stdlib source-only Python here-document parsed the five
existing test files with Python3.10 grammar, verified counts64+2+3+3+2=74, the
12-name/57-row inventory, original case IDs1..23, P1..8/W1..18/C1..3, exact35-byte
environment line and independent248-byte arithmetic: PASS. Its initial counting
expressions were corrected to include the digit in `t0` and scope the original
table separately from later case26..28; no source/oracle was changed for that
check. `git ls-tree HEAD` confirmed all five preserved source blobs above.
`python3 tools/check_project.py` PASS426/122; `python3 tools/progress.py render --check`
current; `git diff --cached --check` PASS before commit. Only this note changes.
No candidate/native/test import, compile/load/probe, thread/child/signal or test
execution was performed or is authorized by this document assignment.
