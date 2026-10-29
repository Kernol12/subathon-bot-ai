# ADR-0001 — Agent development foundation

Date: 29 October 2026. Status: accepted planning baseline.

The product specification v0.45 is copied unmodified into docs/specification.md. This
starter supplies a C++20/CMake foundation, optional Qt >=6.12 probe, a deliberately
unimplemented domain contract and draft GoogleTest cases. Interfaces/build scaffolding
are allowed before production feature implementation. Test agents/reviewers establish the
baseline first. Default builds do not require production dependencies or contact services.

PROGRESS.html is generated from progress/state.json with Python standard-library tooling.
This avoids browser file-URL fetch restrictions and a separate runtime server. Status is
edited in JSON and checked in with regenerated HTML. Verification remains explicit.

No decision makes draft tests approved, live integrations supported, or the complete
application implemented. Remaining prototypes and release gates stay in the specification.
