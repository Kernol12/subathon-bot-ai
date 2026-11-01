# STORAGE-013 independent code and configuration review

Reviewer `/root/storage_work_code_reviewer`, different from implementation author
`/root/debt_code_reviewer`, 2026-11-01. Assigned isolated worktree
`local-evidence/worktrees/storage013-review`, branch `task/STORAGE-013-review`,
from claim4d10d6115eb31a6b4efca299e61cdc8897e24b47 after normal PR35 merge
a97e048b198a2293b58a25bd09d92518e95b0da5. Owned only this new review evidence.

Verdict: **approved actual implementation and committed CI promotion**, with no
unresolved finding. This approves the source/configuration inspected below;
actual selected official-Qt automatic acceptance and latest-head publication
gates remain pending. Earlier baseline CI/red is preserved, never transferred
as implementation proof or repeated on unchanged stubs.

## Actual artifacts and scope

Read the scoped instructions, brief, logical/API and physical contracts, unchanged
ReceiptRepository/Transaction/ReceiptIntake and core values, retention/work-session
interfaces and implementation, actual frozen tests/fixtures and completed baseline
handoff/reviews. Read the complete new source end to end beyond green assertions.
Author candidate d815b928134710400e16461fb69846cacebf66de was integrated as
root458546ec0ae4902ebec11408dc98ac01dbcdd0da. Author evidence1304280 was read
and integrated as f8a13ae; it truthfully separates local results and remaining gates.
Actual coordinated source/config review HEAD is
9098eca1453321a1f5a6f42bcb7d58bb4373d46d.

| Artifact | Reviewed Git blob |
| --- | --- |
| New receipt implementation | af095d97c255182c79d00a4b574e124f75734aad |
| Frozen public API | b43f9b6026498af18df22c2f695e76b23cb4064a |
| Frozen receipt22 | dab97946568e09f15119a8995735539eb2e14c4d |
| Frozen green3 | 8189f0274a38bb840c612673e0f858fe8d1777eb |
| Frozen independent JSON | a79e5e71a3b9ca101441852c7fb7f26f735e9626 |
| Frozen logical/API contract | e1ed975875e8b62d77d8efff67cfe5e8d5c95038 |
| Frozen physical contract | 2ef4f1384c5c95c1bef5728b095ff5f4e7877e6a |
| Root CMake promotion | a52b81fb1716252f12f3938c5058394133203c84 |
| Preset promotion | 6126906eb93707ca0265d591a41ae326a8c3c237 |
| Qt registration promotion | 559292247747fba8d090b209f39469012c198662 |
| Workflow promotion | fa26cdbe41febf6c07ca8fb18aafa344f2b181c8 |

All frozen artifacts match merged main. Existing writer023451987e40a9330fef91c6c0955f0e527f14e6,
migration4ee2365f8dc6f1189d0f7bf02a090596a9ad3c6f and
retentionc96dae927f652088d2326e427740975d18ba4df2 remain unchanged, as do old
core/service/ports, reviewed assertions and retained prefix fixture. Author changed
only the new adapter source and own evidence; root owns shared integration files.

## Source findings and reasoning

The pure codec performs only unsigned uint64 shifts/byte accumulation, preserves
all0..MAX values and rejects null/wrong-length buffers. Persisted domain ID0 is
rejected separately. No signed/REAL/text numeric conversion is introduced. The
manifest copies the immutable prefix factory, appends exact version2/ID/four SQL
values and complete13-object projection, and declares starts0/1/2. Independently
joined all complete C++ SQL literals and compared them directly with the physical
contract fences and frozen JSON; all four are byte-identical. Sorting type/name
is sufficient for these fixed unique ASCII object names; generated indexes retain
NULL SQL. Actual literal tests verify complete payload/digests and unchanged prefix.

Input/options precedence, explicit byte lengths, nonnull empty BLOB bindings,
optional NULL versus present-empty groups and owned results match the contract.
Selected value/typeof/length triples check row shapes, strict BLOB classes and
ID lengths/domain, required references, uint32 ranges, signed64 timestamps,
dispositions and group completeness. Limits measure bytes. Every selected arrival
resolves through the unchanged retention facade with the same reference/value
bounds. Missing candidates differ from corrupt selected targets; well-typed larger
documents map result_limit. No context-known-config equality, fact normalization,
classification, global audit or repair is invented. ReceiptIntake retains policy.

