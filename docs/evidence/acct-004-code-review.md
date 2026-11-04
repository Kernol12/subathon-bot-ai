# ACCT-004 independent implementation review

**Final verdict: APPROVED actual corrected source/configuration ce28ff3.**
The final section pins the accepted artifacts and independent full308 evidence.
Earlier changes-required and bounded-stage findings remain as review history.
Selected official CI, current-head merge gates and full release verification are
separate; no prospective result is approved.

Reviewer: service_reviewer, different from service_author and codec_author,
2026-11-04, isolated task/ACCT-004-review.

## Bounded codec verdict

**APPROVED: command codec and its bounded build integration ONLY.** Actual
candidate b3e4a9003c6c63b57c0e82d8062fd4a451edfcce, new codec source
blob7f0c0e56e97c5f793fb0a8295824c5914b08e817 from author746d5c2/root9eeb140;
server CMake blobd2e81da7ef4b25ede5c80fa8df47c23c3c3b403c adds that source,
and b3e4a90 removes precisely the replaced codec stub definitions. The remaining
service/view methods are still stubs in this candidate and were NOT invoked.
No full service, prospective configuration, ordinary CI or merge approval follows.

Authority: specification v0.45 sections3–5,
[logical service](../contracts/application-service-v1.md),
[concrete API](../contracts/application-service-api-v1.md) sectionsA/B/F at
bbd6cd834b8a959ccff16d63a638e0b77d9c6e4c. The completed reviewed first-red gate
remains in [baseline evidence](accttest-004-handoff.md); it was not replayed.

Independently read every codec line and the exact current grammar/bounds/priority
contract. The decoder scans borrowed byte views without string/vector allocation,
checks complete framing before typed materialization, and cannot let a claimed
choice count create an unbounded loop independent of actual input bytes. Typed
key/source stages precede all counts, then body/conditional groups; no enum wraps
through U8 narrowing. Raw bytes and absent/present-empty groups remain distinct.
All nested options are checked, including dormant store profiles. Logical caller
input, output Envelope bytes and decoded choice items are separate; encode adds no
phantom result items. Checked sizing runs before allocating the encoded buffer.
Decoded fields explicitly own their strings/QByteArrays, including fromRawData,
empty and binary bytes. Healthy unknown format follows input bounds and has no
fallback known-format parser. No review blocker found for this bounded source.

Independent fresh actual CMake build and five unchanged codec slots:

```sh
cmake --preset application-service-debug
cmake --build --preset application-service-debug --target server_application_service_acceptance --parallel 2
ctest --preset application-service-debug -R '^application-service\.(commandEncodingMatchesFrozenFrames|commandDecodingMatchesFrozenMembers|codecBoundsUseActualOwnedDTOs|codecMalformedAndPurePriority|codecOwnsBinaryBuffers)$' --verbose
```

Observed configure0, fresh39 build steps/zero warnings, CTest5/5 PASS0.86s.
The97 actual codec scenarios pass; five separate Qt processes include10 init/cleanup
passes, giving107PASS/0FAIL/0skip/0blacklist. GNU16.2.1, Qt6.12.0, Boost1.92,
Python3.14.8 native host. Exact test CPP36f669edd1b42cdcd78c36145ac61b8b6124c454
remains unchanged. Per-invocation native LastTest SHA256
6f3a064dad7531f5cce2b46f495d94e442de9c2aa387ea8049b34864f59af831.
git diff --check PASS and worktree was clean before this note.
Historical draft-red labels have not yet been promoted in this bounded candidate;
that future actual configuration requires its own inspection. All35/full300 and
service source review remain pending.

## Required full-service audit

Trace callback key-validation before lifetime/thread checks and callback-specific
revocation; actual one-BEGIN/one-COMMIT and only real normal exact-metatype result
publication; unknown cleanup/commit dominance; correct retained/store error source
without duplicate rollback; all historical semantic copies and unsupported formats;
complete current15-field contribution and actual plan-head stage bindings; clock
hard-end advance before paused refusal; sequential runningTimer composition; exact
whole-input numeric/hold evidence; current reference binding/non-retry collisions;
intermediate/final allocation checks before growth. Review actual root build/CI
promotion preserving old guards and pins, then independently execute full acceptance.
No approval is made for source or configuration not actually received.

## First full service candidate: changes required

Actual root candidate95fa152, service source from author722e11b/root e4089bd,
was independently integrated and audited. All35 unchanged service slots PASS3.43s
on this reviewer native build, but those passes do not establish the missing
requirements below. Full implementation approval is withheld.

1. Callback view copies lose their selected key limits after the weak state
   expires, so pure key validation incorrectly uses defaults. ContractC requires
   selected limits before callback expiry/thread checks. Preserve limit DATA
   independently from revocable authority; never extend callback access.
