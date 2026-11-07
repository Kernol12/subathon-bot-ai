# Scene assets, fonts and portable packages — proposal v1

CONTRACT-011 FIRST logical document stage, from main `8fc9edc` and claim `1cacc34d`.
Authority: specification v0.45 §11/MEDIA-01/03/04/05/06/UI-04,
[task](../tasks/CONTRACT-011.md),
[ADR0005](../decisions/0005-scene-contract-prerequisites.md), approved
[scene model](scene-document-v1.md) blob `9a350733` and
[common values](wire-primitives-v1.md) blob `11eeb5f4`.
Proposed definition identity: `subathon.scene-assets-packages.v1`.

**PARTIAL CORRECTION CHECKPOINT, 2026-11-04:** independent review
[R1/R2](../evidence/contract-011-review.md) is still CHANGES REQUIRED. Section 2
now proposes media descriptors/allowed sets/checks; the font profile definitions,
closed SVG vocabulary and new complete independent descriptor/source examples are
still unfinished 011-owned work. Some original later-stage wording below remains
to be reconciled by that correction. Do not freeze this draft or start 012 from it.
See the [author checkpoint](../evidence/contract-011-author.md) for exact scope.

This defines logical values, independent examples and finite review proposals.
It supplies no archive codec, schema, API, decoder, validator, asset, font binary,
fixture, renderer or supported capability. Different actual review precedes another
stage. 010 still owns binding/condition/formatting/animation/presentation semantics;
011 owns the declarative profile definitions below; 012 represents their reviewed
values in complete schemas/imports and freezes concrete checking interfaces/tooling
before executable consumption. 008/003 retain complete integration gates. The
original R1/R2 review remains historical evidence; this corrected proposal requires
the same different reviewer's actual re-review, not author self-approval.

## 1. Server-owned immutable values and integrity

AssetId identifies one installation-owned resource; AssetRevisionId identifies its
immutable exact bytes and metadata. Font/visual/media/package reference kinds stay
distinct even when their opaque identities have equal bytes. Reference values carry
the owning contract/version, kind, resource identity and immutable revision, as in
009. They are not filesystem paths, display names, URLs, bearer credentials or
permission grants. New asset/revision/package identities are nonempty OpaqueBytes;
§7 proposes their bounds. Existing external domain references keep their own bounds.

An asset revision owns its byte length, exact-byte integrity digest, declared kind,
media/font descriptor, provenance and applicable rights evidence references.
Use SHA-256 over the COMPLETE original payload, represented by exactly 64 lowercase
hex characters. This is an engineering integrity choice, not authentication,
ownership, permission, licensing proof or a universal operation fingerprint.
Length/digest mismatches reject; a filename/MIME declaration cannot override bytes.
An unavailable digest/inspection is not a known match. No object changes bytes,
type/profile or immutable metadata under the same revision identity.

Identical payload bytes may share storage, but equal digests do not merge resource
identities, installation access scopes, provenance or license conditions. Every
reference must resolve through its actual scope. A hash cache grants no access.
Conflicting claimed content for one digest is an integrity failure, not permission
to overwrite the earlier object. Actual hashing/storage/deduplication remains later
implementation work; no digest has been measured for any media/font in this stage.

Retain source evidence separately from the admitted renderable revision. Inspection
never silently repairs SVG, normalizes a font, drops a track or substitutes a codec.
An authorized explicit conversion creates a NEW derivative revision with source and
transformation provenance, followed by its own checks. The old bytes/history remain.
This contract selects no converter, sanitizer or transcoding pipeline.

Provenance distinguishes bundled/uploaded/imported/explicitly-derived origin, exact
source/revision/digest evidence when known, and transformations actually performed.
Unknown source version/date/author stays unknown; upload time is not upstream
publication time. Private upload actor/host path/account information is not exported
as public provenance. A public source URL may be descriptive evidence, never an
instruction to fetch a dependency at import, preview or overlay playback.

## 2. Format/profile vocabulary and honest compatibility

Separate three observations:

1. Declared: untrusted metadata accompanying the payload.
2. Inspected: which exact bytes/checks established actual container, codec, parameters,
   decoded dimensions/font facts and selected inspection-profile revision.
3. Compatible: evidence for the intended renderer/backend/platform/profile combination.

None implies the next. An unrecognized declaration does not become a successful
empty asset. Known malformed/forbidden content is rejected; absent/incomplete
inspection or compatibility evidence is UNKNOWN/unavailable, not supported.
Known incompatibility is explicitly unsupported for that output/profile.
Do not turn these distinctions into guessed public error/HTTP codes here.

A media descriptor states the actual container/version, per-track codec and precise
codec profile/level/features, pixel/sample representation, dimensions, alpha/color
handling, frame/time-base or sample-rate/channel data and loop/timing metadata where
applicable. Absent/not-applicable/unknown remain distinct. Counts/lengths are bounded
unsigned exact values; rates/time bases use exact bounded rational values, never
binary floats for stored metadata. Presentation timing/trigger/mute/autoplay policy
is not inferred from a codec or invented here; it stays under 010/renderer owners.

A CodecProfile is an immutable versioned definition with a common
CapabilityIdentifier, exact descriptor variant and check-set revision. The following
011-owned media entries below are DECLARATIVE proposals, not codec selection by an
installed runtime or playback/UI defaults. The proposed media registry has ten
entries; the font registry and complete SVG definitions remain unfinished at this
checkpoint, so the complete 011 profile gate cannot pass. An unrecognized
profile/version is UNKNOWN and cannot authorize compatible publication. A known
variant outside its admitted set is profile-incompatible, not malformed merely
because another profile is needed.
Malformed framing/contradictory inspected fields and forbidden content reject;
incomplete inspection stays UNKNOWN. No required release family is removed. If these
profiles are insufficient for a required valid asset, 011 must review a sufficient
additional version before its schema/implementation; 012 cannot invent that policy.

