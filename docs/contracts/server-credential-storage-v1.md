# Server provider credential storage and provisioning, revision 1 draft

[SESSION-006 #270](https://github.com/Kernol12/subathon-bot-ai/issues/270), Tier A
Stage 1, [draft PR #729](https://github.com/Kernol12/subathon-bot-ai/pull/729).
[Specification v0.45 §16, CRYPTO-01 and AUTH-15](../specification.md#16-deployment-and-operations)
require server-held service secrets, XChaCha20-Poly1305, an external protected key
that permits unattended restart, and restoration material exclusively inside
encrypted backups. Installation-to-provider authorization remains with the owning
integration. [CONTRACTPLAN-001 #155](https://github.com/Kernol12/subathon-bot-ai/issues/155)
and [STORAGE-002 #312](https://github.com/Kernol12/subathon-bot-ai/issues/312) are
recorded done. The former SESSIONTEST-006 is Stage 1 of #270. This is the proposed
boundary among server credential consumers, the writer repository and Linux
provisioning. Backup/restore (STORAGE-005), rotation, provider and client flows,
and host startup and health publication have separate owners.

## Server operations

These are typed operations in `subathon::server`. Implementations can choose
concrete class layout. Borrowed inputs are synchronous and owner-scoped.

| Type or operation | Meaning |
| --- | --- |
| `CredentialScope` | Exact installation ref, `ConnectorProvider`, connector instance ref, a subject variant (`InstallationSubject` or `AccountSubject{account_ref}`), and secret slot ref. The caller supplies stable server-owned refs and selects the scope after its own authority checks. |
| `provision(PrivateKeyPath)` | Return an owned move-only `CredentialKey` handle or a typed failure. Only trusted startup code supplies the path. The key has no raw export from this boundary. |
| `load(scope)` | On the active `SqliteWorkSession` with a provisioned key, return an owned move-only `CredentialValue`, `missing`, or a typed failure. Plaintext is available through a synchronous byte view for trusted server consumers. |
| `stage_put(scope, bytes)` | Stage one exact-scope insert or replacement and return `staged` or a typed failure. On replacement, the selected prior record must authenticate before writing. |

Each ref is 1–4096 opaque bytes. A slot ref identifies the secret's purpose within
the scope and must remain stable as its value changes. Refs are nonsecret
identifiers. Values are 1–65536 opaque bytes, including NUL and non-UTF-8 bytes.
There is no trimming, coercion, case folding, parsing of provider values, or
default scope. [Connector lifecycle](../../server/include/subathon/server/connector_lifecycle.hpp)
owns provider identities and capability/status projection. A stored value
supplies neither capability nor authorization. Provider credential bytes stay
out of client projections and the separate
[account session](session-resolution-v1.md) digest/authority boundary.

Bind the repository to a [caller-owned writer session](writer-transactions-v1.md)
and live key handle. The caller checks current authorization and any freshness
or expected-version requirements in its serialized work. Writes affect exactly
one scope, and durable success requires confirmed COMMIT and normal work
completion. Preserve the writer's actual errors and outcomes, including unknown
outcomes. The repository neither commits nor retries. Plaintext stays in typed
server-owned storage, outside SQL bindings, generic writer completion values,
and public replies. No provider I/O runs in the writer unit of work.

## Exact bytes

`||` means concatenation. Integers in this format are unsigned, big-endian.
Each ref is encoded as a two-byte byte count followed by its exact bytes.
The canonical scope is

```text
ref(installation) || provider:u8 || ref(connector) ||
subject:u8 || [ref(account) if subject=02] || ref(slot)
```

Stable provider tags, independent of C++ enum ordinals, are Twitch `01`,
Betterplace `02`, Patreon `03`, Streamlabs `04`, StreamElements `05`, Ko-fi
`06`, and TipeeeStream `07`. Subject tags are installation `01` and account
`02`. The scope takes 11–16394 bytes. The logical table in `main` has a unique
canonical scope BLOB key and one non-null record BLOB. Indexed lookup is by the
entire scope key with exact BLOB equality. There is no fallback lookup or
cross-scope search. The physical migration and catalog position need separate
coordinator registration.

```text
header = ASCII "SCR1" || key_id[16] || nonce[24] || plaintext_length:be32
aad    = ASCII "subathon.provider-credential.v1" || 00 || header || scope
record = header || combined_ciphertext_and_tag
```

The four magic bytes identify format and version together. Header length is
48 bytes. The combined output is ciphertext of the stated plaintext length
followed by the 16-byte Poly1305 tag. Total record size is exactly
`64 + plaintext_length` bytes. Use the selected libsodium 1.0.22
`crypto_aead_xchacha20poly1305_ietf_*` combined-mode calls with a 32-byte key
and null secret nonce. The opener constructs AAD from the **requested** scope
and the exact selected header. It releases bytes only after successful
authentication and an exact returned length. [Libsodium AEAD reference](https://doc.libsodium.org/doc/secret-key_cryptography/aead/chacha20-poly1305/xchacha20-poly1305_construction).

Require ready libsodium before crypto. Draw a fresh 24-byte nonce with
`randombytes_buf` for every seal, including replacement or a new attempt
after known rollback. There is no ordinary entropy error return, deterministic
production nonce, caller-selected nonce, or fallback. Use the key material
only for credential encryption on its installation. The independent, nonsecret
16-byte key ID selects an exact provisioned key. It is never key material or
permission. The first profile has one provisioned key and tries only that ID.
AAD authenticates record placement, format and key ID. An older valid record
replayed at its original scope under the same key can still authenticate.

Preflight complete inputs and checked size arithmetic before owning copies or
crypto. On read, query selected BLOB classes and lengths before materializing
payloads. A record over 65600 bytes is a resource refusal at this stage. Fetch
only the bounded, admitted row, verify actual types, lengths and scope equality,
then validate the whole versioned record before key selection or decryption.
The advertised plaintext length must be within range and agree with the actual
record length. Preserve cumulative writer budgets. Protect owned secret buffers
using guarded, locked memory and erase plaintext, key and secret scratch on
every owning exit using the existing libsodium pattern. Failure to acquire
required protected memory fails closed. The input owner still owns erasure of
its original bytes.

## Linux provisioning and restart

The operator provides an absolute private file path outside the SQLite state
and ordinary configuration data. Accept up to 4095 path bytes, with no NUL,
empty, dot or dot-dot component. A secret mount may expose the same regular
file directly. At rest its complete contents are 52 bytes

```text
ASCII "SKP1" || key_id[16] || key[32]
```

Provision with an independently generated random key and random ID. Keep the
pair stable across ordinary restarts and reserve any key or ID change for the
separate rotation/recovery workflow. The service runs with its dedicated
non-root effective UID. Resolve from the root with descriptor-relative,
no-symlink traversal. Require each ancestor directory to be owned by root or
that UID and forbid group/other write access. Open the final path read-only
and close-on-exec, without following links or blocking on a FIFO. Inspect the
opened descriptor before reading. Require a regular file owned by the service
UID with mode exactly `0400` or `0600` and the expected size. Read at most
53 bytes and require exactly 52 and EOF, with the correct magic. Trust the
operator to publish complete files and keep opened key files immutable.
Use identical checks for direct secret mounts. Nothing requires a writable mount.

Acquire a fresh handle on startup from the configured file and close the
descriptor after loading. No interaction, SQLite key column, generated
replacement, environment key, embedded default or plaintext backup export.
If loading fails, credential operations remain unavailable. With the same
database and provisioned pair, a new process can recover the exact stored bytes.

## Failures and disclosure

| Outcome | Meaning |
| --- | --- |
| `invalid_input`, `resource_limit` | Input shape is invalid or a finite bound is exceeded. An oversized selected payload is refused before fetching it. |
| `missing` | Healthy, exact lookup proves absence with a ready key handle. |
| `key_unavailable` | File missing/unreadable, or a well-formed record selects an ID absent from the provisioned handle. |
| `key_file_rejected`, `invalid_key_file` | Path or descriptor fails protection checks, or file length/framing is invalid. |
| `invalid_record`, `authentication_failed` | Selected data has invalid SQL shape/framing (including unsupported version), or a well-formed record under the selected ID fails AEAD authentication. |
| `crypto_unavailable` | Crypto readiness, protected memory, or a native operation other than authentication cannot supply a valid result. |

A local validation, provisioning or authentication refusal returns no secret
and stages no write of its own. Writer errors and uncertain outcomes retain
their existing meaning. Diagnostic output
contains only operation and fixed failure category. No path, scope, key ID,
nonce, record dump, value or raw native/driver text appears in a diagnostic.

Independent synthetic examples and proposed suite registration accompany this
draft in the private Stage 1 handoff. Contract agreement precedes test authoring.
