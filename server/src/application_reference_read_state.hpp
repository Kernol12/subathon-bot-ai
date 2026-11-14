// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QThread>
#include <atomic>
#include <subathon/server/application_service.hpp>
namespace subathon::server {
namespace detail {
class ApplicationReferenceReadState {
  public:
    SqliteWorkSession session;
    RetainedLimits limits;
    qsizetype intermediate_owned_bytes{0};
    qsizetype intermediate_items{0};
    QThread *owner{nullptr};
    std::atomic_bool active{true};
};
} // namespace detail
} // namespace subathon::server
