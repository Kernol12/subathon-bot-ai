# RATESTEST-001 literal fixture/test author handoff

Author: codec_author, 2026-11-04, branch `task/RATESTEST-001-test-author`, isolated
worktree `local-evidence/worktrees/ratestest001-test-author`. Actual starting head
`512828c`, after different minimal-source/harness approval `6150962` / import
`520a457`. This assignment authors tests only. The future production implementer
must remain separate from their independent review.

Owns only four new files:

- [Literal raw vectors](../../tests/fixtures/rate_snapshots_vectors.hpp).
- [Acceptance cases](../../tests/acceptance/rate_snapshot_tests.cpp).
- [Boost-only foundation](../../tests/acceptance/rate_snapshot_environment.cpp).
- This author note.

Read root/tests/core instructions, task brief RATES-001, specification §3.5/9 and
NUM-04/FX-06, the approved model/API and actual minimal-source review. Authorities
remain logical blob `76d2d9b3e542c56b7475e99defd717d9902885a5` and API blob
`556840c2827b9c1e9a29c3ec9c378a86a31d4118`. Header/stub/contracts/old assertions
are unchanged. Root owns CMake/profile/CI/state/maps/HTML/schema/pins/publication.

## Literal inputs and independent oracles

The fixture header contains raw input builders only: source O1/S1, empty and short
snapshots, manual evidence and the explicitly specified large-power snapshot.
Builders never invoke a new factory/query or parser. The short values allow exact
literal ownership sums; source metadata is synthetic and does not claim any real
provider capability. Equal money/rate spellings retain their original bytes.

Large input weights are declared as A=1; B..F equal
`3^{536,1072,1608,2144,2584}`; G..K equal
`1/5^{352,704,1056,1408,1764}`. The ten directed source quotes use the independently
specified finite integers `3^536`, `3^440`, `5^352` and `5^356`. Each has at most
256 decimal digits. Existing Boost creates their literal power representations;
no production arithmetic, path solver or observed result constructs an oracle.
The exact K→F expectation is `3^2584 * 5^1764`, an 8192-bit integer from two
4096-bit potential components. An independent standard-Python integer check
confirmed those three bit sizes and the quote digit sizes; this is document
arithmetic, not execution of the new foundation or feature.

Expected fractions are direct integer/rational literals: 2469/2000, 6/5, 5/6,
1/2, 5/3, 3/5 and 5/8. The RS16 independent conversion fact is 7407/5000; the
model itself exposes no amount-conversion or credit operation. No new function
generates expected values or supplies another entrypoint's setup.

## Authored coverage and runner handshake

Exactly **53** ordinary TEST definitions form the durable case inventory in the
acceptance source. Static extraction and inert GoogleTest discovery match exactly:

| Suite | Cases | Bounded scope |
| --- | ---: | --- |
| RateMoneyAcceptance |7| Original precision/exponents/signed zero/negative values, opaque currency bytes, owned copies, decimal grammar/fixed bounds and signed canonical output byte charges |
| RateObservationAcceptance |7| Complete source context, unavailable/present-empty/epoch/extreme time evidence, quote order/ownership, rounded cycles/self quotes, duplicate keys, positive rates and reference/currency domains |
| RateQueryAcceptance |7| Direct literal reciprocal/cross/identity/missing results, disconnected components, whole-candidate validation before every disposition, key priority, owned provenance and distinct pair projection |
| RateSnapshotAcceptance |17| Complete owned snapshots, unsigned-byte and typed/numeric constraint ordering, retained unselected observations, redundant cycles, exact pin/manual evidence, conflicts, missing references/ordinals/endpoints, canonical connected components and positive reduced integer bounds |
| RateLimitsAcceptance |13| All 13 options at zero/one over on all four entrypoints; complete S1 and every field's logical charges; selected/default count ceilings and byte/item boundaries |
| RatePriorityAcceptance |2| Global preflight before earlier lexical errors; lexical/duplicate/shape/reference/equality precedence in declared category/input order |

