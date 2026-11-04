# ACCESSTEST-002 — selected Boost environment author handoff

2026-11-04. Author: service_reviewer. Status: authored and compile/link inspected;
**NOT independently reviewed and NOT RUN.** Root owns the FIRST native foundation
after different codec_author review of the actual source and root's actual harness.
No permission behavior, evaluator, parser, normalizer, fixture or baseline is supplied.

## Scope and exact inputs

Dedicated branch `task/ACCESSTEST-002-environment-author`, isolated
`local-evidence/worktrees/accesstest002-environment-author`, initially clean at
`4429cbb869e5a820a675fc1d72d4110ff207b995`. Owns only
[effective_permissions_environment.cpp](../../tests/acceptance/effective_permissions_environment.cpp)
and this note. Root owns CMake, preset 26, CI, dependency pins, state/HTML and PR62.
This author does not push, publish or merge.

Read root/tests/core instructions, [ACCESS-002 brief](../tasks/EPIC-006/ACCESS-002.md),
the actual partial [logical contract](../contracts/effective-permissions-v1.md)
blob `5dfc6a1e6cff3877a4d2874a26d29d0e6f7f44ed`, settled partial
[review](accesstest-002-contract-review.md) `20919ad37abb4536327a3d711c69ec7cbaaaca04`,
and [owner decision](accesstest-002-owner-decision.md)
`34168d4e5f82a524e9459b13efe055ef6786af8c`. D1's resource-scoped selection is
recorded; the parallel logical correction still needs its own actual review.
This foundation is independent of that choice and establishes no operation domain.

The complete source is Git blob `8d88fd0e7f81de2ba300648ff7066f7bff62c5a3`,
SHA-256 `76e7190817a6f49f4b976d1b5b75362e9ff2655e819842222019543c3f0a90e4`.
It has only standard/Boost headers and its own main; no project/GTest header, type,
helper, fixture, library link, evaluator or marker. All SDK value construction and
arithmetic is lexically inside main, with exact `BOOST_VERSION==108300` static
assertion. No owning global initialization is authored. Exceptions produce exit 2,
failed literal checks exit 1; neither is caught as success or skipped.

## Authored SDK facts and oracle scope

There are 21 focused check sites, covering:

- Signed64 minimum converted before negating, with literal magnitude
  9223372036854775808; no overflowing native abs or negation.
- Unsigned64 zero/maximum 18446744073709551615, exact maximum+1 and independent
  shift expression for literal 18446744073709551616 (2^64).
- Literal negative cross-products -21 and -20, their order for -3/4 versus -5/7,
  and zero cross-products/order. No permission comparator is implemented here.
- SDK gcd(54,24)=6 and gcd(0,24)=24; SDK rational canonical values 42/56=3/4,
  -6/8=-3/4 and zero=0/1. These inspect SDK primitives, not a project normalizer.
- Decimal input 1 followed by 5000 zeros: 5001 decimal bytes and 16610 magnitude
  bits. The parse/format assertion is explicitly a ROUNDTRIP, not an independent
  full-value oracle. Separate expected bit count and a product case use independent
  literal facts: squaring gives 1 followed by 10000 zeros, 10001 digits/33220 bits.

Read-only Python stdlib integer/Fraction/gcd checks confirmed these mathematical
facts separately from Boost and counted all 21 source check sites. No expected
value comes from production code. The long case exercises a bounded SDK capability
beyond 4096 bits, matching EP27's motivation; it neither sets a new domain cap nor
claims all 16384-byte profile values, permission resource budgets or preallocation
checks are implemented. A later API still must define those formulas and gates.

## Actual compile/link/static checks — no native program run

Inherited SDK source: authenticated Ubuntu Noble amd64
`libboost1.83-dev=1.83.0-2.1ubuntu3`, package SHA-256
`747b364a7d578babed9efa4c40446be4ba3470ac5b2c9951b0e3bde3d7bb47d6`.
Read root's actual stage-1 signature/index and stage-2 extraction reviews, plus the
different-author stage-3 probe/handoff and actual narrow selection review. This
foundation author prepared stages 1/2, so does not self-approve their independent
gate; root supplied that review. Stage-3 review covered compile/link selection only,
not runtime arithmetic or this new source/harness.

Actual scratch receipts/raw outputs are under
`/tmp/subathon-accesstest002-environment-author-_l66wb7h`:

```sh
clang-format --dry-run --Werror tests/acceptance/effective_permissions_environment.cpp
/usr/bin/c++ -isystem /tmp/subathon-boost-1.83-sdk/prefix/usr/include -std=c++20 \
  -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -MD -MF SCRATCH/environment.d \
  -c tests/acceptance/effective_permissions_environment.cpp -o SCRATCH/environment.o
/usr/bin/c++ SCRATCH/environment.o -o SCRATCH/environment
/usr/bin/nm -C SCRATCH/environment
```

`SCRATCH` denotes that literal directory; commands.json records the exact absolute
source/output paths and arguments. All four commands exit 0; compile/link stderr
are empty. Child search overrides are removed and RLIMIT_CORE=(0,0), without global
configuration changes. The first format-only check found one wrapping issue; it
was corrected before the successful checks. No compile failed and no failure is
reported as a behavioral red.

Read-only dependency inspection found all 521 used Boost headers inside the exact
isolated prefix and rehashed each against the original authenticated material
manifest. Object .comment records GCC 16.2.1 20260810. Retained nm text defines main,
has no project/GTest symbols and no authored global-initialization function; the
normal CRT frame_dummy init entry is not SDK arithmetic evidence. The binary was
never invoked. No configure/compiler-ID probe, foundation/test, SDK arithmetic,
project/feature/marker or external provider operation ran in this assignment.

Receipt hashes:

| Scratch artifact | SHA-256 |
| --- | --- |
| commands.json | `b90a761c1d10c2a8d3c845babb168e923c56881c13df244e5df5ffb4e0609269` |
| static-inspection.json | `a4db4cbb911d8cdeefe1e5fe5afb5ee68756824d12fc5a938153185a550bfe0a` |
| environment.d | `8f288b2f589fb10b65c1f28a5b90f460358d9ceea51fd10b99c7f3e77159c19c` |
| environment.o | `23acf4abd6e6d0282c20d21b3b527cc478e3f11d306fdb1d8e66d0aa3a6aeb55` |
| unrun environment binary | `2adab04ed9813e791e925eee46bf8c6e45c248357ca27d6e41feaefe2dc985fe` |

Documentation consistency/current HTML/whitespace and exact two-file staged scope
are checked before commit. No existing source/assertion/fixture/contract or shared
file changes. This is not a passing environment, product baseline, intended red or
selected CI/runtime claim. Next requires codec_author's actual independent source
AND root-harness review, then root's explicitly released FIRST selected native
foundation. Separate logical/API/source/test reviews, valid feature red, production
review/acceptance/current CI and all PR62 gates remain.
