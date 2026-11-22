// SPDX-License-Identifier: GPL-3.0-or-later
#include "permission_graph_codec.hpp"
#include "sqlite_rate_override_codec.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <set>
#include <subathon/core/permission_graph_validation.hpp>
#include <subathon/server/sqlite_credential_catalog.hpp>
#include <tuple>

namespace subathon::server {
namespace {
const QStringList rate_ddl{
    "CREATE TABLE subathon_rate_observations("
    "observation_ref BLOB PRIMARY KEY NOT NULL CHECK(typeof(observation_ref)='blob' AND "
    "length(observation_ref) BETWEEN 1 AND 256),"
    "format BLOB NOT NULL CHECK(typeof(format)='blob' AND length(format) BETWEEN 1 AND 128),"
    "document BLOB NOT NULL CHECK(typeof(document)='blob' AND length(document) BETWEEN 4 AND "
    "32768))",
    "CREATE TABLE subathon_rate_revisions("
    "snapshot_ref BLOB PRIMARY KEY NOT NULL CHECK(typeof(snapshot_ref)='blob' AND "
    "length(snapshot_ref) BETWEEN 1 AND 256),"
    "parent_ref BLOB UNIQUE REFERENCES subathon_rate_revisions(snapshot_ref) "
    "CHECK(parent_ref IS NULL OR (typeof(parent_ref)='blob' AND length(parent_ref) BETWEEN 1 AND "
    "256)),"
    "format BLOB NOT NULL CHECK(typeof(format)='blob' AND length(format) BETWEEN 1 AND 128),"
    "document BLOB NOT NULL CHECK(typeof(document)='blob' AND length(document) BETWEEN 4 AND "
    "32768),"
    "CHECK((snapshot_ref=X'45' AND parent_ref IS NULL) OR (snapshot_ref<>X'45' AND parent_ref IS "
    "NOT NULL AND snapshot_ref<>parent_ref)))",
    "CREATE TABLE subathon_rate_decisions("
    "decision_ref BLOB PRIMARY KEY NOT NULL CHECK(typeof(decision_ref)='blob' AND "
    "length(decision_ref) BETWEEN 1 AND 256),"
    "installation_ref BLOB NOT NULL CHECK(typeof(installation_ref)='blob' AND "
    "length(installation_ref) BETWEEN 1 AND 4096),"
    "operation_key BLOB NOT NULL CHECK(typeof(operation_key)='blob' AND length(operation_key) "
    "BETWEEN 1 AND 256),"
    "base_ref BLOB NOT NULL REFERENCES subathon_rate_revisions(snapshot_ref) "
    "CHECK(typeof(base_ref)='blob' AND length(base_ref) BETWEEN 1 AND 256),"
    "successor_ref BLOB UNIQUE NOT NULL REFERENCES subathon_rate_revisions(snapshot_ref) "
    "CHECK(typeof(successor_ref)='blob' AND length(successor_ref) BETWEEN 1 AND 256),"
    "intent_format BLOB NOT NULL CHECK(typeof(intent_format)='blob' AND length(intent_format) "
    "BETWEEN 1 AND 128),"
    "intent BLOB NOT NULL CHECK(typeof(intent)='blob' AND length(intent) BETWEEN 4 AND 229376),"
    "decision_format BLOB NOT NULL CHECK(typeof(decision_format)='blob' AND "
    "length(decision_format) BETWEEN 1 AND 128),"
    "decision BLOB NOT NULL CHECK(typeof(decision)='blob' AND length(decision) BETWEEN 4 AND "
    "32768),"
    "UNIQUE(installation_ref,operation_key), CHECK(base_ref<>successor_ref))",
    "CREATE TABLE subathon_rate_occurrences("
    "override_ref BLOB PRIMARY KEY NOT NULL CHECK(typeof(override_ref)='blob' AND "
    "length(override_ref) BETWEEN 1 AND 256),"
    "decision_ref BLOB UNIQUE NOT NULL REFERENCES subathon_rate_decisions(decision_ref) "
    "CHECK(typeof(decision_ref)='blob' AND length(decision_ref) BETWEEN 1 AND 256),"
    "revision_ref BLOB NOT NULL REFERENCES subathon_rate_revisions(snapshot_ref) "
    "CHECK(typeof(revision_ref)='blob' AND length(revision_ref) BETWEEN 1 AND 256),"
    "body BLOB NOT NULL CHECK(typeof(body)='blob' AND length(body) BETWEEN 1 AND 5535))",
    "CREATE TABLE subathon_rate_head("
    "slot BLOB PRIMARY KEY NOT NULL CHECK(typeof(slot)='blob' AND slot=X'48'),"
    "revision_ref BLOB NOT NULL REFERENCES subathon_rate_revisions(snapshot_ref) "
    "CHECK(typeof(revision_ref)='blob' AND length(revision_ref) BETWEEN 1 AND 256))",
};

} // namespace

MigrationManifest rate_overrides_repository_manifest() {
    auto manifest = provider_credentials_repository_manifest();
    MigrationStep step{17, "rate-overrides-v1", rate_ddl, manifest.steps.back().expected_schema};
    const QStringList names{"subathon_rate_observations", "subathon_rate_revisions",
                            "subathon_rate_decisions", "subathon_rate_occurrences",
                            "subathon_rate_head"};
    const QList<int> indexes{1, 2, 3, 2, 1};
    for (qsizetype i = 0; i < names.size(); ++i) {
        const auto &name = names[i];
        step.expected_schema.append({"table", name, name, rate_ddl[i]});
        for (int index = 1; index <= indexes[i]; ++index)
            step.expected_schema.append(
                {"index", "sqlite_autoindex_" + name + '_' + QString::number(index), name, {}});
    }
    std::ranges::sort(step.expected_schema, [](const auto &a, const auto &b) {
        return std::tie(a.type, a.name) < std::tie(b.type, b.name);
    });
    step.sql.append(
        "INSERT INTO subathon_rate_revisions VALUES("
        "X'45',NULL,X'"
        "7375626174686f6e2e696e7465726e616c2e76616c756174696f6e2d736e617073686f742e7631',"
        "X'525356310000000145000000000000000000000000000000000000')");
    step.sql.append("INSERT INTO subathon_rate_head VALUES(X'48',X'45')");
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions.append(17);
    return manifest;
}

namespace {
using namespace rate_sql;
constexpr auto invalid_input = Refusal::invalid_proposal;
constexpr auto rate_operation = "rates.override.edit.v1";
constexpr auto rate_catalog = "permission-rates-v1";

std::string pair_scope(std::string_view left, std::string_view right) {
    if (byte_less(right, left))
        std::swap(left, right);
    QByteArray framed;
    WireWriter writer(136, invalid_input, &framed);
    writer.bounded(left, 64);
    writer.bounded(right, 64);
    return framed.toHex().toStdString();
}

bool valid_scope(std::string_view scope) {
    // Decode lower-case hex into a fixed buffer before taking either field.
    if (scope.size() > 272 || scope.size() % 2)
        return false;
    std::array<char, 136> bytes{};
    const auto digit = [](char c) -> unsigned {
        if (c >= '0' && c <= '9')
            return c - '0';
        return c >= 'a' && c <= 'f' ? c - 'a' + 10 : 16;
    };
    for (std::size_t i = 0; i < scope.size(); i += 2) {
        const auto high = digit(scope[i]), low = digit(scope[i + 1]);
        if (high >= 16 || low >= 16)
            return false;
        bytes[i / 2] = static_cast<char>(high * 16 + low);
    }
    std::string_view rest(bytes.data(), scope.size() / 2);
    const auto field = [&]() -> std::string_view {
        if (rest.size() < 4)
            return {};
        std::uint32_t size = 0;
        for (unsigned char byte : rest.substr(0, 4))
            size = (size << 8) | byte;
        rest.remove_prefix(4);
        if (size == 0 || size > 64 || size > rest.size())
            return {};
        const auto value = rest.substr(0, size);
        rest.remove_prefix(size);
        return value;
    };
    const auto low = field();
    const auto high = field();
    return !low.empty() && !high.empty() && rest.empty() && byte_less(low, high);
}

void validate_rate_scopes(const core::PermissionEditableState &state) {
    const auto entries = [](const auto &rows) {
        for (const auto &entry : rows) {
            if (entry.operation != rate_operation)
                continue;
            require(entry.resources.size() == 1 && entry.resources.front().slot == "pair" &&
                        entry.quantities.empty(),
                    Refusal::denied);
            const auto &resource = entry.resources.front();
            if (resource.mode == core::PermissionResourceMode::any) {
                require(entry.effect == core::PermissionEffect::deny && resource.allowed.empty(),
                        Refusal::denied);
            } else {
                require(resource.mode == core::PermissionResourceMode::allowed_set &&
                            !resource.allowed.empty() &&
                            std::ranges::all_of(resource.allowed, valid_scope),
                        Refusal::denied);
            }
        }
    };
    for (const auto &role : state.roles)
        entries(role.entries);
    for (const auto &account : state.accounts)
        entries(account.individual_entries);
}

void validate_edit(const RateOverrideEdit &edit) {
    WireWriter scan(intent_bytes, invalid_input);
    edit_fields(scan, edit);
    const auto &manual = edit.override_input;
    require(manual.actor_ref && !manual.recorded_at_utc_ms &&
                manual.from_currency != manual.to_currency,
            invalid_input);
}

Audit authorize(const SqliteWorkSession &session, RateAuthoritySource &source,
                const PermissionGraphTrust &trust, const RateOverrideEdit &edit) {
    using namespace core;
    auto resolution = source.resolve(session);
    if (const auto *error = std::get_if<SessionFailure>(&resolution)) {
        if (*error == SessionFailure::resource_limit)
            throw Refusal::resource_limit;
        if (*error == SessionFailure::unavailable || *error == SessionFailure::invalid_stored_data)
            throw Refusal::unavailable;
        throw Refusal::denied;
    }
    if (const auto *error = std::get_if<PermissionGraphFailure>(&resolution))
        throw *error == PermissionGraphFailure::resource_limit ? Refusal::resource_limit
                                                               : Refusal::unavailable;
    const auto &facts = std::get<RateAuthorityFacts>(resolution);
    const auto &resolved = facts.session;
    const auto &graph = facts.graph;
    require(resolved.installation_ref == edit.installation &&
                edit.override_input.actor_ref == resolved.account_ref &&
                graph.installation == edit.installation &&
                graph.current_installation.current.installation_ref == edit.installation,
            Refusal::denied);
    require(graph.revision > 0 && graph.use.state == PermissionFactsState::complete &&
                graph.grant.state == PermissionFactsState::complete,
            Refusal::unavailable);
    auto expected = reference_permission_graph_trust();
    expected.catalog.revision = rate_catalog;
    expected.catalog.operations.push_back(
        {rate_operation, PermissionOperationClass::ordinary, {"pair"}, {}, {}});
    const PermissionGraphLimits graph_limits;
    const auto matches = [](const auto &candidate, const QByteArray &canonical) {
        permission_graph::Encoder check{view(canonical)};
        check(candidate);
        check.finish();
    };
    try {
        matches(trust, permission_graph::encode(expected, graph_limits));
        matches(graph.catalog, permission_graph::encode(expected.catalog, graph_limits));
        // Bound both borrowed policy channels before inspecting or projecting them.
        permission_graph::Projection projection{graph_limits, graph_limits.selected_bytes};
        projection(graph.use, graph.grant, graph.current_installation, graph.current_accounts);
        permission_graph::data(!projection.empty);
    } catch (const PermissionGraphStorageFailure &failure) {
        throw failure.code() == PermissionGraphFailure::resource_limit ? Refusal::resource_limit
                                                                       : Refusal::unavailable;
    }
    validate_rate_scopes(graph.use);
    validate_rate_scopes(graph.grant);
    internal::validate_permission_graph(graph.catalog, graph.use);
    internal::validate_permission_graph(graph.catalog, graph.grant);

    std::set<std::string_view> roster;
    bool enabled = false;
    for (const auto &row : graph.current_accounts) {
        require(row.current.installation_ref == edit.installation &&
                    roster.insert(row.current.account_ref).second,
                Refusal::unavailable);
        if (row.current.account_ref == resolved.account_ref)
            enabled = row.current.application_access_enabled;
    }
    require(enabled, Refusal::denied);
    for (const auto *channel : {&graph.use, &graph.grant}) {
        require(channel->accounts.size() == roster.size(), Refusal::unavailable);
        for (const auto &row : channel->accounts)
            require(roster.contains(row.account), Refusal::unavailable);
    }

    const auto account = std::ranges::find(graph.use.accounts, resolved.account_ref,
                                           &PermissionEditableAccount::account);
    require(account != graph.use.accounts.end(), Refusal::denied);
    PermissionPolicyFacts policy{PermissionFactsState::complete, account->assigned_roles,
                                 account->individual_entries};
    for (const auto &role : graph.use.roles) {
        if (std::ranges::find(account->assigned_roles, role.role) != account->assigned_roles.end())
            policy.entries.insert(policy.entries.end(), role.entries.begin(), role.entries.end());
    }
    const auto scope =
        pair_scope(edit.override_input.from_currency, edit.override_input.to_currency);
    // The session snapshot is also captured in the immutable audit.
    const auto permit = evaluate_effective_permission({resolved.account_ref,
                                                       resolved.snapshot_ref,
                                                       graph.catalog,
                                                       graph.owner,
                                                       std::move(policy),
                                                       {rate_operation, {{"pair", scope}}, {}}});
    const auto &decision = permit.data();
    require(decision.outcome == PermissionOutcome::permit && decision.witness_row &&
                *decision.witness_row < decision.sources.size(),
            Refusal::denied);
    const auto &witness = decision.sources[*decision.witness_row];
    require(witness.grant &&
                (witness.kind == PermissionSourceKind::role ||
                 witness.kind == PermissionSourceKind::individual) &&
                witness.role.has_value() == (witness.kind == PermissionSourceKind::role),
            Refusal::denied);
    Audit audit{edit.installation,
                edit.operation,
                edit.override_input.decision_ref,
                edit.base,
                edit.successor,
                edit.override_input.override_ref,
                edit.replacement,
                resolved.account_ref,
                resolved.session_ref,
                resolved.record_revision_ref,
                resolved.snapshot_ref,
                decision.catalog_revision,
                graph.revision,
                witness.source,
                *witness.grant,
                witness.role,
                {}};
    WireWriter scan(document_bytes, Refusal::unavailable);
    audit_fields(scan, audit);
    return audit;
}

struct PreparedBasis {
    std::vector<core::FetchedRateObservation> facts;
    std::vector<QByteArray> documents;
};

void check_trace(const std::vector<RateSourceEdge> &edges, const PreparedBasis &basis,
                 Refusal invalid) {
    for (const auto &edge : edges)
        require(edge.observation_index < basis.facts.size() && edge.quote_ordinal > 0 &&
                    edge.quote_ordinal <= basis.facts[edge.observation_index].data().quotes.size(),
                invalid);
}

QByteArray observation_document(const auto &source, Refusal invalid) {
    return encode("ROB1", document_bytes, source, observation_fields, invalid);
}

PreparedBasis prepare_basis(const RateSourceBasis &basis) {
    fit(basis.observations.size() <= observation_count && basis.edges.size() <= edge_count);
    WireWriter scan(intent_bytes, invalid_input);
    scan.bounded(basis.policy_ref, 256);
    std::array<std::string_view, observation_count> refs{};
    auto next = refs.begin();
    std::size_t encoded = 0;
    // Preflight the complete basis before copying any model or document.
    for (const auto &source : basis.observations) {
        WireWriter measure(document_bytes, invalid_input);
        measure.literal("ROB1");
        observation_fields(measure, source);
        measure.observation_budget();
        charge(encoded, measure.size(), observation_count * document_bytes);
        require(std::find(refs.begin(), next, source.context.observation_ref) == next,
                invalid_input);
        *next++ = source.context.observation_ref;
    }
    for (const auto &edge : basis.edges)
        require(edge.observation_index < basis.observations.size() && edge.quote_ordinal > 0 &&
                    edge.quote_ordinal <= basis.observations[edge.observation_index].quotes.size(),
                invalid_input);
    PreparedBasis result;
    for (const auto &source : basis.observations) {
        result.facts.push_back(core::make_fetched_rate_observation(source, storage_limits));
        result.documents.push_back(observation_document(source, invalid_input));
    }
    return result;
}

QByteArray intent_document(const RateOverrideEdit &edit, const RateSourceBasis &basis) {
    fit(basis.observations.size() <= observation_count && basis.edges.size() <= edge_count);
    // Borrow the (checked) source identities and trace directly.
    const auto fields = [&](auto &wire, const RateOverrideEdit &value) {
        edit_fields(wire, value);
        wire.bounded(basis.policy_ref, 256);
        wire.list(basis.observations, observation_count,
                  [&](const auto &source) { wire.bounded(source.context.observation_ref, 256); });
        wire.list(basis.edges, edge_count, [&](const auto &edge) { wire.edge(edge); });
    };
    return encode("RIN1", intent_bytes, edit, fields, invalid_input);
}

QByteArray snapshot_document(const core::EffectiveRateSnapshot &snapshot) {
    return blob(core::encode_valuation_snapshot(snapshot).document);
}

QByteArray snapshot_document(const core::EffectiveRateSnapshotInput &candidate) {
    return snapshot_document(core::make_effective_rate_snapshot(candidate, storage_limits));
}

core::RateOverridePreview propose(const core::EffectiveRateSnapshot &base,
                                  const RateOverrideEdit &edit, const PreparedBasis &basis,
                                  const std::vector<RateSourceEdge> &trace) {
    std::vector<core::RateOverrideFetchedEdge> edges;
    edges.reserve(trace.size());
    for (const auto &edge : trace)
        edges.push_back({basis.facts[edge.observation_index], edge.quote_ordinal});
    return core::preview_rate_override(base, edit.override_input, edges, edit.successor,
                                       edit.replacement);
}

// Borrow the two integer backends to reject an oversized supplied rational
// before the public numerator/denominator accessors make owning copies.
template <class Rational> bool fitting_pair_storage(const Rational &rate) {
    const auto &backend = rate.backend();
    using boost::multiprecision::limb_type;
    constexpr auto limbs = 8192 / std::numeric_limits<limb_type>::digits + 1;
    if constexpr (requires {
                      backend.num();
                      backend.denom();
                  })
        return backend.num().size() <= limbs && backend.denom().size() <= limbs;
    else
        return backend.data().numerator().backend().size() <= limbs &&
               backend.data().denominator().backend().size() <= limbs;
}

// Candidate fields have already passed the tighter RSV1 profile. Continue the
// pure preview's raw byte and item accounting through replacement and deltas.
void check_preview_budget(const core::RateOverridePreview &preview, Refusal invalid) {
    constexpr core::RateSnapshotLimits limits;
    const auto &candidate = preview.candidate;
    WireWriter raw(intent_bytes, invalid);
    raw.bounded(candidate.snapshot_ref, 256);
    raw.optional_ref(candidate.parent_snapshot_ref, 256);
    raw.instant(candidate.constructed_at_utc_ms);
    std::size_t items = candidate.sources.size() + candidate.overrides.size() +
                        candidate.constraints.size() + candidate.components.size();
    for (const auto &source : candidate.sources) {
        observation_fields(raw, source);
        items += source.quotes.size();
    }
    for (const auto &override : candidate.overrides)
        override_fields(raw, override);
    std::size_t scalar_bytes = candidate.overrides.size() + candidate.constraints.size();
    for (const auto &constraint : candidate.constraints) {
        if (const auto *edge = std::get_if<core::FetchedRateConstraint>(&constraint)) {
            raw.bounded(edge->observation_ref, 256);
            scalar_bytes += 8;
        } else {
            raw.bounded(std::get<core::ManualRateConstraint>(constraint).override_ref, 256);
        }
    }
    for (const auto &component : candidate.components) {
        items += component.members.size();
        for (const auto &member : component.members) {
            raw.bounded(member.currency, 64);
            raw.bounded(member.numerator, 1234);
            raw.bounded(member.denominator, 1234);
        }
    }
    raw.optional_ref(preview.replaced_override_ref, 256);
    charge(items, preview.affected_rates.size(), limits.items);
    auto bytes = raw.raw_size() + scalar_bytes;
    const auto exact = [&](const std::optional<core::Exact> &rate) {
        if (!rate)
            return;
        fit(fitting_pair_storage(*rate));
        const auto numerator = boost::multiprecision::numerator(*rate);
        const auto denominator = boost::multiprecision::denominator(*rate);
        require(numerator > 0, invalid);
        fit(boost::multiprecision::msb(numerator) < 8192 &&
            boost::multiprecision::msb(denominator) < 8192);
        charge(bytes, numerator.str().size(), limits.owned_bytes);
        charge(bytes, denominator.str().size(), limits.owned_bytes);
    };
    for (const auto &change : preview.affected_rates) {
        fit(change.from_currency.size() <= 64 && change.to_currency.size() <= 64);
        charge(bytes, change.from_currency.size() + change.to_currency.size(), limits.owned_bytes);
        exact(change.before);
        exact(change.after);
    }
}

bool same_changes(const std::vector<core::RatePairChange> &left,
                  const std::vector<core::RatePairChange> &right) {
    return std::ranges::equal(left, right, [](const auto &a, const auto &b) {
        return a.from_currency == b.from_currency && a.to_currency == b.to_currency &&
               a.before == b.before && a.after == b.after;
    });
}

bool matches_proposal(const core::RateOverridePreview &submitted,
                      const core::RateOverridePreview &computed, const QByteArray &canonical,
                      Refusal invalid) {
    const auto candidate = snapshot_document(submitted.candidate);
    check_preview_budget(submitted, invalid);
    return candidate == canonical &&
           submitted.replaced_override_ref == computed.replaced_override_ref &&
           same_changes(submitted.affected_rates, computed.affected_rates);
}

RateReceiptSelection receipt(RateReceiptOrigin origin, const RateOverrideEdit &edit,
                             core::RateOverridePreview proposal) {
    return {origin,
            {edit.override_input.decision_ref, edit.base, edit.successor,
             edit.override_input.override_ref, edit.replacement,
             std::move(proposal.affected_rates)}};
}

// A successful empty exact selection is the sole source of "absent." Metadata
// is read first, bounded to two rows, and only a fitting singleton may be hydrated.
struct Column {
    QString name;
    std::size_t minimum, maximum;
    bool nullable = false;
};

class RateRows {
  public:
    explicit RateRows(const SqliteWorkSession &session) : session_(session) {
        const auto marker = session_.execute(
            {"SELECT CASE WHEN typeof(version)='integer' AND "
             "((version=17 AND migration_id COLLATE BINARY='rate-overrides-v1') OR "
             "(version=18 AND migration_id COLLATE BINARY='milestone-target-edits-v1') OR "
             "(version=19 AND migration_id COLLATE BINARY='contribution-review-v1') OR "
             "(version=20 AND migration_id COLLATE BINARY='connector-intake-storage-v1') OR "
             "(version=21 AND migration_id COLLATE BINARY="
             "'milestone-slot-interruption-storage-v1')) "
             "THEN 1 ELSE 0 END "
             "FROM main.subathon_migration_history WHERE EXISTS(SELECT 1 FROM "
             "main.subathon_catalog "
             "WHERE singleton=1 AND typeof(catalog_format)='integer' AND catalog_format=1 "
             "AND store_identity='subathon.sqlite-store') ORDER BY version DESC LIMIT 1",
             {}});
        require(marker.size() == 1 && marker.front().front().toInt() == 1, Refusal::unavailable);
    }

