# Final local repository checks

At `fcb0860`, in the task worktree and the already admitted ordinary CPython3.14.9 environment, all following commands completed exit0. Exact stdout/stderr/revision/timestamp/hash receipts are in the [raw archive](schematool-001-final-local-checks.tar.gz).

- `env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8 /tmp/subathon-schematest001-py314/bin/python -I -B tools/schema_validation/check_baseline.py`: exact20 installed metadata and64 reviewed public test registrations, no feature invocation.
- `python3 -m unittest discover -s tests/tooling -v`:19 tests pass.
- `cmake --preset scaffold-debug`, `cmake --build --preset scaffold-debug`, `ctest --preset scaffold-debug`: configure/build pass, all3 infrastructure tests pass. This is not product acceptance.

After final metadata at root90 source plus task status update, `python3 tools/progress.py render`, `python3 tools/check_project.py`, `python3 tools/progress.py render --check` pass:426 task records,122 decisions, current offline HTML. SDK14 pins/libraries are unchanged; prior independently reviewed real-library receipts remain valid and both hosted SDK14 runs are required. They are not repeated locally merely for new logs. Native/public feature acceptance and independent review are separately recorded in [actual pass evidence](schematool-001-native-acceptance-pass.md).

Archive SHA256 `f17cf5c28d8347c1ad1207d7617c14f780ffe4592d64824cbce484f1684cf0be`, 2349 bytes. No live/provider/whole-product/platform verification claimed.
