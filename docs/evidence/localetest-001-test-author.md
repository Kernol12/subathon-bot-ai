# LOCALETEST-001 independent fixture/test author handoff

Author codec_author, 2026-11-04, task/LOCALETEST-001-fixture-author from actual
approved minimal stage6589861. Authority: specification v0.45 §12/I18N-04,
source contract51383296 and concrete APIacb90189, actual immediate stub23089e6b.
No production function/main/script/marker has been invoked by this author.

## Intermediate fixture/probe candidate

Six independently hand-authored synthetic messages cover greeting, integer plural,
percent/braces, XML/CR/LF/TAB/non-ASCII/combining/emoji, reorder/repeat and selector-
only forms. source.json order deliberately differs from the COMPLETE four frozen
canonical TS/browser JSON goldens. manifest.json records their exact SHA256 values;
no project renderer/generator or scratch compilation produced those golden bytes.
The Qt-only probe independently embeds every literal expected raw form.

Probe CLI (root owns shared CMake/CI):

```text
translation_catalog_probe foundation FIXTURE_ROOT LRELEASE
translation_catalog_probe verify BINARY_OUTPUT_ROOT
```

foundation uses ONLY the actual supplied Qt tool/public QTranslator and independent
frozen TS. It compiles en/de in two fresh temporary directories, loads all actual
QMs, checks direct ID/raw forms/witnesses/Unicode/missing/actual-English fallback,
compares both QM byte streams and prints their SHA256. verify checks future generated
QM against those SAME independent raw literal expectations. No new project symbol
is linked/invoked; no named formatter, JSON generator or UI behavior is supplied.

JSON syntax parsing of the three hand-authored JSON files and git diff --check
passed. A syntax-only compile of the actual probe with selected official6.12
headers and -Wall/-Wextra/-Wpedantic/-Wconversion/-Wshadow/-Werror passed. An initial
manual command incorrectly treated third-party Qt includes as -I, exposing a Qt
header conversion warning; corrected -isystem flags match CMake imported headers.
No probe execution, native feature acceptance or baseline red is claimed.

Python feature tests are being authored as the next part of this SAME assignment.
This intermediate commit supplies no independent review/approval or complete test
baseline. Root must obtain separate actual fixture/test review, run mandatory real
selected Qt foundation green, and only then observe the first project-marker red.

## Complete authored fixture/test candidate

The independent actual intermediate review requested TWO additive/precision
corrections before execution. F1 adds fixture.literal_plural: the TC06 exact raw
percent/brace template in both one/other forms and both languages, with the TC14
invisible count selector. The previous six source objects and all their form bytes
remain unchanged. This detects an accidental QCoreApplication numerus substitution
that the old ordinary-message n=-1 case could not detect. The four goldens and
manifest now contain7 messages. F2 uses a fresh English-base translator with its
own actual missing-English load failure instead of labeling an already-unloaded
selected translator as that different case. No prior assertion is removed/weakened,
and no feature run motivated these changes. Corrected actual review is still needed.

The full Python candidate is tests/localization/test_translation_catalog.py;
case_inventory.txt records exactly43 unique qualified methods, matching AST and
inert unittest loader discovery: CatalogValues17, CatalogFiles20, CatalogCLI6.
Shared registration must use those actual names rather than any earlier count.
No inherited duplicate file-test inventory or overridden duplicate method exists.

| Coverage | Authored oracles |
| --- | --- |
| Pure owned values/render | Complete sorted frozen DTO fields/immutability, complete four text byte goldens, message/object reordering, exact CR/Unicode/XML/browser escapes and invisible selector declarations |
| Validation | Nonbytes/strict UTF8/BOM/surrogates, JSON/constants/duplicate keys, integer-token kinds including a5000-digit token, source/depth/field/message/parameter limits and accepted exact field boundaries |
| Semantic priority | Root/message/ID shape and duplicates; parameter/selector declarations; coverage/form distinctions; brace/name agreement/XML-illegal scalars; deterministic sorted error identity and multiple-error precedence |
| Real generation/check | Actual six artifacts for7 messages, immutable real-byte digest summaries, actual Qt-only raw-QM verifier, two fresh generation binary comparisons, read-only matching/stale/equal-size corruption/missing targets and absent-root behavior |
| Ownership/filesystem | Extra owned names refuse without deleting, unrelated subtrees survive, stale expected outputs repair, regular symbolic source allowed, disjoint/source/compiler aliases, source hardlinks, managed symlinks, matching FIFO/directory and nonregular source refusals |
| Rechecks/bounded reads | Actual error-only child changes synthetic source or managed layout then execs real Qt; source/layout must be rechecked before publication; sparse oversized existing target mismatches unchanged |
| Processes | Actual missing executable, changed/malformed/nonzero version, en/de nonzero compilation, exit0 missing/empty/symlink/FIFO/directory/sparse oversized QM; combined40K stdout+40K stderr cap, short real timeout, negative signal metadata and owned-child reap |
| CLI | Exact help/version and direct metadata entry, compact ordered success/digest/error JSON, validate/generate/check and real QMs, usage priority/repeated/unknown/inappropriate flags, exits0/1/64/65/69/70/74, no source path/body/child chatter leakage |

Disposable error-only executables never substitute a successful QM compiler. They
exec the actual selected Qt tool for unaffected phases; their failure branches
explicitly fail or leave unacceptable output. The zero-exit bad-output cases require
actual tool_exit0 rather than inventing a process error. No mock successful process,
clock adapter, reversed failure, skip or weakened oracle is introduced.

