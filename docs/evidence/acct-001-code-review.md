# ACCT-001 independent receipt service review

Date: 2026-10-31. Implementer: `/root/receipt_implementer`.
Reviewer: `/root/baseline_reviewer` (separate from source and test authors).
Verdict: **source and configuration approved** at the exact revisions below.
Final status evidence, observed automatic CI and latest-head publication/merge
remain separate gates.

## Actual source and preserved baseline

Base: merged baseline main `7900a614214f72da85c31d44643cf32a078729cc` (PR15).
Reviewed source commit: `875c1c98a2b12e463528295db7a62f094dcb4d77`.
`server/src/receipt_intake.cpp` blob:
`7a0986bdd3e4b5e38d8b7ac18d76dd56ff002ee8`. Author evidence revision:
`30c9ad9344bba8367eed420f1610e068a107e8f9`. The source commit changes only the
service implementation (14 insertions,2 deletions), replacing the explicit stub.
Reviewer checkout after cherry-pick was `d9fdcabb34ce9659afe86ed3a6bf137d90ea45f7`.
This reviewer owns this document only and changed no source, tests or interfaces.

Authority: specification v0.45 §§3.2–4,14,18;
[receipt intake revision1](../contracts/receipt-intake-v1.md),
[ACCT-001 brief](../tasks/EPIC-002/ACCT-001.md) and the
[independently reviewed/red baseline](accttest-001-baseline-handoff.md).
Root/module instructions, architecture, workflow and test strategy were read.

The approved receipt values/header/tests/fake/behavioral contract remain unchanged:
core values `f11b0d60013d9606ccfc58188f4363814f26c13c`, server header
`273f6b4f1aa99f510779f13b965906ac17169e9e`,15 tests
`94317dc4dde4553c8888f78a433e3003c471de1a`, primitive fake
`99ff2eec97a10c89dc93f0aec5b8c4cee88906a8` and original contract
`149d65b5aa0864c6323e4d5ff6d9580cdbcf2886`. Baseline approval and the actual15-case
stub-red at `18d8670` preceded implementation. Historical baseline identities are
not replaced by later status prose.

## Code assessment

No blocking source findings in this bounded service.

- Each ingest opens exactly one repository-owned transaction, then looks up the
  entire identity. The key includes provider, stable source/account, documented ID
  namespace and opaque ID; it excludes connection, arrival context and payment/
  activity linkage evidence. The service delegates primitive lookup to the port.
- It creates an original only for an absent identity. Existing records retain their
  stable receipt ID; the service compares incoming facts with the original facts,
  including every defaulted equality field and optional absence/source precision.
  It does not compare against the last conflicting observation or replace original
  facts, first arrival or retained context.
- Every successful path appends a complete attempt containing incoming facts,
  arrival/connection/configuration/modifier context and the chosen created/retry/
  conflict disposition. No attempt is dropped for exact retries or equal timestamps.
- Commit follows the staged create/append and precedes the trivial value result.
  Exceptions are propagated without a success result, catch-and-continue or hidden
  retry. The owned transaction is destroyed on exceptional exit, supplying rollback
  according to the port contract. A new call rechecks repository state; there is no
  object cache, separate writer or publication before commit.
- The implementation uses no network/provider/framework call and changes no money,
  eligibility, contribution, effect or delivery state. Prevalidated inputs and the
  repository's consistent lookup/atomicity remain explicit adapter obligations.
  Known uncommitted failure simulation does not resolve ambiguous durable commits.

## Independent commands and observed results

Working directory: `local-evidence/worktrees/acct001-review`, isolated branch
`task/ACCT-001-code-review`. Linux; GCC16.2.1, Python3.14.8, CMake4.4.5,
installed GoogleTest1.18.0 and Boost1.92.0 for the separate numeric lane.
These commands were run independently of the implementer's reported replay:

| Exact commands | Observed result |
| --- | --- |
| `cmake --preset receipt-intake-debug` then `cmake --build --preset receipt-intake-debug` | exit0; configured and compiled actual source/tests |
| `ctest --preset receipt-intake-debug -L receipt-intake` | exit0;15/15 reviewed cases pass |
| `cmake --preset exact-numeric-debug` then `cmake --build --preset exact-numeric-debug` | exit0; installed Boost1.92.0 discovered |
| `ctest --preset exact-numeric-debug -L exact-numeric` | exit0;33/33 pass |
| `cmake --preset acceptance-debug` then `cmake --build --preset acceptance-debug` | exit0 |
| `ctest --preset acceptance-debug -L 'acceptance\|regression'` | exit0;23/23 pass |
| `cmake --preset scaffold-debug` then `cmake --build --preset scaffold-debug` | exit0 |
| `ctest --preset scaffold-debug` | exit0;3/3 pass, including tooling/project consistency |

