// SPDX-License-Identifier: GPL-3.0-or-later
#include <QTest>
#include <set>
#include <subathon/server/connector_lifecycle.hpp>
#include <subathon/server/testing/connector_lifecycle_not_implemented.hpp>

namespace {
using namespace subathon;
using namespace subathon::server;
using Support = ConnectorSupport;
using Feature = ConnectorFeature;
using Field = ConnectorEventField;
using Identifier = ConnectorIdentifier;

class FakeConnector final : public ConnectorAdapter {
  public:
    ConnectorReport snapshot;

    ConnectorReport report() const override { return snapshot; }
};

// Keep each NI diagnostic attached to its Qt invocation. Other fixture or
// product exceptions are ordinary test failures.
template <class Test> void scenario(Test test) {
    try {
        test();
    } catch (const ConnectorLifecycleNotImplemented &error) {
        QTest::qFail(error.what(), __FILE__, __LINE__);
    } catch (const std::exception &error) {
        QTest::qFail(error.what(), __FILE__, __LINE__);
    }
}

const core::ReceiptOrigin source{"synthetic-provider", "source-one"};
const core::ReceiptArrival arrival{2000, "connection-two", "config-one", "context-one"};

class ConnectorLifecycleTests final : public QObject {
    Q_OBJECT

  private slots:
    void releaseCatalog();
    void eventSupport();
    void fieldSupport();
    void featureSupport();
    void missingRegistration();
    void browserLink_data();
    void browserLink();
    void consentIsAccountSpecific();
    void alternateSetup_data();
    void alternateSetup();
    void reconnectKeepsGapSeparate();
    void missingEventFactsStayMissing();
    void knownZeroAndEvidenceArePreserved();
    void identifiersAreScopedAndRoleSpecific();
    void refundTargetIsOnlyAReference();
};

void ConnectorLifecycleTests::releaseCatalog() {
    scenario([] {
        using P = ConnectorProvider;
        const std::set expected{P::twitch,         P::betterplace, P::patreon,     P::streamlabs,
                                P::streamelements, P::kofi,        P::tipeeestream};
        const auto actual = connector_providers();
        QCOMPARE(actual.size(), expected.size());
        QVERIFY((std::set(actual.begin(), actual.end()) == expected));
    });
}

void ConnectorLifecycleTests::eventSupport() {
    scenario([] {
        ConnectorCapabilities capabilities;
        capabilities.events = {{"individual-support", {Support::supported, {}}},
                               {"membership-ended", {Support::unsupported, {}}}};
        QCOMPARE(capabilities.event_support("individual-support"), Support::supported);
        QCOMPARE(capabilities.event_support("membership-ended"), Support::unsupported);
        QCOMPARE(capabilities.event_support("unlisted-event"), Support::unknown);
    });
}

void ConnectorLifecycleTests::fieldSupport() {
    scenario([] {
        ConnectorCapabilities capabilities;
        capabilities.events = {
            {"aggregate-observation",
             {Support::supported,
              {{Field::money, Support::supported}, {Field::actor_id, Support::unsupported}}}},
            {"individual-support", {Support::supported, {{Field::actor_id, Support::supported}}}}};
        QCOMPARE(capabilities.field_support("aggregate-observation", Field::money),
                 Support::supported);
        QCOMPARE(capabilities.field_support("aggregate-observation", Field::actor_id),
                 Support::unsupported);
        QCOMPARE(capabilities.field_support("individual-support", Field::actor_id),
                 Support::supported);
        QCOMPARE(capabilities.field_support("aggregate-observation", Field::payment_id),
                 Support::unknown);
        QCOMPARE(capabilities.field_support("unlisted-event", Field::money), Support::unknown);
    });
}

void ConnectorLifecycleTests::featureSupport() {
    scenario([] {
        ConnectorCapabilities capabilities;
        capabilities.features = {{Feature::history, Support::supported},
                                 {Feature::gap_replay, Support::unsupported},
                                 {Feature::refunds, Support::unknown},
                                 {Feature::api_key_setup, Support::supported},
                                 {Feature::browser_link, Support::unsupported},
                                 {Feature::send_chat, Support::unknown}};
        QCOMPARE(capabilities.feature_support(Feature::history), Support::supported);
        QCOMPARE(capabilities.feature_support(Feature::gap_replay), Support::unsupported);
        QCOMPARE(capabilities.feature_support(Feature::refunds), Support::unknown);
        QCOMPARE(capabilities.feature_support(Feature::api_key_setup), Support::supported);
        QCOMPARE(capabilities.feature_support(Feature::browser_link), Support::unsupported);
        QCOMPARE(capabilities.feature_support(Feature::public_client_setup), Support::unknown);
        QCOMPARE(capabilities.feature_support(Feature::send_chat), Support::unknown);

        ConnectorCapabilities anotherProfile;
        anotherProfile.features = {{Feature::refunds, Support::supported},
                                   {Feature::send_chat, Support::supported}};
        QCOMPARE(anotherProfile.feature_support(Feature::refunds), Support::supported);
        QCOMPARE(anotherProfile.feature_support(Feature::history), Support::unknown);
        QCOMPARE(anotherProfile.feature_support(Feature::send_chat), Support::supported);
    });
}

void ConnectorLifecycleTests::missingRegistration() {
    scenario([] {
        FakeConnector adapter;
        adapter.snapshot.installation = ConnectorInstallation::missing;
        adapter.snapshot.account_consent = ConnectorConsent::granted;
        adapter.snapshot.connection = ConnectorConnection::connected;
        adapter.snapshot.capabilities.features = {
            {Feature::application_registration, Support::supported},
            {Feature::browser_link, Support::supported},
            {Feature::public_client_setup, Support::supported}};
        const ConnectorSetupContext context{
            std::string{"https://installation.invalid/oauth/return"}};

        const auto &[setup, health, gap] = read_connector_status(adapter, context);
        const auto &[installation, accountConsent, registration, apiKey, webhook, callback,
                     browserLink] = setup;
        QCOMPARE(installation, ConnectorInstallation::missing);
        QCOMPARE(accountConsent, ConnectorConsent::granted);
        QCOMPARE(registration, Support::supported);
        QCOMPARE(apiKey, Support::unknown);
        QCOMPARE(webhook, Support::unknown);
        QCOMPARE(callback, std::optional<std::string>{"https://installation.invalid/oauth/return"});
        QCOMPARE(browserLink, ConnectorLink::needs_setup);
        QCOMPARE(health, ConnectorHealth::setup_required);
        QCOMPARE(gap, ConnectorGap::unknown);
    });
}

void ConnectorLifecycleTests::browserLink_data() {
    QTest::addColumn<Support>("support");
    QTest::addColumn<ConnectorInstallation>("installation");
    QTest::addColumn<ConnectorLink>("expected");
    using I = ConnectorInstallation;
    using L = ConnectorLink;
    QTest::newRow("unverified-method") << Support::unknown << I::validated << L::unknown;
    QTest::newRow("unverified-method-missing-installation")
        << Support::unknown << I::missing << L::unknown;
    QTest::newRow("confirmed-unsupported")
        << Support::unsupported << I::validated << L::unsupported;
    QTest::newRow("unverified-installation") << Support::supported << I::unknown << L::unknown;
    QTest::newRow("validated-installation") << Support::supported << I::validated << L::available;
    QTest::newRow("no-registration-required")
        << Support::supported << I::not_required << L::available;
}

void ConnectorLifecycleTests::browserLink() {
    QFETCH(Support, support);
    QFETCH(ConnectorInstallation, installation);
    QFETCH(ConnectorLink, expected);
    scenario([&] {
        FakeConnector adapter;
        adapter.snapshot.installation = installation;
        adapter.snapshot.account_consent = ConnectorConsent::missing;
        adapter.snapshot.capabilities.features = {{Feature::browser_link, support}};
        const auto status = read_connector_status(adapter, {});
        QCOMPARE(status.setup.browser_link, expected);
        QCOMPARE(status.setup.account_consent, ConnectorConsent::missing);
        QVERIFY(!status.setup.callback_url);
    });
}

void ConnectorLifecycleTests::consentIsAccountSpecific() {
    scenario([] {
        FakeConnector firstAccount;
        firstAccount.snapshot.installation = ConnectorInstallation::validated;
        firstAccount.snapshot.capabilities.features = {
            {Feature::application_registration, Support::supported},
            {Feature::browser_link, Support::supported}};
        firstAccount.snapshot.account_consent = ConnectorConsent::granted;
        FakeConnector secondAccount = firstAccount;
        secondAccount.snapshot.account_consent = ConnectorConsent::missing;
        const ConnectorSetupContext context{std::string{"https://installation.invalid/connect"}};

        const auto first = read_connector_status(firstAccount, context);
        const auto second = read_connector_status(secondAccount, context);
        QCOMPARE(first.setup.installation, ConnectorInstallation::validated);
        QCOMPARE(second.setup.installation, ConnectorInstallation::validated);
        QCOMPARE(first.setup.account_consent, ConnectorConsent::granted);
        QCOMPARE(second.setup.account_consent, ConnectorConsent::missing);
        QCOMPARE(second.setup.browser_link, ConnectorLink::available);
        QCOMPARE(second.setup.callback_url,
                 std::optional<std::string>{"https://installation.invalid/connect"});
    });
}

void ConnectorLifecycleTests::alternateSetup_data() {
    QTest::addColumn<Support>("apiKey");
    QTest::addColumn<Support>("webhook");
    QTest::newRow("api-key") << Support::supported << Support::unsupported;
    QTest::newRow("webhook") << Support::unsupported << Support::supported;
}

void ConnectorLifecycleTests::alternateSetup() {
    QFETCH(Support, apiKey);
    QFETCH(Support, webhook);
    scenario([&] {
        FakeConnector adapter;
        adapter.snapshot.installation = ConnectorInstallation::missing;
        adapter.snapshot.account_consent = ConnectorConsent::not_required;
        adapter.snapshot.capabilities.features = {
            {Feature::application_registration, Support::unsupported},
            {Feature::api_key_setup, apiKey},
            {Feature::webhook_setup, webhook},
            {Feature::browser_link, Support::unsupported}};
        // A callback supplied for webhook setup belongs to this installation.
        const ConnectorSetupContext context{
            webhook == Support::supported
                ? std::optional<std::string>{"https://installation.invalid/webhooks/source-one"}
                : std::nullopt};
        const auto status = read_connector_status(adapter, context);
        QCOMPARE(status.setup.application_registration, Support::unsupported);
        QCOMPARE(status.setup.api_key_setup, apiKey);
        QCOMPARE(status.setup.webhook_setup, webhook);
        QCOMPARE(status.setup.browser_link, ConnectorLink::unsupported);
        QCOMPARE(status.setup.account_consent, ConnectorConsent::not_required);
        QCOMPARE(status.setup.callback_url, context.installation_callback_url);
        QCOMPARE(status.health, ConnectorHealth::setup_required);
    });
}

void ConnectorLifecycleTests::reconnectKeepsGapSeparate() {
    scenario([] {
        FakeConnector adapter;
        adapter.snapshot.installation = ConnectorInstallation::validated;
        const auto unknown = read_connector_status(adapter, {});
        QCOMPARE(unknown.health, ConnectorHealth::unknown);
        QCOMPARE(unknown.gap, ConnectorGap::unknown);
        QCOMPARE(unknown.setup.account_consent, ConnectorConsent::unknown);

        adapter.snapshot.connection = ConnectorConnection::connected;
        const auto connected = read_connector_status(adapter, {});
        QCOMPARE(connected.health, ConnectorHealth::connected);
        QCOMPARE(connected.gap, ConnectorGap::unknown);

        adapter.snapshot.gap = ConnectorGap::none;
        const auto connectedWithoutGap = read_connector_status(adapter, {});
        QCOMPARE(connectedWithoutGap.health, ConnectorHealth::connected);
        QCOMPARE(connectedWithoutGap.gap, ConnectorGap::none);

        adapter.snapshot.connection = ConnectorConnection::disconnected;
        adapter.snapshot.gap = ConnectorGap::open;
        const auto disconnected = read_connector_status(adapter, {});
        QCOMPARE(disconnected.health, ConnectorHealth::disconnected);
        QCOMPARE(disconnected.gap, ConnectorGap::open);

        adapter.snapshot.connection = ConnectorConnection::connecting;
        const auto connecting = read_connector_status(adapter, {});
        QCOMPARE(connecting.health, ConnectorHealth::connecting);
        QCOMPARE(connecting.gap, ConnectorGap::open);

        adapter.snapshot.connection = ConnectorConnection::connected;
        const auto reconnected = read_connector_status(adapter, {});
        QCOMPARE(reconnected.health, ConnectorHealth::connected);
        QCOMPARE(reconnected.gap, ConnectorGap::open);

        FakeConnector withoutRecovery = adapter;
        withoutRecovery.snapshot.gap = ConnectorGap::unrecoverable;
        const auto unrecoverable = read_connector_status(withoutRecovery, {});
        QCOMPARE(unrecoverable.health, ConnectorHealth::connected);
        QCOMPARE(unrecoverable.gap, ConnectorGap::unrecoverable);

        adapter.snapshot.gap = ConnectorGap::recovered;
        const auto reconciled = read_connector_status(adapter, {});
        QCOMPARE(reconciled.health, ConnectorHealth::connected);
        QCOMPARE(reconciled.gap, ConnectorGap::recovered);
    });
}

void ConnectorLifecycleTests::missingEventFactsStayMissing() {
    scenario([] {
        ConnectorEventObservation observation{};
        observation.facts.event_type = "individual-support";
        observation.facts.schema_version = 1;
        const auto event = describe_connector_event(source, arrival, observation);
        QCOMPARE(event.origin, source);
        QCOMPARE(event.arrival, arrival);
        QCOMPARE(event.facts.event_type, std::string{"individual-support"});
        QCOMPARE(event.facts.schema_version, std::uint32_t{1});
        QVERIFY(!event.facts.source_timestamp_utc_ms);
        QVERIFY(!event.facts.actor_id);
        QVERIFY(!event.facts.actor_display_name);
        QVERIFY(!event.facts.money);
        QVERIFY(!event.facts.activity_reference);
        QVERIFY(!event.facts.payment_reference);
        QCOMPARE(event.evidence, core::PaymentEvidence::unknown);
        QVERIFY(event.identifiers.empty());
    });
}

void ConnectorLifecycleTests::knownZeroAndEvidenceArePreserved() {
    scenario([] {
        ConnectorEventObservation observation{};
        observation.facts.event_type = "individual-support";
        observation.facts.schema_version = 1;
        observation.facts.source_timestamp_utc_ms = 0;
        observation.facts.money = core::ReceiptMoney{"0.00", "CHF"};
        observation.facts.actor_id = "synthetic-actor";
        observation.facts.actor_display_name = "Synthetic display name";

        observation.evidence = core::PaymentEvidence::unconfirmed;
        const auto reported = describe_connector_event(source, arrival, observation);
        QCOMPARE(reported.facts.source_timestamp_utc_ms, std::optional<std::int64_t>{0});
        QCOMPARE(reported.facts.actor_id, std::optional<std::string>{"synthetic-actor"});
        QCOMPARE(reported.facts.actor_display_name,
                 std::optional<std::string>{"Synthetic display name"});
        const std::optional<core::ReceiptMoney> expectedMoney{core::ReceiptMoney{"0.00", "CHF"}};
        QCOMPARE(reported.facts.money, expectedMoney);
        QCOMPARE(reported.evidence, core::PaymentEvidence::unconfirmed);

        // The adapter's evidence label is independent of amount and identity.
        observation.evidence = core::PaymentEvidence::confirmed;
        const auto confirmed = describe_connector_event(source, arrival, observation);
        QCOMPARE(confirmed.evidence, core::PaymentEvidence::confirmed);
        QCOMPARE(confirmed.facts.money, expectedMoney);

        observation.facts.event_type = "aggregate-observation";
        observation.evidence = core::PaymentEvidence::not_applicable;
        const auto aggregate = describe_connector_event(source, arrival, observation);
        QCOMPARE(aggregate.evidence, core::PaymentEvidence::not_applicable);
        QCOMPARE(aggregate.facts.event_type, std::string{"aggregate-observation"});
    });
}

void ConnectorLifecycleTests::identifiersAreScopedAndRoleSpecific() {
    scenario([] {
        const core::ReceiptLinkReference delivery{"delivery", std::string{"A\0b", 3}};
        const core::ReceiptLinkReference eventId{"event", std::string{"A\0b", 3}};
        const core::ReceiptLinkReference activity{"link", "Activity"};
        const core::ReceiptLinkReference payment{"link", "activity"};
        ConnectorEventObservation observation{};
        observation.facts.event_type = "individual-support";
        observation.facts.schema_version = 1;
        observation.delivery = delivery;
        observation.event = eventId;
        observation.facts.activity_reference = activity;
        observation.facts.payment_reference = payment;

        const auto event = describe_connector_event(source, arrival, observation);
        const std::map<Identifier, ScopedConnectorIdentifier> expected{
            {Identifier::delivery, {source, delivery}},
            {Identifier::event, {source, eventId}},
            {Identifier::activity, {source, activity}},
            {Identifier::payment, {source, payment}}};
        QCOMPARE(event.identifiers, expected);
        QCOMPARE(event.facts.activity_reference, observation.facts.activity_reference);
        QCOMPARE(event.facts.payment_reference, observation.facts.payment_reference);
        QCOMPARE(event.evidence, core::PaymentEvidence::unknown);

        const core::ReceiptOrigin otherAccount{"synthetic-provider", "source-two"};
        const core::ReceiptOrigin otherProvider{"different-provider", "source-one"};
        const core::ReceiptArrival nextArrival{3000, "connection-three", "config-one",
                                               "context-one"};
        const auto retry = describe_connector_event(source, nextArrival, observation);
        QCOMPARE(retry.identifiers, expected);
        QCOMPARE(retry.arrival, nextArrival);
        const auto fromOtherAccount = describe_connector_event(otherAccount, arrival, observation);
        const auto fromOtherProvider =
            describe_connector_event(otherProvider, arrival, observation);
        QCOMPARE(fromOtherAccount.identifiers.size(), expected.size());
        QCOMPARE(fromOtherProvider.identifiers.size(), expected.size());
        for (const auto &[role, identifier] : expected) {
            const ScopedConnectorIdentifier accountId{otherAccount, identifier.reference};
            const ScopedConnectorIdentifier providerId{otherProvider, identifier.reference};
            QVERIFY(fromOtherAccount.identifiers.contains(role));
            QVERIFY(fromOtherProvider.identifiers.contains(role));
            QCOMPARE(fromOtherAccount.identifiers.at(role), accountId);
            QCOMPARE(fromOtherProvider.identifiers.at(role), providerId);
        }
    });
}

void ConnectorLifecycleTests::refundTargetIsOnlyAReference() {
    scenario([] {
        ConnectorEventObservation observation{};
        observation.facts.event_type = "refund-report";
        observation.facts.schema_version = 1;
        observation.event = core::ReceiptLinkReference{"event", "refund-52"};
        observation.refund_target = core::ReceiptLinkReference{"payment", "original-41"};

        const auto event = describe_connector_event(source, arrival, observation);
        QCOMPARE(event.facts.event_type, std::string{"refund-report"});
        const std::map<Identifier, ScopedConnectorIdentifier> expected{
            {Identifier::event, {source, {"event", "refund-52"}}},
            {Identifier::refund_target, {source, {"payment", "original-41"}}}};
        QCOMPARE(event.identifiers, expected);
        QVERIFY(!event.facts.payment_reference);
        QCOMPARE(event.evidence, core::PaymentEvidence::unknown);

        observation.refund_target.reset();
        const auto targetUnknown = describe_connector_event(source, arrival, observation);
        QVERIFY(!targetUnknown.identifiers.contains(Identifier::refund_target));
        QVERIFY(!targetUnknown.facts.payment_reference);
        QCOMPARE(targetUnknown.evidence, core::PaymentEvidence::unknown);
    });
}
} // namespace

QTEST_GUILESS_MAIN(ConnectorLifecycleTests)
#include "connector_lifecycle_tests.moc"
