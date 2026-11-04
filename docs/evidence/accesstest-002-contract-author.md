# ACCESSTEST-002 logical document author handoff

Author: `service_author`, 2026-11-04. Current isolated branch
`task/ACCESSTEST-002-policy-author`, worktree
`local-evidence/worktrees/accesstest002-policy-author`, inspected clean at
`4429cbb869e5a820a675fc1d72d4110ff207b995` before editing. Owns only the
[logical proposal](../contracts/effective-permissions-v1.md) and this note. This is
an authored complete logical candidate awaiting different actual full review;
it does not complete the test-baseline or production task.

## Actual history and inputs

The original author checkpoint `6bc3d51f`, imported as `c88bcf5`, deliberately left
D1 unresolved. Its contract blob was `5dfc6a1e6cff3877a4d2874a26d29d0e6f7f44ed`;
its author-note blob was `66edd21fa135309c1cafc113eec6c9c9e5b3538c`.
Those historical artifacts remain in Git. The different partial review
`b87e16b603a53289b1d31cdf827a60293185b29b`, imported as `f2b98d4`, approved only
settled requirements. Its [note](accesstest-002-contract-review.md), blob
`20919ad37abb4536327a3d711c69ec7cbaaaca04`, is unchanged. No earlier
approval is extended to the completed candidate by this author.

The owner subsequently accepted the coordinator's resource-scoped recommendation
after the explicit A/B explanation. The actual
[owner record](accesstest-002-owner-decision.md), blob
`34168d4e5f82a524e9459b13efe055ef6786af8c`, and updated specification
resolve D1; no elapsed time, UI default or separate timer answer supplies that choice.
The previous unresolved alternatives remain historical evidence, not current policy.

| Input | Actual unchanged Git blob at this handoff base |
| --- | --- |
| [Specification](../specification.md), §10/AUTH-03/AUTH-06 | `9e7c41c8a0cbd1b284111143d215553333c2f6ab` |
| [ACCESS-002 brief](../tasks/EPIC-006/ACCESS-002.md) | `af0eb841abe59e6389acda66ff5aba0de372ea22` |
| [Common wire values](../contracts/wire-primitives-v1.md) | `11eeb5f422a174e042143df6b476ae1d717eeacf` |
| [Event/state boundaries](../contracts/event-state-boundaries-v1.md) | `8b60da43f1362c46c54730e8ca029b858964ea59` |

Read root/core/server instructions, workflow, relevant architecture/test-first and
common/application boundaries, the complete original logical proposal and actual
partial review, task brief, owner record and amended specification. Inspected the
actual ACCESSTEST-002/ACCESS-002 state records; root retains their ownership.

## Current logical revision and rationale

The revised contract blob is `3d82bfa418bb14099ee130fca7a400adca66500b`. Named operation-declared resource slots
each bind one opaque resource reference; complete grant scopes constrain every slot
with Any or an explicit finite allowed set. Quantity slots remain separate exact
values/units with explicit constraints. Global operations have a declared empty
resource tuple, while a missing required binding is invalid. Several resource slots
must fit one whole tuple; no fragments select a tier or manufacture a grant.

An individual operation/resource match selects the individual tier before quantity
limits. A failed limit denies within that tier and cannot use role fallback. Other
resources retain role deny-over-allow/default-deny resolution. D1-A is now denial
for A/+600000 with individual A/max300000; D1-B is role allowance for B/+600000.
EP01–27 are preserved byte for byte, retaining their prior valid partial review.

The bounded representation choices make the accepted policy unambiguous without a
specificity hierarchy. Same-effect individual alternatives may overlap and still
need one complete fitting witness. Opposing individual allow/deny resource scopes
are contradictory input even if their quantity intervals are disjoint or the
current request lies elsewhere; validation rejects them instead of choosing a
narrower entry or same-tier precedence. Disjoint opposing scopes remain valid.
All supplied entries remain subject to validation, including losing/off-scope rows.
The finite overlap check compares per-slot intersections, not an expanded Cartesian
product; the later API must freeze checked work and validation/error priority.

Deny entries use one complete resource/quantity tuple. A fully matching individual
deny is explicit denial. A resource-selected deny whose quantity fails still gives
no complete individual allow and no role fallback. A constrained role deny must
fully match to defeat a complete role allow. This follows the selected individual
tier boundary and existing role precedence without inventing positive authority
outside a denial's condition. There is no new policy selection pending for D1.

EP28–45 add focused inclusive/one-over boundaries, conditional individual/role deny,
same-effect overlaps, conflicting/disjoint individual scopes, whole multi-resource
scope selection/nonunion, global empty shape and malformed complete shapes. The
multi-resource operations are expressly illustrative typed matching obligations,
not a newly implemented batch/transfer feature. Source explanations distinguish
resource selection from quantity failure and record suppressed roles/complete
witnesses/decisive denials in stable unsigned-byte order.

Protected-owner rights, no owner-all-operations bypass, helper-reset protection,
opaque NUL/high-byte identities, exact long values, requested-versus-actual checks,
finite local resource profiles, before-output ownership projection, revocation and
current execution/revision boundaries remain. ACCESS-003 owns edits/delegation and
automation; SESSION/account tasks own authentication/recovery/current account facts.
No provider/category/client/preview/reference acquires authority. The existing
external OWASP citation and its prior read remain unchanged; product-specific D1
comes from the actual owner/specification record.

## Actual checks and limits

Environment: Linux, Python `3.14.8`. Documentation checks only:

- `python3 tools/check_project.py`: PASS, 406 task records/122 decisions, local
  links/presets/current HTML. Its informational draft-test count is not test execution.
- `python3 tools/progress.py render --check`: PASS; shared state/HTML unedited.
- Inline `python3 - <<'PY'` stdlib Fraction/integer/byte/set/Path/re audit: PASS,
  25 independent literal facts; 45 unique EP rows plus both fixed D1 rows; EP01–27
  match the original bytes; seven contract local links, per-table column counts and
  balanced fences. Literal checks include 5/10 minutes, exact 600001/2, signed-min
  magnitude, 2^64, NUL and unsigned-byte ordering, 4096/4097-byte references,
  5001-digit/16610-bit Exact, 108853-bit product within derived 131072-bit scratch,
  inclusive intervals and the explicit A/B/C/D tuple/intersection facts. No
  authorization evaluator or SUT-generated expected result was run. An initial
  documentation checker incorrectly assumed every table had three columns; the
  corrected check uses each table's own header width. No product/test assertion
  or fixture was changed.
- Read-only Git history/status/input-blob reads and `git diff --check`: PASS.
  The final staged two-path inventory/whitespace check accompanies the commit.

No API/header/stub/schema, parser/evaluator, fixture/test, compiler/native/SDK/auth/
crypto/session/provider operation or feature/runtime execution was authored or run.
No shared spec/state/HTML/navigation/maps/build/CI/pin/schema/credential or other
agent file changed. No push, PR update or main merge was performed by this author.

Different full logical review of the exact revision is next. Separate API, inert
source/harness, independent test/foundation review, selected environment and FIRST
intended behavioral red still precede production ACCESS-002. Actual protected
operation mapping, coherent account/session loading, delegation/editing/automation,
execution-time/CAS/audit/disclosure and transport/platform integration remain their
required owning tasks. Root owns CI/PR62 publication and all shared coordination;
this author claims no self-approval or future gate approval.