    std::optional<SqlRow> selected(const QString &table, const QString &where,
                                   const QVariantList &keys,
                                   std::initializer_list<Column> columns) const {
        QStringList metadata, payload;
        for (const auto &column : columns) {
            metadata.append("typeof(" + column.name + ")");
            metadata.append("length(CAST(" + column.name + " AS BLOB))");
            payload.append(column.name);
        }
        const auto tail = " FROM main." + table + where + " LIMIT 2";
        const auto shape = session_.execute({"SELECT " + metadata.join(',') + tail, keys});
        if (shape.empty())
            return std::nullopt;
        require(shape.size() == 1 && shape.front().size() == metadata.size());
        qsizetype i = 0;
        for (const auto &column : columns) {
            const auto &type = shape.front()[i++];
            const auto &length = shape.front()[i++];
            if (column.nullable && type == QString("null")) {
                require(length.isNull());
                continue;
            }
            require(type == QString("blob") && !length.isNull());
            const auto size = length.toLongLong();
            require(size >= static_cast<qlonglong>(column.minimum));
            fit(size <= static_cast<qlonglong>(column.maximum));
        }
        auto rows = session_.execute({"SELECT " + payload.join(',') + tail, keys});
        require(rows.size() == 1 && rows.front().size() == payload.size());
        i = 0;
        for (const auto &column : columns) {
            const auto &value = rows.front()[i];
            const auto &length = shape.front()[2 * i + 1];
            ++i;
            if (column.nullable && length.isNull())
                require(value.isNull());
            else
                require(value.metaType().id() == QMetaType::QByteArray &&
                        value.toByteArray().size() == length.toLongLong());
        }
        return std::move(rows.front());
    }

