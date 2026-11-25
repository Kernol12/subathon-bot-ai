// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QList>
#include <QVariant>

namespace subathon::server {
using SqlRow = QList<QVariant>;
using SqlRows = QList<SqlRow>;
} // namespace subathon::server
