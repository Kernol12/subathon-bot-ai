# STORAGETEST-013 — independent review

Reviewer: `/root/storage_work_code_reviewer`, 2026-11-01; different from logical
author `/root/writer_transaction_reviewer` and physical-proposal author `/root`.
Branch `task/STORAGETEST-013-review`, isolated worktree
`local-evidence/worktrees/storagetest013-review`, from claim `b263f60` after
merged PR34/main `a4bb565`. Own only this review evidence.

## Actual contract and physical proposal verdict

Verdict: **approved for minimal API/stub drafting, contract stage only**.
No API, test, executable baseline/red or future implementation is approved.
No blocking finding remains in these actual documents.

Reviewed author `0c154322d86789a1dbdf7e91927933b715ef6151` (initial logical
blob `c85cc8b966c6a799a04dbd4ed22ef4a56ced51f4`) and actual coordinated root
`845ae2012f9de9092e70e833b4169e2032f64d19`, fast-forward merged into the
clean review branch. Exact actual reviewed artifacts:

- [Logical contract](../contracts/sqlite-receipts-v1.md):
  `f8af1b99ca0348250ca24ff5ac98846f4066e55d`.
- [Physical proposal](../contracts/sqlite-receipts-schema-v1.md):
  `2ef4f1384c5c95c1bef5728b095ff5f4e7877e6a`.
- Root's actual STORAGE-013 brief, author evidence and schema-probe evidence at
  the same coordinated revision, including the money/rate scope reconciliation.

Read root/core/server/tests AGENTS, specification §§3.2–3.4/DB-01, brief,
identified receipt and event/state contracts, actual Receipt* values/service/
ports, all fifteen unchanged simulation cases and primitive fake, application-plan
zero policy, retained-context source/API and existing session/migration contracts.
Initial preparation and initial logical inspection were read-only. Approval waited
for the actual physical proposal rather than inferring an absent schema.

Preparation found a real cross-contract constraint: application-plans-v1 Capture
rejects zero receipt ID, enforced by application_plan.cpp line19 and its reviewed
test at line262. Root withdrew the tentative first0 proposal before any such
behavior was authored/approved. Actual contracts preserve pure uint64 codec
0..MAX while persisted/allocated ReceiptId stays1..MAX, with explicit NULL
allocator absence before first1. The fake's incidental first1 is not the authority.
No existing plan contract or assertion was changed to resolve the issue.

The logical contract preserves these required boundaries:

- ReceiptIntake alone classifies created/retry/conflict from all original facts.
  The repository performs primitive lookup/create/append/commit, with no upsert,
  fact normalization, new correlation or transport-based identity. Four exact
  BLOB identity components have matching database uniqueness.
- Immutable originals and every complete attempt preserve their own facts and
  arrival. A private positive signed64 ordinal orders repeated equal-time
  deliveries; it is not a new core ID or a timestamp uniqueness policy. New
  originals need an appended first attempt before commit, while an own staged
  original may be found before that append. History is complete or fails bounded
  materialization; it is never truncated or silently expired.
- Pure typed uint64 encoding is exactly eight unsigned big-endian bytes. Raw
  stored IDs require BLOB8/nonzero domain validation, with no signed INTEGER,
  REAL or text path. NULL allocator state is distinct from zero/corruption;
  allocator/greatest-original consistency, MAX once, no wrap/reuse and rollback
  of uncommitted allocation are explicit. Missing/malformed metadata is not reset.
- Optional strings and groups distinguish SQL NULL from present-empty BLOB and
  retain exact money/link spelling and bytes. uint32 schema versions and signed64
  UTC extrema are preserved without a new time/schema-zero rule. No source time,
  actor, confirmation or rate is invented. ReceiptFacts has no rate field;
  actual HistoricalValuation/rate provenance remains a mandatory later014/015 gate.
- Arrival configuration and context resolve independently in their typed retained
  namespaces. Retention validates a context's known configuration, but the adapter
  adds no unsupported equality with arrival.configuration_ref. Stored missing or
  malformed targets fail rather than enrich unknown or return fabricated records.
