# STORAGETEST-014 — independent contribution vectors and green SQL environment

Fixture/environment author: `/root/debt_code_reviewer`, explicitly assigned this
bounded role,2026-11-01. New isolated `storagetest014-fixtures` worktree, branch
`task/STORAGETEST-014-fixtures`, refreshed normally from2530654 through actual
API/config approval integration `29443f52f7d0681235b1aedb5da2309308c52150` before
editing. Own only new `tests/fixtures/sqlite_contributions_vectors.json`,
`tests/qt/sqlite_contributions_environment.cpp` and this evidence. The separate
feature author owns contribution tests; root owns CMake/presets/workflow/state/
map/HTML/global migration order. No production or feature test was authored here.

Authority: specification v0.45 §§3.2–3.5/§14/DB-01,
[logical/API contract](../contracts/sqlite-contributions-v1.md),
[exact physical companion](../contracts/sqlite-contributions-schema-v1.md) and
[actual independent contract/API review](storagetest-014-review.md).
Preserved logical blob `f503f385bbd4fd4ce4b43dd3243911dc3bce8f40`, physical
`eeb2fe04be49b7eeced0db6cf9eea29520dffc64`, public header
`0abeea2bb6d1e99faad7d6585400ad3d80e2eae8` and named explicit stubs
`0ad1315601218601289923fe7b02eebfb8573aaa`. No new adapter/factory/codec/service
function or new contribution feature body was called; no feature red was run.

## Independent literal fixture

JSON shape is coordinated with the feature author: top format/provenance,
flat manifest3, ID contribution-evidence-v1, catalogformat1/storeidentity
subathon.sqlite-store, supported starts0/1/2/3, ordered SQL4, complete expected20,
full payload hex/byte count/hash, and codec_vectors with decimalSTRING/hex fields.
Prefixes remain separate immutable fixtures:
[receipt2](../../tests/fixtures/sqlite_receipts_vectors.json), blob
`a79e5e71a3b9ca101441852c7fb7f26f735e9626`, and
[retention1](../../tests/fixtures/retained_context_vectors.json), blob
`0f76fd2c1bb1766c46a47ddcd38f46c18d4704a8`. Their SQL/payload/13/7 projections
are unchanged; full projection3 includes those exact tuples as required.

This author independently read the approved physical companion's four exact
single-line SQL values, combined its named three tables/four NULL-SQL indexes with
the frozen thirteen-object prefix, and implemented a separate Python stdlib
reference for unsigned UTF-8 tuple sorting and ASCII decimal-length F framing.
Every field/null marker was framed; hashlib.sha256 then produced the complete
literal result. No root ignored prototype JSON and no production factory/codec/
service/migration encoder supplied expected results. Independent results match:

- Complete canonical payload: **18126 bytes**, with all bytes retained as JSON hex.
- SHA256: **b72249ba22c4838af98b9ae29600e70969cf3295eea958a30074167221d2ad1d**.
- Projection:20 exact objects, preserving all13 receipt and all7 retention tuples.

Six typed ContributionId vectors are0/0000000000000000,
1/0000000000000001,9007199254740993/0020000000000001,
9223372036854775807/7fffffffffffffff,9223372036854775808/8000000000000000 and
18446744073709551615/ffffffffffffffff. Decimal values remain JSON strings, never
JSON doubles. Python int.to_bytes(8,'big') independently checked all six pairs.
Pure future typed-codec expectations and actual new SQL BLOB representations are
different evidence; the production codec remains a stub.

JSON blob: `44df9918cc8494dc209e3b1812784a080860e06c`.
Final green source blob: `9bf785db253477bf8e074cc920a87b8a23b15299`.

## Three green foundation slots

The new helper includes only the old writer header and Qt. Both earlier catalog
DDL/SQL prefixes, the new literal SQL and exact catalog history are loaded as
independent fixture values. It does not call/link old receipt or new contribution
adapters, codec, manifest factory, migration runner or service. No state-processing
thread, independent SQL connection/handle, provider, credential or live action is
introduced. The same existing private writer executes all SQL on its owner.

| Fixed method | Literal observations |
| --- | --- |
| contributionSchemaPrefixMatchesLiteralProjection | Exact20 actual type/name/table-name/SQL fields, every generated SQLNULL index, full literal history1/2/3 BLOB hashes, singleton INTEGER1/initialSQLNULL contribution allocator, actual Qt/SQLite runtime |
| unsignedContributionIdsAndNullableAssociationsStayDistinct | Actual bound six full-range BLOB8 contribution IDs in canonical byte order; real ID0 row, allocator BLOB0 and observation association BLOB0 versus SQLNULL; only SQLNULL association selected unresolved; FK check clean |
| groupedConditionsRolesAndProofUseLiteralSchema | Exact current and captured support enum/bool dimensions, money pair/long decimal, resolution+missing-rate independence, nonmonetary+original-money independence, empty policy/decision refs, decision choice/actor bytes, optional activity/proof groups and confirmation's absent conditions; real CHECK/type/FK failures |

