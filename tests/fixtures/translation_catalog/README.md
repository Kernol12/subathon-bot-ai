# Independent translation catalog vectors

Synthetic LOCALETEST-001 data, derived manually from specification v0.45 §12 and
translation-catalog-v1.md blob51383296 / APIacb90189. No generator, renderer,
project stub or earlier scratch output produced these expected bytes.

source.json deliberately reverses the original six ASCII ID entries and places parameter
names out of canonical order. The four COMPLETE goldens pin the reviewed empty
Qt context/name, canonical whitespace, XML escaping and CR character reference,
actual LF/TAB/combining acute/emoji, browser security escapes, all declarations,
plural forms and one final LF. Their decoded values retain TC01/02/06/07/08/14; a seventh ADDITIVE
fixture.literal_plural combines TC06 literal-percent text with TC14 selector-only
structure, checking real witness1/2 preserve percent sequences. All original six
objects/forms remain unchanged after the independent review request.
integer type and selector are retained even when the selector is invisible.

The Qt-only probe has independent literal C++ raw-string expectations, so the
XML/JSON and compiled raw forms do not supply their own expected lookup results.
The file hashes in manifest.json identify the independently authored baseline;
they are not a generated substitute for byte/semantic assertions. These fixtures
are authored and unexecuted until actual separate review and foundation checks.
