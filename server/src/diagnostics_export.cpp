// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics_private.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QScopeGuard>
#include <QVersionNumber>
#include <algorithm>
#include <limits>

namespace subathon::server::diagnostics {
namespace {
constexpr std::array health_names{"unknown", "setup_required", "disconnected", "connecting",
                                  "connected"};
constexpr std::array gap_names{"unknown", "open", "unrecoverable", "recovered", "none"};
using Pair = std::pair<ConnectorHealth, ConnectorGap>;
struct Entry {
    unsigned id;
    ConnectorProvider provider;
    std::optional<Pair> pair;
    bool failed = false;
};

template <class T> bool valid_enum(T value, T last) {
    return static_cast<std::size_t>(value) <= static_cast<std::size_t>(last);
}

std::optional<Pair> observe(const ConnectorAdapter &adapter) {
    // Reuse the API-005 precedence on a projection with no setup context,
    // capability declarations or unrelated source fields.
    struct HealthOnly final : ConnectorAdapter {
        ConnectorInstallation installation;
        ConnectorConnection connection;
        ConnectorGap gap;
        ConnectorReport report() const override {
            ConnectorReport copy;
            copy.installation = installation;
            copy.connection = connection;
            copy.gap = gap;
            return copy;
        }
    } projection;
    try {
        const auto latest = adapter.report();
        if (!valid_enum(latest.installation, ConnectorInstallation::not_required) ||
            !valid_enum(latest.connection, ConnectorConnection::connected) ||
            !valid_enum(latest.gap, ConnectorGap::none))
            return {};
        projection.installation = latest.installation;
        projection.connection = latest.connection;
        projection.gap = latest.gap;
        const auto status = read_connector_status(projection, {});
        return Pair{status.health, status.gap};
    } catch (...) {
        return {};
    }
}

QString version_token(const std::optional<Version> &version) {
    if (!version || version->major > 99999 || version->minor > 99999 || version->patch > 99999)
        return "unknown";
    return QStringLiteral("%1.%2.%3").arg(version->major).arg(version->minor).arg(version->patch);
}

std::optional<Version> runtime_version() {
    const auto runtime = QString::fromLatin1(qVersion());
    const auto parsed = QVersionNumber::fromString(runtime);
    if (parsed.segmentCount() != 3)
        return {};
    const Version version{unsigned(parsed.majorVersion()), unsigned(parsed.minorVersion()),
                          unsigned(parsed.microVersion())};
    if (version_token(version) != runtime)
        return {};
    return version;
}

std::optional<QString> disposition_name(core::ReceiptDisposition value) {
    switch (value) {
    case core::ReceiptDisposition::created:
        return QStringLiteral("created");
    case core::ReceiptDisposition::retry:
        return QStringLiteral("retry");
    case core::ReceiptDisposition::conflict:
        return QStringLiteral("conflict");
    }
    return {};
}

// A miss is distinct from a failed projection, including an invalid source
// value in another selected attempt in the same read.
struct Selected {
    QJsonArray objects;
    bool missing = false;
};
std::optional<Selected> render_selected(const std::vector<Selector> &selectors,
                                        const std::vector<Attempt> &attempts) {
    if (selectors.size() != attempts.size())
        return {};
    Selected result;
    for (std::size_t i = 0; i < selectors.size(); ++i) {
        const auto &fields = selectors[i].fields;
        const auto &values = attempts[i];
        if (!values) {
            result.missing = true;
            continue;
        }
        if (fields.size() != values->size())
            return {};
        QJsonObject selected;
        for (std::size_t j = 0; j < fields.size(); ++j) {
            if (fields[j] == Field::disposition) {
                const auto *value = std::get_if<core::ReceiptDisposition>(&(*values)[j]);
                const auto name = value ? disposition_name(*value) : std::nullopt;
                if (!name)
                    return {};
                selected.insert("receipt_disposition", *name);
            } else {
                const auto *value = std::get_if<std::uint32_t>(&(*values)[j]);
                if (!value)
                    return {};
                selected.insert("receipt_schema_version", QString::number(*value));
            }
        }
        result.objects.append(selected);
    }
    return result;
}

struct Stage {
    Ticket ticket;
    Status status = Status::invalid_selection;
    std::vector<Selector> selectors;
    Gate initial;
    QJsonArray connections;
    Versions versions;
    std::shared_ptr<detail::Snapshot> snapshot;
    bool awaiting_authority = false;
};
void abandon(const Stage &stage) {
    if (stage.snapshot)
        stage.snapshot->abandoned.store(true);
}
} // namespace

Versions running_versions() {
    return {Version{SUBATHON_DIAGNOSTICS_VERSION_MAJOR, SUBATHON_DIAGNOSTICS_VERSION_MINOR,
                    SUBATHON_DIAGNOSTICS_VERSION_PATCH},
            runtime_version()};
}

struct Connections::Impl {
    explicit Impl(LogOwner &owner) : logs(owner) { entries.reserve(detail::registry_limit); }
    std::optional<QJsonArray> project() const {
        if (overflowed)
            return {};
        QJsonArray result;
        for (const auto provider : connector_providers()) {
            std::array<std::array<bool, gap_names.size()>, health_names.size()> observed{};
            bool registered = false;
            bool missing = false;
            for (const auto &entry : entries) {
                if (entry.provider != provider)
                    continue;
                registered = true;
                if (!entry.pair) {
                    missing = true;
                    continue;
                }
                const auto [health, gap] = *entry.pair;
                observed[std::size_t(health)][std::size_t(gap)] = true;
            }
            QJsonArray pairs;
            for (std::size_t h = 0; h < health_names.size(); ++h)
                for (std::size_t g = 0; g < gap_names.size(); ++g)
                    if (observed[h][g])
                        pairs.append(
                            QJsonObject{{"health", health_names[h]}, {"gap", gap_names[g]}});
            result.append(QJsonObject{{"provider", detail::name(provider)},
                                      {"observed", pairs},
                                      {"unavailable", !registered || missing}});
        }
        return result;
    }

