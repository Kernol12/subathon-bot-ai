# CORE-001 implementation handoff

Coordinator `/root`, 2026-10-31, `task/CORE-001-exact-numeric`, based on merged
`cab329b`; [PR #13](https://github.com/Kernol12/subathon-bot-ai/pull/13).
Specification v0.45 §3.5/NUM-01/02/04; exact-decimal contract revision1.

## Behavior and review sequence

Implementer `/root/numeric_implementer` implements complete bounded ASCII JSON-number
validation, owned original spelling with exact Boost rational values, positive rates,
nonnegative factors and signed64 nearest-even final effect conversion. Grammar is
checked before internal resource/domain validation; zero is canonicalized after
validation. Finalizer uses absolute exact integers for parity/range, preserving signed
minimum and checking rounded bounds without binary floating point.

Original32 tests/header and all behavioral contract requirements/examples remain
unchanged from [reviewed/red CORETEST-001](coretest-001-baseline-handoff.md).
Contract status prose and preset/CTest labels now distinguish historical red from
implemented behavior; no assertion or error policy changed.

Candidate `4ab8613`/`873cc6d` passed32 cases after an existing zero-factor assertion
caught a noncanonical Boost zero representation. One independent additive test
`5f9b3c0`, blob `347a810b1edba2ffc36d66ba44b700092bb2b381`, exposed finalizer
handling of mathematical0/1 with a large raw denominator. Separate oracle/harness
reviews `cbf9391`/`37f56ae` preceded actual one-case red at coordinator `6224ac3`.
[Regression evidence](core-001-zero-regression.md) and [raw red](core-001-zero-red-ctest.txt)
retain that failure. Only then did `74f3594` add zero handling before denominator
inspection. No requirement or baseline was weakened.

Final source blob `a1ddb4283292a5da0a81a0f4e8255fffc19cafd7` received separate
`/root/numeric_reviewer` code/configuration approval in `db26514`; see
[review](core-001-code-review.md) and [implementer evidence](core-001-implementation.md).
The reviewer independently replayed33numeric/36full-preset/23credit cases and checked
bounds, cursor safety, exact scale/signs, parity, post-round range and CI configuration.
Original test blob `6cf83aeba79bfc4c50e3e11787147dd6bf342c10` and header
`c0e70aeea1f435fd07f9eb83b04f3ef5602e6c69` are preserved exactly.

## Actual checks and limits

Root configure/build exact-numeric-debug and CTest-Lexact-numeric passed33/33 with
GCC16.2.1/Boost1.92.0/GTest1.18.0; existing credit acceptance/regression passed23/23.
Author/reviewer full numeric preset passed36/36 including scaffold checks. No local
package installed and no live provider/state/credentials used. Final done-state
project/render/Python/DOM/diff results are recorded in the checkpoint/task note.

Actual automatic Ubuntu24.04 [numeric run41183487457](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41183487457)
passed at PR head `fd86a6b`. It installed pinned Boost1.83.0-2.1ubuntu3, verified
compiler macro108300, configured actual Boost1.83.0 with GCC13.3.0 and passed33 cases.
Credit/scaffold CI also passed. [CI/dependency evidence](core-001-ci.md) records pins,
actual source/version checks and the missing required-status-rule configuration.
No protection settings changed. Final PR revision CI/discussions/merge must be
rechecked after completion/evidence commits; publication/merge is separate from
local implementation completion.

No expression evaluator, aggregate-state check, durable numeric review/application,
modifier-combination clamp, refund, SQL/API, provider or UI behavior is supplied.
Windows/release compilers, full supported-platform applications, renderers/OBS,
packaging and live integration gates remain open. CORE-001 completes this primitive
slice only. Next dependency-ready slice should establish reviewed/red timer or
accounting behavior before implementation; full agreed release scope remains.
