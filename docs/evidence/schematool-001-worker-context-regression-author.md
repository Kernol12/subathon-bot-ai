# SCHEMATOOL-001 raw-context regression author handoff

Exact implementation candidate9f88767 remains unchanged. Own this new note and
`tests/schema_validation/worker_context_regression.py` only in this revision;
root explicitly assigned the additive focused test. Frozen64f491 baseline and
all contracts/pins/public/worker sources are unchanged. No test/worker/SDK import,
child execution, intended-red run or fix has occurred for this regression yet.

SOURCE finding: `_schema_backend.py` ContextAccessor.context unconditionally calls
alias for both Reference Object and Schema records. A Schema `$ref` must retain
its raw identity and all siblings under accepted context §1 and §5; full native
semantic validation cannot see only its target. Native structural checks and
explicit raw defaults cover different obligations, and do not make this semantic
omission safe. Pinned openapi-spec-validator0.9.0 validation/keywords.py
SchemaValidator.__call__ lines220–235 checks required names when allOf is present
and emits ExtraParametersError when absent from local/allOf properties. Its
earlier recursive/allOf checks leave the synthetic empty-B/empty-allOf fixture
structurally legal. Therefore A's unchanged raw allOf+[missing] sibling must
produce native invalid_schema even with a local `$ref` to empty B. The positive
control changes only required to[] and must pass. This is a native-semantic
requirement/oracle, not a new general claim that JSONSchema required needs
properties; the selected native OAS semantics are explicitly required by §5.

The same accepted profile explicitly defers custom extension fields at structural
OAS positions (logical v1 lines191–198; context lines68–73). Current Index.walk
returns before that check for Schema and `$ref` Reference Object records. Two
literal unsupported_feature cases cover a real OAS Schema x-extra FIELD and a
Parameter Reference Object x-extra FIELD with an actual valid local target.
Three positive controls preserve a PROPERTY NAMED x-extra, raw default DATA with
x-extra and outside-looking literal `$ref`, and standalone Draft unknown-keyword
semantics. This does not narrow or rewrite product/standard scope; these are the
existing finite tool-profile boundaries.

The focused source has3 methods/7 literal genuine-worker observations. The helper
uses the frozen real worker argv/minimal environment, actual selected venv SDK,
real no-site child/FDs, finite wait/read sizes, exact A/outcome EOF+status, and
owned cleanup. No successful fake validator, public injection, SDK replacement or
implementation-generated expectation is used. Definition-only imports are stdlib;
sysconfig discovery happens only when a method actually invokes the helper.

Checks actually run: Python3.10 AST/compile-only on the new test source, project
check, render--check and staged whitespace check. No candidate/test imports or
method execution. DIFFERENT actual regression source/oracle review is required
before the intended reached-bug run; record its exact native failure before any
production fix. Existing baseline/author observations are not projected onto this
new source or a future fix. Independent implementation/source review and full
public acceptance/current hostedCI remain separate task gates.
