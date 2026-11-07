# SESSIONCONTRACT-005 — Concrete lossless and secret-safe ingress solution

Parent: [EPIC-007](../EPIC-007.md). Kind: contract, document only.
Specification v0.45 §2/§10/§17, SERVER-01, AUTH-12/15, API-01, TEST-04/05.
Dependencies/status: progress/state.json. Import accepted transport v1 §1–5,
supplier v1 and existing production bridge contract boundaries unchanged.

Own a focused actual selected-framework ingress/secret-handling proposal, independently
derived wire examples, source findings and author evidence documents. The integrator
owns any dependency pin/build/CI/deployment/schema change. No parser/library/proxy
selection, patch, configuration guarantee or implementation is approved by ADR-0010.

Address selected Drogon's documented duplicate header/cookie loss and Cookie TRACE
before loss/logging can occur. Propose a concrete, reviewable integration point or
alternative correction, retaining complete name/value occurrences or rejecting
ambiguous input before normalization, with bounded lifetime/preallocation and no
secret diagnostic path. An application map wrapper/filter cannot reconstruct lost
multiplicity or erase an earlier library log. Inspect relevant actual pinned source
and official documentation; unknown behavior fails unavailable rather than proving
lossless/TLS/log safety.

Define actual HTTP framing/HTTP2 combination support/refusal, parser and framework
allocation bounds versus semantic adapter limits, Caddy/alternative proxy and direct
listener trust boundaries, configured canonical origin/authority and TLS ownership,
secret-copy/log lifetime and ingress-policy revision production/change. Preserve
closed CORS and fresh-ingress retry after policy revision changes. Name deployment
requirements and which actual paths/versions can be claimed only after tests; do
not silently require an unowned proxy or promise browser/platform behavior.

Supply literal raw repeated Authorization/Origin/marker/Cookie examples, duplicate
account cookies within/across fields, empty/combined/invalid fields, oversized input,
parser/log failure, spoofed forwarding/authority/TLS and policy change. Define
reproducible real-stack red/pass and secret-log observation without real credentials.
Compare requirements and selected artifact compatibility before any future pin or
patch proposal; a substantial reusable runtime prerequisite needs another split.

A different reviewer must examine the actual source-backed solution, complete trust/
secret/bounds contract and proposed tests. Completion supplies SESSIONTEST-010's
contract only. API-001 still owns queue/lifecycle/backpressure; SESSION-003 still
owns complete transport parsing/egress/route integration and supported-client gates.
Record reviewed revision, audits/checks and handoff, with no live/provider operation.
