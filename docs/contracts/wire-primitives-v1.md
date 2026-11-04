# Common wire values and compatibility — proposal v1

CONTRACT-005 first document stage, from main `dd6a141` and claim `c1e9859`.
Authority: specification v0.45 §§2/3.5/10, API-01/02/03 and NUM-04;
[CONTRACT-005](../tasks/CONTRACT-005.md),
[ADR-0004](../decisions/0004-wire-contract-prerequisites.md),
[record boundaries](event-state-boundaries-v1.md) and
[exact decimals](exact-decimal-v1.md). Proposed common definition identity:
`subathon.wire-primitives.v1`. This is a data-representation proposal, not an
available API version or implemented capability.

Different actual document review precedes concrete schemas/APIs, independent
examples/tests, decoder/encoder interfaces and implementation. No schema, codec,
generated client, endpoint, WebSocket protocol or runtime behavior is supplied.
CONTRACT-003 still requires common, HTTP006, persistent-WS007 and scene008 contracts.

## 1. Type selection and exact values

An owning field profile names its common primitive, units/meaning, allowed domain,
presence/nullability and finite resource limits. Consumers use that declared type;
they never guess a type from a property name or string contents. Ordinary bounded
UI numbers may use JSON numbers only where their owning profile explicitly allows
them. Exact accounting values, signed/unsigned64 and rational components use JSON
STRINGS. No binary floating-point intermediate, automatic numeric coercion,
saturation, truncation or locale conversion participates.

Grammar below applies to the decoded JSON string value, not its JSON escape
spelling. Equivalent JSON escapes/structural whitespace do not change that value.
Exact original source bytes inside that value remain data. This document does not
define a canonical serialization of every JSON object or an operation fingerprint.

| Primitive | Representation and mathematical meaning |
| --- | --- |
| Signed64 | Canonical ASCII integer string; range −9223372036854775808..9223372036854775807 |
| Unsigned64 | Canonical nonnegative ASCII integer string; range 0..18446744073709551615 |
| Integer | Canonical signed ASCII integer string with the owning field's bounded arbitrary-precision domain |
| PositiveInteger | Canonical strictly positive ASCII integer string, with the owning field's bounds |
| DecimalSource | An owned original ASCII decimal spelling under its explicitly selected source-number profile |
| FiniteDecimal | Canonical exact finite-decimal VALUE string, when an owning field requires a terminating decimal |
| Rational | Object with exactly `numerator` Integer and `denominator` PositiveInteger strings; reduced and denominator positive |
| OpaqueBytes | Lowercase hexadecimal JSON string representing exact bytes, two characters per byte |
| Text | JSON Unicode scalar string, without normalization or interpretation as markup |

Canonical Integer is `0` or `-?[1-9][0-9]*`; Signed64 uses that grammar plus its
range. Unsigned64 is `0` or `[1-9][0-9]*`; PositiveInteger excludes zero. Reject
leading `+`, leading zeros, `-0`, whitespace, separators, decimal points, exponent,
NUL, junk and JSON number/bool/null substitutions. Signed and unsigned are distinct
types; a negative value is not reinterpreted by casting. Valid opposite signed
extremes can have an Unsigned64 distance of 18446744073709551615.

Rational has exactly two required keys. Reject missing/extra keys, nonstrings,
noncanonical components, zero/negative denominator and gcd(abs(numerator),denominator)
other than 1. Zero is exactly `{"numerator":"0","denominator":"1"}`. Negative
numerators are allowed generally; positive-rate or nonnegative-factor restrictions
belong to the owning field. Reject `10/12`; do not repair an untrusted canonical
value. Original rational expressions, when retained as source evidence, are
separate byte/text data rather than this canonical value representation.

## 2. Original spelling and derived-value bounds

DecimalSource retains every source spelling byte, including trailing zeros,
exponent case/sign/leading exponent zeros and signed zero. Equal numerical values
do not make `1.20`, `1.2000` and `1.2e+00` interchangeable source evidence. Neither
canonical value output nor display rounding replaces the original spelling,
currency, captured rate/source or historical accounting record.

The existing CORE-001 source profile accepts complete ASCII JSON-number grammar
`-?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?`, at most 512 original bytes,
256 mantissa digits and absolute explicit exponent 256. Preserve its complete-
grammar/resource priority and positive-rate/nonnegative-factor rules when that
profile is selected. An unsupported provider lexeme is not silently reinterpreted
as this profile: retain raw evidence separately and use the verified adapter's
reviewed mapping. A retained string named original_decimal is not automatically
validated/reformatted by guessing from its name.

