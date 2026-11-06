# Offline schema-package validation

`tools/schema_validate.py` validates bounded JSON packages containing JSON Schema
2020-12 or OpenAPI 3.1 documents and explicitly bound example instances. It checks
complete local reference closure, including unused schemas. Validation reads local
files; it does not fetch reference targets or validate a running HTTP server.

The developer installation profile is Linux x86-64 with an ordinary, non-debug,
GIL-enabled CPython 3.10, 3.12 or 3.14 in a dedicated stdlib venv. Other Python
ABIs, missing kernel facilities or an unavailable native artifact are refused.
This profile does not establish product-platform or provider compatibility.

## Install and build

Run from the repository root. Choose an installed admitted Python minor; this
example uses 3.12. Keep the venv separate from other project tools and preserve
its `bin/python` spelling when configuring or running the tool.

```sh
schema_env="$PWD/.venv"
python3.12 -m venv "$schema_env"
"$schema_env/bin/python" -I -m pip --isolated --disable-pip-version-check --no-cache-dir install \
  --index-url https://pypi.org/simple --require-hashes --only-binary=:all: \
  --requirement tools/schema_validation/requirements.lock
"$schema_env/bin/python" -I -m pip --isolated uninstall --yes setuptools
"$schema_env/bin/python" -I -m pip --isolated check
```

The uninstall removes optional venv-bootstrap `setuptools`; it is not one of the
20 admitted distributions. The environment permits those exact distributions plus
bootstrap pip. The [lock](../tools/schema_validation/requirements.lock) and
[wheel manifest](../tools/schema_validation/wheels.json) bind versions, archive
hashes and selected installed metadata. Do not install globally, upgrade individual
packages or reuse an environment containing unrelated distributions. Installation
may download wheels; package validation itself stays offline.

Build with CMake >=3.25, Ninja and the reviewed GNU-compatible C++20/ELF toolchain:

```sh
cmake --preset schema-transport-debug \
  -DPython3_EXECUTABLE="$schema_env/bin/python" \
  -DSUBATHON_FETCH_GOOGLETEST=ON
cmake --build --preset schema-transport-debug
```

`SUBATHON_FETCH_GOOGLETEST=ON` explicitly permits fetching the repository's pinned
GoogleTest 1.17 source when a suitable installed package is absent. For a prepared
GoogleTest >=1.17 installation, omit that option; the existing
`SUBATHON_GOOGLETEST_SOURCE_DIR` option can instead select reviewed local sources.
There is no automatic dependency download without that opt-in.

The production artifact must be at
`build/schema-transport-debug/lib/libsubathon_schema_transport_v1.so` in this same
checkout. Build success alone does not admit it: retain the source/configuration,
compiler/libc/architecture and whole-artifact SHA256 provenance, then complete the
ordered generated-code/static, no-child bootstrap, real-facility and acceptance
gates in the [native/public workflow](../.github/workflows/schema-native-foundation.yml).
Use its exact environment and ordering before calling a newly built artifact;
do not run CTest discovery ahead of the static/bootstrap gates. The separate
[library workflow](../.github/workflows/schema-library-foundation.yml) checks the
pinned standards-library foundations. Import and `--help` perform no build, native
load, dependency scan or package read.

## Validate a package

Create these three UTF-8 JSON files in one directory, for example `schema-package/`.

`manifest.json`:

```json
{"profile":"subathon.schema-package.v1","documents":[{"path":"s.json","kind":"jsonschema202012"}],"examples":[{"path":"v.json","schema_document":"s.json","schema_pointer":""}]}
```

`s.json`:

```json
{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"integer"}
```

`v.json`:

```json
1
```

After the build-admission gates above, run the public CLI in the same minimal
environment:

```sh
env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8 \
  "$schema_env/bin/python" -I -B tools/schema_validate.py --help
env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8 \
  "$schema_env/bin/python" -I -B tools/schema_validate.py \
  --manifest schema-package/manifest.json
```

Help prints `Usage: schema_validate.py --manifest PATH` and exits 0. The example
prints exactly `{"code":"ok"}` followed by a newline and exits 0. The only CLI
forms are `--help` and `--manifest PATH`; stdin, URLs, globbing, extra options and
`--manifest=PATH` are not accepted. The manifest path may be absolute or relative
to the current directory. Package paths resolve relative to its opened parent.

Each document entry declares kind `jsonschema202012` or `openapi31`; there may be
at most one OpenAPI document. Each example names a declared schema document and
an RFC 6901 JSON Pointer, with `""` selecting its root. Every declared example must
be associated with its actual schema. Inline OpenAPI/schema examples also need
explicit manifest bindings; see the [public package contract](contracts/schema-validation-tool-api-v1.md#3-exact-manifest-representation-and-limits).

Use regular files and simple relative package paths. Symlinks, parent traversal,
undeclared reference targets and remote retrieval are refused. The `$schema` URI
above selects the already installed trusted dialect; it is not a network request.
This profile admits explicit Draft 2020-12 and OpenAPI 3.1, not a latest-version
alias. Schema `format`, dynamic references/anchors and the other features listed
in the [context contract](contracts/schema-validation-context-v1.md) are explicit
profile refusals. Literal data containing `$ref` remains data.

## Results and bounds

Each normal non-help result is one compact JSON line `{"code":"VALUE"}`, with no
extra keys or stderr diagnostics. These are the complete 18 public enum values:

| Exit | `VALUE` |
| ---: | --- |
| 0 | `ok` |
| 64 | `usage_error` |
| 2 | `invalid_path`, `invalid_json`, `invalid_manifest`, `unsupported_dialect`, `unsupported_format`, `invalid_schema`, `unresolved_reference`, `reference_cycle`, `invalid_inventory`, `unsupported_feature`, `invalid_instance` |
| 3 | `io_unavailable`, `environment_unavailable`, `dependency_unavailable`, `validation_unavailable` |
| 4 | `resource_limit` |

Failures do not echo package contents, paths or backend exceptions. A result
describes this invocation's read snapshot; serialize package edits while it runs.
It is not a certificate for later file changes.

Fixed ceilings include a 64 KiB manifest, 32 documents, 256 example bindings,
1 MiB per document/example, 8 MiB total file bytes, JSON depth 64, and one worker
with a 10-second complete deadline and 512 MiB address-space ceiling. Exact
numeric, graph, token and inventory bounds are in the
[logical contract](contracts/schema-validation-tool-v1.md#3-exact-data-bounded-ownership-and-execution).
The [installation contract](contracts/schema-validation-installation-v1.md) and
[native contract](contracts/schema-validation-native-process-v1.md) define the
trusted environment and resource ownership.

The selected hosted lanes are CPython 3.10/Ubuntu 22.04 and 3.12/Ubuntu 24.04.
Consult the [recorded acceptance evidence](evidence/schematool-001-native-acceptance-pass.md)
and [latest native/public workflow results](https://github.com/Kernol12/subathon-bot-ai/actions/workflows/schema-native-foundation.yml)
for the actual source revision and environment verified. A result from an older
revision does not admit a newly built artifact.
