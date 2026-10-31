# Twitch Subathon Bot — Project Specification

Version: 0.45 · AI-agent development and acceptance-test-first workflow agreed · 29 October 2026

This document consolidates the planning conversation. It is a specification, not an implementation or a claim of live integration testing.

**Decision-review status:** the proposed product, policy, architecture, platform, tooling, presentation defaults, and AI-agent development workflow reviewed in the conversation are agreed. The working title remains Twitch Subathon Bot. Choosing the final project/repository name and owner is a metadata task; engineering details and validation gates in §19 remain unfinished work, not unaccepted scope proposals.

**Status labels:** **Agreed** records requirements and designs accepted in the conversation. **Proposed** identifies additional recommendations requiring review. **Open** identifies implementation decisions or external capabilities not yet established. Broad acceptance of an architecture does not settle every library, default, or protocol detail.

## 1. Product and release scope

**Agreed**

- Open-source, self-hosted subathon system. Each streamer hosts their own installation.
- Do not require or plan a project-hosted authorization service. Installations authorize directly with providers using supported flows and, where required, installation-owned application credentials. Prefer the simplest supported setup without assuming that developer registration can always be avoided.
- One server represents one subathon; multi-tenant hosting is outside this design.
- Hosted Linux server; desktop clients support Windows and Linux.
- Supported processor architecture is x86-64 only, for the Linux server and both desktop clients. ARM builds are outside the agreed scope.
- Server installation must support Docker, Podman, and native Linux installation. Caddy is the default reverse-proxy/HTTPS setup.
- C++/Qt6 for server and desktop application. HTML/CSS/TypeScript for browser interfaces.
- All seven support integrations belong to the intended release: Twitch API/chat, betterplace public API, Patreon, Streamlabs, StreamElements, Ko-fi, and TipeeeStream.
- Authenticated betterplace integration is a possible later extension, not an initial requirement.
- Configurable event processing, rules, milestones, timed modifiers, permissions, presentation, and statistics.
- Invitation-based accounts support local username/password login and optional Twitch sign-in/linking. Recovery uses single-use recovery codes and authorized administrative/server-console recovery, without email-based recovery. One protected owner retains access-management/recovery capabilities outside normal editable roles.
- Native desktop management, browser management for helpers, a public/private viewer website, a browser overlay, and a capturable desktop overlay window.
- English/German initially for desktop, web management, and built-in viewer-site labels. Desktop/web management provide Light, Dark, and System appearance, defaulting to System; language/theme changes apply immediately and remain individual preferences.
- Release when the streamer/developer is satisfied with the complete intended scope. Implementation stages below do not reduce that scope.

**Agreed project license:** GPL-3.0-or-later for project code. Distribution of covered modified versions requires corresponding source under the license conditions; privately running a modified server does not itself require publishing those modifications. [S72] Dependencies/fonts retain their own licenses and required notices. Release packaging must include the required corresponding source/build information and notices; exact dependency/license review remains a release task.

**Agreed repository approach:** one GitHub repository contains the core, server, desktop client, web interfaces, schemas, deployment files, tests, and documentation. Keep it private during development and publish when the developer is happy with the complete release scope, as already planned. GitHub supports private/public repositories. [S75] The actual repository owner/name/location has not yet been selected or created as part of this planning work; creation/publication are separate implementation/release actions.

**Remaining metadata/implementation:** final project name and concrete GitHub repository owner/name, detailed Windows release policy, and exact package/runtime implementation. The working title remains Twitch Subathon Bot. GPL-3.0-or-later, repository approach below, installation-free Windows ZIP, Linux desktop .tar.gz/Arch PKGBUILD, selected platforms/build baselines, native server archive, Wayland/KDE support policy, server installation methods, and x86-64 architecture are settled.

## 2. Architecture

**Agreed architecture:** one authoritative server application with clear internal modules. Clients do not own integration credentials or authoritative timer/accounting state. The server continues without a desktop client connected, according to the configured disconnect policy.

| Surface | Responsibility |
| --- | --- |
| Linux server | Integrations, processing, clocks, accounting, permissions, persistence, statistics, asset delivery |
| Desktop client | Management, visual editors, preview, capturable overlay window |
| Browser management | Live operations governed by the same server permissions |
| Viewer website | Selected milestones, recent support, and statistics with configured visibility |
| Browser overlay | Read-only broadcast presentation |

| Server module | Responsibility |
| --- | --- |
| Integrations | Connection setup, credentials, inbound events, supported outbound actions, reconnects |
| Event intake | Validation, normalization, identifiers, duplicate detection, durable receipt |
| Rules/modifiers | Matching, exclusions, calculations, timed bonuses, generated events |
| Subathon state | Main/grace clocks, pause reasons, caps, end policies, time debt |
| Milestones | Sequential progression, completion retention, deficits, delayed actions |
| Contribution accounting | Eligibility, review, application plans, actual effects, reversals |
| Currency | Provider adapters, snapshots, scheduling, usage display, manual overrides |
| Identity/permissions | Invitations, accounts, roles, overrides, sessions, chat authorization |
| Presentation/statistics | Scene definitions, assets, visibility, public projections, aggregates |
| Persistence/audit | Transactions, configuration versions, recovery, migrations, backups |

The accepted storage recommendation is SQLite on local server storage, with a controlled state-writing path. End-user clients communicate with the application API, never directly with the database. PostgreSQL support is not currently a requirement.

**Agreed database access:** Qt SQL with its SQLite driver, one controlled writer thread, and separate read connections where needed for statistics. Use WAL mode with synchronous=FULL. Each Qt database connection remains within its owning thread. SQLite documents concurrent readers with one WAL writer, while Qt documents database connection thread ownership. [S60–S61] Backup/recovery behavior and database/thread implementation still require testing; acceptance of WAL does not establish completed durability or performance results.

Use a versioned API for commands/configuration and WebSockets for live state updates. Reconnecting clients receive an authoritative snapshot and resume updates. Include unique operation IDs to make command retries duplicate-safe and revisions to detect stale configuration edits or previews.

**Agreed HTTP/WebSocket layer:** Drogon handles HTTP endpoints, webhooks, and WebSockets while Qt Core handles bot application logic, within one server process behind Caddy. Drogon provides these networking capabilities. [S58] Bridge incoming requests explicitly into the authoritative Qt state-processing queue; its additional framework/event loop requires an early startup/shutdown/thread-coordination prototype. Request threads must not become an independent state-writing path.

**Agreed API contracts:** JSON HTTP endpoints documented with OpenAPI 3.1 and separately versioned JSON schemas for WebSocket messages. Keep contracts alongside source and check desktop/web clients against them. OpenAPI supports documentation, code-generation, and testing tooling. [S59] Exact generation/validation libraries, schemas, and compatibility enforcement remain implementation decisions.

**Agreed numeric API encoding:** encode exact decimal/accounting values and 64-bit integers as strings in JSON. Encode rational values with explicit numerator/denominator strings. Define and validate these representations in the HTTP/WebSocket contracts so browser/native clients preserve precision. Ordinary limited-range numeric UI fields need not be represented as accounting values. JSON implementations commonly use floating-point numbers with a smaller exact integer range than signed 64-bit storage. [S71]

**Agreed desktop/server compatibility:** use a connection handshake advertising supported API versions and capabilities. Compatible clients hide unavailable features. Incompatible management clients receive an explanation and upgrade guidance before sending operations. Serve matching web assets with each server release and document supported desktop/API combinations. Exact negotiated version ranges, capability schema, and compatibility-test implementation remain engineering work.

**Agreed desktop UI:** Qt Quick/QML presentation with C++ application logic.

**Agreed editor scope:** the full scene editor is desktop-only. Browser management retains authorized live-management operations; it does not require a full scene editor.

### Native development foundation

**Agreed development desktop:** KDE Plasma on Arch Linux rolling release. Test the desktop on Arch rolling and Kubuntu 24.04 LTS, and support Ubuntu 24.04 LTS for native server deployment. Develop on Arch while building portable releases in a pinned Ubuntu-based environment, covering rolling and stable test environments. Exact compiler/build-tool versions and binary dependency manifests remain implementation decisions.

**Agreed:** use C++20 for server and desktop code. Build with CMake and Ninja, using shared configure/build/test presets for development and continuous integration, with individual developer overrides. Minimum Qt is 6.12; pin the tested Qt patch version for each project release and keep builds reproducible with publicly available sources and patches. Official Windows builds target Windows 11/x86-64 with MSVC 2022; Windows 10 is outside initial supported scope. Development takes place on Arch Linux rolling release. Exact Linux compiler versions and CMake/Ninja versions remain to be selected. Qt documents C++20 support and CMake-based development; CMake presets support shared configuration/build/test workflows. [S30–S31] Qt 6.12 was released on 30 September 2026, and its documented platform configurations include Windows 11/MSVC 2022. [S37–S38]

**Linux compatibility/validation context:** the earlier Ubuntu 24.04/Debian 12 deployment proposal is superseded by the accepted Arch/Kubuntu desktop and Ubuntu server matrix above; Debian 12 is not an initial target. Arch Linux development does not by itself establish binary compatibility with other distributions. Arch currently provides Qt 6.12 base/Wayland packages. [S39] Qt exposes separate Wayland and X11 platform plugins, so the expected benefit of dropping X11 is chiefly reduced packaging/integration/testing scope rather than a different application architecture. [S40] Typical Wayland desktop shells do not expose arbitrary top-level window positioning to clients. [S41] OBS documents native Wayland capture using PipeWire and desktop portals; window capture availability and transparent-overlay behavior require validation on the selected compositors. [S42–S43] The native capturable output and independent browser overlay both remain required. No Linux capture test has yet been performed.

**Agreed Linux desktop support policy:** support native Wayland initially, using KDE Plasma as the reference environment for testing and bug reproduction. Treat other Wayland compositors as best-effort until validated; X11/XWayland operation is outside the initial support commitment. Validate the native timer's OBS capture on Plasma, including transparency, resizing, and reconnecting. The headless server is unaffected and does not require either display system. Linux desktop packaging uses the agreed bundled .tar.gz and Arch PKGBUILD described in §16; Windows uses the agreed installation-free ZIP described in §16.

**Agreed testing foundation:** Qt Test and CTest are accepted components of native testing; this does not require every test to use Qt Test. Qt documents Qt Test/CTest integration. [S32] No application tests have been implemented or run at this planning stage.

**Agreed combined testing strategy:** use GoogleTest for core rules, accounting, refunds, milestone progression, and timer calculations; GoogleMock for service-interface interaction tests where useful; Qt Test for Qt signals, models, and event-loop integration; and Qt Quick Test for QML components and UI behavior. CTest runs the separate test executables locally and in continuous integration, with CMake's GoogleTest discovery for those suites. [S33–S36] Give each behavior a clear testing home rather than duplicating the same assertions across frameworks. Use controllable clocks and reusable fake service adapters for deterministic scenarios, with recorded or synthetic service events; reserve mocks for meaningful interactions. Keep live integration checks separate. Acceptance tests are written and independently reviewed before feature implementation under the agent workflow in §17. Exact dependency versions, fixtures, and suite structure remain implementation choices. Browser testing is settled below. No application tests have been implemented or run at this planning stage.

### Browser development and validation

**Agreed frontend:** React and TypeScript, built with Vite, for browser management and the viewer website. Ship compiled static assets with the server. Node.js is required for development/building, with no separate Node.js production service. React documents the React/TypeScript/Vite build approach. [S55] The independent HTML/CSS/TypeScript browser overlay renderer and desktop-only scene editor remain as agreed; no shared native/browser renderer is introduced.

**Agreed browser testing:** use Vitest for frontend unit/component tests and Playwright for complete browser workflows, including invitations, permission changes, contribution review, and live updates. These tools support real-browser testing. [S56–S57] Exact dependencies, test fixtures, and CI configuration remain implementation work; no browser tests have yet been executed.

**Agreed browser support policy:** management and viewer pages support the current and previous major versions of Chrome, Edge, and Firefox. Safari receives best-effort support initially. Separately test the overlay in actual OBS browser sources on Windows and Linux. Choose the minimum OBS version during the media/rendering prototype; ordinary browser tests do not establish OBS compatibility. Record the concrete browser/OBS versions exercised for each release. Browser engine, media-profile, and OBS validation remain work to perform rather than completed results.

## 3. Events, contributions, effects, and delivery

### 3.1 Separate records

- **Event:** something observed or requested, including service activity, a chat command, a moderator operation, a schedule, or an internal event.
- **Contribution:** a support activity whose payment evidence, valuation, review, credited effects, and reversals are tracked together.
- **Application plan:** an explanation of matched rules and intended actions for an input.
- **Effect:** an internal change actually applied, including cap adjustments.
- **Delivery job:** an external message, announcement, or supported service action.

### 3.2 Event envelope

Each event carries an internal ID; type/schema version; origin and connection; provider identifiers where supplied; source timestamp if available; server arrival timestamp; actor if available; linked contribution; parent/root event IDs; and configuration/context references.

Example event families: subscription, gift bundle, cheer, payment received/confirmed, refund, chat command, manual adjustment, timer expiry, milestone completion, modifier activation/expiry, and named custom event.

Provider IDs must be scoped correctly. Cross-service copies need an explicit source-selection or verified correlation strategy; equal names, amounts, and nearby timestamps alone do not establish identity.

### 3.3 Independent eligibility conditions

| Dimension | Example states |
| --- | --- |
| Payment evidence | Confirmed, unconfirmed, unknown, not applicable |
| Currency valuation | Ready, missing rate |
| Review | Not required, pending, accepted, rejected |
| Application | Not applied, applied |
| Reversal | None, partial, full |

A contribution may have several unresolved conditions simultaneously. Accepting paused input does not implicitly override missing confirmation or valuation. Payment confirmation links to the original contribution and must not create a second credit.

### 3.4 Processing contract

1. Validate and durably record arrival and the relevant configuration/context.
2. Detect duplicate delivery and determine eligibility/review requirements.
3. Match arrival-time rules, resolve exclusions, and construct an inspectable plan.
4. On application, resolve actual caps and milestone changes against current authoritative state.
5. Commit internal effects, application status, and required jobs in one transaction.
6. Deliver queued actions and publish authorized state updates.

Arrival-time rules and modifiers govern pending contributions. Authorized overrides create audited revisions. Missing valuation is resolved explicitly with a recorded rate rather than inventing an arrival-time rate that did not exist.

Effects retain requested and applied changes, target, rule/action/version, calculation details, contribution/event links, reversal behavior, and original-effect references for reversals. Record parentage through generated-event chains.

External delivery needs persisted queued/sending/delivered/failed/outcome-unknown states. Recovery must not blindly replay performed actions. Use provider duplicate-safety mechanisms where available; otherwise specify a retry/review policy for uncertain outcomes.

### 3.5 Numerical precision and validation

**Agreed:** time adjustments use millisecond precision and score supports up to six decimal places. Preserve original monetary amounts with their source precision and currency. Use high-precision decimal or exact rational arithmetic for exchange rates and intermediate calculations. Round time/score only when producing each final credited effect, using round-half-to-even; display precision is separately configurable and cannot change accounting.

Modifier factors may be zero or positive, including fractional factors such as ×0.5. Reject negative and non-finite factors; clamp an additive-bonus combination's final factor to zero if it would otherwise be negative. Exchange rates must be strictly positive. Numerical overflow holds the affected input for review without applying its effects. Explicit corrections and refunds retain signed adjustments. Never silently truncate values.

**Agreed arithmetic/storage implementation:** use Boost.Multiprecision integers/rationals for intermediate calculations. Its cpp_int/cpp_rational types provide arbitrary-precision integer/rational arithmetic. [S69] Store credited time as signed 64-bit milliseconds and score as signed 64-bit millionths of a point. Preserve original money/rates losslessly as decimal data. SQLite supports signed 64-bit integer storage; original decimal values require a lossless representation without implicit REAL conversion. [S70] Apply the agreed final rounding and hold out-of-range results for review. Numeric parser limits, resource budgets, exact decimal schema, safe conversion/aggregation, and bounded execution remain implementation/validation work; arbitrary-precision types do not imply unbounded input acceptance.