    LogOwner &logs;
    std::vector<Entry> entries;
    // A lost registration cannot be inferred from later unregister calls.
    // Rebuild the owner from the installation registry on over-capacity.
    bool overflowed = false;
};

Connections::Connections(LogOwner &logs) : impl_(std::make_unique<Impl>(logs)) {}
Connections::~Connections() = default;

void Connections::register_adapter(unsigned id, ConnectorProvider provider) {
    auto &entries = impl_->entries;
    if (!detail::name(provider) || std::ranges::find(entries, id, &Entry::id) != entries.end())
        return;
    if (entries.size() == detail::registry_limit) {
        impl_->overflowed = true;
        return;
    }
    entries.push_back({id, provider, {}});
}
void Connections::unregister_adapter(unsigned id) {
    std::erase_if(impl_->entries, [id](const auto &entry) { return entry.id == id; });
}
void Connections::report(unsigned id, const ConnectorAdapter &adapter) {
    auto entry = std::ranges::find(impl_->entries, id, &Entry::id);
    if (entry == impl_->entries.end())
        return;
    entry->pair = observe(adapter);
    const auto first_failure = !entry->pair && !entry->failed;
    entry->failed = !entry->pair;
    if (first_failure)
        impl_->logs.connector_report_unavailable(entry->provider);
}

struct Export::Impl {
    Impl(LogOwner &owner, const Connections &registry, std::function<Versions()> observe_versions)
        : logs(owner), connections(registry), versions(std::move(observe_versions)) {}

