# ACCESSTEST-002 independent selected-environment source review

Reviewer `/root/codec_author`, 2026-11-04, different from source author
`service_reviewer` and harness author `/root`. Clean isolated branch
`task/ACCESSTEST-002-environment-review` starts at actual candidate
`dab476e104f0e6831eae099e8878b453b96bc13c`, including source author `a9e44a1`
imported as `58b7f581`. Root/tests/core instructions and the actual author/harness
evidence were read. Only this review note changes.

**APPROVED for the actual standalone environment source and its build/preset/CI
harness.** No blocker found. Root may perform the separate FIRST selected native
environment run. No environment run, permission behavior, future API/source/test
approval, selected CI success or merge approval follows from this source review.

## Actual reviewed artifacts

| Artifact | Git blob |
| --- | --- |
| [Standalone primitive source](../../tests/acceptance/effective_permissions_environment.cpp) | `8d88fd0e7f81de2ba300648ff7066f7bff62c5a3` |
| [Source author evidence](accesstest-002-environment-author.md) | `8951ea9480e8ca5877339c9f1cf3e6af9c34dd63` |
| [Root harness evidence](accesstest-002-environment-harness.md) | `bcf090b3090c4351deb12e1caaf90db3d79b9843` |
| Root CMake | `51c1047b8e0c8dfb2147bb0c83823f65b625c23c` |
| Preset 26 / CMakePresets.json | `948f3b9bcdd9b869837dadfa99d4d41b45ce8080` |
| Test harness CMake | `6358bf9b63b982cf56d660a9b26ce14e6a0923cc` |
| [Workflow source](../../.github/workflows/effective-permissions.yml) | `1649ba21e839083425020ba51fa5bb6cefcc41bf` |

The source's 21 check sites cover signed64 minimum after widening, unsigned64
zero/maximum/2^64, literal negative/zero cross-products, gcd, reduced positive/
negative/zero SDK rationals and bounded long decimal powers. Expected small
fractions and decimal-power/bit counts are independent literals. The explicitly
labelled parse/format roundtrip does not masquerade as an independent value oracle.
Python stdlib independently confirmed 16 associated arithmetic facts, including
10^5000's 16610 bits and its square's 33220 bits. These SDK checks implement no
permission evaluator, project parser, policy normalizer or resource-budget oracle.

All value construction/arithmetic is inside main. Only standard/Boost headers are
included; no project, fixture, GTest, SDK-success fake or authored global initializer
is present. Ordinary failed checks return 1; exceptions return 2. No missing behavior
is caught as a pass or skipped. Static symbol inspection of the actual root binary
finds main and no project/GTest or `_GLOBAL__sub_I` definition. This is source/link
inspection, not an execution trace or claim about all standard-library internals.

## Dependency and harness findings

Inherited authenticated package: Ubuntu Noble amd64
`libboost1.83-dev=1.83.0-2.1ubuntu3`, SHA256
`747b364a7d578babed9efa4c40446be4ba3470ac5b2c9951b0e3bde3d7bb47d6`.
Read the existing different stage-2 and stage-3 reviews under
`/tmp/subathon-boost-1.83-sdk/`, which retain the preceding separate stage-1
authentication approval. No download/authentication/probe was replayed.
The author's type-only and compile/link evidence does not claim runtime proof.

Read actual root cache, Ninja compile/link rules and retained configure/build logs.
Both config directories resolve to the authentic isolated prefix, and the source
compile has `-isystem /tmp/subathon-boost-1.83-sdk/prefix/usr/include`. Read-only
Ninja dependency inspection independently finds 521 Boost headers, all inside that
prefix; every current hash matches the original authenticated material manifest.
The actual target links its one source object and standard toolchain libraries,
without project libraries or GTest. No post-build command runs it. Root binary
SHA256 is `c5100e74d34342e8afc764b8cd538d936ca79fe6874edf041a692e8a38009e9a`.
Its existence/hash establishes no passing runtime result.

The generic CMake lookup permits Boost >=1.83, while this probe's static assertion
requires exactly `BOOST_VERSION==108300`; selecting host 1.92 therefore cannot
silently certify this lane. Root's actual explicit configs and CI's pinned Ubuntu
package/configs supply 1.83.0. Default profiles remain opt-in unchanged. Preset 26
inherits the reviewed exact-numeric lane, so its 37 discovered cases are 33 existing
numeric cases, three old infrastructure checks and this one environment case.

The environment is an enabled ordinary 10-second check labelled
`scaffold;effective-permissions-environment`; retained discovery has no disable,
failure inversion, skip or success-regex property. CI pins checkout, installs the
selected package version, checks compiler macros, configures explicit authentic
config paths, builds, requires exactly one uninverted environment discovery and
executes it as a mandatory green step. The following step runs existing numeric/
infrastructure checks. No blanket error continuation exists. Future intentional
permission red is a separate explicit operation; no permission feature target or
test inventory exists yet. Those later additions require their own actual review.

## Actual checks and remaining gates

This reviewer ran only read-only source/artifact/log/dependency inspections,
`clang-format --dry-run --Werror`, `nm -C`, Python stdlib arithmetic/hash/inventory
checks, YAML parsing, `bash -n` on seven workflow bodies and syntax compilation of
two embedded Python blocks. The retained 37-case discovery was inspected without
calling CTest again. Source format, all header hashes, literal facts and workflow
syntax checks passed. No configure/build, binary/SDK arithmetic, foundation,
project/feature/marker/test body or workflow step was executed by this reviewer.

`python3 tools/check_project.py`, `python3 tools/progress.py render --check` and
`git diff --check` passed: 406 tasks/122 decisions, links/presets/current HTML and
whitespace. Sole-path staged inventory/whitespace was checked before committing.
The actual first native foundation remains NOT RUN at this review. It must pass
and have retained evidence before claiming selected host arithmetic capability.
Ubuntu CI, applicable publication/access/current PR checks/reviews/rules/discussions
and normal merge gates remain separate. The first root configure's resolved imported-
target scope error remains historical infrastructure evidence, not behavioral red.
No future permission API, source, fixture/test, intended feature red, production,
platform or release gate is approved. Root owns shared state/HTML/build/CI/pins/PR62.
