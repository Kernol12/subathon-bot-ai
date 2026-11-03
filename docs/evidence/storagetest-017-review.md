# STORAGETEST-017 — independent feature-test and registration review

Reviewer: bundle_reviewer, 2026-11-03. Verdict: **APPROVED as the authored
test-baseline definition**, after actual corrections649b2c1 andce732e0. Reviewed
root candidate54bface, including unchanged harness3389670. This is independent of
the feature-test author and prior independent fixture author. The new reader is
still its immediate named unimplemented stub. No feature body or new entrypoint
was invoked during this review; intended red remains a subsequent gate.

## Exact reviewed artifacts

| Artifact | Git blob |
| --- | --- |
| Feature CPP, final18 public slots /35 scenarios |0e4f4f006d2a0e161857e38255cac487f7de83a9 |
| Feature author evidence, final35 scenarios |6fdfb5fba6afe9fba4e388908fde15a9a1b876bf |
| Accepted concrete original-claim contract |bad19c47ed397547a552e8f64e2141e87116f0d7 |
| Public bundle/claim header |cf721e7f61f10dacfb7d078e1545a2d403e0a4df |
| Source with immediate named new-method stub |130e3db62b421479ec3d0a25ce2e712480482f0e |
| Unchanged existing bundle helper |b0ca921cb27068ecc33228d136b82506fcb6e0a3 |
| Independently frozen new fixture JSON |bea1273cc6858c3998066e26e11e7d870a02c1bc |
| Independent fixture helper / complete expected DTOs |67d4fbd28a4c92e24c7b9b10e063638a9ba90541 |
| Old-primitive foundation green CPP |7dad8108fd9b902174f304d62789eb0375669b98 |
| Root-owned Qt CMake registration |c35ccf28cf378a0634491fec4e9dc7fef7561e42 |
| Root-owned SQLite workflow |7fec9ade311c9928d019ad465b19d686e7f861b0 |
| Root CMake / presets |4a404d0537e50315efd69d618ed06c9b2acf6438 /323e1db8a5276c36d4ff3dd08dd56617c9f5b760 |

Read root/tests instructions, STORAGE-017 brief, accepted logical/concrete/physical
contracts, actual API/stub, independent fixture/helper, all1062 initial CPP lines,
both actual additive corrections, final author evidence and complete harness diff.
The final CPP/author/harness equal root54bface. Production source/header/helper,
all prior fixtures, fixture helper and green environment equal approvedad69fecf.
Earlier contract/API/fixture reviews and their observed checks remain applicable.

## Contract and oracle assessment

All18 methods were examined, including every parameterized branch. Four data
groups contain6 selector,9 closure,3 byte-bound and3 Exact scenarios;14 other
methods give35 scenarios. init/cleanup and data providers are not extra acceptance
registrations. Coverage is specificationv0.45 §§3.4,5/DB-01/NUM-04 through the
accepted contract's OC01–14, rather than invented provider or service behavior.

- OC01–04: independently frozen A/B competing GLOBAL tagged-subject/source
  predicates; different actual receipt/role/CID; absence for nonexistent9/42;
  selected A despite corrupt unselected B; signed ORIGINAL-kind0 C remains valid,
  whereas the separate old kind1 financial reversal cannot match. Candidate8 is
  explicitly not asserted to be a real linked confirmation in this fixture.
- OC05–06/14: options before invalid role/Receipt0/CID-presence before expired or
  foreign ticket; caught pure refusal preserves prior staged caller work. Valid
  foreign/expired/terminal tickets retain wrong_thread/stale_session/session_closed,
  including actual sticky terminal completion. Worker results are transferred
  after join, with the named stub captured and rethrown safely.
- OC07–08: independent A161=45+3×32+18+1+1, B160=44+3×32+18+1+1; match adds exactly
  one discriminator byte. A SUBJECT162/161, B SUBJECT161/160 and B SOURCE161/160
  test fit/one-less while old lookup's body-only bound remains unchanged. The
  selector is not echoed; absent with input_items1/owned_bytes1 has zero output.
  Old full vectors are20=2+2+2 balances+4 effects+8 components+2 jobs;20 fits/19
  refuses with input_items1. No UUT byte counter supplies these literal totals.
- OC09–10: nine feasible selected faults cover result, SQL audit class, historical
  contribution, dangling selected claim, source-selected missing own claim, missing
  job, group gap, wrong SQL-class Receipt and broken selected PA history. Positive
  healthy PA1/2 and changed current head/contribution retain historical A. Negative
  PA head2/count1/min=max1 rejects broken selected history. Three Exact faults pin
  both SQL classes before size, size before grammar, and noncanonical200/2.
