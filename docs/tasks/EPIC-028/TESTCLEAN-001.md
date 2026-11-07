# TESTCLEAN-001 — Replace schema harness tests with real schema checks

Module: Tooling. Kind: cleanup. Specification: v0.45; §§17–18.
Contract revision: current schema-validation contract and specification v0.45.
Parent: EPIC-028. Dependencies: none. Shared-file ownership: `tests/schema_validation/`,
its CMake registration, relevant schema tooling/contracts, and ADR-0009 updates if needed.

## Scope

Replace the disproportionate native loader/fault-injection/audit machinery with a
data-driven test of the actual project schemas. Validate every schema in `schemas/`
with a selected standards-compliant validator and check its valid and invalid example
documents. Keep the §18 validation purpose. Include `progress/state.schema.json` as a shipped
tooling schema with structural valid/invalid examples and the real state instance;
its editor hints do not prove runtime DAG, review or combined-note-length rules. Review the current validator/tooling and
ADR-0009 plus `docs/contracts/schema-validation-native-process-v1.md` before changing
or deleting any accepted process-boundary requirement.

## Acceptance criteria

- Every shipped schema is included; each has independently authored valid and invalid
  examples with assertions against the real validator, not a harness fake.
- Validator choice and version are explicit and repeatable; no schema or example is
  silently skipped, and validation remains in the appropriate project test gate.
- Any validator dependency follows ADR-0008's isolated developer-tool exception: review
  pinned direct/transitive versions, hashes, licenses and offline foundations; do not
  install globally or add it to server/client runtime dependencies.
- Before removing native process tests or changing native interfaces, obtain the separate
  review required by `tools/AGENTS.md` for ADR-0009 and the native/process-test/installation
  contracts plus oracle map. Preserve existing assertions until that gate approves the
  precise change; do not run native probes/builds as part of this cleanup before approval.
- Native fault children, assembly audits, owner rows and loader-foundation tests are
  removed only after the independent reviewer confirms which contracts they protect.
- If this cleanup removes the native process boundary, update ADR-0009 and its contract
  in the same reviewed change with a specification-based reason; otherwise preserve them.
- A separate reviewer approves the test/oracle and any removed assertions; record the
  precise baseline change in the PR and follow the workflow's task-note and area-summary
  rules.

## Verification and handoff

Run the schema suite and relevant tooling/project checks. Record actual commands,
results and environment. Do not treat JSON parsing, compile failure, missing validator
or skipped cases as successful red or validation. Remains a cleanup task only; it does
not implement runtime authorization or certify provider behavior.
