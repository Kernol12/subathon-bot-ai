# STORAGE-011 — continuous integration evidence

Coordinator `/root`, 2026-11-01. [Draft PR32](https://github.com/Kernol12/subathon-bot-ai/pull/32)
from main ef29c87. [Original reviewed/red baseline](storagetest-011-handoff.md),
[additive Unicode review/red](storage-011-unicode-regression.md) and
[independent code/config review](storage-011-code-review.md) are separate gates.

## Coverage change

Shared66be523 removes the manual migration-acceptance boolean/draft label and
promotes migration acceptance to unconditional PR/main/manual execution. The
same sqlite-writer-build job runs old25, work24, mandatory selected-driver green7,
exact compiled migration discovery and migration24 (original22 plus two focused
Unicode regressions registered by8534118). Full local preset has78 registrations.
The selected Qt/ICU archives, hashes, pinned checkout, permissions, commands and
complete-registration guards remain. No failure inversion, continue-on-error,
WILL_FAIL, skipped baseline or test/fixture weakening was introduced.

## Actual selected official-Qt result

[Run41310940283](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41310940283)
on PR32 head `da7cbde476020b3718d64a81dfebd140c3289ac8` completed successfully
at2026-11-01T13:23:06Z. Metadata and actual logs were read; ignored log copy:
local-evidence/coordinator/storage011-ci-da7cbde.log. The source is corrected
blob4ee2365f8dc6f1189d0f7bf02a090596a9ad3c6f, testblobe94befdc, contractcbdea1a,
header02f83ce, vectors12280c8f, environment832fe616, oldwriter023451.

| Actual job stage | Result |
| --- | --- |
| Ubuntu24.04 / compiler | GNU13.3.0 |
| Official Qt6.12.0 / bundled QSQLITE | Runtime SQLite3.53.4; distinct from host3.54.0 |
| qtbase/ICU SHA-256 checks | Both OK against unchanged pinned hashes |
| Configure/build sqlite-migrations-debug | Both exit0 |
| Qt event-loop probe | 1/1 pass |
| Writer compiled discovery / real acceptance | Exact25;25/25 pass |
| Work compiled discovery / real acceptance | Exact24;24/24 pass |
| Required diagnostic environment | Exact7 slots;9 including init/cleanup pass,0skip |
| Migration compiled discovery / real unconditional acceptance | Exact24;24/24 pass |
| Both initial-U+FEFF regression cases | Individually listed and passed |

The changed workflow was observed executing actual behavior, including every
original22 migration case and both independently reviewed new regressions. All11
PR check runs at da7cbde were observed SUCCESS/MERGEABLE/CLEAN. This result is
attributed only to that head; later metadata/review commits still require their
own latest-head CI/discussion/current-rule verification before merge.

## Local checks and remaining gates

Author/root build and full78 pass with host Qt6.12.0/SQLite3.54.0/GNU16.2.1,
including all new24, old49 and infrastructure5 (one is the seven-slot diagnostic
executable). Exact24 compiled slots match registration; original22 bodies and
frozen contract/header/ten vectors/green probe/oldwriter49 remain unchanged.
Root independently reproduced the full638-byte Unicode payload and literal SHA
baf9a94e...775b7d without using production outputs. Formatting, actionlint1.7.12,
project391/currentHTML, Python tooling19, DOM simulation and whitespace pass.
Different corrected-source/config approval854cace is integrated790afc0 after
independent new24/full78 passes. Final metadata/publication gates remain pending.

Ruleset26170263 was actually rechecked: PR/resolved threads, zero GitHub approvals,
no bypass actors/force/delete, no required-status-check rule. Adding this workflow
does not configure enforcement; the written all-applicable-latest-head CI gate
still applies. Actual required GitHub approval would not be replaced by repository
agent evidence.

Synthetic disposable state only. OPS-02 actual operator initiation, restorable
backup, compatibility/update orchestration and maintenance exclusion remain caller
requirements. No production domain schema/adapter, application bundle, recovery,
power-loss, actual browser, release-platform or live-provider verification follows.
All seven integrations and three exchange-rate providers retain full release scope.
