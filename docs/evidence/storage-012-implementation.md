# STORAGE-012 implementation evidence

Author: `/root/debt_code_reviewer`, explicitly assigned as implementer,
2026-11-01. Branch `task/STORAGE-012-implementation`, isolated worktree
`local-evidence/worktrees/storage012-implementation`, from fresh claim `49719cc`
after PR33 merged as `ae57f2b`. Authority: specification v0.45 §§3/6/7, DB-01,
[task brief](../tasks/EPIC-009/STORAGE-012.md) and
[retained context revision1](../contracts/retained-context-v1.md).

## Scope and preserved gate

Owned paths are only `server/src/sqlite_retained_context.cpp` and this evidence.
Root owns shared CMake/presets/CI/progress/map/HTML/checkpoint and global migration
order. Actual source-only commit `a1142253db276f83f6c5e42ea99a51671038b8ae`, blob
`c96dae927f652088d2326e427740975d18ba4df2`, replaces the three explicit bodies in
the existing source file. It adds no API, dependency or new source file.

The [baseline handoff](storagetest-012-handoff.md),
[different actual review](storagetest-012-review.md),
[original intended-red output](storagetest-012-red-ctest.txt) and
[selected official-Qt prerequisite](storagetest-012-ci.md) establish the completed
test-first gate. Root's original16 red at cb7 followed actual full approval and
actual official green3. That red was not repeated by this implementer. Exact
preserved artifacts, independently checked against `49719cc`:

| Artifact | Git blob |
| --- | --- |
| Retention contract | `5b0ee84acbbf1a3ce8dbc57f99c110f3073d353b` |
| Public retention API | `55c82815986052b996a46c408ab616e68815a4f2` |
| Reviewed feature16 | `c0e54003802f63a6f0393250f3bcca58cea8ee04` |
| Independent green3 fixture | `aaa1644771decdb722b1105d220bac5313d54c6e` |
| Frozen manifest/opaque vectors | `0f76fd2c1bb1766c46a47ddcd38f46c18d4704a8` |
| Existing writer/work source | `023451987e40a9330fef91c6c0955f0e527f14e6` |
| Existing migration source | `4ee2365f8dc6f1189d0f7bf02a090596a9ad3c6f` |

No assertions, helpers, schema/interface/core files or vectors were edited.

## Implemented mechanism

The pure manifest factory returns exactly the approved immutable application
migration1, ID `retain-config-context-v1`, supported starts0/1, the two exact DDL
statements and all seven expected objects, including the three NULL-SQL indexes
and unchanged reserved catalog DDL. Existing codec verification matches the frozen
2000-byte payload and SHA256
`0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e`.
Production uses literal approved schema values, never fixture/JSON loading or a
second catalog. The factory does no SQL, adoption, migration execution or repair.

Find/retain operate only through the caller's copied revocable `SqliteWorkSession`.
No constructor/destructor behavior, independent connection/query/handle, thread,
BEGIN, commit, retry or event-loop wait was added. Every runtime read/write is a
fixed main-qualified statement. Existing session ownership, sticky framework
errors, cumulative binding/result budgets and caller publication rules remain.
No budget is raised or reset and no transaction is fragmented.

Pure method validation checks positive options, known kind, key reference and
then retain-only document/link in the specified order before any SQL/capability
operation. Reference size conversions occur only after their positive qsizetype
bound check. Required null QByteArray input is rejected; present empty is valid.
Input errors perform no cleanup and preserve earlier caller writes. Valid input
delegates unchanged capability/SQL/budget `SqliteWorkFailure` exceptions.

References and links use explicit-length std::string-to-BLOB copies; documents
use explicit-length owned QByteArray copies. No text decoder, trimming, numeric
conversion or NUL termination touches key/payload/link bytes. Missing lookup is
std::nullopt; selected BLOB presence is established by actual SQL typeof/length.
A zero-byte BLOB normalizes to non-null empty QByteArray and can be retained again.
Returned strings, payloads and optional typed links own their storage after the
caller session and writer close.

Selected rows are checked for cardinality/shape, actual types/lengths, exact
reference, required nonempty identity and typed optional-link presence. The stored
known configuration target is validated before any content comparison. An existing
identity compares exact bytes and optional-link presence/value: identical content
is already_present; any change is conflict before looking up a newly proposed
target. Only new identities resolve candidate configuration links before insertion;
the context namespace cannot satisfy that target. Absent links remain SQL NULL.
No update/delete/latest-pointer behavior is provided.

Conflict and missing_reference perform no writes or exceptional cleanup; the
caller still decides whether to commit a diagnostic or roll back the whole unit.
Selected malformed/dangling stored data establishes real known rollback before
RetentionFailure(invalid_stored_data). Well-typed selected data exceeding field
bounds similarly uses result_limit. If cleanup reports uncertainty, unchanged
SqliteWorkFailure(uncertain_outcome) overrides the local error. No unrelated-row
scan or inferred/default/partially repaired record is returned.

## Actual verification and remaining gates

Host Linux x86-64, GNU16.2.1, Qt Core/Sql6.12.0, CMake4.4.5, Ninja1.13.2,
Python3.14.8. Only synthetic disposable files and owned fixtures were used.

- `cmake --preset sqlite-retained-debug`: exit0.
- `cmake --build --preset sqlite-retained-debug`: exit0, actual adapter/migration/
  writer libraries and all test executables built; no warnings in the build log.
- `ctest --preset sqlite-retained-debug -L sqlite-retained --output-on-failure`:
  exit0,16/16 passed, no assertion/interface/fixture changes.
- `ctest --preset sqlite-retained-debug --output-on-failure`: exit0,95/95 passed:
  retention16, existing writer/work/migration73 and infrastructure6, including the
  existing green7 and new green3 environment executables.
- CTest JSON versus compiled executable `-functions`: exact nonempty16 retention,
 24 migration,24 work and25 old writer slots match with duplicate/count checks;
 95 total registrations, exactly7 and3 unique compiled environment slots.
- `clang-format --dry-run --Werror server/src/sqlite_retained_context.cpp` and
  `git diff --check`: exit0.
- Artifact invariants above match; before this evidence `git diff --name-only
 49719cc` contains only the owned production source.
- `python3 tools/check_project.py`: exit0,391 task records/122 decisions and local
 links/presets/current HTML valid. `python3 tools/progress.py render --check`:
 exit0, root-owned snapshot current.

The first successful local run retained the inherited draft label/manual-CI
separation from the baseline; every16 body was actually executed by the commands
above. Root separately owns promotion to unconditional automatic retention CI.
Different actual source/config approval, observed selected official-Qt CI, final
latest-head metadata/discussions/current rules and normal merge gates remain.
Local passing results alone do not mark STORAGE-012 done or authorize merge.

This mechanism retains opaque bytes and caller-supplied known links only. It does
not interpret document semantics, implement an evaluator/resolver, invent numeric
configuration IDs or certify typed uint64 SQL codecs; those remain013/014/015 gates
for actual numeric fields. Caller BEGIN/real commit/normal terminal publication,
operational backup/update/recovery authority, app bundle, wire, platform/provider/
live verification and all seven integrations/three FX providers remain required.
