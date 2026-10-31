# ACCTTEST-001 authoring handoff

Author: `/root/receipt_test_author`; date: 2026-10-31.
Branch: `task/ACCTTEST-001-receipt-author`; base main: `806f69f`.
Coordinator claim/integration branch: `task/ACCTTEST-001-receipt-baseline`.
Specification: v0.45 §§3.2–4,14,18. Contract: [receipt intake revision 1](../contracts/receipt-intake-v1.md),
refining the independently reviewed event/state revision 1 receipt boundary.

## Authored scope and decisions

Added standard-library receipt values; a framework-free server intake/repository
port; an explicit `ReceiptIntakeNotImplemented` service stub; a disposable primitive
transaction fake; and 15 GoogleTest acceptance cases with synthetic namespaces.
The [task brief](../tasks/EPIC-002/ACCT-001.md) records detailed file ownership and
acceptance criteria. Coordinator controls CMake/presets/pins, progress/HTML/map,
checkpoint, CI integration and focused PR publication; this branch edits none of them.

Identity uses provider + stable source/account + documented identifier namespace +
opaque ID. Transport connection and configuration/context are per-attempt evidence.
The original facts/arrival are immutable; equal facts retry, differing facts conflict
with the full observation retained. Revision 1 deliberately compares all normalized
projection fields, including optional presence, display name and source decimal
precision; this conservative consistency policy requires explicit reviewer approval.
Payment/activity references are linkage evidence and never automatic event identity.
No contribution creation, confirmation eligibility, credit or source authority is
provided. The complete examples, oracles and remaining policy gates are in the contract.

The fake supplies lookup, allocation, staged primitive writes and known pre-commit
failure rollback. It does not deduplicate, enforce uniqueness or compare facts.
Tests inspect committed records for first/retry/conflict provenance and failure
atomicity; expected decisions/values are literal contract examples and preservation
invariants. No production algorithm generates expected results.

## Checks and exact artifact gate

Test bodies have **not run** during authoring. Independent test approval must precede
the first intended runtime red; successful compilation alone is not red evidence.
Linux development environment: GCC `16.2.1 20260810`, clang-format `23.1.4`, installed
GoogleTest headers. The coordinator supplies CMake harness integration separately.

Actual author checks in the dedicated worktree on 2026-10-31:

| Command | Actual result |
| --- | --- |
| `g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Icore/include -Iserver/include -c server/src/receipt_intake.cpp -o /tmp/accttest001-receipt-intake.o` | exit 0; stub translation unit compiles, no diagnostics |
| `g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Icore/include -Iserver/include -c tests/acceptance/receipt_intake_tests.cpp -o /tmp/accttest001-receipt-tests.o` | exit 0; all test/fake interfaces compile, no test body execution |
| `clang-format --dry-run --Werror core/include/subathon/core/receipt.hpp server/include/subathon/server/receipt_intake.hpp server/src/receipt_intake.cpp tests/fakes/receipt_repository.hpp tests/acceptance/receipt_intake_tests.cpp` | exit 0 |
| `git diff --check` | exit 0 |
| `rg -c '^TEST_F\(' tests/acceptance/receipt_intake_tests.cpp` | exit 0; 15 authored cases |

The exact committed artifact revision/hashes are supplied to the separate reviewer
at handoff and must be recorded in their review evidence. No CMake configure/build,
discovery or runtime acceptance outcome is claimed by these direct compile checks.

The intended red is `ReceiptIntakeNotImplemented` from the service after successful
configure/build/discovery. No failing compilation, missing dependency, skip or fake
policy failure qualifies. The author cannot approve this baseline. Independent review,
runtime red evidence, latest integration checks/CI and PR/merge status remain open.

## Limitations and next action

No provider calls, real identifiers, credentials or supporter data were used. Missing
stable identity and cross-provider authority/correlation remain API-006/connector
gates. SQL uniqueness, SQLite commit/durability/error resolution, concurrency/restart,
Qt SQL connection ownership and the authoritative queue remain STORAGE-002/API gates.
Fake service recreation is only a repository-consultation test. All integrations,
exchange providers and agreed product/release scope remain required.

Next: coordinator compiles the explicit lane without executing bodies; a different
reviewer examines the actual committed contract/header/stub/fake/tests; resolve any
findings, retain exact reviewed hashes and then observe/document the real intended red.
ACCT-001 production implementation remains gated on that completed baseline and its
own separate code review/passing acceptance checks.
