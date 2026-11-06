# SCHEMATEST-001 fixed OpenAPI resource-scope fact author

Author `/root/session_supplier_author`, 6 November2026. NEW isolated branch
`task/SCHEMATEST-001-oas-scope-author`, worktree
`/tmp/subathon-schematest001-oas-scope-author`, based on actual root
`231886c207d7705dfcc32141f8ede3b43f9ff993`.
Own ONLY [foundation program](../../tests/schema_validation/library_foundation.py)
and this NEW note. **AUTHORED, NOT EXECUTED; different source review required.**
Candidate source blob `30169efc7e2ad27d9a7257b925ad0ae44afabf64`.

Add exactly ONE small fixed SDK feasibility method. All prior12 methods, setUp,
module helpers/imports, persistent guards and exact trusted-byte recipe remain
AST-IDENTICAL to231886c. Prior11 and cursor12 independent reviews/observed receipts
retain their actual scope; no future13-method pass/hosted result is inferred.

The fixture demonstrates a PUBLIC referencing.Specification seam. ONLY its known
OpenAPI root has no schema ID/anchors and supplies the one known Sample Schema
Object as a child. Every enumerated schema descendant delegates native Draft202012
id/children/anchors. Its maybe_in_subresource callback recognizes ONLY the fixed
components/schemas/Sample location, enters the actual native resolver subresource,
then delegates native Draft202012 scope rules below it. No arbitrary object-role
classifier, custom standards engine, raw-schema rewrite or private validator hook
is implemented. Treating any generic non-root object as schema is NOT authorized
by this fixture; full production context tables remain separate gates.

Literal independent expected scopes:

- document https://local.invalid/package/api.json;
- Sample$id=schemas/sample.json gives
  https://local.invalid/package/schemas/sample.json;
- nested leaf$id=nested/leaf.json gives
  https://local.invalid/package/schemas/nested/leaf.json.

Real Registry.crawl must register EXACTLY those three resources, excluding the
literal default object's outside$id/$ref data. A WHOLE OpenAPI fragment lookup
of components/schemas/Sample/$defs/leaf must retain the leaf's actual resolver scope:
#leaf resolves its own resource; #integer resolves that resource's known anchored
integer schema. Sample lookup returns the actual unmodified Sample object.

Both real Draft202012/OAS31 validators then resolve the actual leaf URI through
that same registry, including its nested #integer property reference. Instance
{"value":7} must pass and {"value":"wrong"} must raise native ValidationError.
This uses ordinary public validator(schema,registry=...) calls; no private _resolver
injection, fabricated validating callback or boolean success is supplied. Recording
retrievals must remain zero, along with all unchanged network-tripwire cleanups.

Read selected installed source as TEXT, without importing/evaluating packages:
referencing._core.Specification's public callback constructor/create_resource,
Resource.pointer's context callback and segment reset after scope changes,
Registry.crawl's native id/anchor traversal, Resolver.in_subresource and native
Draft202012 callback rules. The native validator reference path carries its resolved
scope through actual reference descent; the fixture invokes that normal path.
These observations support the tiny source hypothesis; actual compatibility is
still unproven until different review and root's controlled run.

Static-only checks PASS: ast.parse, exact AST comparison of all12 prior methods/
setup/helpers/imports/guards, exactly ONE additive method yielding13, independent
standard-library urllib.parse.urljoin assertions for the two literal URI derivations,
git diff--check, project426records/122decisions/currentHTML and render--check.
No package import, foundation/native validator/probe execution, dependency install,
production role walker/CLI/engine, workflow/pin/state/HTML or GitHub write occurred.

Next: different actual source/oracle review by session_plan_review, THEN controlled
root execution and direct receipts. Root owns any later exact CI inventory change
after observed success. This proves ONLY the known fixture's public metadata/scope
seam, not a complete OpenAPI role index, offline/context/example/closure adapter,
Schema$ref sibling correctness or product validator. Full concrete document/API/
test-first/red/implementation/CI/platform/live/release gates remain.
