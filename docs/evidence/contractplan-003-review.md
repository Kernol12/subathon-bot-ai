# Independent review — CONTRACTPLAN-003

Author: coordinator `/root`. Reviewer: separate agent/session
`/root/reference_review`, isolated branch
`task/CONTRACTPLAN-003-independent-review`. Date: 2026-11-05.
Candidate: `2c00a1d3b127f33a491e3bdbb65386ff824ac38d`, compared with
`origin/main` (`436ed9afb1ac3d3be68f4be8c2b1bdc62557e574`).

Verdict: **approved for planning/logical-profile integration only**. No requested
correction at this stage. This approves neither an unwritten OpenAPI/schema or
C++ interface nor any test baseline, implementation or available capability.

## Actual artifacts and review

Reviewed all 18 changed paths, including ADR-0006, reference-path-v1 revision 1,
new contract/task briefs, navigation and actual progress edge mapping. SHA-256:

- ADR-0006: `e2b7665efce07c8787324725eaa104426bc6c49d21a3741e5b3341c303fed263`.
- reference-path-v1: `5665e5ea441e6a0bad405e3ad224d7bc61c09dd9a3cc7ee55d665b84fc96d3c3`.
- candidate state.json: `1e1fc1e30b03caa81788c2d4a29ec051832cf980c328436970f2ffbc91c0a501`.

Specification v0.45 §17 requires an actual complete synthetic contribution path
without reducing release scope; §§2–4/6/10/18 and API-01/02/03 supply thread,
accounting, permission and API constraints. The candidate replaces complete scene/
WS/planner/recovery prerequisites with bounded tasks that must supply their actual
required guarantees. Real HTTP and Qt SQL remain mandatory; a local JSON helper
does not satisfy API-008. Complete HTTP006/CONTRACT003 retain their previous scope
and require the reference contract as an additional integration import.

The profile explicitly refuses unsupported configuration, requires known empty
modifier/milestone/action sets rather than silently dropping them, and records
Active/no-pause/no-debt/no-deadline/no-recovery as checked fixture facts. It preserves
immutable source/arrival/rule/action provenance, derives the action from a retained
rule rather than accepting a caller award, and checks current full-request
delegation/permission before execution or historical disclosure. Session and access
implementations remain actual prerequisites; loopback/synthetic mode and capability
advertisement grant no authority.

The reference API has only negotiation and a protected committed timer snapshot;
no unfinished mutation/login/scene/WS/provider routes or generic capability claims.
Exact signed64 strings, opaque state tokens, required/null fields, closed objects
and bounded control/projection data are explicit. Concrete HTTP/auth/framing,
resource-preallocation/error precedence and schema validation remain CONTRACT-013;
typed rule/current-adapter contracts and their limits remain CONTRACT-014. Their
absence is not misrepresented as reviewed schema/interface acceptance.

Atomic application retains claim/status, requested/actual/capped effects, timer,
result, audit and required jobs in one controlled writer transaction after durable
intake/capture. Known rollback, commit-unknown, post-COMMIT body failure and later
publication failure are distinct. No provisional session state may become a
snapshot or success; a fresh authorized read may show the last committed state
after failure. Duplicate source claims survive new operation IDs/concurrency and
require current disclosure authority. Missing policy/actions cannot become fake
successful ports. Recovery is explicitly unavailable until its owner establishes
readiness; raw SQLite reopen is not restart verification.

## Independent checks actually run

- `python3 tools/check_project.py`: PASS, 417 task records, 122 decisions, local
  links, presets and current HTML. Its draft-test inventory is not product evidence.
- `python3 tools/progress.py render --check`: PASS, current generated snapshot.
- `python3 -m unittest discover -s tests/tooling -v`: PASS, all 19 tests.
- `git diff --check`: PASS.
- Independent stdlib JSON/set audit against published main: 408 → 417 tasks;
  1508 → 1590 edges, exactly 8 removals and 90 additions. The only removed edges
  are API-003/API-005/RULES-003/STORAGE-003 from each of API-008/APITEST-008.
  Every original ID/status and every original implementation-to-test-baseline edge
  remains. Both API-009/APITEST-009 retain all original edges and gain those four
  deferred prerequisites. CONTRACT008/010/011/012 records remain exactly unchanged.
- Independent integer/Fraction arithmetic: RP01 capacity120000/capped480000,
  RP09 capacity60000/capped540000 and RP10 actual0/capped600000 are correct for
  the literal Main/request/cap inputs; no production calculation was used.
- Read all RP01–14 rows and the literal RP01 JSON against the logical invariants,
  including rollback/unknown/body-failure, no new award on retry, revoked/limited
  authority, captured r1 despite later r2, and unsupported-feature refusal.

No feature tests, baseline red, server, HTTP, database, provider or platform checks
were run in this document review. Existing tooling/component CI is appropriate
for this planning-only diff; latest-head CI and PR rules still govern merge.

## Blocked behavior and remaining gates

API-008/APITEST-008 honestly remain planned and dependency-blocked. Production
API-001 bridge, SESSION-001/002/003, ACCESS-002/003, actual rule evaluator/current
application adapters and concrete reference HTTP/schema are unavailable at this
candidate. These are necessary implementation prerequisites, not unanswered owner
permission questions or review waivers. CONTRACT-013/014 can proceed to bounded
contract authoring next, recording missing auth/delegation formats explicitly.

Each replacement baseline still needs actual contract/interface review, independent
test review, a valid intended red after successful compilation, implementation,
separate code review and passing relevant acceptance/CI. Only after accepted
prerequisites may APITEST-008 establish its real HTTP/SQL end-to-end red baseline;
no prior component pass or these logical examples can substitute for it.

Persistent WS, complete rule categories, modifiers, milestones/presentation,
checkpoint cadence/recovery/backups, complete API/web assets, both renderers, all
seven integrations/three FX providers, localization/themes and platform/package/
live/security gates remain mandatory. This planning approval certifies no such
behavior and authorizes no test weakening, skipped acceptance or premature merge.

## Bounded final metadata audit

Separately inspected root `57b453b691fb72806a2223c46238fdd591a398f9` against the
approved candidate `2c00a1d`. Approved the actual author evidence, imported review,
handoff and planning-task completion metadata only. All dependency arrays, the
logical profile/ADR, all other task records and global verification are identical.
Only CONTRACTPLAN-003 is marked done; its review points to the actual approval and
selected_ci remains explicitly not_run. No absent schema, feature, test/red,
HTTP/runtime duplicate/rollback demonstration, future CI or merge is claimed.

Independent exact stdlib equality/graph checks, `python3 tools/check_project.py`,
`python3 tools/progress.py render --check` and `git diff --check 2c00a1d..57b453b`
all PASS on that root revision. This audit does not rerun or certify the root's
recorded scaffold3/19tooling results, and does not approve future publication/CI,
GitHub protection/discussion state or PR merge. Those remain integrator gates.
