# STORAGE-003 #318 structural Stage 1 handoff under 182 A

## Fixture-source review

Tier A test authoring, source fix round 1 of 2. The protected base is
`e451293a4bc9a6faf8b11bfaa74714f15e85d9c4`, tree
`34367e45f5f1073f10607a769ec31e644f3c8d01`.

| Revision | Commit | Tree |
| --- | --- | --- |
| Independently reviewed fixture source | `d016499dc4d8194541a54b13f85d602500f2d310` | `d45105edeb6896b2f163f18308c2b46b112e0a7f` |
| Prior unapplied proposal | `7bef5b89f99f50c4c759755d98aaf0af51df61c4` | `90f5f01bf4fda7f0b3c47f88956226f338bc03bf` |
| Revised fixture source for separate verification | `a33871d6a13b598337848876c78dece7c8ec3103` | `1216216282467bbab5501696bb8dfc9fb2a6f94b` |

| Frozen review input | SHA-256 |
| --- | --- |
| Structural fixture brief fix2 | `2a9da1356d1b1af0edb113b130371e53a91babeb9af8145d3278c2e7755f2b76` |
| Structural fix2 closing review | `0f902da146148a7e8d6fdc4a683c4f799ac3b0798ccf001d9b22df574493ed50` |
| Writer/catalog prerequisite fix2 | `1ea6643dd0246da5f57aa667293a670111ef250dc920ce2115abe854e0c126dd` |
| Independent fixture-source initial review | `78b30281586a04738e8780e15023b182cdabc85c1e13a53837cb0a9bc47019d5` |
| Static catalog JSON | `881160604889628783927d4b1b3e3948c1fd3f6740a05b94c11b2a23c5d31cc2` |

The source commit owns the two Qt test objects and their local fixture files, plus
the private `server/src/sqlite_clock_checkpoint.{hpp,cpp}` declarations and
effect-free unimplemented returns. The repository issue read was unavailable from
this isolated checkout. The coordinator’s frozen packets supplied the assignment
and approved fixture inputs.

### Fix-round disposition

- **S1** The private probe takes `StorageLimits` and returns an optional
  actual eight-field ledger and terminal query. Exact-limit tests require both
  observed fields even on a refusal. Four one-less controls exercise attempts,
  peak SQL, returned rows and value bytes. The probe has no bindings, so it
  has no lower positive binding-byte edge. The in-unit 114A path requires a
  separately attained preflight ledger and has four guard-stop controls.
  C2 retains all seven child selectors and the exact A at rowid 1.
- **S2** The W1 writer uses the same nonmutating empty-additions SELECT as U1.
  The row is 0 on the occupied W1 seed and 1 on the empty U1 relations. Denied
  statements are governed writes, ordinary-work DDL, a writable PRAGMA, and an
  unsupported verb. The latter two are already refused at the protected pin
  and earn regression coverage only. The fixture observation entry is defined
  only in the private owner variant and currently supplies no counters.
- **S3** The protected statement-11 trigger cut is listed below for every
  dependent selector. Its owning migration/writer admission change requires
  separate Tier A review.
- **S4** Before Ready advance, the protected owner selects all 21 history rows
  in order and compares INTEGER versions, TEXT IDs and raw 32-byte BLOB hashes
  to the static oracle. The same query on an independently reopened Ready and
  blank successor requires all 22 exact rows. The old backup reader compares
  the 21-row oracle again.

### First stops and dependent checks

