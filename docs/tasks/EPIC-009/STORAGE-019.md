# STORAGE-019 — Bounded reference capture structural codecs

Split from STORAGE-018 as permitted by sqlite-initial-captures-v1 §1. This is the
codec implementation only; the SQL producer remains STORAGE-018 and must depend
on this integrated prerequisite. All original release scope remains required.

Dependencies: completed/integrated STORAGETEST-018. Acceptance inputs: actual
independent codec review, original valid27 marker-red, unchanged CPPc207e423 and
literal header53da620d. Do not repeat the historical red or change expected bytes.
Read the full concrete/consumer contracts and final baseline handoff.

Own eight SRC1/SRA1/SRX1/SRI1 structural encode/decode bodies. Keep public DTOs,
limits/errors/signatures unchanged. No admission/evaluation/authority/clock/SQL
or new factory behavior. Narrow optional private borrowed parser header may expose
only source-private shared views/sizing/validation to the later SQL adapter;
review that interface and source before dependent implementation.

Validate all positive representable limits and reserved n+1 capacity; borrowed
complete DTO/per-field B/I; exact tag/reference/shape/value/binding priority;
canonical framed lengths and whole envelope/output projection BEFORE ownership.
Fixed borrowed decoder records<=64/scratch<=512/depth<=3; no dynamic tree/cache.
Preserve signed full width, uint64 CID0/MAX, SRX positive revision versus SRI1
revision1, null-vs-empty buffers, exact nested bytes and owned result lifetime.
Catch own allocation/length failures into fixed input errors; source review must
cover preallocation, wide underlying enums and unreachable malformed fields beyond
black-box outcomes. No whole-document U32 cap when only individual B fields need it.

Actual SQL document references remain under the SQL store profile when reused
later. Fixed adapter role metadata is not a stored reference, but public codec
supplied format refs remain bounded. A shared borrowed body path must preflight
all real nested fields before ownership; any materialized envelope counts its
format bytes in the complete owned budget. No widened-profile-then-postcheck path.

Preserve all original assertions. Add meaningful regressions only for actual gaps
with different test review and behavioral red before a correction. Run27 codec
acceptance plus stable309; retain31 SQL intentional-red registrations separately.
Obtain different code/config review, promote ONLY passing27 into ordinary selected
Qt CI with exact discovery, observe changed workflow and latest-head checks.
Task completion is codec component acceptance, not SQL capture or API011 support.
Root owns state, generated HTML, CMake/CI/pins/schema allocation and integration.
