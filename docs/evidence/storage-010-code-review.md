# STORAGE-010 independent code and configuration review

Reviewer: `/root/storage_work_code_reviewer`, 2026-11-01. Implementation author:
`/root/debt_code_reviewer`; shared build/CI and integration author: `/root`.
The reviewer owns only this evidence in isolated worktree
`local-evidence/worktrees/storage010-review`, branch `task/STORAGE-010-review`.
No production source, tests, expected results, fixtures or public interfaces were
authored or changed by this reviewer.

## Actual reviewed revisions and authority

Source author revision: `ad66ded737f60e66b6dae5bffc2989914011212e`, integrated by
the coordinator as `2aa14cd`. Implementation evidence: author `5025715`, integrated
as `3ed2362`. The reviewer normally merged that actual integration and inspected
the complete source, its diff and the implementation evidence. Shared build/CI
revision `ce0a325` was inspected in preparation and again with the source present.
The coordinator's actual in-review handoff/state at `a1e3a89` was also inspected;
its executable artifacts are unchanged and it leaves review/Ubuntu/publication
gates explicit. PR30 was published as a draft by the coordinator.

Authority: specification v0.45 §2/§3.4, DB-01/SERVER-01, the
[STORAGE-010 brief](../tasks/EPIC-009/STORAGE-010.md),
[work/session revision1](../contracts/writer-transactions-v1.md), and
[existing writer revision1](../contracts/sqlite-writer-v1.md). Root/server/tests
instructions, coordinator instructions, architecture, workflow, test strategy and
the actual reviewed/red STORAGETEST-010 handoff were read. The valid historical
test-first gate was preserved; it was not restarted or replaced with a build result.

| Artifact | Reviewed Git blob |
| --- | --- |
| `server/src/sqlite_writer.cpp` | `023451987e40a9330fef91c6c0955f0e527f14e6` |
| Public writer/work header | `afc9f483da7d8e61a5e73715ca33819216e9e668` |
| Acknowledgment-fault declaration | `a2cd7b8900848d8e7b7091b676aeaf16754efe0e` |
| New24 work tests | `eb9e3b600788f9473c5be7abdda86440cd73a3f4` |
| Existing22 writer acceptance cases | `f8418e83dbd88d74505c047e6b3e2c30304ade69` |
| Existing3 ownership regressions | `34c25c2786db42f6a4483fc874da9d4d3fc525a3` |
| Work contract revision1 | `4ba20506db9a858450099b82888a22c8f6dbd761` |
| Existing writer contract revision1 | `4d280461966f3fd9fcb13ab6d054c88228ca05b8` |

The seven non-source artifacts are byte-identical to merged baseline `c60e1ed`;
an independent exact-path Git comparison returned no diff. Removing the explicit
stub does not remove its evidence: the historical review link now resolves through
immutable `c60e1ed`, whose stub hashes to the original
`b47f131af895311cef790c64ac0187532994a819`. The prior review's findings, assertions
and recorded red remain intact.

## Source assessment

Each executing item gets a distinct private state. Public capabilities retain only
weak references. A live operation first compares immutable owner identity, before
reading mutable validity, phase, budgets or connection state; a foreign call cannot
poison the legitimate transaction. Item finalization revokes validity and clears
connection/callback references before completion. A transient foreign lock on the
state cannot extend connection lifetime or reach the next item's state. Writer
destruction from a body remains outside the contract's lifetime precondition.

The state permits exactly one actual BEGIN IMMEDIATE attempt. Execute, COMMIT and
ROLLBACK use the existing private owner-thread connection, with local query objects
destroyed before transaction resolution or connection close. COMMIT succeeds
synchronously before the service/body proceeds; there is no deferred outer commit.
Confirmed committed state survives later terminal-operation misuse or body failure.
No raw database/driver/native handle escapes, second writer, retry, nested wait,
domain schema, migration or provider capability was introduced.

