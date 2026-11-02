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


## Actual complete baseline build, before full test review

Coordinator `/root`, 2026-11-02. Actual fixture author6c276fc integrated6b4a2a2;
scoped independent fixture/helper approval53c0f621 integrateddab24ab. Actual
feature author2771c9bb4ed1722882b114b7b6d79a4de8fbb9e2 integratedc1cde5b.
Its 29 slots/2624-line source and author evidence are authored, not yet approved
or invoked. Header/stub/logical/physical and independently reviewed JSON/helper
remain unchanged.

| Actual artifact | Blob |
| --- | --- |
| Feature cases | `0f8e625ff7c238776602fd1be102959f8c43b073` |
| Literal JSON | `64eb8b4e0d709bc34ab6d33a4860a62883f06c6b` |
| Old-writer-only green | `a8d63d6a220992fcb71273d89a3aa32609870051` |
| Complete Qt registration | `1ef6dfccf4e6bd4b369aa2886716eeb899a76811` |
| Complete SQL workflow | `4d8c5f8d3a79d40233d7e7851b791535f2729d5b` |

Actual root commands, all successful:

```sh
cmake --preset sqlite-plan-debug
cmake --build --preset sqlite-plan-debug
ctest --preset sqlite-plan-debug --show-only=json-v1
ctest --preset sqlite-plan-debug -LE '^sqlite-plan$' --show-only=json-v1
build/sqlite-plan-debug/tests/qt/server_sqlite_application_plans_acceptance -functions
build/sqlite-plan-debug/tests/qt/server_sqlite_application_plans_environment -functions
ctest --preset sqlite-plan-debug -LE '^sqlite-plan$'
python3 -m unittest discover -s tests/tooling -v
/tmp/subathon-actionlint-1.7.12/actionlint .github/workflows/sqlite-writer.yml
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Actual incremental build **20 steps, zero warnings**, including both new Qt
executables; not an assumed fresh-build count. Linux x86-64/GNU16.2.1/CMake4.4.5/
host Qt6.12/Boost1.92. Exact registration is **173**: original143, new green1 and
feature29. Compiled `-functions` output matches each of29 explicit feature
registrations exactly and all four separately required green names. Listing does
not invoke init, test bodies or new plan entrypoints.

Before executing, root independently checked the excluded-label selection JSON
contained **144** tests and **no** sqlite-plan feature name. Only that actual
selection ran: **all144 passed,3.51s**, including unchanged143 and the approved
old-writer-only new fixture diagnostic. Tooling19 tests passed; actionlint,
project391/122, current render and diff checks pass. Raw logs and discovery files
are ignored `local-evidence/coordinator/storagetest015-baseline-*` and
`storagetest015-tooling.txt`. A scratch summary initially printed an assumed
8-step incremental count; actual build log is20, as recorded here.

Coordinator static review required an additive intra-append rollback assertion
because the original draft faulted only after append returned. The actual candidate
retains that whole-unit proof and additionally injects a disposable conditional
trigger at revision2 request insertion, requiring scalar2 to exist, then asserts
known rollback while facade lives, unchanged full snapshot/catalog restoration and
fault-off retry from durablehead1. Author separately validated the exact trigger
mechanism using raw literal SQLite3.54 without new plan functions; that attributed
mechanism observation is documented in the author's note, not facade acceptance.

The new workflow runs all previous135 SQL cases and required old diagnostics,
adds automatic required new green4 and exact compiled feature discovery, and
keeps intentional-red execution explicit under workflow_dispatch boolean
run_plan_feature_tests. No failure inversion or continue-on-error is present.
This changed workflow has not yet run; earlier f24 CI covers only the minimal
interface stage. Different actual full feature-test/harness approval is next,
then selected official NEW green/build/discovery and FIRST genuine missing-behavior
red. No new plan entrypoint or historical red has run; production15 stays planned.
