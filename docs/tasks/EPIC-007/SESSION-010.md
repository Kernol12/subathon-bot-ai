# SESSION-010 — Actual lossless and secret-safe account ingress source

Parent: [EPIC-007](../EPIC-007.md). Paired baseline: SESSIONTEST-010.
Kinds: test_baseline / implementation. Specification v0.45 §2/§10/§17,
SERVER-01, AUTH-12/15, API-01, TEST-04/05.
Dependencies/status: progress/state.json. Contract: independently accepted
SESSIONCONTRACT-005 plus unchanged transport v1. No ingress solution selected here.

Own only the reviewed actual framework/proxy ingress correction/source and secret
handling path, with focused server/adapter/tests ownership agreed in the claim.
Read scoped instructions. Integrator owns dependencies, pin/patch provenance,
shared build/CI/deployment and state. Use API-001's actual lifecycle/queue boundary;
do not absorb its startup/shutdown/command implementation or SESSION-003 egress.

SESSIONTEST-010 authors independently reviewed raw-wire/real selected-stack tests
for repeated fields and cookie pairs before framework loss, malformed/empty/combined
credentials and complete size/framing limits, actual TLS/proxy/origin facts and
policy revisions. Observe synthetic credential log/copy paths at the actual early
parser/library layer, including the identified TRACE site; application-only log
tests are insufficient. Prove admission before unsafe allocation where required
by the contract. Include unavailable/unknown source facts and listener/proxy
misconfiguration without inventing successful trusted input.

Use disposable local fixtures and no provider traffic/live credentials. A working
real selected-framework harness and executed intended behavioral red, after separate
test approval, are required. A fake `AccountIngressSource` cannot establish this
baseline; build failures or old source inspection are not intended runtime red.

SESSION-010 implements the reviewed solution, preserving assertions and receipts;
passes real ingress plus affected bridge/regression checks; obtains separate source/
native-secret/resource/deployment/configuration review and actual stable selected CI.
State exactly which framing, listener/proxy and framework paths were observed and
which supported-platform/browser/deployment gates remain. Any changed dependency
must receive its actual source/provenance/compatibility review through the integrator.

Completion supplies actual ingress facts, not accepted authentication, protected
execution, browser cookie behavior, full CORS/CSRF route enforcement or native
credential publication. SESSION-003 retains those full integration requirements;
API-010/APITEST-010 remain gated by implemented SESSION-003. Record exact commands,
environment/revisions/results and remaining gates in a handoff; use normal latest-
head CI/PR merge gates.
