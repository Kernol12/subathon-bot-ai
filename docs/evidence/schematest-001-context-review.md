# Independent context-document review — SCHEMATEST-001

Reviewer `/root/session_plan_review`, different from document author
`/root/session_supplier_review` and integrator `/root`, 6 November 2026.
**APPROVED DOCUMENT ONLY**, exact revision3 candidate
`a8b3dc130e2975a4746566725eb11175d55f77c9`, context blob
`32ec87a25dca2d39227a7fe994df3a2d908a26e6`, author-note blob
`92a1a7efc736965cb0d453a5d087fc39a8375c86`.

Actual full document/diff read on NEW isolated branch
`task/SCHEMATEST-001-context-final-review`, worktree
`/tmp/subathon-schematest001-context-final-review`, from that candidate. Only this
review note changes. The earlier review tree from46/d9 and every historical source,
receipt, assertion and FIRST invalid probe remain preserved. Root owns task state,
HTML, integration and GitHub publication; this reviewer changes none of them.

## Authority and exact review chain

Reviewed against specification v0.45 §§17/18, API-01/02/03 and TEST-04, root agent
instructions and workflow, accepted logical SV01–20 and accepted concrete public
API. Actual candidate retains logical blob
`abb426a91173726a703cba70d6a68225de483bdd` and public API blob
`691270f0ff661872e3af5fb061a5df35a2baa48b` byte-for-byte. Original46/d9 author
branches contained the older227 public document: I used actual accepted438/API691
directly rather than treating that stale branch copy as authority. The final
candidate contains accepted691 itself.

I read initial46bf7de, corrected d9e39b7/revision2, root literal correction
`d296f111ec834761f39f1cefc967bf8cd4c3cfe4` (context blob
`595de5440b748577c90d102d3e224cc937d11bf2`) and actual revision3. Relative to its
root846c243 base, revision3 changes ONLY the context document and its author note.
This verdict approves the resulting document, not an earlier prospective recipe.

