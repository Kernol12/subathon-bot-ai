# SPIKE-001 — real Drogon/Qt bridge validation

Coordinator/experiment author: `/root`. Test author: `/root/bridge_test_author`.
Independent test/code reviewer: `/root/bridge_reviewer` (separate approved reviews).
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


## Independent code review and integrated checks

[Separate code review](spike-001-bridge-code-review.md) at `674bbb2` approved
implementation `a54fef1`, bridge blob `4fbfd28247fc37b0065c859b60e4764337bc7edd`,
clarified contract 1.1 blob `96b0eb64123f6f53801f06321fa3a28541a2088c`, and
unchanged test blob `4d4c89388254f25a90b2b3b481b62afc7a6d76c5`.
The reviewer independently configured and built in a separate worktree, then actually
ran CTest: exit 0, 2/2 including all six real cases. This approves only the bounded
feasibility experiment and explicitly excludes production reuse/persistent WS claims.
The separate [dependency review](spike-001-dependency-review.md) also replayed the
installed consumer without LD_LIBRARY_PATH and checked local library resolution.

Coordinator commands after integrating the real experiment:

| Command | Actual result |
| --- | --- |
| `cmake --build build/spike-001-real` | Exit 0; standalone experiment configured/built |
| `ctest --test-dir build/spike-001-real --output-on-failure -V` | Exit 0; 2/2 (synthetic + six real HTTP/WebSocket cases) |
| `cmake --preset scaffold-debug` | Exit 0 |
| `cmake --build --preset scaffold-debug` | Exit 0 |
| `ctest --preset scaffold-debug` | Exit 0; 3/3 infrastructure checks |
| `python3 tools/check_project.py` | Exit 0; 375 records, 122 decisions, links/presets/current HTML |
| `python3 -m unittest discover -s tests/tooling -v` | Exit 0; 18/18 |
| `python3 tools/progress.py render --check` | Exit 0; generated snapshot current |
| `git diff --check` | Exit 0 |

The root product CMake/presets, core implementation and 19-case reviewed numeric
acceptance source have no diff against current main `935e0d6`. The numeric suite
was not rerun here; its previously recorded intentional red remains authoritative.
No service credentials, live provider/network endpoints or supporter state were used.
Only ephemeral 127.0.0.1 sockets and disposable databases were exercised. Ordinary
project configure never downloads or launches this standalone opt-in experiment.

## Durable handoff and remaining work

The bounded SPIKE-001 feasibility slice is complete; the framework is an executable
candidate with known limits, not a production-support declaration. The observed
persistent-client Close issue and additional production lifecycle cases are recorded
in the APITEST-001/API-001 and APITEST-003/API-003 briefs and progress notes. Those
planned reviewed/red baselines must resolve the issue with an appropriate reviewed
version/patch/adapter before product WebSocket support is claimed; per-command close
must not become management/viewer behavior. Production source must not silently copy
this disposable spike.

Other remaining gates: uncooperative/idle clients, partial frames/requests, startup-
time signals, backpressure/limits, crash recovery, Caddy/TLS/remote security,
Ubuntu/Windows and packaging, performance, all live integrations and rate providers.
They remain release scope; no unperformed operation is labeled verified.

Changed paths: tools/setup_drogon.py; experiments/spike-001 standalone CMake, bridge,
contract, external tests and README; dependency/test/code review and red/final evidence;
setup/architecture and task handoffs; centrally maintained progress JSON/HTML. Pin and
setup choices are scoped to this experiment. Local paths stay out of shared presets.
[PR #4](https://github.com/Kernol12/subathon-bot-ai/pull/4) carries publication and
latest-revision CI/merge status; merge still requires the final applicable gates.
