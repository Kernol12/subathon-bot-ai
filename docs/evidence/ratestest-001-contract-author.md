# RATESTEST-001 logical snapshot contract author handoff

Author service_author, 2026-11-04. Isolated branch
`task/RATESTEST-001-contract-author`, worktree ratestest001-contract-author, from
integrated main 0b631dc plus explicit claim 6338966. Owns ONLY
[rate-snapshots-v1.md](../contracts/rate-snapshots-v1.md) and this note. Existing
ACCT/locale worktrees and unrelated work are untouched. Root owns all shared
state/HTML/build/schema/pin/CI/checkpoint integration.

Read root instructions, architecture/workflow/test strategy, RATES-001 and adjacent
provider/scheduler/fallback/manual/history briefs, specification §§3.5,9,15 and
FX-06/NUM-04, CONTRACT-002, existing exact-decimal contract/header, ReceiptMoney and
HistoricalValuation/application-plan public types before proposing this document.

The bounded model keeps original money/rate spelling and opaque currency identity,
fetch/source/publication evidence with honest unknowns, fetched observations separate
from effective data, and canonical positive exact currency potentials. Every
foreign rate derives as a ratio; explicit selected fetched/manual constraints back
connected components and inconsistent candidates reject without choosing/discarding
pins or revaluing history. The original source may contain rounded inconsistent
quotes, retained as observations rather than rewritten into false consistency.
Source support/currency registry/ISO spelling is not inferred; all three providers
retain their actual verification tasks.

Found and reported the concrete existing bridge gap: inverse1.20 is exact5/6 but
HistoricalValuation/STORAGE-015 currently preserve effective_rate_decimal. Root
created explicit RATESTEST-009/RATES-009 dependency-tracked work for an exact
valuation/plan/command/storage bridge. This document does not round recurring values,
retrofit a header/schema or claim that bridge works. Reporting-display changes never
recompute credited time/score, completed milestones or original-effect refunds.

RS01–17 are manual literal algebra/provenance/rejection examples, not generated
expected values, source fixtures or executed tests. No provider/network behavior is
claimed, so this pure-model proposal needs no speculative external capability
lookup. Current primary provider documentation remains required in owning adapters.

No C++/API/stub/schema/storage/fixtures/tests/feature execution/production behavior
was authored or invoked. Different actual document review precedes further stage;
RATES-001 remains planned. Scheduler/cache/HTTP/auth/quota/live metadata, pin-solving/
update/permission workflow, UI and history/display/credit bridge remain their tasks.
Root records publication/PR/status and all future gates separately.

## Actual checks

Documentation/link/progress checks follow. They establish no new numeric/product
feature behavior, provider capability or implementation acceptance.

- python3 tools/check_project.py — PASS: 396 task records, 122 decisions, local
  links, presets and current HTML in this claimed author branch. Root's newer
  integration records are not overwritten or claimed present here.
- python3 tools/progress.py render --check — PASS current snapshot.
- git diff --check — PASS; staged new-document whitespace check accompanies commit.
No executable numeric/provider/feature test or implementation was invoked.