FiniteDecimal matches `-?(0|[1-9][0-9]*)(\.[0-9]*[1-9])?`, with numerical zero
represented ONLY as `0`. Thus `-0.1` is valid while `-0` is not. Fractional leading
zeros remain meaningful; the final fractional digit must be nonzero. No exponent,
redundant fractional trailing zeros or decimal point on an integer is canonical.
This is exact finite value normalization, never rounding. A recurring value such
as 5/6 uses Rational; it cannot be approximated or written as `5/6` inside a decimal
field.

There is NO universal 512-byte, 4096-bit, signed64 or 256-byte-reference limit on
these generic value types. Finite field/message profiles are mandatory, but must
match the owning domain rather than reuse a narrower parser by convenience:

- CORE-001's source limits bound that parser; its 4096-bit limit bounds its finalizer.
  They do not bound every retained Exact or derived wire value.
- [STORAGE-015](sqlite-application-plans-v1.md) explicitly retains canonical Exact
  components beyond signed64/4096 bits, with configurable component_bytes (default
  16384 DECIMAL BYTES each). Its reference_bytes default is 4096, also configurable.
  A matching public field cannot silently acquire a smaller generic limit.
  A byte-only component policy is valid; do not invent an additional smaller bit
  bound absent from that domain.
- Canonical expansion can exceed a source-token bound: a valid negative source
  representing −10^−511 needs 514 bytes as a plain FiniteDecimal. A 512-byte output
  cap copied from the source parser would reject a valid value.
- An 8192-bit integer can need 2467 decimal digits. Profiles supporting such a
  ratio must cover that magnitude, plus a sign byte when negative; they cannot
  route it through Signed64 or a final-effect converter. Larger reviewed storage
  profiles remain possible too.

The expansion example's source is minus/zero/dot, 254 zero digits, then `1e-256`:
263 original bytes, 256 mantissa digits and exponent −256. Its exact scale is 511
fractional places, so the canonical value has minus/zero/dot, 510 zeros, then `1`.
This is notation for an independent boundary family, not a generated codec fixture.

A field must pin its actual decimal grammar, component bytes/bits and aggregate
budget before a decoder is implemented. A resource refusal leaves original data
intact and is explicit; do not truncate, fabricate zero/unknown/missing, round or
accept a fitting prefix. Existing finite-decimal-only historical valuation fields
are not changed merely because this common Rational type exists. Their versioned
domain/codec/storage bridge still needs separate review and implementation.

## 3. Opaque bytes, Unicode and presence

OpaqueBytes is complete `[0-9a-f]*` with EVEN length. Each pair represents one byte;
there is no prefix, padding, whitespace or case folding. Reject uppercase or invalid
hex rather than accepting alternate canonical spellings. The decoded length is
half the string-value length; check both encoded and decoded budgets with checked
arithmetic before copying/decoding. Hex deliberately favors a simple, unambiguous
mapping and costs twice the raw byte length. Owning message budgets include that
expansion and JSON overhead.

Reference, operation/context identity and opaque revision-reference fields use
OpaqueBytes with their own nonempty/domain/length rules. Identity equality is exact
decoded-byte equality. A NUL byte is not an empty reference; non-UTF-8 bytes remain
representable. No ISO/provider/UUID registry, normalization, alias or authority is
inferred. Where reference ordering is required, compare unsigned decoded bytes
lexicographically, shorter exact prefix first; never use locale/case collation or
signed-char order.

Text accepts well-formed Unicode scalar values, preserving combining sequences,
case and empty strings. Reject invalid UTF-8 or lone surrogate escapes; never insert
replacement characters or normalize to NFC. JSON escaping does not authorize HTML,
script execution or a path. Additional text/control/markup limits belong to the
owning field; an XML-template restriction is not a universal Text restriction.
Source data whose exact original byte encoding matters uses OpaqueBytes instead.

The following states remain DISTINCT:

| Wire/data state | Meaning |
| --- | --- |
| Member absent | No member was transmitted; the owning schema decides whether omission is legal and what projection it denotes |
| JSON null | An explicitly transmitted null, allowed only for a nullable field; its domain meaning must be declared |
| Known zero | Signed64/Unsigned64 `0`, or another type's exact zero; not an unknown sentinel |
| Present empty | Text `""` or empty OpaqueBytes; retained when that field permits it |
| Domain unknown/not-applicable/not-disclosed | Distinct knowledge states when the domain distinguishes them; preserve its explicit state/projection rather than infer one from zero/null/omission |

No decoder fills missing members with null, empty, zero, a current timestamp or a
guessed successful state. Optional and nullable are independent field choices.
Complete projections must specify which members are required, even when their
values are null. Where unknown, not applicable and redacted differ, the owning
schema needs an explicit discriminator or separate projection type. Do not invent
those facts from an absent upstream field. Fetch, source-as-of and publication
metadata never fill each other's missing values.

## 4. Correlation, operation identity and revisions

A CorrelationReference is nonempty OpaqueBytes of at most 256 decoded bytes. It is
NEW bounded transport-control metadata, distinct from larger domain references.
A response retains the request's correlation when valid; independent concurrent
requests need distinct correlations, and a retry may use a new one. It is not an
authentication secret, provider event identity or proof of a commit.

An OperationReference identifies one intended operation in the owning endpoint's
explicit installation/principal/operation scope. It uses OpaqueBytes with that
domain's existing bounds; it is NOT narrowed to the correlation limit. The owning
contract must freeze payload identity, retry/result retention, conflict handling
and current disclosure/access checks. Structural JSON key order/escape differences
alone do not establish a different operation, and equal numerical values do not
erase different original source spellings. This common layer defines no hash or
universal payload fingerprint.

Execution/deduplication order follows the owning domain. In particular, do not
globally reject a legitimate stored-result retry merely because today's revision
differs; current permission/disclosure remains required. A transport timeout or
lost acknowledgement does not prove rollback. Do not invent a new operation ID,
reapply effects or report a confirmed failure from an unknown commit outcome.

A RevisionCounter uses Unsigned64; an owning domain may require it to start at 1.
A RevisionReference is a nonempty OpaqueBytes token. Each field pins ONE of these
types and its resource rules; there is no implicit conversion or universal revision
union. Preserve UINT64_MAX where valid. Never parse an opaque token as a counter,
compare unrelated resources' revisions, infer authority from it or substitute a
client preview for current server state. Conflict payload/privacy and the actual
atomic revision check belong to the owning operation.

## 5. Versions and capability vocabulary

ProtocolIdentifier is ASCII `[a-z][a-z0-9_.-]{0,63}`. It is a new bounded vocabulary
label, not a restriction on arbitrary domain references/provider names. Equality
is exact; dots carry no automatic ordering, range or inheritance meaning. Common,
HTTP, WS and scene contracts have separately identified versions and reviewed
imports. An implementation advertises only profiles it actually supports; this
document alone adds none to a running server.

An offer is an ordered list of unique ProtocolIdentifiers. The server selects the
FIRST entry in its own advertised preference order also present in the client's
supported list. No lexical-largest version, wildcard or guessed backward
compatibility is inferred. No intersection means incompatible_version before
operations, with bounded public upgrade guidance supplied by the owning transport.
Unknown valid offered identifiers can be skipped; an unknown/unsupported selected
identifier cannot authorize best-effort operation decoding. Resource bounds apply
before negotiation, including on rejected/unknown offers.

A CapabilityIdentifier uses the same identifier grammar. The owning version pins
its registry and meaning. Only understood, actually available advertised features
may be used; unavailable or unknown features are hidden/disabled for compatible
clients. Requiring an unavailable capability yields unsupported_capability before
effects. A client's assertion never makes a feature implemented or permitted.
Capabilities do not certify provider payload/history/refund/auth support, platform
media conformance, permission, or a broader decoder budget. Public/private capability
disclosure is part of the owning authenticated projection contract.

HTTP006/API-004 still own the concrete handshake envelope, supported combinations,
matching server web assets and upgrade presentation. WS007 owns live changes and
resynchronization; scene008 owns renderer feature imports. All capability/version
names in the examples below are SYNTHETIC, not a list of supported product features.

## 6. Finite profiles, decoding and nonsecret errors

