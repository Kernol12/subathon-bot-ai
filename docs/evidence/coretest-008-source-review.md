# CORETEST-008 independent minimal-source/harness review

Reviewer `/root/codec_author`, different from source author `service_author` and
root harness author, 2026-11-04. Clean candidate
`9538c1514cdb3445af4bbe442c700a145e2fdd54` contains author `09afec23`, imported
as `384fb3a`. Only this review note changes.

**APPROVED for the actual inert source and minimal build configuration.** No defect
found. This permits separately coordinated test authoring, not test-body execution,
implementation, future test approval or merge.

| Actual artifact | Git blob |
| --- | --- |
| [Public header](../../core/include/subathon/core/timer_editorial_commands.hpp) | `92281931bd09e9130b3266b5465f6cff6d5f81de` |
| [Source](../../core/src/timer_editorial_commands.cpp) | `651c68b3db599849c3e539c908e6dd57f633c0bf` |
| [Author evidence](coretest-008-source-author.md) | `d31b72055d5b68e463bbc6ff66220a6d94152a8f` |
| Root CMake | `d53cddc0065a4f97ff4d55af447128c67dd57c18` |
| Core CMake | `b74d0c9288d19a67f38da23b7b7b454f166b4d49` |
| Presets | `dcf0cc52e925d03846ad9d6e5fc89d7d1b75d2b2` |

Read root/core instructions and the approved API `a2ac5a2c`, logical `f4a4c3ce`,
specification `81406806` and actual API-review blob `5debe113`. Complete namespace
token comparison confirms all fields/order/defaults, enums, four-alternative variant,
fixed transition array, limits, signatures, private owned-result construction and
const/noexcept access match the approved declarations. No certificate or test-only
success factory is exposed. Existing lifecycle/debt headers retain their approved
`69cad702`/`f3cc819c` blobs.

The sole `project_timer_editorial_command` body is exactly the immediate
`throw TimerEditorialCommandsNotImplemented{};`. No validation, clock observation,
math/reducer helper, audit or successful proposal precedes it. Private data move,
getters and allocation-free fixed error mechanics are inert; marker and what text
match the API. Static archive inspection confirms the evaluator/inert definitions
without SDK/GTest/global-initializer or Core helper-call symbols.

The opt-in flag forces existing lifecycle/debt targets and tests, and the new static
target publicly links those existing std-only dependencies. Preset 27 inherits the
time-debt profile and explicitly enables lifecycle checks; defaults and existing
password/permission lanes remain. No new SDK or standalone foundation is needed.
Retained root build/list records show 16 successful build steps and 74 old cases:
32 debt, 39 lifecycle and three infrastructure. No editorial test exists yet.
Graph comparison with incorporated main confirms 406 IDs/1497 identical edges and
unchanged password/permission baseline records.

Actual reviewer checks: full source/declaration comparison, Git hashes/diffs,
`clang-format --dry-run --Werror`, read-only `nm -C` and retained build/discovery/
graph inspection; all passed. No build/compiler, constructor/getter, feature/marker,
SDK/foundation or test body was run. `python3 tools/check_project.py`,
`python3 tools/progress.py render --check` and `git diff --check` passed: 406 tasks/
122 decisions, links/presets/current HTML and whitespace. Sole-note staged scope/
whitespace was checked before commit. Existing approved green dependency suites can supply their bounded checks;
new independent tests, their actual review, FIRST intended red and later production/
code/integration/current-CI/normal-merge gates remain separate and root-owned.