- Context mode uses the existing one real BEGIN. Explicit adoption grants the port
  wrapper real commit/cleanup authority over the already active unit, including
  earlier retention. Service success follows acknowledged physical commit; normal
  terminal work completion still controls publication. Repository values are inert,
  wrapper cleanup is nonthrowing and owner finalization remains a backstop. No new
  connection/queue/raw handle/nested wait/deferred vote/retry is authorized.
- Input errors precede primitive SQL and do not falsely claim known rollback or
  undo earlier caught caller writes. Actual local storage failure establishes
  known cleanup before ReceiptStorageError; mapped SQL/budget errors preserve
  existing outer WorkError/outcome. Actual foreign/stale/terminal errors remain
  SqliteWorkFailure, and uncertainty always outranks local reasons. All relevant
  lookups/results remain charged to cumulative session budgets without reset.

Reviewed all SR01–SR12 future oracles, including independent typed literal IDs,
zero/domain distinctions, exhaustion, equal-time histories, optional/value extrema,
reopen, adoption, uniqueness, post-stage flags, synchronous rollback lock probes,
bounds, capability lifetime and physical acknowledgment loss. They are proposed
oracles, not authored tests or observed results. Existing fifteen fake-backed cases
remain service simulation, not retroactive SQL proof.

## Exact additive migration and independent host diagnostics

Accepted contract proposal: version2 `identified-receipts-v1`, four exact ordered
SQL statements, supported starts `[0,1,2]`, unchanged format1/store identity and
bit-for-bit retention step1. The complete version2 projection has13 objects:
old7 plus allocator/original/attempt tables and three new generated indexes.
The allocator INTEGER PRIMARY KEY generates no index; all six total autoindexes
have NULL SQL. The fourth statement seeds exactly `(1,NULL)`.

The DDL covers every actual original/attempt field, BLOB8 domain IDs, four-part
UNIQUE, optional all-or-none BLOB pairs, signed INTEGER timestamps, uint32 range,
positive ordinals, known disposition values and typed retained/original FKs.
It neither introduces semantic crosslink equality nor relies on triggers to
sandbox arbitrary trusted SQL. Runtime structural/history/selected-type/bound/
allocator checks remain required; the schema alone does not establish port errors.
Migration1 is not rewritten and opening/constructing does not advance a database.
Whole-prefix atomicity and operational inspection/advance preconditions remain.

Independently executed an inline Python/sqlite3 in-memory diagnostic derived from
the actual Markdown's four SQL literals and the unchanged existing prefix JSON,
without a production factory/codec. Host SQLite3.54.0, actual results:

- Exactly13 schema tuples, exact stored CREATE SQL, six named NULL-SQL autoindexes,
  unchanged seven prefix objects and allocator singleton1/SQL NULL all match.
- Reconstructed complete F-framed payload from actual sorted UTF-8 object tuples:
  **10535 bytes**, SHA256
  `24614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2`.
  No value from future product code supplied an expected result.
- A MAX-ID original and two equal-time attempts preserve signed64/u32 extrema,
  exact long decimal bytes, present-empty actor/connection/link groups versus NULL
  and independently resolvable differing configuration/context known link.
- Fourteen deliberate constraint violations fail: zero domain ID/allocator,
  empty identity, out-of-range schema values, noninteger timestamp, actor TEXT,
  partial optional groups, wrong-namespace FK, invalid ordinal/disposition and
  duplicate four-part identity under another ID. These are SQL-only checks,
  not implemented allocation, intake classification or rollback behavior.

Root's [Qt/private-writer probe](storagetest-013-probe.md) is separately attributed;
this reviewer read its evidence but did not rerun it. Neither host diagnostic
establishes selected official-Qt support or new feature acceptance/red.

## Checks and next actual gates

Actual reviewer checks: clean status and `git merge --ff-only 845ae20`, full
document/source/oracle inspection, the independent disposable SQL/framing program
above, existing-artifact diff/hash checks, project391/render and whitespace checks.
No native acceptance or historical red was repeated and no source/test/build/
schema file was authored by this reviewer. Only the review note changes here.

