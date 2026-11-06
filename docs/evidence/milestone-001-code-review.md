# MILESTONE-001 independent source review

Reviewer `/root/session_supplier_author`, 6 November2026, different from production
author `/root/session_plan_review` (`milestone_author`). Isolated branch
`task/MILESTONE-001-independent-code-review`, worktree
`/tmp/subathon-milestone001-code-review`, starts at actual authored candidate
`a5d18ae`. Only this review note changes. **Actual source approved; ordinary-CI
promotion and its later selected CI receipt remain separate, not yet reviewed.**

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
