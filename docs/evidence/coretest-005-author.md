# CORETEST-005 authored refund deficit/time-debt baseline

Author `/root/debt_test_author`, 2026-11-01. Branch
`task/CORETEST-005-debt-author`, based on coordinator claim9ff4af9 and integrated
main1183314 after STORAGE-001 merged. This is authored/compiled/discovered work,
not independent approval, observed red, implementation or passing acceptance.

## Scope and local decisions

[Time-debt revision1](../contracts/time-debt-v1.md) refines specification v0.45 §4
Refund overflow, NUM-02/03/04, the task brief and event/state boundary revision1.
It supplies a pure Main/debt numerical proposal with full owned request references,
actual Main/debt/unapplied fields and no UI/Qt/SQL/provider/clock dependency.

There is no invented Active-only restriction: lifecycle/current observation,
permission, post-finish ledger handling, reopening and the authoritative transaction
remain with callers. Existing CORE-002 composes eligible Main credit and normal
zero-time ending transitions. Grace is a separate countdown and is never passed
as a debt balance. ACCT-006 alone calculates proportional/cumulative refund
entitlement from actual original awards/rates; this helper never recalculates it.

The three policies are explicit; no default refund setting is selected. Require
review applies when a reversal exceeds Main, and holds the whole adjustment rather
than first subtracting available Main. The interpretation is derived from §4's
Refund overflow heading, §18's exceeds-remaining-time scenario and CORE-005's
explicit deficit scope. Equality/zero need no new deficit hold. Independent review
must approve these actual contract/oracle choices before red.

Signed reversal data includes INT64_MIN without unsafe signed negation. Final
unrepresentable Main/debt rejects atomically with overflow_error, preserving the
caller-held full request for durable review. Additions pay debt before Main caps;
all actual components are separate, without inventing a single ledger credit
meaning or changing upstream entitlement/provenance rules.

The draft above-cap precondition was reconsidered before approval: a pure balance
addition preserves Main at/above cap with zero headroom, still repays debt first,
and cap-limits the remainder. Literal100/debt5/request8/cap60 yields Main100/debt0,
repayment5/Main0/capped3. This follows the brief without deciding how actual cap
edits are accepted or corrected. Existing calculate_time_credit's explicit
above-cap rejection and all its tests are unchanged.

## Authored oracles

32 GoogleTest cases use literal independently derived expected fields and no result
computed by an implementation under test:

- All three policies for fitting, exactly exhausting and zero reversals; full
  deficit Stop/Carry/Review records include both opaque references and policy.
- Whole Review hold, old debt preserved across selected policies, owned copied
  references, invalid balance/sign/reference/policy inputs including invalid enum
  on both deficit and within-Main paths.
- Signed MIN Stop remainder-1, Carry from Main1 to exact debtMAX, no-Main overflow,
  full MIN Review hold, and existing-debt overflow without a partial proposal.
- Partial/exact debt repayment at full/zero/above cap; debt then cap partition;
  uncapped surplus, zero above-cap request, actual cap/debt results avoiding an
  overflowing hypothetical Main+request, true final Main overflow and validation.
- Existing CORE-002 composition: debt-only additions cannot rescue Grace or
  confirmation wait; only surplus Main credit rescues Grace with budget retained;
  exhausted Main uses Immediate, Shared Grace or Confirmation without making
  Grace negative or silently treating debt as Grace.

The source has only two unconditional TimeDebtNotImplemented throws. No new
production validation, policy, arithmetic, lifecycle or persistence is implemented.
Existing core interfaces/sources/tests are untouched.

Independent preliminary review identified missing opaque reference/policy
assertions on held/limited branches. These were added before any formal approval;
addition early debt-only paths also assert full requested reference/amount. No
assertion was weakened or fixture changed to make production pass.

## Actual compile/discovery

Linux7.2.11-arch1-1/x86-64, GNU16.2.1, CMake4.4.5, Ninja1.13.2, installed
GoogleTest1.18.0. Versions were observed with uname, compiler/CMake/Ninja version
commands and pkg-config. No dependency was downloaded/installed.

Coordinator owns shared CMake/presets/workflows; author used a disposable local
C++20 CMake harness, linking the actual authored stub source, existing timer reducer
and installed GTest::gtest_main. Commands from the author worktree:

```sh
cmake -S /tmp/subathon-coretest005-author-harness -B build/coretest005-author-compile -G Ninja -DSUBATHON_SOURCE="$PWD"
cmake --build build/coretest005-author-compile
build/coretest005-author-compile/core_time_debt_acceptance --gtest_list_tests
git diff --check
```

All exited0. Final compile is warning-free; discovery lists32 cases, without test
body execution. No CTest/red/behavior run occurred. Any intended red must follow
independent approval of actual final artifacts; compilation or absent dependencies
would not count. No fake provider, real supporter data, network or live state is used.

## Handoff and remaining gates

Author owns only new contract/header/explicit stub/test source and this evidence.
Root owns shared build/presets/CI/state/HTML/map/checkpoint/PR; no shared path was
edited here. New debt target needs no Boost or added library. Composition tests
link the already selected timer target and GoogleTest.

Independent exact-revision review and actual intended stub-red remain baseline
gates. CORE-005 must preserve the reviewed assertions and add ordinary automatic
acceptance when implemented. The coordinator's baseline build/discovery CI and any
actual workflow result must be recorded separately; this author observed no CI run.
No finished product, ledger/schema, transaction, live/provider/platform, recovery,
backup, UI or deployment claim follows from this baseline.