## 4. Main timer, grace, pauses, and finishing

**Agreed**

- Configurable remaining-time cap, hard end date, and scheduled pauses, each optional.
- Manual/editorial time adjustments governed by permissions.
- Configurable treatment of server downtime and streamer/client disconnection.
- Main/grace server-downtime policies support counting elapsed downtime, freezing during downtime (default), or requiring authorized review of the gap. An enabled hard end remains authoritative; timed modifiers retain their separately selected clock behavior.
- Unknown Twitch stream status is distinct from confirmed offline. By default retain the current timer state and display a notice; optionally add a pause reason after a configured unknown-status timeout. Unknown status cannot clear a confirmed-offline pause or another active reason.
- Disconnect triggers can include Twitch going offline and a designated streamer client disconnecting.
- During pause, incoming support is logged and held for an authorized acceptance/rejection decision.
- End modes: immediate finish; separate grace period; wait for authorized confirmation.
- Default end mode: separate two-minute grace period, then automatic finish.
- An enabled hard end date finishes the subathon even during pause or grace. Authorized users may change/disable the deadline beforehand; countdown pause does not postpone it.
- Repeated-grace behavior is streamer-selectable: fresh full grace on each new main-timer expiration (default), or one shared grace budget whose unused duration is reused across expirations. Shared budget is consumed while grace actually counts down, not while the main timer runs or grace is paused.
- Configure stream-offline and designated-client disconnect delays independently; both default to 30 seconds. A pending delay does not itself pause a running countdown. Once the condition persists through the delay, add its pause reason. Clear that reason automatically when the signal recovers, while retaining all other active reasons.
- Accepted support yielding positive remaining time during grace returns to the main countdown, subject to debt/caps/policies.
- While waiting for authorized finish confirmation, accepted support yielding positive remaining main time automatically resumes the main countdown and clears the confirmation wait. Zero actual credit leaves the wait unchanged. Independent pause reasons, the retained grace budget and hard-end precedence remain unchanged; a finished subathon still requires explicit reopening. [Owner clarification](evidence/timer-confirmation-credit-decision.md).
- After finish, continue recording inputs. Whether they affect milestones or run automation is configurable.
- Reopening requires a specific permission, preserves history/score/completed milestones, and lets the authorized user set remaining time and explicitly choose to reset or retain the remaining grace budget. A passed hard end must be changed or disabled before reopening. Ordinary incoming support does not automatically reopen a finished subathon.
- Reopening preserves outstanding time debt by default. Clearing or adjusting it requires a separate correction permission and an audit entry. Show pending action groups and explicitly offer Resume, Keep deferred (default), or Skip. Existing review holds remain until resolved; performed actions stay performed.

### State model

| Property | Values |
| --- | --- |
| Lifecycle | Ready, active, finished |
| Countdown phase | Main, grace |
| Pause reasons | Manual, schedule, stream offline, designated-client disconnect, recovery review, as configured |

Pause reasons are independent. Clearing one reason cannot resume while another remains. Preserve the countdown phase across pauses. Present understandable UI states such as Ready, Running, Paused, Grace period, and Finished.

Persist clock references and remaining values independently of display updates. Clients animate locally from synchronized snapshots, while server transitions remain authoritative. Recovery applies the configured downtime policy and the checkpoint/review semantics below.

### Schedules and daylight-saving time

**Agreed:** use the configured subathon timezone for scheduled pauses and other recurring schedules, with UTC timestamps internally. For recurring schedules, a nonexistent local time shifts forward by the daylight-saving gap by default, with Skip as an alternative. A repeated local time runs once by default, with Run twice as an alternative. Show upcoming occurrences before saving. Changing the timezone previews future schedule changes and does not replay past occurrences.

### Recovery checkpoints and review

**Agreed checkpoints:** save clock state with every timer-changing operation and checkpoint advancing clocks every second by default, with a configurable interval. Under Freeze, restore the last durable checkpoint; an abrupt crash can therefore restore a small amount of already elapsed time. Under Count elapsed, reconstruct progression from that checkpoint, respecting scheduled pauses, main/grace transitions, and the hard end. Checkpoint cadence is independent of the millisecond precision of credited time effects.

**Agreed recovery review:** add a separate Recovery review pause while an authorized person decides how to handle the offline gap. Main/grace countdowns and active-subathon modifiers remain frozen while awaiting the decision; real-elapsed modifiers continue and the hard end remains authoritative. Preview Count elapsed and Freeze for the offline gap, and permit an authorized manual correction. Time spent waiting for that decision stays paused. Record and hold incoming support; finalize eligibility that depends on unresolved clock recovery after the recovery decision. Resolving recovery clears only its own pause reason, preserving any other active pause reasons.

### Refund overflow

The streamer configures a policy and authorized moderators can override a particular case:

| Policy | Result |
| --- | --- |
| Stop at zero | Exhaust remaining time, then follow ending policy |
| Carry time debt | Store excess reversal separately; future additions cover debt first |
| Require review | Hold the timer adjustment for an authorized decision |

Always retain the full reversal record. Never represent time debt by silently making the separate grace countdown negative.

**Settled:** enabled hard-end precedence over pause/grace; configurable repeated-grace modes with fresh grace as default; independent 30-second default disconnect delays/recovery; downtime modes with freeze as default; unknown-status policies; permission-controlled reopening with explicit grace-budget, debt-correction, and pending-action choices; schedule timezone/DST defaults and alternatives; one-second default checkpoints and recovery-review clock/input handling. **Open:** implementation validation of recovery across combined transitions and clock discontinuities.

## 5. Paused input review and silent application

**Agreed**

- Accept or reject individually or in batches.
- Default acceptance runs normal events/actions.
- Optional silent acceptance suppresses ordinary messages, sounds, and visual alerts while applying time, score, and internal progression.
- Milestone actions require a separate explicit decision: run, defer, or skip. Completion remains recorded.
- Show affected milestones in the preview.
- Preserve arrival-time handling; permit an authorized override.

Persist each decision, actor, options, and resulting effects. Concurrent reviewers and retried requests cannot apply the same contribution twice. Revalidate a stale preview before applying it.

## 6. Rules and custom events

**Agreed rule structure:** name/description; enabled state; triggers; nested conditions; priority/exclusions; ordered actions; execution limits.

Conditions support all/any/not groups and typed event/context fields. Monetary thresholds explicitly declare a stable comparison currency; changing the reporting currency cannot silently change their meaning.

All matching rules run unless exclusions apply. Exclusive groups select the highest-priority applicable rule; equal numeric priorities use an explicit user-controlled order. Warn about equal priorities and show the winner in previews. Priority alone does not suppress unrelated matching rules.

Process higher-ranked candidates first. Explicit exclusions may point only to lower-ranked rules; backward or contradictory exclusions must be corrected before enabling the configuration. Only selected rules apply their exclusions. For A excluding B and B excluding C, selecting A suppresses B but leaves C eligible. Resolve conflicts before actions and record why rules were skipped. Save ordering/priority/exclusion changes as configuration versions so pending input retains its captured behavior.

Example: normal subscription credit runs independently; 25-gift and 50-gift bonuses share an exclusive group. A 50-gift bundle receives normal credit and the 50-gift bonus, without the 25-gift bonus.

Supported action categories: time/score/counter changes; modifier activation/extension/termination; chat messages; announcements/animations; authorized timer controls; generated named events; supported connector actions.

Evaluate an original event predictably against its captured context. Changes resulting from it generate subsequent events; they do not unexpectedly change earlier matching decisions halfway through evaluation. Trace generated-event causes and enforce configurable per-originating-event limits: maximum chain depth 32 and maximum generated events across the chain 256 by default. Reaching a limit stops further event generation for that chain, records the reason, and notifies authorized moderators while the server continues operating. Already-applied effects remain recorded. Prevent duplicate rule processing of the same event, while permitting the rule to handle distinct events, including several milestone events, in one chain.

The rule editor provides structured trigger/condition/action editing and simulation showing matching/excluded rules, modifiers, intended/applied effects, milestone consequences, messages, and generated events. Changes create rule versions. Initially favor visual configuration and constrained expressions; unrestricted scripting is not required by the current plan.

## 7. Timed modifiers and special event timers

**Agreed:** timed modifiers are persistent instances with independent countdowns, not additions to the main or grace timer.

Configure target(s), qualifying inputs, multiplier/value, duration, activation trigger, clock behavior, overlaps, and reactivation behavior. Targets can independently include added time, milestone score, and selected counters.

- Clock behavior defaults to active-subathon time: advance during main/grace countdown progression, freeze while paused or finished, and follow the selected countdown downtime policy. Real elapsed time remains available. Modifiers affect post-finish contributions only when explicitly enabled in post-finish settings.
- Named stacking groups define how matching modifiers combine independently for each target. Supported operators are multiply (default), add bonuses using 1 + sum(factor - 1), highest multiplier, lowest multiplier, and priority selection. Group results combine at the target level using the same selectable operators, also defaulting to multiply. Priority ties use explicit ordering. Group configuration and combination details are captured with application calculations.
- Reactivation supports extend (default: add new duration to remaining duration), restart (reset to full configured duration), replace (apply new settings and countdown), or ignore (retain the existing instance unchanged). A ten-minute reactivation with three minutes left yields thirteen minutes under Extend.
- Activation: moderator, permitted chat command, milestone, rule, or schedule.
- Explicitly choose whether activation affects its triggering contribution or subsequent contributions only.
- Activation/expiry can generate further rule events.

Example: a ten-minute double-time modifier turns a qualifying 60-second award into a requested 120 seconds, then the normal cap applies. Expiry changes future eligibility, not already credited effects. Pending input preserves its arrival-time bonus. Refunds reverse the recorded credited amount. Persist and recover modifier clocks according to their clock/downtime policy.

Overlay bindings expose modifier name, multiplier, remaining duration, and active state. Hiding the display does not disable the modifier.

**Agreed active-instance edits:** definition edits affect future activations by default; active instances retain their captured settings. Offer an explicit permission-controlled update of selected active instances with a preview. For duration changes choose Keep remaining, Restart, or Set remaining time. Pending inputs retain their arrival-time context, and already-credited effects remain unchanged. Persist definition/instance revisions and update history rather than rewriting prior calculation contexts.

**Settled:** two-level stacking groups/target combination operators with multiplication defaults, active-subathon clock default including phase behavior, Extend reactivation default with restart/replace/ignore alternatives, definition-versus-active-instance edit/version policy with explicit duration choices, numerical precision/storage/factor validation in §3.5, and checkpoint/recovery-review clock behavior in §4. **Open implementation/validation:** revision schema, bounded arithmetic/resource handling, and combined editing/arrival-context/recovery cases.

## 8. Score, milestones, and reversals

**Agreed**

- One sequential milestone track based on a custom score combining selected services/events.
- Progress bar shows progress between the previous completed milestone and the next.
- Milestones and their actions can be edited according to permissions.
- Record every newly crossed milestone as achieved immediately, in threshold order, when the source input applies. Release configured action groups, including announcements and bonus activations, one milestone at a time with configured delays. The overlay may animate progress through each step independently of the recorded achievements.
- Authorized users can edit unfinished targets with a preview of changed progress/newly achieved milestones. Targets must remain in ascending order. Editing an already-completed target requires a separate correction permission, preserves completion, and does not automatically replay actions. Retain original/revised values in the audit history. If an edit newly achieves a milestone, explicitly choose Run, Defer, or Skip for its actions.
- Completed milestones remain completed after a reversal, with deficit tracking.
- Do not automatically repeat completed rewards when a deficit is recovered.

**Agreed manual completion:** a separate permission allows completing the next milestone manually, or several sequentially. Preserve the actual score and show any resulting deficit. Explicitly choose Run, Defer, or Skip for their actions, and record who completed them and why.

**Agreed completion reset:** a separate correction permission allows resetting a selected milestone and all later completed milestones, preserving history and already-applied effects. Cancel their unstarted action groups. Require the current score to be below the first reset target to prevent immediate re-completion; offer a separate audited score correction if necessary. Previously performed actions require explicit authorization to replay when milestones are earned again. Ordinary refunds still preserve completion and do not perform this exceptional reset.

Keep completion and action-delivery records separate. A completion can have queued, deferred, skipped, or performed actions. Distinguish score/progression state from the milestone currently being presented during a multi-milestone sequence. Milestone-triggered configured automation is released with that milestone's sequential action group, not prematurely merely because achievement was recorded.

**Agreed presentation timing:** the server controls presentation duration and the delay between milestone groups, configurable globally and per milestone. Defaults are 10 seconds of presentation followed by a one-second gap. Desktop and browser outputs follow the same server timeline; a disconnected overlay cannot block the queue, and reconnecting shows the current presentation. The queue has its own Pause/Resume controls, independent of countdown pauses by default, with an optional setting to follow countdown pauses. Existing review holds continue to apply independently of presentation timing.

**Agreed queued-action editing:** retain the action definitions captured when a group was created. Editing a milestone normally affects future executions. An authorized user may explicitly replace definitions for actions that have not started, after previewing the changes. Started or completed actions retain their original definitions and history.

**Agreed interruption:** authorized users can stop a group by cancelling its unstarted actions and requesting that playing animations/audio stop where supported. Already-applied time, score, and modifiers, and already-sent messages remain recorded; any reversible internal effects require an explicit correction. Stopping offers Continue with the next group or Hold the queue, defaulting to Hold. Existing review holds remain independently enforceable.

If a refund reduces score below an achieved milestone whose actions have not started, apply a streamer-configurable policy: Continue (run pending actions), Review (default: hold for an authorized decision), or Skip (skip pending actions). Under Review, later milestone action groups wait behind the held group to preserve order. Completion remains recorded and already-performed actions remain unchanged.

Example: milestone 1 at 1,000 is complete; score reaches 1,100 and a refund removes 300. Score is 800; milestone 1 stays complete; its deficit is 200. With milestone 2 at 2,000, 1,200 more points are needed. Display progress as 0% and optionally show the deficit.

Reversals reference original effects, rates, rules, and actual capped additions. Full/partial reversal accounting prevents reversing more than was credited. Preserve irreversible/performed actions and completed milestones according to the agreed policies.

**Agreed partial-refund accounting:** reverse time and score in proportion to the original contribution's actually credited effects. A €20 contribution that credited five minutes and 40 points has a €5 refund entitlement of 75 seconds and 10 points. For successive partial refunds, calculate the cumulative reversal entitlement at the agreed effect precision, then subtract amounts already reversed; a full refund reverses the exact remaining original credit. This avoids accumulation of per-refund rounding errors and never recomputes the original award under current rules. Whole count-type credits are reversed upon full refund rather than fractionally. Preserve raw event history; actual timer subtraction remains subject to the selected deficit/review policy.

Example: at 55 minutes with a 60-minute cap, a requested 10-minute addition credits 5 minutes. Its refund reverses those 5 minutes, with overflow handled by the selected policy.

**Settled:** immediate ordered achievement recording with sequential action-group release; target edits/correction permissions and explicit action choices for newly achieved targets; pending-action refund policies with Review default; captured queued definitions with explicit replacement of unstarted actions; group interruption without automatic effect rollback and Hold as default; separately authorized manual completion/reset with history and replay safeguards; server-controlled 10-second/one-second default presentation timing, synchronized outputs, and independent queue controls. **Open implementation/validation:** persistence and synchronization of the presentation timeline, media timing conformance, and combined correction/reset/recovery scenarios.

## 9. Currency conversion and reporting

**Agreed**

