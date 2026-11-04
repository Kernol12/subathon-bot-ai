# ACCESSTEST-002 selected environment harness

Root authored the opt-in `effective-permissions-debug` preset 26 and the
independent SDK target around source blob
`8d88fd0e7f81de2ba300648ff7066f7bff62c5a3` (service_reviewer). The target links only
Boost headers, has no project or GoogleTest dependency, and is an enabled,
uninverted 10-second CTest check with a separate environment label. Default builds
remain unchanged. The new workflow uses the existing selected Ubuntu package
`libboost1.83-dev=1.83.0-2.1ubuntu3`, explicit authentic configs, compiler macro
verification, mandatory discovery and a mandatory green SDK check. Intentional
feature red will remain an explicit separate operation; its target/inventory has
not been authored yet.

Actual preparation on 2026-11-04, GCC 16.2.1, CMake 4.4.5, Ninja 1.13.2:

```sh
cmake --preset effective-permissions-debug \
  -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 \
  -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset effective-permissions-debug
ctest --preset effective-permissions-debug --show-only=json-v1
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All final commands passed. Discovery lists 37 cases, including exactly one enabled
environment case with the intended label/timeout. No CTest body or native foundation
has run. The first configure attempt exposed the directory scope of Boost's imported
target; finding Boost at the parent scope for this opt-in lane fixed generation.
That configure failure is not a behavioral red. Raw local logs:
`/tmp/accesstest002-configure.log`, `/tmp/accesstest002-build.log` and
`/tmp/accesstest002-discovery.json`. SDK authentication/selection and author's
compile-only receipts are linked in the [source handoff](accesstest-002-environment-author.md).

Different codec_author review of this actual source/harness/workflow is required
before root executes the first selected native environment. No API, evaluator,
feature baseline, product acceptance or remote CI result is claimed here.
