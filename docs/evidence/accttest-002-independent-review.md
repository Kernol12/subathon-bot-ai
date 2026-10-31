# ACCTTEST-002 independent eligibility/linkage baseline review

Reviewer: `/root/baseline_reviewer`; author: `/root/receipt_test_author`.
Date: 2026-10-31. Current verdict: **approved for test baseline and final
integration** at the corrected revisions below. Both initial focused coverage
findings and the final status-prose finding are closed.
This is an actual independent review after explicit reassignment; the earlier
usage-limit checkpoint did not supply review approval. No test body or intended-red
run was executed by this reviewer; the coordinator's approved red was independently
inspected in the final integration pass below.

## Exact candidate and scope

Reviewed author history `12f75b1`, `023a899` and final
`b0fd97ea0295306c084e09f03f5d85fdbe15572e`, integrated as `afd4b75`, `20d08ea`,
`ccdd17c`; coordinator harness `281c8210151b5fdefc3ea359568acd5f6d981a54`.
Actual full candidate: `c13e408ad66a70c96fa05010093be85d70d26a6c`, based on main
`8ad458e` (merged ACCT-001 PR16). The isolated reviewer branch
`task/ACCTTEST-002-eligibility-review` owns only this document.

Authority: specification v0.45 §§3.2–4,5,14,18 and settlement boundary §8;
[contribution eligibility revision1](../contracts/contribution-eligibility-v1.md),
[event/state boundaries](../contracts/event-state-boundaries-v1.md),
[receipt intake revision1](../contracts/receipt-intake-v1.md) and
[ACCT-002 brief](../tasks/EPIC-002/ACCT-002.md). Read root/scoped core/server/tests
instructions, architecture, workflow/test strategy, author evidence and usage-limit
checkpoint. Inspected every one of the21 authored test cases, interfaces/stubs,
fake and root/core/server/test build/preset harness.

| Reviewed artifact | Exact Git blob |
| --- | --- |
| `core/include/subathon/core/contribution.hpp` | `65ad81ac0dc3fc808421258ab89b30ff610be085` |
| Core assessor explicit stub | `fb140178ba0adf2f11ac37f8a311977217ebc9c5` |
| `server/include/subathon/server/contribution_evidence.hpp` | `82c2e5f7315b3f2a9ab15fdec87104f6456c970a` |
| Server observation explicit stub | `1af4468cbc94ea5f8e85d23b00c64293589486ad` |
| Primitive contribution repository fake | `8d35e502a9656d0e65555efb48b3bd8650bd0c5e` |
|21 acceptance cases | `341095486f757b0ff83b4554d929711684346620` |
| Contribution contract revision1 | `5eec5921c355331bb97d351b7e20ad00873dae7a` |
| Root `CMakeLists.txt` | `867657a2627d4f2d290b5dd6e29e3bd6d9c4968e` |
| `CMakePresets.json` | `aefc0a1875a33f17a2e1864bcefcf593dc9cd00c` |
| `core/CMakeLists.txt` | `e4a18361d09c87553aa673d750d02420849e62f8` |
| `server/CMakeLists.txt` | `417bca1f9cc2b42f94601b9821914e9d7cee5a5a` |
| `tests/CMakeLists.txt` | `4c6b7bf3911085196b2789c6d80ba3f3225ede4a` |

## Initial requested focused corrections (closed below)

1. **Preservation oracle must distinguish first and later conditions.** Contract
   “Receipt observations and verified linkage” promises that new support matching an
   existing verified activity preserves all first-support conditions, consistent
   with §3.4 arrival-time handling. `DelayedUnconfirmedSupportCannotEraseConfirmationOrFirstContext`
   supplies a later arrival/context but identical `support_conditions`. Its condition
   equality assertion therefore cannot detect replacing the original review,
   valuation or evidence policy with the later support's conditions. Supply different
   late conditions (for example accepted review, another valuation reference and
   another captured policy/reference), then retain the existing original-condition
   expectation. This is a small addition to the existing fixture, not a new policy
   or permission path. It protects against late input silently clearing original
   review/valuation/policy holds.
2. **Declared malformed observation shapes need executable rejection oracles.**
   Contract “Atomic port, input errors and simulation limits” explicitly rejects
   support without conditions and confirmation with conditions. The existing grouped
   `InvalidEvidenceAndContradictoryReceiptReuseCannotMutateHistory` case exercises
   unconfirmed confirmation, origin mismatch, absent proof reference and conflicting
   receipt reuse, but neither of those two shapes. Add both malformed inputs to that
   grouped case and require `std::invalid_argument` with empty committed storage.
   These are two separate declared API branches; no exhaustive invalid-input matrix
   or invented provider validation is requested.

