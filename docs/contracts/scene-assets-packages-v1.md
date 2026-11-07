# Scene assets, fonts and portable packages — proposal v1

CONTRACT-011 FIRST logical document stage, from main `8fc9edc` and claim `1cacc34d`.
Authority: specification v0.45 §11/MEDIA-01/03/04/05/06/UI-04,
[task](../tasks/CONTRACT-011.md),
[ADR0005](../decisions/0005-scene-contract-prerequisites.md), approved
[scene model](scene-document-v1.md) blob `9a350733` and
[common values](wire-primitives-v1.md) blob `11eeb5f4`.
Proposed definition identity: `subathon.scene-assets-packages.v1`.

**CORRECTED PROPOSAL, 2026-11-07, awaiting independent re-review:** §2/4
supply finite media/font records and §3 the closed static SVG admission profile;
§8 adds complete literal logical descriptors and source cases. Historical
[R1/R2](../evidence/contract-011-review.md) remains CHANGES REQUIRED until the
actual revised packet receives different review. These are proposed policy
requirements, not measured codec/platform support or authorization to start 012.
See [author evidence](../evidence/contract-011-author.md) for preserved receipts.

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
entries, including SVG, and §4 defines two font entries. All require independent
review before the complete 011 profile gate can pass. An unrecognized
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

GIF repeat(n) retains the actual positive NETSCAPE2.0 loop-count integer; once
means no repeat extension, forever is its zero-count encoding. This is source
metadata, not a new scene autoplay/retrigger policy. Its finite
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
| `MPEG1Layer3(rate,mode,rateMode,kbps,samples,priming,padding)` | rate 32000/44100/48000; mode mono/stereo/joint-stereo (latter two ordered L/R); rateMode `cbr` or `vbr`; kbps sorted distinct nonempty subset of {32,40,48,56,64,80,96,112,128,160,192,224,256,320}, singleton for cbr; priming/padding each <=65535; positive samples after valid documented trim (known absence is 0). No free-format, MPEG2/2.5, layers I/II or unexplained sync loss |
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

## 3. Closed restricted static SVG profile

`subathon.asset.svg-static.v1` selects the following COMPLETE source-admission
vocabulary. This is an application subset of SVG, not all conforming SVG and not
Qt Tiny12. Unsupported source rejects; no best-effort ignored property, text
outlining, animation disabling, or sanitizer makes original bytes pass. Shapes,
full path families, groups, fills/gradients, strokes, transforms and typed internal
references remain available. Actual validation and both renderers remain unbuilt.

### 3.1 XML, names, tree and attributes

Source is UTF-8 XML 1.0, optional UTF-8 BOM and XML declaration (version 1.0,
encoding absent or UTF-8, standalone absent or yes). Reject other encodings,
DOCTYPE/DTD/entity declarations, processing instructions, CDATA, comments and
non-whitespace character content. Only XML whitespace (space/tab/CR/LF) may occur
between elements. The five predefined XML escapes and valid XML 1.0 numeric
character references are allowed inside quoted attribute values; validate the
DECODED value against the same grammar/bounds. No external entity or URL resolver
is called. Malformed XML, duplicate expanded attribute names and trailing second
roots reject before renderer loading.

Exactly one unprefixed `svg` root declares `xmlns="http://www.w3.org/2000/svg"`.
Every element is unprefixed in that namespace; no nested namespace redeclaration.
The root may additionally declare exactly
`xmlns:xlink="http://www.w3.org/1999/xlink"`, only to use `xlink:href` below.
All other namespace declarations, prefixed elements, namespaced attributes
(including xml:base/xml:space) and unknown attributes reject. Unqualified attribute
names are case-sensitive. `href` and `xlink:href` are alternatives, never both on
one element. Namespace attributes are included in the attribute count. `id`, where
present, matches ASCII `[A-Za-z_][A-Za-z0-9_.-]{0,127}`, is document-unique and is
never URL-decoded or Unicode-normalized. No opaque scene ID is forced into this
separate source-local identifier grammar.

`G` below means the graphic attributes `id`, `transform`, the paint properties in
§3.3, and `style` restricted to those same properties. Except the root namespace
attributes, the table is the entire attribute/child vocabulary. Optional geometry
fields use the stated defaults; all other named geometry fields are required.
Omitted id/style/transform means no id/no declarations/identity transform.

| Element | Attributes in addition to the stated common set | Permitted children |
| --- | --- | --- |
| svg | G; required width,height,viewBox; optional preserveAspectRatio (default xMidYMid meet) | defs, g, shapes, use |
| defs | id only | g, shapes, use, linearGradient, radialGradient |
| g | G only | g, shapes, use |
| rect | G; x,y default 0; required width,height; rx,ry optional | none |
| circle | G; cx,cy default 0; required r | none |
| ellipse | G; cx,cy default 0; required rx,ry | none |
| line | G; x1,y1,x2,y2 default 0 | none |
| polyline, polygon | G; required points | none |
| path | G; required d | none |
| use | G; required href OR xlink:href; x,y default 0 | none |
| linearGradient | id; required x1,y1,x2,y2,gradientUnits,spreadMethod; optional gradientTransform and href OR xlink:href | stop only |
| radialGradient | id; required cx,cy,r,fx,fy,gradientUnits,spreadMethod; optional gradientTransform and href OR xlink:href | stop only |
| stop | id; required offset; stop-color,stop-opacity and style restricted to those two properties | none |

`shapes` abbreviates rect/circle/ellipse/line/polyline/polygon/path only. One root
`defs` is optional and must precede all graphical children. Gradients occur only
in defs. No nested svg/symbol/view/image/pattern/marker/mask/clipPath, text/textPath,
foreignObject/HTML, script, style element, animation/filter or event attribute is
admitted. No external image/font/style, data/file/HTTP URL, arbitrary CSS property,
class, conditional processing or implementation/native-object extension is admitted.