2. SAS1 clock/balance decoding allocates separate bounded vectors without the
   complete combined intermediate B/I check before materialization. A clock
   segment and two balances are three real decoded items, not separate quotas.
3. Historical advanced-before SAS1 policy revision is not checked against the
   success SAA1/after-state policy, accepting contradictory known meanings.
4. Retained Clock parser checks each elapsed nonnegative but omits checked total
   and nonempty finalUTC equality with observedUTC. Known malformed SAS1/SAA1
   semantics must fail before current resolvers replace evidence.
5. Final response budget checks occur after owning payload copies in historical,
   active-hold, held and ineligible paths. ContractF requires projecting complete
   actual output B/I before growing oversized output. Source audit must verify
   ordering in addition to the existing exact-bound runtime refusal tests.

Promoted root configuration95fa152 is inspected: exact35 registration continues
matching compiled slots, rejects disabled/inverted/draft rows, and unconditionally
executes application-service acceptance after all old guards. No old command,
selected dependency pin or check name is weakened. Header edits are comment-only;
preset/CTest labels accurately promote implementation acceptance. Future additive
registration counts/source changes require their actual separate audit.

## Additive regression test review before first defect red

**APPROVED: actual four additive methods only**, root uncommitted CPP blob
6b198e3a72566432d7ca8a2ad523c195872bc8f0, Qt registration959535e,
CI0ca6947 (four additions, exact39). Reviewed actual source independently before
any new test body was invoked; root message records release for reached-red.
No prior method/assertion/fixture was changed.

- referenceViewRetainsSelectedKeyLimit: min store reference1 survives callback
  expiry as DATA; cfg fails invalid_reference before expiry, legal c remains
  expired. Denied access proves zero protected storage dependency.
- retainedStateBudgetPrecedesPolicy: independent supported SAS1 has one clock
  segment plus two balances, combined3 exceeds intermediate_items2 before policy.
- historicalBeforePolicyMatchesAudit: a same-length literal policy byte mutation
  in only historical advanced-before must give contradictory_document after actual
  disclosure, before current policy/actions; no actual typed snapshot is altered.
- retainedClockFinalUtcMatchesObservation: literal observed1001 differs from final
  segment1000 and must give invalid_known_document before the resolver.

Each test asserts actual known rollback/absence of response and byte-identical
remaining database state. The mutation preconditions require exactly one affected
row or independent byte anchor, preventing a vacuous unrelated fixture failure.
These tests do not claim allocation ordering is measured; that remains source
audit. Approval precedes root's future actual reached-red, not an invented result.

The subsequent FIFTH additive method historicalJobMeaningIsNotOnlyAnEcho is also
**APPROVED before first red**: actual CPP1b7d73ca1d9f4266d36c3ef72ca41b9ea001b2b1,
Qt a4993752/CI cee1d8ab expand enabled exact registration to40. It mutates known
SAJ1 magic in one literal historical job AND its identical SAR1 echo, requiring
exactly one updated row each. Matching repeated bytes cannot certify valid known
job grammar. Expected retained_semantics/invalid_known_document after historical
disclosure, before current action preparation, and unchanged state follow contractE.
First35 and the previously approved four additive methods remain unchanged. No
execution of these five was performed by the reviewer before this approval.

Explicit BoundApplicationPolicy admission unresolved_with_reason represents a
required unavailable policy after known full input closure. Logical durable-hold
requirements and concreteD10 support a specific unresolved_policy hold retaining
its reason, rather than discarding that represented distinction in a generic
lifecycle not-ready response. Ordinary paused/recovery/Ready/Finished and
requires_other_command still do not authorize acceptance or automatic reopening.
A separate actual focused regression must precede a correction to this mapping.

## Two further bounded regressions approved before execution

**APPROVED actual CPP8ab5dba0c7b3edf1b763abfc1cbde6650277fb12**, Qt f5f08ab1,
CI0700f15e, exact42 enabled cases. Previous35 and five additive methods unchanged.

- explicitUnresolvedPolicyRecordsKnownHold changes only supported policy admission
  to unresolved_with_reason with required-action-policy. The existing independently
  frozen dActionsMissing cause/annotation is still exact: admission disposition
  is not serialized into Policy; actual source, command, supported Policy, Clock
  and missing reason are identical. Required exact durable creation, no action
  call, no speculative state reference and no partial application/head change
  follow concreteD6/D10 and logical unresolved-policy hold requirements.
- retainedCommandBoundIsNotMalformedData clears choices only on the NEW caller
  command so pure choice_bytes1 admission succeeds. The old healthy retained SAC1
  has a six-byte choice; its selected internal bound must produce known result_limit
  after disclosure before complete retry comparison, never label valid history
  malformed. This isolates internal classification while preserving all public
  codec caller-input categories.

