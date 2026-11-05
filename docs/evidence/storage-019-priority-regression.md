# STORAGE-019 decoder format-priority regression authoring

Author `/root/capture_fixture_author`, separate from codec source author and the
actual source reviewer `/root/capture_reviewer`. Fresh support branch
`task/STORAGE-019-priority-regression-author`, isolated worktree
`/tmp/subathon-storage019-priority-author`, from actual integrated source candidate
`146f0c3`. Root owns test CMake/CI registration, progress and publication. Own only
additive `tests/qt/reference_capture_values_qt.cpp` changes and this note. No codec
implementation, public API, fixture, dependency, build or shared metadata change.

## Actual finding and specification-derived oracle

Different code review found two priority defects in the actual implemented
`outer()` at this candidate: an empty outer format returns invalid_reference;
an unknown outer format paired with NULL QByteArray returns invalid_shape before
format comparison. This author did not write that production source and does not
fix it here.

The accepted consumer contract §2 explicitly orders decoder checks: options,
complete outer format/body byte bounds and B/I, unsupported outer format, borrowed
framing scan and then decoded DTO semantics. The concrete018 contract §3 repeats
outer bounds/B/I → unsupported outer format → borrowed framing. Therefore a fitting
empty format is unsupported_format before a DTO reference error; a fitting unknown
format with NULL document is unsupported_format before known-format document
shape. This changes no contract or product choice and weakens no baseline.

Add exactly one data-driven slot:
`unsupportedFormatPrecedesDocumentSemantics`, with eight independently named rows:
four public decoders (SRC1/SRA1/SRX1/SRI1), each with (a) empty outer format and the
existing valid literal body, (b) outer format `unknown` and a NULL QByteArray.
All arrangements fit default outer byte/owned bounds. Empty-format cases assert
the existing literal body is non-null/nonempty; unknown-format cases assert the
arranged document is actually NULL. Each dispatches through the existing public
`decode(kind,...)` helper and requires the fixed public unsupported_format error
via the existing exact code/message assertion helper. No encoder/implementation
creates expected values; the existing independent literal fixture is reused.

The original known-format/NULL-document invalid_shape case remains unchanged, as
do ALL existing27 slots, assertions, fixture bytes and expected outcomes. Native
registration should add this ONE real method as case28; `_data` is not a separate
feature case. This is focused coverage of an observed priority bug, not a new
schema/store/admission/runtime capability.

## Actual author checks and pending gates

Actually ran:

```sh
clang-format -i tests/qt/reference_capture_values_qt.cpp
clang-format --dry-run --Werror tests/qt/reference_capture_values_qt.cpp
git diff --check
```

All exit0. A text-only preservation check removes precisely the new slot/data
block and compares the rest byte-for-byte with actual146f0c3 source: equal. The
additive block has exactly eight QTest rows. No test body, decoder, native setup,
fixture generator, Qt/SQLite operation or intended red ran in this author role.

Different actual test review by `/root/capture_reviewer` is required before first
invocation. Root must integrate/review the exact28-case harness, compile/discover,
observe the eight intended reached priority failures against held source, then
release the source author to correct only the reviewed behavior. Preserve original
red/pass evidence and distinguish this new behavioral regression from the original
27 immediate-stub red. Actual passing28, separate source/config review and selected
ordinary CI remain required for STORAGE019 completion. SQL capture, current
admission/authority/evaluation, provider/platform/recovery and full release gates
remain with their owning tasks.
