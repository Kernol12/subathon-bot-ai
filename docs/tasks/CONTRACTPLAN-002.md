# CONTRACTPLAN-002 — Split declarative scene contract prerequisites

Kind: contract/planning. Module: Server/API.
Status, owner and dependencies: progress/state.json.
Authority: specification v0.45 §11, API-01/03, UI-01/02/03/04,
MEDIA-01/02/03/04/05/06 and scoped read-only VIEW-04 boundaries.

## Goal and acceptance

Refine CONTRACT-008 into focused scene-model, binding/timeline, asset/package and
concrete schema contract tasks before schema authoring. Keep CONTRACT-008 as the
complete integration gate, preserving HTTP006 → 008, parent003 → 008 and all
SCENE/RENDER/epic prerequisites and paired test-first baselines. No child depends
on its parent or a downstream implementation that creates a cycle.

Record exact new dependencies, independently review the actual decision/brief/state
revision and retain every §11 requirement. Distinguish logical models, complete
machine schemas, asset validation, permission enforcement, separate renderers,
conformance, performance and actual OBS/platform evidence. No schema, renderer or
supported codec is finalized by this plan. Owning-domain prerequisites must be
explicit before a binding contract claims a concrete domain projection.

## Ownership and checks

Root owns state/HTML, this claim, integration/navigation/maps, schemas/imports,
source/build/CI/pins and publication. Delegated document author owns only the new
ADR0005, CONTRACT009–012 briefs, bounded child/integration section of CONTRACT008
and an author evidence note, in a separate branch/worktree. Different actual review
precedes completion. No production/test/assertion/dependency or schema edit here.

Check project graph/links/current HTML and whitespace; review every prior unrelated
edge and complete requirement map. Existing consistency/component CI covers this
planning change; all applicable final-head checks/rules/threads still gate merge.
Record exact commands/environment/reviewer and remaining release requirements.
