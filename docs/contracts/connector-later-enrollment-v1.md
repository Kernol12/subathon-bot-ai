# Later connector source enrollment

Draft for [API-034](https://github.com/Kernol12/subathon-bot-ai/issues/815). This boundary connects the startup policy writer, current authorization, and durable connector intake. It lets API-006 S10 obtain its target source from the shipped producer.

## Starting state and request

Start with the shipped API-031 registration, activation and live checkpoint for A alone. Registration creates policy 1 and a pending A. Activation appends policy 2 and makes A live. Enrollment requires a live A with a proved cut and frontier on the current policy N (N = 2 for S10). A pending A receives a typed refusal with zero writes. Read the current installation policy document and membership, A's scoped source row and fence, and the absence of B. A and B are distinct full source scopes in the same installation. The requested enrollment names B's exact scope, intended member metadata, configuration revision, profile and cursor contract, and the expected current policy and A fence. The retry carries the same exact intent and expected before-image.

Resolve authority inside the writer scope for the enrollment action and exact affected scopes. Caller-supplied revisions, registry names and permission references are claims. Compare them with current owner records before accepting them. Treat a denied, unavailable, stale or contradictory proof as a refusal with zero writes. Reject a policy document that does not decode as the exact framed immutable policy selected by the startup policy writer. An unframed policy also fails closed at catch-up.

B is enrolled only when its configuration and profile binding are current and its generation is eligible for new enrollment. No provider fetch, import, cut, frontier advance, automatic credit, or handoff occurs in this operation.

## Atomic readback

| Case | Durable result |
| --- | --- |
| A-only live startup and current, authorized B intent | Commit the exact projection below in one writer unit. |
| Exact retry after a committed transition or lost acknowledgment | Re-read and establish the exact committed projection under current authority. Return the original outcome, with no second enrollment or effects. |
| Same expected transition with changed intent | Refuse as a conflict. |
| B already exists without evidence for this exact enrollment | Refuse. B's mere presence does not prove the operation committed. |
| Stale policy, A fence, registry binding or configuration | Refuse. Leave the full before-image intact. |
| Check fails or the SQLite work rolls back | No partial membership, source row or enrollment evidence becomes visible. |
| Later work supersedes the committed projection | Return scoped superseded evidence, or unavailable if current proof is insufficient. Do not relabel a proved commit as refused. |

For API-006 integration, establish the exact A-only setup through the shipped API-031 operation first. Enroll B through this producer, then read the exact durable B head through its owning read path. Keep the existing S10 single-source setup, handoff and accounting assertions. API-029 continues to own handoff qualification and API-030 continues to own original-receipt recovery.

## Before and after rows

Read the policy head and A fence inside the writer unit. Require policy head N and A's exact current scope, epoch, generation, source revision R and policy revision N. Check the next policy and source revision fit before writing. Apply A's CAS and B's absence check in that unit.

| Rows | Before | After |
| --- | --- | --- |
| Policy history and members through N | Immutable reviewed rows | Identical bytes and membership |
| Policy N+1 | Absent | One framed immutable policy from the startup policy writer. Preserve A's authority policy, catch-up mode and references. |
| Members at N+1 | Absent | Copy every member from N byte for byte apart from its policy key. Add B with the reviewed scope and metadata, no eligible primary slot and no award authority. A remains the sole eligible primary for the affected overlap set and effect scope. |
| A current source | Live, source revision R, policy N | Live, source revision R+1, policy N+1. Preserve its scope, epoch, generation, captured capability, cut and frontier. |
| B current source | Absent | Pending with initial epoch and source revision 1, bound to N+1 and its exact enrolled generation and cursor contract. Cut and frontier are null. |
| A's committed history, gaps, intake and accounting | Existing immutable inputs and effects | Unchanged, including each row's recorded authority-policy revision. |
| Operations | Existing rows | Unchanged. Enrollment uses the shipped projection readback pattern. |

New A work uses the advanced fence. Work carrying R or N is stale under the existing CAS rules. The later S10 handoff alone may transfer award authority. Enrollment has no schema or operation-purpose change.

For an ambiguous commit, resolve current authority again and compare the immutable N+1 policy and complete membership, exact pending B source, and exact A after-image including the advanced fence. If all match, return the committed projection. If N+1 and B are absent and the complete original projection still matches, report absence at the original before-image so only the same intent can retry. Partial or divergent evidence cannot establish either result. Report a typed conflict for a proved incompatible transition and unavailable when the evidence is insufficient. If subsequent work has advanced the sources, report superseded only when immutable evidence proves this exact transition and the current fence no longer equals its after-image. Otherwise report unavailable and do not reapply the operation.
