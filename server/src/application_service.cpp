// SPDX-License-Identifier: GPL-3.0-or-later
#include "application_service_command_internal.hpp"
#include "sqlite_application_bundle_access.hpp"
#include "sqlite_application_hold_values.hpp"
#include <QThread>
#include <algorithm>
#include <atomic>
#include <bit>
#include <limits>
#include <string_view>
#include <subathon/server/application_service.hpp>
#include <tuple>

namespace subathon::server {
namespace {
using D = ApplicationServiceDisposition;
using S = ApplicationServiceFailureSource;
using R = ApplicationServiceLocalReason;
using core::Exact;
constexpr auto command_format = "subathon.internal.application-command.v1";
constexpr auto state_format = "subathon.internal.application-state.v1";
constexpr auto result_format = "subathon.internal.application-result.v1";
constexpr auto audit_format = "subathon.internal.application-audit.v1";
constexpr auto job_format = "subathon.internal.application-job-synthetic.v1";
struct LocalFailure {
    S source;
    R reason;
};
[[noreturn]] void fail(R reason, S source = S::port_contract) {
    throw LocalFailure{source, reason};
}
void check(bool condition, R reason = R::invalid_output, S source = S::port_contract) {
    if (!condition)
        fail(reason, source);
}
size_t sum(size_t a, size_t b) {
    check(b <= std::numeric_limits<size_t>::max() - a, R::result_limit);
    return a + b;
}
void bound(size_t bytes, size_t items, const ApplicationServiceLimits &l, bool final = false) {
    check(bytes <= size_t(final ? l.result_owned_bytes : l.intermediate_owned_bytes) &&
              items <= size_t(final ? l.result_items : l.intermediate_items),
          R::result_limit);
}
bool ref(const std::string &s, const ApplicationServiceLimits &l) {
    return !s.empty() && s.size() <= size_t(l.reference_bytes);
}
bool same_plan(const ApplicationPlanKey &a, const ApplicationPlanKey &b) {
    return a.plan_ref == b.plan_ref && a.revision == b.revision;
}
bool same_selector(const ApplicationOriginalClaimSelector &a,
                   const ApplicationOriginalClaimSelector &b) {
    return a.subject_kind == b.subject_kind && a.receipt_id == b.receipt_id &&
           a.contribution_id == b.contribution_id;
}
ApplicationOriginalClaimSelector selector(const OriginalApplicationCommand &c) {
    return {c.subject_kind, c.receipt_id, c.contribution_id};
}
ApplicationHoldSubject subject(const OriginalApplicationCommand &c) {
    return c.contribution_id ? ApplicationHoldSubject{*c.contribution_id}
                             : ApplicationHoldSubject{c.receipt_id};
}
ApplicationServiceResponse response(D d) { return {d, std::monostate{}}; }
ApplicationServiceResponse unavailable(std::string_view reason,
                                       const ApplicationServiceLimits &limits) {
    bound(sum(1, reason.size()), 0, limits, true);
    return {D::not_ready, ApplicationNotReadyPayload{std::string(reason)}};
}
OriginalApplicationCommand retained_command(const ApplicationEnvelope &e,
                                            const ApplicationServiceLimits &l) {
    try {
        return service_detail::decode_retained_command(e, l);
    } catch (const ApplicationServiceInputFailure &x) {
        if (x.error() == ApplicationServiceInputError::result_limit ||
            x.error() == ApplicationServiceInputError::owned_byte_limit ||
            x.error() == ApplicationServiceInputError::collection_limit)
            fail(R::result_limit);
        fail(R::invalid_known_document, S::retained_semantics);
    }
}
// One bounded grammar writer supports a sizing pass before materialization.
class Bytes {
  public:
    explicit Bytes(size_t limit, bool materialize = true)
        : limit_(limit), materialize_(materialize) {}
    void raw(std::string_view s) {
        check(s.size() <= limit_ - size_, R::result_limit);
        size_ += s.size();
        if (materialize_)
            value_.append(s.data(), qsizetype(s.size()));
    }
    void u8(unsigned x) {
        const char c = char(x);
        raw({&c, 1});
    }
    void u32(size_t x) {
        check(x <= std::numeric_limits<std::uint32_t>::max(), R::result_limit);
        for (int n = 3; n >= 0; --n)
            u8(unsigned((x >> (n * 8)) & 255));
    }
    void u64(std::uint64_t x) {
        for (int n = 7; n >= 0; --n)
            u8(unsigned((x >> (n * 8)) & 255));
    }
    void i64(std::int64_t x) { u64(std::bit_cast<std::uint64_t>(x)); }
    void b(std::string_view x) {
        u32(x.size());
        raw(x);
    }
    void b(const QByteArray &x) { b(std::string_view(x.constData(), size_t(x.size()))); }
    template <class T, class F> void opt(const std::optional<T> &v, F f) {
        u8(v ? 1 : 0);
        if (v)
            f(*v);
    }
    template <class T, class F> void vec(const std::vector<T> &v, F f) {
        u32(v.size());
        for (const auto &x : v)
            f(x);
    }
    void exact(const Exact &x) {
        b(boost::multiprecision::numerator(x).str());
        b(boost::multiprecision::denominator(x).str());
    }
    QByteArray take() { return value_; }
    size_t size() const { return size_; }

