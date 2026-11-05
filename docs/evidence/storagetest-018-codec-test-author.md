# STORAGETEST-018 — structural codec test authoring

Author `/root/capture_codec_tests`, 5 November 2026. Support branch
`task/STORAGETEST-018-codec-tests`, separate worktree
`/tmp/subathon-storagetest018-codecs`, based on task revision
`ed898878cdf723cb2c9da8b2469e4a4406c94450`. Root retains all build/CI/progress,
migration/schema allocation and final baseline integration ownership.

This is AUTHORED acceptance, pending different actual test review, compilation
and first intended red. No new codec or feature test was invoked. No production
file, old assertion/fixture, shared build or task-state file is edited.

Authority: specification v0.45 §§3/5/17/18 (DB-01/API-02/NUM-04), accepted
[consumer §2](../contracts/reference-application-adapters-v1.md), concrete
[018 §§3/6](../contracts/sqlite-initial-captures-v1.md), unchanged reviewed
`reference_capture_values.hpp`. Scope is the eight structural codec functions;
no SQL, admission, evaluator, current authority, live/provider or release proof.

## Independent literal provenance

`tests/fixtures/reference_capture_codec_vectors.hpp` contains frozen literal
hex frames. The author separately calculated their byte sequences from grammar
U32/B/U64/I64 with Python standard-library `struct.pack`, never project codec
output. Fixture C/p, synthetic.reference.v1/A/N, R/r/a/b/T, add600000,
automation(I/U)/K, no cap, Immediate/0/Fresh/no deadline, five complete1 facts;
arrival X/V/UTC0/Q/E/absent sourceUTC/present-empty payload; execution Y,
Receipt7/CID0/P1. These choices are well-formed structural DATA, not admission.
SRI1 exactly matches the contract's independent99-byte A annotation.

| Literal | Semantic DTO B | Document bytes | Format bytes | Envelope B | Document SHA256 |
| --- | ---: | ---: | ---: | ---: | --- |
| SRC1 | 60 | 116 | 35 | 151 | cbd2c33d5ef64afc5ef7cf84d80e2d48a897c2546588bb17ea48855fab4c3659 |
| SRA1 | 73 | 157 | 29 | 186 | fc13ab62dd59e8f4fcf02321b38c2dd6228d18cb080109620339aa790e64e66e |
| SRX1 | 70 | 138 | 31 | 169 | 94f8cd2fa147d6e716be24a6145843429dd6dcb7cb482d0daab707ea1c0ff961 |
| SRI1 | 55 | 99 | 37 | 136 | 73d28784a5dfa16200d06ba4889534e5f33164f4f496953958849c8919c33050 |

I is0 throughout. SRC1 refs sum34, scalars/options/tags26. SRA1 adds
four refs4+UTC8+source presence1 to SRC60; Full SRX sum70: outer/source refs28, IDs16, plan-ref1,
revision8, rule17. SRI ten refs31+three U64s24. Frames independently add
magic4 and each B's U32; no outer format is embedded into a document.

## Coverage and harness

`tests/qt/reference_capture_values_qt.cpp` supplies27 private Qt slots, every
function has its own canonical literal encode/decode slot. Other focused slots
cover all six limit fields zero/MAX across all8 functions; whole envelope136/135,
document99/98 and hypothetical115/114; complete DTO B and one under; reference4096/
4097, payload65536/65537 and present-empty versus null; outer input limits before
format/framing; every truncation of each frozen frame, wrong magic/overflow B,
trailing data; all known/unknown category semantics; tag/reference/shape/value/
binding priority; timer/optional signed values and exact signedMIN/MAX UTC bytes;
SRX positive revision throughMAX versus SRI exact1; Receipt1..MAX/CID0..MAX;
fully framed decoded over-limit refs/payloads/empty refs; nested SRC1 validation;
byte-exact embeddedNUL/high-byte references and owned results.

Marker exceptions are caught only to report explicit
`MISSING_REFERENCE_CODEC: ReferenceCaptureValuesNotImplemented` failures. No
WILL_FAIL, skip, failure inversion, placeholder pass, codec-generated literal
oracle or SQL setup is used. `failure` checks the exact public error enum and fixed
message. Canonical decoding compares complete fields to independently constructed
DTOs; encode expectations use literal bytes. Some additional domain/boundary
round trips supplement independent frozen canonical oracles.

Root harness: executable `server_reference_capture_values_acceptance`, source
above, AUTOMOC, `subathon_reference_capture_values` and `Qt6::Test`. Literal
header is relative; no fixture path definitions. Register each of27 slots using
its source name. Keep intentional red separate from ordinary passing acceptance
until STORAGE-018 implements and independent source/config review passes.

## Actual commands and limits

- Read applicable root/tests instructions and concrete/consumer contracts/API.
- Created isolated support branch/worktree from exact base.
- `clang-format -i tests/qt/reference_capture_values_qt.cpp tests/fixtures/reference_capture_codec_vectors.hpp`: exit0.
- Independent Python stdlib framing/count/hash calculation: exit0, literal values
  above. Source slot inventory:27. This executes no C++/SQLite/project codec.
- Native compile/discovery: pending root harness, NOT claimed.
- Different test review: pending actual authored revision, NOT claimed.
- First intended red: NOT run. Entire STORAGETEST-018 baseline still pending,
  including distinct SQL closure tests/diagnostic/schema observation.

Allocation-free borrowed scanning, max64 records/512 scratch/depth3,
preallocation ordering and own bad_alloc/length_error mapping cannot be established
by these black-box values alone; implementation source audit remains mandatory.
No process-global allocation injector is introduced. Large U32 payload construction
is not attempted; framing overflow is tested with a tiny declared oversized frame.
All data are synthetic; no credentials, supporter data or live side effects.
