# TESTCLEAN-008 — Merge regression executables and simplify test labels

Module: Test infrastructure. Kind: cleanup. Specification: v0.45; §17.
Contract revision: current test executable and label contracts. Parent: EPIC-028.
Depends on: TESTCLEAN-004 and TESTCLEAN-007. Shared-file ownership: standalone
`*_regression_tests.cpp` suites, their CMake registrations/labels and test comments.

## Scope

Merge meaningful standalone regression cases into their owning behavior suites; remove
the stale `draft` label on `core_credit_acceptance`; and replace opaque comment ID bundles
with one sentence stating intent. This task organizes tests; it does not authorize
deleting a regression assertion just because it records a previously fixed defect.

## Acceptance criteria

- Every retained regression runs in the owning suite and remains discoverable under the
  intended labels; remove only duplicate or obsolete infrastructure, with rationale.
- `core_credit_acceptance` has accurate status/labels and is not made green by hiding red
  behavior tests.
- Comments explain why a test exists without opaque internal decision-ID lists.
- Relevant tests and CTest labels run; separate review confirms behavior and failure
  visibility are preserved.
