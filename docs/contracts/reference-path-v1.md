# Early reference path — logical profile v1, revision 1

Authority: specification v0.45 §17 and §§2–4/6/10/18, API-01/02/03, NUM-04.
Planning owner: CONTRACTPLAN-003; concrete follow-ups: CONTRACT-013/014.
This is a bounded logical contract proposal and independent oracle set. It is not
an OpenAPI schema, concrete C++ interface, running server or supported capability
announcement. Actual review is in the linked task/evidence, never implied by this
heading. [ADR-0006](../decisions/0006-reference-path-prerequisites.md) preserves
complete release scope and the exact dependency replacements.

## 1. Supported scope and refusal

Target one synthetic eligible contribution → durable identified receipt → one
retained, reviewed rule version → exact nonnegative time request → current
cap-limited actual credit → atomic committed timer state → authorized HTTP
snapshot. Use an isolated disposable installation/database and fake connector;
no live credentials/contributions, external delivery or provider claims.

The reference configuration has exactly one enabled synthetic contribution trigger
with one time-add action, no nested conditions, exclusion/priority competition,
execution cooldown, generated events, score/counter requests, money/FX, modifiers,
milestones, outbound actions or other enabled rules. Retain explicit empty sets
and versioned configuration for these categories. Refuse unsupported or unknown
configuration before intake/application enablement; never silently ignore an
existing rule/modifier/milestone that should have affected the input. This is a
reference-profile limit, not a removal from the product or a contribution limit.

The initial fixture is Active, Main positive, Grace retained, no deadline, no pause,
no debt and no recovery review. Server policy/clock adapters must prove those
facts current; test UTC and monotonic elapsed are separately controlled. The path
may not turn unsupported configuration into those facts. No Start/reopen, general
pause/Grace/debt/recovery/deadline command is advertised. Existing reducers remain
the authority for calculation; unsupported states return not_ready. A restart
cannot resume reference operations until the owning recovery checks establish
readiness; this profile supplies no recovery implementation or restart capability.

## 2. Bounded HTTP API

The proposed profile identifier is `reference.v1`. All common values import
[wire primitives v1](wire-primitives-v1.md), with no scene/WS imports. CONTRACT-013
must publish a complete source-controlled OpenAPI 3.1 package for ONLY:

| HTTP operation | Capability and meaning |
| --- | --- |
| POST /api/reference/v1/negotiate | Public nonsecret version negotiation; no session creation or state/existence disclosure |
| GET /api/reference/v1/timer | `reference.timer-snapshot.v1`: current authenticated, authorized projection of committed timer state |

There is no contribution submission, rule editing, state mutation, login, scene,
asset, WS or provider route in this package. SESSION-003's separately reviewed
session transport is required by the protected route; it does not become an
implemented endpoint by import. Synthetic input enters ONLY the trusted test
connector/authoritative queue, never an unauthenticated HTTP endpoint. Route absence
and capability omission must be tested. Do not register placeholder success routes.

Negotiation request has exactly `supported_versions` (1..16 unique common
ProtocolIdentifiers) and `required_capabilities` (0..16 unique identifiers).
Response has exactly `selected_version` and `capabilities` (unique identifiers).
Select by server preference among mutual versions; reject no intersection as
incompatible_version and unavailable required capability as unsupported_capability,
before protected operations. Concrete HTTP headers/status/error framing and auth
import must be finalized in CONTRACT-013; an unresolved auth shape blocks schema
completion, not permission checks.

Only actually implemented, tested and enabled capabilities can be advertised.
The proposed target registry contains only `reference.timer-snapshot.v1`; the
negotiation operation itself grants no authority. A partially built server must
omit that capability or decline reference.v1 until its required operations work.
No generic api.v1/scene/WS/modifier/milestone capability is implied. Unknown offered
versions may be skipped; unknown required capabilities fail. Compatible clients
hide unavailable features. API-004 and complete HTTP006 retain web assets and
broader supported-client combinations; the profile offers no web asset support.

Snapshot has exactly these required fields; omissions/extra fields are invalid:

| Field | Proposed wire representation |
| --- | --- |
| profile_version | literal `reference.v1` |
| state_revision | nonempty common OpaqueBytes, at most 4096 decoded bytes; opaque state token, never a parsed counter |
| lifecycle | literal `active` in this bounded profile |
| main_ms, grace_ms, debt_ms | canonical nonnegative signed64 decimal strings; debt is `0` for this profile |
| maximum_main_ms | required nullable nonnegative signed64 string; null explicitly means no cap |
| observed_utc_ms | canonical signed64 string, from the committed observation; never request time substituted for absent data |
| paused | literal false in this bounded profile |

