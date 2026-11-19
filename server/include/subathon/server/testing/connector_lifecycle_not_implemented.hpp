// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <stdexcept>

namespace subathon::server {
class ConnectorLifecycleNotImplemented final : public std::logic_error {
  public:
    explicit ConnectorLifecycleNotImplemented(const char *entry_point)
        : std::logic_error(entry_point) {}
};
} // namespace subathon::server