| Case | Authored selector or fixture | First stop and next gate |
| --- | --- | --- |
| U1 compiled catalog | All 22 independently sealed IDs and digests, starts 0–22, raw digest prefixes, exact v22 DDL and NULL autoindex, v21/v22 counts and v22 payload width. | `clock_checkpoint_manifest` returns `feature_not_implemented`. Compiled readback and tuple comparison remain pending. |
| U1 Ready and blank | Real protected v21 Ready retention or an empty file, pre-open and matching stopped backup, exact-limit advance, normal reopened inspection and typed absence/readback. Ready and blank each have five one-less writer limits, and exact reopened history. | `probe_clock_checkpoint_predecessor` is the first stop. Next are the compiled manifest, guarded advance and statement-11 prerequisite. Combined same-unit empty-witness tracing remains pending. |
| U1 populated | Separate owner-produced Finished/SAR2, milestone, application/holds/economics, and intake/rate/modifier specimens. | Pending producer assembly before pre-open. The protected `storage_027::Populated` pattern has a still-open `CatalogFile` with a private path, and covers only part of this closure. Supply actual stopped owner files using those producer patterns, including committed v21 interruption, jobs and attempts, and preserve each incompatible history in its own file. Review Main 99, Grace 120,000, debt 7, score 90 to 105, count 3, SAR2 ordinals (1,1)/(3,2) and the fixed suffix. |
| U2 Active and changed basis | Genuine v21 Active refusal before opening the writer. Isolated Ready Z echo and independently produced Active at in-unit recheck. Old reader verifies unchanged A/K. | Pre-open first. Then the guarded advance must refuse before DDL, after normal inspection and typed compare. Only the dedicated Active/changed-basis dispositions are fixed for these semantic refusals. The frozen brief assigns no separate generic `MigrationReason` value. |
| U2 missing K and bad marker/digest | C2 selects every FK child of K and the exact A/rowid 1 on the intact v21 owner file. The closed orphan preparation deletes only K with FK off and restores FK before close. Independent read-only inspection proves unchanged A and exact sole violation. Each damaged history control has its own copy. | Pre-open is the first missing entry. C1 requires the real sixth-query failure, 83-byte row and no typed guard. Exact-limit assertions cover the FK and both history refusals. Attained readback awaits the real probe. |
| U3 cancellation, drift and restore | Real guarded advance request cancels at `before_history` after all 15 DDL. Reopen sees exact v21. Separate committed copies alter the fixed index or digest. Old build refuses exact v22 and succeeds on a separate restored backup. | Pre-open first, then compiled catalog and guarded executor. Then the protected statement-11 cut precedes the cancellation, and prevents creating a successor for drift, old-reader refusal and restore controls. |
| W1 normal owner and physical checks | Typed K/A through the declared owner entry, exact seed slot/root/head and independent reopen. Separate single-owner constraints enforce head-first, missing-head COMMIT, occupied duplicate/REPLACE/UPSERT/DELETE and slotless head UPDATE. Unreferenced evidence mutations only occur in rolled-back specimen units. | Compiled catalog first in each W1 selector, then pre-open, guarded advance and the statement-11 cut. Normal initial stage is the next stub for positive and occupied-copy cases. The missing-head physical specimen supplies only K/A directly and needs no positive initial stage. |
| W1 runtime denials | Existing batch and work entry points, fixed forbidden statements, no success, unchanged seed readback. | The compiled catalog, pre-open, guarded advance, statement-11 cut and positive initial stage all precede this case. Actual batch/work recipe outcomes precede the pending fixture counter check. A SQL constraint or closed-capability error does not prove recipe admission. Alien/stale-session controls remain pending. |
| W1 joined-read budget | Real writer SELECT with one result row and 298 bytes succeeds. The same one-row projection under 297 bytes must return `result_limit` with acknowledged rollback. The bounded head predicate remains in the LEFT JOIN condition. | The compiled catalog, pre-open, guarded advance, statement-11 cut and initialized owner seed precede a real measured read. |
| W2 deferred-FK | Fixed typed mismatch oracle has B head, A slot and seed root, 298 bytes. Real rollback closure is A/1/1 with Main 10,000. | After compiled catalog, guarded advance and W1 initialized basis, bind the actual OriginalApplicationService / #312 `Access::stage` selected retained command and configured clock ports. Stage D=9,750 and B=9,887 from 250 and +137. One real affected head row, then mismatched readback, then real COMMIT FK error (787 where exposed) and acknowledged rollback. Needs the real stage hook and adapted owner, with no pre-COMMIT FK check. |
| W3 normal and late abort | Fixed B/2/2 slot/applied root oracle and A/1/1 rollback oracle. | Same real producer as W2. Count both CAS changes separately. Pin actual keyed receipt, original claim, applied contribution condition, effect and positive Main component, exact output/result/audit and no transition/job. Compare historical D and B and raw/re-encoded bytes against an independently fixed service fixture. After both CAS checks, the separate late-abort arm repeats the exact root INSERT with existing staged key and bytes. Real constraint plus rollback must leave old closure. |
| W4 lost acknowledgements and mixed closure | W2 and W3 typed slot/root oracles are authored. | Needs genuinely produced W3 and W2 copies. Arm the existing `SqliteWriterWorkAccess` while idle. Losing a successful COMMIT acknowledgement revokes the writer and yields unknown without a value. Throw after an acknowledged commit for committed/work_exception without a value. Lose the successful rollback acknowledgement after the real W2 FK refusal for unknown and exact old closure. Compare reopened effects, result/audit and root before reconciliation. Test denied disclosure and conflicting replay. Use separate copies for missing audit, result octet flip, root op-ref Z, or before-ref B at root byte offset 20. No such mixed copy or W4 outcome has been authored as an executable test. |
| W5 fixed structural replacement | B/3/2 keeps the revision-2 root. B/3/3 appends root S/M with B/B and 2/2→3/3. Both preserve immutable B=9,887 and the owning operation/audit. | Needs actual W3 predecessor and typed checkpoint selector/CAS stage. Independent stale-generation, stale-revision, and historic-base predicates each require an exact miss. A same-base semantic revision invalidates the old revision even when countdown agrees. Both readbacks and no extra append on the generation-only path remain pending. |

