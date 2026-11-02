# STORAGECONTRACT-001 independent logical contract review

Reviewer: `/root/contribution_reviewer`,2026-11-02. **APPROVED: actual internal
logical documentation contract only**, with no remaining blocking finding.
Reviewed author `b9721ca8d8b724dd4f0258be36933d7af0457e00`, integrated root
`1a7b4fc586537581e0f2af01d8a4833dcff69cff` in this isolated review tree.

| Actual artifact | Git blob |
| --- | --- |
| [Final400-line contract](../contracts/application-transaction-bundle-v1.md) | `3c8cbd869800f6aa4cd117016ef568f789d3ace9` |
| [Author evidence](storagecontract-001-author.md) | `10b0864d6a6244da442f32e0ecaabe333f3ed6a0` |

I read the entire original360-line proposal, all21 final examples, actual substantive
revision and final exact wording diff, against specification v0.45 §§3.3–3.5/4/8/16,
NUM-01–04/DB-01, CONTRACT-002 including B08, current plan/credit/debt/timer contracts
and actual core/writer/receipt/contribution/retention/plan ports. Prerequisites
CONTRACT-002/STORAGE-015/ACCT-003/CORE-005 are done; future ACCT-004/STORAGE-002
are consumers, never prerequisites or a dependency cycle. Only the author's two
Markdown paths changed; no code/DDL/API harness/fixture/test/runtime/red is supplied.

## Actual findings and revised artifact

Original contract blob `02e7b70bebcda58c9276948c4367cec525bc8e7a` required revision:

- Its active borrowed-session lookup promised a committed result, although a
  stage followed by lookup/exact duplicate/subject-claim check in the SAME unit
  observes provisional rows. SELECT/weak tickets cannot prove an earlier commit.
  Generic staged-return wording did not cover those reads/refusals, and internal
  provisional result rows needed distinction from durable/published success.
- AT18 grouped a BODY exception after COMMIT but before normal work completion
  with a later publication failure. The former can leave accounting committed with
  no normal work payload; the latter cannot erase/reclassify an already-produced
  committed result. Publication is after normal committed completion.

Actual author42dd/root11a revised these semantics and added AT21. Final
authorb972/root1a7 additionally corrects both durability-inference phrases: presence
**does not prove** prior COMMIT; returned values do not by themselves certify
durability or publishable normal success. A prior committed record and own staged
record are both possible; no positive OR negative durability certificate follows
from their transaction-visible presence. Final3c8 differs from intermediate
`d8e111a2c98a11ce9c53433efcf610300a22a70f` only by those two requested wording
replacements. All21 examples and every other byte remain unchanged.

The actual protocol now explicitly makes ALL port reads/lookups/duplicates/claims/
stages provisional internal returns. Same-unit exact duplicates add no rows; another
key cannot award a staged subject again, and refusal leaves earlier caller staging
active. AT21 uses state-b freshness so that claim checking is not hidden by a stale
token. AT21 specifies two fresh branches: abandon/known rollback leaves prior
durable intake pending; actual COMMIT plus normal completion exposes one
application/result/job. Actual prior intake/capture commits are honest CALLER
preconditions, not inferred from SELECT or satisfied by same-unit source staging.
AT18 now distinguishes the two exception times correctly; AT15 explicitly preserves
the requested amount and actual0. Both substantive findings are resolved.

## Scope, accounting and port consistency

- B08 separates already durable intake from the whole application unit: effect,
  state/status/claim, required job intent, result and audit commit or roll back
  together. Known failure cannot preserve only successful actions. Unknown outcome
  is not known pending/no-effect and grants no replay; prior committed lookup data
  cannot turn THIS failed work into normal success.
- Exactly one caller owns the actual writer transaction. Existing13/14 transaction
  wrappers own commit/cleanup after adoption;12/15 stores borrow sessions. The new
  proposed port cannot pretend those owning services compose inertly unchanged.
  Allocation/known rollback/uncertainty, pure/domain no-own-write and postcommit
  publication obligations preserve actual writer truth without a destructor promise.
- Original intent is separate from freshly computed state/result; scoped operation
  key/payload consistency, original typed-subject claim and per-reversal/original-
  effect uniqueness prevent duplicate awards without excluding distinct partial
  refunds. CID0/ReceiptId domains, ordered request occurrences and actual typed
  references remain explicit. Opaque refs acquire no invented semantic FK or authority.
- Caps/debt/lifecycle use CURRENT advanced state, distinct from retained capture
  and preview freshness. Requested/actual and Main/debt/capped components stay
  separate. Only main_added rescues eligible Active Grace/wait; hard end/Finished
  cannot be reopened. RequireReview holds the whole deficit TIMER adjustment and
  preserves reversal provenance, without inventing global status/job decisions.
- Cumulative refunds use original ACTUAL capped credit/rates and prior reversals;
  full refund clears the remainder, whole counts reverse only on full refund.
  Main-versus-prior-debt-repayment allocation and held multi-effect policy remain
  explicit ACCT-006 gates. Exact rational VALUE, original decimal spellings,
  signed64 units/residuals and overflow-without-partial-effects remain distinct.

## Independent checks and remaining gates

