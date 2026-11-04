# ACCESSTEST-002 independent full test-oracle review

Reviewer `/root/codec_author`, 2026-11-04, different from test author
`service_author`. Actual clean review base
`b519153670b894b2a21b8bc4346d7c7e926f9c77` includes author `25001158`, imported
as `d1d36ee`. Only this review note changes.

**APPROVED for the actual 30-test source and inventory.** No blocking test defect
found. Root CMake/CI harness review is separately assigned; this verdict does not
approve that gate, an unobserved red, production or merge.

| Reviewed artifact | Git blob |
| --- | --- |
| [Test source](../../tests/acceptance/effective_permissions_tests.cpp) | `fc82d17ede5d2afe66e42572c17762510e5682f4` |
| [Inventory/map](../../tests/fixtures/effective-permissions-v1.json) | `d0c25108ec233cdc11260aa1263974805b0fba15` |
| [Author evidence](accesstest-002-tests-author.md) | `ba78ccde3336fc35347bc387b16649922b84fa2b` |

Read all 1385 source lines and the complete inventory against approved logical
`3d82bfa418bb14099ee130fca7a400adca66500b`, API
`dda2793c71299dbe36a321ce3dd5620fecc8b214`, header
`edd6ffc25379e5036d5e1622346eaa3f272a1c5c` and immediate stub
`4cf7bcca19e5ab879b62c70c1521d9f977abfd3b`. Root/tests instructions apply.

All 45 EP examples and both fixed D1 examples map to actual raw setups. Expected
decisions/witness indices/source rows are independent literals. Whole quantity and
resource tuples, resource-first individual selection without fallback, conditional
individual versus role denials, same-effect alternatives and contradictory/disjoint
individual scopes have consistent oracles. Multi-resource A/B plus C/D cannot become
A/D, while one explicit set-valued tuple can cover it. Global empty shape and full
requested quantities preserve their own meaning without actual-effect substitution.

Owner tests retain narrow protected access/recovery, ordinary default denial,
owner-target reset refusal, current-owner transfer and independent current snapshots.
Explanations cover losing/other-operation rows, every decisive denial, suppression,
failed-key completeness, stable witnesses and unsigned-byte/NUL identities. The
failed-slot case validly reuses `a`/`z` across resource and quantity namespaces:
the frozen API §2 requires uniqueness in EACH SEPARATE namespace, not disjoint sets.
No invalid-collision oracle or fixture rewrite is justified by an earlier draft.

Lifetime checks destroy/mutate input and inspect copied/moved results. `check_rows`
uses the actual/expected minimum length; iterator and vector reads are guarded.
The sole fatal ASSERT is at the lifetime test's caller, before optional dereference.
Raw assembly helpers perform no SUT parsing/evaluation. Wrong exceptions, including
the marker, fail nonfatally per table row; no marker is an accepted expected error.
There are no skips, disabled cases or generic-any-throw expectations. All 30 unique
source names exactly match the inventory in order; no runtime setup calls the stub.

Strict fact/catalog/request/policy/rational cases and staged-priority cases align
with the complete-input API order, including irrelevant invalid entries and numeric
canonicality before domain errors. All six limits have refusal cases and meaningful
exact/one-over byte/item boundaries. Independent field sums confirm B/I
11/1→9/0, 18/2→26/1, 12/1→26/1, nested 93/9→51/3 and failed-key output 53/4;
two ordinary result rows require 35/2. Signed-minimum magnitude, exact fractions,
2^64 and 5001-byte/16384-byte components preserve the declared domains.

Actual checks: read-only Git/source/JSON inspection, 24 independent Python stdlib
arithmetic/byte/tuple/B-I facts, frozen-input hashes, exact 30-name/47-example map
and `clang-format --dry-run --Werror` all passed. No feature/marker/test/SDK body,
compiler or discovery was run by this reviewer. No expected values were generated
by the SUT. Internal preallocation/temporary behavior and allocation-failure mapping
remain production source-audit obligations; these public tests do not measure them.

`python3 tools/check_project.py`, `python3 tools/progress.py render --check` and
`git diff --check` passed: 406 tasks/122 decisions, links/presets/current HTML and
whitespace; sole-path staged inventory/whitespace was checked before commit.
Actual separate harness
approval, applicable selected environment/CI evidence and FIRST ordinary intended
red remain required before later implementation gates. Existing valid environment
work was not repeated. Root retains shared state/HTML/build/CI/PR ownership.
