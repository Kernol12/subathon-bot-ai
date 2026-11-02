# STORAGE-015 implementation evidence

Implementation author `/root/contribution_implementer`,2026-11-02. Dedicated
`task/STORAGE-015-implementation` worktree starts at coordinator claim
`ef8fb219279186c42a14fae60fe067639a0768ef`, after actual PR39/maina7a1844.
Scope is ONLY `server/src/sqlite_application_plans.cpp` and this new evidence.
Root owns registration, CI promotion, state/HTML/checkpoint and integration.

## Authority and preserved baseline

Read root/server AGENTS, STORAGE-015 brief, full approved logical/API3af256cd,
physical99bf45b4, header4634d7b5, frozen feature0f8e625f/JSON64eb8b4e/helpera8d63d6a,
full8b/final3dec actual reviews, original red and CI/handoff. Implementation starts
AFTER actual independently reviewed original29 named-stub red at clean aa6907b,
after selected official41494786280. That completed red is never repeated.

| Frozen artifact | Git blob |
| --- | --- |
| Logical/API | `3af256cdf82a4737c2fc71a67c88b8b332d6ba71` |
| Physical proposal | `99bf45b46f6c9d361b899fdb975dc2db51da4554` |
| Public header | `4634d7b57dccc5c559f2ab05a968833b4b925feb` |
| Original feature baseline | `0f8e625ff7c238776602fd1be102959f8c43b073` |
| Independent JSON | `64eb8b4e0d709bc34ab6d33a4860a62883f06c6b` |
| Writer-only diagnostics | `a8d63d6a220992fcb71273d89a3aa32609870051` |
| Original stub-red output | `ae3e0e25956b16b0d3bc26065c71247263bca421` |

No API, contract, fixture, baseline assertion, earlier producer or build/CI file
was edited by this author. A separately reviewed baseline adapter correction is
pending below; it cannot be inferred or silently incorporated here.

## Implemented candidate behavior

- Full0..MAX uint64 codec uses exact big-endian BLOB8, with separate persisted
  positive revision/head/parent and positive ReceiptId roles; CID0 remains present.
  No unsigned identifier is routed through signed64, REAL or QJson double.
- Exact VALUE serialization uses existing Boost cpp_rational's canonical signed
  numerator/positive denominator decimal ASCII. Decoder checks component presence,
  bounds, strict grammar, positive denominator, gcd and zero0/1 before rational
  construction. Limits are independent of parser/finalizer512/256/4096 policies;
  no rounding, numeric recomputation, source spelling rewrite or new dependency.
- Pure manifest copies actual earlier pure contribution manifest and appends the
  exact five SQL statements from the approved physical document, plus five table/
  five autoindex projection entries. Prefixes1/2/3 and migration4 payload29015/
  SHA256bbb8140119b29f54f419479851d4fdf42b6a928f5451e4de756389d2bc359c50 stay exact.
- Constructor/destructor remain inert; every method uses the caller's actual
  active revocable session, no BEGIN/commit/authority adoption/destructor cleanup.
  Input priorities/options/overflow-safe lookahead/vector maxima precede SQL.
  Results are staged until real caller COMMIT and own all returned bytes/values.
- Private strict typed projections resolve the immutable original receipt ONLY,
  uint32/signed64 extrema, actual typed configuration/context/optional known cfg.
  Context cfg may differ from arrival cfg. CID projection is ONLY actual ID,
  original ReceiptId and that original receipt identity target. It does not load
  current contribution state, original support anchor or mutable receipt attempts;
  support receipt need not equal plan confirmation receipt.
- Full find validates all key types/domain/count/min/max/head before read caps,
  then complete scalar groups/ordered children and checked combined child totals.
  Targeted find separately checks head/max and required initial metadata/quote/
  dependency flags, one selected complete body and its local parent identity;
  absent requested row is absent without claiming full continuity. Sparse MAX can
  pass only this declared selected scope; full/append reject the global count gap.
- Existing initial/append health audit traverses structural metadata with private
 64-row keyset pages and exact chain/audit/quote-original-money/dependency checks.
  It does not decode old unselected child payloads or impose full-read quotas on
  append. Existing cumulative writer statement/binding/result budgets still apply.
  Healthy duplicate/conflict/stale/expected0/revision/event results make no own
  writes/cleanup and precede proposed replacement-target resolution.
