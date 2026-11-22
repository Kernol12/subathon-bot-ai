# STORAGE-003 #318 structural Stage 1 handoff under 182 A

## Fixture-source review

Tier A test authoring. Protected base `e451293a4bc9a6faf8b11bfaa74714f15e85d9c4`,
tree `34367e45f5f1073f10607a769ec31e644f3c8d01`. Author source
`d016499dc4d8194541a54b13f85d602500f2d310`, tree
`d45105edeb6896b2f163f18308c2b46b112e0a7f`. The assigned branch was a clean
ancestor and fast-forwarded to the exact protected pin before edits.

| Frozen review input | SHA-256 |
| --- | --- |
| Structural fixture brief fix2 | `2a9da1356d1b1af0edb113b130371e53a91babeb9af8145d3278c2e7755f2b76` |
| Structural fix2 closing review | `0f902da146148a7e8d6fdc4a683c4f799ac3b0798ccf001d9b22df574493ed50` |
| Writer/catalog prerequisite fix2 | `1ea6643dd0246da5f57aa667293a670111ef250dc920ce2115abe854e0c126dd` |
| Static catalog JSON | `881160604889628783927d4b1b3e3948c1fd3f6740a05b94c11b2a23c5d31cc2` |

The source commit owns the two Qt test objects and their local fixture files, plus
the private `server/src/sqlite_clock_checkpoint.{hpp,cpp}` declarations and
effect-free unimplemented returns. The repository issue read was unavailable from
this isolated checkout. The coordinator’s frozen packets supplied the assignment
and approved fixture inputs.

### First stops and dependent checks

| Case | Authored selector or fixture | First stop and next gate |
| --- | --- | --- |
| U1 compiled catalog | All 22 independently sealed digests, starts 0–22, raw digest prefixes, exact v22 DDL and NULL autoindex, v21/v22 counts and v22 payload width. | `clock_checkpoint_manifest` returns `feature_not_implemented`. Compiled readback and tuple comparison remain pending. |
| U1 Ready and blank | Real protected v21 Ready retention or an empty file, pre-open and matching stopped backup, exact-limit advance, normal reopened inspection and typed absence/readback. Blank has five one-less controls for the shipped writer limits. | `probe_clock_checkpoint_predecessor` is the first stop. Next are the compiled manifest and guarded advance. Combined same-unit empty-witness tracing remains pending. |
| U1 populated | Separate owner-produced Finished/SAR2, milestone, application/holds/economics, and intake/rate/modifier specimens. | Pending producer assembly before pre-open. The protected `storage_027::Populated` pattern has a still-open `CatalogFile` with a private path, and covers only part of this closure. Supply actual stopped owner files using those producer patterns, including committed v21 interruption, jobs and attempts, and preserve each incompatible history in its own file. Review Main 99, Grace 120,000, debt 7, score 90 to 105, count 3, SAR2 ordinals (1,1)/(3,2) and the fixed suffix. |
| U2 Active and changed basis | Genuine v21 Active refusal before opening the writer. Isolated Ready Z echo and independently produced Active at in-unit recheck. Old reader verifies unchanged A/K. | Pre-open first. Then the guarded advance must refuse before DDL, after normal inspection and typed compare. |
| U2 missing K and bad marker/digest | C2 selects every FK child of K and the exact A/rowid 1 on the intact v21 owner file. The closed orphan preparation deletes only K with FK off and restores FK before close. Independent read-only inspection proves unchanged A and exact sole violation. Each damaged history control has its own copy. | Pre-open is the first missing entry. C1 requires the real sixth-query failure, 83-byte row and no typed guard. Probe-ledger readback remains pending. |
| U3 cancellation, drift and restore | Real guarded advance request cancels at `before_history` after all 15 DDL. Reopen sees exact v21. Separate committed copies alter the fixed index or digest. Old build refuses exact v22 and succeeds on a separate restored backup. | Pre-open first, then compiled catalog and guarded executor. Protected `allowed_writer_sql` rejects semicolons, including those inside the five trigger definitions. This is an earlier admissibility stop if reached. It earns no DDL, rollback-at-history, or W-case credit. The executor needs scoped compiled-only DDL admission before the intended `before_history` cut. |
| W1 normal owner and physical checks | Typed K/A through the declared owner entry, exact seed slot/root/head and independent reopen. Separate single-owner constraints enforce head-first, missing-head COMMIT, occupied duplicate/REPLACE/UPSERT/DELETE and slotless head UPDATE. Unreferenced evidence mutations only occur in rolled-back specimen units. | Compiled catalog first in each W1 selector, then pre-open and guarded advance. Normal initial stage is the next stub for positive and occupied-copy cases. The missing-head physical specimen supplies only K/A directly and needs no positive initial stage. |
| W1 runtime denials | Existing batch and work entry points, fixed forbidden statements, no success, unchanged seed readback. | Earliest compiled catalog as above. Bind the future pre-prepare counter and alien/stale-session controls after the private governed recipe gate exists. A SQL constraint or closed-capability error does not prove recipe admission. |
| W2 deferred-FK | Fixed typed mismatch oracle has B head, A slot and seed root, 298 bytes. Real rollback closure is A/1/1 with Main 10,000. | After compiled catalog, guarded advance and W1 initialized basis, bind the actual OriginalApplicationService / #312 `Access::stage` selected retained command and configured clock ports. Stage D=9,750 and B=9,887 from 250 and +137. One real affected head row, then mismatched readback, then real COMMIT FK error (787 where exposed) and acknowledged rollback. Needs the real stage hook and adapted owner, with no pre-COMMIT FK check. |
| W3 normal and late abort | Fixed B/2/2 slot/applied root oracle and A/1/1 rollback oracle. | Same real producer as W2. Count both CAS changes separately. Pin actual keyed receipt, original claim, applied contribution condition, effect and positive Main component, exact output/result/audit and no transition/job. Compare historical D and B and raw/re-encoded bytes against an independently fixed service fixture. After both CAS checks, the separate late-abort arm repeats the exact root INSERT with existing staged key and bytes. Real constraint plus rollback must leave old closure. |
| W4 lost acknowledgements and mixed closure | W2 and W3 typed slot/root oracles are authored. | Needs genuinely produced W3 and W2 copies. Arm the existing `SqliteWriterWorkAccess` while idle. Losing a successful COMMIT acknowledgement revokes the writer and yields unknown without a value. Throw after an acknowledged commit for committed/work_exception without a value. Lose the successful rollback acknowledgement after the real W2 FK refusal for unknown and exact old closure. Compare reopened effects, result/audit and root before reconciliation. Test denied disclosure and conflicting replay. Use separate copies for missing audit, result octet flip, root op-ref Z, or before-ref B at root byte offset 20. No such mixed copy or W4 outcome has been authored as an executable test. |
| W5 fixed structural replacement | B/3/2 keeps the revision-2 root. B/3/3 appends root S/M with B/B and 2/2→3/3. Both preserve immutable B=9,887 and the owning operation/audit. | Needs actual W3 predecessor and typed checkpoint selector/CAS stage. Independent stale-generation, stale-revision, and historic-base predicates each require an exact miss. A same-base semantic revision invalidates the old revision even when countdown agrees. Both readbacks and no extra append on the generation-only path remain pending. |

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
| Ready A/K cancellation | 22 / 2684 / 8772 | 0 / 0 | 301 / 3471 / 330206 |
| Ordinary exact v21 inspect | 12 / 187 / 1322 | 0 / 0 | 600 / 6936 / 660360 |
| Ordinary exact v22 inspect | 12 / 187 / 1322 | 0 / 0 | 634 / 7334 / 693858 |
| v22 inspect plus same-unit empty witness | 13 / 187 / 1472 | 0 / 0 | 635 / 7335 / 693866 |

