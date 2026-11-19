// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../server/src/credential_record.hpp"
#include "../fixtures/credential_vectors.hpp"
#include "../support/credential_native_controls.hpp"

#include <algorithm>
#include <array>
#include <gtest/gtest.h>
#include <sodium.h>
#include <string>
#include <type_traits>

using namespace subathon::server;
using namespace subathon::server::credential_detail;
namespace native = credential_native;
namespace v = credential_vectors;

namespace {
static_assert(!std::is_copy_constructible_v<CredentialKey>);
static_assert(!std::is_copy_constructible_v<CredentialValue>);
static_assert(std::is_nothrow_move_constructible_v<CredentialKey>);
static_assert(std::is_nothrow_move_constructible_v<CredentialValue>);

template <class T> void expect_error(const T &result, CredentialError expected) {
    const auto *error = std::get_if<CredentialError>(&result);
    ASSERT_NE(error, nullptr) << "Expected a typed credential refusal";
    EXPECT_EQ(*error, expected);
}

TEST(CredentialRecord, SelectedSdkAuthenticatesIndependentLiteral) {
    ASSERT_TRUE(native::prepare_sdk());
    constexpr auto aad = v::hex(
        "7375626174686f6e2e70726f76696465722d63726564656e7469616c2e763100"
        "53435231101112131415161718191a1b1c1d1e1f404142434445464748494a4b4c4d4e4f5051525354555657"
        "0000000700024900010002ff430200014100027400");
    static_assert(aad.size() == 97 && v::record.size() == 71 && v::key_file.size() == 52);
    std::array<unsigned char, 23> encrypted{};
    unsigned long long size{};
    ASSERT_EQ(crypto_aead_xchacha20poly1305_ietf_encrypt(
                  encrypted.data(), &size, v::plaintext.data(), v::plaintext.size(), aad.data(),
                  aad.size(), nullptr, v::nonce.data(), v::key.data()),
              0);
    ASSERT_EQ(size, encrypted.size());
    EXPECT_TRUE(std::ranges::equal(encrypted, std::span(v::record).subspan(48)));
}

TEST(CredentialRecord, OpensIndependentLiteralIntoOwnedBytes) {
    ASSERT_TRUE(native::prepare_sdk());
    const auto result = open_record(v::scope, v::record, v::id, v::key);
    if (const auto *error = std::get_if<CredentialError>(&result))
        ADD_FAILURE() << "Open literal returned category " << static_cast<int>(*error);
    const auto *value = std::get_if<CredentialValue>(&result);
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(std::ranges::equal(value->bytes(), v::plaintext));
}

TEST(CredentialRecord, SealsIndependentLiteralWithOneApplicationDraw) {
    ASSERT_TRUE(native::prepare_sdk());
    const std::array nonces{v::nonce, v::second_nonce};
    const native::Controls controls(native::Fault::none, 0, nonces);
    const auto result = seal_record(v::scope, v::plaintext, v::id, v::key);
    const auto *record = std::get_if<RecordBytes>(&result);
    ASSERT_TRUE(record != nullptr) << "Seal literal must return a record";
    EXPECT_EQ(controls.observation().nonce_draws, 1U);
    EXPECT_TRUE(std::ranges::equal(*record, v::record));
}

TEST(CredentialRecord, EncodesExactOpaqueAndLengthFramedScope) {
    const auto actual = encode_scope(v::exact);
    const auto *scope = std::get_if<RecordBytes>(&actual);
    ASSERT_TRUE(scope != nullptr) << "Canonical scope must encode";
    EXPECT_TRUE(std::ranges::equal(*scope, v::scope));

    const auto installation = InstallationSubject{};
    const auto left = encode_scope({"A", ConnectorProvider::twitch, "BC", installation, "s"});
    const auto right = encode_scope({"AB", ConnectorProvider::twitch, "C", installation, "s"});
    ASSERT_TRUE(std::holds_alternative<RecordBytes>(left));
    ASSERT_TRUE(std::holds_alternative<RecordBytes>(right));
    EXPECT_TRUE(
        std::ranges::equal(std::get<RecordBytes>(left), v::hex("000141010002424301000173")));
    EXPECT_TRUE(
        std::ranges::equal(std::get<RecordBytes>(right), v::hex("000241420100014301000173")));
    for (const auto ref :
         {std::string_view("a"), std::string_view("A\0", 2), std::string_view("\xc1")}) {
        const auto base = encode_scope({"A", ConnectorProvider::twitch, "C", installation, "s"});
        const auto distinct =
            encode_scope({ref, ConnectorProvider::twitch, "C", installation, "s"});
        ASSERT_TRUE(std::holds_alternative<RecordBytes>(distinct));
        EXPECT_TRUE(std::get<RecordBytes>(base).size() == 11);
        EXPECT_TRUE(std::get<RecordBytes>(distinct) != std::get<RecordBytes>(base));
    }
    constexpr std::array providers{
        ConnectorProvider::twitch,         ConnectorProvider::betterplace,
        ConnectorProvider::patreon,        ConnectorProvider::streamlabs,
        ConnectorProvider::streamelements, ConnectorProvider::kofi,
        ConnectorProvider::tipeeestream};
    for (std::size_t index = 0; index < providers.size(); ++index) {
        const auto tagged = encode_scope({"I", providers[index], "C", installation, "s"});
        const auto *bytes = std::get_if<RecordBytes>(&tagged);
        ASSERT_TRUE(bytes && bytes->size() == 11);
        EXPECT_EQ((*bytes)[3], index + 1);
    }
}

TEST(CredentialRecord, EncodesMaximumAccountScope) {
    const std::string ref(4096, 'A');
    const auto actual =
        encode_scope({ref, ConnectorProvider::twitch, ref, AccountSubject{ref}, ref});
    const auto *scope = std::get_if<RecordBytes>(&actual);
    ASSERT_TRUE(scope && scope->size() == 16394);
    for (const auto offset : {0U, 4099U, 8198U, 12296U})
        EXPECT_TRUE((*scope)[offset] == 0x10 && (*scope)[offset + 1] == 0);
}

TEST(CredentialRecord, AuthenticationRejectsNonceCiphertextAndTagEdits) {
    ASSERT_TRUE(native::prepare_sdk());
    for (const auto offset : {20U, 48U, 70U}) {
        SCOPED_TRACE(offset);
        auto changed = v::record;
        changed[offset] ^= 1;
        expect_error(open_record(v::scope, changed, v::id, v::key),
                     CredentialError::authentication_failed);
    }
}

TEST(CredentialRecord, FramingRefusesBeforeKeySelection) {
    ASSERT_TRUE(native::prepare_sdk());
    auto unavailable = v::record;
    unavailable[4] ^= 1;
    std::vector<RecordBytes> malformed{{},
                                       {unavailable.begin(), unavailable.end() - 1},
                                       {unavailable.begin(), unavailable.end()},
                                       {unavailable.begin(), unavailable.end()}};
    malformed[2].push_back(0);
    malformed[3][3] = '2';
    for (const auto length : {0U, 8U, 65537U, 0xffffffffU}) {
        auto changed = unavailable;
        for (unsigned int byte = 0; byte < 4; ++byte)
            changed[44 + byte] = static_cast<unsigned char>(length >> (24 - 8 * byte));
        malformed.emplace_back(changed.begin(), changed.end());
    }
    for (std::size_t index = 0; index < malformed.size(); ++index) {
        SCOPED_TRACE(index);
        expect_error(open_record(v::scope, malformed[index], v::id, v::key),
                     CredentialError::invalid_record);
    }
}

TEST(CredentialRecord, OpeningFailsClosedOnGuardedAllocationOrExplicitLock) {
    ASSERT_TRUE(native::prepare_sdk());
    for (const auto fault : {native::Fault::allocation, native::Fault::lock}) {
        const native::Controls controls(fault, v::plaintext.size());
        const auto result = open_record(v::scope, v::record, v::id, v::key);
        expect_error(result, CredentialError::crypto_unavailable);
        const auto observations = controls.observation();
        EXPECT_EQ(observations.allocations, 1U);
        EXPECT_EQ(observations.locks, fault == native::Fault::lock ? 1U : 0U);
        EXPECT_EQ(observations.releases, fault == native::Fault::lock ? 1U : 0U);
    }
}
} // namespace
