# LOCALETEST-001 independent catalog document review

**Current verdict: APPROVED corrected documentf1e5297, bounded contract stage.**
Final section pins actual accepted artifacts; earlier changes-required findings
remain preserved. No concrete API, fixture, test, implementation or runtime result
is approved by this document verdict.

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

## Actual corrected document: approved

**APPROVED**, independent reviewer service_reviewer,2026-11-04, actual author
f1e5297 imported into reviewer4b2552a. Read the complete corrected contract and
author handoff, not only the proposed corrections. Exact accepted blobs:

| Artifact | Reviewed Git blob |
| --- | --- |
| [translation-catalog-v1.md](../contracts/translation-catalog-v1.md) | 3a249a586a4b4c04c19ee0cf7d12ba19758132f3 |
| [Author handoff](localetest-001-contract-author.md) | f002fc07f7f687b204d1d747c6300a737f4d54e6 |

All three actual findings are resolved:

- **R1:** the same generated web/en.json is now explicit native ID/parameter-type/
  plural-selector metadata, paired with mandatory English QM. Native text still
  comes from actual QM lookup; JSON forms are consistency data, never a missing-QM
  text fallback. Every base ID/form must match raw English QM. Selector-only TC14
  has full literal en/de templates and witness1/2 behavior even though the count
  placeholder appears nowhere. No native declarations are guessed from QM or
  duplicated in hand-edited C++ code. German runtime absence/bad grammar uses the
  verified base, while build/package consistency compares German JSON and QM.
- **R2:** six expected artifacts and recognized case-sensitive direct-child output
  namespaces are explicit. Extra recognized files refuse before publication;
  unrelated names/subtrees remain untouched. generate replaces only the six
  expected regular files after complete validation/tool staging; check never
  modifies them. Canonical disjoint roots, source exclusion/alias checks and
  non-symlink managed entries protect the sole input without inventing recursive
  arbitrary-tree traversal, silent migration or a two-directory transaction.
  Missing-file/type/race/exception details remain concrete-interface review work.
- **R3:** TS structural indentation, tag/attribute order, form order, newlines,
  forbidden formatting constructs and multiline scalar handling are normative.
  Complete TC01 German XML plus the exact English delta makes full independent
  byte fixtures derivable. Continuation indentation is never inserted into values;
  CR uses the explicit character reference and original Unicode normalization stays.

The primary-source-supported empty-context ID/raw-template/witness approach is
unchanged. Literal-percent/braces and inserted argument data are not processed by
Qt positional/numerus replacement. Exact signed64 strings preserve MIN/MAX without
Number conversion, with explicit±1 category selection and int1/2 witnesses. Strict
source/duplicate-key/ID/coverage/declaration/two-form/template/Unicode validation
remains mandatory before Qt; lrelease normalization cannot certify it. Canonical
browser data and text/QM consistency remain one-source outputs. Runtime adapters,
preferences, immediate switching, formatting locale, themes and layouts retain
their separate actual acceptance/release gates.

Read root's actual [tool environment evidence](localetest-001-tool-preflight.md):
all four official archives were verified and the selected lrelease-version command
now exits0 with6.12.0 and its Qt/ICU closure inside the checked prefix. The original
incorrect ICU placement remains a documented setup failure. This is environment
availability only, not a compiled-QM/translation/reproducibility pass; no such
feature was executed by this reviewer or introduced by the document.

### Independent document verification

A stdlib-only review script parsed the literal TC01 JSON/XML from the document,
checking root attributes, direct message ID with no context, source/description/
translation scalar equality, exact child order and final LF. It checked unique
TC02–14 table rows and signed count stringsMIN,-1,0,1,2,MAX against the explicit
magnitude-one witness identity. All passed; the script imports no project generator
or Qt code. XML parsing a handwritten document example is not actual QM acceptance.

Project396/122/local-link/preset/currentHTML checks, render --check and diff --check
all exit0. No native configuration/build, generator, source catalog, schema artifact,
interface/stub, independent feature fixture/test, lrelease compilation, Qt lookup,
reproducibility or intended red was authored/executed for this review.

This approves integration of these exact two corrected DOCUMENT artifacts and
permits the next separately reviewed concrete-interface preparation. It does NOT
finish LOCALETEST-001 or LOCALE-001, approve future API/tests/code, or waive actual
selected-prefix foundation/QM lookup/reproducibility before first valid project
stub-red. Full specification scope remains, including native/browser consumption,
English fallback packaging, German layouts, supported platforms and live/full
release evidence. CORETEST-008 owner questions remain unanswered and separate.