The changed actual test revision requires a follow-up independent review before
full baseline approval/red. Existing assertions must be preserved; none should be
weakened to satisfy these findings. No contract or product-policy change is needed.

## Assessment without further objection

- Safe default enum/field values now preserve unknown evidence, pending review,
  review-required admission, unresolved monetary valuation and settlement. The new
  default-state test verifies all independent holds and never invents a known rate
  failure from an omitted flag. This resolves the earlier pre-review default concern.
- Evidence facts and §14 admission policy are separate. Ordinary accepted review
  cannot substitute for the separately scoped evidence decision; report admission
  preserves unknown/unconfirmed labels. Record-only blocks even confirmed accepted
  input. References are retained provenance, with authorization/writer operations
  explicitly downstream rather than claimed by a public struct.
- Monetary requirement is explicit. Missing source money stays absent and blocks
  even with a supplied resolution reference; a marked missing rate remains an
  independent hold. Explicit nonmonetary/not-applicable input is distinct. The
  assessor returns every stated blocker without mutating state.
- Applied contributions never become initial candidates again after reversal.
  Never-applied partial/full reversal and unresolved settlement hold for ACCT-006;
  this baseline does not invent net-payment awards, refund accounting or effects.
- The server consumes immutable committed receipt projections without growing
  transport-attempt collections. Identity is verified provider/account/namespace/
  opaque activity identity, excluding its retained proof reference. Raw payment
  references, names, amounts and times cannot establish correlation. Real proof
  construction/provider identity/source authority remain API-006/connector gates.
- Later explicit confirmation changes only aggregate payment evidence. The first
  support snapshot anchors conditions/context even when confirmation arrived first;
  both receipts remain audited. Repeated receipt observations are idempotent,
  distinct confirmation receipts remain traceable, and matching activity produces
  one contribution. The conservative confirmation-only aggregate promotion is an
  explicit local choice; other supersession/reconciliation remains downstream.
- Service/fake separation is sound: the fake performs exact primitive lookups,
  allocation, staged writes and known failure injection. It does not enforce
  uniqueness, deduplicate creates, classify observations or merge evidence.
  Failure cases inspect rollback after create/append/update/link and before commit,
  preserve existing committed evidence and retry. Service recreation is not claimed
  as process/disk restart evidence. No candidate/job/effect is emitted by this slice;
  ACCT-004 must enforce one actual application transaction.
- Core uses the standard library; both behavior functions are unconditional explicit
  unimplemented throws. The optional harness selects actual stubs and GoogleTest,
  preserves stable receipt/numeric/credit/scaffold lanes, and labels the separate
  contribution lane without `WILL_FAIL`, blanket continue-on-error or skipped tests.

## Commands and remaining gates

Independent commands in `local-evidence/worktrees/accttest002-review`:

| Command | Observed result |
| --- | --- |
| Targeted `git show`, `cat`, `sed`, `rg` and artifact-specific `git rev-parse HEAD:<path>` | exit0; actual candidate/artifact identities inspected |
| `cmake --preset contribution-evidence-debug` | exit0; GCC16.2.1/Python3.14.8 |
| `cmake --build --preset contribution-evidence-debug` | exit0;12 compile/link/discovery steps complete |
| `ctest --preset contribution-evidence-debug -N -L contribution-evidence` | exit0;21 cases listed, no test bodies |
| `git diff --check` and `git diff --cached --check` | exit0 before review evidence commit |

Compilation/listing is neither red nor passing behavior. After the actual corrected
baseline receives independent approval, the coordinator must observe CTest failing
from `ContributionEligibilityNotImplemented` / `ContributionEvidenceNotImplemented`
and record exact artifacts/output. Missing dependencies, compilation failure, skips
or green scaffold results cannot satisfy that gate. ACCT-002 implementation waits
for that completed baseline, then needs passing relevant checks, separate code review
and observed stable automatic contribution CI. Publication/latest-head/merge gates
are separate. SQL durability/uniqueness/concurrency, Qt writer ownership, provider
proof, real application/refund/authorization and all release/platform gates remain
explicit. No credentials, live state or side effects were used. Timer's unanswered
owner policy is unchanged and receives no implicit answer from this review.

## Follow-up approval of actual corrected baseline

Separate author `/root/receipt_test_author` supplied actual correction
`929152c1e64024d5bfa3ed641a76fa1ea4a3120b`. Reviewer inspected its entire diff and
cherry-picked it onto the reviewed candidate/evidence checkout as `7c38a49` for
independent compilation/listing. The corrected test blob is
`702a7bf1d8162ba748ee9477ea48ffa73047ef65` (still21 cases); appended author evidence
is `889cb9a02e3cd566efebbf8065eb8edf64afcaed`. The change affects only those two
files. All original assertions, interfaces, stubs, fake, normative contract and build
artifacts above are preserved.

