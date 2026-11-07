# STORAGETEST-020 — independent executable-packet review

Reviewer: fresh `account_session_storage_test_review`, 7 November 2026,
different from `account_session_storage_test_author` and coordinator `/root`.
Worktree `/tmp/subathon-storagetest020-review`, branch
`review/STORAGETEST-020-baseline`. Reviewed packet
`777407abdf19b5e24be3facec06105437247fe81` (author `1b7e7d9`, imported as
`777407a`; coordinator CI `bc0b54c`/`f1cd77f`, reservation `2ae8d1a`), from
actual main `7897e3e`. Reviewer owns this record only; no test, API, fixture,
source, build, CI, state or generated HTML was changed.

**Verdict: request changes.** The four physical-store assertion gaps below must
be corrected and independently reviewed before any new foundation/native/helper
or feature body executes. No approval for first foundation or red is given by
this revision. No new body ran during this review. No historical red was replayed.

## Authority and scope

Read root/server/tests AGENTS, FIRST_AGENT_PROMPT, architecture/workflow/test
strategy, progress state, STORAGE-020 brief, current handoff and author record.
Reviewed specification v0.45 §§10/16, AUTH-01/05/11/12, DB-01, SERVER-01,
OPS-02 and TEST-04/05. Physical contract blob
`8dc7ba9fce0e40bcfa02897c6d88946849cbd1b1` and its different accepted document
review `393015e5` remain inputs, not executable-packet approval. Checked the
actual imported lifecycle value declarations against lifecycle blob
`48e3375bda594a39cca126ba14153d47664f0cdd` and the existing supplier/password/
writer/migration boundaries named in the author record.

Review covered every one of the 40 ordinary feature slots, the five independent
foundation slots, shared disposable fixture helpers, literal full catalog and
byte/hash oracles, headers/immediate stubs/private MAX seam, four CMake/preset
changes, closed case list and the coordinator's new CI preparation/discovery/
foundation steps. All pre-existing tests, fixtures, source, contracts, reviews
and original red receipts are unchanged by this executable packet.

## Blocking findings

1. **R1 — fresh replacement revision has no negative assertion.** Physical
   contract §5 requires a changed revision within the exact identity, including
   protection against deliberately reusing a revision for different bytes.
   `installationAndAccountInsertAndFullCas` tests wrong expected revisions and
   successful fresh replacements, but never replacement revision equal to the
   expected one. `rotationUsesFullExpectedRowAndImmutableTimes` likewise always
   supplies fresh `Q` for a successful replacement. An implementation permitting
   access/epoch/digest changes under the old revision can pass this baseline.
   Add focused preflight assertions for changed installation epoch with old `J`,
   changed account access/epoch with old `K`, and changed session digest with old
   `R`; cover the corresponding local/upgrade/revoke revision requirement where
   it is not exercised by a shared reviewed validation path. These are invalid
   replacement shapes under §§5–6: expect fixed `AccountSessionInputFailure`
   / `invalid_input` before ticket/SQL, with no mutation or caller cleanup. This
   is a physical input rule, not a demand for historical random-value uniqueness.

2. **R2 — current physical fences are not tested on rotation/reauthentication
   and password-upgrade paths.** The issuance test separately changes account
   epoch/access/revision and installation epoch, which is useful. No analogous
   negative test reaches `replace_session` with an exact expected active row
   after current access is disabled or either current epoch changes. The sole
   reauth case succeeds; the upgrade case only varies expected verifier bytes
   and then succeeds/rolls back. Thus an implementation checking current fences
   only for insertion, or ignoring upgrade's current account/install closure,
   can pass. Add deterministic cases for both `replace_session` branches after
   current access/epoch changes, expecting `stale` and unchanged session bytes;
   and capture local facts, change current account/install fences, then attempt
   upgrade with the old facts, expecting `stale` and unchanged local bytes.
   Also isolate full expected fence comparison with equal revision but different
   access/epoch bytes where applicable. Contract §§3–5 require these exact
   current comparisons. Actual host membership, accepted lineage, permission,
   clock and expired-at-execution policy remain their separate owners; no new
   port or invented accepted-login proof is requested.

