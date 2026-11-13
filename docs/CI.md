# Continuous integration

GitHub workflows use the same selected CMake presets as local checks. A passing
lane establishes only its bounded scope. Exact historical runs and reviewed
revisions are linked from each task handoff; inspect the latest PR head before merge.

The nine APT lanes first run `tools/prepare_ubuntu_apt.py`. It requires Ubuntu 24.04,
`GITHUB_ACTIONS=true` and `RUNNER_ENVIRONMENT=github-hosted`, and refuses unknown
`ubuntu.sources` layouts before commands or writes. It is for disposable hosted
runners; local and self-hosted invocation is refused. The SQL fixture reproduction
check remains immediately after checkout, before this preparation.

After the existing host guards and single source read, preparation emits a bounded
`CI_APT_V1` runner record using the already-read OS data and the two named image
environment values. A refused layout emits a fixed-label `source_refused` record
before raising, without printing the source or starting any command. Unknown values
stay unknown. Record-emission or source-decoding failure stops preparation; a
diagnostic record does not admit an unfamiliar layout. The exact format, failure
semantics and evidence limits are in the [CI-002 contract](contracts/ci-setup-gates-v1.md).

Best-effort resolver status, the original mirror list and four IPv4 hostname lookups
are logged with command status and captured output, each with a ten-second deadline.
These are diagnostics only. Preparation prefers the kernel.org Ubuntu mirror with
canonical archive/security fallbacks, preserving Ubuntu signing trust, exact package
versions and SDK admission checks. It changes no resolver, proxy or other source file.

Checked writes set that mirror list, APT's native `Acquire::Retries "1"`, 15-second HTTP/HTTPS
timeouts and strict update errors. A checked `apt-get update` has a 300-second process
deadline; write/update failures stop the job. Other configured repositories remain
enabled and their update failures also fail preparation. No additional process retry
or update-output capture is added. Existing job timeouts bound
the whole run. Mocked tooling tests establish control flow; authenticated acquisition
and all nine selected-host jobs still require actual current-head CI.

| Workflow | Automatic PR/main scope |
| --- | --- |
| native-websocket-profile.yml | Corrected SDK preparation/seal, passive ELF and scrubbed loaded-map gate, then all 44 synthetic loopback native methods |
| scaffold.yml | Infrastructure build, Python tooling, repository/progress checks |
| acceptance.yml | Reviewed time-credit acceptance and regression cases |
| rate-snapshots.yml | All 53 preserved pure rate cases, numeric regressions and infrastructure; pinned Boost 1.83 |
| exact-numeric.yml | Reviewed exact decimal/final conversion and regression cases |
| timer-lifecycle.yml | Reviewed pure Main/Grace reducer |
| pause-policy.yml | Reviewed independent pause reasons and delays |
| receipt-intake.yml | Reviewed receipt service with primitive simulation storage |
| contribution-evidence.yml | Reviewed eligibility/linkage with primitive simulation storage |
| application-plan.yml | Reviewed owned captured-plan transformations |
| time-debt.yml | Reviewed 32 pure debt proposals/composition with unconditional real acceptance |
| sqlite-writer.yml | Qt 6.12 whole-suite SQL acceptance, including account/session storage |
| schema-validation.yml | Real repository schemas/examples with unchanged pinned libraries on CPython 3.10/3.12 |

The SQLite writer lane now runs all22 unchanged reviewed cases plus3 ownership regressions on every PR/main
and manual dispatch. STORAGE-001 implements the original reviewed/red baseline;
[actual acceptance evidence](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/storage-001-ci.md) records the first passing
Ubuntu run. The existing `sqlite-writer-build` check name is retained. Historical
stub-red and the earlier build-only scope remain in baseline evidence. No failure
inversion, skipped baseline, WILL_FAIL or continue-on-error is allowed.

Native/translation presets share `SUBATHON_BUILD_ACCEPTANCE_TESTS` and explicit
`SUBATHON_ACCEPTANCE_SUITES` selection, preserving targeted dependencies and all job
names. Old enabled per-feature caches require a fresh configure. Qt suites invoke
each existing executable without slot filters; GoogleTest keeps built-in per-case
processes and full labels. Translation runs ordinary unittest discovery with the
same real Qt verifier. CI generally derives inventories from source rather than
maintaining duplicate method/count lists. The rate lane additionally pins the 53
reviewed methods while comparing their source names with complete discovery; this
explicit promotion check preserves the baseline. Semantic fixture integrity,
independent literal vectors and requirement maps remain.
Every stable suite selection fails on empty discovery and runs unconditionally.
The SQL job explicitly selects `sqlite-reference-capture;sqlite-account-sessions`:
all stable storage suites, including account/session storage, run ordinary acceptance.
Rate methods also run unconditionally as ordinary acceptance; original stub-red
evidence stays historical. Host lifecycle baselines remain enabled intentional red
with build/discovery-only CI. No failure inversion or blanket continuation hides
unimplemented behavior. [Test strategy](TEST_STRATEGY.md) documents selection
and the distinction between method coverage and suite-level CTest nodes.