### Protected v22 statement-11 cut

Once the manifest, pre-open and guarded advance seams exist, the pinned
`allowed_writer_sql` rejects the semicolon in v22 statement 11,
`CREATE TRIGGER subathon_clock_head_no_delete`, before prepare. The work reason
is `invalid_statement`, mapped to migration `sql_failure`. Statements 12–15
also contain semicolons. This setup cut blocks U1 Ready/blank advance and the
unreached populated upgrade, U3 cancellation at `before_history`, all U3
successor drift/old-reader/restore assertions, and every W1 selector through
`advance_to_22`. W2–W5 depend on the resulting governed setup.

The Ready and blank one-less advance cases that reach statement 11 are blocked
there too. The reduced peak-SQL rows have earlier legitimate resource cuts,
at v22 statement 3 for Ready and v6 statement 2 for blank. The other advance
edges belong at the v22 history INSERT (binding bytes) or final empty witness
(attempts, returned rows and value bytes). U2 probe and pre-DDL in-unit
refusals never require trigger admission.

The migration executor and single writer need scoped compiled-only admission
for the exact reviewed migration statements as a separate Tier A prerequisite.
No v22 trigger admission, SQL policy, or executor change is in this fix. The
statement-11 cut earns no intended-red or downstream credit.

An earlier unimplemented entry is the only candidate for intended red on its own
assertion. Compiler, plugin, unexpected meter, invalid fixture and linked-source
failures are setup failures. Pending W2–W5 statements carry no executable-case
or red credit. Review the original service ports and real owner seam before adding
W2–W4 bodies. The W1 literal initial checkpoint is structural and supplies no
proof of the application service’s admissible clock or retained command inputs.

### Budget and selector ledger

C1 expects **6 / 187 peak / 661 total / 0 binding cells / 0 binding bytes /
301 rows / 3,472 cells / 330,263 result bytes**. The FK row is TEXT child
`subathon_app_states` (27), INTEGER rowid 1 (8), TEXT parent
`subathon_retained_configurations` (40), INTEGER fkid 0 (8). The healthy
first pass is 300 rows, 3,468 cells and 330,180 bytes. C2 counts one matching
state A and zero hold causes, intake entries, receipt attempts, receipts,
initial captures and retained contexts. There is no guard statement after the
violation. Each prepare/reopen control is a separate unit and is excluded from
this ledger.

