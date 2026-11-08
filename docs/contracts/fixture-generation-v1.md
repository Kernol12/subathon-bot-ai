# Independent fixture generation — revision 1

Task: FIXTURE-001 / FIXTURE-002, under TESTCLEAN-006 / ADR-0012 and specification v0.45 §17.
Status: reviewed contract/test baseline; storage source and execution gates remain pending.
This developer tool reproduces independent test data. It establishes no running
server, account/session storage, authority, provider or platform capability.

## Command, selection and paths

```text
python3 tools/generate_fixtures.py (--check | --output-dir DIR) [--fixture NAME ...]
```

Use Python>=3.10 standard library only. Exactly one operation mode is required:
no mode or both modes is a usage error. Unsupported names are usage errors, not a
request to copy an existing fixture. Usage errors exit2 before recipe/file work.
The supported families and fixed filenames after the FIXTURE-002 extension are:

| Name | Filename |
| --- | --- |
| reference_capture | sqlite_reference_capture_vectors.json |
| account_sessions | sqlite_account_sessions_vectors.json |
| bundle | sqlite_application_bundle_vectors.json |
| holds | sqlite_application_holds_vectors.json |
| original_claims | sqlite_original_claims_vectors.json |

No `--fixture` selects all currently supported families. Repeated selections are
deduplicated; processing follows the table order, independent of argument order.
Later families require explicit reviewed registration under their own tasks.
There is no search/fallback for an unsupported family or missing source recipe.

The project root is the parent of the directory containing this resolved script.
Seeds and check comparisons live beneath that root's `tests/fixtures`, independent
of working directory. There is no seed-root option or environment fallback. A
relative explicit output directory is relative to the caller's working directory.

## Allowed independent inputs

Only these four existing seed files are recipe file inputs. Verify all four raw
file SHA256 pins before parsing; a missing/changed seed refuses exit1 before writes.
Use only `manifest.version`, `id`, ordered `sql` and full `expected_schema` literals.
Other fixture fields cannot drive a recipe. Any seed-corpus change needs separate
oracle/contract review, not an automatic pin refresh.

| Seed filename | Approved raw SHA256 |
| --- | --- |
| retained_context_vectors.json | 631260c6050c6e47c36ef69fbf6fecf67016407b7f9cf93d6b3246b9438f1dcf |
| sqlite_receipts_vectors.json | 350e2f22db39661c7966996f8d7ae5da12fad91b929605280ff7930d1bec7e8a |
| sqlite_contributions_vectors.json | 86eadfdf53367e7b3d7ae27f1885483aad9e8183a6b759e40ad526ae1d33ff23 |
| sqlite_application_plans_vectors.json | 9a603917d75019b3fe18c8dd6e7dbb05386d48f64b90ee340f5d2d66d21b968a |

The six large output fixtures are NEVER recipe inputs. Candidate code may read
selected output files only for explicit check comparisons after independently
building their bytes. Source recipe functions supply later-family data. No imports
or reads of production code/headers, project binaries, git output, Qt, SQLite,
libsodium/provider execution or observed database results may create expected values.
No generic JSON patch language, SQL interpreter, application/domain simulator or
giant literal copy of legacy output is an independent source recipe.

Independent literal catalog declarations, stdlib F/UTF-8/byte packing and hashlib
may follow the accepted [migration framing](migration-catalog-v1.md) and
[reference grammars](reference-application-adapters-v1.md). SQL is exact literal
text, never parsed/normalized/executed to derive projected schema. NULL index facts
are independently declared. Expected semantic state, query rows and byte-count
anchors stay independently supplied, not calculated by a product model.

## Output and check semantics

`--output-dir DIR` computes and validates every selected output before file writes,
creates the explicit directory if necessary, and overwrites ONLY the selected fixed
filenames. Unselected and unrelated files remain byte-identical. There is no default
in-place write, retry, multi-file crash-atomicity or process/resource-isolation promise.

Serialization is UTF-8 with `ensure_ascii=False`, `indent=2`, explicit insertion
order and one final newline. The sole legacy-layout exception is holds: the two
named arrangements.pending.setup and arrangements.timer.setup arrays retain each
statement object as one compact JSON line at their existing indentation, with
separators=(',', ':'). Shared rendering still validates all values first. Every
other field/family retains the common layout. The CLI explicitly declares each
family's serializer beside its recipe; the four other families retain common
rendering. No NaN/Infinity/floats are permitted. Preserve all
strings, embedded hex bytes, arrays, types and NULLs exactly; do not normalize text.
Reference/account already have this formatting and must reproduce their original
raw bytes as well as their canonical typed contents.

