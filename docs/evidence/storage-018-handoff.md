# STORAGE-018 — Durable initial capture store handoff

Coordinator /root, 5 November 2026. Focused implementation task under specification
v0.45 §§3/5/17/18, DB-01/API-02/NUM-04 and the accepted
[SQL contract](../contracts/sqlite-initial-captures-v1.md). Baseline PR68 and codec
PR69 are integrated; historical FIRST reds remain valid and were not repeated.

The store implements the immutable version7 manifest and caller-session indexed
source/receipt find/retain port. Two bounded indexed probes precede proposed-target
resolution. A selected capture resolves its complete original receipt/contribution/
plan/retained closure with fixed metadata first, actual payload bounds and strict
typed semantic checks. Exact duplicates reuse the initial meaning; conflicting or
corrupt bindings cannot overwrite it. Inserts remain provisional on the caller's
actual writer session until normal acknowledged commit. Known failures clean up
the entire transaction; unknown cleanup/commit outcomes retain their uncertainty.
No store adopts/commits a session or consults current dimensions, HEAD, history scans,
authority, evaluation or providers.

Author90eeca7/importc0ff1ec, SQL CPP `ccdc4f616529aedc310ffc75259a79f7690239b5`.
Original31/51 assertions and literal878005d remain unchanged. Independent additive
regressions produce38 methods/67 scenarios, with actual review→behavior-red→fix
history in [native evidence](storage-018-native.md) and the
[test author note](storage-018-canonical-regression.md). Four source-audit findings
were resolved only after actual intended reds. The pure codec source/private
boundary, public signatures, selected pins and immutable migration prefix remain
unchanged. [Author](storage-018-author.md) and
[separate source/config review](storage-018-code-review.md) retain exact revisions.

Root full375 PASS28.29s; author375 PASS29.19s; separate reviewer375 PASS28.32s,
zero failed/disabled/skipped/warnings. CMake/CI promotes all38 SQL methods to ordinary
acceptance with exact compiled/registered equality, mandatory labels, no draft-red,
no disabling/failure inversion and an unconditional body invocation. All28 codec
and old suites retain their existing execution. The check name/dependencies/pins
are unchanged. Actual selected Qt CI, final metadata review and latest-head normal
merge checks remain pending; PR70 is a draft until those applicable gates pass.

Root owns shared build/CI/state/map/HTML/integration, capture_fixture_author owns
SQL source/author evidence, capture_codec_tests owns independently authored additive
tests, capture_reviewer owns independent review evidence. All support branches and
old raw evidence are preserved. Default fixtures/state are synthetic/disposable.

This component supplies no authenticated admission, current policy/clock/action,
application producer/orchestrator, recovery, provider/FX, UI, platform or release
capability. DB-01/NUM-04 remain partial. All seven integrations, three FX providers
and full agreed product/release gates remain required. No live operation occurred.
