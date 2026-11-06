# Independent fixed no-ID OpenAPI scope SDK review — SCHEMATEST-001

Reviewer `/root/session_plan_review`, different from author
`/root/session_supplier_review` and integrator `/root`,6 November2026.
**APPROVED exact fourteen-method source/oracles for controlled execution only.**
Actual source candidate `a80bc21182b7a627012a7232496b797a46688493`, then author-note
precision `94ceadbf6184f22afc66bf7f0ea93b3d6aaa4fd1`, examined in NEW isolated branch
`task/SCHEMATEST-001-no-id-scope-review`, worktree
`/tmp/subathon-schematest001-no-id-scope-review`. Only this note changes.

Source [library_foundation.py](../../tests/schema_validation/library_foundation.py)
remains EXACT blob `6585db8f7c2da966158ae0e0946d41932b1b654e`; reviewed author note
blob `88866cf299dbf3e0f18f3e289355b4e82f7fc4d6`. Read full actual source/diff/note,
root/tools/tests instructions, accepted public/logical documents, proposed context
requirements and existing dependency/SDK source reviews. The note correction
properly distinguishes accepted public API from proposed contextd9; no source or
assertion changed to obtain this verdict.

## One retained counterexample and one real public route

Independent standard-library AST comparison removes ONLY
test_sdk_fixed_no_id_oas_transition_resets_native_fragment_scope and finds the
ENTIRE remaining module identical to1b4dee6/30169. All original thirteen methods,
setUp/imports/helpers, exact trusted-resource guard, persistent poison/cleanup and
main invocation are frozen. Fourteen total; no new decorator, skip, broad catch,
dependency/CI/state change or private validator hook.

The fixture isolates the previously unproven no-ID transition. Its Sample Schema
has NO $id, and its nested leaf has relative nested/leaf.json plus leaf/integer
anchors. The ordinary control asserts that native in_subresource(sample) returns
the SAME resolver. A whole OAS fragment still locates the physical leaf but its
returned wrong scope cannot find #leaf and raises exact NoSuchAnchor. That is a
meaningful scope counterexample rather than a missing-file/network failure.

The corrected branch uses ONLY the public Registry.resolver constructor to return
a fresh resolver at the SAME literal document base for this one known OAS→Sample
transition. Resource.pointer then resets its accumulated OAS segments because
resolver identity changed. Later $defs/leaf transitions use native Draft metadata
and in_subresource, establishing the nested relative scope. This supplies real
SDK metadata/scope objects, not fake lookup contents, success flags or a new
validation engine. The registry closure is initialized before pointer use; crawl
uses only id/children/anchors and does not call that scope closure prematurely.

The exact two resources are document and nested leaf; no-ID Sample is not invented
as a third URI. Resolved #leaf/#integer contents and zero recording retrievals are
asserted. Both actual Draft202012/OAS31 validators receive an ordinary immutable
control schema whose $ref is the WHOLE OAS document URI plus canonical fragment,
using the actual same registry. They accept {value:7} and reject {value:"wrong"}
through the real nested #integer assertion. There is no preselected leaf-URI
shortcut, private _resolver argument or test-provided validation verdict.

Unexpected network poison remains BaseException and cannot satisfy NoSuchAnchor
or ValidationError; unchanged persistent cleanup catches swallowed attempts. No
source/global SDK behavior is monkeypatched beyond the already reviewed guards.
Known-root identity/callback conditions are deliberately fixture-only and provide
no arbitrary role classifier or general namespace bridge.

## Selected source and actual static checks

Independently read the selected referencing public callback constructor,
Resource.pointer identity-reset loop, Resolver.in_subresource no-ID return and
Registry.resolver fresh constructor. Source confirms why the SAME-resolver control
retains the prefix and why a fresh same-base resolver changes only structural
transition context. Native validator descent retains resolved.resolver, as already
examined in the separate fixed-scope review; no second relative$id application is
introduced by the ordinary public constructor path.

Actual installed referencing/_core.py and referencing/jsonschema.py byte-equal
their admitted wheel members after rechecking whole-wheel SHA256. The exact
versioned primary sources remain
[resource/resolver](https://raw.githubusercontent.com/python-jsonschema/referencing/v0.37.0/referencing/_core.py),
[Draft metadata](https://raw.githubusercontent.com/python-jsonschema/referencing/v0.37.0/referencing/jsonschema.py) and
[native validator descent](https://raw.githubusercontent.com/python-jsonschema/jsonschema/v4.26.0/jsonschema/validators.py).
These were previously independently opened/read for the same pinned versions;
this review reuses those valid source facts, not an inferred runtime result.

Independent stdlib urljoin asserts api.json plus nested/leaf.json produces the
literal expected leaf URI. quote/unquote assertions confirm the whole-fragment
%24defs spelling and round trip. AST/full-module preservation, exact source hash
and source-member byte checks PASS. `python3 tools/check_project.py` PASS426tasks/
122decisions/current links/presets/HTML; `python3 tools/progress.py render --check`
PASS; `git diff 1b4dee6..94ceadb --check` PASS. No candidate import, probe/test/
native validator/marker body, install or SDK-triggered network request ran here.
No shared/source/CI/dependency/environment or GitHub write occurred.

## Released next step and baseline boundary

Root may now execute ONLY exact6585 source in the admitted isolated environment
and retain the direct byte-safe outcome. A failure is a real SDK/source fact to
diagnose, not wrapper intended red; no weakened assertion or broad refusal is
authorized. Actual14-method/native/current hosted proof remains new evidence.

After this fixed no-ID route actually passes and the final concrete context
document is reviewed/integrated, no generic production adapter/role engine should
be implemented before independent wrapper tests and inert immediate stubs. Minimal
source/harness/fixture reviews and real selected-environment foundations still
precede genuine missing-wrapper red. General role/alias/raw-data/resource/process/
descriptor integration belongs to those acceptance cases and later source audit;
these fixed SDK facts do not certify it. Full product/runtime/platform/live/release
requirements, all seven integrations and three FX providers remain.
