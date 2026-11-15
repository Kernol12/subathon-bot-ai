// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdexcept>
namespace subathon::server {
// Stage-1 testing marker only. Every behavior entrypoint throws its own witness.
class ReferenceNotImplemented final : public std::logic_error {
  public:
    explicit ReferenceNotImplemented(const char *method) : std::logic_error(method) {}
};
} // namespace subathon::server
