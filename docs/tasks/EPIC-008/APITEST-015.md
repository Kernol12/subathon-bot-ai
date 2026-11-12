# APITEST-015 — Correct and extend the host listener baseline

Kind: test_baseline. Parent: EPIC-008. Specification v0.45 §2, SERVER-01,
API-01 and §17–18. Status/owner/dependencies: progress/state.json.

Prerequisite: APICONTRACT-004 approved isolated-listener contract, APITEST-001
reviewed original baseline and API-012 corrected native profile. API-001 remains
responsible for production host/transport implementation after this baseline.

## Scope and preserved evidence

Resolve the documented API-001 B1 contradiction using two INTERNAL listeners in
one process, one framework lifetime and one IO loop. The new contract selects
actual fields, endpoint observations and literal examples. Author the narrow
api_host_drogon.hpp declaration/stub refinements and native fixture/test changes
needed by that approved contract; coordinator owns shared build/admission/CI and
progress. Read tests/server scoped AGENTS and full affected contracts.

Keep every existing assertion and expected behavior. HTTP cases target the
explicit ordinary endpoint; native WS cases still target the isolated profile
endpoint. This is an intentional, independently reviewed baseline revision,
not permission to skip failures, infer serving, manufacture readiness or alter
API-012 source/tests/patch/SDK. Preserve original PR112 source8fa8810, red dd0f625,
all review histories and proof limits. No assertion deletion is authorized; any
such proposal needs separate itemized ADR-0012 review before any edit.

Add focused contract-derived cases for HTTP reachability, rejected Upgrade on
ordinary listener, rejected ordinary HTTP on native listener, no implicit static
file/index/default-route leakage, coherent dual bind/readiness/stop failure and
shared finite resources. Retain ALL HL01–17/75Qt/18N behavior and every existing
SQL/source/thread/lifetime/elapsed/cancellation/true-outcome assertion. Inventory
counts may increase only with declared exact registrations and preserved old
methods; no count inflation substitutes for behavior. No fixture listener or
alternative production transport.

Authored tests/interfaces/fixtures/resources need the full independent Claude
initial → Codex fixes/separate verification → Claude closing cycle BEFORE
execution. Retain native new-consumer passive ELF/ABI/controlled load and runtime
map admission. Observe intended missing-behavior red on a working compiled
foundation; setup, compile failures, skips and unreachable assertions are not red.
Publish exact revised baseline and old/new mapping in the focused PR. No
implementation, admission waiver, completed host or release support is claimed.
