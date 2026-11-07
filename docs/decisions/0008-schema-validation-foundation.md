# ADR-0008 — Select isolated standards libraries for schema validation

Status: accepted dependency/validation purpose; developer-tool sequencing superseded
by the separately reviewed TESTCLEAN-001 boundary under
[ADR-0012](0012-test-suite-simplification.md). Authority: specification v0.45
§§17–18, API-01/02/03 and TEST-04.

Validate actual source-controlled OpenAPI/JSON Schema documents and their independent
valid/invalid examples with real selected standards libraries. JSON syntax or repository
link checks do not establish schema/instance validity. Keep complete actual reference
and example coverage, explicit dialects and offline resolution.

The current [project-check contract](../contracts/schema-validation-tool-v1.md)
reuses all 20 admitted versions/artifacts in the existing lock and wheel/license
manifest, in an isolated developer-only environment. Ordinary progress/repository
tools remain standard-library-only; no dependency is added to server/clients and
no package is installed globally or automatically during validation.

The former generic arbitrary-package API and its process/native boundary are retired,
not silently weakened or replaced with a fake validator. The original decision,
backlog decomposition, reviews and receipts remain in the
[archived ADR](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/decisions/0008-schema-validation-foundation.md).
Actual new tests/oracles, source checks, selected-host CI and normal merge gates still
apply. Schema checks prove no runtime authentication, provider/platform support or
complete release readiness; all v0.45 requirements remain.
