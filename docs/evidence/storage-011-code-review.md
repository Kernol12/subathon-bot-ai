# STORAGE-011 independent code and configuration review

Reviewer: `/root/storage_work_code_reviewer`, 2026-11-01. Source author:
`/root/debt_code_reviewer`; shared configuration/integration author: `/root`.
The reviewer owns only this evidence in isolated worktree
`local-evidence/worktrees/storage011-review`, branch `task/STORAGE-011-review`,
from claim `d93069b` after PR31/main `ef29c87`. No production source, baseline
assertion, fixture, interface or shared build file was edited by this reviewer.

Authority: specification v0.45 §16/DB-01/OPS-02, STORAGE-011 brief, frozen
[migration contract](../contracts/migration-catalog-v1.md), header and reviewed
STORAGETEST-011 baseline. Root/server instructions and the actual contract, tests,
vectors, environment probe and previous writer boundaries were read. Original
test-first approval/red remains valid and was not repeated.

## Actual initial source and configuration

Source author revision `9eb1fef2aec74dc2a841ed68cec0e5c84ed4840d`, integrated
`e183527`, was normally merged and inspected end to end. Source blob:
`d91c7d6831f656ccba93e077d8bf9f541db65a80`. Author evidence `0ac896c` was also
read after normal integration through `a80f30e`; source was unchanged. Shared
configuration `66be523` makes new22 ordinary acceptance after the mandatory
existing-writer diagnostic probe, preserving exact registration checks and old49.
It removes the manual acceptance boolean/draft label without altering assertions.

Exact comparison from claim `d93069b` confirms unchanged migration contract
`cbdea1a`, header `02f83ce5`, original22 cases `a85a6c05`, vectors `12280c8f`,
green probe `832fe616`, writer source `023451987`, writer header/hook and all49
existing assertions. The new implementation includes no Qt SQL/database/native
handle, connection, state thread, nested wait or retry. Every SQL operation routes
through one existing work/session.

The initial source review checked the F-framing/null/byte-order logic, checked
length arithmetic, pure preflight/default inspection/admission priority, exact
manifest accounting, encoding before raw observations, stored types/history,
reserved/full schema identity, whole-prefix transaction and final checks. It also
checked refusal/cancellation cleanup, first-error/uncertainty mapping, unchanged
embedded WorkResult, report suppression and completion lifetime. No additional
blocking finding was identified in those paths at this revision; the Unicode
finding below prevents approval despite passing reviewed cases.

## Blocking finding: valid leading U+FEFF is rejected

`strict_utf8` in [sqlite_migrations.cpp](../../server/src/sqlite_migrations.cpp),
lines49–54 at the reviewed revision, validates a QString by encoding it and then
requiring `QString::fromUtf8(bytes) == text`. Qt's decoder consumes an initial
UTF-8 BOM. A valid leading U+FEFF therefore fails this comparison even though it
is neither NUL nor an unpaired surrogate or invalid Unicode scalar. The contract
requires exact full-Unicode text and never excludes U+FEFF.

Actual host Qt6.12 observations from an isolated `/tmp` consumer linked against
the reviewed `d91c7d` migration library:

| Observation | Actual result |
| --- | --- |
| QString built from QChar(0xfeff), QChar('a') | Two UTF-16 units; toUtf8 hex `efbbbf61` |
| QString::fromUtf8 of those bytes | One unit, hex `61`, equality false |
| Pure codec step with valid quoted table DDL and leading-U+FEFF schema name | Throws MigrationFailure invalid_manifest, enum2 |
| Literal SQL CAST(X'EFBBBF61' AS TEXT) and its BLOB bytes | Returned QString and raw BLOB both preserve `efbbbf61` |
| Actual quoted CREATE TABLE with that name; SELECT name/CAST(name AS BLOB) | Returned name has two units and both byte observations are `efbbbf61` |

The codec input uses version1, printable ID `bom-name`, valid quoted CREATE text
and a schema object named from the explicit two QChars. It is a valid pure-encoding
input; codec acceptance is intentionally separate from reserved catalog acceptance.
The driver observations also show that a real stored schema value can retain the
mark. Source read_schema → stored_text → strict_utf8 routes such preserved text
through the same faulty comparison. That call-chain consequence is a source
inference, separate from the actual codec/Qt-value observations above.