Before implementation, EVERY owning message profile must freeze finite maximum
wire bytes, nesting/collections/items, string values, decoded bytes and any numeric
component bytes/bits, plus total materialized input/output budgets. Bound array
offers/capability sets before negotiation; their complete envelope counts belong
to HTTP006/WS007. No unset/unbounded/default-to-CORE budget is allowed. These limits
are trusted implementation/profile data, never caller-selected authority or values
that a capability claim may enlarge. Valid domain values must have a matching
profile or an explicit versioned migration; a smaller limit cannot be hidden as
normalization. Caller-owned allocation is not retroactively bounded. These are
finite semantic input/output budgets, not global allocator/CPU guarantees; concrete
decoders must also pin their scratch/work bounds and allocation order in review.

Check complete transport byte bounds before decoding; bounded JSON depth/items and
string scanning precede unbounded materialization. Require one complete UTF-8 JSON
value with no BOM, duplicate object keys, NaN/Infinity, trailing value or lone
surrogate. Validate declared primitive shapes/lexical bounds before numeric/byte
construction; check selected bit bounds before rational gcd/arithmetic. Project
complete decoded/output ownership and representation expansion before growth.
No decoder mutates authoritative state or persists a NUM-04 hold by throwing.

Unknown members are rejected unless the owning version explicitly defines their
bounded extension space. Common Rational/error objects are closed. Unknown
extensions never become policy/authority or silently select a different version.
Precise framing/parser failure priority and domain error ordering must be pinned
by the concrete transport/decoder contract before its tests/code, preserving its
owning domain's established priorities.

CommonFailure is a closed object with required `code` and `correlation_id`. Code
is one of: invalid_encoding, invalid_json, invalid_primitive, resource_limit,
incompatible_version, unsupported_capability, not_authorized, conflict, unavailable,
outcome_unknown. correlation_id is a valid CorrelationReference or null when none
is safely available. Do not echo malformed/oversized correlation data. This record
is at most 1024 UTF-8 JSON bytes including its keys, syntax, escaping and whitespace;
outer transport framing/wrappers have their own bounds. No arbitrary message,
request body, path, provider response, credential, supporter field or stack trace
is included. Clients map codes to localized public labels. Concrete transports
own status/close codes and may define reviewed typed result/detail wrappers outside
this closed common record; they cannot add raw diagnostics to it.

Protected outcome/conflict/existence details require CURRENT disclosure permission.
Schema validity, actor/revision references and capability declarations grant none.
Unknown access denies under §10; clients/handlers never become writers. Existing
browser cookie/CSRF and desktop-header session rules remain SESSION-003's work;
account tokens stay out of URLs/local storage. This introduces no hosted auth
service, public-client authorization shortcut or authenticated endpoint capability.
outcome_unknown expresses lack of knowledge, not a confirmed domain failure;
an owning transport may instead have no normal response. Only the actual service/
writer completion can establish its authoritative outcome.

## 7. Independent literal examples

JSON below is hand-authored data, not a schema, generated fixture or executed codec.
Type/profile annotations are essential; the same string can mean different typed
values. Limits not supplied by this document must be pinned by the owning field.

