# STORAGETEST-002 independent concrete-contract review

Reviewer `/root/contribution_reviewer`, 2026-11-02. **APPROVE the actual logical
and physical DOCUMENT contracts below.** This does not approve a concrete C++
interface, migration source, fixture/test baseline, implementation, future CI or
merge. No such new executable artifact exists in this reviewed scope.

## Actual artifacts and independence

Normally integrated frozen root `4956f56a7965fdf4659599db5bbac95039baee4f`, then
the actual separately authored clarification through root
`035268a242c1809ffff6d56b83756edaf950226f`, into the dedicated clean
`task/STORAGETEST-002-contract-review` worktree. Author revisions were
`404de065f69626324b92ee7c31d5175fe2bd3c88` and
`eba8dc5712b79f17f566daf1992cfe62364d38c5`. Read both entire contracts, their
author evidence, specification §§3.3–3.5/4/8/16 and DB-01/NUM-01–04, the accepted
logical bundle, actual core values and existing writer/retention/receipt/
contribution/plan boundaries. The coordinator retains shared integration ownership;
this reviewer owns only this new evidence file.

| Actual accepted artifact | Git blob |
| --- | --- |
| [Concrete logical/API proposal](../contracts/sqlite-application-bundle-v1.md) | `cf4624bf113b375c8614b4163b83245be67d9f0a` |
| [Exact physical DOCUMENT](../contracts/sqlite-application-bundle-schema-v1.md) | `f13fca6d11e57410a31a7d307a8b12eaa21a68f9` |
| [Author evidence, checked for scope/attribution](storagetest-002-api-author.md) | `b737a1ab6cc594739a8534d98e25e357bf6e521d` |

Accepted logical predecessor `3c8cbd869800f6aa4cd117016ef568f789d3ace9`, plan
logical `3af256cdf82a4737c2fc71a67c88b8b332d6ba71`, physical
`99bf45b46f6c9d361b899fdb975dc2db51da4554`, plan header
`4634d7b57dccc5c559f2ab05a968833b4b925feb` and writer header
`afc9f483da7d8e61a5e73715ca33819216e9e668` remain unchanged. These reviewed
documents introduce no dependency, executable schema/source, build or workflow.

## Findings resolved in actual documents

Early actual-draft review identified the one-basis-per-request restriction and
missing direct-component equality. Final SQL permits MANY original component
links for ONE immutable current occurrence; SB13 retains increments -20/-10 for
the one captured request -30, without synthetic request splitting. Direct role0
now requires its sole score/count component exactly equal requested units. SB14
therefore rejects component -1000 for request/link -20 even if a matching after
balance was supplied. Exact arithmetic and basis bounds remain explicit storage
conservation checks; rounded entitlement and financial correlation stay upstream.

During full review of original logical blob
`6d7e1d5ff3759550fbfb4f1dccc1e0c21eea5e69`, the phrase original-kind “complete
application” conflicted with the underspecified targeted source projection. The
actual replacement above resolves that finding. It enumerates actual sibling
COUNT/key/order/group/kind metadata for each snapshot, effects, components, jobs,
transitions and absent original-operation reversal children. It fully checks the
selected original effect's complete component set, including refundable-Main
debt-repayment0, and its actual selected plan/source. It explicitly does not
certify unselected amounts/rationals/job bodies or whole-operation numeric deltas.
Metadata traverses64-row pages/65 lookahead without applying aggregate read caps
as lifetime quotas. Full lookup retains its separate full selected-record scope.
Direct comparison confirms all14 SB rows and the entire physical document remain
byte-identical through this clarification; earlier diagnostics still apply.

## Boundaries assessed

- One inert borrowed-session port; all reads, duplicates, claims and stage returns
  are provisional. Prior intake/capture COMMIT is an honest caller precondition.
  No owning13/14 wrapper, hidden COMMIT, destructor cleanup or inferred durability.
  Known B08 rollback, unknown acknowledgement/cleanup and post-COMMIT body failure
  remain distinct; publication requires actual normal committed completion.
- Immutable head/advanced-before/after snapshots preserve CAS coverage and forbid
  token rebind/reuse/ABA. Clock-only advancement preserves debt/counters/config/
  format. Exact actual-component deltas apply from advanced-before to after.
  Complete CURRENT contribution comparison is separate; original application
  patches only application and preserves unrelated current dimensions.
