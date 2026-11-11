# SESSION-012 — AUTH-route transport budget producer and admission

Parent: [EPIC-007](../EPIC-007.md). Paired baseline: SESSIONTEST-012.
Kinds: test_baseline / implementation. Specification v0.45 §10/§16,
AUTH-01/08/10/11/12, SERVER-01, DEP-04/05/06, TEST-04/05.
Dependencies/status: progress/state.json. Contract prerequisite:
[SESSIONCONTRACT-007](SESSIONCONTRACT-007.md). Planned only; no executable work
is authorized by this split. API-013 alone supplies no transport-budget producer.

Own the separately accepted AUTH edge-anchor/remaining-budget producer and its
additive binding to actual SESSION-010 scope/policy plus API-013 admission. Admit
both bundled and supported alternative proxy AUTH profiles on their existing
DEPLOYMENT-007/008 foundations. Existing contracts/edges remain frozen; absent
producer stays unavailable. No client header/time/DTO may manufacture authority.
No dependency on ACCESS-001/005/008 or SESSION-002/003 implementations is allowed.

SESSIONTEST-012 authors minimal additive interfaces, independently derived oracles
and an actual selected-stack edge/host harness with synthetic secrets. Review tests
before intended missing-behavior red; unavailable dependencies, compilation failure
or skips are not red. Test actual anchor/delay/clock uncertainty/margins for both
edges, source membership/reloads, queues/equality/early close, stop/drain and no
secret log/copy regressions. Fakes prove component semantics only, not edge admission.
SESSION-012 then uses a fresh implementation session, passes the reviewed actual
admission and affected host/ingress/proxy regressions, receives separate source/
resource/clock/secret/config review and selected CI, and merges normally. Coordinator
owns shared pins/build/CI/state. Record exact supported artifacts/configurations
and unavailable paths; no password decision, directory, session issuance or full
release/platform/provider proof follows. All password-auth consumer baselines and
implementations explicitly wait for SESSION-012 as well as their existing gates.
