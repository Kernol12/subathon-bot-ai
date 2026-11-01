# Continuous integration

GitHub workflows use the same selected CMake presets as local checks. A passing
lane establishes only its bounded scope. Exact historical runs and reviewed
revisions are linked from each task handoff; inspect the latest PR head before merge.

| Workflow | Automatic PR/main scope |
| --- | --- |
| scaffold.yml | Infrastructure build, Python tooling, repository/progress checks |
| acceptance.yml | Reviewed time-credit acceptance and regression cases |
| exact-numeric.yml | Reviewed exact decimal/final conversion and regression cases |
| timer-lifecycle.yml | Reviewed pure Main/Grace reducer |
| pause-policy.yml | Reviewed independent pause reasons and delays |
| receipt-intake.yml | Reviewed receipt service with primitive simulation storage |
| contribution-evidence.yml | Reviewed eligibility/linkage with primitive simulation storage |
| application-plan.yml | Reviewed owned captured-plan transformations |
| time-debt.yml | Reviewed32pure debt proposals/composition, exact registration and unconditional real acceptance |
| sqlite-writer.yml | Qt6.12 build/probe, exact registration and writer/work acceptance; stage-specific evidence below |

The SQLite writer lane now runs all22 unchanged reviewed cases plus3 ownership regressions on every PR/main
and manual dispatch. STORAGE-001 implements the original reviewed/red baseline;
[actual acceptance evidence](evidence/storage-001-ci.md) records the first passing
Ubuntu run. The existing `sqlite-writer-build` check name is retained. Historical
stub-red and the earlier build-only scope remain in baseline evidence. No failure
inversion, skipped baseline, WILL_FAIL or continue-on-error is allowed.

The SQLite lane downloads official Qtbase6.12.0 and ICU archives with fixed SHA-256
hashes, extracts with bsdtar, and uses the explicit prefix. Core/Sql/Test compilation and the Qt event loop are checked automatically;
the QSQLITE plugin file is verified and the actual behavior cases load/open it
on disposable files in ordinary automatic acceptance. This adds no installer
action, automatic local download or committed binary. Qt6.12 remains required;
Ubuntu's older system Qt is not substituted. The actual Ubuntu acceptance workflow passed for STORAGE-001; latest-head checks
remain required before every merge. Local archive-consumer evidence alone is insufficient.

The checkout action remains pinned to official v7.0.1. Numeric-dependent stable
lanes retain pinned Boost1.83 Ubuntu headers. System runner/compiler/runtime package
versions are recorded in actual logs rather than called a reproducible OS image.

Add Windows/MSVC, Qt/QML, browser/OBS, deployment/restore and performance jobs as
their harnesses exist. Generic CI cannot substitute live provider, actual OBS or
compositor evidence. These release gates remain in the task/acceptance map.

Ruleset26170263 was inspected on2026-11-01: main requires PRs and resolved threads,
zero GitHub approvals, no bypass actors, and no required-status-check rule. Written
workflow gates still require every applicable latest-head check; adding a workflow
does not configure GitHub enforcement. Recheck live rules/reviews before each merge.

CORE-005 promotes the original32 reviewed time-debt cases to unconditional
ordinary acceptance while retaining the time-debt-build check name. The guard
compares compiled GoogleTest names to all CTestJSON registrations before bodies.
[Actual CI](evidence/core-005-ci.md) records passing Ubuntu32case execution; original
stub-red remains a historical prerequisite, not a present failing feature suite.
This remains bounded pure arithmetic/composition, not complete refund/application
or release proof. No failure inversion or baseline assertion change is permitted.

STORAGETEST-010 established a separate reviewed24-case work baseline with genuine
stub-red, keeping build/discovery distinct from passing old25 behavior; its
[historical CI assessment](evidence/storagetest-010-ci.md) remains valid. STORAGE-010
now configures `sqlite-work-debug` to run both suites unconditionally (old25 plus
new24) on PR/main/manual, retaining the stable job and complete registration guard.
Its [implementation CI assessment](evidence/storage-010-ci.md) records actual
results and remaining gates. Source023451 is independently approved9216d8d; actual Ubuntu41300728417 at
a1e3a89 passed new24/old25/probe1 with verified selected archives and exact
registration. Latest-head CI still applies before merge; these49 cases do not
establish product schemas/recovery/platform/provider release behavior.
