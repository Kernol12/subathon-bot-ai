# STORAGE-011 — migration catalog implementation handoff

Coordinator `/root`, 2026-11-01. Contract: [migration catalog revision1](../contracts/migration-catalog-v1.md),
specification v0.45 §16/DB-01/OPS-02. Branch `task/STORAGE-011-migration-catalog`
starts from PR31/main `ef29c87`. Corrected implementation is under independent review; no source approval or
merge is claimed.

## Change and scope

The runner inspects catalog, complete schema projection, raw Unicode bytes,
encoding, integrity, foreign keys and full content-hashed history before normal
store use. Explicit advancement applies bootstrap and the whole missing prefix
in one existing writer transaction; refusals/cancellation preserve old state,
and unknown outcomes remain fail-closed without a normal readiness report.
Pure codec/preflight, budgets and admission use the frozen revision1 boundary.
No writer API, generic SQL allowlist, domain schema, new dependency or connection
is added. OPS-02 operator initiation/backup/compatibility/quiescence remain
actual trusted caller obligations, not request-field claims.

## Exact evidence and verification

- [Original reviewed/red baseline](storagetest-011-handoff.md) and
  [review](storagetest-011-review.md) remain intact. Original22 assertion coverage
  and ten independent literal vectors are preserved. No original red repeated.
- [Implementation](storage-011-implementation.md): source9eb1fef/blobd91c7d,
  integratede183527; evidence0ac896c integrated0118fea. Author/root real
  configure/build/full76 pass locally with Qt6.12/SQLite3.54.0, GNU16.2.1.
- [CI assessment](storage-011-ci.md): shared66be523 promotes the unchanged22
  to unconditional automatic CI behind mandatory green7 and old49; exact
  compiled registration guards and pinned selected Qt/ICU/actions remain.
- Separate source reviewer found valid initial U+FEFF rejected through a Qt
  fromUtf8 round-trip BOM removal. Passing76 did not cover this. Approval/merge
  are held for separate regression authoring/review/red before correction.

## Ownership and next gate

Implementer `/root/debt_code_reviewer` owns only server/src/sqlite_migrations.cpp
and implementation evidence in storage011-implementation. Different reviewer
`/root/storage_work_code_reviewer` owns code-review evidence in storage011-review.
`/root/writer_transaction_reviewer` owns additive Unicode Qt regressions/evidence
in new storage011-unicode-regression. Root owns shared build/CI/state/HTML/map/
checkpoint and integration. Original baseline contract/header/vectors/green7 and
writer023451/49 are unchanged. No user decision/access is needed.

Next: actual regression test approval, current-source intended red, Unicode
correction and actual different code/config review, then ordinary automatic
Ubuntu acceptance and all latest-head PR/thread/rules merge gates. Default files
are synthetic/disposable. No app bundle/domain adapters, backup/recovery,
power-loss, supported release platform or live-provider verification is claimed.

## Corrected local candidate

Actual additive test approvalad4f54d/root8c70e6e preceded the root selected2
intended red at d91c7d/e94befdc, exit8 with fixture/init/cleanup passing and
zero skips. [Regression evidence](storage-011-unicode-regression.md) and
[raw output](storage-011-unicode-red-ctest.txt) preserve the sequence.
Correction071af3b/root2cb0d1b, sourceblob4ee2365f8dc6f1189d0f7bf02a090596a9ad3c6f,
removes the faulty decoder equality after complete Unicode-scalar validation.
Exact encoded-length and raw stored-byte comparisons remain. Root actual build
and full78 CTest pass (migration24, writer/work49, infrastructure5 including
the seven-slot green probe). Root independent framing rederived all638 bytes and
SHA baf9a94e...775b7d; no UUT supplied expected results. Original22 bodies, ten
JSON vectors, contract/header, green probe and writer49 remain unchanged.
Different corrected-source/config review and actual officialQt Ubuntu CI remain
required before implementation completion. No live/platform gate is upgraded.