For wider admitted legacy refs add their byte sum minus two to the Ready A/K
guard result and to 21→22 advance. The matrix for other supported starts is
`static-migration-budget-fix2.csv`. BEGIN, COMMIT and ROLLBACK charge separate
control attempts and acknowledgements. Successful normal inspect has two
inspection passes in one work unit. Pre-open has a coherent deferred one-pass
read with FK enabled and has no normal writer or WAL setup. The written independent
empty read is a separate read after inspection. Claim the combined (13)-attempt
selector only from the future assembled same-unit trace.

W1 raw counterpart charges one mutation statement in its control transaction.
The fixed missing-head unit charges two successful stage statements, one COMMIT
attempt and one acknowledged rollback. A runtime recipe denial needs zero
prepared operations for its denied statement and no row; a governed batch
rejection also needs zero controls. Independent absence probes still count as
attempts. W2–W4 charge existing retained source/plan/owner reads, D/B insertion,
scoped operation, claim, contribution condition, effect/component, result/audit,
root, head and slot CAS as applicable, each immediate change count and each
readback, and separate independent reopen. W5 charges current basis and selected
root, slot CAS and exact one-cell/eight-byte change result, then exact readback
and historical lookup. Its semantic branch additionally charges root insertion.
The 21-cell joined projection is 168 fixed + seven one-byte returned refs + 65 +
58 = 298 bytes, including the W2 mismatch. Exact failed-CAS and terminal absence
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

`assembly.patch` is the coordinator’s separate registration proposal. It has
passed `git apply --check` against the fixture-source commit. It is unapplied.
The flag defaults OFF. The proposal adds exactly two whole-runner Qt registrations
with an intentional-red label and makes no changes to any existing suite or CI.

The protected link has no ordinary checkpoint owner. Each isolated runner gets
one object from the private checkpoint TU. Once an ordinary owner is introduced,
replace that object throughout the fixture link, including transitive archives.
Never rely on archive extraction order to hide duplicate definitions. Seal the
actual test and ordinary link commands, full map and symbol inventory. Check
exactly one defining object per owner entry in each runner, no ordinary owner
object/member in the fixture, and zero test access symbols in the ordinary target.

The pending W2/W3 adapter belongs to the fixture target and the same owner TU.
Its only arm values are `omit_slot_follow` and `repeat_root_append`. Use an
owner-thread-bound, noncopyable lease minted by the fixture for its disposable
writer/file, matching its expected fixture identity and exact S/Q app-operation
key. Take no caller path, SQL, connection, callback or transferable ticket.
One scope consumes the arm once at the real named stage and records real
observations. Leave other work unchanged. Neither arm owns transaction controls.
Compile the implementation of that access only in the private fixture variant
once the actual transition exists. The present stub and link proposal contain
no simulated arm or access symbol.

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

Source-only checks performed. Packet and static catalog hashes matched. A local
static framing pass matched all 22 digests, the complete ordered v22 SQL block,
payload size and digest prefix. The 65-byte slot and three 58-byte roots were
decoded from literals. Git whitespace and patch applicability checks were run.
#238 retained exclusive runtime. Configure, compilation, discovery, SQLite
execution, independent test review and meaningful intended red remain pending.

Ready→Start, U1F, U1R, W5-live, W6, R1, CORE-006/#720, split pause lanes, live
Review composition and #294 qualification/deployment remain gated. U1F uses the
existing correction for `Populated(99,7)` of Grace 120,000 and debt 7.
