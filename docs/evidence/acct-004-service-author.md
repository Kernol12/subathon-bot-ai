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

## Corrected candidate and additional reached regressions

The first complete candidate722e11b passed the full300 native profile24.66s after
an88-step full build, zero warnings. Independent review then found real gaps beyond
the original baseline. Coordinator separately authored/reviewed eight additive
slots/nine scenarios, and preserved their actual reached behavior failures before
releasing these corrections; [regression evidence](acct-004-regressions.md).
No original assertion, fixture or historical first-red was modified.

The corrected source now caches only private selected read-limit DATA in the view
(the session authority still expires), scans complete SAS1/SAR1/SAA1/SAJ1 grammar
using byte views before semantic allocations, measures aggregate decoded SAS1
ownership including actual clock segments and balances, checks retained clock sum/
finalUTC, verifies advanced-before policy/config consistency, validates historical
synthetic jobs against immutable selected plan requests and actual components, and
maps supported unresolved policy to an honest durable hold with its exact reason.
Every negative unit requires explicit correction purpose; timer deficit policy
remains milliseconds-only. The separately authored private decoder3ef4755 supplies
internal retained-bound versus malformed classification without changing the public
codec. Final response source payloads are measured before vector/variant copies;
encoded envelopes include their format bytes and not-ready reason strings are
projected before allocation.

Historical job validation uses the existing private same-session Access::plan
selected-revision reader after disclosure, with its unchanged bundle typed failure
mapping and result bounds. It reads only the immutable recorded revision, never
selects/revalues a new current plan or invokes policy, clock, arithmetic, actions or
issuer for historical work. This seam was explicitly released by the coordinator
and remains subject to actual different source review.

Actual commands on the corrected source/header plus privatecodec3ef4755 and the
root additive regression source fromdb621cf (importedfb9db4f):

```sh
cmake --preset application-service-debug
cmake --build --preset application-service-debug --target server_application_service_acceptance --parallel 2
ctest --preset application-service-debug -L '^application-service$' --output-on-failure
```

Configure/5 compile-link steps PASS, zero warnings. ALL43 slots/233 scenarios PASS
3.72s (original35/224 plus additive8/9), zero Qt skips/failures. Original author-tree
labels remain draft-red from its copied harness; coordinator owns final promoted
acceptance properties/ordinary CI. Full308 and actual independent review/current
CI are subsequent gates, not implied by this targeted result.
