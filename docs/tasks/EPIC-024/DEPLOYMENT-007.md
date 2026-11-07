# DEPLOYMENT-007 — bundled proxy account ingress admission

Paired baseline DEPLOYMENTTEST-007; implementation DEPLOYMENT-007. ADR-0011.
Authority: specification v0.45 §§2/10/16/17, AUTH-12, SERVER-01, DEP-04/05/06,
API-01, TEST-04/05. Root owns actual artifact/version/toolchain/config/pins/CI.

Admit the exact bundled Caddy artifact/configuration under reviewed006 boundaries,
using actual source evidence and external raw-wire positive controls. Verify all
critical occurrence/pair/mixed/empty/no-fallback behavior across forwarding,
Connection nominations, framing/normalization/protocol/head/body budgets, URL/
authority/TLS/configured peer isolation, policy reload coordination and complete
error/debug/access/export secret-log suppression. A disabled access log or default
credential redaction alone is not proof. Unknown paths are not supported.

Author/review the actual test/harness before behavior/config implementation, observe
valid intended red (not dependency/setup/config-parse failure), then preserve tests,
pass the real admitted path, get separate source/configuration review and stable CI.
Select/admit artifacts/dependencies with exact licenses/hashes/foundations; no new
runtime/proxy module is implicitly authorized. Split a substantial reusable guard/
parser/artifact prerequisite before implementing it. No raw-head protocol assumed.

This is bounded account-ingress admission, not full server packaging/deployment.
Original Compose/Quadlet/proxy/webhook/WS/backup/update/platform tasks keep all scope
and now consume this evidence. No live credentials/provider/chat/payment side effects.
