// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/sqlite_modifier_activations.hpp>

#include <QMetaType>
#include <QStringList>
#include <algorithm>
#include <limits>
#include <subathon/server/milestone_group_decisions.hpp>
#include <tuple>

namespace subathon::server {
namespace {

using Reason = SqliteModifierStorageReason;
using Lifetime = ModifierInstanceLifetime;
using Operation = ModifierInstanceOperation;
using Timing = core::ModifierTriggerTiming;
using Policy = core::ModifierReactivation;
constexpr auto factor_limit = core::maximum_decimal_bytes;
constexpr int target_limit = 1024;
constexpr int ref_limit = 4096;

[[noreturn]] void invalid() {
    throw SqliteModifierInputFailure(SqliteModifierInputError::invalid_input);
}

[[noreturn]] void refuse(Reason reason) { throw SqliteModifierStorageFailure(reason); }

[[noreturn]] void corrupt() { refuse(Reason::invalid_stored_data); }

void cleanup(const SqliteWorkSession &session) {
    if (const auto error = session.cleanup(); error != WorkError::none)
        throw SqliteWorkFailure(error);
}

// Prewrite input/freshness/identity refusals leave preceding staging usable.
template <class Work> auto checked(const SqliteWorkSession &session, Work work) {
    try {
        return work();
    } catch (const SqliteModifierStorageFailure &error) {
        if (error.reason() == Reason::invalid_stored_data ||
            error.reason() == Reason::sequence_exhausted)
            cleanup(session);
        throw;
    } catch (const core::ModifierActivationFailure &error) {
        if (error.code() == core::ModifierActivationError::duration_overflow)
            cleanup(session);
        throw;
    }
}

// Once staging begins, even a local exception must discard the whole unit.
template <class Work> void stage(const SqliteWorkSession &session, Work work) {
    try {
        work();
    } catch (const SqliteWorkFailure &) {
        throw;
    } catch (...) {
        cleanup(session);
        throw;
    }
}

bool bounded_ref(std::string_view ref) { return !ref.empty() && ref.size() <= ref_limit; }

void check_ref(std::string_view ref) {
    if (!bounded_ref(ref))
        invalid();
}

void check_key(const ModifierInstanceKey &key) {
    check_ref(key.instance_ref);
    check_ref(key.revision_ref);
}

template <class Enum> bool in_range(Enum value, Enum maximum) {
    return int(value) >= 0 && int(value) <= int(maximum);
}

bool valid_factor(std::string_view spelling) {
    if (spelling.size() > factor_limit)
        return false;
    try {
        (void)core::parse_nonnegative_factor(spelling);
        return true;
    } catch (const std::invalid_argument &) {
        return false;
    } catch (const std::length_error &) {
        return false;
    }
}

void check_definition(const core::ModifierDefinition &definition,
                      const std::optional<std::string> &expected) {
    check_ref(definition.definition_ref);
    check_ref(definition.revision_ref);
    if (expected)
        check_ref(*expected);
    if (definition.duration_ms <= 0 || definition.targets.size() > target_limit ||
        !in_range(definition.reactivation, Policy::ignore))
        invalid();
    for (const auto &target : definition.targets) {
        check_ref(target.target_ref);
        if (target.qualifier_ref)
            check_ref(*target.qualifier_ref);
        if (!in_range(target.kind, core::ModifierTargetKind::counter))
            invalid();
    }
    for (const auto &target : definition.targets) {
        if (!valid_factor(target.factor_decimal))
            throw core::ModifierActivationFailure(core::ModifierActivationError::invalid_factor);
    }
}

void check_activation(const std::optional<ModifierInstanceKey> &expected,
                      const core::ModifierActivation &activation) {
    if (expected)
        check_key(*expected);
    check_ref(activation.new_instance_ref);
    check_ref(activation.new_revision_ref);
    check_ref(activation.source.source_ref);
    if (!in_range(activation.source.kind, core::ModifierActivationKind::schedule))
        invalid();
    if (activation.trigger) {
        check_ref(activation.trigger->event_ref);
        if (!in_range(activation.trigger->timing, Timing::subsequent_only))
            invalid();
        if (activation.trigger->timing == Timing::unspecified)
            throw core::ModifierActivationFailure(
                core::ModifierActivationError::missing_trigger_timing);
    }
}

QByteArray blob(std::string_view value) {
    return QByteArray(value.data(), qsizetype(value.size()));
}

QVariant nullable_ref(const std::optional<std::string> &ref) {
    return ref ? QVariant(blob(*ref)) : QVariant{};
}

// Filter sizes and types before SQL values cross the writer boundary. An
// invalid non-NULL blob becomes an integer so it cannot masquerade as absence.
QString blob_column(const QString &name, int maximum = ref_limit) {
    return "CASE WHEN " + name + " IS NULL THEN NULL WHEN typeof(" + name + ")='blob' AND length(" +
           name + ") BETWEEN 1 AND " + QString::number(maximum) + " THEN " + name + " ELSE 0 END";
}

QString int_column(const QString &name) {
    return "CASE WHEN typeof(" + name + ")='integer' THEN " + name + " END";
}

std::string bytes(const QVariant &cell) {
    if (cell.isNull() || cell.metaType().id() != QMetaType::QByteArray)
        corrupt();
    return cell.toByteArray().toStdString();
}

std::string ref(const QVariant &cell) {
    auto value = bytes(cell);
    if (!bounded_ref(value))
        corrupt();
    return value;
}

std::optional<std::string> optional_ref(const QVariant &cell) {
    if (cell.isNull())
        return std::nullopt;
    return ref(cell);
}

qint64 integer(const QVariant &cell) {
    const auto type = cell.metaType().id();
    if (cell.isNull() || (type != QMetaType::Int && type != QMetaType::LongLong))
        corrupt();
    return cell.toLongLong();
}

qint64 positive(const QVariant &cell) {
    const auto value = integer(cell);
    if (value <= 0)
        corrupt();
    return value;
}

template <class Enum> Enum code(const QVariant &cell, Enum maximum) {
    const auto value = integer(cell);
    if (value < 0 || value > int(maximum))
        corrupt();
    return Enum(value);
}

std::int64_t next_position(std::int64_t position) {
    if (position == std::numeric_limits<std::int64_t>::max())
        refuse(Reason::sequence_exhausted);
    return position + 1;
}

struct DefinitionRow {
    SqliteModifierDefinitionRevision value;
    qint64 target_count;
};

struct Lineage {
    std::string instance_ref;
    std::string definition_ref;
    Lifetime lifetime;
    std::optional<std::string> successor;
};

struct RevisionRow {
    std::string revision_ref;
    std::string instance_ref;
    qint64 position;
    std::optional<std::string> parent;
    std::string capture;
    std::optional<std::string> applied;
    qint64 remaining;
    Operation operation;
    core::ModifierActivationSource source;
    std::optional<core::ModifierTriggeringContribution> trigger;
};

class ModifierRows {
  public:
    explicit ModifierRows(const SqliteWorkSession &session) : session_(session) {
        const auto marker =
            one("SELECT CASE WHEN typeof(version)='integer' AND "
                "((version=14 AND migration_id COLLATE BINARY='modifier-activations-v1') OR "
                "(version=15 AND migration_id COLLATE BINARY='financial-refunds-v1') OR "
                "(version=16 AND migration_id COLLATE BINARY='provider-credentials-v1') OR "
                "(version=17 AND migration_id COLLATE BINARY='rate-overrides-v1') OR "
                "(version=18 AND migration_id COLLATE BINARY='milestone-target-edits-v1') OR "
                "(version=19 AND migration_id COLLATE BINARY='contribution-review-v1') OR "
                "(version=20 AND migration_id COLLATE BINARY='connector-intake-storage-v1')) "
                "AND EXISTS(SELECT 1 FROM main.subathon_catalog WHERE singleton=1 "
                "AND typeof(catalog_format)='integer' AND catalog_format=1 "
                "AND store_identity='subathon.sqlite-store') THEN 1 ELSE 0 END "
                "FROM main.subathon_migration_history ORDER BY version DESC LIMIT 1",
                {});
        if (!marker || integer(marker->front()) != 1)
            corrupt();

        // Resolve the entire required schema even when the requested ref is absent.
        (void)session_.execute(
            {"SELECT d.revision_ref,d.definition_ref,d.position,d.duration_ms,d.reactivation,"
             "d.target_count,t.definition_revision_ref,t.ordinal,t.kind,t.target_ref,"
             "t.qualifier_ref,t.factor_decimal,i.instance_ref,i.definition_ref,i.lifetime,"
             "i.successor_instance_ref,r.revision_ref,r.instance_ref,r.position,"
             "r.parent_revision_ref,r.captured_definition_revision_ref,"
             "r.applied_definition_revision_ref,r.remaining_ms,r.operation,r.source_kind,"
             "r.source_ref,r.trigger_ref,r.trigger_timing "
             "FROM main.subathon_modifier_instances AS i INDEXED BY subathon_modifier_one_live "
             "CROSS JOIN main.subathon_modifier_definition_revisions AS d "
             "CROSS JOIN main.subathon_modifier_targets AS t "
             "CROSS JOIN main.subathon_modifier_instance_revisions AS r "
             "WHERE i.lifetime=0 LIMIT 0",
             {}});
    }

