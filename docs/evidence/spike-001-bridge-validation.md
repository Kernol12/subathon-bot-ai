# SPIKE-001 — real Drogon/Qt bridge validation

Coordinator/experiment author: `/root`. Test author: `/root/bridge_test_author`.
Independent test/code reviewer: `/root/bridge_reviewer` (code review remains pending).
Specification v0.45 SERVER-01, DB-01, §17/18; bounded experiment contract revision 1.
Work is isolated on `task/SPIKE-001-qt-drogon` (existing PR #4), updated from current
main `935e0d6`. Shared progress/HTML and experiment CMake belong to the coordinator.
No production server/core or the reviewed time-credit baseline is changed.

## Dependency and test-first decisions

The explicit [local setup](../../tools/setup_drogon.py) builds Drogon 1.9.14
`3aae769cc405cf5abb9d477675c6430924f02c7c` and its bundled Trantor 1.5.29
`78e480f5e138acdc05f76c55a2716718cd846364`. Both are shared, C++20/Release,
with OpenSSL and unused ORM/ctl/examples/tests/docs/Brotli/YAML/c-ares disabled.
No system packages or global paths are modified. [Dependency evidence](spike-001-dependencies.md)
records actual build/install, installed consumer, idempotence and refusal checks.
The persistent prefix is the primary checkout's ignored `build/deps/drogon-1.9.14`.
System Qt/compiler/transitive libraries remain system-managed.

The [experiment contract](../../experiments/spike-001/bridge_contract.md) and
[six tests](../../experiments/spike-001/test_bridge.py) were authored at `2888b9f`
(integrated `16d85c3`) before implementing queue behavior.
[Separate test review](spike-001-bridge-review.md) at `0c88c89` approved exact blobs
contract `ca66957b4ee96c07ed38fd4f5537e2694870a53e`, tests
`4d4c89388254f25a90b2b3b481b62afc7a6d76c5`.

A running explicit stub was committed at `b132189`; reviewed baseline execution
revision was `db5fedf`. It started real Drogon and Qt/SQLite infrastructure, emitted
synchronization events, and returned HTTP 501 / WebSocket FeatureNotImplemented
without committing any commands. This is harness code, not an implemented queue.

## Observed intended red before implementation

Commands from the feasibility worktree:

```sh
cmake -S experiments/spike-001 -B build/spike-001-real -G Ninja -DSPIKE_BUILD_REAL_BRIDGE=ON -DCMAKE_PREFIX_PATH=<primary-checkout>/build/deps/drogon-1.9.14
cmake --build build/spike-001-real
python3 experiments/spike-001/test_bridge.py --probe build/spike-001-real/spike_001_bridge -v
```

Configure/build exited 0. The first test invocation inside the network sandbox
could not create sockets (Operation not permitted); that is environment failure,
not red evidence. The same loopback-only tests were then run with the permitted
network sandbox override, with full combined output saved in
[spike-001-bridge-red.txt](spike-001-bridge-red.txt). Python propagated exit 1:

- Five behavior cases fail for missing stub behavior: four assert against HTTP 501
  FeatureNotImplemented where 200/503 is required, and the disconnected-command
  case independently reads empty SQL rows where literal `[41]` is required.
- The occupied-listener case passes as existing real framework infrastructure:
  actual bind failure exits nonzero without false readiness.
- All six execute without skipped cases, missing binaries, compilation errors or
  event/socket timeout failures. No assertion was changed to obtain this red.

This is a bounded spike baseline, not product acceptance. Readiness/SQL/stop flags
in the stub are scaffolding; the later actual implementation must compute/check
runtime values and receive independent code review. Final results are recorded
below only after actual execution.


## Actual implementation and final local result

Implementation followed the recorded red. All authoritative admission, held commands,
SQLite transactions and stop state are owned by a QObject on QCoreApplication's
thread. Drogon uses its own framework thread and one network IO thread. Each reply
is posted back to the exact originating IO loop; pending-reply bookkeeping returns
to Qt after that callback executes. SQL ownership and request/Qt/response-loop identity
are runtime checks, not constant success flags.

Readiness uses a second framework-main-loop post followed by an IO-loop barrier.
Pinned source executes beginning advice before startListening(), so direct startup
advice would report readiness too early. getListeners() supplies the actual ephemeral
port. The initial occupied-port failure follows the real framework's nonzero process
exit path; it is not graceful internal framework teardown.

Stop and SIGTERM close admission, drain admitted commands, schedule replies, then
flush/close connections before calling Drogon quit() (which otherwise force-closes
connections). The Qt loop remains alive until Drogon run() returns; the thread is
joined and the SQL connection is closed/removed on its Qt owner. SIGTERM only writes
a byte through a nonblocking self-pipe using signal-safe storage/errno handling;
handler restoration precedes descriptor closure. These statements describe this
bounded experiment, not a general production shutdown guarantee.

The first implementation compile failed on a helper named event colliding with
QObject::event. It was renamed observation; the subsequent build exited 0. An
accidental test invocation after that failed compile still exercised the previous
stub binary and is excluded from implementation evidence. The first actual compiled
implementation run passed disconnected HTTP, port collision and SIGTERM, but failed
three WebSocket cases on the expected close-frame assertion. Its full output is
retained in [spike-001-bridge-first-run.txt](spike-001-bridge-first-run.txt).

Source inspection found pinned Drogon WebSocketConnectionImpl handles a received
Close by half-closing underlying TCP before invoking the controller; pinned Trantor
then declines further sends. The probe therefore sends each ACK/error and initiates
its normal WebSocket close on the originating IO loop. The same six test fixtures
and assertions are unchanged. Contract revision 1.1 makes the bounded one-command-
per-WebSocket scope explicit; the independent reviewer agreed this clarification
before final source review. No vendor patch or generic/persistent WebSocket support
is claimed. Client-initiated close behavior for persistent production sessions
requires separate investigation before reuse.

The unchanged six-case command shown above then exited 0: all 6 passed in 0.056s.
The full result is [spike-001-bridge-green.txt](spike-001-bridge-green.txt). Tests
independently inspect actual persisted SQLite IDs/count/literal sums, including empty
rows while held, rather than trusting only ownership/completion instrumentation.
Final CTest, integration checks and separate code review are recorded next.
