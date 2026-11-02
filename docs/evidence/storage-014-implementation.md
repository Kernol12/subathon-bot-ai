# STORAGE-014 — contribution SQLite implementation

Implementer `/root/contribution_implementer`,2026-11-02. Assigned isolated
`local-evidence/worktrees/storage014-implementation`, branch
`task/STORAGE-014-implementation`, created clean at claimb065b9d after refreshed
main8859b644e8d99b34b3e899d0e44e1e2cce386e78. Own only
`server/src/sqlite_contributions.cpp` and this evidence. Root owns shared build,
presets/workflows/schema ordering/state/map/HTML/checkpoint. This is not a task
completion or independent approval.

Authority: specification v0.45 §§3.2–3.5/14/DB-01,
[brief](../tasks/EPIC-009/STORAGE-014.md),
[logical/API contract](../contracts/sqlite-contributions-v1.md),
[exact physical companion](../contracts/sqlite-contributions-schema-v1.md).
The explicit owner resume supersedes the previous stop. The existing reviewed,
compiled, observed-red gate was already integrated by PR37 before this author
changed production source: full81e0d5a/final6ee37fa approvals and original24
explicit-stub failures after selected officialQt41347563063. See
[baseline handoff](storagetest-014-handoff.md),
[original red](storagetest-014-red-ctest.txt). That completed stub-red was not rerun.

## Actual change and preserved boundaries

Source commit `5acd3d6d554ee07133620912572fc3ed694e1c3a`, Git blob
`61569bd13e34f515802271da1c2ea9c89b23c473`,889 source lines. It replaces the four
explicit throws with typed codec, pure manifest and controlled transaction primitives.

- Full typed ContributionId0..UINT64_MAX uses exact big-endian8 BLOB. Encoding
  reuses the already lossless pure uint64 primitive; new decoding rejects malformed
  buffers in its declared input category. SQL decoding strictly checks actual
  BLOB/length8; optional SQLNULL remains different from present0. ReceiptId0 stays
  invalid under its separate existing domain rule.
- The pure factory copies the actual immutable receipt manifest1/2 and appends
  exact approved migration3 SQL,20-object schema and starting versions0/1/2/3.
  Literals were transcribed from the physical contract, never from the independent
  test JSON. The existing manifest acceptance confirms full18126-byte payload/SHA
  and every unchanged prefix. No implicit migration or operational authority.
- Private original-only hydration reads actual immutable receipt identity/facts/
  arrival, exact uint32/signed64/optional groups and opaque bytes. The same existing
  session resolves actual retained configuration and modifier context, including
  the context's optional known configuration with identical limits. Historical
  known configuration need not equal arrival configuration. Original comparisons
  never use growing transport history or another repository transaction wrapper.
- Raw observation decoding validates original/evidence/role/support/proof groups
  without recursively loading a linked contribution. Contribution hydration
  validates a raw original-support anchor; public observation hydration then checks
  its contribution association. This bounded layering avoids validation cycles.
  Only this wrapper's own new IDs may temporarily lack their support anchor.
  Commit actually queries/checks all pending anchors before real session.commit.
- Pure input validation precedes owner/capability/mutable-state access. Repository
  wrappers allocate before actual BEGIN/adoption transfer. A returned wrapper owns
  its copied weak capability/options/pending IDs independently of repository
  lifetime; active destruction invokes real existing noexcept cleanup. Single-use,
  true actual-session stale/terminal/thread errors and allocation failures retain
  the agreed ownership/exception boundaries.
- Create reserves pending tracking and prepares both statements before the first
  write. Tracking after successful INSERT/allocator update cannot allocate.
  A transaction guard cleans unexpected allocation exceptions after adapter writes;
  original WorkFailure/capability/protocol/unknown categories are not relabelled.
  Local storage refusals call actual rollback before the known port error, rather
  than using inactive cleanup to fabricate a known-uncommitted outcome. Retention
  errors are mapped only after that existing primitive's actual rollback.
- Current state updates leave original/observation capture immutable. Nullable
  association changes only to a compatible contribution; same-target no-op still
  validates all stored targets. Exact-key unresolved reads are complete, ordered
  and bounded; link/update use targeted rows without applying that read cap as a
  write quota. Actual allocator metadata/maximum/exhaustion/ambiguity failures are
  refused without repair, wrapping, gap reuse or silent truncation.