Owner SQL, validation, budget and protocol failures record their first error and
synchronously roll back active work before exposing a known-uncommitted failure.
The completion preserves that first framework error over later body exceptions.
Failure to establish control acknowledgment records `outcome_unknown` and overrides
earlier errors with `uncertain_outcome`; admission and the connection fail closed.
Queued old batches then get their existing SQL failure/no rows, and queued new work
gets not_started/writer_unavailable without invoking its body. No implicit recovery
or reopening path follows uncertainty.

Cleanup is noexcept, checks thread and recorded phase, and only rolls back a live
active owner transaction. Committed, rolled-back, unknown and expired cleanup does
not repeat physical rollback. The finalizer also resolves an abandoned live
transaction, so retaining a wrapper cannot avoid rollback. Normal values are
published only after normal body return with a confirmed commit and no work error.

The fault seam is physically below outcome classification: `work_control` first
executes real SQL and returns early on a failed control. Only a successful targeted
new-work COMMIT/ROLLBACK loses its acknowledgment. Old batch controls, failed
controls, BEGIN and inactive cleanup cannot consume the pending fault. The seam
sets no work result and fabricates neither SQL execution nor persistence; tests
independently observe committed/rolled-back rows and ordered fail-closed drain.

One mixed FIFO and active flag cover old batches and new work. The active slot
remains occupied through body return and finalization, including the interval after
an early commit. The pending slot is released and the next dispatch scheduled
before external completion. Both completion branches return without further Impl
access, allowing a completion to destroy the writer safely; exceptions are contained.
Shutdown retains the existing drain, close and lease-release ordering.

Execute snapshots SQL/text/blob bindings with the existing deep-ownership helper.
Statement attempts, bound values, result rows and result values accumulate in one
state; SQL text retains its per-statement limit, type/limit errors remain distinct,
and the existing SQL allowlist and semicolon rule are reused. Limits cannot change
while the open writer drains its admitted queue, so the state sees the admission
limits. Returned Qt SQL values are owned after local query destruction. Arbitrary
callable captures/results retain the documented trusted owning-input precondition.

## Configuration and commands actually checked

The actual `ce0a325` build/CI change removes the stub source, retains the selected
Qt/action/archive pins and stable job name, and runs all old25 and new24 bodies
unconditionally on PR/main/manual events. Complete compiled-versus-registered name
and count guards remain. There is no failure inversion, skip, manual acceptance
switch or blanket continue-on-error. Removing the historical draft label changes
neither assertions nor case registration. Actual Ubuntu execution and latest-head
merge checks remain coordinator gates; this review does not infer them from YAML.

Local environment: Linux x86-64, GNU C++16.2.1, Qt Core/Sql/Test6.12.0,
CMake4.4.5, Ninja1.13.2 and Python3.14.8. No personal preset, credentials, live
provider operations or production database was used.

| Command/check | Actual result |
| --- | --- |
| `cmake --preset sqlite-work-debug` | Exit0 at `2aa14cd` |
| `cmake --build --preset sqlite-work-debug` | Exit0; actual library and all Qt test executables built |
| `ctest --preset sqlite-work-debug -L sqlite-work` | Exit0; all24 real work cases passed |
| `ctest --preset sqlite-work-debug` | Exit0; 53/53 passed, comprising new24, unchanged old25 and infrastructure4 including the Qt probe |
| Independent Python comparison of CTest `--show-only=json-v1` with each executable's `-functions` | Exact nonempty name/count equality for work24, old acceptance22 and ownership3; 53 total registrations |
| `python3 tools/check_project.py` | Exit0; 391 tasks/122 decisions, links/presets/current HTML valid |
| `python3 tools/progress.py render --check` | Exit0; current snapshot |
| `/tmp/subathon-actionlint-1.7.12/actionlint .github/workflows/sqlite-writer.yml` | Exit0; plain `actionlint` was unavailable on PATH |
| `clang-format --dry-run --Werror server/src/sqlite_writer.cpp` | Exit0 |
| Exact-artifact Git diff/hash checks and `git diff --check` | Exit0; preserved artifacts above and clean whitespace |

