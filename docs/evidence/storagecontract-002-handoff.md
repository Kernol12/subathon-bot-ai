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
