# CORETEST-008 independent current-main/harness review

Reviewer `/root/codec_author`, different from harness author `service_author` and
root merge author, 2026-11-04. Clean candidate
`732f73e2b0f4e660b7592ec81699ebca682bd390` contains merge `b1cf46f` of main
`fe727c8` and author `96c22392` imported as `4021d74`. Only this note changes.

**APPROVED for the actual main/config/discovery/CI harness.** No blocker found.
Source approval `1648acf1` and different full test approval `bd7f5e3e` remain valid;
their oracle/source gates were not repeated. This approves no FIRST red, future
revision, production or merge.

| Actual artifact | Git blob |
| --- | --- |
| Root CMake | `60981c2775c71ebd87831120b2df41cc4d062a6f` |
| Core CMake | `6d50a97cf49cebb3d08969c4d5717970139a9334` |
| Presets | `d43272ac4d5bdb82f5a26d378cef1f72aba66d2f` |
| Test CMake | `7417c3e39d788d636daff7e05aa7b3dd954cbec4` |
| [Timer label callback](../../tests/cmake/TimerEditorialLabels.cmake) | `d6e6aa448586bd401c4164caba1024e00c6107d5` |
| [Workflow](../../.github/workflows/timer-editorial.yml) | `ed04c5e649bf207ddd0f11986154230cee56559b` |

Read root/core/tests instructions, actual diffs, state/checkpoint and
[author evidence](coretest-008-harness-author.md). Main's 408 identities/1508 edges
and every other task record are identical; only CORETEST-008 differs. Required
bridge009, rate/password/permission statuses, targets, options, presets and callbacks
survive. Frozen test `3b0c8041`, inventory `1a4fb38d`, header `92281931`, stub
`651c68b3`, logical/API/spec and both actual approval notes remain unchanged.

The std-only module retains lifecycle/debt dependencies and preset 27. Its test
target links the actual inert module/GTest main. Native discovery supplies the
`timer-editorial.` prefix, distinct TEST_LIST and timeout 30. The callback assigns
four labels afterward and errors on missing discovery. No body-execution hook,
fixture/assertion edit, skip or failure inversion is introduced. The strict guard
requires 27 unique source/JSON names, all 44 ED mappings and exact 101=27+74
registration; it rejects disabled/inverted/editorial result-rewriting properties.
Only the old green 74 execute after that guard, with no-tests errors and core limit 0.

CI uses Ubuntu24.04, the existing immutable checkout, explicit common pinned GTest
fetch and disabled system GTest lookup. The unchanged dependency helper
`ecedcf7a` supplies commit `063de7e9578f82b369302001269680b4b1553359`; no new SDK
or separate foundation is needed. Existing std-only dependencies retain their own
green evidence rather than manufacturing a new feature-success substitute.

One discovery-only check on root's existing build independently confirms exact101,
27 names/four labels/30s/normal unsuppressed properties and callback-after-discovery.
Retained JSON: `/tmp/coretest008-current-harness-review-discovery.json`, SHA256
`1a269490e7d1569aa991e56d7b179e46cd6afab9d3bd033b59c945a404a54c66`.
Compared the original old74 raw Passed names and direct exit0 receipt with all 74
current green names: identical. The author's later collector summary-wording error
did not change that child result; its separate recovery record did not replay tests.

Actual selected Ubuntu [run41908880067](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41908880067)
at this candidate is retained in `/tmp/coretest008-selected-ci-732.log`: exact guard,
74 passing old checks and no TimerEditorial RUN entry. Read-only Git/JSON/hash/graph
checks, YAML parsing, four `bash -n` bodies and one Python syntax compilation passed.
No compiler/build, editorial/marker/FIRST-red, foundation or test body ran here.
`python3 tools/check_project.py`, `python3 tools/progress.py render --check` and
`git diff --check` passed: 408 tasks/122 decisions, links/presets/current HTML and
whitespace; sole-note staged inventory/whitespace was checked before commit. Root owns FIRST plain27
red/independent attribution, final metadata, current checks/reviews/discussions/rules
and normal merge; no new owner decision is needed.
