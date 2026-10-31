# CORETEST-003 reviewed/red pause baseline

Coordinator `/root`, 2026-10-31, branch `task/CORETEST-003-pause-baseline`,
[PR20](https://github.com/Kernol12/subathon-bot-ai/pull/20). Base/current main5201edd;
specification v0.45 §4/§18, TIMER-03/05, pause-policy revision1.

Original author `/root/pause_test_author` supplied30 cases inbb34d1e. Independent
partial review005b6f4 found those known cases valid while two product choices were
unanswered. The owner explicitly answered both in the
[decision record](pause-signal-policy-questions.md). New author
`/root/pause_owner_case_author` added7 focused cases in3c6d242 without changing any
original30 body, header or stub. Independent `/root/pause_resume_reviewer` examined
all37 actual cases, final contract, specification/decisions and harness and approved
in86c0bbd; [full review](coretest-003-independent-review.md) preserves both stages.

| Reviewed artifact | Exact Git blob |
| --- | --- |
| 37 cases | `646d83f4e38722239bb8b590a9f368141bed4246` |
| Contract before status-only completion prose | `a8174be71e7f4cf453a55df14f57126b1b6e61d8` |
| Public header | `c91b7da3fa1c98a1b2e47ab389f48f74901c2a47` |
| Two unconditional stubs | `dee2a91c31b524c082192aba70e7e0c0efa27501` |

After full approval was integrated atf39ccc8, coordinator executed
`ctest --preset pause-policy-debug -L pause-policy`: actual exit8, all37 bodies fail
on `PausePolicyNotImplemented`, including wrong exception types in validation cases.
[Actual output](coretest-003-red-ctest.txt) retains the failures. Configure/build had
already exited0 and CTest-N listed37; no bodies ran before the full approval.
No skips, dependency/build failures, failure inversion or continuation-to-green
were counted as red. Environment: Linux x86-64, GCC16.2.1, CMake4.4.5,
Ninja1.13.2, GoogleTest1.18.0 and Python3.14.8; no new dependency or download.

Root configure/build/CTest regressions exited0: scaffold3, credit23, numeric33,
receipt15, contribution21 and timer39 using their existing presets/labels. Python19
and the progress DOM simulation pass. Final status/project/render/diff and separate
integration review/latest PR CI/merge are still publication gates.

This finishes reviewed tests plus intended red, not pause implementation. The
optional pause lane stays separate from stable automatic feature CI. CORE-003 must
preserve the37 cases/interfaces, implement the pure policy, obtain separate code
review and add/observe stable automatic pause PR/main CI before completion.
Independent recovery rules compose per reason; no global Resume is implied by
clearing Unknown. Caller segmentation/timer wiring, delay reconfiguration, Qt/state
transactions, real provider signals, schedule/recovery algorithms and release/
platform/live verification remain separate gates. Full product scope is unchanged.
