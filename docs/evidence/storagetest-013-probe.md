# STORAGETEST-013 root schema proposal and local diagnostic

Coordinator/integrator: /root, 2026-11-01. Source-author actual contract candidate
0c154322d86789a1dbdf7e91927933b715ef6151 integrated as1b9cf6c. Root owns the
[physical proposal](../contracts/sqlite-receipts-schema-v1.md), actual migration2
ordering/projection compatibility and shared integration. Different contract
review is still pending; no future API/stub/feature fixture/test/implementation
or intended red is claimed.

The proposed prefix appends identified-receipts-v1 at2 to immutable retention1,
supportedstarts0/1/2. Four exact SQL statements produce3newtables/3newautoindexes
and allocator `(1,NULL)`,13totalobjects. Full independent F-framed step2 payload
is10535bytes, SHA25624614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2.
No production factory/codec generated these expected values. Prefix1's existing
literal DDL/projection/2000bytes/0906f121 digest remain unchanged.

Actual disposable diagnostic commands (all exit0):

```sh
python3 /tmp/subathon-receipt-schema-proposal.py
c++ -std=c++20 -fPIC -Iserver/include local-evidence/coordinator/storagetest013-schema-probe.cpp build/sqlite-retained-debug/server/libsubathon_sqlite_writer.a $(pkg-config --cflags --libs Qt6Core Qt6Sql) -o local-evidence/coordinator/storagetest013-schema-probe
local-evidence/coordinator/storagetest013-schema-probe > local-evidence/coordinator/storagetest013-schema-probe.txt
```

Independent Python stdlib SQLite3.54.0 created metadata, unchanged retention1 DDL
and proposed step2 SQL, inspected exact13stored schema objects and computed
length-framing/hash with hashlib, without importing any production factory.
Diagnostic JSON/source/executable/log remain ignored under local-evidence.
The separate consumer compiled against the already integrated private-writer
library, used QCoreApplication/QTemporaryDir, one existing context.begin and real
session.commit, executed the literal proposal SQL, compared every projection
field/SQL-null marker/SQL string and allocator singleton/type/NULL, then observed
committed normal completion. Output:

```text
Qt 6.12.0 SQLite 3.54.0; exact13objects/null-autoindexes/unchanged-retention-DDL/allocator1NULL PASS
```

Local Linux x86-64/GCC16.2.1/system Qt6.12.0 SQLite3.54.0. This proves this narrow
host SQL projection assumption. It is not official Ubuntu/GCC13.3/selected bundled
SQLite3.53.4 proof, production receipt behavior, a new acceptance fixture or red.
Actual independent contract/API/test review, compiled literal future fixtures,
selected official Qt environment proof and first intended red remain future gates.
No dependency, prior source/test assertion, shared CMake/preset/workflow, current
catalog or deployed database was changed; no live/provider side effect occurred.
