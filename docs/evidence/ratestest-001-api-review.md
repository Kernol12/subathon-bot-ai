# RATESTEST-001 independent concrete API document review

Reviewer: codec_author, different from author service_author. Date: 2026-11-04.
Scope is the API proposal and its author evidence, plus compatibility with the
approved logical model and existing exact/captured-valuation boundaries. The only
new authored file is this review note, on `task/RATESTEST-001-contract-review`.

Actual author candidate `15cd94410f0f1eadbd616b293d6a8a7ec26bd85b`, integrated by
root as `5d5712b150077fd50b83f98cc97181259cb03fbc`, was merged cleanly into this
isolated review tree as `7ded19aaa00449e15e0690d05647ecab93357d14`. The earlier
logical review `7003473` is preserved unchanged. Exact inspected artifacts:

| Artifact | Git blob |
| --- | --- |
| [API proposal](../contracts/rate-snapshots-api-v1.md) | `556840c2827b9c1e9a29c3ec9c378a86a31d4118` |
| [API author evidence](ratestest-001-api-author.md) | `fc1d1baf2d1d2fcf00dad34b5e6981149fe78815` |
| [Approved logical model](../contracts/rate-snapshots-v1.md) | `76d2d9b3e542c56b7475e99defd717d9902885a5` |
| [Logical author evidence](ratestest-001-contract-author.md) | `c5e87c1888bbdb78a9d7e6a1dffebe1824cf8b9d` |
| [Earlier logical review](ratestest-001-contract-review.md) | `cc928f778f451f00dc0158a6566cd54ec966bfc7` |

Read root/core instructions, architecture/workflow/test strategy, RATES-001/007/009,
specification v0.45 §§3.5/9/15 and NUM-04/FX-06, exact-decimal revision 1 and public
header, ReceiptMoney, event/state boundaries, application-plan contract/header and
STORAGE-015's historical rate representation. No new provider capability claim
requires an external lookup; actual provider validation remains separate.

## Verdict

**Approved for the concrete API document stage.** No substantive defect or new
owner requirement decision was found. This approves the exact document boundary,
not an as-yet-unwritten header, stub, harness, fixture, test baseline, observed red,
implementation or usable valuation adapter. RATESTEST-001 remains incomplete and
RATES-001 remains planned. Later authored artifacts need their own actual review.

## Ownership and validation boundary

The four entrypoints are sufficient: money, fetched-observation and effective-
snapshot factories, plus a raw-candidate pair query. Every successful result owns
one private data aggregate; only const access is exposed. Defaulted value copies
and assignment do not turn caller-owned aggregates into validation certificates.
Their allocations and reference lifetimes are explicitly caller responsibilities.

The raw query validates the entire actual candidate, including its complete parsed
output projection, on every call. Identity/missing queries cannot bypass malformed
source data or contradictory pins. It can reach its own future immediate stub with
literal input, without first requiring another unimplemented factory to succeed.
There is no test-only constructor or successful factory hidden in test setup.

Numerator/denominator input is bounded canonical ASCII positive integer text.
Byte/collection preflight precedes copying or numeric construction; grammar and
each integer's bit limit precede gcd/rational construction. Reduced pairs are
required rather than repaired. Outputs use owned Exact values. Existing decimal
parsing preserves original spellings and domain/error distinctions; it introduces
no binary floating point or new numeric library.

## Resource and arithmetic checks

The recursive metric covers every declared input and output field, including
ExactDecimal's spelling AND canonical rational, optional timestamp scalars,
variant tags, actual constraint references and nested source/quote/component/member
vectors. Source wrappers add no phantom copies. The ceilings apply separately to
complete raw input and complete returned data; query keys have their explicit
per-field bounds, and the separately returned pair has its own complete projection.
This is a semantic byte/item budget, not sizeof, allocator capacity or a retroactive
limit on caller construction/copies.

Complete projection must precede output growth, including pair disposition and
provenance. Individually bounded parser temporaries are allowed; accumulating
unprojected parsed outputs or copying a whole speculative snapshot is not. Borrowed
indices/references have their explicit 65536-record ceiling. Input count checks
and size sums cannot overflow or allocate from an unvalidated ordinal.

Independent standard-library integer/Fraction arithmetic checked 15 documented
size, ratio and measurement facts, without importing project code:

- 1234 decimal digits may require 4100 bits; `2^4096` has 1234 decimal digits but
  4097 bits. The lexical bound alone cannot replace the component bit check.
- Two 4096-bit potential integers can produce an 8192-bit product. Existing
  decimal bounds conservatively require at most 1701 numerator or 1698 denominator
  bits. Comparing a derived ratio against a decimal rate needs at most
  `4096 + 4096 + 1701 = 9893` bits on the larger cross-product side, within the
  fixed 16384-bit temporary ceiling. No graph-path multiplication is needed.
- The literal S1 example has 102 raw logical bytes, 106 parsed output bytes and
  9 recursive items. Its EUR→USD result is exact 5/6, with 11 owned logical bytes.
  These sums count context 39 bytes, quoted input/output 24/28, constraints 22,
  components 15 and snapshot identity 2; vectors count actual elements recursively.
- Money `1`/`U` needs 4 complete output bytes. An identity pair needs 3 bytes even
  when its validated empty snapshot needs only 1. Both resource examples agree
  with their declared outcomes.

The fixed pair/temporary ceilings do not relax a caller-selected smaller potential
ceiling or CORE-001's separate 4096-bit finalizer. Allocation failure remains a
real bad_alloc, never a fabricated invalid rate, missing conversion or partial award.

## Ordering, provenance and adjacent scope

All options are checked even when unused. Complete count/byte preflight precedes
lexical errors; declared/input order fixes traversal until canonical sorting.
Duplicate categories, component shape, reference resolution, sorted constraint
equality, required references and connectivity have explicit precedence. Full enum
domain checks occur before narrowing; a valueless variant must never be accessed
as an alternative. No diagnostic reflects arbitrary input bytes.

Fetched quote order and every original spelling remain intact; canonical output
sorts source/override/constraint identities without solving or rewriting the
component graph. Source/manual references are typed evidence, not authorization.
Unknown, present-empty raw time and epoch-zero instants remain distinct; no source,
fetch or publication time is inferred from another. Reporting changes cannot alter
immutable history, pins, awarded effects or original-effect reversal provenance.

Required RATESTEST-009/RATES-009 still owns the finite-decimal historical valuation
gap. This API can represent 5/6 but cannot claim its existing command/plan/SQL bridge
works, round that ratio or misreport it as missing. Provider parsing/capabilities,
pin update/removal policy, persistence, scheduling, UI and platform verification
remain explicit required adjacent work.

## Actual checks and next gates

- Exact Git-object inspection and clean prerequisite merge: passed.
- Read-only Python integer/Fraction document audit: exit 0, 15 facts passed.
- `python3 tools/check_project.py`: exit 0; 398 task records, 122 decisions and
  local links/presets/current HTML.
- `python3 tools/progress.py render --check`: exit 0, current snapshot.
- Final owned-note whitespace/staged-path checks accompany its commit handoff.

No project feature, new C++ code, native test, provider operation or successful
model function was invoked. The documented distinct RateSnapshotNotImplemented
marker is a FUTURE inert-source requirement, not observed behavior. Minimal
source/harness review, selected dependency foundation, independently reviewed
literal tests and a real first named-marker behavioral red remain required before
production implementation. No build/discovery step may execute a new feature;
catching a marker as success, skipping or failure inversion cannot satisfy a gate.