Actual `python3 /tmp/storagecontract001-arithmetic-review.py` passed on final3c8:
21 unique ordered AT rows, no project producer invoked. Independent Fraction/
integer oracles include current58→60min actual120000/capped480000; debt-first
2/5/3 and above-cap0 headroom; debt-only0 versus eligible Main+2; all deficit
policies; signedMIN with Main1→debtMAX versus Main0 overflow; refunds75000/10000000
then225000/30000000; cumulative rounded0/1/1, increments0/1/0 and residuals±1/3;
3/2 and5/2 half-even→2/2 with residuals-1/2,+1/2. Policy/lifecycle and AT21 control
flow were checked against the actual existing contracts/ports, not a fake runtime.

Static preservation check proves18 original rows byte-identical and all original20
numeric literals unchanged; only AT15/18 prose and new AT21 differ. Final project/
link/render/diff checks pass:

```sh
python3 /tmp/storagecontract001-arithmetic-review.py
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Project391/122/current HTML; host Python3.14.8. Independent tooling unittest19
passed0.272s on original integrated4fd; tooling/test code is unchanged by the
subsequent documentation-only revisions. No unchanged native suite, new feature,
stub/red or live operation was invoked. These are documentation/tooling checks.

Approval does not finalize concrete C++/SQL/wire layouts, state-version coverage,
vector/query limits, job/group envelopes, opaque namespace/provider correlation,
permission/evaluator policy, recovery/outcome resolution or refund allocation.
Their named owning gates must provide actual artifacts before consumers are enabled.
STORAGETEST-002 still needs concrete API/physical precision, independently authored
fixtures/tests, different actual review, successful compile and genuine intended
red; later implementation/passing acceptance/source review/automatic CI remain.
All seven integrations/three FX providers, full app/effects/delivery, UI/renderers/
locales/themes/platform/security/backup/recovery/live release obligations remain.
Root completion metadata, latest published-head CI/reviews/discussions/rules and
normal merge are separate future gates; no such external state is preapproved.

## Actual final documentation completion and scope review

2026-11-02, contribution_reviewer. **APPROVED: actual documentation-completion
metadata and scope attribution**, root
`1f046deef45b81f1591db93a5d1d8b05c197df97`, normally merged into this isolated
review tree at `1574f3c52c604be1a1b10ed0d2ec3412161e0625`.
No blocking finding remains; no executable or future merge approval follows.

I read the actual state/HTML/map/brief/architecture/test strategy/checkpoint and
[completion handoff](storagecontract-001-handoff.md). Final contract
`3c8cbd869800f6aa4cd117016ef568f789d3ace9`, author evidence
`10b0864d6a6244da442f32e0ecaabe333f3ed6a0` and original actual review blob
`60ad2225931a3abe48c4703be29fbe37aa6693ca` are unchanged. Root7da0c7a contains
exactly the prior270a1d91 review content; its original findings, author revisions
and actual final-artifact approval remain preserved. No executable/schema/C++ API/
harness/fixture/test/tool/build/workflow changed from integrated mainfba7590.

Done means an accepted LOGICAL DOCUMENT: internal row/port obligations and21
literal examples only. State records contract authoring/review/documentation
checks passed, but github_ci/product_acceptance/platform_release **not_run**.
The existing prerequisite set is exactly CONTRACT-002/STORAGE-015/ACCT-003/CORE-005,
all done; ACCT-004/STORAGE-002 remain consumers rather than prerequisites/cycles.
DB-01 remains partial, with prior scope/test coverage preserved and only document
evidence appended. No logical example is promoted into a feature test or runtime
capability, and no old173 pass or original red is repurposed as bundle acceptance.

The two substantive findings and two inference phrase corrections are accurately
linked. AT21 specifies provisional same-unit lookup/duplicates/claims and fresh
commit-versus-abandon branches; it is not an observed transaction. AT18 correctly
distinguishes postCOMMIT/precompletion body failure from later publication failure.
Prior-commit caller facts, one actual owner, B08 known rollback versus unknown,
Main/debt/held/refund provenance and no durability certificate remain obligations.
Unknown state-version/job/semantic/intent/authority/wire/recovery/refund-allocation/
provider meanings retain explicit owning gates instead of fabricated values.

Earlier author/root/reviewer arithmetic and tooling evidence is correctly scoped
to documentation. For reproducibility, the already-used independent audit
`/tmp/storagecontract001-arithmetic-review.py` has SHA256
`a2b47bf09d63d023c569dc816f03119139f0bebdc39fe79bac5820d7cc5af95b`;
the original reviewer19-tooling-test log `/tmp/storagecontract001-review-tooling.log`
has SHA256 `e8af89f4159eba3eb66021a690618be768e1d666c1450c0d194d0affd5aaf8ca`.
These hashes identify existing completed checks; neither arithmetic/tooling nor
any native suite/red was repeated for this metadata-only review.

Actual final static checks passed:

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Read-only pin/graph/diff inspection also confirmed all statements above;
project391/122/current HTML/links remain consistent. Initial published03c610f's
eleven inherited checks are explicitly claim-head CI, not final-document CI or
bundle behavior. The final published-head result is still a separate future gate.

Concrete STORAGETEST-002 API/physical/limits/closure, independent real fixtures/
tests, different actual review, successful compile and genuine intended red still
precede production. Later source review/passing acceptance/automatic CI, all seven
integrations/three FX providers and full application/delivery/recovery/UI/platform/
security/live release obligations remain required. Root must publish the resulting
reviewed head and observe its own latest CI, discussions/required approvals/current
rules/mergeability before normal exact-head merge, then reconcile actual main.
This final documentation verdict certifies none of those future external states.
