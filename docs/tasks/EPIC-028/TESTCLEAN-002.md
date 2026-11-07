# TESTCLEAN-002 — Remove environment foundation programs

Module: Test infrastructure. Kind: cleanup. Specification: v0.45; §17.
Contract revision: current test strategy and v0.45. Parent: EPIC-028. Dependencies: none.
Shared-file ownership: test-only `*_environment.cpp` programs and their registrations in
`tests/acceptance`, `tests/qt`, `tests/CMakeLists.txt`, and relevant CMake label files.

## Scope

Remove environment/foundation executables that assert third-party library facts, fixture
physical validity or test-harness behavior instead of the product. Include the Boost,
libsodium and SQLite/Qt environment programs identified during review. Preserve actual
behavior tests and any focused product capability check that is not already exercised by
the owning suite.

## Acceptance criteria

- Inventory every test-only environment target and its registration before removal.
- Remove redundant executable sources and their scaffold registrations without deleting
  behavior assertions or hiding failed dependencies in product tests.
- Retain a single focused SQLite capability probe only if the independent reviewer finds
  it establishes a product prerequisite not already checked by the writer suite.
- Build the affected targets and run focused tests; a missing library/compile failure is
  not a valid test result.
- A separate reviewer approves the deletions and confirms the real behavior suites still
  fail meaningfully when their dependencies or product behavior are wrong.

## Verification and handoff

Record exact commands, outcomes and environment in the PR. Follow the workflow for the
500-character task note, area summary, PR evidence and historical archive.
