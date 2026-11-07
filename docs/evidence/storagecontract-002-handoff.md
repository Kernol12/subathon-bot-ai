# STORAGECONTRACT-002 — account/session storage contract

Coordinator `/root`, 7 November 2026. Branch
`task/STORAGECONTRACT-002-account-sessions`, `/tmp/subathon-storagecontract002`,
from current main `e4d46d335e52850acc58a73f2500725cffb7c534`. [Brief](../tasks/STORAGECONTRACT-002.md).

Fresh `session_storage_contract_author` owns only new
`docs/contracts/sqlite-account-sessions-v1.md` and author evidence in an isolated
worktree. Root owns shared migration order/schema/build/CI/pins/state/HTML and
integration. No production header, migration, fixture, test, stub or behavior.

Propose exact physical/types/constraints/indexes and C++ current-reader/conditional-
mutation API for existing accepted supplier/lifecycle/password contracts, on the
existing caller-owned writer session. Preserve hash-only sessions, source-bound
credential facts, immutable capture epochs versus current epochs, no extra writer/
nested transaction/history scan/cached authority, metadata lengths before ownership,
full numeric domains, coherent snapshot references, finite keyset inspection and
honest provisional/commit/unknown outcomes. Shared fact creation/update handoffs
must preserve setup/roles/permissions/recovery/provider/operational ownership.

Next: coherent schema/API/literal packet, different actual review, focused checks
and normal task PR gates. Existing prefix stays byte-identical; actual migration
changes require their later separate test-first task/root integration. This document
alone supplies no accepted login/current producer/restore or platform capability.

## Accepted document packet and integration

Actual fresh author5c3acde0709da625a43e9da52da3363e92e54502/importb01cc18,
contractblob8dc7ba9fce0e40bcfa02897c6d88946849cbd1b1,
[author record](storagecontract-002-author.md). Different actual reviewer
393015e58cf10bcf640cb52b357abd45deca9581/importb996848
[APPROVED](storagecontract-002-review.md) exact b01cc18. All24 literal cases,
physical/types/tenobjectinventory and actual unchanged prefix7 framing were audited;
no SQL/native/feature was executed. No blocking finding or document correction.

Root integrated current main e440c7f (normal supplierPR84), resolving only shared
JSON/HTML/checkpoint; retained all443IDs/1735edges and every other row/review.
Current immutable prefix1–7 and all imported reviewed documents/APIs byte-identical.
Proposed version8 is not an allocated/implemented manifest; selected-driver exact
102-object fixture and tests/firstred are future STORAGETEST020 gates. Done means
this document contract only, never a current source/login/session/restore capability.

CI assessment: existing documentation/project/tooling checks fit scope; no new
executable suite/workflow/pin. Actual source/schema/tests remain later tasks with
their own reviewed/red/implementation/CI gates. Next focused PR/all latest checks,
current rules/discussions and normal merge, then fresh bounded test author.

Root actual 7 November checks after main integration: `python3 tools/progress.py render`
exit0; `python3 tools/check_project.py` PASS443records/122decisions;
`python3 -m unittest discover -s tests/tooling -v` exit0/19PASS/0.347s;
`python3 tools/progress.py render --check` exit0/current;
`git diff --check` exit0. Python3.14.9/Linux shared coordinator environment.
No configure/build/CTest/SQL/native/network-feature/live execution.
