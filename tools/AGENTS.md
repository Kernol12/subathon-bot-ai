# Module agent instructions

Read the root AGENTS.md first.

Python >=3.10 standard-library developer tools. Keep HTML offline/self-contained,
escape editable text, validate local relative links and task dependency DAGs.
Progress data must remain truthful. Tooling checks are not product acceptance or
live-provider verification.

The repository-schema checks have the isolated developer-tool exception in
[ADR-0008](../docs/decisions/0008-schema-validation-foundation.md): use the existing
fully pinned real standards libraries and admitted artifact/license metadata.
Keep ordinary progress/repository tools standard-library-only. Any changed dependency
or version still requires direct/transitive pins, hashes, license and offline foundation
review before execution. Do not install globally or add server/client dependencies.

[ADR-0012](../docs/decisions/0012-test-suite-simplification.md) and the separately
reviewed TESTCLEAN-001 boundary supersede the former
[ADR-0009](../docs/decisions/0009-schema-native-process-owner.md) C++/ctypes worker
exception. It supplies no current generic package API, native process guarantee or
permission to relabel old harness checks as schema evidence. Follow the current
[repository-schema contract](../docs/contracts/schema-validation-tool-v1.md): actual
shipped schemas, every declared example and independent valid/invalid cases with
real selected validators and offline references. New test/oracle source requires
independent review before execution; new first-party behavior retains the root
contract → tests → review → intended red → implementation → code review sequence.
