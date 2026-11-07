# Repository schema checks

The [current contract](contracts/schema-validation-tool-v1.md) requires real
standards validation of the source-controlled OpenAPI document, every declared
example, independent valid/invalid cases for all 17 current component schemas,
and the separate progress editor schema. Scene/WebSocket schemas remain planned.
These checks establish artifact shape, references and examples. Actual test/source
reviews and selected-host CI remain separate from runtime or release acceptance.

The former arbitrary-package `validate_package`/CLI and native worker were retired
under the separately reviewed [cleanup decision](decisions/0012-test-suite-simplification.md).
No replacement generic API, 18-code protocol, hostile-file/resource bounds, native
artifact or process lifetime guarantee is offered. The complete previous setup and
observed proof remain in the
[archive](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/schema-validation.md).
Schema checks do not establish authentication, current authority, raw HTTP ingress,
provider support, product platform compatibility or release readiness.

## Isolated installation

Reuse the exact 20 admitted dependencies in the
[hash lock](../tools/schema_validation/requirements.lock) and
[artifact/license manifest](../tools/schema_validation/wheels.json). Install in a
separate stdlib venv, never globally or into server/client runtime dependencies.
Choose an admitted Linux x86-64 CPython 3.10, 3.12 or 3.14; selected hosted lanes
use 3.10 and 3.12. Preserve the venv's `bin/python` spelling.

```sh
schema_env="$PWD/.venv"
python3.12 -m venv "$schema_env"
"$schema_env/bin/python" -I -m pip --isolated --disable-pip-version-check --no-cache-dir install \
  --index-url https://pypi.org/simple --require-hashes --only-binary=:all: \
  --requirement tools/schema_validation/requirements.lock
"$schema_env/bin/python" -I -m pip --isolated uninstall --yes setuptools
"$schema_env/bin/python" -I -m pip --isolated check
```

Optional bootstrap setuptools is not an admitted validator dependency. Do not upgrade
individual packages or reuse unrelated environments. Installation may download wheels;
validation resolves only preloaded local resources and never installs or fetches a
reference target. The dependency-only metadata bootstrap checks the admitted interpreter
and one SDK root before candidate imports under `-I -S -B`; normal site/.pth processing
is not used for validator startup. This describes a trusted maintained installation,
not protection against an administrator replacing its code.

## Required check invocation

After preparing the isolated environment, run the reviewed suite:

```sh
cmake --preset schema-validation-debug -DPython3_EXECUTABLE="$schema_env/bin/python"
cmake --build --preset schema-validation-debug
ctest --preset schema-validation-debug -L '^schema-validation$' --no-tests=error --verbose
```

No native DSO/compiler-assembly/kernel probe is part of schema validation. CMake runs
ordinary infrastructure and the explicitly registered Python suite. Missing metadata,
library or validator support is a failing check, never a skip or valid schema result.
Expected invalid instances must be rejected by the real standards libraries. Do not
call those expected rejections an unimplemented-feature red or regenerate their
expected outcomes from validator output.

The suite preserves all 23 existing positive HTTP examples. Unknown offered version/
capability identifiers are valid shapes; negotiation support is a later runtime decision.
A literal matrix supplies valid/invalid examples for every actual component target.
Inventory checks reject missing/unclassified schemas, targets and example files.
Whole-document in-memory resolution closes current local references, including unused
components. The progress schema checks structural editor hints only: runtime task DAG,
review and combined-note checks remain in `tools/progress.py`. No FormatChecker is
supplied, so format is annotation-only there; the current public API has no format.

Run the relevant tooling/project checks and inspect actual latest-head CI/current
repository rules before normal merge. Detailed commands, versions, outcomes, independent
reviews and the precise revised baseline belong in the focused PR, not new receipt files.
