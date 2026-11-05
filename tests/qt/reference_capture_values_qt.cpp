// SPDX-License-Identifier: GPL-3.0-or-later
// Structural data codecs only: no admission, SQL, evaluator or authority claim.
#include "../fixtures/reference_capture_codec_vectors.hpp"
#include <QTest>
#include <array>
#include <limits>
#include <subathon/server/reference_capture_values.hpp>

using namespace subathon::server;
namespace core = subathon::core;
namespace {
using Error = ReferenceInputError;
constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
ReferenceRule rule() {
    return {"R", "r", "a", "b", "T", 600000, {ApplicationPrincipalKind::automation, "I", "U"}, "K"};
}
ReferenceConfiguration configuration() {
    return {"C",
            "p",
            {"synthetic.reference.v1", "A"},
            "N",
            rule(),
            std::nullopt,
            {core::TimerEndMode::Immediate, 0, core::GraceReuse::Fresh, std::nullopt},
            ReferenceFactsState::complete,
            ReferenceFactsState::complete,
            ReferenceFactsState::complete,
            ReferenceFactsState::complete,
            ReferenceFactsState::complete};
}
ReferenceArrivalCapture arrival() {
    return {"X", "V", 0, configuration(), {"Q", "E", std::nullopt, QByteArray("")}};
}
ReferenceExecutionBinding execution() {
    return {"Y", "X", "I", {{"synthetic.reference.v1", "A"}, "N", "E"}, {7}, {0}, {"P", 1}, rule()};
}
ReferenceInitialCapture initial() {
    return {
        {"I", {{"synthetic.reference.v1", "A"}, "N", "E"}}, {7}, {0}, "C", "X", "V", {"P", 1}, "Y"};
}
ApplicationEnvelope literal(int kind) {
    static constexpr std::array formats{
        "subathon.reference.configuration.v1", "subathon.reference.arrival.v1",
        "subathon.reference.execution.v1", "subathon.reference.initial-capture.v1"};
    static constexpr std::array frames{
        reference_capture_vectors::configuration, reference_capture_vectors::arrival,
        reference_capture_vectors::execution, reference_capture_vectors::initial};
    return {formats.at(static_cast<size_t>(kind)),
            QByteArray::fromHex(frames.at(static_cast<size_t>(kind)))};
}
ApplicationEnvelope encode(int kind, const ReferenceAdapterLimits &limits = {}) {
    switch (kind) {
    case 0:
        return encode_reference_configuration(configuration(), limits);
    case 1:
        return encode_reference_arrival(arrival(), limits);
    case 2:
        return encode_reference_execution(execution(), limits);
    default:
        return encode_reference_initial_capture(initial(), limits);
    }
}
void decode(int kind, const ApplicationEnvelope &v, const ReferenceAdapterLimits &limits = {}) {
    switch (kind) {
    case 0:
        (void)decode_reference_configuration(v, limits);
        break;
    case 1:
        (void)decode_reference_arrival(v, limits);
        break;
    case 2:
        (void)decode_reference_execution(v, limits);
        break;
    default:
        (void)decode_reference_initial_capture(v, limits);
        break;
    }
}
// Each missing entrypoint is an explicit behavioral failure, never a skipped test.
template <class Body> void scenario(Body body) {
    try {
        body();
    } catch (const ReferenceCaptureValuesNotImplemented &) {
        QTest::qFail("MISSING_REFERENCE_CODEC: ReferenceCaptureValuesNotImplemented", __FILE__,
                     __LINE__);
    } catch (const std::exception &e) {
        QTest::qFail(e.what(), __FILE__, __LINE__);
    }
}
template <class Body> void failure(Body body, Error expected) {
    bool caught = false;
    try {
        body();
    } catch (const ReferenceInputFailure &e) {
        QCOMPARE(e.code(), expected);
        QCOMPARE(QByteArray(e.what()), QByteArray("Reference application input failed"));
        caught = true;
    }
    QVERIFY2(caught, "Expected exact public ReferenceInputFailure");
}
void sameRule(const ReferenceRule &a, const ReferenceRule &b) {
    QCOMPARE(a.rule_ref, b.rule_ref);
    QCOMPARE(a.rule_version_ref, b.rule_version_ref);
    QCOMPARE(a.action_ref, b.action_ref);
    QCOMPARE(a.action_version_ref, b.action_version_ref);
    QCOMPARE(a.timer_ref, b.timer_ref);
    QCOMPARE(a.add_ms, b.add_ms);
    QCOMPARE(a.automation.kind, b.automation.kind);
    QCOMPARE(a.automation.installation_ref, b.automation.installation_ref);
    QCOMPARE(a.automation.principal_ref, b.automation.principal_ref);
    QCOMPARE(a.automation_capability_ref, b.automation_capability_ref);
}
void sameConfig(const ReferenceConfiguration &a, const ReferenceConfiguration &b) {
    QCOMPARE(a.configuration_ref, b.configuration_ref);
    QCOMPARE(a.policy_revision_ref, b.policy_revision_ref);
    QVERIFY(a.synthetic_origin == b.synthetic_origin);
    QCOMPARE(a.receipt_namespace, b.receipt_namespace);
    sameRule(a.rule, b.rule);
    QVERIFY(a.maximum_main_ms == b.maximum_main_ms);
    QCOMPARE(a.timer_config.end_mode, b.timer_config.end_mode);
    QCOMPARE(a.timer_config.grace_duration_ms, b.timer_config.grace_duration_ms);
    QCOMPARE(a.timer_config.grace_reuse, b.timer_config.grace_reuse);
    QVERIFY(a.timer_config.hard_end_utc_ms == b.timer_config.hard_end_utc_ms);
    QCOMPARE(a.other_rules, b.other_rules);
    QCOMPARE(a.modifiers, b.modifiers);
    QCOMPARE(a.milestones, b.milestones);
    QCOMPARE(a.outbound_actions, b.outbound_actions);
    QCOMPARE(a.generated_events, b.generated_events);
}
void sameInitial(const ReferenceInitialCapture &a, const ReferenceInitialCapture &b) {
    QCOMPARE(a.key.installation_ref, b.key.installation_ref);
    QVERIFY(a.key.source == b.key.source);
    QCOMPARE(a.receipt_id.value, b.receipt_id.value);
    QCOMPARE(a.contribution_id.value, b.contribution_id.value);
    QCOMPARE(a.configuration_ref, b.configuration_ref);
    QCOMPARE(a.arrival_context_ref, b.arrival_context_ref);
    QCOMPARE(a.event_ref, b.event_ref);
    QCOMPARE(a.plan.plan_ref, b.plan.plan_ref);
    QCOMPARE(a.plan.revision, b.plan.revision);
    QCOMPARE(a.execution_context_ref, b.execution_context_ref);
}
} // namespace