`--check` independently generates selected bytes and compares them byte-for-byte
with committed selected output files. Success exits0. Missing/different output is
drift and exits1 with an actionable family/path diagnostic. Seed, recipe or I/O
failure also exits1. Check mode writes no fixture/recipe/source files, on success
or refusal; recipe imports do not create bytecode artifacts. It never repairs or
falls back to copying comparison files. Exact private error wording is not frozen.

These are ordinary developer-tool guarantees, not a hostile-input package/security
boundary. Candidate stubs initially fail with reachable named FeatureNotImplemented
markers; those failures are intended-red evidence, not implementation capability.

## Independent expected content

Fingerprints were frozen before candidate authoring in the independently reviewed
[parent task](../tasks/EPIC-028/TESTCLEAN-006.md). Canonical typed JSON uses
`sort_keys=True`, `separators=(',', ':')`, `ensure_ascii=False`, `allow_nan=False`,
UTF-8 and no trailing newline. That comparison ignores only file whitespace/key
order. Candidate output never supplies its own expected fingerprint.

| Family | Original raw-file SHA256 | Canonical typed-content SHA256 |
| --- | --- | --- |
| reference_capture | 48ddd9cd81bfa8b39b61a994a266cda7a68e356facfcda95be8a4c499876ba64 | 4a3416559d4196f14dcd18506d1faf3a4e53e93a260294f4a52ad414cbb02b55 |
| account_sessions | 2e7c373a3bd526c770b85a2b875950820ac4b04db7d35e61849aa02025e7c919 | 8e4e7030349f4fa79276b886ccedfb5859b1868a9d4f7e06192cc2dd8d278fa2 |

Preserve all reference A/B documents, ordered intake/capture/index statements,
26 named queries and 23 constraint refusals; all manifest/prefix framing/projection
pins; account verifier/digest/snapshot literals, five source-byte counts, four base
SQL statements and 17 constraints. Account coordinate8 remains proposed/unimplemented;
its 44 feature bodies stay passive red. No public codec/error/order/budget/authority
or unique rollback/idempotency/ownership assertion is removed.

## Concrete examples

| Invocation/state | Required outcome |
| --- | --- |
| `--output-dir generated` from an unrelated cwd | All five reviewed outputs in cwd/generated, using only script-relative seeds/source recipes; six legacy outputs can be absent |
| `--fixture reference_capture --output-dir selected` | Reference is created/overwritten; existing account/unrelated sentinel bytes stay unchanged |
| `--check` with all five exact committed comparison files | exit0 and no writes |
| `--check` after only a newline is appended to reference output | exit1 drift and no repair, even though typed JSON is unchanged |
| `--check` after whitespace is appended to a seed | exit1 raw-pin refusal/no writes; valid JSON does not bypass the immutable input pin |
| `--fixture application_service --output-dir generated` | exit2 unsupported, no fallback or output writes |
| no mode, or both `--check --output-dir generated` | exit2 usage error/no writes |

## Review and proof limits

Authored reproduction/drift tests and actual inert interfaces require independent
review before execution. Observe the reachable intended red before implementation;
missing imports/files/dependencies, parse or compile errors are invalid red. A
separate source/oracle execution gate precedes real generation and adoption.
Expected goldens cannot be refreshed from a candidate run to make tests pass.

When selected generated fixture files and native source stay byte-identical, preserve
valid prior local Qt evidence instead of replaying native bodies for receipts.
Current hosted selected-Qt CI must still pass normally. Record actual CLI/tooling/CI
proof separately from prior local Qt and unrun platform/provider/full-product work.


## FIXTURE-002 storage-family extension proposal (revision 1)

This section is the storage-family contract/inventory, before authored tests or
implementation. It does not approve execution. Existing FIXTURE-001 behavior,
source recipes and all four raw seed pins remain unchanged. Registration appends
these families in this exact order after reference_capture/account_sessions:

| Name | Filename | Source |
| --- | --- | --- |
| bundle | sqlite_application_bundle_vectors.json | fixture_sources/bundle.py |
| holds | sqlite_application_holds_vectors.json | fixture_sources/holds.py |
| original_claims | sqlite_original_claims_vectors.json | fixture_sources/original_claims.py |

No selection now means all five reviewed families. Explicit selections still
follow registration order and overwrite only their selected fixed files; repeated
names deduplicate. application_service remains unsupported until FIXTURE-003.
Check mode still compares independently rendered raw bytes, never a canonical
comparison that would ignore drift. All selected recipes finish before writes.
Only approved seeds/source functions are inputs even when all six outputs are absent.

