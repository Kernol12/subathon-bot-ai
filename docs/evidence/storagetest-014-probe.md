# STORAGETEST-014 root physical proposal diagnostics

Coordinator/integrator /root, 2026-11-01. Actual logical candidate9897778 integrated
432271f; root owns [migration3 proposal](../contracts/sqlite-contributions-schema-v1.md).
This is contract-stage disposable SQL/environment work, not source implementation,
new acceptance fixtures, a feature test, intended red or provider/permission proof.

Three new tables/four SQL statements append contribution-evidence-v1 at3 to exact
retention1/receipt2. Full projection20 includes all previous13 tuples unchanged;
new contribution/observation/allocator tables and four NULL-SQL autoindexes.
Independent canonical payload18126bytes/SHA256
b72249ba22c4838af98b9ae29600e70969cf3295eea958a30074167221d2ad1d.
No production factory/codec supplies these expected values. Prefix12000/SHA0906f121
and prefix210535/SHA24614f68 remain immutable. Deterministic unresolved order uses
canonicalReceiptId; no extra ordinal schema/overflow policy is introduced.

Actual commands (all exit0):

```sh
python3 /tmp/subathon-contribution-schema-proposal.py
c++ -std=c++20 -fPIC -Iserver/include local-evidence/coordinator/storagetest014-schema-probe.cpp build/sqlite-receipt-debug/server/libsubathon_sqlite_writer.a $(pkg-config --cflags --libs Qt6Core Qt6Sql) -o local-evidence/coordinator/storagetest014-schema-probe
local-evidence/coordinator/storagetest014-schema-probe > local-evidence/coordinator/storagetest014-schema-probe.txt
```

The independent Python stdlib SQLite3.54.0 diagnostic executed literal metadata,
unchanged prefix1/2 fixture SQL and proposed four statements in memory. It compared
all20 actual sorted schema objects/exact CREATE/NULL markers and framed/hashlib-
hashed that actual proposal without importing production factories. JSON/script
artifacts remain ignored under local-evidence or /tmp; they are not new baseline
fixtures. Expected payload/digest will need independently authored/reviewed literals.

Separate C++ consumer compiled against ONLY the existing private writer and QtCore/
Sql, used QCoreApplication/QTemporaryDir, one real context.begin and session.commit,
executed literal SQL from the ignored proposal JSON and compared20 exact objects/
SQL-null markers/oldprefix tuples/allocator `(1,NULL)`. It then inserted synthetic
receipt rows and default-state contribution0/support plus an unassociated explicit
confirmation, binding zero as QByteArray8 and absence as QVariant{}. Raw typeof/
length/hex prove contribution0 is BLOB0000000000000000, not SQLNULL; unresolved
SQL selects only the NULL association. Exact output:

```text
Qt 6.12.0 SQLite 3.54.0; exact20objects/null-autoindexes/unchanged-prefix1and2/allocator1NULL/presentCID0-vs-NULL PASS
```

The first host projection-only consumer also passed before the zero/absence data
checks were added; neither is feature red. Host Linux x86-64/GNU16.2.1/Qt6.12.0/
SQLite3.54.0 is not selected official bundled Qt/SQLite3.53.4 proof. This diagnostic
calls no contribution adapter/codec/service and does not claim receipt rows were
committed before a contribution call: no such call exists here. Same-session SQL
can show existence/type, never prove prior commit or semantic authorization.
No active database, real contributor, credentials or live operation was touched.

Different actual logical+physical review remains required before minimal API/stubs.
Full test/fixture/harness review, successful compilation, selected official green
setup and genuine intended red still precede STORAGE-014 implementation. All old
passing118 artifacts/reviews/reds/immutableprefixes and full release scope remain.