3. **R3 — the absent-local-credential INSERT branch is unestablished.**
   `localReplacementUsesFullSourceAndCurrentFences` supplies `nullopt` only when
   a local row already exists, expecting collision. Every successful local
   replacement/upgrade starts with `L/H0`. A primitive that always returns
   collision for absent-expected can pass, leaving supported local credential
   creation untested. Against healthy installed account parents and no local
   row, assert `replace_local_credential(nullopt, replacement, account,
   installation) == staged`, exact subsequent raw facts, and real commit/reopen
   persistence. Also distinguish present-expected against a missing local row:
   expect `stale` without resurrecting it. Existing no-adoption collision
   assertions must remain. This is the explicit §3 method-table/§5 physical
   creation/CAS contract, independent of setup/invitation authorization.

4. **R4 — immutable rotation shape and no-reactivation are named but not
   negatively asserted.** Rotation's full-source mismatch loop changes both
   expected and replacement times together, so it tests CAS against a differing
   source; it does not test a caller proposing to extend the real source's
   expiry/start/authenticated time. The revoke case's name says no reactivation,
   but it only retries the old active revocation CAS. An implementation accepting
   changed times/bindings on token-only rotation or revoked→active replacement
   can pass all current assertions. Add bounded invalid-shape assertions using
   the exact healthy expected row with changed replacement time and immutable
   identity/captured-epoch fields, plus revoked→active replacement. Expect fixed
   input `invalid_input` before ticket/SQL, no changed rows and no cleanup.
   Contract §§5–6 explicitly preserve those fields/state for rotation and forbid
   reactivation; AUTH-05's finite lifetime must not be extended by token rotation.
   Full-U64 duration arithmetic and execution-time sampling remain lifecycle work.

These findings request stronger, specification-linked baseline assertions. They
do not authorize deleting, weakening or relabeling any of the 40 existing cases.
Batch the related corrections into this same test-review stage, reconcile the
closed compiled/CTest/CI inventories to their actual new count, and return the
exact revised packet for delta review before execution.

## Independently checked and satisfactory at this revision

- The value-only lifecycle header imports exactly the six necessary accepted
  enums/records. It adds no service/host/clock/authority protocol. Store method
  signatures/DTOs/error text match the physical contract. The additive factory
  name is a minimal reviewed interface choice; its ordinary case compares every
  compiled prefix1–8 version/id/statement/projection/hash and starting version
  against literals. Its future implementation must reuse the actual compiled
  `reference_capture_repository_manifest()` prefix with its actual target
  dependency; copied historical production SQL is not approved.
- The source has immediate explicit missing-behavior markers at construction,
  factory and every behavioral method, fixed exception plumbing and a default
  destructor. There is no SQL, codec, randomness, CAS or policy implementation.
  The private test-only friend changes only ordinal/exhaustion setup. The MAX
  assertion independently expects BE64 all-FF once, then resource refusal before
  wrap; it does not implement the counter algorithm.
- Independent Python length framing reproduced every literal prefix1–7 pin.
  The five new statements exactly equal the accepted fenced SQL; the sorted
  head preserves all 92 prior tuples byte-for-byte and adds precisely 10 objects,
  total102. Its independently computed frame is77258 bytes/SHA256
  `ec41d4b3bf7d04a3e846b401060cc3ed9430020db3dfce0e2650ed76f6a46d5b`.
  This is a literal proposal audit, not an observed selected-driver schema.
- H0 is the independent upstream96-byte record, and D0/D1 are literal32-byte
  synthetic digests. SAS1 snapshots are44 bytes. Recomputed semantic budgets:
  source success `(11+44)+58+1=114`; actor115; local
  `(10+44+96)+1+1=152`; page `51+3*63=240`; local complete mutation tuple
  `(1+99)+99+5+3=207`. The exact/one-short expectations count repeated fields,
  tags, snapshots and probe. These are semantic byte limits, not native heap
  measurements. No forged out-of-object string_view is used for overflow tests.
- Selected root/linked type corruption, orphan versus missing, malformed raw
  verifier container, hidden revisions/formats/retention, signed MIN/MIN/MAX,
  binary keys/NUL/high bytes, own-old digest and other collisions have real
  deterministic assertions. Normal setup retains real CHECK/FK enforcement;
  the separate idle-only diagnostic connection deliberately corrupts disposable
  rows and closes before product reads. NOT NULL is not bypassed. A reached
  trigger error tests sticky actual SQL failure rather than guessed collision.