Passing receipt cases exercise all three dispositions, reconnect/context history,
unknown fields, each identity component, all-field conflicting facts, service
recreation and failed create/append/commit rollback. They use the preserved primitive
fake and establish **service/port simulation only**. The inherited intentional-red
label at this source-only checkpoint is historical metadata; coordinator configuration
review must assess activation of passing simulation labels and automatic receipt CI.

The reviewer did not invent or weaken assertions and added no tests that mirror the
implementation. No SQL, disk/restart/concurrency, Qt thread/queue, real provider,
credentials, chat/payment or outbound-action check occurred. Those storage/API/
connector gates and all release scope remain explicit. This source review does not
approve a real writer adapter, provider identity stability, source correlation,
contribution eligibility or one-application accounting.

## Actual coordinator configuration review

Reviewed actual coordinator revision
`e1b47f1175268c7f4aafb305a274cee39dd813b3` after source integration `8cc1e9a` and
author evidence integration `8acccfa`. Reviewer brought the actual author evidence
and configuration into the isolated checkout `ac64df1`; source remains exactly
`7a0986bdd3e4b5e38d8b7ac18d76dd56ff002ee8`.

| Reviewed configuration artifact | Exact Git blob |
| --- | --- |
| `.github/workflows/receipt-intake.yml` | `8b42aae4fea0042359168b7592124a9acf6190fd` |
| Root `CMakeLists.txt` | `4c7aeb67deaaa8465b9b812ac6410e4e51cc7eae` |
| `CMakePresets.json` | `9bfe6800ec5687f1d02b1abbc5b5056f2e8313ad` |
| `server/CMakeLists.txt` | `669e1022ea7e288a27aa1fa062e963ddddea25c6` |
| `tests/CMakeLists.txt` | `36de9308fe9e158d072adbed1ed11048e27f071a` |
| Contract with current status prose | `81308e9bffb3fd9950730dbb2e85ad89e6a6daac` |

No configuration findings. The workflow runs PRs to main, main pushes and manual
dispatch with no path filters. It uses the existing immutable checkout pin, disables
persisted checkout credentials, grants only contents:read, sets a15-minute timeout
and selects Ubuntu24.04. Configure/build/test use the same receipt preset and label
as the independent local replay. The explicit fetch flag retains the already selected
GoogleTest source pin and adds no dependency/provider/credential or live action.
No `continue-on-error`, inverted failure, skipped cases or success masking occurs.
Existing scaffold, credit and numeric workflows/dependency configuration are unchanged.

CTest labels become `receipt-intake;acceptance;simulation;server`; display/option/
library prose now describes the implemented helper. Tests and the primitive fake
are unchanged. Contract/brief edits update status/historical ownership wording and
implementation/CI evidence links only; identity, decisions, comparison, transaction
semantics and errors are preserved. The original reviewed/red contract hash above
remains historical evidence. The CI note honestly keeps actual workflow observation,
required-check enforcement and production storage/provider/platform claims separate.

Independent replay of the new workflow commands at `ac64df1`:

| Exact command | Observed result |
| --- | --- |
| `cmake --preset receipt-intake-debug -DSUBATHON_FETCH_GOOGLETEST=ON` | exit0; installed GoogleTest selected, no download |
| `cmake --build --preset receipt-intake-debug` | exit0; updated discovery metadata generated |
| `ctest --preset receipt-intake-debug -L receipt-intake` | exit0;15/15 pass with simulation/server labels |
| `python3 tools/check_project.py` | exit0; project/link/preset/current HTML consistency |
| `python3 tools/progress.py render --check` | exit0; current snapshot |
| `git diff --check` and `git diff --cached --check` | exit0 before review commit |

**Approval covers source and this configuration, not an unobserved remote run.**
The coordinator must observe the new automatic receipt workflow and all applicable
latest-head checks before marking implementation done or merging, preserve all
reviewed assertions and finish independent final-state integration evidence.
Adding a workflow does not configure required branch-protection checks. SQL
uniqueness/durable outcome/restart/concurrency, Qt writer ownership, source admission/
correlation and contribution application remain their explicit downstream gates.
Timer review stays blocked only on its unanswered product decision; none of its
draft behavior is approved or implemented by this receipt change.
