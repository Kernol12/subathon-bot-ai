# Independent fixture data

The six files below are reproducible synthetic test oracles. Their expected values
come from reviewed literal data and public byte framing. The recipes use the Python
>=3.10 standard library; they do not run production code, SQLite, providers or
password hashing to calculate expected results.

## Check or generate

From the repository root, check all six committed outputs without writing:

```sh
python3 tools/generate_fixtures.py --check
```

Generate all six into a fresh disposable directory, or select one family:

```sh
python3 tools/generate_fixtures.py --output-dir /tmp/subathon-fixture-review
python3 tools/generate_fixtures.py --fixture application_service --output-dir /tmp/subathon-service-review
```

Exactly one of `--check` and `--output-dir` is required. No `--fixture` selects all
six; repeat it to select several families. Selected output files may be overwritten
in the explicit directory; unselected and unrelated files remain unchanged. A
relative output directory is relative to the caller's working directory. Seed and
check paths resolve from the script location, independently of that directory.

Check mode compares exact bytes and never repairs drift. Missing/different outputs,
changed seeds or recipe/I/O failures exit 1; invalid modes or family names exit 2.
The [R1 contract](../../docs/contracts/fixture-generation-v1.md) defines the complete
selection, formatting, input-pin and no-write behavior. Recipe/oracle changes follow
the existing contract, test and independent-review gates; generated fingerprints
cannot replace independently derived expected values.

## Inputs and provenance

Only four existing files are recipe inputs:

- `retained_context_vectors.json`
- `sqlite_receipts_vectors.json`
- `sqlite_contributions_vectors.json`
- `sqlite_application_plans_vectors.json`

All four raw SHA256 pins are verified before parsing. Recipes use only their
manifest version, ID, ordered SQL and expected-schema fields. The remaining data
comes from reviewed declarations in
[`tools/fixture_sources`](../../tools/fixture_sources). The six generated outputs
are never recipe inputs; check mode reads them only after independent generation.
Other files in this directory retain their owning suites and are outside this
six-family generator.

The [independent pre-generator baseline](../../docs/tasks/EPIC-028/TESTCLEAN-006.md#independent-pre-generator-baseline)
freezes the complete typed JSON for every family, preserving arrays, SQL, byte
strings, values and types. Reference/account/bundle/holds retain original raw bytes.
Claims adds only the two reviewed selector metadata fields described below; removing
only those fields recovers its old full canonical content. Service uses the common
JSON layout with its entire old typed object unchanged. Original raw hashes for
claims/service remain provenance. Complete family inventories and framing examples
are in the [R1 contract](../../docs/contracts/fixture-generation-v1.md).

| Family and source | Fixed output | Retained coverage |
| --- | --- | --- |
| `reference_capture` / [source](../../tools/fixture_sources/reference_capture.py) | `sqlite_reference_capture_vectors.json` | A/B documents, ordered setup, 26 queries, 23 constraint refusals and complete catalog/prefix framing |
| `account_sessions` / [source](../../tools/fixture_sources/account_sessions.py) | `sqlite_account_sessions_vectors.json` | Literal verifier/snapshot/source-byte anchors, five source-byte counts, four base SQL statements and 17 constraints |
| `bundle` / [source](../../tools/fixture_sources/bundle.py) | `sqlite_application_bundle_vectors.json` | 14 table declarations, 56 setup statements, six scenarios, 28 refusals, 10 admissions, all arithmetic/reversal/query/budget oracles |
| `holds` / [source](../../tools/fixture_sources/holds.py) | `sqlite_application_holds_vectors.json` | 71 detections/versions, seven causes/views, 35 byte/item anchors, ordered pending/timer setups of 402/64 rows, queries and history |
| `original_claims` / [source](../../tools/fixture_sources/original_claims.py) | `sqlite_original_claims_vectors.json` | 45 setup rows, nine queries, six named selectors and both A/B logical-byte anchors |
| `application_service` / [source](../../tools/fixture_sources/application_service.py) | `application_service_vectors.json` | All 34 cases/13 groups, four codec anchors, 32 source rows, history/hold/retry arrangements, six faults and 26 snapshot queries |

## Claim selector roles

The original six rows retain their order and typed predicates. These explicit names
and boolean roles replace the old row labels:

| Old label | Name | `check_lower_priority_corruption` |
| --- | --- | --- |
| `selector-0` | `contribution-max-receipt-prefers-subject-A` | `true` |
| `selector-1` | `receipt-max-resolves-source-B` | `false` |
| `selector-2` | `contribution-receipt8-prefers-subject-A` | `false` |
| `selector-3` | `missing-contribution42-receipt9` | `false` |
| `selector-4` | `receipt8-selects-original-subject-C` | `false` |
| `selector-5` | `contribution42-receipt8-resolves-source-C` | `false` |

The first role still exercises lower-priority corrupt B after selecting A.
`setup[9]`, its public PA revision and caller bindings 30–33 remain unchanged;
every native assertion stays in its owning suite.

The existing SQL CI job runs the all-six `--check` immediately after checkout,
before dependency preparation or native use. Its normal stable acceptance and
passive account lane remain intact. Account coordinate 8 is still proposed; the
44-case account/session source baseline remains intentional red. CI compiles it
and checks nonempty Qt metaobject functions without running the feature bodies.
Fixture reproduction establishes developer-tool consistency; product, live-provider
and platform verification remain separate.
