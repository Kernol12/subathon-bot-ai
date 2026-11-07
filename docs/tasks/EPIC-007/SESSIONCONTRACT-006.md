# SESSIONCONTRACT-006 — reconcile edge and application ingress boundaries

Kind: contract revision, document/artifact only. State/dependencies: progress/state.json.
Authority: specification v0.45 §§2/10/17, AUTH-12, SERVER-01, DEP-05, API-01, TEST-04/05.
Inputs: accepted transport002/common005/reference013, actual source-backed005 partial
and independent read-only proxy audit. [ADR-0011](../../decisions/0011-ingress-boundaries.md).

Explicitly reconcile unimplemented transport §1/5 and reference HTTP §1/4 engineering
contracts before claiming proxy support. Define old/new document revisions, reasons,
affected negative oracles/consumers and compatibility (no reference server capability
or transport implementation is advertised). Preserve all credential cardinality,
empty-field/mixed/no-fallback, Origin/marker/CORS, actual TLS/configured-peer trust,
framing agreement and secret-log guarantees. Do not silently rename public wire
input as backend input. No raw-head envelope/mTLS/TLS slab/new runtime is mandated.

Prefer the smallest source-backed compatible solution: separate finite edge parser/
head/body/protocol budgets from complete application-listener occurrence/semantic
admission; enumerate allowed proxy decoding/forwarding and refusals. Address actual
Connection nomination, framing normalization, URL/header identity and all pre-/post-
handler logging/error/debug/export sinks. Pin concrete supported candidate controls,
not a blanket stock-proxy promise. Unknown paths remain unadmitted. Preserve both
bundled Caddy and alternative-proxy release support; direct TLS is optional.

A fresh author may revise only explicitly delegated transport/reference documents,
reference OpenAPI descriptive/admission metadata (no payload/operation changes),
and its author/decision evidence. Root owns task graph/shared imports/actual pins/
build/CI/deploy/state/HTML. Actual native artifact and proxy implementation remain
separate011/007/008 tasks with their own reviewed/red baselines. If a new reusable
proxy/parser subsystem becomes necessary, split it before implementation.

Different actual review must examine the revised contracts, complete source-backed
security reasoning/oracles and actual additive graph. Preserve all443old IDs/1735
edges/status/reviews except explicitunfinished005blocked/dependency additions.
Run applicable project/tooling/schema validation for changed artifacts, record
actual results and latest-head CI/normal merge gates. This task supplies no runtime.
