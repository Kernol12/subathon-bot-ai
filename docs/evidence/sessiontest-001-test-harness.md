# SESSIONTEST-001 full candidate harness, before different review

Coordinator /root, 2026-11-04. Actual full author8e6d1dd170f59710ebc989a0e5f85b16567446e6
import82d624de5798f4505a9407250039daa0520cfd00; root owns actual CMake/CI below.
No full-test approval, first feature red or production release is claimed.

| Artifact | Actual Git blob |
| --- | --- |
| `tests/fixtures/password_hashing_vectors.hpp` | `025d4de904c4c07a08cc192fc6cf94318bd5c0c8` |
| `tests/acceptance/password_hashing_tests.cpp` | `52f0e911cc0dbd17b7f6e847ac778c449598c8b7` |
| `tests/support/password_hashing_failures.hpp` | `831a8bb0a4536e52c677a00a70caed5841236394` |
| `tests/support/password_hashing_failures.cpp` | `2782be4171cddafd048efdab130a896a3616adb4` |
| `tests/CMakeLists.txt` | `9b10f68ce44ea5bcc0f279e5576e5c579613ae96` |
| `.github/workflows/password-hashing.yml` | `cb61b3ec028eddee805f7b6c9928d834789b54c7` |

The feature target links only subathon_password_hashing, the explicit real static
Sodium::Sodium and GTest::gtest_main. Its private test observer .cpp supplies exactly
12 GNU-compatible --wrap symbols: sodium_init/malloc/allocarray/mlock/memzero/munlock/
free, crypto_pwhash_str/str_alg/argon2id_str and the generic/Argon2id str_verify.
No operator-new interception or production crypto-provider seam exists. The root
native foundation/generator targets retain their exact unwrapped links and source.
Wrappers default to the real SDK, inject only failure/null or invalid output after
actual real success, and use trivial constinit thread-local bounded observations;
full correctness and oracle independence remain different review's responsibility.
No SDK or SUT call exists in fixture/global setup; source inspection preceded LIST.

CTest PRE_TEST discovery invokes only GTest LIST.37 ordinary TEST definitions map
one-to-one to37 exact enabled names, with labels password-hashing/acceptance/
intentional-red/server and60s timeout, no DISABLED/WILL_FAIL/failure inversion.
Foundation remains a mandatory separate green case; generator has no CTest and is
EXCLUDE_FROM_ALL. Discovery total41=37new+3existinginfrastructure+1foundation.

Selected Ubuntu CI keeps exact authenticated SDK source pin/static disposable
prefix recipe from [preparation](sessiontest-001-ci-preparation.md), adds default
feature-target compilation and strict37-source-vs-discovery/labels/enabled guard.
It requires real selected-SDK green foundation, then runs old3infrastructure only.
It NEVER runs the known-red password feature bodies or input generator. FIRST
plain feature command remains a separate explicitly recorded future operation:
`ctest --preset password-hashing-debug -L '^password-hashing$' --verbose --output-junit <ignored-output>`.
No negative test inversion/continue-on-error or acceptance skip hides a failure.
Changed workflow MUST receive actual different review and currentselected Ubuntu
execution before first feature red or baseline merge. Workflowaccess stillabsent;
oldPR59 has only scaffold41828240775 atf94f5ae, not component/native CI success.

Actual root commands in this worktree:

```sh
cmake --preset password-hashing-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-sodium-1.0.22-sdk/prefix
cmake --build --preset password-hashing-debug
ctest --preset password-hashing-debug --show-only=json-v1
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Configure and3 feature compile/link steps PASS/zero projectwarnings. Independent
stdlib source/discovery inventory found exact37enabled labels+old4, and the ACTUAL
workflow's embedded Python discovery script executed successfully against that
JSON/source. YAML parse/all7bashsyntax blocks passed. These were discovery-only:
no new feature body/marker/SDK call executed. Existing green host native proof from
clean c0646b4 is preserved, not rerun; [actualresults](sessiontest-001-native-results.md).
Frozen logical/API/header/immediate3throw CPP and nativeprograms remainunchanged.
Full differentreview, selectedCI/FIRSTred/attribution andproduction/codeaudit,
ordinaryallocationexception handling andfullsecretcleanup, account/currentlogin/
conditionalstore/admission/benchmark/platform/packaging gates remain.


Local execution ordering clarification: root explicitly adopted the ACTUALdifferent
reviewed [password sequencing](sessiontest-001-local-red-sequencing.md), approved
[environmentreview](sessiontest-001-environment-review.md)d0a3819b/importfb3a5e2.
It replaces only this evidence's earlier agent-added CI-before-FIRST-LOCAL-red order.
Actual R1/R2 corrected-fulltest approval and exact matching runtime-proven hostSDK
are still mandatory before that local operation; currenttests remainchangesrequired.
Remote currentselected Ubuntu CI/access/publication remains required beforebaseline
integration/merge; all productionprerequisites remain. No remoteCI/testred isclaimed
by this clarification and no workflow/source/assertion/fixture ischanged.
