# ADR-0007 — Make the missing session contract an explicit prerequisite

Status: proposed by CONTRACTPLAN-004, pending independent review.
Authority: specification v0.45 §§10/17/18, AUTH-05/AUTH-12, API-01/02/03.
This changes task sequencing only. It chooses no authentication protocol details.

## Actual evidence and problem

CONTRACT-013 is graph-ready with CONTRACTPLAN-003 and CONTRACT-005 complete,
but its brief explicitly blocks finalization on missing SESSION-003 contract
details. The accepted reference-path-v1 proposal likewise leaves authentication
shape to that producer; wire-primitives-v1 defers cookie/CSRF/desktop-header
semantics. SESSION-002/003 briefs have no finalized slice contract. Password
hashing, pure effective permissions and wire integer/reference primitives do not
supply a current session loader or credential transport implementation.

Read-only source/document inspection found no session resolver or transport API
among current server headers and no such approved contract. Claiming HTTP schema
acceptance now would silently invent security behavior; gating all contract work
on SESSION-003 implementation instead would conflate document and runtime gates.

## Decision and exact additive graph change

Create DOCUMENT-only SESSIONCONTRACT-001 with a bounded supplier/API/transport
brief and different actual review. It depends on CONTRACTPLAN-004 and the existing
CONTRACT-005, SESSION-001 and STORAGE-002 component prerequisites. These components
are inputs, not proof of authentication. CONTRACTPLAN-004 itself depends on the
same three integrated prerequisites and closes only the sequencing repair.

Add SESSIONCONTRACT-001 to the existing prerequisites of:

1. CONTRACT-013: actual reviewed session/transport import before HTTP finalization.
2. SESSIONTEST-002: agreed concrete session API/semantics before its test baseline.
3. SESSIONTEST-003: agreed transport/current-session import before its baseline.
4. EPIC-007: aggregate the required new document child.

Every old edge is preserved. SESSION-002/003 retain their original baseline and
runtime prerequisites. APITEST-010/API-010 still require implemented SESSION-003;
no document completion releases runtime authentication or HTTP behavior. The
new task must not depend on SESSION-002/003 implementation, avoiding a cycle with
their test-first gates. If actual document scope needs splitting, preserve each
consumer's required accepted imports through explicit reviewed task updates.

The exact graph delta is two records and eleven edges: three new plan prerequisites,
four new document prerequisites, and the four consumer/epic edges above. From
integrated main41c4842's420records/1608edges, the result is422/1619. No old ID,
requirement, assertion, interface, schema, migration, dependency pin or workflow is
removed/revised. The separate in-progress STORAGE-018 claim lives on its dedicated
branch and must be preserved when this main change is integrated there.

## Boundaries and acceptance

The future document must retain opaque256-bit/hash-only sessions, agreed24h/30day
normal/remembered defaults and five-minute freshness, immediate revocation,
protected browser cookies/CSRF or desktop authorization headers, and prohibition
on session URLs/browser local storage. It must define the currently missing exact
choices and limits through actual review, not this planning decision. Own-installation
credentials and no project-hosted authorization broker remain unchanged.

No new API/header/schema/behavior/test/red is delivered here. All seven integrations,
three FX providers, platforms, both renderers, UI/localization/themes, recovery and
other release requirements stay in scope. Existing automatic consistency and stable
component CI cover this document-only repair; there is no new executable suite,
manual gate, pin or branch-protection requirement. Independent exact graph/document
review and latest-head CI/current rules/normal merge remain required.