| Unit | Attempts / peak SQL / total SQL | Bindings cells / bytes | Result rows / cells / bytes |
| --- | --- | --- | --- |
| Blank pre-open | 4 / 187 / 312 | 0 / 0 | 2 / 2 / 23 |
| Ready/Active A/K pre-open or separate in-unit 114A refusal | 7 / 1586 / 2247 | 0 / 0 | 301 / 3471 / 330206 |
| Blank 0→22 | 198 / 8804 / 142915 | 69 / 1800 | 3239 / 38698 / 4223254 |
| Ready A/K 21→22 | 31 / 2684 / 9865 | 3 / 76 | 911 / 10643 / 1020621 |
| Missing K pre-open, FK refusal | 6 / 187 / 661 | 0 / 0 | 301 / 3472 / 330263 |
| Wrong v21 marker pre-open, history refusal | 4 / 187 / 567 | 0 / 0 | 299 / 3467 / 330126 |
| Wrong v21 digest pre-open, history refusal | 4 / 187 / 567 | 0 / 0 | 299 / 3467 / 330170 |
| Old-inspector refusal or wrong v22 digest | 4 / 187 / 567 | 0 / 0 | 316 / 3666 / 346919 |
| Extra v22 index projection refusal | 4 / 187 / 567 | 0 / 0 | 317 / 3678 / 347271 |
| Ready A/K cancellation | 22 / 2684 / 8772 | 0 / 0 | 301 / 3471 / 330206 |
| Ordinary exact v21 inspect | 12 / 187 / 1322 | 0 / 0 | 600 / 6936 / 660360 |
| Ordinary exact v22 inspect | 12 / 187 / 1322 | 0 / 0 | 634 / 7334 / 693858 |
| v22 inspect plus same-unit empty witness | 13 / 187 / 1472 | 0 / 0 | 635 / 7335 / 693866 |

The one-less probe terminal queries are fixed in attempts, peak SQL, rows and
value-bytes order. Blank stops at FK, schema, integrity, integrity. Ready and
Active stop at the legacy guard in all four. Missing K stops at FK, schema, FK,
FK, with `resource_limit` on each. Both altered v21 history copies stop at
history, schema, history, history. The wrong marker is 22 bytes shorter and
appears twice in the history result, yielding a 44-byte difference. The fixed
extra index adds one 12-cell schema row charging 352 bytes. Exhaustion controls
require a real attached ledger and a typed terminal query. Partial failed-row
accounting is not numerically pinned in the frozen brief.

For wider admitted legacy refs add their byte sum minus two to the Ready A/K
guard result and to 21→22 advance. The matrix for other supported starts is
`static-migration-budget-fix2.csv`. BEGIN, COMMIT and ROLLBACK charge separate
control attempts and acknowledgements. Successful normal inspect has two
inspection passes in one work unit. Pre-open has a coherent deferred one-pass
read with FK enabled and has no normal writer or WAL setup. The written independent
empty read is a separate read after inspection. Claim the combined 13-attempt
selector only from the future assembled same-unit trace. The ordinary inspector
uses the actual early-refusal limits above. Exact statement and cell counters
for that inspector and the full advance/cancel still require assembled tracing.

W1 raw counterpart charges one mutation statement in its control transaction.
The fixed missing-head unit charges two successful stage statements, one COMMIT
attempt and one acknowledged rollback. A runtime recipe denial needs zero
prepares and one actual BEGIN for its denied work; a governed batch needs zero
prepares and zero BEGIN attempts. The positive SELECT proves the same counter
advances. Trace absence carries no evidence. Independent absence probes still count as
attempts. W2–W4 charge existing retained source/plan/owner reads, D/B insertion,
scoped operation, claim, contribution condition, effect/component, result/audit,
root, head and slot CAS as applicable, each immediate change count and each
readback, and separate independent reopen. W5 charges current basis and selected
root, slot CAS and exact one-cell/eight-byte change result, then exact readback
and historical lookup. Its semantic branch additionally charges root insertion.
The 21-cell joined projection is 168 fixed + seven one-byte returned refs + 65 +
58 = 298 bytes, including the W2 mismatch. The W1 typed read checks 298 versus
297 through the actual writer value meter. The W2 mismatch meter stays pending. Exact failed-CAS and terminal absence
queries also charge statement text. Root columns and order are fixed in source.
Per-owner bind cells/text/replays and all cumulative meters beyond the five
existing `StorageLimits` dimensions need the assembled recipe inventory and
exact-fit/one-less controls before admission.