- Selectable reporting currency per subathon, changeable while running.
- Changing reporting currency recalculates monetary displays, lists, totals, and graphs only.
- Preserve original amounts/currencies and historical rate snapshots.
- Awarded time/score, milestone completion, and executed actions remain unchanged.
- Refunds reverse original effects.
- Supported rate providers: Frankfurter, ExchangeRate-API, Open Exchange Rates.
- Sync immediately on provider selection/configuration, then every 12 hours by default, anchored to the first successful sync. Persist the automatic schedule across restarts; manual sync does not reset it. Keep the manual sync button and configurable intervals subject to provider limits.
- Configurable faster updates subject to documented service limits and response handling.
- Failed sync: visible notice, last successful rates retained, age visible.
- Configurable stale-rate warnings default to no successful sync for 24 hours or provider-published rates older than 72 hours. Display fetch age and publication age separately. Warnings do not automatically stop contributions from using cached rates.
- Failed-sync retry delays default to 1 minute, 5 minutes, 15 minutes, then hourly, subject to provider limits/required waits. Invalid credentials await correction; exhausted quotas await a known reset or authorized configuration change. Retry/fallback jobs share centralized request accounting and avoid duplicate requests; reference allowance edits never lift actual provider restrictions.
- Missing currency rate: hold contribution for review or manual valuation.
- Temporary manual override until a successful relevant update; pinned override retained across updates until removed.
- Maintain one consistent effective rate table: a manual currency-pair override determines its inverse, cross-rates are derived, and the editor previews affected rates. Conflicting pinned overrides must be resolved before saving. Provider updates preserve pins; reporting-currency changes do not alter stored overrides. Retain the fetched observations separately for provenance.
- Optional ordered configured fallback providers are supported and disabled by default. On primary-sync failure, enabled fallback may try the next provider within its own limits. Show the primary failure and actual replacement source; store source provenance per snapshot. Without fallback, retain cached rates and their age.
- When a newly selected reporting currency is missing from historical data, first use the stored snapshot/cross-conversion within it. Offer historical retrieval where the configured provider/plan supports it, otherwise permit an authorized historical manual rate. Allow the currency change with visibly unconverted unresolved amounts and affected totals labelled partial. Never silently substitute current rates. Supplemental historical display conversion must retain provenance and must not alter original credited effects.

Preserve source monetary precision and use the decimal/rational arithmetic and final-effect rounding specified in §3.5. Store fetched and effective rates, source, rate timestamp, fetch timestamp, and override provenance. UI locale and reporting currency are independent.

Show provider quota where available, freshness of that quota report, and local requests since the report. If only local usage is known, label it as this server's usage. Allow paid-tier reference allowance entry with a clear statement that it only changes estimates/display and cannot lift provider limits. Keep reference values separate from authoritative limits.

Centralize caching and scheduling. Count quota-check requests where required, coalesce redundant syncs, and respect request limits/backoff for manual sync too. Show last successful fetch, source rate timestamp, and next scheduled fetch separately.

**Settled:** persistent 12-hour default schedule/manual-sync interaction; optional ordered fallback disabled by default; historical display-conversion handling; 24-hour fetch/72-hour publication warning defaults; bounded retry behavior; consistent manual inverse/cross-rate handling and pins; source monetary precision, positive rates, exact intermediate arithmetic, storage/API representation policy, and final-effect rounding. **Open implementation/validation:** effective-table normalization/constraints, decimal parsing/resource bounds, and handling of unavailable provider metadata.

## 10. Accounts, permissions, and chat authorization

**Agreed**

- Invitation-based account registration, using expiring single-use invitations.
- Invitation links are revocable, copied/shared manually, and expire after 48 hours by default; expiry can be configured when creating an invitation. The inviter chooses initial roles within their delegation permissions.
- Support both local username/password login and optional Twitch sign-in. Both map to the same application accounts and permissions; Twitch can be linked to an existing account. The owner retains local credentials as a fallback.
- Recovery uses downloadable single-use recovery codes and administrative recovery only; no email-based recovery or required email-server setup. Provide a server-console owner-recovery command. Authorized administrators may initiate helper password resets without learning the helper's password.
- One protected owner is separate from editable roles and retains access-management/recovery capabilities. Ownership transfers explicitly to another account; normal role edits cannot acquire or remove it. Administrative helper recovery must not become an ordinary administrator's route to taking over ownership.
- Editable preset roles plus individual permission overrides.
- Suggested presets: Administrator, Moderator, Designer, Observer.
- Same effective server permissions for desktop and web.
- Granular operations, not page-level access alone.
- Individually revocable sessions. Optional Twitch linking uses stable user IDs.
- Owner-configurable session defaults: normal login has a 24-hour maximum lifetime before fresh sign-in; "Remember me" has a 30-day maximum. Users can inspect/revoke their own sessions; authorized administrators can revoke others' sessions. Ownership, login-method, and recovery-setting changes require fresh authentication. Session limits govern application access, while the server runs the subathon independently according to the configured client-disconnect policy.
- Check rights at execution, including queued/review operations where relevant; an open page cannot preserve revoked access.
- Audit sensitive operations and overrides.

**Agreed initial-owner setup:** provide a server-console command generating a single-use setup code valid for 15 minutes. It permits creating the initial owner only while no owner exists. The owner sets local credentials and receives recovery codes. Generating a replacement setup code invalidates the previous code; completing setup disables initial-owner registration.

**Agreed fresh authentication and ownership transfer:** authentication within the previous five minutes counts as fresh for sensitive operations. Transfer requires the current owner's fresh authentication followed by the recipient's explicit acceptance with fresh authentication and configured local credentials. The transfer offer expires after 24 hours; ownership remains unchanged until acceptance. Explicitly choose the former owner's resulting roles, defaulting to Observer.

**Agreed administrative recovery:** authorized administrators generate a single-use helper password-reset link valid for 15 minutes, copied/shared manually. Completing recovery revokes existing sessions, invalidates old recovery codes, and offers new recovery codes; permissions remain unchanged. Server-console owner recovery uses the same short-lived reset process for the existing owner account and preserves its ownership. These procedures do not add email-based recovery or automatic message delivery.

**Agreed password hashing:** use libsodium with Argon2id. Store salted password hashes with their parameters and upgrade hashing parameters after successful login when needed. Benchmark hashing cost against supported server hardware. Libsodium provides the hashing, stored-parameter, and verification facilities. [S62]

**Agreed application sessions:** generate random 256-bit opaque session tokens and store only their hashes server-side, supporting immediate revocation. Browser sessions use Secure, HttpOnly, SameSite=Lax cookies with CSRF protection. Desktop clients send session tokens through an authorization header. Keep application session tokens out of URLs and browser local storage. These mechanisms follow OWASP session guidance. [S64] Previously agreed session lifetimes remain unchanged. This account-session policy does not redefine separately scoped viewer/overlay-link access. Exact token hashing/rotation and cookie/CSRF/Origin validation remain implementation/validation details.

**Agreed remembered Windows login:** store only the session token, encrypted using Windows DPAPI, inside the portable application folder. No password is saved. DPAPI normally ties decryption to the same Windows user/computer; moving the folder preserves preferences but generally requires a fresh sign-in. [S67] Exact protected-file handling and clean-machine/move/recovery tests remain implementation work.

**Agreed remembered Linux login:** use QtKeychain with the available desktop keyring, including KWallet/Secret Service support, storing only the session token. [S68] If secure storage is unavailable, keep the session in memory and disable remembered login with a UI explanation. Do not enable an insecure plaintext fallback. Exact backend selection and availability/error handling require validation on supported Linux desktops.

Permission resolution: individual explicit allow/deny first; otherwise role deny wins over role allow; otherwise any role allow grants; unspecified access is denied. Explain effective permission sources in the UI.

Permission families: timer operations; review/silent application/calculation overrides; milestone editing/action release/reset; rules/modifiers; layouts/assets/live publishing; integrations/credentials; rates/overrides; invitations/roles/sessions; audit/export.

Selected permissions may have constraints, such as maximum adjustment or allowed scene. Creating/enabling rules cannot be used to acquire actions beyond the editor's delegated authority. Automatic rule execution uses explicitly authorized automation capabilities; contributing support does not itself grant administrative rights.

**Agreed constraint combination:** evaluate each permission grant with its own constraints. A request must fit one complete applicable grant; constraints from different grants cannot be combined. A grant to add up to five minutes to timer A plus another to add up to ten minutes to timer B does not permit adding ten minutes to A. The established individual-override and deny precedence still applies.

**Agreed delegation:** permission to use an operation is separate from permission to grant it to others. The owner configures which permissions an administrator may delegate and their maximum scope. Role edits, invitations, and individual overrides must remain within that authority, including changes affecting the administrator's own account. Show an effective-permission preview before saving.

Twitch chat categories are independent: broadcaster, moderator, VIP, subscriber, unsubscribed viewer, and explicit user selections. Conditions support combinations such as moderator OR VIP and VIP AND subscriber. Chat command eligibility may map configured Twitch categories to selected operations; Twitch moderator status does not automatically grant unrestricted application access.

Commands support configurable aliases, arguments, responses, actions, and per-user/global cooldowns. A public command need not require a registered application account. Management commands require their configured authorization path.

**Agreed Twitch message identities:** support both a dedicated bot account and the broadcaster's account. Setting up a bot must not prevent selected messages from using the broadcaster. A message action/template can select the configured default, the bot, or the broadcaster; this applies to rule, command-response, and milestone chat messages. The recommended default is the bot when configured, with a streamer-selectable default identity. Record the selected/resolved sender with the delivery job and audit history. Application sign-in and outgoing chat identity are separate concerns.

**Agreed delivery safeguards:** provide separate permissions for bot and broadcaster messages, covering direct sends and authorization of automated messages. An explicitly selected sender never silently changes. By default hold the message with a visible failure; optionally configure an authorized fallback identity. Automatic fallback is allowed only when delivery is known not to have happened. Uncertain delivery outcomes require review to avoid duplicate messages.

**Settled:** both login methods; recovery codes/administrative recovery without email; protected ownership; invitation delivery/default expiry; session default lifetimes/revocation; 15-minute single-use initial-owner setup/reset procedures; five-minute fresh-authentication window and two-party ownership transfer with a 24-hour offer/default Observer role for the former owner; libsodium Argon2id password hashes; opaque 256-bit hashed server-side sessions with protected browser cookies/CSRF and desktop authorization headers; Windows DPAPI portable token file and Linux QtKeychain/keyring remembered-login policies; complete-grant constraint evaluation and owner-scoped delegation; both chat identities with per-message sender selection, separate sender permissions, held-failure default, authorized known-undelivered fallback, and review of uncertain delivery. **Open implementation/validation:** remaining session/setup/reset/transfer token mechanics, provider authentication/reauthentication library and behavior, desktop-token persistence implementation/tests, encryption-key management details, and enforcement of combined account/permission/delivery policies.

## 11. Overlay scenes and visual editor

**Agreed**

- Saved scenes, reusable themes, drag-and-drop editing.
- One declarative scene format, with separate desktop and browser rendering implementations.
- Desktop scene editor, preview, and capturable window use Qt Quick/QML with C++ application logic.
- Browser overlay uses its own HTML/CSS/TypeScript renderer. No shared desktop/browser renderer and no embedded shared web scene editor.
- The full scene editor is available on desktop only. This supersedes the earlier shared-renderer proposal.

**Agreed conformance contract:** both renderers must match data bindings, visibility conditions, layout geometry, progress calculations, and animation timing. Minor font-rendering differences are acceptable. Maintain shared reference scenes to verify these behaviors separately on each renderer. The editor flags unsupported features before publishing; scenes intended for both outputs must use features supported by both. This is a validation requirement, not a claim that conformance tests have already run.

| Element | Examples |
| --- | --- |
| Timer | Main, grace, modifier duration |
| Text | Labels, values, milestone descriptions |
| Progress bar | Sequential milestone progress |
| Image | Backgrounds, logos, artwork |
| Shape/group | Panels, borders, grouped layout elements |
| Recent support | Selected subs, gifts, Bits, donations |
| Announcement area | Event and milestone sequences |

**Agreed release media targets:** static PNG/JPEG/WebP and a documented static SVG subset; animated GIF; MP4/WebM video and WAV/MP3/Ogg audio with explicitly documented codec profiles. SVG excludes scripts/external resources. Built-in animations include fade, move, scale, rotate, and configurable easing/duration. Validate media profiles in both renderers and supported platforms; choosing an import/container format does not claim that every codec/profile is supported. Qt documents backend/platform codec differences. [S28]

**Agreed initial SVG profile:** support basic shapes, paths, groups, fills/gradients, strokes, transforms, and internal references. Initially exclude scripts, external resources, embedded HTML, SVG animations, and filters. Require SVG text to be converted to paths; editable text is available through scene text elements. Validate the profile in both renderers, including rejection of unsupported content rather than silent reinterpretation.

**Agreed canvas/performance targets:** default design canvas 1920 × 1080 with custom dimensions. Target 60 FPS with a selectable 30 FPS mode; benchmark both renderers. Preserve aspect ratio by default when resizing, with crop/stretch alternatives. Elapsed-time animation timing prevents missed frames from extending countdowns or animations. These are targets for implementation validation, not completed performance results.

**Agreed fonts:** support uploaded TTF/OTF fonts, a bundled default font, and an explicitly configured fallback. Package scene fonts with theme exports where redistribution is permitted; provide the same font assets to both renderers. Qt supports loading TrueType/OpenType application fonts. [S29]

**Agreed bundled default font:** use Noto Sans Regular and Bold for built-in interfaces and default themes. Keep uploaded scene fonts and configured fallbacks available. The font retains its SIL Open Font License and notices, separately from the project GPL license. Noto's OFL permits bundling with software subject to its conditions. [S73] Pin and validate the distributed font assets/weights during packaging.

**Agreed configurable asset budgets:** initial maximum file sizes are 32 MiB per image/font, 128 MiB per audio file, 512 MiB per video, and 2 GiB per packaged scene. Limit raster images to 8,192 pixels per edge and 32 megapixels total. Authorized users may adjust limits. Validate decoded dimensions and warn about demanding scenes. Exact upload/decode/animation memory and execution controls remain implementation work; these defaults are not evidence of measured scene performance.

Each element has position, size, layers, styles, data bindings, visibility conditions, and animations. Use selectable data fields and formatting controls. Conditions support all/any/not, milestone position, progress intervals, modifier state, pause, and grace.

Editor functions: drag/resize; align/distribute/snap; grouping/layer locks; undo/redo; canvas size/scaling; previews for lifecycle and modifier states, deficits, large contributions, long names/descriptions; import/export with assets.

Separate drafts from live revisions. Publishing requires permission; support restoring a previous revision. Server-owned packaged assets avoid dependence on a designer's local paths. Read-only overlay credentials are revocable and scoped to the necessary scene data.

Normal progress is between thresholds; show 0% with optional deficit after a reversal below the previous threshold; present multi-milestone crossings sequentially; allow a configurable final completion layout.

**Settled:** media-format targets, initial restricted SVG profile/text-to-path requirement, configurable upload/scene/dimension budgets, built-in animation scope, 1080p/60 FPS default target with 30 FPS alternative, aspect-fit default/crop/stretch, packaged fonts, cross-renderer consistency/reference scenes/pre-publication checks, and separate actual-OBS testing on Windows/Linux. **Open implementation/validation:** codec profiles/backend deployment, minimum OBS/embedded-browser versions, execution of asset-budget/SVG/memory/cross-renderer tests, concrete graphics benchmark hardware, font asset packaging/fallback validation, and desktop editor component design. Browser distribution of a full scene editor remains outside scope.

## 12. Localization and appearance

**Agreed requirements:** English/German at launch for desktop, web management, and built-in viewer-website labels. Desktop/web management provide Light, Dark, and System appearance, defaulting to System. Interface preferences, broadcast scenes, and streamer-configured viewer-page design are independent.

- Individual UI preferences do not affect other users or the live overlay.
- Each user chooses language independently. Initially detect the device/browser language and fall back to English; explicit saved preferences take precedence. Language changes apply without restarting. Desktop/web management appearance changes also apply immediately, with System following the detected system appearance.
- Interface language is distinct from number/date/currency locale.
- Scene language, formatting, colors, and custom content are controlled by the streamer.
- Streamer-authored milestone descriptions, overlay text, and chat messages remain authored content and are not automatically translated.
- Server messages use stable identifiers and structured parameters for client translation.
- Use English fallback and generate Qt/web runtime translation formats from one authoritative translation catalog so shared labels stay consistent.

