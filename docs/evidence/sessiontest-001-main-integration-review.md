# SESSIONTEST-001 independent main-integration/harness review

Reviewer `/root/codec_author`, different from root integration author, 2026-11-04.
Actual clean candidate `9f6cd8294e47544aa1b1612782e8482123b01d67` includes normal
merge `e92f3cb` of main `9345aff` into preserved password task `3c3035ca`.
Only this review note changes.

**APPROVED for this actual main integration and label-only harness correction.**
No blocker found. Existing password source/test/native/red reviews retain their
scope; no feature behavior or new selected-CI/merge result is approved here.

| Actual changed/relevant artifact | Git blob |
| --- | --- |
| Root CMake | `2b1c7720ff8f36bffe0b9610a67aaa5a9674a18f` |
| Presets | `3082b643b69c052549ed02ce7cdff88765b64991` |
| tests/CMakeLists.txt | `102896243fa8a2e1201eb15a72223896e37c070b` |
| New [password label include](../../tests/cmake/PasswordHashingLabels.cmake) | `710e70ab80f7817312f0f3f0585629ae7233c973` |
| Unchanged password workflow | `cb61b3ec028eddee805f7b6c9928d834789b54c7` |
| [Root integration note](sessiontest-001-main-integration.md) | `79f4c899943ac2327f634e1b1acc9922e1b2bbf3` |

Read actual diffs, root/tests instructions, checkpoint/state and integration note.
Both password/Sodium and permission/Boost options, targets, presets and tests remain
present. Imported dependencies stay opt-in. The private 12 wrappers remain confined
to password feature tests; both independent SDK programs and the nonautomatic input
generator retain their previous links/boundaries. No body-execution rule is added.

The password discovery now sets its distinct `TEST_LIST subathon_password_cases`
with scalar TIMEOUT 60. A CTest-time include then applies the complete quoted four-
label list, rejecting an empty list. Actual generated includes place each module's
label callback AFTER its own discovery include; the password and permission list
variables remain distinct. No skip/inversion/result rewriting, assertion or fixture
change accompanies the compatibility correction. The unchanged workflow still
requires all 37 enabled intentional-red methods, the separate green foundation
and old three checks; generator execution remains excluded.

Independently compared all 37 existing password-owned source/workflow/contract/
review/native/red artifacts with `3c3035ca`: byte-identical Git blobs. No SDK chain,
test-oracle audit, first-red attribution or native proof was repeated. Current
checkpoint explicitly supersedes the historical access blocker and retains those
completed gates. Source remains three immediate marker throws, not production.

Compared actual `depends_on` edge sets against main: identical 406 task identities
and 1497 edges. Only SESSIONTEST-001's task fields and two historical note-only
changes in CONTRACT-005/008 differ; every other task record is identical. Permission
completion and planned implementation states are preserved. HTML is regenerated.

Read retained configure/build/list records: password-only discovery 41 includes 37
feature, one SDK and old three; combined discovery 105 = 37 password + 30 permission + two
SDK + old 36. All feature names/labels/timeouts/enabled/uninverted properties agree;
no generator is registered. Combined commands all exit 0 with empty stderr. Scratch
`/tmp/subathon-pw-permission-integration-kiwelb8a/commands.json` SHA256
`272d9b4f6237762cbaaa3108d9fbdef01c9d28cabf5f505b4dd8f13ed733660c`;
discovery SHA256 `696ed79b5ebe4600fa8f98f8bbca24bdcbf4ab5aa6f35f47b75b3ae457ecbc86`.
This is actual retained compile/list evidence, not a new execution by this reviewer.

Actual reviewer checks were read-only Git/diff/blob and Python JSON/graph/property
comparisons; all passed. No configure/build/discovery, SDK/foundation/input-generator,
feature/marker/red or test body ran. `python3 tools/check_project.py`,
`python3 tools/progress.py render --check` and `git diff --check` passed: 406 tasks/
122 decisions, links/presets/current HTML and whitespace. Sole-note staged scope/
whitespace was checked before commit. Root owns normal publication, FIRST selected Ubuntu password CI, latest-head
checks/reviews/discussions/rules and final metadata/merge. These remain required;
the old remote scaffold check cannot satisfy them. No owner decision is needed.
