# STORAGE-014 independent source and CI configuration review

Reviewer `/root/contribution_reviewer`,2026-11-02, separate from implementer
`/root/contribution_implementer` and integrator `/root`. Assigned clean isolated
`local-evidence/worktrees/storage014-review`, branch `task/STORAGE-014-review`,
from actual claimb065b9d after integrated main8859b64/PR37. Own only this new
review evidence. Initial preparation was read-only; no absent implementation was
approved and no unchanged historical stub-red was invoked.

Verdict: **approved the actual source and bounded CI promotion**, with no
unresolved blocking finding. Actual source5acd3d6/roota7659b5 and configuration
root1c08754 were reviewed, then evidence-only roota94e596 was normally merged.
The author note was read and its results remain attributed to its author.
This verdict does not approve absent official CI, final metadata, merge or release.

| Reviewed artifact | Actual Git blob |
| --- | --- |
| server/src/sqlite_contributions.cpp | 61569bd13e34f515802271da1c2ea9c89b23c473 |
| Root CMake option wording | 17ca259638724a90d4732ba4da48cfb1f149824f |
| Preset display wording | 35e0c1506a36288c7596c290002711a75ea62d92 |
| Qt CTest label/comment metadata | 2da4c8e556cb64261d0e5416090a63762e6d6703 |
| Ordinary SQL workflow | 1fe354401128e260c7219d6a46a16f6746791c2b |

## Source and configuration inspection

Read the whole889-line CPP and committed configuration diff against b065b9d,
root/server/tests instructions, specification §§3.2–3.5/14/DB-01, brief, approved
logical/API f503/physicaleeb, core/ports/service, baseline review/red chronology
and existing writer/retention/receipt mechanisms. The integrated test-first gate
predates this source; its original red was preserved rather than repeated.

The following boundaries were checked beyond the passing assertions:

- Typed CId0..MAX remains unsigned BLOB8 throughout codec/allocator/associations;
  NULL differs from present0, ReceiptId stays nonzero, exact singleton/greatest-ID
  consistency and MAX exhaustion prevent reset/wrap/reuse. Pure manifest3 retains
  exact prefixes1/2/fourSQL/full20 objects and the18126-byte literal payload/digest.
- Private original-only hydration validates all receipt fields/types/groups/u32/
  signed64 values and exact bytes. Typed cfg/context/knowncfg resolution forwards
  the same limits and imposes no knowncfg==arrivalcfg equality. Corruption and
  well-typed limits remain distinct; no full-history wrapper/current-context guess.
- Raw observation→original, contribution→raw anchor, then public observation→target
  validation avoids recursion. Only tracked own-new IDs can temporarily lack an
  anchor; commit uses actual SQL before any incomplete claim. Captured support
  and current state remain independent. Same-target links still validate stored
  targets; retarget/key/no-key conflicts refuse, and read caps never become quotas
  on targeted writes.
- Pure input/options priority and all known enum/group/role/origin/byte bounds
  precede owner/session/mutable guards. n+1 fits the exact vector/qsizetype/qint64
  bounds without overflowing or promising allocation. Wrapper allocation precedes
  BEGIN/adoption transfer; copied session/options/pending state outlive repository
  scope. Actual active destructor cleanup occurs before the outer finalizer.
- Create reserves pending tracking and prepares both statements before its first
  write. Trivial tracking cannot allocate afterward; the transaction guard cleans
  unexpected post-write allocation failures and preserves actual uncertainty.
  No extra writer/handle/queue wait/nested unit or fake ownership registry appears.
- Local refusals use actual rollback before known port errors; four mapped writer
  errors and retained selected errors already established their rollback. Original
  capability/protocol/other unmapped WorkFailure and unknown durability remain
  unchanged; no second rollback, normal result or replay permission hides them.
- Existing service retains correlation/duplicate/link/create/promotion policy.
  Prior committed/nonconflicting/classified input and semantic/provider authority
  remain explicit upstream prerequisites, never inferred from SELECT/proof bytes.
  Actual rate/source/time remains015; stored application/reversal/settlement fields
  do not establish effects or permissions.

The actual CI diff removes only the contribution manual boolean/if, executes24
unconditionally after required green/exact registration and removes draft display/
label metadata. All earlier111 cases remain unconditional. Pins, runner, permissions,
timeout and check name are unchanged; no failure inversion/skip/assertion or fixture
weakening/new dependency is introduced. The green target still links only oldwriter/
Qt. Observing the actual official run remains the integrator's next gate.

## Independent commands and observations

Fresh commands in this review worktree, after the actual replacement:

| Command/check | Actual result |
| --- | --- |
| cmake --preset sqlite-contribution-debug | Exit0; fresh configure |
| cmake --build --preset sqlite-contribution-debug | Exit0;70 build steps,0warnings |
| CTest JSON discovery and both compiled -functions lists | Exact143 total,24 compiled/registered feature cases,3 green slots |
| ctest --preset sqlite-contribution-debug -L '^sqlite-contribution$' --output-on-failure --parallel 1 | Exit0;24/24 PASS,0.73s |
| ctest --preset sqlite-contribution-debug --output-on-failure --parallel 1 | Exit0;143/143 PASS,3.18s |
| clang-format --dry-run --Werror server/src/sqlite_contributions.cpp | Exit0 |
| /tmp/subathon-actionlint-1.7.12/actionlint .github/workflows/sqlite-writer.yml | Exit0 |
| python3 tools/check_project.py / progress.py render --check / git diff --check | Exit0;391 tasks/122 decisions/current HTML |

Actual host Linux x86-64/GNU16.2.1, Qt6.12.0 and SQLite3.54.0. CTest LastTest.log
shows all24 init/body/cleanup phases passing with zero skips and current new green
3 plus init/cleanup5PASS/0skip. The old environment probes report the same actual
Qt/SQLite runtime. These host results are not bundled official SQLite3.53.4 proof.
Captures: `/tmp/storage014-review-{configure,build,24,143}.log`, discovery JSON at
`/tmp/storage014-review-discovery.json`, and this tree's ignored CTest LastTest.log.
No extra test or runtime probe was written; beyond the assertions, the complete
source paths and boundary/error/ownership cases above were inspected statically.

Exact frozen checks against main8859 preserve feature46c901e, green577edeb,
JSON44df9918, API0abeea2, logicalf503, physicaleeb and original redaaa572b0. Old
writer/migration/retention/receipt/service/core/interface/assertion/fixture paths are
unchanged. No baseline test, fixture or expected result was edited by this review.
The only review-owned change is this evidence file.

## Remaining gates and release scope

Root still must integrate this exact approval, publish source/config and observe
actual automatic selected official Qt acceptance, complete final evidence/CI-
attribution review, recheck latest-head checks/current protection/discussions and
merge normally. This review does not claim task completion or future CI success.
All seven integrations/three FX providers, real application bundles/effects/jobs,
semantic configuration/authorization, actual plan/rate provenance, backup/update/
recovery/power-loss, both renderers/UI/locales/themes/packaging/platform/live/OBS
and full product verification remain mandatory and incomplete. No owner decision
or access is required for this bounded component.
