# STORAGE-016 — pure private hold-value helpers

Author candidate, **not independent validation or approval**. Explicit reassignment
from planned validator to implementation helper follows root claim4622646 and
role-refinement `b2b59343303c7ba92ae3ae5dd62332430e5628de`. Separate root/reviewer
validation and full source/allocation review remain required. No task completion,
runtime acceptance, ordinary CI promotion or merge is claimed here.
The independent cross-stage priority finding in early a18a was held through root's
actual additive regression/review/reached-red gate, then corrected at explicit
release. A later explicit Captured-token MATRIX finding likewise received its own
reviewed public regression/red before bounded correction. Final source still needs
actual different review; author checks
are not approval or a public runtime/full-suite result.

New isolated branch `task/STORAGE-016-values`, worktree
`local-evidence/worktrees/storage016-values`, actual main
`017d40f6233e3416b4a785924052a9ab9bbeb83f` advanced to b2b5934 before edits.
Prior test-author and unused validation trees are preserved. Own ONLY
[private header](../../server/src/sqlite_application_hold_values.hpp) and this note.
Header commit `1cf31dfe19350da9f6d70f942fc525b5af424f46`, blob
`a18a0bd7705f8043daac75acd365471cc59e49c5` (868 lines), sent to runtime author
for integration before initial author checks. Released correction commits
`d883bdcabf00d07c35bd1271135a0bce9f48dff4` and
`1b8af3652720b90814141c9a6317edf453fe9e6b` gave blob
`ec78b69702ce0e1da3f78b9367dc10170c284bfc`. Final token correction
`18cc890d60a1aedfd1f8be7a148f07d619228bc5` gives blob
`13828cc3c72548791dc4299b7fd4c813c36b59f3`. No shared/other-author files copied
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
semantics; the early priority defect and actual released correction are recorded below:

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
not a proposed total order between unrelated same-stage defects. Source was held
byte-exact1cf31df/a18a through root's actual focused regression/red gate; original
frozen tests were not edited by this helper author. No full helper approval is claimed.

Root independently authored/reviewed additive source `82980fdba7d538e90c66608afd4e2fe45f229cff`
(1a32ed8/70c430f, separate approval1650658). At SAME CLEAN68dc779 with unchanged
runtime0fe97c83b74+a18a, root ran ONLY3 new rows14:20:25.413–25.514Z. Both priority
public rows genuinely failed exact-category assertions (reviewer's direct native
probe corroborates actual11/9 instead of required8), without HOLD-MISSING. The third
lineage row is runtime author's separate fix. Root's original265 valid baseline was
not replayed. Preserved combined raw `docs/evidence/storage-016-regression-red.txt`
SHA256231d325ac11f7ce201ff3b17146968a1d39dd5d5135714b060e5287f2da1bcfc
was inspected unchanged. Root explicitly released this bounded correction afterward.

Final correction uses complete request-wide `body_*` → `matrix_*` → `order_*` passes.
ALL causes, observations, audits, decisions, known enums and required groups/members,
including later replacements and conditional captured/failed-capture members, are
checked before ANY branch-matrix/Exact work, with the explicit Captured token-presence
MATRIX exception corrected below. All matrix/Exact work finishes before
version/occurrence/entry/expected-set/replacement ordering/local links. Matrix ordinal
membership finds the actual supplied member instead of assuming the as-yet-unchecked
sequence is an array index. Existing categories, logical constraints and shape_*
interfaces remain. Options/keys/Exact-size prefix and ENTIRE accounting/equality/
input-wrapper/manifest suffix are byte-exact unchanged against1cf. Stored shape still
has no input aggregate/per-vector caps; runtime owns count-before-allocation.

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

After the released correction, configured native author builds pass0 warnings.
The same35 frozen B/I literals, manifest payloads,71 detections/3 batches remain
PASS. A separate temporary author executable checks the EXACT two reviewed new
priority payloads through the now-implemented private validators: invalid_detection8
in both cases. Additional bounded probes pass for later observation/known enum/
required group/captured required scalar/decision before earlier Exact, matrix before
set order, Exact before occurrence order, and the normal noncontiguous category.
These are author-only direct helper checks, NOT public Store/feature test executions
or a claim that the third lineage regression/full268 now passes. No reviewed test
file was modified or imported; root test commits were inspected read-only.

Actually executed for correction checks:

