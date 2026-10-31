# ACCT-001 identified receipt service handoff

Coordinator `/root`, 2026-10-31, branch `task/ACCT-001-receipt-intake`;
[PR #16](https://github.com/Kernol12/subathon-bot-ai/pull/16), base main7900a61.
Specification v0.45 §§3/14/18; receipt-intake behavioral contract revision1.

## Implementation and preserved baseline

Implementer `/root/receipt_implementer` source875c1c9, exact blob
`7a0986bdd3e4b5e38d8b7ac18d76dd56ff002ee8`, opens one repository transaction,
finds the full stable identity, creates only if absent, compares incoming facts with
immutable original facts for retry/conflict, appends the full attempt and commits
before returning the result. No cache, auto-retry, identity inference or credit logic.
Exceptions propagate with transaction RAII per the port contract.

[ACCTTEST-001](accttest-001-baseline-handoff.md) integrated via PR15/main7900a61
before implementation: author28729c7, independent test review54cff97 and integration
a843e4d; actual15-case stub-red18d8670. Original core/service headers, tests
`94317dc4dde4553c8888f78a433e3003c471de1a` and fake
`99ff2eec97a10c89dc93f0aec5b8c4cee88906a8` remain exact. Contract baseline
`149d65b5aa0864c6323e4d5ff6d9580cdbcf2886` normative identity/decision/port semantics
are unchanged; current contract/brief edits update status/historical framing only.
No assertion or fixture was removed, skipped or weakened; no new algorithm outside
this already-reviewed slice was introduced.

Separate `/root/baseline_reviewer` approved actual source875c1c9 plus configuration
e1b47f1 in ab28d70. [Code review](acct-001-code-review.md) checks original retention,
exact identity/facts, every-attempt evidence, transaction failure/commit ordering,
RAII/no caching and honest CI/scope. [Implementer evidence](acct-001-implementation.md)
records the exact commands and unchanged artifact identities.

## Actual checks and CI

Root receipt-intake-debug configure/build/CTest-Lreceipt-intake: exit0,15/15.
Author/reviewer receipt15, numeric33 and credit23 cases pass; reviewer scaffold3/3
also passed. Root Python19/project/current HTML/diff checks passed; final done-state
results are recorded in the task/checkpoint. Environment GCC16.2.1/CMake4.4.5/
Python3.14.8/GTest1.18.0; no local dependency install or real data used.

Automatic [receipt simulation run41207963538](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41207963538)
passed at PR16 head018e286792a9a61ca28771858d3e66b631ae744a: Ubuntu24.04/GCC13.3.0,
all15 cases with simulation/server labels, selected pinned checkout/GoogleTest refs.
Numeric/credit/PR+push scaffold checks also passed. [CI evidence](acct-001-ci.md)
records configuration, actual result and missing required-status-rule enforcement.
No protection settings changed. Final completion-record head CI/discussions/review/
merge eligibility must still be rechecked before merge; done is not merged.

## Verification limits and continuation

The fake supplies primitive staged storage and known rollback only; production
service owns the decision. These passing checks are fake-backed simulation of the
port/service boundary, not SQL uniqueness/durability/restart/real commit uncertainty,
Qt writer/thread/concurrency, acknowledgements or actual provider support. Identified,
already-validated input is a caller precondition; missing IDs/source authority and
verified correlation remain API-006/connectors. Contribution creation/payment
eligibility and one-application effects remain ACCT-002/004. No live provider,
credentials, deployment or real supporter state were used.

Complete final independent integration review and latest PR gates, then merge if
eligible and start the next dependency-ready baseline from refreshed main. Timer
PR14 remains separately blocked on its unanswered confirmation-wait credit choice;
no timer implementation or reviewed red baseline is claimed. Full release scope
(all seven integrations/three rates/platforms/renderers/UIs/packaging) remains open.
