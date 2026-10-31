# ACCTTEST-001 independent receipt baseline review

Date: 2026-10-31. Author: `/root/receipt_test_author`.
Reviewer: `/root/baseline_reviewer` (separate from the author).
Verdict: **approved for test baseline**, for the exact artifacts below. This is
approval to observe intended stub-red, not implementation, SQL durability or
complete duplicate-safe contribution processing.

## Actual reviewed revisions and ownership

Author commit: `28729c7ea2e4cf35db7a667761f8ea21b7325ccc`, integrated by the
coordinator as `72d9c95`. Build harness: coordinator commit
`accb06451f7f7faff979b0fe58f241293f7202cc`. Reviewer cherry-picked these actual
commits into `task/ACCTTEST-001-receipt-review` from main `806f69f`; compilation and
listing used checkout `eef1581b4ef180575d7f7be9ce742643fa9841f1`. This reviewer owns
this evidence document only and authored none of the reviewed behavior or tests.

Authority: specification v0.45 §§3.2–4,14,18; independently reviewed
[event/state boundaries revision1](../contracts/event-state-boundaries-v1.md);
the actual [receipt intake revision1](../contracts/receipt-intake-v1.md) and
[ACCT-001 task brief](../tasks/EPIC-002/ACCT-001.md). Read root/scoped instructions,
coordinator instructions, architecture, workflow and test strategy before review.

| Reviewed artifact | Exact Git blob |
| --- | --- |
| `core/include/subathon/core/receipt.hpp` | `f11b0d60013d9606ccfc58188f4363814f26c13c` |
| `server/include/subathon/server/receipt_intake.hpp` | `273f6b4f1aa99f510779f13b965906ac17169e9e` |
| `server/src/receipt_intake.cpp` | `fed41ae653c2122130ac52d038639c3d5ff0ca9c` |
| `tests/fakes/receipt_repository.hpp` | `99ff2eec97a10c89dc93f0aec5b8c4cee88906a8` |
| `tests/acceptance/receipt_intake_tests.cpp` (15 cases) | `94317dc4dde4553c8888f78a433e3003c471de1a` |
| `docs/contracts/receipt-intake-v1.md` | `149d65b5aa0864c6323e4d5ff6d9580cdbcf2886` |
| `docs/tasks/EPIC-002/ACCT-001.md` | `1537ff5033592df06a4a0b6d8f652ce97caa45c7` |
| `docs/evidence/accttest-001-authoring.md` | `6563dbc81d81062bff116b3dea67de6ed84d0976` |
| Root `CMakeLists.txt` | `ea23f9da2ccef40186a553690cd87e7a306cb252` |
| `CMakePresets.json` | `53ff8452186aba7744332659da30f7de4549ad30` |
| `server/CMakeLists.txt` | `a19523361cf268e342f29afef8122c6342d81145` |
| `tests/CMakeLists.txt` | `b4af119b2eef60101e3a6d2667681bda3df15f62` |

## Contract and oracle assessment

No blocking findings in this bounded baseline. The 15 authored cases have manually
specified dispositions, identities, literal values and preservation invariants;
expected decisions are not produced by an implementation of intake.

- Identity includes provider, stable configured source/account, documented source
  delivery/event namespace and opaque ID. Each component scopes an otherwise equal
  ID. Connection belongs to attempt provenance, so reconnect/configuration/modifier
  context changes still retry and preserve the first captured context. Synthetic
  namespaces establish no actual provider identifier stability.
- Explicitly approve revision1's conservative local payload-consistency decision:
  compare **every field of the bounded normalized facts projection**, including
  absent/present values, display name, decimal spelling/source precision and linkage
  namespaces. Equal facts retry; differing facts conflict against the unchanged
  original, with the incoming full facts and arrival retained in an attempt. The
  14-mutation grouped case, changed-amount sequence and later original-facts retry
  exercise the policy without overwriting history. This is an audit disposition;
  it does not assert fraud, new support, enrichment eligibility or provider behavior.
  Transport arrival/connection/configuration/modifier context are deliberately
  excluded from this facts comparison and retained separately.