Selector rebind review must cover terminal partition, milestone detail and
interruption, financial catalog, modifier activations, rate overrides, Review
journal, and owning bundle/source/intake/capture readers behind exact catalog
admission. Inspect and admit the whole exact v22 schema first. The Review selector
must require both exact binary tail markers and 32-byte digests in the same query,
exclude only versions 21 and 22, and retain exactly 20 typed prefix rows plus an
overflow witness. Its 640 entry + 69 structural + 59 authority = 768 reservation
is already full. Preserve the 512 row-unit and retained-authority ceilings as
well. #750 owns shared read bounds. #751 owns Review completion. The current v10
host selection remains #294 work.

## Assembly review

`assembly.patch` is the unchanged registration proposal re-pinned to revised
source `a33871d6a13b598337848876c78dece7c8ec3103`. SHA-256
`abaef7e02bdcc4ec4d7d56921b9f6466329ae2befdd71ac372ecb1af6452f595`.
It passed `git apply --check` against that commit and remains unapplied. The flag
defaults OFF and the patch adds exactly two whole-runner Qt registrations with an
intentional-red label. Existing suite definitions and CI retain their bytes.

The protected link has no ordinary checkpoint owner. Each isolated runner gets
one object from the private checkpoint TU. Once an ordinary owner is introduced,
replace that object throughout the fixture link, including transitive archives.
Never rely on archive extraction order to hide duplicate definitions. Seal the
actual test and ordinary link commands, full map and symbol inventory. Check
exactly one defining object per owner entry in each runner, no ordinary owner
object/member in the fixture, and zero test access symbols in the ordinary target. The revised fixture variant
must define `clock_checkpoint_fixture_trace` exactly once in each runner. Confirm
that the returned counter covers the actual writer instance and observations
of prepare and BEGIN in the approved assembly.

The pending W2/W3 adapter belongs to the fixture target and the same owner TU.
Its only arm values are `omit_slot_follow` and `repeat_root_append`. Use an
owner-thread-bound, noncopyable lease minted by the fixture for its disposable
writer/file, matching its expected fixture identity and exact S/Q app-operation
key. Take no caller path, SQL, connection, callback or transferable ticket.
One scope consumes the arm once at the real named stage and records real
observations. Leave other work unchanged. Neither arm owns transaction controls.
Compile the implementation of that access only in the private fixture variant
once the actual transition exists. The present stub has no simulated arm and provides no observed counts.

The coordinator should seal the source, opt-in diff, runner, QSQLITE plugin,
loader selection and maps before execution. Read the actual compiled manifest
through the private factory in the admitted build. Compare complete ordered SQL,
every sorted typed schema tuple with actual SQL NULL, and starts 0–22 directly
against the sealed static JSON. Read back physical per-step schema/history on
admitted files and distinguish true NULL from empty TEXT/BLOB. Independently
frame the bytes, verify the 22 raw per-step hashes, check v21 object count 276,
v22 object count 292 and payload 165,839 and hash the raw-digest prefix. Static
reframing verified the reviewed oracle, while compiled/physical comparison
remains pending. Record the compiled exact budget/selector traces separately
and preserve the SQL error and control acknowledgements from genuine actions.

Fix-round checks were source-only. The five input SHA-256 values matched.
Offline derivation checked every history ID and digest, sealed v22 DDL and
exact trigger locations, all seven C2 child selectors, the exact ledger
tuples, and one-less terminal-query ordinals. Both staged diffs passed
whitespace checks. The assembly patch passed applicability on the revised
source without being applied. #238 retained the runtime. The compiler, Qt NULL
and driver error-code behavior, link maps, compiled/physical manifest readback,
per-owner traces and runner admission remain pending. Independent source
verification, closing review, independent test review and intended red also
remain pending.

Ready→Start, U1F, U1R, W5-live, W6, R1, CORE-006/#720, split pause lanes, live
Review composition and #294 qualification/deployment remain gated. U1F uses the
existing correction for `Populated(99,7)` of Grace 120,000 and debt 7.
