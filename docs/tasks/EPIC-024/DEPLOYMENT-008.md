# DEPLOYMENT-008 — alternative proxy account ingress admission

Paired baseline DEPLOYMENTTEST-008; implementation DEPLOYMENT-008. ADR-0011.
Authority: specification v0.45 §§2/10/16/17, AUTH-12, SERVER-01, DEP-05,
API-01, TEST-04/05. Root owns actual artifact/configuration/pin/build/CI integration.

Preserve actual support for an operator's existing reverse proxy instead of bundled
Caddy. Define an explicit source-backed reference alternative artifact/configuration
and equivalent operator admission requirements under accepted006 boundaries.
Unknown versions/configurations are unadmitted, not silently advertised. Do not
replace this requirement with direct TLS or a Caddy-only success fixture.

Use actual external raw-wire/log positive and negative controls for credential
cardinality/pairs/mixed/no-fallback, Connection nominations, framing/normalization,
head/body/protocol limits, actual TLS/authority/peer isolation/policy changes and
all secret-log sinks. Review tests/harness before config/behavior implementation;
observe valid intended red, then pass real path with separate source/config review
and selected CI. Missing tools/setup/configuration syntax failure/skip is not red.
Any substantial new module/runtime/artifact needs its own split before expansion.

Original DEPLOYMENT006/092 keep complete proxy/webhook/WS/restore/operator/release
scope and consume this admission. Source/clock/login/full transport remain owning
tasks. Use synthetic secrets and disposable state; no live/provider operations.