    std::optional<SqliteModifierDefinitionRevision>
    current_definition(const std::string &key) const {
        auto row = read_definition("d.definition_ref=? ORDER BY d.position DESC LIMIT 1", key);
        if (!row)
            return std::nullopt;
        if (row->value.definition.definition_ref != key)
            corrupt();
        return with_targets(std::move(*row));
    }

    std::optional<SqliteModifierDefinitionRevision>
    definition_revision(const std::string &key) const {
        auto row = exact_definition(key);
        if (!row)
            return std::nullopt;
        return with_targets(std::move(*row));
    }

    std::optional<SqliteModifierInstanceRevision> live_instance(const std::string &key) const {
        const auto owner = read_lineage("definition_ref=? AND lifetime=0 LIMIT 2", key);
        if (!owner)
            return std::nullopt;
        if (owner->definition_ref != key || owner->lifetime != Lifetime::live)
            corrupt();
        return materialize(*owner, std::nullopt);
    }

    std::optional<SqliteModifierInstanceRevision> instance_head(const std::string &key) const {
        const auto owner = exact_lineage(key);
        if (!owner)
            return std::nullopt;
        if (owner->lifetime == Lifetime::live) {
            auto live = live_instance(owner->definition_ref);
            if (!live || live->instance.instance_ref != key)
                corrupt();
            return live;
        }
        return materialize(*owner, std::nullopt);
    }

