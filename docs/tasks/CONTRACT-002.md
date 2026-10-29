# CONTRACT-002 — Define event, effect, clock and persistence boundaries

Module: Core. Kind: contract.
Authoritative status/owner/dependencies: progress/state.json (do not duplicate mutable status here).
Specification references: §2, §3, §4.

## Goal

Separate receipts, contributions, application plans, actual effects and delivery jobs; define controlled clock and writer ownership.

## Assignment preparation

Read root and scoped AGENTS.md, the specification references and contracts. Confirm
all dependencies are done before starting. Record branch/contract revision and shared
file ownership. Link test cases to the required behavior and independently derived results.

## Acceptance and handoff

- Refine this task's bounded acceptance criteria before editing dependent production code.
- For a feasibility task, identify protocol/platform questions, reproduce the experiment and record limitations.
- For a contract task, define versioned examples and error cases before downstream implementation.
- For scaffolding/environment, distinguish successful checks from feature correctness.
- Use the applicable template, record exact evidence and leave a durable handoff.

Run relevant checks, update progress/state.json, regenerate PROGRESS.html and validate before handoff.