### 2.1 Closed logical descriptor notation

`Media(profile, encodedBytes, details)` has exactly those three fields. `profile`
selects the exact details record below. `encodedBytes` is positive U64, matching the
complete actual original bytes and the current §6 cap; it is no digest/certificate.
All record keys in a selected row are required, with no extra keys/property bag.
An empty ordered list is a KNOWN empty set; `none` is a KNOWN absent optional stream,
not UNKNOWN. No unknown or not-applicable field may be replaced by zero. Fields
inapplicable to a selected record simply are not in that record; they cannot appear
as guessed null values. Logical notation is not an adopted JSON/schema encoding.

`U` means 0..18446744073709551615; positive counts exclude zero. Stored integer
text is canonical unsigned decimal. `Q(n,d)` is a reduced rational of integers with
positive denominator and at most 32 decimal digits per component; a rate/duration
below is positive unless explicitly stated otherwise. Native approximations do not
replace retained exact metadata. `mono` means one channel, `stereo` means ordered
left/right; no implicit downmix. Ordered records retain actual stream/frame order.
All profiles require inspected codec/version, complete framing/length/checksum
checks where applicable, no encryption, external data references or executable
metadata, and no hidden extra media tracks. Non-rendering comments/tags may be
retained as bounded inert metadata under §7; their contents never select a loader,
URL, font, script or command. Unknown rendering-affecting extensions cannot be
silently classified as comments or removed from the descriptor.

### 2.2 Static images and GIF

Profile names in this table have prefix `subathon.asset.`. Each static raster has
one image, positive integer width/height subject to BOTH configured raster limits.
No decoder may silently ignore extra animation or apply a second orientation.
`srgb` means the inspected source specifies sRGB; `untagged-srgb` means inspected
absence of a color profile with this profile's explicit common sRGB interpretation.
ICC/other transfer/primaries require another reviewed profile; absence of inspection
is neither of these values. This is a proposed shared interpretation, not an
observation that either renderer currently follows it.

| Exact profile suffix | Complete details record and admitted combinations | Required additional checks |
| --- | --- | --- |
| `png.v1` | `PNG(width,height,colorType,depth,interlace,transparency,color)`; colorType/depth pairs: 0/{1,2,4,8,16}, 2/{8,16}, 3/{1,2,4,8}, 4/{8,16}, 6/{8,16}; interlace 0 or 1; transparency `none`, `key` for types 0/2, `palette` for type 3, or `channel` for 4/6; color `srgb` or `untagged-srgb` | Exact PNG signature, critical chunk ordering/CRC, palette/sample/index bounds, filter/inflate completion; channel types require `channel`, other types prohibit it. `key`/`palette` require valid tRNS. Reject acTL/fcTL/fdAT animation. gAMA/cHRM/iCCP/cICP/color extensions outside the selected two color cases require another profile, never an ignored color mismatch |
| `jpeg.v1` | `JPEG(width,height,process,precision,components,subsampling,orientation,color)`; process `baseline` or `progressive-huffman`; precision 8; components `gray`/subsampling `1x1`, or `ycbcr`/subsampling `444`,`422`,`420`; orientation integer 1..8; color `srgb` or `untagged-srgb` | One complete JPEG through EOI, valid tables/scan progression/restarts, bounded scans, no arithmetic/hierarchical/CMYK/YCCK. Establish EXIF orientation (absence means 1), actual component sampling/color and exactly one corresponding display transform; no auto-rotation disagreement |
| `webp.v1` | `WebP(width,height,coding,alpha,color)`; coding `vp8-lossy` or `vp8l-lossless`; alpha boolean; color `srgb` or `untagged-srgb` | RIFF/WEBP exact bounded chunk sizes/padding, bitstream and alpha consistency; reject ANIM/ANMF or animation flag, multiple primary images, ICC/other rendering extensions outside this profile. VP8X is allowed for bounded alpha/inert metadata, not a bypass |
| `gif.v1` | `GIF(version,width,height,paletteEntries,backgroundIndex,loop,frames)`; version `89a`; paletteEntries 2,4,8,16,32,64,128,256; backgroundIndex in palette; loop `once`, `forever`, or `repeat(U)` with count 1..65535; frames is 1..65536 ordered `Frame(x,y,w,h,paletteEntries,transparentIndex,disposal,delayCS,interlace)` records | Full header/trailer, LZW decode and index/sub-block completion, known global palette plus optional bounded local palette; frame w/h positive, x+w<=width and y+h<=height. Frame palette uses the same finite sizes; transparentIndex is `none` or an in-palette index. Disposal 0/1 keeps canvas, 2 restores logical-screen background, 3 restores saved prior canvas; actual compositing must agree. delayCS integer 2..65535 (centiseconds), interlace boolean; raw delays 0/1 need another profile, not hidden browser clamping. Only the known NETSCAPE2.0 loop extension, comments and graphics control are admitted; no plain-text/unknown application rendering extension |
| `svg-static.v1` | `SVG(width,height,viewBox,aspect,elements,attributes,references,expandedUses,pathSegments)`; width/height and viewBox exact numbers per §3; aspect is §3's closed value; remaining fields are actual counted unsigned values under §3/7 | The COMPLETE closed source/check profile in §3, not a generic native SVG validity result |

