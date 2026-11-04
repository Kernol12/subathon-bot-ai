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