- **Finding1 closed.** The delayed same-activity support now carries a later
  valuation reference and resolved-rate flag, accepted ordinary review, and different
  record-only admission policy/reference. The unchanged full-condition equality
  assertion still requires the original support conditions. A service that replaces
  initial review/valuation/policy with later support values can no longer satisfy
  that oracle. The incoming values are valid separate captured conditions, so this
  tests first-support retention without inventing a new invalid-input policy.
- **Finding2 closed.** The grouped invalid test now submits support with absent
  conditions and confirmation with supplied conditions, requiring invalid_argument.
  Existing empty-store assertions follow these and the other invalid observations;
  original history-protection assertions remain unchanged.

Independent follow-up commands in the review worktree at `7c38a49`:

| Command | Observed result |
| --- | --- |
| `git show 929152c1e64024d5bfa3ed641a76fa1ea4a3120b -- <tests/author evidence>` | exit0; additive13 test lines and author rationale only |
| `cmake --preset contribution-evidence-debug` | exit0 |
| `cmake --build --preset contribution-evidence-debug` | exit0; revised test compiled and executable linked/discovered |
| `ctest --preset contribution-evidence-debug -N -L contribution-evidence` | exit0;21 cases listed; no bodies executed |
| `python3 tools/check_project.py` | exit0;377 tasks,122 decisions, links/presets/current HTML pass |
| `python3 tools/progress.py render --check` | exit0; current snapshot |
| `git diff --check` and `git diff --cached --check` | exit0 before follow-up review commit |

Verdict: **approved for test baseline** for the artifact table above with only the
test/author-evidence blobs replaced by these corrected identities. There are no
remaining baseline findings or new owner policy decisions in this bounded slice.
This approval permits the coordinator to run the actual intended-red test bodies;
it does not itself supply red evidence, passing behavior or final integration/CI/
publication/merge approval. ACCT-002 production remains gated on observed explicit
missing-behavior failures and the completed baseline handoff. All simulation limits,
downstream application/refund/provider/storage/authorization gates and the separate
timer policy blocker remain unchanged.

## Final reviewed/red baseline integration

Reviewer `/root/baseline_reviewer`, 2026-10-31. Actual completion candidate:
`3c5a8cacc83d1d9a0f498da4fba4b7fe4244ff7a`, plus status-prose corrections
`e47388783d4ce3b052cc3bdd38e3ca8d5217462a` and
`158e0d7183df2acdd53e96c0e9224107de21cd77`. Final reviewer checkout before this
append was `01c55445f6eb3166846b8896f65921df0ca7f8fb`. Verdict: **approved for
integration of this reviewed/red baseline**. Publication/latest-head CI, discussion/
review/ruleset gates, merge outcome and later shared-artifact reconciliation remain
coordinator responsibilities; no completed product behavior is implied.

Compared actual completion changes with approved `4d1400f`: both interfaces, explicit
stubs, fake, all21 corrected tests, root/core/server/test CMake and presets remain
unchanged. Test blob stays `702a7bf1d8162ba748ee9477ea48ffa73047ef65`. Contract and
task-brief edits update status/evidence links only; the normative behavioral contract
represented by `5eec5921c355331bb97d351b7e20ad00873dae7a` is preserved. Existing
assertions were neither removed nor weakened.

Independently inspected [raw red output](accttest-002-red-ctest.txt), blob
`264b97410c3e5fdc03fe6d14f06230a70d7a93dd`, and
[baseline handoff](accttest-002-baseline-handoff.md). A read-only comparison against
the exact candidate's test source matched all21 authored case names to all21 RUN
entries and all21 CTest failures, in order. Nine assessor cases report explicit
eligibility-stub body exceptions and twelve service cases report explicit
evidence-stub body exceptions. Wrong-exception assertions in validation/storage
cases are also consistent with those stubs. There are no skips/not-run or missing
executable reports; the summary reports21 failures and0% passing. The coordinator
records real CTest exit8 at `4d1400f`, after prior separate approval and successful
configure/build. This reviewer inspected that committed runtime evidence without
repeating the red bodies.