- Full/targeted/absent scopes, typed physical source closure, complete plan-history
  structural/quote/dependency audit, selected-body provenance, CID0 versus absence,
  positive full-range receipt/revision fields and historical-state freshness are
  explicit. No support/event receipt equality or authority from semantic strings.
- Intent differs from computed outcome; original subject, source receipt and scoped
  reversal identities cannot be bypassed with another key. Immutable request/
  component identity preserves duplicate action occurrences. Actual positive-credit
  bases, cumulative bounds, ordered per-basis progress and both head paths require
  runtime equivalence; SQL FK existence alone is not that proof.
- Signed64 extrema, canonical arbitrary-precision VALUE components, independent
  byte/count/total bounds, n+1 allocation/65-row continuation, pure-input priority,
  selected stored-error priority and cumulative writer budgets are specified.
  All allocations/bindings must precede own writes where possible; every later
  failure needs guarded whole-unit rollback, with actual uncertainty/capability/
  protocol errors preserved. These are future implementation obligations.

## Own independent checks

Host Python3.14.8 / stdlib SQLite3.54.0. Scratch
`python3 /tmp/storage002-review-independent.py` derives expectations directly from
the exact14 document SQL literals, manually enumerated new table/index inventory,
the framing definition and already-frozen prefix fixtures. It does not call a new
producer/manifest/codec or reuse author/root diagnostic scripts as its oracle.

| Actual check | Observed outcome |
| --- | --- |
| Four prefix catalog/framing comparisons | Exact7/13/20/30 objects and2000/10535/18126/29015 bytes; each frozen payload and digest reproduced |
| Proposed step5 expected versus actual catalog |14 tables +22 SQLNULL autoindexes; full66; exact columns/SQL byte lengths and unchanged prefix30 |
| Own canonical step5 framing | F62465 / SHA256 `c9ba98fa4fe2a42cc6ed9776baa011fc2b93cf879e7374b53e9a6128f7d7afbb` |
| Independently supplied raw data | Actual prefix FK targets, CID0/absent subjects, two original Main bases100/50, ONE current request -30 and TWO links -20/-10; foreign_key_check empty, integrity_check OK |
| Disposable constraint probes |28 expected SQL refusals covering types/NULL/domains/groups/source targets/component ownership/reversal positions/head targets and uniqueness |
| Explicit SQL-admitted cases |6 runtime-only obligations: head cumulative equality, claim subject equivalence, application-after role, declared balance count, canonical rational reduction and component conservation |
| Independent Fraction arithmetic | SB03 partitions, SB05 cumulative residuals, SB11 half-even residual literals, SB13 sum and SB14 direct equality pass |
| `python3 tools/check_project.py` | PASS391 tasks/122 decisions/current links/presets/HTML, both original candidate and actual replacement |
| `python3 tools/progress.py render --check` | Current, both revisions; no shared edits |
| `python3 -m unittest discover -s tests/tooling -v` | PASS19,0.282s at4956; unchanged tooling not rerun for prose-only replacement |
| Direct old/new comparison and `git diff --check` |14 SB rows/physical/predecessor pins unchanged; PASS |

Scratch source SHA256
`d88c0f4737a472b35908ec0a6ea78f53afdd0ff88c8f4f3b6d9c0b04068793e1`;
summary `/tmp/storage002-review-independent.json`, SHA256
`585e6c40e7103296928f7d3fb05b4d4bfb6cbc954738069134c19fdac2e64dc5`.
Initial scratch setup omitted existing catalog/history bootstrap tables; those
frozen prefix tables were added before the successful complete diagnostic. This
was diagnostic setup, not feature red. Raw rows establish SQL representability,
not complete current support-anchor/semantic application validity or transaction
behavior. SQL-admitted corruptions are explicitly required runtime rejections.

## Remaining gates

Next comes an actual minimal owned C++ API/immediate named stubs and separate
API/build/config review, then independent literal fixture/old-writer environment
and genuine feature tests/full baseline review, selected official environment
proof and FIRST intended missing-behavior red. STORAGE-002 production must wait
for that baseline, then pass acceptance, separate source/config review and actual
automatic official CI. No native suite, new facade, old red or live action ran
for this review. Root retains latest published-head CI/discussions/rules and normal
merge checks; no future revision or merge is approved here.

