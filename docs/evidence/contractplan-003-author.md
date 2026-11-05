# CONTRACTPLAN-003 — actual author/check evidence

Author/integrator: /root. Date: 2026-11-05. Branch:
`task/CONTRACTPLAN-003-reference-path`, isolated worktree based on freshly fetched
`origin/main` 436ed9afb1ac3d3be68f4be8c2b1bdc62557e574. The pre-existing milestone
checkout and CONTRACT011 worktree were preserved. Candidate230f3b8 + formatting
correction2c00a1d are documentation/state only. Root owns shared state/HTML,
ADR0006/profile/briefs/navigation and publication; different reference_review owns
only its actual independent evidence note on its own branch/worktree.

## Scope and guarantee audit

Original early chain: API008/APITEST008 → API003 → API002 → HTTP006 → scene008 →
bindings010/schema012 → milestone/modifier presentation baselines. Also broad
connector005, planner RULES003 and checkpoint/recovery STORAGE003. The new chain
substitutes concrete reviewed contract/baseline/implementation tasks for bounded
real HTTP, single-rule evaluation, synthetic intake and current application
adapters/operation-time atomic timer state. Real bridge, session/access/delegation
remain prerequisites; no new identity or network permission shortcut is proposed.
See [ADR0006](../decisions/0006-reference-path-prerequisites.md) and
[logical profile](../contracts/reference-path-v1.md) for complete oracles/scope.

Actual stdlib graph comparison of origin/main and candidate2c00a1d:

- 408→417 records; 1508→1590 edges. Exactly8 removed: API008 and APITEST008 each
  replace API003/API005/RULES003/STORAGE003. Exactly90 added, including all4 deferred
  edges to BOTH advanced009 records, actual replacement guarantees and new gates.
- Only8 old task records differ: CONTRACT003/006, EPIC003/008, APITEST008/API008,
  APITEST009/API009. No unrelated task or existing implementation→baseline edge
  changed. All old IDs, global verification values and summary are identical.
- No source, tests/assertions, fixtures, schemas, migration, build, dependency pin,
  CI or specification file changed. HTML reorder follows the actual graph update.
- New records remain planned except the claimed planning task. No feature or test
  authoring baseline is marked ready/reviewed/red/implemented by decomposition.

Profile revision1 hand-derived examples RP01–14 cover capped120000/request600000,
new-key/concurrent duplicates, rollback with earlier durable intake, unknown/no
replay, acknowledged COMMIT then body exception, current request-sized authority,
changed cap, zero actual, immutable rule capture and unsupported/unknown policy.
These are DOCUMENT oracles, not executable acceptance tests or observed behavior.
Current missing runtime session001–003/access002–003/bridge001/wire/rule/current
adapters mean APITEST008 is not yet dependency-ready. Concrete CONTRACT013/014
schemas/signatures must themselves be authored/reviewed; the logical profile is
not their completion. No change of product requirement is proposed.

## Actual checks

Linux x86-64 host, GNU16.2.1, Python3.14.9, CMake4.4.5; all commands below ran in
the isolated worktree after the authored candidate. No live operations occurred.

```sh
python3 tools/progress.py render
python3 tools/check_project.py
python3 -m unittest discover -s tests/tooling -v
python3 tools/progress.py render --check
git diff --check
cmake --preset scaffold-debug
cmake --build --preset scaffold-debug
ctest --preset scaffold-debug
```

PASS project417tasks/122decisions/links/presets/currentHTML; PASS all19 tooling
methods; PASS whitespace; configure/build exit0 (6 steps, zero warnings); CTest
3/3 PASS0.55s. These are consistency/tooling/scaffold checks, not the missing
end-to-end product baseline. No product Qt/HTTP acceptance, feature red, schema
validator, duplicate/rollback runtime demonstration or platform/provider check ran.
Existing historical component evidence remains unchanged and is not repeated.

CI assessment: document/dependency changes use existing consistency/scaffold and
component workflows; no new executable or workflow justifies a CI edit here.
Concrete schema/runtime/test additions must assess/update CI in their owning tasks.
Different actual planning/profile review, latest publication-head CI, current
GitHub rules/discussions and normal PR merge remain gates; do not infer those
from these local results. Full deferred scope is explicitly mapped in ADR0006.