`ACCTTEST-002` is correctly done as a test baseline, with `baseline: observed_red`
and `product_acceptance: not_run`; `ACCT-002` remains planned. The acceptance map
adds only this reviewed/red bounded scenario. Historical usage-limit evidence is
explicitly superseded. One current Test Strategy paragraph initially contradicted
the new red evidence by still saying review was blocked and no bodies had run;
`e473887` replaces that paragraph with the accurate reviewed/red status and removes
the duplicate appended update; `158e0d7` normalizes its final newline. Finding closed.
The durable checkpoint distinguishes this baseline from the separate timer PR and
retains all application/refund/provider/SQL/Qt/release gates.

Final independent checks:

| Command / check | Observed result |
| --- | --- |
| Read-only exact-Git-object Python comparison of source names, raw RUN/failure names, explicit stub-exception counts and baseline/implementation state | exit0;21 exact matches,9 assessor +12 service stub exceptions, baseline-only done and implementation planned |
| `git diff 4d1400f 3c5a8ca -- <reviewed source/interface/fake/test/build paths>` | exit0; no behavioral/build changes |
| `python3 -m unittest discover -s tests/tooling -v` | exit0;19/19 pass |
| `python3 tools/check_project.py` | exit0;377 tasks,122 decisions, links/presets/current HTML pass, including after prose corrections |
| `python3 tools/progress.py render --check` | exit0; current snapshot, including after prose corrections |
| `git diff --check` and `git diff --cached --check` | exit0 before final review commit |

Coordinator evidence records scaffold3/3, credit23/23, numeric33/33 and receipt15/15
passing. This final evidence/status pass did not repeat unchanged native suites,
red bodies or the coordinator's DOM simulation. It supplies no provider, Qt, SQL,
authorization, effect/refund, platform or live verification. Review after future
shared build/state/map merges must assess the actual reconciled revision; this
approval does not preapprove that later integration.

## Current-main shared-artifact reconciliation review

Reviewer `/root/baseline_reviewer`, 2026-10-31. Actual reconciled candidate:
`22b639778c51bb08eeed1f75673eb9314b7a1e82`, merging timer-baseline main
`419d8551c54ad7e3f802f18c10784bef49d7908f` into the approved contribution baseline.
Reviewed from isolated checkout `80f4741`. Verdict: **approved for integration of
this actual reconciliation**, subject to the published head's remaining CI/PR/merge
gates. This section supplies the later shared-artifact review required above.

No behavioral artifacts changed: independently compared timer header/stub/tests/
contract against main419d855 and contribution interfaces/stubs/fake/tests/contract
against approved3ffb71d. All identities match. Timer remains the explicit
`TimerLifecycleNotImplemented` stub; contribution assessor/service remain their
explicit unimplemented stubs. The active CORE-002 claim from `04cd539` changes
ownership/progress/checkpoint only, carries no completed verification and introduces
no timer implementation source into PR17.

The resolved root/core/test CMake and presets retain both optional baseline targets,
discovery labels and preset lanes. GoogleTest setup accepts either flag; server
targets retain their separate receipt/contribution gates. There is no target collision,
failure inversion, skip, new dependency or change to stable workflows. Shared state
retains both CORETEST-00239-case and ACCTTEST-00221-case reviewed/red completions
with product acceptance not_run, plus active CORE-002 ownership and planned ACCT-002.
The map preserves both observed_red scenarios and their independent evidence; the
checkpoint combines the known merged timer baseline and active work without claiming
either implementation or upgrading SQL/Qt/provider/application verification.

Independent checks at the actual reconciled revision:

| Command / check | Observed result |
| --- | --- |
| Read-only exact-Git-object comparison of both artifact sets and baseline/implementation state | exit0; both reviewed sets unchanged, both baseline red states retained, active timer claim metadata only |
| `cmake --preset contribution-evidence-debug -DSUBATHON_BUILD_TIMER_LIFECYCLE_TESTS=ON` | exit0; combined configuration succeeds |
| `cmake --build --preset contribution-evidence-debug` | exit0; both explicit-stub targets/test executables built |
| `ctest --preset contribution-evidence-debug -N -L 'contribution-evidence\|timer-lifecycle'` | exit0;60 cases listed:39 timer +21 contribution; no bodies executed |
| `python3 tools/check_project.py` | exit0;377 tasks,122 decisions, links/presets/current HTML pass |
| `python3 tools/progress.py render --check` | exit0; current reconciled snapshot |
| `git diff --check` and `git diff --cached --check` | exit0 before reconciliation-review commit |

No additional native runtime/body replay was needed: the changed build boundary was
compiled/discovered and all reviewed behavioral inputs are unchanged. Prior observed
red and stable passing evidence remain retained. This approval neither implements
the timer nor repeats the other reviewer's full timer-policy approval; it verifies
preservation of the already integrated timer baseline in this shared merge. Future
source/CI/status changes still require their own applicable review and verification.
