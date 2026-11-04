# ACCESSTEST-002 discovery label compatibility correction

Root author, 2026-11-04. Actual selected CI at head6b9c27a failed in
[run41884253112](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41884253112)
with `Permission case has unexpected suite labels`. Compilation succeeded and the
strict guard stopped before green test steps or any feature body. This is a harness
failure, not behavioral red. Full/failed logs are retained locally under
`/tmp/accesstest002-ci-6b9*.log`.

The correction retains exact discovery, four required labels, timeout, no disable/
inversion/custom skip/result rewriting, and separate intentional red. It moves the
list-valued label assignment out of GoogleTest helper property serialization into
an ordinary CTest-time include, after discovery's explicit TEST_LIST is available.
The include rejects an absent list. No reviewed test, inventory, oracle, API/header/
stub or SDK foundation changed; existing actual test approval1d8e9335 stays scoped
to unchanged sourcefc82d17e/inventoryd0c25108. The earlier local harness approval
7d7f651 remains historical; this actual correction needs a new different review.

Root configure/build/list-only and both unchanged actual CI Python discovery guards
passed:67 total, exactly30 enabled permission cases with all four labels and10s
limit, one independent green environment. No feature/marker/test body ran.
Read the primary upstream
[CMake3.28.3 helper](https://github.com/Kitware/CMake/blob/v3.28.3/Modules/GoogleTestAddTests.cmake).
Its property serialization expands lists; relying on it for multiple labels is
unnecessary. A scratch reproduction invokes that authentic helper with only the
scalar timeout, the actual binary's list-only command, then this exact label include.
All30 names/four labels/timeouts match. Commands/output are in
`/tmp/accesstest002-cmake328-labels/`. This uses host CMake4.4.5/CTest with the old
helper source, not an installation or runtime claim about old Ubuntu tools.

`python3 tools/check_project.py` and whitespace checks pass. New independent
harness review, actual corrected selected Ubuntu CI, FIRST reviewed behavioral red,
its independent attribution, final status/checks and normal merge remain required.
