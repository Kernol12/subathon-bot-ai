// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/application_service.hpp>

namespace qt_test_support {

// A fixture implementing all five ports can lend the service one view of itself.
class ApplicationPortFake : public subathon::server::ExecutionAccess,
                            public subathon::server::CurrentApplicationPolicy,
                            public subathon::server::SegmentedApplicationClock,
                            public subathon::server::RequiredApplicationActions,
                            public subathon::server::FreshApplicationReferences {
  public:
    subathon::server::ApplicationServicePorts ports() {
        return {*this, *this, *this, *this, *this};
    }
};

} // namespace qt_test_support