    std::optional<SqlRow> revision(std::string_view ref) const {
        return selected("subathon_rate_revisions", key_filter("snapshot_ref"), key(ref),
                        {{"snapshot_ref", 1, 256},
                         {"parent_ref", 1, 256, true},
                         {"format", 1, 128},
                         {"document", 4, document_bytes}});
    }
    std::optional<SqlRow> observation(std::string_view ref) const {
        return selected(
            "subathon_rate_observations", key_filter("observation_ref"), key(ref),
            {{"observation_ref", 1, 256}, {"format", 1, 128}, {"document", 4, document_bytes}});
    }
    std::optional<SqlRow> occurrence(const QString &column, std::string_view ref) const {
        return selected("subathon_rate_occurrences", key_filter(column), key(ref),
                        {{"override_ref", 1, 256},
                         {"decision_ref", 1, 256},
                         {"revision_ref", 1, 256},
                         {"body", 1, occurrence_bytes}});
    }
    std::optional<SqlRow> decision(const QString &where, const QVariantList &keys) const {
        return selected("subathon_rate_decisions", where, keys,
                        {{"decision_ref", 1, 256},
                         {"installation_ref", 1, 4096},
                         {"operation_key", 1, 256},
                         {"base_ref", 1, 256},
                         {"successor_ref", 1, 256},
                         {"intent_format", 1, 128},
                         {"intent", 4, intent_bytes},
                         {"decision_format", 1, 128},
                         {"decision", 4, document_bytes}});
    }
    bool has(const QString &table, const QString &column, std::string_view ref) const {
        const auto rows = session_.execute({"SELECT CASE WHEN typeof(" + column +
                                                ")='blob' THEN 1 ELSE 0 END FROM main." + table +
                                                key_filter(column) + " LIMIT 2",
                                            key(ref)});
        if (rows.empty())
            return false;
        require(rows.size() == 1 && rows.front().front().toInt() == 1);
        return true;
    }
    std::string head() const {
        const auto row =
            selected("subathon_rate_head", {}, {}, {{"slot", 1, 1}, {"revision_ref", 1, 256}});
        require(row && (*row)[0].toByteArray() == "H");
        return (*row)[1].toByteArray().toStdString();
    }
    static QString key_filter(const QString &column) { return " WHERE " + key_match(column); }
    static QString key_match(const QString &column) {
        // Detect wrong-class TEXT with the requested bytes rather than mistaking
        // a damaged selected key for a missing row.
        return column + " COLLATE BINARY IN (?,CAST(? AS TEXT))";
    }
    static QVariantList key(std::string_view ref) {
        auto bytes = blob(ref);
        return {bytes, bytes};
    }

