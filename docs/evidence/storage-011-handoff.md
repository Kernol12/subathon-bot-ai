# STORAGE-011 — migration catalog implementation handoff

Coordinator `/root`, 2026-11-01. [PR32](https://github.com/Kernol12/subathon-bot-ai/pull/32),
branch `task/STORAGE-011-migration-catalog`, from PR31/main ef29c87. Internal
implementation gates pass; final metadata review/latest-head CI and normal merge
gates remain. Specification v0.45 §16/DB-01/OPS-02; unchanged
[migration catalog revision1](../contracts/migration-catalog-v1.md).

## Result and limits

Explicit inspection validates UTF-8 encoding, raw text, exact reserved/full schema,
well-typed catalog, complete content-hashed history, integrity and foreign keys.
Explicit advancement commits bootstrap and the whole missing prefix in one
existing writer transaction. Refusals/cancellation preserve old state; budgets,
raw WorkResult and unknown-outcome fail-close semantics remain. Pure F-framing,
unsigned UTF-8 sorting, SHA-256 and preflight implement the frozen contract.
Opening a writer never initiates migration. No new dependency/connection, writer
API/SQL allowlist change, domain table or real repository adapter was added.

OPS-02 actual operator initiation, restorable backup, compatibility information
and maintenance exclusion remain trusted caller/integration requirements. No
request field or diagnostic report proves those. Application bundle, backup/
recovery/update orchestration, power-loss, supported release platforms and live
providers remain separate full-scope gates. Synthetic disposable files only.

## Exact test-first, review and source evidence

- [Original baseline](storagetest-011-handoff.md): actual fullapproval01f0975
  integratedcb3c1b2 BEFORE original22 stub-red(exit8/zero skips), after officialQt
  green7 prerequisite. Final baseline approvale1be8ae retained; no red repeated.
- Frozen contractcbdea1a/header02f83ce/ten vectors12280c8f/greenprobe832fe616 and
  writer023451/old49 are unchanged. Original22 test bodies remain byte-identical.
- [Implementation](storage-011-implementation.md): originalsource9eb1fef/d91c7d
  integratede183527 and evidence0ac896c/root0118fea. Original76 passed locally.
- [Different source review](storage-011-code-review.md): requestchanges20cc636/
  root2fb5f89 found valid initial U+FEFF rejected through a BOM-consuming decoder.
  Green76 did not discharge it; no contract narrowing or assertion weakening.
- [Additive regression](storage-011-unicode-regression.md): different author
  234c2ec/root36aca32, testblobe94befdc, adds95 lines/two slots. Actual approval
  ad4f54d/root8c70e6e BEFORE root selected2 intended red at unchangedd91/e94.
  Both fail for invalid_manifest reason2; rawfixture/initcleanup pass, zero skips.
  [Raw red](storage-011-unicode-red-ctest.txt), durable1482636. Independent full
  638-byte literal payload/SHA baf9a94e...775b7d derived without production.
- Only then source correction071af3b/root2cb0d1b, sourceblob
  4ee2365f8dc6f1189d0f7bf02a090596a9ad3c6f, removes the faulty decoder equality
  after strict scalar validation. Encoded-length and raw stored-byte equality
  remain. Corrected author evidence639650d integrated41c02ea.
- Actual corrected source/config approval854cace integrated790afc0, from different
  `/root/storage_work_code_reviewer`, after independent migration24/full78 passes.

## Verification and publication

Author/root/reviewer configure/build pass and full78 CTest pass: migration24,
unchanged writer/work49, infrastructure5 including separate diagnosticgreen7.
Root exact compiled24 equals registered24; total78. Formatting, project391/current
HTML, Python tooling19, DOM simulation, actionlint1.7.12 and whitespace pass.
Local environment: Linux x86-64, GNU16.2.1, Qt6.12.0/SQLite3.54.0, CMake4.4.5,
Ninja1.13.2, Python3.14.8. No actual browser/platform claim.

[Actual automatic Ubuntu run41310940283](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41310940283)
at PR32 da7cbde passes pinned archive hashes/GNU13.3/Qt6.12 and bundled
SQLite3.53.4, Qt probe1, old25+work24, green7(9 including init/cleanup,0skip),
exact migration24 discovery and all24 unconditional cases including U+FEFF.
All11 PR checks at that head succeeded; [CI evidence](storage-011-ci.md).
Later metadata/review revisions require their own final checks.

Root owns shared CMake/presets/workflow/state/map/HTML/checkpoint/schema order.
Implementer `/root/debt_code_reviewer` owns only source/implementation evidence
in storage011-implementation; reviewer owns only review evidence in
storage011-review; regression author `/root/writer_transaction_reviewer` owns
only additive test/evidence in storage011-unicode-regression. Separate branches/
worktrees and all valid historical reviews are preserved. No owner question.

Next actual final metadata/CI-attribution review, latest-head all applicable CI,
resolved discussions/current rules and normal exact-head merge. Then refresh main
and claim dependency-ready STORAGETEST-012. Full agreed product remains incomplete.

## Actual merge outcome

PR32 merged29f3818a7b202896e1122cd7a787a60a6c4e379d at2026-11-01T13:32:15Z.
Final metadata/CI-attribution approvala70e495 integratedc31edca; exact finalhead
c31edca502d7e661904ccfb38999b13960c396d5 had all11 checks SUCCESS, ready/
MERGEABLE/CLEAN, no review threads and current applicable rules verified before
normal exact-head merge. No bypass or test weakening. Main refreshed; next
STORAGETEST-012 is a separate baseline, no automatic implementation authorization.