    std::optional<SqliteModifierInstanceRevision> instance_revision(const std::string &key) const {
        auto selected = exact_revision(key);
        if (!selected)
            return std::nullopt;
        const auto owner = exact_lineage(selected->instance_ref);
        if (!owner)
            corrupt();
        return materialize(*owner, std::move(selected));
    }

    void require_fresh(const QString &table, const QString &column, const std::string &key) const {
        if (one("SELECT 1 FROM main." + table + " WHERE " + column + "=? LIMIT 1", {blob(key)}))
            refuse(Reason::identity_conflict);
    }

  private:
    std::optional<SqlRow> one(const QString &sql, QList<QVariant> bindings) const {
        auto rows = session_.execute({sql, std::move(bindings)});
        if (rows.empty())
            return std::nullopt;
        if (rows.size() != 1)
            corrupt();
        return std::move(rows.front());
    }

    std::optional<DefinitionRow> read_definition(const QString &where,
                                                 const std::string &key) const {
        const auto columns =
            QStringList{blob_column("d.revision_ref"),
                        blob_column("d.definition_ref"),
                        int_column("d.position"),
                        int_column("d.duration_ms"),
                        int_column("d.reactivation"),
                        int_column("d.target_count"),
                        "CASE WHEN d.position=1 OR EXISTS("
                        "SELECT 1 FROM main.subathon_modifier_definition_revisions AS p "
                        "WHERE p.definition_ref=d.definition_ref AND p.position=d.position-1 "
                        "AND typeof(p.position)='integer' AND typeof(p.revision_ref)='blob' "
                        "AND length(p.revision_ref) BETWEEN 1 AND 4096) THEN 1 ELSE 0 END"};
        const auto row =
            one("SELECT " + columns.join(',') +
                    " FROM main.subathon_modifier_definition_revisions AS d WHERE " + where,
                {blob(key)});
        if (!row)
            return std::nullopt;
        const auto &c = *row;
        const auto count = integer(c[5]);
        if (count < 0 || count > target_limit || integer(c[6]) != 1)
            corrupt();
        return DefinitionRow{
            {{ref(c[1]), ref(c[0]), positive(c[3]), {}, code(c[4], Policy::ignore)},
             positive(c[2])},
            count};
    }

