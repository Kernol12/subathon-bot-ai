# MILESTONETEST-001 logical progression contract author handoff

Author: service_author, 2026-11-04. Dedicated branch
`task/MILESTONETEST-001-contract-author`, worktree
`local-evidence/worktrees/milestonetest001-contract-author`, from integrated main
`3217c21` plus explicit coordinator claim `f04e833`. Owns only
[milestone-progression-v1.md](../contracts/milestone-progression-v1.md) and this
note. Prior ACCT/locale/rate worktrees and reviews are preserved. Root owns all
shared state/HTML/maps/schema/build/CI/pins/checkpoint and publication integration.

Read root/core instructions, architecture/workflow/test strategy, actual task
MILESTONE-001 and all adjacent MILESTONE-002–008 briefs, specification v0.45
§§3.4/3.5/8 and MILE-01/NUM-04, CONTRACT-002, exact-decimal contract/public header,
captured application-plan contract/header, and actual score balance/component/state
interfaces before proposing the model. There is no existing milestone public type
or implementation being retrofitted by this document.

## Actual proposal

Contract blob: `0115a5cc3cdc4fb1ebb8c9739716a27cb3379933`.

The model receives one authoritative post-effect signed64 micro-point score,
strictly ascending current target definitions and an active completed prefix. It
returns an owned proposal retaining earlier completion evidence unchanged, appends
every unfinished target at or below the score in order, and separately projects
per-completed/current track deficits, next remaining and exact interval progress.
It never aggregates/rounds credited score or derives it from requested plan amounts.
Each newly achieved row captures the actual same supplied score, current target/
definition and retained context; overlay animation cannot fabricate intermediate
score observations or delay the achievement commit.

Full signed64 targets/score, including zero and negatives, are supported. A valid
derived distance can be the full unsigned64 magnitude 18446744073709551615; it is
not rejected merely because signed64 subtraction would overflow. The ratio has
bounded exact components and no floating-point/intermediate display rounding.
The caller still handles actual aggregate overflow atomically under NUM-04.

Captured completion target/score/context remain historical evidence. Projection
uses CURRENT definition thresholds, including after an already authorized completed-
target correction, without replaying or rewriting old achievements. Manual-completed
prefix records may legitimately capture score below their target. An omitted or
reset prefix cannot obtain permission/history validity merely by passing this pure
model; actual edit/reset/replay/queue policies remain their required owning tasks.

The specification states an interval between a previous completed milestone and
the next, but provides no first-interval numeric origin. The document explicitly
proposes absent interval progress before the first achievement, while preserving
the first target and exact remaining amount. It does not insert an artificial zero
threshold, constrain the first target's sign or claim a settled first-bar UI policy.
This is an engineering projection choice for actual document review; a later
requirement for such an origin must be explicit before its own presentation work.

MP01–22 are independent literal examples, including score 800 below completed
target 1000, with deficit 200 and next target 2000 requiring 1200 more with progress 0;
ordered multi-crossings; refund/recovery
without repeats; manual deficit; empty/single/first targets; zero/negative targets;
INT64 extremes; current-versus-captured corrections; exact 1/3; malformed order/
prefix and resource refusal. Local reference/count/owned-value limits are explicit,
not arbitrary lower score bounds. Concrete C++ types, exact field walks/errors and
scratch bounds remain the NEXT separately reviewed API stage.

## Actual checks

- `python3 tools/check_project.py` — PASS: 396 task records, 122 decisions, local
  links, presets and current HTML in this claimed branch. Its draft-test notice
  does not establish new milestone test coverage.
- `python3 tools/progress.py render --check` — PASS, current snapshot.
- `git diff --check` — PASS; staged new-file whitespace/owned-path check accompanies
  the commit.
- An inline Python command imported only stdlib `fractions.Fraction` and checked
  ten independent document arithmetic facts: signed-extreme distance, absolute
  INT64_MIN magnitude, the resulting exact interval ratio, deficit 200, remaining
  1200/2200, fractions 1/2 and 1/10, signed-target 1/2, and exact 1/3. Exit 0. It imported
  no project module, wrote no fixture/test and exercised no feature implementation.
  These are document arithmetic checks, not a milestone baseline/acceptance run.

No API/header/source/stub/schema/storage, fixture, test or executable product
behavior was authored/invoked. No provider, writer, action delivery or live user
data was accessed. Different actual logical review is next; this author supplies
no self-approval. MILESTONETEST-001 remains incomplete and MILESTONE-001 remains
planned until their separately reviewed API/test/red/implementation gates. Manual
completion/reset, audited edits, action groups/presentation/holds, post-finish
applicability, integration/CI and platform verification retain their own work.
