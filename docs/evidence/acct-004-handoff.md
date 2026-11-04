# ACCT-004 — original application service implementation

Coordinator root,2026-11-04, task/ACCT-004-application-service,
[draft PR51](https://github.com/Kernol12/subathon-bot-ai/pull/51).
Current checkpoint: component integrated. PR51 merged normally at
2026-11-04T05:56:21Z as 0b631dcbea1ec7765b3a66e41d6059e81de2979d, exact head
80cac620 after all eleven checks passed. All ten post-main workflows then passed.
No bypass, force update or branch deletion; broader release gates remain below.

## Behavior and boundaries

The new bounded server library applies one already captured ORIGINAL operation
through the existing authoritative SqliteWriter work queue. Each executed normal
outcome owns one actual session and acknowledged COMMIT. Current access precedes
protected lookup; complete command retry, global tagged claims and all active
holds precede present-day revaluation. Full current contribution state and actual
plan head are checked independently of the state token. Ordered exact timer/debt/
counter proposals, current cap and segmented clock preserve provenance and reject
partial application. Whole-input overflow/unresolved required policy uses actual
durable review holds in that same unit. Normal publication consumes only the
actual writer's committed, correctly typed, invariant-checked normal work value.
Unknown outcome and post-commit body failure never fabricate a response.

The canonical SAC1 command codec preserves every field/optional/raw byte. SAS1,
SAR1, SAA1, SAH1/SAH2 and synthetic SAJ1 have explicit checked retained meanings;
healthy unsupported formats remain not-ready. Trusted ports receive only callback-
scoped read capability. Private cached read limits outlive revoked authority as
DATA. No public SQL/session capability, schema, migration, wire or provider format
is added. The existing private job validator has a borrowed-vector overload; its
old PreparedBundle wrapper delegates without changing rule order or behavior. The private retained decoder shares the command framing scan while
keeping internal storage/intermediate limits separate from public input/result
budgets. All server/core boundaries remain unchanged.

Actual identity/grants/current-policy/clock/action/unique-reference producers remain
required downstream adapters. Tests use explicit independent synthetic ports and
real disposable SQLite, not live provider credentials. SAJ1 is non-deliverable.
This is no running server, financial-refund allocator, review/continuation command,
job delivery/recovery system, UI or platform/live/full product verification.

## Contracts, baseline and review

[Logical](../contracts/application-service-v1.md)6d0cd67f,
[concrete API](../contracts/application-service-api-v1.md)bbd6cd83, specification
v0.45 sections3–5/NUM-01–04/B08. [Baseline handoff](accttest-004-handoff.md) retains
35 independently reviewed Qt slots/224 scenarios and original same-clean-head
first missing-feature red; it was never replayed or weakened. Every original
method/assertion and frozen fixture remains unchanged.

Actual source audit found additional behavior and allocation gaps despite original
35/full300 passing. [Eight additive regressions](acct-004-regressions.md) preserve
separate pre-execution review and reached behavior red (nine scenarios), followed
by corrections. Original stub-red is historical and distinct. Runtime assertions
do not instrument allocation ordering; that guarantee requires actual source audit.
[Full code/config review](acct-004-code-review.md) and
[bounded other-author service-codec review](acct-004-service-codec-review.md) retain
findings, actual revisions, corrections and limited versus full verdicts.

## Actual native checks to date

GNU16.2.1/Qt6.12.0/QSQLITE SQLite3.54.0/Boost1.92, C++20, disposable files.

```sh
cmake --preset application-service-debug
cmake --build --preset application-service-debug --parallel 4
ctest --preset application-service-debug --verbose
python3 tools/check_project.py
python3 -m unittest discover -s tests/tooling -v
python3 tools/progress.py render --check
```

Corrected candidate7e033e9 (service5ced819/c85aa5d, codec217acc7) root full308PASS
24.84s,1420QtPASS/0FAIL/0skip/0blacklist, zero build warnings. Independent reviewer
samefull308PASS24.61s/1420QtPASS0skip; exact43 unique functions match registrations.
Project396tasks/122decisions, tooling19 and current HTML pass. All43 service bodies
are ordinary acceptance, not failure-inverted or skipped. These native outcomes
are a prior checkpoint. Final allocation-order hardening4612223/importce28ff3
also passes rootfull30824.84s/1420QtPASS0skip/zero warnings (14-step rebuild).
Raw final full log SHA2560617477e0409658462d4074f8ef18b128bd5bc74ac6fa5c80980fb81f51b6330.
Different final-source/config re-review APPROVED c791e72/import2ab1037 at exact
ce28ff3, independent full308PASS24.65s/1420QtPASS0skip. All actual allocation
ordering/formula findings are resolved in source; [review](acct-004-code-review.md).
Raw logs and exact hashes are preserved in ignored local-evidence and checkpoint.
Author isolated fullsuite306/308 had only two missing-review-MD tooling failures;
that context gap is explicit in author evidence, not a product failure or approval.

## CI assessment and remaining gates

Promoted existing selectedQt SQL workflow preserves pinned actions/Qt archives/
Boost1.83, old suites and mandatory six-slot independent foundation. It verifies
all43 enabled compiled service functions/registrations, requires acceptance rather
than draft-red labels and then executes all43 unconditionally. No manual flag,
failure inversion, blanket continue-on-error, credential side effect or hidden
assertion change. Actual changed-workflow [41781024558](acct-004-ci.md) passed at completea49ec06:
GNU13.3/Boost1.83/officialQt6.12/SQLite3.53.4, requiredfoundation41PASS0skip,
exact43 discovery and unconditional43PASS8.01s, all11same-headchecksSUCCESS.
Active GitHub ruleset requires PR/resolved discussions, zero approving reviews and
no bypass; it has no required-check rule. Written latest-head CI and independent
review gates still apply even without enforced check names.

Next: observe latest completion-head CI, current rules/discussions and eligible
normal merge, then refresh main and select the next dependency-ready slice. All seven
integrations/three FX, operational/security/backup/recovery, English/German/themes,
both renderers, supported platforms, packaging and live release scope remain required.


## Publication receipt

Final metadata head80cac620 passed all eleven checks, including selected run
41781864667, before the normal exact-head PR51 merge. Current rules and empty
review-thread list were checked; GitHub reported CLEAN/mergeable. Main was refreshed
clean and its merge tree equals the reviewed head. All ten post-main workflows at
0b631dc passed; selected Qt run 41782291620 is included. Actual API/check/log
records remain under ignored local-evidence. The coordinator immediately claimed
CORETEST-008's document stage on a branch from integrated main; no completed gates
were repeated. Product completion is not implied.