API precision is a separate actual review before test authoring. It must pin
representable positive options/checked lookahead, exact error precedence/categories,
owner-thread/single-use wrapper rules, local second-port-begin protocol policy
versus actual writer sticky errors, adoption transfer only after successful wrapper
construction, context-begin allocation exception cleanup, and transaction-owned
session/options/pending-first-attempt state independent of repository lifetime.
Append must not turn attempts_per_find into a retry quota; bounded per-receipt
count/min/max or equivalent can establish its order without materializing history.
These implementation/API details remain within the accepted logical contract;
no absent concrete declaration or behavior is approved here.

Next minimal interfaces/explicit stubs require their own actual review. Exact
literal full-prefix/schema/payload fixtures and typed-codec/SQL integration tests
then need different actual test/harness review, successful compilation, selected
official-Qt green environment proof and genuine intended missing-behavior red
before STORAGE-013 implementation. No provider, semantic wire/evaluator, complete
application bundle, backup/update/recovery/power-loss or platform gate is completed.
All seven integrations and three FX providers retain the agreed release scope.

## Actual API, explicit stubs and compile/CI harness review

Verdict: **approved for new receipt test/fixture authoring only**. No new
receipt behavior, executable test baseline, selected13-object fixture proof or
intended red is approved. No concrete API/config finding remains.

Reviewed author `21a396b94f185890118adbb8c2cbef8de8a2a052`, integrated root
`54ebbd8`; minimal harness `50c1e8b23dfb5f79514379536429a7454cf8eaec`;
workflow/profile revision `b37daad74940b32439972d9c4affd52bf41ef4b7` and actual
root `a152a82ed66413c73e99a5fbe443eabdaf569528`, normally merged here.
Header blob `b43f9b6026498af18df22c2f695e76b23cb4064a`, source stub
`faf2856d8d61a91e518c50b3c0fdffe80a0f33a3`, logical contract
`e1ed975875e8b62d77d8efff67cfe5e8d5c95038`; physical companion2ef unchanged.
The approved f8 logical body is an exact byte prefix of the appended precision.

Actual declarations preserve context versus explicitly tagged active-session
adoption, noncopyable/nonmovable single-use repository values, copied weak tickets
and options, and construction-thread identity. Constructors only capture inert
values; destructor releases them without cleanup/SQL. The four source functions
unconditionally throw the distinct logic-error stub marker, which makes no false
known-rollback assertion about an adopted caller unit.

Precision correctly distinguishes ordered pure input errors, foreign-thread
rejection before mutable flags, local already_begun leaving the first unit usable,
and actual writer capability/sticky protocol errors. Adoption transfer is after
successful wrapper construction; context BEGIN allocation is exception-safe.
Transaction session/options/owner/pending state must outlive the repository
independently. Primitive preflight, known port failure categories and cleanup
uncertainty retain their agreed separation. Representable positive bounds reserve
checked find lookahead; supplied limits also bound retained-target documents.
Append uses per-receipt aggregate order checks, not find's history bound as a
write quota. No missing-reference/corruption normalization or commit vote is added.

Root's new flag, static target and sqlite-receipt-debug preset provide the existing
core/intake/writer/migration/retention dependencies and Qt include/link settings.
The actual workflow moves every configure/build/discovery/stable-body/executable
path to that preset, preserving pins, permissions, job name, guards and existing
lanes. No new receipt case is registered yet and no failure is skipped/inverted.

Actual independent checks in this worktree:

| Command/check | Result |
| --- | --- |
| `cmake --preset sqlite-receipt-debug`; `cmake --build --preset sqlite-receipt-debug -j2` | Exit0;48 build steps, including new stub archive, zero warnings; GNU16.2.1/Qt6.12 |
| CTest `--show-only=json-v1` plus suite-prefix counts | Exact95 old registrations, no new receipt feature case |
| `ctest --preset sqlite-receipt-debug --output-on-failure` | Exit0; old95/95 pass,2.26s; ignored capture build/sqlite-receipt-debug/reviewer-old95.log |
| Git prefix/hash/diff checks | Approved logical body exact; physical schema2ef and old core/service/ports/tests/retention/work/migrations unchanged |
| `clang-format --dry-run --Werror` on new header/source; actionlint1.7.12 on workflow | Exit0 |
| `python3 tools/check_project.py`; `python3 tools/progress.py render --check`; `git diff --check` | Exit0;391 records/122 decisions/current snapshot |

