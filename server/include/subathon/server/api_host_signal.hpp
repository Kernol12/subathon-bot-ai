// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/api_host.hpp>
namespace subathon::server::api_host {
// Install on Qt main BEFORE framework threads; handler only records/wakes.
class StopSignalBridge final {
  public:
    explicit StopSignalBridge(Host &);
    ~StopSignalBridge();
    Result<StartAccepted> install();
    StopSignalBridge(const StopSignalBridge &) = delete;
    StopSignalBridge &operator=(const StopSignalBridge &) = delete;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace subathon::server::api_host