Only a wholly committed coherent observation may supply these fields. Validate the
state token/config/observation together, using current authorized configuration for
cap interpretation. Unknown/unsupported/missing committed annotations are unavailable,
never a manufactured Active/zero/current timestamp. No supporter, contribution,
secret, actor or historical-result payload is disclosed by this projection.

Proposed fixed bounds, to be concretized and reviewed before decoder/schema code:
request wire bytes4096 (including GET metadata/body policy), response bytes65536,
JSON nesting4, total JSON values128, identifier bytes64, version/capability arrays16,
opaque revision decoded bytes4096/hex8192, numeric string bytes20, common failure
bytes1024. All objects are closed. These are new bounded control/projection fields,
not narrowed storage/reference/rational profiles. CONTRACT-013 must separately pin
HTTP header/framing, pre-materialization scan/scratch limits, method/content-type/
query/body policy, failure priority/status and exact auth transport/CSRF rules.
Do not implement a decoder from an unset bound. No floats, permissive prefixes,
BOM, duplicate keys, unknown members, malformed Unicode or integer JSON numbers.
Capability/version and schema validity never attest authority, commit or durability.

## 3. Receipt, rule and execution guarantees

Trusted synthetic adapter supplies stable connection/provider event identity,
original bytes and evidence states explicitly. Durable intake uses ACCT-001 and
STORAGE-013; normalized contribution/linkage uses ACCT-002/STORAGE-014 and captured
plans use ACCT-003/STORAGE-015. Preserve arrival-time immutable configuration,
original evidence, owned identity and full plan provenance. Never label simulated
confirmation as verified provider support. A duplicate is checked against retained
identity/original facts, not a new operation ID or current rule evaluation.

One immutable enabled rule version matches the synthetic contribution type and
requests its reviewed nonnegative signed64 milliseconds action. The action amount
is a configured rule value, not supplied as a precomputed caller award. Retain
rule/action/version, source occurrence, exact request and half-even finalization
provenance where a rational conversion is used. CONTRACT-014 must finalize typed
trigger/unit/amount/configuration and durable capture signatures/limits/error
priority before RULESTEST-006. No money parser/valuation or fabricated rounding
history for a directly configured integer. Changing the rule later never rewrites
the original capture or retries against today's rule.

RULES-006 retains ACCESS-003. Editing a rule cannot grant its time action; execution
checks actual current automation capability for the FULL requested amount, target
and source scope. HTTP uses real current SESSION-003/ACCESS-002 decisions on the Qt
execution boundary, including queued requests and read/result disclosure after
revocation. Unknown authority fails closed. Synthetic, loopback, owner name, client
principal flag, cap-limited amount or earlier successful preview grants nothing.
Tests use explicit fake identity facts; production adapters cannot return allow
merely because the reference mode is selected. No new auth service/shortcut.

API-011 supplies actual current access/policy/segmented-clock/required-actions and
fresh-reference ports to ACCT-004, binding source/plan, current state/configuration,
permission/policy revisions and separate UTC/elapsed observation. Required actions
can be empty only when the captured/current configuration proves no such actions
are required. Unknown policy is not_ready. Fresh references retain uniqueness;
no constant token factory or fake successful completion. Elapsed advancement uses
CORE-002 before current-cap credit via CORE-005, in occurrence order, with checked
arithmetic and actual effects. Existing accounting handles its exact request;
reference adapters must not use a float or bypass the service with raw SQL.

## 4. Atomic state, duplicate and publication boundary

Retain intake/capture durably before the application transaction. One authoritative
Qt queue and controlled Qt SQL writer owns application. ACCT-004/STORAGE-002 commit
claim/status, effects (requested/actual/capped), new timer snapshot, result, audit
and any required jobs atomically. With this no-outbound configuration, jobs are
explicitly empty; no non-deliverable annotation counts as an actual external job.
Do not split a timer update or success marker into a second write.

A normal result/snapshot consumes only actual acknowledged committed completion.
The read path uses the same authoritative queue/coherent committed read; it cannot
publish staged/session-private state. A known rollback leaves prior intake/capture
but no application claim/effect/new state/result/audit/job. A fresh snapshot may
still show the last committed state; this is not a successful application response.
Commit-unknown suppresses normal success and stops admission pending reconciliation;
never invent a retry key or automatically replay. A post-COMMIT body exception also
suppresses normal success despite durable accounting. Failure after normal commit
publication does not reverse accounting. A matching authorized duplicate returns
historical disposition only after its own normal acknowledged read completion,
without applying credit or incrementing state again. Current disclosure checks
precede historical lookup/return. A new operation key cannot evade the source claim.