Also independently read actual ignored metadata/logs for selected official-Qt
[run41329002993](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41329002993)
at exacta152a82. They show completed SUCCESS, both archive hashes OK, GNU13.3,
new sqlite_receipts.cpp compilation/archive, Qt6.12/SQLite3.53.4, old25/work24/
migration24/retention16 and existing diagnostic7+3 passing with zero skips. This
is API compilation and existing-foundation evidence only: it proves no new
receipt method or new13-object fixture. No new receipt body or historical red
was invoked by this reviewer.

Next actual test/harness/oracle review remains mandatory. A single MAX-ordinal
row is corrupt noncontiguous history, not proof of reachable signed64 ordinal
exhaustion; constructing MAX contiguous rows is impractical. Future tests must
state that limit honestly and later code review must inspect the checked guard.
Independent full-prefix/typed-codec/schema fixtures, selected official-Qt green
proof and intended missing-behavior red still precede production. This verdict
does not transfer to absent tests, later source or a future CI head.

## Full actual test, fixture and harness review

Verdict: **approved actual artifacts for intended feature red, after separate
selected official-Qt new-fixture success is observed**. No unresolved finding.
No receipt feature body/init or historical red was executed by this reviewer.
The official new-green run and first intended receipt red remain pending gates;
neither is approved as an unobserved result.

Read all22 cases/helpers from author135d3d3/root97863f9, actual fixture33df432/
root5561e69 and committed harness1ce0020/14372c8, then all separate corrections.
Final coordinated root is8dfaf22, normally merged here as
`5aae8bcf5b2e0f686fa84733316daa4831711027`. Exact approved artifacts:

| Artifact | Git blob |
| --- | --- |
| Feature22, final actual source | dab97946568e09f15119a8995735539eb2e14c4d |
| Green3, final actual source | 8189f0274a38bb840c612673e0f858fe8d1777eb |
| Independent JSON | a79e5e71a3b9ca101441852c7fb7f26f735e9626 |
| Qt CMake | ff3a197d60b5f7bc6486a69c5f7fe9c9fa49e5cf |
| Workflow | feb5a9ff5d936c63dd4ebff2cfdff8f238e1fefd |

Headerb43/stubfaf/logicale1/physical2ef remain the exact full hashes in the API
review above. Old core/service/ports, original15 simulations, old SQLite sources,
tests and prefix fixtures are unchanged. Production still has four named throws.

Actual review findings were corrected by the assigned different authors, with
root authorization, before approval or any receipt-body run:

| Finding | Actual nonweakening correction inspected |
| --- | --- |
| SR10's single1048577-byte result only proves an individually oversized result, not cumulative charging | 5a8b700 retains it and adds a fresh proved-create/append unit with up to two600000-byte results, each individually valid; either attempted query may exhaust prior cumulative charges, both succeeding fails; outer stage/attempt/rollback/lock/snapshot/no-value guards remain |
| Pure-codec9-byte rejection did not test an overlong selected SQL ID | Same5a8 adds bad-12 at ID0e, mutated to9-byte000000000000000e00, preserving old12 variants/healthy0d; truncating to the former first8 cannot evade the oracle |
| SQL NOT NULL cannot legitimately supply a selected NULL receipt ID | Fixture authorcde2ef8 adds NULL INSERT refusal to existing green zero/INTEGER variants; this proves actual SQL rejection without fabricating a selected-NULL row or weakening schema |
| Missing retained-target failure mapping/value-limit propagation coverage | ad60c19 adds two isolated databases with short valid facts: actual target TEXT3 maps invalid_stored_data; target BLOB4 at value3 maps result_limit; raw types/lengths, synchronous lock release, no record/value and unchanged full snapshot isolate those causes |
| Positive global count alone did not prove a new retry/conflict attempt staged before fault | 9b4f38d reads owner counts before/after each real create/append, requires delta+1 and retains positive-count checks before setting flags/forcing actual failing SQL |
| TEXT allocator with different decoded bytes could fail consistency even without a strict type guard | 913354a adds TEXT with exact canonical ID1 bytes, raw typeof/hex proof, greatest ID1 and a fresh identity; only strict storage-class rejection supplies the expected known failure |
| Worker generic catches hid the initial named missing-feature cause | 183c908 flags named stubs inside each worker and rethrows on owner only after join; every real wrong_thread/input assertion remains, with no fabricated expected error or uncaught thread exception |

