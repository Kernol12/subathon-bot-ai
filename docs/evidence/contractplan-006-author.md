# CONTRACTPLAN-006 — focused prerequisite author packet

Author: fresh `/root/session_slice_plan_author`, 7 November 2026.
Author branch/worktree: `task/CONTRACTPLAN-006-author`,
`/tmp/subathon-session-plan-author`, supplied base `b5143aa`.
Scope: planning documents only; root owns the authoritative progress graph/state,
parent briefs/handoff/HTML and every shared build/CI/schema/pin/runtime/test path.
No independent review is claimed by this author record.

## Result and scope preservation

[ADR-0010](../decisions/0010-session-executable-prerequisites.md) adds a first
bounded supplier component and explicit lifecycle, SQL/source, local-login,
clock and real-ingress prerequisites. Ten new briefs describe fifteen proposed
records. Existing SESSION-002/003 remain full service/transport integration gates;
component acceptance cannot complete them. No API, header, stub, crypto/protocol,
clock heuristic, schema/migration, test source, parser/library/pin or behavior is
selected or implemented by this packet.

Read root instructions/FIRST_AGENT_PROMPT, workflow, architecture/test strategy,
CONTRACTPLAN-006 and handoff; accepted supplier/transport v1; original SESSION and
ACCESS setup/login/recovery, API bridge and storage/restore briefs; relevant password
logical/API current-login/conditional-save and writer transaction outcome contracts;
specification §10 and decision references; current progress graph and actual server
source interface names. Accepted supplier API concreteness ends at prepare/resolve;
its §4 lifecycle guarantees remain required. The actual password interface is
`PasswordLoginSource`; no accepted-login/current-session runtime producer was found.
Transport's recorded pinned-source duplicate/cookie/TRACE gaps stay explicit.

Local acceptance is extracted upstream: ACCESS-008 does not depend on SESSION-002,
ACCESS-004 or Twitch implementation. Existing ACCESS-004 retains both methods,
stable-ID linking/freshness/local-fallback/provider verification and additionally
consumes ACCESS-008. SESSION-002 must bind real local acceptance and a reviewed
boundary for actual later provider authentication; a fake provider proof establishes
no support. No release requirement or original dependency is removed.

Store ownership includes only shared current account/credential/session/epoch facts.
ACCESS-001's setup/invitation policy uses those shared facts through new additive
store prerequisites; it is not absorbed by STORAGE-020 or ACCESS-008. Recovery and
restore retain their original owners and gain explicit store/clock prerequisites;
clock producer acceptance does not establish their real fence transitions. Current
permission/authority and disclosure integration remain full SESSION-002/003/HTTP
acceptance requirements; pure evaluators or caller DTOs do not satisfy them. Any
further substantial producer/runtime gap must be explicitly split before expansion.

Every baseline brief requires separately reviewed specification-derived tests,
working harness and observed intended behavioral red before production; every
implementation needs passing relevant acceptance/regression, different code review
and actual stable selected CI. The clock/ingress briefs explicitly distinguish fake
policy tests from real producer/source evidence. Supported browsers, Windows/Linux
stores/packages, live/Twitch/provider, recovery and full release gates remain.

## Exact proposed new nodes

The following JSON is a coordinator integration proposal, not a second status
source. Use these exact IDs/kinds/dependencies/requirements/briefs/summaries; all
new records start planned/unassigned until normal claims and completed gates.
Add normal empty verification/evidence/review fields as required by the state schema.
Do not overwrite unrelated task fields or statuses. Every new task has its own
focused branch/PR and fresh author session under the normal workflow.

