# Coordinator checkpoint — STORAGE-011 publication gates

Coordinator `/root`, 2026-11-01. Continue the full specification v0.45 standing
loop under AGENTS.md and COORDINATOR_PROMPT.md. No owner decision or access
is needed. Full product remains incomplete.

## Integrated tasks and actual main

Last observed main is `ef29c87c433fb18af3747805961ce87a826b3929` (PR31);
all ten postmerge workflows were observed successful. Recent completed PRs:

| Task | Merged PR | Durable evidence |
| --- | --- | --- |
| CORE-005 | [27](https://github.com/Kernol12/subathon-bot-ai/pull/27), 1259cfb | [Handoff](core-005-handoff.md) |
| CONTRACT-004 | [28](https://github.com/Kernol12/subathon-bot-ai/pull/28), e374788 | [Handoff](contract-004-handoff.md) |
| STORAGETEST-010 | [29](https://github.com/Kernol12/subathon-bot-ai/pull/29), c60e1ed | [Handoff](storagetest-010-handoff.md) |
| STORAGE-010 | [30](https://github.com/Kernol12/subathon-bot-ai/pull/30), 88e86c1 | [Handoff](storage-010-handoff.md), source023451 |
| STORAGETEST-011 | [31](https://github.com/Kernol12/subathon-bot-ai/pull/31), ef29c87 | [Handoff](storagetest-011-handoff.md) |

Earlier core/accounting/writer contracts, reviews and red evidence remain valid
in their linked task records. Do not repeat completed preparation or original red.
Preserve existing refs, reviews and local-only historical checkpoints.

## Active task, ownership and exact evidence

STORAGE-011, root branch `task/STORAGE-011-migration-catalog`, from refreshed
main ef29c87. Scope is the internal codec/catalog/whole-prefix runner using the
existing private writer context/session only. No domain adapter/schema, new
connection, writer API/allowlist change or operational authorization is supplied.
[Task handoff](storage-011-handoff.md), [implementation](storage-011-implementation.md),
[review](storage-011-code-review.md), [CI assessment](storage-011-ci.md).

- Implementer `/root/debt_code_reviewer`: isolated
  `local-evidence/worktrees/storage011-implementation`, branch
  `task/STORAGE-011-implementation`; owns only sqlite_migrations.cpp and own
  implementation evidence. Original9eb1fef/d91c7d integratede183527; evidence
  0ac896c integrated0118fea. Original22/old49/full76 passed.
- Different reviewer `/root/storage_work_code_reviewer`: isolated
  `local-evidence/worktrees/storage011-review`, branch `task/STORAGE-011-review`;
  owns only code-review evidence. Actual source review20cc636/root2fb5f89 found
  valid initial U+FEFF rejected by Qt fromUtf8 BOM consumption, despite green76.
- Separate regression author `/root/writer_transaction_reviewer`: isolated
  `local-evidence/worktrees/storage011-unicode-regression`, branch
  `task/STORAGE-011-unicode-regression`; owns only additive migration test and
  regression evidence. Author234c2ec/root36aca32 adds95 lines/two slots,
  testblobe94befdc. Original22 bodies and ten vectors preserved. Author completed
  clean, no support push. Root8534118 owns shared two-name CMake registration.
- Root owns shared CMake/presets/workflow/state/map/HTML/checkpoint/schema order.

Original baseline: contractcbdea1a/header02f83ce/tests a85a6c05/stub6c507/
greenprobe832fe616/vectors12280c8f, actual fullapproval01f0975/rootcb3c1b2
BEFORE genuine22 stub-red(exit8/zero skips), after actual officialQt green7.
Final baseline approvale1be8ae retained; see PR31 evidence. No original red rerun.

Additive actual testapprovalad4f54d integrated8c70e6e BEFORE root selected2 red
on d91c7d/e94befdc, exit8 with both invalid_manifest reason2; raw stored fixture,
init/cleanup pass, zero skips. Durable1482636: [regression](storage-011-unicode-regression.md),
[raw red](storage-011-unicode-red-ctest.txt). Independent full638-byte payload/
SHA baf9a94e...775b7d reproduced without UUT. Then source author was explicitly
authorized to correct the source.

Correction071af3b integrated2cb0d1b, sourceblob
4ee2365f8dc6f1189d0f7bf02a090596a9ad3c6f. It removes only the faulty decoder
equality after strict Unicode-scalar validation; encoded length and exact raw
stored-byte comparison remain. Root actual build/full78 pass (new24+old49+
infrastructure5 including green7), format/diff/invariant checks pass. Different
actual corrected-source/config approval854cace is integrated790afc0 after
independent new24/full78 passes. Implementation is done, PR still unmerged.
Author final evidence639650d is integrated as41c02ea. Root build/test commands finished successfully;
logs ignored under local-evidence/coordinator/storage011-fixed-*.log.

## Publication, checks and next action

Shared66be523 promotes migration cases to unconditional automatic CI after
mandatory green7 and old49; exact compiled discovery and selected Qt/ICU/action
pins remain. Dynamic registration includes the two new cases:24 migration and
78 total. Actionlint, Python tooling19, DOM simulation, project391/currentHTML
and whitespace checks pass. Actual automatic Ubuntu acceptance is still pending;
host SQLite3.54.0 is distinct from bundled3.53.4. No actual browser claim.

[Draft PR32](https://github.com/Kernol12/subathon-bot-ai/pull/32) is published at
head da7cbde. Actual selected officialQt Ubuntu run41310940283 and all11 checks
at that head passed. Hashes/GNU13.3/Qt6.12SQLite3.53.4/old49/green7/exact24/real24
were read from actual logs; see CI evidence. Root final CI/map/state/HTML metadata
records implementation done, publication still pending. Next obtain final metadata/CI attribution review
and verify latest-head all checks/discussions/current rules before normal merge.
Then refresh main and start dependency-ready STORAGETEST-012 from integrated011.
Ruleset26170263 was rechecked: required PR/resolved threads, zero GitHub approvals,
no bypass actors/force/delete; no required-status-check rule. Written CI gates
remain mandatory; repository evidence does not replace required GitHub approvals.

Last full local audit before the additive author wrote:53 existing worktrees clean,
16 historical absent /tmp worktree references preserved (no pruning). Host scoped
process audit found five CLion processes, no native build/test command then.
Current implementer/reviewer may have active commands/owned edits; inspect actual
state on resume. No process was killed, no work discarded. Root is clean after
this checkpoint commit; no active root command.

All seven integrations, three rate providers, application/domain stores and
bundle, backup/update/recovery, renderers/UI/locales/themes, packaging/platform
and live/OBS verification remain full agreed scope. OPS-02 requires actual
operator initiation, restorable backup, compatibility information and maintenance
exclusion; no boolean or this runner establishes those. No live credentials,
supporter data, provider chat/payment or deployment side effects were used.