### 3.2 Source numbers, lengths, paths and transforms

The following ASCII grammar is after XML attribute decoding. `D=[0-9]`,
`W=[ \t\r\n]`. A number is
`[+-]?(D+(\.D*)?|\.D+)([eE][+-]?D+)?`. No hexadecimal, NaN, infinity,
locale decimal, CSS escape/comment, sign without digits or internal whitespace.
A token has at most 128 bytes; its explicit exponent absolute value is <=256.
Interpret its decimal value exactly, including exponent and negative zero (zero
value). Reduced numerator/denominator each have <=4096 bits. This source grammar
intentionally admits `+1`, `.5`, `1.`, `1e2`, unlike canonical scene geometry.

Every source number's absolute value is <=1048576 (2^20). A length is a number
optionally followed immediately by `px`, with equal user-unit value; no em/rem,
physical unit, viewport unit, auto or calc. Percent is allowed ONLY on stop offset,
where N% means N/100. Root width/height are positive lengths. viewBox is four
unitless numbers (minX,minY,width,height), last two positive. preserveAspectRatio
is exactly `none`, or one of xMinYMin/xMidYMin/xMaxYMin/xMinYMid/xMidYMid/xMaxYMid/
xMinYMax/xMidYMax/xMaxYMax followed by exactly one space and `meet` or `slice`;
no defer. The root clips painting to its explicit viewport after this mapping.
Attribute values allow leading/trailing W unless stated exact literals.
List separator is W+ OR W* comma W*, never a trailing/doubled comma.

Shape widths/heights/radii and stroke widths are nonnegative; zero gives SVG's
empty geometry, not an invalid nonempty picture. rect rx/ry: both absent means 0;
one absent copies the other; clamp each to half its corresponding dimension as
SVG specifies (this is declared shape semantics, not source repair). points is an
even coordinate list with at least two pairs for polyline, three for polygon.
All points/path coordinates are unitless numbers, not lengths. Gradients use
`gradientUnits=userSpaceOnUse` (length coordinates) or `objectBoundingBox`
(unitless coordinates); no coordinate percentages. spreadMethod is
pad/reflect/repeat. Radial r is positive and the focus satisfies
(fx-cx)^2+(fy-cy)^2<=r^2; out-of-circle source needs a different profile rather
than an implementation-specific focal adjustment. A gradient applied to a zero
width/height object bounding box paints nothing, consistently with SVG semantics.

Path uses the SVG 2 path-data grammar for M/m,L/l,H/h,V/v,C/c,S/s,Q/q,T/t,A/a,Z/z,
with this number/separator grammar and exact one-character arc flags 0 or 1.
Successive numeric arguments require an explicit list separator even where
general SVG permits a sign/decimal to separate tokens; no such compact-number
exception is admitted here. Full grammar consumption is mandatory; first command M/m, repeated moveto pairs
become lineto, relative coordinates accumulate from current point, smooth controls
reflect the preceding compatible control (otherwise current point), Z closes to
subpath start. Arcs have nonnegative radii, rotation in degrees, SVG endpoint-to-
center/radius-correction semantics; zero radius is a line and equal endpoints
omit the arc. No truncation at the first bad segment. Counts include each moveto,
implicit lineto, explicit segment and closepath separately. At most 1048576 source
segments and 1048576 expanded segments, charged again on every use instance.

Transforms use the SVG 1.1 transform-list grammar, restricted to matrix(a,b,c,d,e,f),
translate(tx[,ty]), scale(sx[,sy]), rotate(angle[,cx,cy]), skewX(angle), skewY(angle).
Arguments are unitless numbers separated as above; lists are nonempty, at most 64
operations, separated by W+ or W* comma W*. No CSS transform units/3D or skew()
shortcut. Default ty=0, sy=sx, rotation center=(0,0); a rotation center requires both
cx and cy. Matrix maps (x,y) to (a*x+c*y+e,b*x+d*y+f). Written transform matrices
multiply left to right; nested transforms compose parent*child; use contributes
its transform*translate(x,y) before the referenced subtree's own transform.
Rotation is in degrees; skew angle is in [-80,80]. Singular graphical transforms
are admitted (may paint no area); gradientTransform must be invertible. These
choices do not grant a caller permission to execute general CSS transformations.

### 3.3 Paint, style and references

Closed graphic properties and defaults:

| Property | Allowed value; default when not supplied/inherited |
| --- | --- |
| fill, stroke | `none`, color, or exactly `url(#ID)`; defaults #000000 / none |
| fill-rule | nonzero or evenodd; nonzero |
| fill-opacity, stroke-opacity | number in [0,1]; 1 |
| opacity | number in [0,1]; 1; group compositing, not inherited |
| stroke-width | nonnegative length; 1 |
| stroke-linecap | butt, round, square; butt |
| stroke-linejoin | miter, round, bevel; miter |
| stroke-miterlimit | number in [1,1048576]; 4 |
| stroke-dasharray | none, or 1..1024 nonnegative lengths, not all zero; none |
| stroke-dashoffset | signed length; 0 |
| stop-color (stop only) | color; #000000 |
| stop-opacity (stop only) | number in [0,1]; 1 |

Color is exactly #RGB or #RRGGBB, with ASCII hex digits in either case, interpreted
as sRGB. No named colors/currentColor/ICC/CSS function/alpha hex. Case-sensitive
keywords are exactly those printed. Paint fragment syntax contains the §3.1 ID,
no quotes/whitespace inside url(), fallback paint, escape or percent decoding.
Stops do not accept gradient paint. Stop offsets are numbers in [0,1] or
percentages in [0,100]; there are 2..4096 effective stops, nondecreasing, with
first=0 and last=1. Equal offsets retain source order and SVG discontinuity rules.
No clamping/reordering of invalid stop offsets into acceptance.

