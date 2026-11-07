# CONTRACT-011 independent asset/font/package document review

Reviewer: service_author, different from document author service_reviewer.
2026-11-04, dedicated `task/CONTRACT-011-document-review`, isolated
`local-evidence/worktrees/contract011-review`, initially clean at
`d39e20aaf4b33d35e4ff4b400d0dfeb5ce9233ca`. Owns only this new review note.
No author document, source, asset, fixture, schema or shared file is changed.

## Actual candidate and verdict

Author commit `2a11fde4cf6874d8025de7dd8405e75c5cf1c0c1`, imported at the root
revision above. Read all 432 contract lines and all 150 author-evidence lines.

| Artifact | Exact Git blob |
| --- | --- |
| [Asset/font/package proposal](../contracts/scene-assets-packages-v1.md) | `d6e67dbe24d5cfa42d6e9a50c67f767659df8ab9` |
| [Author evidence](contract-011-author.md) | `0ca7b984946a44c81e4b421d973b0d9fd907a635` |
| [Approved scene model](../contracts/scene-document-v1.md) | `9a3507331376a2beed47b93eac7079b45f0f2c94` |
| [Common values](../contracts/wire-primitives-v1.md) | `11eeb5f422a174e042143df6b476ae1d717eeacf` |

**CHANGES REQUIRED.** The candidate preserves many important invariants, but its
format catalogue and SVG/font obligations do not yet supply CONTRACT-011's actual
declarative profile inputs to CONTRACT-012. UNKNOWN runtime compatibility is
correct and must remain honest; it is a different issue from leaving the logical
profile itself undefined. R1/R2 below need a bounded document correction or an
explicit unfinished-011 follow-up gate before schema integration. No owner product
decision or new tool/dependency is requested by this review.

Read root/schema instructions, architecture/workflow/test strategy, actual
[011 brief](../tasks/CONTRACT-011.md), specification v0.45 §11/MEDIA-01/03/04/05/06/
UI-04, [ADR-0005](../decisions/0005-scene-contract-prerequisites.md), complete
approved 009 and common005. ADR-0005 assigns assets/profiles to 011 and complete
schema representation/imports to 012; 012 depends on 011. Actual 009 §9 explicitly
requires 011 to supply complete assets/fonts/packages and required media/SVG profiles.
A catalogue of things a future profile must decide cannot silently stand in for
those prerequisite definitions.

## R1 — Supply the finite media/font profiles, not only a required-family catalogue

Actual §2 lines 80–105 defines CodecProfile abstractly, then calls the table a
catalogue of candidate combinations for later review. It correctly retains every
required family, but does not name actual versioned profiles with finite admitted
parameter values/combinations. For example, MP4 says AVC/H.264 with AAC-LC is a
candidate, WebM names VP8/VP9 and Vorbis/Opus, WAV says PCM is a candidate, and MP3
lists parameters without an admitted set. PNG/JPEG/WebP/GIF similarly enumerate
questions/checks rather than a selected finite logical profile. AP11–14 show missing
evidence/descriptors, but no independently decidable complete descriptor under a
defined profile versus a one-field incompatible/rejected descriptor.

The same issue appears for fonts in §4 lines 157–181: exact face/weight/fallback
and active outline/hinting/variable/color/embedded-feature policy are required, but
no finite admissible font profile/check-set is actually defined. This is separate
from the correctly deferred physical Noto file/version/hash/weight validation.

The 011 brief requires explicitly documented codec profiles and profile vocabulary/
rejection evidence. ADR-0005 and 009 make them 011 inputs to 012. Yet §1 lines 14–16
assigns profile/checker definition freezing to 012 and §9 lines 420–425 again leaves
closed profiles for that stage. §2's statement that a future registry must exist
before 012 is useful, but does not complete the missing 011 definition or clearly
keep 011 unfinished until a named owned follow-up supplies it.

Required correction:

- Within 011, document finite versioned media/font profile definitions for all
  required families, with exact admissible descriptor vocabulary/combinations and
  bounds/check requirements appropriate to each family. Define which track/sample/
  pixel/face/features are allowed, unsupported, not applicable or insufficiently
  inspected; do not infer them from a filename/container/family name.
- Give independent literal complete descriptor examples and parameter/boundary/
  incompatibility counterexamples under those definitions. They are document data,
  not downloaded/generated media or proof that a decoder can play them.
- Keep declared/inspected/compatible observations separate. Declarative profile
  identities may be proposed/reviewed while actual Qt/browser/OBS/platform support
  remains UNKNOWN until its owning evidence exists. Do not advertise support or
  silently drop a required release family to obtain a smaller catalogue.
- If this is intentionally only the first partial 011 document, mark 011 explicitly
  UNFINISHED and name a concrete 011-owned profile follow-up/review gate BEFORE
  012. Do not transfer the unresolved content/profile policy to the schema owner.
  Machine encoding/checker implementation and runtime verification may remain later.