```sh
cmake --preset sqlite-hold-debug -B /tmp/storage016-values-author-build -DCMAKE_PROJECT_INCLUDE=/tmp/storage016-values-priority-author-defer.cmake
cmake --build /tmp/storage016-values-author-build --target storage016_values_priority_author_check -j2
/tmp/storage016-values-author-build/storage016_values_priority_author_check
clang-format --dry-run --Werror server/src/sqlite_application_hold_values.hpp
python3 tools/check_project.py
git diff --check
```

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
| storage016-values-priority-build.txt |112bcd68f8e60725bb1192b47747ff4ee1872d835dc6b4990a3263b1d37d03bb |
| storage016-values-priority-existing-checks.txt |67346f1ab0e72e6d93f6e4e9da886dc80a180e662c37c8f63510198e4d99334f |
| storage016-values-priority-author.cpp |b2deb3793a18be4cc74790df7eb0bfe7a75152b63ac90daf6b15254ac40151a1 |
| storage016-values-priority-author.cmake |11888167e5f494946ccc7f9726b1836ad2b2883ea3a56baec8fa10121cf8e61c |
| storage016-values-priority-author-defer.cmake |4961ab71684e7d362399d7aec69ae9fc3fa575c6a6eb8d08204c05260d10803d |
| storage016-values-priority-configure.txt |5c00cb4d37528e06718593cd3832d85b82be320a74885acd4889de21aa0cdea1 |
| storage016-values-priority-author-build.txt |599f643c0d5e238de7ab28497da289ac5a8560ef756df09503b74a0e5b039173 |
| storage016-values-priority-author-run.txt |013660e31823531bc07f4b27c3694b63f59407585630a071247b9026acf8bf2b |

## Explicit Captured-token MATRIX correction

Final independent review found that ec78 still put conditional Captured known-token
presence into body_cause/body_detection, contrary to API46c's explicit matrix clause.
Root independently authored new public row36 (testd740a175,35 slots/269 total rows),
with separate actual review001896f integrated at SAME CLEAN
`f67ae7393d105a696ae835649162a3110d8a1568`. Actual public row execution
14:54:48.238859–.266877Z exited1:2 init/cleanup passes,1 exact-category failure,
0 skips/missing markers, agreeing with the separate actual ec78 probe6 instead of8.
Tracked `docs/evidence/storage-016-token-regression-red.txt` remains unchanged;
SHA25695d80748d94dddbe89c3ca389791910b403cf5705cb7a29c0717f6e8ac584dc4
was inspected. Root then explicitly released ONLY the token-phase correction.

Commit18cc890 removes conditional first/request token presence ONLY from body_cause
and body_detection (1 inserted/5 deleted lines). Both matrix_cause/matrix_detection
checks remain byte-exact. No other conditional group is moved; accounting/equality/
manifest/options/tests/fixtures are untouched. Captured token absence remains
invalid_plan_evidence when it is the actual matrix failure, while required envelopes
throughout the complete request still precede either conditional token check.

Native direct private-function author checks pass0 warnings: exact reviewed new row
payload→invalid_detection8; missing first token alone/request token alone→6/6;
null audit with either token absent→8; either earlier replacement's token absence
also defers past a later replacement's null audit→8. All prior35 B/I, manifest,
71 detections/3 batches and priority probes remain PASS. This is NOT a claimed
public-row/full-suite result or independent approval; root owns native full checks,
ordinary selected CI and actual final source/config review. The root-authored test
was inspected, not authored or changed here.

Actual commands use the same configure/build pattern with temporary project include
`/tmp/storage016-values-token-author-defer.cmake`, target
`storage016_values_token_author_check`, and run only that direct pure-function
author executable. Formatting/project/diff checks pass. Additional SHA256 artifacts:

| `/tmp/` artifact | SHA256 |
| --- | --- |
| storage016-values-token-author.cpp |26fb9ee2c17f05f9da396329788f80c45c68f658e1a3a115aef69a06b860f311 |
| storage016-values-token-author.cmake |47a5018d02afe15c6fbe2759b2e40df56b0be2fa6919a5ca5b19ac0b6b7995eb |
| storage016-values-token-author-defer.cmake |cb9e5fa1bcbba1fe7102bed3995ff6e11e7f0dda98dc47e4001263b92497fd9d |
| storage016-values-token-configure.txt |5c00cb4d37528e06718593cd3832d85b82be320a74885acd4889de21aa0cdea1 |
| storage016-values-token-build.txt |599f643c0d5e238de7ab28497da289ac5a8560ef756df09503b74a0e5b039173 |
| storage016-values-token-existing-checks.txt |013660e31823531bc07f4b27c3694b63f59407585630a071247b9026acf8bf2b |
| storage016-values-token-author-build.txt |9e6fea44081793f9f1727a228ef785e45052f8a48eaca872c59002f1d7158312 |
| storage016-values-token-author-run.txt |05bcfec11889b6169ff869de3cdb78a148e4a83f93978c5ad96835ad9d19ac8a |
