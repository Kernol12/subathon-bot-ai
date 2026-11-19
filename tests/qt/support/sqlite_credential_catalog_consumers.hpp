// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_credential_catalog_fixture.hpp"
#include "sqlite_modifier_activations_fixture.hpp"

namespace credential_catalog_fixture {
namespace finance = financial_refund_fixture;
namespace milestones = milestone_group_restart_fixture;
namespace modifiers = modifier_activation_fixture;

enum class Consumer { financial, milestone, modifier };
enum class MarkerCase { exact_v16, case_changed_v16, exact_id_v17 };

inline void populate_v15(finance::File &file) {
    // First EUR 5 of EUR 20. One quarter of the actual 300000 ms and
    // 40000000 score, with 7/4 count retained as a fractional entitlement.
    const auto quarter = finance::fraction(1, 4);
    const FinancialEntitlement entitlement{
        0,
        5,
        quarter,
        false,
        GroupedRefundEntitlement{{finance::original, 1, finance::CK::main_delta},
                                 1,
                                 0,
                                 {},
                                 {75000, 0},
                                 75000,
                                 0,
                                 -75000,
                                 0},
        {{2,
          {finance::original, 2, finance::CK::score},
          0,
          0,
          10000000,
          -10000000,
          quarter,
          10000000,
          0,
          false},
         {3,
          {finance::original, 3, finance::CK::whole_count},
          0,
          0,
          0,
          0,
          quarter,
          finance::fraction(7, 4),
          finance::fraction(7, 4),
          false}}};
    finance::Effects effects;
    effects.apply(-75000, 0, {75000});
    const auto receipt =
        file.refund("catalog-refund-delivery", finance::core::ReceiptMoney{"5.00", "EUR"});
    (void)file.accept(file.command("catalog-refund", receipt, 0), entitlement, effects);

    file.write([](const auto &session) {
        // The refund fixture already committed achieved-35 (order 1). Add a
        // skipped group and an empty group whose ref sorts before achieved-35.
        SqliteMilestoneGroupStore(session).stage(
            finance::original, {{{{"skipped-target", "milestone-v1", 36000000, 40000000, "ctx"},
                                  {{MilestoneActionKind::bonus, "skip-bonus", "bonus-v1"}},
                                  {},
                                  MilestoneOccurrenceKey{"m-skipped"}},
                                 {}},
                                {{{"last-target", "milestone-v1", 37000000, 40000000, "ctx"},
                                  {},
                                  {},
                                  MilestoneOccurrenceKey{"a-last"}},
                                 {}}});
        (void)session.execute(
            {"INSERT INTO main.subathon_milestone_acceptance_decisions "
             "(occurrence_ref,mode,choice,actor_ref,decision_ref) VALUES(?,1,2,?,?)",
             {QByteArray("m-skipped"), QByteArray("synthetic-operator"),
              QByteArray("catalog-skip")}});
        modifiers::seed_old_live(session);
        modifiers::seed_definition_edit(session);
    });
}

inline void populate_v14(CatalogFile &file) {
    const milestones::app::Data data;
    auto origin = milestones::app::prepared(data.case_data("add10")["expected_record"]);
    file.write([&](const auto &session) {
        data.retain_sources(session);
        data.populate_case(session, "add10");
        milestones::stage_origin(session, std::move(origin));
        milestones::seed_head(session, 0);
        modifiers::seed_old_live(session);
        modifiers::seed_definition_edit(session);
    });
}

inline void expect_v15_evidence(const SqliteWorkSession &session, Consumer consumer,
                                const FinancialPaymentKey &payment) {
    switch (consumer) {
    case Consumer::financial: {
        const auto history = SqliteFinancialRefundStore(session).history(payment, 16);
        require(history && history->position == 1 && history->accepted_money == 5 &&
                    history->refunds.size() == 1 && history->resolutions.empty(),
                "Exact committed financial payment position and cumulative EUR amount");
        const auto &refund = history->refunds.front();
        require(refund.payment_position == 1 && refund.entitlement.accepted_money == 5 &&
                    refund.command.operation.operation_scope == "test" &&
                    refund.command.operation.operation_ref == "catalog-refund",
                "The same committed refund is selected by history");
        break;
    }
    case Consumer::milestone: {
        const auto pending = SqliteMilestoneGroupStore(session).pending();
        require(pending.size() == 2 && pending[0].group.occurrence == finance::achievement &&
                    pending[1].group.occurrence == MilestoneOccurrenceKey{"a-last"} &&
                    pending[0].group.actions.size() == 1 && pending[1].group.actions.empty(),
                "Pending occurrences retain queue order and exclude the terminal Skip");
        break;
    }
    case Consumer::modifier:
        require(SqliteModifierActivationStore(session).instance_revision("j1") ==
                    SqliteModifierInstanceRevision{modifiers::checkpointed_instance,
                                                   2,
                                                   ModifierInstanceOperation::checkpoint,
                                                   {},
                                                   ModifierInstanceLifetime::live,
                                                   {}},
                "Bare revision read preserves the prior captured definition and saved time");
        break;
    }
}

template <class Failure, class Reason, class Read>
void expect_consumer_refusal(SqliteWriter &writer, Reason reason, Read read) {
    auto refused = std::make_shared<bool>(false);
    const auto result = qt_test_support::run_work(
        writer,
        [=](auto context) -> QVariant {
            const auto session = context.begin();
            try {
                read(session);
            } catch (const Failure &failure) {
                *refused = failure.reason() == reason;
                throw;
            }
            session.commit();
            return 1;
        },
        10000);
    require(*refused && result.transaction == TransactionOutcome::rolled_back &&
                result.error == WorkError::work_exception && !result.value,
            "Public consumer returns the exact typed catalog refusal with known rollback");
}

inline void select_invalid_marker(finance::File &file, MarkerCase marker_case) {
    const int version = marker_case == MarkerCase::exact_id_v17 ? 17 : 16;
    const QString id =
        marker_case == MarkerCase::case_changed_v16 ? QString("Provider-credentials-v1") : marker;
    file.write([&](const auto &session) {
        (void)session.execute(
            {"UPDATE main.subathon_migration_history SET version=?,migration_id=? WHERE version=16",
             {version, id}});
    });
    require(file.rows("SELECT version,migration_id FROM main.subathon_migration_history "
                      "ORDER BY version DESC LIMIT 1") == SqlRows{{version, id}},
            "The altered latest pair is committed before testing a consumer");
}

} // namespace credential_catalog_fixture
