# Schema validation context adapter — document proposal v1, revision2

SCHEMATEST-001, author base5233ff4. Completes the proposed concrete DOCUMENT split
under [logical v1](schema-validation-tool-v1.md), exact blob
`abb426a91173726a703cba70d6a68225de483bdd`, and [public API v1](schema-validation-tool-api-v1.md),
accepted43840fd/blob `691270f0ff661872e3af5fb061a5df35a2baa48b`.
Those requirements, SV01–20, public outcomes/priority/process limits and immediate
named missing-feature stubs remain unchanged. This supplies no executable adapter,
schema, fixture, source/interface/stub, test, worker, dependency or CI change.

The selected eleven fixed SDK methods establish only their actual small recipes.
Plain OpenAPIV31SpecValidator misinterprets a default DATA object's string `$ref`.
The proposed raw-value cursor below requires additional independently reviewed
fixed SDK evidence. Even that evidence would not prove an arbitrary context adapter.
Both actual document reviews, needed SDK facts and later reviewed independent
wrapper tests/intended red remain gates; no compatible backend is asserted here.

## 1. Exact role index before semantic evaluation

Index the immutable decoded document by PHYSICAL document name and canonical raw
RFC6901 pointer. Root is empty; object segments escape ~→~0 then /→~1, array indices
use canonical unsigned decimal. Do not resolve `$ref` while constructing this index.
A physical value can have distinct indexed roles; identity is
`(document ordinal, physical pointer, role)`, never Python id/equal JSON contents.
Early public dialect/format admission builds ONLY this pure closed-table physical
role index, after bounded strict decoding and BEFORE dependency imports. It runs no
SDK operation, URI lookup or schema assertion engine. Schema scope fields remain
EMPTY until post-dependency enrichment. Missing/wrong-shaped containers or schema
children are retained as typed pending-invalid locations for later native structural
checks; do not invent child contents, coerce types or validate assertions early.
DATA is one atomic borrowed view of its raw subtree: descendants do not receive
4096-budget role records or get scanned for keywords. All their JSON values/tokens
still count in the separate accepted per-file parse/resource budgets. After exact
dependency admission, valid Schema records acquire SDK resource identity/resolver.
An alias/reference occurrence retains its own identity and target edge;
it does not replace/erase the original object or schema `$ref` siblings.

The following is the CLOSED structural-location table. Names after * are map keys;
[] is every array member. Metadata children have their own native OpenAPI type but
contain no Schema/Example/Reference roles unless another row explicitly says so.
At a Ref-capable position, a mapping with string `$ref` is an OpenAPI Reference
Object of THAT expected target role, except Schema Objects use JSONSchema rules.
Native structure checks, rather than the role index, validate required keys/types.

