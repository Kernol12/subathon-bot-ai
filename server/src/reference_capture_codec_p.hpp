// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Source-private structural body boundary. The caller knows the document's role;
// no fabricated format reference is selected from storage or added to its limits.
#include <string_view>
#include <subathon/server/reference_capture_values.hpp>

namespace subathon::server::reference_capture_detail {
enum class BodyBounds { public_input, retained };
// Scan all borrowed framing/fields and complete DTO bounds before owning output.
// Retained maps configured byte overages to result_limit, preserving structural
// error categories; this never supplies admission, SQL or authority evidence.
[[nodiscard]] ReferenceConfiguration
decode_configuration_body(std::string_view, const ReferenceAdapterLimits &, BodyBounds);
[[nodiscard]] ReferenceArrivalCapture
decode_arrival_body(std::string_view, const ReferenceAdapterLimits &, BodyBounds);
[[nodiscard]] ReferenceExecutionBinding
decode_execution_body(std::string_view, const ReferenceAdapterLimits &, BodyBounds);
[[nodiscard]] ReferenceInitialCapture
decode_initial_body(std::string_view, const ReferenceAdapterLimits &, BodyBounds);
} // namespace subathon::server::reference_capture_detail
