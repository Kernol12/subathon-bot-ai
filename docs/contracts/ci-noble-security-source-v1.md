# Noble split security source — v1

CI-003; specification v0.45 §§17–18; [task brief](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/CI-003.md).
This is proposed contract material, not review approval, observed red, implementation
or hosted recovery. It narrowly extends [CI-002 v1](ci-setup-gates-v1.md), whose other
requirements and completed historical evidence remain unchanged.

## 1. Evidence and limits

At PR126 source `1fdedfb`, API-host run43401217575/job131804239166 and exact-numeric
run43401217866/job131804240991 both emitted the same bounded refusal: `URIs`,
`uri_not_admitted`, stanza2, line52, `ubuntu_security`, fields_omitted4 and
stanzas_omitted1. Runner facts were Ubuntu24.04.5, ImageOSubuntu24 and
ImageVersion20261111.127.1. The helper stopped before commands/writes/update/build.
[Actual triage](https://github.com/Kernol12/subathon-bot-ai/pull/126#issuecomment-6849584005).

This identifies a URI in the existing four-member Ubuntu-security class and two
evidence stanzas, with five distinct admitted keys in the selected stanza, including
URIs. Both stanzas passed the complete structural scan: only the seven admitted
keys, no duplicates, continuations or malformed field lines. Stanza1 also passed v1
semantics: its stripped URI is the mirror URI, and Enabled is absent or exactly yes.
It does NOT identify the exact security URI spelling, the remaining field names/
values, full source contents or cloud.
No full failed-host source capture is claimed. Image links did not provide a
retrievable complete source configuration. Do not infer unknown values from an image.

Primary design inputs are Canonical cloud-init commit
`30b3ea21ec86269a2e31ae0028fbb4a7cc5b41a1`:
[deb822 template](https://github.com/canonical/cloud-init/blob/30b3ea21ec86269a2e31ae0028fbb4a7cc5b41a1/templates/sources.list.ubuntu.deb822.tmpl),
[APT defaults](https://github.com/canonical/cloud-init/blob/30b3ea21ec86269a2e31ae0028fbb4a7cc5b41a1/cloudinit/config/cc_apt_configure.py),
and Ubuntu Noble's [sources.list(5)](https://manpages.ubuntu.com/manpages/noble/man5/sources.list.5.html)
and [deb822(5)](https://manpages.ubuntu.com/manpages/noble/man5/deb822.5.html).
They document separate distribution/security stanzas, Noble pockets, whitespace
lists and the Ubuntu keyring. These are normative design inputs, NOT a claim that
the failed runner contains every value below. Support is conditional on checking
ALL profile requirements against the actual source string at runtime.
This is the actual-source-evidence path required by CI-002 §1: bounded observations
from the failed source, corroborating primary format documentation, a reviewed
conditional profile, and runtime validation of every still-unobserved value. It is
not permission to assume those values. The pinned template's security URI position
after removal of its template marker is consistent with line52; this is a static
correlation, not a rendered-template experiment or a full failed-host capture.

## 2. Exact additional profile

Keep the existing GitHub-hosted Ubuntu24.04 guards and sole UTF-8 source read.
Keep the existing structural scan, refusal precedence, counts and labels. Exact
order: finish the complete structural scan, check this profile, then (only if it
does not match) run the unchanged v1 semantic loop. On a match, write and flush the
admission record inside validate_sources and return without the v1 semantic loop.
The fixture order is validate, admission write, flush, validate_end, first child.
It must not be selected by ImageVersion, cloud, hostname or any extra I/O.

The source has exactly two evidence stanzas, in archive then security order.
Each has exactly these five keys, once each: `Types`, `URIs`, `Suites`, `Components`,
`Signed-By`. No optional `Enabled`, `Architectures` or diagnostic-only field is
allowed in THIS profile; the existing mirror-only language is unaffected.

| Field | First/archive stanza | Second/security stanza |
| --- | --- | --- |
| Types | exactly `deb` | exactly `deb` |
| URIs | exactly `mirror+file:/etc/apt/apt-mirrors.txt` | one exact member of the four-address set below |
| Suites | exactly the three unique tokens `noble`, `noble-updates`, `noble-backports` | exactly the one token `noble-security` |
| Components | exactly the four unique tokens `main`, `universe`, `restricted`, `multiverse` | the same four unique tokens |
| Signed-By | exactly `/usr/share/keyrings/ubuntu-archive-keyring.gpg` | exactly the same path |

The additional admission allowlist is exactly:

- `http://security.ubuntu.com/ubuntu`
- `http://security.ubuntu.com/ubuntu/`
- `https://security.ubuntu.com/ubuntu`
- `https://security.ubuntu.com/ubuntu/`

This is an admission allowlist, independent of the diagnostic-only label table.
Adding a diagnostic label/address must never silently extend admission. Credentials,
ports, query/fragment, case changes, percent-encoding, extra slashes/paths or multiple
URI tokens do not match. Azure and archive direct addresses remain refused.

Field order is irrelevant. Values may have leading/trailing ASCII space/tab; only
Suites and Components are whitespace-token lists, separated by ASCII spaces/tabs.
Those token lists may be permuted, but duplicates, missing or extra tokens refuse.
Types, URIs and Signed-By otherwise match their exact case-sensitive strings.

These grammar rules apply to the exact decoded string passed to validate_sources.
The unchanged read_text(encoding='utf-8') already performs Python's universal-newline
translation of on-disk CR/CRLF to LF. CI-003 changes neither that read nor its
arguments and adds no further normalization; it cannot detect the original disk
newline spelling. Direct callers/fixtures may supply LF or CRLF in the string,
including mixed LF/CRLF; the final terminator is optional. A remaining lone CR,
VT, FF, FS, GS, RS, NEL or Unicode line/paragraph separator makes this profile
ineligible. A raw-CR fixture tests this string boundary, not on-disk CR detection.
Classify every original decoded line after removing only its LF/CRLF delimiter,
before relying on v1's stripped fields or stanza grouping. For this profile only,
a blank separator must be truly empty; whitespace-only lines, including ASCII
space/tab, NBSP and U+001F, make the profile ineligible. Any number of empty lines
is allowed. A comment's first character must be # in column zero; leading space,
tab or other whitespace before # makes the profile ineligible. Column-zero comments
may appear before, inside, between or after stanzas; they never split or count as
evidence stanzas. Comment-only blocks are ignored, and comment content may be UTF-8.
Each line must be exactly an empty separator, a column-zero comment, or a column-zero
field line containing only printable ASCII (0x20–0x7e) or tab and satisfying the
field rules above. Every other line makes the profile ineligible. In particular,
an indented # line must not be mistaken for a comment rather than a continuation.
The empty-separator restriction is a conservative subset of the documented format;
it does not change v1's existing whitespace/comment handling on fallback. Thus field
trimming/tokenization cannot turn unknown Unicode/control values into known values.
No source file or URI is rewritten.

An ineligible profile does NOT bypass existing validation. Fall back to the existing
semantic checks and exact refusal records. In particular, both-mirror sources keep
their existing accepted behavior, including v1's other accepted forms. Existing
single/mixed-suite direct-security cases still refuse. Preserve every other accepted
or refused v1 input; this profile is the ONLY added accepted set.

## 3. Fixed admission record and unchanged process behavior

When and only when the full new profile matches, `validate_sources` emits one extra
record before returning. The existing runner record still precedes validation.
Exact new line (with one trailing LF) is:

```text
CI_APT_V1 {"event":"source_admitted","profile":"noble_split_security_v1","v":1}
```

This is a scoped extension of the CI_APT_V1 event family. Exact keys/event/profile
are closed; no source values, URI, field names, arbitrary text or additional keys.
Use the existing bounded serialization/emission mechanism, one complete write and
immediate flush, at most4096 ASCII bytes including prefix/LF. Existing runner and
source_refused shapes/bytes are unchanged. Legacy mirror-only success emits only
the existing runner record; refused sources emit no admission record.

Carry [C-A1 precision](https://github.com/Kernol12/subathon-bot-ai/pull/622#issuecomment-6845562002)
into tests, implementation and code review: exact `int` write count equal to full
length; OSError/ValueError, short/wrong/non-int returns and flush failures stop setup.
Admission-record failure returns main1, attempts the fixed diagnostic-unavailable
message once best-effort, emits no normal unsupported-layout message and runs no
children/writes. Earlier complete runner bytes remain in the controlled fixture;
fixture retention is not an OS-durability guarantee. No recursive logging if dead.

The admission record proves only that this source profile was checked. A later
write/update failure must still fail main1. Preserve exactly the seven legacy
diagnostics, two checked tee writes/bytes and single checked inherited-output
update300s, native Acquire::Retries1, trust, pins and all timeout/error behavior.
No source write, extra read/command/process, new retry/capture, image pin, dependency,
resolver/proxy change or signature bypass. No new identity/cloud/Python name lookup.
For this profile, the archive uses the configured mirror list, while the security
pocket is fetched directly from the validated source URI, outside that list.
A security-host outage therefore fails the unchanged strict update; it grants no
retry, fallback rewrite or exception to Error-Mode any/native Retries1.

Code review must statically audit that there are no new socket/urllib/http/ssl
imports or calls, environment keys, Path/os I/O or subprocess surfaces. The shared
fixture omits legacy DNS entry points; the new test file adds its own patches for
gethostbyname, gethostbyname_ex, gethostbyaddr, getfqdn and getnameinfo, raising
AssertionError, without changing either frozen test file. Source audit remains
required and does not claim that fixture barriers alone prove complete isolation.

## 4. Independent examples and test seam

Canonical synthetic positive P1 (not a claimed host capture):

```text
Types: deb
URIs: mirror+file:/etc/apt/apt-mirrors.txt
Suites: noble noble-updates noble-backports
Components: main universe restricted multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg

Types: deb
URIs: http://security.ubuntu.com/ubuntu/
Suites: noble-security
Components: main universe restricted multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
```

Positive P2 is a synthetic template-shaped layout, not a host capture. It is
eligible with either final-LF presence, permitted mixed LF/CRLF and any number of
empty-line/comment-only blocks. All comments start in column zero; the in-stanza
comments are deliberately positive:

```text
## synthetic template-shaped header
# comments do not count as fields

Types: deb
URIs: mirror+file:/etc/apt/apt-mirrors.txt
# comment inside archive stanza
Suites: noble noble-updates noble-backports
Components: main universe restricted multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg

## synthetic security updates
# comment block before fields
Types: deb
URIs: https://security.ubuntu.com/ubuntu
Suites: noble-security
# comment inside security stanza
Components: main universe restricted multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
# trailing comment
```

| Example/mutation | Required result through main |
| --- | --- |
| P1/P2; each other allowed security URI; permitted field/token order/comments/line endings | 0 after the unchanged update; runner then exact admission record, before any child |
| P1 first URI changed to http://azure.archive.ubuntu.com/ubuntu/ | 1; original uri_not_admitted at stanza1/line2, field URIs, azure_archive, omitted4/1 |
| P1 security Suites changed to noble; or security Signed-By changed to /other/keyring.gpg | 1; original uri_not_admitted at stanza2/line8, field URIs, ubuntu_security, omitted4/1 |
| P1 adds Enabled:yes as security line12 | 1; same URI refusal at line8, omitted5/1 |
| P1 inserts Enabled:yes after archive Signed-By, before its blank separator | 1; uri_not_admitted at stanza2/line9, field URIs, ubuntu_security, omitted4/1 (the insertion shifts later lines) |
| P1 archive Types changed to deb deb-src | 1; uri_not_admitted at stanza2/line8, field URIs, ubuntu_security, omitted4/1 |
| P1 archive Suites changed to noble noble noble-updates noble-backports | 1; uri_not_admitted at stanza2/line8, field URIs, ubuntu_security, omitted4/1 |
| P1 security URI changed to http://security.ubuntu.com/ubuntu// | 1; uri_not_admitted at stanza2/line8, field URIs, other, omitted4/1 |
| P1 security Signed-By prefixed by NBSP or U+001F before the canonical path | 1; uri_not_admitted at stanza2/line8, field URIs, ubuntu_security, omitted4/1; raw field fails the profile even though v1 strips its prefix |
| Prefix P1 with one line NBSP + # synthetic comment + LF | 1; uri_not_admitted at stanza2/line9, field URIs, ubuntu_security, omitted4/1; v1 ignores that comment but the new profile rejects its indentation |
| P1 inserts tab + # note after security Components as new line11 | 1; uri_not_admitted at stanza2/line8, field URIs, ubuntu_security, omitted4/1; v1 ignores the indented comment but the new profile must not |
| P1 replaces its empty line6 by a line containing only ASCII spaces/tabs, NBSP or U+001F | 1; uri_not_admitted at stanza2/line8, field URIs, ubuntu_security, omitted4/1; v1 still groups two stanzas, but the new profile rejects the original nonempty separator |
| P1 adds unknown X-Test field as line12 | 1; structural unknown_field at stanza2/line12, field <unknown>, omitted5/1 |
| P1 adds a third mirror-only stanza | 1; URI refusal at stanza2/line8, ubuntu_security, omitted4/2 |
| Both URIs use the original mirror URI | existing0 with runner only; no admission record |
| Admission record cannot be emitted | main1/no children; fixed unavailable best-effort, no unsupported message |
| P1 update fails | main1; one update only, both earlier records remain, no source_refused record |

Tests own only `tests/tooling/test_apt_security_profile.py`. They may import the
already reviewed `AptSandbox` fixture/independent constants from the sibling
diagnostics module, without importing/inheriting its TestCase or changing either
old test file. Its controlled stderr targets an arbitrary event, so source_admitted
faults need no new producer API. Preserve every old assertion and source byte.
No implementation-derived expectations or broad duplicate baseline suite.
The new file explicitly inserts its own resolved tests/tooling directory in
sys.path and imports the sibling test_apt_diagnostics module by filename (or only
its fixture/constants); tests/tooling is not a package. It must include the normal
unittest.main() script guard. Do not expose the old TestCase as an imported class
in the new module, which would duplicate discovery. A producer import after the
fixture's established path setup refers to the same patched producer module.

Focused coverage must isolate each profile field/shape condition, invalid URI/raw
format and structural bypass, record ordering/failure before children, unchanged
successful command set/options and later update failure. Existing exhaustive emitter
cases need not be copied wholesale: exercise the new call site's count/type,
write/flush and persistent-failure paths with the reviewed fixture. Use synthetic
sources, environment and child results; no actual local APT/network/write operation.

The author handback must list every new method's expected current-main outcome,
derived statically: PASS or an explicit behavioral assertion FAIL, never ERROR.
Assert missing records/calls before indexing their trace; do not convert fixture,
import or barrier failures into red. Other positive/fault/update cases will also
fail before implementation, but they are not selected red evidence and remain
unrun until implementation. Only the selected method uses the intended-red marker.

Run EXACTLY this one command after the full test-review cycle, not discovery or
the whole file:

```sh
python3 -B tests/tooling/test_apt_security_profile.py AptSecurityProfileTests.test_reviewed_split_profile_is_admitted -v
```

It uses literal P1 and AptSandbox's default valid runner facts. Before its decisive
assertion, establish the one UTF-8 source read, runner and result-in-{0,1}
prerequisites. If current main returns1, require exactly these three stderr lines
(each with LF), zero child calls and the one source read:

```text
CI_APT_V1 {"event":"runner","image_os":"ubuntu24","image_version":"20261111.127.1","os_id":"ubuntu","os_patch":"24.04.5","os_version":"24.04","redacted_fields":[],"unknown_fields":[],"v":1}
CI_APT_V1 {"event":"source_refused","field":"URIs","fields_omitted":4,"line":8,"reason":"uri_not_admitted","stanza":2,"stanzas_omitted":1,"v":1,"value_class":"ubuntu_security"}
Ubuntu APT setup failed: unsupported Ubuntu APT source layout
```

Then require0 with the exact message
`intended assertion red: CI_SECURITY_SPLIT_PROFILE_NOT_ADMITTED`.
Expect one behavioral assertion FAIL, zero ERRORs/skips. No missing API/fixture,
syntax/dependency/decoding failure is red. No NI stub is necessary or authorized.

## 5. Gates and publication

Full independent contract and test cycles close before this red. A fresh isolated
implementer then changes only tools/prepare_ubuntu_apt.py, followed by full code
review and relevant tooling/schema/scaffold checks. Coordinator owns docs, shared
metadata and publication. No unreviewed tests are imported/run/published.
Before the code packet is reviewed, the coordinator updates docs/CI.md to explain
the conditional profile, source_admitted and direct security-pocket availability.
Code review includes that guidance and the explicit static no-new-I/O/lookup audit.

Keep all21 app-bound contexts and19 workflow declarations unchanged, plus all prior
protections. Use a focused prerequisite PR and normal protected merge. Inspect all
nine actual hosted preparation lanes, final-head producers and the admission event.
Report whether this profile was exercised; mirror-only successes do not establish
new-profile platform execution. A fully green candidate may merge even if no new
admission record appeared, but must report "profile not platform-observed" and
must not claim the originally refused host was fixed. Any required failed lane
still blocks merge. If a split host remains refused after this implementation,
the profile did not match it: retain the failure, do not rerun/relax, and return for
another separately reviewed evidence-based refinement. An unchanged refusal may
add no facts; do not pretend otherwise or guess the hidden mismatch.
Do not rerun for luck or invent missing source facts.
After this prerequisite merges, resume PR126 with fresh checks and normal merge,
then PAUSE. No WORKFLOW-006/007 or stopped product work is authorized here.