- OC11–12: source-only setup uses literal45-statement prefix0..22, changes both
  application states to pending, and inserts initial head a. No retired b/c/d
  state is deleted/repointed to manufacture ABA. Old stage consumes independent
  expected A; the new query never produces its own expectation. Outside observer
  stays pending/unclaimed until caller COMMIT; actual rollback removes the full
  provisional application, commit/reopen retains one A. Facade destruction keeps
  caller staging alive. Healthy present and absent calls preserve caller marker.
- OC13–14: known selected/limit failures require independent BEGIN IMMEDIATE and
  exact raw SQL-class/byte snapshots INSIDE the synchronous catch while facade
  lives, with earlier caller marker gone. The cumulative one-row writer budget
  is actually consumed before the reader, not inferred from SQL-call count.
  Real old fault seams require synchronous rollback/COMMIT uncertainty and no
  normal payload; COMMIT uncertainty additionally proves writer fail-close.
  Six-byte BOM/NUL/FF output survives facade/writer/fixture/file destruction and
  independent copied-buffer mutation; present-empty and absent remain distinct.

Reviewed capture/runner paths preserve the named stub before category matching.
Input/category helpers catch only their intended typed exceptions. Every outcome
checker propagates a captured stub, including worker and uncertainty cases; those
cases also require actual reached/read or synchronous-unknown flags. A stub plus
later owner abandonment cannot accidentally pass an uncertainty oracle. Raw
setup assertions precede new calls; storage failures cannot pass merely because
cleanup eventually occurs after the facade or work ends.

Three concrete review findings were corrected in actual revisions, preserving
existing assertions:649 added B SOURCE's exact byte boundary and moved the common
initial read-only snapshot check ahead of selector0's corruption/return;ce732
added the selected broken PA history refusal. All three fixes were inspected in
the final complete CPP. No remaining blocking test/oracle/registration finding.

## Actual verification and harness review

Linux/GNU16.2.1, CMake4.4.5, Ninja1.13.2, QtCore/Sql/Test6.12.0. Isolated worktree
task/STORAGETEST-017-review, actual final author commits imported; root-owned
configuration copies retained uncommitted for verification. Only this review
document is reviewer-authored.

- `cmake --preset sqlite-original-claim-debug`: exit0.
- `cmake --build --preset sqlite-original-claim-debug --target server_sqlite_original_claim_acceptance`:
  exit0,4 actual MOC/compile/link steps, ZERO warnings/errors.
- Compiled executable `-functions`: EXACT18. `ctest --preset
  sqlite-original-claim-debug -L '^sqlite-original-claim$' --show-only=json-v1`:
  EXACT18 registrations equal the compiled set and single actual executable.
  Global show-only JSON:228=previous210+18. No DISABLED/WILL_FAIL inversion.
- Harness adds only one new executable and explicit18 registrations using the
  approved bundle/QtTest plus six frozen vector macros. Removing that new target/
  registration block restores prior fixture harness548f5e2. Removing only the
  appended workflow discovery block restores priorb7bbc0e. Existing pinned Qt/
  dependencies, all old guards/case counts, mandatory3 foundation guard/run and
  unconditional35 bundle execution remain unchanged. New compiled/CTest equality
  guard is mandatory ordinary CI; it discovers intentionally red cases without
  invoking them. No blanket failure suppression or skip is introduced.
- `python3 tools/check_project.py`: PASS396 task records/122 decisions, links,
  presets/current HTML. `python3 tools/progress.py render --check`: PASS.
  `git diff --check`: PASS. No unchanged native baseline replay was necessary.

Raw reviewer files under ignored local-evidence/storagetest017-review:

| File | SHA256 |
| --- | --- |
| test-review-configure.txt |75da5a7e823457e660e76d243a523d5081c9474974e5d72d1b6e72558d49db0d |
| test-review-build.txt |1adaa1fc07c8d7cdadf54e0e52a82277ea764561ec6771486c881a04452fb105 |
| test-review-functions.txt |896797a7eef57019b222159e37cd81719e56cf31cf030e0fb1111f631d92cf11 |
| test-review-discovery.json |a7386a7082d99d8518ee473502c192320e3701a47b4e85899ebbff4de31f4603 |
| test-review-all-discovery.json |f3aa4c65a92ccadf9b7fb52f6f0fb6a0f3cc1da556eda1c5039912f96f82c2c3 |

The reviewer metadata checker initially used incorrect source-expression counts;
corrected static expectations and the compiled/registration checks pass. That
checker error invoked no body and is not an intended feature red.

Approval applies to these actual authored tests and registration. Next gates are
the complete approved head's own selected official foundation-green/discovery,
then the FIRST actual named-stub reader red with raw attribution. STORAGETEST-017
is not completed by this review alone; STORAGE-017 production still waits for
that observed intended red, then separate actual source review and acceptance/CI.
No future CI head or query/service/hold/provider/recovery/platform capability is
preapproved. Prior valid source/contract/fixture reviews and original reds remain
preserved; all agreed release obligations remain required.