GIF loop is source metadata, not a new scene autoplay/retrigger policy. Its finite
delay restriction is a profile compatibility condition, not a change to built-in
animation timing under 010. Required broader source behavior needs an explicit
011 profile extension rather than silently rewriting the original asset.

### 2.3 Closed audio records

An audio stream record is one of the following complete variants. `samples` counts
audible sample frames per channel after explicitly inspected priming/end trim;
duration is exactly Q(samples,rate), never channel-count multiplied. Rates/channels
must remain constant within a stream. For video-embedded audio the same records
apply, with matching container timing/priming; `none` means no audio track.

| Record | Complete fields and admitted set |
| --- | --- |
| `PCM(rate,layout,bits,samples)` | rate in {8000,11025,16000,22050,24000,32000,44100,48000,96000}; layout mono/stereo; bits 8 or 16, unsigned 8-bit or signed little-endian 16-bit integer samples; samples positive U |
| `AAC(rate,layout,samples,priming,padding)` | AAC-LC only, exact codec `mp4a.40.2`, no SBR/PS; rate 44100 or 48000; layout mono/stereo; priming/padding U each <=65535; decoded frame counts and trims establish positive samples; no implicit zero/unknown trim |
| `MPEG1Layer3(rate,mode,rateMode,kbps,samples,priming,padding)` | rate 32000/44100/48000; mode mono/stereo/joint-stereo (latter two ordered L/R); rateMode `cbr` or `vbr`; kbps sorted distinct subset of {32,40,48,56,64,80,96,112,128,160,192,224,256,320}, singleton for cbr; priming/padding each <=65535; positive samples after valid documented trim (known absence is 0). No free-format, MPEG2/2.5, layers I/II or unexplained sync loss |
| `Vorbis(rate,layout,smallBlock,largeBlock,samples)` | Vorbis I version 0; rate 44100/48000; layout mono/stereo; block sizes powers of two from 64 through 8192, smallBlock<=largeBlock; positive audible samples from final valid granule/timing, not nominal bitrate hints |
| `Opus(rate,layout,mapping,preSkip,gainQ8,samples)` | OpusHead version 1; decoded/granule clock rate 48000; layout mono/stereo and mapping family 0; preSkip 0..65535; gainQ8 signed -32768..32767, applied exactly once using the codec's gain semantics; positive samples after preSkip and final trimming. Header input sample-rate hint is not substituted for the 48000 clock |

Required checks include every frame/packet, valid codec headers/setup/codebooks,
sample/granule count/trim consistency, checksum where present, complete sequential
decode with bounded scratch and no hidden discarded stream. MP3 VBR hints do not
prove its actual bitrate set/sample count. Vorbis requires all three ordered header
packets, valid setup and bounded expanded codebooks, not just identification. Opus
requires actual packet durations/counts/continued-packet/end-trim validation. These
are check obligations, not claims that an in-process API exposes all controls.

### 2.4 Video and audio containers

`Video(codec,width,height,rate,frames,color,alpha,sampleAspect,rotation,refs)` has
exactly those fields. Width/height are positive integers <=8192 with product
<=67108864, distinct from configurable raster IMAGE limits. `rate` is exact Q in
[1/1000,240], `frames` positive <=1048576; these versions require constant presentation
cadence starting at 0, duration Q(frames,1)/rate, no edit-list/VFR/field/interlaced
presentation. Decode-order reordering is allowed when bounded and consistent.
`color` is `bt709-limited` or `bt709-full`, established from actual metadata;
alpha false; sampleAspect Q(1,1); rotation 0/90/180/270 degrees from inspected
container transform (WebM requires 0). These are finite source-profile choices,
not UI rotation, image caps or a measured throughput/duration guarantee. `refs` is
the actual maximum live reference-picture count, 0..16 and also within codec limits.

| Exact profile suffix | Complete details record / allowed combination | Additional rejection/check obligations |
| --- | --- | --- |
| `mp4.v1` | `MP4(brand,compatibleBrands,video,audio)`; brand in {isom,iso2,mp41,mp42,avc1}; compatibleBrands nonempty unique ordered subset of that set containing brand; exactly one Video with `codec=AVC(prefix,level)` below; audio `none` or AAC | Finite self-contained nonfragmented ISO-BMFF, single video/at most one audio, no encryption/external data/extra subtitles/alternate streams; every box length/sample-table offset/count and codec parameter set checked. All sample ranges within exact payload; actual timing/cropping/rotation/color/DPB and complete decode match the descriptor |
| `webm.v1` | `WebM(readVersion,video,audio)`; readVersion 2 or 4; one Video with codec `VP8(0)` or `VP9(0)` (version/profile 0), 8-bit YUV420 only; audio `none`, Vorbis or Opus | Exact WebM doc type and admitted EBML version/features, one bounded segment, complete clusters/cues/lacing/timestamps and track headers; no encryption/attachments/external references/extra tracks/alpha. VP9 level/feature metadata and actual bitstream agree and obey codec limits; metadata absent does not prove a feature |
| `wav.v1` | `WAVE(pcm)` with exactly one PCM record | RIFF/WAVE, integer WAVE_FORMAT_PCM=1 only, fmt/data required exactly once, valid padding/size; blockAlign=channels*bits/8, byteRate=rate*blockAlign, dataBytes=samples*blockAlign. No RF64/RIFX/float/compressed audio or second data stream in this version; bounded inert LIST/JUNK/bext metadata cannot select external content |
| `mp3.v1` | `MP3(audio)` with exactly one MPEG1Layer3 record | Elementary MPEG1 layer III stream, complete contiguous frames; only bounded inert ID3v1/v2 and recognized Xing/Info/LAME timing metadata. No executed tags, automatic external fetch, concealed second stream or best-effort sync repair |
| `ogg.v1` | `Ogg(version,serial,chain,audio)`; version 0, serial U32, chain false; one Vorbis or Opus record | One complete logical stream, valid page sequence/CRC/BOS/EOS/packet continuation/granules, no multiplexing/chained stream or hidden video in this version; codec and all headers genuinely inspected |

