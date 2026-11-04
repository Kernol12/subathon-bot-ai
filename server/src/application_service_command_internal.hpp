// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/application_service.hpp>

namespace subathon::server::service_detail {
// Stored command semantics use the selected storage/intermediate profile. All
// configured-bound refusals are result_limit; malformed and unsupported meanings
// keep their distinct public exception categories. No caller bytes are borrowed.
[[nodiscard]] OriginalApplicationCommand decode_retained_command(const ApplicationEnvelope &,
                                                                 const ApplicationServiceLimits &);
} // namespace subathon::server::service_detail
