# STORAGE-002 — independent review of returned-byte regression

Reviewer: root coordinator; author: bundle_reviewer. 2026-11-03. APPROVED exact
additive test commit291047f18b084019b85d5e5907ace7d8d45dba47/testCPPfadaed4 and
its independently derived oracle; runtime/code approval remains separate.

Root inspected the full53-line additive method and public DTOs, frozen input
INSERTs and rollback helper. No original32 assertion/fixture/port changes. New
public read_inputs test requires a complete ready record at1105 logical bytes and
result_limit at1104, actual marker/whole-unit rollback, and unchanged physical
snapshot. Finite ample writer budgets isolate this from writer consumption limits.

Root independently parsed ONLY frozen JSON input literals with Python stdlib (no
production helper, UUT output or SQL). Independently summed actual DTO members:
identity8/facts77/arrival22/receipt115/event134/capturedstate33/valuation53/
requests63+63+63+61/rules64/modifier46/capture588/currentstate59/currentrecord192/
supportConditions32/anchor190/snapshot112. Complete total:
1 subject+(6+8) PlanKey+8 head+588+112+192+190 = **1105**.
The Conditions member lacks a payment-evidence enum; its observation already has
one. Existing uint32 schema_version counts4, enums/bools1, domain int64/uint64 8.
Raw independently reproduced arithmetic: local-evidence/storage002-independent-byte-review.json.

This is actual separate TEST review before the new test body runs or helper
correction is integrated. Root must compile/register33 and observe the targeted
1105 erroneous result_limit against helper824 plus actual SQL source BEFORE held
helper159 correction; neither compiler failure nor missing-feature stub is that
regression red. Original32 historical red90fa remains unchanged evidence.
Full source/config review, relevant33/full207 and actual changed official CI still
remain required before task completion or normal merge.
