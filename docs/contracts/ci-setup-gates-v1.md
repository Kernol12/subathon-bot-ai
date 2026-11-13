# CI setup and merge gates — v1

CI-002; specification v0.45 §§17–18; [task brief](../tasks/CI-002.md).
This is authored contract material awaiting independent review, not approval,
implementation, an observed red run, hosted recovery or an applied settings change.
It refines [CI-001 recovery revision 2](../tasks/CI-001.md) and
[bounded coordination v2 §5](bounded-coordination-v2.md#5-coordinator-practice-and-ci).
WORKFLOW-006 policy remains inactive; the historical 120-task exemptions and all
existing substantive review gates remain unchanged. WORKFLOW-007 is not a dependency.

## 1. Scope, incident and frozen behavior

The immediate incident is [PR126 triage](https://github.com/Kernol12/subathon-bot-ai/pull/126#issuecomment-6840078428):
`milestone-progression-build`, [run43336339986/job131593351306](https://github.com/Kernol12/subathon-bot-ai/actions/runs/43336339986/job/131593351306),
source `42462d165a5aa90f92eb3625941bcda163f178c4`, refused with
`unsupported Ubuntu APT source layout` before configuration writes, update, build or
acceptance. Public runner facts: Ubuntu24.04.5, image20261111.127.1, cloud AWS.
**The offending field is unknown.** Cloud/image classification does not establish a
source variant or justify normalizing a source. No unclassified retry is authorized.
PR126 remains parked; CI-002 neither changes its approved source nor resumes product work.

Preserve all eight methods and EVERY assertion in
`tests/tooling/test_prepare_ubuntu_apt.py`, including the existing mocked process and
file safety barriers. Preserve the CI-001 accepted/rejected source language, exact
mirror/configuration bytes (including `Acquire::Retries "1";`), host guard, signing
trust, pins, diagnostics, successful command set and order. In particular:

- Only `GITHUB_ACTIONS=true`, `RUNNER_ENVIRONMENT=github-hosted`, OS `ID=ubuntu`,
  `VERSION_ID=24.04` admit work. Existing distinct guard messages remain.
- Exactly one `Path('/etc/apt/sources.list.d/ubuntu.sources').read_text(encoding='utf-8')`
  supplies validation and diagnostics. No alternate source read, reread, source dump,
  rewrite, environment dump, network metadata lookup or command to discover identity.
- Unknown/missing/mixed/duplicate/continued URIs, direct Azure URIs, unknown fields,
  malformed lines, empty input and `Enabled: no` still refuse. On layout refusal there
  are **zero subprocess calls and zero configuration writes**, including diagnostics.
- On admitted input the seven existing best-effort diagnostics keep their commands,
  `check=False, capture_output=True, text=True, timeout=10`, labels and output behavior.
  Two checked `sudo tee` calls retain their bytes, `text=True`, `stdout=DEVNULL`,
  inherited stderr and 10-second deadlines. Only the update gains capture/retry logic.
  No repeat writes/diagnostics on retry. No additional command or shell is permitted.
- No retry around installs, downloads, SDK admission, schema/configure/build/tests or
  a workflow/job rerun. Existing vendor download retry flags remain unchanged and
  are outside this helper's process-call bound. No cache, selective CI, hidden retry,
  continue-on-error, failure inversion, source relaxation or new dependency.

This contract adds evidence and a deliberately narrow update retry. It does not
promise that the incident layout will become accepted. Any layout expansion needs
actual source evidence, a separately reviewed contract refinement and independent
baseline review; do not guess the unlogged field or weaken the frozen assertions.

## 2. Callable surface and diagnostic wire format

Keep `main() -> int`, `prepare_apt() -> None`, `validate_sources(source: str) -> None`
and `diagnostic(command: list[str]) -> None` loadable, with their existing call shapes.
No public root/skip/retry/command knobs or new CLI flags. Helpers may be private;
future tests must exercise these existing entry points, not require an unreviewed API.
`main()` still returns0 only after a successful update and returns1 on guarded setup,
I/O or subprocess failure, reporting `Ubuntu APT setup failed:` on stderr. Source
refusal still raises RuntimeError containing `unsupported Ubuntu APT source layout`.
New errors contain only fixed labels, not captured output or source values.

Every NEW record is one stderr line: literal `CI_APT_V1 `, followed by JSON serialized
with `sort_keys=True, separators=(',', ':'), ensure_ascii=True, allow_nan=False`,
then exactly LF, flushed before proceeding. No timestamps, paths, arbitrary field
names/values, exception repr, raw URI, credentials or output excerpts in these records.
Keys below are exact; no optional extra keys. JSON null means unknown/unavailable;
unknown values must not be relabeled as a known host/layout. No raw ANSI/control
character can become a terminal instruction. Each record is at most4096 ASCII bytes
including prefix/LF; bounded enums/counts below fit without truncating JSON.
Legacy human diagnostic output is separate and retains CI-001 behavior; its historical
full captures are not a credential-redaction or memory-bound guarantee. Never feed a
refused source into those commands.

### 2.1 Runner record

After host guards pass, before source validation (and hence before any subprocess or
write), emit exactly one `runner` record using the SAME OS-release mapping already
read for the guard and only the two named environment values below:

```json
{"cloud":"unknown","event":"runner","image_os":"ubuntu24","image_version":"20261111.127.1","os_id":"ubuntu","os_patch":"24.04.5","os_version":"24.04","redacted_fields":[],"unknown_fields":["cloud"],"v":1}
```

This is a synthetic example, not newly observed helper output. `os_id`/`os_version`
are the admitted constants. `os_patch` is the numeric `24.04` or `24.04.N` prefix of
OS `VERSION` only when its entire value matches
`24\.04(?:\.[0-9]{1,3})?(?: LTS \(Noble Numbat\))?`; otherwise null.
`image_os` is environment `ImageOS` only if exactly `ubuntu24`; otherwise null.
`image_version` is `ImageVersion` only if its entire value matches
`[0-9]{8}(?:\.[0-9]{1,6}){1,2}`; otherwise null. Matching is ASCII and case-sensitive with
fullmatch, without stripping, decoding escapes or removing control characters first.
Missing/empty inputs produce null and an `unknown_fields` entry. Nonempty nonmatching
inputs produce null and a `redacted_fields` entry. Those arrays use sorted output key
names (`image_os`, `image_version`, `os_patch`); cloud always adds `cloud` to unknown.
No claim of live cloud detection: absent a reviewed producer, runtime cloud is always
`unknown`. AWS above is a historical job-setup fact, not a fact inferred by this helper.
Missing image facts do not relax or fail the host guard, and need no further I/O.
Host/OS-read refusals retain their existing errors and no commands/writes; they do
not emit a runner record fabricated from unavailable/admission-failed identity.

### 2.2 Source refusal record

On layout refusal emit exactly one `source_refused` record before raising the existing
RuntimeError. It reports the first applicable reason, not source text. Example:

```json
{"event":"source_refused","field":"URIs","fields_omitted":5,"line":2,"reason":"uri_not_admitted","stanza":1,"stanzas_omitted":0,"v":1,"value_class":"other"}
```

Indices are one-based physical line and stanza, using `str.splitlines()` as the
existing validator does; comments with leading whitespace are ignored, blank/whitespace
lines separate stanzas, CRLF is accepted. For evidence counting, a stanza is a maximal
nonblank block with at least one noncomment line; comment-only blocks do not count.
`fields_omitted` counts nonblank noncomment lines in the selected stanza except the
reported physical line (all such lines for a missing-field reason). It includes
malformed/continuation/duplicate lines, without printing them. `stanzas_omitted` is
the number of other such stanzas. Counts are integers, not estimates; no list of
omitted values is retained in public output. Empty/comment-only input has null
stanza/line/field/value_class and both counts0. This scan reuses the original string.

Preserve existing refusal precedence: first structural error in physical order
anywhere in the file; only if there is none, first semantic refusal in stanza order,
URIs before Enabled in each stanza. For a noncomment line, structural priority is
leading whitespace, missing colon, unknown field name, duplicate known field.
Known names are exactly `Types`, `URIs`, `Suites`, `Components`, `Signed-By`,
`Architectures`, `Enabled`. Known field spelling may be emitted; any unknown name is
`"<unknown>"`. Continuation/malformed-line field is null. Never echo an unknown name,
line, URI, keyring path or otherwise unreviewed source value.

| reason | field / line | value_class |
| --- | --- | --- |
| `continuation` | null / offending line | null |
| `malformed_line` | null / offending line | null |
| `unknown_field` | `<unknown>` / offending line | null |
| `duplicate_field` | known name / second occurrence | null |
| `empty_source` | null / null | null |
| `missing_uri` | `URIs` / null | `missing` |
| `uri_not_admitted` | `URIs` / its line | `empty` if stripped value empty; `multiple` if whitespace splitting yields >1 token; otherwise `other` |
| `enabled_not_admitted` | `Enabled` / its line | `disabled` for exactly stripped `no`; otherwise `other` |

A stanza with exactly one admitted URIs value and absent or exactly stripped `yes`
Enabled remains accepted; other known fields retain CI-001 treatment. This is a guard
for one reviewed runner layout, not a general deb822 parser or a new validator of
suite/keyring semantics. APT's own authenticated update remains mandatory.

## 3. One narrowly classified process retry

Only `['sudo', 'apt-get', 'update']` may run twice. Each invocation uses
`subprocess.run(check=True, capture_output=True, text=True, timeout=300)` without a
shell; no environment/locale mutation. Capture `CalledProcessError` streams as well
as completed results. Valid transcript text is ASCII plus LF; CRLF is converted to LF
for classification only. Any remaining control/non-ASCII character prevents retry.
Native APT `Acquire::Retries "1"` still retries failed files internally. Thus **at
most two update process calls** is not a bound of two HTTP requests, mirror accesses
or total network attempts. Configuration and original overall job deadlines remain.

### 3.1 Complete, strict allowlist

A first invocation qualifies only if it completed normally with exit100, both streams
are present as strings, each is <=16384 UTF-8 bytes, each has <=256 lines, every line
is <=1024 bytes, and the ENTIRE transcript matches the following grammar. No substring
search for an isolated DNS phrase can admit a mixed failure. No stderr is inherited
from the update. A missing stream on failure is unclassified, not an empty success.

The only retry signature is `transient_dns`, the exact English APT temporary-resolution
error. No connection timeout, HTTP status, permanent `Could not resolve`, signing or
integrity error is allowlisted. Restrictive formatting is intentional: untranslated,
new, third-party, mirror-method or otherwise different output fails closed. This
contract does not claim every hosted APT error uses the allowed grammar.

Grammar notation: `H` is exactly one of `mirrors.edge.kernel.org`, `archive.ubuntu.com`,
`security.ubuntu.com`; `S` is exactly `noble`, `noble-updates`, `noble-backports`,
`noble-security`; `N` is `[1-9][0-9]{0,3}`. `B` is literal `https://H/ubuntu` with
optional trailing slash. `U` is literal `https://H/ubuntu/dists/S/InRelease` with
no userinfo, port, query, fragment, percent encoding or other path. Matched H/S values
in one entry must agree. No general URI parser accepting alternate forms is required.

Allowed stdout consists solely of zero or more lines `Ign:N B S InRelease`,
`Hit:N B S InRelease`, or two-line blocks

```text
Err:N B S InRelease
  Temporary failure resolving 'H'
```

followed by exactly one `Reading package lists...` line. At least one Err block is
required; each N identifies one consistent B/S tuple, may have repeated Ign lines,
and has only one terminal Hit or Err. Every Ign must ultimately have a terminal
record; no dangling reason line is accepted. Blank lines are allowed and ignored.
No Get/progress/other stdout line is allowlisted in v1.

Allowed stderr consists solely of one or more

```text
E: Failed to fetch U  Temporary failure resolving 'H'
```

followed by exactly one

```text
E: Some index files failed to download. They have been ignored, or old ones used instead.
```

Blank lines are allowed and ignored. Every failed URL's H/S must match an Err block,
every Err must have a failed URL, and there are no duplicate failed URLs. A recognized
DNS error is not evidence that stale indexes are acceptable; exit100 remains a failed
attempt. Generic summary alone, warnings instead of errors, extra suffixes, malformed
correspondence, unknown app/tool text or any extra line refuses retry.

`transient_dns` is emitted only for that complete grammar. Otherwise classify as
`mixed` if at least one exact Err+reason block or failed-fetch line is present together
with any unmatched/forbidden text or broken correspondence; as `unclassified` if no
such exact DNS item exists. Explicit signing/integrity indicators anywhere in either
stream take precedence and classify `integrity`: case-sensitive substrings
`NO_PUBKEY`, `BADSIG`, `EXPKEYSIG`, `Hash Sum mismatch`, `not signed`,
`signatures couldn't be verified`, `Certificate verification failed`. This list improves
explanation, not eligibility: all other integrity wording is still outside the grammar.
Check size/encoding/completeness before this classification; overflow is `output_limit`,
invalid text/type is `unclassified`. Never classify a truncated prefix as complete.

### 3.2 Receipts, bounds and first-failure preservation

Emit one `update_attempt` record for each invocation, even success, before returning
or deciding another call. Exact keys/example for the positive transcript in §5:

```json
{"attempt":1,"classification":"transient_dns","event":"update_attempt","evidence":[{"host":"mirrors.edge.kernel.org","reason":"temporary_dns","suite":"noble"}],"evidence_omitted":0,"exit_code":100,"retry":true,"stderr_bytes":225,"stderr_lines":2,"stdout_bytes":203,"stdout_lines":4,"v":1}
```

Byte counts above include the final LF of each line in §5's literal first-attempt
streams. Counts describe the complete received streams before CRLF conversion, not
characters or printed excerpts. Lines use splitlines(). Evidence is the first eight
unique complete Err H/S pairs in stdout order; only grammar-admitted host/suite
literals can appear, with the fixed reason `temporary_dns`. `evidence_omitted` is the
number of further unique admitted pairs not shown. Do not print raw captured text,
including unknown lines, URI strings, IPs, credentials, prompts or tracebacks. All raw
lines are withheld, so byte/line counts cover omitted raw output; the evidence array
is a safe semantic projection, never a truncated raw log. For non-transient classes,
evidence may still show exact complete DNS pairs; absence is `[]`, omitted0.

The classifications are `success` (exit0), `transient_dns`, `mixed`, `integrity`,
`unclassified`, `output_limit`, `timeout`, `unavailable`. Nonzero exit other than100
is `unclassified` regardless of phrase; signal return codes are failures. Timeout
and OSError override output classification, use null exit_code and `retry:false`;
Unicode decode failure is `unclassified`, null exit_code. Unavailable covers missing
sudo/apt command or another launch OSError. On success no failure classification can
turn exit0 into recovery evidence; `retry:false`, evidence empty. Missing streams
on exit0 have byte/line counts null. Missing/non-string failure streams have null
counts for those streams. Bytes supplied by TimeoutExpired are counted directly,
not guessed decoded; they never qualify for retry. Non-transient evidence extraction
is bounded to size-admitted text; otherwise empty.

`retry:true` occurs ONLY for eligible first-attempt `transient_dns`. Flush its receipt
successfully BEFORE starting attempt2; an emission failure must fail, not retry without
evidence. The second call uses identical argv/options/configuration, immediately with
no sleep or diagnostic/rewrite replay. There is no third call, even if attempt2 has
the same transient signature; its receipt has retry:false. Retain the first receipt
and second receipt in chronological order; never overwrite the first with the last
attempt or relabel a failed first attempt as success. Only exit0 of attempt2 permits
main0; every failed final attempt returns main1 with the existing failure prefix.
The transcript is not copied to a new raw artifact by this helper. The bounded safe
receipts survive in the ordinary job log; the coordinator retains them, run/job URL,
source revision and final classification in private evidence through task closure.
No new upload/artifact retention policy or public raw-log publication is introduced.

Two update deadlines total at most600 seconds of requested subprocess wait, plus the
existing seven diagnostic deadlines (70) and two writes (20): 690 seconds of requested
child wait, excluding process creation/cleanup/Python/logging overhead. A timeout on
attempt1 forbids attempt2; attempt2 timeout fails. Existing 15/80-minute job limits
remain unchanged. No tight overall wall-clock or surviving-descendant deadline is
claimed. `subprocess.run` waits/reaps the directly invoked process on normal completion;
this is not independent process-tree supervision. Unknown cleanup after timeout or
launch failure never licenses retry. Normal completed `sudo apt-get update` on the
trusted disposable runner is the narrow retry ownership assumption.

**Resource/trust limits:** subprocess.run captures streams in memory before the
16384-byte admission check; this bounds classifier input/retained public evidence,
NOT peak capture memory. The existing full source read and diagnostic captures are
also not memory bounded. Do not advertise streaming/memory/process-tree guarantees.
No Popen/thread/supervisor framework is introduced to evade legacy seams. If a hard
capture-memory or descendant guarantee is required, stop for a separate reviewed
prerequisite instead of silently importing WORKFLOW-007. Environment host flags and
OS/image strings are trusted runner facts, not cryptographic host attestation.

## 4. Reviewed required-check inventory and settings procedure

### 4.1 Inventory v1

Derived from ALL19 PR workflow declarations in main
`3c74966f243b3628a12eb760e2a96ff455717d50` and the 21 actual head contexts at
PR126 source `42462d165a5aa90f92eb3625941bcda163f178c4`. These are observed names/apps,
not proof that CI-002 has passed them. All workflows run on PRs to main (scaffold
runs on all PR targets); none has path filtering or a job-level skip. Each has one
job except the schema job's two explicit matrix rows. No workflow/pin/permission/
trust/coverage change is needed by this inventory. Paths below are under
`.github/workflows/`; Actions rows bind `integration_id:15368`, observed slug
`github-actions`. External row binds `integration_id:46505`, observed slug `gitguardian`.

| Workflow file | Job / matrix declaration | Exact check context | App ID |
| --- | --- | --- | --- |
| acceptance.yml | acceptance | acceptance | 15368 |
| api-host-baseline.yml | api-host-baseline-build | api-host-baseline-build | 15368 |
| application-plan.yml | application-plan | application-plan | 15368 |
| contribution-evidence.yml | contribution-evidence | contribution-evidence | 15368 |
| effective-permissions.yml | effective-permissions-build | effective-permissions-build | 15368 |
| exact-numeric.yml | exact-numeric | exact-numeric | 15368 |
| milestone-progression.yml | milestone-progression-build | milestone-progression-build | 15368 |
| native-websocket-profile.yml | native-websocket-profile-build | native-websocket-profile-build | 15368 |
| password-hashing.yml | password-hashing-build | password-hashing-build | 15368 |
| pause-policy.yml | pause-policy | pause-policy | 15368 |
| rate-snapshots.yml | rate-snapshot-build | rate-snapshot-build | 15368 |
| receipt-intake.yml | receipt-intake | receipt-intake | 15368 |
| scaffold.yml | scaffold | scaffold | 15368 |
| schema-validation.yml | schema-validation; runner ubuntu-22.04, Python3.10 | schema-validation (ubuntu-22.04) | 15368 |
| schema-validation.yml | schema-validation; runner ubuntu-24.04, Python3.12 | schema-validation (ubuntu-24.04) | 15368 |
| sqlite-writer.yml | sqlite-writer-build | sqlite-writer-build | 15368 |
| time-debt.yml | time-debt-build | time-debt-build | 15368 |
| timer-editorial.yml | timer-editorial-build | timer-editorial-build | 15368 |
| timer-lifecycle.yml | timer-lifecycle | timer-lifecycle | 15368 |
| translation-catalog.yml | translation-catalog-build | translation-catalog-build | 15368 |
| External GitGuardian installation | security check | GitGuardian Security Checks | 46505 |

The nine affected acquisition lanes are api-host-baseline, application-plan,
effective-permissions, exact-numeric, milestone-progression, native-websocket-profile,
sqlite-writer, translation-catalog and rate-snapshots. All21 contexts remain merge
gates; successful unaffected lanes do not substitute for a failed setup lane.

### 4.2 Bounded manual readiness and administration

This is a coordinator procedure, not a new general GitHub client, executable checker,
public CLI or authority grant to the contract/test author. Synthetic review examples
below require no live mutation. Keep fresh complete observations privately and publish
only allowed summaries. An unavailable API, insufficient permission, unknown field,
ambiguous run association or inconsistent readback is a pending gate, never success.

1. After full contract, test and code cycles, relevant local checks and actual hosted
   lanes, inspect the candidate PR's **current head**, current main and complete current
   workflow declarations. Record SHA and observation time. Paginate check-runs
   (`GET /repos/Kernol12/subathon-bot-ai/commits/{sha}/check-runs?filter=all&per_page=100`),
   statuses (`.../commits/{sha}/status` and its pagination), and Actions runs/jobs for
   that PR/head. Select each workflow's latest non-superseded PR run/attempt and each
   external check's latest run; old successful attempts cannot replace newer failures
   or in-progress work. Verify suite/run/job association, event, head SHA, workflow
   path and app ID/slug. If GitHub reports a merge-test SHA, explicitly bind it to this
   exact current PR head/base through the run; do not silently accept another SHA.
2. Require this exact inventory: every declared matrix/job produces the named context
   from its bound app, completed with conclusion **success**, on the selected current
   runs. Skipped/neutral/cancelled/timed_out/failure/pending/missing are not success.
   Check every additional reporting context/status; unknown names/apps, duplicate or
   ambiguous current producers, and a same-name status from a different source fail
   readiness pending explicit inventory review. No arbitrary app substitution, app-ID
   omission/null/-1, automatic learning, count-only comparison or all-checks claim
   from a partial snapshot. Add/rename/remove workflows or security contexts requires
   reviewed inventory/settings changes and fresh corresponding actual-head evidence.
3. Freshly GET `.../rules/branches/main`, all applicable repository/parent rulesets,
   full `.../rulesets/26170263` and any traditional branch protection. Record complete
   settings before, including conditions, bypass actors, enforcement and ALL rule
   parameters, including newly added parameters. Aggregate branch rules alone cannot
   reconstruct a ruleset. Require actual administration permission and visible bypass
   data; missing bypass_actors is not an empty list. Do not infer human GitHub approval
   from AI reviews. Existing reviews/discussion requirements still apply literally.
4. Reviewed initial observation: ruleset26170263 `Protect default branch`, target
   `branch`, enforcement `active`, ref include `["~DEFAULT_BRANCH"]`, exclude `[]`,
   bypass_actors `[]`, current_user_can_bypass `never`; deletion/non_fast_forward rules;
   pull_request parameters `required_approving_review_count:0`,
   `dismiss_stale_reviews_on_push:false`, `required_reviewers:[]`,
   `require_code_owner_review:false`, `require_last_push_approval:false`,
   `required_review_thread_resolution:true`,
   `require_extra_approval_for_unattributed_changes:true`,
   `allowed_merge_methods:["merge","squash","rebase"]`. No required_status_checks rule.
   This is a design observation, not an immutable substitute for step3. Drift needs
   comparison/review before mutation; never reset fresh protections to this snapshot.
5. Prepare a private before/after payload for **only** adding the rule below to that
   ruleset. Preserve name/target/enforcement/conditions/bypass_actors and every other
   rule/parameter verbatim as JSON values (including array order). Do not rebuild the
   PR rule from an older API schema. The new rule is:

   ```json
   {"type":"required_status_checks","parameters":{"do_not_enforce_on_create":false,"strict_required_status_checks_policy":false,"required_status_checks":[{"context":"acceptance","integration_id":15368}]}}
   ```

   The one-entry array here is notation ONLY: the real payload must contain **all21**
   table rows, one object per exact context/app pair, sorted by context; no wildcard.
   `strict_required_status_checks_policy:false` selects GitHub's loose check policy;
   this addition enforces the existing all-checks policy without adding a new always-
   up-to-date branch requirement. Fresh current-head checks/reviews are still required.
   `do_not_enforce_on_create:false` adds no creation exemption. Existing strict check
   rules discovered at step3 must not be weakened; unexpected preexisting check rules
   require reviewed reconciliation, not this blind append.
6. Review exact before/after JSON difference and the inventory as substantive settings
   material through Claude initial → Codex correction/separate verification → Claude
   closing (may be included in the implementation packet if the exact payload is
   already reviewed). Re-read head/checks/full settings immediately before applying.
   Any intervening material change invalidates that payload/readiness. Only then the
   authorized coordinator may PUT `.../rulesets/26170263` with a payload containing
   preserved writable fields plus this rule. Response-only IDs/timestamps/links are
   not update fields. If a current parameter cannot be round-tripped by the supported
   API/UI, stop and resolve through review; do not discard it to make the PUT succeed.
   Do not broaden a token, bypass, permission or repository policy to obtain access.
7. Immediately GET the full ruleset and effective main rules again. Compare against
   the reviewed after payload: all21 exact app-bound pairs, both booleans, active
   application to main, and every unrelated setting unchanged. Unexpected response,
   dropped parameter, concurrent change or missing effective rule means enforcement
   is unverified and blocks merge; do not retry a mutation or revert protections
   blindly. Reconcile with a newly reviewed narrow correction. A successful HTTP
   response alone is not enforcement evidence.
8. Inspect GitHub's actual merge requirements/eligibility on the current PR after
   readback, with latest complete checks, applicable independent approvals, resolved
   threads and actual permissions. A pending/failed real required check must remain a
   blocker; use read-only observations of a pending run if available, never introduce
   a failing production commit to demonstrate it. Retain settings/effective-rule
   readback and normal protected merge result as live evidence; do not claim a
   deliberately tested denial without one. Never force/admin-bypass merge. Recheck
   after every new push. Unavailable administration leaves this gate explicitly open,
   while independently authorized repository work can finish.

GitHub may accept skipped/neutral checks at the platform gate; the repository's
reviewed all-checks procedure above requires actual success. App binding establishes
an expected GitHub producer, not that producer's internal security or test quality.
API readbacks and normal protected merge are live settings evidence; mocks prove none
of them. No autonomous probe/mutation runner is added for this procedure.

## 5. Independent literal examples and future test seams

These expectations are authored from this contract, not generated by the helper.
They are not executed tests. Keep tests focused and additive; no blanket copies of
workflow pins/bodies. Source review and actual hosted lanes own those unchanged bytes.

### A. Admitted layout and diagnostic safety

Use this exact six-line stanza, final LF included:

```text
Types: deb
URIs: mirror+file:/etc/apt/apt-mirrors.txt
Suites: noble noble-updates noble-backports
Components: main universe restricted multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
Enabled: yes
```

With guard facts and image values from §2.1, main returns0 after the original seven
diagnostics, exact two writes and a single successful update. Runner record precedes
commands; there is no source_refused record. Comments/CRLF remain admitted. The two
original CI-001 stanzas remain admitted without adding required fields.

Focused edits of A yield the literal record fields below; each main1, no commands or
writes, same one source read. Every record also has event source_refused, v1; omitted
counts are fields/stanzas in that order. Legacy failure prefix/text follows it.

| Input edit | reason | stanza / line / field / value_class | Omitted |
| --- | --- | --- | --- |
| none but replace URI by `https://u:secret@extra.example/x?token=s` | uri_not_admitted | 1 / 2 / URIs / other | 5 / 0 |
| append ` https://extra.example` to URI | uri_not_admitted | 1 / 2 / URIs / multiple | 5 / 0 |
| delete line2 | missing_uri | 1 / null / URIs / missing | 5 / 0 |
| insert second identical URIs line immediately after line2 | duplicate_field | 1 / 3 / URIs / null | 6 / 0 |
| replace line2 by `URIs mirror+file:/etc/apt/apt-mirrors.txt` | malformed_line | 1 / 2 / null / null | 5 / 0 |
| insert `X-Secret: opaque` after line2 | unknown_field | 1 / 3 / <unknown> / null | 6 / 0 |
| insert ` https://extra.example` after line2 | continuation | 1 / 3 / null / null | 6 / 0 |
| replace final yes by no | enabled_not_admitted | 1 / 6 / Enabled / disabled | 5 / 0 |
| replace entire input by `# comment\n\n` (actual newlines) | empty_source | null / null / null / null | 0 / 0 |
| A + blank line + `Types: deb\nURIs: https://third-party.example\n` | uri_not_admitted | 2 / 9 / URIs / other | 1 / 1 |

No output contains `secret`, `token=s`, `X-Secret`, `opaque`, the raw unknown line or
third-party URI. If A has a semantic bad URI and stanza2 has an unknown field, the
structural unknown-field reason in stanza2 wins, preserving validator precedence.
With `ImageVersion="20261111.127.1\n::error::secret"`, `ImageOS="ubuntu24\u001b[31m"`
and absent VERSION, runner image fields/os_patch are null, redacted_fields is
`["image_os","image_version"]`, unknown_fields is `["cloud","os_patch"]`.
No injected text/control survives. VERSION literal `24.04.5 LTS (Noble Numbat)`
produces os_patch `24.04.5`; cloud remains unknown even if an unrelated env field says AWS.

### B. Strict positive recovery and first-failure retention

First update raises CalledProcessError(100) containing exactly these streams:

stdout (four lines, final LF):

```text
Ign:1 https://mirrors.edge.kernel.org/ubuntu noble InRelease
Err:1 https://mirrors.edge.kernel.org/ubuntu noble InRelease
  Temporary failure resolving 'mirrors.edge.kernel.org'
Reading package lists...
```

stderr (two lines, final LF):

```text
E: Failed to fetch https://mirrors.edge.kernel.org/ubuntu/dists/noble/InRelease  Temporary failure resolving 'mirrors.edge.kernel.org'
E: Some index files failed to download. They have been ignored, or old ones used instead.
```

Expect first attempt receipt: classification transient_dns, exit100, retry true,
evidence exactly `[{"host":"mirrors.edge.kernel.org","reason":"temporary_dns","suite":"noble"}]`,
evidence_omitted0. If attempt2 completes exit0 with stdout/stderr `""`, expect second
receipt classification success, exit0, retry false, empty evidence, all four byte/line
counts0, and main0. Two update calls total, identical options/deadlines, each tee once.
A mock should observe that the first receipt has already been flushed at entry to
call2. Do not infer recovery from a receipt without the second successful result.
If attempt2 repeats the first failure, expect two transient_dns receipts, retry flags
true/false and main1; no third call. The first failure remains available in both cases.

### C. Refusal to retry and bounds

Starting with B's first streams, each independent edit below yields one update call,
retry false and main1 (no successful retry scripted/consumed):

| Change | Classification / reason |
| --- | --- |
| exit1 instead of100 | unclassified; no generic nonzero retry |
| stderr only `E: unexplained acquisition failure\n` and stdout empty | unclassified |
| append `E: configuration parse failed\n` to B stderr | mixed |
| append `E: Hash Sum mismatch\n` or `E: repository is not signed\n` | integrity |
| replace DNS text everywhere by `Could not resolve 'mirrors.edge.kernel.org'` | unclassified |
| use unknown host or credential-bearing URI | unclassified; no projected URI/host |
| remove all failed-fetch lines, leaving B stdout | mixed; no complete correspondence |
| stderr absent (None) | unclassified; stderr counts null |
| B stderr followed by enough literal `x` bytes to exceed16384 total | output_limit; no prefix classification |
| B stdout followed by ANSI escape or non-ASCII text | unclassified |
| raise TimeoutExpired(update,300,output=B stdout,stderr=B stderr) | timeout, exit null; partial text never eligible |
| raise FileNotFoundError("sudo missing") | unavailable, exit null |

For the maximum retry path, script B then a second TimeoutExpired at300: deadlines
remain300/300, first receipt retained, second timeout/no retry, main1. Test deadline
arguments and call bounds using mocks, not a 600-second sleep or real apt. Tee timeout
or failure never starts update. Existing diagnostic timeout remains best-effort and
cannot create a retry classification. Missing SDK/tool/configure/build/test errors
in later steps still fail their original lane, without any retry wrapper.

### D. Inventory and settings readiness examples

These are manual review oracles over synthetic observations, not a request for a
new GitHub client/test API or real administration by tests:

- All21 exact pairs in §4 complete success on the fresh associated PR runs, unchanged
  declarations and fully reviewed preserved settings: ready for the coordinator's
  gated addition, not evidence it is already applied.
- Only20 pairs (translation missing), a pending schema row, skipped success-looking
  job, cancelled latest run after an older success, or success from a previous SHA:
  not ready; identify the context/current-attempt mismatch.
- `acceptance` from app46505, GitGuardian from15368, missing app ID, or a same-name
  unrelated producer alongside the expected app: not ready, source mismatch/ambiguity.
- New/renamed workflow or external name with21 still reporting: not ready; compare
  declarations/names/apps, not counts. Review both inventory and settings first.
- After payload adds all pairs, GET drops require_extra_approval_for_unattributed_changes
  or changes required_reviewers, bypass_actors, condition, enforcement or merge methods:
  settings mismatch, enforcement unverified, no merge. Preserve even parameters absent
  from an older API documentation snapshot. An aggregate main-rules response alone
  cannot prove preservation of conditions/bypass/enforcement.
- Correct all21 pairs with integration_id omitted/null/-1, or a correct full ruleset
  not effective on main: not enforced. API denial or hidden bypass data: admin gate
  pending; never disable protection or manufacture a human approving review.

### E. Test author boundary and real gates

Future focused unittest cases may import the CURRENT helper and call main or
validate_sources with synthetic strings. Patch `platform.freedesktop_os_release`,
`os.environ`, `Path.read_text`, `subprocess.run`, stderr/stdout with flush observation;
retain the existing no-real-process/no-direct-write barriers. Script actual
CompletedProcess/CalledProcessError/TimeoutExpired semantics, including stdout/stderr.
Use literal transcript/JSON/count expectations from this contract, not production
classifier calls to calculate expected results. No real apt/DNS/sudo/file writes,
privileged fixture, package install or GitHub mutation is needed. Private helper names
or future SDKs are not acceptance seams. Settings procedure proof comes from independent
inventory/payload review and later actual readback, not a test of invented machinery.

Two legitimate intended-red assertions through the existing callable main are missing
exact source_refused/runner records on a refused input, or B expecting recovery main0
while the current single-update main returns1. No NI/stub/import error/new-symbol
failure, argparse error, missing dependency/SDK, TypeError, compile failure, skipped
test or arbitrary exit is intended red. First complete full independent contract
review, then author tests, then full independent test review, then observe actual
intended red. Fresh implementation and its full code/settings cycle follow. Retain
all CI-001 assertions and all earlier approval receipts for unchanged material.

After implementation/code review run relevant tooling/project/progress/render/scaffold
checks and all nine actual affected hosted setup lanes, plus complete latest-head
CI/source-app inspection. Record separately: authored contract, contract approval,
authored/reviewed tests, observed red, source approval, local mocked pass, hosted lane
pass, live settings before/after/effective readback, current PR gates and normal
protected merge. No single phase substitutes for another. Then the coordinator may
integrate current main once into parked WORKFLOW-005 without overwriting its claim/
reviews, complete the authorized workflow program and STOP; product/API work stays stopped.

## 6. Primary references and assumptions

- [Ubuntu noble sources.list](https://manpages.ubuntu.com/manpages/noble/man5/sources.list.5.html)
  documents deb822 stanzas and Enabled. CI-001 intentionally admits a stricter subset;
  broader APT syntax does not authorize broader runner-layout admission.
- [Ubuntu noble apt.conf](https://manpages.ubuntu.com/manpages/noble/man5/apt.conf.5.html)
  documents file-level Acquire retries. This supports distinguishing internal request
  behavior from CI-002's process retry; it does not bound total network attempts.
- [Ubuntu noble apt-get](https://manpages.ubuntu.com/manpages/noble/man8/apt-get.8.html)
  documents exit0/100. The following archived upstream mirror is cited for output
  construction, not as attestation of the deployed runner package. [APT connect](https://github.com/Debian/apt/blob/main/methods/connect.cc)
  emits the temporary DNS phrase for EAI_AGAIN;
  [upstream update](https://github.com/Debian/apt/blob/main/apt-pkg/update.cc) emits
  failed-fetch errors and the index summary under strict error mode. The deliberately
  narrow English transcript/correspondence profile is OUR admission policy, not a
  claim of a stable machine-readable APT API or observed incident transcript.
- [GitHub rules REST API](https://docs.github.com/en/rest/repos/rules)
  defines required_status_checks context/integration_id and strict policy, full ruleset
  inspection/update and Administration(write). It warns bypass_actors can be hidden
  without write access. Future API fields require preservation, not schema downgrades.
- [GitHub check-runs REST API](https://docs.github.com/en/rest/checks/runs#list-check-runs-for-a-git-reference)
  defines pagination/filter and app-bearing check observations. Run/head association
  and source review remain coordinator checks, not facts authenticated by task JSON.
- [GitHub protected branches](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches)
  explains expected-app binding, loose/strict checks and platform acceptance of
  skipped/neutral checks; the explicit success-only readiness rule is repository policy.
