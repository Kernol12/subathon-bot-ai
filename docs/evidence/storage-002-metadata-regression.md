# STORAGE-002 additive quota and complete-metadata regressions

Author: bundle_validation, 2026-11-03, isolated
`task/STORAGE-002-boundary-regressions`. Root coordinates the actual task and shared
registration/workflow/progress. Different reviewer: bundle_reviewer (actual review
pending at this authoring revision). Production source candidate is f969ba6; held
corrections remain separate and must not precede the new reviewed/reached-red gate.

Authority: sqlite-application-bundle-v1 revision1, Bounds/byte roles/failure priority
and Selected-source structural closure; DB-01, specification v0.45 §§3,16. Input
and returned vector quotas are explicitly separate. Actual COUNT/contiguous
ordinals/kinds/history must cover every scoped row, including malformed values
outside a positive continuation key. No contract or baseline assertion is changed.

## Two additive public API cases

`lookupUsesReturnedQuotaIndependentlyOfInputItems` uses only frozen literal setup
and original_prepare/original_finish to install an existing complete operation.
With input_items1/result_items20/default adequate owned bytes, public lookup must
commit normally and return the complete independently specified original record.
`matches` compares that result against the frozen literal DTOs, and explicit vector
counts establish20=2+2+2 balances +4 effects +8 components +2 jobs +0 transitions
+0 reversals. No new input bundle is staged, and a full typed before/after database
snapshot must be unchanged.

`metadataRejectsRowsOutsidePositiveKeysets` has three independent Qt data rows:

| Scope | Deliberately extra malformed row | Preserved valid subset / explicit actual oracle |
| --- | --- | --- |
| read_state | kind1 balance on tokenstart | declared2, actual3, valid2, kind1count1 |
| full lookup | transitionordinal0 on original | declared0, actual1, positive0 |
| read_reversal_basis | mainbasis1 reversal position0 using existing score-refund current request1 | actual3, positive2, min0/max2; valid positions1/2 and head2/cumulative100/refund2/request1 unchanged |

The observer alone temporarily bypasses CHECKs to inject each synthetic corruption,
then restores CHECK enforcement before any new API call. The selected original
source stays unchanged. Every actual FK remains valid, including the extra progress
row's existing current score-refund effect; no fabricated missing reference supplies
the expected failure. The current score-refund operation's unselected body is not
claimed as valid by this narrower original-basis projection.

Each row verifies the actual corrupt raw counts before calling the public API, then
requires invalid_stored_data with actual whole-unit rollback. Existing refusal
helper observes a prior caller marker, requires an independent BEGIN IMMEDIATE
inside the catch while the facade lives, and rethrows. The case additionally checks
marker absence and the complete typed corrupted snapshot unchanged. Returning only
the valid subset, later outer cleanup, truncation or repairing the malformed row
cannot satisfy this oracle.

## Actual authoring checks; no case execution yet

- `clang-format -i tests/qt/sqlite_application_bundle_tests.cpp`; completed.
- Independent stdlib SQLite diagnostic built the old five FROZEN schema/source
  literals and injected the three planned rows. State and transition raw oracles
  passed. Initial progress proposal used nonexistent refund1/currentrequest2 and
  failed a real FK; it was rejected as setup defect, never feature red. Corrected
  witness uses actual score-refund/currentrequest1; raw3/positive2/min0/max2 and
  exact head2/100/refund2/1 pass with zero foreign_key_check rows and CHECK restored.
  No new facade, test body or production manifest was invoked by these diagnostics.
- `cmake --preset sqlite-bundle-debug`; exit0.
- `cmake --build --preset sqlite-bundle-debug --target server_sqlite_application_bundle_acceptance`;
  exit0. Final incremental build /tmp/storage002-regression-build-final.log has
  zero compiler warnings/errors.
- `build/sqlite-bundle-debug/tests/qt/server_sqlite_application_bundle_acceptance -functions`;
  exit0,34 compiled cases = original32 +new2. The conventional `_data` helper is
  excluded by actual Qt discovery; after merging the reviewer's other additive
  returned-byte case, root's complete guard should discover35.
- Programmatic deletion of ONLY the appended two-slot/data-helper block restores
  original main995e0fa feature CPP byte-for-byte. No original32 assertion changes.
- `git diff --check`; exit0.

Environment: GNU16.2.1, Qt6Core/Sql/Test6.12.0. Final author CPP SHA256
`8c4bccc096d6cfba7d4a28ec8699ff76555e927f25f7b1d4337aae79a39e64fa`.

No new case body has executed. Actual different test review, meaningful red on
f969+uncorrected helper824, source correction and observed passing regression/full
acceptance/ordinary CI remain root's next gates. The author's local tree preserves
held helper159 from the earlier helper assignment; that correction is unrelated to
these two metadata/quota assertions and is NOT authority to integrate it early.
Only the additive test diff and this note are handed off from this branch.
