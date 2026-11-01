# STORAGETEST-013 — independent receipt vectors and green SQL environment

Fixture/environment author: `/root/debt_code_reviewer`, explicitly assigned this
bounded author role, 2026-11-01. Isolated `storagetest013-fixtures` worktree,
branch `task/STORAGETEST-013-fixtures`, from actual approved API/config revision
`16b15e991402cebb4e15d18b51a4b4311220110a`. This author owns only new
`tests/fixtures/sqlite_receipts_vectors.json`,
`tests/qt/sqlite_receipts_environment.cpp` and this evidence. Root owns shared
build/presets/CI/state/map/HTML and global migration order; the different feature
author owns receipt feature tests. Different full fixture/test review remains.

Authority: specification v0.45 §§3.2–3.4/DB-01,
[logical contract](../contracts/sqlite-receipts-v1.md),
[exact schema companion](../contracts/sqlite-receipts-schema-v1.md) and
[actual prior contract/API review](storagetest-013-review.md). Production receipt
source remains explicit stubs, blob `faf2856d8d61a91e518c50b3c0fdffe80a0f33a3`;
header `b43f9b6026498af18df22c2f695e76b23cb4064a`, logical contract
`e1ed975875e8b62d77d8efff67cfe5e8d5c95038` and schema
`2ef4f1384c5c95c1bef5728b095ff5f4e7877e6a` are unchanged. No production API,
codec, manifest factory, schema or feature test was authored/called here.

## Independent literal vectors

The JSON uses the coordinated flat manifest shape, version2,
`identified-receipts-v1`, catalogformat1, storeidentity `subathon.sqlite-store`,
supported starts `[0,1,2]`, four exact ordered SQL statements, complete thirteen
expected objects, full payload hex and SHA. Prefix1 remains the existing separate
[retention fixture](../../tests/fixtures/retained_context_vectors.json), blob
`0f76fd2c1bb1766c46a47ddcd38f46c18d4704a8`; its step/SQL/payload is not duplicated
as a second manifest or changed. Full projection2 necessarily retains its seven
exact object tuples and adds the approved three tables/three NULL-SQL indexes.

This author independently read the four one-line SQL fences from the approved
actual physical companion, combined its six named added objects with the frozen
prefix projection, and constructed the exact schema tuples. A separate Python
stdlib reference sorted unsigned UTF-8 tuples `(type,name,tbl_name,null-marker,sql)`,
framed every field with ASCII decimal byte length and colon, and hashed with
hashlib.sha256. It used no production factory, migration encoder/hash, stub or root
prototype output as an oracle. The full result independently matches:

- Payload: **10535 bytes**, with exact full bytes retained in JSON.
- SHA256: **24614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2**.
- Complete projection:13 objects, all seven prefix objects unchanged.

Six codec entries store decimal and hex as JSON strings, so no JSON double carries
these uint64 numbers. Literal pairs are0/0000000000000000,
1/0000000000000001,9007199254740993/0020000000000001,
9223372036854775807/7fffffffffffffff,9223372036854775808/8000000000000000 and
18446744073709551615/ffffffffffffffff. Python int.to_bytes(8,'big') independently
checked every pair. These are pure-codec future expectations; domain ReceiptId0
remains invalid and no production codec was invoked.

JSON blob: `a79e5e71a3b9ca101441852c7fb7f26f735e9626`.
Green source blob: `b1ca37beed7a13e757710581990c09859dd4f366`.
The feature author received the exact flat shape/path/macros and early JSON bytes
before this green helper was finalized; neither author edits the other's files.

## Three fixed green slots

The helper includes only the existing writer API. Its literal schema/history setup
uses the two fixed catalog DDL strings from the old fixture, unchanged retention1
SQL, the new four literal statements and independently frozen history BLOBs. It
never includes, calls or links receipt stubs/codec/factory or a migration runner.
Only QCoreApplication/QtTest, disposable files and the existing controlled writer
are used. There is no live provider/credential/process side effect.

