# STORAGETEST-020 — interface, literal fixture and test author packet

Author: fresh `account_session_storage_test_author`, 7 November 2026. Worktree
`/tmp/subathon-storagetest020-author`, branch `task/STORAGETEST-020-author`,
base `91567a9e20a7ac318f4514ef7488cf56810ed2ef` (root's claim after actual
main `7897e3e`/normal PR86). Coordinator owns the task/state/HTML/checkpoint,
actual catalog allocation, workflow/pins/CI, integration and publication. Root
explicitly delegated the four changed CMake/preset files and case-list file, plus
the value-only lifecycle header; no concurrent shared-file writer was used.

This packet is **authored, compiled and passively discovered only**. It has no
independent packet approval, foundation pass or observed missing-behavior red.
No new feature, foundation, native-helper or test body has executed. No old
acceptance/red receipts have been replayed. Production STORAGE-020 remains absent.

## Authority and frozen inputs

Read root/server/tests instructions, FIRST_AGENT_PROMPT, architecture/workflow/test
strategy, CLion setup, state, STORAGE-020 brief/current handoff, and the accepted
physical contract with its supplier/lifecycle/password/writer/migration imports.
Specification v0.45 §§10/16, AUTH-01/05/11/12, DB-01, SERVER-01, OPS-02 and
TEST-04/05 supply the task's requirement map. Accepted input blobs:

| Contract | Git blob |
| --- | --- |
| sqlite-account-sessions-v1 | `8dc7ba9fce0e40bcfa02897c6d88946849cbd1b1` |
| session-lifecycle-v1 | `48e3375bda594a39cca126ba14153d47664f0cdd` |
| session-resolution-v1 | `c5f5a6e71664042e18475068d3caf577b0c8d009` |
| password-hashing-v1 | `bc9ccbf66a41c9ae69abde4bd905401900e17461` |
| password-hashing-api-v1 | `7ad1e8d3f3311673dd368f2ef4d297de31dbafcd` |
| sqlite-writer-v1 | `4d280461966f3fd9fcb13ab6d054c88228ca05b8` |
| writer-transactions-v1 | `4ba20506db9a858450099b82888a22c8f6dbd761` |
| migration-catalog-v1 | `cbdea1a48d2989a9c8bd17edf0539bdf7c1bce61` |

Physical contract's different accepted document review is `393015e5`; it does
not approve this new executable packet. No existing assertion/fixture/review/red
artifact is changed.

## Owned artifacts and deliberately absent behavior

- `server/include/subathon/server/session_lifecycle_values.hpp` imports ONLY six
  accepted lifecycle value types/enums needed by storage. The later lifecycle
  header should include these declarations rather than redefine them. No trusted
  host port, service, authentication proof or lifecycle policy is added.
- `server/include/subathon/server/sqlite_account_sessions.hpp` declares the exact
  accepted store methods/DTOs. Fixed input/storage exceptions carry only their
  enum and contract text. `AccountSessionStorageFailure` is required by physical
  contract §5, and corruption mutation assertions exercise its specified category.
- `server/src/sqlite_account_sessions.cpp` contains immediate
  `AccountSessionStorageNotImplemented` markers at construction and every behavior
  method, plus fixed exception plumbing and a default destructor. Marker text is
  `STORAGE-020 account-session storage not implemented`. There is no production
  SQL, migration behavior, read/write/CAS, parsing, hashing, randomness or cleanup.
  The newly delegated `account_session_repository_manifest()` is also an immediate
  marker only; its focused ordinary case compares every compiled1–8 statement/
  projection/version/id/hash/starting version to the independent literals.
  Constructor/factory markers make the ordinary cases fail on explicit missing
  behavior before it can be mistaken for implemented storage.
- One private friend, `testing::SqliteAccountSessionTestAccess`, is defined ONLY
  in the test translation unit. Its inert setup writes private ordinal/exhaustion
  fields so ordinal MAX is testable without 2^64 calls. It exposes no SQL/session
  or public mutator and implements no counter algorithm.
