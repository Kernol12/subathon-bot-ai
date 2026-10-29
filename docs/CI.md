# Continuous integration

The two provided workflows have not been executed on GitHub as part of preparation.
No remote repository has been created. They use the same CMake presets documented locally.

- scaffold.yml: infrastructure-only Linux build, Python tooling and consistency checks.
  A green result is not acceptance of unimplemented behavior.
- acceptance.yml: explicitly/manual workflow with a pinned GoogleTest source fetch.
  It genuinely fails while the domain stub remains. No continue-on-error, WILL_FAIL or
  skipped-case workaround hides those failures.

Test authors first review and record the baseline. As each bounded behavior becomes
implemented, add its reviewed acceptance suite to ordinary PR checks; do not make a
reduced scaffold-only check the permanent release gate. Keep yet-unimplemented behavior
visible in task/acceptance maps and a full acceptance lane. Never label a subset green
as a complete product pass. Expanding CI coverage is a task with a documented change.

Add Windows/MSVC, Qt/QML, browser/OBS, deployment/restore and performance jobs as their
harnesses exist. Actual OBS, compositor and live-provider checks need the specified
environments/credentials; generic CI cannot substitute invented results.

The checkout action is pinned to its official v7.0.1 commit. Verify runner/tool versions
on first execution; no supported-product platform claim follows from the workflow file.