  private:
    size_t limit_, size_{0};
    bool materialize_;
    QByteArray value_{"", 0};
};
template <class F> QByteArray document(const ApplicationServiceLimits &l, F f) {
    const auto limit = size_t(
        std::min({l.storage_document_bytes, l.bundles.value_bytes, l.intermediate_owned_bytes}));
    Bytes sizing(limit, false);
    f(sizing);
    Bytes output(limit);
    f(output);
    return output.take();
}
template <class F>
ApplicationEnvelope encoded(const char *format, const ApplicationServiceLimits &limits, F write) {
    const auto format_bytes = std::char_traits<char>::length(format);
    check(format_bytes < size_t(limits.intermediate_owned_bytes), R::result_limit);
    auto remaining = limits;
    remaining.intermediate_owned_bytes -= qsizetype(format_bytes);
    auto bytes = document(remaining, write);
    return {format, std::move(bytes)};
}
void put_timer(Bytes &w, const core::TimerState &t) {
    w.u8(unsigned(t.lifecycle));
    w.u8(unsigned(t.phase));
    w.i64(t.main_remaining_ms);
    w.i64(t.grace_remaining_ms);
    w.u8(t.awaiting_confirmation);
}
void put_balances(Bytes &w, const std::vector<ApplicationBalance> &v) {
    w.vec(v, [&](const auto &b) {
        w.u8(unsigned(b.kind));
        w.b(b.target_ref);
        w.i64(b.units);
    });
}
void put_clock(Bytes &w, const ApplicationClockObservation &c) {
    w.b(c.clock_lineage_ref);
    w.b(c.observation_ref);
    w.i64(c.observed_utc_ms);
    w.vec(c.segments, [&](const auto &s) {
        w.i64(s.elapsed_ms);
        w.i64(s.utc_now_ms);
        w.u8(s.paused);
    });
}
void put_policy(Bytes &w, const BoundApplicationPolicy &p) {
    w.b(p.configuration_ref);
    w.b(p.policy_revision_ref);
    w.b(p.permission_revision_ref);
    w.opt(p.maximum_main_ms, [&](auto x) { w.i64(x); });
    w.u8(unsigned(p.timer_config.end_mode));
    w.i64(p.timer_config.grace_duration_ms);
    w.u8(unsigned(p.timer_config.grace_reuse));
    w.opt(p.timer_config.hard_end_utc_ms, [&](auto x) { w.i64(x); });
    w.u8(p.paused);
    w.u8(p.recovery_pending);
    w.b(p.pause_recovery_revision);
    w.b(p.explanation);
}
void put_component(Bytes &w, const ApplicationComponent &c) {
    w.u8(unsigned(c.kind));
    w.i64(c.units);
    w.opt(c.target_ref, [&](const auto &s) { w.b(s); });
}
void put_effect(Bytes &w, const ApplicationEffect &e) {
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
void put_job(Bytes &w, const ApplicationJobIntent &j) {
    w.b(j.job_ref);
    w.opt(j.source_request_ordinal, [&](auto x) { w.i64(x); });
    w.opt(j.group_ref, [&](const auto &x) { w.b(x); });
    w.opt(j.group_ordinal, [&](auto x) { w.i64(x); });
    w.b(j.envelope.format_ref);
    w.b(j.envelope.document);
}
void put_transitions(Bytes &w, const std::vector<core::TimerTransition> &t) {
    w.vec(t, [&](auto x) { w.u8(unsigned(x)); });
}
void put_failure(Bytes &w, const ApplicationHoldFailureEvidence &f) {
    w.u8(unsigned(f.stage));
    w.opt(f.failing_ordinal, [&](auto x) { w.i64(x); });
    w.opt(f.failing_target_ref, [&](const auto &x) { w.b(x); });
    w.opt(f.attempted_units, [&](const auto &x) { w.exact(x); });
}
ApplicationEnvelope encode_state(const ApplicationStateSnapshot &s, const std::string &basis,
                                 const ApplicationClockObservation &c,
                                 const BoundApplicationPolicy &p,
                                 const ApplicationServiceLimits &l) {
    return encoded(state_format, l, [&](Bytes &w) {
        w.raw("SAS1");
        w.b(s.configuration_ref);
        w.u8(1);
        w.b(basis);
        put_clock(w, c);
        w.b(p.policy_revision_ref);
        put_timer(w, s.timer);
        w.i64(s.time_debt_ms);
        put_balances(w, s.balances);
    });
}
ApplicationEnvelope encode_result(
    const ApplicationEnvelope &cmd, const std::string &head, const ApplicationStateSnapshot &before,
    const ApplicationStateSnapshot &after, const OriginalApplicationCommand &c,
    const std::vector<ApplicationEffect> &effects, const std::vector<ApplicationJobIntent> &jobs,
    const std::vector<core::TimerTransition> &transitions, const ApplicationServiceLimits &l) {
    return encoded(result_format, l, [&](Bytes &w) {
        w.raw("SAR1");
        w.b(cmd.document);
        w.b(head);
        w.b(before.token);
        w.b(after.token);
        w.u8(c.preview_mode == ApplicationPreviewMode::revalidate_current &&
             c.preview_state_token && *c.preview_state_token != head);
        w.vec(effects, [&](const auto &e) { put_effect(w, e); });
        w.vec(jobs, [&](const auto &j) { put_job(w, j); });
        put_transitions(w, transitions);
    });
}
ApplicationEnvelope encode_audit(const ApplicationEnvelope &cmd,
                                 const OriginalApplicationCommand &c,
                                 const BoundApplicationPolicy &p,
                                 const ApplicationClockObservation &clock,
                                 const std::string &actions, const ApplicationServiceLimits &l) {
    return encoded(audit_format, l, [&](Bytes &w) {
        w.raw("SAA1");
        w.b(cmd.document);
        put_policy(w, p);
        put_clock(w, clock);
        w.opt(c.actor_ref, [&](const auto &x) { w.b(x); });
        w.opt(c.decision_ref, [&](const auto &x) { w.b(x); });
        w.opt(c.reason, [&](const auto &x) { w.b(x); });
        w.b(c.plan.plan_ref);
        w.u64(c.plan.revision);
        w.u8(0);
        w.u8(0);
        w.u8(0);
        w.b(std::string_view{});
        w.b(actions);
    });
}
// Parsers own no byte buffers and reject framing/limits before materializing fields.
class Read {
  public:
    Read(const ApplicationEnvelope &e, const char *format, const char *magic,
         const ApplicationServiceLimits &l)
        : bytes_(e.document.constData(), size_t(e.document.size())), l_(l) {
        if (e.format_ref != format)
            throw ApplicationServiceUnsupportedFormat(e.format_ref);
        check(!e.document.isNull(), R::invalid_known_document, S::retained_semantics);
        check(bytes_.size() <= size_t(l.storage_document_bytes) &&
                  bytes_.size() <= size_t(l.bundles.value_bytes),
              R::result_limit);
        bound(sum(e.format_ref.size(), bytes_.size()), 0, l);
        check(take(4) == magic, R::invalid_known_document, S::retained_semantics);
    }
    std::string_view take(size_t n) {
        check(n <= bytes_.size() - pos_, R::invalid_known_document, S::retained_semantics);
        auto v = bytes_.substr(pos_, n);
        pos_ += n;
        return v;
    }
    unsigned u8(unsigned max = 255) {
        auto x = static_cast<unsigned char>(take(1)[0]);
        check(x <= max, R::invalid_known_document, S::retained_semantics);
        return x;
    }
    std::uint64_t u64() {
        std::uint64_t x = 0;
        for (auto c : take(8))
            x = (x << 8) | static_cast<unsigned char>(c);
        return x;
    }
    std::int64_t i64() { return std::bit_cast<std::int64_t>(u64()); }
    size_t count(size_t ceiling) {
        size_t x = 0;
        for (auto c : take(4))
            x = (x << 8) | static_cast<unsigned char>(c);
        check(x <= ceiling, R::result_limit);
        return x;
    }
    std::string_view b(size_t ceiling) {
        auto size = count(std::numeric_limits<std::uint32_t>::max());
        auto value = take(size);
        check(size <= ceiling, R::result_limit);
        return value;
    }
    std::string str(size_t ceiling) { return std::string(b(ceiling)); }
    std::string reference(bool required = true) {
        auto x = str(size_t(l_.reference_bytes));
        check(!required || !x.empty(), R::invalid_known_document, S::retained_semantics);
        return x;
    }
    template <class F> auto opt(F f) -> std::optional<decltype(f())> {
        if (!u8(1))
            return std::nullopt;
        return f();
    }
    void end() { check(pos_ == bytes_.size(), R::invalid_known_document, S::retained_semantics); }
    const ApplicationServiceLimits &limits() const { return l_; }

  private:
    std::string_view bytes_;
    size_t pos_{0};
    const ApplicationServiceLimits &l_;
};
// Scan a complete document using byte views before any owning semantic field or
// vector is allocated. Budgets describe each actual decoded value, not raw framing.
struct Projection {
    const ApplicationServiceLimits &limits;
    size_t bytes{0};
    size_t items{0};
    void add(size_t b, size_t i = 0) {
        bytes = sum(bytes, b);
        items = sum(items, i);
        bound(bytes, items, limits);
    }
};
std::string_view scan_ref(Read &r, bool required = true) {
    auto v = r.b(size_t(r.limits().reference_bytes));
    check(!required || !v.empty(), R::invalid_known_document, S::retained_semantics);
    return v;
}
void scan_optional_string(Read &r, Projection &p, size_t ceiling, bool required = false) {
    if (r.u8(1)) {
        auto v = r.b(ceiling);
        check(!required || !v.empty(), R::invalid_known_document, S::retained_semantics);
        p.add(v.size());
    }
}
void scan_exact(Read &r) {
    for (int part = 0; part != 2; ++part) {
        auto v = r.b(size_t(r.limits().bundles.component_bytes));
        check(!v.empty(), R::invalid_known_document, S::retained_semantics);
        const auto digits = part == 0 && v.front() == '-' ? v.substr(1) : v;
        check(!digits.empty() && !(digits.size() > 1 && digits.front() == '0') &&
                  std::all_of(digits.begin(), digits.end(),
                              [](char c) { return c >= '0' && c <= '9'; }) &&
                  v != "-0" && (part == 0 || v != "0"),
              R::invalid_known_document, S::retained_semantics);
    }
}
void scan_clock(Read &r, Projection &p) {
    p.add(scan_ref(r).size());
    p.add(scan_ref(r).size());
    const auto observed = r.i64();
    p.add(8);
    auto n = r.count(size_t(r.limits().clock_segments));
    p.add(0, n);
    std::int64_t elapsed = 0;
    std::int64_t final_utc = observed;
    for (size_t k = 0; k != n; ++k) {
        auto step = r.i64();
        final_utc = r.i64();
        (void)r.u8(1);
        check(step >= 0 && step <= std::numeric_limits<std::int64_t>::max() - elapsed,
              R::invalid_known_document, S::retained_semantics);
        elapsed += step;
        p.add(17);
    }
    check(n == 0 || final_utc == observed, R::invalid_known_document, S::retained_semantics);
}
void scan_policy(Read &r) {
    Projection p{r.limits()};
    for (int k = 0; k != 3; ++k)
        p.add(scan_ref(r).size());
    // The parsed BoundApplicationPolicy owns its declared default selector,
    // plan and admission scalar members as well as the encoded policy fields.
    p.add(30);
    if (r.u8(1)) {
        check(r.i64() >= 0, R::invalid_known_document, S::retained_semantics);
        p.add(8);
    }
    (void)r.u8(2);
    check(r.i64() >= 0, R::invalid_known_document, S::retained_semantics);
    (void)r.u8(1);
    if (r.u8(1)) {
        (void)r.i64();
        p.add(8);
    }
    (void)r.u8(1);
    (void)r.u8(1);
    p.add(scan_ref(r).size());
    p.add(r.b(size_t(r.limits().storage_document_bytes)).size());
}
void scan_component(Read &r) {
    auto kind = r.u8(5);
    (void)r.i64();
    bool target = r.u8(1) != 0;
    check(target == (kind == 2 || kind == 3), R::invalid_known_document, S::retained_semantics);
    if (target)
        (void)scan_ref(r);
}
void scan_effect(Read &r) {
    (void)r.i64();
    (void)r.u8(2);
    (void)r.u8(2);
    scan_exact(r);
    (void)r.i64();
    scan_exact(r);
    if (r.u8(1))
        (void)r.i64();
    if (r.u8(1))
        (void)r.u8(2);
    if (r.u8(1))
        (void)r.u8(1);
    (void)r.b(size_t(r.limits().storage_document_bytes));
    auto n = r.count(3); // Existing direct/time roles own one or three components.
    for (size_t i = 0; i != n; ++i)
        scan_component(r);
}
void scan_job_value(Read &r) {
    (void)scan_ref(r);
    if (r.u8(1))
        (void)r.i64();
    if (r.u8(1))
        (void)scan_ref(r);
    if (r.u8(1))
        (void)r.i64();
    (void)scan_ref(r);
    (void)r.b(size_t(r.limits().storage_document_bytes));
}
void scan_state(const ApplicationStateSnapshot &s, const ApplicationServiceLimits &l) {
    Read r(s.envelope, state_format, "SAS1", l);
    Projection p{l};
    auto config = scan_ref(r);
    p.add(config.size());
    scan_optional_string(r, p, size_t(l.reference_bytes), true);
    scan_clock(r, p);
    auto policy = scan_ref(r);
    p.add(policy.size());
    (void)r.u8(2);
    (void)r.u8(1);
    check(r.i64() >= 0 && r.i64() >= 0, R::invalid_known_document, S::retained_semantics);
    (void)r.u8(1);
    check(r.i64() >= 0, R::invalid_known_document, S::retained_semantics);
    p.add(27);
    auto n = r.count(size_t(l.bundles.balances_per_state));
    p.add(0, n);
    for (size_t i = 0; i != n; ++i) {
        check(r.u8(3) >= 2, R::invalid_known_document, S::retained_semantics);
        p.add(sum(scan_ref(r).size(), 9));
        (void)r.i64();
    }
    r.end();
    // Actual returned observation owns these explicit echoed strings too.
    p.add(sum(sum(config.size(), policy.size()), s.token.size()));
}
void scan_result(const ApplicationEnvelope &e, const ApplicationServiceLimits &l) {
    Read r(e, result_format, "SAR1", l);
    (void)r.b(size_t(l.command_bytes));
    for (int i = 0; i != 3; ++i)
        (void)scan_ref(r);
    (void)r.u8(1);
    auto n = r.count(size_t(l.bundles.effects_per_bundle));
    for (size_t i = 0; i != n; ++i)
        scan_effect(r);
    n = r.count(size_t(l.bundles.jobs_per_bundle));
    for (size_t i = 0; i != n; ++i)
        scan_job_value(r);
    n = r.count(size_t(l.bundles.transitions_per_bundle));
    for (size_t i = 0; i != n; ++i)
        (void)r.u8(5);
    r.end();
}
void scan_audit(const ApplicationEnvelope &e, const ApplicationServiceLimits &l) {
    Read r(e, audit_format, "SAA1", l);
    (void)r.b(size_t(l.command_bytes));
    scan_policy(r);
    Projection clock{l};
    scan_clock(r, clock);
    Projection refs{l};
    scan_optional_string(r, refs, size_t(l.reference_bytes));
    scan_optional_string(r, refs, size_t(l.reference_bytes));
    scan_optional_string(r, refs, size_t(l.storage_document_bytes));
    (void)scan_ref(r);
    (void)r.u64();
    (void)r.u8(2);
    if (r.u8(1)) {
        (void)r.u8(1);
        if (r.u8(1))
            (void)r.i64();
        if (r.u8(1))
            (void)scan_ref(r);
        if (r.u8(1))
            scan_exact(r);
    }
    scan_optional_string(r, refs, size_t(l.reference_bytes), true);
    (void)r.b(size_t(l.storage_document_bytes));
    (void)r.b(size_t(l.storage_document_bytes));
    r.end();
}
void scan_synthetic_job(const ApplicationEnvelope &e, const ApplicationServiceLimits &l) {
    Read r(e, job_format, "SAJ1", l);
    (void)r.i64();
    for (int i = 0; i != 3; ++i)
        (void)scan_ref(r);
    (void)r.u8(2);
    (void)r.i64();
    auto n = r.count(3);
    for (size_t i = 0; i != n; ++i)
        scan_component(r);
    (void)scan_ref(r);
    (void)r.b(size_t(l.storage_document_bytes));
    r.end();
}

core::TimerState read_timer(Read &r) {
    core::TimerState t;
    t.lifecycle = core::TimerLifecycle(r.u8(2));
    t.phase = core::CountdownPhase(r.u8(1));
    t.main_remaining_ms = r.i64();
    t.grace_remaining_ms = r.i64();
    t.awaiting_confirmation = r.u8(1) != 0;
    check(t.main_remaining_ms >= 0 && t.grace_remaining_ms >= 0, R::invalid_known_document,
          S::retained_semantics);
    return t;
}
ApplicationClockObservation read_clock(Read &r) {
    ApplicationClockObservation c;
    c.clock_lineage_ref = r.reference();
    c.observation_ref = r.reference();
    c.observed_utc_ms = r.i64();
    auto n = r.count(size_t(std::min(r.limits().clock_segments, r.limits().intermediate_items)));
    for (size_t i = 0; i < n; ++i) {
        core::TimerClockStep s;
        s.elapsed_ms = r.i64();
        s.utc_now_ms = r.i64();
        s.paused = r.u8(1) != 0;
        check(s.elapsed_ms >= 0, R::invalid_known_document, S::retained_semantics);
        c.segments.push_back(s);
    }
    return c;
}
BoundApplicationPolicy read_policy(Read &r) {
    BoundApplicationPolicy p;
    p.configuration_ref = r.reference();
    p.policy_revision_ref = r.reference();
    p.permission_revision_ref = r.reference();
    p.maximum_main_ms = r.opt([&] { return r.i64(); });
    p.timer_config.end_mode = core::TimerEndMode(r.u8(2));
    p.timer_config.grace_duration_ms = r.i64();
    p.timer_config.grace_reuse = core::GraceReuse(r.u8(1));
    p.timer_config.hard_end_utc_ms = r.opt([&] { return r.i64(); });
    p.paused = r.u8(1) != 0;
    p.recovery_pending = r.u8(1) != 0;
    p.pause_recovery_revision = r.reference();
    p.explanation = r.str(size_t(r.limits().storage_document_bytes));
    check((!p.maximum_main_ms || *p.maximum_main_ms >= 0) && p.timer_config.grace_duration_ms >= 0,
          R::invalid_known_document, S::retained_semantics);
    return p;
}
struct StateMeaning {
    std::string configuration;
    core::TimerState timer;
    std::int64_t debt{0};
    std::vector<ApplicationBalance> balances;
    std::optional<std::string> basis;
    ApplicationClockObservation clock;
    std::string policy;
};
StateMeaning check_state(const ApplicationStateSnapshot &s, const ApplicationServiceLimits &l) {
    scan_state(s, l);
    Read r(s.envelope, state_format, "SAS1", l);
    auto config = r.reference();
    StateMeaning m;
    m.basis = r.opt([&] { return r.reference(); });
    m.clock = read_clock(r);
    m.policy = r.reference();
    auto t = read_timer(r);
    auto debt = r.i64();
    auto n = r.count(size_t(std::min(l.bundles.balances_per_state, l.intermediate_items)));
    std::vector<ApplicationBalance> balances;
    for (size_t i = 0; i < n; ++i)
        balances.push_back({ApplicationBalanceKind(r.u8(3)), r.reference(), r.i64()});
    r.end();
    check(config == s.configuration_ref && debt == s.time_debt_ms &&
              std::tie(t.lifecycle, t.phase, t.main_remaining_ms, t.grace_remaining_ms,
                       t.awaiting_confirmation) ==
                  std::tie(s.timer.lifecycle, s.timer.phase, s.timer.main_remaining_ms,
                           s.timer.grace_remaining_ms, s.timer.awaiting_confirmation) &&
              balances.size() == s.balances.size() &&
              std::equal(balances.begin(), balances.end(), s.balances.begin(),
                         bundle_detail::same_balance),
          R::contradictory_document, S::retained_semantics);
    m.configuration = config;
    m.timer = t;
    m.debt = debt;
    m.balances = std::move(balances);
    m.clock.configuration_ref = config;
    m.clock.policy_revision_ref = m.policy;
    m.clock.checked_head_token = s.token;
    return m;
}
size_t record_items(const RecordedApplicationBundle &v) {
    size_t n = 0;
    for (auto count : {v.persisted_head.balances.size(), v.advanced_before.balances.size(),
                       v.after.balances.size(), v.effects.size(), v.jobs.size(),
                       v.transitions.size(), v.reversals.size()})
        n = sum(n, count);
    for (const auto &e : v.effects)
        n = sum(n, e.components.size());
    return n;
}
size_t record_bytes(const RecordedApplicationBundle &v) {
    bundle_detail::ByteCount n;
    using bundle_detail::count_bytes;
    count_bytes(n, v.intent);
    count_bytes(n, v.persisted_head);
    count_bytes(n, v.advanced_before);
    count_bytes(n, v.after);
    count_bytes(n, v.advance_explanation);
    if (v.contribution) {
        n.add(9);
        count_bytes(n, v.contribution->before);
    }
    for (const auto &e : v.effects)
        count_bytes(n, e);
    for (const auto &j : v.jobs)
        count_bytes(n, j);
    n.add(v.transitions.size());
    for (const auto &x : v.reversals)
        count_bytes(n, x);
    count_bytes(n, v.result);
    count_bytes(n, v.audit);
    return n.total();
}
void final_bounds(const ApplicationServiceResponse &r, const ApplicationServiceLimits &l) {
    size_t bytes = 1, items = 0;
    std::visit(
        [&](const auto &v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, RecordedApplicationBundle>) {
                bytes = sum(bytes, record_bytes(v));
                items = record_items(v);
            } else if constexpr (std::is_same_v<T, std::vector<core::ContributionBlocker>>) {
                bytes = sum(bytes, v.size());
                items = v.size();
            } else if constexpr (std::is_same_v<T, ApplicationActiveHoldPayload>) {
                if (v.holds) {
                    bytes = sum(bytes, hold_detail::bytes(*v.holds));
                    items = hold_detail::items(*v.holds);
                }
            } else if constexpr (std::is_same_v<T, ApplicationHeldPayload>) {
                bytes = sum(bytes, sum(v.hold_ref.size(), hold_detail::bytes(v.outcome)));
                items = hold_detail::items(v.outcome);
            } else if constexpr (std::is_same_v<T, ApplicationNotReadyPayload>)
                bytes = sum(bytes, v.reason_ref.size());
            else if constexpr (std::is_same_v<T, ApplicationStageConflict>)
                bytes = sum(bytes, 1);
        },
        r.payload);
    bound(bytes, items, l, true);
}
} // namespace
namespace detail {
class ApplicationReferenceReadState {
  public:
    SqliteWorkSession session;
    RetainedLimits limits;
    QThread *owner{nullptr};
    std::atomic_bool active{true};
};
} // namespace detail
std::optional<RetainedDocument>
ApplicationReferenceReadView::find_retained(const RetainedKey &key) const {
    auto state = state_.lock();
    const auto limits = limits_;
    if (key.kind != RetainedKind::configuration && key.kind != RetainedKind::arrival_context)
        throw RetentionFailure(RetentionError::invalid_kind);
    if (key.reference.empty() || key.reference.size() > size_t(limits.reference_bytes))
        throw RetentionFailure(RetentionError::invalid_reference);
    if (!state || !state->active)
        throw ApplicationReferenceViewFailure(ApplicationReferenceViewError::expired_callback);
    if (QThread::currentThread() != state->owner)
        throw ApplicationReferenceViewFailure(ApplicationReferenceViewError::wrong_thread);
    return SqliteRetainedContextStore(state->session, limits).find(key);
}
namespace {
size_t clock_bytes(const ApplicationClockObservation &c) {
    return sum(sum(sum(sum(c.clock_lineage_ref.size(), c.observation_ref.size()),
                       sum(c.configuration_ref.size(), c.policy_revision_ref.size())),
                   sum(c.checked_head_token.size(), 8)),
               c.segments.size() * 17);
}
size_t policy_bytes(const BoundApplicationPolicy &p) {
    bundle_detail::ByteCount n;
    bundle_detail::count_bytes(n, p.operation);
    n.add(9 + (p.source.contribution_id ? 8 : 0));
    bundle_detail::count_bytes(n, p.plan);
    for (const auto *s : {&p.checked_state_token, &p.configuration_ref, &p.permission_revision_ref,
                          &p.policy_revision_ref, &p.pause_recovery_revision, &p.explanation})
        n.add(s->size());
    n.add(13 + (p.maximum_main_ms ? 8 : 0) + (p.timer_config.hard_end_utc_ms ? 8 : 0));
    if (p.admission.reason_ref)
        n.add(p.admission.reason_ref->size());
    n.add(p.correction_policies.size() * 9);
    return n.total();
}
void check_unavailable(const ApplicationUnavailable &u, const ApplicationServiceLimits &l) {
    bound(u.reason_ref.size(), 0, l);
    check(ref(u.reason_ref, l));
}
void read_exact(Read &r) {
    auto n = r.str(size_t(r.limits().bundles.component_bytes));
    auto d = r.str(size_t(r.limits().bundles.component_bytes));
    auto canonical = [](const std::string &s, bool negative) {
        size_t p = negative && !s.empty() && s[0] == '-' ? 1 : 0;
        return p < s.size() && !(s.size() - p > 1 && s[p] == '0') &&
               std::all_of(
                   s.begin() + std::ptrdiff_t(p), s.end(),
                   [](char c) { return c >= '0' && c <= '9'; }) &&
               s != "-0";
    };
    check(canonical(n, true) && canonical(d, false) && d != "0", R::invalid_known_document,
          S::retained_semantics);
    Exact x{boost::multiprecision::cpp_int(n)};
    x /= boost::multiprecision::cpp_int(d);
    check(boost::multiprecision::numerator(x).str() == n &&
              boost::multiprecision::denominator(x).str() == d,
          R::invalid_known_document, S::retained_semantics);
}
void skip_component(Read &r) {
    (void)r.u8(5);
    (void)r.i64();
    (void)r.opt([&] { return r.reference(); });
}
void skip_effect(Read &r) {
    (void)r.i64();
    (void)r.u8(2);
    (void)r.u8(2);
    read_exact(r);
    (void)r.i64();
    read_exact(r);
    (void)r.opt([&] { return r.i64(); });
    (void)r.opt([&] { return r.u8(2); });
    (void)r.opt([&] { return r.u8(1); });
    (void)r.b(size_t(r.limits().storage_document_bytes));
    auto n = r.count(size_t(r.limits().intermediate_items));
    for (size_t i = 0; i < n; ++i)
        skip_component(r);
}
void skip_job(Read &r) {
    (void)r.reference();
    (void)r.opt([&] { return r.i64(); });
    (void)r.opt([&] { return r.reference(); });
    (void)r.opt([&] { return r.i64(); });
    (void)r.reference();
    (void)r.b(size_t(r.limits().storage_document_bytes));
}
void check_job_meaning(const ApplicationJobIntent &j, const std::vector<ApplicationEffect> &effects,
                       const core::CapturedPlanRevision &capture,
                       const ApplicationServiceLimits &l) {
    check(j.source_request_ordinal && *j.source_request_ordinal > 0 &&
              size_t(*j.source_request_ordinal) <= capture.captured.requested_effects.size() &&
              size_t(*j.source_request_ordinal) <= effects.size(),
          R::contradictory_document, S::retained_semantics);
    scan_synthetic_job(j.envelope, l);
    Read read(j.envelope, job_format, "SAJ1", l);
    const auto ordinal = read.i64();
    const auto action = read.reference();
    const auto version = read.reference();
    const auto target = read.reference();
    const auto unit = read.u8(2);
    const auto requested = read.i64();
    const auto count = read.count(3);
    for (size_t n = 0; n != count; ++n)
        skip_component(read);
    const auto synthetic = read.reference();
    const auto payload = read.str(size_t(l.storage_document_bytes));
    read.end();
    const auto index = size_t(*j.source_request_ordinal - 1);
    const auto &r = capture.captured.requested_effects[index];
    check(ordinal == *j.source_request_ordinal && action == r.action_ref &&
              version == r.action_version_ref && target == r.target_ref &&
              unit == unsigned(r.unit) && requested == r.requested_units,
          R::contradictory_document, S::retained_semantics);
    auto expected = document(l, [&](Bytes &w) {
        w.raw("SAJ1");
        w.i64(ordinal);
        w.b(r.action_ref);
        w.b(r.action_version_ref);
        w.b(r.target_ref);
        w.u8(unsigned(r.unit));
        w.i64(r.requested_units);
        w.vec(effects[index].components, [&](const auto &c) { put_component(w, c); });
        w.b(synthetic);
        w.b(payload);
    });
    check(expected == j.envelope.document, R::contradictory_document, S::retained_semantics);
}

OriginalApplicationCommand check_record(const RecordedApplicationBundle &v,
                                        const ApplicationServiceLimits &l,
                                        const core::CapturedPlanRevision &capture) {
    auto c = retained_command(v.intent.choices, l);
    check(bundle_detail::same_key(c.operation, v.intent.operation) &&
              same_plan(c.plan, v.intent.plan) && c.subject_kind == v.intent.subject_kind &&
              c.receipt_id == v.intent.receipt_id &&
              c.contribution_id == v.intent.contribution_id &&
              v.intent.kind == ApplicationOperationKind::original && !v.intent.reversal_ref,
          R::contradictory_document, S::retained_semantics);
    for (const auto &job : v.jobs)
        check_job_meaning(job, v.effects, capture, l);
    auto head = check_state(v.persisted_head, l);
    auto before = check_state(v.advanced_before, l);
    auto after = check_state(v.after, l);
    scan_result(v.result, l);
    Read result(v.result, result_format, "SAR1", l);
    (void)result.b(size_t(l.command_bytes));
    (void)result.reference();
    (void)result.reference();
    (void)result.reference();
    (void)result.u8(1);
    auto n = result.count(size_t(std::min(l.bundles.effects_per_bundle, l.intermediate_items)));
    for (size_t i = 0; i < n; ++i)
        skip_effect(result);
    n = result.count(size_t(std::min(l.bundles.jobs_per_bundle, l.intermediate_items)));
    for (size_t i = 0; i < n; ++i)
        skip_job(result);
    n = result.count(size_t(std::min(l.bundles.transitions_per_bundle, l.intermediate_items)));
    for (size_t i = 0; i < n; ++i)
        (void)result.u8(5);
    result.end();
    auto expected = encode_result(v.intent.choices, v.persisted_head.token, v.advanced_before,
                                  v.after, c, v.effects, v.jobs, v.transitions, l);
    check(bundle_detail::same_envelope(expected, v.result), R::contradictory_document,
          S::retained_semantics);
    scan_audit(v.audit.envelope, l);
    Read audit(v.audit.envelope, audit_format, "SAA1", l);
    auto command = audit.b(size_t(l.command_bytes));
    auto p = read_policy(audit);
    auto clock = read_clock(audit);
    auto actor = audit.opt([&] { return audit.reference(false); });
    auto decision = audit.opt([&] { return audit.reference(false); });
    auto reason = audit.opt([&] { return audit.str(size_t(l.storage_document_bytes)); });
    ApplicationPlanKey plan{audit.reference(), audit.u64()};
    auto outcome = audit.u8(2);
    bool failure = audit.u8(1) != 0;
    if (failure) {
        (void)audit.u8(1);
        (void)audit.opt([&] { return audit.i64(); });
        (void)audit.opt([&] { return audit.reference(); });
        if (audit.u8(1))
            read_exact(audit);
    }
    auto missing = audit.opt([&] { return audit.reference(); });
    auto calculation = audit.b(size_t(l.storage_document_bytes));
    auto action = audit.str(size_t(l.storage_document_bytes));
    audit.end();
    check(command == std::string_view(v.intent.choices.document.constData(),
                                      size_t(v.intent.choices.document.size())) &&
              actor == c.actor_ref && decision == c.decision_ref && reason == c.reason &&
              v.audit.actor_ref == c.actor_ref && v.audit.decision_ref == c.decision_ref &&
              v.audit.reason == c.reason && same_plan(plan, c.plan) && outcome == 0 && !failure &&
              !missing && calculation.empty() && p.configuration_ref == v.after.configuration_ref &&
              p.policy_revision_ref == after.policy && p.policy_revision_ref == before.policy &&
              p.configuration_ref == v.advanced_before.configuration_ref,
          R::contradictory_document, S::retained_semantics);
    auto clock_doc = [&](const ApplicationClockObservation &x) {
        return document(l, [&](Bytes &w) { put_clock(w, x); });
    };
    check(clock_doc(clock) == clock_doc(before.clock) && clock_doc(clock) == clock_doc(after.clock),
          R::contradictory_document, S::retained_semantics);
    if (v.advanced_before.token != v.persisted_head.token)
        check(before.basis == v.persisted_head.token, R::contradictory_document,
              S::retained_semantics);
    check(after.basis == v.persisted_head.token, R::contradictory_document, S::retained_semantics);
    auto audit_expected = encode_audit(v.intent.choices, c, p, clock, action, l);
    check(bundle_detail::same_envelope(audit_expected, v.audit.envelope), R::contradictory_document,
          S::retained_semantics);
    return c;
}
void check_policy(const BoundApplicationPolicy &p, const OriginalApplicationCommand &c,
                  const ApplicationBundleInputs &i, const ApplicationAccessAllowed &a,
                  const ApplicationServiceLimits &l) {
    bound(policy_bytes(p), p.correction_policies.size(), l);
    check(bundle_detail::same_key(p.operation, c.operation) &&
              same_selector(p.source, selector(c)) && same_plan(p.plan, c.plan) &&
              p.checked_state_token == i.current_state.token &&
              p.configuration_ref == i.current_state.configuration_ref &&
              p.permission_revision_ref == a.permission_revision_ref,
          R::invalid_binding);
    check(ref(p.policy_revision_ref, l) && ref(p.pause_recovery_revision, l) &&
          p.explanation.size() <= size_t(l.storage_document_bytes) &&
          (!p.maximum_main_ms || *p.maximum_main_ms >= 0) &&
          bundle_detail::enum_in(p.timer_config.end_mode, 2) &&
          bundle_detail::enum_in(p.timer_config.grace_reuse, 1) &&
          p.timer_config.grace_duration_ms >= 0 &&
          bundle_detail::enum_in(p.admission.disposition, 3));
    check((p.admission.disposition == ApplicationPolicyAdmission::allow_active) ==
          !p.admission.reason_ref.has_value());
    if (p.admission.reason_ref)
        check(ref(*p.admission.reason_ref, l));
    size_t index = 0;
    std::int64_t previous = 0;
    for (const auto &q : p.correction_policies) {
        check(q.request_ordinal > previous &&
              size_t(q.request_ordinal) <= i.selected_plan.captured.requested_effects.size() &&
              bundle_detail::enum_in(q.deficit_policy, 2));
        const auto &r = i.selected_plan.captured.requested_effects[size_t(q.request_ordinal - 1)];
        check(c.purpose == OriginalApplicationPurpose::signed_original_correction &&
              r.unit == core::RequestedUnit::milliseconds && r.requested_units < 0);
        previous = q.request_ordinal;
    }
    for (const auto &r : i.selected_plan.captured.requested_effects) {
        ++index;
        if (r.requested_units < 0)
            check(c.purpose == OriginalApplicationPurpose::signed_original_correction);
        if (r.unit == core::RequestedUnit::milliseconds && r.requested_units < 0)
            check(c.purpose == OriginalApplicationPurpose::signed_original_correction &&
                  std::any_of(
                      p.correction_policies.begin(), p.correction_policies.end(),
                      [&](const auto &q) { return q.request_ordinal == std::int64_t(index); }));
    }
}
void check_clock(const ApplicationClockObservation &c, const ApplicationBundleInputs &i,
                 const BoundApplicationPolicy &p, const StateMeaning &old,
                 const ApplicationServiceLimits &l) {
    bound(clock_bytes(c), c.segments.size(), l);
    check(c.segments.size() <= size_t(l.clock_segments), R::result_limit);
    check(ref(c.clock_lineage_ref, l) && ref(c.observation_ref, l) &&
              c.configuration_ref == p.configuration_ref &&
              c.policy_revision_ref == p.policy_revision_ref &&
              c.checked_head_token == i.current_state.token,
          R::invalid_binding);
    check(c.clock_lineage_ref == old.clock.clock_lineage_ref, R::invalid_binding);
    std::int64_t elapsed = 0;
    for (const auto &s : c.segments) {
        check(s.elapsed_ms >= 0 &&
              s.elapsed_ms <= std::numeric_limits<std::int64_t>::max() - elapsed);
        elapsed += s.elapsed_ms;
    }
    if (c.segments.empty())
        check(c.observation_ref == old.clock.observation_ref &&
                  c.observed_utc_ms == old.clock.observed_utc_ms,
              R::invalid_binding);
    else
        check(c.segments.back().utc_now_ms == c.observed_utc_ms);
}
size_t proposal_bytes(const ApplicationCalculatedProposal &p) {
    bundle_detail::ByteCount n;
    n.add(46);
    for (const auto &b : p.balances_after)
        bundle_detail::count_bytes(n, b);
    for (const auto &e : p.effects)
        bundle_detail::count_bytes(n, e);
    n.add(p.transitions.size());
    n.add(clock_bytes(p.observation));
    return n.total();
}
size_t proposal_items(const ApplicationCalculatedProposal &p) {
    size_t n = sum(sum(sum(p.balances_after.size(), p.effects.size()), p.transitions.size()),
                   p.observation.segments.size());
    for (const auto &e : p.effects)
        n = sum(n, e.components.size());
    return n;
}
struct NumericFailure {
    ApplicationHoldFailureEvidence evidence;
    unsigned balance;
};
NumericFailure numeric_failure(size_t ordinal, const core::RequestedEffect &r, const Exact &attempt,
                               unsigned balance) {
    return {{ApplicationHoldFailureStage::aggregate_after_finalization,
             std::int64_t(ordinal),
             r.target_ref,
             attempt,
             {}},
            balance};
}
std::int64_t signed_value(const Exact &x) {
    if (x < std::numeric_limits<std::int64_t>::min() ||
        x > std::numeric_limits<std::int64_t>::max())
        throw std::overflow_error("application aggregate overflow");
    return x.convert_to<std::int64_t>();
}
void append_transitions(ApplicationCalculatedProposal &p,
                        const std::vector<core::TimerTransition> &additional,
                        const ApplicationServiceLimits &limits, size_t pending_bytes = 0,
                        size_t pending_items = 0) {
    const auto count = sum(p.transitions.size(), additional.size());
    check(count <= size_t(limits.bundles.transitions_per_bundle), R::result_limit);
    bound(sum(sum(proposal_bytes(p), pending_bytes), additional.size()),
          sum(sum(proposal_items(p), pending_items), additional.size()), limits);
    p.transitions.insert(p.transitions.end(), additional.begin(), additional.end());
}
size_t pending_effect_bytes(const core::RequestedEffect &r, const Exact &residual,
                            const BoundApplicationPolicy &policy) {
    bundle_detail::ByteCount bytes;
    bytes.add(18);
    bundle_detail::count_bytes(bytes, r.exact_requested_units);
    bundle_detail::count_bytes(bytes, residual);
    bytes.add(r.calculation_explanation.size());
    if (r.unit == core::RequestedUnit::milliseconds) {
        bytes.add(27); // Three actual kind/int64 components, including zero values.
        if (r.requested_units >= 0 && policy.maximum_main_ms)
            bytes.add(8);
        if (r.requested_units < 0)
            bytes.add(2); // Policy and applied disposition.
    } else {
        bytes.add(sum(9, r.target_ref.size()));
    }
    return bytes.total();
}

ApplicationCalculatedProposal calculate(const ApplicationBundleInputs &i,
                                        const BoundApplicationPolicy &p,
                                        const ApplicationClockObservation &clock,
                                        const ApplicationServiceLimits &l) {
    bundle_detail::ByteCount initial;
    initial.add(46);
    initial.add(clock_bytes(clock));
    for (const auto &balance : i.current_state.balances)
        bundle_detail::count_bytes(initial, balance);
    bound(initial.total(), sum(i.current_state.balances.size(), clock.segments.size()), l);
    ApplicationCalculatedProposal out;
    out.clock_advanced_timer = i.current_state.timer;
    out.debt_after = i.current_state.time_debt_ms;
    out.balances_after = i.current_state.balances;
    out.observation = clock;
    for (const auto &s : clock.segments) {
        auto advanced = core::advance_timer(out.clock_advanced_timer, p.timer_config, s);
        out.clock_advanced_timer = advanced.state;
        append_transitions(out, advanced.transitions, l);
    }
    out.timer_after = out.clock_advanced_timer;
    return out;
}
// Full arithmetic remains in memory. Borrowed component descriptions let refusal
// paths return before any hypothetical output vector or provenance copy is made.
std::optional<ApplicationServiceResponse> requests(ApplicationCalculatedProposal &out,
                                                   const ApplicationBundleInputs &i,
                                                   const BoundApplicationPolicy &p,
                                                   const ApplicationServiceLimits &l) {
    struct ComponentDescription {
        ApplicationComponentKind kind;
        std::int64_t units;
        std::optional<std::string_view> target;
    };
    size_t ordinal = 0;
    for (const auto &r : i.selected_plan.captured.requested_effects) {
        ++ordinal;
        std::array<ComponentDescription, 3> components{};
        size_t component_count = 0;
        auto timer = out.timer_after;
        auto debt = out.debt_after;
        ApplicationArithmeticRole role = ApplicationArithmeticRole::direct;
        std::optional<core::RefundDeficitPolicy> deficit_policy;
        std::optional<core::TimeReversalDisposition> deficit_disposition;
        std::vector<core::TimerTransition> additional;
        if (r.unit == core::RequestedUnit::milliseconds && r.requested_units >= 0) {
            role = ApplicationArithmeticRole::time_addition;
            const Exact attempt = Exact(timer.main_remaining_ms) +
                                  std::max(Exact(0), Exact(r.requested_units) - Exact(debt));
            try {
                auto n =
                    core::plan_time_addition({timer.main_remaining_ms, debt},
                                             {"original", r.requested_units}, p.maximum_main_ms);
                timer = core::apply_applicable_time_credit(timer, n.main_added_ms);
                check(timer.main_remaining_ms == n.balance_after.main_remaining_ms);
                debt = n.balance_after.time_debt_ms;
                components = {
                    {{ApplicationComponentKind::main_delta, n.main_added_ms, std::nullopt},
                     {ApplicationComponentKind::debt_delta, -n.debt_repaid_ms, std::nullopt},
                     {ApplicationComponentKind::cap_limited_time, n.cap_limited_ms, std::nullopt}}};
                component_count = 3;
            } catch (const std::overflow_error &) {
                throw numeric_failure(ordinal, r, attempt, 0);
            }
        } else if (r.unit == core::RequestedUnit::milliseconds) {
            role = ApplicationArithmeticRole::time_reversal;
            const auto policy = std::find_if(
                p.correction_policies.begin(), p.correction_policies.end(),
                [&](const auto &q) { return q.request_ordinal == std::int64_t(ordinal); });
            check(policy != p.correction_policies.end());
            deficit_policy = policy->deficit_policy;
            const Exact attempt =
                Exact(debt) +
                std::max(Exact(0), -Exact(r.requested_units) - Exact(timer.main_remaining_ms));
            try {
                auto n = core::plan_time_reversal({timer.main_remaining_ms, debt},
                                                  {"original", "captured", r.requested_units},
                                                  *deficit_policy);
                if (n.disposition == core::TimeReversalDisposition::RequiresReview)
                    return unavailable("required_mixed_timer_policy", l);
                deficit_disposition = n.disposition;
                timer.main_remaining_ms = n.balance_after.main_remaining_ms;
                debt = n.balance_after.time_debt_ms;
                components = {
                    {{ApplicationComponentKind::main_delta, n.main_adjustment_ms, std::nullopt},
                     {ApplicationComponentKind::debt_delta, n.debt_added_ms, std::nullopt},
                     {ApplicationComponentKind::unapplied_time, n.unapplied_adjustment_ms,
                      std::nullopt}}};
                component_count = 3;
                auto advanced = core::advance_timer(timer, p.timer_config,
                                                    {0, out.observation.observed_utc_ms, p.paused});
                timer = advanced.state;
                additional = std::move(advanced.transitions);
            } catch (const std::overflow_error &) {
                throw numeric_failure(ordinal, r, attempt, 1);
            }
        } else {
            const auto kind = r.unit == core::RequestedUnit::micro_points
                                  ? ApplicationBalanceKind::score
                                  : ApplicationBalanceKind::whole_count;
            auto target = std::find_if(
                out.balances_after.begin(), out.balances_after.end(),
                [&](const auto &v) { return v.kind == kind && v.target_ref == r.target_ref; });
            if (target == out.balances_after.end())
                return unavailable("required_counter_definition", l);
            const Exact attempt = Exact(target->units) + Exact(r.requested_units);
            try {
                target->units = signed_value(attempt);
            } catch (const std::overflow_error &) {
                throw numeric_failure(ordinal, r, attempt, unsigned(kind));
            }
            components[0] = {r.unit == core::RequestedUnit::micro_points
                                 ? ApplicationComponentKind::score
                                 : ApplicationComponentKind::whole_count,
                             r.requested_units, std::string_view(r.target_ref)};
            component_count = 1;
        }
        const Exact residual = r.exact_requested_units - Exact(r.requested_units);
        const auto effect_bytes = pending_effect_bytes(r, residual, p);
        check(out.effects.size() < size_t(l.bundles.effects_per_bundle), R::result_limit);
        check(sum(out.transitions.size(), additional.size()) <=
                  size_t(l.bundles.transitions_per_bundle),
              R::result_limit);
        bound(effect_bytes, component_count, l);
        bound(sum(sum(proposal_bytes(out), effect_bytes), additional.size()),
              sum(sum(proposal_items(out), sum(1, component_count)), additional.size()), l);
        ApplicationEffect effect;
        effect.request_ordinal = std::int64_t(ordinal);
        effect.arithmetic_role = role;
        effect.unit = r.unit;
        effect.exact_requested_units = r.exact_requested_units;
        effect.requested_units = r.requested_units;
        effect.rounding_residual = residual;
        if (role == ApplicationArithmeticRole::time_addition)
            effect.maximum_main_ms = p.maximum_main_ms;
        effect.deficit_policy = deficit_policy;
        effect.deficit_disposition = deficit_disposition;
        effect.application_explanation = r.calculation_explanation;
        effect.components.reserve(component_count);
        for (size_t n = 0; n != component_count; ++n) {
            const auto &description = components[n];
            effect.components.push_back({description.kind, description.units,
                                         description.target
                                             ? std::optional{std::string(*description.target)}
                                             : std::nullopt});
        }
        append_transitions(out, additional, l, effect_bytes, sum(1, component_count));
        out.effects.push_back(std::move(effect));
        out.timer_after = timer;
        out.debt_after = debt;
    }
    return std::nullopt;
}

struct AttemptState {
    std::optional<ApplicationServiceFailure> failure;
};
using ViewFactory = std::function<ApplicationReferenceReadView(
    std::shared_ptr<detail::ApplicationReferenceReadState>)>;
class Execute {
  public:
    Execute(SqliteWorkSession session, ApplicationServicePorts &ports,
            const ApplicationServiceLimits &limits, const OriginalApplicationCommand &command,
            const ApplicationEnvelope &encoded, ViewFactory factory,
            std::shared_ptr<AttemptState> attempt)
        : session_(session), ports_(ports), l_(limits), c_(command), cmd_(encoded),
          factory_(std::move(factory)), attempt_(std::move(attempt)),
          bundles_(session, limits.bundles), holds_(session, limits.holds) {}
    template <class F> auto call(F f) {
        auto state = std::make_shared<detail::ApplicationReferenceReadState>();
        state->session = session_;
        state->limits = {std::min(l_.reference_bytes, l_.bundles.reference_bytes),
                         std::min(l_.storage_document_bytes, l_.bundles.value_bytes)};
        state->owner = QThread::currentThread();
        auto view = factory_(state);
        struct Revoke {
            std::shared_ptr<detail::ApplicationReferenceReadState> s;
            ~Revoke() { s->active = false; }
        } revoke{state};
        try {
            return f(view);
        } catch (const RetentionFailure &e) {
            attempt_->failure = ApplicationServiceFailure{S::retained, e.error()};
            throw;
        } catch (const SqliteWorkFailure &e) {
            if (e.error() == WorkError::busy || e.error() == WorkError::sql_failure ||
                e.error() == WorkError::resource_limit || e.error() == WorkError::result_limit)
                attempt_->failure = ApplicationServiceFailure{S::retained, e.error()};
            throw;
        }
    }
    ApplicationServiceResponse historical(const RecordedApplicationBundle &record, D d) {
        if (!call([&](const auto &v) { return ports_.access.disclose_application(v, c_, record); }))
            return response(D::disclose_denied);
        // Reread only the immutable selected revision. This same-session helper
        // preserves the bundle's existing typed failure/rollback mapping and never
        // resolves current policy, eligibility, state or a replacement plan head.
        Access access(session_, l_.bundles);
        auto capture = access.guarded([&] {
            auto selected = access.plan(record.intent.plan);
            if (!selected)
                access.fail(SqliteBundleStorageReason::missing_plan);
            access.result_size(access.plan_items(selected->selected),
                               bundle_detail::logical_bytes(selected->selected));
            return std::move(selected->selected);
        });
        (void)check_record(record, l_, capture);
        if (d == D::existing_operation) {
            if (!bundle_detail::same_envelope(record.intent.choices, cmd_))
                return response(D::operation_conflict);
        }
        bound(sum(1, record_bytes(record)), record_items(record), l_, true);
        return {d, record};
    }
    std::optional<std::string> issue(ApplicationReferencePurpose purpose,
                                     const ApplicationBundleInputs &i,
                                     std::optional<std::int64_t> ordinal) {
        ApplicationReferenceBinding binding{c_.operation,          selector(c_), c_.plan,
                                            i.current_state.token, purpose,      ordinal};
        auto result = ports_.references.issue(purpose, binding);
        if (auto *u = std::get_if<ApplicationUnavailable>(&result)) {
            check_unavailable(*u, l_);
            return std::nullopt;
        }
        const auto &r = std::get<IssuedApplicationReference>(result);
        size_t bytes = sum(sum(sum(sum(sum(r.binding.operation.operation_scope.size(),
                                           r.binding.operation.operation_ref.size()),
                                       9 + (r.binding.source.contribution_id ? 8 : 0)),
                                   sum(r.binding.plan.plan_ref.size(), 8)),
                               r.binding.persisted_head_token.size()),
                           sum(1 + (r.binding.request_ordinal ? 8 : 0), r.reference.size()));
        bound(bytes, 0, l_);
        check(bundle_detail::same_key(binding.operation, r.binding.operation) &&
                  same_selector(binding.source, r.binding.source) &&
                  same_plan(binding.plan, r.binding.plan) &&
                  binding.persisted_head_token == r.binding.persisted_head_token &&
                  binding.purpose == r.binding.purpose &&
                  binding.request_ordinal == r.binding.request_ordinal,
              R::invalid_binding);
        check(ref(r.reference, l_));
        return r.reference;
    }
    // Mirrors the published hold DTO field walk without constructing any owned
    // cause/observation/command/occurrence copy. Opaque annotations add no items.
    void project_detection(const ApplicationBundleInputs &inputs,
                           const ApplicationAccessAllowed &access,
                           const BoundApplicationPolicy *policy,
                           const ApplicationClockObservation *clock, const NumericFailure *failure,
                           const std::string *missing, const std::string &hold_ref,
                           const std::string &detection_ref, const std::string &version_ref,
                           size_t annotation_bytes) {
        using bundle_detail::count_bytes;
        const auto &event = inputs.selected_plan.captured.event;
        bundle_detail::ByteCount source;
        source.add(17); // Tagged subject plus receipt ID.
        count_bytes(source, event.event_ref);
        count_bytes(source, event.parent_event_ref);
        count_bytes(source, event.root_event_ref);
        count_bytes(source, event.arrival.configuration_ref);
        count_bytes(source, event.arrival.modifier_context_ref);
        bundle_detail::ByteCount observation;
        count_bytes(observation, inputs.current_state.token);
        observation.add(annotation_bytes);
        if (policy)
            count_bytes(observation, policy->policy_revision_ref);
        count_bytes(observation, access.permission_revision_ref);
        if (c_.actor_ref && !c_.actor_ref->empty())
            count_bytes(observation, *c_.actor_ref);
        if (c_.decision_ref && !c_.decision_ref->empty())
            count_bytes(observation, *c_.decision_ref);
        if (clock)
            observation.add(16); // Actually known UTC and elapsed.
        bundle_detail::ByteCount plan;
        plan.add(2); // Captured-revision kind and complete extent.
        count_bytes(plan, c_.plan);
        const auto &requests = inputs.selected_plan.captured.requested_effects;
        check(requests.size() <= size_t(l_.holds.known_occurrences_per_cause), R::result_limit);
        for (const auto &request : requests) {
            plan.add(17); // Ordinal, unit, finalized units.
            for (const auto *field :
                 {&request.action_ref, &request.action_version_ref, &request.rule_ref,
                  &request.target_ref, &request.calculation_explanation})
                count_bytes(plan, *field);
            count_bytes(plan, request.exact_requested_units);
        }
        bundle_detail::ByteCount cause;
        count_bytes(cause, hold_ref);
        cause.add(source.total());
        count_bytes(cause, c_.operation);
        count_bytes(cause, cmd_);
        cause.add(observation.total());
        cause.add(plan.total());
        cause.add(1); // Cause kind.
        if (failure) {
            cause.add(1); // Failure stage.
            if (failure->evidence.failing_ordinal)
                cause.add(8);
            count_bytes(cause, failure->evidence.failing_target_ref);
            count_bytes(cause, failure->evidence.attempted_units);
            cause.add(annotation_bytes);
        }
        if (missing)
            count_bytes(cause, *missing);
        bundle_detail::ByteCount detection;
        detection.add(cause.total());
        count_bytes(detection, detection_ref);
        count_bytes(detection, version_ref);
        detection.add(observation.total());
        detection.add(annotation_bytes); // Detection audit is its own real copy.
        bound(detection.total(), requests.size(), l_);
        check(detection.total() <= size_t(l_.holds.owned_bytes) &&
                  requests.size() <= size_t(l_.holds.input_items),
              R::result_limit);
    }