**Agreed translation catalog format:** use one authoritative structured JSON catalog with stable message IDs, English/German translations, named parameters, and plural forms. A tested build generator produces Qt Linguist translation files (.ts/XML and compiled .qm) and browser catalogs. Contributors edit the source catalog; generated outputs are checked for consistency. This implements the shared-catalog approach and requires an explicit generator rather than assuming Qt reads the source JSON natively. Qt documents its translation-file/plural representation. [S74]

**Agreed validation:** check missing translations, placeholders, plural forms, and layouts with longer German strings. Translation contributions should not require application-logic edits. **Open implementation details:** JSON schema and generator/placeholder/plural mapping, preference persistence/synchronization, and platform-specific appearance detection/fallback. English/German scope, runtime switching, System default, and JSON source/generated Qt/browser catalog approach are settled.

## 13. Viewer website and statistics

**Agreed scope:** public/private visibility; milestones as the main content; configurable last-X subscriptions, gifts, Bits, and donations; graphs for subscriptions since start, tier/Prime breakdown where available, and donations; useful viewer/streamer metrics with configurable supporter visibility.

**Agreed presentation:** independently configurable current milestone/progress, completed/upcoming milestones, recent subscriptions/gifts/Bits/donations with filters/item counts, selected statistics/graphs, and optional main/grace/modifier status. Default page shows milestones and the latest 10 support events; the streamer can enable other sections.

**Agreed access modes:** Public (anyone), Link access (anyone holding a revocable read-only viewing link with configurable expiry), and Invited accounts (default: signed-in accounts with viewer permission). Viewer permission never grants management operations. Viewing-link credentials are distinct from account-registration invitations and grant only the configured read-only projection.

**Agreed viewer/overlay-link expiry defaults:** viewer links expire after 30 days by default, with configurable expiry including Never. Overlay credentials default to no expiry to avoid broadcast interruptions. Both remain individually revocable and replaceable and grant only their configured read-only projection. These defaults do not change account-session lifetimes or registration-invitation expiry. Exact token/expiry enforcement and rotation UX require implementation validation.

**Agreed supporter privacy:** show supporter display names and amounts by default only where the source permits publication. Provide global/per-source name anonymization and amount hiding, plus hiding individual viewer-page entries. Honor source anonymity/hidden amounts. Viewer hiding does not erase the contribution record or reverse credited effects.

**Agreed metrics and graphs:** support cumulative and per-interval views for:

- Original/gross monetary support, reversals/refunds, and net monetary support, with net as the default monetary view.
- Credited score and credited time, with pending/rejected inputs separately identified.
- New subscriptions, reported resubscriptions, gifted subscriptions/bundles, tiers, and Prime status when reliably supplied.
- Bits and donation trends over the event.

**Agreed chart time basis:** use reliable provider event timestamps, otherwise server arrival. Insert recovered events into their historical intervals; represent refunds as separate negative entries at refund time. Timestamp placement does not replace original recorded reversal valuation/effects. Group charts using one selectable subathon timezone, suggested from the owner's device during setup; store instants in UTC. This does not change arrival-based rule handling.

**Agreed default chart presentation:** cumulative line charts and per-interval bar charts, stacked by service or subscription category where appropriate. Automatically choose roughly 100–200 time buckets, with manual interval selection. Archived graphs extend through later refund corrections, mark the event end, and show the data last-updated time. Existing source/refund timestamp, classification, net-money, privacy, and post-finish inclusion policies remain authoritative.

Default statistics cover support occurring during the event, with later refunds reflected as corrections. New post-finish support can be included through an explicit setting.

**Agreed archive/data defaults:** keep the completed viewer page available as an archive with its access/privacy settings preserved. Retain accounting history, effects, rate snapshots, and audit records until authorized deletion by default; allow configurable retention policies. Offer permission-controlled CSV table exports and JSON structured-data/configuration exports. Ordinary exports exclude secrets; full backups are a separate operation.

Unknown values remain unknown rather than being represented as zero or inferred as confirmed. Subscription valuations configured for rules are not actual cash receipts. Multiplied score cannot inflate actual donation totals.

Public projections exclude credentials, email addresses, private notes, and unpublished supporter details. The API must enforce these limits, not merely hide fields in the UI. Honor source anonymity and amount visibility. Localized formatting is separate from the reporting currency.

**Settled:** section/access/privacy defaults; 30-day viewer-link and no-expiry overlay defaults with configurable expiry/revocation; metrics with cumulative lines/interval bars, optional category stacking and automatic/manual bucketing; source/arrival chart timestamps and refund-time corrections; extended archive correction timeline/end marker/last-update display; selectable timezone; archived page and retain-until-deletion/export defaults; server/viewer benchmark targets. **Open implementation/validation:** chart aggregation/bucketing, retention/deletion safeguards, export schemas, scoped token/expiry handling, and measured viewer scale.

## 14. Integration capability baseline

Documentation checked on 28 October 2026. No live credentials or end-to-end service tests were used. Undocumented/unverified capability is not a promise of absence or support.

**Agreed independent authorization and setup:** do not operate or depend on a project-hosted authorization broker, callback relay, or token service. Browser authorization callbacks and token exchanges belong to the streamer-hosted server. Prefer an integrated Link/Connect button wherever supported, opening the system browser from desktop and the provider authorization page from web management. Separate one-time installation setup from ordinary account authorization; avoid manual access/refresh-token copying when the provider supports an integrated authorization flow. Services exposing API keys or webhook configuration instead receive a guided setup appropriate to their actual capabilities. This is a usability requirement, not a promise that every provider supports OAuth or registration-free setup.

**Twitch setup where installation credentials are required:** guide the installation owner through registering an application, show the exact callback URL to copy, accept its client ID and secret, and validate the configuration before linking accounts. Keep secrets and provider tokens on the hosted server under the agreed encryption/permission policy. Broadcaster, separate bot, and invited users do not each register a developer application merely to authorize their accounts against that installation. Twitch's authorization-code flow requires an application secret; never ship one shared confidential project secret in the source or release packages. [S76–S77]

**Open Twitch registration minimization:** Twitch documents a public device-code flow without a client secret, but public clients cannot obtain app tokens using the client-credentials flow. [S77] Investigate whether a project-registered public application is appropriate under Twitch's application-registration guidance and can support the complete agreed API/chat and optional sign-in behavior without project-hosted infrastructure. Do not promise or select that approach until capability and registration-policy checks pass. Support guided installation-owned credentials wherever needed. The final preferred Twitch registration mode remains an implementation/feasibility decision; rejecting a central service does not by itself prove every installation must register its own application.

| Service | Delivery approach | Established baseline / remaining limitation |
| --- | --- | --- |
| Twitch | EventSub and chat API | Subscription/gift/Bits/chat/stream events. New-sub events exclude resubs; resub notifications depend on chat sharing. Not a complete renewal-payment feed. WebSocket loss has no replay for the gap. [S1–S3] |
| betterplace public | HTTP polling | Public totals and donation-related data; public visibility can hide names/amounts. Keep aggregate changes distinct from individual donations. Separate donation webhooks exist as another product but are not assumed available in this public-API scope. [S4–S6] |
| Patreon | API v2 and webhooks | Member updates include charging events; pledge history documents amount/currency/payment status, including refunds. Reconcile payment records, not every member update. Completeness and identifiers need live tests. [S7] |
| Streamlabs | Socket.IO and REST | Live donation events and paginated donation retrieval. REST distinguishes verified payments from streamer-added entries. Live-to-verified reconciliation needs tests. [S8–S9] |
| StreamElements | Astro WebSocket | Completed tips include status, transaction IDs, amount/currency. Moderation state is separate from payment status. Refund and complete recovery coverage remain unverified. [S10–S12] |
| Ko-fi | Payment webhook | Tips/membership/other payment notifications; repeated delivery uses the same message ID. API is payment-focused, not membership-ending notifications. Refund/history completeness remains unverified. [S13] |
| TipeeeStream | Socket.IO and REST | Live events and filtered event history. Socket documentation is dated 2019; current protocol/payload/confirmation/refund behavior needs testing. [S14–S15] |

Keep connector capability descriptors: events/fields, evidence, reversals, history/recovery, identifiers, outbound actions, authentication, and health. Surface limitations in setup and rule editing.

Where payment evidence is insufficient, streamer policy can accept the provider report with its evidence label, require review, or record without progression. Do not turn an unknown status into a confirmed payment.

Select authoritative sources for overlapping support. An increase in betterplace total is an aggregate observation, not evidence of a single donation of that amount. Aggregate rules must be explicitly distinct.

Record gaps and recovery outcomes. History-based catch-up can use saved cursors/IDs/timestamps where supported. Recovered input shows source/arrival timestamps.

**Agreed connection-gap catch-up:** offer configurable Review (default), Apply automatically, or Record without credit. Review supports individual/batch acceptance and existing silent-processing options. Automatic application still requires sufficient payment evidence, valuation, and duplicate checks. Show gaps the provider cannot recover. Recovered inputs retain agreed arrival-time rule handling and source timestamps for statistics; recovery does not imply a provider can replay missing events.

**Agreed first connection:** establish a baseline and count only new support by default, avoiding automatic credit of the account's entire history. Where supported, offer explicit historical import with a selected starting date. Imported contributions enter review and follow the same eligibility and duplicate checks.

Socket.IO adds a protocol over its transport. Streamlabs/TipeeeStream require a compatible C++ implementation/adapter; a bare WebSocket client is insufficient. Choose and test required protocol versions. [S16]

## 15. Exchange provider documentation baseline

| Provider | Relevant findings |
| --- | --- |
| Frankfurter | Daily updates; public API without key or daily/monthly quotas, with abuse rate limiting. Display freshness/status rather than inventing quota. [S17] |
| ExchangeRate-API | Free keyed plan updates daily. Quota endpoint reports remaining requests/reset day, may lag usage, and consumes quota itself. Open/no-key and keyed plans are distinct. [S18–S19] |
| Open Exchange Rates | Usage endpoint provides plan, update frequency, quota and usage. Usage checks do not consume the monthly request allowance. [S20–S21] |

Rates/prices/plan rules can change; validate the implemented plan profiles against current documentation during development. Keep reference allowances distinct from enforcement.

## 16. Deployment and operations

**Agreed:** hosted Linux application, persistent local state, recovery, and a public endpoint for applicable browser/webhook features. Support Docker, Podman, and native Linux installation. Use Caddy as the default reverse proxy and HTTPS setup. Linux server, Windows desktop, and Linux desktop builds target x86-64 only.

**Agreed application layout:** one application process with persistent SQLite storage, assets, and configuration, plus Caddy in the default deployment. Application endpoints remain behind the reverse proxy. Use one common application image for Docker and Podman, with separately tested deployment instructions. Native installation runs the same application as a system service; exact native package/service configuration remains an implementation decision. These are deployment requirements, not claims of completed testing or a selection of the application HTTP stack.

**Agreed external proxy support:** support an existing reverse proxy as an alternative to bundled Caddy. Provide configuration requirements for HTTPS, WebSockets, webhook routing, forwarded headers, and request limits. Trust forwarded headers only from explicitly configured proxies. Caddy remains the default setup.

**Agreed native server package:** publish a .tar.gz release archive containing the application, required runtime libraries, example configuration, and a systemd service definition. Run under a dedicated service account, with configuration and persistent data outside the application installation directory. Provide documented installation, upgrade, and removal procedures. Exact library selection, system dependencies, install paths, and service hardening remain implementation details.

**Agreed Linux desktop packaging:** use a bundled .tar.gz as the primary desktop download, containing the executable/launcher, required redistributable Qt libraries, Wayland/media plugins, QML imports, translations, and application resources. Keep preferences outside the extracted application directory so replacing it preserves settings, and test relocation/update behavior. Maintain a PKGBUILD for building against Arch system packages. AppImage is deferred to a possible later addition. Qt supports deploying application libraries/plugins with configurable relative paths; packaging still requires validation against the selected system baseline. [S44–S45] The AppImage references remain background for the evaluated alternative: it adds an executable runtime/filesystem image, normally mounted with FUSE, and still requires compatible payload libraries. [S46–S47]

**Agreed Docker deployment:** use Docker Compose with application and Caddy services, persistent storage, and documented startup, backup, upgrade, and removal commands. Use explicit release image versions and keep application endpoints behind Caddy. Docker documents Compose deployment on a single server. [S48]

**Agreed Podman deployment:** use Quadlet/systemd with tested rootless and rootful configurations. The default rootless setup runs the application container under a dedicated account and Caddy as a host service handling public HTTPS. Provide a rootful alternative with containerized Caddy. Podman documents rootless and rootful Quadlet service locations and behavior. [S49] Exact runtime minimums, unit configuration, startup persistence, port/network bindings, and storage permissions require implementation and validation.

Podman's `podman compose` delegates Compose handling to an external provider. [S27] It is background for the evaluated alternative; the selected Podman deployment uses Quadlet, so a Compose provider is not a required deployment dependency. Runtime support is an implementation target, not a claim that these deployments have already been tested.

**Agreed Windows desktop packaging:** distribute an installation-free ZIP that extracts to a user-chosen application folder. The folder contains the executable, bundled Qt/QML/plugins/media dependencies, resources, and required redistributable runtime files; users extract it and launch the executable. Multiple accompanying files are acceptable; a literal single executable and a custom static Qt build are not required. Do not require an installer, system-wide registration, or automatic Start-menu/file-association integration. Use portable folder mode for client-owned settings, cache, and logs so they remain with the chosen folder; the Linux preference-location policy remains separate. Validate startup under a standard account on a clean Windows 11 system without installing Qt or a separate compiler runtime first, including image/video/audio features. Qt supports ready-to-run deployment folders. [S50] Exact runtime redistribution, servicing, dependency manifests, and selected Qt/media/project license obligations remain implementation/validation work. [S52–S54] No Windows packaging prototype has yet been executed. Static-linking references remain background for the evaluated single-file alternative. [S51]

Qt documents that Qt HTTP Server must not be exposed directly to the internet. [S22] This remains background for the evaluated alternative; Drogon is the selected application HTTP/WebSocket layer. Review and validate the resulting Drogon/Caddy request path and Qt event-loop bridge during implementation; choosing the framework does not establish deployment security or completed tests.

**Agreed guided setup:** establish owner; configure public URL; choose language, timezone, and reporting currency; connect services and inspect limitations; configure timer, rules, and milestones; publish a scene; simulate events; start. Allow saving and resuming setup. Keep simulation effects separate from live accounting, and show unresolved requirements before starting.

Operational needs: visible per-service health, failed-job/review queues, tested database-and-asset backups, schema migrations, restore instructions, redacted logs, server/client API compatibility, and controlled shutdown/restart. Backups must include the recovery material required for encrypted credentials if that design is selected.

**Agreed backups:** automatic every 24 hours and before upgrades, plus manual requests. Keep the latest 14 automatic backups by default; manual backups remain until deleted. Configure schedule, retention, and destination directory. Include consistent database state, configuration, scenes/assets, and credential/key material required for restoration inside encrypted backup archives. Restoration requires an owner-held backup recovery key distinct from account recovery codes.

**Agreed backup encryption:** encrypt archives with libsodium secretstream using a fresh encryption key for each backup. Encrypt that per-backup key to an owner-controlled public/private backup key pair using libsodium sealed boxes. These APIs provide streamed file encryption and encryption to a recipient's public key. [S65–S66] The owner downloads/keeps the private recovery key; the server retains only the public key, allowing unattended backups without persistently keeping their decryption key on the server. This is a selected design, not a completed security review: the versioned archive/envelope format, key export/provisioning, authenticated metadata/finalization, consistency and restoration process require implementation review/testing. Sealed boxes provide recipient confidentiality/integrity rather than identifying the sender; the format must not claim sender authentication from sealed boxes alone. [S66]

