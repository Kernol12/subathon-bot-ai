# SCHEMATEST-001 — Candidate dependency admission, not execution proof

Coordinator /root,6 November2026, after logical4ab document approvalb8/import699943b.
The logical review does not approve dependency artifacts, concrete adapters or tests.
[Candidate lock](../../tools/schema_validation/requirements.lock),
[manifest](../../tools/schema_validation/wheels.json),
[notice inventory](../../THIRD_PARTY_NOTICES.md).

Existing tools/AGENTS.md describes standard-library tools. ADR0008 requires existing
standards libraries for this dedicated tool because existing selected tools supply
no standards validator. The scoped guidance now explicitly permits this justified
exception ONLY in an isolated developer environment after actual independent review;
existing progress/repository tools remain standard-library-only. No instruction/test/
release gate is bypassed and no global/server/client dependency is added. Root owns
this policy/pin/notices change; different review is still required before admission.

Actual downloads, with no package installation/import/execution:

```sh
python3 -m pip --isolated --disable-pip-version-check --no-cache-dir download --index-url https://pypi.org/simple --only-binary=:all: --dest /tmp/subathon-schema-wheels-cp314 openapi-spec-validator==0.9.0 jsonschema==4.26.0 referencing==0.37.0 openapi-schema-validator==0.9.0 jsonschema-path==0.5.0
```

Equivalent candidate resolution was run for CPython3.10 and3.12 using --python-version,
--implementation cp and the corresponding --abi. Fresh root pip26.2.2/hostPython3.14.
Unconstrained transitive latest picks lazy-object-proxy1.13, pydantic-settings2.16.2,
rpds-py2026.9.1 with >=3.11 metadata. The universal proposal instead pins3.10-compatible
1.12.0/2.15.0/0.30.0, preserving the declared developer minimum rather than silently
raising it. All other20-package versions match. This is an engineering proposal,
not a security/compatibility certification of those versions.

Exact twenty candidate pins were written to /tmp/subathon-schema-candidate-pins.txt
and downloaded afresh for3.10/3.12/3.14 with --only-binary=:all: into
/tmp/subathon-schema-pinned-cp310, -cp312 and -cp314. All three commands exit0.
Logs /tmp/schematest001-pinned-cp310.txt, -cp312.txt, -cp314.txt preserve commands'
output. Twenty packages/28unique wheels are available, including actual compiled
wheel candidates. No sdist build, install, wheel import or validator call occurred.
Cross-target download success is not runtime compatibility or marker correctness.

A standard-library zip/email/hash inspection read actual wheel METADATA and bundled
license-file bytes, without extracting/executing package code. Candidate manifest
records each filename/size/SHA256/target set, declared license, Requires-Python,
Requires-Dist and actual license-file hashes. Static pip-vendored packaging marker/
specifier checks additionally verified every non-extra dependency against these
pins for Linux x86-64 Python3.10/3.12/3.14. Optional extras are not selected. This
static closure proof does not replace independent upstream-hash/license audit,
real isolated installation/pip-check or actual green library foundations.

The hash lock is wheels-only and includes all20 direct/transitive pins, not just
top-level package bounds. Future install must require hashes, disable source builds,
use an isolated environment and verify exact installed versions/dependency closure.
No implicit optional backend/network behavior or ambient system package is accepted.
Actual minimum/host/CI API compatibility, exact Fraction/integer semantics,
OpenAPI3.1 selection, no-network handlers/registry and complete examples remain
separately reviewed real-library foundation gates before minimal wrapper/test code.
No selected validator execution, interface/stub/test/red/implementation is proven.

Root project/render/whitespace checks pass426/122 after these text-only changes.
Different actual dependency/policy/notice review is pending; source/schema/assertions/
CI remain unchanged. All product/runtime/platform/live/release gates are retained.