The unchanged ContributionEvidenceService owns created/linked/duplicate/unresolved,
correlation and evidence promotion. SQL introduces no eligibility, provider-proof,
settlement, authorization or financial-truth inference. Each supplied receipt's
prior committed/nonconflicting/classified status remains an honest upstream
precondition; current SELECT cannot prove it. Semantic policy/decision/actor/
verification/resolution meaning stays with its owning future contracts. Actual
HistoricalValuation rates/source/time remain015. No extra writer, connection,
raw SQL handle, private writer capability, nested commit, retry or live action.

## Actual checks and environment

Host Linux x86-64, GNU16.2.1 (`20260810`), CMake/CTest4.4.5, Qt6.12.0 and actual
SQLite3.54.0 as reported by all four existing/independent green diagnostics.
Native execution followed the actual source replacement; no unmodified stub was
run as a new red. No compile failure, runtime failure, assertion edit or fixture
change was needed for this initial implementation candidate.

| Command/check | Actual result |
| --- | --- |
| `cmake --preset sqlite-contribution-debug` | Configure succeeds |
| `cmake --build --preset sqlite-contribution-debug` | Fresh70-step build succeeds,0warnings; real new adapter/test executables linked |
| `ctest --preset sqlite-contribution-debug -L '^sqlite-contribution$' --output-on-failure --parallel 1` | First changed-source run:24/24 PASS,0failures,total0.76s |
| `ctest --preset sqlite-contribution-debug --output-on-failure --parallel 1` | Full143/143 PASS,0failures,total3.25s |
| `clang-format --dry-run --Werror server/src/sqlite_contributions.cpp` | Pass |
| `git diff --check` | Pass |
| `python3 tools/check_project.py` | Pass391 tasks/122 decisions/current links/presets/HTML |
| `python3 tools/progress.py render --check` | Root-owned snapshot current |
| Exact frozen-artifact comparison against integrated main8859 | All assigned immutable artifacts/prior producer sources match |

Full143 log is local `/tmp/storage014-implementation-full143.log`; Qt/SQLite
runtime lines are retained in this worktree's ignored CTest LastTest.log. The
24 actual cases cover optional0, signed-boundary/MAX allocator behavior, immutable
originals/conditions, independent state, real service composition, direct SQL
constraints, typed original/retained corruption and limits, staged write/link/state
rollback, active RAII before outer finalization, owner/weak lifetimes and physical
commit/cleanup acknowledgment uncertainty. Passing simulations/live/provider or
platform coverage is not inferred from those results.

Frozen actual artifacts are unchanged:

| Artifact | Git blob |
| --- | --- |
| Reviewed24 feature cases | 46c901e0225b7755153decf25f4a5ff226d063db |
| Independent green consumer | 577edeb0517e02e79263f5d439dedf5bc12306b1 |
| Independent literals | 44df9918cc8494dc209e3b1812784a080860e06c |
| Public adapter API | 0abeea2bb6d1e99faad7d6585400ad3d80e2eae8 |
| Logical/API contract | f503f385bbd4fd4ce4b43dd3243911dc3bce8f40 |
| Physical companion | eeb2fe04be49b7eeced0db6cf9eea29520dffc64 |

Exact comparisons also preserve existing writer/migration/retention/receipt source,
core/ports/service and every earlier assertion. Only this assigned CPP and own
new evidence are changed; no API/test/fixture/root-CMake/preset/CI/state/map edit.

## Required remaining gates

Different actual source/config review, root integration and ordinary CI promotion,
actual automatic selected official Qt acceptance, final evidence/CI-attribution
review and latest-head rules/discussions/checks/normal merge remain root gates.
This author does not self-certify them. Current local host SQLite3.54 proof is not
bundled officialQt/SQLite3.53.4 verification. All seven integrations/three FX
providers, application bundles/effects/jobs, semantic config/permissions, actual
rates/plan capture SQL, backup/update/recovery/power-loss, both renderers/UI/locales/
themes/packaging/platform/live/OBS and full release remain incomplete and required.
