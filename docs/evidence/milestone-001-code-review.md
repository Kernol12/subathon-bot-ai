# MILESTONE-001 independent source review

Reviewer `/root/session_supplier_author`, 6 November2026, different from production
author `/root/session_plan_review` (`milestone_author`). Isolated branch
`task/MILESTONE-001-independent-code-review`, worktree
`/tmp/subathon-milestone001-code-review`, starts at actual authored candidate
`a5d18ae`. Only this review note changes. **Actual source and ordinary-CI
configuration and exact selected hosted CI receipt approved; later completion
metadata/current-head merge gates remain separate.**

## Actual scope and frozen artifacts

Read root/core instructions, complete logical/API contracts, public header, all
730lines/53 tests and literal vectors, actual independent baseline reviews and
historical FIRST-red attribution. No completed FIRST red was replayed.
Specification v0.45 §8/MILE-01/NUM-04 remains authoritative. Reviewed exact blobs:

| Artifact | Git blob |
| --- | --- |
| Actual production source | `d79b77cad26bf876fd4bd89038161d4fc0c1b809` |
| Frozen public header | `9a1678137ba6f85cbfbb545630f08d36a261790b` |
| Frozen test source | `019973844dceb289aa82ec8aabd3d9274faef98e` |
| Frozen literal fixture | `409be19d57b2cba4025e236a67a539af63b08be4` |
| Logical contract | `0115a5cc3cdc4fb1ebb8c9739716a27cb3379933` |
| API contract | `58b8e129815c0525176cc6c1b6830a7e6f0a9fdb` |

Source addition implements the sole existing raw-input boundary, not action
delivery, authoritative score/history selection, a queue/store, manual/edit/reset
authority or platform capability. No new dependency, public ABI, assertions,
fixtures, arithmetic limits or release requirement was changed by the author.

## Audit beyond passing assertions

All five options precede counts/items, all reference ceilings precede total input
bytes, and both precede empty refs/duplicate/order/prefix/output phases. Full
depth-first reference passes retain embedded NUL/high bytes. No duplicate hash
index or input/output ownership is built before resource/reference preflight.
Duplicate detection preserves target order with one fixed4096-slot array of
borrowed string_views. Because every ID is already nonempty, empty slots are
unambiguous. Before each of at most4096 insertions, at most4095 slots are occupied;
linear probing therefore reaches a free slot or the exact duplicate. Collision
patterns cannot produce a full-table infinite loop. Scratch is4096 borrowed
records plus fixed scalars, within the API's record bound, without owning keys.
No global CPU/time guarantee follows from this bounded table.

Current target ordering uses comparison; prefix identity validation permits
legitimate captured targets/revisions/scores/contexts that differ from today's
definitions and manual completions below thresholds. Advancement appends every
consecutive target<=actual score and captures that SAME supplied score for each.
Old records remain identical. Empty/before-first/between/all-completed states,
current deficits (last, not sum), previous/next, absent first interval and exact
threshold transition agree with the contracts. Refund/recovery cannot reappend
the retained prefix. No unauthorized history reconstruction is attempted.

Every nonnegative distance has a proven high>=low comparison before converting
BOTH signed operands to uint64 and subtracting modulo2^64, giving the full
mathematical0..UINT64_MAX distance without signed overflow or negating INT64_MIN.
Between targets, next>previous and score<next ensure span>0 and earned<span.
Unsigned std::gcd gives bounded reduced components, including earned0→0/1.
Decimal digit counting allocates no string and counts20-digit uint64 extremes.
Only after complete output preflight does a returned Exact receive its<=64-bit
components; no floating point, second rounding or arbitrary input Exact exists.

ByteBudget maintains used<=ceiling and checks amount<=ceiling-used; input/output
walks count every contract field and real repeated copy. Output projection includes
unchanged/new completion records, suffix16/state1, all current deficit targets,
previous/next copies, track deficit/remaining and interval scalars+canonical digits.
Both complete output count and item/ref/byte obligations precede ANY result
string/vector/Exact construction or reserve. No capped prefix escapes. Exceptions
and bad_alloc leave caller/prior results untouched; partially built local state is
RAII-owned and never published. Result transfer moves its owning value; it does not
retain input views or an extra target/history copy.

