# STORAGETEST-010 — independent test, interface and harness review

Reviewer: `/root/writer_transaction_reviewer`, 2026-11-01. Test/interface author:
`/root/storage_decomposition`; shared build/workflow/metadata author: `/root`.
This reviewer authored only this evidence in isolated worktree
`local-evidence/worktrees/storagetest010-review`, branch
`task/STORAGETEST-010-review`. No production implementation or test assertions were
authored by the reviewer.

## Exact reviewed baseline

Approved candidate: `d09d3302eea46045fb7cc0522348f269607f6303`, based on merged
main `e374788`, integrating author revisions `6532c07`, `76d3a00` and the requested
test corrections `0c6fc8ee429150bda3f0beeccd9f956c8f5b022b`. Shared harness revision
`d680dd4` was reviewed before the full candidate. Exact reviewed artifacts:

| Artifact | Git blob |
| --- | --- |
| [24 Qt Test cases](../../tests/qt/sqlite_writer_work_tests.cpp) | `eb9e3b600788f9473c5be7abdda86440cd73a3f4` |
| [Additive public test boundary](../../server/include/subathon/server/sqlite_writer.hpp) | `afc9f483da7d8e61a5e73715ca33819216e9e668` |
| [Acknowledgment fault seam](../../server/include/subathon/server/testing/sqlite_writer_work_faults.hpp) | `a2cd7b8900848d8e7b7091b676aeaf16754efe0e` |
| [Explicit missing-behavior stub](../../server/src/sqlite_writer_work_stub.cpp) | `b47f131af895311cef790c64ac0187532994a819` |
| [Author evidence and WT map](storagetest-010-author.md) | `12ff38326ea248db2c8579dbec111a79ff144b28` |
| Root CMake | `7a76e27d4ab5e191d846d57ce04981cd99ef1485` |
| Server CMake | `0d630394b3d53acef334f7f60fc1945a2c5fd0c5` |
| Qt test CMake | `452aa29706717c3a5687936c001e9ac52f7692c6` |
| Presets | `d4c63892e819c1945838ab82b1c969516adee8b5` |
| SQLite writer workflow | `4c684ef0781fd7fe93628ef5f6f84d752560f34f` |

Authority: specification v0.45 §2/§3.4, DB-01/SERVER-01,
[STORAGE-010 brief](../tasks/EPIC-009/STORAGE-010.md),
[writer transactions revision 1](../contracts/writer-transactions-v1.md), existing
SQLite writer/receipt/contribution contracts and actual synchronous ports/services.
Root/server/tests AGENTS instructions, workflow and test strategy were read.
The original writer source, both old test files and approved transaction contract
have no diff from main. The existing 25 writer assertions are preserved.

## Independent assertion and interface assessment

All 24 test bodies and their WT01–WT20 map were inspected, including admission and
cleanup additions. Expected values come from literal independent SQL observations,
explicit transaction outcomes and fixed arithmetic. The observer owns a separate
disposable fixture connection and supplies no work/session algorithm. Its tables
are synthetic mechanism fixtures, not proposed domain schemas.

The real ReceiptIntake service is composed with a narrow primitive repository
that delegates physical transactions to the new session. Immediate independent
sum 30 after the service returns, while work completion is still pending, rejects
deferred outer-commit semantics. The known-rollback and uncertain-commit service
paths are distinct. The fixture does not claim real receipt encodings, deduplication
or provider behavior. Production repository adapters remain later work.

Duplicate-key, deferred-constraint COMMIT, budget and known-service failures use
independent zero-wait BEGIN IMMEDIATE/ROLLBACK probes inside the body after catching
the synchronous error. These prove transaction release before that error escapes;
row invisibility alone would not establish rollback. Other cases establish real
pre/postcommit visibility, rollback of prior writes, busy/no replay, retained
committed effects after a body exception, and suppression of normal success values
for unsuccessful work. The first SQL failure survives later protocol/body errors.

The acknowledgment-loss seam is restricted to one pending owner/idle installation
and the targeted successful real new-work COMMIT/ROLLBACK. It supplies neither an
executor nor a fabricated work result. Tests independently require physical sum 30
after lost commit acknowledgment and zero rows after lost rollback acknowledgment,
ordered old/new queued failure, no queued body execution and closed admission.
Existing queued batches correctly retain `sql_failure` with no rows, matching the
unchanged closed-connection dispatch path; new work uses
`not_started/writer_unavailable`. This is distinct from new admission's
`shutting_down`. The future implementation review must verify the seam's actual
placement below classification; no placement exists in the explicit stub yet.

Copied context/session tickets contain only weak private state and expose no SQL,
driver or writer ownership. Tests safely retain values after commit, during later
work and after writer destruction, without calling dangling references. Live
foreign begin/execute/commit/rollback/cleanup must reject while the legitimate
owner still commits. Terminal same-body operations and stale later-work operations
have separate errors. Noexcept cleanup, inactive cleanup and uncertain active
cleanup are covered without allowing a redundant rollback to replace an outcome.

