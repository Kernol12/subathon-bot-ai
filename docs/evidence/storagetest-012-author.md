# STORAGETEST-012 — retained context contract authoring

Author: `/root/writer_transaction_reviewer`, 2026-11-01. Different reviewer:
`/root/storage_work_code_reviewer`. Initial authorization: contract/evidence only.
Branch `task/STORAGETEST-012-author`, isolated worktree
`local-evidence/worktrees/storagetest012-author`, from claim
`036fad8d9c2da414ef92e69496b32311d0ade9b7` after merged PR32/main29f3818.
Own only [retained-context revision 1](../contracts/retained-context-v1.md) and
this note. Root owns briefs, actual schema/migration ordering, builds/workflows,
state/HTML and checkpoint. No support push/PR is authorized to this author.

## Scope, existing types and decisions

Read root/server/tests AGENTS, STORAGE-012 brief, specification §§3/6/7/DB-01,
event/state boundary, actual receipt/contribution ports and application-plan types/
contracts. Existing `ReceiptArrival.configuration_ref`/`modifier_context_ref` and
rule/modifier version refs are opaque std::string. No numeric configuration/context
version-key type exists. ReceiptId/ContributionId and CapturedPlanRevision have
actual uint64 fields in their later persistence slices.

Proposed minimal contract:

- Two immutable document kinds, each keyed by exact version-specific opaque bytes;
  BLOB storage avoids text/number normalization and Qt initial-BOM binding traps.
- Required present opaque document bytes, with present empty BLOB distinct from
  missing/null; actual SQLite typeof/length controls read interpretation.
- At most one optional typed context-to-configuration link. Missing stays absent;
  present targets must resolve in the configuration namespace, even when staged
  earlier in the same session. No guessed current values or unknown facts.
- Exact duplicate bytes plus link presence/value are idempotent; changed reuse
  conflicts without mutation. Primitive domain refusals do not write or claim
  earlier caller writes rolled back. The caller owns whole-command commit/abort.
- Existing active SqliteWorkSession composition, real commit/terminal publication,
  no independent connection/transaction/automatic migration or new writer API.
- Two small proposed BLOB tables with uniqueness and optional foreign key; root
  must accept actual DDL/order/fixtures before test freeze. No domain doc schema,
  evaluator, wire format, provider facts or authorization fields are invented.

The brief's uint64/codec language is reconciled explicitly, not removed. This
slice reviews/proves the exact BLOB byte codec and full-range decimal/binary
representations. It introduces no unused numeric ID/helper. Typed lossless numeric
codecs and zero policies remain gates in STORAGE-013/014 for actual ReceiptId/
ContributionId and STORAGE-015 for actual plan revisions. Root agreed to update
brief cross-references after actual contract review. Preserving numeric-looking
bytes here does not claim a uint64-to-SQL conversion was implemented or verified.

Twelve literal future examples cover empty/binary/BOM/full-range bytes, exact
duplicate/conflict, optional/missing/wrong-kind links, historical isolation,
same-session atomic caller composition, bounds, reopen and inherited uncertainty.
They are not authored executable tests or observed red.

## Initial contract verification and remaining gates

Inspected baseline source blobs: migrations
`4ee2365f8dc6f1189d0f7bf02a090596a9ad3c6f`, controlled writer
`023451987e40a9330fef91c6c0955f0e527f14e6`, migration contract
`cbdea1a48d2989a9c8bd17edf0539bdf7c1bce61`. Preserve their valid reviews and
24 migration/49 writer-work/seven environment cases. No completed run/red was
repeated. No source/header/test/fixture/schema/build/workflow file changed here.

Documentation checks actually run in this author worktree:

| Command/check | Actual result |
| --- | --- |
| `python3 tools/check_project.py` | Exit0; 391 records/122 decisions, links/presets/current HTML valid |
| `python3 tools/progress.py render --check` | Exit0; current root-owned snapshot |
| `git diff --check` | Exit0 |
| `git status --short` | Only the two owned Markdown paths untracked before commit |

The author normally refreshed shared root revision `3f40af2` before validation.
Its [existing-writer probe evidence](storagetest-012-probe.md) records actual host
Qt6.12/SQLite3.54 empty-BLOB/null/binary distinctions through unchanged session
SQL. This author read that evidence but did not independently rerun the probe;
it is not new retention, official-bundle or platform acceptance. The contract
keeps null input, present empty BLOB and absent optional reference separate and
requires actual stored typeof/length checks.

No new native build, test body, schema, baseline, feature behavior, live provider
or platform verification was claimed in that contract-only phase. Existing writer null/empty ownership behavior
is a compatibility boundary, not something this document silently changes.

Different-agent actual contract review and integrator acceptance of the proposed
metadata format come first. Only then may root assign minimal interfaces/stubs/
tests, followed by independent actual test review, compilation/intended red,
implementation, code review and applicable real acceptance/CI. Full application,
backup/recovery, formats/resolvers/evaluators, all integrations/rate providers and
release-platform gates remain mandatory. No user decision or access is identified.

