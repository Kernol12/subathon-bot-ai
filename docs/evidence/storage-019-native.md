# STORAGE-019 native integration

Coordinator /root, 5 November 2026. Source is under independent review; component
implementation is not yet complete. No SQL capture capability or release proof.

Initial source c2db28be/import146f0c3, CPP423f1197/private9fc608d. Fresh root build:

```sh
cmake --preset sqlite-reference-capture-debug -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset sqlite-reference-capture-debug -j4
ctest --preset sqlite-reference-capture-debug -LE '^sqlite-reference-capture$' --no-tests=error --output-junit /tmp/storage019-root-336.xml
```

Build145 steps, exit0/no warnings. Root336 (original27 plus stable309) PASS25.45s,
0fail/disabled/skipped. Author separately27 PASS0.10s and309 PASS25.42s. The31
unimplemented SQL cases remain enabled and were excluded by their explicit label.
These passes did not establish complete source correctness: separate source review
found a real untested decoder-priority defect.

## Independently reviewed additive regression before correction

Consumer reference-application-adapters-v1 §2 and concrete sqlite-initial-captures-v1
§3 require complete outer bounds before unsupported-format refusal, then framing
and decoded semantics. Initial outer() instead reported invalid_reference for an
empty supplied format and invalid_shape for unknown-format+NULL body.

Different test author capture_fixture_author authored0deb60d/import850fd79,
CPP73d0256: one additional data-driven method/eight rows across all four decoders.
The old27 tests and literals are byte-identical after removing the additive block.
Different reviewer capture_reviewer approved actual tests and root28 harnessdbe8fa3
BEFORE root's behavioral red. Source author held c2 unchanged throughout.

At19:45:12Z same clean dbe8fa392af7e1be657e2fe84a8dd0e1b8320f4d:

```sh
ctest --preset sqlite-reference-capture-debug -R '^reference-capture-values.unsupportedFormatPrecedesDocumentSemantics$' --verbose --output-junit /tmp/storage019-priority-red.xml
```

Directexit8, one method/eight actual wrong-error failures: four actual3 versus7,
four actual4 versus7;2initcleanupPASS/0skip/setup/stub failures. [Raw](storage-019-priority-red-ctest.txt)
SHA56911fd705b32b7e0f7a7d8d94c086f82d13b06c8d5ec628ece1bc0db6de4afe.
Only after this observed behavior red was the bounded source reorder authorized.
Known-format NULL still requires existing invalid_shape; all outer limit priorities
and original assertions remain. Historical codec FIRST550 was never replayed.

Current registry368 =old308+foundation1+codec28+SQL31. Actual corrected-source
review,337 passing selected checks, ordinary selected28 CI and latest PR gates
remain pending. No source/helper SQL execution or live/provider action occurred.