`style` is a nonempty list of `property W* : W* value` separated by semicolons,
with optional final semicolon and surrounding W. Property is allowed for that
element by the table; value has exactly its grammar above. Reject duplicate style
properties, or a property present in BOTH style and presentation attributes;
no cascade ambiguity/last-wins. Reject !important, selectors, at-rules, comments,
escapes, var(), inherit/initial/unset/revert and all unlisted properties. Geometry,
transform, id and href cannot occur in style. Graphic properties except opacity
inherit along SVG graphic parentage; root uses defaults. Stop properties use their
own defaults (gradient containers have no inherited style in this profile).
Use shadow instances inherit graphic properties from the use element, then apply
the referenced subtree's explicit declarations; original source ancestors of the
referenced root do not inject a second inherited style. Required group opacity,
paint order (fill then stroke) and odd dash-list duplication follow SVG semantics.

`use` targets a g, shape or use by exact document-local ID. It cannot target svg,
defs, stop or gradient. A paint URL targets linearGradient or radialGradient.
Gradient href targets the SAME gradient kind and supplies stops ONLY: all geometry,
units/spread fields are still explicit on the referencing gradient; an absent
transform means identity. With own stops, use own stops; otherwise inherit the
nearest referenced effective stop list. No eventual stops is invalid. This is the
restricted SVG template case, not general cross-kind/template style inheritance.

All original source nodes, even unused defs, must validate. Resolve every reference;
reject missing/wrong-kind targets and cycles. Check the directed expansion graph
including containment and reference/template edges, not only href-to-href edges:
a group's descendant use referring to that group is a cycle. Each syntactic href
or paint URL is one source reference occurrence. Each element visited in a use
instance is one expandedUses occurrence (nested instances charge repeatedly);
gradient-template/paint walks and effective stops also charge repeated work.
No unique-node/digest discount hides exponential expansion.

### 3.4 Finite semantic and intermediate admission

These inclusive bounds add to §7. One decoded attribute <=1048576 UTF-8 bytes;
aggregate decoded attributes <=16777216; numeric tokens <=4194304; all source
attributes and XML depth/elements still obey §7. No unbounded entity expansion,
stylesheet, string body, list, transform or implicit path expansion is permitted.
There is no free unknown attribute/body to be delegated to a native loader.

Before numerical growth, charge logical arithmetic operations and bound exact
rational intermediate numerator/denominator to 8192 bits each, reduced result to
4096 bits. Charge operations/visits against the 16777216 work ceiling, including
source decoding, implicit path points, reflected controls, use/gradient repeats,
compositing surfaces, outline/stroke generation and flattening. Per-inspection
generated edges/vertices are <=1048576 each; generated gradient stops <=1048576;
owned buffer/scratch/surface bytes remain within §7's aggregate. Streaming does
not waive aggregate simultaneous ownership or cumulative work.

Use conservative finite geometry envelopes for admission, not machine overflow or
renderer-dependent clipping. All resolved path endpoints/control points must have
absolute coordinates <=2^20. For a nondegenerate arc from P0 to P1 with positive
rx,ry, let R=max(rx,ry), r=min(rx,ry), and delta=|x1-x0|+|y1-y0|. Charge the scalar
envelope max(|P0.x|,|P0.y|,|P1.x|,|P1.y|)+4*(R+delta*R/r), which must be <=2^30.
It conservatively covers rotation and SVG radius enlargement; zero/equal-endpoint
cases follow §3.2 without division. Other geometry uses the convex hull of control
points or shape extents. For a painted stroke, expansion adds stroke-width*(miterlimit+2) before
transform; stroke=none adds zero. This intentionally conservative inspection
bound is not an exact curve bounding-box or a new scene geometry limit.

For matrix envelopes use absolute source matrix coefficients; translate/scale
have exact absolute envelopes. Rotation linear coefficients are bounded by 1,
center translation by 2*(|cx|+|cy|); skew off-diagonal by 6 and diagonal by 1.
Compose nonnegative envelopes by ordinary matrix multiplication, including viewBox
and objectBoundingBox mappings. Each envelope coefficient and transformed geometry/
stroke coordinate bound must be <=2^40. Derived gradient inverse coefficients
also have absolute value <=2^40. Singular graphical transforms are never inverted;
zero bounding boxes paint nothing. Root viewport/viewBox/aspect mapping uses the
explicit dimensions and SVG meet/slice/none semantics; it is charged too.
All computations, including irrational arc/rotation/inverse quantities, require
bounded finite evaluation, no NaN/infinite intermediates. A later checker may use
certified outward bounds; inability to establish an inclusive bound within its
finite work/precision budget is unavailable, never an assumed pass. No approximate
native validity result decides an uncertain boundary. Thus admission ceilings,
semantic operation ordering and required proof are defined here; numeric tooling/
representations/check ordering remain separate implementation review, not policy
left for 012 to invent.

Raw bytes and width/height do not establish rasterized memory or safe loading.
Project the intended output/cache/compositing demand before renderer handoff; if
actual containment cannot enforce work/time/memory, inspection is unavailable.
This chooses no XML/SVG library, renderer flag, tessellator or process supervisor.

## 4. Fonts, fallback, rights and Noto provenance

Retain uploaded TTF/OTF and an explicitly configured fallback. A FontRef resolves
to exact font bytes/revision plus an explicit face/style/weight selection; variable
axes or collection/embedded-feature claims need their own precise profile and actual
compatibility evidence. A filename/family string or an operating-system match does
not establish the intended face. A failure cannot silently substitute the host font
while claiming the original face succeeded. No claim of universal glyph coverage
follows from the family name.

### 4.1 Closed SFNT font profiles

Two proposed identifiers retain uploaded TrueType and CFF OpenType families:
`subathon.asset.font-ttf.v1` and `subathon.asset.font-otf-cff.v1`. These are
admission subsets, not a statement that every TTF/OTF or any downloaded Noto file
fits. Nonadmitted valid features require a reviewed sufficient 011 profile; do
not remove the upload/default-font requirement or silently strip tables to pass.
The filename extension never chooses the outline interpreter.

