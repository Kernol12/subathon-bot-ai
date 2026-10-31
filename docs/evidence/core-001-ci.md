# CORE-001 numeric CI and dependency evidence

Coordinator `/root`, 2026-10-31, branch `task/CORE-001-exact-numeric`.
Specification v0.45 NUM-04 and evolving-CI policy. CORETEST-001's reviewed/red
baseline integrated via [PR #12](https://github.com/Kernol12/subathon-bot-ai/pull/12)
at `cab329b`; its 32 assertions remain the automatic acceptance source.

## Selected dependency and workflow

Boost.Multiprecision is already selected by NUM-04, not a new library choice.
The numeric target discovers installed `Boost >=1.83` with upstream CONFIG mode,
links `Boost::headers` publicly for the public rational API, and performs no implicit
fetch or installation. Local1.92.0 is a development version, not a release pin. CI explicitly selects
the pinned1.83 Boost/config-header CMake directories and verifies that the compiler
preprocessor actually selects `BOOST_VERSION108300`, rather than assuming package
installation controls every installed Boost version. Configure logs selected Boost
version and include directories.

The new `.github/workflows/exact-numeric.yml` runs `exact-numeric` on every PR to
main and main push, with manual dispatch retained. Ubuntu24.04 installs exact
`libboost1.83-dev=1.83.0-2.1ubuntu3`; checkout and opt-in GoogleTest source references
retain existing immutable pins. Only contents:read permission and a15-minute job
budget are used. The job configures/builds the same exact-numeric-debug preset as
local checks and runs real reviewed cases without failure inversion or skipping.
Existing credit/scaffold workflows remain unchanged and are separate merge gates.

Read-only preparation verified the [official Noble amd64 package metadata](https://archive.ubuntu.com/ubuntu/dists/noble/main/binary-amd64/Packages.xz)
and [exact Boost archive](https://archive.ubuntu.com/ubuntu/pool/main/b/boost1.83/libboost1.83-dev_1.83.0-2.1ubuntu3_amd64.deb).
The archive's SHA256 is `747b364a7d578babed9efa4c40446be4ba3470ac5b2c9951b0e3bde3d7bb47d6`;
its only declared dependency is libstdc++-13-dev and it includes cpp_int.hpp,
BoostConfig.cmake and boost_headers-config.cmake. No local package was installed.
An explicit apt version pin uses the distribution's signed package metadata; CI
must fail visibly if that exact package ceases to resolve, without silent fallback.

## Observation and remaining configuration

Actual pull_request [run41183487457](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41183487457)
passed for PR13 head `fd86a6b9e8546bf5308a26e5561be0faf49e35fd` on2026-10-31.
Observed log: apt installed exact `1.83.0-2.1ubuntu3`; compiler preprocessor printed
`#define BOOST_VERSION 108300`; CMake selected Boost1.83.0 at /usr/include with
GCC13.3.0; all33 numeric cases passed. Existing credit/scaffold jobs also passed.
This establishes that CI dependency/build/test combination, not full Ubuntu
application deployment or Windows/release support. Final completion-record revision
CI must still be checked again before merge. The historical red output and reviewed assertions are
preserved; removal of the intentional-red CTest label after implementation is a
status update, not a skipped/changed test. Future server targets must link the
numeric library explicitly; this is a bounded primitive, not application integration.

Active ruleset26170263 currently requires PRs/resolved discussions and zero GitHub
approvals, with no enforced status-check rule. The coordinator requires all latest
checks anyway and makes no ruleset changes. Making exact-numeric/scaffold/acceptance
required is an explicit repository-settings enforcement gap; a new workflow does
not configure branch protection. Windows/release/compiler/platform coverage and Qt
CI remain separate gates; no live providers or credentials are used by this suite.