### Frozen input/output and oracle crosswalk

Read-only inventory at main dd03e1d confirms the unchanged parent baseline files.
The complete-object fingerprints below are independent pre-generator oracles from
TESTCLEAN-006, not values computed by the candidate recipes:

| Family | Original raw SHA256 | Canonical typed SHA256 |
| --- | --- | --- |
| bundle | aebff7061f91fdf7598b321963bae155c139ed1dc2d1705e3122268bed84c312 | a374acb6441ab366b67100d742da7883f98a44f782fccf736b0b06b33e7a20dc |
| holds | c7f14e0aac721e186d5a327e2e4bd20f9617d9516f633f36ff7c205619082ef5 | 996f84632e9b1fa3b77c84fd12655aa090f81236308993bb44dcb83ddf842f95 |
| original_claims (metadata projected away) | 1f0eb63e8fe1b026c511eac3d9efb7b11053caa8e8599b24989dd1424fb8cf92 | a824e1a98c2f2a06916e8c65d2f5674ff1bd7718e7e92c9b1395dfba9f159803 |

Bundle/holds must reproduce original raw bytes as well as typed contents. Claims
preserves every old typed value and array position; remove ONLY the two metadata
fields from each of the six selectors to recover its old canonical fingerprint.
The original claims raw hash identifies provenance, not the newly annotated file.
New metadata is independently checked against this table, with exact bool type:

| Old row | name | check_lower_priority_corruption |
| --- | --- | --- |
| selector-0 | contribution-max-receipt-prefers-subject-A | true |
| selector-1 | receipt-max-resolves-source-B | false |
| selector-2 | contribution-receipt8-prefers-subject-A | false |
| selector-3 | missing-contribution42-receipt9 | false |
| selector-4 | receipt8-selects-original-subject-C | false |
| selector-5 | contribution42-receipt8-resolves-source-C | false |

Every row below remains an independently supplied DATA oracle. Named construction
may expand repeated literal records/typed bindings; it may not simulate application
policy, interpret SQL, or calculate expected query/state/byte totals from a product
model. Exact SQL and independent schema tuples come from source declarations, not
from SQL parsing, database observations or output-file recipe inputs.

| Output fields | Exact retained content and source plan |
| --- | --- |
| All families: format/provenance; bundle boundaries; holds synthetic_data_meanings | Original literal strings, including synthetic-only and invalid raw-history meanings. No revised provider or permission claims. |
| bundle.manifest | Shared catalogs.build step5, retaining catalog_format/store_identity/starting versions and payload_hex; 14 ordered table SQL declarations, full66-object independent schema, exact framing/hash. |
| bundle.table_columns | All14 independently supplied table-name/count pairs, no DDL-derived counts. |
| bundle.uint64_vectors/signed64_vectors/exact_vectors/historical_vectors | All6/2/7/2 rows, preserving typed boundaries, hex, reduced exact/invalid values and known/absent history literals. |
| bundle.arithmetic_vectors/reversal_vectors/control_vectors | All10/8/7 named literal requests, actual effects, cap/debt/reversal/control outcomes. Expected arithmetic remains supplied, not reimplemented. |
| bundle.literal_setup | All56 statements in original order: retained config2/context1, receipt6/attempt6, contribution2/observation6, allocator2, plan header4/revision4/request7/rule8/modifier4, state1/balances2/head1. Exact typed bindings retained. |
| bundle.scenarios | Six named arrays: original_prepare22, original_finish5, reversal_one16, reversal_two16, score_reversal12, boundary_raw_only15. Source contains explicit scenario rows and small INSERT framing helpers; it never derives effects or snapshots. |
| bundle.queries | original6, reversals4, typed8, preservation/prepared/pending one each. All names/exact SQL/literal result matrices retained. |
| bundle.constraint_refusals/runtime_only_admissions/fault | All28 refusal SQLs,10 named admission descriptions and complete trigger/drop/statement/meaning remain literals, including cross-column, rollback, canonicality and bytes. |
| holds.prefixes/manifest | Prefix1–5 fixture/version/blob/framed payload/hash tuples through shared catalog functions; exact step6 12SQL/full89 schema and framing/hash. Prefix file references remain descriptions, never opened as recipe inputs. |
| holds.causes | Seven named literal causes hOverflow/hPause/hFail/hReceipt/hOld/hNew/hTimer: source,command,observation,plan,kind,failure,timer,missing_policy,supersedes and every optional/type distinction remain. |
| holds.detections/versions | Six individually named nonpause rows plus explicit finite dPause1–65 expansion. Exact mutation/expected/proposed/audit/observation and immutable historical version/parent/position/producer values remain. No state machine or lookup simulation. |
| holds.views/resolutions/proposals | Seven views; rReplace request/produced rows; timer_correction and forbidden_old_beside_alias. All first/current/event locators, explicit application/composition links and absent/present-empty documents remain independently supplied. |
| holds.byte_item_oracles/literal_arithmetic | All35 named byte/item pairs and six explanatory arithmetic strings stay literal, independent of any packing/DTO model. |
| holds.arrangements | pending: original ordered402 setup statements/10 queries; timer: original ordered64 setup statements/seven queries. Small named construction shares exact source/intake/plan rows and expands only the known pause1–65 sequence, preserving SQL and typed values. No interpreter or mutation engine. |
| holds.history_pages/synthetic_data_meanings | Both pause_first2 and old_resolution pages, exact ordering/checked cursor/next/has_more and all11 explanatory strings. |
| original_claims.prefixes | All five original fixture/version/hash descriptions composed from shared literal catalogs, no output reads. |
| original_claims.setup | All45 ordered statements; shared explicit typed row framing, exact retained sources/A/B/C plans/snapshots/operations/claims/results/audits/effects. setup[9] stays PA revision with34 bindings and public revision/caller positions30–33; setup38/39 stays unchanged for native consumers. |
| original_claims.queries | All nine named exact SQL/result matrices, including actual type/MAX/priority/integrity meanings; no SQL execution creates expected rows. |
| original_claims.selectors | Six original rows/typed predicates/kind/record order unchanged; ONLY the two reviewed metadata fields added. |
| original_claims.logical_byte_oracles/old_nonempty_foundation | Both A/B full logical-byte terms and all eight nonempty foundation fields remain original literals, including scopes/IDs/items and bounded evidence. |