The closed logical descriptor is
`Font(profile,encodedBytes,SFNT(scaler,face,tables,unitsPerEm,glyphs,weight,width,slant,cmap,outline,hints,layout,variation,color,embedded))`.
encodedBytes is positive U matching §1/6. face is exactly 0 (one SFNT face);
scaler is `00010000` for TTF or `OTTO` for CFF. No TTC/OTC/WOFF/WOFF2, Apple
`true`/`typ1`, collection face selection, or compressed wrapper in these versions.
tables is the ASCII-tag-sorted unique list of `Table(tag,version)` records below;
`none` means that table format has no version field, not uninspected. Validate
actual directory offsets/lengths/alignment/checksums (including head adjustment),
no overlapping tables/duplicate tags/hidden trailing payload except zero alignment
padding, all normative reserved fields and exact format lengths. An unrecognized
or forbidden table is incompatible/forbidden as appropriate, never ignored.

| Tables | Finite versions and combinations |
| --- | --- |
| Both profiles: required OS/2,cmap,head,hhea,hmtx,maxp,name,post | OS/2 versions 0..5; cmap 0; head,hhea 1.0; hmtx none; name 0 or 1; post 2.0/3.0 for TTF, 3.0 for CFF; maxp 1.0 for TTF, 0.5 for CFF |
| TTF only: required glyf,loca | versions none; head indexToLocFormat 0 or 1; no CFF/CFF2 table |
| TTF only: optional cvt ,fpgm,prep,gasp | cvt /fpgm/prep none; gasp 0 or 1. The cvt tag includes its fourth ASCII space. Hint-mode combinations below are mandatory |
| CFF only: required CFF  | version 1.0; tag ends with ASCII space; one name-keyed font, CharstringType 2; no CID FontDict/FDSelect, glyf/loca, CFF2 or TrueType program tables |
| Both: optional GDEF,GSUB,GPOS,kern | GDEF 1.0/1.2/1.3; GSUB,GPOS 1.0/1.1; feature-variation/item-variation store offsets must be zero. kern version 0, horizontal format-0 pairs only, no cross-stream/minimum tables |

This table is CLOSED: absent optional tables are not synthesized. In particular
fvar/avar/gvar/HVAR/VVAR/MVAR/STAT/CFF2 (variable), COLR/CPAL/SVG /CBDT/CBLC/sbix/
EBDT/EBLC/EBSC (color/bitmap/embedded graphics), AAT tables, BASE/JSTF/MATH/vhea/
vmtx/VORG (other layout), DSIG/meta and all other tags are outside these profiles.
Their presence cannot be excused as inert name metadata. variation/color/embedded
are each exactly `none`; no hidden axis, embedded SVG or alternate bitmap is loaded.
No variable instance or synthesized weight is represented by an invented static face.

unitsPerEm is 16..16384; glyphs 1..65535 including glyph 0; weight 1..1000 matches
OS/2 weight, width 1..9 matches OS/2 width; slant `upright`/`italic`/`oblique`
matches actual head/OS/2/post style declarations, with inconsistent flags rejected.
All names/metrics/bounds and horizontal advance/side-bearing counts must be checked,
not merely header-copied. name strings are bounded inert data, decoded using their
normative encoding; unsupported encodings are incompatible, never host-decoded.
Allow Unicode platform 0 encodings 0..6 and Windows platform 3 encodings 0,1,10
(UTF-16BE); Macintosh platform 1 encoding 0 (Mac Roman); other name encodings need
another profile. Check UTF-16/scalar and name-format-1 language-tag validity;
no name/URL/license string executes or fetches content. Their notices/rights still
need separate actual evidence and are not established by name-table claims.

cmap is a sorted unique nonempty list of `UnicodeCmap(platform,encoding,format)`:
platform 0 encoding 3 with format 4, or encoding 4 with format 12; platform 3
encoding 1 with format 4, or encoding 10 with format 12. No symbol/non-Unicode,
variation-selector or alternative cmap format. Validate every segment/group,
Unicode scalar range (no surrogates), glyph ID, offset and ordering; overlapping
subtables must agree for a shared scalar. Coverage is the actually inspected map,
not an assertion that every text character is present. Selected text/fallback
coverage still requires actual renderer checks.

outline is one of:

- TTF: `Glyf(locaFormat,maxContours,maxPoints,maxComponents,maxDepth)`, with
  locaFormat 0/1 matching head, maxima over complete expanded glyphs;
  maxContours<=65535, maxPoints<=1048576, maxComponents<=65535, maxDepth<=64.
  Validate simple contours/flags/coordinate deltas and composite glyph IDs,
  argument/transform modes/point attachment/metrics flags. Composite dependencies
  are complete and acyclic, every expansion is charged, all resolved coordinates
  have absolute value <=1048576, bounding boxes and maxp maxima are consistent.
- CFF: `CFF1(maxStack,maxSubrDepth,maxStems,maxOps)`, with operand maxStack<=48,
  maxSubrDepth<=10, maxStems<=96 and maxOps<=1048576 for each expanded glyph.
  Validate every INDEX/DICT/charset/encoding/private dictionary/CharStrings and
  biased subroutine index. Local/global call graphs must be acyclic; charge all
  repeated calls, operands, mask bytes, emitted points and stems. Defined Type 2
  drawing/move/flex, stem/mask, call/return/endchar and arithmetic/stack operators
  from Adobe #5177 are admitted EXCEPT random and deprecated endchar seac arguments.
  Reserved operators/Type 1/PostScript programs are forbidden; no generic PostScript
  interpreter is selected. Division by zero, invalid sqrt/stack/transient access,
  incorrect hint masks or nonfinite arithmetic reject. Arithmetic intermediate
  absolute values <=2147483647, resolved outline coordinates <=1048576.

