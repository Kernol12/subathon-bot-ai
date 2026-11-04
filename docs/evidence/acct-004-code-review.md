# ACCT-004 independent implementation review

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
