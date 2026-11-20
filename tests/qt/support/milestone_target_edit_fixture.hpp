// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "milestone_target_edit_v17_sql.hpp"
#include "sqlite_credential_catalog_consumers.hpp"
#include <limits>
#include <subathon/server/sqlite_milestone_target_edits.hpp>

namespace milestone_target_edit_fixture {
namespace cc = credential_catalog_fixture;
namespace finance = financial_refund_fixture;
using qt_test_support::require;
using namespace std::chrono_literals;
using Target = subathon::core::MilestoneTarget;
using Status = subathon::core::MilestoneEditStatus;

inline const QString v17_marker = "milestone-target-edits-v1";
inline constexpr qsizetype v16_bytes = 1'065'558;
inline constexpr qsizetype v16_final_projection_bytes = 96'456;
inline constexpr qsizetype v17_statement_bytes = 19'280;
inline constexpr qsizetype v17_new_projection_bytes = 22'552;
// The complete v16 final projection occurs once more in v17. Statement
// text appears both as ordered SQL and as the present SQL of new objects.
inline constexpr qsizetype v17_bytes =
    v16_bytes + v16_final_projection_bytes + v17_statement_bytes + v17_new_projection_bytes;
static_assert(v17_bytes == 1'203'846);

inline qsizetype projection_bytes(const MigrationStep &step) {
    qsizetype bytes = 0;
    for (const auto &o : step.expected_schema) {
        bytes += o.type.toUtf8().size() + o.name.toUtf8().size() + o.table_name.toUtf8().size();
        if (o.sql)
            bytes += o.sql->toUtf8().size();
    }
    return bytes;
}

inline qsizetype statement_bytes(const MigrationStep &step) {
    qsizetype bytes = 0;
    for (const auto &sql : step.sql)
        bytes += sql.toUtf8().size();
    return bytes;
}

inline qsizetype manifest_bytes(const MigrationManifest &manifest) {
    qsizetype bytes = 0;
    for (const auto &step : manifest.steps)
        bytes += statement_bytes(step) + projection_bytes(step);
    return bytes;
}

inline bool canonical(const CatalogObject &a, const CatalogObject &b) {
    return std::tie(a.type, a.name, a.table_name, a.sql) <
           std::tie(b.type, b.name, b.table_name, b.sql);
}

inline MigrationManifest literal_v17() {
    auto v16 = provider_credentials_repository_manifest();
    require(v16.steps.size() == 16 && v16.steps.back().id == cc::marker &&
                manifest_bytes(v16) == v16_bytes &&
                projection_bytes(v16.steps.back()) == v16_final_projection_bytes,
            "The measured protected prefix is the fixture base");

    MigrationStep step{17, v17_marker, {}, v16.steps.back().expected_schema};
    for (const auto &table : edit_tables) {
        const QString name = QString::fromLatin1(table.name);
        const QString sql = QString::fromLatin1(table.sql);
        step.sql.append(sql);
        step.expected_schema.append({"table", name, name, sql});
        for (int index = 1; index <= table.indexes; ++index)
            step.expected_schema.append({"index",
                                         "sqlite_autoindex_" + name + "_" + QString::number(index),
                                         name, std::nullopt});
    }
    step.sql.append(QString::fromLatin1(edit_pending_sql));
    step.expected_schema.append(
        {"index", edit_pending_index, "subathon_milestone_edit_occurrences", edit_pending_sql});
    std::ranges::sort(step.expected_schema, canonical);
    require(step.sql.size() == 16 && step.expected_schema.size() == 200 &&
                statement_bytes(step) == v17_statement_bytes &&
                projection_bytes(step) == v16_final_projection_bytes + v17_new_projection_bytes,
            "Independent v17 statements and complete canonical projection fit the measured frame");
    v16.steps.append(std::move(step));
    v16.supported_starting_versions.append(17);
    require(manifest_bytes(v16) == v17_bytes, "All repeated UTF-8 fields count to the exact bound");
    return v16;
}

inline MigrationResult migrate(SqliteWriter &writer, MigrationMode mode, MigrationManifest manifest,
                               qsizetype budget = v17_bytes) {
    MigrationLimits limits;
    limits.manifest_utf8_bytes = budget;
    return finance::completed<MigrationResult>([&](auto done) {
        return SqliteMigrationRunner::enqueue(writer, {mode, std::move(manifest), limits, {}},
                                              std::move(done))
            .error;
    });
}

inline void advance_v16(SqliteWriter &writer) {
    cc::expect_report(migrate(writer, MigrationMode::advance,
                              provider_credentials_repository_manifest(), v16_bytes),
                      CatalogState::current, 16, 16, 1);
}

inline void advance_literal(SqliteWriter &writer) {
    cc::expect_report(migrate(writer, MigrationMode::advance, literal_v17()), CatalogState::current,
                      17, 17, 1);
}

inline void write(const SqliteWorkSession &s, QString sql, QList<QVariant> bindings = {}) {
    (void)s.execute({std::move(sql), std::move(bindings)});
}

inline const MilestoneTrackKey track{"install", "track"};
inline const MilestoneEditActor actor{"tester", "session-lineage"};
inline const MilestoneOccurrenceKey a{"z-predecessor"};
inline const MilestoneOccurrenceKey b{"b-edit"};
inline const MilestoneOccurrenceKey c{"a-edit"};
inline const std::vector<Target> targets{{"A", 100}, {"B", 200}, {"C", 300}, {"D", 400}};
inline constexpr auto max = std::numeric_limits<std::int64_t>::max();
inline constexpr auto min = std::numeric_limits<std::int64_t>::min();

inline MilestoneEditCapture capture_b(bool with_job = false) {
    MilestoneEditCapture capture{
        "B", b, {{MilestoneActionKind::bonus, "award", "award-v1"}}, {11ms, 3ms}, {}};
    if (with_job) {
        capture.actions = {{MilestoneActionKind::automation, "announce", "announce-v1"}};
        capture.jobs = {{1,
                         {delivery_job_format, QByteArray::fromHex("0081")},
                         {"local/route", DeliverySenderSelection::explicit_identity, "edit-sender",
                          "announce", "announce-v1"}}};
    }
    return capture;
}

inline const MilestoneEditCapture empty_c{"C", c, {}, {}, {}};

inline MilestoneEditIntent ordered() {
    return {{track, "ordered"},
            {actor.actor_ref, "decision-ordered"},
            {{"D", 250}, {"C", 175}, {"B", 150}},
            {{c, MilestoneActionChoice::run}, {b, MilestoneActionChoice::defer}}};
}

inline MilestoneEditIntent single(std::string op, Target target,
                                  std::vector<MilestoneAcceptanceChoice> choices = {}) {
    return {
        {track, op}, {actor.actor_ref, "decision-" + op}, {std::move(target)}, std::move(choices)};
}

struct Authority final : MilestoneEditAuthority {
    MilestoneEditFacts facts{"cfg",
                             "captures-v1",
                             "grants-v1",
                             {"lifecycle-active",
                              finance::timer(subathon::core::TimerLifecycle::Active, 300000),
                              0,
                              {},
                              {},
                              {},
                              true,
                              true},
                             true,
                             true,
                             true,
                             "definition-edit",
                             "prefix-edit",
                             {capture_b(), empty_c}};