hints is `none`, `type2-stems`, or
`TT(programBytes,cvtEntries,maxStack,maxStorage,maxTwilight,maxFunctions)`.
TTF `none` requires no cvt /fpgm/prep, zero glyph instruction lengths and zero
maxSizeOfInstructions/maxStackElements/maxStorage/maxFunctionDefs/
maxInstructionDefs/maxTwilightPoints (maxZones remains its valid 1 or 2); gasp may still supply its checked finite flags.
TTF TT requires actual programBytes>0 (aggregate fpgm/prep/glyph programs <=1048576),
cvtEntries<=65535, maxStack/maxStorage/maxTwilight/maxFunctions each <=65535 and
consistent actual/maxp counts. Only instruction opcodes in hex ranges
00..27,29..4e,50..7a,7c..7d,80..82,85..88,8a..8e,b0..ff are admitted at instruction
boundaries (push operand bytes are data). This excludes reserved/variation opcodes,
DEBUG, IDEF and obsolete SANGW/AA; no dynamically defined opcode extension.
Validate push widths, definition/jump boundaries, function/cvt/storage/point
references and stack state under the defined TrueType instruction semantics.
A static source-byte scan does not prove dynamic safety or termination. Actual
interpreter use needs enforceable limits: stack/storage/twilight as declared,
call depth<=64, <=1048576 executed instructions per glyph load and <=16777216 per
bounded inspection/render job, including fpgm/prep/loop calls. Unknown/uncontrolled
native execution is unavailable, never a reason to run first and infer safety.
This states required containment, not an implementation/selection of a supervisor.
CFF hints is none when no stem/mask operators occur, otherwise type2-stems; no TT
hint mode with CFF, no type2-stems mode with TTF. Hint execution differences remain
actual dual-renderer layout/raster evidence, not an automatic compatible result.

layout is `Layout(gdef,gsub,gpos,kern)`: gdef is none/1.0/1.2/1.3 matching the table;
gsub/gpos are sorted distinct lists of the actual lookup types present (including
extension targets), empty iff their table is absent; kern is none or `horizontal0`
matching the table. Admit GSUB types 1..8, GPOS 1..9 and their defined subformats
in OpenType 1.9.1, without variation data. All script/language/feature/coverage/
class/anchor/lookup indices and glyph references validate. Extension lookups target
non-extension types, nested contextual invocation graphs are acyclic with depth<=16;
charge every repeated lookup/application, not just distinct tables. One table's
record count<=1048576, aggregate glyph/lookup/component/charstring/arithmetic visits
<=16777216. Table presence alone is neither full language support nor a permission
to guess shaping/fallback settings. Actual shaping inputs belong to 009/010/renderer
contracts, with same bytes/selection and measured conformance below.

Across both: each table body<=33554432 bytes AND within actual encoded cap; aggregate
decoded name strings<=1048576 UTF-8 bytes; per-name<=65536; per-glyph emitted points
<=1048576, aggregate emitted points/components<=16777216; mathematical numeric
intermediates use checked <=4096-bit numerator/denominator (8192-bit transient)
and finite native demand. §7 memory/work/time applies simultaneously. File/table/
maxp declarations do not prove actual counts, all-code interpreter safety, cache
capacity or permission. Inspect all glyphs/programs/layout, not a name or sample.
If full inspection/execution cannot be bounded in the admitted environment, it is
unavailable; never advertise the font as compatible. Actual parser/tooling/API and
resource/platform verification remain later reviewed work; the finite admission
sets/requirements above are 011-owned.

### 4.2 Same assets, configured fallback and preserved notices

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
every opaque codec/hint-interpreter CPU instruction. The source/semantic/count/
bit/container limits and repeated-work charging are defined by §2/3/4 and this table. Concrete parser field walks, error ordering,
byte encodings, ownership interfaces and enforceable allocator/interpreter controls
need separate reviewed tooling before an executable baseline; 012 represents the
reviewed policy rather than choosing missing profile content. This is not proof of
a safe native allocation or a complete executable parser allocation contract.
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

### 8.1 Complete media descriptors and one-field counterexamples

The following are LITERAL closed logical descriptor values, one for every media
registry entry; each profile string is fully specified. They are hypothetical
INSPECTED metadata with known encoded lengths under the initial caps. Except S01's
literal source-byte length, lengths are premises, not measured payload facts.
Additional complete-content, integrity, rights and checks from §1..7 are separately
assumed for the logical profile-matching exercise. A value fitting the vocabulary
is not proof that corresponding bytes exist, decode correctly or play on any output.
Compatibility observations for ALL these examples remain UNKNOWN. There is no
portable-success fixture or source-generated expected result.

```text
MD01 = Media(subathon.asset.png.v1,4096,
  PNG(2,1,6,8,0,channel,untagged-srgb))
MD02 = Media(subathon.asset.jpeg.v1,4096,
  JPEG(16,16,baseline,8,gray,1x1,1,untagged-srgb))
MD03 = Media(subathon.asset.webp.v1,4096,
  WebP(2,1,vp8l-lossless,true,untagged-srgb))
MD04 = Media(subathon.asset.gif.v1,512,
  GIF(89a,1,1,2,0,forever,[Frame(0,0,1,1,2,none,1,2,false),
                          Frame(0,0,1,1,2,0,2,2,false)]))
MD05 = Media(subathon.asset.svg-static.v1,152,
  SVG(2,2,[0,0,2,2],none,2,7,0,0,4))
MD06 = Media(subathon.asset.mp4.v1,65536,
  MP4(isom,[isom,avc1],
    Video(AVC(6400,2a),1920,1080,Q(60,1),60,bt709-limited,false,Q(1,1),0,4),
    AAC(48000,stereo,47104,0,0)))
MD07 = Media(subathon.asset.webm.v1,65536,
  WebM(4,Video(VP9(0),1280,720,Q(30,1),30,bt709-limited,false,Q(1,1),0,3),
    Opus(48000,stereo,0,312,0,48000)))
MD08 = Media(subathon.asset.wav.v1,192044,
  WAVE(PCM(48000,stereo,16,48000)))
MD09 = Media(subathon.asset.mp3.v1,20000,
  MP3(MPEG1Layer3(44100,stereo,cbr,[128],44100,576,1404)))
MD10 = Media(subathon.asset.ogg.v1,8192,
  Ogg(0,7,false,Vorbis(48000,stereo,64,256,48000)))
```

