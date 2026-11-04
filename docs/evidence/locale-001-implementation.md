# LOCALE-001 catalog generator implementation candidate

Author: service_author, 2026-11-04. Dedicated branch
`task/LOCALE-001-generator-author`, worktree
`local-evidence/worktrees/locale001-generator-author`, from merged baseline
`07dc53e` plus coordinator claim `c1d565a`. The coordinator authorized production
implementation after PR53's reviewed FIRST intended red and normal merge. Read
root/tools instructions, the actual source/API contracts, baseline handoff, full
test review, independent red attribution, fixture inventory and all 43 tests before
editing. The old stub baseline was not replayed.

Owned changes are only [translation_catalog.py](../../tools/translation_catalog.py),
[messages.json](../../localization/messages.json) and this note. Root owns generated
product text/QM publication, all state/HTML/maps/checkpoint/build/CI/pins. No test,
fixture, inventory, contract, native probe or shared configuration is changed.

## Candidate and behavior

Implementation source blob: `d7985160d17875f8eae5b990baa549a4ef9338ca`.
Product catalog blob: `cb392400f942446989297d605f6c67ac1ae32200`.
The approved source/API blobs remain respectively
`5138329642d112423431d0314a6f9f9a78c7795d` and
`acb90189c371ab4aa8979cbb2ac8d117b2e8ab0e`. The unchanged reviewed tests remain
blob `6a7da9282e52ca0656284eca19732280d9d6c098`.

The module implements its five public entrypoints with the existing frozen owned
DTOs and signatures. It validates strict UTF-8/BOM/scalars, bounded JSON depth,
duplicate keys, lossless number-token kinds, complete global schema/parameter/
coverage/template stages and deterministic sorted identities. JSON integer parsing
never converts an arbitrary token to a host integer/float. Named braces, declaration
agreement and selector-only forms follow the reviewed grammar. No named formatter
or runtime argument substitution is introduced.

Rendering preserves source scalars, CR, Unicode separators, literal Qt-looking
percents and the required XML/JSON escapes. It produces the four canonical text
artifacts, with per-artifact size projections before encoded result growth.
Real generation always executes the explicit supplied tool's version check, then
English/German compilation in a new private stage. No PATH lookup, shell, network,
cached QM or alternate successful compiler is used. Capture combines both pipes
under 65536 bytes, with monotonic per-process deadlines and termination/reaping of
the invoked child. Worker read failures return to the caller without printing a
thread traceback or raw child data. Tool exit metadata is actual, including exit0
when a regular/nonempty/64MiB output check fails.

Source stat/type verification precedes its read-only nonblocking open and opened
file identity check; ordinary resolved source symlinks work. Canonical disjoint
roots, exact direct-child owned inventory, managed symlinks, special files and
source/compiler aliases are checked before tool work. Source identity/content and
layout are rechecked after compilation. Digests, comparisons and copies use checked
ordinary files and at most 65536-byte reads, with an exact-size/one-byte-EOF guard.
All six actual artifacts stage successfully before target work. Generate uses
per-file temporary replacement, preserving unrelated files and hard-link aliases;
check compares actual bytes without target creation/replacement/removal. No
filesystem-wide transaction, crash durability or hostile concurrent actor isolation
is claimed. A publication error can leave mixed old/new expected files and returns
failure, never a complete BuildResult. Owned temporary stages/files and streams
are cleaned on ordinary success/error paths.

One prose ambiguity was reported before choosing behavior. The coordinator and
different reviewer explicitly confirmed the specific reviewed form-shape rule:
wrong ordinary/plural keys, including missing text/other, are `invalid_plural`;
exact-key null/empty required text is `coverage`; a nonnull nonstring is
`invalid_schema`; missing/extra locale or null locale entry is `coverage`.
This preserves the existing explicit examples/assertions. No baseline or contract
was edited to obtain passing results.

The initial product source has six stable IDs: common cancel/close/save actions,
connected/disconnected status and a remaining-timer label with declared text
parameter `duration`. It supplies English/German, descriptions and GPL-3.0-or-later.
There are no fixture IDs, implementation labels or independent generated sources.
These catalog labels alone do not implement UI consumption/preferences.