## Minimal API/stub stage after actual contract approval

Independent contract review40d0dc6 approved actual root proposal791cacf/contract
`a884f01693ceda555779909e907d3c31bf19b6ef`, integratededdbe2b. Root explicitly
expanded this author's scope to new `sqlite_retained_context.hpp` and
`sqlite_retained_context.cpp`, plus precision in the existing contract/note. No
Qt case, environment probe, vector, schema, CMake or state file is authored here.

Normal refresh had one add/add conflict because root integrated the initial new
contract through a separate commit. The author retained root's exact approved
a884f016 and normally merged both histories in cf147a7; no unfinished content or
review was replaced.

The concrete header declares kind-scoped byte keys, a strong configuration-link
value, required document bytes, limits, find/retain and the migration factory.
The facade copies only the revocable session/limits. Its value constructor and
destructor do no SQL/cleanup/commit; actual caller ownership is preserved.
All three new source functions are explicit feature_not_implemented throws.
There is no validation, binding/lookup, result decoding, retention, migration
manifest construction or transaction algorithm in the stub.

Contract/header precision pins:

- Pure validation order: positive options, key kind, key bytes, retain payload,
  retain link. Exact error categories precede stale/foreign capability errors
  without any SQL/private-state probe. They do not abort earlier caller writes.
- Existing selected data and stored known target are validated before comparison;
  healthy existing-row conflict precedes resolution of a new proposed target.
  Domain conflict/missing make no own writes or cleanup.
- Only selected rows/known targets are checked. Malformed selected data/dangling
  stored links use invalid_stored_data after known active rollback; well-typed
  larger selected fields use result_limit, not corruption/missing. Uncertain
  cleanup overrides as the unchanged SqliteWorkFailure uncertain_outcome.
- Loaded typeof(blob)/length0 becomes a non-null empty owned QByteArray. The
  returned record can be retained again as already_present. SQL NULL/absence and
  inherited SQL/capability/budget failures retain their exact distinctions.

Proposed next-stage cases are not yet authored: frozen manifest, exact byte/reopen,
empty-read/idempotence, full-range representations, namespaces, duplicate/conflict,
optional/missing/wrong-kind links, historical isolation, caller commit/rollback,
input precedence, exact bounds/selected limits, malformed selected rows, capability
lifetimes and uncertainty. Focused driver probes can cover empty/null/BLOB binding,
the approved two-table/index/FK shape and selected optional/empty read facts through
existing SQL only. Root owns their eventual sqlite-retained-debug harness.

Actual API-stage checks:

| Command/check | Actual result |
| --- | --- |
| `clang-format -i` on the two new C++ files | Completed |
| Python argument-vector `c++ -std=c++20 -fPIC -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Iserver/include <pkg-config Qt6Core/Qt6Sql cflags> -fsyntax-only server/src/sqlite_retained_context.cpp` | Exit0; header/stub syntax only, no link/execution; manual non-system Qt includes emitted one Qt qbitarray.h conversion warning |
| `python3 tools/check_project.py` | Exit0; 391 records/122 decisions, links/presets/current HTML valid |
| `python3 tools/progress.py render --check` | Exit0; current |
| `git diff --check` | Exit0 |

Root owns actual CMake target/preset compilation after integration; syntax checking
does not establish feature acceptance, intended red or a new test baseline.
These artifacts still require different-agent actual API/precision review before
root expands test/probe/vector ownership. No future body, red, implementation or
platform result is transferred from the earlier contract approval.

## Actual 16-case / three-environment / literal-vector candidate

After actual different-agent API/stub/precision/shared-harness approval46fc8ee
(root integrated2d0289b), this author normally refreshed root in f16745e. Root
expanded ownership only to the two new Qt sources, JSON fixture and this note.
The approved contract5b0ee84, header55c82815 and explicit stub b27eb488 are
unchanged. No production retention algorithm, new schema source, core port,
existing assertion, build registration, CI or progress record is authored here.

The committed candidate is **authored but not compiled or executed yet**. Root
explicitly permitted this bounded handoff before adding the real shared testcase
registration, so compilation/discovery can overlap a different actual artifact
review. No missing dependency/compiler/init/skip result is a red baseline.
None of the 16 new feature bodies has run. The three environment bodies likewise
have not run in this author stage; they link/use only existing writer/QtSQL and
contain no retention header/factory/store calls. Selected official Qt green
fixture proof and actual different full baseline review still precede new red.

The independent JSON freezes exact migration1/retain-config-context-v1, starts
[0,1], two SQL statements in approved order and all seven objects, including the
three NULL-SQL generated indexes. A standalone Python stdlib reference constructed
ASCII decimal-length F frames and unsigned UTF-8 tuple order from literal contract
DDL, then hashlib.sha256; it never called a production factory, codec or runner.
Its complete payload is **2000 bytes**, SHA-256
`0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e`.
Full payload hex and exact expected projection are durable JSON, not only a hash.
The manifest feature case compares actual factory data and existing codec output
to these literals. Fixture setup uses only independent literal SQL/BLOB/history
values; it never calls the new stub factory during init.