Allocator validation requires the exact singleton row/type, NULL iff no originals,
and otherwise equality to the strictly decoded raw greatest selected ID. MAX is
checked before unsigned increment, with no reset/gap search or committed-ID reuse.
Find uses representable count+1 lookahead and returns complete ordered history or
fails; it never truncates. Append uses its separate COUNT/MIN/MAX and bad-type/
positivity aggregate, without applying attempts_per_find as a write quota. Under
the required unique positive INTEGER ordinal key, count=N/min=1/max=N proves
contiguity. It checks maximum==INT64_MAX before maximum+1. This is a mathematical
source guard; no claim of constructing MAX contiguous rows follows from the
single-MAX corruption fixture. Find's loop count is bounded below that signed
maximum by checked materialization limits.

Repository options precede owner and local single-use checks. Owner checks precede
mutable pending/consumed state; actual session calls enforce weak-ticket owner,
lifetime and phase. Local already_begun leaves the first unit usable. The disarmed
wrapper is allocated before BEGIN/adoption transfer; attaching a weak ticket and
returning unique_ptr cannot allocate. Context mode consumes the real begin attempt;
adoption transfers only after successful construction, with no second BEGIN/probe.
The wrapper owns copied session/options/thread identity/pending state independently
of repository lifetime, and destruction uses only existing noexcept cleanup.

Create checks targets/allocator, reserves pending capacity and prepares BOTH owned
write statements before mutation. Its generic mutation guard cleans the same
session if an unexpected exception crosses execution after a write. Reserved
uint64 push cannot allocate; append erases pending only after real successful
INSERT with trivial nonallocating erase. Thus caught allocation failure cannot
leave an untracked original available for commit. This is inspected exception
safety, not injected OOM evidence. Prior speculative state and every SQL/retention
query still consume the same unchanged writer budgets.

Incomplete commit uses actual session.rollback before its known failure, so an
expired/terminal/unknown ticket cannot become a false rollback claim via inactive
cleanup. Ordinary commit is the actual synchronous session.commit. Known busy/
SQL/resource/result failures map only from writer categories that already establish
not_started/rollback; local selected failures establish cleanup first. Retention's
already-known selected failures map accurately. Actual invalid-statement/thread/
stale/terminal/protocol errors remain SqliteWorkFailure. Unknown commit/cleanup
overrides local reasons and never derives from ReceiptStorageError or causes replay.
No raw handle, second connection/writer, nested wait, retry or budget reset exists.

## Independent actual checks and CI review

Linux x86-64, GNU16.2.1, CMake4.4.5/Ninja1.13.2 with installed Qt6.12. Captures are
local `/tmp/storage013-review-*` and this isolated build directory. The author's
runtime evidence records hostSQLite3.54.0; this local run is not selected bundled
CI3.53.4 or another-platform proof.

| Command/check actually performed | Result |
| --- | --- |
| `cmake --preset sqlite-receipt-debug` | Exit0; fresh configure/generation at source458546e |
| `cmake --build --preset sqlite-receipt-debug` | Exit0; all56 steps/new source and executables compile/link, zero warnings |
| `ctest --preset sqlite-receipt-debug -L sqlite-receipt --output-on-failure` | Exit0; unchanged22/22 actual behavior passes,0.38s |
| `ctest --preset sqlite-receipt-debug --output-on-failure` | Exit0; full118/118 passes,2.56s, including previous stable96 and all three green executables |
| CTest JSON/compiled `-functions`/actual-label guard | Exact118 total,22 receipt/16 retention/24 migration/24 work/25 writer; compiled22/green3 match |
| Source/frozen-artifact and direct physical-SQL comparison scripts | Pass; no oracle regenerated from the UUT |
| `clang-format --dry-run --Werror server/src/sqlite_receipts.cpp` | Exit0 |
| `python3 tools/check_project.py`, `python3 tools/progress.py render --check`, `git diff --check` | Exit0;391 tasks/122 decisions/current HTML |

An initial registration script assumed an old-writer name prefix; corrected it
to the actual existing label and the complete guard passed without artifact edits.
No baseline stub-red was rerun. Source/results above were already final af095.

Read the actual pending four-file promotion, then merged9098eca and verified all
four committed hashes/diff. It removes the optional manual input/condition and
runs unchanged receipt22 unconditionally after required green3 and exact compiled
registration. Pins, permissions, job name, preset flags and old stable lanes stay
unchanged. No failure inversion, ignored executed failures or assertion changes.
Descriptions/comment change and draft labels are removed. Reconfigured at9098eca
and compared all118 registrations: same names/commands/properties, except draft
removal on22. No broader native repetition was needed for that configuration-only
change. Root separately reports actionlint passing; this reviewer inspected the
actual workflow but did not independently invoke actionlint, absent from PATH.

