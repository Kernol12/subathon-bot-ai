# STORAGETEST-016 — separate setup correction review

Reviewer: bundle_reviewer, 2026-11-03. **APPROVED** actual author correction
`da588312228515c0943735d33e61a02dd514a47b` and final author note `3aa286f`.
Test blob `816441205e4a2ff5c2b69dee535818ad648b1e40` becomes
`0cce9545c3a3851b25df9131e6e35e97366aacee`; actual note blob is
`1a65cc9e6f61a7c6a69f012a1b596c4165f89537`. Read the complete correction diff,
updated [author evidence](storagetest-016-author.md), diagnostic generator/generated
C++/CMake and actual build/run/discovery logs. Prior [full review](storagetest-016-review.md)
remains valid for unchanged tests; this separate review establishes the revision.
Root `e567141` contains those actual source/note blobs and unchanged Qt CMake
`ea1e3cabc0de800d05ee8c8aa8b08f116c233cd4` / SQLite workflow
`b9529c189b33399bd895ca7fdfcee8dd57c1330a`; Git objects independently checked.

The earlier full static review missed two genuine test setup defects. The first
coordinator execution at clean `af310e7d68dfe6c239719bcdf69569a06c4ba526` does
NOT establish the complete intended red. Independently read original LastTest:
35 invocation summaries,70 init/cleanup PASS,265 scenario FAIL,0skip; exactly261
unique HOLD-MISSING rows and FOUR HOLD-ASSERT rows. The verbose/outputOnFailure
stdout repeats output; its522 marker occurrences/70 summaries are not invocation
counts. Preserve the original attempt and261 reached markers, not a false full red.

Specification-based reasons, confirmed against the unchanged physical contracts:

- DB-01/H11 retains complete original records when adding a resolution. Step5's
  `subathon_app_operations` has operation/receipt identity; `effect_count` is on
  `subathon_app_results`. BOTH snapshots now select those same four real fields
  through their full `(operation_scope,operation_ref)` relationship, ordered by
  qualified operation ref. Before/after equality and every other assertion remain.
- An incoming existing-but-inconsistent retained context must reach the specified
  source_mismatch check. Step1 context order is reference,document,configuration_ref;
  the old positional values supplied a forbidden empty known config reference.
  Explicit named columns now store BLOB other, non-null empty document, BLOB other,
  after configother exists. This creates the intended valid contradictory source,
  preserving source_mismatch and actual whole-unit rollback requirements.

These are SQL-only corrections, not changed requirements or weaker expected results.
The35 slots,265 data scenarios, all assertions, fixture/helper, headerfe50/CPP5b10,
API46c, root harness and every previous baseline are unchanged. Six hold entries
remain unconditional named stubs. No production behavior is authorized by this fix.

Actual author diagnostic inspected, not rerun by the reviewer: its generator
extracts BOTH source observer queries and BOTH context inserts, and links ONLY the
OLD bundle library/QtTest. Native Qt6.12/QSQLITE3.54 reports two independent literal
four-field rows for each timer label: correction receipt8/count1 and original
receipt7/count2. Both exact query spellings agree. Typed context other/empty/other,
actual receipt cfg/ctx mismatch and FK0 are checked. No HoldStore/factory/feature
body call occurs. Native diagnostic26-step build and corrected feature compile
have0warnings; -functions/show-only retain35 slots/global264. This validates setup,
not hold behavior. Reviewer ran only source/log inspection, hashes and diff checks.

Selected actual artifacts inspected (SHA256):

| Artifact | SHA256 |
| --- | --- |
| Original FIRST-red CTest |4c2dd57f6f6480f3c6bd21bf957e737daedd5de281a81180d61af49ea7907be0 |
| Original FIRST-red LastTest |6d4398039fafa4bdf0823d058a3667f5685e440b785245cdc809e7909dfa19fa |
| /tmp/hold016-setup-native-generate.py |610229888b3b0d17cc238fe6eb78ad1590980c5f72d718b42c1c0bd656166ee7 |
| /tmp/hold016-setup-native.cpp |2379a090b1a474c90cd57592b17a26d286170a43c03c03ca6b50e69bc20d8bb0 |
| /tmp/hold016-setup-native-run.txt |a755bee5c7e8c7c1a1d76c37140373f4ef9ed90936d2e4ad014661f3f7cb9c6f |
| /tmp/hold016-feature-setup-correction-functions.txt |94056a922e885d2c658367be53890097a6d8ca043fa83d2252bc4ce5fcaf965f |

Original logs are retained in the coordinator's initial-attempt evidence. No feature
or first-red replay was performed here. The corrected complete candidate's OWN
selected CI, then coordinator-recorded revised full intended red and attribution,
remain required BEFORE production. No future run, source implementation, service
policy/authority/provider/platform or release capability is approved by this review.