- Real writer staging/late failure/whole rollback, lost successful-COMMIT ACK,
  confirmed commit plus later body exception, external pinned WAL snapshot,
  same-unit fresh read and two real queued competing CAS commands are exercised.
  Marker/assertion exceptions escape expected-writer-failure wrappers, so a
  stub cannot accidentally satisfy a rollback/unknown expectation. The existing
  queued writer executes bodies/completions on the same owner thread; these
  captured completion/marker fields are not a cross-thread fake race.
- BLOB page ordering, nonexistent cursor boundary, probe validation, complete
  probe-inclusive budget, intervening insert/new-scope behavior and MAX ordinal
  are independent oracles. The512-row tail with finite result/statement budgets
  constrains returned materialization/per-row queries for that scenario only.
  Foundation EXPLAIN checks two literal index-capable shapes; neither mechanism
  proves the later SUT avoids engine scans or uses those exact statements.
  Actual fixed positional/main-qualified SQL, full scalar metadata before any
  payload, guarded payload recheck, driver/output copies, resource charges,
  engine scratch and actual query plans remain mandatory different SOURCE review.
- Pure bounds precede shape/ticket checks in the authored assertions, and pure
  refusal preserves already-staged caller work. Foreign/expired/terminal tickets,
  facade destruction, sticky SQL/resource errors and honest source mapping are
  asserted through the actual writer. No store owns cleanup or a transaction.
- Foundation links only old migrations/writer, selected Sodium and Qt; checked
  generated linker command contains neither new storage nor supplier library.
  Five slots separately cover SDK/init/fixed32 guarded call, full literal schema/
  catalog frames, constraints/exact types/signed extremes, index/corruption and
  additive upgrade/reopen/old-executable refusal. Initialization and runtime/schema
  outcomes are still unobserved here.
- CI's SDK download/hash/size/configure/static-build/install block is byte-for-byte
  the already accepted password workflow block. Existing action/Qt/Boost pins,
  permissions, job name, old suites and old commands remain intact. New discovery
  checks closed40 against compiled functions/ordinary registration and five
  compiled foundation slots; new foundation is mandatory. No new feature body
  is scheduled and there is no skip, inversion or continue-on-error escape.
  Static Python/shell syntax checks passed. Actual remote CI has not run and
  no branch-protection/ruleset claim is made.

Minor comment correction with the test revision: `server/CMakeLists.txt` says
the new target "owns no migration factory", but now contains the deliberately
stubbed factory. Describe that marker accurately; no source behavior is requested.

## Boundaries retained

The constructor accepts only caller session, scope and limits. Copied physical
`AcceptedSessionAuthentication` fields and `t` are trusted-caller comparison data,
not accepted-attempt membership, reservation, replay prevention or login proof.
AS13–15 actual host membership/request association, at-most-once settlement,
rollback release/freshness, reset invalidation and known-commit rehash lineage
remain ACCESSCONTRACT-001/ACCESS-008/SESSION-002 obligations. The real password
source worker/owner bridge, request lifetime, no-transaction-across-crypto and
fixed public unavailable mapping also remain with that actual host. R1–R4 do
not move physical mutation checks to those owners.

Clock/freshness, full positive-U64 lifetime arithmetic/overflow, permission per
page/target, lifecycle cursor/result bounds, commit/publication/replay binding
and one secret emission stay mandatory lifecycle/transport gates. Actual fresh
scope/session/revision production is a host producer/source gate. Inspected
already admitted local libsodium1.0.22 primary source: `sodium_init()` initializes
the selected RNG; `randombytes_buf` forwards the supplied size to the selected
implementation; sysrandom fails via `sodium_misuse` on unavailable entropy rather
than returning an ordinary error. The proposed foundation tests version/init and
one fixed32 call with guards only. It proves no historical uniqueness, global
memory bound, producer nonreuse, restart/restore freshness or recoverable RNG code.
No new SDK operation ran and no earlier readiness receipt was repeated.

Setup/invitation/recovery/roles, provider facts, stopped restore, backup/update/
readiness, retention and platform/live checks remain open. All seven integrations
and all three FX providers remain intended release scope.

## Actual checks and environment

Linux Arch x86-64, kernel7.2.14-arch1-1; GCC16.2.1, CMake4.4.5,
Python3.14.9. Generated compile/link paths select `/usr/lib/libQt6*.so.6.12.0`.
Selected already admitted static SDK `/tmp/subathon-session-sdk/prefix`,
libsodium1.0.22 ABI26/4. Runtime identity/SQLite version/schema are not newly
verified by compilation or passive discovery.

