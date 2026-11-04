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