All byte fields use literal BLOB or QByteArray bindings. BOM/NUL/FF bytes stay exact;
no QString decoder, numeric SQL conversion, DTO reconstruction or guessed semantic
reference namespace creates expected values. Present empty BLOB and SQLNULL remain
distinct. The current state deliberately differs from captured support conditions,
and applied/reversed/unsettled flags remain separate recorded dimensions. This
is representation proof, never eligibility, permission, evidence truth, settlement
or applied-effect verification.

Negative SQL cases preserve real old-writer sql_failure/rolled_back/no normal
value for unknown enum/bool, partial or empty activity/proof, partial money or
decision, wrong/NULL ID types, invalid confirmation/support shape, dangling
association and ReceiptId0. A separate NULL ContributionId INSERT uses existing
unanchored receipt3 with otherwise valid fields, isolating required SQLNULL rejection
while ID0 remains valid. No persisted selected-NULL row is fabricated. All setup
writes/readbacks complete successfully before these constraint failures are accepted.

Raw receipt originals/attempts and contribution rows are seeded by literal SQL in
the diagnostic transaction. They prove same-session relational/byte compatibility;
they do **not** prove the adapter's upstream requirement that input receipts were
committed before a contribution unit began. No claimed earlier commit, normalized
classification, verified provider fact, policy/actor authorization or semantic
resolution is inferred. Actual feature fixtures separately arrange prior commit.

## Actual local commands and scope

- `cmake --preset sqlite-contribution-debug`: exit0.
- `cmake --build --preset sqlite-contribution-debug --target subathon_sqlite_writer -j2`:
  exit0; existing writer archive only was built for this consumer.
- Selected Qt moc generated a temporary include under
 `/tmp/storagetest014-fixtures-native`; a Python subprocess argv invoked
 `c++ -std=c++20 -fPIC -Wall -Wextra -Wpedantic`, server include and temporary moc
 paths, three absolute JSON macro values, this green source, the existing writer
 archive and pkg-config Qt6Core/Qt6Sql/Qt6Test flags. Exit0, zero warnings/output.
 No new/old adapter/factory/service library linked.
- `/tmp/storagetest014-fixtures-native/sqlite_contributions_environment -functions`:
 exactly the three method names above.
- `/tmp/storagetest014-fixtures-native/sqlite_contributions_environment`: exit0,
 all3 slots plus init/cleanup pass (5total),0failed/0skipped. Actual Linux x86-64,
 GNU16.2.1, Qt6.12.0, SQLite3.54.0; this host result is not official bundled proof.
- A separate verification pass rebuilt full F bytes from approved document SQL and
 sorted JSON tuples: complete hex/hash, all six decimal/hex strings and unchanged
 prefixes13/7 pass. Native symbol audit finds actual old SqliteWriter::enqueue_work
 and no contribution/receipt codec/repository/factory/semantic service symbol.
- `clang-format --dry-run --Werror tests/qt/sqlite_contributions_environment.cpp`
 and `git diff --check`: exit0.
- `python3 tools/check_project.py` and `python3 tools/progress.py render --check`:
 exit0,391 task records/122 decisions/local links/presets/current root-owned HTML.

A transcribed long-decimal expected hex literal was corrected against independent
Python ASCII bytes before the first green execution. First green3 passed; focused
raw support-dimension/decision readbacks and allocator-zero/isolated-NULL checks
were then added and final green3 rebuilt/reran successfully. These are fixture
authoring checks, not feature red or product implementation. Local captures:
`/tmp/storagetest014-fixtures-{configure,writer-build,compile,green}.log`.

Root shared registration/CI attachment, exact CTest/link guards, different full
actual test/fixture/harness review, selected official-Qt green fixture execution
and first authorized intended feature red remain separate gates. No existing
acceptance or historical red was rerun for this bounded author task. Existing
writer023451/migration4ee236/retentionc96/receipt af095d source and every completed
assertion/interface/prefix are unchanged. No contribution implementation, actual
numeric Capture codec/rates, bundle/effects/jobs, backup/update/recovery/power-loss,
platform/provider/live capability follows; all seven integrations/three rate
providers remain required release scope.