Actual authority, numeric policy, full state/job/wire/recovery/delivery meaning,
provider correlation and refund allocation across prior debt remain owning gates.
ACCT-004/STORAGE-002 consume this contract, not prerequisites creating a cycle.
All seven integrations/three FX and full application/UI/platform/package/security/
backup/recovery/live release requirements remain open and unchanged.

## Actual typed-read extension / minimal API and build review

2026-11-02: **APPROVE this actual additional contract projection, minimal owned
API, immediate named stubs and build/config revision.** Normally merged root
`848e76d7b4269391ff457e572cf9cafbe78cf313` into the clean review branch as
`0465ea0f654ef359cd5f43a2d9b9219b8925456b`; its tree matches the supplied candidate.
Source/config were frozen by root `79b69d437d12df5511f7e38fdc975eb7f0452e12`.
The earlier approval above remains valid for its unchanged exact subsets.

| Actually reviewed artifact | Git blob |
| --- | --- |
| `server/include/subathon/server/sqlite_application_bundle.hpp` | `8e24f0624673bf88f86405651f9aabee1026a226` |
| `server/src/sqlite_application_bundle.cpp` | `83e9a0a745bad413066ad75cf9eb82fd210b2186` |
| Concrete logical with separately reviewed typed read | `2f6e9a8ea8aa76a979fe9d800c3bf69ffbee24b6` |
| Author evidence | `4f09e3b70f581113371ba53d10bedf2614fdcfc4` |
| `CMakeLists.txt` | `1f25732eb1d6b114df21bd9327203dd2e551149f` |
| `core/CMakeLists.txt` | `9fb0b58ba84fd081b34c59013c858120eb0fc42a` |
| `server/CMakeLists.txt` | `23b740486282e1bf01c85e5845c1403795e3ac6d` |
| `CMakePresets.json` | `70af89b3f04995224b1210baa53b6affacd71495` |
| `.github/workflows/sqlite-writer.yml` | `a9835c8cded5d0568ad27dd6696cbe3271e5b402` |

Read the complete375-line header/36-line source, actual logical extension/author
and root CI evidence and all five config changes. There are exactly SEVEN immediate
`SqliteApplicationBundleNotImplemented` bodies: manifest and six facade methods.
The constructor only stores the existing revocable weak session and options;
implicit copying/destruction transfers no owner or cleanup responsibility. There
is no validation, codec, query, manifest/DDL, numeric or transaction implementation.

The added `read_reversal_basis` and SB15 were NOT covered retroactively by the prior
contract verdict. They now receive actual review: typed locator contains original
operation/request position/credited kind0/2/3; ready owns selected complete capture,
effect/component set, actual credit/target/provenance and progress with complete
last-occurrence identity. Valid source without history gives exact zero progress;
missing/corrupt source is a typed failure, not invented zero credit. Unsupported
actual credit returns the declared conflict. It uses the explicitly reviewed
selected-source closure and separate64/65 per-basis audit; unselected payloads,
whole-operation arithmetic, policy and authority are not certified. Future stage
rechecks actual progress. All results remain provisional under caller ownership.

DTOs retain all three snapshots, current and historical complete contribution
dimensions/application-after, bounded support provenance, full selected plan,
ordered occurrences and MANY links per immutable request. Explicit subject/CID0
versus absence, positive full-range receipt/revision fields, signed64 units/positions,
known enum roles, presence invariants, positive option ceilings versus valid empty
vectors, concrete vector n+1/total bounds and method preflight/absence/error priorities
match the contract. Pure failure/healthy refusal versus actual whole-unit rollback/
unknown semantics are obligations for later implementation, not behavior observed
from these stubs. Existing Exact codec semantics remain unchanged.

Independent byte comparisons confirm removing ONLY the new method row/subsection/
SB15 restores approved logicalcf462 EXACTLY. Physicalf13/all14 SQL and old14 SB
rows are unchanged; previous raw SQL/framing/arithmetic checks need no repeat.
All old core/server headers/producers and tests/fixtures remain byte-identical.
Workflow normalization replacing only `sqlite-bundle-debug` with `sqlite-plan-debug`
reproduces its previous file exactly: old commands/guards/pins/permissions/job names
are preserved. The new flag/profile/library compile existing selected dependencies;
no new test registration or new feature call is introduced.

