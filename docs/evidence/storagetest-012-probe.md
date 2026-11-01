# STORAGETEST-012 — existing writer binary-binding probe

Coordinator `/root`, 2026-11-01, task branch
`task/STORAGETEST-012-retained-context` from main29f3818/claim036fad8.
This focused diagnostic exercises an existing capability before the new retention
contract/test fixture is frozen. It is neither new retention implementation nor
feature acceptance/red. No tracked source, header, schema, fixture or test changed.

## Actual setup and observation

Host Linux x86-64, GNU16.2.1, Qt6.12.0/QSQLITE with SQLite3.54.0. The actual
existing writer source remains023451987e40a9330fef91c6c0955f0e527f14e6. An
isolated C++ QCoreApplication consumer links the built writer static library,
opens a disposable QTemporaryDir database and enqueues one ordinary work item.
Inside its real session it executes, for each byte array:

```sql
SELECT typeof(?),length(?),hex(?)
```

All three parameters receive the same QByteArray value. It commits synchronously;
completion checks no work error/confirmed commit, then exits the harness event loop.
No nested event processing occurs inside the writer callback.

| Input | Actual SQLite type | Actual length | Actual hex |
| --- | --- | --- | --- |
| `QByteArray("",0)` (nonnull empty) | blob | 0 | empty |
| `QByteArray{}` (null buffer) | null | SQL NULL | empty |
| `QByteArray::fromHex("00ffefbbbf61")` | blob | 6 | 00FFEFBBBF61 |

Actual compile/run exit0. This establishes the local empty-BLOB/null distinction
and exact binary bytes, including NUL/FF/UTF8 BOM bytes, through the unchanged
writer boundary. It does not decide what a new optional field means; the reviewed
contract must specify that separately and verify stored typeof, not guess from
a zero-length or Qt-null buffer.

## Commands, artifacts and limits

The scratch consumer/source and raw stdout are ignored local artifacts:
local-evidence/coordinator/storagetest012-binding-probe.cpp and .txt.
Corrected source SHA-256: `d9aa342e581cd60be6b5a97d3e5ca13cf46ea87c861839ab9f60017e96101380`.
Python subprocess composed the compiler argument vector (no shell interpolation):
`c++ -std=c++20 -fPIC -Iserver/include <probe.cpp>
 build/sqlite-migrations-debug/server/libsubathon_sqlite_writer.a
 <pkg-config --cflags --libs Qt6Core Qt6Sql> -o <probe>`, then ran the consumer.

The first scratch compilation used a nonexistent two-argument execute overload.
Correcting it to `session.execute(SqlStatement{sql, bindings})` compiled and ran
successfully. That scratch setup error is not product red or a failed dependency
gate. No assertion or production API was adjusted.

Only the host driver was exercised here. Selected official Qt bundledSQLite3.53.4,
new module schema/adapter behavior, later typed uint64 persistence, actual browser,
platform/package/backup/recovery/provider/live behavior are not established by this
probe. New contract/examples still need actual separate review, then authored/
reviewed tests and a compiling intended feature red before STORAGE-012 production.
