# CORETEST-005 CI assessment

Coordinator `/root`, 2026-11-01. New `time-debt-debug` explicitly builds the pure
stub target plus32 GoogleTest cases and existing CORE-002 target for real lifecycle
composition. It uses existing selected GoogleTest >=1.17, explicit pinned1.18source
fetch in GitHub and no new external dependency or Boost requirement.

New time-debt.yml runs ordinary PR/main build and nonempty CTest discovery only;
no passing debt behavior is claimed. Explicit manual `run_acceptance=true` runs the
real known-red cases and propagates failure normally. No skip/inversion/WILL_FAIL/
continue-on-error makes missing behavior green. CORE-005 must promote these same
reviewed cases to unconditional ordinary acceptance after implementation and observe
actual Ubuntu success. Baseline red remains a separate actual local reviewed gate.

Root `actionlint1.7.12 .github/workflows/time-debt.yml` exited0. Root and author
compiled/discovered32 with real GoogleTest1.18/GNU16.2.1; no behavior run before
separate review. Actual Ubuntu workflow execution and exact independent config
review are pending. Existing ten checks (including original/corrected writer25)
remain required written gates. No GitHub required-status-check rule is inferred.
No provider/credential/live/platform operation is part of default checks.


Actual [run41291917578](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41291917578)
atPR26headc2be886 succeeded. Retained actual logs show GNU13.3.0/Python3.12.3,
selected `_deps/googletest-build` compilation and `Discovered 32 time-debt cases;
behavior not run`. All11checks passed atc2be886. This observes real Ubuntu baseline
build/discovery, not passing debt behavior. Workflowe96126f8 was independently
reviewedb3413e5; actual source/check hashes and latest-head gates remain explicit.
