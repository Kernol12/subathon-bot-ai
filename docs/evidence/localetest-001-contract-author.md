# LOCALETEST-001 catalog contract author handoff

Author /root/service_author, 2026-11-04, isolated branch
`task/LOCALETEST-001-contract-author`, from integrated main0b631dc plus coordinator
claim028c6f8. Owns ONLY [contract](../contracts/translation-catalog-v1.md) and this
note. Shared progress/build/CI/schema/catalog/tests remain coordinator-owned.

The specification v0.45 §12/I18N-01/03/04 and LOCALE-001 require one JSON catalog,
English/German, named parameters/plurals and deterministic generated QtTS/QM/browser
formats. Proposed explicit source shape, finite named-token grammar, exact Unicode/
XML/JSON mappings and resource limits preserve that scope. Literal TC01–14 examples
are manually stated expected data, not outputs of a new generator or Qt probe.

The selected simple mapping keeps SAME named templates in Qt/browser outputs.
Direct QTranslator lookup selects an ID/form without the QCoreApplication `%n`
replacement step; a later bounded named adapter substitutes once, treating injected
braces/percent/HTML-looking text as plain DATA. Qt plural witnesses1/2 select en/de
one/other while actual signed64 count strings, including MIN/MAX and -1, remain
lossless ordinary named values. No positional conversion, QString::arg escaping,
JavaScript floating-point narrowing, new runtime dependency or private sentinel
alphabet is introduced.

The separate read-only investigator /root/codec_author reported installed Qt6.12.0
lrelease/lupdate/LinguistTools targets and audited actual6.12 primary sources for
ID keys, deprecated-idbased option, direct versus QCoreApplication translation,
Qt en/de numerus tables, percent handling and lrelease validation/reproducibility.
The contract cites those primary references and distinguishes observed tool
availability/source facts from FUTURE generatedQM/runtime/reproducibility evidence.
Root then independently downloaded the exact official QtTools29144192-byte archive,
verified its published SHA256, listed589 safe paths and inspected extracted lrelease
ELF dependencies: Qt6Qml/Network/Core. Current qtbase+ICU alone is insufficient;
matching pinned Qtdeclarative runtime and actual tool-execution evidence remain
required. This is preflight evidence, not generatedQM acceptance. Root owns the
later actual dependency/CI selection.

No code, source catalog, generator interface/stub, fixture, test or native feature
has been authored or invoked. ProductionLOCALE001 remains planned. Actual two-form
QM generation, direct QTranslator byte/scalar/plural lookup, pinned reproducibility,
English-base fallback, independent test review/intended red and implementation/code
review/ordinaryCI are required future gates. Native/browser preferences, runtime
switching, number/date/currency locale, themes, German layouts and supported-package/
platform verification remain their original tasks.

Independent document reviewer /root/service_reviewer must inspect this ACTUAL
candidate; no prospective/source/runtime approval is claimed. Existing parked
CORETEST008/PR52 and owner questions are untouched.

## Checks

Read root/scoped instructions, task brief, spec§12 and neighboring LOCALE002–004
boundaries, architecture/test/workflow/progress preparation. Author checks below
record actual results; they are documentation/link checks only.

- python3 tools/check_project.py — PASS396 tasks/122 decisions/local links/presets/HTML.
- python3 tools/progress.py render --check — PASS current snapshot.
- git diff --check — PASS for the current tracked diff; staged new-file whitespace
  validation accompanies the author commit. No native/generator feature run.
