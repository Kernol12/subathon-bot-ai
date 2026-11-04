# SESSIONTEST-001 logical password-contract author handoff

Author service_reviewer, 2026-11-04. Dedicated
task/SESSIONTEST-001-contract-author worktree sessiontest001-author, from refreshed
main dd6a141 and coordinator claim a7b0d4f. This role is AUTHOR, not independent
reviewer of its own new document. Root owns state/HTML/maps/build/CI/dependency
pins/schemas/checkpoint and publication. Only these two new files are authored:

- [Logical contract](../contracts/password-hashing-v1.md).
- This evidence note.

Read root/server/tests instructions, SESSION-001 brief, specification v0.45
section 10/AUTH-11/S62, architecture/workflow/test strategy and reviewed record/
application ownership boundaries. The wire decomposition has removed an unrelated
coarse dependency; it neither reviews this new password contract nor waives any
test-first or independent-review gate.

## Actual investigation and proposals

Read installed headers and metadata only:

```sh
pkg-config --modversion libsodium
pkg-config --cflags --libs libsodium
```

Observed version 1.0.22 and link flag -lsodium. Inspected
/usr/include/sodium/version.h, crypto_pwhash.h and crypto_pwhash_argon2id.h:
explicit Argon2id13 ID 2, password minimum 0, 16-byte salt, 128-byte string buffer,
library cost bounds and explicit-length encoded-string APIs are declared. No
library symbol is linked/called; no sodium_init/version/hash/verify/needs-rehash,
random generator, memory-lock or native foundation is executed. Installed header/
pkg-config identity is not a runtime or selected-artifact verification.

Browsed official primary documentation and pinned-release source on 2026-11-04:

| Primary source | Used for |
| --- | --- |
| [Password storage API, specification S62](https://doc.libsodium.org/password_hashing/default_phf) | Self-contained stored salt/parameters, explicit lengths, success/error returns and need for actual cost measurement |
| [Password hashing overview](https://doc.libsodium.org/password_hashing) | Distinguishing storage hashing from other constructions; no client-side server-relief protocol adopted |
| [Initialization](https://doc.libsodium.org/usage) | Readiness return values and initialization-before-use boundary |
| [Secure memory](https://doc.libsodium.org/memory_management) | Explicit clearing/locking, guarded allocation limitations and no global secret-erasure guarantee |
| [1.0.22 public header](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/include/sodium/crypto_pwhash_argon2id.h) | Declared algorithm, buffer/salt constants and supported entrypoints |
| [1.0.22 Argon2id source](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/crypto_pwhash/argon2/pwhash_argon2id.c) | Creation profile p=1, digest length, internal salt generation and KiB conversion |
| [1.0.22 record encoding](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/crypto_pwhash/argon2/argon2-encoding.c) | Whole-record/version parsing, decimal and original unpadded Base64 representation |
| [1.0.22 dispatch](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/crypto_pwhash/crypto_pwhash.c) | Explicit algorithm selection and distinction from generic/default-algorithm behavior |
| [1.0.22 synthetic tests](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/test/default/pwhash_argon2id.c), [published expected output](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/test/default/pwhash_argon2id.exp) | Independent complete H0 empty-password verifier and parameter-mismatch examples, not author execution |

The contract proposes explicit Argon2id13/v19, libsodium 1.0.22 for later selected
pinning, 0..1024 raw password bytes, bounded canonical encoded record parsing and
separate stored/new cost profiles. Initial desired cost t=3/64 MiB, maxima t=10/
256 MiB and the online benchmark budget are engineering proposals, not measured
hardware/security claims. Root has not pinned a package/archive here. No weaker
client prehash, alternate algorithm, pepper service or new product password minimum
is introduced.

Important evidence-driven details:

- The verifier takes its cost from the stored record. Preflight record length,
  encoding, version/parallelism, cost and fixed salt/digest bounds BEFORE any native
  verification or needs-rehash call. Do not let a hostile stored m/t choose an
  unbounded allocation or wrap during KiB conversion.
- Native verification -1 does not prove every failure is a wrong password. The
  document preserves unsuccessful verification without fabricated reason, with
  separate preflight/unavailable outcomes and no upgrade/authentication on error.
- Parameter mismatch alone is not permission to reduce an already higher cost.
  The proposed upgrade target uses componentwise maxima after actual login success,
  retaining the old record on failure and requiring caller-owned conditional
  replacement against the verified current credential. No boolean/ref becomes a
  reusable account-authentication certificate.
- Password bytes, NUL, spaces and Unicode spelling remain exact. Password strength/
  minimums, actual account/session checks, persistence and admission remain their
  explicit owning boundaries; those obligations are not claimed implemented.

The small stdlib-only metadata check of published H0 verified 96 ASCII bytes,
canonical 22/43-character Base64 fields decoding to 16/32 bytes, and the literal
KiB arithmetic 4882*1024=4999168, 65536*1024=67108864 and
262144*1024=268435456. It performs Base64 parsing/integer arithmetic only, not
password hashing, verification or a new project feature. H0's published verification
expectation is attributed to upstream, not inferred from this metadata check.

PH01–18 are proposed independent literal byte/profile/failure/upgrade examples.
They preserve unsuccessful-login/no-upgrade, no normalization/truncation, bounded
resource refusal, non-downgrading costs, stale credential replacement and secret
lifecycle distinctions. The benchmark is a concrete later measurement procedure
with explicit environment/admission/sample statistics and no fabricated result.
No additional decomposition is necessary for this logical document; actual worker,
account/store and platform integration remains mandatory adjacent work rather than
an unimplemented framework added to this slice.

## Checks and remaining gates

Actual documentation checks all exit 0:

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Project consistency passes 401 task records, 122 decisions, local links/presets and
current HTML. Progress render is current and whitespace checks pass. Logical
contract Git blob is bc9ccbf66a41c9ae69abde4bd905401900e17461. Final staged-path
checks confirm only the two owned Markdown files; no shared state is overwritten.

No C++ API/header, schema, source/stub, dependency installation/build, fixture,
test/marker/native-crypto invocation, real secret/token, provider operation, push,
PR or merge is supplied by this stage. Different actual logical review is next.
Concrete API/secret-ownership review, minimal source/harness review, independent
tests and their review, selected real-library foundation, FIRST valid feature red,
production implementation/source review and relevant CI are still required.
SESSIONTEST-001 is incomplete and SESSION-001 remains planned. No measured-cost,
supported-hardware/platform, account-login or cryptographic capability is claimed.
