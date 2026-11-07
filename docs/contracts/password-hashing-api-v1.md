# Password hashing C++ boundary — proposal v1

SESSIONTEST-001 concrete API document stage, from release claim `f94f5ae`.
This refines the independently approved [logical contract](password-hashing-v1.md),
blob `bc9ccbf66a41c9ae69abde4bd905401900e17461`, without changing PH01–18 or the
AUTH-11/§10 responsibility split. [Logical review](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/sessiontest-001-contract-review.md)
does not approve this new API. Different actual review is required before any
header/source/stub/harness/test, followed by reviewed tests and FIRST behavioral
red before production SESSION-001. No native function is invoked by this document.

## 1. Three raw entrypoints and the trust boundary

Future header: `server/include/subathon/server/password_hashing.hpp`, namespace
`subathon::server`, C++20 standard-library declarations. Libsodium is private to the
future implementation; no sodium, Qt, SQL, provider or UI type enters the header.
Its standard headers are `<cstddef>`, `<cstdint>`, `<optional>`, `<span>`,
`<stdexcept>`, `<string>` and `<string_view>`.

```cpp
using PasswordBytes = std::span<const std::byte>;

struct PasswordHashingOptions {
    std::uint32_t desired_operations{3};
    std::uint64_t desired_memory_bytes{67108864};
    std::uint64_t admitted_memory_bytes{268435456};
};

[[nodiscard]] std::string create_password_hash(
    PasswordBytes password, const PasswordHashingOptions &options = {});
[[nodiscard]] PasswordVerification verify_password(
    PasswordBytes password, std::string_view encoded_verifier,
    const PasswordHashingOptions &options = {});
[[nodiscard]] PasswordUpgradeResult propose_password_upgrade(
    PasswordBytes password, std::string_view account_ref, PasswordLoginSource &source,
    const PasswordHashingOptions &options = {});
```

These are the only feature entrypoints. Each accepts literal raw data, and upgrade
can use a server-owned source port whose initial state is a literal H0 credential.
No preceding SUT hash/factory/verification/certificate is needed to arrange a test
call. Every minimal future body immediately throws its own module marker before
validation, source calls, initialization or crypto.

Creation returns an owned encoded verifier only. Verification accepts raw encoded
bytes and returns an observation, not an account authentication capability. Upgrade
does not accept that observation or a caller-created verified/login boolean. It
reads the actual credential through its source port and performs native verification
again, then checks the actual accepted-login/current-source state before proposing
replacement. This deliberate re-verification keeps raw direct setup and avoids
reusable proof objects. Benchmarks must include its additional work.

The port is supplied by the actual trusted account/session service and bound to
one actual login attempt. HTTP/CLI clients cannot supply or implement it. The port
contract requires real retained login and current-credential checks, not a wrapper
around caller-supplied flags/opaque references. Test ports are explicitly simulated
account state, never alternate successful crypto implementations. The password
module has no write port, authenticates no account and issues no session.

## 2. Exact values, source interface and owned output

All structs are owned aggregates with the declared field order/defaults; unspecified
strings default empty and optionals absent. account_ref is an internal opaque
account identity, not a username/provider name. Account and revision references are
nonempty exact byte strings, each at most 256 bytes, with no case/Unicode normalization.
Embedded NUL and non-ASCII reference bytes remain legal; these are not text labels.
They prove no permission, currentness or actual credential ownership by themselves.