For AVC, `prefix` is exactly one of `4200,42c0,42e0,4d00,4d40,6400` (hexadecimal
profile/constraint bytes), and `level` is one of `1e,1f,28,29,2a,32,33,34` (levels
3.0/3.1/4.0/4.1/4.2/5.0/5.1/5.2). The source must conform to the indicated
H.264 profile/constraints, progressive 8-bit YUV420 and its level/DPB/bitrate bounds;
an `avc1` string does not prove this. Codec strings follow the RFC 6381 distinction
between container and codec. No SVC/MVC/high-bit-depth/4:2:2 extension is inferred.
The proposed additional frame/cadence admission ceilings in macroblocks are:

| Level hex | ceil(width/16)*ceil(height/16) <= | macroblocks*rate <= |
| --- | ---: | ---: |
| 1e | 1620 | 40500 |
| 1f | 3600 | 108000 |
| 28 or 29 | 8192 | 245760 |
| 2a | 8704 | 522240 |
| 32 | 22080 | 589824 |
| 33 | 36864 | 983040 |
| 34 | 36864 | 2073600 |

The codec's remaining normative restrictions still apply; this table is not an
AVC validator. Media/container facts come from the linked primary specifications
in §9; the restricted admitted combinations and finite limits here are engineering
proposals for actual review, not declarations that those standards impose our caps.

### 2.5 Finite inspection checks and ownership

Every profile uses `scene-asset-inspection.v1` (§7), including current configured
encoded caps, 1 GiB concurrent ownership and real supervised attempts. Additional
011-owned inclusive ceilings: container nesting 32; box/chunk/page/frame/packet
records 1048576; one packet or codec setup body 16777216 bytes; aggregate decoded
inert metadata 16777216 bytes; JPEG scans 1024; cumulative GIF frame-rectangle
pixels 2147483648; held decoded audio 16777216 bytes and a decoded sample-frame
block <=65536. Project complete byte/item/decoded/scratch/output demand BEFORE
growth/native handoff. Parse integers/counts/offset products/rational projections
with checked arithmetic; a claimed size and cached descriptor cannot waive this.

Charged walks include every table/packet/sample record and codec-setup expansion,
not just file bytes. Stream full inspection in bounded rolling buffers; neither a
finite frame count nor decoded-audio block permits whole-video/audio caching.
If full inspection cannot finish inside actual containment/work/time/memory limits,
result is unavailable/UNKNOWN, not a fabricated complete check. A known profile
limit overrun is explicit incompatibility/resource refusal. Profile matching never
grants source authenticity, account rights, execution permission or codec capacity.
Concrete byte parsers/check ordering/error APIs remain separately reviewed work;
the closed logical allowed sets/check requirements themselves are now owned here.

Compatibility evidence binds the immutable payload/descriptor where asset-specific,
the profile/check-set revision, renderer implementation, Qt/backend or browser/CEF
build, OS/architecture and actual result/scope. Generic platform evidence is not a
claim that every file is valid; partial/header/sample checks are not mislabeled full
decode/playback. Required admission checks must be completed, with no required
UNKNOWN result, before compatible publication. Later corrupt-frame/playback failures
still produce bounded visible failure; inspection is not a guarantee against every
native failure. Actual reference tests must exercise both intended outputs and
rejection cases. Required features cover actual content, not a shorter caller list.

## 3. Restricted static SVG: required semantics and rejection

Preserve basic shapes, paths, groups, fills/linear and radial gradients, strokes,
transforms and same-document internal references. The profile must cover rectangle,
circle/ellipse, line/polyline/polygon and full path-command families including curves
and arcs, with their reviewed numeric grammar. `svg`, grouping/definition containers
and these required primitives must not be reduced to a raster-only substitute.
Editable text belongs to scene Text; SVG text must have been converted to paths.

Internal IDs are unique and references resolve in this exact document. Same-document
fragment references for reuse/paint/gradient definitions are allowed only with the
correct target kind, complete closure and bounded acyclic expansion. Shared targets
still charge each use/work occurrence. A missing/wrong-kind/cyclic reference is not
an empty successful drawing. Source numeric spellings follow the reviewed SVG-number
profile, not 009's canonical geometry string grammar or a financial parser shortcut.

Reject scripts, event handlers, external resources, embedded HTML/foreignObject,
SVG/SMIL/CSS animations and filters. Reject `<text>`/textPath rather than silently
drawing system text or outlining it during validation. Remote/file URLs, data URLs,
external styles/fonts/images and xml:base resolution cannot bypass the prohibition
on external resources. Reject DTD/entity declarations and external resolution;
built-in XML escapes/numeric character references remain subject to decoded bounds.
No loader/network access occurs to discover whether an external reference is safe.

Presentation properties require a closed reviewed whitelist that preserves required
fill/gradient/stroke/transform semantics. Its exact attribute/restricted-style grammar
and namespace rules must be frozen before parsing under 012; there is no general
stylesheet/expression/native-object escape hatch. Unknown elements/attributes or
features outside that profile reject with a precise bounded issue, never disappear
through best-effort rendering. Required compatibility covers the same semantics on
both renderers. Disabling animation playback is not rejection of forbidden content.

