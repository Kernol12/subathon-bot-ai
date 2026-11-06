# SCHEMATOOL-001 — context source audit, changes requested

Reviewer `/root`, different from worker author `/root/session_supplier_review`,
6 November2026. Actual worker commit9f88767984f1febede7a402357d1c8f5b6b1ca82,
root importf1bdaee. Backend blob5ed074a89b5831ef7a1e304cbbe846734b2d8b8e;
index37bb83799aa587bba7edb2db368b8b7b20d86870;
commonafcf8b2eb6258d27031cbdb3804620968c9dacab. Verdict: CHANGES REQUESTED.
This source audit executes no candidate/helper/native methods and does not approve
future code. Separate C review owns worker prologue/snapshot/installation security.

The actual design delegates assertions to pinned Draft/OAS validators. It uses a
real registry with deny retriever, public Specification bridge for indexed OAS schema
roots, native IDs/subresources/anchors/lookups, a real SchemaAccessor and DATA-only
SchemaPath.read_value override. Whole raw structural schemas/default/instance
controls, complete closure before cycle/inventory and raw DATA borrowing are
coherent directions; actual source below still blocks approval, and full acceptance
is not established by those directions or the author's finite observations.

1. Backend ContextAccessor.context calls alias for Schema as well as OAS Reference
   Objects at lines316–320. It presents a target in place of the original Schema
   occurrence, dropping semantic $ref siblings. Actual OSV0.9.0 allOf/required
   semantic check supplies a concrete missed path. Accepted context §1/§5 require
   raw siblings/native semantics. [New focused regression author derivation](schematool-001-worker-context-regression-author.md) and
   [different test review](schematool-001-worker-context-test-review.md) document
   the source-derived oracle before any reached-bug run/fix.
2. Index.walk returns before its structural extension check for Schema and typed
   Reference Objects. OAS x-* FIELDS at these positions therefore bypass the frozen
   unsupported_feature profile. Dynamic map names/DATA and standalone Draft unknown
   keywords remain exempt. The same new regression covers both missed cases and
   the positive exemptions; do not rewrite the contract to fit implementation.
3. Context §3 requires bounded record indices for edges and counts retained graph
   key aliases, with reserve/check before growth. Node.identity builds fresh
   (document,pointer,role) tuples retained across target/visit/outgoing/inventory
   tables, while Budget accounts only selected spellings. Reusing string objects
   does not exempt retained aliases under the explicit contract. pointer_child
   constructs the resulting pointer before the aggregate Budget.key check;
   locate performs native lookup/urljoin before admitting the absolute key.
   Replace repeated graph identities with bounded numeric record indices and make
   key ownership/reservation explicit before construction, including native-entry
   controls. Existing independently reviewed/red graph-key/record/edge tests cover
   this behavior; no algorithm-mirroring new test or frozen-assertion change is
   required. Actual corrected representation/allocation paths need source review.

The metadata bridge must retain lexical scope across no-id/relative-id OAS roots,
boolean contexts, aliases, whole-fragment controls and encoded pointers. Source
uses actual SDK operations, not an assertion engine, but finite author checks do
not approve all paths. Continue auditing complete physical roles, native exception
mapping, ordered phases, required/free example pair equality, cycle graph bounds
and no IO fallback against actual corrected source and full public acceptance.

No changed public assertion, schema fixture, contract, limit, pin, dependency or
release scope is requested. Authoring/fix ownership remains with A; root owns this
review and integration. Unknown source behavior remains unverified, not supported.
