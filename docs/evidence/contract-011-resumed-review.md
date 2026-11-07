# CONTRACT-011 — independent resumed profile correction review

Reviewer: `/root/asset_contract_review`, a fresh actual reviewer session, different
from resumed document author `/root/asset_contract_correction`.
Date: 2026-11-07. Dedicated branch `review/CONTRACT-011-resumed-profiles`, isolated
`/tmp/subathon-contract011-resumed-review`, initially clean at
`36a8750a7f7c582ad05a06e91d127f739970219d` (author commit
`f9d062cae0d3161df4a6a1cf246399702994ef40`, base `8a54faf`). Owns only this new
review note; root retains state/HTML/shared files/schema/build/CI/pins/publication.

## Actual packet and verdict

Read the complete 1149-line contract and 302-line author note, the
[brief](../tasks/CONTRACT-011.md), [handoff](contract-011-handoff.md), root instructions,
workflow/test strategy, architecture, FIRST_AGENT_PROMPT, current task state,
specification v0.45 §11/MEDIA-01/03/04/05/06/UI-04, ADR-0005, and the approved 009
asset/profile boundary. This review concerns logical document definitions only.

| Artifact | Exact reviewed Git blob |
| --- | --- |
| [Corrected contract](../contracts/scene-assets-packages-v1.md) | `c4ba2f9bc4a864e07edb93a558d18f9c8c809a10` |
| [Author evidence](contract-011-author.md) | `b21ce2a51a3620c727f4e9b4530075d06d0f794d` |
| [Original independent review](contract-011-review.md) | `6691f0667c7cb3f9359ed97f18ad3305161e4038` |
| [Specification](../specification.md) | `81406806c02df0f015a8c71c40b149ae1ff0ed19` |
| [Approved scene model](../contracts/scene-document-v1.md) | `9a3507331376a2beed47b93eac7079b45f0f2c94` |
| [Common values](../contracts/wire-primitives-v1.md) | `11eeb5f422a174e042143df6b476ae1d717eeacf` |

**CHANGES REQUIRED.** The original R1 definition gap is addressed by this packet:
ten finite media entries, two closed font entries and complete logical descriptors
now exist, independently of still-UNKNOWN playback/font/platform support. R2's
missing grammar is also substantially supplied, but the two findings below must
be corrected before approving the SVG logical profile/examples for downstream 012.
They form one bounded author correction packet, sent to root during this review.

The original review by service_author of service_reviewer's document, authored at
`deb0d06672ccc1ff98a9a25c0b5e6e10021b9974` and imported at
`72a00a761497671d9b8dd606d1d4394fb583b404`, remains unchanged and linked above.
This is a new independent audit, not retroactive alteration of that review or
invented participation by its unavailable original reviewer. Unchanged valid
reviewed portions retain their original scope and attribution.

## R2-a — preserve gradient template transform semantics

Contract §3.1 permits optional `gradientTransform` on both gradient kinds;
§3.3, lines 422–426, says href supplies stops ONLY and an absent transform means
identity. Those rules admit a source with an inherited nonidentity transform but
give it a different meaning from SVG. This conflicts with MEDIA-05's rejection
instead of silent reinterpretation and the profile's own application-subset claim.