Primary authorities are [OpenAPI3.1.1](https://spec.openapis.org/oas/v3.1.1.html)
and [JSON Schema2020-12 core](https://json-schema.org/draft/2020-12/json-schema-core.html).
The selected-library source and actual SDK fact reviews below support feasibility;
they do not replace either standard or provide arbitrary-package acceptance.

## Findings resolved in the actual text

- The closed OpenAPI role table identifies actual Schema/Reference/Example/Data
  positions, including unused components and schema children. Literal default,
  const, enum and example payloads are atomic DATA views, never searched for
  keyword-looking keys or rejected because they contain `$ref`/`$id`. All values
  still count toward complete strict JSON parse budgets. A property named `$ref`
  or discriminator is distinguished from a real keyword at a Schema role.
- Early dialect/format admission uses only the bounded pure physical role table
  before candidate imports. SDK resource/scope fields remain empty until dependency
  admission. Malformed children remain pending native structural failures rather
  than becoming permissive data or requiring SDK use in the wrong public phase.
- Complete typed closure examines unused references, exact physical target roles,
  URI/anchor collisions and aliases. Native instance success/crawl does not close
  an unused reference. Same-instance cycle rejection and descending edges preserve
  logical v1's explicit conservative cycle rule; no custom assertion evaluation,
  RecursionError catch or timeout pretends to establish it.
- Context revision2 explicitly gates package Schema.discriminator: OpenAPI §§4.3
  and4.8.25 give its implicit names/mapping URIs real schema connections that this
  finite closure does not implement. Dynamic references/anchors, undefined mixed
  PathItem-reference merges and use-context examples without associated schemas
  are also explicit TOOL profile limitations. They do not declare standard OAS
  invalid, remove product requirements or scan/refuse admitted trusted SDK
  metaschemas. A later reviewed profile extension remains required when needed.
- Example inventory equality preserves every required physical value/schema pair,
  including unused component Examples and multiple forced uses. A free component
  Example means zero forced associations; once used, only all its forced pairs
  are allowed. Missing/extra/aliased duplicate bindings cannot hide a bad example.
- Canonical URI-fragment examples now encode `$defs` as `%24defs` without changing
  raw RFC6901 pointers. Literal `%`, `#`, UTF8 and `~0`/`~1` handling is explicit;
  an already encoded URI reference is not encoded again.
- Native stage order follows the accepted public priority: profile admission,
  structural/metaschema checks, complete typed closure, cycles, inventory equality,
  native semantics/defaults, then every bound instance. Structural/default errors
  remain invalid_schema; bound-instance errors remain invalid_instance. An
  unavailable adapter/internal exception cannot falsely classify a package invalid.

## Real library delegation and bounded public route

The public Specification bridge supplies metadata and lexical scope only. Real
Draft resource/id/anchor/subresource semantics and native validators retain all
assertion work. Whole-OAS fragment instance references must use that same bridge;
a correct closure-only resolver is explicitly insufficient. Original raw documents,
schemas, `$ref` siblings and literal data remain intact.

Actual revision3 removes a mandatory private `_resolver` constructor dependency.
Each native instance/default evaluation uses an owned immutable one-key control
schema whose ordinary `$ref` identifies the admitted document plus canonical RAW
schema fragment, with target-dialect OAS31/Draft202012 class and the actual registry.
This is native reference entry plumbing, not a replacement schema, permissive
validator, caller-injected verdict or transformed package. Optional later private
hooks require their own proof; upstream usage alone admits none.

The complete encoded absolute URI, including prefix/fragment, is counted BEFORE
materialization against existing4096-byte per-key and1048576-byte aggregate limits.
Every control-key/URI copy and retained alias counts separately. A4096-byte raw
pointer may fail after percent expansion. No ceiling increases or free shared-string
ownership is inferred. Finite context/resource/ref/frame tables and worker memory/
deadline limits remain separate, complete bounds rather than truncation permission.

The raw-value cursor remains a private SchemaPath subclass overriding the public
read_value hook only for actual DATA roles, borrowing the raw tree and delegating
all other reads. No sentinel/fake validator/stripped keyword or global SDK patch is
approved. General alias-aware cursor selection, resolve/membership reads, defaults,
schema siblings, scope transfer and no-fetch integration remain acceptance and
implementation/source-review obligations.

Pinned primary source [Resource/Specification/resolver](https://raw.githubusercontent.com/python-jsonschema/referencing/v0.37.0/referencing/_core.py)
confirms public callback signatures, pointer segment reset on resolver identity
change, no-ID in_subresource returning the same resolver and fresh public
Registry.resolver construction. [Native validator reference descent](https://raw.githubusercontent.com/python-jsonschema/jsonschema/v4.26.0/jsonschema/validators.py)
uses the actual resolved contents/resolver. These two versioned primary files were
reopened for this final review; installed text was read without candidate imports.
Prior cursor/source reviews preserve the selected SchemaPath and OSV source audit.

## Actual evidence and released next step

Dependency admission [review](schematest-001-dependency-review.md), public API
[review](schematest-001-api-review.md), original context-fact
[review](schematest-001-context-foundation-review.md), fixed raw-default
[review](schematest-001-literal-cursor-review.md) and fixed relative-ID OAS scope
[review](schematest-001-oas-scope-review.md) remain valid for their exact artifacts.
Root's retained [native receipts](schematest-001-foundation-cursor-scope-native.md)
record separately reviewed source867fff5/twelve OK0.120s and30169ef/thirteen OK0.116s
on the admitted CPython3.14 environment. Those are integrator observations, not
executions by this reviewer or general adapter proof.

The separate no-ID source review, `docs/evidence/schematest-001-no-id-scope-review.md`
at commit `8ba72eb` (separate integration pending on this base), approves actual
candidatea80bc21 plus note-only94ceadb, exact source
`6585db8f7c2da966158ae0e0946d41932b1b654e`, FOR CONTROLLED EXECUTION. Its ordinary
same-resolver control reaches a physical target but loses anchor scope; the fresh
same-base public resolver fixes that one known transition. Both real native
validators use a WHOLE-OAS-fragment control reference with positive/negative
assertions and zero retrievals. No actual fourteen-method run is claimed here.

With this document approval, the remaining finite pre-stub feasibility gate is
the actual passing reviewed no-ID source run, plus integration of accepted exact
documents/dependencies/foundations. Current ordinary CI must exercise the promoted
actual foundation revision; earlier7/11/13-method receipts cannot prove14. No
required-check enforcement is inferred from merely adding a workflow.

After these finite gates, independent wrapper fixtures/oracles/tests and the exact
two immediate named inert feature stubs may be authored with the minimal harness.
Different actual test review, selected green foundations and a working minimal
environment precede the genuine intended missing-wrapper red. Failed imports,
process/setup failures, skips or SDK defects are not that red. General production
role/alias/cursor/descriptor/process/resource/offline behavior must NOT be implemented
as a prerequisite to writing its tests. It needs independent acceptance cases and
later SCHEMATOOL001 implementation/source review/passing relevant ordinary CI.
This approval supplies no executable API, wrapper test baseline/red, working worker,
full offline guarantee or product/runtime/platform/live/full-release acceptance.
CONTRACT013, all seven integrations and all three FX providers remain in scope.

## Checks actually run

In the isolated final-review tree: `python3 tools/check_project.py` PASS426tasks/
122decisions/local links/presets/current HTML; `python3 tools/progress.py render
--check` PASS; `git diff 846c243..a8b3dc1 --check` PASS and exact two-path scope.
Independent standard-library git/blob/urllib.parse assertions PASS accepted API/
logical identity, all three canonical fragment literals and round trips, Unicode/
percent encoding, expected relative URI base and a fitting raw-pointer expansion
counterexample exceeding4096 encoded bytes. No candidate import, probe/native
validator/test/stub body, install, environment edit or GitHub write ran here.
