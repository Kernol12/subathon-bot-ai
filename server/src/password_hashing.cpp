// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/password_hashing.hpp>

#include <sodium.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <new>
#include <utility>

namespace subathon::server {
namespace {
using Error = PasswordHashingError;
constexpr std::uint64_t minimum_new_memory = 67108864;
constexpr std::uint64_t maximum_memory = 268435456;
constexpr std::size_t record_capacity = 128;
constexpr std::size_t reference_limit = 256;
static_assert(crypto_pwhash_argon2id_STRBYTES == record_capacity);
static_assert(crypto_pwhash_argon2id_SALTBYTES == 16);
static_assert(SODIUM_LIBRARY_VERSION_MAJOR == 26 && SODIUM_LIBRARY_VERSION_MINOR == 4);
static_assert(std::string_view(SODIUM_VERSION_STRING) == "1.0.22");

[[noreturn]] void fail(Error error) { throw PasswordHashingFailure(error); }

void validate_options(const PasswordHashingOptions &options) {
    if (options.desired_operations < 2 || options.desired_operations > 10 ||
        options.desired_memory_bytes < minimum_new_memory ||
        options.desired_memory_bytes > maximum_memory || options.desired_memory_bytes % 1024 != 0 ||
        options.admitted_memory_bytes > maximum_memory || options.admitted_memory_bytes % 1024 != 0)
        fail(Error::invalid_options);
}

void validate_password(PasswordBytes password) {
    if (password.size() > 1024)
        fail(Error::resource_limit);
}

bool canonical_decimal(std::string_view value) {
    if (value.empty() || (value.size() > 1 && value.front() == '0'))
        return false;
    return std::all_of(value.begin(), value.end(),
                       [](unsigned char c) { return c >= '0' && c <= '9'; });
}

// Both inputs have already been checked canonical; compare before bounded conversion.
bool decimal_above(std::string_view value, std::string_view maximum) {
    return value.size() > maximum.size() || (value.size() == maximum.size() && value > maximum);
}

std::uint32_t small_decimal(std::string_view value) {
    std::uint32_t result = 0;
    for (const auto c : value)
        result = result * 10 + static_cast<unsigned>(c - '0');
    return result;
}

int base64_digit(unsigned char c) {
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 26;
    if (c >= '0' && c <= '9')
        return c - '0' + 52;
    if (c == '+')
        return 62;
    if (c == '/')
        return 63;
    return -1;
}

// Ordinary fixed-byte preflight runs before sodium_init, so it uses no native helper.
// The decoded private verifier metadata is cleared even on malformed early returns.
template <std::size_t Size> struct DecodedBytes final {
    std::array<unsigned char, Size> bytes{};
    ~DecodedBytes() {
        volatile unsigned char *wipe = bytes.data();
        for (std::size_t i = 0; i < Size; ++i)
            wipe[i] = 0;
    }
};

template <std::size_t Size> bool canonical_base64(std::string_view encoded) {
    if (encoded.size() != (Size * 8 + 5) / 6)
        return false;
    DecodedBytes<Size> decoded;
    unsigned accumulator = 0, bits = 0;
    std::size_t output = 0;
    for (const auto c : encoded) {
        const int digit = base64_digit(static_cast<unsigned char>(c));
        if (digit < 0)
            return false;
        accumulator = (accumulator << 6) | static_cast<unsigned>(digit);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (output == Size)
                return false;
            decoded.bytes[output++] = static_cast<unsigned char>(accumulator >> bits);
            accumulator &= (1U << bits) - 1;
        }
    }
    if (output != Size || accumulator != 0)
        return false; // unused pad bits must be zero
    constexpr std::string_view alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    accumulator = 0;
    bits = 0;
    output = 0;
    for (const auto byte : decoded.bytes) {
        accumulator = (accumulator << 8) | byte;
        bits += 8;
        while (bits >= 6) {
            bits -= 6;
            if (encoded[output++] != alphabet[(accumulator >> bits) & 63])
                return false;
        }
        accumulator &= (1U << bits) - 1;
    }
    return bits == 0 || encoded[output] == alphabet[accumulator << (6 - bits)];
}

struct Cost {
    std::uint32_t operations;
    std::uint64_t memory_bytes;
};

Cost preflight_record(std::string_view encoded, std::uint64_t admitted_memory) {
    if (encoded.size() >= record_capacity)
        fail(Error::resource_limit);
    if (encoded.empty() || !std::all_of(encoded.begin(), encoded.end(),
                                        [](unsigned char c) { return c > 32 && c < 127; }))
        fail(Error::invalid_record);
    std::array<std::string_view, 6> fields;
    std::size_t start = 0;
    for (std::size_t i = 0; i < fields.size() - 1; ++i) {
        const auto end = encoded.find('$', start);
        if (end == std::string_view::npos)
            fail(Error::invalid_record);
        fields[i] = encoded.substr(start, end - start);
        start = end + 1;
    }
    fields.back() = encoded.substr(start);
    if (!fields[0].empty() || fields.back().find('$') != std::string_view::npos)
        fail(Error::invalid_record);
    const auto algorithm = fields[1];
    if (algorithm.empty() || algorithm.size() > 32 || algorithm.front() < 'a' ||
        algorithm.front() > 'z' ||
        !std::all_of(algorithm.begin(), algorithm.end(),
                     [](unsigned char c) {
                         return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' ||
                                c == '-';
                     }) ||
        !fields[2].starts_with("v="))
        fail(Error::invalid_record);
    const auto version = fields[2].substr(2);
    const auto parameters = fields[3];
    const auto time_at = parameters.find(",t=");
    const auto parallel_at = parameters.find(",p=");
    if (!parameters.starts_with("m=") || time_at == std::string_view::npos ||
        parallel_at == std::string_view::npos || time_at < 2 || parallel_at < time_at + 3)
        fail(Error::invalid_record);
    const auto memory = parameters.substr(2, time_at - 2);
    const auto operations = parameters.substr(time_at + 3, parallel_at - time_at - 3);
    const auto parallelism = parameters.substr(parallel_at + 3);
    if (!canonical_decimal(version) || !canonical_decimal(memory) ||
        !canonical_decimal(operations) || !canonical_decimal(parallelism) ||
        !canonical_base64<16>(fields[4]) || !canonical_base64<32>(fields[5]))
        fail(Error::invalid_record);
    if (operations == "0" || (memory.size() == 1 && memory.front() < '8') || parallelism == "0")
        fail(Error::invalid_record);
    if (algorithm != "argon2id" || version != "19" || parallelism != "1")
        fail(Error::unsupported_record);
    if (decimal_above(operations, "10") || decimal_above(memory, "262144"))
        fail(Error::resource_limit);
    const auto memory_bytes = static_cast<std::uint64_t>(small_decimal(memory)) * 1024;
    if (memory_bytes > admitted_memory || memory_bytes > std::numeric_limits<std::size_t>::max())
        fail(Error::resource_limit);
    return {small_decimal(operations), memory_bytes};
}

void require_runtime() {
    const int initialized = sodium_init();
    if ((initialized != 0 && initialized != 1) || sodium_version_string() == nullptr ||
        std::strcmp(sodium_version_string(), SODIUM_VERSION_STRING) != 0 ||
        sodium_library_version_major() != SODIUM_LIBRARY_VERSION_MAJOR ||
        sodium_library_version_minor() != SODIUM_LIBRARY_VERSION_MINOR)
        fail(Error::runtime_unavailable);
}

// One allocation remains locked across verification, both source checks and new hashing.
class ProtectedPassword final {
  public:
    explicit ProtectedPassword(PasswordBytes password)
        : size_(std::max<std::size_t>(password.size(), 1)), data_(sodium_malloc(size_)) {
        if (!data_)
            fail(Error::secret_memory_unavailable);
        if (sodium_mlock(data_, size_) != 0) {
            clear();
            fail(Error::secret_memory_unavailable);
        }
        if (!password.empty())
            std::memcpy(data_, password.data(), password.size());
        else
            *static_cast<unsigned char *>(data_) = 0;
    }
    ProtectedPassword(const ProtectedPassword &) = delete;
    ProtectedPassword &operator=(const ProtectedPassword &) = delete;
    ~ProtectedPassword() { clear(); }
    [[nodiscard]] const char *data() const noexcept { return static_cast<const char *>(data_); }