MD05 width/height/viewBox values are exact rationals written as small integers;
its element/attribute/reference/expanded-use/source-segment counts are explicitly
2/7/0/0/4 from S01. SVG width/height are vector viewport dimensions, not claimed
raster decode dimensions. For all rows an uninspected field would instead make
inspection incomplete; it cannot be filled with the value shown by assumption.

Each row below is the complete corresponding MD descriptor with ONLY the named
field replaced. The original fits its selected logical profile; the replacement
is independently incompatible, or a specified integrity failure. No codec is
invoked to derive either expected outcome.

| Original | One-field substitution | Hand-derived outcome |
| --- | --- | --- |
| MD01 | depth=4 | colorType 6 allows 8 or 16 only; incompatible even though type-3 depth 4 exists |
| MD02 | precision=12 | outside exact 8-bit profile; changing a filename would not help |
| MD03 | coding=vp8l-lossless-animated | not an admitted coding literal; source ANIM/ANMF also forbidden |
| MD04 | second frame delayCS=1 | below inclusive 2; no browser delay clamping into acceptance |
| MD05 | viewBox=[0,0,0,2] | zero viewBox width forbidden; replacing descriptor alone also contradicts S01's bytes |
| MD06 | AVC level=29 (4.1) | ceil(1920/16)*ceil(1080/16)=8160; 8160*60=489600>245760; level 2a's 522240 ceiling admits the original |
| MD07 | Video alpha=true | alpha is false only; container name cannot authorize alpha/codec substitution |
| MD08 | PCM bits=24 | outside {8,16}, also invalidates the preserved two-byte-per-channel alignment/byte count |
| MD09 | rate=22050 | outside MPEG1 Layer III's {32000,44100,48000}; no implicit switch to MPEG2 |
| MD10 | chain=true | explicitly outside the single-stream unchained Ogg profile |

The alternative admitted codec branches are also literal: replacing MD07's codec
ONLY by VP8(0) fits the video set under matching inspected bitstream premises;
replacing MD07's audio ONLY by Vorbis(48000,stereo,64,256,48000) fits; replacing
MD10's audio ONLY by Opus(48000,stereo,0,312,0,48000) fits. MD06 audio=none is
admitted, audio=PCM(48000,stereo,16,48000) is incompatible. These exercises do not
infer actual source bitstreams from a record substitution.

Independent finite boundaries: MD01 width=8192,height=3906 gives 31997952 pixels;
height=3907 gives 32006144>32000000. A GIF delay 2 means exactly Q(1,50) seconds;
65535 fits delayCS, 65536 does not. MD08 has blockAlign=4, byteRate=192000,
dataBytes=192000 and, under the simple 44-byte-header premise, encodedBytes=192044;
PCM rate=96000 is admitted, 96001 is not. MD09's 44100+576+1404=46080=40*1152
sample frames; audio duration is Q(1,1), not two seconds for stereo. MD06's AAC has
47104=46*1024 sample frames with zero trim and duration Q(368,375), shorter than
its one-second video (no edit list needed); MD07's Opus preSkip 65535 fits its
integer set and 65536 does not (actual trim/packet consistency still independently required).
Video cadence Q(240,1) fits the shared rate bound, Q(241,1) does not; codec level/
other checks may still reject either. An otherwise admitted image/font encoded
length 33554432 fits its initial cap, 33554433 does not; no valid bytes are implied.

### 8.2 Complete font descriptors

Both literals use exactly the versioned font profile's required tables and no
unlisted optional tables. Glyph/outline maxima and lack of instructions are
hypothetical complete-inspection premises, not a claim about any actual font.
Name/rights/coverage/metric checks still apply separately and actual compatibility
is UNKNOWN. These are not disguised Noto binary pins or universal coverage claims.

```text
FD01 = Font(subathon.asset.font-ttf.v1,16384,
  SFNT(00010000,0,
    [Table(OS/2,4),Table(cmap,0),Table(glyf,none),Table(head,1.0),
     Table(hhea,1.0),Table(hmtx,none),Table(loca,none),Table(maxp,1.0),
     Table(name,0),Table(post,3.0)],
    1000,256,400,5,upright,[UnicodeCmap(3,1,4)],
    Glyf(1,2,8,0,0),none,Layout(none,[],[],none),none,none,none))
FD02 = Font(subathon.asset.font-otf-cff.v1,16384,
  SFNT(OTTO,0,
    [Table("CFF ",1.0),Table(OS/2,4),Table(cmap,0),Table(head,1.0),
     Table(hhea,1.0),Table(hmtx,none),Table(maxp,0.5),Table(name,0),Table(post,3.0)],
    1000,256,700,5,upright,[UnicodeCmap(3,10,12)],
    CFF1(4,0,0,24),none,Layout(none,[],[],none),none,none,none))
```

FD01 face=1 is incompatible: this profile is one face/index 0, not a TTC selector.
FD01 unitsPerEm=16 or 16384 fits the field; 15 or 16385 is incompatible. Changing
only cmap to [UnicodeCmap(3,1,12)] is incompatible: that encoding/format pairing is
not admitted. Changing only variation to `wght` or color to `COLR` is incompatible;
adding the corresponding table cannot hide it. Changing only hints to type2-stems
is incompatible for TTF. FD02 hints=type2-stems would require actual stem/mask
operators and a matching nonzero maxStems; its unchanged CFF1 maxStems=0 therefore
contradicts that declaration. FD02 maxSubrDepth=10 fits the depth field; 11 does
not. A scalar glyph mapping to glyph 256 in either 256-glyph font is invalid;
255 fits the index range only, not automatically coverage for a requested scalar.

