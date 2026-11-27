# Later connector source enrollment

Draft for [API-034](https://github.com/Kernol12/subathon-bot-ai/issues/815). This boundary connects the startup policy writer, current authorization, and durable connector intake so API-006 S10 can obtain B from a shipped producer.

## Admission and authority

Start from API-031's shipped A-only registration, activation, and live checkpoint. Registration writes policy 1 with pending A. Activation writes policy 2 and opens A's live cut. Enrollment requires live A on the current policy N, with its qualified cut and committed frontier (N = 2 for S10). A pending source or missing qualified cut or frontier returns `IntakeStatus::unproved` with zero writes.

The request binds the installation, exact distinct A and B scopes, expected installation head N, A's current fence and source facts, and B's exact configured enrollment intent. Add the C++ authority action `StartupAction::enroll_source`. Resolve current authority inside the writer for B and for the installation policy append, on both initial execution and readback. Denied and unavailable authority return their respective typed outcomes with no writes or disclosure. Validate claims against current owner configuration and registry. A mismatched head, A fence or profile/configuration reference is `stale`. Invalid scope or unframed policy input is `invalid_input`. Catch-up also fails closed for an unframed policy. Use the existing writer and document limits and refuse exhausted revisions or oversized input with `limit_exceeded`.

## Atomic write and byte identity

In one writer unit, verify the installation head and exact A before-image and check B's absence by its full installation/provider/account/stream key. Append the new policy and complete membership, then insert B. Existing B without exact replay evidence is `conflict`. Concurrent admissions serialize through the writer. There is no A CAS write, schema migration, or new durable operation purpose.

| Rows | Before | After |
| --- | --- | --- |
| Policy history and members through N | Existing immutable rows | Byte-identical. |
| Policy N+1 | Absent | Existing framed representation `frame(mode from N) + frame(enrollment configuration revision) + frame(enrollment permit decision)`. Use the startup policy writer's canonical framing. |
| Members at N+1 | Absent | Copy every N member unchanged except its revision key. Insert B's exact scope, award class, effect scope and overlap set from the validated enrollment intent. For S10 those values are `individual/main/overlap`. B's `primary_slot` is SQL NULL. Its document is `frame(lane_ref) + frame(execution_principal) + frame(profile_revision) + frame(cursor_contract)`, the shipped `configured_member_document` encoding. |
| A current source | Live on N | Byte-identical, including epoch, generation, source revision, policy pointer, capability document, cut and frontier. |
| B current source | Absent | Pending, fence `{1,1,N+1,gen_B}` in epoch/revision/policy/generation order. Store the exact enrolled cursor contract in the source document. Cut and frontier are SQL NULL. |
| A history, open gap G, active catch-up O, intake and accounting | Existing rows | Byte-identical, including their recorded revisions. No operations are inserted or updated. |

The immutable enrollment evidence consists of the installation and N+1 key, inherited mode, enrollment configuration revision and permit decision, complete copied membership, and B's scope, award class, effect scope, overlap set, primary eligibility, lane, principal, profile and configured cursor contract. A's exact fence and source facts are checked from A's current row. B's enrollment generation, current fence, pending state, cut/frontier and actual source document are proved by B's mutable row. A later handoff can replace B's generation and source document. Do not infer lost enrollment values from B's later state.

A remains the sole eligible primary for the affected overlap set and effect scope. A's authority keeps resolving against its installed revision N. Since B does not appear in N, B has no effect on A's live admissions, overlap or disjointness checks before handoff. No cut or prior authority is superseded on A, so r3 row 175's historical preservation requirement is met with the retained N records.

## Readback and retry

| Case | Result |
| --- | --- |
| First complete authorized write | `IntakeStatus::committed` with the exact projection. |
| Acknowledgment lost or exact retry | Re-resolve current authority. Compare the exact intent bytes, including the newly resolved permit decision, all immutable N+1 evidence, A's unchanged before-image, and B's exact pending row. A match returns `IntakeStatus::exact_replay` with the projection and zero writes. |
| Same expected transition with differing intent bytes | `IntakeStatus::conflict`. A newly resolved decision with different bytes cannot establish exact replay. |
| N+1 and B absent and the complete before-image still matches | Report absence at the expected before-image. A complete retry may proceed through normal admission. |
| Partial, incompatible or superseded state | Return `conflict` for proved incompatible evidence. Report superseded only if the immutable evidence establishes the exact intent and the current rows establish that a later transition replaced its projection. If required intent bytes are no longer provable, return `unavailable`. Never apply the enrollment again on incomplete evidence. |
| Retry of A's already committed API-031 checkpoint after N+1 is appended | The shipped write retry returns `Status::stale`. Inspection returns `Status::stale` with `StartupReadback::superseded` because the installation head moved. A's source, frontier and operation remain unchanged. Read A's current source state to recover the committed frontier. Do not retry the checkpoint write. |

## S10 setup and handoff

In #288's tests stage, establish the live A-only setup using shipped API-031 with the existing single-source and setup assertions intact. Commit and inspect original H under live A first. Then enroll B with `gen_B = "g4"`, re-read `previous` from A and read the target from the exact durable B head, replacing the synthetic target fence literal in setup. Keep every assertion and assertion literal. Any required assertion change needs a separately approved revised baseline. #815 does not edit S10's fixture or assertions.

For the handoff, use the installation head N+1 and each source's own readback fence. A's open G and O still match A's fence at N, while B's pending fence points to N+1. The shipped writer checks the installation head separately and validates each active operation against its own source fence. With the existing qualified new-only proof and all other handoff preconditions met, #288 can transition both sources and award memberships together to N+2. API-029 owns handoff qualification, API-030 owns original-receipt recovery, and API-031 remains unchanged.
