# Coordinator checkpoint

Owner authorization (2026-11-10): finish API-012 through independent reviews,
acceptance and normal protected merge, then STOP. APITEST-001, API-001 and actual
session-clock binding are not authorized. The complete release scope remains required.

## Current state

- Primary branch: `task/API-012-native-websocket-profile`. Implementation and its
  native/code/hosted reviews are complete; final publication gates remain below.
  Local completion metadata follows d554934. Remote PR head is 6f719a7, with main
  still 642c9a0. Inspect actual refs and PR state before acting on this checkpoint.
- [PR110](https://github.com/Kernol12/subathon-bot-ai/pull/110) is draft, linked and
  watched. All 21 check entries (20 names, duplicate scaffold) passed at 6f719a7.
  The last inspected PR has zero reviews/comments/threads. Do not relink or rewatch
  on wakes. Unwatch before the final handback.
- [API-012 state](state.json) records the completed component, separate approved
  review, passing acceptance and a readable note under 500 characters. This does
  not claim that the PR has already merged. Final packet/current-head publication
  evidence belongs in PR110; no unrun check is marked passed.
- Coordinator `codex:27731ff9-5bb1-4e9f-b2f3-b78e37ae011e` owns integration and merge.
  All source/fixture/CI authors and stage reviewers have completed their handoffs.
  No old session is assumed active. Historical worktrees and receipts are preserved.

## Valid approvals and actual proof

Detailed raw receipts/reports are under ignored `local-evidence/API-012/` and linked
from PR110. Earlier findings and exact assignments remain in git history.

- Source/resource/protocol: candidate4 at 2cc999c approved after B1–B7, M1–M3 and H1
  corrections. Production cpp, header, patches and manifest have not changed since.
  Fixed regions and an M-byte slot pool establish nonfragmenting bounds beyond what
  snapshots alone prove. Pins and all 16 build options are unchanged.
- SDK/CMake/compiled ABI: 82bf92c approved. The distinct local candidate4 SDK has
  161 sealed names, seal SHA8c82448f. Preserve pristine stock sources/SDK and the
  one-time seal; do not rerun the one-off sealing script. Trantor is BSD three-clause;
  older MIT labels are incorrect. Framework/ORM/project/selected package notices stay.
- Tests/fixture: all 44 method bytes and assertions remain identical to d9ac991.
  Original38/stock WS02 red and six supplemental missing-profile reds remain distinct.
  First corrected native run passed 43/44; independent review approved a genuine
  sampling defect, not a production defect. The per-request IO observation fix at
  150c280 was independently approved; original failure/diagnostic are preserved.
- Local fixture-only rebuild: main SHAe6c93ba4, unchanged native implementation
  object/archive, SDK and 16-library closure/ABI requirements. Controlled complete
  18-object map matched. Revised native44 passed all methods with no setup/capture/
  cleanup faults or skips and all 1,563 inputs unchanged.
- CI recipe: exact a28dc4f independently approved. Absolute configured CTest and
  passive symbol/version/export/RTTI admission precede the scrubbed identity load.
  Actual Ubuntu run [42890430204](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42890430204)
  passed at 6f719a7: 172 sealed SDK names, exact complete 14-object map (main, vDSO,
  12 libraries), ordinary CTest native44 with zero skips, and three infrastructure
  nodes including tooling50. CTest is /usr/local/bin/ctest; actual suite Python is
  /usr/bin/python3.12. Raw logs, seal, map and normalized receipts are preserved.
- Independent hosted-results/native runtime/code review APPROVED d554934, retaining
  every unchanged prior approval. Report `hosted-native-review.md`; reviewer:
  <t3-thread://v2/2a8cf5b0-ea0f-4760-972f-17effee4968b/thread:delegated-task:command%3Amcp%3A9c70731a-4f93-4378-90dd-4932b6f36dc4%3Adelegate-task%3Abc359916-b82d-4426-a0c2-94a6af66795a>.
  Runner-local passive-abi.json was not archived; the passing fail-closed gate is
  evidence that checks ran, not an independent Ubuntu binary re-audit. No readlink
  proof is claimed for /usr/bin/python3 versus /usr/bin/python3.12.

## Final gates and stop

1. Independently review the exact final completion packet and proposed PR body.
2. Push only the reviewed metadata; immediately link the full PR URL after pushing.
   Update the PR body/review receipts and take it out of draft after approval.
3. Let every applicable latest-head check pass, including actual native44 through
   the unchanged admitted recipe. Recheck head/base, live protection, reviews and
   unresolved discussions. Any material source/recipe/fixture change needs review.
4. Merge normally with an exact head guard, never bypass protection. Refresh main.
   Record actual merge/check evidence in PR110 and ignored local receipts, unwatch,
   and STOP. If PR110 is already merged when resumed, verify it and stop; do not
   select another task or create a follow-up metadata PR.

Source residuals remain documented: unreachable released-buffer append-zero UB,
bounded pending-Upgrade CPU spin, off-owner legacy shutdown, global-factory/IO-join
lifetime, conservative lease charging, one pending output frame and half-open idle
retention. Actual host/clock/auth/sync, TLS/Caddy/proxy routing, platforms, performance
and distribution remain separate gates. No live provider/chat/payment effects were used.
