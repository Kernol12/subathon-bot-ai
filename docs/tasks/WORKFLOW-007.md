# WORKFLOW-007 — Reusable bounded execution runner

Kind: implementation. Specification v0.45 §§17–18.
Work remains stopped.
Contract: bounded-coordination-v2 §§2–3. Author and independently review a precise
bounded Linux command/manifest/process contract before tests; no generic workflow
engine, loader emulator or new dependencies. New substantial prerequisites split first.

Coordinator owns brief/contract, integration/docs/CI/proof. Test author owns focused
synthetic process and manifest tests; fresh implementer owns maintained stdlib execution
support under tools/. Source/resource/passive/native admission remains consumer-specific.
Reuse selected existing seal/ELF primitives only after reviewing their boundary.

Separate immutable approved execution data from approval records. The runner owns
exact argv/env, bounded output/time, exclusive scratch, interruption, child ownership,
actual wait result and honest cleanup/observations; workflow.py owns review grammar.
Import WORKFLOW-006's canonical manifest digest function from tools/workflow.py;
do not reimplement identity or define another approval-binding format.
Unknown status/ownership never succeeds or triggers automatic retry. Independent
expected result rules classify setup/fixture/product failures and named intended red.

Tests target observed defects: launch interruption/constructor uncertainty, ignored
SIGCHLD, misleading nonzero exit, unexpected file mapping, changed artifact, output
overflow, timeout and cleanup failure. Use harmless reviewed fixtures with no external
effects. Preserve all old transport assertions/native44 and private historical operators.
Do not rerun APITEST-016 or claim a new consumer admission. Demonstrate reuse with bounded
synthetic configurations; document how future consumers supply reviewed manifest data.

Contract → reviewed tests → intended red → fresh implementation → full code review,
relevant real process/tooling checks and protected PR. Retain complete actual attempts,
private evidence/public allowlist separation and narrower results-only eligibility.
