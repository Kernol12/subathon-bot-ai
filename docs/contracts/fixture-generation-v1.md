# Independent fixture generation — revision 1

Task: FIXTURE-001, under TESTCLEAN-006 / ADR-0012 and specification v0.45 §17.
Status: contract and authored-test baseline; candidate recipes are initially inert.
This developer tool reproduces independent test data. It establishes no running
server, account/session storage, authority, provider or platform capability.

## Command, selection and paths

```text
python3 tools/generate_fixtures.py (--check | --output-dir DIR) [--fixture NAME ...]
```

Use Python>=3.10 standard library only. Exactly one operation mode is required:
no mode or both modes is a usage error. Unsupported names are usage errors, not a
request to copy an existing fixture. Usage errors exit2 before recipe/file work.
The supported families and fixed filenames in FIXTURE-001 are:

| Name | Filename |
| --- | --- |
| reference_capture | sqlite_reference_capture_vectors.json |
| account_sessions | sqlite_account_sessions_vectors.json |

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
order and one final newline. No NaN/Infinity/floats are permitted. Preserve all
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
| `--output-dir generated` from an unrelated cwd | Both exact outputs in cwd/generated, using only script-relative seeds/source recipes; six legacy outputs can be absent |
| `--fixture reference_capture --output-dir selected` | Reference is created/overwritten; existing account/unrelated sentinel bytes stay unchanged |
| `--check` with both exact committed comparison files | exit0 and no writes |
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

When both generated fixture files and native source stay byte-identical, preserve
valid prior local Qt evidence instead of replaying native bodies for receipts.
Current hosted selected-Qt CI must still pass normally. Record actual CLI/tooling/CI
proof separately from prior local Qt and unrun platform/provider/full-product work.