Own actual commands in the isolated review tree:

| Command | Outcome |
| --- | --- |
| `cmake --preset sqlite-bundle-debug` | PASS; GNU16.2.1/CMake4.4.5/Boost1.92.0/Python3.14.8 |
| `cmake --build --preset sqlite-bundle-debug` | Fresh84 steps PASS, zero warnings; new source object and library actually built |
| `ctest --preset sqlite-bundle-debug --show-only=json-v1` | Exactly173 OLD cases, zero new bundle registrations; inspected before execution |
| `ctest --preset sqlite-bundle-debug` |173/173 PASS,4.48s;170 Qt summaries all0failed/0skipped/0blacklisted; actual Qt6.12.0/SQLite3.54.0 |
| `clang-format --dry-run --Werror` on both new C++ files | PASS |
| `/tmp/subathon-actionlint-1.7.12/actionlint .github/workflows/sqlite-writer.yml` | PASS |
| `python3 tools/check_project.py`, `python3 tools/progress.py render --check`, `git diff --check` | PASS391/122/current HTML and consistency |

Preserved scratch logs `/tmp/storagetest002-api-review-*` SHA256:

| Suffix | SHA256 |
| --- | --- |
| `configure.log` | `763893e9ca897f016c8a3c66853509674229d32ff843bc36eebfe0b0fee641c7` |
| `build.log` | `3576f43fd4b9bda7f762c2a38db1a6bea406b5f72f4d6a92ef1da89bca0146ff` |
| `discovery.json` | `141fc7776727b5537ae564c9d6e6e91828b6379ec768496a5eb5d43e793bfd1a` |
| `old173.log` | `b312cd9deae9ddbbe2ecd0ef69882c388f95624cd2ad806325f5ef7de115e0a9` |
| `LastTest.log` | `98efc692ed789efec91a56b8b87380b535df2d2665d5b5d4f52c0c4495709248` |

A read-only postprocessor initially searched for compact `SQLite3.54.0`; the actual
banner contains `SQLite 3.54.0`. Corrected that parser without rerunning tests.
No new bundle entrypoint, original red or additional native suite was invoked.
Old173 proves existing stability and minimal compilation, not bundle acceptance.

Actual selected official CI for this profile remains the coordinator's separately
observed gate; this verdict does not presume a future head passed. Independent
fixture/old-writer green and feature authoring/review, selected NEW-green/exact
discovery and FIRST genuine missing-behavior red remain required. No feature-test
baseline, source implementation, release capability, future CI or merge approval
follows; all earlier unknown semantic/financial/platform/release gates remain.

## Actual independent fixture/helper and harness review

2026-11-02: **APPROVE the actual independent literals, old-writer foundation
helper and its two-file harness below.** Normally merged complete root candidate
`e2e290b548cf38bfb3ef4527b75dc3833ad0f7ef` as review merge
`c134543e089eef6cf524ebfb9ed6494d54c2f4f7`. Author7375b327 and narrow setup-fix
5b161be6 are integrated. Read the entire JSON,667-line helper, author evidence,
changed harness and coordinator CI assessment. Earlier contract/API verdicts are
preserved; their headers/source/physical and all old tests remain unchanged.

| Actually reviewed artifact | Git blob |
| --- | --- |
| `tests/fixtures/sqlite_application_bundle_vectors.json` | `e8b6c77ef103c0d1f9803ea7eda73aebb0b76d5d` |
| `tests/qt/sqlite_application_bundle_environment.cpp` | `b0a5c72f152f193370d6c39af526fba61d332fdb` |
| `docs/evidence/storagetest-002-fixture-author.md` | `b5a22d25416015572f8381aa80bf60c4cd3c11af` |
| `tests/qt/CMakeLists.txt` | `203db2f439add44c6fe20b6ce5152134aa61f572` |
| `.github/workflows/sqlite-writer.yml` | `b9e1a49583e8e54800997215ba4e65ec58ee2b8c` |

