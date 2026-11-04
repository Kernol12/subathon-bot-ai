# RATESTEST-001 independent fixture, test and harness review

**Current verdict: CHANGES REQUIRED for the full53 feature methods: R1 below.**
The actual Boost-only foundation and its narrow harness remain approved. No
feature body or marker execution is released before the actual corrected test
review and selected foundation green.

Reviewer service_reviewer, different from author codec_author and shared integrator
root, 2026-11-04. Owns only this note in task/RATESTEST-001-test-review. Actual
candidate5905f6e includes authorb0517b7/import03124ab and shared harness7ff7d2d,
with current main3217c21 merged. Approved model76d2d9b3, API556840c2 and immediate
stub3a06ff18/4cf1c1ed remain unchanged.

## Narrow foundation gate

Read the complete actual Boost-only source and shared registration/workflow.
Exact foundation blob28a829aad3769f176db82b497120ba4ad05a0818 contains only
Boost/std headers and its own main. It links Boost::headers, not the rate/core
library. Symbol inspection of the actual compiled executable finds no subathon
symbols. It contains no parser/factory/query, marker, project include, data builder
or successful substitute for new behavior.

Its14 literal facts cover independent exact fractions, the existing decimal
place-value product,4096/4097/8192/16384-bit geometry and the independently declared
large-potential pair. A separate Python stdlib integer/Fraction calculation confirms
all14 facts without importing project code or executing the native foundation.
These are dependency/arithmetic facts, not model acceptance.

Exact shared test registration is8cc2bbc17ccd09a36e4ee55e0701c19fd171319b;
workflowc336395d6235e5ae4900398cd8069f0c4737d252. The standalone foundation is
registered as scaffold.rate_snapshot_environment. The workflow requires it before
running the old36 checks, explicitly excluding intentional-red features. The
actual guard checks53 raw/unique TEST definitions against enabled/uninverted CTest
cases and the required labels. No feature body runs during compilation/discovery.

Actual isolated commands:

```sh
cmake --preset rate-snapshot-debug
cmake --build --preset rate-snapshot-debug
ctest --preset rate-snapshot-debug --show-only=json-v1
```

Fresh configure/build pass18 steps with zero compiler warnings: GNU16.2.1,
installed Boost1.92.0/GoogleTest1.18.0 and Python3.14.8. Discovery lists90 cases:
53 new rate methods,33 existing numeric, three infrastructure and this foundation.
Executing the actual inline workflow discovery guard passes53, without test bodies.
The selected CI Boost1.83 environment remains a separate coordinator observation.
No native foundation, feature test/setup or rate entrypoint/marker has run in this
review worktree before this approval.

Root may now run ONLY the accepted Boost-only foundation and old non-intentional-
red checks/ordinary CI. Final actual full fixture/test review and selected green
foundation remain required before FIRST rate marker red. No future corrected test,
production source, provider, valuation bridge or platform capability is approved.

## Actual complete fixture/test review

Read all1158 acceptance-source lines, the complete raw vector header, all53
ordinary TEST definitions, helpers/compile-time traits, author evidence and shared
registration/workflow against the approved model/API. Actual reviewed author
candidateb0517b7b6198b3034c40ca3a0a305359efbb143a has these blobs:

| Artifact | Git blob |
| --- | --- |
| Literal raw vectors | 8936f4fae9bd17d5e592256e0c395871bcea4849 |
| Acceptance methods | 21b819b9fd870fb94936013f51dbac52b9c99637 |
| Test author note | bc64f0b4f97c26957a9f3d804e168190b13155fd |
| Boost-only foundation | 28a829aad3769f176db82b497120ba4ad05a0818 |

The independent literal setup is valid: no factory/query/parser creates another
entrypoint's input, expected result or discovery data. All builders return raw
aggregates; private output construction is not bypassed. Independent Boost integer/
rational literals supply expectations, not the new implementation. No input claims
verified provider capability, permission, durable provenance resolution or a
working historical valuation adapter.