```json
[
  {
    "id": "SESSIONTEST-008",
    "title": "Review and demonstrate red: bounded account-session supplier",
    "module": "Identity",
    "kind": "test_baseline",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "CONTRACTPLAN-006",
      "SESSIONCONTRACT-001",
      "SESSION-001"
    ],
    "spec_refs": [
      "§10",
      "AUTH-05",
      "AUTH-12",
      "TEST-04",
      "TEST-05"
    ],
    "brief": "docs/tasks/EPIC-007/SESSION-008.md",
    "summary": "Independently review supplier prepare/resolve tests and observe intended component red; no actual source, clock, lifecycle or transport claim."
  },
  {
    "id": "SESSION-008",
    "title": "Bounded token preparation and current-session resolution",
    "module": "Identity",
    "kind": "implementation",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "CONTRACTPLAN-006",
      "SESSIONCONTRACT-001",
      "SESSION-001",
      "SESSIONTEST-008"
    ],
    "spec_refs": [
      "§10",
      "AUTH-05",
      "AUTH-12",
      "TEST-04",
      "TEST-05"
    ],
    "brief": "docs/tasks/EPIC-007/SESSION-008.md",
    "summary": "Implement only accepted supplier v1 prepare/resolve with bounded ownership and honest current-fact/clock outcomes; full SESSION-002/003 remain required."
  },
  {
    "id": "SESSIONCONTRACT-003",
    "title": "Concrete session lifecycle mutation and inspection contract",
    "module": "Identity",
    "kind": "contract",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "CONTRACTPLAN-006",
      "SESSIONCONTRACT-001",
      "SESSIONCONTRACT-002",
      "SESSION-001",
      "STORAGE-010",
      "ACCESS-002"
    ],
    "spec_refs": [
      "§10",
      "§16",
      "AUTH-01",
      "AUTH-05",
      "AUTH-09",
      "AUTH-10",
      "AUTH-11",
      "AUTH-12",
      "AUTH-15",
      "DB-01",
      "TEST-04",
      "TEST-05"
    ],
    "brief": "docs/tasks/EPIC-007/SESSIONCONTRACT-003.md",
    "summary": "Specify independently reviewed internal issuance, rotation, reauthentication, revocation and inspection APIs/outcomes before full session-service tests; document only."
  },
  {
    "id": "STORAGECONTRACT-002",
    "title": "Account, credential and current-session storage contract",
    "module": "Operations",
    "kind": "contract",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "CONTRACTPLAN-006",
      "SESSIONCONTRACT-003",
      "STORAGE-010",
      "STORAGE-011",
      "SESSION-001"
    ],
    "spec_refs": [
      "§10",
      "§16",
      "AUTH-01",
      "AUTH-05",
      "AUTH-11",
      "AUTH-12",
      "DB-01",
      "SERVER-01",
      "OPS-02"
    ],
    "brief": "docs/tasks/STORAGECONTRACT-002.md",
    "summary": "Define separately reviewed physical account/credential/session/epoch facts, bounded current readers and conditional writer primitives; no schema or runtime implementation."
  },
  {
    "id": "STORAGETEST-020",
    "title": "Review and demonstrate red: physical account/session store",
    "module": "Operations",
    "kind": "test_baseline",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "STORAGECONTRACT-002",
      "STORAGE-010",
      "STORAGE-011"
    ],
    "spec_refs": [
      "§10",
      "§16",
      "AUTH-01",
      "AUTH-05",
      "AUTH-11",
      "AUTH-12",
      "DB-01",
      "SERVER-01",
      "OPS-02",
      "TEST-04",
      "TEST-05"
    ],
    "brief": "docs/tasks/EPIC-009/STORAGE-020.md",
    "summary": "Review literal real-SQL account/session/source and atomicity tests, then observe intended storage red with the selected working Qt SQL harness."
  },
  {
    "id": "STORAGE-020",
    "title": "Physical account/session store and current fact producers",
    "module": "Operations",
    "kind": "implementation",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "STORAGECONTRACT-002",
      "STORAGE-010",
      "STORAGE-011",
      "STORAGETEST-020"
    ],
    "spec_refs": [
      "§10",
      "§16",
      "AUTH-01",
      "AUTH-05",
      "AUTH-11",
      "AUTH-12",
      "DB-01",
      "SERVER-01",
      "OPS-02",
      "TEST-04",
      "TEST-05"
    ],
    "brief": "docs/tasks/EPIC-009/STORAGE-020.md",
    "summary": "Implement the reviewed store/migration/current snapshot readers on the existing writer; no login acceptance, service authorization or credential publication."
  },
  {
    "id": "SESSIONCONTRACT-004",
    "title": "Trustworthy current session-clock producer contract",
    "module": "Identity",
    "kind": "contract",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "CONTRACTPLAN-006",
      "SESSIONCONTRACT-003",
      "STORAGECONTRACT-002",
      "CONTRACT-002"
    ],
    "spec_refs": [
      "§10",
      "§16",
      "AUTH-05",
      "AUTH-09",
      "AUTH-12",
      "DB-01",
      "SERVER-01",
      "TEST-04",
      "TEST-05"
    ],
    "brief": "docs/tasks/EPIC-007/SESSIONCONTRACT-004.md",
    "summary": "Define actual clock-source trust, scope, discontinuity/recovery refusal and publication sampling for later tests; no clock mechanism or implementation chosen by this plan."
  },
  {
    "id": "SESSIONTEST-009",
    "title": "Review and demonstrate red: actual current session clock",
    "module": "Identity",
    "kind": "test_baseline",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "SESSIONCONTRACT-004",
      "STORAGE-020",
      "BOOT-003"
    ],
    "spec_refs": [
      "§10",
      "§16",
      "AUTH-05",
      "AUTH-09",
      "AUTH-12",
      "DB-01",
      "SERVER-01",
      "TEST-04",
      "TEST-05"
    ],
    "brief": "docs/tasks/EPIC-007/SESSION-009.md",
    "summary": "Review deterministic trust/fence cases and actual producer observation tests, then observe intended clock-producer red distinct from supplier fake-clock tests."
  },
  {
    "id": "SESSION-009",
    "title": "Actual current session-clock producer",
    "module": "Identity",
    "kind": "implementation",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "SESSIONCONTRACT-004",
      "STORAGE-020",
      "BOOT-003",
      "SESSIONTEST-009"
    ],
    "spec_refs": [
      "§10",
      "§16",
      "AUTH-05",
      "AUTH-09",
      "AUTH-12",
      "DB-01",
      "SERVER-01",
      "TEST-04",
      "TEST-05"
    ],
    "brief": "docs/tasks/EPIC-007/SESSION-009.md",
    "summary": "Implement the reviewed trustworthy runtime session clock and refusal fences; timer/recovery/restore workflows and supported-platform evidence remain separate."
  },
  {
    "id": "ACCESSCONTRACT-001",
    "title": "Actual accepted-local-login authority contract",
    "module": "Identity",
    "kind": "contract",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "CONTRACTPLAN-006",
      "SESSIONCONTRACT-003",
      "STORAGECONTRACT-002",
      "SESSIONCONTRACT-004",
      "SESSION-001"
    ],
    "spec_refs": [
      "§10",
      "AUTH-01",
      "AUTH-05",
      "AUTH-08",
      "AUTH-09",
      "AUTH-10",
      "AUTH-11",
      "AUTH-12",
      "AUTH-15",
      "SERVER-01",
      "TEST-04",
      "TEST-05"
    ],
    "brief": "docs/tasks/EPIC-006/ACCESSCONTRACT-001.md",
    "summary": "Define current local-login acceptance, source-bound accepted-attempt checks and conditional password-upgrade integration upstream of sessions; document only."
  },
  {
    "id": "ACCESSTEST-008",
    "title": "Review and demonstrate red: accepted local-login authority",
    "module": "Identity",
    "kind": "test_baseline",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "ACCESSCONTRACT-001",
      "ACCESS-001",
      "SESSION-001",
      "STORAGE-020",
      "SESSION-009"
    ],
    "spec_refs": [
      "§10",
      "AUTH-01",
      "AUTH-05",
      "AUTH-08",
      "AUTH-11",
      "AUTH-12",
      "AUTH-15",
      "SERVER-01",
      "TEST-04",
      "TEST-05"
    ],
    "brief": "docs/tasks/EPIC-006/ACCESS-008.md",
    "summary": "Review actual local-account/store/accepted-attempt/rehash integration tests and observe intended red without a preexisting session or manufactured authority."
  },
  {
    "id": "ACCESS-008",
    "title": "Accepted local login and current password-source integration",
    "module": "Identity",
    "kind": "implementation",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "ACCESSCONTRACT-001",
      "ACCESS-001",
      "SESSION-001",
      "STORAGE-020",
      "SESSION-009",
      "ACCESSTEST-008"
    ],
    "spec_refs": [
      "§10",
      "AUTH-01",
      "AUTH-05",
      "AUTH-08",
      "AUTH-11",
      "AUTH-12",
      "AUTH-15",
      "SERVER-01",
      "TEST-04",
      "TEST-05"
    ],
    "brief": "docs/tasks/EPIC-006/ACCESS-008.md",
    "summary": "Implement actual local acceptance and source-bound conditional upgrade integration; ACCESS-004 retains full local/Twitch login/linking and session services stay separate."
  },
  {
    "id": "SESSIONCONTRACT-005",
    "title": "Concrete lossless and secret-safe ingress solution",
    "module": "Identity",
    "kind": "contract",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "CONTRACTPLAN-006",
      "SESSIONCONTRACT-002",
      "CONTRACT-005"
    ],
    "spec_refs": [
      "§2",
      "§10",
      "§17",
      "SERVER-01",
      "AUTH-12",
      "AUTH-15",
      "API-01",
      "TEST-04",
      "TEST-05"
    ],
    "brief": "docs/tasks/EPIC-007/SESSIONCONTRACT-005.md",
    "summary": "Define separately reviewed source-backed selected-framework ingress/log/TLS/proxy/policy solution before tests; no parser patch, library or proxy guarantee approved here."
  },
  {
    "id": "SESSIONTEST-010",
    "title": "Review and demonstrate red: actual lossless account ingress",
    "module": "Identity",
    "kind": "test_baseline",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "SESSIONCONTRACT-005",
      "API-001",
      "BOOT-003"
    ],
    "spec_refs": [
      "§2",
      "§10",
      "§17",
      "SERVER-01",
      "AUTH-12",
      "AUTH-15",
      "API-01",
      "TEST-04",
      "TEST-05"
    ],
    "brief": "docs/tasks/EPIC-007/SESSION-010.md",
    "summary": "Review actual raw-wire selected-stack duplicate/framing/secret-log/trust tests and observe intended ingress red; fake ingress and static source inspection are insufficient."
  },
  {
    "id": "SESSION-010",
    "title": "Actual lossless and secret-safe account ingress source",
    "module": "Identity",
    "kind": "implementation",
    "status": "planned",
    "owner": "Unassigned",
    "depends_on": [
      "SESSIONCONTRACT-005",
      "API-001",
      "BOOT-003",
      "SESSIONTEST-010"
    ],
    "spec_refs": [
      "§2",
      "§10",
      "§17",
      "SERVER-01",
      "AUTH-12",
      "AUTH-15",
      "API-01",
      "TEST-04",
      "TEST-05"
    ],
    "brief": "docs/tasks/EPIC-007/SESSION-010.md",
    "summary": "Implement only the reviewed actual ingress solution/source; SESSION-003 keeps full transport/current-execution/egress/client integration gates."
  }
]
```