No uncovered semantic defect requiring a new regression was found in this audit.
The existing53 independently authored/reviewed tests remain unchanged and were
meaningful for these outcomes. A future newly discovered defect still requires
separate regression review→actual behavior-red→bounded fix; green existing tests
are not a waiver. Minor stale public-header implementation-status wording was
reported to root; this reviewer did not alter the frozen header.

## Independent fresh native checks

Fresh Linux GNU16.2.1/CMake4.4.5/Ninja/Python3.14.9, installed GoogleTest1.18.0 and
authentic selected Boost1.83 headers at `/tmp/subathon-boost-1.83-sdk/prefix/usr/include`.
Actual commands in reviewer worktree:

```sh
cmake --preset milestone-progression-debug -DSUBATHON_FETCH_GOOGLETEST=ON -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset milestone-progression-debug
ctest --preset milestone-progression-debug --show-only=json-v1
ctest --preset milestone-progression-debug --no-tests=error --output-on-failure --output-junit /tmp/subathon-milestone001-independent-full.xml
clang-format --dry-run --Werror core/src/milestone_progression.cpp
python3 tools/check_project.py
git diff --check
```

All exit0. Fresh18-step build has no compiler warning. Discovery exactly90total/
53milestone, all enabled with timeout30 and no inversion. Full90/90 PASS0.75s:
all unchanged53 milestone cases0.09s, old33numeric, three infrastructure and one
reviewed25fact SDK foundation; zero failures/disabled/skipped. No label exclusion
was used despite the author's historical intentional-red metadata. Byte-safe Python
subprocess capture retains stdout/stderr and direct return codes under
`/tmp/subathon-milestone001-independent-{discovery,full}.*`. Native JUnit SHA256
`f0ffd12c88d9e4562d18c68616153c3910e4203f946cd6099eb3dfbb384e284e` independently
parses tests90/failures0/disabled0/skipped0. Formatting/whitespace/consistency pass
422tasks/122decisions/currentlinks/HTML.

Approval is exact a5 source and passing pure-component acceptance. Root still owns
ordinary CMake/workflow promotion, observed selected Ubuntu/current-head checks,
completion metadata/state/map/HTML, applicable independent metadata review and
normal GitHub rule/discussion/merge gates. All application/authority/history/
actions/persistence/UI/provider/FX/platform/OBS/package/security/recovery/full
release obligations remain separate. Only synthetic data; no live side effect.

## Actual ordinary-acceptance configuration review

Same independent reviewer, NEW isolated branch
`task/MILESTONE-001-ordinary-config-review`, worktree
`/tmp/subathon-milestone001-config-review`, based on actual combined
`b945ff20f85f2af579efd4b6a62b9eda64cfcf8c` (root promotion2792791 plus imported
source review). Reviewed ALL four promotion files, surrounding CMake registration,
source/test/fixture preservation and actual inline workflow guard. **Approved.**

| Actual reviewed artifact | Git blob |
| --- | --- |
| Milestone workflow | `48f6c9a4c73ef2d5186efaff7170158d8b139c0b` |
| Presets | `f347131262262ba67da191da49ed8550dcfa21b2` |
| Exact suite label include | `af26db7b60aa96a6b3ae93ad219ba8f4181ca2e0` |
| Public header, comment-only correction | `1f5e1c5c62a8f82c95216352626bfb1e10b92cf5` |

Preset display name and stale public-header status comment now match implemented
pure proposal behavior; no signature/type/default/ABI or assertion changed.
Discovery retains original raw33ordinary+20namedMP cases and timeout30. Labels
become milestone-progression/acceptance/core; historical intentional-red is removed
only for this now-passing implemented suite. Inline guard requires the exact source
33+20inventory, required ordinary labels, zero intentional-red, enabled/uninverted
cases and the unchanged independent foundation. No assertion/fixture was removed
or weakened and no failure skip/inversion/continue-on-error was added.

