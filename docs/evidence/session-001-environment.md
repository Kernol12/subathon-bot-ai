# SESSION-001 current selected SDK restoration

Coordinator /root,2026-11-05. The old disposable SDK prefix was absent. Downloaded
exact official libsodium1.0.22 source whose GPG authentication was already actually
reviewed and recorded in sessiontest-001-source-authentication.json; no new GPG
verification or new FIRST authentication/runtime/generator claim is invented.
Current download size2008529/SHA256
adbdd8f16149e81ac6078a03aca6fc03b592b89ef7b5ed83841c086191be3349
matches that authenticated pin. Extracted under /tmp only with Python data filter.

Actual current commands, all exit0:

```sh
curl --fail --location --retry 3 https://download.libsodium.org/libsodium/releases/libsodium-1.0.22.tar.gz --output /tmp/subathon-sodium-1.0.22-sdk/libsodium-1.0.22.tar.gz
# In /tmp/subathon-sodium-1.0.22-sdk/build, RLIMIT_CORE=0:
CC=gcc CFLAGS=-O2 /usr/bin/bash /tmp/subathon-sodium-1.0.22-sdk/source/libsodium-1.0.22/configure --prefix=/tmp/subathon-sodium-1.0.22-sdk/prefix --libdir=/tmp/subathon-sodium-1.0.22-sdk/prefix/lib --disable-shared --enable-static
make -j2
make -j2 install
```

Source/header metadata1.0.22 ABI26/4. Root rechecked all three actual artifact
size/hash entries against the preserved original SDK-build JSON: static archive
836744bytes/SHA959cdbe83cd0c80264e080efdd1e861b20b57ded2a85bd9a924297b8a10f1935,
version.h and libsodium.pc ALL MATCH prior reviewed artifacts exactly. Configure
performs native compiler probes; eight known Libtool convenience-version warnings
occur in make, no build error. Raw configure/make/install logs remain alongside
this disposable SDK. No global install or vendor test suite, input generator,
project feature, SDK foundation or benchmark was invoked by restoration itself.
Current implementation/full acceptance remains separate. Original FIRST foundation,
PH12/13 input generation, tests/review/red records are reused, not repeated.
