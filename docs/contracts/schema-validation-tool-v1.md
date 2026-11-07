# Repository schema validation — v1, revision 2

TESTCLEAN-001; authority: specification v0.45 §§17–18, API-01/02/03, TEST-04,
[ADR-0008](../decisions/0008-schema-validation-foundation.md) and
[ADR-0012](../decisions/0012-test-suite-simplification.md). This is the current
project-check contract. Actual test/oracle review, execution, source review and CI
remain separate gates; a document does not establish passing implementation.

## Scope and superseded surface

Validate source-controlled repository schemas and independently authored examples
with the selected real standards libraries. The current public artifact is
`schemas/http/reference-v1/api.json`: OpenAPI 3.1.1, 17 component schemas, and all
23 positive example bindings in its manifest. Scene and WebSocket directories
contain planned work, not schemas or supported capabilities. Also check the separate
Draft 2020-12 editor schema `progress/state.schema.json`, with actual progress data
and small synthetic structural examples.

This revision retires the generic arbitrary-package developer API and its six old
logical/API/context/installation/process/native profiles. The old `validate_package`,
18-outcome enum, fixed CLI/help/status/redaction protocol, hostile-file/snapshot and
lexical/number/graph budgets, exact rational decoder, generic context/cycle/inventory
engine, worker protocol, ten-second/512 MiB enforcement, native ABI/ELF/kernel and
allocation/cleanup guarantees are not replacement-check capabilities. No alternate
subprocess owner or compatible generic API is introduced. They were engineering
promises of that developer tool, not required product features. Their exact source,
assertions, reviews and runs remain in the
[archived logical contract](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/contracts/schema-validation-tool-v1.md)
and linked archived contracts; they must not be relabeled as new checks or deleted
from historical evidence.

Product raw HTTP decoding, credential handling, permissions, exact money/score,
queue/thread ownership, current-state provenance and runtime resource/security
requirements remain unchanged. Schema shape is no proof of any of those, provider
support, a running endpoint or release readiness. All seven integrations, three FX
providers, clients/renderers/locales/themes and platform/release gates remain.

## Required artifact and example checks

- Check the full unchanged OpenAPI document with its explicit 3.1 semantic and
  structural/metaschema validators. An independently invalid document example must
  be rejected by those real validators.
- Check every actual component schema definition and provide independently derived
  valid and invalid instance cases for each named target. Case target names must
  match the actual component inventory exactly, without silently skipped new roots.
- Validate every existing manifest binding and example file. Existing unknown offered
  version/capability strings are structurally valid; runtime compatibility is separate.
  New negative cases cover canonical string bounds, required/extra fields, array
  uniqueness, opaque bytes and status-specific error envelopes from the HTTP/common
  wire contracts. Expected results never come from executing the validator.
- Close every current local-fragment reference, including unused components, using
  the real in-memory resolver before instance checks. Instances enter through the
  whole document context, never a detached component losing its local reference root.
- Classify every JSON file below `schemas/` as schema, declared manifest or bound
  example. A new/unclassified artifact or missing example/case is a failing check,
  not an implicit supported feature. The current API has local fragments and no
  nested IDs, anchors, defaults or inline examples; no arbitrary-package/context
  support is inferred from it. Keep literal data opaque when collecting references.
- Check the progress schema and positive/negative structural instances independently.
  It is an editor hint, not the runtime DAG/review/combined-500-note validator. No
  FormatChecker is supplied: `format` is annotation-only there; the actual public
  API currently has no format keyword. No optional format dependency is selected.

Read repository fixtures as UTF-8 JSON, rejecting duplicate decoded object keys and
NaN/Infinity for source-file correctness. This is not the former generic exact
rational/lexical-resource or adversarial-filesystem contract. Numerical domain wire
values remain strings; no floating-point shortcut for money/score is introduced.

## Selected standards libraries and offline resolution

Reuse the admitted exact 20 distribution versions, whole-wheel hashes, paired
installed metadata identities and license declarations in the existing
[lock](../../tools/schema_validation/requirements.lock) and
[manifest](../../tools/schema_validation/wheels.json). No global installation,
automatic validator-time install, new dependency or server/client runtime link.

The engines remain OpenAPIV31SpecValidator 0.9.0, OAS31Validator 0.9.0,
jsonschema 4.26.0 Draft202012Validator, referencing 0.37.0 and jsonschema-path 0.5.0.
Retain dependency-only standard-library metadata admission before imports, one
checked SDK root and `-I -S -B` startup. Existing exact Linux x86-64 CPython 3.10,
3.12 and 3.14 artifact profiles remain; selected hosted runs use 3.10 and 3.12.
This is a trusted maintained installation, not protection against a malicious admin.

Resolve schemas through an explicit in-memory registry with denied retrieval.
Use the already admitted bundled OpenAPI 3.1 metaschema (34,260 bytes, SHA256
`e7cb616a2a10849a166c4e4a93c62c56cfea02cc00eadf287e2fb875e7124098`)
and the real selected structural validator in the semantic validator; do not invoke
a lazy URL-reading metaschema wrapper. SchemaPath receives a SchemaAccessor bound
to that real registry, without default URL/file fallback. Missing targets fail,
never fetch. No transformed schemas/data, fake validating callback or skipped backend.

## Execution and revised baseline

The replacement is ordinary data-driven unittest/CTest over trusted repository
artifacts, with opt-in `schema-validation-debug` and label `schema-validation`.
It introduces no public generic validator API or new schema engine. Real library
rejection of negative instances is an expected check, not an unimplemented-feature
red. Do not manufacture red evidence by missing dependencies, parse/setup failures
or skips. Genuinely new first-party behavior still requires the normal test-first
sequence before implementation.

The old generic-wrapper 74-method map, native owner/fault rows, loader/binding/
assembly/resource controls and library-fact foundations are deliberately retired
under the separately reviewed cleanup decision. Their guarantees belong to the
removed surface; tests must not become unreachable mocks or renamed green counts.
Preserve exact dependency admission and replace synthetic standards-engine checks
with real current schema/examples on both selected hosts. CI installs the unchanged
hash lock in an isolated venv and runs all new checks unconditionally, plus ordinary
infrastructure. CTest uses `--no-tests=error`; unavailable SDK or unexpected validator
failure is failure/error, not successful validation. CI timeouts are infrastructure
containment, not a replacement worker deadline guarantee.