- `tests/fixtures/sqlite_account_sessions_vectors.json` contains literal contract
  DDL/base rows/constraint refusals, independently fixed D0/D1/H0/snapshots/byte
  counts, immutable prefix identities, and full 102-object step8 projection.
- `tests/qt/support/sqlite_account_sessions_fixture.hpp` uses the existing actual
  writer and migration runner with independently literal manifests. It owns no
  product behavior. Its diagnostic connection is confined to disposable tests.
- `tests/qt/sqlite_account_sessions_tests.cpp` has 40 ordinary Qt slots, registered
  explicitly by `tests/cmake/SqliteAccountSessionCases.cmake`. Marker/assertion
  exceptions are preserved as ordinary test failures, including in scenarios
  inspecting an intentionally failing writer outcome.
- `tests/qt/sqlite_account_sessions_environment.cpp` has five foundation slots.
  Its target links ONLY old writer/migrations, Qt and selected SDK, never the new
  account-session or supplier implementation. No build command runs it.
- Root/server/tests CMake and `CMakePresets.json` add
  `sqlite-account-sessions-debug`, inheriting the old migration/writer/work lane.
  Feature labels are ordinary `sqlite-account-sessions;acceptance;qt;server`.
  There is no disable, skip, WILL_FAIL, inversion or configure-time native run.
  CI integration remains explicitly with root before baseline publication.

## Independent literal fixture derivation

An author-only Python script read accepted §2's FIVE exact fenced SQL statements
and copied prior **fixture** manifests, never a new production factory, SQL schema
query or codec. Existing prefix 1–7 expected projections and independent canonical
length framing were used to assemble the sorted 92+10 object proposal. Canonical
framing of each old literal manifest reproduces its existing pinned hash. The new
literal frame is 77258 bytes, SHA256
`ec41d4b3bf7d04a3e846b401060cc3ed9430020db3dfce0e2650ed76f6a46d5b`.
This is a proposed literal oracle, **not an observed selected-driver schema**.
Root separately reserved actual next version8/id `account-sessions-v1`; this author
adds no production migration implementation or automatic migration behavior.

H0 is the exact public upstream 96-byte encoded vector already accepted in the
password contract/fixture. D0/D1 are synthetic literal 32-byte values; SAS1 scope0
snapshots are literal 44-byte binary records. These prove no password verification,
real acceptance, production randomness or historical uniqueness. No bearer,
password, supporter data or live credential is introduced.

The isolated corruption mechanism opens a separate fixture connection only while
production writer work is idle, disables CHECK/FK enforcement on that diagnostic
connection, commits deliberate malformed rows, and closes before product calls.
Normal setup keeps real constraints ON. NOT NULL is never bypassed: null constraint
refusals remain foundation checks. An isolated trigger additionally forces a
reached unexpected UPDATE constraint failure through the real writer. No diagnostic
connection/native handle is exposed to product code.

## Acceptance map

Each row refers to accepted physical contract §7's literal examples, with their
§§2–6 invariants. Several rows share focused cases rather than multiplying cases
for each field.

