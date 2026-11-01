# CORE-005 bounded refund deficit/time-debt implementation

Coordinator `/root`, implementer `/root/debt_implementer`, 2026-11-01, branch
`task/CORE-005-time-debt`, base integrated mainbca06e8 after PR26 merge.
Specification v0.45 §4/NUM-03; [contractrevision1](../contracts/time-debt-v1.md).

Baseline contract909f16e7/headerf3cc819c/tests c3cd6c65 are unchanged. Separate
baseline approvalb3413e5/final5474fdc and actual32stub-red atdba4401(exit8) remain
valid and precede production implementation. [Baseline](coretest-005-handoff.md).

Sourceauthor6394cab integratedcbfdd85, blob
`68141b15a4578cc85b4537a9b1403638c6e445f2`, implements checked pure proposals.
Reversal comparison and signed remainder avoid abs/negation ofINT64_MIN; Carry
checks available debt headroom before negation/addition. Review holds the entire
request; Stop retains unapplied signed data. Additions repay debt before Main cap
headroom, retain above-cap Main, and reject only unrepresentable actual uncapped
results. Owned full request references/policy remain distinct from actual effects.
[Author evidence](core-005-implementation.md).

Root ran configure/build time-debt-debug, CTest `-L time-debt` and fullpreset:
exit0,32/32 and35/35 respectively. Author independently recorded same counts.
Environment GNU16.2.1/GTest1.18/CMake4.4.5/Ninja1.13.2/Python3.14.8. Separate
code/configreview of actualf0c888f and real Ubuntu acceptance remain pending.
[CI assessment](core-005-ci.md) records ordinary acceptance promotion.

No existing public contract/assertion or lifecycle source is changed. This helper
accepts already-calculated eligible inputs; it supplies no permission, override
audit, deduplication, proportional entitlement/revaluation, ledger allocation,
authoritative transaction, persisted review record, clock selection, post-finish
admission or provider/platform support. Core lifecycle composition is tested with
real existingCORE002; actual application wiring remains its recorded release gate.

Root owns shared build/preset/CI/labels/map/state/HTML/checkpoint and focusedPR.
Final taskdone waits codeapproval and actual ordinaryUbuntu acceptance, followed
by final integration/latesthead CI/discussions/currentnormalrules before merge.


## Independent approval and actual CI

Separate code/configreview98f3f04 (integrated091eda9) approves source68141b15 and
workflow792b14b6 after independent32label/full35pass and signedrange/wholehold/
debt-first/caller-boundary/ownedrequest/error inspection. Guard32equality and negative
metadata probes plus actionlint/project/render/diff passed. No source/assertion
change was requested; [review](core-005-code-review.md).

Actual UbuntuCI41294422355 atPR27headde78828 passed all32 real proposal/composition
cases, exact compiled32registration and GNU13.3.0. All11checks passed there. Root
retained the observed logs; no known-red waiver or manual bodyguard remains.
Bounded CORE005 behavior is done; final completionmetadata/actualCI-evidence review,
latest-head checks/discussions/currentnormalrules remain before
[PR27](https://github.com/Kernol12/subathon-bot-ai/pull/27) merges. Other full-release
and application gates above remain explicit and unverified by this helper.

Actual [PR27](https://github.com/Kernol12/subathon-bot-ai/pull/27) merged1259cfb
at09:53:12Z on2026-11-01. Finalheadcba35c3 all11expectedchecks success, no
review threads/currentnormalrules. Final integrationapproval387a4f6 retained;
no bypass or required-approval substitution. CONTRACT-004 is separately claimed
from refreshed main to define the writer/service transaction prerequisite before
new storage adapters; CORE005 is not a full refund/application completion claim.
