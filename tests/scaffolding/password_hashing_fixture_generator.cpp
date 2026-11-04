// SPDX-License-Identifier: GPL-3.0-or-later
// SDK-only synthetic INPUT preparation. Not a SUT oracle, project test or cost benchmark.
// Execute only after different review and a green selected-library foundation.
#include <sodium.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string_view>

namespace {
struct InputCase {
    const char *id;
    const char *password;
    const char *prefix;
    std::size_t memory_bytes;
    unsigned int memory_kib;
};
constexpr std::array<InputCase, 2> cases{{
    {"PH12", "Subathon public fixture PH12", "$argon2id$v=19$m=32768,t=6,p=1$", 33554432, 32768},
    {"PH13", "Subathon public fixture PH13", "$argon2id$v=19$m=131072,t=6,p=1$", 134217728, 131072},
}};

bool field_is_canonical(std::string_view field, std::size_t size) {
    std::array<unsigned char, 32> decoded{};
    std::array<char, 45> encoded{};
    std::size_t decoded_size{};
    const char *end{};
    if (sodium_base642bin(decoded.data(), decoded.size(), field.data(), field.size(), nullptr,
                          &decoded_size, &end, sodium_base64_VARIANT_ORIGINAL_NO_PADDING) != 0 ||
        decoded_size != size || end != field.data() + field.size() ||
        sodium_bin2base64(encoded.data(), encoded.size(), decoded.data(), decoded_size,
                          sodium_base64_VARIANT_ORIGINAL_NO_PADDING) == nullptr) {
        return false;
    }
    const auto encoded_end = std::find(encoded.begin(), encoded.end(), '\0');
    return encoded_end != encoded.end() &&
           field == std::string_view(encoded.data(),
                                     static_cast<std::size_t>(encoded_end - encoded.begin()));
}

bool prepare(const InputCase &input, std::array<char, 128> &record) {
    if (crypto_pwhash_argon2id_str(record.data(), input.password, std::strlen(input.password), 6,
                                   input.memory_bytes) != 0) {
        return false;
    }
    const auto end = std::find(record.begin(), record.end(), '\0');
    if (end == record.end()) {
        return false;
    }
    const std::string_view text(record.data(), static_cast<std::size_t>(end - record.begin()));
    const std::string_view prefix(input.prefix);
    if (!text.starts_with(prefix)) {
        return false;
    }
    const auto separator = text.find('$', prefix.size());
    if (separator == std::string_view::npos) {
        return false;
    }
    const auto salt = text.substr(prefix.size(), separator - prefix.size());
    const auto digest = text.substr(separator + 1);
    return salt.size() == 22 && digest.size() == 43 && field_is_canonical(salt, 16) &&
           field_is_canonical(digest, 32) &&
           crypto_pwhash_argon2id_str_verify(record.data(), input.password,
                                             std::strlen(input.password)) == 0;
}
} // namespace

int main() {
    if (std::string_view(SODIUM_VERSION_STRING) != "1.0.22" || SODIUM_LIBRARY_VERSION_MAJOR != 26 ||
        SODIUM_LIBRARY_VERSION_MINOR != 4) {
        std::fputs("Synthetic password input preparation: wrong selected headers\n", stderr);
        return 1;
    }
    const int initialized = sodium_init();
    if ((initialized != 0 && initialized != 1) ||
        std::strcmp(sodium_version_string(), "1.0.22") != 0 ||
        sodium_library_version_major() != 26 || sodium_library_version_minor() != 4) {
        std::fputs("Synthetic password input preparation: selected runtime unavailable\n", stderr);
        return 1;
    }
    std::array<std::array<char, 128>, 2> records{};
    for (std::size_t i = 0; i < cases.size(); ++i) {
        if (!prepare(cases[i], records[i])) {
            sodium_memzero(records.data(), sizeof(records));
            std::fputs("Synthetic password input preparation failed; no records published\n",
                       stderr);
            return 1;
        }
    }
    // Both records are validated before any output. Literals/PHC alphabet need no JSON escaping.
    std::puts("[");
    for (std::size_t i = 0; i < cases.size(); ++i) {
        std::printf("  {\"case_id\":\"%s\",\"password\":\"%s\",\"encoded_verifier\":\"%s\","
                    "\"operations\":6,\"memory_kib\":%u}%s\n",
                    cases[i].id, cases[i].password, records[i].data(), cases[i].memory_kib,
                    i + 1 == cases.size() ? "" : ",");
    }
    std::puts("]");
    sodium_memzero(records.data(), sizeof(records));
    if (std::fflush(stdout) != 0 || std::ferror(stdout)) {
        std::fputs("Synthetic password input publication failed; output may be partial\n", stderr);
        return 1;
    }
    return 0;
}