class ReferenceCaptureValuesTest : public QObject {
    Q_OBJECT
  private slots:
    void configurationEncodeLiteral() {
        scenario([] {
            const auto a = encode(0), b = literal(0);
            QCOMPARE(a.format_ref, b.format_ref);
            QCOMPARE(a.document, b.document);
            QCOMPARE(a.document.size(), 116);
        });
    }
    void configurationDecodeLiteral() {
        scenario([] { sameConfig(decode_reference_configuration(literal(0)), configuration()); });
    }
    void arrivalEncodeLiteral() {
        scenario([] {
            const auto a = encode(1), b = literal(1);
            QCOMPARE(a.format_ref, b.format_ref);
            QCOMPARE(a.document, b.document);
            QCOMPARE(a.document.size(), 157);
        });
    }
    void arrivalDecodeLiteral() {
        scenario([] {
            const auto a = decode_reference_arrival(literal(1)), b = arrival();
            QCOMPARE(a.arrival_context_ref, b.arrival_context_ref);
            QCOMPARE(a.event_ref, b.event_ref);
            QCOMPARE(a.server_arrival_utc_ms, b.server_arrival_utc_ms);
            sameConfig(a.configuration, b.configuration);
            QCOMPARE(a.observation.connection_ref, b.observation.connection_ref);
            QCOMPARE(a.observation.event_id, b.observation.event_id);
            QVERIFY(!a.observation.source_utc_ms);
            QVERIFY(!a.observation.original_payload.isNull());
            QVERIFY(a.observation.original_payload.isEmpty());
        });
    }
    void executionEncodeLiteral() {
        scenario([] {
            const auto a = encode(2), b = literal(2);
            QCOMPARE(a.format_ref, b.format_ref);
            QCOMPARE(a.document, b.document);
            QCOMPARE(a.document.size(), 138);
        });
    }
    void executionDecodeLiteral() {
        scenario([] {
            const auto a = decode_reference_execution(literal(2)), b = execution();
            QCOMPARE(a.execution_context_ref, b.execution_context_ref);
            QCOMPARE(a.arrival_context_ref, b.arrival_context_ref);
            QCOMPARE(a.installation_ref, b.installation_ref);
            QVERIFY(a.source == b.source);
            QCOMPARE(a.receipt_id.value, 7U);
            QCOMPARE(a.contribution_id.value, 0U);
            QCOMPARE(a.plan.plan_ref, b.plan.plan_ref);
            QCOMPARE(a.plan.revision, 1U);
            sameRule(a.rule, b.rule);
        });
    }
    void initialEncodeLiteral() {
        scenario([] {
            const auto a = encode(3), b = literal(3);
            QCOMPARE(a.format_ref, b.format_ref);
            QCOMPARE(a.document, b.document);
            QCOMPARE(a.document.size(), 99);
        });
    }
    void initialDecodeLiteral() {
        scenario([] { sameInitial(decode_reference_initial_capture(literal(3)), initial()); });
    }

