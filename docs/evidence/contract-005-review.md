# CONTRACT-005 independent common-wire document review

**Verdict: APPROVED actual document candidate at 378da1b.** Reviewer
service_reviewer, different from author codec_author and integrator root,
2026-11-04. Owns only this new note on task/CONTRACT-005-independent-review.
This approves common representation/compatibility documentation and examples;
it approves no schema, API, parser, encoder, generated client or runtime capability.

Actual author 1404de537497d437fdf7edc1ad8ff3720ff6d472 is integrated as 378da1b.
Exact reviewed artifacts:

| Artifact | Git blob |
| --- | --- |
| [Common contract](../contracts/wire-primitives-v1.md) | 11eeb5f422a174e042143df6b476ae1d717eeacf |
| [Author evidence](contract-005-author.md) | 7cebb719aca46a3e124a996ba7f674958eef9bf1 |
| [CONTRACT-005 brief](../tasks/CONTRACT-005.md) | 67723a402ff0f51cd8aa8c9b445e63d0b1a6f67d |
| [Accepted decomposition ADR](../decisions/0004-wire-contract-prerequisites.md) | b1a2965e8fff9ad9406995dffa45c264aa9af21d |

Read root/schema instructions, all 323 proposed contract lines and WP01–27,
the complete brief/ADR, specification v0.45 API-01/02/03/NUM-04 and relevant
record/exact-decimal/application-plan/storage/application-service boundaries.
No substantive defect, requirement conflict or new owner decision was found.

## Exact values and existing-domain compatibility

The representation separates original DecimalSource spelling from canonical
FiniteDecimal VALUE and Rational. Trailing zeros, exponent spelling and signed
source zero remain evidence; canonical zero, integer domains and rational reduction
do not rewrite that evidence. JSON number/bool/null substitutes cannot silently
become exact values. Signed64/U64 endpoints, arbitrary bounded Integer and positive
denominator are distinct. Rational zero is 0/1, nonreduced input rejects, and a
recurring rate stays rational rather than a rounded decimal or missing value.

FiniteDecimal's lexical rule plus its explicit zero rule correctly admits -0.1
while excluding -0, redundant fractional trailing zeros and exponents. CORE's
source grammar/resource/domain order is imported only when that profile is
explicitly selected. Other upstream lexemes remain separately retainable as raw
evidence, rather than guessed from an original_decimal property name.

The lack of a universal 512-byte/4096-bit/256-byte-reference cap is necessary.
Actual STORAGE-015 documentation retains canonical Exact components beyond signed64/
4096 bits, with configurable default 16384 DECIMAL BYTES per component and default
4096-byte references. Actual ApplicationServiceLimits also uses 4096 reference bytes.
The proposal preserves those bounds instead of confusing decimal bytes with bits
or importing a narrower parser/finalizer. Owning profiles must still select finite
input/output/scratch budgets before implementation; byte-only numeric bounds remain
valid. This is not approval of an unbounded generic decoder.

The negative source expansion example is correct: 263 source bytes, 256 mantissa
digits and exponent -256 represent minus 10^-511. Plain value output needs 514 bytes.
An 8192-bit integer can need 2467 decimal digits, plus a negative sign where applicable.
These demonstrate why output/profile expansion cannot simply copy source/parser
limits. Existing finite-decimal HistoricalValuation/storage fields are not migrated
by defining a common Rational type; their separate versioned bridge remains required.

## References, presence, versions and authority

Lowercase even-length hex gives one simple canonical byte representation, preserving
NUL/non-UTF-8 and charging twice the raw value length plus JSON/envelope overhead.
Decoded unsigned-byte ordering, including ASCII/high-bit boundaries and shorter
prefixes, matches opaque domain identity. Unicode Text is separate, preserves scalar
sequences and rejects invalid UTF-8/surrogates without normalization/replacement.
The document explicitly distinguishes member omission, nullable value, known zero,
present empty and domain knowledge/disclosure states. No provider/source/publication
time or success fact is synthesized from missing data.

The 256-decoded-byte CorrelationReference is new bounded control metadata, not a
retroactive operation/revision-reference limit. Operation scope/payload equality,
retention/retry/current disclosure and atomic outcome stay with their actual owning
domain. A legitimate protected stored-result retry is not globally invalidated by
today's unrelated revision. Opaque revision references remain distinct from full-U64
counters. A timeout cannot certify rollback or authorize a new award intent.

Version/capability identifiers have finite ASCII vocabulary, exact identity and
explicit server-preference intersection selection. There is no inferred lexical
version range/backward compatibility. Unknown offers/unsupported selection and
unavailable required capabilities fail before effects under the owning protocol.
Capabilities grant neither authority nor provider/platform support. Current access,
private capability disclosure, account cookies/CSRF/header rules and the authoritative
queue remain independent. No unresolved timer/product policy is selected here.

CommonFailure is closed, finite and nonsecret: fixed code plus safe correlation/null,
with a 1024-byte total JSON bound. No source body, supporter field, credential,
arbitrary path/message or stack diagnostic is reflected. A valid maximum correlation
can be emitted compactly within 565 bytes including the longest declared code, so
the cap does not make valid control metadata impossible. Whitespace/escape/framing
expansion still counts under the explicit complete-wire bound. Private outcome/
existence/conflict details retain current disclosure gates. Unknown outcomes remain
unknown; this common object establishes no authoritative transaction result.

## Independent checks and scope verdict

A read-only Python stdlib Fraction/gcd/integer calculation independently verifies
23 arithmetic/size/order facts: signed/U64 extrema and difference, 2^64, equivalent
source values, canonical rational/zero reductions, 256/263/511/514 expansion counts,
4097-bit storage fit, 8192-bit digit length, 4096-byte reference hex expansion,
16384-component object overhead and unsigned 4f/80/ff order. Separate literal key/
code-length arithmetic verifies the 565-byte compact error bound. No JSON/hex/wire
parser or encoder, project import, fixture, schema, generated output or native
program is invoked. Static source-label inspection finds WP01–WP27 exactly once.
Non-numeric examples were checked directly against their declared types and owning
domain rules; these are document checks, not passing executable wire tests.

Actual infrastructure commands:

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All pass: 401 task records, 122 decisions, links/presets/current HTML, current progress
render and clean whitespace. No build, new test/feature, provider or network write
occurs. Only this review note is authored. Existing documentation CI is appropriate;
root still owns applicable latest-head CI, coherent state/checkpoint and publication.

CONTRACT-005's brief requires a bounded versioned DOCUMENT and independent examples
first; this actual candidate satisfies that stage. Per-field/message profiles,
concrete schema/import ownership and failure priority remain explicit mandatory
preconditions to downstream implementation, not missing claims hidden as completed
code. The common identity is proposed data vocabulary, not a supported API version.
After its ordinary review/CI/integration gates, the document task may complete.

CONTRACT-003 still requires 005/006/007/008 integration and consistent actual schema
packages/examples. HTTP/OpenAPI envelopes/handshake/web assets remain006/API-004;
persistent synchronization and SPIKE-001 close limitations remain 007/API-001/003;
scene imports, separate renderers/media/conformance remain 008 and their baselines.
No child loses its own reviewed examples/tests/red/code/source-review gates. All
seven integrations, all three FX providers, auth/security, platform and release
scope remain required. This review approves none of those future artifacts.
