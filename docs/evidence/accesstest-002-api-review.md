# ACCESSTEST-002 independent concrete API review

Reviewer `/root/codec_author`, 2026-11-04, different from author `service_author`.
Clean review branch starts at actual import
`5757a45a3909e2c4ac478113ff5b3bb5d8f2ab7a`, author
`646a11e36a8253d73357efa1828677d07875667a`. Only this note changes.

**APPROVED for the actual API document.** No concrete defect or further product
decision found. This approves neither future source/tests nor implementation or merge.

| Actual artifact | Git blob |
| --- | --- |
| [API proposal](../contracts/effective-permissions-api-v1.md) | `dda2793c71299dbe36a321ce3dd5620fecc8b214` |
| [Author evidence](accesstest-002-api-author.md) | `0680aa30738e693612376a026a6d71e4898bdf28` |
| Approved logical model | `3d82bfa418bb14099ee130fca7a400adca66500b` |
| Actual full logical review | `ce5950a599c1039d8c5b0c42a04a22c8e0cd7470` |

Checked the complete document against specification blob
`9e7c41c8a0cbd1b284111143d215553333c2f6ab`, owner decision
`34168d4e5f82a524e9459b13efe055ef6786af8c`, common values and execution boundaries.
Root/core instructions and the existing test-first gates apply.

The single entrypoint accepts directly constructible raw facts, without requiring
a successful SUT factory or caller certification. Only it constructs the owned
immutable decision; all request/source data survives input destruction. Caller
copies and borrowed lifetime/data-race responsibilities are explicitly separate.

Complete catalog/owner/policy states, opaque references, canonical reduced rationals,
typed units/domains, source/grant identities and all supplied irrelevant entries
have explicit staged validation. Global stage/code priority is independent of input
permutation. Contradictory individual resource scopes precede quantity semantics;
invalid supplied facts cannot hide behind protected ownership. The only unknown-
policy success is the current owner's narrowly classified protected operation,
with catalog/request checks intact. Ordinary precedence has no individual quantity
fallback; complete conditional deny and outside-scope role rules match the model.

Every supplied entry receives a stable source explanation, including failed slots,
suppressed roles and decisive denials; a permit has exactly one complete witness.
Unique identities make the prescribed row ordering total. References, raw facts,
protected classification and results remain evidence, not authentication/currentness
or authority for later execution. Owner-target reset, delegation and SESSION seams
remain separate.

All six limits cover complete input, individual strings and complete output before
ownership, including optional tags, virtual rows, copied requests and failed keys.
Independently summing declared fields confirms input/result B/I 11/1→9/0,
18/2→26/1 and 12/1→26/1; the ordinary/virtual source rows cost 9/8 bytes. Two
ordinary rows require result B/I 35/2. Checked derived bounds preserve signed-minimum
magnitude, large exact domains and the default 131072-bit comparison capacity;
16D numeric payload reserve plus 8I index reserve is arithmetically consistent.
Actual Boost temporary/limb/work behavior still requires production source audit.

Actual read-only Git and Python stdlib checks passed 18 independent B/I/exact-number
facts, frozen-input/current-artifact hashes and document links/fences. No SDK chain
was repeated and no API source, test, compiler, SDK or runtime body was created/run.
`python3 tools/check_project.py`, `python3 tools/progress.py render --check` and
`git diff --check` passed: 406 tasks/122 decisions, links/presets/current HTML and
whitespace; sole-path staged inventory/whitespace was checked before commit. Separate
minimal-source/harness and independent test reviews, selected foundation, FIRST
behavioral red, production review/acceptance and current CI/publication/normal merge
gates remain required. Root owns shared state/HTML/build/CI/PR62 and stage releases.
