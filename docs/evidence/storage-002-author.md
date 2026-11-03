# STORAGE-002 implementation author handoff

Author: bundle_implementer, isolated task/STORAGE-002-implementation, 2026-11-03.
Integrator owns progress/HTML/build/workflow and publication. Scope is the reviewed
[concrete bundle revision1](../contracts/sqlite-application-bundle-v1.md) and
[physical migration5](../contracts/sqlite-application-bundle-schema-v1.md), preserving
all prior ports/schema/tests/fixtures. Production source changes only bundle.cpp and
the separately delegated values.hpp. No current-provider or semantic authority is inferred.

All seven former stubs are implemented. Internal ephemeral access borrows the
actual caller ticket; it never adopts/commits/destructor-cleans. Strict row decoding
uses SQL typeof/length and QVariant classes, existing fulluint64/canonical Exact
codecs, immutable-prefix5 factory, real B08 rollback and original unknown/protocol
failures. Pure shape/count/conservation/byte validation precedes ticket access.
Whole-stage statements/bindings are prepared before writes, known faults abort the
entire caller unit, and head advancement is last under exact expected token.

Selected plan metadata and historical siblings/progress use finite64/65 keyset
pages without materialization caps or growing group maps. Full lookup validates all
selected bodies; narrower reversal source checks declared sibling metadata while
leaving unselected job documents/effect Exact bodies outside its certification.
Current contribution patch changes only application; historical identity lookup
avoids comparing historical snapshots with today's state.

## Actual checks so far (candidate, not independent approval)

- `cmake --preset sqlite-bundle-debug`: PASS using system GNU16.2.1/C++20,
  Qt6.12.0, Boost1.92; runtime diagnostics identify hostSQLite3.54.0.
- First compilation found missing receipt-codec include spelling and a QVariant
  int64 conversion ambiguity; corrected to existing plan uint64 codec and explicit
  qint64. No missing dependency or compilation failure counted as intended red.
- `cmake --build --preset sqlite-bundle-debug --parallel 2`: PASS, ZERO warnings
  after source correction/clang-format; source changes then rebuilt with3 steps.
- `ctest --preset sqlite-bundle-debug -L '^sqlite-bundle$'`: original32 unchanged,
  first run29PASS/3FAIL, then31PASS/1FAIL (2.16s). Two own defects corrected:
  operation_conflict returns no existing payload; full lookup item count excludes
  plan DTO vectors which are validated internally but not returned.
- Remaining original case borrowedStoreLifetimeAndCapabilityErrorsStayTruthful
  reaches actual rolled_back/session_closed after deliberate rollback then terminal
  read. Its line3264 expects not_committed, contrary to existing writer sticky
  protocol contract/tests. Integrator and separate reviewer are handling a written
  one-assertion correction; author did not alter baseline or writer behavior.
- Debugger skill availability gate found no CLion router. Standalone GDB fallback
  after ptrace escalation inspected first and second clean_rollback frames: first
  rolled_back/uncommitted, second rolled_back/session_closed, no input/storage/
  synchronous/problem payload. No production logging or fixture changes.

Actual raw configure/build/32 outputs retained in this worktree's ignored
local-evidence/storage002. Historical reviewed/red baseline remains unchanged and
was not repeated as preparation. Fresh source review/full206/official selected-Qt
CI and latest-head merge gates are still pending. Separate helper824 one-byte
returned-support-count finding has a held correction; author has wired actual
returned byte counts but deliberately not integrated that correction before the
separately reviewed additive regression's reached-red gate.

All integrations/FX/current application policy/provider delivery/refund allocation,
recovery/backup/UI/platform/release checks remain outside this component proof.

## Author self-review before separate source verdict

Added actual selected-scope cardinality cross-checks to all positive keyset walks
(state balances, effect/job/transition rows, full component count and basis progress)
so an extra corrupt nonpositive/orphan row cannot hide outside visited keys while
declared counts omit it. Narrow source snapshots also require common configuration/
format identity. Complete reads apply returned aggregate quotas; pure input_items/
owned-input quotas do not become unrelated read quotas. Current-only plan resolution
checks complete scalar history before returning stale, without decoding stale body
payloads; historical fixed-record resolution still loads the older selected revision.

Actual rebuild7 passes ZERO warnings; unchanged original32 now31PASS/1 unchanged
sticky terminal-session oracleFAIL,2.13s. An intermediate author typo applied the
current-only projection to historical fixed records and the65-history case failed;
corrected that call to historical selection and reran after this real code change.
No assertion/fixture/old component changes. Separate source verdict remains pending.

## Source reviewer follow-up

Different reviewer requested checked returned-vector sums and signed64 metadata
component counters, plus preconstruction of the final `SELECT changes()` statement
before own writes. These explicit arithmetic/preallocation corrections preserve
all existing tests and are included in this source revision. Actual build9 passes
ZERO warnings; original32=31PASS/only known terminal-session baselineFAIL,2.16s.
Full preset at clean93f75ef had205/206PASS7.02s with that same single known assertion.

At root's request, the author also inspected the independently added returned-byte
regression on clean root e6bc06d, using GDB fallback because no CLion router is
available. At exact test2159 (after the1105-byte public read), observed
limits.owned_bytes1105, storage=result_limit, work=work_exception,
transaction=rolled_back, absent value, empty problem. The stack identifies the
supportAnchorConditionsUseExactReturnedByteBoundary lambda→scenario→case→Qt
metacall. This attributes the preserved separately-reviewed regression's actual
first-red to held helper824's extra Conditions byte. No raw GDB logfile was
retained; tool output records the exact fields/stack. No source/fixture/log injection
or baseline preparation rerun occurred. Root controls subsequent correction159
release and the separate strengthened metadata/input-quota regression gates.

Final bounded preallocation guard, agreed by different reviewer before authoring:
state balance actualCOUNT must equal declared count before materialization/cap;
operation effect/component/job/transition/reversal actualCOUNT must equal declared
result counters before full child-vector allocation. Narrow source metadata keeps
its64/65 traversal and no historical materialization quotas. Existing postwalk
structural comparisons/cumulative writer budgets remain; no reset or failure mask.
Build10 ZERO warnings; original32=31PASS/only knownsticky baselineFAIL2.19s.
Source is now frozen for integrator's reviewed correction/regression integration
and final actual different full-source verdict; no additional author changes pending.
