# ADR-0013 — Admit test programs in CI and record runs by commit

Date: 24 November 2026. Status: proposed for the owner's decision; nothing changes
until the owner approves and this record merges. Until then the standing test-binary
admission in `progress/coordinator-checkpoint.md` applies to every run.
Authority: owner-chosen direction (options 2 and 3) relayed by the advisor; rule 4(e)
and the CI/tools rule in [ADVISOR.md](../ADVISOR.md) item 3.

## Context

The standing admission applies to every Qt or GoogleTest binary, twice per stage:
passive ELF/ABI and dependency hashes, a first real run with bounded loader and
plugin logs, reconciliation of every loaded object, and hashes again afterwards.
Runs are also packaged and sealed locally, reviewed before and after, and pinned
to protected `main`. Most Claude-track load and most restaging after `main` moves,
a reboot or a coordinator restart come from these steps, not from product work.

**History: precaution, not incident.** The procedure began with the patched Drogon
and Trantor SDK of API-012/API-016, which sits beside the stock SDK
([native WebSocket profile §1](../contracts/native-websocket-profile-v1.md) requires a
distinct, admitted prefix). The owner then widened it step by step:

| Date | Step | Source |
| --- | --- | --- |
| 2026-11-17 | Qt loader method extended to rebuilt API-001 binaries only | [#278 comment](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6960584143) |
| 2026-11-18 | "Extend the existing procedure … unchanged" to every task running host binaries | `ea3e9df0`, [#703 comment](https://github.com/Kernol12/subathon-bot-ai/pull/703#issuecomment-6970529227) |
| 2026-11-18 | Every Qt or GoogleTest binary; `.dynsym`/`.symtab` Session ruling | `292c55f2`, [#703 ruling](https://github.com/Kernol12/subathon-bot-ai/pull/703#issuecomment-6971865504) |

None of these decisions cites a failure. Issues, PRs, `git log` on `main`, the
[evidence summary](../evidence/README.md) and tag `evidence-archive-2026-11` record
no run in which a wrong SDK or library was loaded or a symbol mismatch changed
behaviour. The incidents recorded around 2026-11-18 were caused by the procedure:

- The consumer Session RTTI check stopped a Linux clock binary that has no Session
  symbol at all ([#582](https://github.com/Kernol12/subathon-bot-ai/issues/582#issuecomment-6971795521));
  the owner narrowed the check.
- BUILD-002's `example8_twoEvents` crash was SIGXFSZ from the 1 MiB file-size cap
  the admission logs passed on to the test process
  ([#711](https://github.com/Kernol12/subathon-bot-ai/pull/711#issuecomment-6989789029)).
- SESSION-010 admission stopped on a sandbox netlink permission before any test ran
  ([#588](https://github.com/Kernol12/subathon-bot-ai/issues/588#issuecomment-7086960662)).

Because no incident is on record, this decision does not keep the full local check
(option 1) for SDK or network consumers. Their existing CI gate stays unchanged (below).

## Decision

1. **Admission moves into CI.** `tools/binary_closure.py` runs in the `core` and `qt`
   jobs after each build tree is built and before its first `ctest`. For every
   program the tree registers (the command plus ELF arguments inside the build tree)
   it asks the system loader (`ldd`) for the DT_NEEDED closure with resolved paths,
   using the test's CTest `ENVIRONMENT`. It compares each `soname => path` with the
   committed list in `tools/binary_closures/`. Any extra, missing or relocated
   library, unresolved name, unlisted program or unsupported registration fails the
   job. It logs each program's SHA-256. Runner-specific roots appear as `$QT_PREFIX`
   or `$RUNNER_TEMP`. A changed list is an ordinary diff reviewed in its PR.
2. **The commit and its CI run are the record of a test run.** Credited passing
   results are the required CI jobs on the reviewed PR head. On a pull request CI
   tests the merge of that head with current `main`, which replaces the separate
   protected-`main` pin. The PR names the head commit and links the run. Local
   sealed run packages and their before/after reviews end.
3. **One local run observes intended red.** Red cannot run in a passing CI gate, so
   the tests stage keeps one local run on the reviewed commit. Its record is the
   commit, the command and the failure summary in the PR. It needs no admission,
   seal or loader log. The independent tests review still reads it and still
   rejects compile failures, setup failures and skips as red.
4. **Suites CI does not run keep today's procedure.** Until a suite runs in a CI
   job, its credited local runs keep the standing admission unchanged. Today that
   covers the host lifecycle and other local-only native runs. Move a suite into CI
   to leave that path.
5. **Unchanged:** test-first order, specification-derived expected values,
   independent reviews, the Tier A cycle for accounting, authentication, migrations
   and concurrency, no weakening of tests, and the `native` job's SDK gate
   (`tools/native_websocket_ci.py`: passive ELF/ABI closure, controlled identity,
   loaded-map comparison and seals for every Drogon SDK consumer).

## What is given up

- Byte hashes of system libraries before and after a run. CI checks where each
  library comes from, not its bytes. Qt, ICU and libsodium stay pinned by archive
  SHA-256 and Boost by package version. A runner-image update that replaces a
  system library in place is not detected.
- Plugins loaded with `dlopen` (for example Qt's SQLite driver) are outside
  DT_NEEDED. `QT_PLUGIN_PATH` and the job's prefix check still point them at the
  pinned Qt.
- Reconciliation of the live loaded map against the passive closure for Qt and
  GoogleTest programs. Without `LD_PRELOAD` or `dlopen`, the loader resolves the
  same closure that `ldd` reports.
- Readelf Session-symbol checks for Qt programs that do not link the SDK. SDK
  consumers keep them in the `native` job.
- Local red runs carry no loader evidence. A wrong local library would show up as an
  unexplained red, which the tests review must classify before crediting it.

## Alternatives

- **Keep the procedure (status quo):** rejected by the owner for its cost.
- **Option 1, full check for SDK/network programs and light path elsewhere:** the
  fallback if an incident had been on record. It remains the first choice if
  the trial finds a defect.
- **Run every test, including red, only in CI:** rejected. Unreviewed tests must
  stay local, and red cannot enter a passing gate.

## Trial

The new procedure is a trial for the next ten tasks that run test programs, starting
when this record merges. After the tenth, the coordinator reports to the owner any
defect found later that the old local procedure would plausibly have caught: a wrong
library or SDK loaded, a symbol or ABI mismatch, a dependency changed during a run,
or an environment leak into a test. The owner then keeps this decision, restores
option 1 for SDK and network programs, or restores the full procedure. The report
also counts the closure-list updates and CI failures this gate caused.

## Consequences

- On merge, `docs/AGENT_WORKFLOW.md`, `docs/CI.md` and the checkpoint's standing
  admission rule change as in this PR. Earlier evidence keeps its recorded meaning.
- A PR that adds a test program, links a new library or changes a CI prefix updates
  the list in `tools/binary_closures/` in the same PR. On a mismatch the gate
  prints the observed list; reviewers check that the change is intended.
- The tooling tests cover the gate with synthetic loader output only.
