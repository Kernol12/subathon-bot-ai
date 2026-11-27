// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../storage_003/protected_predecessor_fixture.hpp"
#include "sqlite_connection.hpp"
namespace storage_003_fixture {
// Select an independently verified whole-file pin without opening the donor.
inline QByteArray storage_021_donor_pin(int pages) {
    subathon::server::detail::Connection engine;
    require(engine.open(":memory:", false), "Open in-memory QSQLITE engine witness");
    QSqlQuery identity(engine.database());
    require(identity.exec("SELECT sqlite_version(), sqlite_source_id(), sqlite_compileoption_used('TEMP_STORE=1')") &&
                identity.next() && identity.value(2).toInt() == 1,
            "Read supported QSQLITE donor engine identity");
    const auto version = identity.value(0).toString();
    const auto source = identity.value(1).toString();
    require(pages == 1024 || pages == 1025, "Recognize pinned donor extent");
    if (version == "3.54.1" &&
        source == "2026-10-16 14:45:20 555e07ef33f69a0957d24d8e555b702464a131d6f1607f4c3c1e4551e8e6alt1")
        return pages == 1024 ? "729cec12d8abada51bb92459f5bb1f8b4580a236c585be29cf6ce08669218d11"
                             : "e34c69cbaa282f18f173b52e4051688c6523e7f2f9315f271bffb59ca85b01da";
    require(version == "3.53.4" &&
                source == "2026-07-24 19:02:57 bf7c7f30031888f4e796e429ab3978879485813aaca6f641c7b33e4e09459bcc",
            "Recognize pinned hosted QSQLITE engine");
    return pages == 1024 ? "d3b3e6da15c868cf154429745c0fea178c9a4051c9cef34f66f3aea01fdba713"
                         : "4e57dc3a6f5800f7a5f70b7137ab427c4e4722c03d044e88bc1da64fae20afd9";
}
} // namespace storage_003_fixture
