# STORAGE-016 — pure private hold-value helpers

Author candidate, **not independent validation or approval**. Explicit reassignment
from planned validator to implementation helper follows root claim4622646 and
role-refinement `b2b59343303c7ba92ae3ae5dd62332430e5628de`. Separate root/reviewer
validation and full source/allocation review remain required. No task completion,
runtime acceptance, ordinary CI promotion or merge is claimed here.
**Known independent review finding OPEN:** early a18a violates cross-stage pure
priority; its correction is HELD for root's additive regression/review/reached-red
gate. Passing author checks below do not establish that missing combination.

New isolated branch `task/STORAGE-016-values`, worktree
`local-evidence/worktrees/storage016-values`, actual main
`017d40f6233e3416b4a785924052a9ab9bbeb83f` advanced to b2b5934 before edits.
Prior test-author and unused validation trees are preserved. Own ONLY
[private header](../../server/src/sqlite_application_hold_values.hpp) and this note.
Header commit `1cf31dfe19350da9f6d70f942fc525b5af424f46`, blob
`a18a0bd7705f8043daac75acd365471cc59e49c5` (868 lines), sent to runtime author
for integration before these author-only checks. No shared/other-author files copied
or changed. Runtime author owns actual SQL/session/reference/rollback implementation.

## Authority and agreed interface

Read root/server instructions and STORAGE-016 brief. Fixed normative blobs:
concrete701c710f83b0361a075c0d0bca7cf6ec07fb351e,
physical83e96362dd8dd9af0e6ab2ac0fea91269774571c,
API46c70297bacddbdd1649abfe58a76b33ce9773c4,
public headerfe50fb10e6f5a7198df27358666734ad02fb47d1.
Reviewed baseline CPP0cce9545c3a3851b25df9131e6e35e97366aacee and frozen
fixture63de09c5/helper7ca946f2/environmenta17312fe remain byte-exact. Root's valid
35-slot/265-scenario SAME CLEAN598 red followed actual review/own selected
CI41640390883 before production assignment; original invalid261+4 is preserved.
This role does not replay either red or change any reviewed assertion/fixture.

Runtime author agreed `subathon::server::hold_detail` interface and required target
semantics (the early implementation has the priority defect recorded below):

- `validate_options`, `validate_find`, `validate_active`, `validate_history`,
  `validate_detection`, `validate_resolution`: complete pure preflight; ALL options,
  ALL keys/typed subjects, ALL nested counts, body/matrix/Exact/local shape, then
  actual input owned bytes. No ticket/SQL/cleanup/current state inspection.
- `shape_cause`, `shape_observation`, `shape_detection_data`, `shape_detection`,
  `shape_resolution`: stored-body checks with already trusted options. They exclude
  INPUT aggregate input_items/owned_bytes AND per-vector count ceilings. Runtime
  proves actual selected declarations/domain/counts before allocation and applies
  appropriate result/reference materialization caps. Strict SQL must distinguish
  typed field overage from malformed data before calling shape helpers.
- `key_subject`, `key_observation`, `key_cause`, `key_detection_data`, `key_detection`,
  `key_resolution` and `key` overloads expose the full early tier; `raw_less` compares
  unsigned byte strings, including NUL/high bytes. Optional DATA action/rule/target/
  event fields preserve permitted empty versus unknown; actual keys remain nonempty.
- `bytes(T)`/`items(T)` cover every owned public DTO/result and optional/vector/variant
  composition. Checked size_t overflow throws length_error; complete input validators
  map it at the appropriate collection/owned-byte tier. Optional engagement adds0,
  enum/bool/actual variant tag1, signed/unsigned64/count8, canonical Exact numerator
  and denominator their actual decimal lengths. Every repeated owning member counts;
  unreturned old references, padding, framing and vector capacity do not.
- `same` compares all public owned fields, optional presence, variant alternative and
  QByteArray null-versus-present-empty; includes the six requested types plus nested
  value/result overloads. It does not copy the test's framed structural projection.
- `make_manifest()` is pure: calls the existing old pure factory, preserves all old
  five steps/66 objects and adds exact approved12 literal statements/23 new objects,
  full89/F89890/SHA9f5b. Literals were transcribed at authoring time from the normative
  physical document; runtime loads no fixture/document/file and performs no migration.

Exact old option reuse is `bundle_detail::validate_options(l.references)` in unchanged
old helper blob `b0ca921cb27068ecc33228d136b82506fcb6e0a3`. Only its invalid_options
is translated to hold invalid_options. New count/vector max_size/n+1 and checked
2*entries+1 constraints are separate; positive byte ceilings get no invented maximum.
Fixed find/active/history input bytes are charged normally, including active9.