    std::optional<DefinitionRow> exact_definition(const std::string &key) const {
        auto row = read_definition("d.revision_ref=? LIMIT 2", key);
        if (row && row->value.definition.revision_ref != key)
            corrupt();
        return row;
    }

    SqliteModifierDefinitionRevision with_targets(DefinitionRow row) const {
        auto &definition = row.value.definition;
        const auto columns = QStringList{blob_column("definition_revision_ref"),
                                         int_column("ordinal"),
                                         int_column("kind"),
                                         blob_column("target_ref"),
                                         blob_column("qualifier_ref"),
                                         blob_column("factor_decimal", int(factor_limit))};
        const auto rows = session_.execute(
            {"SELECT " + columns.join(',') +
                 " FROM main.subathon_modifier_targets WHERE definition_revision_ref=? "
                 "ORDER BY ordinal LIMIT ?",
             {blob(definition.revision_ref), row.target_count + 1}});
        if (rows.size() != row.target_count)
            corrupt();
        definition.targets.reserve(std::size_t(row.target_count));
        qint64 expected = 1;
        for (const auto &c : rows) {
            if (ref(c[0]) != definition.revision_ref || integer(c[1]) != expected++)
                corrupt();
            auto factor = bytes(c[5]);
            if (!valid_factor(factor))
                corrupt();
            definition.targets.push_back({code(c[2], core::ModifierTargetKind::counter), ref(c[3]),
                                          optional_ref(c[4]), std::move(factor)});
        }
        return std::move(row.value);
    }

    std::optional<Lineage> read_lineage(const QString &where, const std::string &key) const {
        const auto columns =
            QStringList{blob_column("instance_ref"), blob_column("definition_ref"),
                        int_column("lifetime"), blob_column("successor_instance_ref")};
        const auto row = one("SELECT " + columns.join(',') +
                                 " FROM main.subathon_modifier_instances WHERE " + where,
                             {blob(key)});
        if (!row)
            return std::nullopt;
        const auto &c = *row;
        Lineage owner{ref(c[0]), ref(c[1]), code(c[2], Lifetime::expired), optional_ref(c[3])};
        if ((owner.lifetime == Lifetime::replaced) != owner.successor.has_value() ||
            owner.successor == owner.instance_ref)
            corrupt();
        return owner;
    }

    std::optional<Lineage> exact_lineage(const std::string &key) const {
        auto owner = read_lineage("instance_ref=? LIMIT 2", key);
        if (owner && owner->instance_ref != key)
            corrupt();
        return owner;
    }

    std::optional<RevisionRow> read_revision(const QString &where, const std::string &key) const {
        const auto columns =
            QStringList{blob_column("revision_ref"),
                        blob_column("instance_ref"),
                        int_column("position"),
                        blob_column("parent_revision_ref"),
                        blob_column("captured_definition_revision_ref"),
                        blob_column("applied_definition_revision_ref"),
                        int_column("remaining_ms"),
                        int_column("operation"),
                        int_column("source_kind"),
                        blob_column("source_ref"),
                        blob_column("trigger_ref"),
                        "CASE WHEN typeof(trigger_timing)='integer' THEN trigger_timing "
                        "WHEN trigger_timing IS NOT NULL THEN -1 END"};
        const auto row = one("SELECT " + columns.join(',') +
                                 " FROM main.subathon_modifier_instance_revisions WHERE " + where,
                             {blob(key)});
        if (!row)
            return std::nullopt;
        const auto &c = *row;
        RevisionRow revision{ref(c[0]),
                             ref(c[1]),
                             positive(c[2]),
                             optional_ref(c[3]),
                             ref(c[4]),
                             optional_ref(c[5]),
                             integer(c[6]),
                             code(c[7], Operation::checkpoint),
                             {code(c[8], core::ModifierActivationKind::schedule), ref(c[9])},
                             {}};
        if (const auto trigger = optional_ref(c[10])) {
            const auto timing = code(c[11], Timing::subsequent_only);
            if (timing == Timing::unspecified)
                corrupt();
            revision.trigger = core::ModifierTriggeringContribution{*trigger, timing};
        } else if (!c[11].isNull()) {
            corrupt();
        }

        if (revision.parent.has_value() != (revision.position > 1) ||
            (revision.position == 1 && revision.operation != Operation::activation) ||
            (revision.operation == Operation::activation) != revision.applied.has_value() ||
            revision.remaining < 0 ||
            (revision.operation == Operation::activation && revision.remaining == 0))
            corrupt();
        return revision;
    }

