# RELEASE-003 — Write operator setup recovery and client workflow guides

Parent: [EPIC-027](../EPIC-027.md). Kind: scaffold.
Specification: v0.45; §16, DEV-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

User/operator documentation for actual supported installation/setup/operations, not developer internals in product UI.

Primary module paths: release manifests, server/ diagnostics and docs/
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Document installation-owned authorization, all service/rate limitations, backup/private-key custody, restore/recovery and explicit updates.
- Provide native/browser/viewer/editor workflow and troubleshooting guides consistent with permissions and release matrix.

## Boundary and failure cases

Do not promise unverified registration-free auth, complete history/refunds or unsupported platform/media behavior.

## Documentation gate

Check this documentation against implemented contracts and recorded evidence, using
reviewed examples and reproducible operator steps. Do not assert unsupported product
behavior or execute public release/provider operations as a documentation side effect.

## Checks and handoff

Primary home: Manifest/redaction checks with synthetic data and final artifact/document audit.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
