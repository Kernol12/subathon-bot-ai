# Continuous integration

One workflow, [`.github/workflows/ci.yml`](../.github/workflows/ci.yml), runs on every
PR to `main`, every push to `main` and on manual dispatch. It uses the same CMake
presets as local checks. A passing job establishes only its bounded scope; inspect the
latest PR head before merge.

## Jobs

| Job | Scope | Presets |
| --- | --- | --- |
| `scaffold` | Infrastructure build, Python tooling tests and repository checks. Not product acceptance | `scaffold-debug` |
| `schema-validation (ubuntu-22.04)`, `schema-validation (ubuntu-24.04)` | Real repository schemas and examples with the pinned validators on CPython 3.10 and 3.12 | `schema-validation-debug` |
| `core` | Every pure GoogleTest suite in one build: time credit, exact numeric, timer lifecycle, pause policy, time debt, timer editorial, milestone progression, effective permissions, rate snapshots, receipt intake, contribution evidence and application plans. Pinned Boost 1.83 | `core-acceptance-debug` |
| `qt` | Qt 6.12 suites with one shared toolchain setup: all real-SQLite storage suites and the application service, password hashing and session resolution against the selected libsodium, and translation catalogs | `sqlite-reference-capture-debug`, `password-hashing-debug`, `session-resolution-debug`, `translation-catalog-debug` |
| `native` | Corrected Drogon SDK: the 44-method native WebSocket loopback profile, then the host lifecycle baseline build and registration check | `native-websocket-profile-debug`, `api-host-native-debug` |

The single-suite presets (`exact-numeric-debug`, `time-debt-debug` and so on) remain
for focused local runs; `core-acceptance-debug` selects the same suites together.

## Rules

- Never use `WILL_FAIL`, continue-on-error, skips or failure inversion to hide a
  failure. Every stable suite fails on empty discovery. The `core` job checks that
  each of its twelve suites is discovered with its labels and timeouts intact.
- Host lifecycle baselines stay enabled as intentional red: the `native` job compiles
  and inspects their registration but does not run them.
- The `native` job runs the WebSocket profile first, so its package and loaded-map
  gate sees the same host state as before the Qt and Boost packages are installed.
- Actions and dependencies stay pinned: the checkout action by commit, Qt 6.12.0, ICU
  and libsodium archives by SHA-256, Boost 1.83 headers by package version. Qt 6.12
  is required; Ubuntu's older system Qt is not substituted.
- Nothing is cached between runs. Add caching only with a reviewed design, because
  the native recipe seals every SDK file before use.
- Add a suite to the matching job when its module lands. Add Windows/MSVC, Qt/QML,
  browser/OBS, deployment and performance jobs as their harnesses exist. Generic CI
  cannot substitute live provider, actual OBS or compositor evidence.

## Required checks

Ruleset 26170263 protects `main`: PRs with resolved threads, no bypass actors and
required status checks. The required contexts are the job names above plus
`GitGuardian Security Checks`:

```text
GitGuardian Security Checks
scaffold
schema-validation (ubuntu-22.04)
schema-validation (ubuntu-24.04)
core
qt
native
```

Until 2026-11-14 the ruleset required 21 contexts, one per former workflow file. The
change that introduced `ci.yml` must update the ruleset to the seven contexts above
after its own PR shows them passing and before it merges; otherwise PRs wait forever
for the retired contexts. Renaming, adding or removing a job needs the same update in
the same change. The earlier procedure and receipts are in the
[CI-002 contract](contracts/ci-setup-gates-v1.md).

## Ubuntu package preparation

The `core`, `qt` and `native` jobs first run `tools/prepare_ubuntu_apt.py`. It requires Ubuntu 24.04,
`GITHUB_ACTIONS=true` and `RUNNER_ENVIRONMENT=github-hosted`, and refuses unknown
`ubuntu.sources` layouts before commands or writes. It is for disposable hosted
runners; local and self-hosted invocation is refused. The SQL fixture reproduction
check remains immediately after checkout, before this preparation.

After the existing host guards and single source read, preparation emits a bounded
`CI_APT_V1` runner record using the already-read OS data and the two named image
environment values. A refused layout emits a fixed-label `source_refused` record
before raising, without printing the source or starting any command. Unknown values
stay unknown. Record-emission or source-decoding failure stops preparation; a
runner record does not admit an unfamiliar layout. The exact format, failure
semantics and evidence limits are in the [CI-002 contract](contracts/ci-setup-gates-v1.md).

The [CI-003 profile](contracts/ci-noble-security-source-v1.md) also recognizes an
exact two-stanza Noble archive/security layout. It checks all five required fields
in both stanzas, the suites, components and Ubuntu keyring, and the original decoded
line grammar. Only a complete match emits the fixed `source_admitted` record with
profile `noble_split_security_v1`, before any command or write. This record confirms
profile validation; later acquisition can still fail. Other layouts use the existing
validation and refusal rules. The contract defines the exact literal URI allowlist,
column-zero comments and empty separators.

Best-effort resolver status, the original mirror list and four IPv4 hostname lookups
are logged with command status and captured output, each with a ten-second deadline.
These are diagnostics only. Preparation prefers the kernel.org Ubuntu mirror with
canonical archive/security fallbacks, preserving Ubuntu signing trust, exact package
versions and SDK admission checks. It changes no resolver, proxy or other source file.
For the split profile, the security pocket is fetched directly from its validated
`security.ubuntu.com` URI, outside that mirror list. An outage there fails the strict
update; preparation adds no fallback rewrite or process retry.

Checked writes set that mirror list, APT's native `Acquire::Retries "1"`, 15-second HTTP/HTTPS
timeouts and strict update errors. A checked `apt-get update` has a 300-second process
deadline; write/update failures stop the job. Other configured repositories remain
enabled and their update failures also fail preparation. No additional process retry
or update-output capture is added. Existing job timeouts bound
the whole run. Mocked tooling tests establish control flow; authenticated acquisition
and the selected-host jobs still require actual current-head CI.
Hosted evidence must say whether the new admission record appeared. A green run
without it establishes only "profile not platform-observed". Any failed required
lane blocks merge; a remaining source refusal needs a separately reviewed refinement.
Local passes or a green run do not show that the originally refused PR126 host
matched this profile; its unobserved values remain unknown. Rerunning for luck or
guessing hidden source values does not establish compatibility.

## Repository-schema checks

The separately reviewed TESTCLEAN-001 boundary replaces the generic arbitrary-package
API and native/library-fact harness with real project schema/example validation. Its
replacement workflow must install the unchanged 20 hash-locked wheels in an isolated
venv, retain exact dependency metadata admission, then unconditionally run every
actual schema/declared example and independent valid/invalid component case. The
progress editor schema is a separate structural check, not runtime DAG/review proof.
No native artifact, kernel owner, assembly audit or fault child is a current check.

Changed tests/oracles/source need separate review and actual hosted execution;
record those results in the focused task PR. Old SDK/native/public results remain valid historical proof
of their original surface; they are not renamed replacement passes. Missing libraries,
skips, empty discovery, fake validators or unsupported reference retrieval cannot
produce a green schema result. All runtime/provider/platform/release gates remain.