Concrete witness: start with literal S02 and add ONLY
`gradientTransform="scale(2)"` to gradient B. All current source, reference,
invertibility and finite-bound rules still fit. G has explicit geometry and
`href="#B"`, but no transform. SVG supplies B's scale(2) to G; the candidate
instead assigns G identity. The difference changes the gradient vector and is
independent of any renderer experiment. The
[W3C template algorithm and linear-gradient href attribute list](https://www.w3.org/TR/SVG2/pservers.html#LinearGradientElementHrefAttribute)
include `gradientTransform` among the attributes copied when absent.

Required correction: either resolve the source's inherited gradient transform
with the same finite closure/work/inverse checks, or require an explicit
`gradientTransform` on href-bearing gradients so template transform inheritance
cannot arise. An explicit identity matrix can select identity without rewriting
original bytes. Retain both gradient kinds and internal templates; add coherent
literal omitted/inherited/explicit transform cases with their correct outcomes.
No parser, sanitizer or renderer implementation is requested by this document fix.

## R2-b — bind the binary-use boundary oracle to the actual charged elements

The §8.3 line-1067 example says a compact binary-use tree at 20 levels has
2^20−1=1048575 expanded occurrences and fits. The integer identity is true, but
that is not a complete source construction under the defined SVG vocabulary.
Each `use` has one target; compact binary reuse needs branching containers plus
their child `use` elements. §3.3 charges every visited element in every instance,
so omitting those elements understates the expansion.

Use this explicit construction, for example: G0 is one rect; Gn is one g with
two child use elements, both referencing G(n−1); the root has one use referencing
Gn. Count the target subtree's visited elements, including child use elements
and their nested expansions. Then T0=1 and
Tn=1+2*(1+T(n−1))=4*2^n−3. G18 gives 1048573, within 1048576; G19 gives 2097149,
over it. The root source use can be expressly excluded as the instance host or
charged additionally, but the counting convention must be stated consistently.

Required correction: replace the ambiguous level/count oracle with a concrete
admitted source/DAG construction, explicit count convention and corrected
below/above boundary. Source counts, references and total work/memory ceilings
still apply separately. This is a hand-derived logical example, not a request
to execute an XML parser or construct a runtime fixture.

## Remaining reviewed scope

No additional blocking finding was found in this packet's R1 scope. §2 defines
closed records and admitted pixel/color/track/codec/sample/timing combinations,
the finite container/setup/count/rolling-buffer obligations and complete MD01–10
values plus incompatibilities/alternatives. Required formats remain in scope;
the profile subsets are proposals with review of sufficient extensions required
for other required valid assets. Declared, inspected and compatible observations
remain separate. A container, cache or native API cannot establish support.

§4 defines the two SFNT scalers, face 0, closed required/optional table versions,
Unicode cmap combinations, complete glyf/CFF outlines, specialized hint modes,
TrueType opcode set and execution budgets, finite layout lookup families and
explicit variable/color/bitmap/AAT/embedded-feature exclusions. FD01–02 and the
three-byte hinted variant are actual closed logical records. No stripped table,
synthesized weight or guessed name substitutes for the requested font. Actual
native interpreter control, shaping and both outputs remain separate evidence.

R2 otherwise supplies closed XML/namespace/tree/attribute/style/property grammars,
distinct SVG source numbers/units, every required path/transform family, typed
internal reference/containment cycle checks, complete source examples and finite
arithmetic/geometry/matrix/inverse/expansion/output bounds. Unknown numerical or
native containment cannot become an assumed pass. While correcting R2-a, an
explicit normative `href`/`xlink:href` literal `#ID` production would also make the
existing exact-fragment examples easier to consume; this is not another blocking
scope expansion.

Identity/integrity/scope, generated path namespace, portable closure/import mapping,
rights, staging/publication uncertainty, configurable product caps, same-assets
fallback/Noto/OFL obligations and later release gates retain the original review's
valid observations. They are requirements, not implemented safe import/export,
font packaging or runtime compatibility.

## Independent verification and actual checks

Primary read-only verification used W3C SVG
[gradients/templates](https://www.w3.org/TR/SVG2/pservers.html),
[paths](https://www.w3.org/TR/SVG2/paths.html),
[transforms](https://www.w3.org/TR/SVG11/coords.html#TransformAttribute) and
[linking](https://www.w3.org/TR/SVG2/linking.html), Microsoft OpenType
[SFNT](https://learn.microsoft.com/en-us/typography/opentype/spec/otff),
[maxp](https://learn.microsoft.com/en-us/typography/opentype/spec/maxp),
[cmap](https://learn.microsoft.com/en-us/typography/opentype/spec/cmap),
[name](https://learn.microsoft.com/en-us/typography/opentype/spec/name) and
[instructions](https://learn.microsoft.com/en-us/typography/opentype/spec/tt_instructions),
the [WebM container guidelines](https://www.webmproject.org/docs/container/) and
[Opus Ogg RFC 7845](https://www.rfc-editor.org/rfc/rfc7845.html).
These establish source distinctions, not project/platform support. The selected
profile caps/opcode/layout subsets remain application policy proposals.

A separate read-only Python stdlib/Git audit passed **42** named text/literal/
arithmetic checks. This independently checks the author's claims without relying
on its untracked 26-check script:

- All 29 AP rows, §1/5/6 and same-assets/Noto/rights paragraphs are byte-preserved
  against original blob `d6e67dbe`; author history is append-only against `8a54faf`;
  the original review blob exactly matches `deb0d066`.
- Ten unique media labels, complete MD01–10, two font labels and FD01–02 exist.
  S01 has exactly 152 ASCII bytes, two elements, seven attributes and four command
  segments; S03 has ten commands and arc-envelope arithmetic 11.
- AVC 8160 macroblocks at 60 Hz gives 489600, above 245760 and below 522240;
  WAV alignment/rate/data bytes are 4/192000/192000; MP3 44100+576+1404=40*1152;
  AAC 46*1024=47104 and 47104/48000=368/375; 2 cs=1/50 second.
- Raster/decoded-byte, 64-translation, matrix/stroke, tiny-number bit, zero-token,
  glyph index and MiB/GiB boundaries are correct. Both the published binary
  arithmetic and the corrected actual-use-DAG recurrence were checked separately;
  arithmetic truth alone does not resolve R2-b's source-accounting defect.

Environment: Linux 7.2.14-arch1-1 x86_64 GNU/Linux, Python 3.14.9.
Actual commands from the isolated review worktree:

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All passed: 427 task records/122 decisions, local links/presets/current HTML and
whitespace. The same documentation checks and sole-note staged-scope check are
performed for this review-note commit. Existing consistency/component CI is
appropriate to these Markdown definitions; root must inspect actual final-head
CI/reviews/merge rules. No new executable workflow or dependency is warranted here.

No asset/font download or hash, archive/XML/font/media parse, instruction/native
execution, decoder, renderer, provider operation, schema/source/API/test authoring,
fixture execution, historical red replay or performance measurement occurred.
These logical checks are not a behavioral baseline or implementation approval.

Next: author corrects both related findings in one packet; this different reviewer
reviews the actual changed contract and evidence revision. 012/schema integration
remains unapproved. Concrete checking/tooling/ownership interfaces and enforced
native/resource controls, reviewed tests and valid intended red, source/config
review and acceptance, full 010/008/003 closure, Noto binary/weight/hash/OFL
packaging and dual-renderer/OBS/minimum-version/platform/deployment/performance/
release gates remain. Root owns publication; this reviewer does no push/PR/merge.
