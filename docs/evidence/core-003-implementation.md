# CORE-003 bounded pause-policy implementation

Implementer: `/root/pause_policy_implementer`, 2026-10-31. Branch:
`task/CORE-003-implementation`, seeded claim `c9f6c5f99fd520ec941997958b7f359421e37c91`
from integrated main `01058e2`. Authority: specification v0.45 §4/§18,
TIMER-03/05, pause-policy revision1 and the recorded owner signal decisions.

## Scope and gate

Only `core/src/pause_policy.cpp` and this evidence note are owned here. The
coordinator owns progress, build labels/presets, automatic CI and publication.
The reviewed baseline was approved in `86c0bbd` (final `565be7c`), observed
37/37 intended stub-red at `f39ccc8`, and published through PR20 final
`024974f`/integrated `01058e2`; the baseline handoff retains those artifacts.
Production implementation therefore follows the reviewed/red gate.

The source implements pure proposals for externally managed Manual/Schedule/
RecoveryReview and independent automatic Offline, client-disconnect and Unknown
reasons. Structural and observation/configuration validation precedes any proposal,
even for recovering signals. Pending intervals advance despite other active holds.
Unknown cancels pending Offline while retaining established Offline; either known
status clears Unknown notice/reason/pending. Online and connected-client recovery
compose independently. Each new episode receives its full delay. Remaining-delay
comparison before addition bounds all signed64 arithmetic; active episodes keep no
accumulator. No clock, Qt, network, persistence, authoritative write or dependency
was introduced. No baseline assertions, fixtures, headers or contracts changed.

Final source blob: `d080ac757bedb1e621420c31aca691a6d25b4052`.
Unchanged header blob: `c91b7da3fa1c98a1b2e47ab389f48f74901c2a47`.
Unchanged37-case blob: `646d83f4e38722239bb8b590a9f368141bed4246`.

## Actual local checks

Environment: Linux x86-64; GCC16.2.1 (20260810), CMake4.4.5, Ninja1.13.2,
GoogleTest1.18.0 (`pkg-config`), Python3.14.8. Installed dependencies only.
Commands ran in this isolated implementation worktree; all final checks exit0.

- `cmake --preset pause-policy-debug`
- `cmake --build --preset pause-policy-debug`
- `ctest --preset pause-policy-debug -L pause-policy`:37/37 pass.
- For each preset below, `cmake --preset PRESET`, `cmake --build --preset PRESET`,
  then `ctest --preset PRESET` all exit0:

| Preset | Observed passing tests |
| --- | --- |
| scaffold-debug | 3 scaffold |
| acceptance-debug | 23 credit +3 scaffold =26 |
| exact-numeric-debug | 33 numeric +3 scaffold =36 |
| timer-lifecycle-debug | 39 timer +3 scaffold =42 |
| receipt-intake-debug | 15 receipt simulation +3 scaffold =18 |
| contribution-evidence-debug | 21 contribution simulation +3 scaffold =24 |

The scaffold includes tooling unit and consistency checks. The first
`clang-format --dry-run --Werror core/src/pause_policy.cpp` reported two wrapping
violations; `clang-format -i core/src/pause_policy.cpp` corrected formatting.
The final dry-run passed, followed by a rebuilt37/37 passing pause run.
`git diff --check` passed before the evidence write and is repeated at commit.
No added tests are needed for behavior already covered by the reviewed37-case
baseline; no test edits were authorized or made. Existing stable suites were run
as regressions without relabeling their scope.

## Remaining gates and limits

This records implemented and locally simulation-tested behavior, not independent
code approval. Separate review of this actual candidate, coordinator integration,
automatic pause CI and final publication/status checks remain. The old
intentional-red label was retained under coordinator ownership pending that work.
No task completion, PR publication or merge is claimed by this note.

Delay reconfiguration within an episode and out-of-domain enum values remain
unsupported caller inputs as specified. Timer segmentation/within-interval countdown,
schedules, recovery algorithms, Qt/writer wiring, provider reliability, live operation
and supported-platform verification remain separate gates. No live effects occurred.
All intended release scope remains required.
