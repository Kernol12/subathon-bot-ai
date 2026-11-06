# Independent dependency admission — SCHEMATEST-001

Reviewer `/root/session_plan_review`, different from dependency/policy/notice author
`/root`,6 November2026. **APPROVED for the exact isolated dependency artifacts,
policy exception and declared-license inventory below.** This permits the next
isolated installation/foundation preparation stage; it does not approve an
unreviewed foundation program, validator behavior, wrapper/API/tests/red or release.

Actual candidate `c221519e89643dcf537554d62e2fbe6920692767`, NEW isolated branch
`task/SCHEMATEST-001-dependency-review`, worktree
`/tmp/subathon-schematest001-dependency-review`. Only this note changes here.
Read root/tools instructions, accepted ADR0008, actual logical contract/review,
dependency proposal, complete lock/manifest and notice inventory.

| Admitted artifact | Exact Git blob |
| --- | --- |
| tools/AGENTS.md | 1df38a544eb2e5f00dd5e67550025b6c5f7f8a67 |
| tools/schema_validation/requirements.lock | 859d7cfaa04a207a3052839e9622287566770259 |
| tools/schema_validation/wheels.json | 8fc489818cbbc094ca6739a6b03331df9f8774a1 |
| THIRD_PARTY_NOTICES.md | 2427eaee13c21d60553ade6fc21c11bade491907 |
| Dependency proposal evidence | 8783636eacceae2da2b99da2b729af6d883ef2f0 |

## Policy, scope and actual artifact audit

The explicit scoped exception is justified by the accepted plan's requirement to
use existing standards libraries rather than implement an OpenAPI/JSONSchema
engine. It applies only to this dedicated isolated developer environment. Existing
progress/repository tools remain standard-library-only; global installs and server/
client runtime additions remain forbidden. The standing authorized engineering
task and accepted plan support that isolated addition; changing local guidance is
not itself treated as new owner permission or a waiver of review/test-first gates.
No requirement conflict or separate owner decision is needed for this bounded scope.

Independently read each wheel as an archive with Python standard-library zipfile,
email, hashlib, json and pathlib. No candidate dependency was installed, imported
or executed. The three actual `/tmp/subathon-schema-pinned-cp310`, `-cp312` and
`-cp314` directories each contain EXACTLY their20 admitted wheel filenames, no
sdist or extra artifact. Together they have28 distinct wheels/60 target copies.

For every target copy I checked actual byte size/SHA256 against the manifest;
the one METADATA's normalized Name, Version, Requires-Python, ordered Requires-Dist
and License-Expression or legacy License against its declared record; and the
WHEEL compatibility tags against the claimed CPython target or pure py3 wheel.
All pass. Actual lock parsing confirms exactly20 pinned name/version entries and
exactly the manifest's28 SHA256 values, including every compiled target wheel.
No unpinned transitive requirement, source URL, optional extra or source build is
introduced by the lock. Future installation must actually use hash enforcement,
binary-only selection and isolation; comments alone do not enforce pip flags.

The four distributions with per-CPython native candidates are lazy-object-proxy,
pydantic-core, PyYAML and rpds-py. Matching filename/WHEEL tags and successful
cross-target downloads establish availability/metadata only. They do not prove
native load, glibc/ABI behavior or interpreter compatibility. No Python3.10/3.12/
3.14 execution is claimed here.

## Independent upstream provenance and static dependency closure

Fetched all20 EXACT per-version primary PyPI JSON records named in the manifest
using standard-library urllib (five concurrent reads, bounded8MiB per response).
Every selected filename exists in that version and all28 SHA256/size pairs match
the live upstream records; none of the selected artifacts is marked yanked.
The corresponding local bytes independently match those same SHA256 values.
This verifies artifact identity/provenance, not upstream source reproducibility,
maintainer trust, vulnerability absence or a security certification.