Reviewed SR01–SR12 coverage in actual code: independent typed-codec literals and
zero/domain distinction; exact immutable prefix/manifest; real unchanged intake
classification/all facts; ordered equal-time attempts; all four binary identity
branches; optional groups, byte strings and integer extrema/reopen; typed arrival
resolution without invented equality; allocation across signed/MAX boundaries;
actual post-stage rollback; adoption/physical commit/destructor/incomplete guards;
SQL uniqueness, preflight/bounds, selected corruption/targets/history, cumulative
budgets, local versus actual capability errors, owned wrapper/results and physical
acknowledgment-loss uncertainty/no replay. One MAX ordinal is explicitly corrupt
noncontiguous history; true MAX-contiguous increment safety remains a later code
guard review, not a falsely claimed executable exhaustion fixture.

Setup uses literal m1/m2 JSON with the existing implemented migration runner and
independent raw targets, never the new receipt factory/codec/repository in init.
The real failure decorator forwards primitives before observed new-row deltas and
real failing SQL; it contains no deduplication/disposition policy or fake outcome.
Known port failures and uncertainty remain separately asserted. First red must
still confirm actual setup/init/cleanup success rather than counting fixture errors.

Independently rederived actual JSON from contract SQL and host SQLite tuples:
complete10535-byte F payload/SHA24614f68…553a2, all13 objects, unchanged seven
prefix objects and all six decimal/hex STRING uint64 pairs match, without UUT
oracles. Read the complete green helper, including NULL/empty/group/type/FK/raw
numeric assertions. Its actual link command uses only old writer/Qt, not receipt,
migration or retention implementations. Thus green cannot pass by invoking a stub.

Actual committed harness matches the examined preview:22 separate feature
processes, vector macros on both targets, separate required three-slot green
executable and exact compiled-registration guards. Existing stable lanes/pins/
permissions remain. New green runs before feature discovery; explicit manual
run_receipt_feature_tests defaults false and propagates real CTest failure. No
WILL_FAIL, blanket continuation or executed-failure suppression was introduced.

| Actual independent check | Result |
| --- | --- |
| Configure/build sqlite-receipt-debug at7b78e5a, then incremental build after final8df patches | Exit0, both new executables compile/link, zero warnings |
| CTest JSON and executable `-functions` only, final revision | Exact118 registrations,22 compiled/registered receipt cases and3 compiled environment slots; no receipt body/init run |
| `ctest --preset sqlite-receipt-debug -R '^scaffold.sqlite_receipts_environment$' --verbose` at7b | Exit0; final unchanged818 helper,5 passes including init/cleanup,0skips, Qt6.12/hostSQLite3.54 |
| `ctest --preset sqlite-receipt-debug -E '^sqlite-receipt\.' --output-on-failure` at7b | Exit0;96/96 stable checks,2.25s; all22 feature names explicitly excluded |
| Final patch/hash and old-artifact checks | Final dab examined/rebuilt; green/JSON and every stable-lane input unchanged from7b, so no broad rerun; frozen API/contract/old-source invariants pass |
| Format, actionlint1.7.12, project391/render and diff checks | Exit0; current snapshot/122 decisions |

118 is discovery, not full118 passing acceptance. Local captures are ignored under
this worktree's build directory; no tracked source/test/fixture/harness was edited
by this reviewer. Only this note changes. Previous API-only official CI does not
prove the new13-object818 fixture. Root must integrate this approval, observe
actual selected official-Qt green3/current schema plus complete22 discovery, and
only then record the first genuine intended receipt red. Actual implementation,
its separate code/config review, passing acceptance/unconditional CI and all
previous operational/domain/provider/platform gates remain subsequent work.

## Final baseline metadata, first-red and CI attribution review

Verdict: **approved the actual baseline completion metadata and recorded gates**.
Reviewed root80ad5f29300a7cad9c9a7f6e4c1aef778e1fa43c and the concrete wording
correction57e1ad0e10398ebcc049e9970276ccabcb14589a, normally merged here at
7f9804ed34e40df546e05170999f464406591b3a. No unresolved finding. This is no
production approval or approval of future/latest-head CI. No native, feature,
setup or historical-red body was rerun for this metadata review.

