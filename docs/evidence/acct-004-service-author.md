# ACCT-004 service implementation candidate

Author service_author, 2026-11-04, isolated task/ACCT-004-service from actual
main1124532+claim603a62a. Source server/src/application_service.cpp only; codec
source imported from coordinator commit9eeb140 as prerequisite. Coordinator-owned
server/CMakeLists.txt verification copy b3e4a90 remains uncommitted and is excluded
from this author's commit.

Authority: [logical service](../contracts/application-service-v1.md),
[concrete API](../contracts/application-service-api-v1.md), specification v0.45
§§3–5/NUM-01–04/B08 and [reviewed baseline](accttest-004-handoff.md). The original
35-slot/224-scenario stub-red and all reviewed assertions/fixtures are preserved.

Candidate implements owned queued command snapshots, callback-scoped finite retained
read views, current access/disclosure, complete retained semantic checks, early claim/
active hold ordering, current full contribution/plan binding, segmented clock and
exact ordered debt/cap/counter/correction proposal composition, all-or-none durable
numeric/policy holds, action/job binding, one-time fresh references, staged atomic
bundle, actual caller commit and exact QVariant normal-completion publication.
Existing private bundle/hold pure validation and logical accounting helpers are
reused without changing repository schemas, contracts or public interfaces.

## Actual checks to this initial candidate

GNU16.2.1, Qt6.12.0, Boost1.92, host SQLite (component synthetic fixture databases).

```sh
cmake --preset application-service-debug
cmake --build --preset application-service-debug --target subathon_original_application_service --parallel 2
cmake --build --preset application-service-debug --target server_application_service_acceptance --parallel 2
ctest --preset application-service-debug -L '^application-service$' --output-on-failure
```

Configure passed. The first library compile found a C++ vexing declaration in
read_exact; corrected to brace construction. Subsequent target build passed8steps
without warnings. First actual implementation acceptance execution passed ALL35
unchanged slots/224scenarios (3.41s), zero Qt skips/unrelated failures. This is a
passing implementation run, not a replay of the historical stub-red. Original
labels in this isolated verification copy remain draft-red; coordinator promotes
stable CI at integration after all gates.

Coordinator preliminary inspection then corrected original result-byte option
validation before admission-only codec outputcap expansion, made callback revocation
atomic for concurrent foreign read rejection, and moved unsupported-format mapping
inside the guarded body so low final response bounds retain actual cleanup/failure
metadata. Final checks/review are pending on the corrected candidate.

This author does not self-certify separate code review. Full integration/regression,
actual selected officialQt CI, independent allocation/codec/source review and merge
are separate gates. Providers, authority/policy/clock/action producers, financial/
refund/review continuation, recovery/UI/platform/packaging/live release remain open.