A fully specified hinted variant of FD01 replaces tables by
[Table(OS/2,4),Table(cmap,0),Table("cvt ",none),Table(fpgm,none),Table(glyf,none),
Table(head,1.0),Table(hhea,1.0),Table(hmtx,none),Table(loca,none),Table(maxp,1.0),
Table(name,0),Table(post,3.0)] and hints by TT(3,1,1,0,0,0); ALL other fields are
exactly FD01. Premise: fpgm is the three instruction bytes B0 00 21 (push byte 0,
pop), cvt contains one valid value, no other glyph/prep programs, maxp fields match.
The program has bounded stack 1 and no calls/storage/twilight. Changing its final
opcode to 4F (DEBUG) is forbidden under the exact opcode set even though an
interpreter might accept it. Raising declared maxStack from 65535 to 65536 is a
one-over profile refusal, not a license to allocate first. No instruction or font
loader was executed to establish these symbolic examples.

### 8.3 Literal SVG source and independently decidable edges

S01 is the following SINGLE line, no BOM/declaration/final newline. Its ASCII
UTF-8 byte length is 152. Hand count: root has five attributes (including
xmlns), path two; two elements; M,L,L,Z are four segments; no references/use.
It satisfies the closed source grammar and all small integer bounds. This is
logical source admission only, not parser/dual-renderer/platform verification.

```xml
<svg xmlns="http://www.w3.org/2000/svg" width="2" height="2" viewBox="0 0 2 2" preserveAspectRatio="none"><path d="M0 0 L2 0 L0 2 Z" fill="#f00"/></svg>
```

S02 exercises typed use, a gradient template and restricted style. The five IDs
are unique; G references B for its stops, the rect in tile paints with G, and the
root use references tile. The expanded tile has two element visits (g,rect).
Both gradients have explicit geometry/units/spread and effective stops at 0/1.
SVG inheritance at the use instance supplies fill-rule; the rect's explicit fill
selects G. No reference target lies on its own containment/reference path.

```xml
<svg xmlns="http://www.w3.org/2000/svg" width="10px" height="10" viewBox="0 0 10 10">
  <defs>
    <linearGradient id="B" x1="0" y1="0" x2="1" y2="0" gradientUnits="objectBoundingBox" spreadMethod="pad">
      <stop id="s0" offset="0%" stop-color="#000"/>
      <stop id="s1" offset="100%" style="stop-color:#fff;stop-opacity:1"/>
    </linearGradient>
    <linearGradient id="G" x1="0" y1="0" x2="1" y2="0" gradientUnits="objectBoundingBox" spreadMethod="pad" href="#B"/>
    <g id="tile"><rect width="2" height="2" fill="url(#G)" stroke="#000" stroke-width=".5"/></g>
  </defs>
  <use href="#tile" x="1" y="1" style="fill-rule:evenodd;opacity:.5" transform="translate(1 0) rotate(45)"/>
</svg>
```

S03 replaces ONLY S01's path d with
`M0 0 h1 v1 l-1 0 c0 0 1 0 1 1 s0 0 0 0 q0 0 1 0 t0 0 a1 1 0 0 1 1 0 z`.
It exercises all path families with finite coordinates, isolated flags and exact
consumption. Resolve current points after h/v/l: (1,0),(1,1),(0,1); after c/s/q/t/a:
(1,2),(1,2),(2,2),(2,2),(3,2); z returns to (0,0). The arc has rx=ry=1,
delta=1 and envelope 3+4*(1+1)=11<2^30. Its source count is 10. Crossing the
2-unit viewBox is permitted clipping, not a reason to discard the asset. `l-1`
is the command letter followed by one signed number, not an omitted numeric
separator; repeated number arguments still need §3.2's separators.

S04 supplies the remaining basic shapes, radial paint and each transform family.
It uses six transform operations, positive finite geometry and a centered unit-box
radial gradient. The radii/focus satisfy the declared circle bound; its small
conservative matrix/shape envelopes fit §3.4 independently of raster output.

```xml
<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 32 32">
  <defs>
    <radialGradient id="R" cx=".5" cy=".5" r=".5" fx=".5" fy=".5" gradientUnits="objectBoundingBox" spreadMethod="pad">
      <stop offset="0" stop-color="#fff"/><stop offset="1" stop-color="#000"/>
    </radialGradient>
  </defs>
  <g fill="url(#R)" stroke="#000" stroke-width="1" transform="matrix(1 0 0 1 0 0) translate(1 1) scale(.5) rotate(30 0 0) skewX(10) skewY(-10)">
    <rect x="0" y="0" width="4" height="4" rx="1"/>
    <circle cx="4" cy="4" r="2"/><ellipse cx="10" cy="4" rx="3" ry="2"/>
    <line x1="0" y1="8" x2="6" y2="8"/>
    <polyline points="0 12 4 8 8 12"/><polygon points="12 12 20 12 20 16"/>
  </g>
</svg>
```