  private:
    void clear() noexcept {
        if (data_) {
            sodium_memzero(data_, size_);
            sodium_free(data_); // the selected guarded allocator unlocks while freeing
            data_ = nullptr;
        }
    }
    std::size_t size_;
    void *data_;
};

// Construct only after readiness; all 128 native scratch bytes are cleared on every exit.
struct NativeRecord final {
    std::array<char, record_capacity> bytes{};
    NativeRecord() = default;
    NativeRecord(const NativeRecord &) = delete;
    NativeRecord &operator=(const NativeRecord &) = delete;
    ~NativeRecord() { sodium_memzero(bytes.data(), bytes.size()); }
};

void copy_record(NativeRecord &out, std::string_view encoded) {
    std::copy(encoded.begin(), encoded.end(), out.bytes.begin());
    out.bytes[encoded.size()] = '\0'; // caller already completed <=127-byte preflight
}

std::string_view generate_record(NativeRecord &out, const ProtectedPassword &password,
                                 std::size_t password_length, Cost cost) {
    if (crypto_pwhash_argon2id_str(out.bytes.data(), password.data(), password_length,
                                   cost.operations,
                                   static_cast<std::size_t>(cost.memory_bytes)) != 0)
        fail(Error::hash_unavailable);
    const auto end = std::find(out.bytes.begin(), out.bytes.end(), '\0');
    if (end == out.bytes.end())
        fail(Error::native_output_invalid);
    const std::string_view encoded(out.bytes.data(),
                                   static_cast<std::size_t>(end - out.bytes.begin()));
    try {
        const auto produced = preflight_record(encoded, cost.memory_bytes);
        if (produced.operations != cost.operations || produced.memory_bytes != cost.memory_bytes)
            fail(Error::native_output_invalid);
    } catch (const PasswordHashingFailure &) {
        fail(Error::native_output_invalid);
    }
    return encoded;
}

PasswordCredentialRead read_source(PasswordLoginSource &source, std::string_view account) {
    try {
        return source.read_current(account);
    } catch (const std::bad_alloc &) {
        fail(Error::allocation_unavailable);
    } catch (...) {
        fail(Error::source_unavailable);
    }
}

void validate_read(const PasswordCredentialRead &reading, std::string_view requested) {
    if (reading.state != PasswordCredentialState::present &&
        reading.state != PasswordCredentialState::missing &&
        reading.state != PasswordCredentialState::unavailable)
        fail(Error::source_invalid);
    if ((reading.state == PasswordCredentialState::present) != reading.credential.has_value() ||
        (reading.state != PasswordCredentialState::present && !reading.login_attempt_ref.empty()))
        fail(Error::source_invalid);
    if (reading.state != PasswordCredentialState::present)
        return;
    const auto &credential = *reading.credential;
    if (credential.account_ref.size() > reference_limit ||
        credential.revision_ref.size() > reference_limit ||
        credential.encoded_verifier.size() >= record_capacity ||
        reading.login_attempt_ref.size() > reference_limit)
        fail(Error::resource_limit);
    if (credential.account_ref.empty() || credential.account_ref != requested ||
        credential.revision_ref.empty() || reading.login_attempt_ref.empty())
        fail(Error::source_invalid);
}

PasswordLoginCheck check_source(PasswordLoginSource &source, const PasswordVerifiedSource &proof) {
    PasswordLoginCheck result;
    try {
        result = source.check_current_successful_login(proof);
    } catch (const std::bad_alloc &) {
        fail(Error::allocation_unavailable);
    } catch (...) {
        fail(Error::source_unavailable);
    }
    if (result != PasswordLoginCheck::accepted && result != PasswordLoginCheck::denied &&
        result != PasswordLoginCheck::stale && result != PasswordLoginCheck::unavailable)
        fail(Error::source_invalid);
    if (result == PasswordLoginCheck::unavailable)
        fail(Error::source_unavailable);
    return result;
}

PasswordUpgradeData outcome(PasswordUpgradeOutcome value) { return {value, {}, {}}; }

PasswordUpgradeData unavailable(Error reason) {
    return {PasswordUpgradeOutcome::upgrade_unavailable, {}, reason};
}

PasswordUpgradeData rejected_check(PasswordLoginCheck check) {
    return outcome(check == PasswordLoginCheck::stale ? PasswordUpgradeOutcome::stale_source
                                                      : PasswordUpgradeOutcome::login_not_accepted);
}
} // namespace