Final JSON SHA256 `aebff7061f91fdf7598b321963bae155c139ed1dc2d1705e3122268bed84c312`
equals the exact earlier draft snapshot independently checked by this reviewer.
`python3 /tmp/storage002-fixture-draft-review.py`, source SHA256
`0207baa56a662bb8038a75cc3aecab21965b4dc02c1b3cd92459413c453a2433`, passed:
manual14-table/22-NULL-index/full66 projection versus document SQL/frozen prefix30,
F62465/c9ba98 full payload,56 typed setup statements/all six raw scenarios/all
expected queries,28 feasible SQL refusals and10 SQL-admitted runtime obligations,
conditional trigger's actual prepared predicate/exact error, uint64/signed64/
canonical4201/4200-bit and Fraction arithmetic. It uses no future producer or
author/root diagnostic as oracle. An initial hash guard caught authoring drift
from38fa BEFORE running; then the actual aeb snapshot was pinned and checked.
These completed raw checks were retained for byte-identical final JSON, not rerun.

Checked source validity beyond FKs against actual old15 source/capture rules:
positive plan quotes match their ACTUAL event money20/5/15/4 on receipts7/8/9/10,
while CID0's original support receipt1/amount20 remains distinct. Original/support
anchor, confirmation links, known-versus-arrival configuration, complete current/
captured dimensions, typed groups and all11 historical-quote cells resolve under
the stated storage scopes. SQLNULL uses actual nullness/typeof, not Invalid
QVariant equality. ONE immutable refund request references TWO bases; per-basis
positions/cumulative/head paths and direct-copy SB14 remain independent oracles.
Sparse MAX/raw-boundary rows are explicitly INCOMPLETE/INVALID aggregate evidence.
Supplied clock400→advanced0→after300 and opaque state/job formats certify no clock,
authority, policy, delivery or complete application semantics.

The initial d9 helper failed ONLY trigger setup before its application unit:
the existing writer rejects ANY semicolon. The preserved coordinator first-run
record is honestly setup failure, not a feature red. The b0 correction installs
the SAME disposable compound trigger through its owned observer ONLY while the
controlled writer is idle. Actual BEGIN/ROLLBACK and exact catalog row visibility
on both connections prove instrumentation installation/removal. It changes no
writer restriction, JSON/predicate or staged/rollback assertion. Fresh prior
pending0, actual in-unit application1/effects/state with OLD head, outside pending0,
real SQL abort, inside-catch lock acquisition/full typed snapshot, trigger removal
and identical fault-off commit are all asserted. This proves raw/old-writer
foundation reach and rollback, not future bundle-facade reach.

Own actual commands (GNU16.2.1/CMake4.4.5/Boost1.92/Qt6.12.0,SQLite3.54.0):

| Command/check | Outcome |
| --- | --- |
| `cmake --preset sqlite-bundle-debug` | PASS |
| `cmake --build --preset sqlite-bundle-debug --target server_sqlite_application_bundle_environment` |4 compile/link steps PASS, zero warnings |
| Built helper `-functions` | Exactly five slots listed below |
| `ctest --preset sqlite-bundle-debug --show-only=json-v1` |174=exact prior173 plus this one driver; no feature case |
| `ctest --preset sqlite-bundle-debug -R '^scaffold.sqlite_application_bundle_environment$' --verbose` |7PASS/0FAIL/0SKIP/0blacklisted,443ms; CTest0.45s |
| Actual Ninja link command / `nm -C` | Only old writer project library; no new Store/manifest symbols |
| Project391/122, current render, diff, actionlint and helper clang-format checks | PASS |

Exact compiled slots are `bundleSchemaPrefixMatchesLiteralProjection`,
`typedBundlePayloadsAndHistoricalProvenanceRemainExact`,
`multipleOriginalBasesShareOneRequestAndHeads`,
`bundleConstraintsAndRuntimeOnlyAdmissionsAreExplicit` and
`conditionalMidstageFaultRollsBackPreparedUnit` (each `()`). Removing the15-line
CMake addition restores the previous file exactly; workflow only appends18 lines
with unconditional exact-five compiled discovery and required execution. All old
commands/pins/guards/permissions/check names are preserved.

Own `/tmp/storagetest002-fixture-review-*` logs SHA256:

