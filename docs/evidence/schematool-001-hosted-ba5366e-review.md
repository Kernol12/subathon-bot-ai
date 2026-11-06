# SCHEMATOOL-001 — actual ba5366e hosted results

Reviewer C independently queried GitHub run/job/commit APIs and downloaded both
complete logs for published `ba5366e9ce1fe7726f10d91ff00ea4d74a46b175`.
Fresh isolated note-only branch; no local test, checker, SDK or native execution.
**SDK foundations accepted for this head. Native hosted acceptance was NOT run:
both jobs stopped at the actual static comparison classification failure.**

Both workflows checked out merge41a5321fab4a72582a03ac8b27b0e55a619d94ec,
parents095d192c/ba5366e. Independent GitHub git-commit queries confirm its tree
`c34cf487c3158355bd59bcd0359dce2225cf22de` equals the published head's tree.
The actual source therefore matches the reviewed head, not a previous receipt.

[Library run42304624540](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42304624540)
is actual SUCCESS, attempt1, updated22:25:59Z. Both jobs and every step succeed:

| Job | Actual environment | Actual observed SDK methods |
| --- | --- | --- |
| 128190707651 | Ubuntu22.04 / CPython3.10.12 | 14 OK / .239s |
| 128190708025 | Ubuntu24.04 / CPython3.12.3 | 14 OK / .119s |

I independently matched all14 actual method names in each full log to the frozen
library source. Hash-only/binary-only isolated installation gives the reviewed20
packages, bootstrap cleanup and pip check complete, and no-site harness verifies
actual cp310/cp312 SDK projection plus twenty same-artifact metadata identities.
The harness registers64 wrapper definitions only; it runs no wrapper feature body.
No SDK failure/error/skip occurred. Full independently downloaded library log is
120857 bytes/SHA256
`4a5fc69bfad4ca1593d34d8af1d6a468cc20b946768397f0c9c80ee5212362e2`.

[Native run42304624411](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42304624411)
is actual FAILURE. Both hosts compile and pass preceding8/8/5 literal controls,
then the production owner audit rejects exact `cmp %r11,%rsp` at fixed_rsp261:
`RuntimeError: unbounded/unknown generated stack-pointer write`.

| Job | Actual recorded compiler/libc/kernel | Final job outcome |
| --- | --- | --- |
| 128190707206 | Ubuntu22.04 GNU11.4.0 / glibc2.35 / 6.8.0-1070-azure | failure22:26:19Z |
| 128190707589 | Ubuntu24.04 GNU13.3.0 / glibc2.39 / 7.0.0-1017-azure | failure22:26:18Z |

API step conclusions and full logs independently agree: fault static controls/
audit, exact bootstrap, native discovery/full56, semantic inventory/full66 are
skipped after that failed prerequisite. No candidate native ELF/owner/kernel/
acceptance stage executes; no native or whole-feature capability is inferred.
A pipeline step skipped by a failed gate is recorded as unexecuted, not a passing
or skipped feature test. Full native log196595 bytes/SHA256
`c4305e83a2a974c292622ec831867ad87da2dc8574d869e2fb98cf8b1014274c`.

CMP's read-only register operands explain the actual guard defect; the later
separately reviewed control/red/correction are different evidence and do not
retroactively make this run green. A corrected current head still needs actual
both-host static/bootstrap/foundation/full66 plus SDK14 outcomes and all other
latest-check/review/metadata/normal-merge gates. No task completion or waiver.

Actually performed: full logs and exact SDK-name/count/host/stage/checkout/hash
analysis, API conclusions/tree equality, known credential-prefix scan0 (checkout
token masked), project426/122/render/diff PASS. Raw downloads stay in reviewer
/tmp paths for root's preservation/privacy workflow; only this review note is
owned here. No GitHub mutation, local replay or candidate execution occurred.