- Equal names, amounts and timestamps cannot merge distinct identities.
  Payment/activity references remain separate source linkage evidence; received
  and confirmation events can produce distinct receipts without creating two
  contributions or granting payment eligibility here. Cross-source authority and
  verified correlation are explicit downstream gates, not simulated capabilities.
- Optional source time, actor, amount and linkage stay absent when unknown.
  Source time is distinct from server arrival. Later supplied optional facts under
  the same identity conflict without making the unknown original appear known.
  The owned-value test also mutates caller buffers after ingest and checks retained
  original values and the first attempt.
- The port exposes primitive lookup/create/append/commit. The fake implements
  exact-key lookup, allocation, transaction-local staging and publication at commit;
  it does not compare facts, classify dispositions, enforce uniqueness or deduplicate
  inserts on the service's behalf. Consequently, an always-insert implementation
  cannot pass retry/identity tests because of hidden fake policy.
- Failure tests inject after staged create, after first/retry/conflict append, and
  before commit for all three dispositions. They inspect committed state, require
  typed failure rather than success, preserve prior committed records/history and
  then retry successfully with the right disposition/attempt count. These assertions
  protect the service's atomic-port obligation. The fake's known-uncommitted errors
  establish no SQLite commit-outcome, disk, restart, concurrency or thread evidence.
- A recreated service must consult the same repository; that test explicitly avoids
  claiming process restart durability. The contract separately requires matching
  production uniqueness and owned-writer/thread behavior in storage/API baselines.

The server source is a single unconditional `ReceiptIntakeNotImplemented` throw.
Core owns standard-library values only. Neither the interface nor harness introduces
Qt, network/provider calls, contribution eligibility or authoritative effects.

## Commands actually run

Working directory: `local-evidence/worktrees/accttest001-review`.
Linux; GCC16.2.1, Python3.14.8, CMake4.4.5 and installed GoogleTest1.18.0.

| Command | Observed result |
| --- | --- |
| `git show 28729c7:<path>` / `git show accb064 -- <build paths>` | exit0; actual source/contract/author evidence/harness inspected |
| Artifact-specific `git rev-parse HEAD:<path>` | exit0; identities match the table above |
| `cmake --preset receipt-intake-debug` | exit0 |
| `cmake --build --preset receipt-intake-debug` | exit0;10 compile/link/discovery steps complete |
| `ctest --preset receipt-intake-debug -N -L receipt-intake` | exit0;15 cases listed; no test bodies executed |
| Targeted `rg` for failure inversion/skips in source/tests/build files | exit1 with no matches; no `WILL_FAIL`, disabled/skipped tests or blanket continue-on-error |
| `git diff --check` and `git diff --cached --check` | exit0 before review evidence commit |

The build/list commands are compilation and discovery evidence only. **No receipt
test body or runtime red was executed by this reviewer.** After this approval, the
coordinator must run `ctest --preset receipt-intake-debug -L receipt-intake` against
these artifacts and record genuine failures from `ReceiptIntakeNotImplemented`.
Missing dependencies, failed compilation and skipped cases do not satisfy that gate.

## Remaining gates and CI assessment

The optional harness links the actual server stub and GoogleTest and applies
`receipt-intake;acceptance;intentional-red` labels without hiding failure. It leaves
stable scaffold/credit/numeric lanes unchanged. Keep intentional-red work separate;
ACCT-001 must add and observe stable passing receipt CI for relevant PRs/main before
implementation completion, plus separate code review and applicable regression checks.
Approval applies only to these exact artifacts; affected changes require review.