    ApplicationServiceResponse hold(const ApplicationBundleInputs &i,
                                    const ApplicationAccessAllowed &access,
                                    const BoundApplicationPolicy *policy,
                                    const ApplicationClockObservation *clock,
                                    const NumericFailure *failure, const std::string *missing) {
        auto ordinal = failure ? failure->evidence.failing_ordinal : std::optional<std::int64_t>{};
        auto h = issue(ApplicationReferencePurpose::hold, i, ordinal);
        if (!h)
            return unavailable("required_application_reference", l_);
        auto d = issue(ApplicationReferencePurpose::detection, i, ordinal);
        if (!d)
            return unavailable("required_application_reference", l_);
        auto v = issue(ApplicationReferencePurpose::hold_version, i, ordinal);
        if (!v)
            return unavailable("required_application_reference", l_);
        check(*h != *d && *h != *v && *d != *v);
        bool v2 = policy && !policy->correction_policies.empty();
        const char *format = v2 ? "subathon.internal.application-hold-observation.v2"
                                : "subathon.internal.application-hold-observation.v1";
        auto write_annotation = [&](Bytes &w) {
            w.raw(v2 ? "SAH2" : "SAH1");
            w.b(cmd_.document);
            w.b(i.current_state.token);
            w.u8(policy ? 1 : 0);
            if (policy)
                put_policy(w, *policy);
            w.u8(clock ? 1 : 0);
            if (clock)
                put_clock(w, *clock);
            w.u8(failure ? 1 : 0);
            if (failure)
                put_failure(w, failure->evidence);
            w.u8(missing ? 1 : 0);
            if (missing)
                w.b(*missing);
            w.b(std::string_view{});
            if (v2) {
                w.vec(policy->correction_policies, [&](const auto &x) {
                    w.i64(x.request_ordinal);
                    w.u8(unsigned(x.deficit_policy));
                });
                w.u8(failure ? 1 : 0);
                if (failure)
                    w.u8(failure->balance);
            }
        };
        auto annotation_limits = l_;
        annotation_limits.bundles.value_bytes =
            std::min(l_.bundles.value_bytes, l_.holds.references.value_bytes);
        Bytes sizing(size_t(std::min({annotation_limits.storage_document_bytes,
                                      annotation_limits.bundles.value_bytes,
                                      annotation_limits.intermediate_owned_bytes})),
                     false);
        write_annotation(sizing);
        const auto annotation_bytes = sum(std::char_traits<char>::length(format), sizing.size());
        project_detection(i, access, policy, clock, failure, missing, *h, *d, *v, annotation_bytes);
        auto annotation = encoded(format, annotation_limits, write_annotation);
        ApplicationHoldDetection detection;
        auto &cause = detection.cause;
        cause.hold_ref = *h;
        const auto &e = i.selected_plan.captured.event;
        cause.source = {subject(c_),
                        c_.receipt_id,
                        e.event_ref,
                        e.parent_event_ref,
                        e.root_event_ref,
                        e.arrival.configuration_ref,
                        e.arrival.modifier_context_ref};
        cause.command = {c_.operation, cmd_};
        ApplicationHoldObservation observation;
        observation.persisted_state_token = i.current_state.token;
        observation.annotation = annotation;
        observation.permission_revision_ref = access.permission_revision_ref;
        if (policy)
            observation.policy_revision_ref = policy->policy_revision_ref;
        if (c_.actor_ref && !c_.actor_ref->empty())
            observation.actor_ref = c_.actor_ref;
        if (c_.decision_ref && !c_.decision_ref->empty())
            observation.decision_ref = c_.decision_ref;
        if (clock) {
            observation.observed_utc_ms = clock->observed_utc_ms;
            std::int64_t elapsed = 0;
            for (const auto &s : clock->segments)
                elapsed += s.elapsed_ms;
            observation.elapsed_ms = elapsed;
        }
        cause.first_observation = observation;
        cause.plan_evidence.plan = c_.plan;
        size_t n = 0;
        for (const auto &r : i.selected_plan.captured.requested_effects) {
            ++n;
            cause.plan_evidence.occurrences.push_back(
                {std::int64_t(n), r.action_ref, r.action_version_ref, r.rule_ref, r.target_ref,
                 r.unit, r.exact_requested_units, r.requested_units, r.calculation_explanation});
        }
        cause.kind = failure ? ApplicationHoldCauseKind::numeric_overflow
                             : ApplicationHoldCauseKind::unresolved_policy;
        if (failure) {
            cause.failure = failure->evidence;
            cause.failure->annotation = annotation;
        }
        if (missing)
            cause.missing_policy_reason = *missing;
        detection.request = {*d, std::nullopt, *v, observation, annotation};
        bound(hold_detail::bytes(detection), hold_detail::items(detection), l_);
        auto result = holds_.record_detection(detection);
        using H = SqliteHoldMutationDisposition;
        if (result.disposition == H::recorded) {
            check(result.outcome.has_value());
            const auto &o = *result.outcome;
            check(o.mutation_ref == *d && o.kind == ApplicationHoldMutationKind::detection &&
                  o.produced_versions.size() == 1);
            const auto &version = o.produced_versions[0];
            check(version.locator.hold_ref == *h && version.locator.version_ref == *v &&
                  version.position == 1 && !version.parent_version_ref &&
                  version.state == ApplicationHoldState::active &&
                  version.producer_mutation_ref == *d && !version.replacement);
            bound(sum(1, sum(h->size(), hold_detail::bytes(o))), hold_detail::items(o), l_, true);
            return {failure ? D::held_numeric : D::held_policy, ApplicationHeldPayload{o, *h}};
        }
        check(result.disposition != H::stale_active_set && result.disposition != H::missing_hold &&
              result.disposition != H::replacement_conflict);
        if (result.disposition != H::already_recorded)
            check(!result.outcome);
        return {D::stage_conflict, ApplicationStageConflict{result.disposition}};
    }
    ApplicationServiceResponse run() {
        auto access = call([&](const auto &v) { return ports_.access.check(v, c_); });
        auto *allowed = std::get_if<ApplicationAccessAllowed>(&access);
        if (!allowed)
            return response(D::denied);
        bound(allowed->permission_revision_ref.size(), 0, l_);
        check(ref(allowed->permission_revision_ref, l_));
        if (auto old = bundles_.lookup(c_.operation))
            return historical(*old, D::existing_operation);
        if (auto claim = bundles_.lookup_original_claim(selector(c_)))
            return historical(claim->existing, claim->kind == SqliteOriginalClaimKind::subject
                                                   ? D::subject_claimed
                                                   : D::source_claimed);
        auto active = holds_.active(subject(c_));
        if (!active.holds.empty()) {
            bool disclose =
                call([&](const auto &v) { return ports_.access.disclose_holds(v, c_, active); });
            bound(disclose ? sum(1, hold_detail::bytes(active)) : 1,
                  disclose ? hold_detail::items(active) : 0, l_, true);
            return {D::active_hold,
                    ApplicationActiveHoldPayload{disclose ? std::optional{active} : std::nullopt}};
        }
        auto read = bundles_.read_inputs(c_.plan, c_.subject_kind);
        if (read.disposition == SqliteBundleInputDisposition::missing_plan)
            return response(D::missing_plan);
        if (read.disposition == SqliteBundleInputDisposition::stale_plan)
            return response(D::plan_conflict);
        check(read.inputs.has_value());
        auto &i = *read.inputs;
        const auto &e = i.selected_plan.captured.event;
        check(i.subject_kind == c_.subject_kind && same_plan(i.plan, c_.plan) &&
                  i.actual_plan_head == c_.plan.revision && e.receipt_id == c_.receipt_id &&
                  e.contribution_id == c_.contribution_id,
              R::invalid_binding);
        if (c_.preview_mode == ApplicationPreviewMode::require_exact &&
            c_.preview_state_token != i.current_state.token)
            return response(D::state_conflict);
        auto old = check_state(i.current_state, l_);
        auto resolved =
            call([&](const auto &v) { return ports_.policy.resolve(v, c_, i, access); });
        if (auto *u = std::get_if<ApplicationUnavailable>(&resolved)) {
            check_unavailable(*u, l_);
            return hold(i, *allowed, nullptr, nullptr, nullptr, &u->reason_ref);
        }
        auto policy = std::get<BoundApplicationPolicy>(std::move(resolved));
        check_policy(policy, c_, i, *allowed, l_);
        auto observed = call([&](const auto &v) { return ports_.clock.observe(v, c_, i, policy); });
        if (auto *u = std::get_if<ApplicationUnavailable>(&observed)) {
            check_unavailable(*u, l_);
            return hold(i, *allowed, &policy, nullptr, nullptr, &u->reason_ref);
        }
        auto clock = std::get<ApplicationClockObservation>(std::move(observed));
        check_clock(clock, i, policy, old, l_);
        // Ready is excluded before the reducer. Paused Active/Finished still observes
        // hard-end transitions before final admission, and a refusal writes no clock.
        std::optional<ApplicationCalculatedProposal> proposal;
        if (i.current_state.timer.lifecycle != core::TimerLifecycle::Ready)
            proposal = calculate(i, policy, clock, l_);
        if (i.current_contribution) {
            auto eligibility =
                core::assess_contribution_eligibility(i.current_contribution->contribution.state);
            if (!eligibility.blockers.empty()) {
                bound(sum(1, eligibility.blockers.size()), eligibility.blockers.size(), l_, true);
                return {D::ineligible,
                        std::vector<core::ContributionBlocker>(eligibility.blockers.begin(),
                                                               eligibility.blockers.end())};
            }
        }
        if (!proposal || proposal->clock_advanced_timer.lifecycle != core::TimerLifecycle::Active ||
            policy.paused || policy.recovery_pending)
            return unavailable("required_application_lifecycle_policy", l_);
        if (policy.admission.disposition == ApplicationPolicyAdmission::unresolved_with_reason)
            return hold(i, *allowed, &policy, &clock, nullptr, &*policy.admission.reason_ref);
        if (policy.admission.disposition != ApplicationPolicyAdmission::allow_active)
            return unavailable(*policy.admission.reason_ref, l_);
        try {
            if (auto refusal = requests(*proposal, i, policy, l_))
                return *refusal;
        } catch (const NumericFailure &f) {
            return hold(i, *allowed, &policy, &clock, &f, nullptr);
        }
        bound(proposal_bytes(*proposal), proposal_items(*proposal), l_);
        auto actions = call(
            [&](const auto &v) { return ports_.actions.prepare(v, c_, i, policy, *proposal); });
        if (auto *u = std::get_if<ApplicationUnavailable>(&actions)) {
            check_unavailable(*u, l_);
            return hold(i, *allowed, &policy, &clock, nullptr, &u->reason_ref);
        }
        auto jobs = std::get<PreparedApplicationActions>(std::move(actions));
        bundle_detail::ByteCount jobbytes;
        jobbytes.add(jobs.explanation.size());
        for (const auto &j : jobs.jobs)
            bundle_detail::count_bytes(jobbytes, j);
        bound(jobbytes.total(), jobs.jobs.size(), l_);
        check(jobs.jobs.size() <= size_t(l_.bundles.jobs_per_bundle), R::result_limit);
        check_jobs(jobs.jobs, *proposal, i);
        bound(bundle_detail::logical_bytes(i.current_state), i.current_state.balances.size(), l_);
        ApplicationStateSnapshot before = i.current_state;
        before.timer = proposal->clock_advanced_timer;
        auto candidate = encode_state(before, i.current_state.token, clock, policy, l_);
        // Full byte/typed equality is required for reuse; annotations are never hashes.
        bool reuse = bundle_detail::same_envelope(candidate, i.current_state.envelope) &&
                     bundle_detail::same_state(before, i.current_state);
        if (!reuse) {
            auto token = issue(ApplicationReferencePurpose::advanced_state, i, std::nullopt);
            if (!token)
                return unavailable("required_application_reference", l_);
            const auto base = bundle_detail::logical_bytes(before) - before.token.size() -
                              bundle_detail::logical_bytes(before.envelope);
            bound(sum(sum(base, token->size()), bundle_detail::logical_bytes(candidate)),
                  before.balances.size(), l_);
            before.token = *token;
            before.envelope = std::move(candidate);
        }
        ApplicationStateSnapshot after = before;
        auto token = issue(ApplicationReferencePurpose::after_state, i, std::nullopt);
        if (!token)
            return unavailable("required_application_reference", l_);
        bound(sum(bundle_detail::logical_bytes(after) - after.token.size(), token->size()),
              after.balances.size(), l_);
        after.token = *token;
        after.timer = proposal->timer_after;
        after.time_debt_ms = proposal->debt_after;
        after.balances = proposal->balances_after;
        auto after_envelope = encode_state(after, i.current_state.token, clock, policy, l_);
        bound(
            sum(bundle_detail::logical_bytes(after) - bundle_detail::logical_bytes(after.envelope),
                bundle_detail::logical_bytes(after_envelope)),
            after.balances.size(), l_);
        after.envelope = std::move(after_envelope);
        if (after.token == before.token || after.token == i.current_state.token ||
            (!reuse && before.token == i.current_state.token))
            return response(D::state_token_conflict);
        const char *advance_explanation = reuse ? "" : "clock-observation-v1";
        auto result_envelope =
            encode_result(cmd_, i.current_state.token, before, after, c_, proposal->effects,
                          jobs.jobs, proposal->transitions, l_);
        auto audit_envelope = encode_audit(cmd_, c_, policy, clock, jobs.explanation, l_);
        bundle_detail::ByteCount prepared;
        using bundle_detail::count_bytes;
        count_bytes(prepared, c_.operation);
        prepared.add(18 + (c_.contribution_id ? 8 : 0));
        count_bytes(prepared, c_.plan.plan_ref);
        count_bytes(prepared, i.current_state.token);
        if (i.current_contribution)
            count_bytes(prepared, i.current_contribution->contribution.state);
        count_bytes(prepared, cmd_);
        count_bytes(prepared, before);
        count_bytes(prepared, after);
        prepared.add(std::char_traits<char>::length(advance_explanation));
        size_t prepared_items = sum(before.balances.size(), after.balances.size());
        prepared_items = sum(prepared_items, sum(sum(proposal->effects.size(), jobs.jobs.size()),
                                                 proposal->transitions.size()));
        for (const auto &effect : proposal->effects) {
            count_bytes(prepared, effect);
            prepared_items = sum(prepared_items, effect.components.size());
        }
        for (const auto &job : jobs.jobs)
            count_bytes(prepared, job);
        prepared.add(proposal->transitions.size());
        count_bytes(prepared, result_envelope);
        count_bytes(prepared, c_.actor_ref);
        count_bytes(prepared, c_.decision_ref);
        count_bytes(prepared, c_.reason);
        count_bytes(prepared, audit_envelope);
        bound(prepared.total(), prepared_items, l_);
        check(prepared.total() <= size_t(l_.bundles.owned_bytes) &&
                  prepared_items <= size_t(l_.bundles.input_items),
              R::result_limit);
        PreparedApplicationBundle bundle;
        bundle.intent = {c_.operation,
                         ApplicationOperationKind::original,
                         c_.subject_kind,
                         c_.receipt_id,
                         c_.contribution_id,
                         c_.plan,
                         i.current_state.token,
                         i.current_contribution
                             ? std::optional{i.current_contribution->contribution.state}
                             : std::nullopt,
                         cmd_,
                         std::nullopt};
        bundle.advanced_before = std::move(before);
        bundle.after = std::move(after);
        bundle.advance_explanation = advance_explanation;
        bundle.effects = std::move(proposal->effects);
        bundle.jobs = std::move(jobs.jobs);
        bundle.transitions = std::move(proposal->transitions);
        bundle.result = std::move(result_envelope);
        bundle.audit = {c_.actor_ref, c_.decision_ref, c_.reason, std::move(audit_envelope)};
        auto staged = bundles_.stage(bundle);
        using B = SqliteBundleStageDisposition;
        switch (staged.disposition) {
        case B::staged: {
            auto record = bundles_.lookup(c_.operation);
            check(record.has_value());
            (void)check_record(*record, l_, i.selected_plan);
            bound(sum(1, record_bytes(*record)), record_items(*record), l_, true);
            return {D::applied, std::move(*record)};
        }
        case B::already_present:
            check(staged.existing.has_value());
            return historical(*staged.existing, D::existing_operation);
        case B::subject_claimed:
            check(staged.existing.has_value());
            return historical(*staged.existing, D::subject_claimed);
        case B::source_claimed:
            check(staged.existing.has_value());
            return historical(*staged.existing, D::source_claimed);
        case B::operation_conflict:
            return response(D::operation_conflict);
        case B::stale_state:
            return response(D::stale_state);
        case B::stale_contribution:
            return response(D::stale_contribution);
        case B::stale_plan:
            return response(D::plan_conflict);
        case B::state_token_conflict:
            return response(D::state_token_conflict);
        case B::subject_unavailable:
            return response(D::subject_unavailable);
        default:
            fail(R::invalid_output);
        }
    }

