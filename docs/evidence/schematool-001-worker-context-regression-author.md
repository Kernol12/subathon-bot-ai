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

## Reviewed FIRST observation, then correction

Root independently approved the exact test source in9aedf4f (root importbf8b4b9
unchanged6894a22). ONLY this new module ran against unchanged worker9f88767:

```sh
/tmp/subathon-schematest001-py314/bin/python -I tests/schema_validation/worker_context_regression.py
```

FIRST start2026-11-06T12:11:07.766722Z, nativeexit1,3 methods/7 observations,
3 assertion failures/0 errors, suite1.453s/wall1.515838s, stdoutEMPTY. All4 positive
controls passed. Actual bad raw-sibling case returned ok rather than invalid_schema;
OAS Schema x-field returned ok rather than unsupported_feature; Parameter Ref
x-field reached invalid_schema instead of the earlier unsupported_feature gate.
The raw files `/tmp/schematool001-worker-context-first.stdout`, `.stderr`, `.json`
are preserved; stderr4158bytes SHA256
`f93e8925f83b0925c11e41eee10131f33833f8dbe8c1078db498f9c893cb4cbb`.
No original baseline method or prior FIRST was replayed. This is a reached-bug
regression with genuine SDK/process operation, not a missing dependency/import red.

After this observation the owned cursor was corrected to alias ONLY actual OAS
Reference Object roles; Schema nodes retain raw siblings/identity and actual scope.
The existing OAS structural x-field profile gate now precedes both Schema and
Reference Object returns; dynamic names, DATA and standalone keywords retain their
positive behavior. The SAME unchanged new module then PASS3/7 in1.451s. Native
regression bytes and observed pre-fix failures are not replaced by the green run.
Different source review and matching integrated/public/hosted gates remain open.
