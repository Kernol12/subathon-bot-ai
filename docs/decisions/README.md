# Architecture decision records

Use the template for new engineering decisions; link existing specification decision IDs.
Do not rewrite accepted product requirements as newly settled implementation facts.
Record date, status, context, decision, alternatives, consequences, affected tasks/contracts
and verification. The repository name and final branding remain open metadata.

[ADR-0002](0002-storage-executable-slices.md) records the independently reviewed bounded storage
execution split and actual writer/repository transaction prerequisite.

[ADR-0004](0004-wire-contract-prerequisites.md) records the reviewed wire-contract
prerequisites and precise helper dependencies; actual review status is in the record.

[ADR-0005](0005-scene-contract-prerequisites.md) proposes focused scene-model,
binding/timeline, asset/package and complete-schema prerequisites; actual independent
planning review remains required.


[ADR-0006](0006-reference-path-prerequisites.md) records the §17 reference-path
proposal, exact replacement guarantees and preserved complete release scope.
Consult its task/evidence for actual independent review status.

- [ADR-0007 — Session supplier contract prerequisite](0007-session-contract-prerequisite.md):
  additive document/test-baseline sequencing repair, no authentication behavior.

- [ADR-0008 — Schema validation foundation](0008-schema-validation-foundation.md):
  developer-tool baseline/implementation prerequisite before actual package acceptance.

- [ADR-0009 — Schema native process owner](0009-schema-native-process-owner.md):
  proposed narrow first-party process boundary and explicit contract/test/source gates;
  consult its actual independent review before dependent work.
