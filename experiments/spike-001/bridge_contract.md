# SPIKE-001 real-framework experiment contract — revision 1

Author: `/root/bridge_test_author`. Specification v0.45 §2/18, SERVER-01 and DB-01.
Coordinator approved this bounded interface before test authoring. This is a disposable
loopback experiment, not the product API, accounting contract or production server.
Independent test review and intended missing-behavior execution remain required.

## Process and observations

The probe is launched as `PROBE --port 0 --database PATH [--hold]`. It binds only
127.0.0.1; zero selects an actual ephemeral listener port. The caller owns a separate
empty temporary directory/database. Stdout emits one JSON object per line; framework
log lines may also appear and are diagnostic only. Stdin accepts `release`, `close`
and `stop`, each terminated by a newline. Socket/process timeouts are failure bounds,
not ordering mechanisms. Tests synchronize on explicit events and network replies.

After successful framework/listener startup and database initialization, emit:

```json
{"event":"ready","port":12345,"qt_version":"6.12.0","drogon_version":"1.9.14","journal_mode":"wal","synchronous":2}
```

The actual runtime versions are strings; the displayed values are examples. The
reported port must come from the bound listener, not an earlier free-port guess.
Readiness confirms the Qt SQL connection is open on its owner, WAL was obtained and
synchronous=FULL was set/read back on that same connection. SQLite stores commands
in `commands(id INTEGER PRIMARY KEY)`. Probe IDs are small integers used solely to
check ownership and transport; they do not define production numeric JSON encoding.

Each admitted command emits `{"event":"admitted","id":N,"transport":"http"}`
or transport `websocket`. The event is a barrier after admission, before any
hold is released. `--hold` keeps admitted operations pending without blocking the
Qt event loop. `release` applies pending commands and permits ordinary progression,
then emits `{"event":"released"}`. Each command commits exactly one row on the
single Qt SQL writer; network threads never access that connection/state directly.

## Transport replies and admission

HTTP `POST /command`, content type application/json, body `{"id":N}`, replies
status 200 only after commit. Its JSON body is:

```json
{"id":7,"qt_owner":true,"sql_owner":true,"request_off_owner":true}
```

Use `Connection: close` for probe HTTP responses so bounded shutdown can wait for
response flushing/connection closure. A real WebSocket upgrade on `/ws` accepts
text `{"id":N}` and returns the same committed-acknowledgement JSON as a text frame.
Clients perform a close handshake after the reply. Ownership flags are observable
instrumentation; independent source review must verify they check actual runtime
thread identity rather than constants. They alone do not prove thread ownership.
Tests also independently inspect the actual SQLite rows after process exit.

`close` closes admission while leaving the listener and held operations alive,
then emits `{"event":"admission_closed"}`. Later HTTP commands return status 503
and `{"error":"admission_closed"}`; WebSocket commands return a text frame with
that error. Neither creates a row. Previously admitted work remains eligible to
commit. These choices describe this experiment's shutdown test, not product retry,
idempotency, transaction or service-availability policy.

## Shutdown and startup failure

`stop` and SIGTERM use the same bounded shutdown path: close admission, emit
`{"event":"stopping"}`, drain admitted work, complete transport replies where
clients remain connected, quit/join Drogon, and close/remove the database on its
Qt owner. A client disconnected after admission must not cancel or duplicate its
already-admitted database command or cause callback use-after-free. Connected
WebSocket clients close after receiving their acknowledgements. Successful exit
is zero and ends with:

```json
{"event":"stopped","writer_closed":true,"drogon_joined":true,"owner_violations":0}
```

Shutdown must actually complete within the harness failure deadline; self-reported
flags cannot substitute for process exit or SQL inspection. A held HTTP/WS operation
must receive its committed reply while stop drains it. On startup with an already
bound listening port, exit nonzero within the deadline and never emit `ready`.
A `startup_failed` diagnostic event is allowed but is not required: the pinned
framework may terminate before user startup advice. No preflight port availability
check can replace checking the real framework bind failure.

## Six independent experiment oracles

| Case | Actual input | Required independent result |
| --- | --- | --- |
| Mixed transport concurrency | HTTP IDs 0–3, WebSocket IDs 4–7 concurrently | Eight committed acknowledgements, direct SQL IDs `[0,1,2,3,4,5,6,7]`, count 8/sum 28 |
| Disconnect while queued | Held HTTP ID 41; disconnect after its admitted event, then release | Successful shutdown, direct SQL IDs `[41]`, count 1/sum 41 |
| Close admission while queued | Held HTTP ID 7; close admission; later HTTP 8 and WS 9 | Late rejection replies, admitted 7 commits, direct SQL IDs `[7]`, count 1/sum 7 |
| Stop with queued work | Held HTTP 11 and WS 12; stop after both admitted barriers | Both committed replies and clean process exit, SQL IDs `[11,12]`, count 2/sum 23 |
| Occupied port | Existing loopback listener owns requested port | Nonzero bounded startup exit; no ready event |
| SIGTERM with queued work | Held HTTP 21; SIGTERM after admission | Committed reply and clean process exit, SQL IDs `[21]`, count 1/sum 21 |

The six cases are ordinary local-network experiments with synthetic IDs. They do not
verify Caddy/TLS, remote/public network security, live providers, production API/schema,
crash durability/recovery, retries, backpressure, statistics reader isolation, performance,
Windows, Wayland/OBS or every possible shutdown/client behavior. Those release gates
remain explicit; passing these cases is not permission to reuse the spike as production.
