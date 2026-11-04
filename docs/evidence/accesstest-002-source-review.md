# ACCESSTEST-002 independent minimal-source review

Reviewer `/root/codec_author`, 2026-11-04, different from author
`service_reviewer` and root harness author. Actual clean starting candidate
`f1eb3d6bf51e89b3e0656e8a854496d669f7788d` contains source author `78195bef`
imported as `b29274b`. Only this review note changes.

**APPROVED for this actual inert source and minimal build harness.** No defect
found. This allows the separately coordinated test-authoring stage, not feature
execution or production approval.

| Actual artifact | Git blob |
| --- | --- |
| [Public header](../../core/include/subathon/core/effective_permissions.hpp) | `edd6ffc25379e5036d5e1622346eaa3f272a1c5c` |
| [Source](../../core/src/effective_permissions.cpp) | `4cf7bcca19e5ab879b62c70c1521d9f977abfd3b` |
| [Author evidence](accesstest-002-source-author.md) | `04e5ed8732bf0a3699bc7d1d23c82ae929ca73dd` |
| core/CMakeLists.txt | `1f01466ed7231e2a2852c3b494e7ad167cc1f88e` |

Compared the complete header namespace with approved API blob
`dda2793c71299dbe36a321ce3dd5620fecc8b214` and actual review
`e007e35b1a0f34ca360dbe18dc77da1c876addaf`. Token comparison confirms all 16
structs, 14 uint8 enums, ordered fields/defaults, signatures, six limits, private
result construction and borrowed const getter exactly match. The public header
uses only the permitted standard headers; it creates no certificate or test-only
result constructor. Root/core instructions remain satisfied.

The evaluator's entire body is exactly the immediate
`throw EffectivePermissionsNotImplemented{};`, with unnamed implementation
parameters. No validation, numeric/parser/permission work, successful result,
dependency call or global initialization precedes it. Private owned-data move,
const getters and fixed allocation-free failure mechanics are inert. Error code,
what() and the distinct marker text match the approved API exactly.

Root's six-line opt-in target compiles this one TU into an unlinked static library
with the existing C++20 warning options. Default builds remain unchanged; no new
post-build/run/discovery action is added. Actual root Ninja rules and archive
symbols corroborate this: evaluator defined, no Boost/GTest/Qt/SQL/main or authored
global-initializer symbol. The independent SDK executable remains separate.
Retained 37-case discovery contains only old numeric/infrastructure and SDK checks;
no permission feature test exists or was invoked.

Actual checks: full source/declaration comparison, Git blob/diff reads,
`clang-format --dry-run --Werror`, read-only `nm -C` and retained build/discovery
inspection passed. No compiler/build, constructor/getter, evaluator/marker, SDK or
test body ran in this review. `python3 tools/check_project.py`,
`python3 tools/progress.py render --check` and `git diff --check` passed: 406
tasks/122 decisions, links/presets/current HTML and whitespace. Sole-path staged
inventory/whitespace was checked before committing.
Only actual independent test review, selected environment evidence, FIRST intended
behavioral red and later production/code/integration review can satisfy subsequent
gates. Current CI/publication/normal merge remain root-owned separate requirements.
