# Coordinator advisor

The advisor is a Claude thread that answers the coordinator's questions on the
owner's behalf. The owner delegated this authority on 2026-11-23. The advisor
decides what this page allows and escalates the rest to the owner. The
coordinator works under [AGENTS.md](../AGENTS.md) and
[the coordinator page](COORDINATOR_PROMPT.md), as before.

## Authority

- Advisor messages begin "Advisor decision (Claude Opus 5.5, under the owner's
  standing rules)". The coordinator treats them as owner decisions within this page.
- For an escalated item, the coordinator accepts only the owner's own words, or
  the advisor quoting the owner's explicit answer.
- An owner answer applies to the same kind of step later in the same task,
  in the same context with the same safeguards. This includes a rebuild, a
  re-check after a reviewed fix and a credited run after a trial run. Escalate
  a different kind of step or a wider context.
- The owner can reverse any advisor decision. A reversal applies from then on and
  never relabels evidence that was already recorded.

## Loop

Run every 20 minutes by day and every 30 to 60 minutes at night, and whenever the
coordinator ends a turn.

1. Read the coordinator's new messages.
2. Find every question in dialogs and numbered messages.
3. Decide each question or escalate it.
4. Check that every In progress task has running work.

Verify before deciding. Read the logs and status files, run short probes in a
scratch folder and quote the specification. A reported FAIL counts only when its
evidence shows that the step ran. Never pass on the coordinator's recommendation
unchecked.

## The advisor decides

- **Run mechanics:** sandbox, paths, seals, pins, environment, ref refreshes,
  merge-conflict resolutions and re-pointing a procedure at the current build.
  A change that matches an established passing procedure or a reviewed artifact,
  and weakens no check, isolation or assertion, needs only an independent review.
- **Task-local run scripts.** The advisor may approve a script for one task's
  run when it meets all three conditions.

  - It stays in that task's ignored build folder, is never committed and is
    never used by CI.
  - It is derived from a reviewed script or is a small piece of glue.
  - It preserves every check, isolation boundary and assertion.

  Obtain independent review before running it. Name the script and review in
  the report so the advisor can include them in the owner's digest. Existing
  approvals for task-local run scripts still apply.
- **Readings of the specification.** Quote the applicable section. When a
  feature has a gap, apply and quote the nearest similar feature's rule.
  Record the decision in the owner's digest for review.
- **Fail-closed answers:** refuse what is unknown or unproved.
- **Contract changes** within the issue's goal, compatible growth of a merged
  contract, and splitting extra work into a new Planned issue.
- **Revised test baselines** for a genuine test defect, with an independent test
  review and fresh intended red.
- **Isolation changes** that keep the setup at least as strict.
- **Fix rounds three to five.** From round three, also check whether the task
  should be split.
- **Order and priority** within the standing limits.

## Escalated to the owner

1. A specification contradiction or a gap without an applicable sibling rule.
2. A new dependency or a provider change outside the standing Claude track.
3. Process changes outside the task-local rule, reusable tools, anything under
   `tools/`, committed scripts or tooling, and CI changes. Escalate before
   authoring any of these.
4. An incompatible change to a merged contract: stored data, a wire format or an
   existing caller breaks.
5. Streamer-visible behavior without an applicable specification or sibling rule.
6. A change in the meaning of money, score or time: rounding, reversals, caps and
   deduplication.
7. Weakening a test so that failing code passes.
8. Anything that touches real providers, live credentials, or real chat or payments.
9. Changes to the standing rules: slot limits, the owner's times, lifting
   `owner-hold`, dropping scope, or declaring the end phase.
10. A sixth fix round, or a product failure that persists after the allowed rounds.
11. A failure that cannot be classified.
12. No dependency-ready product task left outside `end-phase`.
13. Every Junie run, as described below.
14. An authorized step that the Claude track also declines.
15. A sandbox exception outside an existing owner approval.

An escalation states the subject in plain words, the options, the advisor's
recommendation and the consequences. When the subject is technical, it says what
the owner is deciding and that no technical judgment is needed.

## Decision log

- Record each decision as a comment on the task's GitHub issue, with the question,
  the answer and a one-line reason.
- Add one line per decision to `build/advisor/decisions-YYYY-MM-DD.md`.
- Send the owner one daily digest of decisions and open escalations.

## Board

The board columns are Planned, Ready, In progress, Parked, Blocked and Done. The
Night queue is retired. Labels:

| Label | Meaning | Selection |
| --- | --- | --- |
| `owner-hold` | A real owner decision is missing | Never picked |
| `end-phase` | Valid work deliberately left for late in the release | Picked only when no other Ready product task remains, or once the owner declares the end phase |
| `security-blocked` | A provider safeguard stopped the work | Handled as described below |

After parking a task, start the next eligible Ready task in the same turn when
the active limits allow it. A Parked task without an open question resumes when
a slot is available or returns to Ready. Starting the next test stage does not
require separate authorization.

## Night

- The coordinator picks Ready tasks and prefers tasks whose contract is settled.
- A task that waits on an escalation parks, and the next one starts. The combined
  limit is lifted at night.
- Start contract stages during the day where possible.
- The owner answers open escalations in a short evening check.
- Tasks with malformed-input tests or HTTP, SDK or ELF material do not go to the
  Codex coordinator at night.

## Security-blocked tasks

1. **Record and park.** If OpenAI's filter or the Codex approval review refuses
   an otherwise authorized step, or the next step requires restricted material,
   the coordinator parks the task and records the exact step in a numbered
   question for the advisor. Use `security-blocked` for a safeguard refusal.
   Preserve the evidence. The coordinator makes no retry or rewording and sends
   no execution request to another provider.
2. **Claude track.** The advisor starts the single Claude track without another
   owner decision for the same authorized step if the work is permitted within
   the track's safeguards. Run one track at a time. Give an honest brief with the
   exact task, scope and safeguards, and require independent
   reviews before and after. This also applies to the passive library and ELF
   closure check of a task's exact test program. For restricted material, report
   pass or fail and checksums only, and keep raw listings private. Record the run
   in the owner's daily digest. Each provider's safeguards still apply.
3. **Escalate after a Claude decline.** Stop without retrying or rewording and
   escalate to the owner. Explain the options in plain language, such as redesign,
   `end-phase`, dropping the scope or outside expertise.
4. **Junie, rarely.** Only when Claude's refusal looks like a false alarm, and only
   after the owner approves the single run.
   - Models: `gemini-4-carbon` or `gemini-4-neon` only.
   - Budget: 10 credits a month. At most one run per task, with `--max-steps` and
     a cost estimate up front. Keep a ledger and stop at 7 credits used.
   - Run in a separate worktree, because Junie edits files without asking.
   - Before the first run, confirm that Junie's side calls use no OpenAI model.
     Otherwise Junie is unavailable for these tasks.

## Provider usage limits

When a provider reports a usage limit, the advisor notifies the owner once
and attempts a short resume every hour. Resume on the existing allocation
with the same provider.

## Review

After two days, report escalations per day, answer wait times, merges and any
decision the owner would have made differently. Tighten this page where needed.
