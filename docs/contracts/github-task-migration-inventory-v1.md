# GitHub migration affected-consumer inventory v1

Part of MIGRATE-001 contract packet; authored, not reviewed. Discovery uses the actual
tracked tree based on `4153a7ab2b3245e36fd5450b8897d2df659a53ee`, with the restored
PR126 metadata retained. No fixed task/brief/test count defines completeness.

## Dispositions and ownership

| Consumers | Actual dependency and required disposition | Stage/owner |
| --- | --- | --- |
| progress/state.json, progress/state.schema.json, PROGRESS.html | Current canonical tasks/proof/policy and generated snapshot; archive final bytes, extract proof once, retire at single cutover | 004 coordinator |
| tools/progress.py | State loading, semantic task validation, graph/availability, context, set/render/reconcile CLI and HTML generation; retain proof validate/context via tools/proofs.py, retire planning/controller commands | 002 proof author; 004 coordinator integration |
| tools/workflow.py | Existing stage grammar, frozen policy, dependency/publication claims, context metrics and safe output helpers; reuse grammar once with tree-free historical validation and separate full-history selected-source binding; replace closure with explicit --complete, retire local dependency_complete; preserve history and invariants | 002 fresh implementer |
| tools/progress_template.html, tools/check_progress_ui.mjs | Inline controller/UI and offline DOM simulation; retire only after approved assertion mapping | 004 separate cleanup review |
| tools/check_publication.py | Imports progress/workflow, selected source binding and actual progress-schema witness; adapt input/schema driver while preserving checkout/history/privacy/admitted validation | 003 publication author; 004 activation |
| tools/check_project.py | Coupled tracker/spec refs/HTML checks plus independent acceptance-map, evidence, presets and Markdown links; separate these, retain independent checks | 003 project author; 004 activation |
| tests/tooling/test_progress.py, test_workflow.py, test_compact_context.py, test_publication.py, test_check_project.py | Existing graph/render/context/review/publication/project assertions and fixture state imports; reuse focused meaningful assertions, including compact stage handoffs; ledger/UI-only removal requires exact separate review | 002/003 tests authors, 004 cleanup reviewer |
| tests/support/publication_fixture.py | Fresh committed working-tree copies, synthetic state mutation and publication setup; prepare proof fixtures without changing checkout/source or privacy assertions | 003 |
| tests/schema_validation/test_project_schemas.py, schema_cases.py, test_publication_integration.py | Actual state/editor-schema cases and fixed one-test witness, real admitted SDK, valid/invalid/empty-interpreter scenarios; replace tracker-specific cases with actual proof/policy validation | 002/003; 004 activation |
| tools/schema_validation/, docs/schema-validation.md, docs/contracts/schema-validation-tool-v1.md, schemas/README.md | Isolated pinned standards-validator admission and documentation; retain admission/OpenAPI/examples, update only progress-specific contract | 003/004 |
| CMakeLists.txt | progress renderer target and check-project; retire only renderer, preserve independent project target | 003 prepare; 004 retire |
| tests/CMakeLists.txt, tests/schema_validation/CMakeLists.txt | scaffold.python_tools/project_consistency and schema.publication registration; retain meaningful suites/labels and real witness integration | 003/004 |
| CMakePresets.json, cmake/AcceptanceSuites.cmake, cmake/AcceptanceRegistration.cmake | Indirect suite selection/labels, schema/scaffold runner closure; inspect, preserve current product registrations/semantics | 003; 005 audit |
| .github/workflows/ (enumerated below) | Scaffold/tooling/repository checks run directly or through common test closure; schema jobs run actual progress validation/publication. Preserve all app-bound required checks, job names, pins, permissions, lane coverage | 003/004 coordinator; 005 live audit |
| AGENTS.md, CLAUDE.md, docs/AGENT_WORKFLOW.md, docs/COORDINATOR_PROMPT.md, progress/coordinator-checkpoint.md, .github/pull_request_template.md | Authority, startup, claims/context, structured receipts, publication/reconcile and status instructions; switch together, retain stops and review cycle | 004 coordinator; checkpoint 001 authorized |
| README.md, docs/PROGRESS.md, docs/ARCHITECTURE.md, docs/TEST_STRATEGY.md, docs/CLION_SETUP.md, docs/CI.md | Dashboard/task navigation, commands and honest evidence limits; retire obsolete instructions, document bounded issue/brief/proof retrieval | 004 |
| scaffold/main.cpp | Printed startup instruction opens PROGRESS.html; replace navigation at cutover (no product behavior) | 004 |
| BASELINE_MANIFEST.json, docs/decisions/0001-baseline.md | Historical path/digest manifest and baseline provenance; preserve original snapshot semantics, pin historical paths rather than regenerate as a second tracker | 004 reconciliation |
| tests/acceptance-map.json, docs/evidence/README.md, tests/fixtures/README.md, dependency/module READMEs | Evidence and task/brief links; preserve all decision/scenario coverage, historical names, limitations and required artifacts | 004 links; 006 per-brief links; 005 audit |
| docs/contracts/context-publication-v1.md, bounded-coordination-v2.md, docs/tasks/WORKFLOW-003.md | Current normative record/source/publication policy; retain immutable prior contract/review links, explicitly supersede storage assumptions without activating WF006 policy | 004 |
| docs/tasks/WORKFLOW-006.md, WORKFLOW-007.md | Future manifest/results storage/runner dependency; rescope WF006 integration/dependency only, retain WF007 prerequisite and owner stops | 004 coordinator |
| Every task brief and docs/tasks/README.md (enumerated below) | Tracker fields, shared technical requirements, inbound links and immutable review context; accessible through cutover, then per-consumer active/shared/historical classification and coverage mapping | 006; 005 verifies |
| GitHub issues/Projects/native relationships/PRs/ruleset | External planning mirrors now, authority after cutover; preserve board-only items, configure closure, verify actual merged PRs/checks/threads without ordinary CI API dependency | 004/005 coordinator |

