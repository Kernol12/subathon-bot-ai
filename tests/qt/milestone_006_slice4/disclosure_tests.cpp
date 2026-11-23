// SPDX-License-Identifier: GPL-3.0-or-later
#include "../milestone_006_slice3/application_fixture.hpp"

namespace {
using namespace milestone_006_slice3;
using Kind = MilestoneActionKind;
using Mode = DeliverySenderSelection;

class OwnerEntryMissing final : public std::logic_error {
  public:
    using std::logic_error::logic_error;
};

template <class Body> void check(Body body) {
    try {
        body();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

bool same_target(const Occurrence &seen, const Occurrence &wanted) {
    const auto *actual = std::get_if<ApplicationOperationKey>(&seen.origin);
    const auto &exact = std::get<ApplicationOperationKey>(wanted.origin);
    return seen.track == wanted.track && seen.occurrence == wanted.occurrence && actual &&
           finance::same(*actual, exact);
}

// Synthetic trusted profile and sender-selection facts. The default sender
// and the explicit selection deliberately resolve to the same concrete identity.
class CaptureProfile {
  public:
    virtual ~CaptureProfile() = default;
    virtual std::optional<Mode> sender_selection(const CommandAuthority::Facts &,
                                                 const Replacement &) = 0;
    virtual std::optional<bool> compatible(const CommandAuthority::Facts &, const Replacement &,
                                           const DeliveryJobKey &original,
                                           std::string_view original_format) = 0;
};

constexpr auto inherited_format = "subathon.internal.delivery-job.v1";
const Replacement automation{1, {Kind::automation, "audio", "replacement-v3"}};
const Replacement bonus{2, {Kind::bonus, "award", "bonus-v3"}};

struct ProfileFixture final : CaptureProfile {
    std::optional<bool> compatibility{true};
    int profile_reads{};
    int sender_reads{};

    std::optional<Mode> sender_selection(const CommandAuthority::Facts &facts,
                                         const Replacement &slot) override {
        ++sender_reads;
        if (slot != automation)
            return {};
        if (facts.grant_revision == "bound-default")
            return Mode::configured_default;
        if (facts.grant_revision == "current-explicit")
            return Mode::explicit_identity;
        return {};
    }

    std::optional<bool> compatible(const CommandAuthority::Facts &facts, const Replacement &slot,
                                   const DeliveryJobKey &original,
                                   std::string_view original_format) override {
        ++profile_reads;
        if (slot != automation || facts.capability_revision != "capability-1" ||
            original_format != inherited_format ||
            !finance::same(original.operation, {"test", "group-A"}) || original.job_ref != "job-1")
            return {};
        return compatibility;
    }
};

struct DisclosureAuthority final : CommandAuthority {
    Authority storage;
    bool facts_available{true};
    int selections{};

    std::optional<Facts> select(const SqliteWorkSession &s, const Command &cmd) override {
        ++selections;
        return facts_available ? storage.select(s, cmd) : std::nullopt;
    }

    std::optional<qint64> historical_bonus_amount(const SqliteWorkSession &s,
                                                  const Occurrence &target,
                                                  const Replacement &definition) override {
        return storage.historical_bonus_amount(s, target, definition);
    }
};

struct AutomationView {
    MilestoneAction definition;
    std::string format;
    QByteArray payload;
    std::string route;
    std::string sender;
    bool operator==(const AutomationView &) const = default;
};

struct BonusView {
    MilestoneAction definition;
    qint64 milliseconds;
    bool operator==(const BonusView &) const = default;
};

using CaptureView = std::variant<AutomationView, BonusView>;

struct SlotDisclosure {
    SelectedVersion selected;
    CaptureView old_capture;
    CaptureView new_capture;
    std::optional<Mode> proposed_sender_selection;
    bool operator==(const SlotDisclosure &) const = default;
};

struct DisclosedPreview {
    Occurrence target;
    bool released;
    std::vector<MilestoneHoldKey> holds;
    std::vector<SlotDisclosure> ordered;
    std::shared_ptr<const ReplacePreview> confirmation;
};

using DisclosureReply = std::variant<Disposition, DisclosedPreview>;

DisclosureReply preview_disclosure(const SqliteWorkSession &, CommandAuthority &, CaptureProfile &,
                                   const Command &) {
    throw OwnerEntryMissing("R7: produce authorized ordered replacement disclosure");
}

// Ordinary publisher accepts proposed definitions and resolves authorization
// from the current owner port. It cannot rewrite any existing occurrence.
struct DefinitionEdit {
    MilestoneTrackKey track;
    MilestoneEditActor actor;
    QByteArray operation;
    std::string expected_revision;
    MilestoneAction proposed;
};

class DefinitionAuthority {
  public:
    virtual ~DefinitionAuthority() = default;
    virtual std::optional<AutomationView> admitted_definition(const SqliteWorkSession &,
                                                              const DefinitionEdit &) = 0;
};

struct PublicationAuthority final : DefinitionAuthority {
    int selections{};

    std::optional<AutomationView> admitted_definition(const SqliteWorkSession &,
                                                      const DefinitionEdit &edit) override {
        ++selections;
        if (edit.track != storage_027_fixture::edit::track ||
            edit.actor.actor_ref != storage_027_fixture::edit::actor.actor_ref ||
            edit.actor.session_lineage != storage_027_fixture::edit::actor.session_lineage ||
            edit.expected_revision != "application-v1")
            return {};
        return AutomationView{{Kind::automation, "audio", "ordinary-v2"},
                              inherited_format,
                              QByteArray::fromHex("00a5"),
                              "local/published",
                              "published-S"};
    }
};

Disposition publish_definitions(const SqliteWorkSession &, DefinitionAuthority &,
                                const DefinitionEdit &) {
    throw OwnerEntryMissing("R1: publish ordinary milestone definitions under current authority");
}

struct LaterCrossing {
    std::string milestone_ref;
    qint64 target;
    qint64 score;
};

void capture_later_group(const SqliteWorkSession &, const ContributionReceipt &, const Occurrence &,
                         const LaterCrossing &) {
    throw OwnerEntryMissing("R1: capture a later milestone from the published definition");
}

struct DisclosureFixture {
    ApplicationFixture f{true, true};
    DisclosureAuthority owner;
    ProfileFixture profile;

    DisclosureFixture() {
        owner.storage.revision = "bound-default";
        owner.storage.historical_bonuses.push_back(
            {f.a, {2, {Kind::bonus, "award", "bonus-v1"}}, 30000});
        configure_final();
    }

    void configure_final() {
        owner.storage.captures = {
            {bonus, {}, {}, {}, 15000},
            {automation, QByteArray::fromHex("0081ff"), "local/route-v3", "S"}};
    }

    Command request() {
        return {edit::track, "disclose-and-replace", edit::actor,
                ReplaceIntent{f.a, {bonus, automation}}};
    }

    std::array<SqlRows, 8> snapshot() {
        return {
            f.file.rows("SELECT * FROM main.subathon_milestone_command_journal ORDER BY operation"),
            f.file.rows("SELECT * FROM main.subathon_milestone_slot_versions ORDER BY version"),
            f.file.rows(
                "SELECT * FROM main.subathon_milestone_slot_attempts ORDER BY version,revision"),
            f.file.rows("SELECT * FROM main.subathon_milestone_groups ORDER BY queue_order"),
            f.file.rows("SELECT * FROM main.subathon_milestone_actions ORDER BY "
                        "occurrence_ref,action_ordinal"),
            f.file.rows("SELECT * FROM main.subathon_delivery_events ORDER BY "
                        "operation_scope,operation_ref,job_ref,revision"),
            f.file.rows("SELECT * FROM main.subathon_milestone_acceptance_decisions ORDER BY "
                        "occurrence_ref"),
            f.file.rows("SELECT * FROM main.subathon_delivery_captures ORDER BY "
                        "operation_scope,operation_ref,job_ref")};
    }

    void require_original() {
        f.file.write([&](const auto &s) {
            const auto key = f.job(f.a, 1);
            const auto original = SqliteDeliveryJobStore(s).lookup(key);
            const auto bundle = SqliteApplicationBundleStore(s).lookup(key.operation);
            require(original && original->action_document == "captured-audio" &&
                        original->capture.action_ref == "audio" &&
                        original->capture.action_version_ref == "v1" &&
                        original->capture.route_ref == "local/route" &&
                        original->capture.sender_ref == "original-sender" &&
                        original->capture.selection == Mode::explicit_identity &&
                        original->revision == 1 && original->state == DeliveryJobState::queued &&
                        bundle &&
                        std::ranges::any_of(bundle->jobs,
                                            [&](const auto &job) {
                                                return job.job_ref == key.job_ref &&
                                                       job.envelope.format_ref == inherited_format;
                                            }),
                    "Owning normal reads retain original bytes, verified format, sender and queued "
                    "state");
        });
    }

    Decision confirm(const Command &command, const ReplacePreview &preview) {
        std::optional<Decision> staged;
        f.file.write([&](const auto &s) { staged = stage_command(s, owner, command, &preview); });
        return *staged;
    }

    std::vector<SelectedVersion> seed_chain() {
        const Replacement mid_auto{1, {Kind::automation, "audio", "replacement-v2"}};
        const Replacement mid_bonus{2, {Kind::bonus, "award", "bonus-v2"}};
        owner.storage.captures = {
            {mid_auto, QByteArray::fromHex("0042"), "local/route-v2", "historical-S"},
            {mid_bonus, {}, {}, {}, 22000}};
        const Command command{edit::track, "seed-first-replacement", edit::actor,
                              ReplaceIntent{f.a, {mid_bonus, mid_auto}}};
        const auto proof = f.preview(owner.storage, command);
        const auto value = confirm(command, *proof);
        require(value.disposition == Disposition::committed && value.versions.size() == 2 &&
                    value.versions[0].ordinal == 1 && !value.versions[0].version.isEmpty() &&
                    value.versions[1].ordinal == 2 && !value.versions[1].version.isEmpty(),
                "Normal protected confirmation seeds exact v2 selections without starting either");
        configure_final();
        reopen_financial(f.file);
        return value.versions;
    }

    DisclosedPreview preview(const Command &cmd) {
        std::optional<DisclosedPreview> result;
        f.file.write([&](const auto &s) {
            const auto reply = preview_disclosure(s, owner, profile, cmd);
            const auto *disclosed = std::get_if<DisclosedPreview>(&reply);
            require(disclosed && disclosed->confirmation,
                    "Current authority returns a typed view and an opaque confirmable preview");
            result = *disclosed;
        });
        return *result;
    }
};

std::vector<SlotDisclosure> expected(const std::vector<SelectedVersion> &before, bool chain,
                                     Mode mode) {
    const AutomationView old_action =
        chain ? AutomationView{{Kind::automation, "audio", "replacement-v2"},
                               inherited_format,
                               QByteArray::fromHex("0042"),
                               "local/route-v2",
                               "historical-S"}
              : AutomationView{{Kind::automation, "audio", "v1"},
                               inherited_format,
                               QByteArray("captured-audio"),
                               "local/route",
                               "original-sender"};
    const AutomationView new_action{automation.action, inherited_format,
                                    QByteArray::fromHex("0081ff"), "local/route-v3", "S"};
    const BonusView old_bonus{{Kind::bonus, "award", chain ? "bonus-v2" : "bonus-v1"},
                              chain ? 22000 : 30000};
    const BonusView new_bonus{bonus.action, 15000};
    return {{before[0], old_action, new_action, mode}, {before[1], old_bonus, new_bonus, {}}};
}

} // namespace

class Milestone006DisclosureTests final : public QObject {
    Q_OBJECT

  private slots:
    void r1OrdinaryPublicationAffectsOnlyLaterCapture() {
        check([] {
            DisclosureFixture d;
            d.require_original();
            const auto state = d.snapshot();
            PublicationAuthority authority;
            const DefinitionEdit edit_request{edit::track,
                                              edit::actor,
                                              "publish-v2",
                                              "application-v1",
                                              {Kind::automation, "audio", "ordinary-v2"}};
            d.f.file.write([&](const auto &s) {
                require(publish_definitions(s, authority, edit_request) == Disposition::committed &&
                            authority.selections > 0,
                        "The ordinary publication is accepted using current owner authority");
            });
            require(
                d.snapshot() == state,
                "Publication alone creates no captured group, replacement, claim, or reservation");
            const auto receipt = d.f.file.intake("ordinary-later-C", {}, "synthetic-owner-command");
            const Occurrence later_group{edit::track,
                                         {"ordinary-later-C"},
                                         ApplicationOperationKey{"test", "ordinary-later-C"}};
            const LaterCrossing crossing{"C", 37000000, 40000000};
            d.f.file.write(
                [&](const auto &s) { capture_later_group(s, receipt, later_group, crossing); });
            d.require_original();
            d.f.file.write([&](const auto &s) {
                const MilestoneAction later_audio{Kind::automation, "audio", "ordinary-v2"};
                const std::vector<MilestoneAction> later_actions{later_audio};
                const auto pending = SqliteMilestoneGroupStore(s).pending();
                require(pending.size() == 3 && pending[0].group.occurrence == d.f.a.occurrence &&
                            pending[0].group.actions.front().version_ref == "v1" &&
                            pending[2].group.occurrence == later_group.occurrence &&
                            pending[2].group.actions == later_actions,
                        "Owning capture uses the new definition only for later C");
                const auto &key = std::get<ApplicationOperationKey>(later_group.origin);
                const auto bundle = SqliteApplicationBundleStore(s).lookup(key);
                require(bundle && bundle->jobs.size() == 1 &&
                            bundle->jobs.front().group_ref == later_group.occurrence.ref &&
                            bundle->jobs.front().group_ordinal == 1 &&
                            bundle->jobs.front().envelope.format_ref == inherited_format,
                        "Later C owns a real versioned capture and exact enrollment");
                const auto job =
                    SqliteDeliveryJobStore(s).lookup({key, bundle->jobs.front().job_ref});
                require(job && job->action_document == QByteArray::fromHex("00a5") &&
                            job->capture.sender_ref == "published-S" &&
                            job->capture.route_ref == "local/published" &&
                            job->capture.action_version_ref == "ordinary-v2",
                        "Normal readback finds the newly published capture only on C");
            });
        });
    }

    void r7OrderedDisclosureConsumesExactConfirm_data() {
        QTest::addColumn<bool>("chain");
        QTest::newRow("original-held") << false;
        QTest::newRow("reopened-chain-released") << true;
    }

    void r7OrderedDisclosureConsumesExactConfirm() {
        QFETCH(bool, chain);
        check([&] {
            DisclosureFixture d;
            std::vector<SelectedVersion> old{{1, {}}, {2, {}}};
            if (chain) {
                const MilestoneHoldKey cause{MilestoneHoldCause::deferred_acceptance,
                                             "accept-" + first.ref};
                d.f.file.write([&](const auto &s) {
                    require(SqliteMilestoneGroupDecisions(s).resolve(
                                first, cause, MilestoneHoldResolution::continue_actions,
                                {"operator", "resolve-preview-hold"}),
                            "Exact retained Defer resolves before normal release");
                });
                require(d.f.release(d.f.a) == MilestoneReleaseDisposition::released,
                        "A reaches released unstarted state by owning entry");
                old = d.seed_chain();
            }
            d.require_original();
            const auto cmd = d.request();
            require(bool(d.f.preview(d.owner.storage, cmd)),
                    "Owning storage can already bind the exact unstarted mixed shape");
            const auto before = d.snapshot();
            const auto holds =
                chain ? std::vector<MilestoneHoldKey>{}
                      : std::vector<MilestoneHoldKey>{
                            {MilestoneHoldCause::deferred_acceptance, "accept-" + first.ref}};
            d.f.file.write([&](const auto &session) {
                const auto current =
                    read_occurrence_selection(session, d.owner, stop("preflight-A", d.f.a));
                require(current && current->released == chain &&
                            current->independent_holds == holds &&
                            current->selected_versions.size() == 6 &&
                            current->selected_versions[0] == old[0] &&
                            current->selected_versions[1] == old[1],
                        "Ordinary v21 views prove exact selected versions and real gates before "
                        "disclosure");
            });
            const auto disclosure = d.preview(cmd);
            require(
                same_target(disclosure.target, d.f.a) && disclosure.released == chain &&
                    disclosure.holds == holds &&
                    disclosure.ordered == expected(old, chain, Mode::configured_default) &&
                    d.owner.selections > 0 && d.profile.profile_reads > 0 &&
                    d.profile.sender_reads > 0 && d.snapshot() == before,
                "Ordered mixed view binds exact old/new values, current mode and durable gates");
            const auto accepted = d.confirm(cmd, *disclosure.confirmation);
            require(accepted.disposition == Disposition::committed &&
                        accepted.versions.size() == 2 && accepted.versions[0].ordinal == 1 &&
                        accepted.versions[1].ordinal == 2 &&
                        !accepted.versions[0].version.isEmpty() &&
                        !accepted.versions[1].version.isEmpty() &&
                        accepted.versions[0].version != old[0].version &&
                        accepted.versions[1].version != old[1].version,
                    "Protected confirm consumes the exact opaque disclosure token");
            d.f.file.write([&](const auto &s) {
                const auto projected = read_occurrence_selection(s, d.owner, stop("read-A", d.f.a));
                const auto queued = lookup_replacement_attempt(s, d.f.a, accepted.versions[0], 1);
                require(projected && projected->released == chain &&
                            projected->independent_holds == holds &&
                            projected->selected_versions.size() == 6 &&
                            projected->selected_versions[0] == accepted.versions[0] &&
                            projected->selected_versions[1] == accepted.versions[1] &&
                            projected->selected_versions[2] == SelectedVersion{3, {}} && queued &&
                            queued->state == DeliveryJobState::queued,
                        "Exact successor selection retains gates and untouched original slots");
            });
            const auto stored =
                d.f.file.rows("SELECT payload,qualified_sender,predecessor "
                              "FROM main.subathon_milestone_slot_versions WHERE version=?",
                              {accepted.versions[0].version});
            require(stored.size() == 1 && stored.front().size() == 3 &&
                        stored.front()[0].toByteArray() == QByteArray::fromHex("0081ff") &&
                        stored.front()[1].toByteArray() == "S" &&
                        (chain ? stored.front()[2].toByteArray() == old[0].version
                               : stored.front()[2].isNull()),
                    "Existing v21 history retains exact sender, payload and predecessor link");
            if (chain) {
                const auto retained = d.f.file.rows(
                    "SELECT payload,qualified_sender FROM main.subathon_milestone_slot_versions "
                    "WHERE version=?",
                    {old[0].version});
                require(retained ==
                            SqlRows{{QByteArray::fromHex("0042"), QByteArray("historical-S")}},
                        "The superseded intermediate capture retains its own qualified sender and "
                        "bytes");
            }
            d.require_original();
        });
    }

    void r7ChangedCurrentModeStalesDisclosure() {
        check([] {
            DisclosureFixture d;
            const auto command = d.request();
            const auto before = d.snapshot();
            const auto old = d.preview(command);
            const std::vector<SelectedVersion> originals{{1, {}}, {2, {}}};
            require(old.ordered == expected(originals, false, Mode::configured_default),
                    "The first current grant resolves configured default");
            d.owner.storage.revision = "current-explicit";
            require(d.confirm(command, *old.confirmation).disposition == Disposition::refused &&
                        d.snapshot() == before,
                    "A new grant and mode stale the old bound preview even at the same resolved "
                    "sender");
            const auto latest = d.preview(command);
            require(latest.ordered == expected(originals, false, Mode::explicit_identity) &&
                        d.snapshot() == before,
                    "A newly authorized view discloses explicit mode with the same exact sender");
            require(d.confirm(command, *latest.confirmation).disposition == Disposition::committed,
                    "Only the exact newly bound view confirms");
        });
    }

    void r7DisclosureAndCompatibilityFailClosed_data() {
        QTest::addColumn<int>("gate");
        QTest::newRow("revoked-disclosure") << 0;
        QTest::newRow("absent-authoritative-facts") << 1;
        QTest::newRow("incompatible-profile") << 2;
        QTest::newRow("unproved-profile") << 3;
    }

    void r7DisclosureAndCompatibilityFailClosed() {
        QFETCH(int, gate);
        check([&] {
            DisclosureFixture d;
            const auto command = d.request();
            // The exact durable shape supports an ordinary protected preview;
            // the owner entry must separately apply the disclosure/profile gate.
            require(bool(d.f.preview(d.owner.storage, command)),
                    "Positive normal protected preview proves the exact producer lane");
            const auto before = d.snapshot();
            d.owner.storage.disclosure = gate != 0;
            d.owner.facts_available = gate != 1;
            d.profile.compatibility = gate == 3 ? std::nullopt : std::optional<bool>{gate != 2};
            d.f.file.write([&](const auto &s) {
                const auto reply = preview_disclosure(s, d.owner, d.profile, command);
                const auto *disposition = std::get_if<Disposition>(&reply);
                require(disposition, "A gated request returns no typed view or confirmable token");
                if (gate < 2) {
                    require(*disposition ==
                                (gate == 0 ? Disposition::denied : Disposition::unavailable),
                            "Known revocation and absent authoritative facts remain distinct");
                } else {
                    require(d.profile.profile_reads > 0 &&
                                (*disposition == Disposition::refused ||
                                 *disposition == Disposition::unavailable),
                            "Incompatible or unproved format remains gated without fallback");
                }
                require(d.owner.selections > 0, "The owner reads actual current authority");
            });
            require(d.snapshot() == before,
                    "A gated view reserves no key, mutates no capture, and starts no action");
        });
    }
};

QTEST_GUILESS_MAIN(Milestone006DisclosureTests)
#include "disclosure_tests.moc"
