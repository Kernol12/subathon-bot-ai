// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "application_reference_read_state.hpp"

namespace subathon::server::detail {

class ContributionReviewAccess {
  public:
    template <class F> static auto call(const SqliteWorkSession &session, F fn) {
        auto state = std::make_shared<ApplicationReferenceReadState>();
        state->session = session;
        state->owner = QThread::currentThread();
        ApplicationReferenceReadView view;
        view.state_ = state;
        view.limits_ = state->limits;
        // The only strong reference expires here, even when authority throws.
        return fn(view);
    }
};

} // namespace subathon::server::detail