| Case | Contract oracles |
| --- | --- |
| manifestMatchesApprovedRetentionMigration | Exact version/ID/start policy/SQL/order/projection, full literal F payload and digest |
| retainsExactBytesAndResolvesAfterReopen | RC01/03: binary/NUL/BOM bytes, input mutation after synchronous return, physical commit before body success, explicit missing, reopened exact records |
| presentEmptyRoundTripsAndRetainsAsDuplicate | RC02: present empty BLOB normalizes to non-null owned QByteArray, re-retains as duplicate, distinct missing |
| opaqueReferencesAndFullRangeBytesStayDistinct | RC03/10: twelve exact decimal/binary vectors, zero through uint64 max as opaque spellings, distinct 1/01, NUL/FF/BOM and raw SQLtypeof/hex |
| sameBytesRemainSeparateAcrossNamespaces | RC05: identical key bytes in two kinds remain separate |
| exactDuplicateAndConflictDoNotMutate | RC04: idempotence, payload conflict, complete raw snapshot unchanged |
| optionalConfigurationLinkPresenceIsImmutable | RC04/06: absent/present/retarget/removal conflict, exact known-link duplicate, new missing target cannot mask healthy existing conflict, absence across reopen |
| missingAndWrongNamespaceLinksCannotCreateContext | RC05: missing/wrong-kind targets cannot create orphan; subsequent config write proves healthy transaction |
| newVersionsPreserveHistoricalReferences | RC07: v2 insert preserves v1 config/context bytes and exact old link |
| sameSessionStagesConfigurationAndContext | RC08: same-session uncommitted link resolution, observer invisibility, facade destruction does not commit/rollback, caller's real commit visible before return |
| callerOwnsCommitRollbackAndConflictPolicy | RC08/09: conflict preserves prior write; explicit caller rollback versus diagnostic commit; uncaught caller failure abandons staged config+context together |
| invalidInputPrecedencePreservesHealthySession | Ordered options/kind/ref/payload/link errors; stale preflight precedence, null versus empty, no undo of prior healthy retain |
| configuredBoundsAndSelectedLimitsAreExact | RC11: exact 4096/262144 defaults and 4/3 configured bounds, one-over rejection, selected valid larger record is result_limit and synchronous rollback lock release; literal 1048577-byte SQL result after proved prior retain rolls back whole work |
| selectedInvalidRowsAndDanglingLinksAbort | Selected wrong document type/dangling stored link/bad known target abort; conflict cannot mask corruption; lock probe proves cleanup before throw; unrelated corrupt record does not imply global audit |
| ownedResultsAndCapabilityFailuresRemainSafe | Owned binary result after writer destruction; valid foreign/stale/terminal calls keep inherited errors; invalid input wins foreign/stale; caught postcommit protocol error retains committed physical outcome and no normal payload |
| sqlFailureAndUnknownOutcomeAreNotSuccess | RC12: known SQL error after retain rolls back; existing physical commit-ack seam persists data but reports unknown/no payload; rollback-ack loss during corrupt-read cleanup overrides local reason with uncertainty and physically rolls back staged write |

The many byte vectors use a generous explicit statement allowance to avoid
inventing an internal query-count plan. The resource-failure oracle uses a literal
result larger than the existing default budget after a proved retain, not a
calibrated implementation cost or a reset session. Prior exact session-boundary
writer assertions remain untouched. The physical-ack seam is the already reviewed
one-shot existing-writer seam after actual SQL control; it neither fabricates a
retention result nor claims power-loss/disk-failure coverage.

Green environment cases are:
`emptyNullAndOpaqueBindingsRemainDistinct` (actual bound null/nonnull-empty/binary
SQLtype/length/hex), `retentionMigrationShapeAndLinksUseExistingSql` (literal seven
objects, SQLNULL indexes, enabled FK/valid known target/orphan SQL rejection), and
`selectedEmptyAndOptionalValuesKeepTheirSqlTypes` (literal selected BLOB-empty,
optional NULL, BOM/NUL/FF bytes and SQL rejection of requiredNULL/wrong-type/empty
known-link). They deliberately do not assert driver empty-buffer nullness: stored
SQL type and length establish presence; adapter normalization is a feature oracle.

Actual author-stage checks: clang-format completed on the two new sources;
`python3 tools/check_project.py` passed (391 records/122 decisions),
`python3 tools/progress.py render --check` passed and `git diff --check` passed.
These are documentary/format checks only. Root owns actual CMake attachment,
Qt compilation, exact16/3 discovery, green existing73+green7 compatibility and
selected official-Qt environment results. Any compiler/test defect requires an
explicit separate revision; no current check is presented as new acceptance.

Typed uint64 SQL codecs/zero policies still belong to the actual downstream
ReceiptId/ContributionId/CapturedPlanRevision slices. Opaque full-range vectors
here do not certify numeric SQL conversion. Configuration schemas/resolvers,
backup/recovery/update authority, evaluators, providers and release-platform
verification remain outside this bounded baseline and remain required later.
