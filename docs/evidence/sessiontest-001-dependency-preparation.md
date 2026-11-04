# SESSIONTEST-001 source provenance and remaining SDK gates

Coordinator /root and codec_author,2026-11-04. This is dependency preparation under
the active password baseline, not password API/source/foundation acceptance.
Actual logicalbc9ccbf6 and differentreview98f0494 remain unchanged. Proposed selected
libsodium1.0.22/default t3/64MiB is not a measured hardware or supported-runtime claim.

## Actual authenticated artifact

The official immutable candidate is
[libsodium-1.0.22.tar.gz](https://download.libsodium.org/libsodium/releases/libsodium-1.0.22.tar.gz),
2008529bytes, actualSHA256
`adbdd8f16149e81ac6078a03aca6fc03b592b89ef7b5ed83841c086191be3349`.
Downloaded archive and both signatures match
[official release asset metadata](https://github.com/jedisct1/libsodium/releases/expanded_assets/1.0.22-RELEASE).
The corrected tag currently resolves commit77e1ce5d6dee871c49ef211222ba18ef0c486bda;
exact authenticated bytes are authoritative here, not an assumed movable tag.
[Upstream tag explanation](https://github.com/jedisct1/libsodium/discussions/1533).

Minisign was absent(exit127). Root checked the
[official distribution instructions](https://download.libsodium.org/libsodium/releases/README.html)
and authorized the existing GnuPG alternative with the exact RSA public-key block
published by [upstream installation/Integrity checking](https://doc.libsodium.org/installation.md).
Only an isolated scratch homedir and explicit dearmored keyring were used; no
user/global keyring changes, keyservers, trust-always flags or security bypass.

At2026-11-04T12:41:07Z, installed gpgv2.4.9 exited0 with GOODSIG and VALIDSIG.
Official primary fingerprint:54A2B8892CC3D6A597B92B6C210627AABA709FE1.
Signing fingerprint:0C7983A8FD9A104C623172CB62F25B592B6F76DA.
Signature timestamp2026-04-09T22:46:33Z; no expiry/mismatch/missing-key status.
The official key block is8086bytes/SHA256
`ddd820fc4097db9d7aed729643e6c3135e20643110dd1e2ace7c42c04358f825`.

[Observed receipt](sessiontest-001-source-authentication.json) records exact argv,
UTC/result/status summary and artifact hashes. Full raw commands/status/audit note
remain in /tmp/subathon-sodium-1.0.22-source-audit. Root independently re-read the
raw result and re-hashed archive/signature/key; all sizes/hashes matched. Source
authentication gap is resolved. No source extraction, installation, compilation,
linking or password/library init/hash/verification/RNG/foundation ran.

## Environment, ownership and later evidence

Host metadata shows Arch1.0.22-1, header1.0.22/ABI26/4 and libsodium.so.26.4.0.
Metadata is not runtime identity. [Ubuntu24.04's default libsodium-dev](https://packages.ubuntu.com/noble/libsodium-dev) is the1.0.18 line;
it cannot stand in for this proposed exact dependency. Before native foundation,
root must select/pin actual source/build inputs and explicit include/library paths,
compile/link the intended SDK and independently review the actual standalone
foundation. Standard Autotools configure can execute compiler/link probes; do not
label it strictly compile-only or fabricate cache answers. No makecheck or KDF
execution belongs to compile/discovery. [Upstream installation guidance](https://doc.libsodium.org/installation.md).

Use no march-native/LTO/sanitizer shortcut contrary to upstream requirements.
Keep CPU implementations and checked runtime identity explicit. Header constants
prove neither password cost nor secure-memory locking. Caller admission, protected
password lifetime and real memory-lock results still need their owning tests.
Only a separately reviewed foundation may establish actual selected header/runtime
agreement, initialization/vectors/memory behavior; actual reference-hardware
benchmark remains separate. [Secure memory documentation](https://doc.libsodium.org/memory_management.md).

Preserve [upstream ISC license](https://raw.githubusercontent.com/jedisct1/libsodium/77e1ce5d6dee871c49ef211222ba18ef0c486bda/LICENSE)
and bundled notices in later dependency/package manifests. No repository dependency
pin or supported platform is created by this note. Full API/minimal-source/test/
firstred/implementation/code-review/CI and hardware gates remain. Default data is
synthetic; no real password, supporter or credential was used.
