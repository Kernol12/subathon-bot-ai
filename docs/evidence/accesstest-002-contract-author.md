# ACCESSTEST-002 partial logical document author checkpoint

Author: service_author, 2026-11-04. Dedicated
`task/ACCESSTEST-002-contract-author`, worktree
`local-evidence/worktrees/accesstest002-author`, initially clean at claim
`d3a664a00e4de787707f66b9cd689ec8b89d619f` from refreshed main `b2b14bb`.
Owns only the new logical proposal and this note. This is a bounded PARTIAL author
checkpoint, not a completed/reviewed logical contract, baseline or implementation.

## Artifact, inputs and scope

[Effective-permissions proposal](../contracts/effective-permissions-v1.md), Git blob
`5dfc6a1e6cff3877a4d2874a26d29d0e6f7f44ed`, preserves unambiguous requirements while
D1 remains open. Read root/core/server instructions, architecture/workflow/
test strategy, task claim/checkpoint, specification v0.45 §10/AUTH-03/AUTH-06,
[ACCESS-002](../tasks/EPIC-006/ACCESS-002.md),
[ACCESS-003](../tasks/EPIC-006/ACCESS-003.md), common values and application boundaries.
Actual input blobs remain unchanged:

| Input | Git blob |
| --- | --- |
| [Specification](../specification.md) | `3e008efaef3181dd8b06a038f6309347ec31619d` |
| [Common wire values](../contracts/wire-primitives-v1.md) | `11eeb5f422a174e042143df6b476ae1d717eeacf` |
| [Event/state boundaries](../contracts/event-state-boundaries-v1.md) | `8b60da43f1362c46c54730e8ca029b858964ea59` |
| [ACCESS-002 brief](../tasks/EPIC-006/ACCESS-002.md) | `af0eb841abe59e6389acda66ff5aba0de372ea22` |

The partial proposal describes exact opaque actor/role/source/grant/operation
identities, current owner separate from editable roles, declared operation resource/
quantity dimensions and exact inclusive constraint matching. One complete grant
must cover the complete request; no tuple union, cap-limited-actual shortcut,
floating-point/rounding or role-label authority is introduced. Individual precedence,
role deny over allow and default deny are preserved where applicability is
unambiguous. A stable whole witness/source explanation is proposed, with current
disclosure/queue/revision guards remaining server responsibilities.

Protected ownership retains access-management/recovery separately from editable
roles; it gives no general timer/layout/provider bypass. Owner transfer/fresh-auth,
console/owner recovery and protection against helper-reset takeover retain their
actual account/session workflows. The exact protected operation catalog is a required
integration seam, not a client is_owner/protected flag or an invented implemented
capability. ACCESS-003 owns delegation/editing/automation authority; application
session authentication and chat category paths are not silently replaced here.

Proposed local defaults are 4096 decoded reference bytes, 16384 decimal component
bytes, 4096 total input entries, 16MiB owned input and 32MiB/4096 owned explanation
output. These are configurable finite evaluation profiles, not universal transport
or storage caps. They preserve existing STORAGE-015/common value domains and do not
borrow a 256-byte correlation, 512-byte decimal parser or 4096-bit finalizer bound.
Derived comparison scratch is conservatively bounded from the selected component
profile; concrete byte/item/arithmetic/preallocation rules still need API review.
No dependency beyond existing std/selected Boost is proposed.

## D1 is a real pending product decision

Specification §10 states individual explicit allow/deny first and one complete
applicable constrained grant, but does not define what selects the individual tier
when its resource/quantity constraints only partially match. No prior recorded owner
clarification was found. Root has asked the owner asynchronously; no actual answer
has arrived. This is a requirement choice, not routine implementation permission.

For an individual A/max5min plus role A/max10min, request A/+10min differs under:

1. Resource scope selects individual override first: deny A/+10; role grants for
   other resources remain usable.
2. Individual policy replaces the whole operation: deny A/+10 and suppress role
   grants for other resources of that operation too.
3. Only a complete-request match selects individual policy: role A/+10 may allow.

The document records all three, including the separate timer-B discriminator.
No option is chosen by naming, conservatism, elapsed time, resource profile or a
default implementation. The final scope-versus-other-constraint distinction and
constrained individual-deny treatment must follow the actual answer. D1-A/B are
explicitly pending cases with alternative consequences, not frozen test oracles.
Root requested this partial checkpoint before reassigning unrelated work. Permission
task completion/full review/API/tests/production remain blocked on the actual D1
answer; the agreed full permission/delegation/owner release scope is unchanged.

## Independent examples and actual checks

EP01–27 are independently written unambiguous requirements/boundary examples:
default deny/zero, individual versus roles, role deny precedence, A5+B10 nonunion,
whole multi-quantity tuple, exact fractions/signed minima/large Exact, protected
owner scope, role-label nonauthority, owner-target recovery protection, revocation/
transfer after preview, unsigned opaque ordering, exact limits, unavailable/invalid
input and complete explanation budgets. These are document examples, not fixtures,
tests or observed behavior. The two pending D1 cases remain explicitly unresolved.

Read the primary [OWASP authorization guidance](https://cheatsheetseries.owasp.org/cheatsheets/Authorization_Cheat_Sheet.html)
for its distinction between authentication/authorization and default-deny/per-request
checks. Product-specific precedence and the unresolved D1 policy derive from the
repository specification, not an external framework's defaults. This read-only
documentation check certifies no running security boundary or provider behavior.

Actual author checks, Linux/Python 3.14.8:

- `python3 tools/check_project.py`: PASS, 406 task records/122 decisions, local
  links/presets/current HTML. No feature test or evaluator is invoked.
- `python3 tools/progress.py render --check`: PASS; HTML/state are unedited.
- Inline Python stdlib fractions/Path/re arithmetic and byte audit: 18 literal
  facts PASS, 27 unique EP examples, balanced fenced blocks and local links PASS.
  Facts include 5/10min =300000/600000ms, exact300000.5 comparison, signed64-min
  magnitude, 2^64, unsigned byte sorting, NUL versus empty, 4096/4097 reference
  lengths, a 5001-byte/16610-bit component and derived131072-bit work capacity.
  No authorization algorithm, SUT helper or oracle generator was implemented/run.
- Git status/exact input blob reads and `git diff --check`: PASS. Final sole-owned
  two-path staged inventory/whitespace checks accompany the commit.

No API/header/stub/schema, parser/evaluator, fixture/test/feature code, compilation,
native/crypto/auth/session/network-provider operation or product runtime was created
or executed. Password source/tests under different review were not edited. No shared
state/HTML/navigation/maps/build/CI/pin/schema or credential changed; no push/PR/main
merge was performed. Source/render/link checks are documentation infrastructure only.

After an actual D1 answer, update its dependent definitions/examples and obtain
different actual logical review. Only separately released/reviewed API, inert source/
harness, independent tests/environment and FIRST intended behavioral red can precede
production ACCESS-002. Actual owner catalog/account/session loading, delegated edits,
automation, current execution/CAS/disclosure and platform/transport verification
retain their owning tasks. This author claims no self/future approval or completion.