Raw byte length does not bound paths, references, numeric construction or generated
pixels. §7 proposes separate finite structure/work/intermediate limits. Actual
content validation must precede unsafe renderer loading; inability to prove/enforce
the profile is unavailable, not a reason to set a trusted-source flag. There is no
selected XML/SVG validator or sanitizer in this document.

## 4. Fonts, fallback, rights and Noto provenance

Retain uploaded TTF/OTF and an explicitly configured fallback. A FontRef resolves
to exact font bytes/revision plus an explicit face/style/weight selection; variable
axes or collection/embedded-feature claims need their own precise profile and actual
compatibility evidence. A filename/family string or an operating-system match does
not establish the intended face. A failure cannot silently substitute the host font
while claiming the original face succeeded. No claim of universal glyph coverage
follows from the family name.

Font profiles must identify active outline, hinting, variable/color/embedded-feature
behavior and its bounded validation/runtime strategy. An SFNT header or successful
name lookup alone does not admit an unknown interpreter/embedded-content feature.
Keep TTF/OTF required; define sufficient tested profiles instead of declaring every
font or every internal program safe from its extension.

The native and browser outputs receive the SAME admitted font assets and equivalent
face/fallback selection. Loading, glyph coverage, weight selection and metrics need
actual separate checks; minor permitted raster differences do not waive layout or
binding conformance. Do not synthesize a missing Bold face and call it the validated
bundled Bold asset. Typography/layout parameter semantics remain 009/010/renderer
work, not an implicit platform default chosen by import.

Bundle Noto Sans Regular and Bold for built-in interfaces/default themes. Retain
SIL OFL and copyright/notices separately from project GPL. Before distribution,
record actual source/version, exact file names/bytes/hashes, face/weight evidence,
license/notices and any transformation. Do not infer these facts from a source tag.

