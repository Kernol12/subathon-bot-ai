# SPIKE-001 — Validate Drogon and Qt lifecycle/thread bridge

Module: Server/API. Kind: feasibility.
Authoritative status/owner/dependencies: progress/state.json (do not duplicate mutable status here).
Specification references: SERVER-01, DB-01.

## Goal

Prototype startup, request-to-Qt queue, one writer and safe shutdown before broad server work.

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


## Bounded real-framework experiment

The next executable slice uses the disposable
[revision-1 experiment contract](../../experiments/spike-001/bridge_contract.md):
pinned local Drogon/Trantor discovery; actual loopback HTTP/WebSocket commands
bridged to one owned Qt SQL writer; listener readiness and occupied-port failure;
closed admission, queued disconnect, in-flight reply drain and SIGTERM shutdown.
Independent tests and observed missing-behavior evidence precede the experiment
implementation. Passing these bounded checks does not approve reuse as production,
Caddy/TLS, all possible client behavior, recovery/backpressure or platform support.