Actual automatic selected official-Qt unconditional22 CI must still be observed
on the published implementation, then current completion metadata reviewed and
latest-head CI/discussions/rules/normal merge gates satisfied. Application bundles,
downstream contribution/plan codecs and rates, evaluator/provider meaning,
backup/update/recovery, OOM/disk/power-loss, live/platform/UI/renderer/packaging
gates remain separate. No live data, credentials or external operation was used.

## Final completion metadata and actual CI attribution review

Verdict: **approved actual bounded-component completion metadata**, no unresolved
finding. Reviewed root e5128ad8df9e9bf616a2beddae93fcedc5226266 and concrete
metadata corrections ab197291be9fe12f42ad4b92120a374d5c962e8c in the assigned
review worktree. Only this appendix was authored by the reviewer. No native,
receipt, setup or historical-red body was rerun for metadata; no future-head CI,
merge or broader product capability is approved here.

Compared all14 source/API/test/green/vector/contract/configuration/old-production
blobs across actual approval84e2045, executed source-CI head
af6540171a09ccc7d6fe68abd6a218794a2687de and final metadata. Every hash above
remains identical: sourceaf095d97c255182c79d00a4b574e124f75734aad,
APIb43/testdab/green818/JSONa79/logicale1/physical2ef and promotiona52/612/559/fa26.
The integrated39e3177 review is byte-identical to actual84e2045 and is an ancestor
of the CI head. Writer023/migration4ee/retentionc96 and original baseline review/
raw-red files remain unchanged. Fresh `gh pr view 35 --json
state,mergedAt,mergeCommit,headRefOid,url` confirms actual prerequisite merge
a97e048 at18:16:21Z/final8a66a3a; git ancestry proves that merge precedes separate
claim4d10d61 and source authoring. The completed stub red is preserved, not rerun
or converted into implementation evidence.

Read the complete cached metadata/executed log for
[automatic run41337727868](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41337727868)
and independently fetched `gh run view 41337727868 --json
databaseId,headSha,status,conclusion,event,url` plus exactaf65401 check-runs.
Actual event pull_request, completed SUCCESS; all11 checks at that exact source
head are completed/success. Observed both Qt/ICU hashes OK, GNU13.3, new source
compiled at12/56, Qt6.12.0/SQLite3.53.4, old25/work24/migration24/retention16
passing, and three diagnostic summaries9/5/5 passes including init/cleanup with
zero failed/skipped. Complete22 compiled registration matches. Parsed all22 exact
frozen receipt names in the successful **unconditional** acceptance step:22/22
PASS in0.61s. The frozen workflow has no former manual receipt input/condition.
This is actual implementation execution, beyond earlier fixture/API CI. Its job
completion18:40:10Z precedes the recorded component completion metadata. Host
GNU16.2.1/SQLite3.54 evidence and local22/full118 results stay separately attributed;
CI's executed lanes are not mislabeled a full118 CTest run.

Read state, DB-01 map, architecture, strategy, brief, CI, handoff and checkpoint.
All391 task IDs/dependencies are unchanged; only STORAGE-013 task data changed
since approval84. It is done as this bounded implementation component, with actual
acceptance/regression/code_review/github_ci passed. The initial required-key
validation issue was corrected to acceptance/regression metadata using already
observed results; no checker/test/source was weakened. Broad platform/domain/
bundle/recovery/provider verification remains not_run, DB-01 stays partial and
STORAGETEST-014 stays planned until real integration. Classification stays in
ReceiptIntake; domainReceiptId1..MAX, purecodec0..MAX and downstream actual numeric/
rate requirements remain. No full server/application, provider/platform/live,
recovery, browser, OOM or power-loss proof is claimed.

Actual ab197291 corrections resolve the review's metadata findings: DB-01 marks
its old stub/no-pass sentence as historical baseline evidence; strategy identifies
the preserved historical red command and current unconditional CI; checkpoint
names committed completion metadata and active final review/latest checks pending;
the inaccurate nonessential source line count is removed. These actual edits
were inspected before approval and change no reviewed behavior/configuration.

Independent inline Python hash/ancestry/state/map and full CI-log parsing passed;
the initial log-parser tab-column assumption was corrected against the actual
capture without changing artifacts. `python3 tools/check_project.py` passes391
tasks/122 decisions; `python3 tools/progress.py render --check` and
`git diff --check` pass. Root's tooling19/DOM simulation/format/actionlint reports
remain attributed to root; no actual browser validation was performed here.
Fresh latest-head CI, ready PR36, resolved discussions, current rules/required
approvals, normal exact-head merge and refreshed main remain root publication
gates before any dependent claim. Historicalaf65401 success is not transferred
to this metadata/review commit or a future head.