Persist the timer on EVERY timer-changing application. This replaces only
operation-time state persistence from STORAGE-003. Periodic checkpoint cadence,
portable clock lineage, restart/downtime/recovery/backup and operational durability
remain STORAGE-003 and its owners; read-after-reopen of raw SQL is not product
recovery. End-to-end tests must exercise real HTTP and SQLite transactions with
disposable state; in-process JSON or fake writer success alone cannot finish API-008.

## 5. Independent literal path examples

These expected values are hand derived from the specification, not observed tests.
All rows use a real disposable database, synthetic identity e1, immutable rule r1,
request600000ms, Active Main3480000ms, debt0, Grace120000ms, cap3600000ms,
paused=false, observation UTC1000/elapsed0 and no additional enabled features,
unless overridden. `a` and `b` denote distinct stored tokens (wire `61` and `62`).

| ID | Input/fault | Required observation |
| --- | --- | --- |
| RP01 | First e1/r1 | Receipt durable first; request600000, actual120000, capped480000; state a→b/Main3600000/debt0/Grace120000; one claim/effect/result/audit, zero jobs; authorized HTTP snapshot b/3600000 only after commit |
| RP02 | Exact retry after RP01 | No new award/effect/state revision; same historical actual120000; HTTP snapshot remains b/3600000 |
| RP03 | Same e1 through new operation ID or concurrently queued duplicate | Source/contribution claim prevents second application; one total actual120000; no second success-as-new or state revision |
| RP04 | Fault after application/state staging before COMMIT | Actual acknowledged rollback; earlier receipt/capture durable; state a/Main3480000, no claim/effect/result/audit/job and zero application success publications; authorized GET may return unchanged a |
| RP05 | Explicit permitted retry after RP04 rollback | Revalidate current rights/state/cap, then apply once exactly as RP01; original receipt retained, no guessed replay after unknown |
| RP06 | COMMIT applied but acknowledgement unknown | No normal application response/success/snapshot derived from tentative work; writer fails closed; no automatic replay/new key; unknown outcome distinct from rollback |
| RP07 | COMMIT acknowledged then outer work body throws | Accounting durable once, no normal success publication; later independently authorized read resolves committed b without another award |
| RP08 | Permission revoked while queued or before duplicate disclosure | Deny, no private result; no new application; request600000 cannot fit a grant allowing only120000 even though actual cap permits120000 |
| RP09 | Cap changes to3540000 before execution | Current authorized config bound; actual60000/capped540000; snapshot Main3540000; never preview-derived120000 |
| RP10 | Current Main already3600000 | Actual0/capped600000, one committed application with distinct after-token; duplicate still awards0 and creates no second application |
| RP11 | New r2 request900000 after r1 capture | e1 remains request600000/r1, actual120000/capped480000; later distinct input uses its own captured version |
| RP12 | Unsupported modifier/milestone/rule/pause/deadline/recovery or missing current-policy facts | Explicit not_ready/unsupported admission, no success or silently omitted behavior, no fallback to reference defaults |
| RP13 | Anonymous/revoked snapshot, or unsupported version/required scene capability | No state disclosure; incompatible/unsupported before protected operations; scene/WS capability omitted and no placeholder scene route |
| RP14 | Request-sized overflow/resource/malformed primitive/configuration | Explicit failure/owning NUM-04 durable hold when applicable; no truncation/rounding/partial application; concrete limits/error oracles belong to CONTRACT013/014 |

RP01 wire projection (literal expected data, not generated by a future serializer):

```json
{"profile_version":"reference.v1","state_revision":"62","lifecycle":"active","main_ms":"3600000","grace_ms":"120000","debt_ms":"0","maximum_main_ms":"3600000","observed_utc_ms":"1000","paused":false}
```

## 6. Remaining gates

This logical profile/planning change requires actual independent review. Concrete
OpenAPI/schema/auth/framing bounds (CONTRACT013), typed rule/current-adapter contract
(CONTRACT014), each actual prerequisite baseline/implementation and APITEST008's
reviewed real HTTP/SQL baseline/red all precede dependent production behavior.
Initial review cannot approve an unwritten schema/interface/test. Full HTTP/WS/
scene/modifier/milestone/recovery and every release requirement remain as ADR0006
records. See state/evidence for actual review and observed commands; none are
inferred from these examples.
