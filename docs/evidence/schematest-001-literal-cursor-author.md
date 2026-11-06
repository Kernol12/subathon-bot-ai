# SCHEMATEST-001 fixed raw-default cursor fact author

Author `/root/session_supplier_author`, 6 November2026. NEW isolated branch
`task/SCHEMATEST-001-literal-cursor-author`, worktree
`/tmp/subathon-schematest001-literal-cursor-author`, based on actual root
`5233ff4d4897c22e6182a41c610ff463441f3815`.
Own ONLY [foundation program](../../tests/schema_validation/library_foundation.py)
and this NEW note. **AUTHORED, NOT EXECUTED; different actual source review required.**
Candidate source blob `867fff53678846a2c1f41ff70be1452c5cb0bb30`.

The independently reviewed11-method source/actual local proof and earlier7-method
CI/history stay intact. Add exactly ONE fixed SDK feasibility method; all original
11 methods, setUp, module imports/helpers/tripwire/resource guards are AST-IDENTICAL
to5233. No dependency, production wrapper/parser, generic role traversal, manifest,
CLI, public API/schema, workflow/state/HTML or GitHub write is included.

Source-supported hypothesis: SchemaPath.from_dict creates the concrete subclass;
inherited pathable child cloning preserves its class/accessor, and public read_value
is the reader native OSV calls for a schema's default. A TEST-LOCAL subclass can
override read_value at ONLY the one exact static fixture path
(components,schemas,Sample,default), return that fixture's REAL unmodified raw
default object, and delegate every other read to super. No schema validation
verdict, fake validator, boolean success callback or general data-role decision is
returned by this cursor. An explicit assertion checks the derived path's subclass.

The real OpenAPIV31SpecValidator still performs actual structure/schema/default
validation. Its positive fixture has type object, a required ordinary property
NAMED $ref with type string, no additional properties and raw default
{"$ref":"https://outside.invalid/literal-data"}. Its negative fixture has exactly
the same schema but default{"$ref":7}. Positive validation must succeed; the
negative must raise native jsonschema ValidationError. The exact raw-reader ledger
must identify both real fixture values once, and the recording HTTPS denial callback
must have zero calls. Existing persistent DNS/socket/URI poison also remains zero
through unchanged cleanup. If context/class propagation fails, the literal URI
would reach the denier and fail rather than silently receive a successful result.

Read actual selected installed SOURCE TEXT without importing/evaluating packages:
jsonschema_path.paths.SchemaPath.from_dict passes cls to construction;
pathable.paths.AccessorPath._clone_with_parts/_from_parsed_parts preserve concrete
class/accessor; its read_value normally delegates to accessor.read; OSV
SchemaValidator.__call__ reads (schema/default).read_value and feeds that value to
its native default validator. The fixture override is intentionally at this exact
data-read seam, leaving native validation unchanged. This is a source hypothesis
for a tiny fixed compatibility fact, not a claim the general adapter already works.

No arbitrary schema context, references, URI namespace, resource budget, nested
data cursor, annotation inventory or Schema$ref sibling semantics is implemented
or proven by this fixture. The selected SchemaPath's eager schema dereference still
cannot by itself establish raw-sibling/default coverage. Actual complete raw-schema
checks, per-schema defaults, explicit-registry instance checks and bounded generic
context/cursor/closure/process handling remain concrete document/API/source/test
gates. Accepted literal-data semantics are preserved; the profile is not narrowed
to reject valid data merely because the SDK's ordinary traversal misreads it.

Static author checks ONLY: ast.parse and explicit AST comparison PASS for all11
old test bodies/setUp/module helpers/imports; exactly one additional test gives12.
`git diff --check`, project426records/122decisions/local links/currentHTML and
render--check PASS. No package import, selected SDK/foundation method, validator,
probe, install or native command ran here. Current hosted11/old7 evidence remains
its own actual revision, never prospective12-method proof.

Next: DIFFERENT actual source/oracle review by session_plan_review, THEN controlled
root execution and actual receipt. Root owns any subsequent exact CI inventory
change after a real passing run. Failure is a source/API foundation issue, not
wrapper missing-behavior red. Full wrapper baseline/test-first, later source/CI,
product/runtime/platform/live/release obligations remain.
