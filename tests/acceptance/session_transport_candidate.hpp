// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <subathon/server/session_resolution.hpp>
#include <variant>

namespace subathon::server {
enum class AccountTransportPurpose : std::uint8_t { protected_access, browser_sign_in_guard };
enum class AccountTransportKind : std::uint8_t { browser_cookie, desktop_bearer };
enum class AccountTransportFailure : std::uint8_t {
    invalid_limits,
    resource_limit,
    invalid_syntax,
    authentication_required,
    origin_or_csrf_rejected,
    ingress_unavailable,
    crypto_unavailable
};
struct AccountHeaderView {
    std::string_view name, value;
};
struct AccountIngressView {
    std::string_view method, configured_origin, policy_revision_ref;
    std::span<const AccountHeaderView> fields;
};
struct AccountIngressUnavailable {};
using AccountIngressRead = std::variant<AccountIngressView, AccountIngressUnavailable>;
struct AccountTransportLimits {
    std::uint64_t field_count = 64, field_bytes = 8192;
    std::uint64_t header_bytes = 32768, cookie_pairs = 64;
    std::uint64_t origin_bytes = 1024, policy_reference_bytes = 4096;
    std::uint64_t result_bytes = 4096;
};
class AccountIngressSource {
  public:
    virtual ~AccountIngressSource() = default;
    virtual AccountIngressRead read_current(const AccountTransportLimits &) = 0;
};
struct PreparedTransportSession {
    AccountSessionDigest digest;
    AccountTransportKind transport;
    std::string policy_revision_ref;
};
struct BrowserSignInGuard {
    std::string policy_revision_ref;
};
using AccountTransportResult =
    std::variant<PreparedTransportSession, BrowserSignInGuard, AccountTransportFailure>;

inline AccountTransportResult prepare_account_transport(AccountIngressSource &,
                                                        AccountTransportPurpose,
                                                        const AccountTransportLimits & = {}) {
    return AccountTransportFailure::ingress_unavailable;
}
namespace testing {
using TransportSupplier = PreparedAccountSession (*)(void *, std::string_view);
inline AccountTransportResult prepare_transport(AccountIngressSource &, AccountTransportPurpose,
                                                const AccountTransportLimits &, TransportSupplier,
                                                void *) {
    return AccountTransportFailure::ingress_unavailable;
}
} // namespace testing
} // namespace subathon::server
