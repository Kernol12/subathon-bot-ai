# ADR-0009 — Native schema worker ownership (superseded)

Status: superseded for TESTCLEAN-001 after separate review of the complete boundary,
contract and oracle disposition under [ADR-0012](0012-test-suite-simplification.md).
The original accepted native decision and all of its source findings, assertions,
reviews and runs remain in the
[published archive](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/decisions/0009-schema-native-process-owner.md).

The specification requires real validation of source-controlled schemas/examples,
not a generic hostile-package validation service, kernel worker, C ABI or fixed
process resource protocol. Static consumers of the former generic API are only its
developer tests, admission checker, CI and docs; no core/server/client/provider path
uses it. The complete generic API/logical/context/install/process/native guarantee
set is retired together. Removing only the DSO while claiming its safety properties
survive would be incorrect.

Use the current [repository schema-validation contract](../contracts/schema-validation-tool-v1.md)
with the unchanged admitted standards-library pins and real artifact/example checks.
There is no replacement Popen/QProcess/native owner, callback-success seam, standalone
generic CLI or claim of the former ten-second/512 MiB/zero-survivor guarantees.
Native fault children, ownership rows, loader/assembly/allocator audits and associated
library-fact harnesses may be removed only as the precisely reviewed cleanup baseline.
Actual new test/oracle/source review and selected-host CI precede completion/merge.

The original CPython/native source findings remain valid for their old scope. This
decision does not assert they were false, replay their red evidence, certify a runtime
transport, or reduce any v0.45 product/provider/platform/security/release requirement.
