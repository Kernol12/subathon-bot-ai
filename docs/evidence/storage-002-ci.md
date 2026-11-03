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
