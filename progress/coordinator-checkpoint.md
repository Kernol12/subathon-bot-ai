# Coordinator checkpoint

Owner: Kernol12. Issues own brief/status/prerequisites; PR comments own reviews.
Complete specification v0.45, all seven integrations and all three FX providers
remain release scope. This checkpoint grants no new authorization.

## Current work

Use the [Subathon bot ai development board](https://github.com/users/Kernol12/projects/1)
for the queue, status and owner holds. [AGENTS.md](../AGENTS.md) defines task
selection and slot limits. Issue comments retain task-specific
decisions and review chronology. History that left this file on 2026-11-27 is in the
[checkpoint archive](coordinator-checkpoint-archive.md).

## Current coordinator and advisor threads

Sessions check the sender of a publication request against this list
([AGENTS.md](../AGENTS.md), "Work protocol"). Update it when either thread is
replaced.

- Coordinator: `mcp:launch:f1ee79a5e11251893a2b1f1f060365f119d97d329a08fe8d349d0102b7fcfb52`
  ("Subathon Bot Coordinator 2026-11-24")
- Advisor: `mcp:launch:12ffde70f0dff8951bf9974306820d3a8fe79bfb5e86718338325d950115fa03`
  ("Coordinator Auto-Advisor 2026-11-23")

## Standing authorization — continuous coordination (2026-11-18)

The owner replaced the former “merge, then STOP” instruction. Continue through
both stages, reviews, observed intended red, implementation, checks, proof and
protected merge without routine permission pauses. After every protected merge,
close the issue, update its board status, and select the next task under
[AGENTS.md](../AGENTS.md).
Apply the workflow tier; report each merge briefly and continue without awaiting
a reply. The original [#278 handover](https://github.com/Kernol12/subathon-bot-ai/issues/278#issuecomment-6930279142)
remains historical technical context, not a continuing stop directive.

Since 2026-11-23, questions go to the advisor under
[docs/ADVISOR.md](../docs/ADVISOR.md), which lists what still reaches the owner.
An explicit owner stop always applies. Ask with options and a recommendation,
while continuing independent work not blocked. Pre-build/test CI infrastructure
allows one recorded rerun; routine in-scope choices, self-caught mistakes and
filing planned issues are not stops.

**Test runs.** Since [ADR-0014](../docs/decisions/0014-plain-test-runs-and-leaner-tasks.md)
a credited pass is required CI on the reviewed head and intended red is one plain
local run recorded in the PR with commit, command and failure summary. A suite that
no CI job runs joins a CI job in its implementation PR. The former standing
test-binary admission, its `readelf` rules and the pinned local runs are retired;
their records keep their recorded meaning. No merge hold or pin is set for a test
run. The #588 merge hold of 2026-11-25 is released.

Before the first review packet for either stage, run each candidate suite once
([AGENT_WORKFLOW](../docs/AGENT_WORKFLOW.md)); these runs earn no test evidence.

Test processes and their children run without a process file-size limit.
The owner’s BUILD-002 diagnostic reproduced SIGXFSZ at a 1,048,576-byte
SQLite WAL under an inherited cap. The original
`example8_twoEvents` observation remains in the ledger as a harness fault.

### Structure review of `199dc02` (owner decision 2026-11-18)

- [API-008 #292](https://github.com/Kernol12/subathon-bot-ai/issues/292) remains
  planned. Its brief now calls for one contribution operation shared by the dev
  CLI and HTTP host, by default in `subathon_reference_intake`. Keep the existing
  receipt-to-snapshot transaction boundaries, return a typed clock-reservation
  refusal, and test both callers. The CLI builds a synthetic event, calls the
  operation and prints its outcome.
- Move encode/decode code out of `application_service.cpp` or
  `sqlite_application_bundle_access.hpp` only within an authorized task that
  already changes the respective file. Use a separate commit and name it in
  that task's review packet.

## Open tasks with entries in the archive

The [checkpoint archive](coordinator-checkpoint-archive.md) holds the handover of
2026-11-21, the ADR-0012 decision for APITEST-015, the bounded authorizations of
2026-11-17 and the API-011 capability map. Its text is unchanged and keeps its
recorded meaning. Before starting or resuming one of these open tasks, read its entry
there together with the issue:

- [#288 API-006](https://github.com/Kernol12/subathon-bot-ai/issues/288),
  [#318 STORAGE-003](https://github.com/Kernol12/subathon-bot-ai/issues/318) and
  [#272 SESSION-007](https://github.com/Kernol12/subathon-bot-ai/issues/272):
  frozen contracts, pins and resume conditions in "Handover (2026-11-21)".
- [#320 STORAGE-004](https://github.com/Kernol12/subathon-bot-ai/issues/320),
  [#221](https://github.com/Kernol12/subathon-bot-ai/issues/221),
  [#774](https://github.com/Kernol12/subathon-bot-ai/issues/774),
  [#776](https://github.com/Kernol12/subathon-bot-ai/issues/776) and
  [#777](https://github.com/Kernol12/subathon-bot-ai/issues/777): order and gates in
  "Historical continuation after ACCT-007 (2026-11-21)".
- [#588 SESSION-010](https://github.com/Kernol12/subathon-bot-ai/issues/588) and its
  slices: the verified Caddy readback in "Historical continuation after ACCT-007
  (2026-11-21)" and the malformed-input stop limit under "Provider safeguards".
- [#689 APITEST-018](https://github.com/Kernol12/subathon-bot-ai/issues/689): the
  historical gate note in "Bounded owner authorizations (2026-11-17)".
- [#560 API-010](https://github.com/Kernol12/subathon-bot-ai/issues/560): the reply
  rule and its criterion in "ADR-0012 decision for APITEST-015 (2026-11-16)".
- [#292 API-008](https://github.com/Kernol12/subathon-bot-ai/issues/292) and #560:
  "API-011 capability map".
- Kept worktrees, backups and local evidence of 2026-11-21: "Local evidence" in
  "Handover (2026-11-21)". Read it before removing an old worktree or build folder.

Where the archive names a procedure that was retired later, such as the night order
or the test-binary admission, the current rules in [AGENTS.md](../AGENTS.md) and
[the workflow](../docs/AGENT_WORKFLOW.md) apply.

## Parallel work and standing CI rerun rule

- The board carries work status and the [root instructions](../AGENTS.md) set
  slot limits. Keep concurrent tasks out of each other's owned files. The
  coordinator owns shared CMake, CI, acceptance map and checkpoint.
- Serialize local test execution. Builds and
  reviews may overlap. At a provider limit, pause and retry the same provider
  on the next active turn.
- Main CI stops new work on a build or test failure. A failure in
  **Prepare authenticated Ubuntu package acquisition** before build or test
  does not stop new work. The authorized [main rerun](https://github.com/Kernol12/subathon-bot-ai/pull/714#issuecomment-6993941079)
  passed after the original APT timeout on `c8674545`.
- Until the owner reopens [#715](https://github.com/Kernol12/subathon-bot-ai/issues/715),
  a failure in that exact setup step may be rerun up to twice on fresh
  runners (three attempts total). Record each rerun on the PR. Park after
  the third setup failure.

Keep credentials and supporter data out of source, fixtures, reports and HTML.
Use synthetic or redacted fixtures. Live chat and payment side effects need
separate owner authorization.

## Standing owner rules for this path

**Contract corrections.** Every correction to a task's contract is a numbered item in
the PR description and is named explicitly in each tier A review packet. It goes to
the owner first if it adds more than one SDK function or touches a first-party header
([rule](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6909553282)). Where no contract sentence governs a behaviour that a fix must decide,
bring the owner a numbered correction before the fix. A recorded reason for a
correction states the design reason; do not claim "matches earlier behaviour" without
measuring it.

**Rulings cover one case.** A coordinator or owner ruling applies to the case at
hand; it grants no class of future corrections or permissions.

**Class audit** ([rule](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6912622673)). When a review finding is one case of a class of defect,
the author audits the whole change for that class in the same fix round and reports
the sites checked. A later finding of the same class is a non-blocking follow-up, not
a new round. Put this rule into every review packet and fix assignment.

**Failed CI runs and re-runs** ([rule](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6921048499)). An author's handback lists every failed
CI run on its branch with the cause. No author re-runs a failed required check
without the coordinator's say. The review packets name those runs. Never merge on a
green re-run of a failed check; find the cause first. A change to a frozen test is
the owner's decision. (Coordinator practice, not an owner rule: a setup failure whose
cause lies outside the change, such as an apt mirror timeout on the checkpoint PR
#674, was re-run by the coordinator after recording the cause on the PR.)

**Fix rounds (owner clarification, 2026-11-18).** Each separately reviewed revision,
including a coordinator integration revision, has its own two fix rounds. After a
fix round, verify the changed commit and the dispositions of the findings. Bring
new defects found after a fix round on that same revision to the owner before
folding them into the round. Findings from the first review of a new revision use
the new revision's fix rounds. Ask the owner before a third round. The other stop
rules continue to apply.

**C++ compile gate (owner decision, 2026-11-18).** Compile each C++ source or
test revision before sending it for review. This gate is build-only, with no
test or product execution. Report compile failures to the author as ordinary
findings. They earn no intended-red credit and do not consume a fix round.

**Stop rule of a last pass** (API-016 third pass, [decision](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6924951572)). When the owner
grants a third and last author pass: the author first lists what the fix must cover
and the correction is limited to that list; an independent investigator then repeats
the whole measurement; any further regression means no new author pass, and the
coordinator brings the owner the fallback options instead. The verification and
closing re-check cover everything since the last approved head and name every defect
the measurement found that no frozen test covers.

**Measurement before merge** (API-016 fix round 2, [decision](https://github.com/Kernol12/subathon-bot-ai/issues/616#issuecomment-6921048499)). A defect that appears only intermittently in CI is
investigated, not re-run away. After a transport fix the investigator measures the
fixed build with the same method and sample size as the investigation, and the
unmodified frozen test runs at least 20 times locally; a non-zero failure rate stops
the task and goes to the owner.

**Provider flags.** If a Codex author run is flagged for cybersecurity risk, save the
uncommitted work as a patch and ask the owner; do not retry or switch providers on
your own. If a Codex verification is flagged, stop and bring it to the owner.

**Roles and providers — default restored (2026-11-17).**
The owner revoked the temporary 2026-11-16 split. Codex authors; Claude provides
initial and closing reviews for tier A and reviews for tier B; Codex verifies.
Investigations and measurements stay with Codex. Completed or running reviews
under the temporary rule remain valid and are not redone. The next new review
uses the default. Every review/investigation is a fresh session that did not
author or fix the work under review. If the required reviewer is unavailable,
leave the gate pending and continue other authorized work; do not substitute.
A provider flag stops for owner. Record actual provider/model in every review
comment; an author never approves its work. Any other provider or model switch
needs the owner. Other standing rules remain unchanged.

**Codex model — owner decision (2026-11-18).** The coordinator and every new
Codex child use `gpt-6.2-sol`, including authors, investigators and verifiers.
The provider stays Codex, so this is within the standing authorization. Existing
running sessions, completed reviews and receipts retain their original models.

## Working rules from the owner (2026-11-15)

- `/tmp` is RAM-backed on this machine. Sessions put build directories on disk, in an
  ignored directory of their worktree (`build/TASK-ID/<session>/`) with `TMPDIR` set
  there, and follow the "Temporary files" section of the workflow. Name that
  directory in every assignment and review packet.
- Unreviewed test source stays local.
- Tier A proof record: `progress/proofs/TASK-ID.json` (and its policy entry if the task
  is not enrolled yet), in one commit after the final closing approval, immediately
  before merge. `tools/check_publication.py` requires `origin/main` as an ancestor of
  the head, so merge `main` into the branch before the proof commit.

## Rules kept from the bounded authorizations (2026-11-17)

The task-specific authorizations of that date are in the
[checkpoint archive](coordinator-checkpoint-archive.md). These parts apply to every
task and stay here unchanged:

- Since ADR-0014 a run script for one task needs no approval when it
  is routine run mechanics as [the advisor page](../docs/ADVISOR.md) defines
  them (it changes no test's isolation, needs no sandbox exception and weakens
  no check or assertion), stays in that task's ignored build folder and remains
  uncommitted and unused by CI. Escalate reusable tools, anything under
  `tools/`, committed scripts or tooling, and CI changes to the owner before
  authoring. The one exception is adding a suite to its existing CI job with
  its library-list entry, which is routine task work.
- Batch checkpoint updates into one documentation PR per stage, not per decision.

Coordinator owns any future shared CI/proof/checkpoint integration only within
new owner authorization. Build/scratch remain on disk under owned ignored task
roots; verify actual checkout/PR/session before any future authorized task.

## Settled policy and prerequisites

[Owner decisions249](https://github.com/Kernol12/subathon-bot-ai/issues/249#issuecomment-6879902355): restrictive edits need management USE+GRANT scope;
untouched out-of-scope grants may remain, changed/newly exposed rights and every
shared-role account are checked. Automation needs author USE+GRANT and separate
sender checks; recheck at execution and hold after revocation. Full249 remains open;
identity251, delivery208/232 and UI424/473 retain their scope. Prefix1–10 migration
bytes are frozen; no new migration is assigned.

Do not reverify pre-MIGRATE-004 records.
