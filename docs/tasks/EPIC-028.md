# EPIC-028 — Simplify agent workflow and focus test coverage

Module: Quality and tooling. Kind: epic. Specification: v0.45; §§17–18.
Authoritative status, owners and dependencies: `progress/state.json`.

This epic makes the repository easier for future agents to work in by reducing
duplicated process instructions and removing test machinery that does not establish
specified product behavior. It does not reduce release scope or authorize unreviewed
assertion deletion. Every child is a cleanup task with a separate approved review.

| Task | Bounded outcome |
| --- | --- |
| [TESTCLEAN-001](EPIC-028/TESTCLEAN-001.md) | Replace schema-validation harness tests with real schema/example checks |
| [TESTCLEAN-002](EPIC-028/TESTCLEAN-002.md) | Remove library/fixture environment foundation programs |
| [TESTCLEAN-003](EPIC-028/TESTCLEAN-003.md) | Simplify CMake test registration and labels |
| [TESTCLEAN-004](EPIC-028/TESTCLEAN-004.md) | Share Qt test helpers and deduplicate writer plumbing cases |
| [TESTCLEAN-005](EPIC-028/TESTCLEAN-005.md) | Remove repeated implementation-detail assertions |
| [TESTCLEAN-006](EPIC-028/TESTCLEAN-006.md) | Replace giant unreproducible fixtures with named scenarios |
| [TESTCLEAN-007](EPIC-028/TESTCLEAN-007.md) | Trim libsodium call-counting while retaining security assertions |
| [TESTCLEAN-008](EPIC-028/TESTCLEAN-008.md) | Merge regression executables and simplify labels/comments |

TESTCLEAN-001 and TESTCLEAN-002 are initially dependency-ready. Remaining order and
shared-file ownership are in `progress/state.json`; avoid concurrent edits to test
registration, `progress/state.json`, migration data or common fixtures without the
integrator assigning ownership.

## Preserved scope and follow-up candidates

All seven integrations, all three exchange-rate providers and every v0.45 product
requirement remain in scope. This epic is test/process cleanup, not product completion.
The test review identified valuable behavior coverage that should be scheduled as
separate reviewed test-first tasks when dependencies permit:

- contribution → captured plan → application bundle → timer/score effect, including a
  capped/refund case and restart between steps;
- process-kill crash/reopen atomicity, populated-database migration, corrupted database
  refusal, disk-full failure, and readers observing only committed state;
- authentication expiry/revocation/disabled-account/permission behavior using a fake
  clock, plus password failure handling;
- exact conservation properties and, as features land, fake-adapter integration/FX,
  Qt Quick, browser and OBS acceptance.

These are candidates, not completed tests or permission to replace any existing scope.
An epic is complete only after every child and explicitly retained dependency is done,
all required PR/review gates pass, progress is regenerated, and closure is recorded.