Metadata-only candidate: upstream [NotoSans-v2.015](https://github.com/notofonts/latin-greek-cyrillic/releases/tag/NotoSans-v2.015),
source commit `c4a321e123e4d4ff315f57f4e0adf294fe3a95be`, corroborated by
[Google Fonts metadata](https://raw.githubusercontent.com/google/fonts/main/ofl/notosans/METADATA.pb).
The release advertises a 112 MB archive; it was NOT downloaded or inventoried.
The metadata describes variable TTF assets, not checked static Regular/Bold files.
Actual archive/file hashes, weight/coverage tests, static-versus-variable instance
choice and reproducible packaging remain root-owned review gates. No binary pin is
selected here. The source [OFL-1.1](https://raw.githubusercontent.com/notofonts/latin-greek-cyrillic/c4a321e123e4d4ff315f57f4e0adf294fe3a95be/OFL.txt)
allows software bundling subject to its conditions, including copyright/license
retention and applicable modified-name rules; it does not relicense fonts as GPL.

For every asset, distinguish usage/delivery scope and redistribution evidence from
account permission. Known permitted, known prohibited and unknown rights evidence
are separate states with exact conditions/provenance. An upload or owner role is not
license evidence. Immutable evidence is historical; current applicable scope,
conditions and known expiry/revocation still need checking at actual use/export.
Immutability does not freeze entitlement or imply that every license is revocable.
Portable exports include required permitted fonts/assets and
notices; unknown or prohibited redistribution cannot produce a purportedly complete
portable package by dropping the font or substituting an unrecorded fallback.
An explicitly authorized alternative theme can select different permitted assets,
but that creates a new reviewed content/package revision. This models evidence and
required checks, not automated legal adjudication or a new licensing service.

## 5. Portable closure and path/archive safety

A package revision selects exact scene content revision(s), typed dependencies,
asset/font revision mappings, payload lengths/digests, license/notices and its own
definition/profile versions. Resolve every required transitive dependency once in
the correct namespace and reject conflicting definitions. Include exact required
assets/fonts and notice content; no dependency depends on the designer's machine,
current mutable setting, credential or an automatic external download.
010 definitions are still UNFINALIZED; this document cannot complete their closure
with a placeholder. 009 binding/visibility/presentation requirements remain intact.

Package source identities/scopes do not automatically become destination authority.
Cross-installation import or collision handling needs an explicit complete mapping
to destination scene/asset/revision identities, updating ALL typed references in a
new imported revision while retaining source provenance. Equal raw IDs cannot alias
different existing destination content. Rebinding installation-specific 010 domain
references needs that owner's actual rules and current rights; unknown mappings
remain a draft issue, not permission to select a different timer/private data source.
This selects no identity generator or automatic merge policy.

Proposed logical entry namespace: `manifest`, `scene`, `assets/<sha256>` and
`notices/<sha256>`, where sha256 is exactly 64 lowercase ASCII hex characters.
These are abstract relative entry names, not a choice of JSON/archive serialization.
The manifest maps typed opaque identities to entries; raw IDs and uploaded display
names NEVER become path components. This small generated ASCII namespace avoids
case/Unicode normalization and Windows-name collisions without normalizing identity.
Equal payloads may share one entry, with every typed mapping/reference still checked.

Every logical entry occurs exactly once. Reject duplicate names even if bytes match,
absolute/drive/UNC paths, empty/dot/dot-dot components, backslashes, NUL, encoded
separator/traversal tricks, case aliases and names outside the closed namespace.
No URL/path percent-decoding, Unicode normalization or native-path reinterpretation
turns an invalid name into a valid one. Manifest membership is exact; reject missing
or extra entries, digest/length mismatches and undeclared assets. A same digest/path
with different claimed payload is a conflict, not last-entry-wins.

The eventual container profile must reject links, symlinks/hardlinks, device/FIFO/
socket/sparse/privileged entries, executable metadata and unsupported/encrypted or
unbounded nested ARCHIVE containers. Required declared media containers such as
MP4/WebM are evaluated by their media profiles, not confused with nested archives.
Preflight the full inventory, declared compressed/
expanded bounds, destination closure and entry types before creating entries.
Enforce ACTUAL expanded byte counts, lengths and digests during bounded staging;
declared sizes alone are not evidence of actual expansion. Never
follow a container's path or create a link outside a private staging root. An
unsupported container is refused, not interpreted by a convenient host program.
This selects NO ZIP/tar/library/tool; exact container framing and tests require a
later reviewed choice under root ownership before 012/executable consumption.

Import stages the COMPLETE candidate separately from current assets/live scene.
Check expected revision/current execution-time rights, inventory/integrity/content,
required compatibility and permissions before authoritative publication. Pre-publication
failure must not change current heads. Clean owned staging, or explicitly report
incomplete cleanup/quarantined work rather than claim it disappeared. Actual
transaction/publication atomicity, known rollback versus unknown outcome, retries,
current disclosure and cleanup failure belong to the server/writer integration;
do not promise filesystem-wide or database/filesystem cross-resource atomicity.
Export publication failures may leave partial external output and must report that
honestly; only a complete validated successful export is a portable artifact.

## 6. Product budgets and authorized changes

Preserve the specified INITIAL, authorized-configurable maximums exactly:

| Encoded payload / package | Initial maximum bytes |
| --- | ---: |
| Each image or font | 33554432 (32 MiB) |
| Each audio asset | 134217728 (128 MiB) |
| Each video asset | 536870912 (512 MiB) |
| Packaged scene representation | 2147483648 (2 GiB) |

Raster images also have maximum 8192 pixels per edge and 32000000 pixels total.
Both dimension conditions apply, using checked arithmetic; an 8192×8192 image does
not fit merely because each edge fits. Animated-image frames/canvas must be checked
against the actual selected image profile; small encoded size never proves small
decoded dimensions/work. Do not silently apply these image limits as a universal
video/font/scene-canvas product limit. Warn on demanding scenes under a reviewed
warning policy; no demanding-scene threshold or user-facing default is invented here.

Authorized limit changes are versioned trusted configuration, checked at actual
admission/publication with current rights. A package's claimed limits/capabilities
cannot raise them. A higher configured file cap does not enlarge an implementation's
worker/decoded budget; use an actually sufficient reviewed processing profile or
report explicit resource/unavailable failure. Never truncate or silently rewrite
retained valid assets. Product budget changes and safe implementation-profile changes
are distinct; neither authorizes bypassing the other or resurrecting rejected work.

## 7. Finite inspection/processing profile — proposals, not measured capacity

Propose `scene-asset-inspection.v1` as a review starting point. These finite technical
ceilings are separate from configurable MEDIA-04 budgets and from 009's scene-owned
profile. They bound NEW metadata/inspection work, not existing financial references
or all possible valid media. Actual adoption/enforcement and sufficient profiles for
required assets need different review before decoding or support claims.

| Quantity | Proposed inclusive ceiling |
| --- | ---: |
| New asset/revision/package identity, raw bytes | 65536 per field |
| Display label/source-description Text | 65536 UTF-8 bytes per field |
| One notice Text; aggregate metadata/notices | 1048576; 16777216 UTF-8 bytes |
| Logical package entries / typed dependency mappings / reference uses | 4096 / 4096 / 32768 |
| Expanded ORIGINAL entry bytes staged by one package inspection | 4294967296 (4 GiB) |
| One SVG XML nesting depth / elements / attribute occurrences | 64 / 65536 / 262144 |
| One SVG reference-expansion occurrences / path segments | 1048576 / 1048576 |
| One SVG source numeric token / absolute exponent | 128 ASCII bytes / 256 |
| Font table count / glyph count / outline-component depth | 1024 / 65535 / 64 |
| One complete admitted decoded raster buffer | 268435456 (256 MiB) |
| Concurrent owned decoded/intermediate inspection bytes | 1073741824 (1 GiB) |
| Distinct animated-image frames examined per asset | 65536 |
| Per-new-media rational numerator/denominator | 32 decimal digits each |
| Cumulative checked inspection work occurrences | 16777216 |
| One active expensive inspection worker / supervised attempt time | 1 / 30 seconds |

Rationale: counts/depth bound graph/table walks independently of file size; expansion
counts charge repeated SVG use instead of assuming a compact DAG is cheap. The 4 GiB
staging ceiling is an explicit expanded-entry proposal, not a reinterpretation of
the 2 GiB serialized package cap or permission to hold 4 GiB in RAM. New 64 KiB IDs
avoid imposing correlation/geometry limits on byte identities. Per-field text and
16 MiB aggregate prevent multiplied maxima. Font counts/component depth bound table/
outline work; successful parsing alone does not establish safe caching or coverage.

The 256 MiB raster allowance can hold a 32000000-pixel eight-byte-per-pixel buffer
(256000000 bytes), before other allocations. The 1 GiB aggregate separately charges
all simultaneously owned buffers/copies, codec/parser scratch and intermediates.
It is not a claim that a native API exposes an allocator cap. Decode projections,
frame/cache admission and actual containment/accounting must be established before
calling a potentially expanding decoder; otherwise inspection is unavailable.
No unlimited frame cache or whole-video decompression follows from an upload cap.

Checked work charges include every traversed entry/element/attribute/reference/
path segment/font table/glyph component/outline point/frame and repeatedly expanded
use. These are logical inspection-walk occurrences, not a claim to count or bound
every opaque codec/hint-interpreter CPU instruction. Concrete
walk order, distinct versus repeated storage, numeric bit/range/intermediate bounds,
compressed/container/token limits and output ownership MUST be frozen by 012 before
an executable baseline; this table is not a complete parser allocation contract.
Existing 009/external limits still apply together, with actual bytes charged every
occurrence. Neither unknown metadata nor a reference may hide an unbounded body.

Time is an inspection-attempt bound, NOT a 30-second media-duration limit or native
cooperative cancellation guarantee. An implementation must actually enforce its
reviewed supervision/work/memory strategy; it cannot abandon a live decoder/thread
and free borrowed buffers on timeout. Timed-out/incomplete work yields no successful
inspection/publication. No process library, hard in-process allocator/deadline or
throughput claim is selected. Actual resource/cancellation tests and measurements
remain required; no supplied Qt/browser flag alone proves these controls.

## 8. Independent logical examples

AP rows are conceptual inputs/outcomes, not archive bytes, executable fixtures or
claims that a decoder/font/media ran. D/N denote explicitly assumed matching
64-lowercase-hex digests of the stated complete asset/notice payloads, NOT actual
measured file hashes or literal path letters. Each row
has its own disposable context. A structural/metadata success is labeled with its
scope and does not fill in unresolved 010/schema/renderer/license evidence.

| ID | Independent condition | Required logical outcome / scope |
| --- | --- | --- |
| AP01 | VisualAssetRef to asset b"A", revision b"r1", declared exact byte length and D, matching admitted payload and checks | Identity/integrity link is coherent under those premises; not permission, source authenticity or renderer compatibility proof |
| AP02 | FontRef and VisualAssetRef share bytes b"A"/b"r1" | Kind remains essential; a font definition cannot satisfy a visual asset merely by equal identity bytes |
| AP03 | IDs b"\x00\x80\xff" and b"\x00"; Unicode display label "e\u0301" versus "\u00e9" | Exact distinct byte/scalar identities survive; no path/C-string/normalization collapse |
| AP04 | Claimed length differs by one or digest differs while filename/MIME matches | Integrity refusal; no import/publish or repair from filename |
| AP05 | Identical bytes/digest under two differently scoped authorized mappings | Payload storage may be shared; both mappings/rights and every reference remain checked, no access grant from the digest |
| AP06 | Scene requires font F and visual V, package contains V only, or required 010 layout remains UNFINALIZED | Missing transitive closure; no complete portable/publication success, no local-font/unknown-layout substitute |
| AP07 | Abstract entries manifest, scene, assets/D and notices/N exactly match all required mappings/lengths/digests/rights | Structural closure can be coherent under the stated premises; complete container/profile/content/renderer proof still required |
| AP08 | Duplicate assets/D entry with equal or different bytes; undeclared extra asset; absent declared entry | Reject duplicate, extra or missing inventory respectively; no last-entry-wins |
| AP09 | Entry ../font.ttf, /font.ttf, C:\\font.ttf, backslash path, percent-encoded separator, symlink or sparse entry | Reject outside closed portable namespace/type profile before extraction; do not normalize into acceptance |
| AP10 | Opaque AssetId is 4097 bytes and is mapped to generated assets/D | Can fit new identity/aggregate bounds; it is not a 4097-byte filesystem filename or a 256-byte correlation |
| AP11 | PNG/JPEG/static WebP descriptor has bounded valid metadata, but browser profile evidence is UNKNOWN | Not compatible publication for both outputs; retaining a draft is not dropping that release format |
| AP12 | MP4 container declared, actual AVC profile/level or audio codec not established | Container name cannot establish compatibility; require precise inspected descriptor and actual evidence |
| AP13 | WebM descriptor names VP9/Opus; native evidence succeeds but selected OBS/CEF evidence absent | Browser/OBS support remains UNKNOWN; native or Google Chrome evidence cannot fill it |
| AP14 | WAV, MP3 and Ogg are declared, without respective sample/MPEG/stream codec parameters | All remain required families; insufficient descriptor is not supported playback or a reason to remove the formats |
| AP15 | SVG paths/groups/gradient with unique same-document IDs and bounded acyclic use, no forbidden content | Fits the logical required subset under those premises; actual parser/dual-renderer checks remain required |
| AP16 | SVG script/event handler, file/HTTP/data URL, xml:base, DTD, foreignObject, animate/style animation, filter or text | Reject forbidden content; no trusted-source flag, disabled-animation pass or silent text outlining |
| AP17 | SVG references a missing/wrong-kind ID, self-cycle or a two-node use cycle | Reject incomplete/cyclic expansion; compact encoded graph is no work-budget exemption |
| AP18 | Uploaded TTF/OTF loads by name on one host, but actual face/hash/fallback or other output is unknown | No matching-font/conformance claim; actual uploaded format/faces and configured fallback need evidence |
| AP19 | Font use is permitted for one scope, but package redistribution evidence is unknown/prohibited | No complete portable export; do not omit/substitute font while claiming original theme portability |
| AP20 | Proposed Noto source tag/OFL metadata known, binary files/weights/hashes uninspected | Metadata-only pin candidate; no bundled Regular/Bold or glyph/metrics/package validation claim |
| AP21 | Initial image/font 33554432 vs 33554433 bytes; audio 134217728 vs +1; video 536870912 vs +1; package 2147483648 vs +1 | At-boundary fits only its individual initial cap, one-over explicitly refuses; all other checks still apply and authorized revised caps are separate |
| AP22 | Raster 8192×3906 versus 8192×3907 | 31997952 fits both initial dimension bounds; 32006144 exceeds 32000000 despite each edge fitting |
| AP23 | Raster 8192×8192; tiny compressed image claims 32000000 decoded pixels at eight bytes each | First fails total pixels; second requires 256000000 decoded bytes plus actual scratch, not its compressed length |
| AP24 | 65536-byte new identity vs 65537; 4096 entries vs 4097; repeated use exceeds reference/work ceiling | Explicit technical-profile refusal at one-over, no truncation/dedup discount; valid required larger work needs reviewed sufficient profile |
| AP25 | Authorized file cap raised while actual decoded/worker profile still too small; package asserts unlimited budget | Neither raises implementation capacity; explicit resource/unavailable outcome, no dangerous native call or successful compatibility claim |
| AP26 | Package staging/content checks fail or source live head changes before commit | Existing assets/history/live head stay; actual rights/CAS/transaction checks must reject stale work, not partially publish |
| AP27 | Export output write/flush fails after some bytes, or inspection attempt times out | Partial external output/incomplete inspection is honestly unsuccessful; no portable artifact or known rollback inferred from timeout |
| AP28 | Integrity token is 64 ASCII zeroes, versus 63/65 characters or uppercase A followed by 63 zeroes | First fits token grammar only, not a claimed payload match; others fail exact lowercase/length grammar, with no normalization into acceptance |
| AP29 | Imported source asset b"A"/b"r1" collides with different destination content under equal raw IDs | No implicit alias/overwrite; an authorized explicit complete remapping may create a new imported revision only with updated closure and actual destination-domain rules, not guessed 010/private-source rebinding |

## 9. Primary facts and later gates

Primary documents establish available APIs/standards, not project support:

- [Qt Multimedia 6.12](https://doc.qt.io/qt-6.12/qtmultimedia-index.html) documents its
  FFmpeg 9.0.1 desktop backend, platform differences and Linux deployment exception.
  [QMediaFormat](https://doc.qt.io/qt-6.12/qmediaformat.html) exposes decode queries;
  those queries do not prove the actual intended asset played.
- [Qt image formats](https://doc.qt.io/qt-6.12/qtimageformats-index.html) documents
  WebP plugins; [QImageReader](https://doc.qt.io/qt-6.12/qimagereader.html) has bounded
  allocation controls but quick canRead is not complete decode success.
- [QtSvg options](https://doc.qt.io/qt-6.12/qtsvg.html) can disable animations, while
  AssumeTrustedSource disables resource checks. Tiny12 is not this app's whitelist.
  [SVG processing modes](https://www.w3.org/TR/SVG2/conform.html#secure-static-mode)
  distinguish secure static from secure animated image embedding; neither a bare
  browser image element nor disabled playback rejects all forbidden source content.
- [Chromium media](https://www.chromium.org/audio-video/) documents build-dependent
  AVC/AAC support; Chrome does not certify OBS/CEF. [RFC 6381](https://www.rfc-editor.org/rfc/rfc6381.html)
  distinguishes codecs/profiles from containers; [WebM guidelines](https://www.webmproject.org/docs/container/)
  identify candidate video/audio combinations, not tested project profiles.
- [PNG third edition](https://www.w3.org/TR/png-3/), the official
  [WebP container specification](https://developers.google.com/speed/webp/docs/riff_container)
  and [GIF89a specification](https://www.w3.org/Graphics/GIF/spec-gif89a.txt) provide
  the source-format distinctions behind the proposed static/animated descriptors.
  [W3C's JPEG summary](https://www.w3.org/Graphics/JPEG/) distinguishes baseline and
  progressive; the [T.81 listing](https://www.itu.int/rec/T-REC-T.81-199209-I/en)
  does not expose its full paid text. That text was not inspected; no decoder ran.
- [WAVEFORMATEX](https://learn.microsoft.com/en-us/windows/win32/api/mmreg/ns-mmreg-waveformatex)
  defines PCM alignment/rate fields; [RFC 3003](https://www.rfc-editor.org/rfc/rfc3003.html)
  distinguishes MPEG audio layers and inert metadata. The finite sample-rate/codec
  subsets here are proposals, not every stream allowed by those formats.
  [Vorbis I](https://www.xiph.org/vorbis/doc/Vorbis_I_spec.html) defines version 0,
  ordered setup and block sizes; [RFC 7845](https://www.rfc-editor.org/rfc/rfc7845.html)
  separates Opus's 48000 granule clock, pre-skip and output gain from input-rate hints.
- [QFontDatabase](https://doc.qt.io/qt-6.12/qfontdatabase.html) loads TrueType/OpenType
  and offers application fallbacks; [CSS Font Loading](https://www.w3.org/TR/css-font-loading-3/)
  specifies browser loading. Neither a family match nor source/license metadata
  establishes actual same-font renderer behavior.

No QtBase/Core installation proves Multimedia/FFmpeg/SVG/plugin/font behavior.
Before 012/implementation require actual closed profile/descriptor/entry/checker
definitions, field walks/error priority, finite representation/work budgets and
reviewed pinned tooling choices. Then require independent literal fixtures/tests,
different review and valid intended red before production SCENE-005/006/RENDER-004,
with separate actual source/configuration review and passing acceptance/CI.
No missing dependency/compile failure/skip is a valid red.

010 semantic closure, 008/003 integration, independent renderer/reference scenes,
SVG rejection, font/Noto/OFL/notices, configured budgets/demand warnings, 60/30 FPS,
SPIKE-004 codec/OBS minimums and actual Windows/Linux/Wayland/browser/native OBS,
deployment/package/license/release gates all remain. No media/font was downloaded,
hashed, parsed, decoded, loaded or played, and no support/performance was measured.
