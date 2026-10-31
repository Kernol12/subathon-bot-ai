# ACCT-002 bounded contribution implementation

Implementer: `/root/contribution_implementer`, 2026-10-31, branch
`task/ACCT-002-implementation`, claimed base `3d82b344b9caf5485978c096be827cf4bf1d1041`
from integrated main `f886af9`. This evidence accompanies the implementation commit.
Authority: specification v0.45 §§3,5,14,18 and settlement boundary §8;
[contribution eligibility revision 1](../contracts/contribution-eligibility-v1.md),
[receipt intake revision 1](../contracts/receipt-intake-v1.md) and
[event/state boundaries revision 1](../contracts/event-state-boundaries-v1.md).

The [reviewed/red baseline](accttest-002-baseline-handoff.md) records separate
approval `a561508`, actual21 explicit-stub failures at `4d1400f`, and final/reconciled
review `3ffb71d`/`4e614fe`. Production work started after that baseline was merged.
No baseline assertions, fixtures, interfaces, fake, normative contract or build files
were changed by this implementation.

## Behavior and owned paths

- `core/src/contribution.cpp` assesses every independent evidence-policy, monetary
  valuation, ordinary-review, application and settlement/reversal blocker. It leaves
  the supplied facts, evidence labels, references and conditions unchanged. An empty
  result only permits further planning in this bounded projection.
- `server/src/contribution_evidence.cpp` validates observation shape and supplied
  verified same-source identity/proof fields; compares full immutable observations
  by receipt ID; and retains each distinct receipt. Only verified activity identity
  establishes linkage. The first support owns original facts, arrival/context and
  all conditions, including when matching confirmations arrived earlier. Explicit
  confirmation alone promotes aggregate evidence; delayed weaker evidence cannot
  erase it. No other state dimension advances.
- Every successful result follows one transaction commit. Unmatched confirmations
  remain unresolved, exact repeats return their current association without adding
  another observation, and contradictory receipt reuse throws without replacing
  history. Creation, pending links and evidence updates use primitive port operations
  and rely on destruction without commit for known-uncommitted rollback.
- This evidence file is the third owned path. Coordinator `/root` owns shared
  build/labels/CI, status/HTML/map, publication and final integration. No effects,
  jobs, application candidates, financial calculations or settlement reconciliation
  are implemented here.

## Exact artifacts

Git blobs from `git hash-object` after formatting and before commit:

| Artifact | Blob |
| --- | --- |
| Implemented core assessor | `c3b2ea0d8449fd3752512b35eac052b2f300ece7` |
| Implemented server observation service | `975067bb952383e08c015d87139d6a806240ecb0` |
| Unchanged21 reviewed cases | `702a7bf1d8162ba748ee9477ea48ffa73047ef65` |
| Unchanged core interface | `65ad81ac0dc3fc808421258ab89b30ff610be085` |
| Unchanged server interface/primitive port | `82c2e5f7315b3f2a9ab15fdec87104f6456c970a` |
| Unchanged primitive fake | `8d35e502a9656d0e65555efb48b3bd8650bd0c5e` |

## Actual local verification

Executed 2026-10-31 around19:45 UTC in
`local-evidence/worktrees/acct002-implementation`, with disposable synthetic data.
Environment: Linux7.2.11-arch1-1 x86-64, GCC16.2.1 (20260810), CMake4.4.5,
Ninja1.13.2, GoogleTest1.18.0, Python3.14.8, clang-format23.1.4.
Exact-numeric configure selected installed Boost1.92.0 at `/usr/include`.
Qt is unused in this framework-free simulation and was not probed.

All commands below actually exited0:

| Exact command | Observed result |
| --- | --- |
| `cmake --preset contribution-evidence-debug` | Configured successfully |
| `cmake --build --preset contribution-evidence-debug` |12 compile/link/discovery steps complete |
| `ctest --preset contribution-evidence-debug -L contribution-evidence` |21/21 bodies pass;9 assessor and12 service cases, including injected rollback/retry |
| `cmake --preset receipt-intake-debug` | Configured successfully |
| `cmake --build --preset receipt-intake-debug` | Built successfully |
| `ctest --preset receipt-intake-debug -L receipt-intake` |15/15 pass |
| `cmake --preset exact-numeric-debug` | Configured successfully, Boost1.92.0 |
| `cmake --build --preset exact-numeric-debug` | Built successfully |
| `ctest --preset exact-numeric-debug -L exact-numeric` |33/33 pass |
| `cmake --preset acceptance-debug` | Configured successfully |
| `cmake --build --preset acceptance-debug` | Built successfully |
| `ctest --preset acceptance-debug -L acceptance` |19/19 credit acceptance cases pass |
| `ctest --preset acceptance-debug -L 'acceptance\|regression'` |23/23 credit acceptance/regression cases pass |
| `cmake --preset scaffold-debug` | Configured successfully |
| `cmake --build --preset scaffold-debug` | Built successfully |
| `ctest --preset scaffold-debug` |3/3 pass |
| `python3 tools/check_project.py` |377 tasks,122 decisions, local links/presets/current HTML pass |
| `python3 -m unittest discover -s tests/tooling -v` |19/19 pass |
| `python3 tools/progress.py render --check` | Current snapshot |
| `clang-format --dry-run --Werror core/src/contribution.cpp server/src/contribution_evidence.cpp` | Formatting passes |
| `git diff --check` | No whitespace errors |

The contribution CTest lane still reports its inherited `intentional-red` label at
this isolated implementation revision. This run is an actual21-case pass against
implemented source; the coordinator owns conversion to stable simulation metadata
and automatic CI. No failure inversion, skipping or baseline change was used.

## Remaining gates and limits

Independent implementation review, latest integrated revision regression checks,
stable automatic contribution CI, task status/HTML and PR/merge gates remain the
coordinator's work. This author does not certify separate review or task completion.

Passing evidence is pure-unit and primitive-store simulation only. It does not
establish SQL uniqueness/durability/concurrency or indeterminate-commit handling,
Qt writer/thread ownership, real provider proof/correlation, permissions/review
operations, process/disk recovery, effects/application or refund/settlement behavior.
Those retain their existing storage/API/connector/accounting baselines. No live
provider call, credential, supporter data or live side effect was used. All seven
integrations, three rate providers and release/platform requirements remain in scope.
