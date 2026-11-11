# ACCESS-001 — Initial owner setup and invitation registration

Parent: [EPIC-006](../EPIC-006.md). Kind: implementation.
Specification: v0.45; §10, AUTH-04, AUTH-08.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Server account/bootstrap/invitation service using password hashing primitives; no automatic email delivery.

Primary module paths: core/ permissions and server/ identity services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Console issues single-use 15-minute owner setup code only while no owner exists; replacement invalidates prior code and completion disables bootstrap.
- Invitations default to 48-hour expiry, can be revoked and consumed once, and create accounts with inviter-authorized initial roles.

## Boundary and failure cases

Test concurrent consumption, expired/replaced codes, existing owner and attempted role escalation; local owner credentials remain required.

## Separate baseline and implementation tasks

- `ACCESSTEST-001` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `ACCESS-001` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest for pure permission/command rules; Qt Test for account transactions.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.


## ADR-0010 additive session prerequisites

[Session executable decomposition](../../decisions/0010-session-executable-prerequisites.md)
preserves all original scope and test-first/release gates. The authoritative task
graph adds the relevant supplier/lifecycle/store/local-login/clock/ingress prerequisites.
These components do not complete this task's own full acceptance or real integration.

## Local-login directory and host prerequisites

[Local-login authority](../../contracts/local-login-authority-v1.md) hands this
slice the actual bounded indexed username-to-stable-account directory and exact
binding/revision producer, including re-resolution in the caller's SAME writer
transaction. Define username grammar/normalization, uniqueness, rebinding and
revision semantics before tests; do not infer them from STORAGE-020, whose schema
has no usernames. ACCESS-001 owns this logical contract/producer; the coordinator
owns independent review and integration of any additive physical schema/migration,
preserving the existing prefix. Split further substantial storage work before code.
Test absence, collisions, concurrent binding/reset changes and stale revisions.

ACCESSTEST-001 and ACCESS-001 depend on the planned
[API-013](../EPIC-008/API-013.md) extension for the installation-wide password-work
permit and reviewed secret admission/erasure profile. Setup remains independent of
an accepted login, ACCESS-008 and any preexisting session. Other setup/invitation
requirements and owner fallback are unchanged.

## Current delegation and audit producer gates

APICONTRACT-003's guarded invitation paths require ACCESS-003's actual current
delegation evaluator before this slice's baseline/implementation, as recorded in
state. ACCESS-001 owns the same-transaction enrollment/access security-audit adapter;
ACCESS-005 owns its recovery counterpart. Missing producers refuse. Any substantial
physical prerequisite must be split and reviewed before executable expansion. This
planning clarification implements no policy/storage or host behavior.