Independently checked all nine frozen blobs above across actual full approval
7df446e, executed CI/red head5ffe2e4, completion80ad5f2, correction57e1ad0 and
this review HEAD. They remain exactly testdab97946568e09f15119a8995735539eb2e14c4d,
green8189f0274a38bb840c612673e0f858fe8d1777eb,
JSONa79e5e71a3b9ca101441852c7fb7f26f735e9626 and the previously listed complete
APIb43/stubfaf/logicale1/physical2ef/CMakeff3/workflowfeb hashes. The integrated
b1c77e1 review file is byte-identical to actual7df446e; b1c77e1 is an ancestor of
5ffe2e428c1b219688a268ed155056d54a224988. Old writer/migration/retention source
and retained prefix fixture are unchanged from merged maina4bb565.

Read actual cached metadata and full executed log for
[run41334070282](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41334070282),
and independently fetched `gh run view 41334070282 --json
databaseId,headSha,status,conclusion,event,url` plus the exact5ffe2e4 commit's
check-runs API. This pull_request run completed SUCCESS at that exact head; all11
checks there are completed/success. Observed Qt/ICU archive hashes OK, GNU13.3.0,
new executables compiled, Qt6.12.0/SQLite3.53.4, actual old25/work24/migration24/
retention16 success, migration green7 and retention green3 success, and NEW current
818 receipt fixture3 with5passes including init/cleanup and0skips. Complete22
compiled/CTest discovery succeeded. The manual feature-body step was not invoked.
This is new-fixture proof, distinct from earlier API-only CI and hostSQLite3.54.

Actual CI job completion17:58:13Z precedes the first-red metadata timestamp
2026-11-01T18:00:26.517274+00:00 at the same5ffe2e4. Read the separate ignored
storagetest013-red-metadata.json and parsed the complete tracked
[raw first red](storagetest-013-red-ctest.txt), blob
5b345cc45f2b7ddf643edd27a1290ea161e85870. Recorded command is exactly
`ctest --preset sqlite-receipt-debug -L sqlite-receipt --output-on-failure --parallel 1`,
actual returncode8. Every one of22 unique failed names matches an approved slot
and explicitly identifies missing receipt behavior. All22 initTestCase and22
cleanupTestCase passes are present; all22 summaries are2passed/1failed/0skipped/
0blacklisted, total0.22s. There is no crash, dependency or setup failure supplying
the red. Most later behavioral assertions remain unreached until implementation;
these failures do not prove their eventual runtime behavior.

Read current state, DB-01 map, brief, architecture/test strategy, handoff, CI and
checkpoint. Only STORAGETEST-013 task data changed since8dfaf22; all391 IDs and
dependencies are unchanged. Its kind is test_baseline and done means reviewed,
compiling, observed-red baseline only. STORAGE-013 stays planned/unassigned with
no verification or code review; DB-01 stays partial. Four explicit named throws
remain. ReceiptIntake still owns classification, domainReceiptId1..MAX versus
purecodec0..MAX is preserved, downstream actual numeric/rate codecs and all
application/operational/provider/platform gates remain. No full118 pass, live
operation, browser, power-loss or product-release claim was introduced.

The current CI intro's stale future-red wording was corrected in57e1ad0, along
with checkpoint text that still described already committed completion work.
Those actual corrections were inspected; historical preparation sections keep
their stage attribution. Source-head checks are not transferred to the metadata
or this review commit. Fresh latest-head CI, ready PR, resolved discussions,
current rules/required approvals, normal exact-head merge and refreshed main
remain root publication gates before a separate STORAGE-013 claim.

Actual independent checks: inline Python raw/metadata/hash/ancestry/state/map
validation passed; fresh GitHub readbacks matched the cached historical run;
`python3 tools/check_project.py` passed391 tasks/122 decisions;
`python3 tools/progress.py render --check` and `git diff --check` passed. Initial
review-script assumptions about inline slot syntax/map collection were corrected
before the successful complete parse; no repository artifact changed for them.
Only this review appendix was authored here. Prior native results remain attributed
above; no native or red rerun and no actual browser validation were performed.
