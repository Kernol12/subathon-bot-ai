# ACCTTEST-003 reviewed/red application-plan baseline

Coordinator `/root`, 2026-10-31, branch `task/ACCTTEST-003-application-plan-baseline`,
base mainafcf528. Specification v0.45 §§3/6/7, internal application-plans revision1.

Author `/root/application_plan_test_author` suppliedd615cb4,6ed861e and finale864ff4.
The bounded API needs pure owned core transformations only, without a server service,
repository or fake. Replaceable sources return already-resolved rule/modifier
selections; evaluator wiring and actual atomic application remain API-008/ACCT-004.
Separate `/root/application_plan_reviewer` examined all22 oracles, contract/interface/
stubs and coordinator harness and approved in36ac654;
[independent review](accttest-003-independent-review.md).

| Reviewed artifact | Exact Git blob |
| --- | --- |
| Public interface | `501112d64959d20f682420d33a260e7745f5b378` |
| Three unconditional stubs | `ac9dcd0a73ec12fffaf3bdb1ff45f9f1f80fc23f` |
| 22 acceptance cases | `f93287fa2c7c3ef4753e6bbbedd1d2a54f4529b9` |
| Contract before status-only completion prose | `5bb9a914f8b80a7ad43cabcccf3905294a9f7a63` |

After approval was integrated ata524447, root ran
`ctest --preset application-plan-debug -L application-plan`: actual exit8,22/22
body failures from explicit ApplicationPlanNotImplemented, including wrong exception
types in invalid-input cases. [Actual output](accttest-003-red-ctest.txt) retained.
Author/reviewer/root configure/build and CTest-N discovery22 had exited0 before
bodies. No skip, missing dependency/compilation error or failure inversion counted
as red. Local Linux GCC16.2.1/CMake4.4.5/GTest1.18/Python3.14.8/Boost1.92;
the existing selected Boost>=1.83 headers are reused, no dependency added.

The optional application-plan-debug lane links Boost::headers directly, without
calling the numeric parser/finalizer to generate its expected data. Shared Boost
discovery preserves exact-numeric selection: root configure/build/CTest numeric33
and scaffold3 pass, plus tooling19/DOM/project/current HTML/diff checks. The small
concurrent author-evidence merge conflict was resolved by retaining the author's
complete committed evidence and leaving owned formatting untouched until author
handoff; final actual artifacts were independently reviewed afterward.

This completes reviewed tests and intended red, not plan behavior or persistence.
The intentional-red lane stays separate from stable automatic CI. ACCT-003 must
preserve22 assertions/interfaces, implement the pure functions, obtain separate code
review and add/observe automatic application-plan CI with the selected pinned Boost
setup. Final baseline integration review/latest PR CI/merge remain publication gates.
Current preview means freshness only, not eligibility/authority to commit. Real
permission checks, storage/audit, evaluator, historical-value validation and live
cap/effect/writer/provider integration remain their explicit downstream gates.