Pure shape deliberately does not compare current/expected/proposed version freshness,
query identities, require entries to belong to submitted expected_active, or infer
current permissions/source existence. Captured occurrences require known unit/Exact/
finalized values; optional copied fields compare when supplied in runtime. Failed
capture keeps unknown finalized amount/token/plan absent. Timer policy/composition
is explicit DATA, with no numerical evaluator or entitlement/default invented.
Replacement local creation/subject/backlink/order shape is checked; existing database
collisions, complete active CAS and selected source/link closure remain runtime.

## Actual author checks

Independent reviewer identified and author confirmed an actual cross-STAGE defect
in candidate a18a: validate_detection calls shape_cause, reaching branch/Exact
checks before request audit/observation required-body validation. A valid-key/count
input with null audit and oversized Exact therefore reaches exact_component_limit
instead of earlier invalid_detection. Resolution similarly interleaves ordering
with required bodies of later entries/replacements. API46c explicitly puts required
scalar/envelope/whole-group shape before matrix/Exact and then ordering; this is
not a proposed total order between unrelated same-stage defects. Source remains
byte-exact1cf31df/a18a for root's actual focused regression and red gate. No source
correction or frozen test edit has been made; no full helper approval is claimed.

Host Linux/GNU16.2.1 20260810, Qt6.12.0, Boost109200, C++20; distinct from selected
official GNU13.3/Boost108300. Temporary untracked harness links existing OLD bundle
library/QtTest and includes the new private header. No SQL session is opened by the
author check. Public hold source remains independently owned; no public stub/factory
or full acceptance body was substituted or replayed.

Actually executed:

```sh
# Header syntax: repository warning flags, Qt imported-target-equivalent SYSTEM includes.
cmake --preset sqlite-hold-debug -B /tmp/storage016-values-author-build -DCMAKE_PROJECT_INCLUDE=/tmp/storage016-values-author-defer.cmake
cmake --build /tmp/storage016-values-author-build --target storage016_values_author_check -j2
/tmp/storage016-values-author-build/storage016_values_author_check
clang-format --dry-run --Werror server/src/sqlite_application_hold_values.hpp
python3 tools/check_project.py
git diff --check
```

PASS: final syntax and fresh26-step configured build0 warnings/errors. Initial ad hoc
plain-I Qt syntax compilation exposed a third-party qbitarray conversion warning;
target-equivalent SYSTEM includes and actual configured target have0 warnings.
No project warning is suppressed.

Author executable PASS:

- ALL35 independently frozen literal B/I oracles, including first/current repeated
  versions, finite replacement resolution, nested history and optional/variant tags.
- All five old canonical payload/hash values unchanged; exact new full89/F89890 and
  SHA2569f5b61d41dc644c417808d781cea73144a8d019ad95646373c30751e71596d0a.
- ALL71 frozen detections and3 batches pass pure validation. The forbidden old Beside
  alias intentionally passes pure preflight: its selected composition/source rejection
  belongs to runtime, not a fabricated pure policy.
- Isolated key-before-count-before-body errors, numeric failure/observation/timer
  categories, input309 fit/308 refusal, stored shapes under input/owned/occurrence
  ceilings1, Exact-3/0 lengths, optional empty envelope/no flag, null-aware equality
  and checked size_t overflow.

Project check PASS396 task records/122 decisions; formatting/diff checks pass.
These are implementation-author checks, not new independent tests or approval.
Actual SQL class/size/canonical ordering, complete64/65 metadata, count-before-allocation,
same-session closure/capabilities/cumulative budgets/known rollback versus unknown,
new35/full264 acceptance and separate code review are still integrator/runtime gates.

Local `/tmp/` artifacts (not committed), SHA256:

| Artifact | SHA256 |
| --- | --- |
| storage016-values-syntax.txt |e1791e07319e73bd4c8a5b888bef592329f3d4353fbdca8e47996697568c2f43 |
| storage016-values-author.cpp |58dc027fcd06320d998824dca905da7b460f6886a7d70bca3eeff5433cf7bdf8 |
| storage016-values-author.cmake |e5ce76c92b51f1a641ef9cc294839869eac1e05fb9c716807791e17969c8b782 |
| storage016-values-author-defer.cmake |d83fadb8b27fd1a0d4a73bc89cf23f4173bb07344e61a70eda0c20c82f899281 |
| storage016-values-author-configure.txt |c6c5b6daa1c4858eca36b3b56187d391cb0281a71fe3227196cb64b529469484 |
| storage016-values-author-build.txt |58694af03699b783ab0d76a4f48bb5e83cc0761dcc733256bd39048d59fe596e |
| storage016-values-author-run.txt |67346f1ab0e72e6d93f6e4e9da886dc80a180e662c37c8f63510198e4d99334f |