There are multiple independent rows within cases. The option matrix alone has
104 negative feature calls. Fifteen observation-field rows and ten snapshot/manual
rows pin literal complete output sizes at the boundary and one below. S1 has
raw/output bytes **102/106**, recursive items **9**, and its EUR→USD pair has
**11** owned bytes. The short snapshot is **25/27 bytes, 6 items**; adding the
short override AND its reference gives **38 output bytes, 8 items**. Identity needs
3 bytes even for an empty snapshot of 1 byte; missing needs only its one-byte tag.

Default boundaries include 16/17 observations, 4096/4097 quotes per observation,
16384/16385 total included quotes, 1024/1025 overrides, 4096/4097 constraints and
256/257 effective members across components. Unselected quote currencies do not
consume the effective-member budget. The 4096-edge input is a declared uniform
rate-1 table; no graph algorithm determines expected weights. Smaller selected
item/byte/count ceilings exercise otherwise unreachable combinations without
claiming that every independent maximum can be reached simultaneously.

`rejects` uses nonfatal EXPECT_THROW for RateSnapshotFailure, verifies the exact
enum/message, then rethrows that expected type. The distinct missing-feature
marker is an ordinary wrong-type FAILURE, never a passing expectation. Later rows
remain reachable; source_location adds the exact caller line and SCOPED_TRACE
identifies loop inputs/categories. Positive rows
use nonfatal EXPECT_NO_THROW around the complete independent scenario. No new
factory runs at static initialization, data provision or discovery; compile-time
traits check only private result construction and const owned access.

Proposed root-owned runner handshake:

```text
core_rate_snapshot_acceptance  (GTest::gtest_main + subathon_rate_snapshots)
rate_snapshot_environment     (own main, Boost::headers ONLY, no arguments)
```

The foundation includes only Boost headers and iostream. Its 14 direct third-party
facts cover exact literal ratios plus 4096/8192/16384-bit resource geometry. It has
no project header, parser/factory/query call or successful substitute. It was built
standalone without a project target; nm finds no core namespace symbols. It must
not link the rate library in shared CMake. **Its main has not been executed.**

## Actual author checks

Environment: GCC 16.2.1, installed Boost 1.92.0, GoogleTest 1.18.0 (pkg-config),
clang-format 23.1.4. No dependency installation or provider/network operation.

