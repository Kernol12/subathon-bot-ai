# SCHEMATOOL-001 — first integrated functional public64 and focused checks

Observer `/root`,6 November2026. Actual new-code diagnostic run on CLEAN tracked
root0f8ed17c2f96ca60236f8cf961c48caaff541502, parent3438a92c/worker8f4c8c7,
original public test source64f4911 unchanged. Worker source actually approved root98bc
and C0e1f; parent e0a98d9 approves only two published-handle paths and withholds
OVERALL ownership/deadline approval. C separately assessed that the full unchanged
functional run is allowed; it is not a historical-stub-red replay or a source waiver.
No root source/HEAD changed during these five invocations.

Exact first full command at13:28:36.955226Z:

```sh
env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8 /tmp/subathon-schematest001-py314/bin/python -I -B tests/schema_validation/wrapper_acceptance.py
```

Cwd `/tmp/subathon-schematool001`, admitted Linux x86-64 CPython3.14.9/isolated20
packages. Real worker/fresh synthetic temp state and actual pinned backends. Actual
exit0,64tests/38.297s unittest/38.367851s external, no failure/error/skip; independent
300s external watchdog did NOT fire. [Raw stderr](schematool-001-public-functional-first.stderr.txt)
9268bytes SHA2564a7a34aa099cc933f6b12027196b0e1c3ad92f79386fd6e7924298025b3f7987;
[stdout](schematool-001-public-functional-first.stdout.txt) empty;
[receipt](schematool-001-public-functional-first.receipt.json) records all exact source
blobs/argv/environment/head/review scope. Output is actual native bytes, never rebuilt
from expectations. The observer propagates suite failure/timeout; no inversion.

Then on the SAME unchanged HEAD/production sources, ran each whole focused module
with the identical minimal environment/interpreter/-I-B. Each had a separate45s
external watchdog/new process group and direct raw logs; no watchdog fired. No
selector, decorator, skip, expectedFailure, validating fake or oracle change.

| Module | Actual methods / native time | Exit | Receipt |
| --- | --- | ---: | --- |
|parent_cleanup_regression.py|2 / .010s|0|[receipt](schematool-001-integrated-parent_cleanup_regression.receipt.json)|
|worker_context_regression.py|3 / 1.452s|0|[receipt](schematool-001-integrated-worker_context_regression.receipt.json)|
|worker_lexical_regression.py|3 / .106s|0|[receipt](schematool-001-integrated-worker_lexical_regression.receipt.json)|
|parent_spawn_regression.py|2 / 1.017s|0|[receipt](schematool-001-integrated-parent_spawn_regression.receipt.json)|

All10 focused methods pass, alongside the separately invoked public64. Context3
contains7 literal private-worker observations; it does not exercise the public
parent. Other focused modules cover only their documented actual failure phases.
Do not substitute source-counted sub-observations for method/runtime counts.
Each receipt links exact empty stdout/native stderr/hash/timing/source; no claimed
standalone artifact or command beyond those actually recorded.

These are exercised FUNCTIONAL observations on stated Python implementation, not
a full native resource/cancellation/availability certificate. Known source gates
remain: unpublished PID/return allocation, constructor deadline, thread-start
acknowledgement and before-real-close tracking. C overall parent approval/taskdone/
merge remain WITHHELD. Native C++ boundary is DOCUMENT-ONLY proposal; its private
contract/policy/build/fixture/source/facility/red/integration gates remain. No tests,
limits/public outcomes/worker semantics or release scope are reduced by these passes.

Original63+new1 FIRST artifacts remain byte-identical and were not replayed. No
live provider/chat/contribution/deploy operation, secret fixture, global dependency
or actual exhaustion/stress occurred. Same-code passing work should not be repeated
for logs; future actual parent/native/fixture changes justify relevant reruns.
Different actual receipt attribution and a concrete diagnostic draft-CI publication
scope decision remain before pushing this functional checkpoint. All current
native/source completion gates must stay visible even if hosted tests turn green.
