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


## Actual independent review corrections R1–R3

Reviewer service_reviewer changes-required6e870dc (root importe3240eb) inspected
actual a06b3a7. This document-only revision addresses that same bounded batch:

- R1: native declaration metadata is the SAME generated web/en.json, packaged with
  mandatory English QM and selected German QM. Descriptor IDs/parameter types/
  plural selector are explicit inputs; selector-only TC14 now has exact English/
  German literal forms and count1/2 observations. The native base pairs actual QM
  raw lookups with metadata English forms; JSON strings never replace required QM.
  Selected absent/malformed locale falls back to the verified English pair;
  invalid/missing base is catalog_unavailable. Generator/package consistency still
  checks actual German QM against generated de.json. No hand-maintained native map
  or new application-logic edit per translation is introduced.
- R2: exact case-sensitive direct-child owned output namespaces, six expected
  filenames, extra-owned refusal, unrelated preservation, replacement behavior,
  nonmutating check, canonical disjoint roots and source alias/hard-link/symlink
  protection are now explicit. No recursive tree walk, filename from messageID,
  silent obsolete deletion or imaginary two-directory transaction is promised.
- R3: canonical TS indentation, root/message attribute order, child/form order,
  structural newlines and no self-closing/extra tags are normative; actual multiline
  text is never pretty-printed or indented. COMPLETE TC01 de TS literal plus exact
  en delta makes independent full-byte goldens derivable.

Read coordinator actual environment evidence f3f9f03 without editing it. The
contract now records all4 official archive verification/runtime closure and actual
selected lrelease-version exit0, retaining the initial ICU placement setup error
as such. This establishes environment availability only; no project catalog, QM,
lookup/reproducibility fixture or new native feature is authored/executed here.

The original source grammar, Unicode/CR preservation, raw percent/directQTranslator,
lossless signed64/witness selection and all release/task boundaries remain binding.
Actual different reviewer recheck of this revision is pending; no self-approval,
concrete API/test release or LOCALE001 implementation is claimed.

Actual revised-document checks: python3 tools/check_project.py PASS396/122/local
links/presets/HTML; python3 tools/progress.py render --check PASS; git diff --check
PASS. No generator/native/QM/lookup/reproduction command ran.


## R4 actual selected-tool contradiction and bounded document correction

After actual R1–R3 approval534d4d0 (root7b8252f) of contract blob3a249a58,
coordinator-authorized scratch third-party feasibility contradicted its top-level
message serialization. Investigator codec_author's actual selected official
Qt6.12 lrelease exited1: Unexpected tag message at line3. This was an existing-
tool feasibility failure, NOT project stub-red, generator acceptance or a revision
of baseline assertions (no project fixture/test exists yet). The prior XSD-based
serialization approval is reopened and preserved in history.

The SAME investigator then proved an explicit EMPTY context/name wrapper with
actual selected QtTools/Core: six finished literal entries per language, four QM
compilations across two fresh directories,56 exact raw lookup checks PASS, both
per-language binary comparisons PASS. Read the preserved initial rejection and
complete successful probe logs/evidence.json under ignored disposable
/tmp/subathon-locale-tool-probe-ihwtzipp; no scratch file was edited or promoted
by this author. Example checks covered literalpercent/braces, CR/LF/TAB/Unicode,
ordinary/plural/witness/selector-only and explicit English QM fallback. These are
bounded Qt feasibility observations only; generator/schema/CLI/named formatter/UI/
package/platform verification and project baseline remain future gates.

The coordinator explicitly reauthorized ONLY these same two Markdown files for
this supported mapping correction, after proof. Revised canonical TS has ONE
<context>, explicit <name></name>, every ID message inside that empty context;
message/child/form indentation becomes4/6/8 spaces and COMPLETE TC01 TS is updated.
No nonempty context or idbased flag is introduced; all source/metadata/plural/
percent/ownership mappings remain intact. The actual Qt TS-reader primary source
is now cited alongside the permissive schema and QM writer.

Different reviewer service_reviewer requested R4 and must examine this ACTUAL
corrected serialization before any API/stub/test approval. The separately assigned
API document may develop pure interfaces, but cannot freeze the rejected TS bytes.
No generator, source catalog, schema, API Python file, fixture/test or project
native feature is authored/invoked by this correction.