PR/main triggers, stable job milestone-progression-build, checkout digest/read-only
permissions, Ubuntu24.04/selectedBoost1.83 package/config/macro checks, GTest input
and limits remain unchanged. Old37/foundation execute with exact feature-label
exclusion, then all53 feature bodies execute in a separate UNCONDITIONAL step
selected by exact label. Green build/discovery no longer substitutes for acceptance.
Actual branch rules/required-check configuration and later hosted results remain
root's gates, not facts implied by workflow authoring.

Independent fresh same selected1.83 configure/build18steps PASS, no compiler warning.
Actual workflow inventory Python block was extracted verbatim and executed: PASS
`Discovered 53 enabled milestone cases; all will execute as ordinary acceptance`.
Ran BOTH literal promoted CTest commands with verbose output and additional JUnit:

```sh
ctest --preset milestone-progression-debug -LE '^milestone-progression$' --no-tests=error --verbose --output-junit /tmp/subathon-milestone001-independent-promoted-old.xml
ctest --preset milestone-progression-debug -L '^milestone-progression$' --no-tests=error --verbose --output-junit /tmp/subathon-milestone001-independent-promoted-feature.xml
```

Old37/37 PASS0.63s and all53/53 feature PASS0.09s, both exit0 and zero native JUnit
failures/disabled/skipped. Byte-safe stdout/stderr retained beside those JUnits.
SHA256 old`50e3d3df797302982729f1c4bfd9f7e38b3a64b8b494ff248d66f421e1a81681`,
feature`e15d6076c10ebf2211d9d806e0a7213a455a93c4596fcde81945454a39de845b`.
Project consistency/whitespace PASS422/122/currentHTML. No hosted CI run is claimed
by these local commands; actual selected receipt/final metadata/latest-head merge
gates remain pending.

## Independent actual selected hosted-CI receipt

Same different reviewer, NEW isolated receipt branch
`task/MILESTONE-001-selected-ci-review`, worktree `/tmp/subathon-milestone001-ci-review`,
from actual published5492ac6e7958c602f9be8b71251bbfe3f0c2c2a7. **Approved exact
[run42168586251/job127730814851](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42168586251/job/127730814851).**
Read-only independent `gh api` run/job queries report PR73, same exact head,
pull_request event, stable milestone-progression-build job, attempt1, completed
SUCCESS. Run updated05:28:48Z6November2026; job completed05:28:47Z. Every ordinary
foundation/feature guard/execution step succeeded.

Downloaded the full OWN log via `gh run view 42168586251 --job 127730814851 --log`
to `/tmp/milestone001-independent-selected-ci.txt`:2130lines/397748bytes/SHA256
`8c6df279882fc72711793b6832d32f067025baa55d52daff77d18b9229225aaa`, independently
equal to root's stated receipt. Source d79, workflow48f6, presetsf347, labelsaf26,
comment-only header1f5 and original tests0199/fixture409 remain exact approved blobs.

Actions actually checks out test merge1661d6256cbab17a5c35489e08c72634ed79eba1,
with parents main e2cd5eb and published5492ac6. Independent GitHub Git-commit API
queries verify that merge and published head have the SAME full tree
1691df9486fe29fd9badbc1472442cf21cf07199; no unseen merge resolution is attributed
to the source approval. Log shows pinned checkout, Ubuntu24.04, GNU13.3.0,
selected Boost1.83 package/config and actual BOOST_VERSION108300, fresh26-step
compile/link including real feature test source, with zero compiler warning.

Actual compiled inventory guard prints enabled53 and ordinary-acceptance intent.
Old/foundation lane executes37unique passing tests, independent25literal
std/Boost facts/no project link, exit0/all37PASS1.28s. Separate UNCONDITIONAL feature
lane executes53unique cases, all native Passed/OK, zero skipped/not-run/failed,
exit0/all53PASS0.16s. Independently parsed the full per-case log names and compared
its53-set against the unchanged33ordinary+20namedMP source inventory: exact equality.
Required ordinary labels remain visible and no intentional-red execution exclusion,
failure inversion or blanket continue-on-error hides this lane.

