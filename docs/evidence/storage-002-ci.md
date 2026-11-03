# STORAGE-002 — CI coverage and observed checks

Scope: concrete SQLite application-bundle revision1, specification v0.45 §3/§16,
DB-01/OPS-02. Synthetic disposable real SQL only; no provider/live/platform claim.

Current status: reviewed implementation and official source6133 acceptance PASS.
Final completion-head CI/normal merge gates remain separate. Historical checkpoints
are retained below, followed by actual final local and official evidence.

## Historical claim and intermediate source checkpoints

The inherited baseline [PR42](https://github.com/Kernol12/subathon-bot-ai/pull/42)
compiles/discovers32 cases and runs required five-method fixture diagnostics, but
intentionally does not run feature bodies while source is stubbed. Its original
review/red evidence is preserved in [baseline handoff](storagetest-002-handoff.md).
Do not repeat that completed first-red gate.

For implementation, root prepared the existing sqlite-writer-build workflow to
retain its exact32 compiled-registration guard, mandatory fixture diagnostics,
all previous suites and pinned actions/dependencies, then run
`ctest --preset sqlite-bundle-debug -L '^sqlite-bundle$'` unconditionally. Only stale
intentional-red wording and one CMake comment change. No condition, skip, failure
inversion or manual feature input. Existing check name and effective repository
rules remain unchanged. Different reviewer inspected that actual configuration
diff and found the promotion appropriate; full source/runtime approval was pending
at that initial preparation checkpoint (completed below).

[Draft PR43](https://github.com/Kernol12/subathon-bot-ai/pull/43) initial claima13970e
received all11 checks PASS, including [SQL41582682577](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41582682577).
That revision contains no implementation or promotion, so this is inherited
stability only. At that initial claim, implementation acceptance and changed-
workflow official results had not yet run; completed results are recorded below.

Actual current host version probe: GCC16.2.1, CMake4.4.5, Ninja1.13.2,
QtCore/Sql/Test6.12.0, Python3.14.8; existing cache selects Boost1.92. Raw command
results retained in local-evidence/storage002-environment.json. Host SQLite and
selected official CI SQLite are recorded separately in the completed evidence below.
Root project consistency391/122/current HTML/diff checks PASS; tooling19 PASS0.245s.
`actionlint` is absent from current PATH; no lint pass is claimed. Actual changed
workflow execution is a merge gate, independent of local lint availability.

At intermediate source candidatef969/helper824, the independently reviewed terminal
oracle correction945e and additive byte regression291 compiled warning-free.
CMake/CI guard became33/207 registrations; targeted regression red was pending then.
The unconditional run was unpublished until correction/acceptance (completed below).


## Actual final local source/configuration verification

Root CLEAN6a6a200/sourcea905/helperb0ca/test8bf: configurePASS, final3-step build
ZERO warnings, target35PASS2.33s/full209PASS7.07s, all206Qt totals0fail/0skip/
0blacklisted, hostSQLite3.54.0. Different reviewer5b71576a actual fresh final20-step
build0warnings,35PASS2.27s/full209PASS7.06s/exact35compiled and enabled; same
source/test/config blobs. New slot with3data rows is one compiled/CTest method.

| Root raw artifact (local-evidence/) | SHA256 |
| --- | --- |
| storage002-final-build.txt |aa842f10adb17280755a4b6dee29763a073311badcf4885b30e967cb6bd4c1a1 |
| storage002-final-bundle35.txt |925dc1825460af836d2db3b5e9cb63c7f4ff6048e472921a73abeec38c37bd4e |
| storage002-final-full209.txt |686e85ed12a5ee1ad6f2f5cf430f81765cde9f1ecd180f1af2fa52c6b1cf66c8 |

Workflow final783dda7 preserves pins/check name, old164 acceptance and required
fixture diagnostics, exact35 compiled guard then UNCONDITIONAL35 invocation. It
has actual independent configuration approval. At that local-only checkpoint,
published-source official acceptance was still pending; it is completed below.
No local actionlint pass is claimed. Effective rules26170263 have no
required-check list/GitHubapproval count0, but written all-latest-check/independent
review/resolved-thread/mergeability gates still apply.


## Actual promoted official selected-Qt acceptance

[SQL41585131851](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41585131851)
SUCCESS at actual complete reviewed published6133afe28db43a841234a4e683e9b20d4291afbe,
2026-11-03T05:38:58Z–05:40:55Z. Fresh92 buildZEROwarnings, GNU13.3/pinnedBoost1.83
compiler macro108300, verified officialQt6.12 archives, diagnostic actualSQLite3.53.4.
Old164 cases and mandatory7+3+3+3+4+5 fixture methods/QtprobePASS, exact35 compiled
registration guard, UNCONDITIONAL35/35PASS5.26s including all new regressions.
All11 actual6133 PR checksSUCCESS. No failure inversion/manual input/skipping.

Raw local-evidence/storage002-official-source-ci.txt SHA256
c59682ee7f9b0a61767653dd0ee7ba1dc86a135d4322c98120de151dbd81303e.
This is actual selected official component acceptance separate from hostSQLite3.54.
GitHub sourcehead6133 MERGEABLE/CLEAN/zero threads, formal reviews0 and required
GitHub approvals0 verified via effective26170263. It was still draft at inspection.
Task acceptance/code review are complete; final completion-metadata head must pass
its OWN latestCI and readiness/threads/rules/merge checks before normal merge.
No provider/platform/semantic application or full release claim follows.
