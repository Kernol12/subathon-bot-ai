# SPIKE-001 real bridge — authored experiment baseline

Author: `/root/bridge_test_author`, 2026-10-30.
Branch/worktree: `task/SPIKE-001-bridge-tests`, `/tmp/subathon-bridge-tests`, from
current main `935e0d6` plus the original synthetic feasibility experiment `837821b`.
Coordinator centrally claims SPIKE-001 and owns shared progress/HTML, CMake and
integration. This author owns only the experiment contract, external test source
and this note. No production code or existing acceptance assertion is changed.

Specification v0.45 §2/18, SERVER-01 and DB-01 require the early same-process
Drogon → authoritative Qt queue → controlled Qt SQL writer lifecycle experiment.
The coordinator approved the bounded interface before tests were written; the
separate reviewer gave initial feedback on startup readiness and bind failure.
[Contract revision 1](../../experiments/spike-001/bridge_contract.md) fixes the
probe interface and explicitly separates these experiments from product behavior.

## Cases and independent expected results

The six cases in [test_bridge.py](../../experiments/spike-001/test_bridge.py) use
only Python's standard library, ephemeral loopback sockets, synthetic bounded IDs
and a fresh disposable database for each child process. They inspect real SQL rows
through read-only connections, including proving held work has not yet committed.
No expected result comes from the implementation under test.

| Case | Independent oracle and requirement |
| --- | --- |
| Concurrent HTTP/WS | IDs 0–7, eight rows and sum 28; both transports acknowledge their own IDs after commit; actual thread instrumentation must pass (SERVER-01, DB-01) |
| Queued HTTP disconnect | Held ID 41 is absent before disconnect/release and present exactly once after stop; one row/sum 41 (callback lifetime and authoritative admission) |
| Admission closes | Held 7 survives; late HTTP 8 returns 503/error, WS 9 returns error; only `[7]`, count 1/sum 7 persists (shutdown admission versus drain) |
| Stop drains work | Held HTTP 11 + WS 12 return committed replies during stop; SQL `[11,12]`, count 2/sum 23; real process exits zero (SERVER-01 shutdown) |
| Occupied listener | Another listening socket retains the requested port; child exits nonzero and emits no ready event, preventing false readiness (startup lifecycle) |
| SIGTERM | Signal after held HTTP 21 admission drains its reply, closes writer and joins framework; SQL `[21]`, count 1/sum 21, zero exit (shutdown lifecycle) |

The harness uses JSON events/network responses as ordering barriers; monotonic
clock/socket/process timeouts bound failure rather than implement sequencing.
A hand-written bounded RFC6455 client avoids a new test dependency: validates
upgrade accept, masks client frames, reads bounded server text/close frames, and
handles pings while waiting. HTTP uses connection-close responses. The Linux
runner disables core dumps because occupied-port startup may exit fatally upstream.

Thread-owner and SQL-policy fields are checked, but require independent source
review to ensure real thread/connection checks. Readiness must follow actual bind,
not merely BeginningAdvice or a prior free-port check. An optional startup_failed
JSON event is not required because the pinned framework's fatal bind path may
terminate before application advice; mandatory results are bounded nonzero/no-ready.

## Checks actually run by author

- `python3 -m py_compile experiments/spike-001/test_bridge.py`: exit 0.
- `python3 experiments/spike-001/test_bridge.py --help`: exit 0, documents required probe path.
- A one-off AST enumeration finds exactly six `test_` methods.
- `git diff --check`: exit 0.

No external behavior test, red run, production implementation or independent
approval is claimed by authoring. Next: separate reviewer checks the exact baseline;
coordinator compiles a listening explicit FeatureNotImplemented stub and records
intended response/persistence failures, then implements only the disposable real
framework spike after approval/red. Build, missing binary, dependency, timeout or
harness failures must not be counted as intended behavioral red.

TLS/Caddy, remote security, public product API/schema, crash recovery, backpressure,
statistics-reader isolation, performance, providers, Windows and Wayland/OBS remain
unverified. This baseline does not complete those release gates or approve reuse
of experiment code in production. Coordinator publishes/integrates into existing
PR #4 and updates shared state centrally.
