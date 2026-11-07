# CONTRACTPLAN-006 — independent planning and actual-graph review

Author: fresh `/root/session_slice_plan_author`, author commit `2e5fde9c`
imported as `987d475`; coordinator `/root` owns actual graph/shared integration.
Reviewer: fresh `/root/session_slice_plan_review`, a different session from both
writers. Date: 7 November 2026. Review worktree `/tmp/subathon-session-plan-review`,
branch `review/CONTRACTPLAN-006-session-slices`.

Verdict: **approved for integration of the planning documents and graph only**.
No blocking findings. This approves no runtime choice, contract/API/schema,
test baseline, red run, implementation or supported authentication capability.

## Exact packet and prior approvals

Reviewed actual integrated commit
`16b071b73b8fc5c2ad313aadb8d53ed4844d6a7e`, against current main
`ed73270b7343d74886d64d6a64d5d64d44320a83`. The packet comprises ADR-0010,
CONTRACTPLAN-006 and the ten new child briefs, both JSON proposals in the author
note, actual progress/state.json and generated PROGRESS.html, parent/epic brief
additions and task/coordinator handoffs. The new plan brief is additional to the
ten child briefs; the plan record is additional to the fifteen proposed records.

Authority: specification v0.45 §§10/16/17/18 and the cited authentication,
database, server, workflow and test-first decisions; root AGENTS.md,
FIRST_AGENT_PROMPT.md, architecture, workflow and test strategy. Checked the
original SESSION-002/003, ACCESS-001/003/004/005 and STORAGE-007 scopes and current
password/current-login/conditional-upgrade obligations against the new allocation.

Accepted supplier document blob `c5f5a6e71664042e18475068d3caf577b0c8d009`
and transport blob `2c33c2bbe9052455107c3056783d6bb605a5fef2` are byte-identical
to main. Their approvals remain valid and are not expanded to runtime behavior.
No source, test, fixture, schema, migration, build, pin or CI file changes appear
in this planning diff; historical receipts and other review files are unchanged.

## Independent graph audit

A read-only Python audit loaded main and the actual state, parsed both author
JSON blocks and asserted exact integration, rather than accepting the proposal's
in-memory result as authoritative:

- All 427 original IDs and 1,630 original edges remain. Actual totals are 443
  unique IDs and 1,735 edges: the plan adds one record/four edges, the author
  proposal adds fifteen records/62 edges and existing records gain 39 edges.
- Existing row differences are confined to the fifteen documented dependency/
  note additions and three epic summaries. Every original note remains an exact
  prefix; all other old fields, including status, kind, owner, requirements,
  verification, evidence and review attribution, are equal to main.
- All dependencies and briefs exist; no duplicate edges or cycles exist across
  all 443 nodes. New children have no dependency on their epic or downstream
  SESSION-002/003, ACCESS-004/005 or STORAGE-007.
- Each of the five implementation records includes its matching test_baseline;
  all other paired prerequisites match exactly. Each pair is required by its
  owning epic. All fifteen new records remain planned/unassigned, with empty
  verification/evidence and no review. The actual plan alone is in_review.
- SESSIONTEST-008 requires the plan, accepted supplier and completed SESSION-001.
  Its other prerequisites are done, so it becomes dependency-ready after this
  plan is completed/integrated. SESSION-008 still waits for its own baseline.

The author count 428/1,634 includes the coordinator's earlier plan claim; the
handoff count 427/1,630 uses current main without that record. Both reconcile
exactly to the same actual 443/1,735 graph; neither represents feature progress.
No original dependent is active, so retaining its planned state is appropriate.

## Scope, authority and failure-boundary assessment

SESSION-008 is bounded to the accepted prepare/resolve component; source/clock
fakes explicitly prove simulation only. Its native readiness, secret scratch,
ordering, whole-input/ownership preflight, exact time and source-failure coverage
are future baseline obligations, not authored or executed tests in this packet.

