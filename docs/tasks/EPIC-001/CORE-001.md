# CORE-001 — Bounded exact decimal parsing and final-effect rounding

Parent: [EPIC-001](../EPIC-001.md). Kind: implementation.
Specification: v0.45; §3, NUM-01, NUM-02, NUM-04.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: [exact decimal revision 1](../../contracts/exact-decimal-v1.md);
its independent-review/red evidence is recorded separately before implementation.

## Scope and ownership

Core exact-value types, decimal/rate/factor parsers and an already-scaled signed64
finalizer; no Qt, SQL, provider or display dependencies. Owned source spellings plus
Boost.Multiprecision exact rationals retain source precision. No expression engine,
modifier group combination/clamping, aggregate-state transaction, refund or service
review implementation belongs to this primitive slice; those requirements remain
in their rule/modifier/accounting baselines.

Primary module paths: core/
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Preserve original monetary decimals and positive rates losslessly using Boost.Multiprecision intermediates; declare parser digit/exponent/resource bounds in the reviewed contract.
- Round exact, already-scaled final milliseconds and micro-points half-to-even,
  including signed corrections; throw on out-of-range rounded results. The application
  caller must hold the input and commit no partial effects; this primitive does not
  persist a review hold.
- Review the local512-byte/256-mantissa-digit/absolute-exponent256/component4096-bit
  limits, full JSON-number grammar, original-spelling ownership and explicit error
  precedence in the contract. Cover each limit and one beyond, both signs of ties
  and signed64 endpoints.
- Keep the intentional-red `exact-numeric-debug` lane explicit during CORETEST-001.
  CORE-001 preserves the baseline and adds stable numeric acceptance to automatic
  PR/main CI with observed dependency/build/test results before completion.

## Boundary and failure cases

Cover both sides of half ties, signed 64-bit endpoints, huge inputs, non-finite/invalid factors and zero/negative rates. Display rounding cannot alter effects.

## Separate baseline and implementation tasks

- `CORETEST-001` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `CORE-001` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest via CTest with literal exact reference vectors; no clock
is needed for these pure primitives. Build with `cmake --preset exact-numeric-debug`
and `cmake --build --preset exact-numeric-debug`; after separate test approval, run
`ctest --preset exact-numeric-debug -L exact-numeric` and record the actual result.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