| ID | Typed input / condition | Required interpretation or rejection |
| --- | --- | --- |
| WP01 | Signed64 `"-9223372036854775808"`, `"9223372036854775807"` | Both exact endpoints; no floating conversion |
| WP02 | Unsigned64 `"18446744073709551615"`; `"18446744073709551616"` | First accepted, second out of range; neither is a JSON number |
| WP03 | Signed64 `"-0"`, `"01"`, `"+1"`, `"1e0"`, JSON number `1`, bool `true` | All reject as that primitive |
| WP04 | CORE DecimalSource `"1.20"`, `"1.2000"`, `"1.2e+00"`, `"-0.000e+03"` | Values 6/5, 6/5, 6/5, 0; every original spelling retained |
| WP05 | CORE DecimalSource `"0e257"` or `" 1.2"` | CORE resource or grammar refusal respectively; raw evidence is not rewritten/discarded |
| WP06 | FiniteDecimal `"1.2"`, `"-0.1"` versus `"1.20"`, `"1e0"`, `"-0"` | First two canonical; others reject as canonical VALUE strings, while valid source spellings remain separately retainable |
| WP07 | Valid CORE source for −10^−511, emitted as FiniteDecimal | Exact plain value has minus/zero/dot + 510 zeros + 1: 514 bytes; output profile must cover it, not copy source cap 512 |
| WP08 | Rational `{"numerator":"5","denominator":"6"}` or `{"numerator":"-1","denominator":"3"}` | Exact 5/6 or −1/3; no finite approximation |
| WP09 | Rational `{"numerator":"0","denominator":"1"}` versus `{"numerator":"0","denominator":"2"}`, `{"numerator":"10","denominator":"12"}` | Only first canonical; no silent reduction |
| WP10 | Rational denominator `"0"`, `"-1"`, `"01"`; extra key `authorized:true` | Reject; neither bad components nor caller flags grant a valid value |
| WP11 | Rational `{"numerator":"18446744073709551616","denominator":"1"}` under a sufficient Integer profile | Exact 2^64 is valid here; Unsigned64 rejection must not leak into this type |
| WP12 | A stored canonical component with 4097 bits within its reviewed component-byte bound | Generic Integer/Rational preserves it; CORE finalizer's 4096-bit rule is not imposed on storage/wire data |
| WP13 | OpaqueBytes `"0080ff"` | Exactly bytes 00, 80, ff; includes NUL/non-UTF-8 without Unicode guessing |
| WP14 | OpaqueBytes `"00"`, OpaqueBytes `""`, Text `""` | One NUL byte, empty bytes, present empty text. Required opaque references reject empty bytes; empty Text belongs only to its declared text field |
| WP15 | OpaqueBytes `"0"`, `"FF"`, `"0x00"`, `"0g"` | Reject odd/noncanonical/invalid hex, no trimming/case folding |
| WP16 | Text `"e\u0301"` versus `"\u00e9"` | Distinct scalar sequences preserved; malformed UTF-8/lone surrogate rejects rather than replacing |
| WP17 | Synthetic complete metadata profile: required nullable raw_bytes/utc_ms, `{"raw_bytes":"","utc_ms":null}` versus `{"raw_bytes":null,"utc_ms":"0"}` | Present empty raw bytes/unknown instant versus absent raw bytes/known epoch; omitting a required member is a different invalid shape |
| WP18 | RevisionCounter `"0"` / `"18446744073709551615"`; opaque RevisionReference `"00"` | Numeric range permits both counters; a domain's min-1 rule can reject 0. Opaque token remains one byte, not counter/epoch 0 |
| WP19 | Server offers example.v2, example.v1; client offers example.v1 | Select example.v1; names are synthetic, no runtime support claimed |
| WP20 | Same server; client offers only example.v3, or selects unsupported example.v3 | Incompatible before operations; never guess v1 compatibility from the shared prefix |
| WP21 | Server/client understand example.read; only client claims example.write | Only actually available mutually understood features can be used; client claim supplies no implementation or permission |
| WP22 | Valid CorrelationReference `"00"` on a retry with a new correlation but unchanged OperationReference | Correlation matches this exchange, operation identity retains the owning scope/retry meaning; neither is an auth token |
| WP23 | CommonFailure `{"code":"invalid_primitive","correlation_id":null}` | Bounded nonsecret classification; an extra input/debug/credential member rejects |
| WP24 | Source JSON `{"numerator":"1","numerator":"2","denominator":"3"}`, or null where nonnullable | Whole message rejects; no duplicate overwrite or implicit zero |
| WP25 | Protected stored-operation retry after access revocation | Current disclosure check still applies; compatibility/operation ID cannot reveal the record |
| WP26 | Lost acknowledgement after possible commit | Preserve unknown outcome and stable operation intent; do not fabricate rollback or generate a new award key |
| WP27 | Ordered opaque references `"4f"`, `"80"`, `"ff"` | Decoded ASCII O precedes bytes 80 then ff in unsigned-byte order; no signed-char comparison |

## 8. Ownership and next gates

Root coordinates ONE shared definition/import version for future common schemas;
HTTP006/WS007/scene008 reference it rather than fork representations. This document
does not finalize every endpoint, handshake envelope, domain error/payload order,
validator library, wire transport framing, scene renderer or auth adapter. Split
those focused stages under ADR-0004 instead of adding unrelated behavior here.

Different actual document review comes next. Concrete schema/API ownership, bounded
independent examples/tests and their review, minimal missing-feature interfaces,
selected dependency foundation and observed valid behavioral red precede any codec/
client/feature implementation, followed by separate source/CI acceptance. No passing
provider, platform, renderer, security or compatibility implementation is claimed.
All seven integrations, all three FX providers and unresolved owner product choices
remain in scope and are not decided by these representations.