The exact lexical-hit list below also includes historical ADRs, provider/module and
dependency READMEs, contracts and code consumers. A listed historical reference may
remain pinned; it is not blanket permission to rewrite archival prose. Paths with no
lexical hit can still be transitive consumers (CMake/CTest/CI/acceptance-map): those are
explicitly covered above. Never equate a grep result with evidence of safe deletion.

## Reproduction and completeness

Run `git ls-files` and `rg -n --hidden` over tracked text for `progress/state`,
`tools/progress`, `PROGRESS.html`, `state.schema`, `workflow_policy`, `tools/workflow`,
`check_publication`, `docs/tasks/`, relative `tasks/` links and WORKFLOW-003–007.
Inspect imports, CMake/CTest registrations, CI invocations, PR template, instructions,
all brief links, state brief/evidence fields and acceptance-map evidence/test fields.
Separately enumerate every `docs/tasks/*.md` descendant: shared briefs cannot be
inferred from unique task IDs. Use the frozen state to map all tasks sharing each
brief, then collect inbound Markdown/GitHub/evidence/review references before removal.
Re-run against current cutover HEAD and remote inventory; record additions/removals
and dispositions in the one-time report. No unlisted/new consumer is implicitly exempt.
Local raw search and assertion inventories are private in `local-evidence/EPIC-029`.

## Direct tracked text hits outside task briefs

Each exact path requires disposition under the table above (retain, adapt, pin or retire).

