# ADR-0003 — Separate application semantics and durable review holds

Date/status: 2026-11-03; proposed for independent contract/planning review.
Authority: specificationv0.45 §§3.3–3.5,4,5; NUM-02/03/04, DB-01.

## Actual gap

After integrated STORAGE-002 (PR43), application-bundle storage can atomically stage
successful supplied outcomes. It deliberately cannot authorize/evaluate them or
persist a standalone whole-input numeric-review hold. Returning a transient Held
while current eligibility remains clear cannot enforce NUM-04's durable review
requirement. The old contribution adapter transfers transaction ownership; silently
nesting it or overwriting the one existing review dimension loses independent
conditions/provenance and does not supply a reviewed hold repository.

## Executable sequence

1. ACCTCONTRACT-001 defines documentation-only application-service semantics and
   literal examples, including explicit typed current access/policy/clock ports,
   stable hold identity/history/authorized resolution obligations, and actual
   commit/normal-completion publication. Different actual review is required.
2. STORAGETEST-016 → STORAGE-016 separately defines/reviews concrete nonowning
   durable application-hold storage, exact additive schema/API/fixtures and a real
   intended red before production implementation. Preserve immutable prefix1–5.
3. Original ACCTTEST-004 → ACCT-004 then establishes/implements the actual service
   over integrated numeric/bundle/hold prerequisites. Old209 passes are neither
   service acceptance nor its own intended red. Its original atomic/idempotency
   requirements remain; this is no aggregate-red shortcut.

The service contract does NOT depend on future service tests/implementation.
Hold storage depends on that reviewed logical contract and integrated storage;
service baseline depends on implemented hold storage. No epic/child cycle.

Whole-input numerical overflow must hold without applying any mixed effects/jobs.
Timer-only deficit RequireReview is a distinct CORE-005 proposal and cannot silently
become an equivalent whole-product status. Holds need stable tagged subject gating,
not just one request key or plan revision; another operation/replan cannot bypass
review. Retain immutable cause/source/provenance and separately authorized active
resolution; do not erase another independent condition or infer authority from an
opaque document. Exact layouts/limits and the nonowning port await their own review.

## Scope and continuity

The initial ACCTTEST-004 claim on task/ACCTTEST-004-application-service authorized
only its logical-document stage. That same focused PR44/branch now delivers the
separate ACCTCONTRACT-001 executable contract task; its historical branch name is
retained to preserve published work. No service API/test/red or production change
was authored. ACCTTEST-004 returns planned behind explicit new prerequisites.

Batch/silent review remains ACCT-005; financial entitlement/refund allocation remains
ACCT-006; manual authority/cap/deadline commands remain CORE-008; milestones,
delivery/recovery/wire/provider/UI/platform work retains its required tasks. Signed
original corrections must be explicitly supported/mapped, not confused with
financial refunds. All seven integrations/three FX/full release scope remain.
No existing public API/schema/test assertion is changed by this planning record.
