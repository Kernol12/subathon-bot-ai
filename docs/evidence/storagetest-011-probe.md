# STORAGETEST-011 — existing SQLite diagnostic capability probe

Coordinator `/root`, 2026-11-01, task branch after merged main88e86c1. Read-only
investigation before migration contract/test authoring. No repository C++/schema/
assertion or production behavior changed. Disposable files/memory databases only.

The [official SQLite PRAGMA-functions reference](https://www.sqlite.org/pragma.html#pragma_functions)
describes read-only result-bearing PRAGMAs available through SELECT table-valued
functions. The writer already permits trusted SELECT, so a new low-level writer
API is not necessarily needed for migration diagnostics. Runtime support and strict
result shapes must still be verified, not inferred from the documentation.

Actual local Qt6.12 QSQLITE driver reports SQLite3.54.0. Direct Qt SQL probe:

| SQL | Observed result |
| --- | --- |
| SELECT sqlite_version() | 3.54.0 |
| SELECT * FROM pragma_integrity_check | one row: ok |
| SELECT * FROM pragma_integrity_check(1) | error: no such table: 1 |
| SELECT * FROM pragma_foreign_key_check | zero rows |
| PRAGMA integrity_check(1) | one row: ok |
| PRAGMA foreign_key_check | zero rows |

The numeric direct-PRAGMA argument cannot be assumed equivalent to the TVF call.
The initial writer probe using TVF(1) ended with work failure/exit3; Qt info logging
was suppressed, so direct driver diagnostics identified the SQL argument error.
This was not a migration acceptance/red run. No missing dependency or failed probe
is counted as the new baseline.

A corrected independent consumer linked the EXISTING source023451 writer library
and used one normal queued work/session: SELECT sqlite_version(), SELECT * FROM
pragma_integrity_check, SELECT * FROM pragma_foreign_key_check, then real commit.
Actual stdout: version3.54.0; integrity rows1/first ok; FK rows0;
TransactionOutcome1(committed)/WorkError0(none), process exit0. This confirms the
existing private-session SELECT capability on the current host without changing
its allowlist/header/source or any of the49 reviewed assertions. It does not
implement a version catalog, migrations, identity checking or release readiness.

The two C++ probe sources are preserved in ignored
local-evidence/coordinator/storagetest011-probes; they originally compiled from
/tmp/subathon-migration-{pragma,writer}-probe.cpp. Actual compile/run orchestration
used Python subprocess, without shell interpolation:

```python
flags = shlex.split(subprocess.check_output(
    ['pkg-config', '--cflags', '--libs', 'Qt6Core', 'Qt6Sql'], text=True))
subprocess.run(['g++', '-std=c++20', '-Iserver/include',
    '/tmp/subathon-migration-writer-probe.cpp',
    'build/sqlite-work-debug/server/libsubathon_sqlite_writer.a',
    *flags, '-o', '/tmp/subathon-migration-writer-probe'], check=True)
subprocess.run(['/tmp/subathon-migration-writer-probe'], check=True)
```

The direct diagnostic probe compiled similarly without the existing writer archive
or header include path. Both final probes exited0. No migration feature tests ran.

Author independently confirmed the no-argument forms with Python SQLite and also
observed that an FK-violating fixture can still report physical integrity ok. That
is technical evidence, not selected Qt CI or catalog acceptance. The contract must
check both diagnostics, fail closed on unsupported/error/truncated/unexpected
results, and avoid treating a version row or physical integrity alone as a verified
schema. A focused green selected-Qt diagnostic environment probe must execute in
actual CI before genuine new stub-red can be accepted. Existing49 acceptance and
all future migration test/review/red/implementation/platform gates remain separate.