    std::optional<MilestoneEditFacts> select(const SqliteWorkSession &,
                                             const MilestoneEditActor &who,
                                             const MilestoneEditIntent &intent) override {
        if (who.actor_ref != actor.actor_ref || who.session_lineage != actor.session_lineage ||
            intent.evidence.actor_ref != actor.actor_ref || intent.key.track != track)
            return std::nullopt;
        auto selected = facts;
        selected.candidate_definition_revision += "-" + intent.key.operation_ref;
        selected.candidate_prefix_revision += "-" + intent.key.operation_ref;
        return selected;
    }
};

inline MilestoneTrackSeed seed(std::vector<Target> definition = targets) {
    return {track,
            "cfg",
            "definition-initial",
            "prefix-initial",
            std::move(definition),
            std::vector<MilestoneTrackSeed::Member>{
                {{"A", "earned", 100, 120, "ctx"}, finance::original, a}}};
}

// Literal initialization for scenarios whose first stop belongs to a reader or
// edit. The independent initializer scenario exercises the public bootstrap.
inline void seed_initial(const SqliteWorkSession &s, const MilestoneTrackSeed &input) {
    write(s, "INSERT INTO subathon_milestone_definition_versions VALUES(X'696e7374616c6c',"
             "X'747261636b',X'646566696e6974696f6e2d696e697469616c',X'636667',NULL)");
    qsizetype ordinal = 0;
    for (const auto &target : input.targets)
        write(
            s,
            "INSERT INTO subathon_milestone_definition_targets VALUES(X'696e7374616c6c',"
            "X'747261636b',X'646566696e6974696f6e2d696e697469616c',?,?,?)",
            {++ordinal, finance::bytes(target.milestone_ref), qint64(target.target_micro_points)});
    write(s,
          "INSERT INTO subathon_milestone_definition_heads VALUES(X'696e7374616c6c',X'747261636b',"
          "X'646566696e6974696f6e2d696e697469616c')");
    write(s,
          "INSERT INTO subathon_milestone_prefix_revisions VALUES(X'696e7374616c6c',X'747261636b',"
          "X'7072656669782d696e697469616c',1)");
    write(s, "INSERT INTO subathon_milestone_prefix_heads VALUES(X'696e7374616c6c',X'747261636b',"
             "X'7072656669782d696e697469616c')");
    write(s,
          "INSERT INTO subathon_milestone_prefix_members VALUES("
          "X'696e7374616c6c',X'747261636b',X'7072656669782d696e697469616c',1,?,?,?,?,?,0,?,?,?,"
          "NULL,NULL)",
          {QByteArray("A"), QByteArray("earned"), 100, 120, QByteArray("ctx"), QByteArray("test"),
           QByteArray("original"), finance::bytes(a.ref)});
}

class File {
  public:
    explicit File(std::int64_t extra = 55, std::int64_t tail = 7,
                  std::vector<Target> definition = targets, bool initialized = true,
                  bool historical_job = false)
        : db(credit(historical_job)) {
        db.write([&](const auto &s) {
            if (historical_job) {
                // Retain the protected real application capture as inactive
                // history. Its queued job can still be examined by recovery.
                write(s,
                      "UPDATE subathon_milestone_groups SET target_micro_points=60, "
                      "score_micro_points=120, released=1 WHERE occurrence_ref=?",
                      {finance::bytes(finance::achievement.ref)});
            }
            SqliteMilestoneGroupStore(s).stage(
                finance::original, {{{{"A", "earned", 100, 120, "ctx"}, {}, {10ms, 2ms}, a}, {}}});
            write(s, "UPDATE subathon_milestone_groups SET queue_order=? WHERE occurrence_ref=?",
                  {qint64(tail), finance::bytes(a.ref)});
            // The independent score leg already exists at genesis. The
            // original payment changes only its own score leg (0 -> 120).
            write(s,
                  "INSERT INTO subathon_app_balances SELECT token,2,?,? "
                  "FROM subathon_app_states",
                  {QByteArray("second"), qint64(extra)});
            write(s, "UPDATE subathon_app_states SET balance_count=balance_count+1");
        });
        advance_v16(*db.writer);
        advance_literal(*db.writer);
        if (initialized)
            db.write([&](const auto &s) { seed_initial(s, seed(std::move(definition))); });
    }