## Actual verification

Host: Linux x86-64, Python 3.14.8. Actual official selected compiler:
`/tmp/subathon-locale-qttools-6.12.0/prefix/bin/lrelease`, observed
`lrelease version 6.12.0`. Its verified official Qt Base/ICU/Tools/Declarative closure
is recorded in [tool preflight](localetest-001-tool-preflight.md). The independent
selected native probe is the root build's
`build/translation-catalog-debug/tests/qt/translation_catalog_probe`.

Exact final acceptance command in this author worktree:

```sh
SUBATHON_TRANSLATION_LRELEASE=/tmp/subathon-locale-qttools-6.12.0/prefix/bin/lrelease SUBATHON_TRANSLATION_PROBE=/home/archnoob/ProgrammingProjects/SubathonProject/subathon-bot-ai/build/translation-catalog-debug/tests/qt/translation_catalog_probe python3 -m unittest discover -s tests/localization -p test_translation_catalog.py -v
```

Exit0, all 43 tests PASS in 3.753s, no skips. This exercises all four frozen text
goldens, actual newly generated QM raw lookup via the independent native verifier,
two fresh-directory QM byte reproduction, strict CLI records, global stage order,
bounded error-only child timeout/output/file failures with actual exits and reaping,
ordinary/FIFO/missing/symlink source cases, aliases/inventory, source/layout rechecks,
stale/oversized targets and read-only check. No expected result was generated or
modified using the implementation. Ignored raw log in this worktree:
`local-evidence/locale001-author/acceptance.txt`, SHA256
`411143ad230de3ab159f2f41b37704500bfc42e13bd2df4e507720e609b5e50f`.

Additional checks:

- `python3 -m py_compile tools/translation_catalog.py` — PASS.
- `python3 -m unittest discover -s tests/tooling -v` — 19 PASS.
- `python3 tools/check_project.py` — PASS, 396 task records/122 decisions and local
  links/presets/current HTML in this claimed branch. Root's separate rate planning
  work is not overwritten or claimed imported here.
- `python3 tools/progress.py render --check` — PASS.
- `git diff --check` and the exact staged owned-path check — PASS before commit.
- `python3 tools/translation_catalog.py validate --source localization/messages.json`
  — exit0, exact bounded success record with six messages, no tool invocation.
- A temporary-directory Python harness called generate/check twice on the actual
  six-message product source, checked source preservation and target bytes/mtimes
  before/after each check, then compared both full BuildResult digests. PASS, actual
  selected compiler, six artifacts each time. Both temporary output trees were
  removed; no root-owned product output was published. The same harness checked
  `ast.parse(..., feature_version=(3, 10))` successfully. This proves grammar only,
  not execution under Python 3.10. Ignored product log SHA256
  `bcd88cf884fbd4c02684878c0a1db99032157515f08ed66183e0bfd555284663`.

The product's actual reproducible QM summaries were:

| File | Bytes | SHA256 |
| --- | ---: | --- |
| messages_de.qm |491|81962ff8f3407e65342f4c9ea8dcd10abc6240e8f662be27907130b1c6eaca3c|
| messages_en.qm |457|afdb33727b0bd3f9f47743157f3a7d921e72d0408512079c64dc7a90c02b7b78|

Product source SHA256 is
`415fbdc9812963d606b2b21158781df753135debb138a2cdb0fb57502e3fb4a7`.
Those hashes are observed outputs for reproduction, not independent lookup oracles.
Independent fixture/raw-string expectations remain the unchanged reviewed tests.

## Remaining gates

This is an author implementation candidate, not self-approval or final task closure.
Different actual source review, root integration/publication, latest automatic CI
and required merge gates remain. Python 3.10 and Windows runtime/package behavior
were not executed; no Python 3.10 interpreter was installed locally for a runtime
claim. Only Python 3.10 grammar compatibility was checked. Runtime named formatting,
native/browser catalog consumption, language/number/date/currency preferences,
theme, German layouts and platform release checks remain LOCALE-002/003/004/090.
No provider/live/user-data operation was invoked.