PasswordHashingFailure::PasswordHashingFailure(PasswordHashingError code)
    : std::runtime_error("Password hashing operation failed"), code_(code) {}

PasswordHashingError PasswordHashingFailure::code() const noexcept { return code_; }

PasswordHashingNotImplemented::PasswordHashingNotImplemented()
    : std::logic_error("Password hashing is intentionally unimplemented; review tests first") {}

PasswordVerifiedSource::PasswordVerifiedSource(const PasswordCredential &credential,
                                               std::string_view login_attempt_ref,
                                               const PasswordLoginSource &source) noexcept
    : credential_(&credential), login_attempt_ref_(login_attempt_ref), source_(&source) {}

const PasswordCredential &PasswordVerifiedSource::credential() const noexcept {
    return *credential_;
}

std::string_view PasswordVerifiedSource::login_attempt_ref() const noexcept {
    return login_attempt_ref_;
}

bool PasswordVerifiedSource::belongs_to(const PasswordLoginSource &source) const noexcept {
    return source_ == &source;
}

PasswordUpgradeResult::PasswordUpgradeResult(PasswordUpgradeData data) : data_(std::move(data)) {}

const PasswordUpgradeData &PasswordUpgradeResult::data() const noexcept { return data_; }

std::string create_password_hash(PasswordBytes password, const PasswordHashingOptions &options) {
    try {
        validate_options(options);
        validate_password(password);
        if (options.desired_memory_bytes > options.admitted_memory_bytes ||
            options.desired_memory_bytes > std::numeric_limits<std::size_t>::max())
            fail(Error::resource_limit);
        require_runtime();
        ProtectedPassword protected_password(password);
        NativeRecord generated;
        return std::string(
            generate_record(generated, protected_password, password.size(),
                            {options.desired_operations, options.desired_memory_bytes}));
    } catch (const std::bad_alloc &) {
        fail(Error::allocation_unavailable);
    } catch (const std::length_error &) {
        fail(Error::allocation_unavailable);
    }
}

