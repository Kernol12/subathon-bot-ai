// SPDX-License-Identifier: GPL-3.0-or-later
#include "intake_recovery_codec.hpp"
#include "modifier_capture_codec.hpp"

#include <QtEndian>
#include <array>
#include <subathon/core/contribution.hpp>
#include <subathon/server/reference_capture_values.hpp>

namespace subathon::server::connector_intake::recovery::detail {
namespace {

struct InvalidDocument {};

constexpr std::array lanes{IntakeAdmission::creation_eligible_verified_support,
                           IntakeAdmission::audit_only, IntakeAdmission::confirmation_only};
constexpr std::array policies{core::EvidencePolicy::require_evidence_review,
                              core::EvidencePolicy::accept_provider_report,
                              core::EvidencePolicy::record_only};
constexpr std::array<std::optional<HistoryPurpose>, 3> origins{
    std::nullopt, HistoryPurpose::catch_up, HistoryPurpose::import};

class Reader {
  public:
    explicit Reader(QByteArrayView input) : input_(input) {}

    QByteArrayView take(qsizetype count) {
        if (count > input_.size())
            throw InvalidDocument{};
        const auto part = input_.first(count);
        input_ = input_.sliced(count);
        return part;
    }
    unsigned tag(unsigned maximum) {
        const auto value = static_cast<unsigned char>(take(1).front());
        if (value > maximum)
            throw InvalidDocument{};
        return value;
    }
    template <typename Value, std::size_t Size>
    Value selected(const std::array<Value, Size> &tags) {
        return tags[tag(Size - 1)];
    }
    QByteArray field() {
        const auto size = qFromBigEndian<quint32>(take(4).data());
        if (size == 0 || size > 4096)
            throw InvalidDocument{};
        return take(size).toByteArray();
    }
    qint64 integer() { return qFromBigEndian<qint64>(take(8).data()); }
    bool empty() const { return input_.isEmpty(); }

  private:
    QByteArrayView input_;
};

} // namespace

bool current_capture(const QByteArray &bytes) {
    return bytes.startsWith(QByteArrayView("ARC1\1", 5));
}

std::optional<QByteArray> detail_reference(const QByteArray &bytes) {
    if (!current_capture(bytes) || bytes.size() > capture_limit)
        return {};
    try {
        Reader reader(QByteArrayView(bytes).sliced(5));
        auto reference = reader.field();
        if (reader.empty())
            return reference;
    } catch (const InvalidDocument &) {
    }
    return {};
}

bool configuration_role(const QByteArray &document) {
    for (const auto *magic : {"MCA1", "ARC1", "ARD1", "SRA1", "SRI1", "SRX1"}) {
        if (document.startsWith(magic))
            return false;
    }
    return true;
}

bool matches_detail(const Closure &closure, std::optional<HistoryPurpose> purpose,
                    const QByteArray &policy) {
    const auto &arrival = closure.attempt.arrival;
    const auto &detail = closure.detail;
    if (detail.key.kind != RetainedKind::arrival_context ||
        detail_reference(closure.capture) != QByteArray::fromStdString(detail.key.reference) ||
        detail.key.reference == arrival.modifier_context_ref ||
        detail.configuration != ConfigurationReference{arrival.configuration_ref} ||
        closure.configuration.key !=
            RetainedKey{RetainedKind::configuration, arrival.configuration_ref} ||
        closure.modifier_arrival.key !=
            RetainedKey{RetainedKind::arrival_context, arrival.modifier_context_ref} ||
        closure.modifier_arrival.configuration !=
            ConfigurationReference{arrival.configuration_ref} ||
        !configuration_role(closure.configuration.document) ||
        !detail.document.startsWith(QByteArrayView("ARD1\1", 5)) ||
        detail.document.size() > detail_limit)
        return false;
    try {
        core::ModifierStackLimits limits{262144, 262144, 65536};
        const auto modifier = modifier_capture_detail::decode(closure.modifier_arrival, false, limits);
        if (modifier.identity != closure.identity || modifier.arrival != arrival)
            return false;

        Reader reader(QByteArrayView(detail.document).sliced(5));
        const Scope scope{reader.field(), reader.field(), reader.field(), reader.field()};
        if (scope != closure.scope || reader.field() != closure.intake_ref ||
            reader.field().toStdString() != arrival.configuration_ref ||
            reader.field().toStdString() != arrival.modifier_context_ref ||
            reader.integer() != arrival.server_timestamp_utc_ms)
            return false;

        std::optional<VerifiedActivityLink> activity;
        if (reader.tag(1)) {
            const auto ns = reader.field().toStdString();
            const auto id = reader.field().toStdString();
            activity = {{closure.identity.origin, ns, id}, reader.field().toStdString()};
        }
        const auto connection = reader.tag(1) ? reader.field().toStdString() : std::string{};
        std::optional<std::int64_t> source;
        if (reader.tag(2))
            source = reader.integer();
        if (activity != closure.activity || connection != arrival.connection_ref ||
            source != closure.attempt.observed_facts.source_timestamp_utc_ms)
            return false;

        const auto classification = reader.selected(lanes);
        if (classification != closure.admission)
            return false;
        (void)reader.selected(policies); // Evidence authority belongs to its separate owner turn.
        if (classification == IntakeAdmission::creation_eligible_verified_support) {
            const auto origin = reader.selected(origins);
            if (origin != purpose)
                return false;
            if (origin == HistoryPurpose::catch_up) {
                constexpr std::array<QByteArrayView, 3> modes{
                    "review", "automatic", "record_without_credit"};
                Reader admitted(policy);
                const auto mode = admitted.field();
                (void)admitted.field(); // Startup configuration revision.
                (void)admitted.field(); // Startup policy permit decision.
                if (!admitted.empty() || mode != modes[reader.tag(modes.size() - 1)])
                    return false;
            }
        }
        (void)reader.field(); // Historical authority decision reference.
        (void)reader.field(); // Historical authority reason.
        if (reader.tag(1)) {
            if (classification != IntakeAdmission::creation_eligible_verified_support)
                return false;
            const auto rule = reader.field().toStdString();
            const auto revision = reader.field().toStdString();
            const auto decoded = decode_reference_configuration(
                {"subathon.reference.configuration.v1", closure.configuration.document});
            if (decoded.configuration_ref != arrival.configuration_ref ||
                decoded.rule.rule_ref != rule || decoded.rule.rule_version_ref != revision)
                return false;
        }

        const auto valuation = reader.tag(2);
        const bool has_revision = reader.tag(1);
        if (has_revision)
            (void)reader.field();
        const bool has_resolution = reader.tag(1);
        if (has_resolution)
            (void)reader.field();
        return reader.empty() && has_revision == (valuation != 0) &&
               has_resolution == (valuation == 2);
    } catch (const InvalidDocument &) {
        return false;
    } catch (ModifierCaptureRefusal) {
        return false;
    } catch (const ReferenceInputFailure &) {
        return false;
    }
}

} // namespace subathon::server::connector_intake::recovery::detail