This baseline supplies no missing-ID fallback, provider-specific normalization,
source-authority/correlation policy, contribution creation/confirmation linkage,
application transaction, SQL adapter, durable event schema, real queue, acknowledgement
durability or remote exactly-once behavior. API-006, ACCT-002, STORAGE-002 and their
respective reviewed tests retain these responsibilities. Indeterminate durable commit
outcomes must be resolved against durable identity rather than assumed rolled back.
All seven integrations, all three exchange providers and other release/platform/live
gates remain required. No credentials, live services or supporter data were used.

## Final baseline integration review

Reviewer `/root/baseline_reviewer`, 2026-10-31. Actual coordinator candidate:
`377083ead90c2c96f4551743979821888ff434ef`. Verdict: **approved for integration**
of this reviewed/red test baseline, subject to outstanding publication and latest-PR
CI/merge gates. This is not approval of receipt production behavior.

Reviewed the candidate diff, [baseline handoff](accttest-001-baseline-handoff.md),
[raw CTest output](accttest-001-red-ctest.txt), progress state, rendered progress,
acceptance map, test strategy and shared coordinator checkpoint. Compared actual
candidate artifact identities with the approved table: header, port, stub, fake,
all15 tests and contract are unchanged. Diff from reviewed coordinator `18d8670`
contains no production, harness, interface, fixture or assertion change. No stable
workflow was weakened. Timer source/header/tests are absent from this receipt
candidate; its concurrent ownership claim and pending policy remain explicit.

The raw red log blob is `a4411f2bf2abba4828fb0d8a00f221cbb26a3dbd`.
An independent read-only check against Git objects confirmed that the authored
15 test names exactly match all15 `[ RUN ]` entries and all15 CTest failures, in
order, with15 explicit unimplemented-service exception reports. The summary reports
0% passed and15 failures; there are no skipped/not-run cases or absent-executable
errors. Storage-error expectations also report the wrong explicit
`ReceiptIntakeNotImplemented` exception, consistent with missing service behavior.
The coordinator records the real CTest exit8 at `18d8670` after successful build
and prior independent approval. This reviewer inspected that run's committed
evidence and did not rerun the runtime-red bodies.

`ACCTTEST-001` is correctly `done` as a `test_baseline`, with reviewed/build evidence,
`baseline: observed_red` and `product_acceptance: not_run`. `ACCT-001` remains
`planned`. The acceptance map adds only the identified receipt/attempt scenario as
`observed_red`, explicitly excluding SQL, real provider identity/correlation,
contribution creation and one-application capability. The test strategy and handoff
retain those limits, future automatic passing receipt CI and downstream storage/API
gates. Existing full-scope release requirements remain intact.

Independent commands after merging the exact candidate into the isolated reviewer
worktree:

| Command / check | Observed result |
| --- | --- |
| `git diff 18d8670 377083e -- <reviewed source/build/contract paths>` and artifact-specific `git rev-parse 377083e:<path>` | exit0; no reviewed artifact changes; exact blobs retained |
| Read-only Python comparison of exact-candidate test names, raw-run names/failures, stub-exception count, baseline status and tracked timer paths | exit0;15 exact matches, baseline-only status, no timer implementation artifacts |
| `python3 tools/check_project.py` | exit0;377 tasks,122 decisions, links/presets/current HTML pass |
| `python3 -m unittest discover -s tests/tooling -v` | exit0;19/19 pass |
| `python3 tools/progress.py render --check` | exit0; current generated snapshot |
| `git diff --check` and `git diff --cached --check` | exit0 before integration-review commit |

The coordinator handoff records stable scaffold3/3, numeric33/33 and credit23/23
passing; this final evidence review did not repeat those unchanged native suites or
the coordinator's DOM simulation. No new behavior changed to justify broadening
the earlier compile/discovery review. Publication, actual latest-head CI, GitHub
review/discussion/ruleset checks and merge outcome remain coordinator gates and
are not implied by this local integration approval. Timer's overall review remains
request changes solely for the unanswered confirmation-wait credit decision.