This request does not select a codec, archive library, interpreter, binary pin,
playback default or new product limitation on the author's behalf.

## R2 — Complete the restricted SVG profile at its owning 011 boundary

Actual §3 lines 119–153 preserves the required semantic families and important
exclusions, but repeatedly refers to a reviewed SVG-number profile and a closed
whitelist that do not yet exist here. Lines 141–145 explicitly defer exact attribute/
restricted-style/namespace grammar to parsing under 012. The technical proposal's
128-byte token/absolute exponent 256 limits do not specify number syntax, admitted
values/units, path/transform intermediate bounds or their semantic interpretation.
AP15 is a conditional description of a good drawing, not a literal case whose
selected element/attribute/numeric/reference rules can be independently decided.

MEDIA-05 and the 011 brief require a documented restricted static SVG profile,
including the full required basic shapes/paths/groups/fills/gradients/strokes/
transforms/internal references and rejection instead of silent reinterpretation.
The corrected 009 deliberately relies on 011 for that profile; its geometry grammar
does not resolve SVG source numbers or paint/style/reference semantics.

Required correction: supply or explicitly gate the unfinished 011-owned closed
element/attribute/namespace and restricted-style/property vocabulary, source-number/
unit grammar and finite semantic/work bounds, full required path/transform families,
typed same-document reference syntax/target/closure rules and reject behavior.
Include independent literal accepted/rejected source/descriptor examples and
numeric/reference/resource edges without invoking an XML/SVG parser or renderer.
Do not reduce the required subset to rectangles/raster substitution, borrow a
financial/canonical-scene parser by convenience, or treat a Qt option/native loader
or bare browser image element as the validator. Keep the existing exclusions for
scripts/events/external/data/HTML/animation/filters/text and counted acyclic closure.

As with R1, a concrete unfinished-011 follow-up gate before 012 is acceptable if the
full definition needs a separate bounded document stage. Actual tooling/schema/
baseline/runtime safety remains separately reviewed work, not approved here.

## Remaining portions inspected without another blocking finding

The following observations concern this actual revision only, not prospective
approval of the missing profiles or an implementation:

- Asset/revision/kind/scoped identities are separate from paths and rights. Immutable
  original bytes/metadata, full-length SHA-256 integrity and explicit derivative
  provenance are coherent. Equal bytes/digests can share storage without merging
  installation scope, entitlement, source or licensing evidence. Unknown source/
  publication facts and unavailable inspection are not fabricated known values.
- Portable closure includes exact scene/dependency/asset/font revisions and notices.
  Unfinalized 010 definitions still prevent a complete package; no placeholder timer/
  layout/private-source binding is invented. AP29 correctly rejects implicit alias/
  overwrite on equal raw IDs across installations with different content. A new
  imported revision requires complete authorized mapping and updated typed closure;
  current destination-domain rights still apply. These are requirements, not a
  demonstrated import mapper or cross-installation capability.
- The generated ASCII manifest/scene/assets-digest/notices-digest namespace avoids
  using arbitrary opaque/Unicode IDs or display labels as filenames. Duplicate
  names, extras/missing entries, traversal/aliases, host paths, links and unsafe
  archive entry kinds reject. Actual expanded byte/length/digest checks in staging,
  current rights/CAS, failed cleanup/unknown outcome and partial external export
  are stated honestly. No filesystem-wide/database-filesystem atomicity is promised.
  Choosing an actual archive/framing/library later still requires its explicit claim.
- The product defaults remain authorized-configurable 32/128/512 MiB and 2 GiB, plus
  8192 raster edges/32000000 total pixels. The technical table separately proposes
  expanded-original staging, decoded/intermediate/count/work/time controls, not a
  media-duration limit or measured throughput. Raised product caps cannot enlarge
  worker/memory capacity. Opaque decoder allocation/CPU/cancellation containment is
  explicitly unproved; inability to enforce it yields no successful inspection.
  Concrete compressed/container/number/field-walk bounds still need the closed
  profile and later reviewed parser interface; current limits alone certify none.
- Uploaded TTF/OTF, explicit fallback, the same exact admitted font assets on both
  renderers and actual face/weight/coverage/metrics evidence remain required. No
  synthesized Bold or host-font substitution is mislabeled success. Noto Regular/
  Bold and separate OFL/notices are retained. Unknown/prohibited redistribution
  cannot produce a purported complete portable theme by omitting its font. Current
  applicable usage/redistribution conditions remain distinct from account permission.

## Primary fact verification — documentation only

Browsed only primary official documentation/repository metadata:

- [Qt Multimedia 6.12](https://doc.qt.io/qt-6.12/qtmultimedia-index.html) confirms the
  FFmpeg 9.0.1 backend, deployment caveats and platform differences. The
  [QMediaFormat](https://doc.qt.io/qt-6.12/qmediaformat.html) API exposes decode-mode
  capability queries; these do not demonstrate the intended payload's full playback.
- [Qt image plugins](https://doc.qt.io/qt-6.12/qtimageformats-index.html) and
  [QImageReader](https://doc.qt.io/qt-6.12/qimagereader.html) support the plugin/
  allocation-limit discussion. Quick canRead remains weaker than successful complete
  decode, and a QImage allocation control does not prove a total decoder scratch cap.
- [QtSvg options](https://doc.qt.io/qt-6.12/qtsvg.html) distinguish animation disabling,
  Tiny12 and AssumeTrustedSource's reduced resource checks. W3C's
  [SVG processing modes](https://www.w3.org/TR/SVG2/conform.html#secure-static-mode)
  distinguish disabling features from rejecting source; this supports the candidate's
  warning, not an application whitelist or profile validation claim.
- [Chromium media](https://www.chromium.org/audio-video/),
  [RFC 6381](https://www.rfc-editor.org/rfc/rfc6381.html) and
  [WebM guidelines](https://www.webmproject.org/docs/container/) support separating
  codecs/profiles from container names and actual build/platform capability. Chrome
  does not establish OBS/CEF compatibility. Qt/browser/OBS APIs were not invoked.
- [QFontDatabase 6.12](https://doc.qt.io/qt-6.12/qfontdatabase.html) and
  [CSS Font Loading](https://www.w3.org/TR/css-font-loading-3/) expose font loading/
  fallback facilities, not evidence of identical selected font bytes or metrics.
- The [official Noto release](https://github.com/notofonts/latin-greek-cyrillic/releases/tag/NotoSans-v2.015)
  and [asset listing](https://github.com/notofonts/latin-greek-cyrillic/releases/expanded_assets/NotoSans-v2.015)
  corroborate the advertised 112 MB archive. Current
  [Google Fonts metadata](https://raw.githubusercontent.com/google/fonts/main/ofl/notosans/METADATA.pb)
  names exact source commit c4a321e123e4d4ff315f57f4e0adf294fe3a95be and variable TTFs,
  not measured static Regular/Bold binaries. The pinned-source
  [OFL 1.1](https://raw.githubusercontent.com/notofonts/latin-greek-cyrillic/c4a321e123e4d4ff315f57f4e0adf294fe3a95be/OFL.txt)
  permits software bundling subject to its conditions, preserving font licensing and
  notices rather than relicensing the font as project GPL. No archive/font binary
  was fetched, hashed, inventoried or loaded. Actual binary/weight/license packaging
  proof remains a release gate; these metadata statements are appropriately limited.

## Independent examples, arithmetic and actual checks

A read-only Python stdlib audit (Counter/graphlib/Path/re/subprocess for Git reads)
passed 32 independent integer/byte/abstract-graph facts, all 29 unique AP rows and
four exact input blobs. No project parser/evaluator or implementation supplies an
oracle. Checks included the seven MiB/GiB conversions, both AP22 products,
8192-square rejection, decoded versus concurrent-buffer arithmetic, opaque/hex/
Unicode distinction, generated ASCII path lengths, duplicate/missing closure,
acyclic dependency order/self/two-node cycles, compact repeated-expansion work and
AP29's equal raw IDs versus distinct installation/content tuples. In particular:

- 8192×3906=31997952; 8192×3907=32006144; only the former fits 32000000.
- 8192²=67108864; 32000000×8=256000000<268435456; two such buffers fit 1 GiB while
  five do not. This is arithmetic, not proof of an actual decoded representation.
- Generated assets/digest and notices/digest names are 71/72 ASCII bytes. A 64-zero
  token fits digest grammar only, without hashing any payload or proving integrity.
- A 21-level doubling example expands 2097151 occurrences despite a compact graph,
  exceeding 1048576. This supports charging repeated use rather than unique nodes.

Actual repository commands:

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All passed, 406 tasks/122 decisions/local links/presets/current HTML. Git status/
exact blob reads and final sole-review-note staged scope/whitespace checks accompany
the commit. No schema/API, asset/font download, archive/XML/font/media parse, compiler,
native/crypto/renderer/provider/test/foundation/feature execution or performance
measurement occurred. The symbolic graph/math checks are document facts, not asset
validation tests or a red baseline. Permission D1 remains unanswered and untouched.

After actual R1/R2 correction, a different reviewer must inspect the exact revised
artifact again. No schema 012 integration or production stage is approved by this
review. Independent profiles/checker/API/schema/examples/tests/tool review, valid
behavioral red, implementations/source/CI, 010/008/003 closure and all conformance/
codec/OBS/minimum-version/platform/deployment/font-license/package/release gates
remain. Root owns state/HTML/build/CI/pins/schema/PR; no push/PR/main merge is done here.