PasswordVerification verify_password(PasswordBytes password, std::string_view encoded,
                                     const PasswordHashingOptions &options) {
    try {
        validate_options(options);
        validate_password(password);
        (void)preflight_record(encoded, options.admitted_memory_bytes);
        require_runtime();
        ProtectedPassword protected_password(password);
        NativeRecord native_record;
        copy_record(native_record, encoded);
        return crypto_pwhash_argon2id_str_verify(native_record.bytes.data(),
                                                 protected_password.data(), password.size()) == 0
                   ? PasswordVerification::verified
                   : PasswordVerification::not_successful;
    } catch (const std::bad_alloc &) {
        fail(Error::allocation_unavailable);
    } catch (const std::length_error &) {
        fail(Error::allocation_unavailable);
    }
}

PasswordUpgradeResult propose_password_upgrade(PasswordBytes password, std::string_view account,
                                               PasswordLoginSource &source,
                                               const PasswordHashingOptions &options) {
    try {
        validate_options(options);
        validate_password(password);
        if (account.size() > reference_limit)
            fail(Error::resource_limit);
        if (account.empty())
            fail(Error::invalid_account_reference);
        const auto reading = read_source(source, account);
        validate_read(reading, account);
        if (reading.state == PasswordCredentialState::missing)
            return PasswordUpgradeResult(outcome(PasswordUpgradeOutcome::credential_missing));
        if (reading.state == PasswordCredentialState::unavailable)
            fail(Error::source_unavailable);
        const auto &credential = *reading.credential;
        const auto stored =
            preflight_record(credential.encoded_verifier, options.admitted_memory_bytes);
        require_runtime();
        ProtectedPassword protected_password(password);
        NativeRecord native_record;
        copy_record(native_record, credential.encoded_verifier);
        if (crypto_pwhash_argon2id_str_verify(native_record.bytes.data(), protected_password.data(),
                                              password.size()) != 0)
            return PasswordUpgradeResult(
                outcome(PasswordUpgradeOutcome::verification_not_successful));
        const PasswordVerifiedSource proof(credential, reading.login_attempt_ref, source);
        const auto first_check = check_source(source, proof);
        if (first_check != PasswordLoginCheck::accepted)
            return PasswordUpgradeResult(rejected_check(first_check));
        if (stored.operations >= options.desired_operations &&
            stored.memory_bytes >= options.desired_memory_bytes)
            return PasswordUpgradeResult(outcome(PasswordUpgradeOutcome::not_needed));
        const Cost joined{std::max(stored.operations, options.desired_operations),
                          std::max(stored.memory_bytes, options.desired_memory_bytes)};
        if (joined.memory_bytes > options.admitted_memory_bytes)
            return PasswordUpgradeResult(unavailable(Error::resource_limit));
        NativeRecord generated;
        std::string_view replacement;
        try {
            replacement = generate_record(generated, protected_password, password.size(), joined);
        } catch (const PasswordHashingFailure &failure) {
            if (failure.code() != Error::hash_unavailable &&
                failure.code() != Error::native_output_invalid)
                throw;
            return PasswordUpgradeResult(unavailable(failure.code()));
        } catch (const std::bad_alloc &) {
            return PasswordUpgradeResult(unavailable(Error::allocation_unavailable));
        }
        const auto second_check = check_source(source, proof);
        if (second_check != PasswordLoginCheck::accepted)
            return PasswordUpgradeResult(rejected_check(second_check));
        try {
            // Source ceilings and the full <=127-byte output were checked before these copies.
            PasswordReplacement proposal{credential.account_ref, reading.login_attempt_ref,
                                         credential.revision_ref, credential.encoded_verifier,
                                         std::string(replacement)};
            return PasswordUpgradeResult(
                {PasswordUpgradeOutcome::replacement_ready, std::move(proposal), {}});
        } catch (const std::bad_alloc &) {
            return PasswordUpgradeResult(unavailable(Error::allocation_unavailable));
        } catch (const std::length_error &) {
            return PasswordUpgradeResult(unavailable(Error::allocation_unavailable));
        }
    } catch (const std::bad_alloc &) {
        fail(Error::allocation_unavailable);
    } catch (const std::length_error &) {
        fail(Error::allocation_unavailable);
    }
}
} // namespace subathon::server