| Examples | Authored executable assertions |
| --- | --- |
| AS01 | Exact installation+digest complete/missing, full linked raw facts, current snapshot; binary composite keys/NUL/high-byte/prefix distinction. |
| AS02–03 | Exact account/access/epoch and installation epoch CAS; source retains captured epochs/digest/times; actual supplier revokes mismatch before clock. Foundation covers unchanged epoch after actual normal reopen. |
| AS04 | Valid revoked and expired rows remain complete; actual existing supplier owns effective revocation/expiration and clock ordering. |
| AS05–06 | Missing account/install; invalid hidden revisions, singleton, access, state, retention, format, times, reference class and digest class/length. Mutation validates corruption before stale; exact wrong actor digest is honestly missing. Foundation independently checks real normal DDL refusal first. The ordinary factory case compares full compiled1–8 catalog output to independent literals. |
| AS07 | Global identity, installation+digest and own-old-digest collisions; no adoption; unexpected trigger constraint remains actual sticky sql_failure. |
| AS08–09 | Full expected digest/epoch/times despite same revision; one rotation winner, old digest absent, stale second CAS; revoke preserves captured digest/epochs/times, including disabled/epoch-mismatched selected target. |
| AS10–12 | Real late SQL rollback; same-unit fresh queries; external WAL snapshot before/after commit; lost successful real-COMMIT ACK; confirmed commit plus later body exception; two real queued competing CAS commands. |
| AS13–14 | Full expected local verifier/revision and account/install fences; local CAS loses after reset. Physical upgrade known rollback preserves L/H0; lost commit ACK has unknown/no normal value and may leave physical Z. Host accepted lineage is explicitly outside this unit. |
| AS15 | No standalone owner/replay claim: real membership/reservation/consumption is a separate host gate, explained below. Physical writes stay provisional and real outcomes are preserved for that owner. |
| AS16–18 | Complete metadata overage before malformed/revoked semantics; verifier128 before ownership; raw empty/malformed/unsupported fitting PHC unchanged; local missing distinct from orphan/format failure; local success B152/151. Actual PasswordLoginSource bridge conversion remains its owner. |
| AS19–20 | Raw BLOB NUL/prefix/high-byte order, exact absent versus cursor boundary, bounded source includes one probe, bad probe fails whole page, later page observes intervening inserts/new scope. Cursor emission/permission freshness belongs to lifecycle. |
| AS21 | Complete source B240 versus239 for three one-byte probe-inclusive rows. Large 512-row unrelated tail and finite actual writer result/statement budgets prevent load-all/per-row SQL materialization for this scenario; literal EXPLAIN verifies index capability only. |
| AS22 | Real Qt INTEGER classes and full MIN/MIN/MAX tuple, physical read/rotation and fresh read-only reopen. Full positive U64 duration arithmetic/overflow is lifecycle's policy gate; duration is not narrowed or persisted here. |
| AS23 | Missing physical table and prior sticky SQL failure map source unavailable while actual sql_failure/rollback survive; actual result budget maps resource while sticky result_limit survives; unknown remains unknown. |
| AS24 | Constructor zero/U64 aggregate/row-limit overflow; complete borrowed bounds before malformed shape/stale capability; mutation B3/2 and full local tuple B207/206; pure refusal preserves earlier caller staging; foreign/expired/terminal capability errors and destruction without caller cleanup. |
| Snapshot contract §4 | Literal scope/BE ordinal0/1, unsuccessful read does not advance, new scope on later work, controlled MAX succeeds once then resource refusal before wrap. |

All complete page rows assert every SessionRecordFacts field, not only refs. DTO
budgets use independently fixed sums (source success114, actor115, local152,
page240, supplier strings55) and include repeated references/tags; limits remain
independent from actual SQL budgets. Input overflow cases use real U64 option
values, never a forged string_view beyond an allocated object's valid range.

Outcomes cannot prove preownership allocation or an index plan. Later different
source/resource review must inspect **actual SUT** fixed positional/main-qualified
SQL, complete scalar metadata-before-payload, guarded payload recheck, finite
source/driver/output copies, all writer charges and scratch. Foundation EXPLAIN
shows that the literal required two query shapes can use the index; it does not
observe the stub's SQL. Large-data tests do not prove absence of engine scans.
No generic native-SQL trace/handle subsystem was added for these tests.

## Boundary findings requiring explicit review and later owners

The accepted constructor takes only ticket/scope/limits. It has no host acceptance,
request membership, permission, clock, lifetime-policy or PasswordLoginSource
bridge port. Adding such ports here would invent architecture beyond this task.
Root explicitly agreed to the following boundary classification:

1. Physical insert/reauth/upgrade test calls assume the trusted caller already
   performed its real acceptance/authority preconditions in the same scope. The
   literal `AcceptedSessionAuthentication` is ONLY borrowed physical comparison
   data; a copied DTO or `t` does not authenticate or reserve anything. Store tests
   check deterministic current fences and exact SQL transitions, not a fake login.
