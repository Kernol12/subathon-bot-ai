# STORAGETEST-017 — independent final evidence review

Reviewer: bundle_reviewer, 2026-11-03. **APPROVED: actual first-red attribution
and test-baseline completion metadata**, candidate6757b7d9b1351d53f92f9ab3e319e7e127ba5262
plus inspected clarificationf856f7f64680a639e3a1dfff10a62371c2f4bcc7.
The earlier [full test/harness review](storagetest-017-review.md)f5c6dc1 remains
valid. This review ran no native suite or reader call and did not replay first red.

## Actual official CI and chronology

Read the complete recorded official SQL log, completed run-result JSON and final
PR check rollup. [Run41602589680](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41602589680)
is completed/SUCCESS for reviewed headc24fb6eb4d7c4bb666b1dba3ff077a54014dd865,
08:20:48–08:22:39Z. The checkout log identifies PR merge07caccce6f2ab93c583614a94c39d8bdefbee6ae,
combining that exact head with main21c087154638ffcdc03663696b568a4bf454462d.
Final rollup contains11 COMPLETED/SUCCESS checks at c24. Earlier cached run/status
snapshots are intermediate in-progress observations, not completion evidence.

Actual log confirms fresh100 build steps, ZERO build warnings/errors, GNU13.3.0,
compiler-selected BOOST_VERSION108300, hash-verified official Qt6.12 archives,
actual SQLite3.53.4, old stable suites including unconditional35 bundle cases,
required NEW three-slot foundation guard and5PASS/0FAIL/0SKIP/0blacklisted88ms,
and exact18 compiled/registered new cases. New feature bodies were not run there.

Only afterwards, the preserved FIRST native invocation ran at SAME clean c24:
`ctest --preset sqlite-original-claim-debug -L '^sqlite-original-claim$'`,
08:24:35.551068–08:24:36.110621Z. Actual exit8/0.55s,18/18 CTest cases failed.
Independently parsed raw output and original LastTest agree, including exact row
sequence: all18 reviewed compiled methods appear;14 plain rows plus6 selector,
9 closure,3 byte-bound and3 Exact rows give35 unique failures, each with precisely
`SQLite original-claim lookup is intentionally unimplemented; review tests first`.
All36 init/cleanup results PASS;0skip/blacklist/crash/setup failure. No missing or
duplicate scenario row. The foreign-thread and uncertainty tests retain the named
marker instead of passing through later owner cleanup or an invented unknown.

These observations prove setup reached each scenario's FIRST new-reader call.
Assertions after the immediate stub remain unexecuted; their passing behavior is
an implementation acceptance obligation. The intended red is the missing query,
not compilation, dependency, SQL fixture setup or a skipped case. No old or first
red was rerun by this review.

| Preserved actual artifact | SHA256 |
| --- | --- |
| Complete official SQL log |80ab1c952321792a8419f1dc82c716610fc43d6345d0cab449abd5faf6bbe341 |
| Completed SQL result JSON |b5bec405400ab115d08ef733063c7eb746b7294d2ac5b73991b91da3489d1c71 |
| Final c24 PR check rollup JSON |456784772f3008758cb34b456fc55049a8123d0d41730d8bb65e899ad42effef |
| Original first-red raw, including tracked copy |90f3c739a735c13ca4eddecf0055ce7322cf1247e6dd52bc030e0956b1ee19b0 |
| Original native LastTest log |821b3d3d46bd372cc570056d0db8bccda4e8324212cd1bd4ee7eb50218b9ffa5 |

The tracked docs/evidence/storagetest-017-red-ctest.txt is byte-identical to the
original ignored local-evidence record, not a reconstructed or replacement run.

## Actual completion metadata and unchanged boundary

Read all eight completion-file changes: CI chronology, handoff, coordinator
checkpoint, original raw red, progress JSON, generated HTML, acceptance map and
test strategy. Only STORAGETEST-017 changes status in_review→done. Its feature/
product acceptance explicitly say observed_red; STORAGE-017, hold016 and service
baseline remain planned, with service depending on both integrated prerequisites.
DB-01 and NUM-04 remain partial. Baseline DONE/UNMERGED and latest completion-head
CI/discussions/rules/normal merge still pending are explicit; c24 CI is not claimed
to verify a future metadata head. The unchanged source remains an immediate stub.

One actual review finding was corrected in f856: the CI lead had still said no
observed red. It now states current completed c24 CI/red and labels earlier stage
sections historical; both CI and handoff explicitly distinguish marker reach from
unexecuted downstream assertions. Earlier valid reviews and original evidence are
retained. All unfinished branches/configuration copies remain preserved.

Critical approved Git blobs remain exactly:

| Artifact | Git blob |
| --- | --- |
| Public header |cf721e7f61f10dacfb7d078e1545a2d403e0a4df |
| Source with immediate new-query stub |130e3db62b421479ec3d0a25ce2e712480482f0e |
| Final feature tests |0e4f4f006d2a0e161857e38255cac487f7de83a9 |
| Qt CMake registration |c35ccf28cf378a0634491fec4e9dc7fef7561e42 |
| SQLite workflow |7fec9ade311c9928d019ad465b19d686e7f861b0 |
| Corrected CI chronology / handoff |d49958f4d0a3e4de9374e0419fca107218349ca9 /c63d01a2b80f8c578b3b7b9a4b32e54cf99891a5 |
| Progress JSON / acceptance map |561080afcb4b792d8829a8ff4b2e18b33dbe5049 /22894fdff67541a9bdf6415426274f7b7e6f749a |

Actual git diff c24→f856 is empty for production/server, fixtures, Qt tests/build,
workflows and root CMake/presets. Root f856 is clean. Read-only static checks at
that actual candidate: check_project PASS396 records/122 decisions/local links/
presets/current HTML; render --check PASS; git diff --check PASS. Existing review
worktree verification configuration copies were left untouched. Only this new
review Markdown is reviewer-authored.

Approval finishes independent attribution for the baseline evidence. Root still
must publish completion, observe ALL applicable latest-head gates and perform the
normal exact-head merge before claiming implementation017 from refreshed main.
Query implementation requires separate source/config review, green18/full228 and
promoted ordinary selected CI; this review preapproves none of those future
artifacts or outcomes. Hold/service/authority/provider/FX/recovery/UI/platform and
full agreed release remain required. No live effects or owner action are needed.