Only read-only Git/blob/log/metadata parsing and document consistency/whitespace
checks were performed for this receipt; no red or native suite was repeated.
This approves the actual hosted component proof, not an unseen completion commit,
later latest-head CI/rules/discussions or normal merge. No broader platform/live/
application authority/atomic state or full-release claim follows.

## Independent actual completion-metadata review

Same reviewer, different from coordinator/metadata author `/root`. NEW isolated
branch `task/MILESTONE-001-final-metadata-review`, worktree
`/tmp/subathon-milestone001-final-review`, initially actual7ca796d and advanced only
to actual correction52618d167b008a30dd5a84ce77991138b244ac41 before approval.
**APPROVED exact526 completion metadata; later publication/merge gates separate.**

| Actual completion artifact | Git blob |
| --- | --- |
| Task state | `0c202b5201f3b4f73d97427911bf07d3343c4cf6` |
| Acceptance map | `4e46a3d5d95bb189f731d6e4d2c556e42ff321e5` |
| Generated progress | `aee0c4e39a534c393e13d921551e72be58bd20d8` |
| Corrected checkpoint | `9b35645da0c7f727d640c8f9b02fb98361894846` |
| Corrected handoff | `3c8b3bf0d1e9d988675be54d29c1a6642cef475f` |
| Actual selected CI receipt | `ef2af1583e5067432a7223e5b5dd55544e85bf8c` |
| Root native receipt | `d7515a2aa313228404464a07cec988b894a18df9` |

Parsed actual main0605948 and candidate preserve EXACT423task IDs/1624dependency
edges, with ONLY MILESTONE-001's task row changed. Main activity is an exact prefix.
All other tasks, including supplier001done/transport002planned and the completed
MILESTONETEST001 baseline, remain identical to main. The active parallel transport
claimdf1 is explicitly preserved on its separate task branch, rather than silently
overwritten or falsely imported into this milestone snapshot.

MILESTONE-001 done has actual unchanged53/full90 acceptance, different exactsource/
resource/config reviews and independently downloaded actual ordinary selectedCI,
with source/config/selected_ci/acceptance passed and platform_release not_run.
Current GitHub publication/merge remains explicitly separate. Source d79, workflow
48f6, labelsaf26, presetsf347, comment-only header1f5, tests0199/fixture409 and both
contracts remain the exact approved blobs after main's documentation integration.
No assertion, fixture, historical FIRST red or dependency gate changed.

Map changes affect ONLY MILE-01/NUM-04, both still partial. Existing test lists are
exactly retained; prior evidence lists are exact prefixes. NUM04's complete prior
scope remains before its additive milestone description; scenario rows are EXACTLY
unchanged. New architecture/strategy/brief/handoff/native/CI descriptions distinguish
this pure component from authoritative score/history/application/actions/manual/
edit/reset/UI/provider/platform/full-release behavior. All broader scope remains.

Review found two stale resume sentences in7ca (PR72 still needed integration and
002 could be newly claimed). Actual526 changes ONLY those two evidence documents:
PR72/main060 already integrated; preserve existing002claim and continue assigned
roles. This correction removes duplicate-claim directions without changing any
source/config/test/state/result or earlier valid review. Both original and corrected
revisions were inspected; this approval covers the corrected actual526, not unseen
future prose or commits.

Read-only parsed Git/JSON/blob checks, `python3 tools/check_project.py`,
`python3 tools/progress.py render --check`, all19 tooling unittests,
`node tools/check_progress_ui.mjs` and `git diff --check` PASS423/122/currentHTML.
DOM simulation is not actual browser/OBS verification. No build/native suite or
FIRST red was repeated for metadata. Latest publication-head applicableCI,
current rules/required approvals/resolved discussions/GitHub-permitted normal merge
remain root's gates. No owner action or release-completion claim follows.