The probe intentionally supplies NO application named formatter, integer argument
parser, language preferences, browser runtime, QML binding, rendering/layout or
package implementation. Signed MIN/MAX display, argument substitution and live
preference switching remain owning LOCALE-002/003 tests; this generator baseline
checks their exact declaration/raw-template contract. Streaming allocation order,
transient same-size adversarial replacement, unexpected internal exception mapping
and publication partial-I/O behavior also require implementation source review;
these cases do not claim hostile-concurrency or filesystem-wide atomicity proof.

Additional actual author commands (all exit0):

```sh
python3 -m py_compile tests/localization/test_translation_catalog.py
# Static AST/inventory uniqueness, disposable child-literal compile(), inert module
# import and unittest loader discovery:43; no test method/setup/data provider called.
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -fsyntax-only -DQT_CORE_LIB -isystem /tmp/subathon-locale-qttools-6.12.0/prefix/include -isystem /tmp/subathon-locale-qttools-6.12.0/prefix/include/QtCore -isystem /tmp/subathon-locale-qttools-6.12.0/prefix/mkspecs/linux-g++ tests/qt/translation_catalog_probe.cpp
```

NO Python feature/main/script/marker or native probe body has been called by this
author, including help/version. Python subcase data is authored, not observed.
The completed original other-task acceptance/red evidence is neither repeated nor
substituted for this task. Independent full fixture/test/harness review and SAME
selected-environment real foundation green must precede the coordinator's FIRST
actual named-marker red. LOCALETEST-001 remains in_progress; LOCALE-001 remains planned.

## Actual full review corrections T1/T2

Independent reviewer service_reviewer inspected the complete `bbdeefb` candidate
and recorded changes required in `e76a9a0`. Root explicitly released only this
Python module and author note for correction, before any feature test execution.
Source contract `51383296` sections 3/4/5 and API `acb90189` sections 2/3 already
require the following behavior; these additions introduce no new product choice.

T1: validation priority applies globally across ASCII-sorted message IDs within
each stage. The previous same-message precedence and same-stage ID ordering cases
could not reject an implementation that validated one whole message at a time.
The existing priority method now adds three independent two-message inputs:

- `a.first` malformed template, `z.last` invalid parameter type: invalid_schema
  identifies `z.last`.
- `a.first` missing German coverage, `z.last` invalid parameter type:
  invalid_schema identifies `z.last`.
- `a.first` malformed template, `z.last` missing German coverage: coverage
  identifies `z.last`.

Both IDs are valid and bounded; input order is deliberately later ID first. The
old four priority input/assertion groups remain exact. Each old group and each
new input has its own unittest subTest so an earlier natural marker error cannot
prevent a later independently authored input from being reached. A marker still
records an ordinary error, never a successful expected outcome.

T2: the existing complete-output method now has a separate small source containing
`A` + U+2028 + `B` + U+2029 + `C` in both languages. Four COMPLETE expected TS/browser
outputs are literal constants, hand-authored independently of project code. Browser
bytes require `\u2028`/`\u2029`, reject the raw UTF-8 separator bytes and decode to
the exact original scalars. Complete Qt TS expectations retain both scalars raw in
source and translation. The original frozen-source invocation and every original
assertion remain, in their own subTest, so its marker cannot mask this new source.

All 43 method names and the exact inventory remain unchanged: 17 values, 20 files,
6 CLI. No fixture, frozen golden, manifest, native probe, source/API contract, stub
or shared harness file is changed. The already approved seven-message Qt-only
foundation remains untouched. Root separately reported its selected-tool foundation
green with 100 assertions; this author did not replay it or promote that result to
feature acceptance.

Actual correction checks, all exit 0:

- `python3 -m py_compile tests/localization/test_translation_catalog.py`.
- Static AST comparison with `bbdeefb`: every previous assertion/failure-call node
  remains unchanged and in source order; 43 unique definitions exactly match the
  unchanged inventory. Inert import/loader discovery finds the same 43 methods;
  no test method, setup, data provider or project feature is called.
- Static extraction of only the four new literal expectation constants, with
  standard-library JSON/XML parsing: valid syntax, exact decoded scalars, escaped
  browser bytes and raw Qt separator bytes. No rendered/generated data is used.
- `python3 tools/check_project.py`: PASS, 396 task records, 122 decisions and local
  links/presets/current HTML in this isolated author branch. Root's newer shared
  records remain root-owned and are not overwritten.
- `python3 tools/progress.py render --check`: PASS.
- `git diff --check`: PASS; final staging checks confirm only the two owned files.

Corrected Python SHA256:
`df6c9a958bdd3d1858f2fa9f2c73377c7efbc8e35b0db945e4f3fcf9128d548c`.
Unchanged inventory SHA256:
`407e79bdef751002e7da3f30eca73ca01bdd7fee786444d52034bf16f16d2b86`.
Unchanged native probe SHA256:
`cab50606267675d470f0fbfa74ee1004f9580996c533a5676168ce232380d5b7`.

No feature/probe/generator/main/script/marker has been invoked by this author,
including help/version. Actual revised full-test review remains required before
the coordinator's first feature red. These are authored scenarios and static
checks; no Python feature result or self-certified independent approval is claimed.
