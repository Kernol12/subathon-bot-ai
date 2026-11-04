# SESSIONTEST-001 resumed main integration and label compatibility

Root author,2026-11-04. PR59 resumes from clean3c3035ca after PR62 normally merged
9345aff49a5cee8a1e0fa8f8251ad4fb01c853d9 fromacf4e7b/all13SUCCESS. Merge/head trees
matchc9a4a9c2; all12 post-main checks subsequently SUCCESS. Normal SSH publication
works; the old access blocker is obsolete and no owner decision remains.

Mergee92f3cb resolves five shared paths without dropping password or permission
options, targets, presets, tests or evidence. Merged state retains406IDs/1497edges,
SESSIONTEST001's valid local baseline, current permission completion and additional
historical CONTRACT005/008 notes without changing those tasks' other fields.
Generated HTML is regenerated, not conflict-spliced. All reviewed password source,
header, observer, fixture, test and original red/native evidence remain unchanged.

The only new harness behavior uses explicit TEST_LIST plus CTest-time label include
for password cases, following the actually selected-CI-verified PR62 correction.
All four password labels and60s timeout remain; DISCOVERY_MODE PRE_TEST,37 names,
private failure wrappers, real SDK defaults, SDK foundation/fixture-generator
isolation and mandatory exact inventory/green CI steps stay intact. No reviewed
assertion or expected input is changed; no error inversion/skip is introduced.
Existing test reviews stay valid for their unchanged blobs. This new root
integration/harness revision still needs actual different review and selected CI.

Actual configure/build/list-only checks passed:

```sh
cmake --preset password-hashing-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-sodium-1.0.22-sdk/prefix
cmake --build --preset password-hashing-debug
ctest --preset password-hashing-debug --show-only=json-v1
python3 tools/check_project.py
python3 tools/progress.py render --check
```

Exactly41 total,37 feature+1 independent SDK foundation+old3. The unchanged actual
CI Python guard passed on real discovery:37 enabled intentional-red methods with
all labels and mandatory independent green SDK check. Logs are
`/tmp/sessiontest001-{main,labels}-{configure,build,discovery}.*`.
A preliminary printed prefix counter used the wrong suite-name assumption; the
actual source-definition/label guard confirmed37. No test content changed.

Meaningful combined-module integration check configured a separate scratch build
with password+effective-permissions+exact-numeric enabled, both authentic explicit
SDK prefixes and hostGTest1.18.0. Compile/list passed105 total with exactly37 password
and30 permission cases, each enabled/uninverted and intentional-red. Commands and
raw outputs: `/tmp/subathon-pw-permission-integration-kiwelb8a/commands.json` and
configure/build/discovery stdout/stderr. Both TEST_LIST variables and label callbacks
coexist. This checks shared wiring without running a feature body.

No feature, marker, native SDK foundation, input generator, cryptography or live
operation was replayed. Existing FIRST native/PH inputs and FIRST37-case red remain
as recorded in [historical handoff](sessiontest-001-handoff.md). No SESSION-001
production release. Next: actual new harness review, normal fast-forward publish,
FIRST selected Ubuntu source/staticSDK/runtime-foundation/exact37/old3 CI and all
current checks; final status/normal review-discussion-protection/merge gates.
