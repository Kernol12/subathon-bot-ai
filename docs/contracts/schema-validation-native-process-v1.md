# Schema native process owner — superseded

TESTCLEAN-001 replaces this developer-tool contract under
[ADR-0012](../decisions/0012-test-suite-simplification.md) and the separately reviewed
boundary decision. The current contract is
[repository schema validation](schema-validation-tool-v1.md), revision 2.

The native ABI/artifact/raw-syscall ownership, clone3/pidfd/signal/descriptor/scratch/allocation limits, protocol, journal/fault phases and generated-code/loader/allocator audits are retired with the generic tool. No native library or kernel feature is required by the replacement checks.

The complete former contract and its original guarantees remain available in the
[published archive](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/contracts/schema-validation-native-process-v1.md).
Historical source, assertions, reviews and observed runs stay historical; neither
this notice nor the new checks claim those removed capabilities still exist.
Actual new test/oracle, source, execution and CI/merge gates remain separate.

All product runtime/authentication/accounting/security and full release requirements
remain unchanged. Schema checking supplies no provider/platform or authority proof.