  private:
    const SqliteWorkSession &session_;
};

void format(const QVariant &field, std::string_view expected) {
    require(view(field.toByteArray()) == expected, Refusal::unsupported_retained_data);
}

const QByteArray root_document =
    QByteArray::fromHex("525356310000000145000000000000000000000000000000000000");

struct Revision {
    core::EffectiveRateSnapshot snapshot;
    QByteArray document;
};

Revision read_revision(const RateRows &rows, std::string_view ref) {
    const auto row = rows.revision(ref);
    require(row && view((*row)[0].toByteArray()) == ref);
    format((*row)[2], core::valuation_snapshot_format_ref);
    auto document = (*row)[3].toByteArray();
    auto snapshot =
        core::decode_valuation_snapshot(core::valuation_snapshot_format_ref, view(document));
    const auto &model = snapshot.data();
    require(model.snapshot_ref == ref);
    if (ref == "E") {
        require((*row)[1].isNull() && document == root_document);
    } else {
        require(!(*row)[1].isNull() && model.parent_snapshot_ref &&
                *model.parent_snapshot_ref != ref &&
                view((*row)[1].toByteArray()) == *model.parent_snapshot_ref);
    }
    std::set<std::string> pairs;
    for (const auto &manual : model.overrides)
        require(pairs.insert(pair_scope(manual.from_currency, manual.to_currency)).second);
    return {std::move(snapshot), std::move(document)};
}

struct StoredDecision {
    Intent intent;
    Audit audit;
    QByteArray intent_document;
};

StoredDecision read_decision(const SqlRow &row) {
    format(row[5], intent_format);
    format(row[7], decision_format);
    auto intent_document = row[6].toByteArray();
    auto intent = decode<Intent>("RIN1", intent_bytes, intent_document, intent_fields);
    auto audit = decode<Audit>("RDC1", document_bytes, row[8].toByteArray(), audit_fields);
    const auto &edit = intent.edit;
    const auto &entry = edit.override_input;
    require(
        view(row[0].toByteArray()) == audit.decision &&
        view(row[1].toByteArray()) == audit.installation &&
        view(row[2].toByteArray()) == audit.operation && view(row[3].toByteArray()) == audit.base &&
        view(row[4].toByteArray()) == audit.successor && audit.installation == edit.installation &&
        audit.operation == edit.operation && audit.decision == entry.decision_ref &&
        audit.base == edit.base && audit.successor == edit.successor &&
        audit.introduced == entry.override_ref && audit.replaced == edit.replacement &&
        entry.actor_ref == audit.account && !entry.recorded_at_utc_ms &&
        audit.catalog == rate_catalog && audit.graph_revision > 0 && edit.base != edit.successor &&
        edit.successor != "E" && entry.from_currency != entry.to_currency);
    return {std::move(intent), std::move(audit), std::move(intent_document)};
}

struct StoredObservation {
    core::FetchedRateObservation observation;
    QByteArray document;
};

StoredObservation read_observation(const SqlRow &row, std::string_view ref) {
    require(view(row[0].toByteArray()) == ref);
    format(row[1], observation_format);
    auto document = row[2].toByteArray();
    const auto input = decode<core::FetchedRateObservationInput>("ROB1", document_bytes, document,
                                                                 observation_fields);
    require(input.context.observation_ref == ref);
    return {core::make_fetched_rate_observation(input, storage_limits), std::move(document)};
}

class Closure {
  public:
    explicit Closure(const RateRows &rows) : rows_(rows) {}

