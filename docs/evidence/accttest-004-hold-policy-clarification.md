# ACCTTEST-004 negative-time hold provenance clarification

Author root coordinator, 2026-11-03. DOCUMENT proposal only; actual independent
review is pending. No service, codec, view, test body or missing marker has run.

Actual complete test review by bundle_reviewer found that the existing MIN
overflow case distinguishes two targets: its captured request selects main,
whereas the failed arithmetic balance is debt. The old hold matrix requires a
present FailureEvidence target to equal its captured occurrence target. Those
selectors must not be conflated or changed to bypass old validation.

The reviewer and API/test author independently identified a genuine provenance
gap: SAH1 Policy retains a policy revision identity but omits the actual selected
per-ordinal correction enums. No existing contract requires that identity to
resolve a durable immutable policy document. A whole-input numeric hold cannot
reuse timer-deficit evidence; its selected CarryTimeDebt enum would be lost.
Logical service whole-input evidence and explicit negative correction policy
requirements, specification v0.45 §§3–5/NUM-04 and concrete CS13 require this data.

The bounded proposal adds SAH2/v2 for known nonempty correction lists only. It
keeps the complete SAH1 field order and appends the full ordered correction
vector plus optional explicit failed-balance discriminator. Typed target remains
main; the MIN example retains debt1, exact attempted2^63 and CarryTimeDebt1.
All applicable copies are identical and nonrecursive. Parsed suffix ownership
is intermediate data, while returned opaque hold accounting charges real bytes.
There is no header, storage, schema, writer, success-wire or old SAH1 change.

Root owns this bounded concrete-contract edit and this note. Full reviewer must
inspect and approve the actual revision before independent fixture authoring.
Proposed next DATA work is one additive min_policy_overflow case and fresh
hMin/dMin/vMin, preserving every old case object/oracle. The independent fixture
author must derive all SAH2 bytes/B-I and real old-only storage closure; separate
actual fixture review and OWN updated selected foundation CI precede affected
tests. Complete test review/current-head CI and FIRST same-clean named red remain
required. Existing reviewed evidence remains valid for its pinned artifacts;
there is no prospective approval or completed production service claim.