No new test body was invoked before this independent source approval. Root may
observe actual reached-red on the still-uncorrected candidate. Full source, fixed
regression passes, complete acceptance and selected CI remain separate gates.

## Negative counter purpose regression

**APPROVED before execution**, actual CPP7329b24fe97c02ee98307cf20ed8d947c90364cb,
Qt8e9d85ee/CIc63ee177, exact43 cases. Added only
negativeCounterRequiresCorrectionPurpose (two data rows: score/count). One old
plan row is changed to exact-1/1, finalized-1 and matching counter target, and an
actual typed old-store read verifies a valid complete signed input before service.
The incoming ordinary captured_original purpose is explicitly asserted. ContractA
requires the correction purpose for negative requests; exact signed counter support
does not create an implicit correction grant. No timer deficit policy is invented
for counters. Expected access/policy-only ordering isolates this missing gate from
a later synthetic job mismatch that would otherwise happen to produce the same
generic invalid_output reason. Known rollback leaves all arranged rows unchanged.
Prior35 and seven additive methods remain unchanged; no new body invoked before
this actual source approval. Full corrected-source verdict remains pending.

## Retained-command decoder correction review

**APPROVED bounded source correction217acc7** (author3ef4755), command TU
05091e2552a8b2baaac410524a583386bb20bb1c, private header
7d9171cd45bd2b05edc5ec48c927beee74be976e. Independently inspected every diff
line and complete shared framing/validation/materialization paths. Public codec
preflight categories remain unchanged. Internal healthy selected field/count bounds
now use result_limit; actual malformed keys/roles/tags/conditional groups retain
structural categories. Complete retained envelope uses selected storage/value and
intermediate limits, decoded Command uses its own intermediate B/I, both checked
before any owned materialization. No public caller aggregate or public result
budget substitutes for these independent values. No widened allocating public
decode precedes the internal projection. The actual service call-site replacement
and complete regression execution remain pending in this bounded candidate.

Independent actual CMake target recompiled the codec/test objects and linked
successfully. The same exact five public codec CTest slots listed above passed
5/5 in0.71s,97scenarios+10initcleanup=107PASS/0FAIL/0skip. The local Ninja cache
reported premature-log-end recovery twice during CMake regeneration; compilation
itself produced no source warnings/errors. No retained-command regression or other
uncorrected service regression body was replayed. Full corrected service review,
all43/full308 acceptance and selected CI remain required.

## Final actual corrected source and configuration: approved

Independent reviewer service_reviewer, 2026-11-04. **APPROVED** actual corrected
rootce28ff3 (author4612223), retaining previously reviewed codec correction217acc7
and all eight separately reviewed/reached-red regression slots. No runtime source,
public method, test oracle or configuration blocker remains for these exact blobs:

| Artifact | Accepted Git blob |
| --- | --- |
| application_service.cpp | f20187a7880ca53fcd5060d8ba3bc970484b9ec2 |
| private sqlite_application_bundle_values.hpp | 162373edddadee8583c83abbcbb421a0fae9990e |
| application_service_command.cpp | 05091e2552a8b2baaac410524a583386bb20bb1c |
| private application_service_command_internal.hpp | 7d9171cd45bd2b05edc5ec48c927beee74be976e |
| public application_service.hpp | 84f09f21ae354faede0bca255ab61714922feb45 |
| server CMake | d2e81da7ef4b25ede5c80fa8df47c23c3c3b403c |
| feature tests | 7329b24fe97c02ee98307cf20ed8d947c90364cb |
| Qt registration | 8e9d85eee4724b9663e3d11af430f2e57fe1f3a1 |
| selected workflow | c63ee1772ea3c5a46123256077e8c8935b0c8f44 |
| presets | 90eabe4503eaf70dfb20174b0733c6137bf2d8a1 |

Read the actual complete service and successive correction diffs, including the
final459-line service hardening and tiny private overload. Checked these source
requirements separately from runtime observations:

- Selected read-limit DATA survives view expiry; weak shared authority plus atomic
  revocation limits each view to its own callback. Key validation precedes expiry
  and thread checks. Live foreign calls cannot touch Qt SQL/session state.
- Current execution authorization precedes protected lookups. Historical disclosure
  precedes semantic validation and immutable selected-capture reading. That read
  uses existing guarded same-session Access::plan for the recorded revision, never
  revalues against current policy/head. Full canonical command envelope equality
  after known-semantic decoding retains every logical member without re-encoding.
