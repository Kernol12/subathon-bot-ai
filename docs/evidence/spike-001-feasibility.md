# SPIKE-001 — initial Qt queue/writer feasibility and real transport gap

This is the **historical partial result** at initial commit `c7e0f3e`. Its missing-
Drogon and unperformed-real-framework statements describe that initial experiment.
The subsequent pinned local build, reviewed real HTTP/WebSocket tests, actual results
and remaining limitations are recorded in [current validation](spike-001-bridge-validation.md).
The original evidence is retained below without relabeling its commands as real-network tests.

Date: 2026-10-30. Investigator: `/root/feasibility` (separate subagent).
Branch: `task/SPIKE-001-qt-drogon`, based on current `origin/main`
`9982001b732f06793f6c17d5f912b82c7cd1e3cc`.
Authority: specification v0.45 §2 and §18 concurrent-request/statistics scenarios,
SERVER-01 and DB-01. No time-credit or application contract is changed.

Scope is an isolated feasibility experiment, not production implementation or a
reviewed product acceptance baseline. The coordinator centrally owns
progress/state.json, PROGRESS.html and shared build/dependency files. This agent
owns only this evidence note and experiments/spike-001. The bounded experiment
criteria and reproduction commands are in
[the experiment README](../../experiments/spike-001/README.md).

## Observed result

**Partial.** The synthetic transport → Qt queue → one Qt SQL writer → drain path
passes on this machine. Real Drogon is not discoverable; therefore no claim is
made about combined Drogon/Qt startup, real request handling or safe framework
shutdown. SPIKE-001 is not complete.

Four synthetic C++ producer threads submit 64 distinct integer IDs (0 through 63)
without accessing SQLite. Explicit queued invocations execute against an object
owned by the QCoreApplication thread. The connection is created, checked, used,
closed and removed in that same thread. Each insert is committed in its own
transaction. The file database reports journal_mode=wal and synchronous=2 (FULL).
After producers join, admission closes, one late submission is rejected and a
queued drain barrier verifies 64 rows and sum 2016. Those literal expected values
come from the experiment's input list, not a production algorithm. All producer
threads join before the queue and database are destroyed.

This shows that the narrow Qt/SQL arrangement is executable here. It does not
establish durability across crashes, transaction design for product effects, or
behavior of a second framework's request/response lifetimes.

## Environment and exact checks

Arch Linux, Linux 7.2.11-arch1-1, x86-64. GCC 16.2.1 (20260810),
CMake 4.4.5, Ninja 1.13.2, Qt Core/SQL 6.12.0. Both pkg-config SQLite and the
actual QSQLITE runtime report 3.54.0. Dependencies were already installed;
no downloads, package installation, network listeners or live services were used
by the executable. Public documentation was browsed separately.

Commands were run from the isolated task worktree root:

| Command | Exit | Observed result |
| --- | --- | --- |
| `pkg-config --modversion Qt6Core Qt6Sql drogon` | 1 | Drogon not found; this is dependency evidence, not red acceptance evidence |
| `pkg-config --modversion Qt6Core Qt6Sql sqlite3` | 0 | 6.12.0, 6.12.0, 3.54.0 |
| `cmake --version` | 0 | 4.4.5 |
| `c++ --version` | 0 | GCC 16.2.1 |
| `ninja --version` | 0 | 1.13.2 |
| `uname -srmo` | 0 | Linux 7.2.11-arch1-1 x86_64 GNU/Linux |
| `cmake -S experiments/spike-001 -B build/spike-001-dependency -G Ninja -DSPIKE_REQUIRE_DROGON=ON` | 1 | Missing Drogon.cps/drogon.cps/DrogonConfig.cmake/drogon-config.cmake; configuration incomplete |
| `cmake -S experiments/spike-001 -B build/spike-001 -G Ninja` | 0 | Qt6 Core/SQL and Threads found; synthetic subset configured |
| `clang-format -i experiments/spike-001/queue_probe.cpp` | 0 | Existing repository format applied |
| `cmake --build build/spike-001` | 0 | Synthetic executable compiled and linked without warning output |
| `ctest --test-dir build/spike-001 --output-on-failure -V` | 0 | 1/1 passed; final output below |
| `git diff --check` | 0 | No whitespace errors |
| `python3 tools/check_project.py` | 0 | 375 tasks, 122 decisions, links/presets/current HTML pass; six draft acceptance cases not executed by this command |
| `python3 tools/progress.py render --check` | 0 | Existing snapshot current; not edited by this agent |

