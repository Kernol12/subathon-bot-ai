# Schema worker process and fault tests — superseded

TESTCLEAN-001 replaces this developer-tool contract under
[ADR-0012](../decisions/0012-test-suite-simplification.md) and the separately reviewed
boundary decision. The current contract is
[repository schema validation](schema-validation-tool-v1.md), revision 2.

The worker argv/channels/activation/outcome/deadline/reap protocol and its Python/native negative fault seams are retired. No subprocess, worker lifetime guarantee or replacement process owner is part of the repository checks.

The complete former contract and its original guarantees remain available in the
[published archive](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/contracts/schema-validation-process-test-v1.md).
Historical source, assertions, reviews and observed runs stay historical; neither
this notice nor the new checks claim those removed capabilities still exist.
Actual new test/oracle, source, execution and CI/merge gates remain separate.

All product runtime/authentication/accounting/security and full release requirements
remain unchanged. Schema checking supplies no provider/platform or authority proof.