| Independent source change / literal condition | Required result, by the selected grammar/bounds |
| --- | --- |
| S01 width="+2e0px", or path coordinates `.0`, `2.` in place of equivalent integers | admitted equivalent source numbers; same byte length is NOT assumed |
| S01 width="2em", width="NaN", viewBox="0 0 0 2", or d="M0 0 L1" | reject unsupported unit/non-number/nonpositive viewBox/incomplete path respectively |
| S01 add onload="x", xml:base="file:/", style="filter:url(#f)", or a text/filter/animate child | forbidden source; never rely on native ignoring or disabled execution |
| S01 replace SVG namespace by an HTTPS spelling or prefix the path name | reject exact namespace/element profile mismatch |
| S01 path has fill="#f00" AND style="fill:#0f0"; or style="fill:#f00;fill:#0f0" | reject duplicate presentation/style or style property; no cascade winner |
| S02 rect fill="url(#tile)" or use href="#G" | reject wrong-kind paint/use target respectively |
| S02 G href="#missing" or G href="#G" | reject dangling target/self-cycle even if G supplied its own stops |
| In S02 tile contains use href="#tile" | containment+reference cycle; reject before expansion |
| S02 use href="data:image/svg+xml,...", href="other.svg#tile", or href="#ti%6ce" | reject outside exact same-document fragment/ID grammar; no fetch/percent normalization |
| S02 add matching xmlns:xlink on root and replace href by xlink:href everywhere | admitted alternative spelling; supplying both on one node rejects |
| S02 second stop offset="101%" or first=".1" | reject range or required endpoint; no clamp/sort |
| S04 radialGradient fx="2" only | reject: (2-.5)^2=2.25>.5^2=.25; no focal clamping |
| S01 path d="M1048576 0 l0 1" versus d="M1048576 0 l1 0" | first fits resolved endpoint bound, second reaches 1048577 and rejects although each individual source token fits |
| Source numeric `1e-256` versus `1e-257`; `1e256` | first fits exponent/value/bit limits; second fails exponent; third fails value limit despite valid exponent/token grammar |
| translate(1048576 0) repeated 64 times versus 65 in one transform list | first fits list/coefficient bounds (sum=67108864); second fails operation count |
| Nested scale(1048576) twice versus three times, on a unit line | envelope coefficient=2^40 fits versus 2^60 exceeds; do not overflow/clip into acceptance |
| A 1048576-coordinate line with scale(1048576) and stroke="none" versus adding positive stroke width 1 | unexpanded coordinate envelope reaches 2^40; positive stroke expansion exceeds it and rejects |
| Number token of 128 bytes versus 129, otherwise zero-valued; 65536 elements versus 65537 | first of each pair fits its own inclusive limit; second refuses; other grammar/counts still apply |
| Acyclic binary use tree with 20 versus 21 levels, including its root instance | 2^20-1=1048575 fits expanded-use ceiling; 2^21-1=2097151 exceeds; compact unique nodes do not change repeated counts |

These are hand-derived profile decisions over explicit literals/premises. Expanded
work/bit/memory/native containment and every other bound still apply together;
a source fitting one isolated boundary is not a complete successful inspection.
They do not execute XML/font/media parsers, test fixtures or native renderers.

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
- The OpenType 1.9.1 [SFNT structure/table catalogue](https://learn.microsoft.com/en-us/typography/opentype/spec/otff),
  [glyf](https://learn.microsoft.com/en-us/typography/opentype/spec/glyf),
  [maxp](https://learn.microsoft.com/en-us/typography/opentype/spec/maxp),
  [name](https://learn.microsoft.com/en-us/typography/opentype/spec/name) and
  [cmap](https://learn.microsoft.com/en-us/typography/opentype/spec/cmap) distinguish
  outlines, directory/face metadata and character mapping. The
  [TrueType instruction set](https://learn.microsoft.com/en-us/typography/opentype/spec/tt_instructions)
  defines bytecode, while [CFF](https://learn.microsoft.com/en-us/typography/opentype/spec/cff)
  references Adobe [Type 2 #5177](https://adobe-type-tools.github.io/font-tech-notes/pdfs/5177.Type2.pdf).
  These explain why table/outline/program checks are distinct. §4's selected table/
  version/operator subsets, finite counts and exclusions are application proposals,
  not guarantees by Microsoft/Adobe or proof of safe native interpreter control.
- SVG [path semantics](https://www.w3.org/TR/SVG2/paths.html),
  [SVG 1.1 transform-list semantics](https://www.w3.org/TR/SVG11/coords.html#TransformAttribute),
  [coordinates](https://www.w3.org/TR/SVG2/coords.html),
  [paint](https://www.w3.org/TR/SVG2/painting.html),
  [gradients](https://www.w3.org/TR/SVG2/pservers.html) and
  [use instances](https://www.w3.org/TR/SVG2/struct.html#UseElement) supply the
  underlying feature meanings. §3 deliberately restricts spellings, units, tree,
  styles and references, and adds conservative finite admission envelopes. General
  SVG's error recovery or ignored CSS is not this application's reject behavior.
- [QFontDatabase](https://doc.qt.io/qt-6.12/qfontdatabase.html) loads TrueType/OpenType
  and offers application fallbacks; [CSS Font Loading](https://www.w3.org/TR/css-font-loading-3/)
  specifies browser loading. Neither a family match nor source/license metadata
  establishes actual same-font renderer behavior.

No QtBase/Core installation proves Multimedia/FFmpeg/SVG/plugin/font behavior.
Before 012 require actual independent review of these closed media/font/SVG
profile definitions, descriptors, examples and finite admission budgets. 012 owns
complete encoding/schema/import representation. Concrete entry framing/checker
interfaces, field walks/error priority, bounded ownership and pinned tooling need
separate actual review before executable consumption; the schema owner does not
supply unfinished 011 policy. Then require independent literal fixtures/tests,
different review and valid intended red before production SCENE-005/006/RENDER-004,
with separate actual source/configuration review and passing acceptance/CI.
No missing dependency/compile failure/skip is a valid red.

010 semantic closure, 008/003 integration, independent renderer/reference scenes,
SVG rejection, font/Noto/OFL/notices, configured budgets/demand warnings, 60/30 FPS,
SPIKE-004 codec/OBS minimums and actual Windows/Linux/Wayland/browser/native OBS,
deployment/package/license/release gates all remain. No media/font was downloaded,
hashed, parsed, decoded, loaded or played, and no support/performance was measured.