2. Actual accepted-attempt membership, at-most-once reservation, commit/unknown
   consumption, rollback release/freshness, reset invalidation and committed rehash
   lineage belong to ACCESSCONTRACT-001/ACCESS-008 and SESSION-002's real host.
   AS15 and these portions of AS13–14 remain mandatory downstream tests.
3. The password source's actual worker/owner bridge, source address/request lifetime,
   no-transaction-across-crypto and unavailable/no-attempt conversion cannot be
   tested through this store-only API. Internal raw facts/failures are tested here;
   no new PasswordLoginSource implementation/proof protocol is supplied.
4. Actual clock/freshness, full-U64 start+duration expiry/overflow, permission per
   page/target, lifecycle cursor/result bounds, actual commit/publication/replay
   binding and one secret emission remain owning lifecycle/transport tests.
5. Production fresh scope/session/revision entropy uses the already selected
   initialized SDK under the accepted host contract. The foundation only checks
   SDK identity/init and one actual fixed32 random call with guards. Actual producer
   lifetime/reuse/restart/restore/entropy failure/source audit is not fabricated.
6. Root explicitly delegated the minimal factory name/declaration and marker after
   identifying that the accepted document did not fix its exact C++ name. That
   interface, literal comparison and starting-version behavior require the same
   different packet review. Future implementation must reuse the actual compiled
   `reference_capture_repository_manifest()` prefix1–7, adding the appropriate old
   target dependency when needed; copied historical production SQL is forbidden.
   No additional reference/app-service/Boost source chain is required merely to
   link the marker and compare it against independent literal fixture values.

These are explicit contract-boundary findings for a DIFFERENT reviewer, not silent
release-scope deletions or claims of authentication capability. All seven
integrations, all three FX providers, setup/invitation/recovery/roles, stopped
restore/backup/update/readiness, retention and platform/live gates remain required.

## Commands and observed author results

Run from `/tmp/subathon-storagetest020-author`; no machine paths were committed to
presets. Configure identified GNU16.2.1, Python3.14.9; CMake4.4.5 and
clang-format23.1.4 were observed. Qt6.12 config/library/plugin availability was
inspection/compile-link only, **not** a fresh runtime/foundation readiness result.
The explicitly admitted existing SDK prefix is
`/tmp/subathon-session-sdk/prefix` (libsodium1.0.22 ABI26/4). No download/system
package change/new dependency or live/network operation occurred.

| Exact command | Observed author result |
| --- | --- |
| `cmake --preset sqlite-account-sessions-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-session-sdk/prefix` | Exit0, configure/generate only; no new runtime/test body. |
| `cmake --build --preset sqlite-account-sessions-debug --parallel 4` | Exit0, compiled/linked new feature/foundation and inherited old targets, no compiler warnings; later authored revisions rebuilt. |
| `build/sqlite-account-sessions-debug/tests/server_sqlite_account_sessions_acceptance -functions` | Exit0, passive compiled feature slot discovery only. |
| `build/sqlite-account-sessions-debug/tests/server_sqlite_account_sessions_environment -functions` | Exit0, passive compiled foundation slot discovery only. |
| `ctest --preset sqlite-account-sessions-debug --show-only=json-v1` | Exit0, passive registration/label discovery only. |
| `git diff --check` | Exit0, whitespace check. |

Final stdlib-only comparison of the passive outputs passed: all40 compiled feature
slots equal the explicit case list and CTest registrations; every feature has
exact ordinary acceptance labels, is enabled/non-inverted and has no skip/pass
regular-expression escape. Five foundation slots and one foundation registration
are present; the inherited complete preset lists119 CTest registrations. This
comparison executes no selected test body. Commit is reported to root.
Neither `ctest` without `--show-only` nor a selected feature/foundation method has
run. No feature/foundation pass/red is claimed and no implementation task is done.

## Next gate