Receipt: `/tmp/subathon-schematest001-dependency-review-pypi.json`, SHA256
`49f3badfb10f4359b5da18f7038c765e866a650071018842c1df6e09715dad52`.
It records all20 metadata URLs, response hashes and actual matching outcomes.
Primary pages were independently opened for
[openapi-spec-validator0.9.0](https://pypi.org/project/openapi-spec-validator/0.9.0/),
[jsonschema4.26.0](https://pypi.org/project/jsonschema/4.26.0/) and
[referencing0.37.0](https://pypi.org/project/referencing/0.37.0/).
The manifest preserves the complete per-version metadata URLs for all transitives.

A separate standard-library marker/specifier evaluator checked all actual
Requires-Python and non-extra Requires-Dist against the20 pins for Linux x86-64
CPython3.10/3.12/3.14. It evaluates only the inspected finite metadata syntax with
an AST whitelist and numeric dotted-version comparisons, never eval or dependency
imports. Starting from the five proposed logical-contract distributions, closure
reaches exactly all20 locked packages. No optional extra is activated:

| Python target | Active dependency edges | Inactive optional/platform-marker rows |
| --- | ---: | ---: |
| 3.10 | 34 | 28 |
| 3.12 | 34 | 28 |
| 3.14 | 33 | 29 |

The difference is referencing's typing-extensions requirement before3.13; that
package remains required by other admitted pins on3.14. Every selected version
satisfies all its active constraints. Extra-only requests/cloud-secret/format/
email/timezone/CLI dependencies do not enter the admitted graph. Ambient packages
or optional backends cannot substitute for this exact reviewed set.

Independently verified the documented reason for avoiding the newer transitive
choices: exact PyPI metadata for
[lazy-object-proxy1.13.0](https://pypi.org/pypi/lazy-object-proxy/1.13.0/json),
[pydantic-settings2.16.2](https://pypi.org/pypi/pydantic-settings/2.16.2/json) and
[rpds-py2026.9.1](https://pypi.org/pypi/rpds-py/2026.9.1/json) declares Python>=3.11.
The selected1.12.0/2.15.0/0.30.0 actual wheels declare minima compatible with3.10
and satisfy the reviewed parent ranges. This is a justified minimum-preserving
pin choice, not a claim these or other versions are free from defects.

## Declared licenses and material preservation

Every actual dist-info LICENSE/LICENCE/COPYING/NOTICE file is represented by the
manifest, with no omitted matching dist-info license filename in the28 artifacts.
All63 actual target-copy license/notice hashes pass. I also read the bundled text:
the declared MIT, Apache2, BSD2/BSD3 and PSF labels agree with the inspected material;
rfc3339-validator's legacy `MIT license` declaration is represented as MIT in the
human inventory. openapi-spec-validator's separate Apache NOTICE is retained by
its own exact file hash. typing-extensions' complete historical multi-section
license file is hashed whole, not reduced to its short declaration.

The20-row THIRD_PARTY_NOTICES inventory matches these exact versions and declared
families and accurately labels the proposal as developer-only, uninstalled and not
redistributed. No wheel/code is added to the repository. Future redistribution
must retain the complete actual material and satisfy the full release license/
source manifest, including any further bundled native dependency obligations.
This review admits the present declared-license/artifact inventory; it is not a
complete future distribution/legal-compliance certification.

## Observed checks and explicit remaining gates

Actual independent standard-library archive/hash/metadata/marker/closure assertions
and20 upstream-version/28 artifact comparisons all pass. Exact3.10/3.12/3.14 directory
inventory assertions pass. On candidatec221519:

```text
python3 tools/check_project.py
PASS426 tasks/122 decisions/current links,presets,HTML
python3 tools/progress.py render --check
PASS
python3 -m unittest discover -s tests/tooling -v
PASS19/19,0.333s (legacy standard-library tooling only)
git diff 699943b..c221519 --check
PASS
```

No candidate package import, pip install/pip-check, native extension load, schema
validation, wrapper/foundation/test/red run or live application/provider operation
occurred during this admission review. No global environment changed. Root's
download-only evidence is consistent with the artifact inventory; resolution and
static closure are not relabeled real installed compatibility.

Next stages still require actual hash-checked isolated installation/version checks,
pip-check and independently reviewed real-library foundations on the selected
minimum/host/CI environments. They must prove explicit3.1/dialect selection,
exact Fraction/integer/boolean/enum/uniqueItems semantics, no-network registry and
complete reference/example/context handling through the actual APIs. Default
resolvers/optional backends are not approved merely because their distributions
are admitted. Concrete API/source/harness/fixture/test review, real intended
missing-wrapper red, later implementation/source review and ordinary offline CI
remain separate gates. Full runtime/platform/live/release scope is unchanged.
