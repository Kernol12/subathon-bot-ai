# STORAGETEST-018 independent structural codec test review

5 November 2026. Reviewer `/root/capture_reviewer`, different from test author
`/root/capture_codec_tests` and root harness author. Actual authored candidate
`367f758189621ef12a8575ea2ec5a77a34a60974`, imported unchanged as `f7e4653`;
actual complete root source/harness candidate
`f7c3127014e7f80a3703640454260d2ef21ac126`. Same isolated review branch/worktree
as [foundation review](storagetest-018-fixture-review.md).

**APPROVED for these actual27 codec tests/literals and their CMake/CI harness.**
No codec or feature body was invoked in this review. The intended red remains a
subsequent observed gate; this approval does not finish all STORAGETEST-018 or
release STORAGE-018 production while its SQL baseline is unfinished.

| Inspected artifact | Exact blob |
| --- | --- |
| tests/fixtures/reference_capture_codec_vectors.hpp | 53da620d645304600340d361a84d91abda7ce74c |
| tests/qt/reference_capture_values_qt.cpp | c207e4233adcf9e6720429df8a0f31900885d205 |

Read complete source/header/literals/author note and concrete018/consumer014 codec
grammar/priority/limits against specification v0.45 DB-01/API-02/NUM-04. Independently
parsed all four literal documents and checked each mutation offset and stated
complete DTO/envelope accounting. No oracle came from a project encoder/decoder.

| Literal | Semantic DTO B | Document bytes | Format bytes | Envelope B |
| --- | ---: | ---: | ---: | ---: |
| SRC1 | 60 | 116 | 35 | 151 |
| SRA1 | 73 | 157 | 29 | 186 |
| SRX1 | 70 | 138 | 31 | 169 |
| SRI1 | 55 | 99 | 37 | 136 |

All semantic I0. Reproduced author SHA256 values and literal A SRI1 from the
physical contract. Configuration policy p/Immediate is explicitly distinct from
the SQL closure fixture policyW/Grace120000; no unsupported admission is inferred.
Independent full field expectations cover all8 encode/decode entrypoints.

Focused cases check six positive/representable limit fields across all8 methods;
complete input versus framed output bounds and outer format-byte charge; exact
refs/payload bounds; null versus present-empty; every truncation/magic/oversized
declared length/trailing data; decoded per-field limits and empty references;
all known/unknown tags and priority; required refs, nonnegative cap/add/Grace,
signed full-range timestamps/optional values, installation/principal binding;
positive Receipt/full CID including0/MAX; SRX revision1..MAX versus SRI exactly1;
nested SRC1 and embedded NUL/high-byte ownership. Canonical literal assertions
anchor the supplementary boundary round trips; no encode/decode-only expected
generator substitutes for independent bytes.

Error assertions preserve options/input→tags→refs→shape→values→binding→result
priority. Decoders bound complete outer input before unsupported format/framing,
then validate a fully framed body; unreadable frames remain invalid_document.
Account principal is invalid_binding while unknown enum is invalid_tag. Unknown0
facts remain structural DATA; admitted/closure completeness is a separate gate.
Marker exceptions produce explicit test failures, never skipped/inverted success.
The outer scenario catches the first missing call in each slot; an eventual
27-marker red establishes entrypoint failure, not that every subsequent assertion
in its method was reached or passed.

Actual independent fresh configure/build and show-only/discovery commands/results
are recorded in the companion foundation review:141-step compile/link succeeds,
336 enabled registrations, exact27 compiled slot names equal CMake registered
cases, no DISABLED/WILL_FAIL. YAML/23 embedded Python blocks parse. Ordinary CI
checks complete27 registration/explicit draft-red acceptance labels without
executing known-red bodies; foundation remains mandatory green. No existing
source/assertion/fixture/pin/check was weakened.

Allocation-free preflight, max64 borrowed records/512 scratch/depth3 and own
bad_alloc/length_error mapping need later implementation source audit; these
black-box values alone do not prove preallocation ordering or allocation behavior.
No huge U32 allocation or process-global injector was introduced. Observe genuine
intended red only after actual reviewed setup/selected environment gates, and
retain separate full SQL test/review/red gates. No SQL, admission, evaluator,
authority, provider/recovery/platform/UI or release capability follows.