The SQLite lane downloads official Qtbase6.12.0 and ICU archives with fixed SHA-256
hashes, extracts with bsdtar, and uses the explicit prefix. Core/Sql/Test compilation and actual writer event-loop operations are checked automatically;
the QSQLITE plugin file is verified and the actual behavior cases load/open it
on disposable files in ordinary automatic acceptance. This adds no installer
action, automatic local download or committed binary. Qt6.12 remains required;
Ubuntu's older system Qt is not substituted. The actual Ubuntu acceptance workflow passed for STORAGE-001; latest-head checks
remain required before every merge. Local archive-consumer evidence alone is insufficient.

The checkout action remains pinned to official v7.0.1. Numeric-dependent stable
lanes retain pinned Boost1.83 Ubuntu headers. System runner/compiler/runtime package
versions are recorded in actual logs rather than called a reproducible OS image.

The native WebSocket lane keeps the `native-websocket-profile-build` context and
exact eight Ubuntu package checks. Its [recipe](../tools/native_websocket_ci.py)
prepares the pinned, ordered corrected sources in a fresh disposable prefix,
retains notices/provenance and seals every SDK file before configuration. A passive
ELF closure is compared with the scrubbed probe's complete loaded map before the
ordinary 44-method loopback suite can run. Native failures fail CI; no inversion,
skipping or continuation supplies acceptance. Local native44 and the first selected Ubuntu native44 run pass; the recipe has
independent approval. [PR110](https://github.com/Kernol12/subathon-bot-ai/pull/110)
records the actual run and remaining review/merge gates. Ubuntu
artifacts are assessed on that host, separately from the admitted local Arch set.
The real application host, TLS/proxy routing, authoritative clock, authentication,
synchronization and platform/release support remain separate gates.

Add Windows/MSVC, Qt/QML, browser/OBS, deployment/restore and performance jobs as
their harnesses exist. Generic CI cannot substitute live provider, actual OBS or
compositor evidence. These release gates remain in the task/acceptance map.

Ruleset26170263 was inspected on2026-11-01: main requires PRs and resolved threads,
zero GitHub approvals, no bypass actors, and no required-status-check rule. Written
workflow gates still require every applicable latest-head check; adding a workflow
does not configure GitHub enforcement. Recheck live rules/reviews before each merge.

CI-002's [reviewed inventory and settings procedure](contracts/ci-setup-gates-v1.md#4-reviewed-required-check-inventory-and-settings-procedure)
bind all 21 current check contexts to their source apps. On 2026-11-13, after full
reviews and 21 successful current-head checks at `d252201`, ruleset26170263 was
updated to require those contexts. Separate full and effective-main readbacks
verified all 21 context/app pairs and preserved every prior rule and parameter,
including the empty bypass list. The [hosted and administration receipt](https://github.com/Kernol12/subathon-bot-ai/pull/622#issuecomment-6847660001)
records the exact reviewed payload and observations. Fresh current-head checks,
reviews and normal protected merge requirements still apply.
Adding, renaming or removing a job, matrix row or security context needs a reviewed
inventory and settings update in the same change. With the rule enforced,
renaming or removing a required context without that update leaves PRs waiting for
the old context and blocks their merge.

CORE-005 promotes the original32 reviewed time-debt cases to unconditional
ordinary acceptance while retaining the time-debt-build check name. Built-in
GoogleTest discovery registers each case in its own process.
[Actual CI](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/core-005-ci.md) records passing Ubuntu32case execution; original
stub-red remains a historical prerequisite, not a present failing feature suite.
This remains bounded pure arithmetic/composition, not complete refund/application
or release proof. No failure inversion or baseline assertion change is permitted.

STORAGETEST-010 established a separate reviewed24-case work baseline with genuine
stub-red, keeping build/discovery distinct from passing old25 behavior; its
[historical CI assessment](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/storagetest-010-ci.md) remains valid. STORAGE-010
now configures `sqlite-work-debug` to run both suites unconditionally (old25 plus
new24) on PR/main/manual, retaining the stable job and ordinary failure visibility.
Its [implementation CI assessment](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/storage-010-ci.md) records actual
results and remaining gates. Source023451 is independently approved9216d8d; actual Ubuntu41300728417 at
a1e3a89 passed new24/old25/probe1 with verified selected archives and exact
registration. Latest-head CI still applies before merge; these49 cases do not
establish product schemas/recovery/platform/provider release behavior.

The migration catalog suite runs all 24 reviewed cases as ordinary acceptance with
Qt-native whole-suite discovery in the SQLite lane. Its original stub-red and the
former seven-slot driver diagnostic remain in the
[archived baseline evidence](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/storagetest-011-ci.md).
Redundant environment programs are removed only through separately approved cleanup;
real writer/work/migration behavior, dependency configuration and failure visibility
remain required. Missing dependencies, skips or empty discovery are not acceptance.

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
