# ACCTTEST-001 reviewed/red receipt baseline handoff

Coordinator `/root`, 2026-10-31, branch `task/ACCTTEST-001-receipt-baseline`.
Base main `806f69f`; specification v0.45 §§3.2–4,14,18 and receipt-intake revision1.
No receipt service algorithm is implemented by this task.

## Exact authored and reviewed baseline

Author `/root/receipt_test_author` committed `28729c7ea2e4cf35db7a667761f8ea21b7325ccc`.
Coordinator integrated it as `72d9c95` and added optional harness `accb064`.
Separate `/root/baseline_reviewer` approved the actual contract, interface, fake,
15 tests and harness in `54cff97`. See [author evidence](accttest-001-authoring.md)
and [independent review](accttest-001-independent-review.md).

| Artifact | Exact reviewed Git blob |
| --- | --- |
| Core receipt values | `f11b0d60013d9606ccfc58188f4363814f26c13c` |
| Server service/repository port | `273f6b4f1aa99f510779f13b965906ac17169e9e` |
| Explicit unimplemented service | `fed41ae653c2122130ac52d038639c3d5ff0ca9c` |
| Primitive fake repository | `99ff2eec97a10c89dc93f0aec5b8c4cee88906a8` |
| 15 acceptance cases | `94317dc4dde4553c8888f78a433e3003c471de1a` |
| Contract revision1 | `149d65b5aa0864c6323e4d5ff6d9580cdbcf2886` |

The generic service owns decisions; the fake only stores primitive values/lookup,
stages transactions and injects known uncommitted failures. It does not classify
retries/conflicts or enforce uniqueness on behalf of production. Conservative full
normalized-facts equality is an explicit independently reviewed audit policy; arrival/
connection/configuration/modifier context remain per-attempt provenance, not identity
or payload equality. Payment/activity linkage references do not replace receipt IDs.

## Actual observed intended red

After approval, coordinator checkout `18d8670` ran the unchanged reviewed baseline
in persistent isolated `local-evidence/worktrees/accttest001-coordinator`.
Local GCC16.2.1, CMake4.4.5, Python3.14.8 and GoogleTest1.18.0; no downloads/install.

| Command | Actual result |
| --- | --- |
| `cmake --preset receipt-intake-debug` | exit0; installed dependencies discovered |
| `cmake --build --preset receipt-intake-debug` | exit0; ten compile/link/discovery steps complete |
| `ctest --preset receipt-intake-debug -L receipt-intake` | exit8; all15 bodies fail from explicit `ReceiptIntakeNotImplemented` |
| `cmake --preset scaffold-debug` / build / CTest | exit0; scaffold3/3 pass |
| `cmake --preset exact-numeric-debug` / build / CTest-Lexact-numeric | exit0; numeric33/33 pass |
| `cmake --preset acceptance-debug` / build / CTest-Lacceptance/regression | exit0; credit23/23 pass |

[Raw red output](accttest-001-red-ctest.txt) retains real behavioral exceptions and
wrong-exception assertions in storage-failure cases. No skips, missing dependency,
compilation failure or failure inversion establishes this result. The capture wrapper
returned CTest's actual exit8. Baseline completion means reviewed tests plus observed
red; it does not mean receipt behavior, contribution deduplication or durability works.
Final done-state tooling/integration and publication results are recorded in the task
note/checkpoint and final PR; latest-head CI/review/merge gates remain separate.

## CI, scope and continuation

Receipt tests use the explicit receipt-intake-debug lane while intentionally red;
default numeric/credit/scaffold lanes remain enabled. No workflow is weakened or
made green by masking red cases. ACCT-001 must add stable automatic receipt PR/main
CI alongside implementation and observe that new workflow before completion.
No Qt runtime, provider/network, Boost or real database is required by this port test.

All provider namespaces/data are synthetic. Actual provider identifiers, missing-ID/
source admission and verified cross-provider correlation remain connector/API-006
contracts. SQL uniqueness, durable commit/restart/concurrency/acknowledgment and Qt
writer/thread ownership remain STORAGE-002/API integration gates. Contribution
creation/eligibility and one-application effects remain ACCT-002/004. No live input,
credentials or external actions were used. Full release scope remains unchanged.

Next: final independent integration review, focused baseline PR with latest CI and
merge gates; only then claim ACCT-001 from integrated main. Preserve all reviewed
assertions/fixtures/interfaces and report its fake-backed passing checks as simulation,
not production SQL/provider verification. Timer draft is independent and still waits
for the owner's confirmation-wait credit decision.
