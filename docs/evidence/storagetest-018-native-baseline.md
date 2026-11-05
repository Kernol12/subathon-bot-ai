# STORAGETEST-018 native baseline integration

Coordinator /root, 5 November 2026. Baseline incomplete; no production authorized.

## Build and first diagnostic

Authored codec367f758/importf7e4653, fixture3dc3e5c/importd677c8c plus
rollback-oracle correctione35deae/importf7c3127; root harness0cec1d8. Separate
capture_reviewer source/fixture/harness approval preceded first invocation.
Configure/build sqlite-reference-capture-debug with the existing selected Boost1.83
paths succeeds; CTest show-only336 = prior308 + codec27 + one foundation executable.
All enabled, zero failure inversion. No feature entrypoint had been invoked.

First diagnostic command atf7c3127:

```sh
ctest --preset sqlite-reference-capture-debug -R '^scaffold.sqlite_reference_capture_environment$' --verbose --output-junit /tmp/storagetest018-foundation.xml
```

Output was redirected to /tmp/storagetest018-foundation.txt. Actual exit8:
5 Qt passes/1 failure/0 skips including init/cleanup,195ms. Schema/full92 projection,
all constraints and whole rollback passed. A+B closure check failed because each
literal index-row query scoped only installationI and selected BOTH healthy rows.
This is a diagnostic oracle defect, NOT an intended missing-feature red. Neither
new codecs, store nor manifest was invoked. Preserve this observed failure.
Author correction3f5082e adds all five exact source-key predicates; all13 typed
expected members and one-row assertion remain. Different actual correction review
and new diagnostic invocation are required; no fixture freeze from this failed run.

Initial output SHA256: 742e43f7b6dfff68aa6e4caeffc3060b456ba83b47ce9d2456492dd8c76576da

## Remaining gates

Reviewed corrected foundation must pass natively and in selected official Qt CI.
Codec test review is complete. SQL source/config review, selected-driver proof
and genuine intended red remain distinct. Existing308 passing history does not establish018 acceptance.

## Corrected reviewed foundation observation

Different capture_reviewer approved actual source21c212d, additive dual-key query
assertionsdd661f0 and final JSON878005d95a0c12d69df6b836ac09cd59ba5168ba
BEFORE the second invocation. Independent notes11e07c9/1c6ac77 retain both
findings and approvals. Same diagnostic command with output suffix
foundation-corrected ran at21c212d, exit0: all4 diagnostic slots/6 Qt passes
including init/cleanup,0fail/skip,192ms. Host Qt6.12.0; selected official driver remains a separate CI gate. The actual
92-object projection and full seven history identities match independent literals;
both key projections, constraints, A/B closure and whole rollback pass. New
codec/store/manifest entrypoints remain uninvoked. Selected official CI remains
a separate prerequisite to final fixture freeze and FIRST red.

Corrected output SHA256: b5e6c97fff3654cbbe0566ebe92d5083c502d74cdaaa9fe248c6041f8a7a425e

Stable integration command after corrected foundation approval:

```sh
ctest --preset sqlite-reference-capture-debug -LE '^draft-red$' --no-tests=error --output-junit /tmp/storagetest018-stable309.xml
```

Actual exit0,309/309 PASS (old308 plus the new foundation),25.40s, no failures
or disabled/skipped cases. Raw output /tmp/storagetest018-stable309.txt. This
explicitly excludes enabled intentional-red feature baselines and is not a full
product suite pass. New57 cases require actual review/discovery/FIRST red.