**Agreed integration credential storage:** encrypt service tokens/API keys with libsodium XChaCha20-Poly1305 authenticated encryption. [S63] Store the encryption key outside SQLite in a permission-protected file or mounted secret, allowing unattended restarts. Include restoration material only inside encrypted backups, following the separate owner-held backup-recovery-key policy. Exact nonce/record format, key provisioning/rotation, protected-file/mount permissions, and implementation of the selected backup mechanism require validation.

**Agreed restoration:** use a server-console command while the application is stopped. Decrypt into a staging directory, validate integrity/schema/assets, and preserve the existing installation's data before replacement. On startup enter Recovery review, invalidate restored application sessions, and hold pending outbound actions for review. The hard end remains authoritative. Restoration must not silently replay old messages or automation. Staging/replacement atomicity, validation, clock recovery, connector reconciliation, and session/action handling need implementation tests.

**Agreed key rotation:** provide separate authorized operations for credential-encryption keys and backup-recipient keys. Credential-key rotation re-encrypts stored credentials and retains the old key until successful completion is verified. Backup-key rotation affects future backups; older archives still require their corresponding private keys. Record key identifiers and rotation actions without logging secrets. Crash-safe rotation and retention/provisioning of required keys remain implementation/validation work.

**Agreed updates:** explicitly initiated updates, with no unattended upgrades during an active subathon. Before applying an update, create a backup and present downtime/compatibility information. Run schema migrations as part of updates; rollback must restore matching database state when required by schema changes.

**Agreed diagnostics:** retain local operational logs for 14 days by default, configurable separately from accounting/audit retention. Permission-controlled diagnostic exports include versions, connection health, and redacted logs. Exclude secrets and supporter personal details by default; additional event details require explicit authorized selection. Ordinary secret-free exports and complete encrypted backups remain separate operations.

**Agreed server benchmark target:** target 2 vCPUs, 2 GiB RAM, and local SSD storage. Test 500 simultaneous public viewers, 10 management clients, and five overlays; five eligible inputs per second continuously and 50 per second in 30-second bursts. Target 95% of automatically eligible inputs committed internally within one second of server receipt. Exclude provider delays, intentional review holds, and scheduled announcements from that latency measurement. These are benchmark targets to validate, not measured minimum requirements or hard viewer limits. Record concrete CPU/storage/network/environment details and test results during implementation.

**Open implementation details:** Windows ZIP/portable-settings/runtime implementation, container/archive contents and dependency manifests, runtime minimum versions and Quadlet/service/network/storage details, Drogon/Caddy configuration and Qt bridge implementation, encryption/key provisioning and crash-safe rotation implementation, provider authentication library, DPAPI/QtKeychain persistence implementation, selected backup format/consistency/staged restoration implementation, compatibility handshake/migration/rollback mechanics, diagnostic redaction/export schema, and measured resource requirements. No deployment or restoration test has been performed. Staged stopped-server restoration with recovery/session/action holds, separate key-rotation policies, API capability/version compatibility, backup encryption/key ownership, remembered-login storage mechanisms, libsodium password/credential/session policies, server architecture, platform/package/deployment choices, and backup/update/diagnostic policies are settled.

## 17. Implementation sequence and AI-agent development

### Development handbook and task structure

**Agreed agent instructions:** prepare a repository-root AGENTS.md with the repository map, architectural boundaries, coding conventions, verified setup/build/test commands, and links to the specification and task backlog. Use scoped instructions for server, desktop, web, and tests where useful. Keep requirements authoritative in the specification and link its decision IDs rather than duplicating divergent copies. These files are planned repository deliverables, not files already created by this planning exercise.

**Agreed contracts and boundaries:** define public C++ interfaces, dependency directions, normalized events, API/WebSocket schemas, transaction ownership, and clock abstraction before assigning dependent implementations. Connectors submit normalized events through the authoritative intake/queue; they cannot write timer/accounting state or the database directly. Routine implementation choices can proceed within accepted requirements; a change to agreed observable behavior must be identified and recorded explicitly. Unverified provider capabilities remain unresolved rather than invented.

**Agreed task backlog:** divide work into small independently verifiable tasks. Each task records its ID, linked specification decisions/§18 scenarios, purpose, dependencies, scope/affected module, contract version, concrete acceptance criteria including relevant failures, and required verification. Record owner, status, blockers, resulting commit, and handoff notes. A replacement agent must be able to resume from repository records without the preceding chat. Use repository decision records for architectural choices and changes.

**Agreed parallel coordination:** use a coordinator/integrator, scoped implementation agents, and a separate review pass. Concurrent writers work on separate branches/worktrees with explicit ownership of overlapping files; assign shared schemas and interfaces an owner before parallel implementation. Integrate incrementally and rerun affected checks after integration. Parallelize independent tasks after contracts are settled; assigning more agents is not a substitute for resolving a shared dependency.

**Agreed simulator and evidence:** build reusable fake service adapters, a controllable clock, synthetic/appropriately redacted fixtures, and reviewed expected results early. Cover subscriptions/gift bundles, donations, duplicates, refunds, pauses, disconnects, and restarts. Ordinary development tests use separate test state and fake adapters; live service/platform checks remain separately identified. Keep task evidence explicit: implemented, tested with simulated data, and verified against the actual provider/platform are distinct claims. CI and local commands use the same selected framework/preset conventions. A task is done only when its acceptance behavior and relevant checks pass and affected contracts/documentation are updated; stubs, skipped tests, and unperformed platform/provider checks cannot be presented as completed functionality.

### Acceptance-test-first workflow

**Agreed initial assignment:** test-authoring agents work first, before feature-implementation agents begin production behavior. They derive a requirement-to-test plan and an initial executable core acceptance baseline from the agreed specification. Define the necessary public contracts and minimal build/test scaffolding first so those tests can run; scaffolding, fake adapters, and explicitly unimplemented interface stubs are allowed, without pre-implementing the behavior under test. Apply the same test-first handoff to subsequent modules as their contracts and dependencies become available. This does not require guessing every future UI interaction or undocumented provider capability before implementation starts.

| Stage | Responsibility and required result |
| --- | --- |
| Specify contracts and examples | Record the tested behavior, public interface/schema, linked decision/scenario IDs, input, expected result, and relevant boundary/failure cases; resolve ambiguous expectations before treating them as acceptance requirements |
| Author acceptance tests | Test agents write deterministic tests, fixtures, and expected results from the specification, independently of the future production algorithm |
| Review and demonstrate failure | A separate reviewer checks specification fidelity, meaningful assertions and coverage; build/run the baseline and confirm unimplemented behavior fails for the intended reason |
| Implement against baseline | Implementation agents receive the reviewed tests and contracts, implement the behavior, run the baseline, and may add meaningful lower-level or regression tests |
| Review and integrate | A separate review checks behavior, boundaries, test integrity, and evidence; integrate and run affected contract/regression checks, retaining any unperformed live/platform gates |

**Agreed red-baseline evidence:** record the command/environment and observed failing assertions or explicit unimplemented-operation failures. A missing dependency, compile/link error, empty test executable, or disabled/skipped suite is not evidence that a behavior test detects missing functionality. Expected values come from reviewed examples or independently derived results, not from calling the production calculation to generate its own expected answer. Use the existing GoogleTest/GoogleMock, Qt Test/Qt Quick Test, CTest, Vitest, and Playwright responsibilities; do not duplicate all behavior across frameworks.

A test-authoring task is complete when its tests are reviewed and the intended failure of unimplemented behavior is demonstrated. This marks the test baseline ready for implementation; it does not mark the feature complete or require those intentionally failing feature tests to pass before the handoff.

**Agreed baseline integrity:** implementation agents must not delete, disable, weaken, replace assertions, or regenerate expected fixtures merely to make the reviewed baseline pass. They can add tests. A genuine test defect, contract mismatch, or approved requirement change can justify baseline edits, with an explicit specification-based explanation and a separate test-review pass before the revised baseline is used for acceptance. Tests are reviewable artifacts, not an authority above the specification. Add a regression case when correcting a discovered defect. Passing the existing suite alone does not establish complete correctness or verified external support; retain review and the §19 validation gates.

Prioritize initial deterministic tests for duplicate intake, timer main/grace/pause/recovery behavior, exact rounding, cap-limited credit and its reversal, cumulative partial refunds, arrival-time rule/modifier context, sequential milestones, and permission boundaries. Record unknown provider behavior as a feasibility question; do not encode an assumption as verified capability. Isolated early feasibility prototypes remain allowed to discover protocol/platform blockers, but do not count as completed production features or replace the reviewed acceptance baseline.

### Feature implementation order

After the initial contract/test handoff, start with one complete path: simulated contribution → durable receipt → rule evaluation → credited effect → timer update → API output. It supplies a working reference for later tasks without reducing the intended release scope.

1. **Core:** types, database/migrations, transactions, timer states, pause reasons, grace/debt/recovery, deterministic clock abstraction.
2. **Accounting and automation:** contribution eligibility, rule versions, plans/effects/reversals, modifiers, milestones, action queue, simulator.
3. **Access and interfaces:** invitations/accounts/roles, permission checks, versioned API/live sync, desktop/web management, localization/themes.
4. **Connectors:** all seven integrations and three currency providers, capability descriptors, duplicate detection, evidence/recovery policies, diagnostics and tests.
5. **Presentation and distribution:** desktop Qt Quick scene editor/rendering, independent browser renderer, common scene-format checks, capture/browser outputs, viewer statistics, packaging, backups and upgrade verification.

Connector feasibility prototypes should happen early where protocol/payment capabilities are uncertain; the sequence does not postpone discovering blockers until the end. Every release requirement remains in scope.

## 18. Behavioral acceptance scenarios

