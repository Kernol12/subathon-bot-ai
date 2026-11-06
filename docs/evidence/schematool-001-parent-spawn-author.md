# Native spawn ownership regressions — SCHEMATOOL-001

Author `/root/session_supplier_author`,6 November2026. **AUTHORED; NOT REVIEWED OR
EXECUTED.** Existing isolated parent branch/worktree at7850a7106fb83931942b520602002698b9938ab5,
`task/SCHEMATOOL-001-parent-author` / `/tmp/subathon-schematool001-parent-author`.
Only NEW parent_spawn_regression.py and this note change. No production fix,
profile/contract, frozen64 assertion or earlier regression is modified.

Different C and root identified a constructor ownership gap separate from the two
already observed/fixed parent bugs: actual Popen may spawn, then raise before the
caller assigns its return to process. Caller cleanup still has process=None.
Root authorized these TWO focused tests and requires actual different source/
oracle review followed by new-only reached-bug red BEFORE ownership correction.
The tests add no public injection, success factory or validating fake.

## Source-backed requirement and actual native boundary

API§4 requires one child owned/reaped on failure or cancellation while Python can
unwind. Known allocation refusal yields resource_limit; it cannot leave a running
child simply because the Python constructor did not return. The frozen module-
attribute Popen/12argv/twoFD/minimal-environment boundary remains.

Read selected local /usr/lib/python3.14/subprocess.py, SHA256
7fa4af83422024a19e64a80937f6c8094244c3397372a495b348dfbe44a5dd72:
_execute_child sets pid/_child_created and performs further Python operations;
__init__ catches constructor failure to close IO handles and rethrows, without
unconditional kill/reap. __del__ polls and retains a live child for later polling;
it does not establish synchronous parent cleanup.