Source-size planning (review aid, never a deletion target): bundle roughly700–1100
lines/45–70KB; holds roughly800–1200 lines/45–75KB; claims roughly250–450 lines/
15–25KB, excluding unchanged shared catalogs. The legacy files are466409/839563/
68225 bytes respectively. Static source authoring refined bundle to about96KB/1406
lines: it retains every literal result, runtime description and source binding;
shared exact-number/byte anchors avoid opaque duplication. Claims is about21KB/277
lines. This refinement preserves the same bounded design and every field; source
review evaluates the actual declarations rather than treating the forecast as a cap. Most duplication is schema/payload framing and repeated
SQL text/typed rows. Use only narrowly named data constructors and explicit column
lists for fixed INSERT text/typed bindings; repeat exact independent SQL/projection
literals once. No external input, generic SQL evaluator/patch DSL, whole-JSON literal
wrapper or policy/byte-accounting simulator is introduced. If these estimates need
substantial expansion, report and review the design before expanding scope.

### Examples and focused acceptance

- From an unrelated cwd, `--fixture bundle --fixture holds --fixture original_claims
  --output-dir generated` reproduces those three files using a relocated script,
  source functions and ONLY four pinned seeds; all six legacy outputs can be absent.
- No selection creates all five files. A single selected family may overwrite its
  stale output while unselected-family/unrelated sentinel files stay byte-identical.
- `--check --fixture holds` with the frozen comparison succeeds without writes;
  appended whitespace is drift/exit1 even when typed JSON is unchanged. Claims check
  requires exactly annotated generated bytes, not the old unannotated comparison.
- Unknown application_service remains exit2 before recipes/file work; raw seed drift
  remains exit1 before output writes. No F001 assertion/pin/expected value is weakened.

Authored CLI tests use pre-generator whole-object fingerprints and an independent
six-row metadata table, never generated expected values. New reachable named inert
FeatureNotImplemented stubs permit intended red. Independent contract/test approval
must precede their run, and intended red precedes implementation. Independent source/
oracle execution approval precedes real generation, JSON adoption and native use.

The native delta is ONLY data-row naming and explicit bool metadata selection in
frozenSelectorsPreserveGlobalPriorityAndRoles. Keep the original lower-priority
corrupt-B-after-selected-A assertion; replace index0 branching with the explicit
bool field. Keep setup[9]/revised_scalar30–33, every method/assertion and all other
native files unchanged. Runtime review maps the six old selector PASS IDs one-to-one
to the six new names; the remaining863 PASS IDs and all361 methods remain unchanged. Run
relevant Qt/tooling once for the changed baseline, compile/passively discover account44
without its feature bodies, and distinguish actual results from planned counts.