Root should integrate/reconcile this coherent packet and obtain actual different
interface/fixture/test/foundation/configuration review at its exact revision. After
approval, root runs and preserves FIRST foundation green with the selected actual
QtSQLite/SDK, then FIRST ordinary feature marker red. Missing SDK/compiler/setup,
skips, or inherited old green are not red. Only then may a fresh STORAGE-020
implementation session supply production algorithms, followed by separate source/
resource review and ordinary selected acceptance/regression/CI/publication gates.

## R1–R4 correction packet after the actual independent review

Same test author, same task/branch/worktree, resumed at
`05e75ab584efc9d1748a6ef49bba4b2fc02d9605`, including actual review commit
`39c95d5858ec0bc193752c8724047355f3f1c4a0` (root import `4689c78`). Read the
complete [independent review](storagetest-020-review.md), blob
`8bc558b87cb56ada2cc95e065879dbf1c16b328b`. Its R1–R4 verdict requests stronger
physical-store assertions; its other actual interface/fixture/native/harness/CI
checks are satisfactory. It still grants **no foundation or red execution approval**.
This correction makes no new approval claim and does not edit the reviewer record.

The original packet/evidence above remains historical: its exact40/119 passive
counts and original receipt attribution are preserved. The corrected closed
inventory is **44 ordinary feature slots, five foundation slots, 123 total
registrations in the minimal account-session preset**. Root owns the corresponding
CI closed-inventory40→44 update and the same reviewer's actual delta approval.
The author does not edit workflow, progress/state, HTML, checkpoints or other
shared integration records, pull another branch, or update another worktree.

Four focused slots are appended; all40 original slots, their assertions and the
original surrounding test source remain byte-for-byte unchanged. The only other
code change corrects `server/CMakeLists.txt`'s stale comment: the target contains
an additive manifest **marker**, rather than owning no factory. The case-list
file appends exactly the four new names. All production source/API, old/new JSON
fixtures, foundation and shared fixture helpers are unchanged.

| Review finding | Added independent assertions |
| --- | --- |
| R1 — replacement revision freshness | `reusedRevisionsAndImmutableRotationShapesRefuseBeforeSql` rejects epoch H under installation J; account access and account epoch changes separately under K; new verifier bytes under local L; upgrade under L; changed digest under session R in both replacement branches; and revoke under R. Every refusal requires the fixed input exception/text and `invalid_input`. The same matrix runs with an expired ticket and an actual active caller unit. Literal raw rows and prior caller-staged account B must remain intact, followed by real normal caller commit. |
| R2 — current session fences | `rotationAndReauthenticationRecheckCurrentPhysicalFences` first reads the actual complete row, then independently changes access, account epoch or installation epoch inside that same real unit. Both replacement branches must return stale without changing any of the thirteen physical session fields. Fresh-revision and deliberately same-revision diagnostic changes are both covered. Additional expected captured-account/install epoch mismatches keep revision R, and reauthentication explicitly compares accepted account revision K with current M. No host acceptance, clock or permission proof is invented. |
| R2 — complete upgrade fences | `passwordUpgradeRechecksCompleteCurrentAccountAndInstallationFences` captures actual local/current account/install facts before changing current access, account epoch/revision or installation epoch/revision. Old facts must produce stale, leaving all five local fields and the original session unchanged. Separate equal-K/J expected-fact cases vary access, account epoch and installation epoch bytes, so a revision-only comparison cannot satisfy the assertions. |
| R3 — absent credential creation | `absentLocalCredentialCreatesButPresentExpectedMissingNeverResurrects` uses healthy real parents and no current local row. Present expected L/H0 must return stale and leave the table empty; absent expected must stage creation of exact Z/H1. All credential/current parent fields are checked, then actual caller commit, writer destruction and a NEW writer reopening the same file must preserve the complete raw facts. The prior no-adoption collision assertions remain unchanged. Creation supplies no setup/invitation or login authority. |
| R4 — immutable rotation/no reactivation | The combined pure-input slot separately changes started/authenticated/expiry times while keeping valid time ordering, and changes installation/account/session identities, either captured epoch, or active replacement state. It also attempts revoked→active replacement. Each illegal shape must return the same fixed `invalid_input` before an expired capability can be used. An active unit checks unchanged rows/no caller cleanup, and a separately staged actually revoked V row remains revoked V after another reactivation attempt. |