```cpp
enum class PasswordVerification { verified, not_successful };
struct PasswordCredential {
    std::string account_ref;
    std::string revision_ref;
    std::string encoded_verifier;
};
enum class PasswordCredentialState { present, missing, unavailable };
struct PasswordCredentialRead {
    PasswordCredentialState state{PasswordCredentialState::unavailable};
    std::optional<PasswordCredential> credential;
    std::string login_attempt_ref;
};
enum class PasswordLoginCheck { accepted, denied, stale, unavailable };

class PasswordLoginSource {
  public:
    PasswordLoginSource(const PasswordLoginSource &) = delete;
    PasswordLoginSource(PasswordLoginSource &&) = delete;
    PasswordLoginSource &operator=(const PasswordLoginSource &) = delete;
    PasswordLoginSource &operator=(PasswordLoginSource &&) = delete;
    virtual ~PasswordLoginSource() = default;
    virtual PasswordCredentialRead read_current(std::string_view account_ref) = 0;
    virtual PasswordLoginCheck check_current_successful_login(
        const PasswordVerifiedSource &verified) = 0;

  protected:
    PasswordLoginSource() = default;
};

struct PasswordReplacement {
    std::string account_ref;
    std::string login_attempt_ref;
    std::string expected_revision_ref;
    std::string expected_encoded_verifier;
    std::string replacement_encoded_verifier;
};
enum class PasswordUpgradeOutcome {
    credential_missing, verification_not_successful, login_not_accepted,
    stale_source, not_needed, replacement_ready, upgrade_unavailable
};
struct PasswordUpgradeData {
    PasswordUpgradeOutcome outcome{PasswordUpgradeOutcome::credential_missing};
    std::optional<PasswordReplacement> replacement;
    std::optional<PasswordHashingError> unavailable_reason;
};
```

Forward declarations/order in the later header may resolve the class/enum references
above; no additional public fields or entrypoints are implied. PasswordLoginSource
is noncopyable/nonmovable, with a protected default constructor. Its implementing
service keeps it alive and at the same address through a call. It serializes its
bound attempt and bridges actual account reads/checks through the authoritative
owner; the crypto worker never becomes a second writer or performs crypto on that
writer thread. There is no hidden callback executor or login implementation here.
Port arguments are borrowed only until their method returns. A queue bridge copies
the bounded credential/attempt metadata it needs into an owned command; it never
queues a borrowed proof/view or asks the writer to hash a password. Verifier
metadata remains private account material even though it contains no plaintext.

`read_current` returns present with exactly one full owned credential and the
nonempty opaque reference of this source's actual bound login attempt, or missing/
unavailable with neither a credential nor an attempt reference. Attempt references
are bounded to 256 bytes. A present account must equal the requested account byte-
for-byte; its actual revision/encoded bytes are copied unchanged from authoritative
state, not reconstructed from current policy. Wrong account, inconsistent optional/
state combinations and enum values outside the declared set are source_invalid.
The source producer must itself bound database reads; the helper cannot retroactively
bound allocation already performed inside a port.

`PasswordVerifiedSource final` is an ephemeral noncopyable/nonmovable borrow. It has
NO public/default/data-taking constructor. Only propose_password_upgrade is its
friend and may construct it, after actual native verification returned 0. Public
accessors are exactly:

```cpp
[[nodiscard]] const PasswordCredential &credential() const noexcept;
[[nodiscard]] std::string_view login_attempt_ref() const noexcept;
[[nodiscard]] bool belongs_to(const PasswordLoginSource &source) const noexcept;
```

It privately refers to this call's owned immutable credential snapshot and exact
source object. belongs_to compares source-object identity, not reference text.
It contains no password, password hash shortcut or login-success flag. A port may
borrow it only during the check method and must not retain/copy its address or
credential borrow beyond that method. There is no test-only proof constructor.

The class has a public defaulted destructor and explicitly deleted copy/move
constructors/assignments. Its private constructor is
`PasswordVerifiedSource(const PasswordCredential &, std::string_view, const PasswordLoginSource &) noexcept`;
its only data members, in order, are `const PasswordCredential *credential_`,
`std::string_view login_attempt_ref_` and `const PasswordLoginSource *source_`.
They refer to this call's owned credential/attempt and actual port. Only
propose_password_upgrade is a friend. These are inert borrow mechanics, not a
public verification factory or a reusable ticket.