    void allLimitsValidateBeforeInput() {
        scenario([] {
            using Member = std::uint64_t ReferenceAdapterLimits::*;
            const std::array<Member, 6> members{&ReferenceAdapterLimits::reference_bytes,
                                                &ReferenceAdapterLimits::payload_bytes,
                                                &ReferenceAdapterLimits::document_bytes,
                                                &ReferenceAdapterLimits::value_owned_bytes,
                                                &ReferenceAdapterLimits::value_items,
                                                &ReferenceAdapterLimits::clock_journal_entries};
            for (auto member : members)
                for (auto bad : {std::uint64_t{0}, maximum}) {
                    ReferenceAdapterLimits limits;
                    limits.*member = bad;
                    for (int k = 0; k < 4; ++k) {
                        failure([&] { (void)encode(k, limits); }, Error::invalid_limits);
                        failure([&] { decode(k, {"", QByteArray()}, limits); },
                                Error::invalid_limits);
                    }
                }
        });
    }
    void encodedEnvelopeBoundsIncludeFormat() {
        scenario([] {
            // IC18-02: SRI1 DTO55, document99, outer format37: complete envelope136.
            ReferenceAdapterLimits l;
            l.value_owned_bytes = 136;
            l.document_bytes = 99;
            QCOMPARE(encode_reference_initial_capture(initial(), l).document, literal(3).document);
            l.value_owned_bytes = 135;
            failure([&] { (void)encode_reference_initial_capture(initial(), l); },
                    Error::result_limit);
            l.value_owned_bytes = 136;
            l.document_bytes = 98;
            failure([&] { (void)encode_reference_initial_capture(initial(), l); },
                    Error::result_limit);
            // IC18-01: ten one-byte references, record34, document78, envelope115.
            auto v = initial();
            v.key.source.origin.provider = "S";
            l.value_owned_bytes = 115;
            l.document_bytes = 78;
            QCOMPARE(encode_reference_initial_capture(v, l).document.size(), 78);
            l.value_owned_bytes = 114;
            failure([&] { (void)encode_reference_initial_capture(v, l); }, Error::result_limit);
        });
    }
    void borrowedInputBoundsPrecedeSemanticChecks() {
        scenario([] {
            auto v = initial();
            v.key.source.opaque_id = std::string(4097, 'x');
            v.receipt_id = {0};
            failure([&] { (void)encode_reference_initial_capture(v); }, Error::input_limit);
            v = initial();
            v.key.source.opaque_id = std::string(4096, 'x');
            auto a = encode_reference_initial_capture(v);
            QCOMPARE(decode_reference_initial_capture(a).key.source.opaque_id,
                     v.key.source.opaque_id);
            // Complete input B55 must be checked before the invalid Receipt0 domain.
            v = initial();
            v.receipt_id = {0};
            ReferenceAdapterLimits l;
            l.value_owned_bytes = 54;
            failure([&] { (void)encode_reference_initial_capture(v, l); }, Error::input_limit);
        });
    }
    void decoderOuterBoundsPrecedeFormatAndFraming() {
        scenario([] {
            for (int k = 0; k < 4; ++k) {
                auto a = literal(k);
                a.format_ref = "unknown";
                ReferenceAdapterLimits l;
                l.document_bytes = static_cast<std::uint64_t>(a.document.size() - 1);
                failure([&] { decode(k, a, l); }, Error::input_limit);
                l = {};
                l.value_owned_bytes =
                    static_cast<std::uint64_t>(a.document.size() + a.format_ref.size() - 1);
                failure([&] { decode(k, a, l); }, Error::input_limit);
                l = {};
                failure([&] { decode(k, a, l); }, Error::unsupported_format);
                a = literal(k);
                a.document.append('x');
                failure([&] { decode(k, a); }, Error::invalid_document);
            }
        });
    }
    void decoderFramingRejectsEveryTruncation() {
        scenario([] {
            for (int k = 0; k < 4; ++k) {
                const auto full = literal(k);
                for (qsizetype n = 0; n < full.document.size(); ++n) {
                    auto a = full;
                    a.document = n == 0 ? QByteArray("") : full.document.first(n);
                    failure([&] { decode(k, a); }, Error::invalid_document);
                }
                auto a = full;
                a.document[0] = 'X';
                failure([&] { decode(k, a); }, Error::invalid_document);
                a = full;
                a.document[4] = char(0xff);
                a.document[5] = char(0xff);
                a.document[6] = char(0xff);
                a.document[7] = char(0xff);
                failure([&] { decode(k, a); }, Error::invalid_document);
            }
        });
    }
    void nullDocumentsAndPayloadsAreNotEmpty() {
        scenario([] {
            auto v = arrival();
            v.observation.original_payload = QByteArray();
            failure([&] { (void)encode_reference_arrival(v); }, Error::invalid_shape);
            v.observation.original_payload = QByteArray("");
            QVERIFY(!decode_reference_arrival(encode_reference_arrival(v))
                         .observation.original_payload.isNull());
            for (int k = 0; k < 4; ++k) {
                auto a = literal(k);
                a.document = QByteArray();
                failure([&] { decode(k, a); }, Error::invalid_shape);
            }
        });
    }
    void payloadBoundariesRetainOpaqueBytes() {
        scenario([] {
            auto v = arrival();
            v.observation.original_payload = QByteArray(65536, '\0');
            v.observation.original_payload[1] = char(0xff);
            const auto a = encode_reference_arrival(v);
            QCOMPARE(decode_reference_arrival(a).observation.original_payload,
                     v.observation.original_payload);
            v.observation.original_payload.append('x');
            v.configuration.rule.add_ms = -1;
            failure([&] { (void)encode_reference_arrival(v); }, Error::input_limit);
        });
    }
    void knownUnknownFactsRemainStructuralData() {
        scenario([] {
            auto v = configuration();
            v.other_rules = ReferenceFactsState::unknown;
            v.generated_events = ReferenceFactsState::unknown;
            auto expected = literal(0);
            expected.document[111] = char(0);
            expected.document[115] = char(0);
            QCOMPARE(encode_reference_configuration(v).document, expected.document);
            sameConfig(decode_reference_configuration(expected), v);
        });
    }
    void invalidTagsPrecedeReferencesAndValues() {
        scenario([] {
            auto v = configuration();
            v.other_rules = static_cast<ReferenceFactsState>(2);
            v.configuration_ref.clear();
            v.rule.add_ms = -1;
            failure([&] { (void)encode_reference_configuration(v); }, Error::invalid_tag);
            v = configuration();
            v.timer_config.end_mode = static_cast<core::TimerEndMode>(3);
            failure([&] { (void)encode_reference_configuration(v); }, Error::invalid_tag);
            v = configuration();
            v.timer_config.grace_reuse = static_cast<core::GraceReuse>(2);
            failure([&] { (void)encode_reference_configuration(v); }, Error::invalid_tag);
            auto e = execution();
            e.rule.automation.kind = static_cast<ApplicationPrincipalKind>(2);
            failure([&] { (void)encode_reference_execution(e); }, Error::invalid_tag);
            e.rule.automation.kind = ApplicationPrincipalKind::account;
            failure([&] { (void)encode_reference_execution(e); }, Error::invalid_binding);
        });
    }
    void everyRequiredReferenceIsChecked() {
        scenario([] {
            auto v = initial();
            std::array<std::string *, 10> refs{&v.key.installation_ref,
                                               &v.key.source.origin.provider,
                                               &v.key.source.origin.source_account,
                                               &v.key.source.identifier_namespace,
                                               &v.key.source.opaque_id,
                                               &v.configuration_ref,
                                               &v.arrival_context_ref,
                                               &v.event_ref,
                                               &v.plan.plan_ref,
                                               &v.execution_context_ref};
            for (auto *p : refs) {
                auto saved = *p;
                p->clear();
                failure([&] { (void)encode_reference_initial_capture(v); },
                        Error::invalid_reference);
                *p = saved;
            }
            auto c = configuration();
            c.policy_revision_ref.clear();
            c.rule.add_ms = -1;
            failure([&] { (void)encode_reference_configuration(c); }, Error::invalid_reference);
            c = configuration();
            c.rule.automation_capability_ref.clear();
            failure([&] { (void)encode_reference_configuration(c); }, Error::invalid_reference);
            auto a = arrival();
            a.observation.connection_ref.clear();
            failure([&] { (void)encode_reference_arrival(a); }, Error::invalid_reference);
        });
    }
    void configurationNumericAndTimerBoundaries() {
        scenario([] {
            auto v = configuration();
            v.rule.add_ms = -1;
            failure([&] { (void)encode_reference_configuration(v); }, Error::invalid_value);
            v = configuration();
            v.maximum_main_ms = -1;
            failure([&] { (void)encode_reference_configuration(v); }, Error::invalid_value);
            v = configuration();
            v.timer_config.grace_duration_ms = -1;
            failure([&] { (void)encode_reference_configuration(v); }, Error::invalid_value);
            v = configuration();
            v.rule.add_ms = 0;
            v.maximum_main_ms = 0;
            v.timer_config = {core::TimerEndMode::Confirmation,
                              std::numeric_limits<std::int64_t>::max(), core::GraceReuse::Shared,
                              std::numeric_limits<std::int64_t>::min()};
            auto a = encode_reference_configuration(v);
            auto b = decode_reference_configuration(a);
            sameConfig(b, v);
            // Present cap0 and signed MIN deadline remain present in canonical optional groups.
            QCOMPARE(a.document.mid(99, 9).toHex(), QByteArray("010000000000000000"));
            QCOMPARE(a.document.mid(118, 9).toHex(), QByteArray("018000000000000000"));
        });
    }
    void arrivalTimesPreserveSignedFullRange() {
        scenario([] {
            auto v = arrival();
            v.server_arrival_utc_ms = std::numeric_limits<std::int64_t>::min();
            v.observation.source_utc_ms = std::numeric_limits<std::int64_t>::max();
            auto a = encode_reference_arrival(v);
            QCOMPARE(a.document.mid(14, 8).toHex(), QByteArray("8000000000000000"));
            QCOMPARE(a.document.mid(152, 9).toHex(), QByteArray("017fffffffffffffff"));
            auto b = decode_reference_arrival(a);
            QCOMPARE(b.server_arrival_utc_ms, v.server_arrival_utc_ms);
            QVERIFY(b.observation.source_utc_ms == v.observation.source_utc_ms);
        });
    }
    void executionUnsignedDomainsAndInstallationBinding() {
        scenario([] {
            auto v = execution();
            v.receipt_id = {maximum};
            v.contribution_id = {maximum};
            v.plan.revision = maximum;
            auto expected = literal(2);
            expected.document.replace(60, 16, QByteArray(16, char(0xff)));
            expected.document.replace(81, 8, QByteArray(8, char(0xff)));
            QCOMPARE(encode_reference_execution(v).document, expected.document);
            auto b = decode_reference_execution(expected);
            QCOMPARE(b.receipt_id.value, maximum);
            QCOMPARE(b.contribution_id.value, maximum);
            QCOMPARE(b.plan.revision, maximum);
            v = execution();
            v.receipt_id = {0};
            failure([&] { (void)encode_reference_execution(v); }, Error::invalid_value);
            v = execution();
            v.plan.revision = 0;
            failure([&] { (void)encode_reference_execution(v); }, Error::invalid_value);
            v = execution();
            v.installation_ref = "J";
            failure([&] { (void)encode_reference_execution(v); }, Error::invalid_binding);
        });
    }
    void initialRequiresRevisionOneAndPositiveReceipt() {
        scenario([] {
            auto v = initial();
            v.receipt_id = {maximum};
            v.contribution_id = {maximum};
            auto expected = literal(3);
            expected.document.replace(50, 16, QByteArray(16, char(0xff)));
            QCOMPARE(encode_reference_initial_capture(v).document, expected.document);
            sameInitial(decode_reference_initial_capture(expected), v);
            for (auto revision : {std::uint64_t{0}, std::uint64_t{2}, maximum}) {
                v = initial();
                v.plan.revision = revision;
                failure([&] { (void)encode_reference_initial_capture(v); }, Error::invalid_value);
            }
            v = initial();
            v.receipt_id = {0};
            failure([&] { (void)encode_reference_initial_capture(v); }, Error::invalid_value);
        });
    }
    void framedSemanticFailuresUseExactPriority() {
        scenario([] {
            auto a = literal(0);
            a.document[111] = char(2);
            a.document.replace(75, 8, QByteArray(8, char(0xff)));
            failure([&] { (void)decode_reference_configuration(a); }, Error::invalid_tag);
            a = literal(0);
            a.document[83] = char(0);
            failure([&] { (void)decode_reference_configuration(a); }, Error::invalid_binding);
            a = literal(0);
            a.document[99] = char(2);
            failure([&] { (void)decode_reference_configuration(a); }, Error::invalid_document);
            a = literal(2);
            a.document[127] = 'J';
            failure([&] { (void)decode_reference_execution(a); }, Error::invalid_binding);
            a = literal(3);
            a.document.replace(50, 8, QByteArray(8, '\0'));
            failure([&] { (void)decode_reference_initial_capture(a); }, Error::invalid_value);
            a = literal(3);
            a.document[93] = char(2);
            failure([&] { (void)decode_reference_initial_capture(a); }, Error::invalid_value);
        });
    }
    void nestedConfigurationIsValidatedWithinArrival() {
        scenario([] {
            auto a = literal(1);
            a.document[26] = 'X';
            failure([&] { (void)decode_reference_arrival(a); }, Error::invalid_document);
            a = literal(1);
            a.document[26 + 111] = char(2);
            failure([&] { (void)decode_reference_arrival(a); }, Error::invalid_tag);
            auto v = arrival();
            v.configuration.timer_config.grace_duration_ms = -1;
            failure([&] { (void)encode_reference_arrival(v); }, Error::invalid_value);
        });
    }
    void completeStructuralInputsUseEveryScalar() {
        scenario([] {
            // Semantic B: config60 (refs34+scalars/options/tags26), arrival73, execution70.
            // At complete input bound only framed OUTPUT fails; one under fails INPUT.
            const std::array<std::uint64_t, 3> bytes{60, 73, 70};
            for (int k = 0; k < 3; ++k) {
                ReferenceAdapterLimits l;
                l.value_owned_bytes = bytes[static_cast<size_t>(k)];
                failure([&] { (void)encode(k, l); }, Error::result_limit);
                --l.value_owned_bytes;
                failure([&] { (void)encode(k, l); }, Error::input_limit);
            }
        });
    }
    void decodedFieldsRespectReferenceAndPayloadBounds() {
        scenario([] {
            for (int k = 0; k < 4; ++k) {
                auto a = literal(k);
                // All formats <=37. A fully framed 38-byte FIRST reference exceeds
                // this profile, even though its outer envelope fits comfortably.
                a.document.replace(4, 5, QByteArray::fromHex("00000026") + QByteArray(38, 'x'));
                ReferenceAdapterLimits l;
                l.reference_bytes = 37;
                failure([&] { decode(k, a, l); }, Error::input_limit);
                a = literal(k);
                a.document.replace(4, 5, QByteArray::fromHex("00000000"));
                failure([&] { decode(k, a); }, Error::invalid_reference);
            }
            auto a = literal(1);
            a.document.replace(153, 4, QByteArray::fromHex("00010001") + QByteArray(65537, 'x'));
            failure([&] { (void)decode_reference_arrival(a); }, Error::input_limit);
        });
    }
    void exactOpaqueReferencesAndOwnedResults() {
        scenario([] {
            auto v = initial();
            v.key.source.opaque_id = std::string("E\0\xff", 3);
            auto encoded = encode_reference_initial_capture(v);
            auto expected = literal(3);
            expected.document.replace(45, 5, QByteArray::fromHex("000000034500ff"));
            QCOMPARE(encoded.document, expected.document);
            auto a = decode_reference_initial_capture(expected);
            expected.document.fill('x');
            v.key.source.opaque_id = "changed";
            QCOMPARE(a.key.source.opaque_id, std::string("E\0\xff", 3));
            QCOMPARE(a.configuration_ref, std::string("C"));
            auto source = initial();
            auto b = encode_reference_initial_capture(source);
            source.configuration_ref = "changed";
            QCOMPARE(b.document, literal(3).document);
        });
    }
};
QTEST_GUILESS_MAIN(ReferenceCaptureValuesTest)
#include "reference_capture_values_qt.moc"