Independently checked primary tagged
[CPython3.10.12 subprocess](https://raw.githubusercontent.com/python/cpython/v3.10.12/Lib/subprocess.py),
[3.12.3 subprocess](https://raw.githubusercontent.com/python/cpython/v3.12.3/Lib/subprocess.py)
and [3.14.9 subprocess](https://raw.githubusercontent.com/python/cpython/v3.14.9/Lib/subprocess.py)
through the web tool: the same structural gap appears. The
[Python signal/thread rules](https://docs.python.org/3.10/library/signal.html#signals-and-threads)
state Python handlers run on the main interpreter thread. This supports a real
SIGINT fixture; no global handler or signal mask is changed. A private spawn thread
may avoid main-thread signal interruption inside the constructor, but by itself
cannot claim safe post-spawn MemoryError ownership. Both independently required
oracles below must pass; no implementation solution is preapproved.

## Actual test source and independent oracles

A stdlib-only inert module defines ParentSpawnRegression with2 ordinary test names
and one fixture helper. setUpClass lazily loads the trusted local parent; no SDK
or fixture operation runs at module import. Tests use native methods of the actual
selected Popen class, not a replacement successful constructor.

The helper records ONLY the two real parent os.pipe pairs before Popen-interposer
entry, excluding native Popen's internal error pipe. The frozen public module call
is still exercised and its actual argv/FD/DEVNULL/shell/close options checked.
The fault-only argv interposer launches a REAL tiny -I -S child with both actual
protocol writes plus separate readiness/blocking pipes. It acknowledges readiness
then blocks indefinitely on a held test pipe, sends NO activation or result, and
has no standards validator/package IO. It never forges validation success.

The test-only native _execute_child wrapper FIRST calls the SAVED ACTUAL selected
CPython method. Only AFTER native spawn/exec succeeds does it retain the real self/
PID/_child_created, close unused sync copies, obtain a real bounded readiness byte
and hold __init__ before return. Keeping that self strongly referenced prevents
__del__ or its poll from manufacturing parent cleanup. A real5-second watchdog
can kill ONLY and must not fire; it never reaps or supplies an accepted result.

- test_sigint_during_native_constructor_reaps_before_propagating requires the
  actual default SIGINT handler and leaves it unchanged. A controller waits for
  that real native hold, calls signal.pthread_kill on the actual main thread, then
  releases the hold. A bounded1-second constructor-interruption acknowledgement
  lets the old main-thread constructor see the signal before release; it is not
  the cleanup oracle. A future owned constructor thread need not itself receive
  main-thread Python signal exceptions. Require a real KeyboardInterrupt propagated
  by the public call; if caught at the native hook, require that SAME object.
- test_post_native_spawn_allocation_refusal_reaps_before_resource_limit raises
  the specific MemoryError POST_NATIVE_SPAWN_ALLOCATION_REFUSAL AFTER that saved
  native spawn/readiness and BEFORE constructor return. Require exact resource_limit
  and no logs, with no runtime/SDK fake. This tests known failure containment AND
  actual child ownership, not actual heap exhaustion or a successful worker.

BOTH capture REAL waitpid(WNOHANG) and ALL4 parent fstat states BEFORE any test
poll/wait/close/kill fallback. Require ChildProcessError (parent already reaped),
all4FDs closed, no watchdog/controller error and empty public logs. A running OR
waitable unreaped child fails; a probe-reaped child cannot pass. Diagnostics preserve
whether the outer factory returned and the actual FD states. Only AFTER the oracle
may finally release/join controller, cancel/join watchdog, close remaining sync/
parent descriptors and kill/wait a child. No test cleanup can establish parent reap.

This fixture deliberately covers ACTUAL native constructor work rather than merely
launching a completed Popen then raising from an unrelated wrapper. A future fix
may not add test-detection branches, bypass the frozen public module seam or claim
helper-thread-only safety from the SIGINT case while the allocation case fails.
If private ownership cannot meet both under the frozen boundary, report the precise
source/contract conflict for different review; do not weaken either oracle.

## Intended new-only observation and current verification

After ROOT's actual different source/oracle review, intended command:

```sh
env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8 /tmp/subathon-schematest001-py314/bin/python -I -B /tmp/subathon-schematool001-parent-author/tests/schema_validation/parent_spawn_regression.py
```

cwd the author worktree. Preserve real raw output/status/source/test/review hashes;
inspect constructor readiness/signal/factory/child/FD attribution. Native result is
FUTURE, not inferred from this plan. A handler/setup/readiness/timeout or unrelated
failure cannot substitute for reached constructor-ownership red. No full64 or old
red replay is requested.

Actual author checks ONLY: Python3.10-feature ast.parse/compile WITHOUT exec PASS,
2 test definitions/247 source lines; source hashes verify parent1579604, frozen64f
and previousacde regression unchanged. Project/render/diff checks accompany commit.
No candidate/helper/test import or body, SIGINT/controller/watchdog/native method,
child/probe/SDK/clone/installation executed. Production remains gated pending
actual review/red; corrected implementation needs different source review and all
relevant regressions/full64/ordinaryCI. All worker/offline/RLIMIT/no-site/read-race/
allocation and original product/platform/provider/live/release gates remain.

## FIRST actual approved new-only constructor red

Read actual private revision3/document approval93a5e1 and root's actual test-source
approval57a79f1 BEFORE execution. At unchanged cleana23026537a2fab9923fe77f5b71d06e4d35bc7f4,
parent1579604/test777d907, actual command above ran at
2026-11-06T12:37:06.899997+00:00. Native exit1,2 tests0.019s,2failures, external
0.11370279500260949s, no timeout; stdout empty. BOTH tests reached ownership
assertion with child_state running, factory_returned=[] and ALL4 parent FD states
closed. Prior real readiness/default handler/controller/signal or literal resource
enum/watchdog assertions passed. No setup/import/timeout failure substitutes.
Actual fallback cleanup ran ONLY after those captured oracles; no old64/FIRST replay.

Preserved raw files/receipt before interpretation:
/tmp/schematool001-parent-spawn-first-red.stdout.txt (0bytes, SHA256e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855),
/tmp/schematool001-parent-spawn-first-red.stderr.txt (3138bytes, SHA25614fe4719a6cdab8638829dd30a830c74e873c2d07f1dec09d6e1890022c70797),
/tmp/schematool001-parent-spawn-first-red.receipt.json (actual command/environment/
clock/source/parent/test/document/source-review/status/hashes). External observer
preserved native failure1 rather than inversion. Root retains durable-artifact
integration ownership. This is actual author attribution pending different review.
No ownership correction preceded this evidence or weakened its assertions.

## Coherent published-handle correction (unpublished PID remains OPEN)

AFTER actual red was anchored ina269e17, implement owned construction with ONE
private preallocated owner and ONE non-daemon helper thread. The actual factory
call remains module-attribute subprocess.Popen with the SAME argv/keywords/twoFD/
minimal environment. Native type/init code references are IDENTITY METADATA ONLY;
never call an alias, __new__, alternate factory, profiler, handler or signal mask.
Python signals execute in main, so the constructor thread can publish its actual
returned handle before caller interruption propagates. Sticky stop state prevents
an interrupted thread startup from later starting an unowned child; a constructor
that already entered is awaited before caller cleanup.

For native constructor failure AFTER a PID is published on self, walk at most32
exception control frames, match the EXACT saved native __init__ code and EXACT
native class, then accept only a builtin positive-int PID from that object's own
state. This recovers the REAL partial object, never a test PID, argument string,
protocol result or destructor guess. Materialize locals for ONLY that one native
frame; all its data are fixed stdlib/control values, not package trees/SDK values.
The exception traceback is cleared immediately after recovery; failure/control
references are cleared after synchronous cleanup. No exception/frame/cache is
stored globally or sent through the public return.

Actual selected3.10.12/3.12.3
[threading source](https://raw.githubusercontent.com/python/cpython/v3.10.12/Lib/threading.py)
and [3.12 threading](https://raw.githubusercontent.com/python/cpython/v3.12.3/Lib/threading.py)
were checked through primary web reads: interrupted join's lock exception path can
mark a thread stopped. Therefore REAL owner completion is published through an
explicit Event AFTER process/error recovery, not inferred from is_alive/join's
state after interruption. Join follows that ownership acknowledgement. No callback
from package data participates. Deadline is still recorded via module monotonic
immediately before the actual Popen call IN the constructor thread; startup/IO/exit
are charged to that same deadline and never reset on activation.

Exact corrected parent blob3438a92c9a98a4aa326ed733abffeee6c26c7502:
SAME new constructor2 tests actual PASS1.017s/exit0; previous cleanup/allocation2
actual PASS0.010s/exit0; reviewed11 parent methods actual PASS0.207s/exit0. Same
selectedCP314 -I -B/minimal environment/cwd as recorded commands. No assertions
changed. Initial owned-thread draft2 passed1.018s before the explicit completion
refinement; only1.017 is attributed to3438a92. No full64, old red, clone or SDK
probe/install ran. Static3.10 AST/compile and exact-one-module-Popen-call assertions
PASS; project/render/diff checks accompany commit. Regression777d/prioracde/frozen64f
remain exact. Different corrected CODE review remains required; these are author
observations, not independent approval or complete task acceptance.

### Specific allocation/ownership limitations retained for different review

This is a PUBLISHED-handle correction only. Primary selected CPython
[_posixsubprocess.c3.14.9](https://raw.githubusercontent.com/python/cpython/v3.14.9/Modules/_posixsubprocess.c)
returns PyLong_FromPid AFTER do_fork_exec; real MemoryError there may occur before
Popen.self.pid receives the native PID. No valid handle exists in that traceback
state. Traceback-local materialization itself also allocates bounded control
metadata; failure there cannot be advertised as established child cleanup. The
helper records/refuses failure but this source does NOT supply the missing kernel
ownership proof. No unqualified global-OOM guarantee or taskdone claim is made.

Root explicitly retains that known source gate and separately investigates a
preopened Linux per-birth-thread child-ownership fallback, keeping the birth thread
alive through recovery. No /proc/pidfd fallback, PID guessing, other-child reaping,
factory bypass, private runtime relaxation or test-detection branch is implemented
here. Any fallback needs source-backed bounded parsing/identity/reaping semantics,
a separate reviewed negative native-fork regression, actual reached red and
independent code review. Existing public64/worker/ordinaryCI/allocation/security
and product/release gates remain. No silent scope shrink or fake successful worker.
