// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QByteArray>
#include <cstdint>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <subathon/core/receipt.hpp>
#include <subathon/core/timer_lifecycle.hpp>
#include <subathon/server/application_service.hpp>
#include <subathon/server/sqlite_application_bundle.hpp>

namespace subathon::server {
// Shared owned structural DATA from the reviewed reference consumer and capture
// contracts. Decoding does not establish admission, authority or durable state.
enum class ReferenceFactsState : std::uint8_t { unknown, complete };
struct ReferenceRule {
    std::string rule_ref;
    std::string rule_version_ref;
    std::string action_ref;
    std::string action_version_ref;
    std::string timer_ref;
    std::int64_t add_ms = 0;
    ApplicationPrincipalBinding automation;
    std::string automation_capability_ref;
};
struct ReferenceConfiguration {
    std::string configuration_ref;
    std::string policy_revision_ref;
    core::ReceiptOrigin synthetic_origin;
    std::string receipt_namespace;
    ReferenceRule rule;
    std::optional<std::int64_t> maximum_main_ms;
    core::TimerConfig timer_config;
    // Admission requires each category to be explicitly complete and empty.
    ReferenceFactsState other_rules = ReferenceFactsState::unknown;
    ReferenceFactsState modifiers = ReferenceFactsState::unknown;
    ReferenceFactsState milestones = ReferenceFactsState::unknown;
    ReferenceFactsState outbound_actions = ReferenceFactsState::unknown;
    ReferenceFactsState generated_events = ReferenceFactsState::unknown;
};
struct ReferenceSyntheticInput {
    std::string connection_ref;
    std::string event_id;
    std::optional<std::int64_t> source_utc_ms;
    QByteArray original_payload;
};
struct ReferenceArrivalCapture {
    std::string arrival_context_ref;
    std::string event_ref;
    std::int64_t server_arrival_utc_ms = 0;
    ReferenceConfiguration configuration;
    ReferenceSyntheticInput observation;
};
struct ReferenceExecutionBinding {
    std::string execution_context_ref;
    std::string arrival_context_ref;
    std::string installation_ref;
    core::ReceiptIdentity source;
    core::ReceiptId receipt_id{0};
    core::ContributionId contribution_id{0};
    ApplicationPlanKey plan;
    ReferenceRule rule;
};
struct ReferenceCaptureKey {
    std::string installation_ref;
    core::ReceiptIdentity source;
};
struct ReferenceInitialCapture {
    ReferenceCaptureKey key;
    core::ReceiptId receipt_id{0};
    core::ContributionId contribution_id{0};
    std::string configuration_ref;
    std::string arrival_context_ref;
    std::string event_ref;
    ApplicationPlanKey plan;
    std::string execution_context_ref;
};
struct ReferenceAdapterLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t payload_bytes = 65536;
    std::uint64_t document_bytes = 262144;
    std::uint64_t value_owned_bytes = 1048576;
    std::uint64_t value_items = 4096;
    std::uint64_t clock_journal_entries = 64;
};
enum class ReferenceInputError : std::uint8_t {
    invalid_limits,
    input_limit,
    invalid_tag,
    invalid_reference,
    invalid_shape,
    invalid_value,
    invalid_binding,
    unsupported_format,
    invalid_document,
    result_limit,
    allocation_failure
};
class ReferenceInputFailure final : public std::exception {
  public:
    explicit ReferenceInputFailure(ReferenceInputError code) noexcept : code_(code) {}
    [[nodiscard]] ReferenceInputError code() const noexcept { return code_; }
    [[nodiscard]] const char *what() const noexcept override {
        return "Reference application input failed";
    }

  private:
    ReferenceInputError code_;
};
class ReferenceCaptureValuesNotImplemented final : public std::logic_error {
  public:
    ReferenceCaptureValuesNotImplemented()
        : std::logic_error(
              "Reference capture values are intentionally unimplemented; review tests first") {}
};

// SRC1/SRA1/SRX1/SRI1 structural codecs preserve canonical owned data under
// explicit limits. They provide no admission, authority or durable-state proof.
[[nodiscard]] ApplicationEnvelope
encode_reference_configuration(const ReferenceConfiguration &, const ReferenceAdapterLimits & = {});
[[nodiscard]] ReferenceConfiguration
decode_reference_configuration(const ApplicationEnvelope &, const ReferenceAdapterLimits & = {});
[[nodiscard]] ApplicationEnvelope encode_reference_arrival(const ReferenceArrivalCapture &,
                                                           const ReferenceAdapterLimits & = {});
[[nodiscard]] ReferenceArrivalCapture decode_reference_arrival(const ApplicationEnvelope &,
                                                               const ReferenceAdapterLimits & = {});
[[nodiscard]] ApplicationEnvelope encode_reference_execution(const ReferenceExecutionBinding &,
                                                             const ReferenceAdapterLimits & = {});
[[nodiscard]] ReferenceExecutionBinding
decode_reference_execution(const ApplicationEnvelope &, const ReferenceAdapterLimits & = {});
[[nodiscard]] ApplicationEnvelope
encode_reference_initial_capture(const ReferenceInitialCapture &,
                                 const ReferenceAdapterLimits & = {});
[[nodiscard]] ReferenceInitialCapture
decode_reference_initial_capture(const ApplicationEnvelope &, const ReferenceAdapterLimits & = {});
} // namespace subathon::server