The value check was prompted by primary
[Qt conversion documentation](https://doc.qt.io/qt-6/qstringconverter.html#Flag-enum),
which distinguishes normal initial-BOM consumption from preserving it with
ConvertInitialBom. The CLion debugger skill was read, but its required debugger
router/minimum tool set is unavailable in this session. An isolated native
consumer was therefore used as the fallback. The consumer constructs the explicit
QChars, prints sizes/hex/equality, calls the actual public codec and observes
independent Qt SQL literal/schema values. It changes no repository source/test or
live state.

Compilation used Python subprocess with `c++ -std=c++20 -Iserver/include`, the
actual review-build migration/writer static libraries and pkg-config Qt6Core/Sql
flags; the corrected QCoreApplication consumer exited0. An initial expanded
driver consumer omitted QCoreApplication and crashed before useful driver evidence;
that probe setup error is not a product failure or baseline red. A separate bound-
QString experiment dropped the initial mark before storage, so it is unsuitable
as the regression fixture. Literal SQL/quoted DDL plus explicit raw-byte assertions
avoid hiding the edge through that separate binding behavior.

Required resolution: preserve valid initial U+FEFF through canonical encoding and
stored-text validation while retaining malformed-text rejection. A BOM-aware
validation path or the already checked scalar encoding can satisfy this; silently
forbidding U+FEFF or changing existing assertions cannot. The coordinator assigned
a different test author to add focused independent codec/stored-schema regressions.
Their actual assertions need separate review and an observed intended red against
this source before a production correction. No production fix or assertion change
has been made by this reviewer.

## Commands actually run and current verdict

Linux x86-64, GNU16.2.1, Qt6.12.0/host SQLite3.54.0, CMake4.4.5,
Ninja1.13.2 and Python3.14.8; synthetic/disposable state only.

| Command/check | Actual result |
| --- | --- |
| `cmake --preset sqlite-migrations-debug` | Exit0 at `e183527` |
| `cmake --build --preset sqlite-migrations-debug` | Exit0 |
| `ctest --preset sqlite-migrations-debug -L sqlite-migrations` | Exit0; all original22 passed |
| `ctest --preset sqlite-migrations-debug` | Exit0; full76 passed, including old49 and the seven-slot environment executable |
| Exact frozen-artifact comparison from `d93069b` | Empty diff for contracts/interfaces/assertions/vectors/probe/old writer |
| Isolated actual Qt/public-codec/literal-schema value consumer | Exit0 with the concrete observations above; diagnostic only |

The passing76 do not cover this Unicode edge and do not discharge the finding.
No original red was rerun, no passing product claim follows from stubs/history,
and no prior baseline CI result is transferred to this source.

**Verdict: request changes for actual source blob
`d91c7d6831f656ccba93e077d8bf9f541db65a80`.** Source/configuration approval is held
pending separately reviewed regression evidence, source correction and relevant
passing checks. Actual new automatic Ubuntu acceptance, final metadata and
latest-head discussion/rules/merge gates remain with the coordinator. This internal
mechanism grants no update/backup authority, domain migration, application bundle,
recovery, power-loss, provider or release-platform capability.

## Separate actual U+FEFF regression review before red/fix

The reviewer normally merged and inspected actual regression author
`234c2ec9f9b61b010cf875a140f3bdffa5d415f3`, integrated `36aca32`. Regression author
`/root/writer_transaction_reviewer` is different from both this reviewer and the
production author. Actual test blob:
`e94befdc01ceff62d2e486e1e5bff556cabe1e55`; the new CTest names are coordinated
in shared `8534118`. Production source remains the rejected `d91c7d` above.

Both actual new slots and their fixture/helper/constants were read. The codec case
constructs the exact U+FEFF+a QString from QChar, asserts two units and literal
`efbbbf61` bytes, then requires the entire independent payload/digest. Its ID stays
ASCII. The stored case uses literal quoted DDL/data and a frozen history BLOB;
before the runner it asserts actual name/table-name QString values and raw BLOB
types/bytes, then requires committed current inspection, version1/applied0,
normal report/value, retained row17, unchanged raw snapshot and a released lock.
No expected catalog value/digest comes from the production codec or runner.

The reviewer independently reconstructed the full F-framed payload with standard
Python bytes/hashlib, from the exact metadata DDL, quoted Unicode table, ordered
CREATE/INSERT SQL and unsigned UTF-8 tuple order. Every byte matches the frozen
inline hex:638 bytes, SHA256
`baf9a94eeb7cbdd3994a277363ae6dd1cd4b63af76f99b02b48f8f3ac4775b7d`.
The name frame independently equals `343aefbbbf61`. These oracles implement the
existing full-Unicode contract without weakening malformed-text checks. The
stored test can initially fail in preflight, while its full successful-inspection
requirement also protects against a correction limited to only the public codec.

The actual diff adds95 lines and deletes zero: original22 method bodies and all
ten JSON vectors remain unchanged. Source/header/contract also remain unchanged.
Independent configure/build passed. Compiled-functions versus CTest JSON confirms
exactly24 migration names,78 total entries, including both new names. Project,
progress render-check and whitespace checks passed. Neither new body was executed
by this reviewer, and compilation/discovery is not a regression red result.

**Regression-test verdict: approved the actual two new U+FEFF assertions/fixture
and their registration at test blob `e94befdc01ceff62d2e486e1e5bff556cabe1e55`,
integrated `36aca32`, for the intended current-source red.** The coordinator may
now run and record those exact two against rejected source `d91c7d` after this
approval, without replaying the original22 stub-red. Only then may the production
author correct the source. Source/config approval remains held until that actual
fix is reviewed and all applicable unchanged acceptance/regression checks pass.
