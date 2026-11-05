# CONTRACTPLAN-003 — Early end-to-end reference path

Kind: planning/contract. State/owner/dependencies: progress/state.json.
Authority: specification v0.45 §17, §§2–4/6/10/18, API-01/02/03, NUM-04.

Inspect API-008's complete prerequisite closure. Replace presentation and complete
transport/planner/recovery prerequisites only with reviewed tasks supplying the
actual needed guarantees. [ADR-0006](../decisions/0006-reference-path-prerequisites.md)
records the exact edge changes and preserved requirements.

Author and independently review the bounded
[reference profile proposal](../contracts/reference-path-v1.md), including literal
success/duplicate/rollback/unknown/authorization expectations. It is a logical
contract; CONTRACT-013/014 still finalize concrete schemas and interfaces before
executable tests. No endpoint or runtime capability is implemented by this plan.

Root owns ADR/profile/briefs/navigation/state/HTML and publication. Independent
reviewer owns only a new planning/profile review note on its own branch/worktree.
No production/source/test/assertion/schema/migration/build/pin/CI edits here.
Validate graph, exact edge replacements, retained baseline gates, complete deferred
scope, links and rendered progress. Record commands, review findings and blockers.
Existing planning/component CI suffices for these document-only changes; latest
head checks, rules, discussions and normal PR merge remain mandatory.