| Scenario | Required observable result |
| --- | --- |
| A feature is assigned to an implementation agent | Reviewed specification-linked acceptance tests and contracts exist first, with observed intended failure for unimplemented behavior; implementation and integration run the baseline |
| A reviewed acceptance test appears incorrect | Record the specification-based reason and obtain a separate test-review pass before changing the baseline; do not weaken or skip tests just to obtain a passing result |
| Same input delivered twice | One contribution/application; delivery attempts remain traceable |
| Concurrent HTTP/WebSocket requests reach the server | Drogon bridges commands to the authoritative Qt queue in the same process; preserve controlled state/database writes and validate startup/shutdown/thread coordination |
| API clients are built or compatibility-checked | Validate JSON HTTP against source-controlled OpenAPI 3.1 and WebSocket messages against separately versioned JSON schemas for desktop/web consumers |
| Exact numeric values cross the API | Decimal/accounting and 64-bit integer values use strings; rational values use explicit numerator/denominator strings, with schema validation and no browser/native precision loss |
| Desktop connects to a different server release | Handshake advertises API versions/capabilities, compatible clients hide unavailable features, incompatible management clients receive guidance before operations, and matching web assets ship with server |
| Statistics reads overlap state writes | Qt SQL/SQLite uses one controlled writer and correctly owned read connections with WAL/synchronous=FULL; validate transaction/backup/recovery behavior |
| Gift bundle qualifies for 25 and 50 bonuses | Normal rule plus selected 50 bonus; 25 bonus excluded with explanation |
| Exclusive-group priorities tie | User-controlled order determines the winner and the preview explains the tie-break |
| A excludes B and B excludes C | Selected A suppresses B; skipped B cannot suppress C |
| Exclusion points backward or is contradictory | Configuration cannot be enabled until its ordering/exclusions are corrected |
| Generated-event chain reaches a configured limit | Stop further generation for that chain, log/notify, preserve committed effects, and keep the server operating |
| Two overlapping modifiers | Configured target-specific combination, recorded in calculation |
| Base award 100 with factors 2 and 3 in a group | Multiply gives 600; additive bonuses 400; highest 300; lowest 200; priority uses the selected factor |
| Modifier clock during main/grace, pause, or finish | Active-subathon default advances only with countdown progression; real elapsed remains selectable; post-finish effects require explicit enablement |
| Ten-minute modifier reactivated with three minutes left | Default Extend yields thirteen minutes; selected restart/replace/ignore behavior is respected |
| Modifier expires before paused input acceptance | Arrival-time modifier retained unless authorized override |
| Modifier definition or active instance is edited | Definition changes affect future activations by default; selected active-instance update requires permission/preview and explicit duration handling, preserving pending arrival context and prior credited effects |
| Calculation reaches an effect precision boundary | Round final time to milliseconds and score to six decimal places using half-to-even; retain source money precision and avoid intermediate effect rounding |
| Exact calculation is persisted | Use Boost.Multiprecision intermediates, signed 64-bit credited milliseconds/micro-points, lossless source decimals, checked final conversion/aggregation, and review rather than truncation on range overflow |
| Modifier/rate is invalid or arithmetic overflows | Reject negative/non-finite factors and non-positive rates; permit zero/fractional factors, clamp negative additive-result factors to zero, and hold overflowing input for review without effects |
| 55-minute timer, 60-minute cap, requested +10 | Actual +5 recorded; full refund requests reversal of that +5 |
| €5 refunded from €20 that credited five minutes and 40 points | Reverse 75 seconds and 10 points from actual credit; cumulative partial refunds avoid rounding drift and a full refund reverses exactly the remaining credit |
| Partial refund affects a whole count credit | Keep the count whole until full refund; preserve raw event history |
| Refund exceeds remaining time | Configured clamp/debt/review policy applied, override audited |
| Score falls below completed milestone | Completion retained, deficit visible, no automatic repeat reward |
| Large input crosses several milestones | Record all new achievements immediately in order; release action groups, including bonuses, sequentially with configured delays |
| Live milestone target is edited | Preview consequences, retain ascending order, require correction permission for a completed target, preserve completion/history, and select actions for newly achieved milestones |
| Authorized user manually completes milestones | Complete the next milestone or several sequentially without changing score, show deficit, explicitly select Run/Defer/Skip, and record actor/reason |
| Authorized user resets completion | Reset selected and later completed milestones, preserve history/applied effects, cancel unstarted groups, require score below first reset target or separate audited score correction, and require explicit authorization to replay performed actions on re-earning |
| Milestone presentations run across desktop/browser outputs | Follow the server timeline with global/per-milestone durations and default 10-second presentation/one-second gap; disconnected renderers cannot block progress and reconnecting joins the current presentation |
| Presentation queue or main countdown is paused | Support independent queue Pause/Resume by default and optional following of countdown pauses; retain independent review holds |
| Refund creates deficit for a milestone whose actions have not started | Default Review holds that group and later groups for an authorized choice; configured Continue/Skip alternatives work; completion and performed actions remain |
| Milestone actions are edited after a group is queued | Keep the group's captured definitions by default; authorized previewed replacement may affect only unstarted actions, preserving original definitions/history for started or completed actions |
| Authorized user stops an in-progress action group | Cancel unstarted actions and request supported playback stop; preserve applied effects/sent messages; hold the queue by default or explicitly continue subject to existing review holds |
| Silent batch crosses milestone | Explicit run/defer/skip choice; ordinary alerts suppressed |
| Manual pause and disconnect overlap | Clearing disconnect alone does not resume |
| Zero reached in default mode | Separate 120-second grace; accepted positive available time rescues; expiry finishes |
| Hard end reached during pause or grace | Subathon finishes; pause/grace does not postpone the enabled deadline |
| Positive accepted time arrives while waiting for finish confirmation | Resume the active main countdown automatically and clear the wait, preserving independent pauses/grace budget; zero actual credit does not resume and an observed hard end still wins |
| Main timer expires again after rescue | Fresh full grace by default, or only unused shared budget when that mode is selected |
| Brief configured disconnect | No new pause before the configured delay; persistent disconnect adds its reason and recovery clears only that reason |
| Reporting currency changes | Monetary displays recalculate; credited effects/completions do not |
| Rate sync fails / rate missing | Notice and aged cached rates / pending input respectively |
| Sync scheduler restarts or manual sync runs | Persist the 12-hour anchored automatic schedule; manual sync does not shift it |
| Primary provider fails | With fallback enabled try configured providers within their limits and record/show the actual source; otherwise retain aged cached rates |
| New reporting currency lacks a historical conversion | Use snapshot/cross-rate or an explicitly sourced historical/manual rate; unresolved displays/totals remain visibly unconverted/partial, never silently valued at today's rate |
| Rate fetch/publication exceeds warning thresholds | Show configurable default 24-hour fetch or 72-hour publication warning and both ages; cached valuation remains available |
| Rate sync repeatedly fails | Apply 1/5/15-minute then hourly delays within provider waits/quotas; invalid credentials and quota exhaustion use their dedicated waiting rules without duplicate jobs |
| Manual pair conflicts with inverse/cross-rate pins | Derive consistent inverse/cross-rates, preview changes, and block contradictory pins before saving |
| Paid-plan reference allowance changed | Display changes; provider restrictions do not |
| Permission revoked while page open | Subsequent unauthorized operation rejected |
| Local password is created or verified | Use salted libsodium Argon2id hash with recorded parameters, benchmark supported hardware, and upgrade parameters after successful login when needed |
| Account session is issued or used | Generate random 256-bit opaque token with server-side hash only; protected cookie/CSRF for browser or authorization header for desktop, no application session URL/local-storage token, immediate revocation and existing lifetimes |
| Windows client remembers a login or is moved | Store only DPAPI-encrypted session token within portable folder; no password; preserve preferences and normally require fresh sign-in on a different Windows user/computer |
| Linux client remembers a login without a usable keyring | Use QtKeychain/desktop keyring for token only; if unavailable, retain in memory, disable remembered login with explanation, and do not fall back to plaintext |
| Integration credential is persisted or server restarts | Store XChaCha20-Poly1305 encrypted credential, load key from separately protected file/secret mount for unattended restart, and place restoration material only inside encrypted backups |
| Different grants have different limits and targets | Request must fit one complete applicable grant after individual-override/deny resolution; constraints cannot be mixed to create broader authority |
| Administrator edits roles, invitations, or overrides | Changes, including changes affecting their own account, remain within owner-configured delegation scope; show effective-permission preview and distinguish use from grant authority |
| Invited account uses local or Twitch sign-in | Same account permissions apply; linking an identity does not grant extra rights |
| Helper or owner recovery | Use the authorized recovery route; consumed recovery codes cannot be reused and passwords are not disclosed |
| First owner is registered | Require a console-generated single-use setup code within 15 minutes and no existing owner; replacement invalidates prior code, owner sets local credentials/receives recovery codes, and successful setup disables initial registration |
| Sensitive operation requires fresh authentication | Authentication within the previous five minutes qualifies; older authentication requires renewal |
| Ownership transfer is proposed or accepted | Current owner authenticates freshly; recipient explicitly accepts within 24 hours with fresh authentication/local credentials; ownership changes only on acceptance and former-owner roles are selected with Observer default |
| Administrative helper or console owner recovery completes | Use manually shared single-use 15-minute reset link, revoke existing sessions, invalidate old recovery codes and offer replacements, preserve permissions, and preserve existing owner identity/ownership in console recovery |
| Normal role edit attempts to change ownership | Ownership remains protected; only the explicit authorized transfer/recovery procedure can change it |
| Invitation expires or is revoked/consumed | It cannot create another account; selected initial roles remain constrained by inviter authority |
| Normal/remembered session reaches its configured maximum | Fresh sign-in is required; expired application access does not itself terminate the server process |
| Bot configured but a message selects broadcaster | That message uses the authorized broadcaster identity, independently of the default sender |
| Explicit chat sender becomes unavailable | Hold with visible failure by default; use only an explicitly configured authorized fallback when known undelivered, and require review if outcome is uncertain |
| User sends or authorizes automated chat | Enforce separate bot/broadcaster permissions, including authorization of automated messages |
| Two reviewers accept concurrently | One application; other operation gets current result/conflict |
| Server restarts during countdown/action queue | Configured clock recovery; no blind repeat of performed actions |
| Downtime policies applied to a five-minute gap | Count-elapsed deducts the gap; freeze retains remaining duration; review waits for an authorized choice; hard end still applies |
| Recurring schedule crosses a daylight-saving change | Default shifts nonexistent local time forward by the gap and runs repeated local time once; configurable Skip/Run twice alternatives and occurrence previews work |
| Schedule timezone is changed | Preview future occurrences in the selected subathon timezone, retain UTC instants internally, and do not replay past occurrences |
| Abrupt crash occurs between clock checkpoints | Freeze restores the last durable checkpoint; Count elapsed reconstructs scheduled pauses, main/grace transitions, and hard-end precedence; default checkpoint interval is one second and timer-changing operations save state |
| Recovery awaits an authorized decision | Recovery review pauses main/grace and active-subathon modifiers, permits real-elapsed modifiers/hard end to advance, holds support and unresolved eligibility, and previews offline Count elapsed/Freeze with permitted correction |
| Recovery review is resolved while another pause remains | Waiting-for-review time remains paused; clear only Recovery review and retain all other active reasons |
| Twitch status becomes unknown | Default preserves current state and reports uncertainty; configured timeout may add a separate pause; confirmed-offline reason is not cleared |
| Finished subathon is reopened | Specific permission required; preserve history/score/milestones, explicitly choose time/grace handling, and resolve any passed hard end |
| Reopened subathon has debt or pending actions | Preserve debt unless separately authorized audited correction; choose Resume/Keep deferred (default)/Skip for pending groups, preserve review holds, and do not replay performed actions |
| Aggregator mirrors Twitch activity | One counting path according to source selection/correlation |
| Connector recovers support after a gap | Default Review supports individual/batch and silent handling; selected automatic/record-only alternatives respect evidence, valuation, duplicates, arrival-time rules, and explicit unrecoverable-gap notices |
| Service is connected for the first time | Establish a new-support baseline without automatically crediting historical activity; optional supported import uses a selected start date, review, and normal eligibility/duplicate checks |
| English/German or light/dark switches | Individual UI updates; other users and scene appearance unaffected |
| Desktop/web/viewer built-in language changes | English/German available with detection/English fallback and immediate switching; authored content is unchanged |
| Shared translation catalog changes | Generate corresponding Qt/web formats and validate missing entries, placeholders, plural forms, and German layouts |
| Scene used by desktop and browser | Separate renderers interpret supported scene data consistently; unsupported features are identified |
| Shared renderer reference scenes are validated | Match bindings, visibility, geometry, progress calculations, and animation timing on each renderer; allow minor font-rendering differences and flag incompatible features before publication to both outputs |
| Browser interfaces are built and deployed | Build React/TypeScript management/viewer assets with Vite and ship static output with the C++ server; no separate production Node.js service |
| Browser workflows and compatibility are validated | Use Vitest unit/component and Playwright workflow tests; cover current/previous Chrome/Edge/Firefox, best-effort Safari, and separate actual OBS overlay checks on Windows/Linux with recorded versions |
| Media/font assets imported or exported | Validate documented image/animation/audio/video profiles and restricted SVG; package permitted TTF/OTF fonts and use explicit fallback on both renderers |
| Asset limits or SVG profile are applied | Enforce configurable per-file/scene/raster-dimension defaults; validate decoded dimensions, warn on demanding scenes, accept supported static SVG features, reject exclusions, and require SVG text paths |
| Scene resized or frames missed | Default aspect-fit and selectable crop/stretch work; elapsed-time animation timing preserves duration; benchmark 1080p at target 60/optional 30 FPS |
| Hidden source amount or unknown subscription detail | Unknown/hidden remains explicit; no fabricated amount or classification |
| Viewer page first configured | Invited-account access is default; show milestones and latest 10 support events with source-permitted name/amount visibility |
| Viewer link or account accesses the page | Enforce the chosen read-only access mode and projection; expiry/revocation removes link access without granting management |
| Viewer or overlay link reaches configured expiry | Default viewer expiry is 30 days, configurable including Never; default overlay credential has no expiry; revocation/replacement and configured read-only scopes remain enforced |
| Streamer anonymizes/hides a viewer entry | Apply global/source/entry visibility settings and source privacy without erasing accounting or changing credited effects |
| Statistics and graph modes selected | Cumulative/interval views use recorded support/effects, default net money, and explicit unknown classifications without inflated monetary totals |
| Late input or refund enters statistics | Place source-timed support in its historical bucket and refunds at refund time; keep arrival-time rule policy and default event-window/post-finish inclusion rules |
| Default charts or archived corrections are displayed | Use cumulative lines/interval bars and appropriate stacking, auto-select roughly 100–200 buckets with manual override, and extend archive timeline through later refunds with event-end marker and last-updated time |
| Event finishes or ordinary data is exported | Preserve archived page access/privacy and default retained history; enforce export permissions, CSV/JSON formats, and secret exclusion |
| Event finishes | Input recording continues; progression/actions obey post-finish policy |
| Server installed with Docker, Podman, or natively | Same application behavior and recoverable persistent state; each path is independently tested on x86-64 |
| Server load profile is benchmarked | Measure 2-vCPU/2-GiB/local-SSD target at 500 viewers/10 managers/5 overlays with 5 eligible inputs/s and 50/s 30-second bursts; report eligible receipt-to-commit p95 against one-second target, exclusions, and actual environment |
| Native application build is prepared | Require Qt 6.12 or newer and the release's pinned tested Qt patch; official Windows build targets Windows 11/x86-64 with MSVC 2022; validate Linux deployment targets separately from the Arch rolling development environment |
| Linux desktop and capture behavior is validated | Test native Wayland on reference KDE Plasma, including OBS timer transparency/resize/reconnect; treat other compositors as best-effort until validated and exclude X11/XWayland from initial support commitment; headless server needs no display system |
| Linux release compatibility is validated | Test desktop on Arch rolling/Kubuntu 24.04 and native server on Ubuntu 24.04; build portable releases in a pinned Ubuntu-based environment and validate bundled/system dependencies |
| Native server archive is installed or upgraded | Supply application/runtime libraries/example configuration/systemd definition in .tar.gz, run as dedicated service account, preserve separately stored configuration/data, and document installation/upgrade/removal |
| Linux desktop package is installed or replaced | Bundled .tar.gz supplies executable/launcher and required Qt/QML/plugins/resources; relocation works on tested targets, settings survive replacement outside application directory, and maintained Arch PKGBUILD builds against system packages |
| Windows ZIP is extracted and launched | Run from the chosen folder under a standard account on clean Windows 11 with bundled runtime/Qt/QML/media dependencies and no installer or separate runtime installation; retain client settings/cache/logs in portable folder mode and validate all media features |
| Default public setup | Caddy routes public HTTPS traffic; its persistent certificate state survives supported updates/restarts |
| Common application is deployed through supported methods | Run one application process with persistent SQLite/assets/configuration behind the proxy; test the same application image separately with Docker/Podman and run the native application as a system service |
| Docker deployment is started or upgraded | Use Compose/application+Caddy with persistent storage and explicit release image versions; keep application behind Caddy and document startup/backup/upgrade/removal |
| Podman deployment is started or upgraded | Use Quadlet/systemd; default dedicated-account rootless application plus host Caddy, with tested rootful/containerized-Caddy alternative and validated persistence/network/storage behavior |
| Existing reverse proxy replaces bundled Caddy | Meet documented HTTPS/WebSocket/webhook/header/request-limit requirements and trust forwarded headers only from configured proxies |
| Provider setup or account linking is performed | Require no project-hosted authorization service; use direct supported authorization, guide required installation registration/keys/webhooks, and keep secrets server-side; public-client registration minimization remains unverified |
| Initial setup is saved, resumed, or simulated | Resume owner/URL/preferences/services/timer/rules/milestones/scene setup, keep simulated effects separate from live accounting, and show unresolved requirements before starting |
| Automatic/manual/pre-upgrade backup | Create a consistent encrypted restorable snapshot; apply configurable retention with 14 automatic backups by default and preserve manual backups until deletion |
| Unattended backup is encrypted or restored | Use secretstream with fresh per-backup key sealed to owner public key; server persistently retains public key only, owner private recovery key is required for decryption, and validate format/integrity/finalization/restore |
| Backup is restored to an installation | Stop application and use console command; stage/decrypt/validate integrity/schema/assets, preserve prior data before replacement, then enter Recovery review with restored sessions invalidated and outbound actions held; enforce hard end and avoid silent replay |
| Encryption keys are rotated | Separate authorized credential/backup-key operations; verify credential re-encryption before dropping old key, use new backup key for future archives while retaining old private keys for older archives, and audit identifiers without secrets |
| Authorized update/rollback | Explicitly initiated, with pre-update backup and compatibility information; match code/schema state on rollback and avoid unattended active-event upgrades |
| Operational logs or diagnostic exports handled | Apply separate 14-day default log retention and export permissions/redaction; extra event details require explicit selection and accounting/audit retention remains unchanged |

These are behavior scenarios for future implementation tests, not tests that have already been executed.

## 19. Remaining choices and implementation/validation work

### Decision-review status and remaining metadata

The reviewed product/policy/architecture decisions are settled, including link-expiry defaults, chart presentation, and the single private-during-development GitHub repository approach. No further acceptance of those choices is needed. Final branding and the concrete repository owner/name/location remain metadata to choose when implementation starts; Twitch Subathon Bot remains the working title.

The following engineering choices and validation gates remain deliberately unresolved until implementation/prototypes provide evidence. They must not be represented as completed tests or guaranteed capabilities.

### Engineering implementation decisions

- Prepare the repository AGENTS.md/scoped instructions, module contracts, task backlog/decision records, initial test-agent assignments, requirement-to-test map, runnable reviewed acceptance baseline with intended-failure evidence, simulator fixtures, and CI/handoff conventions before production feature implementation. No executable tests or agents have been launched by this specification update.
- Pin compatible public compiler/build/runtime/library versions and package manifests after the first builds. Qt 6.12 minimum, C++20, selected platforms, packaging formats, and framework choices are already agreed. Define detailed Windows release support at that stage.
- Implement Drogon/Qt lifecycle/thread bridge, controlled Qt SQL writes/read connections, OpenAPI/WebSocket contracts, capability handshake, migrations, and rollback.
- Select/validate the compatible Socket.IO adapter and provider OAuth/OIDC/reauthentication components against the services' actual protocols. These remaining library choices depend on feasibility work; they are not promises of live compatibility.
- Implement the easiest supported provider onboarding without a project-hosted authorization service: generated callback guidance, installation-owned credentials where required, and integrated browser linking. Validate Twitch public-client suitability, complete feature/token/sign-in coverage, and application-registration guidance before choosing a registration-minimizing default; no registration-free promise is made.
- Specify bounded exact-number parsing/aggregation, revision and effective-rate-table schemas, and unavailable-provider-metadata handling within the agreed arithmetic/accounting rules.
- Implement token generation/verification/rotation, permission checks, DPAPI/QtKeychain persistence, credential/key rotation, and the reviewed backup format/snapshot/staged restoration process.
- Implement the JSON translation schema/generator, locale/theme preferences, packaged Noto Sans/fallback assets, chart aggregation, export schemas, retention/deletion safeguards, and scoped link tokens.
- Choose tested media codec profiles, minimum OBS/embedded-browser versions, concrete graphics benchmark hardware, and desktop editor components within the agreed media/SVG/asset policies.

### Validation gates before claiming support

- Run core accounting/refund/rule/modifier/milestone and combined clock/pause/recovery/correction scenarios, including bounded input and concurrent operation cases.
- Test provider payment evidence, identifiers, overlap/deduplication, refunds/history/catch-up and reconnect gaps with the necessary test accounts/data. In particular retain the documented Twitch renewal and betterplace public-data limitations.
- Test native Wayland/KDE and Windows capture, both scene renderers/reference scenes/media profiles, actual OBS, browser matrix, German layouts, portable package relocation/clean-machine startup, and remembered-login behavior.
- Validate each Docker/Podman/native deployment, lifecycle/proxy/storage permissions, credential/key handling, consistent encrypted backups, restore, schema migrations and supported client/server combinations.
- Measure the accepted server/viewer load and 1080p/60 FPS renderer targets on recorded hardware; derive actual minimum requirements from results.
- Review dependency/font licenses, corresponding source/notices, release manifests, and diagnostic redaction before distribution.

