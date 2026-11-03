# STORAGE-002 — returned support-condition byte boundary

Author: bundle_reviewer, 2026-11-03, task/STORAGE-002-code-review. Root explicitly
assigned additive regression authoring after independent source inspection found
an extra fictitious payment-evidence byte in helper824f58d/blobd9982c99. The author
cannot approve this new test; root's separate actual oracle review and reached red
must precede the held correction159850c. Original32 baseline assertions/fixtures/
ports are unchanged; historical original32 red is not repeated.

Authority: specification v0.45 DB-01/NUM-04; concrete bundle revision1's logical
owned-byte rule: count actual logical owned members once, enums/bools1, signed/
uint64 fields8, absent optionals0; actual ReceiptFacts schema_version is uint32 (4).
ApplicationContributionInput owns ContributionConditions in its support anchor,
not a nested ContributionState. Observation evidence is already a separate member.

## Independently derived literal boundary

The additional public API case
`supportAnchorConditionsUseExactReturnedByteBoundary` reads actual disposable-file
`read_inputs({"plan-P",1}, contribution)` from the unchanged frozen literal_setup.
It requires a complete ready payload at1105 and known whole-unit result_limit at
1104, including prior caller-marker rollback and unchanged physical snapshot. It
uses the existing ample writer limits, so this is the DTO owned-byte quota rather
than a writer binding/result quota. No expected count is obtained from production
helpers, UUT return values or observed database output.

Every length below comes from the frozen input INSERT literals and public DTO
fields; source receipt1 and event receipt7 have equal relevant byte lengths.

| Member | Independent arithmetic | Bytes |
| --- | --- | ---: |
| Subject kind, PlanKey, actual head |1+(6 plan ref+8 revision)+8 |23 |
| Selected CapturedPlanRevision |8 revision+134 event+33 captured state+53 valuation+250 requests+64 rules+46 modifier |588 |
| Current state |5 token+35 format+6 document+11 config+27 timer/debt+2×(1 kind+5 target+8 units) |112 |
| Current ContributionRecord |16 IDs+18 verified identity+77 receipt facts+22 arrival+59 current state |192 |
| Support anchor |8 optional CID+115 receipt+2 observation kind/evidence+33 verified link+32 Conditions |190 |
| Complete returned inputs |23+588+112+192+190 |**1105** |

The event134 is7 event-ref+0 present-empty parent+4 root+8 receipt ID+8 receipt
identity+77 facts+22 arrival+8 CID. Receipt identity8 is1 provider+1 account+5
namespace+1 opaque ID. Facts77 is17 event-type+4 schema+8 timestamp+0 actor+0 absent
display-name+10 original-money+22 payment reference+16 activity reference. Arrival22
is8 timestamp+0 connection+11 config+3 context. A receipt115 is8 ID+8 identity+
77 facts+22 arrival.

Captured state33 is1 evidence+7 Conditions enums/bools+10 money+15 policy. The
resolved valuation53 is1 state+2 resolution+10 money+3 comparison-currency+7
effective-rate+8 fetched-rate+6 snapshot+16 timestamps+0 present-empty source/manual
refs. Three request rows each63 are11 action+9 version+7 rule+5 target+1 unit+1
dependency+4 canonical Exact bytes+8 requested+6 explanation+11 reversal behavior;
the fourth is61 because its Exact is1/1. Each rule32 is7 ref+18 version+1 selected+
6 explanation. Modifier46 is11 definition+11 instance+5 target+1 group+8 group-op+
3 target-op+7 factor.

Current state59 is1 evidence+7 Conditions enums/bools+10 money+18 resolution+14
policy+1 decision choice+6 decision ref+2 actor. Verified activity identity18 is1
provider+1 account+7 namespace+9 opaque ID; verified link33 adds15 verification.
Support Conditions32 is7 actual enums/bools+10 money+15 policy. It has no separate
payment-evidence member: that enum is already in the anchor's2 observation bytes.

Authored/static only at this commit; no test body, new API or correction executed.
Root owns explicit CMake registration and exact33 compiled CI guard integration.
Full implementation/code/config approval and normal latest-head CI remain separate.