    std::optional<RevisionRow> exact_revision(const std::string &key) const {
        auto row = read_revision("revision_ref=? LIMIT 2", key);
        if (row && row->revision_ref != key)
            corrupt();
        return row;
    }

    DefinitionRow definition_for(const std::string &key, const Lineage &owner) const {
        auto definition = exact_definition(key);
        if (!definition || definition->value.definition.definition_ref != owner.definition_ref)
            corrupt();
        return std::move(*definition);
    }

    // Validate the selected edge and its endpoints without traversing history.
    DefinitionRow validate_revision(const RevisionRow &revision, const Lineage &owner) const {
        if (revision.instance_ref != owner.instance_ref)
            corrupt();
        if (revision.parent) {
            const auto parent = exact_revision(*revision.parent);
            if (!parent || parent->instance_ref != owner.instance_ref ||
                parent->position != revision.position - 1 || parent->capture != revision.capture ||
                parent->remaining == 0)
                corrupt();
            if (revision.operation == Operation::checkpoint &&
                (revision.remaining > parent->remaining || revision.source != parent->source ||
                 revision.trigger != parent->trigger))
                corrupt();
        } else if (revision.applied != revision.capture) {
            corrupt();
        }

        auto capture = definition_for(revision.capture, owner);
        if (revision.applied && *revision.applied != revision.capture)
            (void)with_targets(definition_for(*revision.applied, owner));
        return capture;
    }

    SqliteModifierInstanceRevision materialize(const Lineage &owner,
                                               std::optional<RevisionRow> selected) const {
        if (owner.successor) {
            const auto successor = exact_lineage(*owner.successor);
            if (!successor || successor->definition_ref != owner.definition_ref)
                corrupt();
            const auto initial =
                read_revision("instance_ref=? AND position=1 LIMIT 2", successor->instance_ref);
            if (!initial)
                corrupt();
            (void)with_targets(validate_revision(*initial, *successor));
        }

        auto head =
            read_revision("instance_ref=? ORDER BY position DESC LIMIT 1", owner.instance_ref);
        if (!head || head->instance_ref != owner.instance_ref ||
            (head->remaining == 0) != (owner.lifetime == Lifetime::expired))
            corrupt();

        auto capture = validate_revision(*head, owner);
        if (selected && selected->revision_ref != head->revision_ref) {
            if (selected->position >= head->position || selected->remaining == 0 ||
                selected->capture != head->capture)
                corrupt();
            capture = validate_revision(*selected, owner);
        } else {
            selected = std::move(head);
        }

        return {{std::move(selected->instance_ref), std::move(selected->revision_ref),
                 std::move(selected->parent), with_targets(std::move(capture)).definition,
                 selected->remaining, std::move(selected->source), std::move(selected->trigger)},
                selected->position,
                selected->operation,
                std::move(selected->applied),
                owner.lifetime,
                owner.successor};
    }

