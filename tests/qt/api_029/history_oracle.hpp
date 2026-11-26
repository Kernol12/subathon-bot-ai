// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "startup_fixture.hpp"
#include <QCryptographicHash>

namespace api_029_fixture {
inline QByteArray range_frame(const QByteArray &upper, const QByteArray &proof) {
    return QByteArray("CV1\x00\x02\x01", 6) + f("key-v1") + f("S/history") + f("11") + f(upper) +
           f({}) + f(proof);
}

inline QByteArray page_frame(const QList<QByteArray> &items, const std::optional<QByteArray> &next,
                             const QByteArray &range) {
    char count[4];
    qToBigEndian(quint32(items.size()), count);
    QByteArray result = QByteArray("P") + QByteArray(count, 4);
    for (const auto &item : items)
        result += f(item);
    return result + f(next.value_or(QByteArray{})) + f(range);
}

inline const auto covered_11_13 = range_frame("13", "cover-11-13");
inline const auto recovered_11_14 = range_frame("14", "recover-11-14");
inline const QList<QByteArray> audit_items{"norm-A-12-audit-v1", "norm-A-13-audit-v1"};
inline const auto audit_page = page_frame(audit_items, QByteArray("U"), covered_11_13);

inline void require_frame(const QByteArray &bytes, qsizetype size, const QByteArray &digest) {
    require(bytes.size() == size &&
                QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex() == digest,
            "Independent frame agrees with the frozen length and digest");
}

inline void check_page_oracles() {
    require_frame(audit_page, 118,
                  "fc07d50f49296ba088cb514abd92585eaf9891ae38dddaa0534ffb4e85e4d01a");
    require_frame(page_frame({}, {}, {}), 13,
                  "b2b4fc5433d6f52993f722c491f072502efee7d7be396935538f8ada6d1492dd");
    require_frame(page_frame({}, {}, recovered_11_14), 75,
                  "132fab617015a3df7d7171b852ef40861d9562387c52a832fcb4ab5daf4d0daa");
}
} // namespace api_029_fixture
