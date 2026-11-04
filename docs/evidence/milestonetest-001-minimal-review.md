# MILESTONETEST-001 independent minimal source and harness review

**Verdict: APPROVED actual minimal source/build configuration at
d8b3d6ae0eddbc27452509cf1f48b82170deaf44.** Reviewer service_reviewer, different
from source author service_author, API reviewer codec_author and integrator root,
2026-11-04. Owns only this new note in the fresh milestone minimal-review tree.
No feature test, foundation, selected CI or progression implementation is approved.
Evidence-only correction53bdf04 is also included; source/build blobs remain exact.

Actual source author2e36bc8/importd481d08 follows the separately approved concrete
APIb8932cd/import21ba96d. Read root/core/tests instructions, frozen model/API and
their actual reviews, complete header/TU, author evidence and shared build diff.
Exact reviewed artifacts:

| Artifact | Git blob |
| --- | --- |
| [Header](../../core/include/subathon/core/milestone_progression.hpp) | 9a1678137ba6f85cbfbb545630f08d36a261790b |
| [Stub source](../../core/src/milestone_progression.cpp) | 4e3291f0e832237470a8cd0bc349e0106ad8fd77 |
| [Source author note](milestonetest-001-stub-author.md) | 5ddb999184e597d79d435085af6d8204b91bb41c |
| Approved API | 58b8e129815c0525176cc6c1b6830a7e6f0a9fdb |
| Approved model | 0115a5cc3cdc4fb1ebb8c9739716a27cb3379933 |
| Root CMake | 8ab796040345500b7f6f0aceec121d9e45fc8558 |
| Core CMake | 1d9eb9e498375ac77ffda60a959598f13559604b |
| Presets | 94ef52deb3716a96c1c898a51b649c6e1c2f0552 |
| Corrected harness evidence | 48fac9e22dff1b9ad2d035c1dd5e48b6a6b4bc42 |

## Source findings

No blocking source/configuration defect was found. All eight complete aggregate
field lists, types, field order and defaults match the frozen API, including the
five limit defaults, signed64 inputs, unsigned64 derived values and Exact fraction.
Both enums match exactly. A separate read-only balanced-brace/whitespace-normalized
declaration comparison confirms those complete matches. No target/history/score
semantics or wire/storage field is added by this minimal source.

The final result owns exactly one private MilestoneProgressionData. It has the
approved public defaulted copy/move operations, const noexcept accessor and private
data-taking constructor, with only advance_milestone_progression as friend. No
default/public successful constructor, mutable accessor, implicit conversion,
borrowed input member or validated-certificate input is introduced. The accessor
lifetime comment follows the approved owner/assignment/move boundary.

Failure construction retains its enum, exposes noexcept code() and fixes what()
to Invalid milestone progression input. The distinct logic_error marker has the
exact approved text. The entire sole feature definition is the unconditional
single statement throw MilestoneProgressionNotImplemented{};. Its complete
signature/default limits match the API. Unnamed definition arguments avoid unused
warnings without adding statements. No validation, helper/parser, arithmetic,
successful result, clock/permission/state/action or other feature call precedes the
throw. Header includes are standard library plus the existing core Exact boundary;
no Qt, provider/network, SQL or UI dependency appears. GPL identifiers and repository
format checks pass. No exception or marker constructor is invoked by this review.

## Build and actual independent checks

The new default-off option enables the existing exact-numeric lane before core
configuration. Its explicit static library compiles the actual TU, publicly links
subathon_exact_numeric and uses existing C++20/include/warning settings. Preset24
milestone-progression-debug inherits exact-numeric-debug. All previous configure/
build/test preset entries are preserved exactly; slot23 remains reserved for the
separate rate branch. There are no new test registrations or workflow changes.

Actual commands in the fresh isolated review worktree:

```sh
cmake --preset milestone-progression-debug
cmake --build --preset milestone-progression-debug
ctest --preset milestone-progression-debug --show-only=json-v1
ctest --preset milestone-progression-debug --output-on-failure
clang-format --dry-run --Werror core/include/subathon/core/milestone_progression.hpp core/src/milestone_progression.cpp
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All pass. Fresh configure selects GNU16.2.1, installed Boost1.92.0,
GoogleTest1.18.0 and Python3.14.8 without a guessed local source override or download.
Build completes14 steps, zero compiler warnings. Parsed CTest discovery confirms
exactly33 existing numeric plus three infrastructure checks, none disabled or
failure-inverted and no milestone command. All36 pass in0.68 seconds. Source/search
inspection confirms no build/discovery/setup invokes the new operation or marker;
only declarations, definition and friend reference them. Existing numeric executables
link the existing numeric target, while the new milestone library is merely built.
Project consistency passes396 records/122 decisions, links/presets/current HTML;
render and whitespace checks pass. No fixture, new test or feature body is authored.

An evidence-only correction was sent to root: the initial harness note139b0376
said installed GoogleTest1.17. The actual root CMake cache selects
/usr/lib/cmake/GTest, whose ConfigVersion file is1.18.0, matching this independent
fresh build. The minimum requirement1.17 is distinct from the installed version.
Root corrected the actual note in53bdf04; this reviewer merged and inspected that
one-line correction, resolving the finding. Source/configuration is unchanged, so
the passing native checks were not repeated. The earlier nonexistent /tmp
GoogleTest-source setup attempt remains a setup error, never an intended red.

## Remaining gates

This exact minimal source/configuration may integrate. There is no new selected
Boost1.83 CI lane in this candidate; existing green checks do not establish that
the new target compiled there. Root must supply/review its actual workflow and
record current selected configure/build/discovery/foundation evidence. Future
fixtures/tests need different actual review before FIRST ordinary named-marker red.
Compilation/dependency failure, a skipped case or catching the marker as success
cannot replace that baseline. No future schema, test, foundation or source revision
is prospectively approved.

MILESTONETEST-001 remains incomplete; production MILESTONE-001 remains planned.
Progression arithmetic/output allocation, actual score/history authority, edit/reset/
manual policy, durable atomic application, actions/presentation, UI/provider/platform
and release integration remain separate reviewed/tested work. Root retains shared
state/evidence/checkpoint/CI/publication and subsequent gate ownership.
