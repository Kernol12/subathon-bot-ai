# WORKFLOW-005 — Compact context and publication checks

Kind: implementation. Specification v0.45 §§17–18. Depends on WORKFLOW-004.
Contract: [compact context/publication v1](../contracts/context-publication-v1.md),
refining bounded-coordination-v2 §§2,4–5. Review it before test authorship.
No product task is resumed.

Coordinator owns brief/docs, shared state/HTML/checkpoint/build integration. Test
author owns additive focused tests in tests/tooling/ and actual-schema cases if
needed; fresh implementer owns tools/progress.py and a small publication checker.
Read root/tools/tests instructions. Preserve default context API and all assertions.
The reviewed contract also assigns the test author narrow explicit NI seams in
tools/check_publication.py and tools/progress.py; they accept the new interfaces but
fail by name until the fresh implementation. No missing-interface/setup failure
counts as red. Publication unit tests patch private ROOT/main/run_step seams;
SDK integration uses a fresh committed copy of working-tree source, not HEAD cloning.

Implement opt-in compact task/stage context without duplicated history. Publication
checks must invoke actual admitted schema validation, project/progress/render checks,
inspect explicit outgoing history for private paths (including subsequently deleted
files), and compare selected stages' existing task.workflow.stages revision/paths
with the candidate tree. Missing or stale selected bindings refuse. Introduce no
new manifest/digest convention or schema fields; WORKFLOW-006 owns those and the
later publication-check extension. Do not claim generic secret or
review-authentication proof. No package install/network/push/merge in the checker.

Independent examples: rich APITEST-016 selected-stage context excludes supplemental
workflow/activity and other stages but retains next gate/limitations; full context is
unchanged; unknown task/stage refuses without writes; invalid schema fails even if
semantic validation passes; a private file added then deleted inside the outgoing
range is refused; clean history passes; stale required path binding refuses; unavailable
schema environment is setup failure, never success. Use disposable git fixtures.

Use contract/examples → authored tests → full independent test cycle → meaningful
intended red through existing entry points → fresh implementation → full independent
code cycle and relevant tooling/schema/scaffold checks. Keep unreviewed tests local.
Record actual proof and limitations in a focused PR, pass latest-head CI and merge
normally. Update guidance to use compact context and the publication command.
WF004-C-A2 is resolved by the accompanying task-index wording: practices/private
evidence guidance applies at WORKFLOW-004 merge; record/results policy waits for
reviewed WORKFLOW-006 tooling. Preserve that activation split in command guidance.
