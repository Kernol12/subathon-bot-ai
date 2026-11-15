// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/reference_configuration.hpp>
namespace subathon::server {
ReferenceConfigurationArchive::ReferenceConfigurationArchive(
    std::vector<ReferenceConfigurationVersion> versions, ReferenceAdapterLimits limits)
    : versions_(std::move(versions)), limits_(limits) {}
ReferenceConfigurationResolution ReferenceConfigurationArchive::snapshot(const std::string &) {
    throw ReferenceNotImplemented("API-011 NI: configuration.snapshot");
}
ReferenceConfigurationResolution
ReferenceConfigurationArchive::load(const ApplicationReferenceReadView &, const std::string &) {
    throw ReferenceNotImplemented("API-011 NI: configuration.load");
}
} // namespace subathon::server
