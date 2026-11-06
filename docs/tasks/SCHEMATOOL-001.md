# SCHEMATOOL-001 — Offline schema-package validation foundation

Kind: bounded DEVELOPER tooling implementation, paired baseline SCHEMATEST-001.
State/owner/dependencies: progress/state.json. Authority: specification v0.45
§§17/18, API-01/02/03 and TEST-04; [plan](CONTRACTPLAN-005.md).
This is not an HTTP server, client decoder or authentication capability.

## Required baseline and implementation

SCHEMATEST-001 first specifies a small JSON-package/manifest/CLI/error/resource
contract and independent literal valid/invalid examples, selects/pins existing
standards libraries with primary-source/license evidence, and obtains different
actual document/API/dependency review. Prefer existing OpenAPI3.1 and JSONSchema
Draft2020-12 validation libraries; do not implement a standards engine. No specific
library/version or upstream guarantee is finalized by this brief.

After that review, minimal explicit unimplemented wrapper/harness and independent
fixtures/tests are allowed. Separately review actual tests/oracles and observe
intended missing-wrapper behavior red only after installed pinned-library foundation
checks pass. Missing dependencies, unsupported dialect, import errors, skipped
checks or library/network failures are not feature red. Expected outputs must not
come from the wrapper under test. No CONTRACT013 schema is required for this small
upstream-style developer foundation.

SCHEMATOOL-001 production waits for that integrated reviewed/red baseline. Preserve
its assertions and implement only the bounded wrapper, with different source review,
passing acceptance, unconditional offline CI and actual selected dependency checks.
Do not call tooling success product/runtime acceptance.

## Validation and refusal scope

Validate complete OpenAPI3.1 structure/dialect and JSONSchema schemas, all allowed
local reference closure and declared literal request/response examples against their
schemas. Pin exact supported dialect/version and explicit format assertion policy;
format annotations alone cannot enforce signed64 strings or owning-domain rules.
Exercise valid package, invalid structure, missing/wrong local ref, wrong example
instance, unsupported dialect, duplicate JSON keys and bounded size/depth/count or
untrusted path/URI refusal. Exact inventory/limits/oracles/error priority are frozen
in the reviewed baseline, not invented by this plan.

Default validation is offline, deterministic and confines local resolution to the
admitted package/dependency registry. No implicit remote fetch, live endpoint/provider,
working-directory escape or secret-bearing diagnostics. Fail closed on unsupported
features; no silent example omissions, broad exceptions or success-after-skip.
Library validation cannot establish raw HTTP duplicate rejection, permissions,
CSRF/session correctness, actual framework behavior or running-server support.

Root owns dependency pins/lock/hashes/notices/shared schema imports/tooling/test/CI/
state/HTML changes and explicitly assigns paths. Keep actual CONTRACT013 package in
its own later task. Add stable automatic checks when wrapper behavior passes;
intentional baseline red stays separate. Record exact commands, installed versions,
independent reviews and remaining CI/rules/runtime/platform gates. All agreed release
scope remains. Read tools/tests/schemas scoped AGENTS before any corresponding edits.