- All allocating header/scalar/child/head SQL and binding buffers are prepared
  before first own write. Complete atomic writes are guarded through head update.
  Known local selected failures perform actual active rollback before port errors;
  writer/retention already-aborted failures map without a second cleanup. Existing
  capability/protocol/invalid-statement/unmapped WorkFailure remains original;
  commit/cleanup unknown outranks local reasons and no replay is introduced.
  After established rollback, own-write tracking clears before constructing an
  allocating exception, avoiding a terminal second-control masking bad_alloc.
  Any other allocation failure after own writes rolls back the entire caller unit
  or propagates actual rollback uncertainty; store destruction owns no promise.

## Actual first changed-source verification

Host GNU16.2.1 (20260810), CMake4.4.5, Qt6.12.0, Boost1.92 headers. This is host
synthetic SQLite evidence, distinct from later selected official pinned CI.

| Command | Actual outcome |
| --- | --- |
| `cmake --preset sqlite-plan-debug` | Fresh configure PASS |
| `cmake --build --preset sqlite-plan-debug -j 4` | Fresh82 steps PASS,0warnings |
| `ctest --preset sqlite-plan-debug -L '^sqlite-plan$'` | Changed-source28PASS/1FAIL of29,1.16s; not passing acceptance |
| `ctest --preset sqlite-plan-debug` | Changed-source172PASS/1 same FAIL of173,4.68s; all144 inherited/diagnostic cases PASS |
| `python3 tools/check_project.py` | PASS391 task records/122 decisions/links/presets/current HTML |
| `python3 tools/progress.py render --check` | Root-owned snapshot current |
| `clang-format --dry-run --Werror server/src/sqlite_application_plans.cpp` and `git diff --check` | PASS |

These first runtime checks were built from actual source blob
`b69784303454029a19509c8e5375ee0746036445`. Full raw output is preserved in scratch
`/tmp/storage015-initial-full173-LastTest.log`. No feature test is skipped/inverted.
All28 other cases pass, including actual mid-append SQL rollback, sparse MAX,
>4096-bit exact values, typed target scopes, limits, inert lifetime and unknowns.

The sole failure is frozen
`revisionsAppendContiguouslyWithoutRewritingInitial`, raw list comparison at line1003:
SQLNULL `override_authority` is returned as a typed-null QString QVariant by host
Qt6.12 QSQLITE, while the expected list uses invalid `QVariant{}`. The lists differ
by QVariant metatype despite correct SQL NULL. The preceding complete DTO/immutable
row checks pass, and `initialRoundTripPreservesCompleteOwnedSnapshot` independently
passes `typeof(override_authority)='null'`. Contract requires SQL NULL/no initial
provenance, not invalid QVariant representation. The author reported the exact
requirement-based issue to root and DID NOT edit the assertion or production
behavior to normalize a raw test connection's driver buffer. Root assigned a
separate minimal `typeof`/explicit nullness+numeric projection correction and
requires a different actual review before integrating it. Original red is preserved.

Subsequent static source refinement reuses the configured component bound during
prepared-write serialization, simplifies statement-capacity counting, and handles
allocation while constructing mapped exceptions AFTER established rollback.
Intermediate untested c784c9a was transient; fdc1496a compiled successfully in3 steps.
Current candidate source is `9b0b01f1f6cc8862ed4abac80a43ab0b54211a47`, compiled
successfully in another3 steps/0warnings; this later source has not yet run the
feature/full suites. Exact attribution is preserved; earlier runs do not certify
later source revisions.

## Remaining gates

Await separately reviewed baseline adapter correction, normal integration without
altering this author's scope, then changed-source targeted29/full173 reruns.
Different actual implementation/config review, unconditional automatic feature CI
promotion, latest-head selected official results/reviews/discussions/current rules
and normal merge remain root gates. No passing29/full173, completed implementation,
source approval, official acceptance or merge permission is claimed yet.

No evaluator, semantic namespace proof, permission engine, provider verification,
actual effects/jobs/full application transaction, live action or full release
claim is supplied. All wider release gates remain explicit and incomplete.
