# LOCALETEST-001 independent catalog document review

Reviewer service_reviewer, different from author service_author,2026-11-04.
Isolated task/LOCALETEST-001-contract-review from coordinator claim028c6f8 and
integrated main0b631dc. Initial author candidatea06b3a7 imported into reviewer6f8da11.

## Initial actual candidate: changes required

| Inspected artifact | Git blob |
| --- | --- |
| translation-catalog-v1.md | 51c39d836857d46b23ca61b8f829c726cebc0afb |
| localetest-001-contract-author.md | ba0f66d5c843d981d2cae580a50217d7a9960cb6 |

Read all sections and TC01–14 against specification v0.45 section12/I18N-01/03/04
and LOCALE-001. This is document-only preparation: no generator/catalog/schema/
interface/stub/test or new Qt translation feature was executed by this reviewer.

The selected Qt6.12 mapping is supported by independently inspected primary source:
[QM writer](https://raw.githubusercontent.com/qt/qttools/v6.12.0/src/linguist/shared/qm.cpp)
uses ID keys for empty context plus nonempty ID;
[TS grammar](https://doc.qt.io/qt-6/linguist-ts-file-format.html) allows direct messages;
[lrelease](https://raw.githubusercontent.com/qt/qttools/v6.12.0/src/linguist/lrelease/main.cpp)
recognizes idbased as deprecated/no-op;
[QTranslator](https://raw.githubusercontent.com/qt/qtbase/v6.12.0/src/corelib/kernel/qtranslator.cpp)
returns selected raw strings without QCoreApplication's later percent substitution;
[plural tables](https://raw.githubusercontent.com/qt/qttools/v6.12.0/src/linguist/shared/numerus.cpp)
provide the two English-style branches used by en/de. This supports the proposed
literal-percent/named-template and witness1/2 design; it does not establish actual
compiled-QM behavior or reproducibility. Those remain required future evidence.

Three concrete document corrections are required before approval:

1. **Native declaration metadata.** QM contains message IDs/raw translations but
   does not retain declared parameter names/types or the plural selector name.
   TC14 permits that selector to be absent from every raw template. Therefore the
   native adapter cannot infer its witness argument or perform exact missing/extra/
   type validation from QM. Explicitly choose generated native metadata inputs,
   packaging/pairing and consistency obligations. Reusing the existing generated
   English JSON metadata is one simple option; no second hand-maintained map or
   application-logic edits per translation may be introduced.
2. **Output ownership/publication.** The CLI promises detection of extra owned
   files while only six fixed names are defined. Specify the precise owned set/
   namespace and its inventory rules, generate replacement behavior and unrelated
   file preservation. check must remain read-only and must not traverse arbitrary
   user trees. The sole source catalog cannot alias an owned publication target;
   generation must not overwrite its own source. Concrete path/error interfaces
   still require later separate review.
3. **Canonical TS bytes.** UTF8/LF/entities/tags alone allow multiple indentation,
   attribute/tag-order and multiline formatting choices. Define one structural
   serialization or a complete TC01 TS exemplar plus normative whitespace rules
   so independent complete XML goldens are uniquely derivable. Formatting must
   never insert indentation/newlines inside actual translated text values.

These findings were sent as one bounded document-only correction batch to the
sole author and coordinator. No user decision is required for these routine
mapping/ownership choices. No API/test/stub or production implementation is released
by this changes-required review, and no future revision is approved.

## Checks and retained gates

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All exit0;396 task records/122 decisions/local links/presets/current HTML pass.
No native configure/build, lrelease generation, QTranslator lookup or acceptance
was run. Tool/archive/ELF preflight from the coordinator and read-only investigator
is distinct from future actual selected-prefix QM/runtime/reproducibility proof.
That proof requires the matching Qtdeclarative/runtime closure as well as actual
QtTools and QtBase. Missing dependencies/compile failure/skipped lookup or TS-only
checks cannot establish a valid red or finish LOCALE-001.

Keep named single-pass DATA substitution, exact signed64 counts and witness-only
plural selection, mandatory English QM fallback, XML CR/scalar preservation,
strict coverage/two-form validation, deterministic browser data and truthful
runtime/generator boundaries. Actual preferences/switching/formatting/themes,
German layouts, packaging/platform/live and full-release obligations remain their
own required tasks. CORETEST-008's unanswered owner questions remain unrelated.
