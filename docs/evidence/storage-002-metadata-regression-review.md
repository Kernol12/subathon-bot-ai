# STORAGE-002 — independent quota and metadata test review

Author: bundle_validation. Reviewer: bundle_reviewer, separate agent/worktree,
2026-11-03. Actual candidate068249ea0a51d24e1d2c48d9d884798d14b271f4,
test blob03e7a9d, SHA2568c4bccc096d6cfba7d4a28ec8699ff76555e927f25f7b1d4337aae79a39e64fa.
Verdict: **approved for the additive regression baseline**, before first execution.
No runtime correction/full-source/CI/integration approval is supplied here.

Read actual143-line appended test diff, author evidence, concrete bundle revision1,
public DTOs, frozen JSON source/schema and actual f969ba6 page/read implementations.
Programmatic removal of ONLY appended two-slot/data-helper block restores original
4b60fcaa source byte-for-byte. All original assertions and fixtures are preserved.
Root separately owns the already-reviewed terminal oracle correction and another
agent's returned1105-byte test; this review concerns ONLY these two new slots.

The returned-quota test installs an EXISTING complete original using independent
frozen INSERTs. Public lookup receives no PreparedBundle, so input_items1 cannot
limit that read. result_items20 permits the literal2+2+2 balance entries+4 effects+
8 components+2 jobs+0 transitions+0 reversals. `matches` checks every typed member
against independently frozen values; the explicit counts validate20 instead of
generating an expected number from implementation. Complete physical typed snapshot
remains unchanged. This genuinely reaches f969's wrong reuse of input preflight.

The three metadata Qt data rows exercise distinct public projections. They append
an extra kind1 balance while retaining declared2/valid2; an extra transitionordinal0
while retaining declared0/valid0; and an extra per-basis position0 while retaining
positions1/2 and exact terminal head2/cumulative100/refund2/request1. Explicit raw
actual-count oracles expose each outside-keyset row before UUT access. Updating an
existing valid row to0 would have produced an already-detectable gap, so append is
necessary. The progress witness correctly uses existing score-refund/request1 and
the existing original Main basis; its PK differs from score-refund's existing score
link. Every real FK remains valid, and CHECK enforcement is restored before API
calls. The selected original source is unchanged; the narrower projection does not
certify the unselected current score-refund application's full payload.

Expected invalid_stored_data follows the contract's ACTUAL-count/full ordinal and
history closure, not adapter-specific SQL. Existing refusal helper observes earlier
caller staging, catches the precise typed failure and proves a real independent
BEGIN while the facade remains alive; final marker absence and unchanged complete
corrupted snapshot prove whole-unit rollback without repair. Writer limits use the
existing adequate finite budgets. These cases cannot pass by later outer cleanup,
truncating to the valid subset or classifying an unrelated FK/setup failure.

Actual reviewer static checks: full diff/contract/frozen-literal inspection;
programmatic exact original32 preservation; git diff --check. One initial scratch
removal script retained an extra newline; corrected boundary restores exact source.
This was read-only text comparison, not compilation/test setup or behavioral red.
Author's actual successful compile/-functions34 proof is recorded separately; data
helper is not a test slot. Root must compile/discover integrated35 slots including
its separately approved byte regression, then observe FIRST meaningful failing
new cases on uncorrected runtimef969 BEFORE importing held93+3e corrections.
No case body or production entrypoint was invoked by this review. Full runtime/
config review, all35/full209 passes and observed ordinary selected-Qt CI remain.