    const SqliteWorkSession &session_;
};

bool matches(const SqliteModifierInstanceRevision &revision, const ModifierInstanceKey &key) {
    return revision.instance.instance_ref == key.instance_ref &&
           revision.instance.revision_ref == key.revision_ref;
}

void insert_revision(const SqliteWorkSession &session, const core::ModifierInstance &instance,
                     std::int64_t position, Operation operation,
                     const std::optional<std::string> &applied) {
    const auto &trigger = instance.trigger;
    (void)session.execute(
        {"INSERT INTO main.subathon_modifier_instance_revisions "
         "(revision_ref,instance_ref,position,parent_revision_ref,captured_definition_revision_ref,"
         "applied_definition_revision_ref,remaining_ms,operation,source_kind,source_ref,"
         "trigger_ref,trigger_timing) VALUES(?,?,?,?,?,?,?,?,?,?,?,?)",
         {blob(instance.revision_ref), blob(instance.instance_ref), qint64(position),
          nullable_ref(instance.parent_revision_ref),
          blob(instance.captured_definition.revision_ref), nullable_ref(applied),
          qint64(instance.remaining_ms), int(operation), int(instance.source.kind),
          blob(instance.source.source_ref),
          trigger ? QVariant(blob(trigger->event_ref)) : QVariant{},
          trigger ? QVariant(int(trigger->timing)) : QVariant{}}});
}

} // namespace

MigrationManifest modifier_activations_repository_manifest() {
    auto manifest = milestone_group_decisions_repository_manifest();
    MigrationStep step{14, "modifier-activations-v1", {}, manifest.steps.back().expected_schema};
    step.sql = {
        // These five reviewed byte strings are part of the catalog digest.
        R"sql(CREATE TABLE subathon_modifier_definition_revisions(revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND length(revision_ref) BETWEEN 1 AND 4096), definition_ref BLOB NOT NULL CHECK(typeof(definition_ref)='blob' AND length(definition_ref) BETWEEN 1 AND 4096), position INTEGER NOT NULL CHECK(typeof(position)='integer' AND position>0), duration_ms INTEGER NOT NULL CHECK(typeof(duration_ms)='integer' AND duration_ms>0), reactivation INTEGER NOT NULL CHECK(typeof(reactivation)='integer' AND reactivation BETWEEN 0 AND 3), target_count INTEGER NOT NULL CHECK(typeof(target_count)='integer' AND target_count BETWEEN 0 AND 1024), PRIMARY KEY(revision_ref), UNIQUE(definition_ref,position)))sql",
        R"sql(CREATE TABLE subathon_modifier_targets(definition_revision_ref BLOB NOT NULL, ordinal INTEGER NOT NULL CHECK(typeof(ordinal)='integer' AND ordinal BETWEEN 1 AND 1024), kind INTEGER NOT NULL CHECK(typeof(kind)='integer' AND kind BETWEEN 0 AND 2), target_ref BLOB NOT NULL CHECK(typeof(target_ref)='blob' AND length(target_ref) BETWEEN 1 AND 4096), qualifier_ref BLOB CHECK(qualifier_ref IS NULL OR (typeof(qualifier_ref)='blob' AND length(qualifier_ref) BETWEEN 1 AND 4096)), factor_decimal BLOB NOT NULL CHECK(typeof(factor_decimal)='blob' AND length(factor_decimal) BETWEEN 1 AND 512), PRIMARY KEY(definition_revision_ref,ordinal), FOREIGN KEY(definition_revision_ref) REFERENCES subathon_modifier_definition_revisions(revision_ref)))sql",
        R"sql(CREATE TABLE subathon_modifier_instances(instance_ref BLOB NOT NULL CHECK(typeof(instance_ref)='blob' AND length(instance_ref) BETWEEN 1 AND 4096), definition_ref BLOB NOT NULL CHECK(typeof(definition_ref)='blob' AND length(definition_ref) BETWEEN 1 AND 4096), lifetime INTEGER NOT NULL CHECK(typeof(lifetime)='integer' AND lifetime BETWEEN 0 AND 2), successor_instance_ref BLOB, CHECK((lifetime=1 AND typeof(successor_instance_ref)='blob' AND length(successor_instance_ref) BETWEEN 1 AND 4096 AND successor_instance_ref<>instance_ref) OR (lifetime IN (0,2) AND successor_instance_ref IS NULL)), PRIMARY KEY(instance_ref), FOREIGN KEY(successor_instance_ref) REFERENCES subathon_modifier_instances(instance_ref) DEFERRABLE INITIALLY DEFERRED))sql",
        R"sql(CREATE TABLE subathon_modifier_instance_revisions(revision_ref BLOB NOT NULL CHECK(typeof(revision_ref)='blob' AND length(revision_ref) BETWEEN 1 AND 4096), instance_ref BLOB NOT NULL, position INTEGER NOT NULL CHECK(typeof(position)='integer' AND position>0), parent_revision_ref BLOB, captured_definition_revision_ref BLOB NOT NULL, applied_definition_revision_ref BLOB, remaining_ms INTEGER NOT NULL CHECK(typeof(remaining_ms)='integer' AND remaining_ms>=0), operation INTEGER NOT NULL CHECK(typeof(operation)='integer' AND operation BETWEEN 0 AND 1), source_kind INTEGER NOT NULL CHECK(typeof(source_kind)='integer' AND source_kind BETWEEN 0 AND 4), source_ref BLOB NOT NULL CHECK(typeof(source_ref)='blob' AND length(source_ref) BETWEEN 1 AND 4096), trigger_ref BLOB, trigger_timing INTEGER, CHECK((position=1 AND parent_revision_ref IS NULL AND operation=0) OR (position>1 AND typeof(parent_revision_ref)='blob')), CHECK((operation=0 AND typeof(applied_definition_revision_ref)='blob' AND remaining_ms>0) OR (operation=1 AND applied_definition_revision_ref IS NULL)), CHECK((trigger_ref IS NULL AND trigger_timing IS NULL) OR (typeof(trigger_ref)='blob' AND length(trigger_ref) BETWEEN 1 AND 4096 AND typeof(trigger_timing)='integer' AND trigger_timing IN (1,2))), PRIMARY KEY(revision_ref), UNIQUE(instance_ref,position), FOREIGN KEY(instance_ref) REFERENCES subathon_modifier_instances(instance_ref), FOREIGN KEY(parent_revision_ref) REFERENCES subathon_modifier_instance_revisions(revision_ref), FOREIGN KEY(captured_definition_revision_ref) REFERENCES subathon_modifier_definition_revisions(revision_ref), FOREIGN KEY(applied_definition_revision_ref) REFERENCES subathon_modifier_definition_revisions(revision_ref)))sql",
        R"sql(CREATE UNIQUE INDEX subathon_modifier_one_live ON subathon_modifier_instances(definition_ref) WHERE lifetime=0)sql",
    };
    const QStringList tables{"subathon_modifier_definition_revisions", "subathon_modifier_targets",
                             "subathon_modifier_instances", "subathon_modifier_instance_revisions"};
    for (qsizetype i = 0; i < tables.size(); ++i) {
        const auto &table = tables[i];
        const auto implicit = "sqlite_autoindex_" + table;
        step.expected_schema.append({"table", table, table, step.sql[i]});
        step.expected_schema.append({"index", implicit + "_1", table, {}});
        if (i == 0 || i == 3)
            step.expected_schema.append({"index", implicit + "_2", table, {}});
    }
    step.expected_schema.append({"index", "subathon_modifier_one_live", tables[2], step.sql[4]});
    std::ranges::sort(step.expected_schema, [](const auto &left, const auto &right) {
        return std::tie(left.type, left.name) < std::tie(right.type, right.name);
    });
    manifest.steps.append(std::move(step));
    manifest.supported_starting_versions.append(14);
    return manifest;
}

SqliteModifierDefinitionRevision
SqliteModifierActivationStore::append_definition(const core::ModifierDefinition &definition,
                                                 const std::optional<std::string> &expected) const {
    check_definition(definition, expected);
    return checked(session_, [&] {
        const ModifierRows rows(session_);
        const auto head = rows.current_definition(definition.definition_ref);
        if (head ? !expected || head->definition.revision_ref != *expected : expected.has_value())
            refuse(Reason::stale_definition);
        rows.require_fresh("subathon_modifier_definition_revisions", "revision_ref",
                           definition.revision_ref);
        SqliteModifierDefinitionRevision result{definition,
                                                next_position(head ? head->position : 0)};
        stage(session_, [&] {
            const auto revision = blob(definition.revision_ref);
            (void)session_.execute(
                {"INSERT INTO main.subathon_modifier_definition_revisions "
                 "(revision_ref,definition_ref,position,duration_ms,reactivation,target_count) "
                 "VALUES(?,?,?,?,?,?)",
                 {revision, blob(definition.definition_ref), qint64(result.position),
                  qint64(definition.duration_ms), int(definition.reactivation),
                  int(definition.targets.size())}});
            int ordinal = 1;
            for (const auto &target : definition.targets) {
                (void)session_.execute(
                    {"INSERT INTO main.subathon_modifier_targets "
                     "(definition_revision_ref,ordinal,kind,target_ref,qualifier_ref,factor_"
                     "decimal) "
                     "VALUES(?,?,?,?,?,?)",
                     {revision, ordinal++, int(target.kind), blob(target.target_ref),
                      nullable_ref(target.qualifier_ref), blob(target.factor_decimal)}});
            }
        });
        return result;
    });
}

core::ModifierActivationProposal
SqliteModifierActivationStore::activate(const std::string &definition_ref,
                                        const std::string &expected_definition_revision,
                                        const std::optional<ModifierInstanceKey> &expected_live,
                                        const core::ModifierActivation &activation) const {
    check_ref(definition_ref);
    check_ref(expected_definition_revision);
    check_activation(expected_live, activation);
    return checked(session_, [&] {
        const ModifierRows rows(session_);
        const auto head = rows.current_definition(definition_ref);
        if (!head || head->definition.revision_ref != expected_definition_revision)
            refuse(Reason::stale_definition);
        const auto live = rows.live_instance(definition_ref);
        if (live ? !expected_live || !matches(*live, *expected_live) : expected_live.has_value())
            refuse(Reason::stale_instance);

        auto proposal = core::propose_modifier_activation(
            head->definition, live ? std::optional{live->instance} : std::nullopt, activation);
        if (!proposal.revision_to_append)
            return proposal;

        const auto &revision = *proposal.revision_to_append;
        const bool new_instance = !revision.parent_revision_ref;
        if (new_instance)
            rows.require_fresh("subathon_modifier_instances", "instance_ref",
                               revision.instance_ref);
        rows.require_fresh("subathon_modifier_instance_revisions", "revision_ref",
                           revision.revision_ref);
        const auto position = new_instance ? 1 : next_position(live->position);

        stage(session_, [&] {
            if (proposal.superseded_instance_ref) {
                (void)session_.execute(
                    {"UPDATE main.subathon_modifier_instances SET "
                     "lifetime=1,successor_instance_ref=? "
                     "WHERE instance_ref=?",
                     {blob(revision.instance_ref), blob(*proposal.superseded_instance_ref)}});
            }
            if (new_instance) {
                (void)session_.execute({"INSERT INTO main.subathon_modifier_instances "
                                        "(instance_ref,definition_ref,lifetime,successor_instance_"
                                        "ref) VALUES(?,?,0,NULL)",
                                        {blob(revision.instance_ref), blob(definition_ref)}});
            }
            insert_revision(session_, revision, position, Operation::activation,
                            expected_definition_revision);
        });
        return proposal;
    });
}

SqliteModifierInstanceRevision
SqliteModifierActivationStore::checkpoint_remaining(const ModifierInstanceKey &expected_live,
                                                    const std::string &new_revision,
                                                    std::int64_t observed_remaining_ms) const {
    check_key(expected_live);
    check_ref(new_revision);
    if (observed_remaining_ms < 0)
        invalid();
    return checked(session_, [&] {
        const ModifierRows rows(session_);
        auto head = rows.instance_head(expected_live.instance_ref);
        if (!head || head->lifetime != Lifetime::live || !matches(*head, expected_live))
            refuse(Reason::stale_instance);
        if (observed_remaining_ms > head->instance.remaining_ms)
            invalid();
        rows.require_fresh("subathon_modifier_instance_revisions", "revision_ref", new_revision);

        const auto position = next_position(head->position);
        auto revision = std::move(head->instance);
        revision.parent_revision_ref = std::move(revision.revision_ref);
        revision.revision_ref = new_revision;
        revision.remaining_ms = observed_remaining_ms;
        const bool expired = observed_remaining_ms == 0;
        stage(session_, [&] {
            insert_revision(session_, revision, position, Operation::checkpoint, {});
            if (expired)
                (void)session_.execute(
                    {"UPDATE main.subathon_modifier_instances SET lifetime=2 WHERE instance_ref=?",
                     {blob(revision.instance_ref)}});
        });
        return SqliteModifierInstanceRevision{std::move(revision),
                                              position,
                                              Operation::checkpoint,
                                              {},
                                              expired ? Lifetime::expired : Lifetime::live,
                                              {}};
    });
}

std::optional<SqliteModifierDefinitionRevision>
SqliteModifierActivationStore::current_definition(const std::string &key) const {
    check_ref(key);
    return checked(session_, [&] { return ModifierRows(session_).current_definition(key); });
}

std::optional<SqliteModifierDefinitionRevision>
SqliteModifierActivationStore::definition_revision(const std::string &key) const {
    check_ref(key);
    return checked(session_, [&] { return ModifierRows(session_).definition_revision(key); });
}

std::optional<SqliteModifierInstanceRevision>
SqliteModifierActivationStore::live_instance(const std::string &key) const {
    check_ref(key);
    return checked(session_, [&] { return ModifierRows(session_).live_instance(key); });
}

std::optional<SqliteModifierInstanceRevision>
SqliteModifierActivationStore::instance_revision(const std::string &key) const {
    check_ref(key);
    return checked(session_, [&] { return ModifierRows(session_).instance_revision(key); });
}

} // namespace subathon::server