Final experiment output:

```text
Qt 6.12.0 SQLite runtime 3.54.0
synthetic producers=4 submitted=64 applied=64 rejected=1 single-owner WAL/FULL drain=PASS
1/1 Test #1: spike_001.synthetic_queue_writer_drain ... Passed
100% tests passed out of 1
```

The experiment was compiled/run again after formatting and switching its two
summary messages to standard output (the environment did not show Qt info
messages). This does not add product test coverage.

## Documentation findings and proposed next experiment

Qt documents explicit queued invocation in the context object's event loop and
thread-safe functor invocation. This is the mechanism actually used by the probe.
See [QMetaObject](https://doc.qt.io/qt-6/qmetaobject.html).
Qt requires use of a SQL connection in its owning thread; the probe checks both
the context and connection thread before writing. See
[QSqlDatabase](https://doc.qt.io/qt-6/qsqldatabase.html) and
[Threads and the SQL module](https://doc.qt.io/qt-6/threads-modules.html#threads-and-the-sql-module).
The numeric FULL value and journal-mode query follow
[SQLite PRAGMA documentation](https://sqlite.org/pragma.html).

The upstream Drogon header describes run() as blocking its calling thread and
quit() as callable across threads after run has started. This permits a candidate
design with Qt's main event loop and a separate Drogon run thread; it is an
inference from documentation, not a tested integration or selected release pin.
The inspected source is a moving upstream branch, so version-specific behavior
must be checked again against a justified dependency pin. See
[Drogon's public framework interface](https://github.com/drogonframework/drogon/blob/master/lib/inc/drogon/HttpAppFramework.h)
and [its threading-model documentation](https://github.com/drogonframework/drogon/wiki/ENG-FAQ-1-Understanding-drogon-threading-model).

The next actual-framework experiment should:

1. Select/record a supported Drogon/Trantor version and establish a discoverable
   installation without adding an unreviewed production dependency pin.
2. Start Qt and Drogon in one process, record real request thread identity and
   execute queued commands on the authoritative Qt owner only.
3. Establish startup readiness/failure behavior and investigate response callback
   lifetime/thread semantics for that exact version, including disconnect while
   a command is queued.
4. Close command admission under synchronization, define rejection handling,
   drain admitted work and pending responses while the required event loops
   remain alive, then quit/join Drogon and close the writer on its Qt owner.
5. Exercise real HTTP/WebSocket concurrency, shutdown with in-flight requests,
   startup failure and termination signals. Document which admitted operations
   commit and how each response reaches a defined outcome.

Backpressure, reader isolation, backup/restore, recovery, performance, Windows,
Wayland/OBS and every live provider remain unverified. All seven integrations,
three rate providers and the selected two-framework architecture remain in scope.
Missing local Drogon is not a reason to substitute another product transport.

## Durable handoff

Changed paths: this evidence note and experiments/spike-001/{README.md,
CMakeLists.txt,queue_probe.cpp}. No production module, root CMake, dependency pin,
existing acceptance assertion or public schema was changed.

No independent review has yet occurred. The coordinator will record task state
and regenerate progress centrally, arrange review, publish the task branch/PR
and preserve the unresolved real-framework gate. Do not reuse this disposable
code as production without separate review and the appropriate test-first gates.