| Fixed slot | Independent foundation observations |
| --- | --- |
| receiptSchemaPrefixMatchesLiteralProjection | Exact13 stored type/name/table/SQL fields in literal order; all generated SQL-NULL indexes; singleton allocator INTEGER1/SQLNULL; literal catalog/header/history prefix and BLOB digests; actual selected Qt/SQLite runtime |
| unsignedIdsAndSignedFieldsRemainExact | All six raw eight-byte BLOBs, typeof/length/hex and big-endian byte order; actual receipt columns retain upper-half/MAX IDs, signed64 UTC min/max/zero, uint32 zero/MAX as INTEGER; scalar-constructed U+FEFF plus NUL/FF bound as BLOB; present-empty connection BLOB; actual DDL rejects zero-domain and INTEGER IDs |
| optionalGroupsAndTypedReferencesUseLiteralSchema | SQLNULL versus present-empty BLOB for actors and every optional pair; exact long decimal/currency raw bytes; optional binary namespace/id bytes; two ordinal1/2 attempts at equal timestamp17; separate known context->config a versus arrival config b accepted when both resolve; CHECK rejects partial groups/wrong types and FK rejects wrong retained namespaces |

All receipt byte fields bind QByteArray or use literal BLOB SQL, never QString
text binding. The U+FEFF source is built with QChar then encoded once before NUL/FF
are appended; no decoder strips a leading mark. Expected results are literal SQL
types/hex/integer rows, not new product DTOs or implementation-produced results.
The raw SQL numeric fixture checks storage representation; it does not implement
allocator advancement, receipt creation/history guards, service classification,
repository adoption or new transaction behavior.

## Actual local compilation and green execution

Commands run in this isolated worktree:

- `cmake --preset sqlite-receipt-debug`: exit0.
- `cmake --build --preset sqlite-receipt-debug --target subathon_sqlite_writer -j2`:
  exit0; only the existing writer archive was built for this manual consumer.
- Qt's selected `moc` generated a temporary include in
  `/tmp/storagetest013-fixtures-native`. Python subprocess invoked `c++ -std=c++20
  -fPIC -Wall -Wextra -Wpedantic`, repository server includes, that temporary moc
  directory, both absolute fixture macro definitions, the green source, the
  existing writer archive and pkg-config flags for Qt6Core/Qt6Sql/Qt6Test. Exit0,
  zero warnings/output. No new receipt/migration/retention library linked.
- `/tmp/storagetest013-fixtures-native/sqlite_receipts_environment -functions`:
  exactly the three fixed names above.
- `/tmp/storagetest013-fixtures-native/sqlite_receipts_environment`: exit0,
  all3 slots plus init/cleanup passed (5total),0failed/0skipped. Actual runtime:
  Linux x86-64/GNU16.2.1/Qt6.12.0/SQLite3.54.0. This is host proof, not selected
  official bundled Qt/SQLite proof.
- Python JSON/payload/hash/type/prefix checks: all pass, including all six decimal
  strings, full10535 bytes/SHA and unchanged seven-object prefix.
- Native symbol audit: existing SqliteWriter::enqueue_work is linked; no receipt
  codec/repository/factory, migration runner or retention facade symbol is linked.
- `clang-format --dry-run --Werror tests/qt/sqlite_receipts_environment.cpp` and
  `git diff --check`: exit0.
- `python3 tools/check_project.py` and `python3 tools/progress.py render --check`:
  exit0,391 records/122 decisions/local links/presets/current root-owned snapshot.

The first local green development run passed schema and numeric slots but failed
one QList<QVariant> expectation: SQLite length(NULL) is SQLNULL, while this driver
exposes a typed-null QString rather than invalid QVariant{}. The fixture now
explicitly asserts that selected value isNull, and that both present-empty lengths
are nonnull zero, before comparing every remaining literal column. No SQLNULL
fact, empty-BLOB/type assertion or product oracle was dropped. A preliminary
text-replacement script also assumed pre-format whitespace and stopped before
writing; the corrected source edit was formatted/recompiled. Neither development
issue is feature red, a dependency failure or production evidence.

Logs remain local in `/tmp/storagetest013-fixtures-{configure,writer-build,compile,green}.log`.
Shared CMake attachment, exact CTest registrations/link inspection, different
actual full test/fixture/harness approval, selected official-Qt green execution and
then first authorized intended feature red remain root gates. No new receipt
feature body, original red or absent production behavior was run/claimed here.
Existing writer023451/migration4ee236/retentionc96 source, core/service/ports,
interfaces and prior assertions remain unchanged. No provider support, numeric
ContributionId/plan codec, full application bundle, operational backup/update/
recovery, power-loss or release-platform capability is certified.
