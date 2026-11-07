# FIXTURE-001 — Independent catalog, reference and account fixture recipes

Module: Fixture tooling. Kind: implementation. Specification: v0.45; §17.
Contract revision: fixture-generation R1, to define and independently review before tests.
Parent: EPIC-028 / TESTCLEAN-006. Depends on: TESTCLEAN-005.

## Scope and ownership

Own `tools/generate_fixtures.py`, `tools/fixture_sources/{__init__,common,catalogs,
reference_capture,account_sessions}.py`, focused `tests/tooling/test_fixture_generation.py`,
and adoption of ONLY the reference-capture/account-session JSON fixtures. Coordinator
owns state/HTML/current documentation/evidence/PR and shared CI/CMake. Read both scoped
`tests/AGENTS.md` and `tools/AGENTS.md`. Use a fresh implementation session and branch.

Create small independent literal catalog declarations and stdlib framing/byte primitives.
Declare the exact reviewed stage5–8 SQL once and independently enumerate NULL-SQL index
facts; compose projections by specified UTF-8 ordering, never SQL inspection/normalization.
Reference captures A/B and all23 constraint rows remain independent source/format data.
Account verifier/source-byte literals remain fixed; proposed8 stays unimplemented and
account44 stays passive red. No hashing passwords, Qt/SQLite/provider execution, production
imports/headers/binaries, git-derived values, JSON patch DSL or domain/SQL simulator.

The only existing fixture recipe inputs are the four seed files below, under
`tests/fixtures`. Read only their approved literal version/ID/orderedSQL/schema fields.
The six large output fixtures may NEVER be recipe inputs; output reads are permitted
only for explicit check-mode comparisons. Source functions supply later-family data.

| Seed file | Baseline file SHA256 at f250357 |
| --- | --- |
| retained_context_vectors.json | 631260c6050c6e47c36ef69fbf6fecf67016407b7f9cf93d6b3246b9438f1dcf |
| sqlite_receipts_vectors.json | 350e2f22db39661c7966996f8d7ae5da12fad91b929605280ff7930d1bec7e8a |
| sqlite_contributions_vectors.json | 86eadfdf53367e7b3d7ae27f1885483aad9e8183a6b759e40ad526ae1d33ff23 |
| sqlite_application_plans_vectors.json | 9a603917d75019b3fe18c8dd6e7dbb05386d48f64b90ee340f5d2d66d21b968a |

## Contract and test-first gates

Before implementation, define R1 contract/examples for repeatable `--fixture NAME`,
`--output-dir PATH` and `--check`. Explicitly settle no-selection behavior, supported-name
set, unknown names, check/output mode combinations, check comparison semantics, output
format, and script-relative seed-root/pin resolution independent of working directory.
Support only implemented/reviewed families; never fall back to copying legacy outputs.
Keep options small; do not add a parser matrix or generic resource-admission framework.

Authored focused tests must precede implementation and run only after separate test/oracle
review. Provide reachable named inert FeatureNotImplemented recipe stubs for intended red;
missing imports/files/dependencies or parse errors are not red. Test actual generation in
an isolated tree with source recipes and ONLY four declared seeds, all six outputs absent.
Freeze independent typed-content oracles from TESTCLEAN-006's baseline, not a generator run.
Cover meaningful reproduction and check-mode drift refusal/no write. No private-helper tests.

## Acceptance criteria

- Reference/account typed JSON contents exactly preserve their pre-generator canonical
  fingerprints; formatting may change, embedded bytes/SQL/values/status may not.
- Exact independent catalog/frame/hash/NULL/public format coverage and all unique fixture
  families remain. Original provenance/blob strings remain literal. Review source inputs
  and both output families, not just successful command exit.
- Shared mechanics remain small; no giant literal copy of legacy JSON. Give reviewed
  recipe/input/output/coverage mapping and refine source-size estimates before coding.
  Split substantial additional prerequisites explicitly instead of growing a framework.
- Contract/tests/review -> observed intended red -> implementation -> independent source/
  oracle execution approval -> real generation and relevant Qt/tooling checks. Preserve
  original outputs until approved adoption. Compile/discover account44 without its bodies.
- Record actual local vs CI/platform proof, separate runtime/integration review and current
  CI/rules/discussions/normal protected PR merge. This is tooling acceptance, not product
  feature implementation. No unrelated native replay or new dependency/environment probe.