| Suffix | SHA256 |
| --- | --- |
| `configure.log` | `9b0b79c1777c96ff52f3958b5ee1d85523fb582cd5786698ed980d4f1c662e73` |
| `build.log` | `ed07969bef561630a7d6782ad49ca1610606a604943c5659c51e27752e502e6b` |
| `functions.log` | `c5d94e66cd638a4d8da0a78c3b07cfafc512ff734436c093fbf753ee89d61417` |
| `discovery.json` | `57377aeb34efdde99bb3d97d322ccec352f9eeda77bb0d50aa87e3c6856b0ea7` |
| `green.log` | `552c2c0e2b181b4dc6ede71fe6cacf26761c725a3a6f85b33428aac19c0355a6` |
| `LastTest.log` | `746538cd957010fcd64bad0f46e2968f39f1ed2101f743310f1e2f94bf1d67f5` |

No old173 replay, new bundle entrypoint, feature body, original red or live action
ran. Selected official NEW-five green remains a separate coordinator-observed
gate before feature authoring. Actual feature assertions/full baseline review,
compiled exact discovery/environment ordering and FIRST genuine named-stub red
remain required before STORAGE-002 production. This approves no absent test/source,
future CI/head/merge or wider semantic/financial/platform/release capability.

## Actual post-SQL snapshot-mismatch category precision review

Author: coordinator `/root`; different reviewer `/root/contribution_reviewer`,
2026-11-02, NEW isolated `task/STORAGETEST-002-category-review` tree.
Actually reviewed root `e6e0be348c3112093fee18803f86fabc1ed88578`, logical blob
`80f8ee65869a27c6b1b7b6d3819a0065777958b7`, against specification §3.4/B08,
accepted logical2f6, existing known-storage-error/unknown-outcome protocol and
the concrete API. Scope is ONLY the additional12-line contract paragraph.

The prior contract required persisted-head→advanced-before binding/clock-only
conservation but did not specify this post-SQL case's category or cleanup: expected
head matches, selected stored rows are healthy, but same-token data differs or a
fresh advanced-before changes preserved debt/counters/configuration/format. It
cannot be a pure-input failure based on unseen data, matching-token `stale_state`,
or stored corruption. I had proposed a coherent normal `state_token_conflict`
alternative; that was a proposal, not an established test oracle.

The actual chosen `source_mismatch` with actual whole-unit rollback is independently
consistent with source-relative validation and B08: the supplied proposal disagrees
with the resolved head binding, and the existing known-storage-failure protocol
aborts the whole caller unit. Earlier DURABLE intake survives; earlier merely staged
caller work is rolled back. Existing operation/claim/current and plan freshness/
fresh-token priority remains ahead of this relation check. It introduces no clock
evaluation, permission or new stored corruption claim.

Requested changes on original58ea wording were material: cleanup had also been
underspecified, and reporting a known error needs actual rollback proof. Actual80f8
resolves both: it explicitly selects category AND cleanup while preserving the
preexisting rejection, requires rollback BEFORE reporting `source_mismatch`, and
preserves real rollback/cleanup uncertainty as WorkFailure. No inactive cleanup or
successful rollback is presumed. This is approval of the actual replacement,
not agreement inferred from the coordinator's choice.

Own commands: `python3 tools/check_project.py` PASS391/122;
`python3 tools/progress.py render --check` current; `git diff --check` PASS.
Independent byte comparison removes exactly the paragraph to restore2f6, including
all15 literal vectors. Header8e24/source83e9/physicalf13/JSONe8b/helperb0 remain
byte-identical. No API/schema/source/test/fixture changes or native/new-entrypoint/
feature-body/red invocation occurred.

Verdict: **APPROVED for integration of this documentation precision ONLY.** Prior
unchanged reviews remain valid. Actual feature assertions/full baseline review,
selected same-revision green/discovery and FIRST intended red are still required;
no future artifact, executable behavior, CI/head/merge or release approval follows.
Owner scope remains finish existing PR42, then STOP; no implementation assignment.

## Actual full32 feature-test and harness review

2026-11-03 (Europe/Zurich), different reviewer `/root/contribution_reviewer`:
**APPROVED FOR TEST BASELINE**, limited to the actual assertions/setup/harness
below. This is not observed red, passing bundle behavior, production/source
approval or a future CI/head/merge approval. Reviewed the complete3335-line source,
278-line author note and actual root harness at clean
`60a2fcc7f0755f2b6325947d7dc133f363477667` in the NEW isolated
`task/STORAGETEST-002-feature-review` tree. No feature body has run.

