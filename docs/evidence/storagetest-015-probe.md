# STORAGETEST-015 root physical proposal diagnostics

Root/integrator,2026-11-02. Logical authora44aa00 integratedaf2b724, original
logical6e37b3720788db15c32af674adad67aee58292c8. Root only adds physical companion
link; no core/API/source/test/fixture change. [Physical proposal](../contracts/sqlite-application-plans-schema-v1.md).
This is contract-stage SQL representation work, NOT acceptance fixtures, new plan
codec/store implementation, feature body, red or permission/provider proof.

Exact migration4 captured-application-plans-v1 appends five CREATE statements,
five empty tables/five generated SQLNULL indexes, total30 objects. Header7 columns,
revision34, rule7, modifier10, request14; exact prefix20/13/7 projections unchanged.
Independent F framing/SHA256:29015 bytes,
`bbb8140119b29f54f419479851d4fdf42b6a928f5451e4de756389d2bc359c50`.
No future product factory/codec/runner supplied expected values.

Actual commands/checks:

```sh
python3 /tmp/subathon-plan-schema-proposal.py
clang-format -i local-evidence/coordinator/storagetest015-schema-probe.cpp
```

Python independently constructs exact approved-logical field literals/schema,
uses immutable prefix fixture SQL, compares30 actual SQLite schema objects/exact
CREATE/NULL index SQL and old20/13/7 tuples, then implements decimal-length F
framing and hashlib.sha256. Actual Python stdlib SQLite3.54.0: PASS. Scratch
script and ignored JSON local-evidence/coordinator/storagetest015-schema-proposal.json
retain literal SQL/projection/fullpayloadhex; these are proposal artifacts, not a
new baseline fixture. Later fixture author must independently derive expectations.

A separate actual C++ consumer links ONLY old SqliteWriter+QtCore/Sql, not any
plan API/codec/factory/service. Python argv invoked:

```sh
c++ -std=c++20 -fPIC -Wall -Wextra -Wpedantic -Werror -Iserver/include local-evidence/coordinator/storagetest015-schema-probe.cpp build/sqlite-contribution-debug/server/libsubathon_sqlite_writer.a $(pkg-config --cflags --libs Qt6Core Qt6Sql) -o local-evidence/coordinator/storagetest015-schema-probe
local-evidence/coordinator/storagetest015-schema-probe local-evidence/coordinator/storagetest015-schema-proposal.json
```

Final compile/diagnostic exit0, zero warnings. Initial scratch formatting caused
one misleading-indentation warning; formatting was corrected and actual -Werror
rebuild/diagnostic passed. This was not feature red. Actual output:

```text
Qt 6.12.0 SQLite 3.54.0; exact30/NULLindexes/oldprefixes/rawMAX+parent/presentCID0/ExactBLOB+signedMIN PASS; sparse history deliberately INVALID (count3 vs headMAX)
```

It performs real existing context BEGIN/COMMIT, applies literal old metadata/prefix
SQL and five new statements, compares30 exact schema tuples/NULL markers. Synthetic
raw rows show present ContributionId0 remains BLOB8 versus absent parentEvent SQLNULL;
actual selected MAX/parentMAX-1 fields are BLOB8, first parentNULL; rational components
remain exact ASCII BLOB and requested units signedMIN INTEGER. Foreign-key check
returns empty. New revisions1,MAX-1,MAX deliberately have count3/headMAX; intermediary
MAX-1 names parent1. SQL membership/less-than constraints hold, but this is INVALID
complete core history. No full-plan or append validity/exhaustion is claimed.

Only SQL representation/driver capability is proved: no new store or codec exists,
no actual CompleteApplicationPlan is returned, and seeded old source rows do not
establish prior commit/normalization/semantic validity through a product call. No
schema/role checks are promoted into provider authority or operational migration
permission. Host GNU16.2.1/Qt6.12/SQLite3.54 is NOT selected official bundled proof.
Raw ignored output is local-evidence/coordinator/storagetest015-schema-probe.txt.

Different actual logical+physical approval remains required before API/stubs.
Separate actual API/test/fixture/harness review, successful compile/official green
and first genuine missing-behavior red still precede STORAGE-015. All earlier
producer/assertion/review/red/prefix gates remain valid, not repeated. Full application,
semantic authorization/provider/FX/recovery/platform/UI release scope remains open.
