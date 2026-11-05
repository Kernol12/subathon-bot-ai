# Handoff — CONTRACTPLAN-004 session supplier gate repair

Coordinator /root,5 November2026, dedicated task branch from main41c4842.
Document/graph authoring complete; independent review/publication gates pending.

## Problem and change

Actual CONTRACT013 brief/reference profile and wire primitives explicitly require
missing SESSION003 contract details. Existing password hashing and effective
permission calculations supply no session resolver or cookie/CSRF/header transport.
SESSION002/003 briefs explicitly have no finalized contract. Graph-ready therefore
did not establish finalization-ready. The read-only investigator capture_codec_tests
confirmed current files/headers and these exact requirements, with no feature calls.

[ADR-0007](../decisions/0007-session-contract-prerequisite.md) adds DOCUMENT-only
[SESSIONCONTRACT001](../tasks/EPIC-007/SESSIONCONTRACT-001.md) before CONTRACT013
and SESSIONTEST002/003. Existing runtime/test-first gates and every release
requirement remain. No exact auth protocol choice or runtime/schema/API/test is
finalized here. No implementation capability follows from the new task.

Compared with actual main41c, graph delta is exactly two records/eleven edges:
new plan004→{CONTRACT005,SESSION001,STORAGE002}; new document001→{plan004,
CONTRACT005,SESSION001,STORAGE002}; four new edges from CONTRACT013,
SESSIONTEST002,SESSIONTEST003,EPIC007 to document001. Every old420ID/1608edge
and old task status is preserved; plan004 in_progress and document001 planned.
Result422 records/1619edges. This avoids a document→runtime→baseline cycle.
API010/APITEST010 retain actual SESSION003 and all old prerequisites.

Root owns ADR/briefs/navigation/evidence/state/HTML; separate reviewer owns only
new review evidence. No production/interface/schema/assertion/migration/pin/CI
changes. Concurrent SQL018 claim d73 and its source worktree remain preserved;
when this plan integrates there, retain its own active row/activity rather than
copying the main snapshot over that claim.

## Actual checks and remaining gates

`python3 tools/progress.py render`, `python3 tools/check_project.py`,
`python3 tools/progress.py render --check`, `git diff --check`: PASS,422/122.
Exact old-ID/edge/status audit and19 tooling checks recorded before publication.
Documentation/graph checks do not establish session authentication or red tests.
Existing consistency and stable component CI adequately cover this plan; no new
executable suite, manual body input, dependency or required-check setup is needed.
Independent actual document/graph review, latest-head CI/current rules/discussions
and normal merge remain. FutureSESSIONCONTRACT001 requires its own authored
contract/oracles/different review; SESSIONTEST002/003 retain test-first gates.

Changed paths: ADR0007/index, taskplan004/newSESSIONCONTRACT001, three affected
child briefs/epic navigation, state/HTML and this evidence. All source/tests and
wire/schema packages remain unchanged. No live data or credentials used.
