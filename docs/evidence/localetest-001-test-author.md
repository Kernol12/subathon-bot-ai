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
