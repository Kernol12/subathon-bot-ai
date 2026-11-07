# ADR-0011 — explicit ingress boundary reconciliation

Status: proposed under SESSIONCONTRACT-006; actual revision/graph review required.
Authority: specification v0.45 §§2/10/17, AUTH-12, SERVER-01, DEP-05, TEST-04/05.

Source-backed005 work is preserved at author4b0178cc/importd720282 on
`task/SESSIONCONTRACT-005-ingress`, [blocked draftPR85](https://github.com/Kernol12/subathon-bot-ai/pull/85).
It is not an accepted or complete ingress solution. Independent read-only audit
found an unimplemented engineering-contract conflict, not a product-scope waiver.

The app must preserve every received decoded occurrence before Drogon map loss and
prevent earlier secret logging. It need not invent an unchanged public raw-head
protocol. Yet reference013 explicitly bounds the original external request/H2
payload, which stock proxy handler data cannot recover after normalization.
Connection-nominated fields and normalized framing also prevent blindly treating
backend data as the original external request. Merely relabeling would be wrong.

Create a separate actual contract revision006, preserving old revisions/receipts,
security reasoning, affected oracles and compatibility. Prefer separate finite
edge/admitted normalization and application occurrence budgets over a new envelope/
proxy subsystem if the exact source-backed safety proof supports it. No choice is
preapproved; a necessary new subsystem must still be separately owned before code.
No release default/alternative proxy requirement may be removed, and optional
direct TLS is not a substitute for the bundled path.

Add native artifact baseline/implementation011 before source integration010.
Add independent bundled proxy007 and alternative proxy008 test-first admission
pairs; default full transport consumes007, existing deployment owners consume the
relevant profile. Preserve all original IDs/edges/full acceptance/test-first gates.
005 stays blocked on006 until the reviewed revision integrates, then its complete
source-backed solution still needs actual review. No old completed task is reset.

Task records/briefs are authoritative for the explicit split, not runtime support.
No parser/proxy/pin/toolchain/config/header/migration/test/source is implemented by
this decision. Root coordinates shared graph/schema/build/pins/deploy/CI and normal
latest-head merge gates; separate authors/reviewers own actual bounded tasks.