The check must verify belongs_to(*this), exact actual bound account/attempt reference, a genuinely
accepted local-password login based on this same full source credential/revision,
all other required login/account checks, and the actual STILL-CURRENT credential
revision/record. It is a read-only observation of the service's accepted login,
not an instruction to accept/issue a login. accepted is returned only when all
hold. denied means login/account checks do not permit upgrading; stale means the
bound source is no longer current; unavailable makes no successful observation.
The helper trusts this owned dependency's contract, not untrusted client flags.
The check returns a current decision; it does not wait for this helper to finish
the login, re-enter password operations or retain a pending proof for later use.

`PasswordUpgradeResult final` owns exactly one PasswordUpgradeData, exposes
`const PasswordUpgradeData &data() const noexcept`, and has public defaulted copy/
move construction and assignment. Its data-taking constructor is private to
propose_password_upgrade and is exactly
`explicit PasswordUpgradeResult(PasswordUpgradeData data)`; its only member is
`PasswordUpgradeData data_`. It has a public defaulted destructor, no default/public
data constructor and no mutable accessor. A data() borrow lasts until owner
destruction/assignment/move. Explicit
caller copies allocate outside a function call. The raw aggregates and even this
result are not permission or a durable/currentness certificate for a later write.

Only replacement_ready has replacement and no reason. Only upgrade_unavailable has
unavailable_reason, from resource_limit, hash_unavailable, native_output_invalid or
allocation_unavailable, and no replacement. Other outcomes have neither optional.
Replacement owns exact account/login attempt/source revision/source verifier plus
newly produced verifier. It retains no password, borrowed source or additional
editable parameter set. Encoded records remain sensitive account material, not
logs/public payloads.

## 3. Options, raw input and exact preflight priority

All operations check the following options before any other work, including fields
unused by standalone verification:

1. desired_operations is 2..10 inclusive.
2. desired_memory_bytes is 67108864..268435456 inclusive and divisible by 1024.
3. admitted_memory_bytes is 0..268435456 inclusive and divisible by 1024.

Other values are invalid_options. Zero admission is an explicit zero budget, never
an unlimited/disabled check. This scalar is only a caller-declared native Argon
memory ceiling; it is NOT an acquired work permit. The actual caller must admit
and bound its workers/password buffers before retaining/enqueuing them, initially
one active expensive call, with memory/worker/library overhead and other application
RAM budgeted separately. No global queue/concurrency controller exists in this API.
The t=3/64MiB default is an unmeasured application proposal, not libsodium's
INTERACTIVE (t=2/64MiB) or MODERATE (t=3/256MiB) profile. The
[tagged constants](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/include/sodium/crypto_pwhash_argon2id.h)
establish that distinction, not a supported-hardware benchmark.

Input spans/views must address valid stable memory until synchronous return. Their
prior construction/conversion/ingress copies are caller responsibilities. Password
length 0..1024 is accepted without strength/minimum/normalization/trimming changes;
1025+ fails resource_limit before copying, a port call or native work. A null empty
span is valid C++ input; a null nonempty span is an invalid caller memory range,
not something native verification can safely repair.

Standalone verify then checks record length <=127 before further inspection; empty
is invalid_record. Upgrade checks account bytes <=256, then nonempty, before
read_current. Invalid account input gives invalid_account_reference; excess length
gives resource_limit. Call read_current once. Check enum/state/credential-optional
consistency first; missing/unavailable also require an empty attempt reference.
Protocol failure is source_invalid. Then missing returns credential_missing and
unavailable gives source_unavailable. For present, first check all field ceilings,
in order account/revision/encoded verifier/attempt; overage is resource_limit before more
copies. Then empty/wrong account, empty revision or empty attempt is source_invalid.
Actual attempt/account/login membership is checked by the trusted port gate after
native verification, not
inferred from those strings. Present empty verifier goes through invalid_record.
Preserve the original verifier bytes throughout; no default/current record is substituted.

The same private record preflight is used by verify and upgrade, before native
verification or any needs-rehash helper. It operates only on the already byte-
bounded view, without strlen or native parsing of untrusted memory:

1. Require nonempty ASCII/no NUL, exactly six `$`-split fields (leading empty,
   algorithm, version, ordered m/t/p, salt, digest). Require exact literal `v=`,
   `m=`, `,t=`, `,p=` and no whitespace/trailing fields. Algorithm token grammar is
   lower-case ASCII `[a-z][a-z0-9_-]{0,31}`; case variants are malformed. Numeric
   fields are complete canonical unsigned decimals: 0 or a nonzero first digit
   followed by digits, no plus/leading zero. Any failure is invalid_record.
2. Require salt 22/digest 43 characters from original Base64 alphabet, no padding;
   decode only to fixed 16/32-byte storage and require exact unpadded re-encoding,
   including zero unused pad bits. This cheap bounded preflight uses ordinary
   byte/bit operations, not a sodium call before runtime readiness. A malformed
   field gives invalid_record. All-zero decoded fields are not automatically
   malformed or verified.
3. t=0, m<8 or p=0 is invalid_record. Compare canonical decimal strings safely;
   no unbounded integer conversion is allowed. An otherwise well-framed algorithm
   other than exact argon2id, version other than exact 19, or positive p other than
   exact 1 is unsupported_record. No fallback algorithm is invoked.
4. For supported profile, t>10 or m>262144 KiB is resource_limit. Scan/compare digit
   strings with checked bounded arithmetic before narrowing. Only then convert
   admitted t/m and multiply m by 1024; the maximum is 268435456 bytes. Check native
   size representability. No native cost rounding or overflow is accepted.
5. If admitted stored memory exceeds admitted_memory_bytes, fail resource_limit
   before initialization, a protected password copy or crypto. Then an accepted
   record may be copied into a zero-terminated 128-byte native buffer; copy no
   untrusted terminator and perform no unbounded read of the original view.

This order fixes combined format/profile/cost failures as well as p=0 rejection.
Desired cost is never used instead of an admitted stored record's verification
parameters. Creation has no input record: its desired memory must fit the caller's
admitted ceiling before initialization/password copying, otherwise resource_limit.

## 4. Native readiness, secret storage and ordinary verification

