# STORAGE-002 — CI coverage and observed checks

Scope: concrete SQLite application-bundle revision1, specification v0.45 §3/§16,
DB-01/OPS-02. Synthetic disposable real SQL only; no provider/live/platform claim.

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
diff and found the promotion appropriate; full source/runtime approval is pending.

[Draft PR43](https://github.com/Kernol12/subathon-bot-ai/pull/43) initial claima13970e
received all11 checks PASS, including [SQL41582682577](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41582682577).
That revision contains no implementation or promotion, so this is inherited
stability only. Implementation acceptance and changed-workflow official results
are NOT YET RUN and will be recorded against their actual revisions.

Actual current host version probe: GCC16.2.1, CMake4.4.5, Ninja1.13.2,
QtCore/Sql/Test6.12.0, Python3.14.8; existing cache selects Boost1.92. Raw command
results retained in local-evidence/storage002-environment.json. Host SQLite and
selected official CI SQLite will be recorded separately when the suites run.
Root project consistency391/122/current HTML/diff checks PASS; tooling19 PASS0.245s.
`actionlint` is absent from current PATH; no lint pass is claimed. Actual changed
workflow execution is a merge gate, independent of local lint availability.

Root source candidatef969/helper824 plus actual independently reviewed terminal
oracle correction945e and additive byte regression291 now compile warning-free.
CMake/CI guard become33/207 registrations; actual targeted regression red pending.
The proposed unconditional run remains unpublished until correction/acceptance.


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
has actual independent configuration approval; actual published-source selected
UbuntuQt6.12/Boost1.83/SQLite3.53.4 run remains pending at this checkpoint. No local
actionlint or official CI pass is invented. Effective rules26170263 have no
required-check list/GitHubapproval count0, but written all-latest-check/independent
review/resolved-thread/mergeability gates still apply.