| Actual accepted artifact | Git blob |
| --- | --- |
| [Feature assertions and independent builders](../../tests/qt/sqlite_application_bundle_tests.cpp) | `4b60fcaa06dec8136f0b81e7ac6d01374df94a48` |
| [Actual author evidence /32-case specification map](storagetest-002-test-author.md) | `caac20224fbd364149d907e175a3ca21830d8a61` |
| `tests/qt/CMakeLists.txt` | `91035af6f5ab00916770c1ba508db01997354ffe` |
| `.github/workflows/sqlite-writer.yml` | `453f1a2df00f11442330f2649d9c4ded41d7841e` |

Source SHA256 is
`6f84dfa02c0a6ad2dfccf1d6c87b004cb188678ca439a43b35858a59467a0b19`.
Header8e24/source83e9 (seven immediate named stubs), logical80f8/category42f,
physicalf13, JSONe8b6 and old-writer helperb0 remain exactly unchanged. Earlier
contract/API/fixture/SQL/green reviews remain valid for these frozen subsets.

### Actual findings and resolutions

The initial bounded read-only snapshot240402b was not a baseline verdict. All six
reported defects are resolved in this actual source: MAX expected current state
comes from its own literal row; deliberate rollback rejects hidden assertion/work
exceptions; NULL corruption uses a genuinely nullable required quote cell; complete
current activity and support verification fields are compared; full typed prefix
snapshot/schema is compared with ONLY the separately asserted CID0 application
delta normalized; optional presence uses explicit bool-compatible assertions.

Four further findings from the whole authored source are resolved in the actual
87372b2/0a655f9 revisions integrated into60a:

1. Cumulative-budget proof no longer assumes a public lookup reads a600k value
   once. After observed refund staging, ONE explicit old-session query consumes
   at least600000 bytes; the required full result cannot fit the remaining1MiB
   budget. Typed result-limit, actual rollback inside catch and complete prior
   snapshot preservation remain. No SQL algorithm or budget reset is prescribed.
2. Opaque-key setup no longer changes a referenced PK before its immediate FKs.
   It inserts a complete fresh snapshot/balances then repoints the head, explicitly
   retaining old `start` data with writer FK enforcement intact.
3. Three independent selected-Exact corruptions distinguish equivalent noncanonical
   200/2 from actual100, well-typed oversized malformed bytes before grammar, and
   wrong denominator SQL class before numerator size. Untouched selected components
   fit the chosen limit3. BOTH classes/shapes→BOTH sizes→grammar/gcd is asserted.
4. A capped original request600/actual Main120/cap480 with1000→1120 independently
   proves actual-credit projection. A matching fresh refund request-130/after990,
   fraction1/entitlement120/residual-10 is locally valid yet above actual credit.
   It must return `reversal_conflict`, preserving a prior caller marker through
   actual COMMIT. Removing ONLY that marker in a later acknowledged unit restores
   the exact before snapshot. A getter using requested600 or a missing over-original
   guard can no longer escape the oracle.

The Qt-keyword `slots` build defect and later shadow warning were also corrected
without assertion changes. Their initial build results are honestly setup/compiler
history, never a feature red. Approval is for the actual final source above.

### Assertions and reachable scopes assessed

Checked against specification v0.45 §§3.3–3.5/4/8/16, DB-01/NUM-01–04 and actual
AT01–21/SB01–15: complete owned records/provenance and all independent current
dimensions; explicit receipt/CIDNULL/CID0/MAX roles with healthy revision1; supplied
signed cap/debt/deficit/residual values and exact aggregate conservation; provisional
same-unit duplicate/claim branches with fresh state-b and real commit versus abandon;
intent equality versus VALID changed computed outcome; freshness/token binding and
actual80f8 post-SQL failure; selected/full/absent closure and type/group/count errors.

