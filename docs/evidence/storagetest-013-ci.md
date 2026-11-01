# STORAGETEST-013 actual CI evidence

Coordinator /root, 2026-11-01. This record distinguishes compiled interface,
selected fixture support and future intentional-red behavior.

## Actual API stage

[Automatic PR run41329002993](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41329002993)
at actual head `a152a82ed66413c73e99a5fbe443eabdaf569528` completed SUCCESS,
event pull_request. Root read actual GitHub metadata and full executed logs using
`gh run view 41329002993 --json databaseId,headSha,status,conclusion,event,jobs`
and `gh run view 41329002993 --log`. Ignored copies are under
`local-evidence/coordinator/storagetest013-api-ci-a152a82.{json,log}`.

Observed on Ubuntu24.04: both pinned official Qt/ICU archive SHA256 checks OK,
CMake GNU13.3.0, actual selected Qt6.12.0/SQLite3.53.4. The changed workflow uses
sqlite-receipt-debug and actually compiled new sqlite_receipts.cpp at13/48 and
linked libsubathon_sqlite_receipts.a at35/48. It ran all old25writer,24work,
24migration and16retention cases successfully. Existing migration diagnostic7
methods report9passes including init/cleanup/0skip; existing retention diagnostic3
report5passes including init/cleanup/0skip. Discovery matches each compiled suite.
All11 PR checks at this exact head were separately observed SUCCESS.

This is interface/stub compilation plus stable foundation evidence. Four named
SqliteReceiptsNotImplemented throws remain; no new receipt fixture/case/body is
authored or run at this stage. It is not receipt acceptance, new fixture proof,
intended red, production implementation or full-release verification.

## Remaining CI and baseline gates

Different actual API/stub/precision/build/workflow review precedes new test and
literal-fixture authoring. Those then need separate actual full review, successful
compilation, complete compiled/CTest registration, required green receipt-fixture
diagnostics in this selected official environment, and only then the first genuine
intended missing-behavior red. Intentional-red feature bodies stay separately
invoked and visible; never invert/hide failure in ordinary passing CI. STORAGE-013
implementation remains a later focused task after the baseline merges normally.
No platform/live/provider, recovery, power-loss or deployment proof follows.


## Actual local full-candidate preparation

Fixtureauthor33df432/root5561e69 supplies JSONa79e5e71 and greenb1ca37be;
featureauthor135d3d3/root97863f9 supplies testa68cd5a7. Root actual
`cmake --preset sqlite-receipt-debug` and build/link passed without warnings.
`ctest --preset sqlite-receipt-debug --show-only=json-v1` and both new executables'
`-functions` match118registrations (old95 +feature22 +one green executable),
exact22new compiled/CTest slots and the three required green slots.
`ctest --preset sqlite-receipt-debug -R '^scaffold.sqlite_receipts_environment$' --verbose`
passed all3methods/5including init/cleanup/0skip on local Qt6.12/SQLite3.54.0.
`ctest --preset sqlite-receipt-debug -LE sqlite-receipt --output-on-failure`
passed96stable registrations (old95 plus new green executable). New receipt feature
bodies were deliberately not invoked before their actual full review. Ignored root
captures are storagetest013-{discovery.json,functions.txt,green-functions.txt,
green-root.txt,stable-root.txt} under local-evidence/coordinator. These are actual
local setup/discovery results; selected official new-fixture CI proof and full
independent test/harness approval are still pending before intended red.


## Actual revisions after independent test inspection

Before any feature body/red, the reviewer requested focused oracle additions;
all previous assertions and22names remain. Feature5a8b700/rootaa80102 adds a
true cumulative two600000-result variant alongside the original oversized value,
and a nine-byte stored ID. Fixturecde2ef8/roote33d742 adds actual NULL INSERT
refusal. Featuread60c19/root7fa9f0a isolates retained TEXT3 versus BLOB4 at value3;
9b4f38d/rootf3b2517 proves an observed new-row delta before each forced-stage fault.
Current test0e61209e and green8189f027, unchanged JSONa79/APIb43/stubfaf/logicale1/
physical2ef. Root incremental build/link passes without warnings; revised118
registrations/exact22+3 compiled functions match; revised green3 passes5 including
init/cleanup/0skip. Captures use storagetest013-revised-* under the same ignored
coordinator directory. No feature body/red was run. Actual full re-review and
selected official new-fixture proof remain pending; no prior API-only CI result
is relabeled as the new fixture proof.


## Actual full review before selected fixture publication

Final strict-type913354a/root8630e74 and safe worker diagnostic183c908/root8dfaf22
produce testdab97946. Root rebuilt it without warnings and rediscovered exact118/
22. Different actual full approval7df446e, integratedb1c77e1, covers finaldab,
green818/JSONa79/frozen interfaces/contracts and shared harness. All oracle and
diagnostic findings are resolved. Reviewer independently compiled/discovered final
artifacts; prior green5/stable96 inputs are unchanged. No feature body/red has run.
Root now publishes the approved full candidate for required actual selected NEW
fixture green3 and22discovery. Its actual run/result must be recorded before the
first intended feature red; earlier API-only success cannot replace this proof.