## Exact additive edges for existing records

Append each listed dependency to that existing record. Keep its entire original
`depends_on`, ID, kind, requirements, status, evidence, notes and reviews. SESSION-002
and its baseline gain explicit runtime suppliers; SESSION-003 and its baseline gain
actual ingress while retaining their SESSION-002 dependency. New epic edges are
aggregation only; no new child depends on an epic. No reverse SESSION-002 →
ACCESS-004 dependency is introduced. Root should reconcile task prose/index counts
and mark existing active dependents blocked only if their actual state requires it.

```json
{
  "SESSIONTEST-002": [
    "SESSIONCONTRACT-003",
    "SESSION-008",
    "STORAGE-020",
    "SESSION-009",
    "ACCESS-008"
  ],
  "SESSION-002": [
    "SESSIONCONTRACT-003",
    "SESSION-008",
    "STORAGE-020",
    "SESSION-009",
    "ACCESS-008"
  ],
  "SESSIONTEST-003": [
    "SESSION-010"
  ],
  "SESSION-003": [
    "SESSION-010"
  ],
  "ACCESSTEST-001": [
    "STORAGE-020"
  ],
  "ACCESS-001": [
    "STORAGE-020"
  ],
  "ACCESSTEST-004": [
    "ACCESS-008"
  ],
  "ACCESS-004": [
    "ACCESS-008"
  ],
  "ACCESSTEST-005": [
    "STORAGE-020",
    "SESSION-009"
  ],
  "ACCESS-005": [
    "STORAGE-020",
    "SESSION-009"
  ],
  "STORAGETEST-007": [
    "STORAGE-020",
    "SESSION-009"
  ],
  "STORAGE-007": [
    "STORAGE-020",
    "SESSION-009"
  ],
  "EPIC-006": [
    "ACCESSCONTRACT-001",
    "ACCESSTEST-008",
    "ACCESS-008"
  ],
  "EPIC-007": [
    "SESSIONTEST-008",
    "SESSION-008",
    "SESSIONCONTRACT-003",
    "SESSIONCONTRACT-004",
    "SESSIONTEST-009",
    "SESSION-009",
    "SESSIONCONTRACT-005",
    "SESSIONTEST-010",
    "SESSION-010"
  ],
  "EPIC-009": [
    "STORAGECONTRACT-002",
    "STORAGETEST-020",
    "STORAGE-020"
  ]
}
```

