# STORAGE-018 — Durable initial source/capture binding

Parent: [EPIC-009](../EPIC-009.md). Baseline STORAGETEST-018; implementation
STORAGE-018. State/owners/dependencies: progress/state.json. Specification v0.45
§§3/5/17/18, DB-01/API-02/NUM-04. Consumer contract:
[reference application adapters](../../contracts/reference-application-adapters-v1.md).

Provide the narrow caller-session durable index for the source's FIRST immutable
arrival/plan/execution binding. The consumer contract defines exact identity/data/
lookup/retain semantics; independently review its concrete storage API and additive
physical schema/migration before minimal stubs and literal real-SQL fixtures/tests.
Preserve existing receipt/contribution/plan/retained stores, reviewed assertions and
immutable migration prefix. Root exclusively allocates/coordinates schema versions.

Acceptance requires exact scoped identity and typed positive ReceiptId/CId0..MAX
handling; complete referenced immutable closure; exact already-present versus
conflict; first binding never overwritten; no hash-collision uniqueness assumption,
raw unbounded scan or process-only map; same session stages initial plan, SRX1 and
index as ONE atomic unit, provisional until real normal acknowledged commit.
Separate absence, malformed/oversized/missing-reference/conflict outcomes and actual
known rollback/commit-unknown. Borrowed full input/read/result budgets precede
ownership; use existing writer capability/connection/thread/SQL-budget boundaries.
Test duplicate/interrupted/retried capture, competing candidates, empty/off-scope
reads, referenced-record corruption, rollback visibility and checked complete bounds
with independent expected values and disposable state. No application claim/effect,
authority/rule decision or recovery capability is manufactured by this index.

STORAGETEST-018: contract/examples → independent actual contract/API/schema review
→ minimal interfaces/explicit immediate stubs → independent fixtures/tests/review
→ compiled genuine intended behavior red (not dependency/setup/compilation failure).
STORAGE-018 waits for that integrated baseline; preserve assertions, implement only
this adapter, run relevant acceptance/regressions, obtain different source/config
review and observe ordinary selected Qt CI. No skipped/inverted known-red suite.
Record exact revisions/commands/results/environment and CI coverage/gaps. API011
baseline AND implementation depend on the accepted producer, not just this plan.
All provider/recovery/full release requirements stay under existing owner tasks.
