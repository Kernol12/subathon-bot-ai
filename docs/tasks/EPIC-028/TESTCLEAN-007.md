# TESTCLEAN-007 — Trim libsodium call-counting tests

Module: Identity tests. Kind: cleanup. Specification: v0.45; §§10, 17.
Contract revision: current password-hashing and session-resolution contracts.
Parent: EPIC-028. Depends on: TESTCLEAN-003. Shared-file ownership: password hashing and
session resolution acceptance tests plus any linker-wrap test setup they exclusively use.

## Scope

Remove exact `sodium_*` call counts and ordering assertions that freeze implementation
details rather than specify security behavior. Keep the relevant failure handling,
zeroization and allocation-lifetime checks, and retain behavior required by the password
and session contracts.

## Acceptance criteria

- The tests still prove required buffers are cleared, sensitive allocations are released,
  and hashing failures are handled without disclosing secrets.
- Wrong-password, session expiry and permission behavior are not silently claimed as
  covered by these storage-level tests; uncovered behavior remains explicit follow-up.
- Drop linker wrapping only where no retained security assertion needs it.
- Run focused identity tests; a separate reviewer approves the exact assertion changes.