## Author checks and remaining gates

Read-only proposed-graph check on this author tree: 428 old IDs / 1634 old edges → 443 IDs / 1735 edges; 15 new nodes, 62 new-node edges and 39 additive existing-node edges. All old IDs/edges preserved, all dependencies exist, every proposed brief exists and DFS found no cycle. This is a proposed in-memory graph, not the coordinator's actual state integration.

Actual author checks on 7 November 2026 in the supplied Linux worktree:

| Command/check | Observed result |
| --- | --- |
| `python3 tools/check_project.py` | Exit 0; 428 current records, 122 decisions, links/presets/current HTML pass. It checks the existing authoritative graph, not the proposed graph's integration. |
| `python3 tools/progress.py render --check` | Exit 0; existing snapshot current, no generated/shared file written. |
| `python3 -m unittest discover -s tests/tooling -v` | Exit 0; 19/19 tooling cases pass in 0.364 s. |
| Inline Python parse/link/whitespace check | Exit 0; both proposal JSON blocks parse and all 12 owned documents' local links/line whitespace pass. |
| Inline Python proposed-graph preservation/DFS | Exit 0; counts and limits above, no mutation of state.json. |

This packet defines test acceptance, not tests or a red/pass feature claim. No
CMake/native/crypto/network/provider/client command was run. The coordinator must
integrate the actual graph, update parent briefs/epic counts/handoff/progress HTML,
run project/tooling/whitespace/current-render checks, obtain a different actual
review of the final plan **and actual graph**, then observe latest-head CI/current
rules and complete the focused planning PR. Original tests, receipts, source,
completed task states and every release requirement must remain intact.