```text
.github/pull_request_template.md
AGENTS.md
BASELINE_MANIFEST.json
CLAUDE.md
CMakeLists.txt
README.md
dependencies/native-websocket-profile/README.md
deploy/README.md
desktop/README.md
docs/AGENT_WORKFLOW.md
docs/ARCHITECTURE.md
docs/CLION_SETUP.md
docs/COORDINATOR_PROMPT.md
docs/PROGRESS.md
docs/TEST_STRATEGY.md
docs/contracts/account-ingress-v1.md
docs/contracts/api-host-lifecycle-boundary-v1.md
docs/contracts/api-host-listeners-v1.md
docs/contracts/bounded-coordination-v2.md
docs/contracts/ci-noble-security-source-v1.md
docs/contracts/ci-setup-gates-v1.md
docs/contracts/context-publication-v1.md
docs/contracts/effective-permissions-v1.md
docs/contracts/exact-retained-valuations-v1.md
docs/contracts/exact-valuation-storage-v1.md
docs/contracts/fixture-generation-v1.md
docs/contracts/host-auth-continuations-v1.md
docs/contracts/local-login-authority-v1.md
docs/contracts/migration-catalog-v1.md
docs/contracts/milestone-progression-api-v1.md
docs/contracts/milestone-progression-v1.md
docs/contracts/password-hashing-v1.md
docs/contracts/rate-snapshots-api-v1.md
docs/contracts/rate-snapshots-v1.md
docs/contracts/retained-context-v1.md
docs/contracts/scene-assets-packages-v1.md
docs/contracts/scene-document-v1.md
docs/contracts/schema-validation-tool-v1.md
docs/contracts/session-lifecycle-v1.md
docs/contracts/session-resolution-v1.md
docs/contracts/session-transport-v1.md
docs/contracts/sqlite-application-plans-v1.md
docs/contracts/sqlite-contributions-v1.md
docs/contracts/sqlite-receipts-v1.md
docs/contracts/timer-editorial-commands-v1.md
docs/contracts/translation-catalog-v1.md
docs/contracts/wire-primitives-v1.md
docs/decisions/0001-baseline.md
docs/decisions/0004-wire-contract-prerequisites.md
docs/decisions/0005-scene-contract-prerequisites.md
docs/decisions/0006-reference-path-prerequisites.md
docs/decisions/0010-session-executable-prerequisites.md
docs/decisions/README.md
docs/evidence/README.md
docs/schema-validation.md
integrations/README.md
progress/coordinator-checkpoint.md
progress/state.schema.json
scaffold/main.cpp
schemas/README.md
server/README.md
tests/fixtures/README.md
tests/schema_validation/schema_cases.py
tests/schema_validation/test_project_schemas.py
tests/schema_validation/test_publication_integration.py
tests/support/publication_fixture.py
tests/tooling/test_check_project.py
tests/tooling/test_compact_context.py
tests/tooling/test_progress.py
tests/tooling/test_publication.py
tests/tooling/test_workflow.py
tools/check_progress_ui.mjs
tools/check_project.py
tools/check_publication.py
tools/progress.py
tools/progress_template.html
tools/workflow.py
web/README.md
```

## CI workflow closure

Each workflow is inspected for direct checks and inherited scaffold/tooling registrations.

```text
.github/workflows/acceptance.yml
.github/workflows/api-host-baseline.yml
.github/workflows/application-plan.yml
.github/workflows/contribution-evidence.yml
.github/workflows/effective-permissions.yml
.github/workflows/exact-numeric.yml
.github/workflows/milestone-progression.yml
.github/workflows/native-websocket-profile.yml
.github/workflows/password-hashing.yml
.github/workflows/pause-policy.yml
.github/workflows/rate-snapshots.yml
.github/workflows/receipt-intake.yml
.github/workflows/scaffold.yml
.github/workflows/schema-validation.yml
.github/workflows/sqlite-writer.yml
.github/workflows/time-debt.yml
.github/workflows/timer-editorial.yml
.github/workflows/timer-lifecycle.yml
.github/workflows/translation-catalog.yml
```

## Complete tracked task-brief file universe before registration

These paths are the 006 classification input, not a determination that any is removable.
Every task sharing a path and every inbound consumer must be mapped at cleanup. The
seven new EPIC-029 briefs registered by 001 join this universe at cutover.