```sh
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -Icore/include tests/acceptance/rate_snapshot_tests.cpp core/src/rate_snapshot.cpp -lgtest_main -lgtest -pthread -o /tmp/subathon-rate-snapshot-authored-tests
c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror tests/acceptance/rate_snapshot_environment.cpp -o /tmp/subathon-rate-snapshot-authored-environment
/tmp/subathon-rate-snapshot-authored-tests --gtest_list_tests
clang-format --dry-run --Werror tests/fixtures/rate_snapshots_vectors.hpp tests/acceptance/rate_snapshot_tests.cpp tests/acceptance/rate_snapshot_environment.cpp
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Final commands pass. Both compile/link commands emit zero warnings. The initial
draft syntax check found ordinary initializer-list/range-loop-copy/shadow/trace-line
issues; these were corrected before the clean final build, without feature
execution. Inert discovery lists exactly 53 unique static names (7/7/7/17/13/2).
The scratch inventory `/tmp/subathon-rate-snapshot-case-inventory.txt` is an exact
static extraction; the committed TEST definitions remain the durable inventory.
Project consistency reports 398 task records, 122 decisions and valid links/
presets/current HTML; render check is current. Final staged-path/whitespace checks
accompany the commit. No test body, setup, foundation, factory, query, new exception
marker or feature was invoked. No passing feature or intended red is claimed.

| Artifact | Git blob | SHA256 |
| --- | --- | --- |
| Vectors | `8936f4fae9bd17d5e592256e0c395871bcea4849` | `2b1162c59d07194a4ee459a82979791bee85ce21e61517aa058bf1e1ec643840` |
| Acceptance | `21b819b9fd870fb94936013f51dbac52b9c99637` | `e99bae74ddb788f118d2c6ad9cf300afc9481b3698a75cf0895d54e499a48f27` |
| Foundation | `28a829aad3769f176db82b497120ba4ad05a0818` | `d353f9e882436bd70da4a9bd137168e77277951766d4e2778390195daaea82fc` |

## Remaining proof and gates

Actual different fixture/test/harness review is required before any test body or
foundation execution. Root must then observe the selected Boost-only foundation
green and the first real behavioral red at the distinct missing-feature marker.
Compile/link/discovery success, old numeric passes, skips or failure inversion do
not replace those gates. There are no skips, marker-as-pass catches or inversions.

These are public value-boundary assertions. Allocation-before-projection, temporary
arithmetic/scratch order, bad_alloc propagation and the valueless-variant defensive
path still require independent implementation source audit; no unsafe forged
std::variant or global allocator interception was invented as a fixture. Caller
construction/copies remain outside per-call budgets. No permission, temporal pin
update, cache/store, credit/refund/effect, reporting UI or live/provider capability
is implemented or verified here. Required RATESTEST-009/RATES-009 must preserve the
recurring rational through historical valuation/commands/codecs/SQL before a usable
award adapter exists. All three providers and platform/release work remain required.
RATESTEST-001 remains in progress; RATES-001 remains unimplemented.

## Actual independent review correction R1: mixed-byte ordering

Independent service_reviewer recorded the complete candidate review in `6d4a8db`.
Its blocking R1 finding is valid: the old identity-ordering case compares only
0x80 with 0xff. Both signed-char (-128 before -1) and unsigned-byte (128 before
255) comparison give that same order. The old assertions therefore cannot enforce
API section 2's explicit unsigned-byte ordering requirement, also required by the
approved logical raw-key model. This is an oracle coverage defect, not a new
product policy or an implementation-driven fixture adjustment.

Root released an additive correction in the existing method only. Every previous
high-byte and typed-namespace assertion remains unchanged. The new independent
input has observation IDs 0xff, ASCII `O`, and 0x80 in that source order. Each owns
the same consistent A→B rate 1 and has a real active fetched constraint; all source
IDs are distinct and the existing A/B component is connected. Default bounds fit
without weakening input validity or inventing an ID encoding restriction.

The literal expected source AND fetched-constraint order is `O`, 0x80, 0xff.
These fixed assertions detect a signed-char comparator; no project result computes
an expected sequence. The new input has its own nonfatal accepts block after the
old block, so the earlier unexpected marker cannot prevent this scenario from
being reached under ordinary GoogleTest failure handling. No marker is caught as
success. The exact 53 case names and every old source line/assertion are preserved.

Only acceptance source and this note change. Fixture blob
`8936f4fae9bd17d5e592256e0c395871bcea4849` and foundation blob
`28a829aad3769f176db82b497120ba4ad05a0818` remain unchanged, as do the header,
four immediate stubs and approved contracts. Root separately reported the narrowly
reviewed Boost foundation green with 14 facts after approval `4e9c615`; this author
did not replay it, and that result supplies no model capability or feature approval.

Correction checks, all exit 0:

- The same strict C++20 acceptance compile/link command above: zero warnings.
- `--gtest_list_tests` only: exactly 53 names, matching static definitions and
  the original candidate. No test body/setup/feature or exception marker runs.
- Static comparison with `b0517b7`: there are no deleted or modified old source
  lines, and the old/new case-name lists are identical and unique.
- clang-format dry-run, project/link/progress consistency and staged whitespace/
  ownership checks accompany this correction's commit.

Corrected acceptance Git blob:
`64fd60b27359db8ba7b3d17f77f8e349ae7e30f9`.
Corrected acceptance SHA256:
`88638401670905088f2b9cb0daaedee78327d40b5642abe932d92785491d2528`.

Actual revised full-test review is mandatory before the first feature red. No
foundation, one of the 53 test bodies, new feature or marker body has been invoked
by this author, and no self-certified review or passing feature is claimed.
