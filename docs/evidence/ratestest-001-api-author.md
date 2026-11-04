# RATESTEST-001 concrete API document author handoff

Author: service_author, 2026-11-04. This stage owns only
[rate-snapshots-api-v1.md](../contracts/rate-snapshots-api-v1.md) and this note in
`task/RATESTEST-001-contract-author`, isolated worktree
`local-evidence/worktrees/ratestest001-contract-author`. Root owns shared state,
HTML, maps, schema/build/CI/pins, publication and checkpoint integration.

The coordinator released this document stage after actual independent logical
approval `7003473` / integration `6a4a487` and explicit claim `4853a42`. Merged the
actual prerequisite branch `task/RATESTEST-001-snapshot-baseline` cleanly as
`4c7219c`; no concurrent work or shared state was overwritten. Old ACCT/locale
trees are preserved. The source logical contract and its author note remain
unchanged at blobs:

- `76d2d9b3e542c56b7475e99defd717d9902885a5` — rate-snapshots-v1.md.
- `c5e87c1888bbdb78a9d7e6a1dffebe1824cf8b9d` — logical author note.

Read the actual approved model/review, root/core instructions, architecture,
workflow/test strategy, RATES-001/007/009, specification §§3.5, 9, 15, existing
exact-decimal contract/header, ReceiptMoney and captured valuation/boundary contracts.
This proposal does not change those source contracts.

## Proposed API decisions

The API document blob is `556840c2827b9c1e9a29c3ec9c378a86a31d4118`.
It specifies four C++20 core entrypoints: original money, fetched observation and
effective snapshot factories, plus a pair query accepting a complete raw candidate.
Each result has private construction, an owned data aggregate and const access;
mutable caller aggregates are not trusted validation/authorization certificates.
The query independently validates its candidate, so literal valid input reaches
its own future stub without first executing an unimplemented factory. No test-only
production constructor is needed.

Raw potential numerator/denominator strings are bounded canonical positive
integers, then checked for exact component bits and reduction before rational
construction. The approved logical contract explicitly permits that engineering
boundary. Selected Boost/existing exact decimals remain the only numeric tools.
This avoids an unbounded caller-created Exact copy or a Boost-version-specific
backend inspection seam. Existing caller construction/serialization and explicit
copies of returned objects remain caller allocation scope; no retroactive bound
is claimed. A 1234-digit integer can require 4100 bits, so the separate 4096-bit
check is mandatory. Derived pairs use at most 8192-bit products; checked bounded
constraint comparisons have a fixed 16384-bit temporary ceiling.

The document fixes complete input/output field walks, positive lowerable limits,
deterministic sorting, byte equality, validation/error priority and preprojection
before complete output ownership growth. Individual bounded parser temporaries
are distinguished from actual returned-data copies. Literal examples preserve
unknown/epoch-zero timestamps, exact 5/6 and 5/3 ratios, original precision and
independent byte-budget/error expectations. No provider capabilities or ISO currency
registry are inferred, and pin references provide no authorization.

The distinct future RateSnapshotNotImplemented marker must be the immediate first
action of every minimal feature stub. No feature may execute at build/discovery.
A selected-tool green foundation, separately reviewed literal tests and actual
intended marker-red must precede production implementation. The required RATES-009
bridge remains unimplemented: current HistoricalValuation/retained codecs cannot
represent recurring 5/6 in a finite decimal field. There is no rounding fallback,
new schema/header or claim of a usable award adapter.

## Actual checks and limits

- `python3 tools/check_project.py` — PASS: 398 task records, 122 decisions, local
  links, presets and current HTML. Its 19 draft GoogleTest cases are explicitly
  not compiled/run or approved by this documentation check.
- `python3 tools/progress.py render --check` — PASS, current snapshot.
- `git diff --check` — PASS before staging; new files receive the staged check
  before commit as well.
- `git hash-object docs/contracts/rate-snapshots-v1.md docs/evidence/ratestest-001-contract-author.md`
  — unchanged frozen blobs above.
- Independent standard-Python integer arithmetic only:
  `python3 -c 'assert (10**1234 - 1).bit_length() == 4100; assert len(str(2**4096)) == 1234; print("PASS: independent integer-bound arithmetic")'`
  — exit 0. This imports no project module and establishes only the two documented
  integer-size facts, not a rate-model baseline or implementation.

No actual C++ API/header/source/stub, schema/store, fixture, test, feature, provider
request or production behavior was authored/invoked. No observed red, numerical
acceptance or CI execution is claimed. Actual different API review is the next
gate; RATESTEST-001 remains incomplete and RATES-001 remains planned. Root records
actual publication/PR/status and releases further work after that review.