```text
docs/tasks/ACCTCONTRACT-001.md
docs/tasks/BOOT-001.md
docs/tasks/BOOT-002.md
docs/tasks/BOOT-003.md
docs/tasks/BOOT-004.md
docs/tasks/BOOT-005.md
docs/tasks/BOOT-006.md
docs/tasks/BOOT-007.md
docs/tasks/BOOT-008.md
docs/tasks/BOOT-009.md
docs/tasks/CI-001.md
docs/tasks/CI-002.md
docs/tasks/CI-003.md
docs/tasks/CONTRACT-001.md
docs/tasks/CONTRACT-002.md
docs/tasks/CONTRACT-003.md
docs/tasks/CONTRACT-004.md
docs/tasks/CONTRACT-005.md
docs/tasks/CONTRACT-006.md
docs/tasks/CONTRACT-007.md
docs/tasks/CONTRACT-008.md
docs/tasks/CONTRACT-009.md
docs/tasks/CONTRACT-010.md
docs/tasks/CONTRACT-011.md
docs/tasks/CONTRACT-012.md
docs/tasks/CONTRACT-013.md
docs/tasks/CONTRACT-014.md
docs/tasks/CONTRACTPLAN-001.md
docs/tasks/CONTRACTPLAN-002.md
docs/tasks/CONTRACTPLAN-003.md
docs/tasks/CONTRACTPLAN-004.md
docs/tasks/CONTRACTPLAN-005.md
docs/tasks/CONTRACTPLAN-006.md
docs/tasks/DECISION_OWNERS.md
docs/tasks/EPIC-001.md
docs/tasks/EPIC-001/CORE-001.md
docs/tasks/EPIC-001/CORE-002.md
docs/tasks/EPIC-001/CORE-003.md
docs/tasks/EPIC-001/CORE-004.md
docs/tasks/EPIC-001/CORE-005.md
docs/tasks/EPIC-001/CORE-006.md
docs/tasks/EPIC-001/CORE-007.md
docs/tasks/EPIC-001/CORE-008.md
docs/tasks/EPIC-002.md
docs/tasks/EPIC-002/ACCT-001.md
docs/tasks/EPIC-002/ACCT-002.md
docs/tasks/EPIC-002/ACCT-003.md
docs/tasks/EPIC-002/ACCT-004.md
docs/tasks/EPIC-002/ACCT-005.md
docs/tasks/EPIC-002/ACCT-006.md
docs/tasks/EPIC-002/ACCT-007.md
docs/tasks/EPIC-003.md
docs/tasks/EPIC-003/RULES-001.md
docs/tasks/EPIC-003/RULES-002.md
docs/tasks/EPIC-003/RULES-003.md
docs/tasks/EPIC-003/RULES-004.md
docs/tasks/EPIC-003/RULES-005.md
docs/tasks/EPIC-003/RULES-006.md
docs/tasks/EPIC-004.md
docs/tasks/EPIC-004/MODIFIER-001.md
docs/tasks/EPIC-004/MODIFIER-002.md
docs/tasks/EPIC-004/MODIFIER-003.md
docs/tasks/EPIC-004/MODIFIER-004.md
docs/tasks/EPIC-004/MODIFIER-005.md
docs/tasks/EPIC-004/MODIFIER-006.md
docs/tasks/EPIC-005.md
docs/tasks/EPIC-005/MILESTONE-001.md
docs/tasks/EPIC-005/MILESTONE-002.md
docs/tasks/EPIC-005/MILESTONE-003.md
docs/tasks/EPIC-005/MILESTONE-004.md
docs/tasks/EPIC-005/MILESTONE-005.md
docs/tasks/EPIC-005/MILESTONE-006.md
docs/tasks/EPIC-005/MILESTONE-007.md
docs/tasks/EPIC-005/MILESTONE-008.md
docs/tasks/EPIC-006.md
docs/tasks/EPIC-006/ACCESS-001.md
docs/tasks/EPIC-006/ACCESS-002.md
docs/tasks/EPIC-006/ACCESS-003.md
docs/tasks/EPIC-006/ACCESS-004.md
docs/tasks/EPIC-006/ACCESS-005.md
docs/tasks/EPIC-006/ACCESS-006.md
docs/tasks/EPIC-006/ACCESS-007.md
docs/tasks/EPIC-006/ACCESS-008.md
docs/tasks/EPIC-006/ACCESSCONTRACT-001.md
docs/tasks/EPIC-007.md
docs/tasks/EPIC-007/SESSION-001.md
docs/tasks/EPIC-007/SESSION-002.md
docs/tasks/EPIC-007/SESSION-003.md
docs/tasks/EPIC-007/SESSION-004.md
docs/tasks/EPIC-007/SESSION-005.md
docs/tasks/EPIC-007/SESSION-006.md
docs/tasks/EPIC-007/SESSION-007.md
docs/tasks/EPIC-007/SESSION-008.md
docs/tasks/EPIC-007/SESSION-009.md
docs/tasks/EPIC-007/SESSION-010.md
docs/tasks/EPIC-007/SESSION-011.md
docs/tasks/EPIC-007/SESSION-012.md
docs/tasks/EPIC-007/SESSIONCONTRACT-001.md
docs/tasks/EPIC-007/SESSIONCONTRACT-002.md
docs/tasks/EPIC-007/SESSIONCONTRACT-003.md
docs/tasks/EPIC-007/SESSIONCONTRACT-004.md
docs/tasks/EPIC-007/SESSIONCONTRACT-005.md
docs/tasks/EPIC-007/SESSIONCONTRACT-006.md
docs/tasks/EPIC-007/SESSIONCONTRACT-007.md
docs/tasks/EPIC-008.md
docs/tasks/EPIC-008/API-001.md
docs/tasks/EPIC-008/API-002.md
docs/tasks/EPIC-008/API-003.md
docs/tasks/EPIC-008/API-004.md
docs/tasks/EPIC-008/API-005.md
docs/tasks/EPIC-008/API-006.md
docs/tasks/EPIC-008/API-007.md
docs/tasks/EPIC-008/API-008.md
docs/tasks/EPIC-008/API-009.md
docs/tasks/EPIC-008/API-010.md
docs/tasks/EPIC-008/API-011.md
docs/tasks/EPIC-008/API-012.md
docs/tasks/EPIC-008/API-013.md
docs/tasks/EPIC-008/API-014.md
docs/tasks/EPIC-008/API-016.md
docs/tasks/EPIC-008/APICONTRACT-001.md
docs/tasks/EPIC-008/APICONTRACT-002.md
docs/tasks/EPIC-008/APICONTRACT-003.md
docs/tasks/EPIC-008/APICONTRACT-004.md
docs/tasks/EPIC-008/APICONTRACT-005.md
docs/tasks/EPIC-008/APITEST-015.md
docs/tasks/EPIC-009.md
docs/tasks/EPIC-009/STORAGE-001.md
docs/tasks/EPIC-009/STORAGE-002.md
docs/tasks/EPIC-009/STORAGE-003.md
docs/tasks/EPIC-009/STORAGE-004.md
docs/tasks/EPIC-009/STORAGE-005.md
docs/tasks/EPIC-009/STORAGE-006.md
docs/tasks/EPIC-009/STORAGE-007.md
docs/tasks/EPIC-009/STORAGE-008.md
docs/tasks/EPIC-009/STORAGE-009.md
docs/tasks/EPIC-009/STORAGE-010.md
docs/tasks/EPIC-009/STORAGE-011.md
docs/tasks/EPIC-009/STORAGE-012.md
docs/tasks/EPIC-009/STORAGE-013.md
docs/tasks/EPIC-009/STORAGE-014.md
docs/tasks/EPIC-009/STORAGE-015.md
docs/tasks/EPIC-009/STORAGE-016.md
docs/tasks/EPIC-009/STORAGE-017.md
docs/tasks/EPIC-009/STORAGE-018.md
docs/tasks/EPIC-009/STORAGE-019.md
docs/tasks/EPIC-009/STORAGE-020.md
docs/tasks/EPIC-009/STORAGE-021.md
docs/tasks/EPIC-009/STORAGECONTRACT-003.md
docs/tasks/EPIC-009/STORAGETEST-021.md
docs/tasks/EPIC-010.md
docs/tasks/EPIC-010/TWITCH-001.md
docs/tasks/EPIC-010/TWITCH-002.md
docs/tasks/EPIC-010/TWITCH-003.md
docs/tasks/EPIC-010/TWITCH-004.md
docs/tasks/EPIC-010/TWITCH-005.md
docs/tasks/EPIC-010/TWITCH-090.md
docs/tasks/EPIC-011.md
docs/tasks/EPIC-011/BETTERPLACE-001.md
docs/tasks/EPIC-011/BETTERPLACE-002.md
docs/tasks/EPIC-011/BETTERPLACE-003.md
docs/tasks/EPIC-011/BETTERPLACE-090.md
docs/tasks/EPIC-012.md
docs/tasks/EPIC-012/PATREON-001.md
docs/tasks/EPIC-012/PATREON-002.md
docs/tasks/EPIC-012/PATREON-003.md
docs/tasks/EPIC-012/PATREON-090.md
docs/tasks/EPIC-013.md
docs/tasks/EPIC-013/STREAMLABS-001.md
docs/tasks/EPIC-013/STREAMLABS-002.md
docs/tasks/EPIC-013/STREAMLABS-003.md
docs/tasks/EPIC-013/STREAMLABS-090.md
docs/tasks/EPIC-014.md
docs/tasks/EPIC-014/STREAMELEMENTS-001.md
docs/tasks/EPIC-014/STREAMELEMENTS-002.md
docs/tasks/EPIC-014/STREAMELEMENTS-003.md
docs/tasks/EPIC-014/STREAMELEMENTS-004.md
docs/tasks/EPIC-014/STREAMELEMENTS-090.md
docs/tasks/EPIC-015.md
docs/tasks/EPIC-015/KOFI-001.md
docs/tasks/EPIC-015/KOFI-002.md
docs/tasks/EPIC-015/KOFI-003.md
docs/tasks/EPIC-015/KOFI-090.md
docs/tasks/EPIC-016.md
docs/tasks/EPIC-016/TIPEEESTREAM-001.md
docs/tasks/EPIC-016/TIPEEESTREAM-002.md
docs/tasks/EPIC-016/TIPEEESTREAM-003.md
docs/tasks/EPIC-016/TIPEEESTREAM-004.md
docs/tasks/EPIC-016/TIPEEESTREAM-090.md
docs/tasks/EPIC-017.md
docs/tasks/EPIC-017/RATECONTRACT-001.md
docs/tasks/EPIC-017/RATECONTRACT-002.md
docs/tasks/EPIC-017/RATES-001.md
docs/tasks/EPIC-017/RATES-002.md
docs/tasks/EPIC-017/RATES-003.md
docs/tasks/EPIC-017/RATES-004.md
docs/tasks/EPIC-017/RATES-005.md
docs/tasks/EPIC-017/RATES-006.md
docs/tasks/EPIC-017/RATES-007.md
docs/tasks/EPIC-017/RATES-008.md
docs/tasks/EPIC-017/RATES-009.md
docs/tasks/EPIC-017/RATES-090.md
docs/tasks/EPIC-018.md
docs/tasks/EPIC-018/DESKTOP-001.md
docs/tasks/EPIC-018/DESKTOP-002.md
docs/tasks/EPIC-018/DESKTOP-003.md
docs/tasks/EPIC-018/DESKTOP-004.md
docs/tasks/EPIC-018/DESKTOP-005.md
docs/tasks/EPIC-018/DESKTOP-006.md
docs/tasks/EPIC-018/DESKTOP-007.md
docs/tasks/EPIC-018/DESKTOP-008.md
docs/tasks/EPIC-018/DESKTOP-009.md
docs/tasks/EPIC-018/DESKTOP-010.md
docs/tasks/EPIC-019.md
docs/tasks/EPIC-019/SCENE-001.md
docs/tasks/EPIC-019/SCENE-002.md
docs/tasks/EPIC-019/SCENE-003.md
docs/tasks/EPIC-019/SCENE-004.md
docs/tasks/EPIC-019/SCENE-005.md
docs/tasks/EPIC-019/SCENE-006.md
docs/tasks/EPIC-019/SCENE-007.md
docs/tasks/EPIC-020.md
docs/tasks/EPIC-020/RENDER-001.md
docs/tasks/EPIC-020/RENDER-002.md
docs/tasks/EPIC-020/RENDER-003.md
docs/tasks/EPIC-020/RENDER-004.md
docs/tasks/EPIC-020/RENDER-005.md
docs/tasks/EPIC-020/RENDER-090.md
docs/tasks/EPIC-020/RENDER-091.md
docs/tasks/EPIC-020/RENDER-092.md
docs/tasks/EPIC-021.md
docs/tasks/EPIC-021/WEBAPP-001.md
docs/tasks/EPIC-021/WEBAPP-002.md
docs/tasks/EPIC-021/WEBAPP-003.md
docs/tasks/EPIC-021/WEBAPP-004.md
docs/tasks/EPIC-021/WEBAPP-005.md
docs/tasks/EPIC-021/WEBAPP-006.md
docs/tasks/EPIC-021/WEBAPP-007.md
docs/tasks/EPIC-021/WEBAPP-008.md
docs/tasks/EPIC-021/WEBAPP-009.md
docs/tasks/EPIC-021/WEBAPP-010.md
docs/tasks/EPIC-021/WEBAPP-011.md
docs/tasks/EPIC-022.md
docs/tasks/EPIC-022/VIEWER-001.md
docs/tasks/EPIC-022/VIEWER-002.md
docs/tasks/EPIC-022/VIEWER-003.md
docs/tasks/EPIC-022/VIEWER-004.md
docs/tasks/EPIC-022/VIEWER-005.md
docs/tasks/EPIC-022/VIEWER-006.md
docs/tasks/EPIC-022/VIEWER-007.md
docs/tasks/EPIC-023.md
docs/tasks/EPIC-023/LOCALE-001.md
docs/tasks/EPIC-023/LOCALE-002.md
docs/tasks/EPIC-023/LOCALE-003.md
docs/tasks/EPIC-023/LOCALE-004.md
docs/tasks/EPIC-023/LOCALE-090.md
docs/tasks/EPIC-024.md
docs/tasks/EPIC-024/DEPLOYMENT-001.md
docs/tasks/EPIC-024/DEPLOYMENT-002.md
docs/tasks/EPIC-024/DEPLOYMENT-003.md
docs/tasks/EPIC-024/DEPLOYMENT-004.md
docs/tasks/EPIC-024/DEPLOYMENT-005.md
docs/tasks/EPIC-024/DEPLOYMENT-006.md
docs/tasks/EPIC-024/DEPLOYMENT-007.md
docs/tasks/EPIC-024/DEPLOYMENT-008.md
docs/tasks/EPIC-024/DEPLOYMENT-090.md
docs/tasks/EPIC-024/DEPLOYMENT-091.md
docs/tasks/EPIC-024/DEPLOYMENT-092.md
docs/tasks/EPIC-025.md
docs/tasks/EPIC-025/PACKAGE-001.md
docs/tasks/EPIC-025/PACKAGE-002.md
docs/tasks/EPIC-025/PACKAGE-003.md
docs/tasks/EPIC-025/PACKAGE-004.md
docs/tasks/EPIC-025/PACKAGE-005.md
docs/tasks/EPIC-025/PACKAGE-006.md
docs/tasks/EPIC-026.md
docs/tasks/EPIC-026/VALIDATE-001.md
docs/tasks/EPIC-026/VALIDATE-002.md
docs/tasks/EPIC-026/VALIDATE-003.md
docs/tasks/EPIC-026/VALIDATE-004.md
docs/tasks/EPIC-026/VALIDATE-005.md
docs/tasks/EPIC-026/VALIDATE-006.md
docs/tasks/EPIC-027.md
docs/tasks/EPIC-027/RELEASE-001.md
docs/tasks/EPIC-027/RELEASE-002.md
docs/tasks/EPIC-027/RELEASE-003.md
docs/tasks/EPIC-027/RELEASE-004.md
docs/tasks/EPIC-028.md
docs/tasks/EPIC-028/EVIDENCE-001.md
docs/tasks/EPIC-028/EVIDENCE-002.md
docs/tasks/EPIC-028/FIXTURE-001.md
docs/tasks/EPIC-028/FIXTURE-002.md
docs/tasks/EPIC-028/FIXTURE-003.md
docs/tasks/EPIC-028/TESTCLEAN-001.md
docs/tasks/EPIC-028/TESTCLEAN-002.md
docs/tasks/EPIC-028/TESTCLEAN-003.md
docs/tasks/EPIC-028/TESTCLEAN-004.md
docs/tasks/EPIC-028/TESTCLEAN-005.md
docs/tasks/EPIC-028/TESTCLEAN-006.md
docs/tasks/EPIC-028/TESTCLEAN-007.md
docs/tasks/EPIC-028/TESTCLEAN-008.md
docs/tasks/IMPL-001.md
docs/tasks/README.md
docs/tasks/REVIEW-001.md
docs/tasks/SCHEMATOOL-001.md
docs/tasks/SPIKE-001.md
docs/tasks/SPIKE-002.md
docs/tasks/SPIKE-003.md
docs/tasks/SPIKE-004.md
docs/tasks/STORAGECONTRACT-001.md
docs/tasks/STORAGECONTRACT-002.md
docs/tasks/TEST-001.md
docs/tasks/TEST-002.md
docs/tasks/WORKFLOW-001.md
docs/tasks/WORKFLOW-002.md
docs/tasks/WORKFLOW-003.md
docs/tasks/WORKFLOW-004.md
docs/tasks/WORKFLOW-005.md
docs/tasks/WORKFLOW-006.md
docs/tasks/WORKFLOW-007.md
```
