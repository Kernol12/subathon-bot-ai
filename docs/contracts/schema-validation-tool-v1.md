# Offline schema-package validation — document proposal v1

SCHEMATEST-001, from claimab822715 after accepted
[ADR-0008](../decisions/0008-schema-validation-foundation.md). Authority:
specification v0.45 §§17/18, API-01/02/03, TEST-04. This proposes a DEVELOPER
tool contract; it selects no installed dependency, executable API/header, schema,
fixture/test/stub or working validator. Different document/dependency/API review,
real selected-library foundations and reviewed tests/red remain required.

Validate an admitted JSON package with existing OpenAPI3.1/JSONSchema2020-12
libraries, complete local reference closure and explicit example-instance validation.
This does not validate HTTP bytes, authentication, current authority or a running
server. CONTRACT013 remains a separate two-route package; no tool result advertises
its endpoint. Financial/domain wire values remain strings under their own contracts;
the tool's generic JSON-number model does not narrow those product domains.

## 1. Proposed libraries and actual foundation gate

Propose explicit OpenAPIV31SpecValidator from openapi-spec-validator0.9.0,
OAS31Validator from its openapi-schema-validator0.9.0 dependency, and
jsonschema4.26.0 Draft202012Validator/referencing0.37.0 for schema resources and
instance evaluation. jsonschema-path0.5.0 supplies OpenAPI reference plumbing.
These are candidate versions with Python>=3.10 metadata, not finalized pins or
observed compatible artifacts. No custom OpenAPI/JSONSchema validation engine.
[Tagged OSV metadata](https://raw.githubusercontent.com/python-openapi/openapi-spec-validator/0.9.0/pyproject.toml),
[jsonschema metadata](https://raw.githubusercontent.com/python-jsonschema/jsonschema/v4.26.0/pyproject.toml),
[schema-validator metadata](https://raw.githubusercontent.com/python-openapi/openapi-schema-validator/0.9.0/pyproject.toml),
[path metadata](https://raw.githubusercontent.com/p1c2u/jsonschema-path/0.5.0/pyproject.toml).

OSV/path license Apache2, jsonschema/referencing MIT, schema-validator BSD3.
Full transitive dependencies, exact artifact hashes/provenance/licenses/notices,
Python3.10/current-host/CI compatibility, optional compiled-wheel availability and
actual APIs require a DIFFERENT review and green foundation before minimal wrapper
stubs/tests. No global pip install or ambient-library compatibility is implied.
Keep this dedicated dependency environment separate from existing standard-library
progress tools. Root must reconcile tools/AGENTS.md's existing standard-library
guidance explicitly when admitting this justified dedicated library-backed tool.

Close these known traps in the later selected adapter:

- Explicit3.1 classes and dialects; default latest aliases now include3.2.
- SchemaPath's default HTTP/HTTPS/file/all-URL handlers MUST be replaced by the
  admitted in-memory registry. No fallback reader/validate_url or global network
  resolver. Supplying a jsonschema Registry alone does not replace SchemaPath.
- Spec validation is not complete example-instance validation. Enumerate every
  admitted example and evaluate it against its actual schema separately.
- Unknown formats normally pass FormatChecker. This v1 profile refuses schema
  `format` keywords as unsupported_format; it never silently asserts them. A later
  reviewed format profile may add explicit real checkers without changing product
  numeric domains. OpenAPI `bearerFormat` documentation is not a schema format.

[OSV classes](https://raw.githubusercontent.com/python-openapi/openapi-spec-validator/0.9.0/openapi_spec_validator/validation/validators.py),
[SchemaPath default handlers](https://raw.githubusercontent.com/p1c2u/jsonschema-path/0.5.0/jsonschema_path/handlers/__init__.py),
[OSV schema traversal](https://raw.githubusercontent.com/python-openapi/openapi-spec-validator/0.9.0/openapi_spec_validator/validation/keywords.py),
[registry behavior](https://python-jsonschema.readthedocs.io/en/stable/referencing/),
[format behavior](https://python-jsonschema.readthedocs.io/en/stable/validate/#validating-formats).

## 2. One package, manifest and prospective entrypoints

Future module `tools/schema_validate.py`, raw entrypoint
`validate_package(manifest_path: str) -> SchemaValidationOutcome`; future enum
outcomes are exactly the codes in section5, including ok. CLI is exactly
`python3 tools/schema_validate.py --manifest PATH`; optional `--help` emits static
usage only. No URL/stdin/recursive glob/environment/config option enlarges admission.
Unknown/duplicate arguments are usage_error, never echoed. Path is a bounded
caller-owned string, no arbitrary user-defined path-conversion callback. The later
concrete API review must freeze declarations and marker mechanics before source.

The package root is the opened manifest's parent directory. Manifest is one UTF8
JSON object with EXACT required keys `profile`, `documents`, `examples`:

```json
{"profile":"subathon.schema-package.v1","documents":[{"path":"api.json","kind":"openapi31"},{"path":"values.json","kind":"jsonschema202012"}],"examples":[{"path":"sample.json","schema_document":"values.json","schema_pointer":""}]}
```

documents has1..32 entries, each EXACT path/kind; at most ONE openapi31 root.
examples has0..256 entries, each EXACT path/schema_document/schema_pointer.
Paths are distinct except several bindings may intentionally share the same
example file with DIFFERENT target schemas; duplicate identical bindings fail.
Each schema_document names a declared document. JSON Pointer is empty for root or
an RFC6901 slash pointer, decoded by the library with exact ~0/~1 handling, not a
percent-decoded filesystem path. Its target must be an actual Schema Object under
that document's dialect; a random mapping elsewhere is not a schema root.

Relative file paths are1..256 ASCII bytes, slash-separated nonempty segments from
`[A-Za-z0-9_.-]+`, with no `.`/`..` segment, leading slash, backslash, NUL, percent
encoding or URI syntax. Manifest argument is at most4096UTF8 bytes/nonempty/noNUL.
Only regular files are admitted. No symlink in manifest/root traversal/package
paths, directory/device/socket, parent escape or arbitrary external file. Open and
walk package files relative to owned directory descriptors with no-follow and
verify actual handles; a prior Path.resolve string check alone is insufficient.
Unsupported secure file-opening facilities return environment_unavailable. Initial
CI/Linux tooling support is a developer-environment gate, not a reduction of the
Windows/Linux product-platform scope. Package owner must serialize edits while
validation reads; no whole-filesystem atomic/currentness certificate is returned.

Every declared document/example file must be read and validated; no unknown glob
or silently unused example. An instance may be any JSON value, including null.
Negative acceptance fixtures are separate bad packages, never `expected_valid:false`
exceptions inside a successful package. No library success manufactures a test's
expected result. A non-exact-str API path is invalid_path without conversion or
repr of the supplied object.

## 3. Exact data, bounded ownership and execution

Trust fixed profile ceilings, not manifest-selected options:

| Quantity | v1 ceiling |
| --- | --- |
| Manifest bytes | 65536 |
| Unique package files including manifest | 289 (1+32+256) |
| One document/example file | 1048576 bytes |
| Complete raw files including manifest | 8388608 bytes |
| JSON nesting (root container depth1), complete values per file | 64 / 65536 |
| Raw quoted string token including quotes | 65536 bytes |
| Number lexeme / mantissa digits / absolute explicit exponent | 512 bytes / 256 / 4096 |
| Exact number numerator and denominator | 16384 bits each |
| Actual schema resources / schema-context locations / reference edges | 256 / 4096 / 2048 |
| One resource URI/JSON Pointer key / complete owned graph-key bytes | 4096 / 1048576 UTF8 bytes |
| Complete declared example bindings | 256 |
| Public diagnostic bytes including newline | 256 |
| One worker / complete deadline / address-space ceiling | 1 / 10 seconds / 536870912 bytes |

Check checked sizes and counts before payload reads/growth; stat is not a substitute
for bounded actual reads and actual regular-file identity. Read at most admitted
size+1, reject growth/changed-read evidence; total actual bytes count every distinct
owned file. No duplicate read per binding. Retain one immutable raw snapshot per
file; parsed ownership is separate and bounded by pre-scanned token/value/depth
ceilings. A preliminary lexical-resource scan before json.loads bounds nesting,
token size and numeric conversion; it is not a new standards validator. At most
one raw input copy and one decoded tree per file are retained. Fixed scanner frames
are bounded64; resource records256, schema-context records4096 and edge records2048. Diagnostics retain
no original bytes, arbitrary value, filesystem path, URI, library message or traceback.
Context keys include resource URI, pointer and context discriminator; count every
materialized occurrence/alias in the complete graph-key budget before growth. More
than4096 distinct fragment targets is resource_limit even in ONE resource. No
4096-entry table is inferred from the smaller256-resource count. These semantic
ceilings do not pretend to measure Python/library allocator overhead.

Strict JSON decoding rejects BOM, invalid UTF8/lone-surrogate strings, duplicate
object keys (after escape decoding), NaN/Infinity, comments and trailing values.
The lexical-resource scan runs before general JSON decoding, so a resource excess
can precede a later syntax defect. UTF8/BOM checks precede that scan. Standard JSON
grammar, including number spelling, is required before exact numeric construction.
Bound complete number lexemes, digit/exponent magnitude and projected bits BEFORE
integer powers/Fraction construction; no parse-float overflow or fitting prefix.

Parse finite JSON numbers exactly from their original lexemes to rational values;
normalize denominator1 to Python int, otherwise retain fractions.Fraction. No float
or bounded-precision Decimal arithmetic/limit_denominator approximation. Thus1.0
and1e0 satisfy integer, true does not;0.3 is exactly3/10 and a multiple of0.1.
Numerical equality/enum/uniqueItems follow JSON mathematical values, including
1==1.0 and -0==0, independently of source spellings. Numeric-keyword constants use
the SAME exact representation. Source bytes remain available for strict decoding;
this tool does not retain financial source provenance for the product.

This is a proposed adapter obligation. Selected-library green checks must establish
actual Fraction number/comparison/modulo/enum/uniqueItems and integer-normalization
behavior, including schema-count1.0, large neighboring decimals and booleans. If a
selected path coerces to float or cannot preserve these values, stop before approving
it; do not substitute an unreviewed arithmetic or standards engine. Python Fraction
supports exact decimal/exponent strings; jsonschema's default integer recognition
does not by itself accept arbitrary Decimal/Fraction objects.
[Python Fraction](https://docs.python.org/3.10/library/fractions.html),
[jsonschema types](https://raw.githubusercontent.com/python-jsonschema/jsonschema/v4.26.0/jsonschema/_types.py),
[numeric keywords](https://raw.githubusercontent.com/python-jsonschema/jsonschema/v4.26.0/jsonschema/_keywords.py).

All package read/scan/parse/validation work runs in ONE fresh controlled worker;
the parent retains only its bounded path/argv and diagnostic, not parsed trees.
This prevents process-global
library caches from accumulating across calls. Parent uses bounded protocol reads,
never unbounded capture_output/read-all of stderr. Worker library stdout/stderr are
suppressed; only the fixed outcome protocol is published. Parent enforces the
monotonic10-second deadline and worker address-space limit before library work,
kills/reaps on timeout/oversized protocol and releases every owned handle on all
paths. Unsupported enforcement is environment_unavailable. Timeout/known memory
refusal is resource_limit; crashes/unclassified errors are validation_unavailable.
No worker fault, skipped stage or partial result becomes ok. Concrete process/API
design and source-level allocation/lifetime audit remain later reviewed gates.

## 4. Dialects, references and complete example inventory

Admit OpenAPI version3.1.0,3.1.1 or3.1.2; explicitly select the3.1 validator.
Standalone JSON schemas require `$schema` exactly
`https://json-schema.org/draft/2020-12/schema`. OpenAPI jsonSchemaDialect, if present,
is exactly `https://spec.openapis.org/oas/3.1/dialect/base`; its Schema Objects may
explicitly select that dialect or Draft2020-12. Unknown/custom vocabularies or
unsupported assertions are refused, never reinterpreted as current/default latest.
No3.0 nullable shortcut or3.2 feature is silently accepted.

To keep the first wrapper finite, the OpenAPI inventory profile admits top-level
info/servers/paths/components/security/tags/externalDocs and ordinary operations,
parameters, request bodies, responses/headers and Schema/Example/Security Objects.
Callbacks, webhooks, links, custom extension payloads or other locations that the
actual reviewed adapter cannot inventory completely are unsupported_feature, never
claimed invalid OpenAPI. The final baseline must enumerate the exact admitted
object-location table before stubs/tests. This is a developer-tool profile gate;
required product endpoints/WS/providers/features remain in scope and require a
later reviewed extension when their packages need it. Reference013's two-route
package needs none of these deferred locations.

Assign each document a synthetic base URI below
`https://schemas.subathon.invalid/package/` followed by its exact relative path.
It is an IN-MEMORY identifier, never a request target. Explicit schema $id resources
must remain unique and within that admitted namespace; resolve relative identifiers
using the standards library's actual lexical scope. Official dialect/metaschema
resources come only from the reviewed pinned dependency registry. References to
any other URI/file/undeclared document fail even when resolvable on this machine.
No home-directory/cwd/cache/http/file fallback. Build the complete admitted registry
before evaluating instances; do not fetch on a missing identifier.

Reference inventory is CONTEXT aware. OpenAPI Reference Objects are interpreted
only at their standard object positions. Schema traversal uses Draft2020-12
subschema semantics/resource APIs and exact lexical $id/anchor scope, not a recursive
search for every key named `$ref`. Schema child maps include $defs/properties/
patternProperties/dependentSchemas; child applicators include items/contains/
additionalProperties/propertyNames/unevaluatedItems/unevaluatedProperties/if/then/
else/not and allOf/anyOf/oneOf/prefixItems. Validate other standard annotations and
assertions through the selected library. Literal const/enum/default/example data
are NOT schema children; a property NAMED `$ref` is not a reference keyword.

Every actual schema/reference root must be reached, including unused declared
components/resources, and each actual reference target must exist with the right
OpenAPI/schema context. Use visited resource+pointer+context records for closure;
cycles do not mean "skip validation" or unbounded recursive expansion. Build the
schema evaluation graph using same-instance edges ($ref/composition/conditions)
and instance-descending edges (for example object properties/array items). The
same-instance-only subgraph must be acyclic. Thus productive recursive schemas
over finite bounded instances remain valid; every admitted cycle descends into
the instance. A pure alias cycle
with no instance descent (for example root `$ref:"#"`) is reference_cycle, not ok;
unknown/no-progress evaluation is never certified by a caught RecursionError. The
selected adapter must prove this partition; the worker deadline remains a final
resource refusal, not an implementation of reference semantics.

Manifest bindings independently name each external instance file and its actual
schema location. EVERY inline OpenAPI example/value and Schema Object examples
entry must also be covered by an exact binding: for that purpose an example path
may instead be the closed object `{"document":"api.json","pointer":"/..."}`.
Its pointer addresses the actual example VALUE, not its containing Example Object.
No other path object shape is allowed. Count these bindings in the same256 ceiling.
Referenced Example Objects are resolved only through admitted local refs;
externalValue examples are unsupported_feature, not silently ignored/network-read.
Unused component examples require explicit target-schema bindings too. A data value
`{"$ref":"https://outside.invalid/x"}` is literal data; no resolver sees that URI.

Compare the complete context-aware declared-example inventory with manifest bindings:
missing/duplicate/wrong-context bindings are invalid_inventory. One value may have
several explicit different target-schema bindings, each evaluated once. Schema
`examples` entries bind to their containing schema; OpenAPI request/response/parameter
examples bind to the actual associated schema, not a convenient permissive one.
No inline example/annotation/unused component is silently excluded. Actual traversal
and selected-library adapters need independent source/API and green foundation
review; if they cannot supply this finite closure/inventory, record the blocker or
split the adapter work rather than add an unreviewed standards engine.

## 5. Exact phases, outcomes and diagnostic protocol

Each synchronous API call returns one enum; no arbitrary exception text is public.
CLI emits exactly ONE compact ASCII JSON line `{"code":"CODE"}\n` to stdout and
no stderr. ok uses CODE=ok; no counts/paths/input-derived labels are included.
Known issues use the first applicable phase, then document/manifest order and
schema/example JSON-pointer unsigned UTF8 byte order within that phase:

1. CLI argument shape/path length, worker enforcement, then secure root/manifest
   open. usage_error(exit64), invalid_path(exit2),
   io_unavailable/environment_unavailable(exit3). Parent establishes the worker
   deadline/address-space controls before package reads; no unenforced fallback.
2. Actual file/count/complete-byte preflight, UTF8/BOM and lexical-resource limits,
   strict manifest JSON then closed manifest shape/path/duplicate bindings.
   resource_limit(exit4), invalid_json/invalid_manifest/invalid_path(exit2).
   Within a file UTF8/BOM→resource scan→strict JSON. Files are visited in declared
   document order followed by first occurrence of each external example.
3. All admitted payload strict decoding and exact number conversion, dialect/format
   admission. invalid_json/unsupported_dialect/unsupported_format(exit2), or
   resource_limit(exit4). No libraries run on an unadmitted package.
4. Actual pinned dependency availability/backend foundation; missing/mismatched
   → dependency_unavailable(exit3). No global auto-backend/optional installation
   changes behavior.
5. Ordered substages, each completed before the next: reject unsupported tool
   locations/features (unsupported_feature); native metaschema/structural checks
   of every document/schema root (invalid_schema); complete context-aware
   target closure (unresolved_reference); same-instance cycle analysis
   (reference_cycle); example binding/inventory equality (invalid_inventory);
   complete native semantic OpenAPI/schema checks (invalid_schema). All exit2.
   No permissive partial validation after an earlier refused stage. The concrete
   selected adapter must actually provide these ordered calls; inability is a
   documented dependency/API gate, never an invented implementation guarantee.
6. Every declared example-instance evaluation, in manifest binding order:
   invalid_instance(exit2). Only after ALL checks complete return ok(exit0).

Throughout: hard limits/deadline/memory→resource_limit(exit4); unclassified internal
failure/crash/protocol defect→validation_unavailable(exit3). Failure diagnostics
contain no exception repr, invalid value, JSON Pointer, path, URI or traceback.
--help exits0 with EXACT static text `Usage: schema_validate.py --manifest PATH\n`.
No percentages or validation success after a refused/failed/skipped item.

## 6. Independent literal oracles and remaining stages

These are authored examples, not executable fixtures or observed validator behavior.
One COMPLETE minimal valid package is:

```text
manifest.json: {"profile":"subathon.schema-package.v1","documents":[{"path":"s.json","kind":"jsonschema202012"}],"examples":[{"path":"v.json","schema_document":"s.json","schema_pointer":""}]}
s.json: {"$schema":"https://json-schema.org/draft/2020-12/schema","type":"integer"}
v.json: 1.0
expected: {"code":"ok"} followed by one newline; CLI exit0
```

A COMPLETE OpenAPI-only document (no examples) is
`{"openapi":"3.1.1","info":{"title":"Synthetic","version":"1"},"paths":{}}`,
with one openapi31 manifest entry and examples=[]. Expected ok, no capabilities or
HTTP routes supplied. Delete info from this literal: invalid_schema, exit2.
For the remaining table use one `s.json` jsonschema202012 document and one
`v.json` external binding to its root. Schema has the exact required $schema above.

| ID | Literal schema/data variation | Required observation |
| --- | --- | --- |
| SV01 | schema `{"type":"integer"}`, value1 | ok |
| SV02 | same, raw value1.0 or1e0 | ok, mathematical integer |
| SV03 | same, true or1.5 | invalid_instance |
| SV04 | schema `{"type":"number","multipleOf":0.1}`, value0.3 | ok, exact3/10 ÷1/10=3 |
| SV05 | same,0.3000000000000000000000000000000000000001 | invalid_instance, no Decimal/float rounding to0.3 |
| SV06 | schema `{"uniqueItems":true}`, value[1,1.0] | invalid_instance |
| SV07 | schema `{"const":{"$ref":"https://outside.invalid/x"}}`, identical object value | ok; data causes ZERO retrieval |
| SV08 | schema `{"$ref":"https://outside.invalid/x"}` | unresolved_reference; ZERO retrieval |
| SV09 | schema with property named `$ref`, value containing that ordinary string property | validate as ordinary property, no reference inferred from the name |
| SV10 | declared local schema reference to absent resource/pointer | unresolved_reference, no cache/cwd/network fallback |
| SV11 | root schema `$ref:"#"` | reference_cycle, not ok/skip |
| SV12 | recursive node schema child points to same node; finite matching instance | ok under complete bounded evaluation, not automatically rejected for graph cycle |
| SV13 | duplicated raw key `"type"` and `"t\u0079pe"` | invalid_json, no last-key-wins |
| SV14 | bad required OpenAPI info/paths shape | invalid_schema through explicit3.1 library |
| SV15 | valid OpenAPI metadata but wrong declared response example | invalid_instance even if spec validator alone passes |
| SV16 | inline example exists but no exact manifest binding | invalid_inventory |
| SV17 | schema format unrecognized or date-time | unsupported_format under explicit v1 policy, no silent pass |
| SV18 | undeclared file/../path/symlink/remote reference | invalid_path or unresolved_reference at the corresponding phase, never read/fetch |
| SV19 | max file/package boundary+1, depth65,513-byte number, exponent4097 | resource_limit before costly ownership/arithmetic/library work |
| SV20 | missing selected library or unsupported worker limits | dependency/environment_unavailable; never intended feature red |

The later independently authored fixtures must spell out COMPLETE valid manifests,
schemas, values and expected codes for these families, including exact boundaries,
productive/no-progress cycles and cross-context $ref. Expected outcomes must not
be generated by the future wrapper. SV19's separate variants must isolate the
intended boundary; an earlier file/JSON defect is not evidence for a later limit.

Initial document review does not approve the future concrete API, dependency lock,
numeric/context/process adapter, native environment, fixture inventory or source.
After their actual separate review: authenticate/pin dependencies and pass real
standalone foundations, author/review independent tests with inert immediate named
missing-wrapper stubs, then observe intended wrapper red. Only integrated reviewed
baseline releases SCHEMATOOL001 implementation/source review/passing ordinary CI.
No fixture/harness can provide fake successful standards validation. All later
schema/package/auth/runtime/platform/live and full agreed product gates remain.
