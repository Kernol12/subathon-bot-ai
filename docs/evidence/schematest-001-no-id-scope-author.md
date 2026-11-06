# SCHEMATEST-001 no-id OAS scope fixed fact — author

Author `/root/session_supplier_review`,6 November2026. NEW isolated worktree
`/tmp/subathon-schematest001-no-id-scope-author`, branch
`task/SCHEMATEST-001-no-id-scope-author`, from actual root1b4dee6.
Own ONLY [foundation source](../../tests/schema_validation/library_foundation.py)
and this NEW note. Root/tests instructions, accepted API691/proposed contextd9, actual
thirteen-method source30169 and pinned referencing source were read before editing.
**AUTHORED, NOT EXECUTED.** No selected package import occurred here.

Exactly ONE fixed method is added; every old13 method, setup/helper/import/guard,
main invocation and the ENTIRE remainder of the module AST remain unchanged.
No wrapper/parser/manifest/role engine, fixture file, private resolver hook,
dependency/pin/CI/state/HTML/GitHub change or installation is supplied by this probe.

The independent literal has one OAS document, Sample with NO $id, a nested relative
leaf id, leaf/integer anchors and an object property referencing #integer. Expected
URIs are hand-derived: document https://local.invalid/no-id/api.json; nested leaf
https://local.invalid/no-id/nested/leaf.json. Exactly these two resources exist.
Sample is not a third id-bearing resource. The whole native schema reference is
`https://local.invalid/no-id/api.json#/components/schemas/Sample/%24defs/leaf`.

Meaningful control: native in_subresource on that no-id Sample returns the SAME
resolver. Ordinary pointer traversal still finds the literal leaf, but its returned
scope cannot resolve #leaf, yielding exact NoSuchAnchor. The proposed fixed-context
correction returns a fresh public registry.resolver at the SAME known document base
at ONLY components/schemas/Sample. Resource.pointer then resets accumulated segments;
all subsequent Draft metadata/scope operations remain actual native SDK behavior.
Both real Draft202012Validator and OAS31Validator then follow the WHOLE OAS fragment,
accept {value:7} and reject {value:"wrong"} with actual ValidationError. This does not
select the leaf via a precomputed absolute leaf URI or inject a private _resolver.
The recording deny callback must remain empty; persistent original network poison
and independent cleanup are unchanged and cannot be satisfied by expected exceptions.

This is ONE known structural transition, not arbitrary role classification or a
general adapter. Callback identity checks exist only for the closed literal OASroot
whose sole known schema child is Sample; raw schema descendants use native Draft.
No private state mutation, successful fake validator/lookup, transformed schema or
catch-all acceptance is used. If the actual behavior differs, preserve the failure
and require separate specification-based correction/review; do not weaken it.

Primary selected source: [Resource.pointer/Resolver](https://raw.githubusercontent.com/python-jsonschema/referencing/v0.37.0/referencing/_core.py),
[Draft subresource semantics](https://raw.githubusercontent.com/python-jsonschema/referencing/v0.37.0/referencing/jsonschema.py).
Source shows pointer reset is conditional on resolver object identity, in_subresource
returns self when no id, public Registry.resolver constructs a fresh resolver, and
native Draft owns subsequent nested resource/anchor scope. These are source-derived
expectations only, not an observed green run or general backend guarantee.

Read-only baseline-readiness conclusion: after actual different source/oracle review
and green execution of this exact additional fact, plus final context-document
approval/integration, no generic adapter/role engine should be implemented as a
prerequisite to WRITING independent tests and inert immediate missing-feature stubs.
Actual SchemaAccessor construction is source-supported but its complete registry/
cursor integration belongs to later acceptance/source audit; the fixed cursor fact
does not prove arbitrary roles. This method deliberately avoids the private _resolver
hook: the native absolute-fragment path is the narrow meaningful alternative. If
production retains that hook, prove its exact scope in implementation acceptance;
do not claim this probe tested it. Trusted descriptor/no-site/RLIMIT/secure file/
resource loading/offline process behavior likewise need independent tests and real
implementation verification, rather than pre-test production. Minimal harness must
still establish the selected actual environment before genuine missing-wrapper red;
no dependency/import/process failure or skip can supply that red.

Actual STATIC checks only: ast.parse PASS; removing the one added method yields
the exact original13/module AST; no method decorators added; `git diff --check`
PASS. Document consistency/render checks recorded with the commit. No candidate
import, test/foundation run, installation, worker or live call occurred. Different
actual source/oracle review precedes root execution. Existing all13 source/reviews/
receipts remain valid for their exact scope; fourteen-method execution/CI is new
evidence and cannot be inferred. All logical/API/data/product/release gates remain.