    LogOwner &logs;
    const Connections &connections;
    std::function<Versions()> versions;
    std::optional<Stage> stage;
    Ticket sequence = 0;
    bool in_turn = false;
};

Export::Export(LogOwner &owner, const Connections &connections, std::function<Versions()> versions)
    : impl_(std::make_unique<Impl>(owner, connections, std::move(versions))) {}
Export::~Export() {
    if (impl_->stage)
        abandon(*impl_->stage);
}

Ticket Export::begin(std::optional<QByteArrayView> request) {
    auto &state = *impl_;
    if (state.stage || state.in_turn || state.sequence == std::numeric_limits<Ticket>::max())
        return 0;
    try {
        Stage staged{};
        staged.ticket = ++state.sequence;
        auto parsed = request ? detail::parse_selectors(*request) : std::vector<Selector>{};
        if (parsed) {
            staged.selectors = std::move(*parsed);
            staged.awaiting_authority = true;
        }
        const auto ticket = staged.ticket;
        state.stage = std::move(staged);
        return ticket;
    } catch (...) {
        return 0;
    }
}

const std::vector<Selector> *Export::selection(Ticket ticket) const {
    const auto &staged = impl_->stage;
    if (!staged || staged->ticket != ticket || !staged->awaiting_authority)
        return nullptr;
    return &staged->selectors;
}

bool Export::stage(Ticket ticket, Gate current, std::function<void()> ready) {
    auto &state = *impl_;
    if (!selection(ticket) || state.in_turn)
        return false;
    state.in_turn = true;
    const auto reset = qScopeGuard([&] { state.in_turn = false; });
    auto discard = qScopeGuard([&] { state.stage.reset(); });
    auto &staged = *state.stage;
    try {
        if (!current.scope || !current.expires ||
            (current.use != Use::allowed && current.use != Use::denied))
            return false;
        staged.initial = std::move(current);
        if (staged.initial.use == Use::denied) {
            staged.status = Status::denied;
        } else {
            const auto events = render_selected(staged.selectors, staged.initial.attempts);
            if (!events)
                return false;
            if (events->missing) {
                staged.status = Status::selection_unavailable;
            } else {
                auto projected = state.connections.impl_->project();
                if (!projected)
                    return false;
                staged.status = Status::artifact;
                staged.connections = std::move(*projected);
                staged.versions = state.versions ? state.versions() : Versions{};
                staged.snapshot = std::make_shared<detail::Snapshot>();
                staged.snapshot->ready = std::move(ready);
            }
        }
        staged.awaiting_authority = false;
        if (!staged.snapshot || !state.logs.snapshot(staged.snapshot)) {
            auto notify = staged.snapshot ? std::move(staged.snapshot->ready) : std::move(ready);
            staged.snapshot.reset();
            if (notify)
                notify();
        }
        discard.dismiss();
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<PublicationRead> Export::publication_read(Ticket ticket) const {
    const auto &stage = impl_->stage;
    if (!stage || stage->ticket != ticket ||
        (stage->status != Status::artifact && stage->status != Status::selection_unavailable))
        return {};
    return PublicationRead{stage->selectors, *stage->initial.scope};
}

void Export::handoff(Ticket ticket, const ReadGate &read_current,
                     const std::function<bool(std::optional<Utc>)> &can_transfer,
                     const Transfer &transfer, qsizetype host_byte_limit) {
    auto &state = *impl_;
    if (!state.stage || state.stage->ticket != ticket || state.in_turn)
        return;
    state.in_turn = true;
    const auto reset = qScopeGuard([&] { state.in_turn = false; });
    auto stage = std::move(*state.stage);
    state.stage.reset();
    const auto release = qScopeGuard([&] { abandon(stage); });
    try {
        if (stage.awaiting_authority)
            return;
        const auto send = [&](Status status, QByteArray artifact = {},
                              std::optional<Utc> expires = {}) {
            if (can_transfer(expires) && host_byte_limit > 0 && artifact.size() <= host_byte_limit)
                transfer({status, std::move(artifact)});
        };
        if (stage.status == Status::invalid_selection) {
            send(Status::invalid_selection);
            return;
        }
        if (stage.status == Status::denied) {
            send(Status::denied);
            return;
        }
        const auto final = read_current(stage.selectors, &*stage.initial.scope);
        if (!final.scope || !final.expires || final.scope != stage.initial.scope)
            return;
        if (final.use == Use::denied) {
            send(Status::denied);
            return;
        }
        if (final.use != Use::allowed || final.attempts != stage.initial.attempts)
            return;
        const auto events = render_selected(stage.selectors, final.attempts);
        if (!events)
            return;
        if (events->missing) {
            send(Status::selection_unavailable, {}, final.expires);
            return;
        }
        detail::View prepared;
        if (stage.snapshot) {
            std::unique_lock lock(stage.snapshot->mutex, std::try_to_lock);
            if (!lock || !stage.snapshot->result)
                return; // The private preparation has not completed.
            prepared = std::move(*stage.snapshot->result);
        }
        state.logs.publish(std::move(prepared), [&](detail::View view) {
            QJsonArray logs;
            for (const auto &row : view.rows)
                logs.append(detail::encode(row));
            // Only bounded typed projections build the artifact. There is no
            // general JSON input or way to add members at the publication gate.
            QJsonObject object{
                {"format", "subathon.diagnostic-export.v1"},
                {"versions", QJsonObject{{"server", version_token(stage.versions.server)},
                                         {"qt_runtime", version_token(stage.versions.qt_runtime)}}},
                {"connections", stage.connections},
                {"logs", logs},
                {"logs_available", view.available},
                {"logs_limited", view.limited}};
            if (!events->objects.empty())
                object.insert("events", events->objects);
            auto data = QJsonDocument(object).toJson(QJsonDocument::Compact);
            if (data.size() <= 128 * 1024)
                send(Status::artifact, std::move(data), final.expires);
        });
    } catch (...) {
        // Every failure discards the private reservation and has no implicit retry.
    }
}

void Export::cancel(Ticket ticket) {
    if (!impl_->in_turn && impl_->stage && impl_->stage->ticket == ticket) {
        abandon(*impl_->stage);
        impl_->stage.reset();
    }
}
} // namespace subathon::server::diagnostics