SESSIONCONTRACT-003 retains full issuance/rotation/reauthentication/inspection/
revocation, current permission/disclosure, collisions, epochs, absolute lifetimes
and commit/cleanup/publication uncertainty. SESSION-002 and its baseline retain
their original scope/edges and explicitly require the new real suppliers. A raw
reference, password-helper result or pure permission decision cannot grant access.

STORAGECONTRACT-002 and STORAGE-020 own physical shared facts/current readers on
the existing controlled writer, with separate physical contract, real-SQL baseline
and implementation gates. Account setup/invitation policy remains ACCESS-001;
its baseline and implementation now both require this store. Migration ordering,
schema/build/CI/pins and shared integration remain coordinator-owned.

ACCESS-008 supplies actual current local acceptance and source-bound conditional
rehash without a prior session. ACCESS-004 retains full local/Twitch linking,
stable IDs, fresh changes and owner fallback, and consumes the extracted local
component. There is no SESSION-002→ACCESS-004 edge. Actual provider evidence must
reach the lifecycle boundary under ACCESS-004's own review/tests; fake proof
cannot advertise Twitch support.

SESSION-009 requires a separately reviewed actual clock mechanism and evidence,
with refusal on unresolved discontinuity/recovery and real producer binding.
Its contract selects no heuristic now. Recovery ACCESS-005 and restore STORAGE-007
keep their epoch/fence protocols and gain store/clock edges in both stages;
neither workflow is claimed by clock acceptance or made an upstream dependency.

SESSION-010 separately requires a source-backed ingress solution, reviewed real
selected-stack baseline and implementation. Early multiplicity loss, Cookie TRACE,
TLS/proxy/origin/policy and allocation obligations remain explicit; an ordinary
map wrapper, application-only log test or client trust flag is insufficient.
SESSION-003 retains full transport/current execution/egress/client acceptance;
API-001 retains bridge/lifecycle and API-003 retains distinct WebSocket gates.

All later contracts must independently settle their concrete API/schema/runtime
choices and split further substantial prerequisites if discovered. No new parser
patch, library/pin, crypto/protocol, credential-proof format, SQL layout or runtime
guarantee is selected here. All original integrations, FX providers, account/UI/
renderer/localization/themes, recovery and platform/release requirements remain.

## Commands actually run and remaining gates

Environment: Linux 7.2.14-arch1-1 x86-64, Python 3.14.9, isolated review worktree at the exact
packet above. Commands on 7 November 2026:

| Command/check | Actual result |
| --- | --- |
| `python3 tools/check_project.py` | Exit 0; 443 records, 122 decisions, local links/presets/current HTML pass. |
| `python3 tools/progress.py render --check` | Exit 0; current generated snapshot. |
| `python3 -m unittest discover -s tests/tooling -v` | Exit 0; 19/19 pass in 0.368 seconds. These are offline tooling tests. |
| `git diff --check main...HEAD` | Exit 0; no whitespace errors. |
| Read-only inline Python: load `git show ed73270:progress/state.json`, current state and both author JSON blocks; assert exact proposal fields/edges, old metadata/notes, references, brief files, five baseline pairs/epic edges, first-supplier readiness, full DFS and document-only changed paths | Exit 0; all preservation/readiness/DAG assertions pass, counts above. |
| `git rev-parse` of both accepted session document paths at main and HEAD | Both exact blobs above unchanged. |

No native, feature, crypto, provider or client operation ran. This review does
not independently recheck the coordinator's historical remote CI/rules/PR claims.
Root must integrate this review, update current metadata/render, run applicable
final checks and satisfy latest-head CI/current rules/resolved discussions/normal
merge. Only then claim SESSIONTEST-008 with a fresh author session. Every later
behavior retains reviewed specification-linked tests and intended red before
implementation, separate code/configuration review and relevant passing acceptance/
CI. Browser, DPAPI/QtKeychain, OS/package/security/live and full release gates remain.