Reviewed all reversal assertions: one current request/many original bases, repeated
same basis at two current ordinals, actual credited bounds, independent progress/
terminal identity, zero increments/full-final-zero and unsupported prior-debt
allocation. Real65 job/plan/progress rows exercise64/65 continuation and late
corruption without materialization quotas; sparse MAX remains invalid. Targeted
source metadata explicitly leaves unselected body/numeric certification to full
lookup. No provider/correlation/permission/clock/job-delivery or financial policy
is inferred from synthetic data.

Rollback assertions require actual inside-catch independent BEGIN and typed
preservation while the facade lives. Conditional trigger predicates require real
prepared rows/application patch/old head; fault-off identical retry separates
mid-stage failure from a later caller exception. Actual control seams distinguish
COMMIT/cleanup acknowledgement loss, confirmed-COMMIT body exception and later
publication. Borrowed copying/destruction, owner abandonment, foreign/closed/stale
tickets and pure-before-ticket priority remain observable. No destructor cleanup,
inactive rollback proof or replay is invented.

Builders/init use only independently frozen literals and old writer operations.
No new entrypoint supplies setup or expected data. Marker-aware wrappers preserve
the named NotImplemented failure; deliberate rollback cannot absorb failed
assertions. All32 bodies still need their first actual missing-behavior observation.

### Own checks, without feature execution

Independent pure-Python field weighting from frozen JSON and public logical
members, `python3 /tmp/storage002-feature-owned-review.py`, reproduces
145+122+121+41+216+116+36+41=838 input bytes,112 current-state bytes and20/9/10
returned vector items. Script SHA256
`66d2c7eaa476b146a7fc7bb90b2c63e5717a580a3b786e00e99181c9ca78b081`.
It invokes no SQL, producer or new API. Input838/837, state112/111 and the returned
item boundaries are therefore independent expected values.

| Actual command/check | Result |
| --- | --- |
| `cmake --preset sqlite-bundle-debug` | PASS; GNU16.2.1/CMake4.4.5/Boost1.92, Qt>=6.12 build dependency |
| `cmake --build --preset sqlite-bundle-debug --target server_sqlite_application_bundle_acceptance` | Fresh28 dependency/object/link steps PASS, zero warnings |
| Built acceptance executable `-functions` | Exactly32 compiled slots; no body/init invocation |
| `ctest --preset sqlite-bundle-debug -L '^sqlite-bundle$' --show-only=json-v1` |32 matching enabled commands; no inversion |
| `ctest --preset sqlite-bundle-debug --show-only=json-v1` |206=unchanged174 earlier names plus32 feature registrations |
| Project391/122, current render, diff, actionlint, source clang-format | PASS |

An initial read-only slot inventory also matched ordinary fixture helper methods;
restricting it to Qt's actual slot section matches the already captured32 compiled
names. No test execution or source change resulted. Removing the49-line feature
CMake block restores the approved fixture harness byte-for-byte; workflow only
appends19 lines of compiled-registration checks after required new-five diagnostics.
Old commands/pins/guards/check names/permissions remain unchanged. Neither automatic
CI nor these commands execute intentional-red bodies or suppress their failures.

Own `/tmp/storagetest002-feature-review-*` evidence SHA256:

| Suffix | SHA256 |
| --- | --- |
| `configure.log` | `4288f26fcc8d31998c48929132b8fb231ff554c790591c0398ebf4e2a38231d8` |
| `build.log` | `1bca3d9e247a535ae1ffba46dfd1ed286b941090779cb6cc9f1b37cf3915ff6d` |
| `functions.log` | `b7f236b0608cfae062eec4ff3670967d4cf50453e5467d89894d88ef361c4ee6` |
| `bundle-discovery.json` | `427b6fff1f7ee820fee745560cd5ac9a66296bb132848d46ba2edfd834c40e05` |
| `all-discovery.json` | `d99f7d7de579ca098e932ce2008b1249d435247821eba88aaeaf9bec5846d93a` |

No feature body/new entrypoint, original red or unchanged native/fixture suite ran
in this review. Root must observe required selected official NEW-five green and
exact32 discovery on the complete clean approved revision BEFORE FIRST intended
red, then record the original32 failures and separately review final attribution/
completion metadata. Latest published-head CI/discussions/rules/normal merge gates
remain. Production STORAGE-002 stays planned; owner scope is finish PR42 baseline
then STOP, with all broader semantic/financial/platform/release gates preserved.
