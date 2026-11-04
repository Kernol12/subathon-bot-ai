# SESSIONTEST-001 minimal source/SDK import harness

Coordinator /root,2026-11-04. Logicalbc9ccbf6/API7ad1e8d3 are independently approved;
actual APIreview18e1a109/import503a7d3. Source author d34f2d000ad74c13cd07cd54bd8923bd5b69d047,
importc7d4c45 adds only header ec3c5cf87afe406db8501053321598599325a239,
CPP eaf56f41bafd502b8e4cb38c0de3082180bffcd5 and authornote1465014f0a85bfb880b53b69f037d7bf1cae0a5e.
All THREE featurebodies are single immediate PasswordHashingNotImplemented throws;
no validation/sourcecall/secret/protection/crypto operation before them.
[Author compile-only evidence](sessiontest-001-stub-author.md) records61 assertions.

Root adds default-off password-hashing option, explicit framework-free server static
target and preset25 password-hashing-debug, with normal GoogleTest discovery setup.
No feature test/foundation exists yet; no successful algorithm is supplied. The
SDK importer cmake/SodiumDependency.cmake requires an explicit Linuxx86-64 prefix,
reads exact1.0.22/ABI26/4 header metadata and globally imports only that prefix's
static archive plus Threads. Public password header is std-only; Sodium is private.
Configure never downloads or invokes a libsodium function. Trusted local prefix/
header metadata is not an authentication or runtime certificate; source provenance
is recorded separately and future CI must build from exact authenticated bytes.
No machine path is committed in the preset; the temporary prefix is a CLI input.

## Actual host SDK phase

[Source authentication](sessiontest-001-dependency-preparation.md) is complete.
Actual source/header/pkgconfig identify1.0.22/ABI26/4. SDK recipe:

```sh
/usr/bin/bash /tmp/subathon-sodium-1.0.22-sdk/source/libsodium-1.0.22/configure --prefix=/tmp/subathon-sodium-1.0.22-sdk/prefix --libdir=/tmp/subathon-sodium-1.0.22-sdk/prefix/lib --disable-shared --enable-static
/usr/bin/make -j2
/usr/bin/make -j2 install
```

Out-of-tree build uses Arch/Linuxx86-64/GCC16.2.1/glibc2.44, CFLAGS=-O2, portable
upstream CPU implementations and no native-opt/LTO/sanitizer/security-disable/cache
answer. Exits0/0/0;5.872/7.675/0.214s. All678 vendorfiles remain unchanged.
Configure ran THREE native compilerprobes: SIGABRT probe exited134 with core-dump
report and upstream cannot-catch warning. External systemd retention is unknown;
future runs must set child core limits and assess host handling. This is not
strictlycompile-only and not a productred. Eight Libtool convenience warnings,
no compiler error. Install used only prefix-local ldconfig-n, no sudo/globalinstall.

[Observed SDK receipt](sessiontest-001-sdk-build.json) records hashes/results;
full raw evidence remains /tmp/subathon-sodium-1.0.22-sdk/SDK_ENVIRONMENT.md and audit/.
Static archive836744bytes/SHA959cdbe83cd0c80264e080efdd1e861b20b57ded2a85bd9a924297b8a10f1935;
header/.pc metadata and19 defined symbolnames were inspected without calling them.
No makecheck/library test/sample/applicationprobe/runtime-version/initialization/
password/hash/verify/RNG/secure-memory API ran. Hostbuild is not Ubuntu/currentCI,
runtime/ABI/CPU/locking/cost/reproducible-binary/platform or password acceptance.

## Actual project interface checks

- `cmake --preset password-hashing-debug -DSUBATHON_SODIUM_ROOT=/tmp/subathon-sodium-1.0.22-sdk/prefix`:
  PASS, explicitSDKmetadata and installedGTest1.18.0; no downloads or password APIs.
- `cmake --build --preset password-hashing-debug`: PASS,8steps/no project warnings.
  The vendor's eight separate Libtool warnings above are not relabeled as clean.
- `ctest --preset password-hashing-debug --show-only=json-v1`: PASS,3 old infrastructure
  cases only; no password/foundation case or feature body exists in discovery.
- `ctest --preset password-hashing-debug`: PASS3/3/0.63s. Infrastructure only.
- Project consistency401tasks/122decisions, currentHTML and whitespace PASS.

Actual different source/harness review is next. Native selected-library foundation,
actual protected-memory/emptyinput/H0vector proof, independently reviewed tests and
FIRST plain modulemarkerred remain later gates. No password body/marker/constructor
runtime was executed. Explicit newSDK build/compile/foundation CI is required before
baseline merge; old PR59 component CI does not cover this new option/target. Adding
new workflows still needs the pending actual GitHub workflow access; no bypass.
Runtime and measured reference-hardware/platform gates remain separate.
