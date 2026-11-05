# CONTRACTPLAN-004 — Explicit session supplier contract gate

Kind: planning/contract. Status/owner/dependencies: progress/state.json.
Authority: specification v0.45 §§10/17/18, AUTH-05/AUTH-12, API-01/02/03.

Reconcile CONTRACT-013's missing SESSION-003 contract prerequisite with the actual
ready graph. Existing approved wire primitives explicitly defer cookie/CSRF and
desktop-header transport; password hashing and permission evaluation supply no
current-session producer or transport contract. Do not invent those capabilities.

Add one bounded DOCUMENT-only SESSIONCONTRACT-001 supplier/API/transport task and
explicit prerequisites for CONTRACT-013, SESSIONTEST-002 and SESSIONTEST-003.
Preserve every existing dependency and later runtime/acceptance/test-first gate.
Record the requirement inventory, exact edge additions, compatibility impact,
unchanged release scope and independent graph/document review in an ADR/handoff.

Root owns task briefs/navigation/ADR/evidence/state/HTML. Separate reviewer owns
only a new planning-review note on its isolated branch. No production/interface,
wire schema, test assertion, migration, dependency pin or workflow changes. This
plan must not finalize any auth token/cookie/CSRF choice or claim running support.

Run actual graph/link/render/tooling checks, independently review final artifacts,
then publish one focused PR and normally merge only after latest CI/rules/reviews.
Existing automatic consistency/component CI suffices for this document-only change;
no new executable suite or required-check configuration is introduced.
