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
No production factory/codec supplies these expected values. Prefix1: 2000 bytes/SHA0906f121
and prefix2: 10535 bytes/SHA24614f68 remain immutable. Deterministic unresolved order uses
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

At this proposal stage, different actual logical+physical review was required
before minimal API/stubs; actual f76817a subsequently supplied that approval.
Full test/fixture/harness review, successful compilation, selected official green
setup and genuine intended red still precede STORAGE-014 implementation. All old
passing118 artifacts/reviews/reds/immutableprefixes and full release scope remain.

## Actual minimal API build, before test authorization

Actual author candidate94681d321ebecd2527f0dd18506c23a3674fa9c1 integrated727f59d.
New header blob0abeea2bb6d1e99faad7d6585400ad3d80e2eae8; explicit four-throw
stub0ad1315601218601289923fe7b02eebfb8573aaa; logical API precision
1a96354b34cebfaac22f091b4b3f620c044e0e3b. Root added minimal CMake option/target
and sqlite-contribution-debug preset inheriting the existing receipt profile.
The service/core contribution targets are enabled without adding GoogleTest or
changing their source. No new Qt feature test or fixture is registered/authored.

Actually ran on the local GNU16.2.1/Qt6.12.0/SQLite3.54.0 environment:

```sh
cmake --preset sqlite-contribution-debug
cmake --build --preset sqlite-contribution-debug
ctest --preset sqlite-contribution-debug --output-on-failure --parallel 1
clang-format --dry-run --Werror server/include/subathon/server/sqlite_contributions.hpp server/src/sqlite_contributions.cpp
```

Configure/build pass,62 Ninja steps and no warnings. All118 inherited tests pass,
0 failures,2.56s. This executes existing passing behavior only: new contribution
stubs have not been invoked, and no new intended red is observed. Formatting/diff
pass. Raw existing-suite output is ignored local-evidence/coordinator/
storagetest014-api-regression.txt. Separate actual API/stub/shared-build review
remains required before authorizing new tests/fixtures. Selected official Qt
contribution fixture proof is still absent and must precede first feature red.