    const StoredObservation &observation(std::string_view ref) {
        if (auto existing = observations_.find(ref); existing != observations_.end())
            return existing->second;
        const auto row = rows_.observation(ref);
        require(row.has_value());
        return observations_.emplace(ref, read_observation(*row, ref)).first->second;
    }

    void embedded_observations(const Revision &revision) {
        for (const auto &source : revision.snapshot.data().sources) {
            const auto &stored = observation(source.data().context.observation_ref);
            require(observation_document(source.data(), Refusal::invalid_retained_data) ==
                    stored.document);
        }
    }

    // Verify the introducing decision and exact body of every distinct
    // occurrence, including survivors. No recursive expansion of history.
    void occurrences(const Revision &revision, const StoredDecision *selected = nullptr) {
        for (const auto &manual : revision.snapshot.data().overrides) {
            auto body = encode("", occurrence_bytes, manual, override_fields,
                               Refusal::invalid_retained_data);
            if (const auto cached = occurrences_.find(manual.override_ref);
                cached != occurrences_.end()) {
                require(cached->second == body);
                continue;
            }
            const auto row = rows_.occurrence("override_ref", manual.override_ref);
            require(row && view((*row)[0].toByteArray()) == manual.override_ref &&
                    view((*row)[1].toByteArray()) == manual.decision_ref &&
                    (*row)[3].toByteArray() == body);
            const auto check_introduction = [&](const StoredDecision &decision) {
                require(decision.audit.decision == manual.decision_ref &&
                        decision.audit.introduced == manual.override_ref &&
                        view((*row)[2].toByteArray()) == decision.audit.successor &&
                        encode("", occurrence_bytes, decision.intent.edit.override_input,
                               override_fields, Refusal::invalid_retained_data) == body);
            };
            if (selected && selected->audit.decision == manual.decision_ref) {
                check_introduction(*selected);
            } else {
                const auto decision = rows_.decision(RateRows::key_filter("decision_ref"),
                                                     RateRows::key(manual.decision_ref));
                require(decision.has_value());
                const auto introducing = read_decision(*decision);
                check_introduction(introducing);
                // Validate the separate relational target without opening an
                // ancestor document or following its own decision closure.
                require(rows_.has("subathon_rate_revisions", "snapshot_ref",
                                  introducing.audit.successor));
            }
            occurrences_.emplace(manual.override_ref, std::move(body));
        }
    }

