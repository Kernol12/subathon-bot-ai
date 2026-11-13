# Noble split security source — v1

CI-003; specification v0.45 §§17–18; [task brief](../tasks/CI-003.md).
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
evidence stanzas, with five noncomment lines in the selected stanza. It does NOT
identify the exact URI spelling, other field names/values, source contents or cloud.
No full failed-host source capture is claimed. Image links did not provide a
retrievable complete source configuration. Do not infer unknown values from an image.

Primary design inputs are Canonical cloud-init commit
`30b3ea21ec86269a2e31ae0028fbb4a7cc5b41a1`:
[deb822 template](https://github.com/canonical/cloud-init/blob/30b3ea21ec86269a2e31ae0028fbb4a7cc5b41a1/templates/sources.list.ubuntu.deb822.tmpl),
[APT defaults](https://github.com/canonical/cloud-init/blob/30b3ea21ec86269a2e31ae0028fbb4a7cc5b41a1/cloudinit/config/cc_apt_configure.py),
and Ubuntu Noble's [sources.list(5)](https://manpages.ubuntu.com/manpages/noble/man5/sources.list.5.html).
They document separate distribution/security stanzas, Noble pockets, whitespace
lists and the Ubuntu keyring. These are normative design inputs, NOT a claim that
the failed runner contains every value below. Support is conditional on checking
ALL profile requirements against the actual source string at runtime.

## 2. Exact additional profile

Keep the existing GitHub-hosted Ubuntu24.04 guards and sole UTF-8 source read.
Keep the existing structural scan, refusal precedence, counts and labels. Only
after the complete structural scan succeeds may this additional profile match.
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

For this new profile, line endings are LF or CRLF; lone CR, VT, FF, FS, GS, RS,
NEL, Unicode line/paragraph separators are ineligible. Empty separators contain
only ASCII space/tab. A comment starts with `#` after zero or more ASCII spaces/tabs;
its content may be UTF-8. Every noncomment field line contains only printable ASCII
(0x20–0x7e) or tab. Thus trimming/tokenization cannot turn unknown Unicode/control
field values into known values. No source file or URI is rewritten or normalized.

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

| Example/mutation | Required result through main |
| --- | --- |
| P1; each other allowed security URI; permitted field/token order/comments/CRLF | 0 after the unchanged update; runner then exact admission record, before any child |
| First URI changed to direct Azure | 1; original uri_not_admitted at stanza1/line2, azure_archive, omitted4/1 |
| P1 security Suites changed to noble; or its keyring changed | 1; original uri_not_admitted at stanza2/line8, ubuntu_security, omitted4/1 |
| P1 adds Enabled:yes as security line12 | 1; same URI refusal at line8, omitted5/1 |
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

Focused coverage must isolate each profile field/shape condition, invalid URI/raw
format and structural bypass, record ordering/failure before children, unchanged
successful command set/options and later update failure. Existing exhaustive emitter
cases need not be copied wholesale: exercise the new call site's count/type,
write/flush and persistent-failure paths with the reviewed fixture. Use synthetic
sources, environment and child results; no actual local APT/network/write operation.

Selected intended red is ONLY
`AptSecurityProfileTests.test_reviewed_split_profile_is_admitted`, on literal P1.
Before its decisive assertion, establish valid host/read/runner prerequisites;
if current main returns1, require the exact old runner/refusal/unsupported outcome,
no children and the one source read. Then require0 with the exact message
`intended assertion red: CI_SECURITY_SPLIT_PROFILE_NOT_ADMITTED`.
Expect one behavioral assertion FAIL, zero ERRORs/skips. No missing API/fixture,
syntax/dependency/decoding failure is red. No NI stub is necessary or authorized.

## 5. Gates and publication

Full independent contract and test cycles close before this red. A fresh isolated
implementer then changes only tools/prepare_ubuntu_apt.py, followed by full code
review and relevant tooling/schema/scaffold checks. Coordinator owns docs, shared
metadata and publication. No unreviewed tests are imported/run/published.

Keep all21 app-bound contexts and19 workflow declarations unchanged, plus all prior
protections. Use a focused prerequisite PR and normal protected merge. Inspect all
nine actual hosted preparation lanes, final-head producers and the admission event.
Report whether this profile was exercised; mirror-only successes do not establish
new-profile platform execution. Do not rerun for luck or invent missing source facts.
After this prerequisite merges, resume PR126 with fresh checks and normal merge,
then PAUSE. No WORKFLOW-006/007 or stopped product work is authorized here.