Acceptance settles the specified design; it does not substitute for these builds, implementations, measurements, or live service tests. No requested integration is removed by treating unresolved capability checks as validation tasks.

## 20. Official references

References support the documentation findings above; architectural proposals are design recommendations. S1–S26 were accessed during planning on 28 October 2026; S27–S77 were checked on 29 October 2026.

- **S1:** [Twitch EventSub subscription types](https://dev.twitch.tv/docs/eventsub/eventsub-subscription-types)
- **S2:** [Twitch PubSub migration and resubscription equivalence](https://dev.twitch.tv/docs/pubsub)
- **S3:** [Twitch EventSub WebSocket handling and recovery limits](https://dev.twitch.tv/docs/eventsub/handling-websocket-events/)
- **S4:** [betterplace official API documentation](https://github.com/betterplace/betterplace_apidocs)
- **S5:** [betterplace fundraising-event data](https://github.com/betterplace/betterplace_apidocs/blob/master/sections/fundraising_events_list.md)
- **S6:** [betterplace donation anonymity and hidden amounts](https://content-help.betterplace.org/c/help/can-i-donate-anonymously-andor-hide-the-donation-amount)
- **S7:** [Patreon API v2, payment history, and webhooks](https://docs.patreon.com/)
- **S8:** [Streamlabs Socket API](https://dev.streamlabs.com/docs/socket-api)
- **S9:** [Streamlabs donation retrieval and verified filter](https://dev.streamlabs.com/reference/donations)
- **S10:** [StreamElements Astro gateway](https://docs.streamelements.com/websockets)
- **S11:** [StreamElements completed tips](https://docs.streamelements.com/websockets/topics/channel-tips)
- **S12:** [StreamElements tip moderation](https://docs.streamelements.com/websockets/topics/channel-tips-moderation)
- **S13:** [Ko-fi payment webhooks and limitations](https://help.ko-fi.com/hc/en-us/articles/360004162298-Does-Ko-fi-have-an-API-or-webhook)
- **S14:** [TipeeeStream Socket.IO guide](https://api.tipeeestream.com/api-doc/socketio)
- **S15:** [TipeeeStream event history](https://api.tipeeestream.com/api-doc/events)
- **S16:** [Socket.IO protocol versus WebSocket](https://socket.io/docs/v4/)
- **S17:** [Frankfurter API](https://frankfurter.dev/)
- **S18:** [ExchangeRate-API open/free/paid distinctions](https://www.exchangerate-api.com/docs/free)
- **S19:** [ExchangeRate-API quota endpoint](https://www.exchangerate-api.com/docs/request-quota-endpoint)
- **S20:** [Open Exchange Rates usage endpoint](https://docs.openexchangerates.org/reference/usage-json)
- **S21:** [Open Exchange Rates usage accounting](https://support.openexchangerates.org/article/60-managing-api-usage-request-volume)
- **S22:** [Qt HTTP Server security considerations](https://doc.qt.io/qt-6/qthttpserver-security.html)
- **S23:** [Qt localization](https://doc.qt.io/qt-6/localization.html)
- **S24:** [Qt Quick light/dark/system theme support](https://doc.qt.io/qt-6/qtquickcontrols-universal.html)
- **S25:** [Qt WebEngine embedding](https://doc.qt.io/qt-6.5/qtwebengine-overview.html)
- **S26:** [SQLite application-server suitability](https://www.sqlite.org/whentouse.html)
- **S27:** [Podman Compose provider behavior](https://docs.podman.io/en/latest/markdown/podman-compose.1.html)
- **S28:** [Qt Multimedia backends and codec/platform differences](https://doc.qt.io/qt-6/qtmultimedia-index.html)
- **S29:** [Qt application font loading](https://doc.qt.io/qt-6.12/qfontdatabase.html)
- **S30:** [Qt C++20 support](https://doc.qt.io/qt-6/cpp20-overview.html)
- **S31:** [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
- **S32:** [Qt Test overview and CTest integration](https://doc.qt.io/qt-6/qtest-overview.html)
- **S33:** [GoogleTest primer](https://google.github.io/googletest/primer.html)
- **S34:** [GoogleMock cookbook](https://google.github.io/googletest/gmock_cook_book.html)
- **S35:** [Qt Quick Test](https://doc.qt.io/qt-6/qtquicktest-index.html)
- **S36:** [CMake GoogleTest discovery and CTest integration](https://cmake.org/cmake/help/latest/module/GoogleTest.html)
- **S37:** [Qt 6.12 release announcement](https://www.qt.io/blog/qt-6.12-released)
- **S38:** [Qt 6.12 supported platforms and binary build baseline](https://doc.qt.io/qt-6/supported-platforms.html)
- **S39:** [Arch Qt base](https://archlinux.org/packages/extra/x86_64/qt6-base/) and [Qt Wayland packages](https://archlinux.org/packages/extra/x86_64/qt6-wayland/)
- **S40:** [Qt platform abstraction and plugins](https://doc.qt.io/qt-6/qpa.html)
- **S41:** [Qt window-system peculiarities](https://doc.qt.io/qt-6/application-windows.html)
- **S42:** [OBS native Wayland/PipeWire capture](https://obsproject.com/blog/obs-studio-27-released)
- **S43:** [XDG ScreenCast portal and backend capabilities](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.ScreenCast.html)
- **S44:** [Qt Linux application deployment](https://doc.qt.io/qt-6/linux-deployment.html)
- **S45:** [Qt configurable relative deployment paths](https://doc.qt.io/qt-6/qt-conf.html)
- **S46:** [AppImage runtime architecture](https://docs.appimage.org/reference/architecture.html)
- **S47:** [AppImage payload compatibility practices](https://docs.appimage.org/reference/best-practices.html)
- **S48:** [Docker Compose production deployment](https://docs.docker.com/compose/how-tos/production/)
- **S49:** [Podman Quadlet/systemd units](https://docs.podman.io/en/stable/markdown/podman-systemd.unit.5.html)
- **S50:** [Qt Windows deployment](https://doc.qt.io/qt-6/windows-deployment.html)
- **S51:** [Static QML plugin imports](https://doc.qt.io/qt-6/qt-import-qml-plugins.html)
- **S52:** [Building Qt Multimedia and FFmpeg linkage](https://doc.qt.io/qt-6/qtmultimedia-building-from-source.html)
- **S53:** [Microsoft runtime redistribution and servicing](https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files)
- **S54:** [Qt open-source licensing FAQ](https://www.qt.io/faq/qt-open-source-licensing)
- **S55:** [React/TypeScript/Vite application build approach](https://react.dev/learn/build-a-react-app-from-scratch)
- **S56:** [Vitest browser-mode documentation](https://github.com/vitest-dev/vitest/blob/main/docs/guide/browser/index.md)
- **S57:** [Playwright browser support and testing](https://playwright.dev/docs/browsers)
- **S58:** [Drogon framework capabilities](https://github.com/drogonframework/drogon)
- **S59:** [OpenAPI 3.1 specification](https://spec.openapis.org/oas/v3.1)
- **S60:** [Qt SQL database connections/thread ownership](https://doc.qt.io/qt-6/qsqldatabase.html)
- **S61:** [SQLite WAL concurrency, synchronization, and recovery considerations](https://sqlite.org/wal.html)
- **S62:** [Libsodium password hashing and stored parameters](https://doc.libsodium.org/password_hashing/default_phf)
- **S63:** [Libsodium XChaCha20-Poly1305 authenticated encryption](https://doc.libsodium.org/doc/secret-key_cryptography/aead/chacha20-poly1305/xchacha20-poly1305_construction)
- **S64:** [OWASP session management guidance](https://cheatsheetseries.owasp.org/cheatsheets/Session_Management_Cheat_Sheet.html)
- **S65:** [Libsodium encrypted streams/file encryption](https://doc.libsodium.org/secret-key_cryptography/secretstream)
- **S66:** [Libsodium sealed boxes and recipient-key encryption](https://libsodium.gitbook.io/doc/public-key_cryptography/sealed_boxes)
- **S67:** [Windows DPAPI CryptProtectData scope](https://learn.microsoft.com/en-us/windows/win32/api/dpapi/nf-dpapi-cryptprotectdata)
- **S68:** [QtKeychain supported storage backends and fallback behavior](https://github.com/frankosterfeld/qtkeychain/)

- **S69:** [Boost.Multiprecision cpp_int](https://www.boost.org/latest/libs/multiprecision/doc/html/boost_multiprecision/tut/ref/cpp_int_ref.html) and [cpp_rational](https://www.boost.org/latest/libs/multiprecision/doc/html/boost_multiprecision/tut/interval/rational/cpp_rational.html)
- **S70:** [SQLite integer/REAL/text storage and affinity](https://www.sqlite.org/datatype3.html)
- **S71:** [JSON numeric interoperability and precision, RFC 8259](https://www.rfc-editor.org/rfc/rfc8259.html)

- **S72:** [GNU GPL source/distribution and privately hosted modifications FAQ](https://www.gnu.org/licenses/gpl-faq.en.html)
- **S73:** [Noto fonts SIL Open Font License](https://github.com/notofonts/noto-fonts/blob/main/LICENSE)
- **S74:** [Qt translation file format](https://doc.qt.io/qt-6/linguist-ts-file-format.html)

- **S75:** [GitHub repositories and public/private visibility](https://docs.github.com/en/repositories/creating-and-managing-repositories/about-repositories)

- **S76:** [Twitch application registration, client IDs, and client-secret confidentiality](https://dev.twitch.tv/docs/authentication/register-app)
- **S77:** [Twitch OAuth flows and public device-code client limitations](https://dev.twitch.tv/docs/authentication/getting-tokens-oauth)

## 21. Decision log

| Decision | Date | Resolution |
| --- | --- | --- |
| UI-01 | 28 October 2026 | Qt Quick/QML desktop presentation with C++ application logic accepted |
| UI-02 | 28 October 2026 | Shared desktop/browser renderer rejected; separate native Qt Quick and browser renderers read a common scene format |
| UI-03 | 28 October 2026 | Full scene editor required on desktop only; shared embedded web editor rejected |
| DEP-01 | 29 October 2026 | Support Docker, Podman, and native Linux server installation |
| DEP-02 | 29 October 2026 | Include Caddy as the default reverse-proxy/HTTPS setup |
| DEP-03 | 29 October 2026 | Support x86-64 only for the Linux server, Windows desktop, and Linux desktop |
| AUTH-01 | 29 October 2026 | Support local username/password login and optional Twitch sign-in/linking through the invitation/account system; owner keeps local fallback |
| AUTH-02 | 29 October 2026 | Use single-use recovery codes and authorized administrative/server-console recovery only; no email-based recovery |
| AUTH-03 | 29 October 2026 | One protected owner retains access-management/recovery outside editable roles; ownership can be explicitly transferred |
| AUTH-04 | 29 October 2026 | Revocable single-use invitations are copied manually, expire after 48 hours by default with configurable expiry, and carry inviter-authorized initial roles |
| AUTH-05 | 29 October 2026 | Owner-configurable normal/remembered session defaults are 24 hours/30 days; support session inspection/revocation and fresh authentication for sensitive account changes |
| CHAT-01 | 29 October 2026 | Support bot and broadcaster senders, including explicit broadcaster selection for particular messages while a bot is configured |
| TIMER-01 | 29 October 2026 | Enabled hard end takes precedence over countdown pause and grace |
| TIMER-02 | 29 October 2026 | Support fresh grace for each expiration (default) and an optional shared remaining grace budget |
| TIMER-03 | 29 October 2026 | Separate configurable offline/client-disconnect delays default to 30 seconds; recovery automatically clears only the recovered reason |
| TIMER-04 | 29 October 2026 | Support elapsed, freeze (default), and review downtime handling for main/grace countdowns; hard end and modifier clock policies remain authoritative |
| TIMER-05 | 29 October 2026 | Unknown stream status preserves current state with notice by default, with optional pause-after-timeout; it cannot clear a confirmed-offline pause |
| TIMER-06 | 29 October 2026 | Permission-controlled reopening preserves history/score/milestones, explicitly selects time/grace budget, and requires resolving an already-passed hard end; ordinary support does not reopen |
| RULE-01 | 29 October 2026 | Highest numeric priority wins within an exclusive group; ties use explicit user-controlled order with editor warning/preview |
| RULE-02 | 29 October 2026 | Higher-ranked selected rules may exclude lower-ranked rules; backward/contradictory exclusions block enabling, and skipped rules apply no exclusions |
| RULE-03 | 29 October 2026 | Configurable per-root generated-event limits default to depth 32 and 256 generated events; halt further generation for the affected chain with audit/notice, not the server |
| MOD-01 | 29 October 2026 | Named groups and target-level combination support multiply (default), additive bonuses, highest, lowest, and priority selection independently per target |
| MOD-02 | 29 October 2026 | Active-subathon clock is default: runs with main/grace, freezes during pause/finish, follows countdown downtime policy; real elapsed is optional and post-finish effects require enablement |
| MOD-03 | 29 October 2026 | Same-modifier reactivation supports Extend (default), Restart, Replace, and Ignore |
| MILE-01 | 29 October 2026 | Record crossed achievements immediately in order; release milestone action groups, including announcements/bonuses, sequentially with configured delays |
| MILE-02 | 29 October 2026 | Preview live target edits and keep ascending order; completed-target edits require correction permission and retain completion; explicitly choose Run/Defer/Skip for newly achieved targets |
| MILE-03 | 29 October 2026 | Refund-caused deficit before actions start uses Continue, Review (default), or Skip; Review blocks later action groups, while completion/performed actions remain |
| FX-01 | 29 October 2026 | Sync on configuration, then every 12 hours anchored to first success; preserve schedule across restarts and do not reset it on manual sync |
| FX-02 | 29 October 2026 | Optional ordered provider fallback is disabled by default, respects per-provider limits, and records/displays failures and the replacement source |
| FX-03 | 29 October 2026 | Missing historical reporting rates use snapshot/cross-conversion, supported historical retrieval, or authorized manual input; allow visibly partial totals without silently using today's rate or changing credited effects |
| FX-04 | 29 October 2026 | Configurable warnings default to 24 hours without successful fetch or 72-hour-old published rates; show both ages and keep cached valuation usable |
| FX-05 | 29 October 2026 | Retry delays default to 1/5/15 minutes then hourly within provider limits; wait for credential correction/quota reset as appropriate and coalesce/account for jobs |
| FX-06 | 29 October 2026 | One consistent effective rate table derives inverses/cross-rates, previews manual changes, rejects conflicting pins, preserves pins across updates, and leaves overrides unchanged on reporting-currency changes |
| I18N-01 | 29 October 2026 | English/German for desktop, web management, and built-in viewer labels; individual immediate switching with device/browser detection and English fallback; no automatic authored-content translation |
| I18N-02 | 29 October 2026 | Desktop/web management support Light, Dark, and System (default), with immediate individual changes independent of scenes and viewer-page design |
| I18N-03 | 29 October 2026 | One authoritative translation catalog generates Qt/web formats; validate completeness, placeholders, plurals, and German layouts |
| VIEW-01 | 29 October 2026 | Configurable milestone/recent-support/statistics/timer/modifier sections; default milestones and latest 10 support events |
| VIEW-02 | 29 October 2026 | Support Public, revocable read-only Link access with configurable expiry, and Invited accounts (default); viewer access grants no management rights |
| VIEW-03 | 29 October 2026 | Default source-permitted display names/amounts; global/per-source anonymization and amount hiding plus individual entry hiding; never expose private identifiers/notes or override source privacy |
| STATS-01 | 29 October 2026 | Cumulative/interval graphs cover gross/refund/net money (net default), subscription classifications where known, Bits, and credited score/time; unknown remains explicit |
| STATS-02 | 29 October 2026 | Reliable source time else arrival for charts; historical catch-up buckets and refund-time negative entries; selectable event timezone with UTC storage; event support plus corrections by default, post-finish support opt-in |
| DATA-01 | 29 October 2026 | Keep completed viewer page as archive with preserved access/privacy; retain history/effects/rates/audit until authorized deletion by default; configurable retention and permissioned CSV/JSON exports without secrets |
| OPS-01 | 29 October 2026 | Encrypted full backups every 24 hours, pre-upgrade, and manually; retain latest 14 automatic by default and manual backups until deletion; owner-held backup recovery key is separate from account codes |
| OPS-02 | 29 October 2026 | Explicit updates, no unattended active-event upgrades, backup/compatibility information first, migrations as needed, and matching database state for rollback |
| OPS-03 | 29 October 2026 | Local operational logs default to 14 days; permissioned diagnostic exports include redacted versions/health/logs without secrets or supporter personal details by default |
| MEDIA-01 | 29 October 2026 | Target PNG/JPEG/WebP/static restricted SVG, GIF, MP4/WebM, WAV/MP3/Ogg with tested/documented profiles, plus configurable fade/move/scale/rotate animations |
| MEDIA-02 | 29 October 2026 | Default 1920×1080 custom-size canvas, target 60 FPS and optional 30 FPS, default aspect-fit with crop/stretch, and elapsed-time animation timing |
| MEDIA-03 | 29 October 2026 | Support TTF/OTF uploads, a bundled default and explicit fallback; distribute permitted scene fonts with themes to both renderers |
| NUM-01 | 29 October 2026 | Millisecond time effects and six-decimal score; preserve source monetary precision, use high-precision decimal/exact rational intermediates, and round final effects half-to-even independently of display precision |
| NUM-02 | 29 October 2026 | Permit zero/positive finite modifier factors, clamp negative additive-result factors to zero, require positive exchange rates, hold numerical overflow for review, and retain signed corrections/refunds |
| NUM-03 | 29 October 2026 | Partial refunds proportionally reverse actual credited time/score using cumulative entitlement minus prior reversals; full refund clears exact remaining credit, whole counts reverse on full refund, and raw history/deficit policies remain |
| TIMER-07 | 29 October 2026 | Use subathon timezone/UTC storage for schedules; default DST-gap shift forward and repeated-time single execution, with Skip/Run twice alternatives and future-occurrence previews; timezone changes do not replay history |
| TIMER-08 | 29 October 2026 | Save clock state on timer changes and checkpoint advancing clocks every second by default; Freeze restores durable state, while Count elapsed reconstructs schedule/phase/deadline progression |
| TIMER-09 | 29 October 2026 | Separate Recovery review pause freezes main/grace and active-subathon modifiers while real-elapsed modifiers/hard end continue; hold inputs/unresolved eligibility, preview offline-gap decision/correction, keep review-wait time paused, and clear only its pause reason |
| TIMER-10 | 29 October 2026 | Reopening retains debt unless separately authorized audited correction; offer Resume/Keep deferred (default)/Skip for pending groups while preserving review holds and performed actions |
| MILE-04 | 29 October 2026 | Queued groups retain captured action definitions; explicit authorized previewed replacement affects only unstarted actions, while started/completed definitions and history remain |
| MILE-05 | 29 October 2026 | Authorized group stop cancels unstarted actions and requests supported playback stop without undoing effects/messages; choose Continue or Hold (default), retaining independent review holds |
| MILE-06 | 29 October 2026 | Separate permission permits sequential manual completion without score changes; show deficit, select Run/Defer/Skip, and audit actor/reason |
| MILE-07 | 29 October 2026 | Separate correction permission resets selected and later completed milestones, preserves history/effects, cancels unstarted groups, requires score below first reset target or separate correction, and explicitly authorizes replay of previously performed actions |
| MILE-08 | 29 October 2026 | Server controls global/per-milestone presentation and gap with 10-second/one-second defaults; desktop/browser follow the timeline without blocking on disconnected overlays; independent queue Pause/Resume can optionally follow countdown pauses |
| AUTH-06 | 29 October 2026 | Evaluate each constrained permission grant as a whole after established override/deny precedence; do not combine constraints from different grants into broader authority |
| AUTH-07 | 29 October 2026 | Separate use from grant permission; owner configures delegation scope, role/invitation/override changes including self-affecting edits stay within it, and show effective-permission preview |
| CHAT-02 | 29 October 2026 | Separate bot/broadcaster permissions cover direct/automated authorization; explicit sender does not silently change, default holds failed message, configured authorized fallback requires known non-delivery, and uncertain outcomes require review |
| AUTH-08 | 29 October 2026 | Console-generated single-use 15-minute setup code creates first owner only while none exists; replacement invalidates prior code, setup establishes local credentials/recovery codes, and completion disables initial registration |
| AUTH-09 | 29 October 2026 | Five-minute fresh-authentication window; current owner proposes transfer and recipient accepts freshly with local credentials within 24 hours; ownership remains until acceptance, former-owner roles explicitly selected with Observer default |
| AUTH-10 | 29 October 2026 | Authorized helper recovery uses manually shared single-use 15-minute reset link; completion revokes sessions/old recovery codes and offers new codes without changing permissions; console recovery retains existing owner identity/ownership |
| INTAKE-01 | 29 October 2026 | Connection-gap catch-up supports Review (default), Apply automatically, or Record without credit; preserve evidence/valuation/duplicate safeguards, normal arrival-time handling, source-timed statistics, and visible unrecoverable gaps |
| INTAKE-02 | 29 October 2026 | First connection establishes a new-support baseline without historical auto-credit; supported explicit historical import selects a starting date and enters review with normal eligibility/duplicate checks |
| UI-04 | 29 October 2026 | Separate renderers match bindings, visibility, geometry, progress, and animation timing with minor font-rendering differences allowed; shared reference scenes verify each output and editor flags unsupported features before publication |
| DEP-04 | 29 October 2026 | One application process with persistent SQLite/assets/configuration behind proxy; common Docker/Podman application image with separate testing; native application runs as a system service |
| DEP-05 | 29 October 2026 | Support existing reverse proxy instead of default bundled Caddy; document HTTPS/WebSocket/webhook/forwarded-header/request-limit requirements and trust only configured proxies |
| SETUP-01 | 29 October 2026 | Resumable guided owner/URL/preferences/services/timer/rules/milestones/scene/simulation/start flow; simulations stay separate from live accounting and unresolved requirements appear before start |
| BUILD-01 | 29 October 2026 | Use C++20 for server and desktop; exact compiler versions follow platform selection |
| BUILD-02 | 29 October 2026 | Use CMake/Ninja with shared configure/build/test presets for development and CI, allowing individual overrides; exact tool versions remain open |
| TEST-01 | 29 October 2026 | Qt Test and CTest accepted as native testing components; combined framework refinement subsequently accepted in TEST-02 |
| TEST-02 | 29 October 2026 | GoogleTest covers core behavior, GoogleMock meaningful service interactions, Qt Test Qt integration, and Qt Quick Test QML/UI; CTest runs separate suites locally/CI; use controllable clocks, reusable fakes and recorded/synthetic events, avoid duplicate assertions, and separate live checks |
| BUILD-03 | 29 October 2026 | Minimum Qt 6.12; pin tested Qt patch per release and keep builds reproducible with publicly available sources/patches |
| PLATFORM-01 | 29 October 2026 | Official Windows baseline is Windows 11/x86-64 with MSVC 2022; Windows 10 outside initial supported scope |
| PLATFORM-02 | 29 October 2026 | Development takes place on Arch Linux rolling release; desktop support policy subsequently resolved in PLATFORM-04 and deployment targets in PLATFORM-05 |
| PLATFORM-03 | 29 October 2026 | KDE Plasma is the developer's desktop on Arch rolling; reference support/test policy subsequently accepted in PLATFORM-04 |
| PLATFORM-04 | 29 October 2026 | Native Wayland initially with KDE Plasma reference testing/bug reproduction; other compositors best-effort until validated; X11/XWayland outside initial support commitment; validate native OBS capture/transparency/resize/reconnect; headless server unaffected |
| PLATFORM-05 | 29 October 2026 | Test desktop on Arch rolling/Kubuntu 24.04 and native server on Ubuntu 24.04; build portable releases in a pinned Ubuntu-based environment |
| PACKAGE-01 | 29 October 2026 | Native server .tar.gz contains application/runtime libraries/example configuration/systemd definition; dedicated service account and external configuration/data; documented install/upgrade/removal |
| PACKAGE-02 | 29 October 2026 | Linux desktop bundled .tar.gz with launcher/Qt/QML/plugins/resources plus maintained Arch PKGBUILD using system packages; settings outside application directory, test relocation/replacement, and defer AppImage |
| DEP-06 | 29 October 2026 | Docker uses Compose with application/Caddy, persistent storage, explicit release image versions, private application endpoints, and documented lifecycle/backup commands |
| DEP-07 | 29 October 2026 | Podman uses Quadlet/systemd with rootless/rootful configurations; default dedicated-account rootless application plus host Caddy, alternative rootful/containerized Caddy; exact runtime/network/storage details require validation |
| PACKAGE-03 | 29 October 2026 | Windows ZIP extracts to chosen folder with executable/dependencies/resources and portable client settings/cache/logs; no installer or literal single-file requirement; validate clean Windows 11 standard-account startup without separate Qt/compiler-runtime installation |
| WEB-01 | 29 October 2026 | React/TypeScript with Vite for management/viewer; ship static assets with server and require Node.js only for development/build, with no separate production Node.js service |
| TEST-03 | 29 October 2026 | Vitest frontend unit/component tests and Playwright browser workflow tests, including invitations, permissions, review, and live updates |
| WEB-02 | 29 October 2026 | Support current/previous major Chrome/Edge/Firefox for management/viewer, best-effort Safari; separately validate actual OBS overlays on Windows/Linux and choose minimum OBS version in prototype |
| SERVER-01 | 29 October 2026 | Drogon HTTP/webhooks/WebSockets plus Qt Core bot logic in one process behind Caddy; explicit bridge into authoritative Qt queue and early startup/shutdown/thread-coordination prototype |
| API-01 | 29 October 2026 | JSON HTTP with source-controlled OpenAPI 3.1 plus separately versioned JSON WebSocket schemas; validate desktop/web consumers against contracts |
| DB-01 | 29 October 2026 | Qt SQL SQLite driver, one controlled writer thread, separate owned read connections as needed, WAL with synchronous=FULL; backup/recovery implementation validation remains |
| AUTH-11 | 29 October 2026 | Libsodium Argon2id salted password hashes with stored parameters, cost benchmark, and parameter upgrade on successful login when needed |
| AUTH-12 | 29 October 2026 | Random 256-bit opaque account sessions with server-side hashes, immediate revocation and existing lifetimes; protected browser cookies/CSRF or desktop authorization header, no session URLs/browser local storage |
| CRYPTO-01 | 29 October 2026 | Encrypt integration tokens/API keys using libsodium XChaCha20-Poly1305; separate protected key file/secret mount permits unattended restart; restoration material only inside encrypted backups under separate backup-key policy |
| CRYPTO-02 | 29 October 2026 | Backup archives use secretstream/fresh per-backup key sealed to owner's public backup key; owner keeps downloaded private recovery key, server retains public key only; archive format/provisioning/restore require review/testing |
| AUTH-13 | 29 October 2026 | Remember Windows session token only via DPAPI-encrypted file in portable folder; same-user/computer binding normally requires new sign-in after moving, preferences retained |
| AUTH-14 | 29 October 2026 | Remember Linux session token only via QtKeychain/available keyring (KWallet/Secret Service); unavailable storage means memory-only session and explained disabled remembered login, no plaintext fallback |
| OPS-04 | 29 October 2026 | Stopped-server console restoration stages/decrypts/validates backup and preserves existing data; restart enters Recovery review, invalidates restored sessions, holds outbound actions, enforces hard end, and avoids silent replay |
| CRYPTO-03 | 29 October 2026 | Separate authorized credential/backup-key rotations; verify credential re-encryption before dropping old key, future backups use new recipient while older archives retain old-key requirement, and audit identifiers only |
| API-02 | 29 October 2026 | Desktop/server handshake advertises API versions/capabilities, hides unavailable compatible features and explains incompatibility before operations; ship matching server web assets and document supported combinations |
| NUM-04 | 29 October 2026 | Boost.Multiprecision exact intermediates; signed 64-bit credited milliseconds and micro-points; preserve original money/rates losslessly, apply final rounding, and hold out-of-range results for review |
| API-03 | 29 October 2026 | Exact decimal/accounting and 64-bit integer JSON values use strings; rational values use explicit numerator/denominator strings with contract validation |
| MOD-04 | 29 October 2026 | Definition edits affect future activations by default; explicit permitted previewed updates may change selected active instances with Keep remaining/Restart/Set remaining duration, preserving pending arrival context and prior effects |
| MEDIA-04 | 29 October 2026 | Configurable defaults: image/font 32 MiB, audio 128 MiB, video 512 MiB, packaged scene 2 GiB; raster maximum 8,192 pixels/edge and 32 MP; validate dimensions and warn on demanding scenes |
| MEDIA-05 | 29 October 2026 | Initial SVG supports shapes/paths/groups/fills/gradients/strokes/transforms/internal refs; excludes scripts/external resources/HTML/animation/filters, requires text paths, and is validated in both renderers |
| PERF-01 | 29 October 2026 | Benchmark target 2 vCPU/2 GiB/local SSD, 500 viewers/10 management clients/5 overlays, 5 eligible inputs/s sustained and 50/s for 30-second bursts; eligible receipt-to-commit p95 under one second excluding provider/review/scheduled delays; not measured minimums |
| LICENSE-01 | 29 October 2026 | GPL-3.0-or-later for project code; preserve dependency/font licenses and required source/build information/notices in releases |
| MEDIA-06 | 29 October 2026 | Bundle Noto Sans Regular/Bold for built-in interfaces/default themes, retain custom scene fonts/fallbacks, and include OFL notices |
| I18N-04 | 29 October 2026 | Authoritative structured JSON with stable IDs/en-de/named parameters/plurals generates Qt translation and browser catalogs; contributors edit source and consistency is tested |
| VIEW-04 | 29 October 2026 | Viewer links default to 30-day expiry with configurable Never; overlay credentials default to no expiry; both individually revocable/replaceable and scoped read-only |
| STATS-03 | 29 October 2026 | Default cumulative lines/interval bars with appropriate stacking, about 100–200 automatic buckets/manual interval, and archive timeline extended through refunds with event-end marker/last-updated time |
| REPO-01 | 29 October 2026 | One GitHub repository for core/server/desktop/web/schemas/deploy/tests/docs; private during development and public when ready; final name/owner still metadata, working title Twitch Subathon Bot |
| AUTH-15 | 29 October 2026 | No project-hosted authorization service/broker/relay; authorize directly through each installation and provider, with server-held credentials and supported browser flows |
| INTEGRATION-01 | 29 October 2026 | Prefer easiest supported guided onboarding and Link/Connect; separate installation registration from account consent, avoid manual token copying where supported, and validate public Twitch client suitability before promising registration-free setup |
| DEV-01 | 29 October 2026 | Repository/scoped AGENTS.md instructions, clear module dependency/interface/transaction contracts, and links to authoritative specification decisions guide agent development |
| DEV-02 | 29 October 2026 | Small tasks record requirements, contracts, dependencies, acceptance criteria, checks, status/blockers/commit and durable handoffs; architecture decisions remain in repository records |
| DEV-03 | 29 October 2026 | Coordinate parallel work through an integrator, separate branches/worktrees, explicit shared-file/contract ownership and separate review; integrate incrementally |
| DEV-04 | 29 October 2026 | Early reusable simulator/fake adapters/controllable clock and reviewed fixtures; completion requires relevant passing checks and updated contracts/docs, distinguishing simulated from live/platform verification |
| TEST-04 | 29 October 2026 | Test agents author and independently review specification-derived acceptance tests before production feature implementation, initially for the core and then each module; minimal contracts/harness/stubs allowed and intended failing behavior demonstrated |
| TEST-05 | 29 October 2026 | Implementation agents use the reviewed baseline without weakening/deleting/skipping assertions to pass; legitimate test/contract changes require explicit specification-based rationale and separate test review; retain additional tests and live/platform validation gates |