  private:
    void check_jobs(const std::vector<ApplicationJobIntent> &jobs,
                    const ApplicationCalculatedProposal &p, const ApplicationBundleInputs &i) {
        try {
            bundle_detail::validate_jobs(jobs, p.effects.size(), l_.bundles);
        } catch (const SqliteBundleInputFailure &) {
            fail(R::invalid_output);
        }
        std::int64_t previous = 0;
        for (const auto &j : jobs) {
            check(j.source_request_ordinal && *j.source_request_ordinal > 0 &&
                  size_t(*j.source_request_ordinal) <=
                      i.selected_plan.captured.requested_effects.size());
            check(*j.source_request_ordinal >= previous);
            previous = *j.source_request_ordinal;
            try {
                check_job_meaning(j, p.effects, i.selected_plan, l_);
            } catch (const LocalFailure &failure) {
                if (failure.reason == R::result_limit)
                    throw;
                fail(R::invalid_output);
            }
        }
    }
    SqliteWorkSession session_;
    ApplicationServicePorts &ports_;
    const ApplicationServiceLimits &l_;
    const OriginalApplicationCommand &c_;
    const ApplicationEnvelope &cmd_;
    ViewFactory factory_;
    std::shared_ptr<AttemptState> attempt_;
    SqliteApplicationBundleStore bundles_;
    SqliteApplicationHoldStore holds_;
};
bool normal_payload(const ApplicationServiceResponse &r) {
    switch (r.disposition) {
    case D::applied:
    case D::existing_operation:
    case D::subject_claimed:
    case D::source_claimed:
        return std::holds_alternative<RecordedApplicationBundle>(r.payload);
    case D::ineligible:
        return std::holds_alternative<std::vector<core::ContributionBlocker>>(r.payload);
    case D::active_hold:
        return std::holds_alternative<ApplicationActiveHoldPayload>(r.payload);
    case D::held_numeric:
    case D::held_policy:
        return std::holds_alternative<ApplicationHeldPayload>(r.payload);
    case D::not_ready:
        return std::holds_alternative<ApplicationNotReadyPayload>(r.payload);
    case D::stage_conflict:
        return std::holds_alternative<ApplicationStageConflict>(r.payload);
    case D::operation_conflict:
    case D::denied:
    case D::disclose_denied:
    case D::missing_plan:
    case D::plan_conflict:
    case D::state_conflict:
    case D::stale_contribution:
    case D::stale_state:
    case D::state_token_conflict:
    case D::subject_unavailable:
        return std::holds_alternative<std::monostate>(r.payload);
    }
    return false;
}
} // namespace
OriginalApplicationService::OriginalApplicationService(SqliteWriter &writer,
                                                       ApplicationServicePorts &ports,
                                                       ApplicationServiceLimits limits)
    : writer_(writer), ports_(ports), limits_(std::move(limits)) {}
ApplicationServicePreparedWork
OriginalApplicationService::make_work(const OriginalApplicationCommand &command,
                                      Completion completion) const {
    if (QThread::currentThread() != writer_.thread())
        throw ApplicationServicePreparationFailure(StorageError::wrong_thread);
    if (!completion)
        throw ApplicationServicePreparationFailure(StorageError::invalid_work);
    if (limits_.result_owned_bytes <= 0 ||
        limits_.result_owned_bytes >= std::numeric_limits<qsizetype>::max())
        throw ApplicationServiceInputFailure(ApplicationServiceInputError::invalid_options);
    auto preflight = limits_;
    preflight.result_owned_bytes = std::numeric_limits<qsizetype>::max() - 1;
    auto encoded = encode_original_application_command(command, preflight);
    auto owned = command;
    for (auto &choice : owned.choices)
        choice.document = QByteArray(choice.document.constData(), choice.document.size());
    auto attempt = std::make_shared<AttemptState>();
    auto factory = [](std::shared_ptr<detail::ApplicationReferenceReadState> state) {
        ApplicationReferenceReadView view;
        view.state_ = state;
        view.limits_ = state->limits;
        return view;
    };
    auto body = [this, c = std::move(owned), encoded = std::move(encoded), attempt,
                 factory](SqliteWorkContext context) -> QVariant {
        auto session = context.begin();
        try {
            Execute execute(session, ports_, limits_, c, encoded, factory, attempt);
            ApplicationServiceResponse r;
            try {
                r = execute.run();
            } catch (const ApplicationServiceUnsupportedFormat &) {
                r = unavailable("unsupported_retained_format", limits_);
            }
            final_bounds(r, limits_);
            session.commit();
            return QVariant::fromValue(std::move(r));
        } catch (const LocalFailure &e) {
            attempt->failure = ApplicationServiceFailure{e.source, e.reason};
            auto cleanup = session.cleanup();
            if (cleanup != WorkError::none)
                throw SqliteWorkFailure(cleanup);
            throw;
        } catch (const SqliteBundleStorageFailure &e) {
            attempt->failure = ApplicationServiceFailure{S::bundle, e.reason()};
            throw;
        } catch (const SqliteHoldStorageFailure &e) {
            attempt->failure = ApplicationServiceFailure{S::hold, e.reason()};
            throw;
        } catch (const RetentionFailure &e) {
            attempt->failure = ApplicationServiceFailure{S::retained, e.error()};
            if (e.error() != RetentionError::invalid_stored_data &&
                e.error() != RetentionError::result_limit) {
                auto cleanup = session.cleanup();
                if (cleanup != WorkError::none)
                    throw SqliteWorkFailure(cleanup);
            }
            throw;
        } catch (const ApplicationReferenceViewFailure &) {
            attempt->failure = ApplicationServiceFailure{S::port_contract, R::invalid_output};
            auto cleanup = session.cleanup();
            if (cleanup != WorkError::none)
                throw SqliteWorkFailure(cleanup);
            throw;
        } catch (const SqliteWorkFailure &) {
            throw;
        } catch (const SqliteBundleInputFailure &) {
            attempt->failure = ApplicationServiceFailure{S::port_contract, R::invalid_output};
            auto cleanup = session.cleanup();
            if (cleanup != WorkError::none)
                throw SqliteWorkFailure(cleanup);
            throw;
        } catch (...) {
            auto cleanup = session.cleanup();
            if (cleanup != WorkError::none)
                throw SqliteWorkFailure(cleanup);
            throw;
        }
    };
    auto done = [completion = std::move(completion), attempt](SqliteWorkResult actual) {
        ApplicationServiceCompletion out;
        out.transaction = actual.transaction;
        out.work_error = actual.error;
        if (actual.transaction != TransactionOutcome::outcome_unknown)
            out.failure = attempt->failure;
        if (actual.transaction == TransactionOutcome::committed &&
            actual.error == WorkError::none) {
            if (actual.value &&
                actual.value->metaType() == QMetaType::fromType<ApplicationServiceResponse>()) {
                const auto *r =
                    static_cast<const ApplicationServiceResponse *>(actual.value->constData());
                if (r && normal_payload(*r))
                    out.response = *r;
                else
                    out.failure =
                        ApplicationServiceFailure{S::bridge, R::invalid_completion_payload};
            } else
                out.failure = ApplicationServiceFailure{S::bridge, R::invalid_completion_payload};
        }
        completion(std::move(out));
    };
    return {std::move(body), std::move(done)};
}
ApplicationServiceAdmission
OriginalApplicationService::enqueue(const OriginalApplicationCommand &command,
                                    Completion completion) {
    if (QThread::currentThread() != writer_.thread())
        return {StorageError::wrong_thread};
    if (!completion)
        return {StorageError::invalid_work};
    auto p = make_work(command, std::move(completion));
    return {writer_.enqueue_work(std::move(p.body), std::move(p.completion))};
}
} // namespace subathon::server