Mixed FIFO, capacity two with one active/one queued, capacity retained after commit
until body return, admission from completion, mixed shutdown drain and connection
release before shutdown completion are asserted. Completion exceptions and
completion destruction of the writer cannot change fixed outcomes or stop healthy
work. Owning request captures and returned text/blob values are tested through
session/writer closure; arbitrary borrowed captures are not promised normalization.

Resource oracles are literal and independently checked: three session statements
are accepted and a fourth rejected; SQL text is 48 versus 49 UTF-8 bytes; binding
and result budgets use `8 + UTF-8(é) = 10` and `8 + blob(7) = 15`, hence exact 25
versus 26 bytes; scalar/null bindings cost eight each; result rows total two versus
three. Each overage follows a speculative write, requires synchronous rollback
and suppresses a success payload. Limits accumulate across execute calls while
SQL text remains a per-statement limit. Unsupported QVariant and prohibited SQL
tests preserve the selected trusted SQL boundary.

## Findings resolved before approval

The reviewer requested a direct `<utility>` include for the new `std::move` use;
the candidate includes it. The full first test revision also required two focused
outcome-matrix additions, without new slots or weakened assertions:

- Automatic rollback acknowledgment loss after a first duplicate-key SQL failure
  now requires synchronous `uncertain_outcome` and terminal
  `outcome_unknown/uncertain_outcome`, overriding the original SQL error. The direct
  rollback variant remains; both assert physical zero rows, one body invocation
  and ordered queued failures without executing the queued valid work.
- A pre-BEGIN validation/body exception now requires `not_started/work_exception`
  and is followed by healthy committed work with exactly one persisted row/sum 40.

Both changes were inspected in actual author commit `0c6fc8e`, not inferred from
the author's description. The optional result dereference is guarded. The corrected
old-batch error mapping and strengthened rollback probes are retained. No blocking
finding remains in the reviewed interface, tests or shared harness.

## Commands actually run

Observed local environment: Linux x86-64, GNU C++ 16.2.1, Python 3.14.8,
CMake 4.4.5, Ninja 1.13.2 and Qt 6.12.0. No personal preset or new dependency.

| Command/check | Actual result |
| --- | --- |
| `cmake --preset sqlite-work-debug` | Exit 0 at `9a9f29c` |
| `cmake --build --preset sqlite-work-debug` | Exit 0 at `9a9f29c`; rebuilt corrected test target successfully at `d09d330` |
| `ctest --preset sqlite-work-debug -L sqlite-writer` | Exit 0; all 25 unchanged writer/ownership cases passed at `9a9f29c`; their source/header/stub artifacts are unchanged in the final candidate |
| `ctest --preset sqlite-work-debug -L scaffold` | Exit 0; all four infrastructure cases, including Qt event loop, passed at `9a9f29c` |
| Independent Python comparison of compiled `-functions` and CTest `--show-only=json-v1` | Exact 24-name/count equality; 53 total registered cases; passed before and after the requested fixes |
| `python3 tools/check_project.py` | Exit 0 at final candidate; 391 records, 122 decisions, links/presets/current HTML valid |
| `python3 tools/progress.py render --check` | Exit 0 at final candidate |
| `git diff --check`, unchanged-source comparison and `git hash-object` | Exit 0; exact artifacts above and no changes to old source/tests/contract |

No new work test body was run by this reviewer before approval. Compilation,
discovery, existing passing cases and this review are not an intended-red result.

## CI assessment, verdict and next gates

The shared preset builds the new interface/stub/cases alongside old acceptance
and the Qt probe. The workflow preserves its stable job name, pinned action,
selected verified Qt/ICU archives and automatic old-case execution. It checks exact
compiled-versus-registered new-case names/counts. The separate `sqlite-work` label
keeps known-red bodies out of the stable automatic pass gate; explicit manual
`run_work_acceptance=true` runs them with normal failure propagation. There is no
failure inversion, WILL_FAIL, blanket continue-on-error or skipped acceptance claim.
STORAGE-010 must promote these reviewed cases to ordinary automatic acceptance
when implementation passes.

An independent live `gh pr view 29 --json ...` query observed
[PR 29](https://github.com/Kernol12/subathon-bot-ai/pull/29) OPEN/DRAFT at exact
`d09d330`, all 11 check runs SUCCESS and no GitHub reviews. This is check metadata,
not a claim that new work bodies ran; actual first Ubuntu build/discovery/old-case
logs are attributed separately in [coordinator CI evidence](storagetest-010-ci.md).

**Verdict: approved actual interface, test oracles and harness for the
STORAGETEST-010 baseline at `d09d330`.** The coordinator may now run and record the
genuine intended missing-behavior red against these exact reviewed artifacts after
successful build. That red must still be observed before baseline task completion
or STORAGE-010 implementation. Do not substitute compile/environment failure,
missing registration, timeout or skip for intended red.

Publish the integrated review, complete final metadata and all latest-head CI,
discussion and normal merge gates. No production work/session implementation,
domain schema, real repository durability, recovery, live provider or release
platform capability is approved. Deterministic lost-ack tests do not prove disk or
power-loss behavior; all downstream test-first and full-release gates remain.