    PreparedBasis basis(const Intent &intent) {
        PreparedBasis result;
        std::size_t bytes = 0;
        std::set<std::string_view> refs;
        for (const auto &ref : intent.observations) {
            require(refs.insert(ref).second);
            const auto &stored = observation(ref);
            charge(bytes, view(stored.document).size(), observation_count * document_bytes);
            result.facts.push_back(stored.observation);
            result.documents.push_back(stored.document);
        }
        check_trace(intent.edges, result, Refusal::invalid_retained_data);
        return result;
    }

  private:
    const RateRows &rows_;
    std::map<std::string, StoredObservation, std::less<>> observations_;
    std::map<std::string, QByteArray, std::less<>> occurrences_;
};

struct VerifiedDecision {
    PreparedBasis basis;
    core::RateOverridePreview proposal;
};

VerifiedDecision verify_decision(const RateRows &rows, Closure &closure,
                                 const StoredDecision &decision, const Revision &successor) {
    const auto &edit = decision.intent.edit;
    require(successor.snapshot.data().snapshot_ref == edit.successor &&
            successor.snapshot.data().parent_snapshot_ref == edit.base);
    const auto base = read_revision(rows, edit.base);
    const auto has_override = [](const Revision &rev, std::string_view ref) {
        return std::ranges::any_of(rev.snapshot.data().overrides,
                                   [&](const auto &entry) { return entry.override_ref == ref; });
    };
    require(!has_override(base, decision.audit.introduced) &&
            has_override(successor, decision.audit.introduced));
    if (edit.replacement)
        require(has_override(base, *edit.replacement) &&
                !has_override(successor, *edit.replacement));
    const auto intro = rows.occurrence("decision_ref", decision.audit.decision);
    require(intro && view((*intro)[0].toByteArray()) == decision.audit.introduced &&
            view((*intro)[1].toByteArray()) == decision.audit.decision &&
            view((*intro)[2].toByteArray()) == edit.successor);
    closure.occurrences(base, &decision);
    closure.occurrences(successor, &decision);
    auto basis = closure.basis(decision.intent);
    auto proposal = propose(base.snapshot, edit, basis, decision.intent.edges);
    require(snapshot_document(proposal.candidate) == successor.document &&
            proposal.replaced_override_ref == edit.replacement);
    return {std::move(basis), std::move(proposal)};
}

Revision verified_revision(const RateRows &rows, std::string_view ref) {
    auto revision = read_revision(rows, ref);
    if (ref == "E")
        return revision;
    Closure closure(rows);
    auto row = rows.decision(RateRows::key_filter("successor_ref"), RateRows::key(ref));
    if (row) {
        (void)verify_decision(rows, closure, read_decision(*row), revision);
    } else {
        // Accepted source updates can have no edit decision.
        const auto parent = read_revision(rows, *revision.snapshot.data().parent_snapshot_ref);
        for (const auto &survivor : revision.snapshot.data().overrides) {
            const auto &prior = parent.snapshot.data().overrides;
            const auto previous = std::ranges::find(prior, survivor.override_ref,
                                                    &core::ManualRateOverrideData::override_ref);
            require(previous != prior.end() &&
                    encode("", occurrence_bytes, *previous, override_fields,
                           Refusal::invalid_retained_data) ==
                        encode("", occurrence_bytes, survivor, override_fields,
                               Refusal::invalid_retained_data));
        }
        closure.embedded_observations(revision);
        closure.occurrences(revision);
    }
    return revision;
}

Revision active_revision(const RateRows &rows) {
    const auto active = rows.head();
    require(!rows.has("subathon_rate_revisions", "parent_ref", active));
    if (active != "E")
        (void)read_revision(rows, "E");
    return verified_revision(rows, active);
}

std::optional<StoredDecision> operation(const RateRows &rows, const RateOverrideEdit &edit) {
    auto keys = RateRows::key(edit.installation);
    keys.append(RateRows::key(edit.operation));
    const auto row = rows.decision(RateRows::key_filter("installation_ref") + " AND " +
                                       RateRows::key_match("operation_key"),
                                   keys);
    if (!row)
        return std::nullopt;
    auto stored = read_decision(*row);
    require(stored.audit.installation == edit.installation &&
            stored.audit.operation == edit.operation);
    return stored;
}

bool identity_present(const RateRows &rows, const RateOverrideEdit &edit) {
    return rows.has("subathon_rate_revisions", "snapshot_ref", edit.successor) ||
           rows.has("subathon_rate_decisions", "decision_ref", edit.override_input.decision_ref) ||
           rows.has("subathon_rate_decisions", "successor_ref", edit.successor) ||
           rows.has("subathon_rate_occurrences", "override_ref",
                    edit.override_input.override_ref) ||
           rows.has("subathon_rate_occurrences", "decision_ref",
                    edit.override_input.decision_ref) ||
           rows.has("subathon_rate_occurrences", "revision_ref", edit.successor);
}

void write_one(const SqliteWorkSession &session, const SqlStatement &statement) {
    const auto rows = session.execute(statement);
    // A late CAS miss or ignored insert is a work failure: let the owning unit
    // unwind and roll back instead of returning a committable typed refusal.
    if (rows.size() != 1 || rows.front().size() != 1 || rows.front().front().toInt() != 1)
        throw std::runtime_error("Rate override write cardinality changed");
}

template <class Body> auto model_context(Refusal invalid_model, Body body) {
    try {
        return body();
    } catch (const core::RateSnapshotFailure &failure) {
        throw failure.code() == core::RateSnapshotError::resource_limit ? Refusal::resource_limit
                                                                        : invalid_model;
    } catch (const core::ExactValuationFailure &failure) {
        switch (failure.code()) {
        case core::ExactValuationError::allocation_failure:
        case core::ExactValuationError::resource_limit:
            throw Refusal::resource_limit;
        case core::ExactValuationError::unsupported_format:
            throw Refusal::unsupported_retained_data;
        default:
            throw invalid_model;
        }
    }
}

template <class Result, class Body> Result rate_result(Refusal invalid_model, Body body) {
    try {
        return model_context(invalid_model, body);
    } catch (Refusal refusal) {
        return refusal;
    } catch (const core::EffectivePermissionFailure &failure) {
        using Error = core::EffectivePermissionError;
        switch (failure.code()) {
        case Error::input_limit:
        case Error::result_limit:
        case Error::allocation_failure:
            return Refusal::resource_limit;
        case Error::facts_unavailable:
            return Refusal::unavailable;
        default:
            return Refusal::denied;
        }
    } catch (const std::bad_alloc &) {
        return Refusal::resource_limit;
    }
}

template <class Body> auto retained(Body body) {
    return model_context(Refusal::invalid_retained_data, body);
}

RateReceiptSelection retry(const RateRows &rows, const RateOverrideEdit &edit,
                           const RateEditPreview &preview, const StoredDecision &stored) {
    constexpr auto conflict = Refusal::identity_conflict;
    // Verify the retained closure before comparing the submitted intent and model.
    const auto successor = read_revision(rows, stored.audit.successor);
    Closure closure(rows);
    auto verified = verify_decision(rows, closure, stored, successor);
    require(intent_document(edit, preview.basis) == stored.intent_document &&
                intent_document(preview.edit, preview.basis) == stored.intent_document,
            conflict);
    model_context(conflict, [&] {
        require(
            prepare_basis(preview.basis).documents == verified.basis.documents &&
                matches_proposal(preview.proposal, verified.proposal, successor.document, conflict),
            conflict);
    });
    return receipt(RateReceiptOrigin::retained, stored.intent.edit, std::move(verified.proposal));
}
} // namespace

RateRevisionResult SqliteRateOverrideUnit::head() const {
    return rate_result<RateRevisionResult>(Refusal::invalid_retained_data, [&] {
        return active_revision(RateRows(session_)).snapshot;
    });
}

RateRevisionResult SqliteRateOverrideUnit::revision(std::string_view ref) const {
    return rate_result<RateRevisionResult>(Refusal::invalid_retained_data, [&] {
        fit(ref.size() <= 256);
        require(!ref.empty());
        return verified_revision(RateRows(session_), ref).snapshot;
    });
}

RatePreviewResult SqliteRateOverrideUnit::preview(RateAuthoritySource &source,
                                                  const PermissionGraphTrust &trust,
                                                  const RateOverrideEdit &edit,
                                                  const RateSourceBasis &basis) const {
    return rate_result<RatePreviewResult>(invalid_input, [&]() -> RatePreviewResult {
        validate_edit(edit);
        (void)authorize(session_, source, trust, edit);
        const RateRows rows(session_);
        const auto base = retained([&] { return active_revision(rows).snapshot; });
        require(core::check_rate_preview_base(base, edit.base) ==
                    core::RatePreviewBaseStatus::current,
                Refusal::stale);
        require(edit.successor != "E" && !identity_present(rows, edit), Refusal::identity_conflict);
        const auto prepared = prepare_basis(basis);
        (void)intent_document(edit, basis);
        auto proposal = propose(base, edit, prepared, basis.edges);
        (void)snapshot_document(proposal.candidate);
        return RateEditPreview{edit, basis, std::move(proposal)};
    });
}

RateSaveResult
SqliteRateOverrideUnit::save(RateAuthoritySource &source, const PermissionGraphTrust &trust,
                             const RateOverrideEdit &edit, const RateEditPreview &preview,
                             const RateSourceBasis &current,
                             const std::function<std::optional<std::int64_t>()> &sample) const {
    // Complete all fallible typed validation before the first write. Only the
    // transaction owner observes COMMIT and may release the provisional receipt.
    struct StagedWrite {
        RateReceiptSelection selection;
        WriteBatch statements;
    };
    using Plan = std::variant<StagedWrite, RateReceiptSelection, Refusal>;
    auto plan = rate_result<Plan>(invalid_input, [&]() -> Plan {
        validate_edit(edit);
        auto audit = authorize(session_, source, trust, edit);
        const RateRows rows(session_);
        if (const auto stored = operation(rows, edit))
            return retained([&] { return retry(rows, edit, preview, *stored); });

        const auto base = retained([&] { return active_revision(rows).snapshot; });
        require(core::check_rate_preview_base(base, edit.base) ==
                    core::RatePreviewBaseStatus::current,
                Refusal::stale);
        require(edit.successor != "E" && !identity_present(rows, edit), Refusal::identity_conflict);
        const auto prepared = prepare_basis(current);
        const auto previewed = prepare_basis(preview.basis);
        auto intent = intent_document(edit, current);
        require(intent == intent_document(preview.edit, preview.basis) &&
                    prepared.documents == previewed.documents,
                Refusal::preview_changed);

        WriteBatch statements;
        const auto append = [&](const QString &sql, QVariantList bindings) {
            statements.append({sql + " RETURNING 1", std::move(bindings)});
        };
        for (std::size_t i = 0; i < current.observations.size(); ++i) {
            const auto &ref = current.observations[i].context.observation_ref;
            if (const auto stored = rows.observation(ref)) {
                const auto document =
                    retained([&] { return read_observation(*stored, ref).document; });
                require(document == prepared.documents[i], Refusal::identity_conflict);
            } else {
                append("INSERT INTO main.subathon_rate_observations VALUES(?,?,?)",
                       {blob(ref), blob(observation_format), prepared.documents[i]});
            }
        }

        auto proposal = propose(base, edit, prepared, current.edges);
        auto candidate = snapshot_document(proposal.candidate);
        require(model_context(Refusal::preview_changed,
                              [&] {
                                  return matches_proposal(preview.proposal, proposal, candidate,
                                                          Refusal::preview_changed);
                              }),
                Refusal::preview_changed);

        auto selection = receipt(RateReceiptOrigin::newly_staged, edit, std::move(proposal));
        const auto &refs = selection.receipt;
        const auto body =
            encode("", occurrence_bytes, edit.override_input, override_fields, invalid_input);
        audit.accepted_at = sample();
        append("INSERT INTO main.subathon_rate_revisions VALUES(?,?,?,?)",
               {blob(refs.successor), blob(refs.base), blob(core::valuation_snapshot_format_ref),
                std::move(candidate)});
        append("INSERT INTO main.subathon_rate_decisions VALUES(?,?,?,?,?,?,?,?,?)",
               {blob(refs.decision), blob(audit.installation), blob(audit.operation),
                blob(refs.base), blob(refs.successor), blob(intent_format), std::move(intent),
                blob(decision_format),
                encode("RDC1", document_bytes, audit, audit_fields, Refusal::unavailable)});
        append("INSERT INTO main.subathon_rate_occurrences VALUES(?,?,?,?)",
               {blob(refs.introduced), blob(refs.decision), blob(refs.successor), body});
        append("UPDATE main.subathon_rate_head SET revision_ref=? WHERE slot=X'48' "
               "AND revision_ref=?",
               {blob(refs.successor), blob(refs.base)});
        return StagedWrite{std::move(selection), std::move(statements)};
    });
    if (const auto *refusal = std::get_if<Refusal>(&plan))
        return *refusal;
    if (auto *selected = std::get_if<RateReceiptSelection>(&plan))
        return std::move(*selected);

    auto &write = std::get<StagedWrite>(plan);
    for (const auto &statement : write.statements)
        write_one(session_, statement);
    return std::move(write.selection);
}

RateRecoveryResult SqliteRateOverrideUnit::recover(RateAuthoritySource &source,
                                                   const PermissionGraphTrust &trust,
                                                   const RateOverrideEdit &edit,
                                                   const RateEditPreview &preview) const {
    return rate_result<RateRecoveryResult>(invalid_input, [&]() -> RateRecoveryResult {
        validate_edit(edit);
        (void)authorize(session_, source, trust, edit);
        const RateRows rows(session_);
        return retained([&]() -> RateRecoveryResult {
            if (const auto stored = operation(rows, edit))
                return retry(rows, edit, preview, *stored);
            require(intent_document(edit, preview.basis) ==
                        intent_document(preview.edit, preview.basis),
                    Refusal::identity_conflict);
            require(edit.successor != "E" && !identity_present(rows, edit) &&
                    rows.head() != edit.successor);
            const auto prepared = prepare_basis(preview.basis);
            const auto base = verified_revision(rows, edit.base);
            const auto proposal = model_context(Refusal::identity_conflict, [&] {
                return propose(base.snapshot, edit, prepared, preview.basis.edges);
            });
            require(model_context(Refusal::identity_conflict,
                                  [&] {
                                      return matches_proposal(preview.proposal, proposal,
                                                              snapshot_document(proposal.candidate),
                                                              Refusal::identity_conflict);
                                  }),
                    Refusal::identity_conflict);

            for (std::size_t i = 0; i < prepared.facts.size(); ++i) {
                const auto &ref = prepared.facts[i].data().context.observation_ref;
                if (const auto row = rows.observation(ref)) {
                    // An exact observation may have been retained without a selected edge.
                    const auto stored = read_observation(*row, ref);
                    require(stored.document == prepared.documents[i]);
                }
            }
            return RateAttemptAbsent{};
        });
    });
}

std::optional<RateReceiptSelection> finish_rate_receipt(std::optional<RateReceiptSelection> result,
                                                        const SqliteWorkResult &work) {
    if (work.transaction == TransactionOutcome::committed && work.error == WorkError::none &&
        work.value)
        return result;
    return std::nullopt;
}
} // namespace subathon::server