- SAS1/SAR1/SAA1/SAJ1 byte-view scans validate known grammar before owning parsing.
  Combined SAS1 ownership includes clock segments, balances and actual echoed
  reference copies. Clock sums and finalUTC are checked. Both before/after policy
  and configuration agree with successful audit; synthetic jobs match immutable
  selected requests and actual occurrence components, not merely repeated SAR1 bytes.
- Complete current ContributionState is copied directly to expected_contribution,
  preserving all15 fields/optionals independently of state token. Actual plan head
  is checked on input and by direct bundle stage. Every original stage refusal maps
  explicitly; reversal-only outcomes cannot become original success. Ready is
  excluded before timer reduction; paused Active still observes hard end before
  final refusal. Only current running timer and actual main_added feed each positive
  occurrence. Signed minimum uses Exact arithmetic; all negative units require
  correction purpose, with deficit policy restricted to milliseconds.
- Numeric failure preserves whole-input SAH1/SAH2 evidence, full correction list and
  actual failed balance while keeping captured target separate. No fitting prefix
  is staged. Explicit unresolved policy retains its reason in a durable hold.
  Historical already_recorded detection never certifies a fresh active hold.
- Complete initial proposal is measured before copying clock/balances. Pending
  effect byte formula matches published field walk: fixed18, actual Exact/residual/
  explanation, three9-byte time components or one9+target counter component,
  optional cap8 or deficit tags2. Complete proposal plus pending effect/transition
  items is checked before construction/append. Refusal paths do not manufacture
  hypothetical provenance/components.
- Hold sizing runs without materialization; exact detection projection follows
  published source/command/plan/observation/failure fields and all occurrence data.
  Three policy or four numeric annotation copies are charged independently before
  cause/observation/occurrence construction. Encoded format bytes, before/after
  snapshots, complete PreparedBundle and final protected response each receive
  their own checked projection before owning growth. The private borrowed job
  validator removes duplicate shell copies; its original wrapper delegates with
  unchanged validation body/order. No existing store rule or oracle is weakened.
- Actual one-BEGIN session owns every normal commit. Healthy denials/duplicates
  commit read-only units; known local failures perform actual cleanup, established
  store/query rollbacks are not repeated, and unknown transaction truth dominates
  remembered reasons. Only committed/none/present EXACT-metatype normal WorkResult
  can provide the response. Tail exception, wrong QVariant or publication failure
  cannot fabricate or undo accounting.

Allocation ordering is established by actual source audit; no runtime test is
claimed to instrument allocations or create a private timer/current-state race.
Component reducer/storage runtime evidence and the source traces above retain
their distinct proof roles.

Independent final native checks in the isolated reviewer worktree:

```sh
cmake --build --preset application-service-debug --parallel 2
ctest --preset application-service-debug --verbose
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Build91 actual steps PASS, zero compiler warnings/errors. Ninja reported recovery
of a prematurely ended local cache log before the successful build; no source/test
change was made to mask it. Full CTest308/308 PASS24.65s;305Qt processes total
1420PASS/0FAIL/0skip/0blacklist, including43 feature slots/233 scenarios and their
86init-cleanup passes. Native GNU16.2.1/Qt6.12.0/Boost1.92/Python3.14.8. Final
verbose full SHA2561232be399c589a8d47c0d4a0c4b5ff803255818f0fde79a27e97bdbbda520a4a;
build SHA256541c7b2aa9afcc1be8be94137c3b0dab41deefc69a560767342c22ca0757ed09.
Project396/122/link/preset/currentHTML/render/diff checks PASS. Compiled/registered
feature count remains exactly43; all enabled acceptance, no draft/failure inversion.

Independent byte comparison strips only the one additive block and reproduces the
entire original35-method test CPP byte-identically. Actual committed raw additive
red audit confirms5+2+2 genuine behavior failures across8 processes,16initcleanup
PASS, zero setup/skip/missing markers. Review messages and exact earlier blob pins
above preceded each first invocation. The additive files were uncommitted at those
runs; no clean-head claim is made. No historical red was replayed. Canonical raw
SHA256s: first5 7787aa63e444c8fecc3a59b61659538536b43758f0b23fa6eb38bf8d7e7ccaf6;
next2 0848edeb34de33a17bae5fe2dbdfbb21d2433e9e7ec5b2f9c1d254d3dd5ceaa7;
last2 250bd59fb931da405d02ddebe80b04722dca7d2e1a17ed1dbb6c7539a0ecf238.

Actual selected officialQt run41781024558 at publisheda49ec06 was still active when
this source verdict was written. Coordinator must observe its actual outcome and
latest-revision CI/review/discussion/rules gates before normal merge. Source/config
approval applies only to the pinned artifacts, not future changes or unknown CI.
No real authority/policy/clock/reference/action/provider adapter, financial refunds/
review continuation, recovery/UI/platform/packaging/live/full-release proof follows.