    static void insert_score(const SqliteWorkSession &s, const char *ref, std::int64_t units) {
        write(s, "INSERT INTO subathon_app_balances SELECT token,2,?,? FROM subathon_app_head",
              {QByteArray(ref), qint64(units)});
        write(s, "UPDATE subathon_app_states SET balance_count=balance_count+1 "
                 "WHERE token=(SELECT token FROM subathon_app_head)");
    }

    MilestoneEditPreview preview(const MilestoneEditIntent &intent) {
        std::optional<MilestoneEditPreview> result;
        db.write([&](const auto &s) {
            result = preview_milestone_target_edit(s, authority, actor, intent);
        });
        return std::move(result.value());
    }

    static void confirmable(const MilestoneEditPreview &preview) {
        require(preview.plan.status == Status::eligible && preview.confirmation,
                "Eligible choice-complete preview has an opaque confirmation binding");
    }

    MilestoneEditReply confirm(const MilestoneEditIntent &intent, const MilestoneEditPreview &p) {
        confirmable(p);
        std::optional<MilestoneEditReply> result;
        db.write([&](const auto &s) {
            result = confirm_milestone_target_edit(s, authority, actor, intent, *p.confirmation);
        });
        return std::move(result.value());
    }

    void reopen() {
        auto stopped = std::make_shared<bool>(false);
        require(db.writer->shutdown([stopped] { *stopped = true; }) == StorageError::none &&
                    QTest::qWaitFor([stopped] { return *stopped; }, 10000),
                "Drain the writer before opening the same committed file");
        db.observer.reset();
        db.writer.reset();
        db.open();
    }

    finance::File db;
    Authority authority;

  private:
    static finance::OriginalCredit credit(bool historical_job) {
        finance::OriginalCredit result;
        result.amount = "12.0000";
        result.score = 120;
        result.count = 700;
        result.milestone = historical_job;
        return result;
    }
};

} // namespace milestone_target_edit_fixture
