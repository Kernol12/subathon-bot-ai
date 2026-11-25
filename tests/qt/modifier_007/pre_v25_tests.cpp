// SPDX-License-Identifier: GPL-3.0-or-later
#include "../support/sqlite_modifier_activations_fixture.hpp"

#include <QTest>

namespace {
using namespace modifier_activation_fixture;

template <class Scenario> void check(Scenario scenario) {
    try {
        scenario();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

void seed(ModifierFile &file) {
    (void)committed(*file.writer, [](const auto &session) {
        const SqliteModifierActivationStore store(session);
        require(store.append_definition(old_definition, {}) ==
                    SqliteModifierDefinitionRevision{old_definition, 1},
                "Legacy fixture appends D/z-old");
        require(store.append_definition(independent_definition, {}) ==
                    SqliteModifierDefinitionRevision{independent_definition, 1},
                "Legacy fixture appends E/e0");
        return true;
    });
}

auto rows(const SqliteWorkSession &session) {
    return std::pair{
        session.execute({"SELECT revision_ref,definition_ref,position,duration_ms,reactivation,"
                         "target_count FROM main.subathon_modifier_definition_revisions "
                         "ORDER BY revision_ref",
                         {}}),
        session.execute({"SELECT definition_revision_ref,ordinal,kind,target_ref,qualifier_ref,"
                         "factor_decimal FROM main.subathon_modifier_targets "
                         "ORDER BY definition_revision_ref,ordinal",
                         {}})};
}

class PreV25ModifierNames final : public QObject {
    Q_OBJECT

  private slots:
    void readback_data() {
        QTest::addColumn<QString>("definition");
        QTest::addColumn<QString>("revision");
        QTest::addColumn<bool>("exists");
        QTest::newRow("exact-old") << QString("D") << QString("z-old") << true;
        QTest::newRow("wrong-owner") << QString("E") << QString("z-old") << false;
        QTest::newRow("missing-revision") << QString("D") << QString("missing") << false;
    }

    void readback() {
        QFETCH(QString, definition);
        QFETCH(QString, revision);
        QFETCH(bool, exists);
        check([&] {
            ModifierFile file;
            seed(file);
            (void)committed(*file.writer, [](const auto &session) {
                require(SqliteModifierActivationStore(session).append_definition(new_definition,
                                                                                 "z-old") ==
                            SqliteModifierDefinitionRevision{new_definition, 2},
                        "Legacy fixture advances D to a-new");
                return true;
            });
            reopen(file);
            const auto actual = committed(*file.writer, [&](const auto &session) {
                return SqliteModifierActivationStore(session).definition_revision_name(
                    definition.toStdString(), revision.toStdString());
            });
            const auto expected =
                exists ? std::optional<ModifierNameReadback>{ModifierNameEvidenceMissing{}}
                       : std::nullopt;
            require(actual == expected, "Exact legacy name evidence or outer absence");
        });
    }

    void named_append_refusal_data() {
        QTest::addColumn<int>("operation");
        QTest::newRow("set") << 0;
        QTest::newRow("clear") << 1;
        QTest::newRow("keep") << 2;
    }

    void named_append_refusal() {
        QFETCH(int, operation);
        const ModifierNameOperation operations[]{SetModifierName{" Doppelt \xc3\xa9 "},
                                                 ClearModifierName{}, KeepModifierName{}};
        check([&] {
            ModifierFile file;
            seed(file);
            const auto before = committed(*file.writer, rows);
            (void)committed(*file.writer, [&](const auto &session) {
                const SqliteModifierActivationStore store(session);
                // Keep has no legacy evidence. The catalog refusal precedes any lookup.
                bool refused = false;
                try {
                    (void)store.append_definition(new_definition, "z-old", operations[operation]);
                } catch (const SqliteModifierInputFailure &error) {
                    require(error.error() == SqliteModifierInputError::invalid_input,
                            "Expected existing invalid_input");
                    refused = true;
                }
                require(refused, "Name-aware append refuses on pre-v25");
                require(store.current_definition("D") ==
                            SqliteModifierDefinitionRevision{old_definition, 1},
                        "Healthy refusal leaves the exact old head readable");
                require(rows(session) == before,
                        "Refusal leaves definition and target rows intact");
                return true;
            });
            reopen(file);
            require(committed(*file.writer, rows) == before,
                    "Definition and target rows survive the committed refusal and reopen");
        });
    }
};
} // namespace

QTEST_GUILESS_MAIN(PreV25ModifierNames)
#include "pre_v25_tests.moc"