The native runs occurred before the evidence-only `3ed2362` integration; its source,
tests and shared executable configuration are unchanged. No extra red run or new
test assertions were authored. Passing these checks establishes this bounded real
SQLite mechanism and synchronous synthetic service composition, not arbitrary SQL
deadlines, process-memory bounds, real repository adapters or power-loss behavior.
Project/progress/whitespace checks were repeated successfully after merging
`a1e3a89`; an exact executable-path diff from tested `2aa14cd` remained empty.

## Verdict and remaining gates

**Approved for STORAGE-010 source/configuration integration at source blob
`023451987e40a9330fef91c6c0955f0e527f14e6` and shared revision `ce0a325`.** No blocking
finding or source change is requested. The implementation evidence accurately
separates actual local results from independent review and Ubuntu CI still pending
at its authoring revision.

The coordinator must update final task/handoff metadata, observe the changed
automatic Ubuntu acceptance, and satisfy latest-head CI, review-discussion and
normal merge rules. Material source/test/interface/config changes require renewed
review. Application schemas/adapters/migrations, durable application atomicity,
recovery authorization, disk/power-loss, supported platforms and live-provider
verification remain explicit later gates. No full-release claim follows this review.

## Final integration, completion metadata and actual CI attribution

The same independent reviewer inspected final completion candidate
`f370eebfae870d5f2e8fc7c5170c1d7312e41747` on 2026-11-01 after a normal merge
into the isolated review branch. Before this appendix the review tree exactly
matched the candidate. Initial independent approval `9216d8d` is integrated as
`17adb35`; its review content is unchanged.

An exact executable-path comparison from tested `2aa14cd` is empty. Source remains
`023451987e40a9330fef91c6c0955f0e527f14e6`, and all seven baseline artifact hashes
remain the table above. Historical STORAGETEST-010 evidence, including actual
review/red and the immutable stub link, is unchanged from `ce0a325`. No passing
native suite or original red was repeated for these metadata-only changes.

State, acceptance map, architecture, test strategy, CI assessment, implementation
handoff and coordinator checkpoint were inspected. All391 task identities and
dependency lists are preserved; only STORAGE-010 task data changed. Its done status
means this bounded internal mechanism passed acceptance, independent review and
actual ordinary CI. Publication is still pending, and application schemas/adapters,
migrations/application bundles/recovery, power loss, providers and release
platforms remain explicit separate gates. Current documentation now describes
ordinary passing work acceptance while retaining the original red as history.

The reviewer independently executed `gh run view 41300728417 --json ...` and
`gh run view 41300728417 --log`. Actual run metadata identifies pull_request head
`a1e3a89131439c69dcc2258021f0774f8efa4f74`, completed successfully, with successful
real writer/work acceptance steps. Independently fetched logs establish both
selected Qt/ICU SHA256 checks OK, GNU13.3.0, Qt probe1/1, exact compiled old25/new24
registration, 25 distinct old passing bodies and 24 distinct unconditional new
passing bodies with zero failures. The actual command outputs were parsed against
those fixed counts. An exact-commit GitHub check-runs query independently confirms
all11 checks completed successfully for `a1e3a89`. The
[recorded Ubuntu run](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41300728417)
is correctly attributed to that revision, rather than transferred to a later head.

Independent project consistency, progress render-check, whitespace, exact-artifact
and task/dependency comparisons all passed for the final candidate. A live
[PR30](https://github.com/Kernol12/subathon-bot-ai/pull/30) query observed OPEN/DRAFT
at exact `f370eeb`, MERGEABLE with no GitHub reviews; its latest SQLite job was
still IN_PROGRESS at that observation. This supplies no later-head success claim.

**Final verdict: approved completion metadata and CI attribution for STORAGE-010
at `f370eeb`.** No blocking finding or owner decision remains. Publish this final
review, then observe all applicable checks for the exact final PR head and satisfy
resolved discussions/current rules and the normal merge checklist. The task is not
assumed merged, and no full-product or later storage capability is approved.
