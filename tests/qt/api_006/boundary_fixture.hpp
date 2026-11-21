// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "intake_fixture.hpp"

namespace api_006_fixture {

inline void seed_mirror_policy(Fixture &f) {
    f.store.db.write([](const auto &s) {
        // Only policy/source prerequisites are seeded. Intake receipts use the
        // real receipt owner, and the product phase selects its exact intake key.
        (void)s.execute({"UPDATE main.subathon_intake_members SET primary_slot=NULL,"
                         "award_class=X'696e646976696475616c' WHERE provider=X'50' "
                         "AND account=X'41' AND stream=X'53'",
                         {}});
        (void)s.execute({"INSERT INTO main.subathon_intake_members VALUES("
                         "X'696e7374616c6c',1,X'50',X'7072696d617279',X'53',"
                         "X'696e646976696475616c',X'6d61696e',X'6f7665726c6170',1,X'70')",
                         {}});
        (void)s.execute({"INSERT INTO main.subathon_intake_sources "
                         "SELECT installation,provider,?,stream,epoch,generation,revision,"
                         "policy_revision,baseline_state,cut,frontier,document "
                         "FROM main.subathon_intake_sources WHERE installation=? AND provider=? "
                         "AND account=? AND stream=?",
                         {QByteArray("primary"), QByteArray("install"), QByteArray("P"),
                          QByteArray("A"), QByteArray("S")}});
    });
}

inline void seed_additional_mirror(Fixture &f, const intake::Scope &scope) {
    f.store.db.write([&](const auto &s) {
        (void)s.execute(
            {"INSERT INTO main.subathon_intake_members VALUES("
             "?,1,?,?,?,X'696e646976696475616c',X'6d61696e',X'6f7665726c6170',NULL,X'6d')",
             {scope.installation, scope.provider, scope.account, scope.stream}});
        (void)s.execute(
            {"INSERT INTO main.subathon_intake_sources VALUES("
             "?,?,?,?,1,X'6733',9,1,1,X'637574',X'3131',X'637572736f722d636f6e7472616374')",
             {scope.installation, scope.provider, scope.account, scope.stream}});
    });
}

// Protected v20 durable P grammar: a tag and BE32 item count, one BE32 length
// per normalized I item, and two BE32-length fields for continuation and proof.
inline storage::PageAdmission framed_page(const QList<QByteArray> &items, const QByteArray &proof) {
    auto page = storage::page_admission(1, items);
    page.binding.frontier_before = "11";
    page.binding.frontier_after = "14";
    page.binding.upper_bound = "14";
    page.binding.qualified_proof = proof;
    page.binding.page += storage::frame_fields({page.binding.next_continuation, proof});
    return page;
}

} // namespace api_006_fixture