The new H1 verifier is a literal public hashcat record already frozen in the
unchanged `tests/fixtures/password_hashing_vectors.hpp`, blob
`025d4de904c4c07a08cc192fc6cf94318bd5c0c8`. A stdlib-only text audit confirms
exact equality and97 bytes. H1 expectations are not generated by a hash call,
SUT parser, store read or implementation output. The added physical helper
assertions compare directly with literal BLOB/INTEGER rows; they derive no
expected value from the implementation being tested. Deliberate same-revision
fixture changes are diagnostics against revision-only comparisons, not permitted
production revision reuse.

Exact code/fixture revisions for the delta reviewer:

| Path/input | Git blob |
| --- | --- |
| `tests/qt/sqlite_account_sessions_tests.cpp` corrected | `33d8a57f55a19f39846c8234b6bd9fc4ae080930` |
| `tests/cmake/SqliteAccountSessionCases.cmake` corrected | `5403ebbffbc77ac6a8cb9e33cc0ac6a6ff875c5c` |
| `server/CMakeLists.txt` comment corrected | `b3fe96b511c402e36de33309931de3c1b4bcfa46` |
| `server/src/sqlite_account_sessions.cpp` unchanged immediate markers | `6d1551a0981322ae4aa59e178d5601ad2c54d3dc` |
| `tests/fixtures/sqlite_account_sessions_vectors.json` unchanged | `86fe179c6533a0330dfca32d699114491f0ccbb7` |
| physical contract unchanged | `8dc7ba9fce0e40bcfa02897c6d88946849cbd1b1` |

The source still throws the explicit marker at constructor, factory and every
behavior method. No production SQL, parser, cryptography, mutation, revision,
policy or fresh-identity algorithm was added. Other frozen supplier/lifecycle/
password/writer contracts and the mandatory downstream/source gates above remain
unchanged; these four findings cannot be deferred to the host.

Actual correction checks, from the same worktree and admitted SDK:

| Command/check | Observed result |
| --- | --- |
| `cmake --preset sqlite-account-sessions-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-session-sdk/prefix` | Exit0, configure/generate only. |
| `cmake --build --preset sqlite-account-sessions-debug --parallel 4` | Exit0, changed feature source compiled and linked without warnings. No new body. |
| `build/sqlite-account-sessions-debug/tests/server_sqlite_account_sessions_acceptance -functions` | Exit0,44 compiled slots, passive only. |
| `build/sqlite-account-sessions-debug/tests/server_sqlite_account_sessions_environment -functions` | Exit0, unchanged five foundation slots, passive only. |
| `ctest --preset sqlite-account-sessions-debug --show-only=json-v1` | Exit0,123 registrations. |
| Python3 stdlib inline audit of these passive outputs | Exit0; exact44 compiled/listed/registered equality, ordinary acceptance labels, enabled/non-inverted/no-skip guards; five foundation slots. |
| Python3 stdlib text-preservation/literal audit | Exit0; removing only new helpers/slots reproduces the reviewed test source byte-for-byte; H1 equals the original independent public97-byte literal. No driver/feature/crypto helper executed. |
| `git diff --check` | Exit0. |

The original author `git add` failure remains a metadata receipt: default sandbox
could not create the shared worktree `index.lock` (read-only filesystem, exit128).
It happened after successful passive discovery, did not execute a test and is not
an environment/red receipt. Narrow Git metadata escalation was then approved and
created original author commit `1b7e7d9`; this correction does not rewrite or
reclassify that history. No historical test/readiness/red receipt was replayed.

No feature, foundation, native-helper or new source-function body has executed
in this correction either. The same independent reviewer must assess this exact
R1–R4 delta plus root's reconciled CI inventory before root's FIRST selected
foundation green and FIRST ordinary marker-red runs. Production implementation
still waits for the completed reviewed/red baseline and keeps the later actual
compiled-prefix dependency, source/resource, real acceptance and normal CI gates.