Commands from this review worktree:

| Command | Actual result |
| --- | --- |
| `cmake --preset sqlite-account-sessions-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-session-sdk/prefix` | Exit0 configure/generate only. |
| `cmake --build --preset sqlite-account-sessions-debug --parallel 4` | Exit0,48 build steps, new/inherited targets compile/link; no test body. |
| `build/sqlite-account-sessions-debug/tests/server_sqlite_account_sessions_acceptance -functions` | Exit0,40 compiled feature slots, passive only. |
| `build/sqlite-account-sessions-debug/tests/server_sqlite_account_sessions_environment -functions` | Exit0,5 compiled foundation slots, passive only. |
| `ctest --preset sqlite-account-sessions-debug --show-only=json-v1` | Exit0,119 registrations; new40 enabled ordinary cases plus one foundation. |
| `ninja -C build/sqlite-account-sessions-debug -t commands server_sqlite_account_sessions_environment` | Exit0, inspected link metadata only; old writer/migrations+Qt+selected SDK, no new SUT. |
| Python3 stdlib inline audit of fixture framing and passive registration | Exit0; exact computations/inventory described above. No SQLite driver or feature helper called. |
| Python3 stdlib `ast.parse` of added CI discovery heredoc, SDK-block equality; `bash -n` of added SDK shell blocks | Exit0, parse/static comparison only; no CI script execution. |
| `python3 tools/check_project.py` | Exit0:443 task records/122 decisions, local links/presets/current HTML pass; this does not approve the baseline. |
| `python3 tools/progress.py render --check` | Exit0, snapshot current; no shared file regenerated. |
| `git diff --check` | Exit0. |

A diagnostic read of a Makefile-generator `link.txt` path returned missing-file;
this Ninja build's link command was then inspected with `ninja -t commands` as
listed above. It was not a test, foundation or environmental readiness failure.
No CTest command without `--show-only` and no selected Qt slot ran.

Reviewed blob inventory:

| Path | Git blob |
| --- | --- |
| `.github/workflows/sqlite-writer.yml` | `09afe3806e0285b479e6b68125b90508367de08a` |
| `CMakeLists.txt` | `99169d8b50358656f1f1e955d851d49b27861eed` |
| `CMakePresets.json` | `cbb92872d7642652fd971858222675730ecd9abe` |
| `server/CMakeLists.txt` | `d6429782e09d812e9ab3bbb18df1e08d10045385` |
| `tests/CMakeLists.txt` | `e94aa94b8274be279845e76c2c85a8900863ef05` |
| `server/include/subathon/server/session_lifecycle_values.hpp` | `bc7ac0034f338681bae896375e1f51e1d41191ba` |
| `server/include/subathon/server/sqlite_account_sessions.hpp` | `c9dbf6771b5697983ad87f3f015eb16e4726899c` |
| `server/src/sqlite_account_sessions.cpp` | `6d1551a0981322ae4aa59e178d5601ad2c54d3dc` |
| `tests/cmake/SqliteAccountSessionCases.cmake` | `f3e14268f84dd521f2b06fa5642b2905b3999f77` |
| `tests/fixtures/sqlite_account_sessions_vectors.json` | `86fe179c6533a0330dfca32d699114491f0ccbb7` |
| `tests/qt/sqlite_account_sessions_environment.cpp` | `bffd149b3b4b2b284fdd34d675f32addf8fe5d16` |
| `tests/qt/sqlite_account_sessions_tests.cpp` | `1fecc5eaba13f16e4e80c5f20be0b289028d6ec4` |
| `tests/qt/support/sqlite_account_sessions_fixture.hpp` | `8d4bf11bed3ce95731b2e1a4cdc73792cac11ff7` |

After the batch correction/different delta approval, coordinator first runs the
reviewed selected-driver/SDK foundation green and captures its original output,
then FIRST ordinary missing-marker red with per-case attribution and immutable
raw receipts. Missing dependencies, compilation failures, skips and old green
cannot establish that red. Production STORAGE-020 waits for the completed merged
baseline, then retains separate source/resource review, real acceptance, latest-
head selected CI and normal PR/publication gates. This review branch is not pushed.