The dependency remains PROPOSED libsodium 1.0.22 pending root-owned selected artifact,
build/link/runtime checks. Header/pkg-config strings alone are not a verified pin.
The later implementation checks runtime readiness only AFTER input/cost preflight:
sodium_init returning 0/1 is ready; another result or selected header/runtime version mismatch
is runtime_unavailable, never a fallback. The version check is not a substitute for
artifact provenance. [Official initialization rules](https://doc.libsodium.org/usage)
require initialization before library use; constructors/build discovery do not do it.

For each admitted operation, copy password bytes once into bounded guarded memory
of size max(password.size(),1), explicitly check sodium_mlock, THEN copy the raw
bytes. Allocation/locking failure is secret_memory_unavailable. The zero-length
case supplies a valid nonnull native pointer with length 0, never a null pointer
passed to a nonnull API. Native length is always explicit, including NUL/non-ASCII.
This follows the [tagged header's declarations](https://raw.githubusercontent.com/jedisct1/libsodium/1.0.22/src/libsodium/include/sodium/crypto_pwhash_argon2id.h).

Guarded allocation success alone is not proof of locking. One RAII owner keeps the
same checked protected copy alive across verification, both login-source checks
and replacement generation. On every return/exception it clears the entire owned
allocation with sodium_memzero and frees/unlocks it appropriately; it never logs,
serializes or returns plaintext. Bounded native record/output scratch is cleared
on exit too. Caller-owned earlier/UI/network buffers must be cleared by their
owners; Argon/native/register/stack internals are not claimed all locked/erased.
These limits follow the [secure-memory documentation](https://doc.libsodium.org/memory_management).

Creation uses explicit Argon2id13 (or its algorithm-specific encoded API), v19/p1,
16-byte internal random salt and 32-byte digest. It exposes no fixed salt, algorithm
fallback or caller crypto provider. Native creation −1 is hash_unavailable. Native
success must produce a terminated complete canonical record at the exact requested
cost/profile; otherwise native_output_invalid. A bounded 128-byte scan is used,
not strlen on a potentially unterminated output. No partial/empty verifier returns.
Native output-to-owned-string allocation failure is allocation_unavailable.

Standalone verification uses the admitted raw record/profile and checked protected
password copy. Native 0 returns verified. Native −1 returns not_successful, covering
mismatch OR unclassified native failure; it is not wrong_password proof. No errno
or timing text is exposed. Readiness/protection/preflight failures remain typed
errors and cannot masquerade as a completed native verification. Verification
returns no proof/certificate accepted by propose_password_upgrade.

## 5. Post-login upgrade and caller's conditional save

After options/password/account validation, read and validate the CURRENT source
credential through the port, perform record/admission preflight, initialize and
freshly verify it as above. Preserve this full owned snapshot unchanged. Native −1
returns verification_not_successful, with no login-source check, hash or replacement.

After native 0, create the private borrowed verified-source view bound to this
exact port/snapshot. Check current successful login ONCE. denied returns
login_not_accepted; stale returns stale_source; unavailable gives source_unavailable.
Unknown enum/port protocol shape is source_invalid. Only accepted may continue.
Only now compare costs: if both stored dimensions meet/exceed desired, return
not_needed without hashing or changing encoded bytes. Otherwise choose componentwise
maxima. Joined cost stays within new-profile ceilings and never lowers either
dimension. No needs-rehash/default-algorithm helper alone decides this policy.

If joined memory exceeds the admitted ceiling, return upgrade_unavailable with
resource_limit, no new hash. Otherwise create and validate a fresh encoded record
at the joined cost. Native hash failure, malformed/mismatched successful native
output or replacement-materialization allocation failure returns upgrade_unavailable
with the corresponding reason, preserves the old stored record and returns no
partial replacement. The actually accepted login is not reclassified as wrong.
Preservation means this helper performs no write: if another actor already reset
the credential, that newer value remains and the old snapshot is never restored.

After successful creation, check the SAME actual current-login/source state AGAIN
with its private borrow. A reset/revision change or denied login discards the new
record and returns stale_source/login_not_accepted. Unavailability/protocol failure
produces its typed failure, not replacement_ready. Only a second accepted check
allows the complete source-bound replacement value to return. Repeated checks do
not create or extend a login/session. The port cannot retain the proof borrow.

There is still a race AFTER return. The actual account-store caller MUST atomically
compare the source account, actual revision AND full encoded verifier, and the
actual bound login attempt's still-accepted login/account conditions, before saving
the exact replacement.
It must reject stale/missing/currently unauthorized work and retain newer credentials.
Neither the result class nor a copied raw DTO/boolean authorizes that save. This
module has no save method or write-capable port; those actual transactions/recovery/
session consequences remain account/store integration gates, not simulated behavior
that can certify this helper. No success or durable-upgrade claim precedes that save.

Reverification and the two actual source checks are deliberate. The benchmark must
measure the real end-to-end login/upgrade call sequence, including any earlier
standalone verification and the joined-cost hash, not claim a one-hash latency.
The caller keeps admission, source and plaintext ingress valid until return; timeout
or cancellation cannot destroy them while native work is in flight. No cooperative
hard deadline, hidden retry, queue, network call or writer-thread crypto is supplied.

These direct setups refine PH01–18; they are future expectations, not executed
crypto or generated fixtures:

| Raw setup | Required observation |
| --- | --- |
| Empty password and literal upstream H0 from logical §6, default options | Standalone verified if the selected runtime/protection is available; no account-login claim |
| The same H0 with one NUL byte instead of empty | Native unsuccessful observation; never replacement authority |
| Source holds account A/revision R/H0/actual accepted attempt L; both checks remain accepted | replacement_ready owns A/L/R/exact H0 and a newly encoded t=3/m=65536/p=1 record; no database write |
| Same source, first check denied | login_not_accepted; no replacement generation |
| Same source, actual revision changes before the second check | stale_source; discard the new verifier and preserve the new current credential |
| Same source with 8388608 admitted bytes | H0's 4999168-byte verification fits; after accepted login, the desired 67108864-byte replacement does not: upgrade_unavailable/resource_limit, no new hash |
| desired_operations=1 and password length 1025 together | invalid_options precedes input resource_limit |
| Canonical record with p=0 versus positive p=2, otherwise valid framing | invalid_record versus unsupported_record, both before native verification |

PH12/13's stronger stored-cost scenarios require independently valid verifier
fixtures and known passwords. Editing H0's m/t fields does not preserve its digest
or provide a successful-verification oracle. New random salt/profile assertions
must not compare against a verifier generated by the implementation as its oracle.

## 6. Fixed failures, bounds and proof partition

```cpp
enum class PasswordHashingError {
    invalid_options, resource_limit, invalid_record, unsupported_record,
    invalid_account_reference, runtime_unavailable, secret_memory_unavailable,
    hash_unavailable, native_output_invalid, allocation_unavailable,
    source_unavailable, source_invalid
};
class PasswordHashingFailure final : public std::runtime_error {
  public:
    explicit PasswordHashingFailure(PasswordHashingError code);
    [[nodiscard]] PasswordHashingError code() const noexcept;
};
```

what() is exactly `Password hashing operation failed`, with the supplied code and
no secret/account/record/native error text. The only private member is
`PasswordHashingError code_`; construction stores the supplied enum without native
work. Port exceptions become source_unavailable without retaining/rethrowing their
diagnostic text; source-allocation failure is
allocation_unavailable. Unknown helper programming errors are not reclassified as
wrong passwords or malformed credentials. Standard allocation can still fail while
constructing any exception; std::bad_alloc may then propagate without a successful
result. Documented native return failures are handled; this API promises no recovery
from fatal library/runtime faults or invalid caller memory ranges.
Other ordinary module allocation failures before the accepted upgrade-generation
stage give allocation_unavailable; allocation failures while materializing that
replacement give upgrade_unavailable/allocation_unavailable. Guarded password
allocation/locking always uses secret_memory_unavailable. None of these paths
reclassifies the caller's already accepted login or performs persistence.

No unbounded collections or parser numbers exist. Dynamic module password ownership
is at most 1024 bytes (or one byte for empty input), record buffers 128, decoded salt/
digest 16/32, reference fields 256 each and stored/create records 127 each. A credential
owns at most 639 string bytes, a present read including attempt at most 895, and a
replacement at most 1022. Check each complete field and output bound before
additional copies/growth. This is a semantic bound, not a
claim about guarded pages, native Argon memory, allocator/worker overhead or caller-
constructed source objects. There is no public fault-injected successful crypto,
test-only salt or fake-ready runtime. Later independently reviewed PRIVATE error-only
test seams may prove cleanup/failure paths; actual successful hash/verify artifacts
must use the selected real library.

Each of the THREE future feature bodies must have a single immediate first statement:

```cpp
throw PasswordHashingNotImplemented{};
```

PasswordHashingNotImplemented final derives from std::logic_error, with exact text
`Password hashing is intentionally unimplemented; review tests first`. It is
distinct from PasswordHashingFailure. Constructors/accessors/port declarations are
inert; they never initialize/link-call a native symbol, validate input, invoke a
port or manufacture success. Import/configure/build/discovery must never execute
an entrypoint/marker, initialization, hash/verify/RNG or guarded allocation.

Raw H0 can be passed directly to verify; a test source port can hold H0 and actual
simulated login/revision state directly for upgrade. The private proof is created
inside the future implemented upgrade path, not by test setup. Public synthetic
fixtures and error-only controls need different actual review. A separately reviewed
standalone selected-library foundation must first initialize and verify independent
upstream H0/byte vectors successfully, with no project helper supplying its oracle.
Only THEN may reviewed project tests run FIRST intended named-marker red. Dependency,
compile/setup/protection failure, skips or exit inversion are not a valid red.

This document executes none of those operations and approves no later code/tests.
Selected pins/native runtime, protected lifetimes, actual port/store implementation,
conditional-save races, account policy/admission/transport, measured supported server
cost and Linux/Windows package behavior retain their separate review/acceptance gates.
