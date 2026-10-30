# SPIKE-001 — independent real bridge code and evidence review

Date: 2026-10-30. Experiment author/integrator: coordinator `/root`.
Test author: `/root/bridge_test_author`. Reviewer: `/root/bridge_reviewer`, a
separate agent/session with no authorship of the implementation or tests.
Authority: specification v0.45 sections 2/17/18, SERVER-01, DB-01, and the
explicitly bounded experiment contract. This follows the separate
[test-baseline review](spike-001-bridge-review.md) and
[dependency review](spike-001-dependency-review.md).

**Verdict: approved for integration of the bounded Linux loopback feasibility
experiment and its evidence.** No blocking code-review request remains. This is
not production-reuse approval, general WebSocket conformance, release support,
or permission to bypass latest-revision CI, review discussions or branch rules.

## Exact baseline and implementation

Implementation commit: `a54fef147dbb0df9e582e15a6d7705ec5f5edd11`;
review-worktree equivalent prerequisite: `c91e533`.

| Path | Reviewed Git blob |
| --- | --- |
| experiments/spike-001/bridge_probe.cpp | `4fbfd28247fc37b0065c859b60e4764337bc7edd` |
| experiments/spike-001/bridge_contract.md (clarified revision 1.1) | `96b0eb64123f6f53801f06321fa3a28541a2088c` |
| experiments/spike-001/test_bridge.py | `4d4c89388254f25a90b2b3b481b62afc7a6d76c5` |
| experiments/spike-001/CMakeLists.txt | `7b77d640142ed869e8705463d029327601be45f4` |

The test source is byte-identical to author `2888b9f` and the approved original
baseline. No oracle, assertion or fixture was changed to make the experiment pass.
The production time-credit stub, its 19 reviewed acceptance cases, progress
validator and root/tests CMake files have no diff from main `935e0d6`.
The real experiment remains explicit opt-in under its own standalone CMake file.

## Thread, state and lifetime assessment

- The QCoreApplication thread constructs the Bridge QObject and one named Qt SQL
  connection. Admission, held commands, transactions, stop state and connection/
  reply accounting execute on that owner. Network threads post immutable command
  values and response ownership instead of writing SQL or authoritative state.
- Admission is observed only after the command is stored on the Qt owner; stdout
  barriers therefore cannot race a stop/SIGTERM ahead of already-admitted work.
  Close/release observations follow their corresponding state change.
- Qt QObject thread identity, actual QSqlDatabase thread ownership and network
  origin identity are runtime comparisons. WAL and synchronous=FULL are queried
  back from the actual connection. SQL commit precedes a successful reply.
- Each reply captures its originating Drogon I/O loop, posts delivery back there,
  checks the executing loop and only then posts completion accounting to Qt.
  HTTP response callbacks and weak WebSocket connections outlive queued work as
  needed; a disconnected admitted request still commits independently of delivery.
- Stop closes admission and drains stored work. After reply callbacks complete,
  connections are shut down to flush their buffers and disconnection observations
  are awaited before Drogon quit, whose pinned implementation force-closes network
  resources. The Qt loop remains alive until framework run returns; the framework
  thread joins before the writer closes/is removed on its Qt owner.
- Signal handling uses volatile sig_atomic_t descriptor storage, saved/restored
  errno and only nonblocking self-pipe write in the handler. Qt consumes that pipe
  and invokes the normal stop path. Handler restoration precedes descriptor closure.
- The source-derived readiness ordering is correct for the configured one-I/O-thread
  probe: a second framework-main-loop post follows startListening dispatch, then an
  I/O-loop barrier follows the actual listen. The advertised port comes from the
  bound listener. Occupied-port failure genuinely exercises framework bind failure;
  its fatal process exit is not represented as graceful internal teardown.

These conclusions cover the inspected code and six bounded cases, not every
possible interleaving, hostile client, malformed input or application transaction.
In particular, idle/uncooperative clients can prevent the cooperative drain; the
external failure deadline detects a hang but is not a production shutdown policy.

## WebSocket clarification and preserved failures

The captured initial real implementation output preserves three WebSocket close-
frame failures. Pinned-source review found received-Close processing half-closes
TCP before invoking the controller, while Trantor declines sends after transition
to Disconnecting. The reviewer identified that raw TCP shutdown alone cannot
supply the required close frame and independently agreed the bounded clarification.

The final probe sends ACK/error then initiates WebSocketConnection shutdown on
the same originating I/O loop, before TCP half-close. The unchanged client receives
its reply and matching protocol close frame. Revision 1.1 explicitly states one
command per WebSocket connection/server-initiated close. Revision 1 never promised
persistent multiple-command sessions; no reviewed test is weakened and no vendor
patch or production protocol fix is claimed. Persistent/client-initiated close
semantics remain a separate investigation before production reuse.

The reviewer also inspected captured intended red at `db5fedf`: six cases ran,
five failed for actual missing-operation responses/persistence, and occupied-port
startup passed. Four failures show HTTP 501 FeatureNotImplemented where reviewed
200/503 replies are required; one reads SQL `[]` instead of `[41]`. They are neither
build/dependency failures nor absent-event timeouts. The reviewer did not replay
that historical stub; source/log chronology records red before implementation.

## Independent reviewer execution

The reviewer brought the exact authored prerequisites and implementation into
`/tmp/subathon-bridge-review`, configured a fresh build and ran it independently.
Existing shared local dependency prefix was reused; no dependency download or
system package installation occurred. GNU C++ 16.2.1, Qt 6.12.0, SQLite runtime
3.54.0, Drogon 1.9.14 and Trantor 1.5.29 were used on Linux x86-64.

| Command/check | Exit / observation |
| --- | --- |
| `cmake -S experiments/spike-001 -B build/spike-001-review -G Ninja -DSPIKE_BUILD_REAL_BRIDGE=ON -DCMAKE_PREFIX_PATH=<primary-checkout>/build/deps/drogon-1.9.14` | 0; exact installed Drogon and Qt found |
| `cmake --build build/spike-001-review` | 0; fresh synthetic and real probes compiled/linked without warning output |
| `ctest --test-dir build/spike-001-review --output-on-failure -V` | 0; 2/2 passed: retained synthetic probe plus all six actual HTTP/WebSocket cases |
| Six-case suite within that CTest run | All six passed in 0.054s; total CTest 0.11s |
| Baseline/source Git blob comparison | Exact blobs above; approved test source unchanged |
| `git diff 935e0d6` for core stub/numeric tests/root build/validator paths | Empty |
| One-off historical output checks | Intended five failures/one startup pass and initial three close-frame failures confirmed |
| `git diff --check` | 0 |

Loopback CTest required the permitted network sandbox override. Only local
127.0.0.1 sockets, disposable SQLite files and synthetic IDs were used; no external
provider, credentials, chat or contribution processing occurred.

The coordinator remains responsible for final central progress/HTML consistency,
current evidence, latest PR revision CI and merge conditions. Production contracts,
TLS/Caddy, remote security, crash durability/recovery, backpressure, statistics-reader
isolation, persistent WebSockets, performance, Ubuntu deployment, other release
platforms and all live providers remain outside this completed bounded experiment.
