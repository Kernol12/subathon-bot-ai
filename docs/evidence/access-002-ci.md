# ACCESS-002 CI integration

Coordinator /root, 2026-11-05. Claim54d45fd, configurationed30e98.
The existing `effective-permissions-build` check name is preserved. This task
promotes the separately reviewed30-case baseline from intentional-red discovery
to ordinary unconditional acceptance; it preserves every test/fixture assertion,
all inventory identities, disabled/failure-inversion/custom-result guards and
existing10-second case timeout. The historical first red remains linked from
[the baseline handoff](accesstest-002-handoff.md), not rerun or relabeled green.

The workflow still authenticates selected Ubuntu's version-pinned Boost1.83
package through APT, verifies compiler header108300, explicitly permits the common
pinned GoogleTest fetch, checks exact enabled30-case discovery, separately discovers
and runs the existing21-fact SDK environment, and runs the old36 numeric/infrastructure
checks. An additional ordinary CTest command executes all30 permission cases,
with no inversion or continue-on-error. Labels lose only intentional-red; ordinary
suite/acceptance/core labels remain. The old-suite command excludes exactly this
now-separate permission label and environment, rather than relying on red omission.

The production target gains an explicit private Boost::headers dependency and
conditional Boost discovery even if enabled independently of the numeric suite.
Public headers remain standard-library-only. Preset/option descriptions now say
acceptance. No dependency version, schema, migration, permission facts or runtime
interface changes. Actual independent code/config review and selected official
run remain pending until candidate publication; this note alone is not a passing
workflow receipt. Actual repository ruleset26170263 has no required-check rule;
project workflow still requires all applicable latest-head checks before merge.
No branch enforcement is claimed merely from adding this execution step.

Actual local preparation: python3 tools/check_project.py PASS417tasks/122decisions;
python3 -m unittest discover -s tests/tooling -v PASS19; git diff --check PASS.
Host GNU16.2.1/CMake4.4.5/Python3.14.9. Selected1.83 host SDK restoration is separate
because the prior disposable SDK is absent and installed Boost is1.92. Frozen
static_assert108300 is preserved. Native acceptance and official CI will be
recorded in the implementation handoff after actual execution.
