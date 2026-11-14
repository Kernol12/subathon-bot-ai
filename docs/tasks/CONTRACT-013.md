# CONTRACT-013 — Bounded reference HTTP contract and schema

Kind: contract.
Authority: specification v0.45 §17, API-01/02/03, §§2/3/10.
Prerequisite logical proposal: [reference-path-v1](../contracts/reference-path-v1.md).

Finalize a complete OpenAPI 3.1 package for reference.v1 negotiation and protected
committed timer snapshot ONLY. Independently review actual schema/examples/imports;
do not use the logical proposal as schema acceptance. Pin method/path/header/body/
query/content-type policy, session transport import and CSRF rules, current access,
HTTP status/error priority, finite transport/pre-scan/scratch/result limits and all
closed representations. Missing SESSION-003 contract details block finalization;
versioning cannot decide or bypass authentication. No implementation/client code.

Use common exact integer strings/opaque tokens, explicit required/null semantics
and no scene import. Cover all RP13 compatibility/disclosure cases and malformed/
oversized/duplicate-key input, complete reference resolution and numeric endpoints.
Advertise only actually supported capability reference.timer-snapshot.v1 after
implementation acceptance. No pending endpoint or provider feature may be listed.
Full HTTP006 retains all remaining endpoint families and imports this reviewed
package; CONTRACT003 retains full HTTP/WS/scene completion. Version changes must
record compatibility impact; no automatic generic-v1 alias.

Root owns shared schema imports/state/HTML/build/CI/pins; assigned author owns only
the approved reference package/doc/examples. Actual separate contract review
precedes APITEST-010; all later tests/red/code reviews remain. Run actual schema/
link/consistency checks, assess CI for executable validation and record exact
results without claiming a running server or schema-validator success not observed.

[ADR-0007](../decisions/0007-session-contract-prerequisite.md) makes the missing
reviewed supplier document explicit. After the focused split, wait for accepted
SESSIONCONTRACT-001 supplier/API and SESSIONCONTRACT-002 transport imports rather
than inventing cookie/CSRF/header behavior or requiring
SESSION-003 implementation just to author the document. Actual runtime acceptance
still requires SESSION-003 and every existing APITEST-010/API-010 prerequisite.

The original combined session document was split before authoring: both
[SESSIONCONTRACT-001](EPIC-007/SESSIONCONTRACT-001.md) supplier and
[SESSIONCONTRACT-002](EPIC-007/SESSIONCONTRACT-002.md) transport must be independently reviewed and integrated
before this task; existing test-first/runtime gates remain.

[ADR-0008](../decisions/0008-schema-validation-foundation.md) additionally requires
the separately reviewed/red [schema-validation foundation](SCHEMATOOL-001.md) before
claiming actual schema/reference/example acceptance. Repository link/JSON syntax
checks alone do not establish that gate. All original imports/runtime gates remain.