Manual field walks verify S1 raw102/output106 bytes/items9 and short snapshot
raw25/output27/items6. The output-size rows independently add actual optional
strings/int64s, original spelling plus canonical Exact components, enum/variant
tags and separately owned constraint references. Short manual evidence plus its
reference adds11 output bytes and two items. Negative money includes its canonical
minus; signed zero canonicalizes numerically while its original spelling remains.
Available S1 pair projection has11 bytes; identity is3 and missing1. Queries must
still validate the entire candidate's106-byte projection first, with borrowed
query keys excluded from snapshot bytes. Input-preflight and post-semantic output
boundaries use distinct expected categories as the API requires.

The53 methods comprise7 money,7 observation,7 query,17 snapshot,13 limit and2
priority cases. They cover exact source spelling, signed amounts and zero, opaque
currency bytes, unknown/present-empty/epoch/extreme time fields, immutable ownership,
selected versus unselected rounded quotes, duplicate identities/keys/references,
manual-only/pin conflicts, canonical anchors/order/shared membership, reference/
ordinal/endpoint resolution, complete-candidate query validation and identity/
available/missing results. Global input preflight, declared lexical order and
duplicate/shape/reference/equality precedence are explicitly distinguished.

The large input is internally consistent without a graph solver: B..F potentials
are the declared powers of3, G..K reciprocals of the declared powers of5, and ten
finite decimal source edges match their literal exponent differences. Quote
integers fit the256-digit decimal policy; F/K components are4096 bits and their
K-to-F ratio is an8192-bit integer. Lowering the potential bound to4095 refuses
that valid value, while4097-bit or1235-byte input refuses before rational/shape
work. Existing separate CORE finalizer limits are not redefined.

All13 options test zero/above maximum on all four entrypoints. Other cases pin
selected ceilings and default16 observations,4096 per-source quotes,16384 included
quotes,1024 overrides,4096 constraints and256 effective members. The uniform4096-
edge table has distinct directed keys, connected members and consistent rate1;
the total-quote table retains unselected keys without charging them as effective
members. These inputs fit their other independent item/byte ceilings. The suite
does not pretend every maximum is simultaneously reachable or that public tests
measure allocation ordering. Valueless-variant construction, bad_alloc propagation,
arithmetic scratch/preallocation and permission/durable/update behavior remain
explicit future source/adjacent-operation obligations.

rejects uses nonfatal EXPECT_THROW for RateSnapshotFailure, independently checks
the exact code/message and rethrows that expected type. A distinct marker is a
wrong-type ordinary failure, not success. accepts wraps each positive scenario in
nonfatal EXPECT_NO_THROW. Source-location/loop traces permit later row attribution
without running factories in setup. No skipped/disabled/inverted case or successful
fake value exists. Runtime statements after an unexpected exception inside one
positive scenario still will not run; first-red attribution must report reached
calls/rows rather than claim every later assertion executed.

### R1: mixed ASCII/high-bit ordering oracle required

API section2 explicitly requires unsigned-byte lexicographic string ordering, and
the logical model requires canonical raw-byte ordering. The current
IdentityOrderingUsesUnsignedBytesAndTypedNamespaces case uses only source IDs
0x80 and0xff. Their relative order is the SAME for signed-char comparison
(-128 before-1) and unsigned-byte comparison(128 before255). The current assertions
therefore cannot reject the very signedness error named by the case.

Add one independently valid mixed ASCII/high-bit input requiring ASCII before
0x80 before0xff, with real active references for every included source. Preserve
the existing high-bit and typed-namespace assertions; the addition can remain in
the same method with an independent accepts block. A small mixed-byte canonical
currency-member case can also pin that ordering domain without changing policy.
This is an additive oracle for an existing explicit API rule, not a product choice,
weakened expectation or observed-feature-driven fixture change. Root must release
the bounded author correction and obtain actual revised-artifact review before
any first-red feature execution. The accepted foundation remains unchanged.

Root explicitly agreed this finding and released only acceptance source/author
evidence for the additive independent scenario within the existing53 methods,
preserving all existing assertions/vectors/foundation. Actual corrected review is
still required; no future revision is approved. Independent digit/bit calculations
and project398-record/122-decision consistency, render and whitespace checks pass.

The actual full fixture/test candidate is not approved until R1 is resolved. No
feature test body, setup, foundation or marker has been executed by this reviewer.
Build/discovery success and the narrow foundation approval do not finish
RATESTEST-001 or authorize RATES-001 implementation.