| Parent role | Actual indexed children / Ref-capable target |
| --- | --- |
| Document openapi31 | paths/*→PathItem; components→Components; info, servers[], security[], tags[], externalDocs→Metadata |
| Components | schemas/*→Schema; responses/*→Response/Ref; parameters/*→Parameter/Ref; requestBodies/*→RequestBody/Ref; headers/*→Header/Ref; examples/*→Example/Ref; securitySchemes/*→SecurityScheme/Ref; pathItems/*→PathItem |
| PathItem | get/put/post/delete/options/head/patch/trace→Operation; parameters[]→Parameter/Ref; servers[]→Metadata; `$ref`→PathItem |
| Operation | parameters[]→Parameter/Ref; requestBody→RequestBody/Ref; responses→Responses; security[], servers[], externalDocs→Metadata |
| Responses | every status/default entry→Response/Ref |
| Response | headers/*→Header/Ref; content/*→MediaType |
| Parameter or Header | schema→Schema; content/*→MediaType; example→Data; examples/*→Example/Ref |
| RequestBody | content/*→MediaType |
| MediaType | schema→Schema; example→Data; examples/*→Example/Ref; encoding/*→Encoding |
| Encoding | headers/*→Header/Ref |
| Example | value→Data; externalValue is a refused tool feature |
| SecurityScheme | flows and nested OAuth flow/scopes→Metadata |
| Metadata | actual nested metadata only: Info contact/license, Server variables, Tag externalDocs, OAuth flows/scopes, security-requirement maps; NO inferred refs/schema/examples |
| Schema | schema children in the next table; example, examples[], default, const, enum→Data |
| Data | complete raw JSON value; descendants stay DATA, regardless of key spellings |
| Document jsonschema202012 | root→Schema |

Reference Object summary/description remain metadata; they cannot supply schema
assertions. At PathItem `$ref`, this initial profile admits the reference ALONE;
mixed local PathItem fields are unsupported_feature because OpenAPI leaves their
merge behavior undefined. This is an explicit tool-profile gate, not invalid OAS.
Callbacks, webhooks, links, externalValue and extension payloads at structural OAS
positions remain unsupported_feature as already permitted by logical v1. Unknown
structural field names are left to native structural validation, not silently
treated as schema/data. Dynamic map names (including a property/example named
`$ref` or x-...) are not fields and do not trigger that refusal. Data payloads may
contain ANY otherwise admitted JSON, including `$id`, `$ref`, format and x- keys.

Schema CHILD locations match pinned Draft202012 resource semantics, not a key-name
walk. Index every location, even an unused component/$defs or annotation schema:

| Schema keyword position | Child schemas | Evaluation edge |
| --- | --- | --- |
| $defs, definitions | every map value | inventory only |
| properties, patternProperties | every map value | descending |
| dependentSchemas | every map value | same-instance |
| allOf, anyOf, oneOf | every array value | same-instance |
| prefixItems | every array value | descending |
| additionalProperties, items, contains, unevaluatedItems, unevaluatedProperties | keyword value | descending |
| propertyNames | keyword value | descending name facet |
| if, then, else, not | keyword value | same-instance |
| contentSchema | keyword value | inventory only; content vocabulary is annotation |

Boolean schemas are real schema records. Do not recurse into an invalid child as
arbitrary data to hide a native schema error. `$ref` at a Schema Object creates a
schema edge AND leaves every sibling active. All other standard assertions and
annotations are delegated to the selected validator, never reimplemented here.
Package `$dynamicRef`/`$dynamicAnchor` are an explicit unsupported_feature v1 gate:
the finite static cycle proof below does not establish dynamic-scope evaluation.
This written profile limitation needs actual independent review; no standard or
product feature is declared invalid or deleted. Official trusted metaschemas may
use dynamic references inside their real SDK evaluation. Package Schema.discriminator
is also explicitly unsupported_feature in revision2: OAS3.1.1 §§4.3/4.8.25 gives
its implicit/explicit component names and URI mappings real schema connections,
which the finite typed closure above does not establish. Do not silently ignore
these mappings or rely on ordinary instance validation to close them. A separately
reviewed profile extension must supply their complete typed/native-backed resolution.
This applies only to the discriminator FIELD at a real package Schema role, never
a property NAMED discriminator or literal data. Dynamic/discriminator gates never
scan/refuse trusted SDK metaschemas or their internal native references; those use
the admitted finite official registry and actual SDK semantics. Unknown/custom dialects
and package vocabularies retain logical unsupported_dialect admission.

## 2. Scope, URI lookup and complete typed closure

Each declared document has exact synthetic URI
`https://schemas.subathon.invalid/package/` + RelativePath. No URI is a file-open
instruction. AFTER dependency admission, build the actual in-memory registry BEFORE
instance validation, using `referencing.Registry(retrieve=deny)` and the following
pinned Draft202012/explicit OAS-container Resources. The deny
callback raises precise NoSuchResource, performs no IO, and cannot provide a fake
successful schema. Preloaded official SDK metaschemas are a separate trusted finite
registry, not permission for package refs to target arbitrary dependencies.

For each Schema root and child, obtain resource/id/subresource/anchor semantics
from DRAFT202012.create_resource, Resource.id/subresources/anchors and SDK resolver
in_subresource. A raw indexed ancestor chain identifies WHERE to apply these SDK
operations; it does not interpret assertion truth or implement URI resolution.
Explicit `$id` must resolve uniquely inside the package namespace. Reject a second
physical resource claiming the same canonical URI, duplicate anchor in its resource,
or a target outside the admitted package as unresolved_reference at closure.
Resource/root aliases do not permit overwriting an earlier declaration.

Standalone-schema resolver scope follows actual SDK subresource traversal. An OAS
document is not a Draft202012 schema root: registering its whole JSON as though it
were one does NOT supply Schema Object scope. The REQUIRED proposed namespace bridge
uses public referencing.Specification(name, id_of, subresources_of, anchors_in,
maybe_in_subresource), with callbacks driven ONLY by the bounded indexed role metadata:

- OAS document/container nodes have no schema id/anchors. Their subresources callback
  exposes the actual indexed Schema roots; it never guesses from data key spellings.
- At schema positions, id/subresources/anchor callbacks delegate to native DRAFT202012.
  Structural-container→typed-Schema transitions in maybe_in_subresource enter the
  actual child through the SDK resolver.in_subresource. Schema-descendant transitions
  use native Draft semantics with the correct schema-relative segment context.
- Register this Specification's resource for the actual OAS document URI, alongside
  complete native schema resources. This bridge MUST govern native instance `$ref`
  lookup of `api.json#/.../Schema` too; merely fixing our closure-pass scope is insufficient.
  It must preserve schema roots WITH and WITHOUT $id, nested relative ids/anchors,
  cross-document targets and raw `$ref` siblings. Unknown contexts cannot masquerade
  as Schema; malformed contexts stay for native invalid_schema at its earlier phase.

No assertion checker, resolver replacement or transformed document is introduced by
the metadata bridge. Retain the actual SDK entry scope on each schema record. Lookup
targets map back to physical typed records and their scope, not guessed path prefixes.
The separately authored5d8c962 fixed Specification recipe is only a source hypothesis
pending different actual review/run; it covers ONE known OASroot→Sample transition
with a relative root id/nested id/anchors. It does not prove the general role bridge,
no-id roots or all native instance lookups. Those remain exact integration/test gates;
the earlier standalone pointer fact supplies none of this OAS-container proof.

For RAW manifest pointers, first admit RFC6901 escapes. To create a URI fragment,
UTF8-percent-encode pointer bytes except unreserved bytes and '/', preserving ~0/~1;
literal %→%25, #→%23 and non-ASCII→its encoded UTF8 bytes. Prefix '#'. Native lookup
consumes the URI fragment and performs its own percent decoding. Thus raw key
`p%2Fq` has pointer `/$defs/p%2Fq` and lookup fragment `#/%24defs/p%252Fq`; key a/b
uses `#/%24defs/a~1b`, key a~b uses `#/%24defs/a~0b`. Never URI-decode a raw pointer or
percent-decode a filesystem path. URI references already use URI-fragment syntax;
they must NOT be encoded a second time as if they were raw manifest pointers.

Complete typed closure enumerates EVERY reference occurrence at indexed reference
positions, including unused schema definitions and unused OAS components. For each,
use the actual scope's SDK resolver.lookup; verify the returned contents and stored
physical identity have the required role: Schema→Schema; each OAS Ref→its expected
OAS role. An arbitrary data mapping or equal JSON elsewhere cannot satisfy it.
JSON pointers may land only on actual indexed targets of that required role; anchors
are schema targets. Record both source occurrence and target identity. Missing URI,
pointer, anchor, ambiguous alias or wrong role is unresolved_reference. No ordinary
instance-validation success or Registry.crawl success can substitute for this pass.

Package lookup uses ONLY this prebuilt registry. Never instantiate SchemaRetriever,
from_file/from_path/validate_url, default handlers or a fallback URL reader for a
package URI. `handlers={}` is explicitly insufficient: missing schemes fall back.
The proposed SchemaPath cursor must be constructed on the admitted SDK resolver,
not from_dict's automatic fallback retriever. Native imports/official resource reads
must also stay within the actual admitted installation, with exact reviewed bundled
resource provenance; no wholesale file:// permission or network/cache resolution.

## 3. Finite graph ownership and cycle partition

All physical-role records, target/alias views and work frames belong to the one
worker. Across the COMPLETE graph: at most4096 context records,256 actual schema
resources,2048 reference edges,4096UTF8 bytes per URI/pointer key and1048576 total
materialized key bytes. Every retained spelling/alias occurrence counts; shared
content or interned pointers do not create a fictitious free budget. Reserve/check
record and key additions before constructing/adding them; crossing any ceiling is
resource_limit. Role and graph edges store bounded record indices, not copied trees.
At most4096 graph visit/frame records and4096 outgoing-edge records are retained:
one per non-root schema-child record plus the separate2048 reference-edge table;
these are separate tables, not an assertion that total edges fit4096. Walk all
remaining targets after a cached hit; a visited record cannot hide an unvisited edge.
Library/cache/intermediate allocation still obeys worker memory/deadline limits.

Build an evaluation graph over schema records AFTER complete closure. Add schema
application edges from the table and every static `$ref` edge as same-instance.
Inventory-only children seed independent checking but are not evaluation edges.
Every same-instance-only directed cycle is reference_cycle, even in an unused
schema or branch; this is the existing conservative logical admission rule, not an
attempt to prove whether a conditional is reachable. A productive graph cycle must
cross a descending edge. propertyNames applies to one finite member-name facet;
that child is a string, not the original containing object. Native validation owns
actual branch/unevaluated/property-name semantics; the graph does not emulate them.
OpenAPI reference-only alias cycles similarly refuse as reference_cycle before
native semantic recursion. No RecursionError catch, timeout, success cache or node
deduplication supplies a cycle proof. Full finite instances still run real validators.

## 4. Independently complete example inventory

Inventory DATA occurrences without traversing their `$ref`/`$id` contents. Every
external manifest instance must be used. Required inline occurrences are Schema
example/examples entries, MediaType/Parameter/Header example, and every Example
Object value INCLUDING unused components and the resolved local Example references.
Schema default/const/enum are literal data, not new manifest examples. Native schema
checks still apply; standalone2020-12 default is an annotation, not an added instance
assertion. OAS native default checks retain their owning behavior. Null is a present
value, not a missing one.

Identity of a value is its physical document+canonical pointer (external files use
file identity/root), not a use-site alias or equal JSON. A referenced Example Object
contributes its actual target value pointer; do not bind to the `$ref` object itself.
Each required occurrence has an associated raw schema identity: containing Schema
for Schema example/examples; actual sibling schema for MediaType, Parameter/Header
schema form. Content form associates through that actual MediaType. No convenient
permissive target or dereferenced replacement for a schema `$ref` object is accepted.
An example in such a use context without an associated schema is unsupported_feature
under this finite profile, not silently omitted or validated against implicit true.
FREE component Example values mean precisely ZERO forced use-schema associations
anywhere in the complete package: require at least one explicit binding to an actual
indexed schema. Once such a component value is used with an associated schema, it
is no longer free and permits ONLY its complete set of forced schema associations,
with no arbitrary extra target. Reuse in several contexts requires every
distinct associated schema binding; exact repeated use of the same pair needs one.

Normalize manifest sources/targets to these physical identities only AFTER typed
closure; this does not normalize Unicode/data or execute references inside values.
Every required `(value identity, associated schema identity)` pair must occur EXACTLY
once in admitted bindings. Every inline binding must be such a required pair OR
an explicit actual-schema association for a free component value; each free value
needs at least one such association. No other inline pair is allowed. External
sources may explicitly bind to several actual schemas.
Reject missing/extra/misdirected bindings and resolved duplicates as invalid_inventory.
An inline binding to arbitrary data outside this inventory is extra, not a shortcut
to turn an annotation/property into an example. All pairs/bindings share the same256
ceiling; required inventory growth beyond256 is resource_limit before truncation.
Then validate EVERY admitted binding once in manifest order. Removing a bad declared
example from the manifest cannot make this equality pass. Native spec success alone
does not establish inventory completeness or any example instance's validity.

## 5. Ordered real backend delegation and proposed raw-value cursor

After the API's package/dialect/format/dependency phases, complete EACH substage
before starting the next. Ordering within a stage is manifest document order then
unsigned UTF8 canonical physical-pointer order. Resource failures may interrupt.

1. Admit the exact tool location/profile above; unsupported_feature refuses the
   documented unsupported locations/dynamic graph shapes, not ordinary literal data.
2. Native structural/metaschema checks→invalid_schema: real bundled OAS3.1 structural
   metaschema with Draft202012Validator(meta, registry=official_no_fetch).validate
   against the unchanged whole OAS document; real OAS31Validator.check_schema
   or Draft202012Validator.check_schema against EVERY raw indexed schema, including
   unused locations and `$ref` siblings. Select by the admitted dialect, never latest.
   Official metaschema/validator registries remain preloaded/no-fetch. This explicit
   ordering must not call full SpecValidator semantics before closure/cycle/inventory.
3. Complete typed reference closure→unresolved_reference.
4. Complete same-instance and OAS alias cycle partition→reference_cycle.
5. Complete inventory equality and associations→invalid_inventory.
6. Native semantic checks→invalid_schema: OpenAPIV31SpecValidator on the unchanged
   document through the proposed context/raw-data cursor; also evaluate EVERY present
   OAS schema default against its full RAW containing schema with the real selected
   validator and exact stored SDK scope. This covers defaults in unused OAS schemas
   and schema `$ref` siblings that native SchemaPath traversal could otherwise omit.
   Do not impose that extra instance assertion on standalone2020-12 annotations.
7. Finally evaluate every bound instance with OAS31Validator for OAS base dialect,
   Draft202012Validator for explicit2020-12, the complete in-memory registry and its
   exact lexical resolver. Wrong instance→invalid_instance; all stages passing→ok.

Instance/default validator construction is `(raw_schema, registry=registry,
_resolver=entry_resolver)`, then validate(raw_value). This is a PINNED integration
hook already used by upstream OSV, not a stable cross-version public-library promise;
its exact embedded-scope recipe requires the additional fixed integration fact.

Native API/exception dispatch is pinned, not injectable by a manifest. Structural
SchemaError/ValidationError and native semantic ValidationError map to invalid_schema;
binding evaluation ValidationError maps to invalid_instance. Exact reference lookup
exceptions map only in the closure stage. An unexpected SDK failure, unavailable
required cursor/registry mechanism or internal inability is validation_unavailable;
missing/mismatched installed dependencies remain dependency_unavailable. No broad
catch reports an invalid package merely because this adapter cannot validate it.

The REQUIRED proposed extension point is a PRIVATE SchemaPath subclass overriding
public read_value(self) to return the unchanged raw decoded value at DATA-role positions,
with inherited child cloning retaining that subclass and accessor. All other reads
delegate to the real superclass. Its constructor must retain a real SchemaAccessor
bound to the admitted actual SDK resolver: proposed construction is
SchemaAccessor(document, registry.resolver(base_uri=document_uri), resolved_cache_maxsize=0),
then the SchemaPath subclass on that accessor. Raw DATA returns borrow the worker's
one decoded tree, never copy/transform it. This is not a custom successful validator. No
global library monkeypatch, sentinel object, stripped `$ref`, annotation deletion,
document rewrite or fake resolver result is permitted. Cursor lookup uses the exact
resolved physical role index; the returned data may contain arbitrary `$ref` values.
Schema evaluation remains real SDK work on raw schemas, including `$ref` siblings.

The separately authored fixed cursor experiment tests ONLY one literal default path,
native positive AND negative validation, clone propagation and zero retrieval. Until
actual independent review/execution it is a PROPOSAL. Even if green, general alias-
aware role selection, parameter defaults, raw data reads through resolve()/membership,
scope transfer, every schema/default and complete offline integration still require
actual source/API audit and independent wrapper tests. Do not claim plain SpecValidator
or that single experiment covers them. If the proposed integration cannot preserve
all these requirements, STOP the baseline and record the exact blocker; no profile
refusal of literal default/const/enum/example `$ref` data merely to fit the SDK bug.

## 6. Independent literals, evidence and remaining gates

These are document oracles, not executable fixtures or observed wrapper results:

- Same root-schema literal `{"$ref":"#"}` has a same-instance self-edge: reference_cycle.
  Root with properties.next→same root is productive; finite matching objects must
  reach real instance validation. `$defs.unused` with an outside ref refuses closure
  even if an integer root and example1 would otherwise pass.
- OAS Schema `{"type":"object","properties":{"$ref":{"type":"string"}},"required":["$ref"],"additionalProperties":false,"default":{"$ref":"https://outside.invalid/data"}}`
  must retain that literal default, no retrieval. Replacing ONLY its default value
  with `{"$ref":7}` is invalid_schema in native default checking; nothing is stripped.
- For OAS schema component S with examples `[1,"wrong"]` and type integer, required
  raw source pointers are `/components/schemas/S/examples/0` and `/components/schemas/S/examples/1`,
  both targeting `/components/schemas/S`. One binding is invalid_inventory; both
  bindings reach invalid_instance. An unused component example value still binds.
- A map property NAMED `$ref` with schema `{"type":"string"}` creates an ordinary
  schema child. A const/enum/example value containing an outside `$ref` creates no
  reference edge; a Schema Object `$ref` at the same physical-looking key DOES.

Primary authority: [OpenAPI3.1.1 objects/references/examples](https://spec.openapis.org/oas/v3.1.1.html),
[Draft2020-12 core scope/applicators](https://json-schema.org/draft/2020-12/json-schema-core.html).
Pinned source: [Draft202012 resource semantics](https://raw.githubusercontent.com/python-jsonschema/referencing/v0.37.0/referencing/jsonschema.py),
[resource/pointer/resolver](https://raw.githubusercontent.com/python-jsonschema/referencing/v0.37.0/referencing/_core.py),
[OSV native checks/default reads](https://raw.githubusercontent.com/python-openapi/openapi-spec-validator/0.9.0/openapi_spec_validator/validation/keywords.py),
[SpecValidator stage calls](https://raw.githubusercontent.com/python-openapi/openapi-spec-validator/0.9.0/openapi_spec_validator/validation/validators.py),
[SchemaPath extension](https://raw.githubusercontent.com/p1c2u/jsonschema-path/0.5.0/jsonschema_path/paths.py),
[SchemaAccessor construction](https://raw.githubusercontent.com/p1c2u/jsonschema-path/0.5.0/jsonschema_path/accessors.py).
These support the stated facts/risks, not a successful new arbitrary-package recipe.

Remaining: actual different context-document review; additional reviewed/observed
cursor and OAS-fragment/scope integration facts; actual installed SDK/resource/offline
and process harness audit; minimal inert interfaces only after these gates; complete
independent fixtures/oracles and test review; intended missing-wrapper red; later
SCHEMATOOL001 implementation/source review/passing ordinary CI. Logical/public API
requirements and every CONTRACT013/runtime/platform/live/full-release gate remain.
