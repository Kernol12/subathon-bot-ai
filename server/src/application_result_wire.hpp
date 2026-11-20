// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <subathon/server/sqlite_application_bundle.hpp>

namespace subathon::server::application_result_wire {

template <class Writer> void put_component(Writer &w, const ApplicationComponent &c) {
    w.u8(unsigned(c.kind));
    w.i64(c.units);
    w.opt(c.target_ref, [&](const auto &s) { w.b(s); });
}
template <class Writer> void put_effect(Writer &w, const ApplicationEffect &e) {
    w.i64(e.request_ordinal);
    w.u8(unsigned(e.arithmetic_role));
    w.u8(unsigned(e.unit));
    w.exact(e.exact_requested_units);
    w.i64(e.requested_units);
    w.exact(e.rounding_residual);
    w.opt(e.maximum_main_ms, [&](auto x) { w.i64(x); });
    w.opt(e.deficit_policy, [&](auto x) { w.u8(unsigned(x)); });
    w.opt(e.deficit_disposition, [&](auto x) { w.u8(unsigned(x)); });
    w.b(e.application_explanation);
    w.vec(e.components, [&](const auto &x) { put_component(w, x); });
}
template <class Writer> void put_job(Writer &w, const ApplicationJobIntent &j) {
    w.b(j.job_ref);
    w.opt(j.source_request_ordinal, [&](auto x) { w.i64(x); });
    w.opt(j.group_ref, [&](const auto &x) { w.b(x); });
    w.opt(j.group_ordinal, [&](auto x) { w.i64(x); });
    w.b(j.envelope.format_ref);
    w.b(j.envelope.document);
}
template <class Writer>
void put_transitions(Writer &w, const std::vector<core::TimerTransition> &t) {
    w.vec(t, [&](auto x) { w.u8(unsigned(x)); });
}
template <class Writer>
void put_result_fields(Writer &w, const ApplicationEnvelope &command, const std::string &head,
                       const std::string &before, const std::string &after, bool revalidated,
                       const std::vector<ApplicationEffect> &effects,
                       const std::vector<ApplicationJobIntent> &jobs,
                       const std::vector<core::TimerTransition> &transitions) {
    w.b(command.document);
    w.b(head);
    w.b(before);
    w.b(after);
    w.u8(revalidated);
    w.vec(effects, [&](const auto &e) { put_effect(w, e); });
    w.vec(jobs, [&](const auto &j) { put_job(w, j); });
    put_transitions(w, transitions);
}

} // namespace subathon::server::application_result_wire
