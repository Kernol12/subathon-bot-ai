# STORAGETEST-017 — independent minimal API/stub and profile review

Author: bundle_implementer; shared configuration author: root coordinator.
Reviewer: bundle_reviewer, separate agent/new isolated storagetest017-review tree.
Date: 2026-11-03. Verdict: **approved for the actual MINIMAL API/STUB and compile
configuration at these exact artifacts**. This is no query implementation, fixture/
test-baseline, new-entrypoint execution or intended-red approval.

Actual authored revisionbd3a830, imported by rootd29033d, follows actual document
approval39a50970 of contractbad19c47. Reviewer imported actual documentsdcae/d775
and minimalAPIbd3; copied exactly root's three configuration files for verification.
Root retains configuration authorship/shared-file ownership; only this review file
is committed by reviewer for integration.

| Reviewed artifact | Git blob |
| --- | --- |
| Bundle public header |cf721e7f61f10dacfb7d078e1545a2d403e0a4df |
| Bundle source |130e3db62b421479ec3d0a25ce2e712480482f0e |
| Root CMake |4a404d0537e50315efd69d618ed06c9b2acf6438 |
| Presets |323e1db8a5276c36d4ff3dd08dd56617c9f5b760 |
| SQL workflow |4015c5474a75b4674e5595c0e6372819ecb5e163 |

Read applicable root/server/tests instructions, accepted concrete claims and logical
service contracts, actual full API/source/config diffs and author evidence.
The new selector has EXACT existing SubjectKind, ReceiptId and optional CID.
Kind enum is subject0/source1; Match owns that discriminator plus complete
RecordedApplicationBundle. The only new method is const, accepts selector const&
and returns optional<Match>. No constructor/equality/convenience authority or
extra identifier/proposal/plan/current-state field is introduced.

The unique SqliteOriginalClaimNotImplemented marker belongs only to this feature.
New source definition is FOUR lines and immediately unconditionally throws that
marker. It performs no options/source validation, Access/helper call, lookup/SQL,
transaction/mapping/cleanup or callback. Contract comments describe future reviewed
behavior and explicitly identify the present named stub. Existing seven implemented
functions, constructor/destructor/facade copy ownership and old marker are untouched.
Programmatic removal of ONLY the scoped marker/types/declaration and four-line
method restores old header8e24f062/sourcea9056be4 BYTE-EXACT.

Root CMake adds one default-OFF ORIGINAL_CLAIM_TESTS option which enables its existing
bundle dependency before targets are configured. New profile19
sqlite-original-claim-debug inherits18; all previous configure/build/test presets
remain exact. New test preset retains noTestsAction=error. Workflow changes ONLY
profile/binary-directory tokens: replacing new profile name with old restores
exact783dda7 workflow. Actions/dependency pins, permissions, check name, mandatory
fixture guards, actual old case counts and unconditional old35 execution are
unchanged. There is no new feature suite/body/skip/failure inversion in this phase.
No public interface/schema/manifest/fixture/test assertion was weakened.

## Actual independent checks

Environment: GNU16.2.1, system Qt6Core/Sql/Test6.12.0 and Boost1.92 on Linux.
In the isolated tree using the exact root configuration above:

- cmake --preset sqlite-original-claim-debug: exit0.
- cmake --build --preset sqlite-original-claim-debug: exit0; fresh92 work steps,
  ZERO compiler warnings/errors, including actual changed header/source compilation.
- ctest --preset sqlite-original-claim-debug: **209/209 existing tests PASS,7.03s**,
  exit0. Parsed206 Qt execution totals:0failed/0skipped/0blacklisted.
- CTest show-only JSON: EXACT209 enabled registrations, including unchanged35 bundle
  cases; no DISABLED/WILL_FAIL. This is existing acceptance/stability, not a new
  reader test or missing-behavior red. The new entrypoint was NOT invoked.
- Programmatic API removal, workflow token reversal and preset19 removal restore
  exact accepted old artifacts. check_project PASS396 records/122 decisions/current
  links/presets/HTML; render --check PASS; git diff --check PASS.

Raw reviewer diagnostics retained in this worktree's ignored
local-evidence/storagetest017-review/, including old209 LastTest/discovery JSON:

| Raw check | SHA256 |
| --- | --- |
| Configure |3dc305c032a8293a35007536f32120e15100cf6792ffae670958f9141f128fc0 |
| Build |9457a1fded9f6f9b1ea986fe9f69aeb67f5832e30a1a111e12fc9f95bf1f8495 |
| Existing209 |59a478ab5814326cde125c5fec0fe08cad48e356cd06f6f3f5b8d7c8fd924ba8 |

No unresolved minimal-interface/configuration finding remains. Root must publish
this actual API/profile revision and observe its OWN selected official-Qt CI before
assigning independent fixture/test authoring. Existing claim/old-suite CI is no
future artifact approval. Fixture/API/test/environment/first-red gates remain
separate: independently frozen coherent literals, actual test review, successful
compilation/exact discovery and required selected-environment proof must precede
THIS new method's intended stub-red. STORAGE-017 production remains unimplemented/
planned. All service/authority/hold/provider/platform/release gates remain explicit.
Three uncommitted root configuration copies remain deliberate reviewer setup; no
native command is running. No new reader body, SQL, provider or live effect ran.
