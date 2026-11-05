// SPDX-License-Identifier: GPL-3.0-or-later
#include "reference_capture_codec_p.hpp"
#include <array>
#include <bit>
#include <cstddef>
#include <limits>
#include <new>
#include <string_view>
#include <subathon/server/reference_capture_values.hpp>
#include <type_traits>

namespace subathon::server {
namespace {
using Error = ReferenceInputError;
using Limits = ReferenceAdapterLimits;
constexpr std::string_view configuration_format = "subathon.reference.configuration.v1";
constexpr std::string_view arrival_format = "subathon.reference.arrival.v1";
constexpr std::string_view execution_format = "subathon.reference.execution.v1";
constexpr std::string_view initial_format = "subathon.reference.initial-capture.v1";
constexpr auto maximum_frame = std::numeric_limits<std::uint32_t>::max();
[[noreturn]] void fail(Error error) { throw ReferenceInputFailure(error); }
template <class Body> auto allocation_boundary(Body body) {
    try {
        return body();
    } catch (const std::bad_alloc &) {
        fail(Error::allocation_failure);
    } catch (const std::length_error &) {
        fail(Error::allocation_failure);
    }
}
void validate_limits(const Limits &l) {
    const auto positive = [](std::uint64_t n) {
        return n > 0 && n < std::uintmax_t(std::numeric_limits<std::size_t>::max()) &&
               n < std::uintmax_t(std::numeric_limits<std::ptrdiff_t>::max()) &&
               n < std::uintmax_t(std::numeric_limits<qsizetype>::max()) &&
               n < std::uintmax_t(std::numeric_limits<qint64>::max());
    };
    for (auto n : {l.reference_bytes, l.payload_bytes, l.document_bytes, l.value_owned_bytes,
                   l.value_items, l.clock_journal_entries})
        if (!positive(n))
            fail(Error::invalid_limits);
    // The configured per-container ceilings reserve n+1 before any ownership.
    if (l.reference_bytes >= std::string{}.max_size() ||
        l.payload_bytes >= std::uintmax_t(QByteArray::maxSize()) ||
        l.document_bytes >= std::uintmax_t(QByteArray::maxSize()))
        fail(Error::invalid_limits);
}
class Size final {
    std::uint64_t limit_, used_{0};
    Error error_;

  public:
    Size(std::uint64_t limit, Error error) : limit_(limit), error_(error) {}
    void add(std::uint64_t n) {
        if (n > limit_ - used_)
            fail(error_);
        used_ += n;
    }
    [[nodiscard]] std::uint64_t get() const { return used_; }
};
// Fixed owned-free borrowed views. Wide C++ enums remain wide until tag validation.
// Largest payload is ArrivalView; no dynamic tree, vector or span-index array.
struct RuleView {
    std::string_view rule, version, action, action_version, timer, installation, principal,
        capability;
    std::int64_t add{0}, kind{0};
};
struct ConfigurationView {
    std::string_view configuration, policy, provider, account, receipt_namespace;
    RuleView rule;
    std::optional<std::int64_t> cap, hard_end;
    std::int64_t end_mode{0}, grace{0}, reuse{0};
    std::array<std::uint8_t, 5> facts{};
};
struct ArrivalView {
    std::string_view context, event, connection, event_id, payload;
    ConfigurationView configuration;
    std::int64_t utc{0};
    std::optional<std::int64_t> source_utc;
    bool null_payload{false};
};
struct ExecutionView {
    std::string_view context, arrival, installation, provider, account, receipt_namespace, opaque,
        plan;
    RuleView rule;
    std::uint64_t receipt{0}, contribution{0}, revision{0};
};
struct InitialView {
    std::string_view installation, provider, account, receipt_namespace, opaque, configuration,
        arrival, event, plan, execution;
    std::uint64_t receipt{0}, contribution{0}, revision{0};
};
// Arrival: 18 actual B fields plus its borrowed embedded-SRC1 frame =19 records.
// Fixed parser stack is Reader→embedded Reader→rule/config, depth at most3.
static_assert(sizeof(ConfigurationView) <= 512 && sizeof(ArrivalView) <= 512 &&
              sizeof(ExecutionView) <= 512 && sizeof(InitialView) <= 512);
static_assert(19 <= 64);
template <class E> std::int64_t raw_enum(E value) {
    return static_cast<std::underlying_type_t<E>>(value);
}
void borrow(const ReferenceRule &v, RuleView &out) {
    out = {v.rule_ref,
           v.rule_version_ref,
           v.action_ref,
           v.action_version_ref,
           v.timer_ref,
           v.automation.installation_ref,
           v.automation.principal_ref,
           v.automation_capability_ref,
           v.add_ms,
           raw_enum(v.automation.kind)};
}
void borrow(const ReferenceConfiguration &v, ConfigurationView &out) {
    out.configuration = v.configuration_ref;
    out.policy = v.policy_revision_ref;
    out.provider = v.synthetic_origin.provider;
    out.account = v.synthetic_origin.source_account;
    out.receipt_namespace = v.receipt_namespace;
    borrow(v.rule, out.rule);
    out.cap = v.maximum_main_ms;
    out.hard_end = v.timer_config.hard_end_utc_ms;
    out.end_mode = raw_enum(v.timer_config.end_mode);
    out.grace = v.timer_config.grace_duration_ms;
    out.reuse = raw_enum(v.timer_config.grace_reuse);
    out.facts = {std::uint8_t(v.other_rules), std::uint8_t(v.modifiers), std::uint8_t(v.milestones),
                 std::uint8_t(v.outbound_actions), std::uint8_t(v.generated_events)};
}
void borrow(const ReferenceArrivalCapture &v, ArrivalView &out) {
    out.context = v.arrival_context_ref;
    out.event = v.event_ref;
    out.utc = v.server_arrival_utc_ms;
    borrow(v.configuration, out.configuration);
    out.connection = v.observation.connection_ref;
    out.event_id = v.observation.event_id;
    out.source_utc = v.observation.source_utc_ms;
    out.payload = {v.observation.original_payload.constData(),
                   std::size_t(v.observation.original_payload.size())};
    out.null_payload = v.observation.original_payload.isNull();
}
void borrow(const ReferenceExecutionBinding &v, ExecutionView &out) {
    out.context = v.execution_context_ref;
    out.arrival = v.arrival_context_ref;
    out.installation = v.installation_ref;
    out.provider = v.source.origin.provider;
    out.account = v.source.origin.source_account;
    out.receipt_namespace = v.source.identifier_namespace;
    out.opaque = v.source.opaque_id;
    out.receipt = v.receipt_id.value;
    out.contribution = v.contribution_id.value;
    out.plan = v.plan.plan_ref;
    out.revision = v.plan.revision;
    borrow(v.rule, out.rule);
}
void borrow(const ReferenceInitialCapture &v, InitialView &out) {
    out.installation = v.key.installation_ref;
    out.provider = v.key.source.origin.provider;
    out.account = v.key.source.origin.source_account;
    out.receipt_namespace = v.key.source.identifier_namespace;
    out.opaque = v.key.source.opaque_id;
    out.receipt = v.receipt_id.value;
    out.contribution = v.contribution_id.value;
    out.configuration = v.configuration_ref;
    out.arrival = v.arrival_context_ref;
    out.event = v.event_ref;
    out.plan = v.plan.plan_ref;
    out.revision = v.plan.revision;
    out.execution = v.execution_context_ref;
}
template <class F> void references(const RuleView &v, F &f) {
    f(v.rule);
    f(v.version);
    f(v.action);
    f(v.action_version);
    f(v.timer);
    f(v.installation);
    f(v.principal);
    f(v.capability);
}
template <class F> void references(const ConfigurationView &v, F &f) {
    f(v.configuration);
    f(v.policy);
    f(v.provider);
    f(v.account);
    f(v.receipt_namespace);
    references(v.rule, f);
}
template <class F> void references(const ArrivalView &v, F &f) {
    f(v.context);
    f(v.event);
    f(v.connection);
    f(v.event_id);
    references(v.configuration, f);
}
template <class F> void references(const ExecutionView &v, F &f) {
    f(v.context);
    f(v.arrival);
    f(v.installation);
    f(v.provider);
    f(v.account);
    f(v.receipt_namespace);
    f(v.opaque);
    f(v.plan);
    references(v.rule, f);
}
template <class F> void references(const InitialView &v, F &f) {
    f(v.installation);
    f(v.provider);
    f(v.account);
    f(v.receipt_namespace);
    f(v.opaque);
    f(v.configuration);
    f(v.arrival);
    f(v.event);
    f(v.plan);
    f(v.execution);
}
std::uint64_t scalars(const ConfigurationView &v) {
    return 26 + (v.cap ? 8 : 0) + (v.hard_end ? 8 : 0);
}
std::uint64_t scalars(const ArrivalView &v) {
    return scalars(v.configuration) + 9 + (v.source_utc ? 8 : 0);
}
std::uint64_t scalars(const ExecutionView &) { return 33; }
std::uint64_t scalars(const InitialView &) { return 24; }
void extra_bounds(const ConfigurationView &, const Limits &, Size &, Error) {}
void extra_bounds(const ExecutionView &, const Limits &, Size &, Error) {}
void extra_bounds(const InitialView &, const Limits &, Size &, Error) {}
void extra_bounds(const ArrivalView &v, const Limits &l, Size &size, Error error) {
    if (v.payload.size() > l.payload_bytes || v.payload.size() > maximum_frame ||
        v.payload.size() >= std::uintmax_t(QByteArray::maxSize()))
        fail(error);
    size.add(v.payload.size());
}
bool tag(std::int64_t value, std::int64_t maximum) { return value >= 0 && value <= maximum; }
void tags(const RuleView &v) {
    if (!tag(v.kind, 1))
        fail(Error::invalid_tag);
}
void tags(const ConfigurationView &v) {
    tags(v.rule);
    if (!tag(v.end_mode, 2) || !tag(v.reuse, 1))
        fail(Error::invalid_tag);
    for (auto f : v.facts)
        if (f > 1)
            fail(Error::invalid_tag);
}
void tags(const ArrivalView &v) { tags(v.configuration); }
void tags(const ExecutionView &v) { tags(v.rule); }
void tags(const InitialView &) {}
void shape(const ConfigurationView &) {}
void shape(const ExecutionView &) {}
void shape(const InitialView &) {}
void shape(const ArrivalView &v) {
    if (v.null_payload)
        fail(Error::invalid_shape);
}
void values(const RuleView &v) {
    if (v.add < 0)
        fail(Error::invalid_value);
}
void values(const ConfigurationView &v) {
    values(v.rule);
    if ((v.cap && *v.cap < 0) || v.grace < 0)
        fail(Error::invalid_value);
}
void values(const ArrivalView &v) { values(v.configuration); }
void values(const ExecutionView &v) {
    values(v.rule);
    if (v.receipt == 0 || v.revision == 0)
        fail(Error::invalid_value);
}
void values(const InitialView &v) {
    if (v.receipt == 0 || v.revision != 1)
        fail(Error::invalid_value);
}
void bindings(const RuleView &v) {
    if (v.kind != 1)
        fail(Error::invalid_binding);
}
void bindings(const ConfigurationView &v) { bindings(v.rule); }
void bindings(const ArrivalView &v) { bindings(v.configuration); }
void bindings(const ExecutionView &v) {
    bindings(v.rule);
    if (v.installation != v.rule.installation)
        fail(Error::invalid_binding);
}
void bindings(const InitialView &) {}
template <class V> void validate(const V &v, const Limits &l, Error bound_error) {
    // Entire borrowed DTO and every field precede ALL semantic checks/ownership.
    Size bytes(l.value_owned_bytes, bound_error);
    auto field = [&](std::string_view s) {
        if (s.size() > l.reference_bytes || s.size() > maximum_frame ||
            s.size() >= std::string{}.max_size())
            fail(bound_error);
        bytes.add(s.size());
    };
    references(v, field);
    bytes.add(scalars(v));
    extra_bounds(v, l, bytes, bound_error);
    tags(v);
    auto required = [](std::string_view s) {
        if (s.empty())
            fail(Error::invalid_reference);
    };
    references(v, required);
    shape(v);
    values(v);
    bindings(v);
}
class Reader final {
    std::string_view bytes_;
    std::size_t position_{0};

  public:
    explicit Reader(std::string_view bytes) : bytes_(bytes) {}
    std::string_view bytes(std::size_t n) {
        if (n > bytes_.size() - position_)
            fail(Error::invalid_document);
        const auto result = bytes_.substr(position_, n);
        position_ += n;
        return result;
    }
    std::uint64_t integer(std::size_t n) {
        std::uint64_t result = 0;
        for (unsigned char b : bytes(n))
            result = (result << 8) | b;
        return result;
    }
    std::uint8_t u8() { return std::uint8_t(integer(1)); }
    std::int64_t signed64() { return std::bit_cast<std::int64_t>(integer(8)); }
    std::string_view blob() { return bytes(std::size_t(integer(4))); }
    std::optional<std::int64_t> optional_signed() {
        const auto present = u8();
        if (present > 1)
            fail(Error::invalid_document);
        return present ? std::optional<std::int64_t>{signed64()} : std::nullopt;
    }
    void magic(std::string_view expected) {
        if (bytes(4) != expected)
            fail(Error::invalid_document);
    }
    void finish() const {
        if (position_ != bytes_.size())
            fail(Error::invalid_document);
    }
};
void scan_rule(Reader &r, RuleView &v) {
    v.rule = r.blob();
    v.version = r.blob();
    v.action = r.blob();
    v.action_version = r.blob();
    v.timer = r.blob();
    v.add = r.signed64();
    v.kind = r.u8();
    v.installation = r.blob();
    v.principal = r.blob();
    v.capability = r.blob();
}
void scan(Reader &r, ConfigurationView &v) {
    r.magic("SRC1");
    v.configuration = r.blob();
    v.policy = r.blob();
    v.provider = r.blob();
    v.account = r.blob();
    v.receipt_namespace = r.blob();
    scan_rule(r, v.rule);
    v.cap = r.optional_signed();
    v.end_mode = r.u8();
    v.grace = r.signed64();
    v.reuse = r.u8();
    v.hard_end = r.optional_signed();
    for (auto &f : v.facts)
        f = r.u8();
    r.finish();
}
void scan(Reader &r, ArrivalView &v) {
    r.magic("SRA1");
    v.context = r.blob();
    v.event = r.blob();
    v.utc = r.signed64();
    Reader embedded(r.blob());
    scan(embedded, v.configuration);
    v.connection = r.blob();
    v.event_id = r.blob();
    v.source_utc = r.optional_signed();
    v.payload = r.blob();
    r.finish();
}
void scan(Reader &r, ExecutionView &v) {
    r.magic("SRX1");
    v.context = r.blob();
    v.arrival = r.blob();
    v.installation = r.blob();
    v.provider = r.blob();
    v.account = r.blob();
    v.receipt_namespace = r.blob();
    v.opaque = r.blob();
    v.receipt = r.integer(8);
    v.contribution = r.integer(8);
    v.plan = r.blob();
    v.revision = r.integer(8);
    scan_rule(r, v.rule);
    r.finish();
}
void scan(Reader &r, InitialView &v) {
    r.magic("SRI1");
    v.installation = r.blob();
    v.provider = r.blob();
    v.account = r.blob();
    v.receipt_namespace = r.blob();
    v.opaque = r.blob();
    v.receipt = r.integer(8);
    v.contribution = r.integer(8);
    v.configuration = r.blob();
    v.arrival = r.blob();
    v.event = r.blob();
    v.plan = r.blob();
    v.revision = r.integer(8);
    v.execution = r.blob();
    r.finish();
}
class Sizing final {
    Size bytes_;

  public:
    explicit Sizing(std::uint64_t limit) : bytes_(limit, Error::result_limit) {}
    void bytes(std::string_view b) { bytes_.add(b.size()); }
    void integer(std::uint64_t, std::size_t n) { bytes_.add(n); }
    [[nodiscard]] std::uint64_t get() const { return bytes_.get(); }
};
class Writer final {
    QByteArray bytes_;

  public:
    explicit Writer(std::uint64_t n) { bytes_.reserve(qsizetype(n)); }
    void bytes(std::string_view b) { bytes_.append(b.data(), qsizetype(b.size())); }
    void integer(std::uint64_t value, std::size_t n) {
        for (std::size_t i = n; i > 0; --i)
            bytes_.append(char((value >> ((i - 1) * 8)) & 0xff));
    }
    [[nodiscard]] QByteArray finish() && { return std::move(bytes_); }
};
template <class Sink> void blob(Sink &s, std::string_view b) {
    s.integer(b.size(), 4);
    s.bytes(b);
}
template <class Sink> void signed64(Sink &s, std::int64_t value) {
    s.integer(std::bit_cast<std::uint64_t>(value), 8);
}
template <class Sink> void optional_signed(Sink &s, const std::optional<std::int64_t> &v) {
    s.integer(bool(v), 1);
    if (v)
        signed64(s, *v);
}
template <class Sink> void emit_rule(Sink &s, const RuleView &v) {
    blob(s, v.rule);
    blob(s, v.version);
    blob(s, v.action);
    blob(s, v.action_version);
    blob(s, v.timer);
    signed64(s, v.add);
    s.integer(std::uint64_t(v.kind), 1);
    blob(s, v.installation);
    blob(s, v.principal);
    blob(s, v.capability);
}
template <class Sink> void write_document(Sink &s, const ConfigurationView &v, const Limits &) {
    s.bytes("SRC1");
    blob(s, v.configuration);
    blob(s, v.policy);
    blob(s, v.provider);
    blob(s, v.account);
    blob(s, v.receipt_namespace);
    emit_rule(s, v.rule);
    optional_signed(s, v.cap);
    s.integer(std::uint64_t(v.end_mode), 1);
    signed64(s, v.grace);
    s.integer(std::uint64_t(v.reuse), 1);
    optional_signed(s, v.hard_end);
    for (auto f : v.facts)
        s.integer(f, 1);
}
template <class Sink> void write_document(Sink &s, const ArrivalView &v, const Limits &l) {
    s.bytes("SRA1");
    blob(s, v.context);
    blob(s, v.event);
    signed64(s, v.utc);
    Sizing embedded(l.document_bytes);
    write_document(embedded, v.configuration, l);
    if (embedded.get() > maximum_frame)
        fail(Error::result_limit);
    s.integer(embedded.get(), 4);
    write_document(s, v.configuration, l);
    blob(s, v.connection);
    blob(s, v.event_id);
    optional_signed(s, v.source_utc);
    blob(s, v.payload);
}
template <class Sink> void write_document(Sink &s, const ExecutionView &v, const Limits &) {
    s.bytes("SRX1");
    blob(s, v.context);
    blob(s, v.arrival);
    blob(s, v.installation);
    blob(s, v.provider);
    blob(s, v.account);
    blob(s, v.receipt_namespace);
    blob(s, v.opaque);
    s.integer(v.receipt, 8);
    s.integer(v.contribution, 8);
    blob(s, v.plan);
    s.integer(v.revision, 8);
    emit_rule(s, v.rule);
}
template <class Sink> void write_document(Sink &s, const InitialView &v, const Limits &) {
    s.bytes("SRI1");
    blob(s, v.installation);
    blob(s, v.provider);
    blob(s, v.account);
    blob(s, v.receipt_namespace);
    blob(s, v.opaque);
    s.integer(v.receipt, 8);
    s.integer(v.contribution, 8);
    blob(s, v.configuration);
    blob(s, v.arrival);
    blob(s, v.event);
    blob(s, v.plan);
    s.integer(v.revision, 8);
    blob(s, v.execution);
}
ReferenceRule own(const RuleView &v) {
    return {
        std::string(v.rule),
        std::string(v.version),
        std::string(v.action),
        std::string(v.action_version),
        std::string(v.timer),
        v.add,
        {ApplicationPrincipalKind(v.kind), std::string(v.installation), std::string(v.principal)},
        std::string(v.capability)};
}
ReferenceConfiguration own(const ConfigurationView &v) {
    return {std::string(v.configuration),
            std::string(v.policy),
            {std::string(v.provider), std::string(v.account)},
            std::string(v.receipt_namespace),
            own(v.rule),
            v.cap,
            {core::TimerEndMode(v.end_mode), v.grace, core::GraceReuse(v.reuse), v.hard_end},
            ReferenceFactsState(v.facts[0]),
            ReferenceFactsState(v.facts[1]),
            ReferenceFactsState(v.facts[2]),
            ReferenceFactsState(v.facts[3]),
            ReferenceFactsState(v.facts[4])};
}
ReferenceArrivalCapture own(const ArrivalView &v) {
    auto payload = v.payload.empty() ? QByteArray("")
                                     : QByteArray(v.payload.data(), qsizetype(v.payload.size()));
    return {std::string(v.context),
            std::string(v.event),
            v.utc,
            own(v.configuration),
            {std::string(v.connection), std::string(v.event_id), v.source_utc, std::move(payload)}};
}
ReferenceExecutionBinding own(const ExecutionView &v) {
    return {std::string(v.context),
            std::string(v.arrival),
            std::string(v.installation),
            {{std::string(v.provider), std::string(v.account)},
             std::string(v.receipt_namespace),
             std::string(v.opaque)},
            {v.receipt},
            {v.contribution},
            {std::string(v.plan), v.revision},
            own(v.rule)};
}
ReferenceInitialCapture own(const InitialView &v) {
    return {{std::string(v.installation),
             {{std::string(v.provider), std::string(v.account)},
              std::string(v.receipt_namespace),
              std::string(v.opaque)}},
            {v.receipt},
            {v.contribution},
            std::string(v.configuration),
            std::string(v.arrival),
            std::string(v.event),
            {std::string(v.plan), v.revision},
            std::string(v.execution)};
}
template <class View, class DTO>
ApplicationEnvelope encode(const DTO &value, const Limits &l, std::string_view format) {
    return allocation_boundary([&] {
        validate_limits(l);
        View borrowed;
        borrow(value, borrowed);
        validate(borrowed, l, Error::input_limit);
        Sizing projected(l.document_bytes);
        write_document(projected, borrowed, l);
        if (format.size() > l.reference_bytes || format.size() >= std::string{}.max_size() ||
            projected.get() >= std::uintmax_t(QByteArray::maxSize()))
            fail(Error::result_limit);
        Size complete(l.value_owned_bytes, Error::result_limit);
        complete.add(format.size());
        complete.add(projected.get());
        Writer writer(projected.get());
        write_document(writer, borrowed, l);
        return ApplicationEnvelope{std::string(format), std::move(writer).finish()};
    });
}
std::string_view outer(const ApplicationEnvelope &v, const Limits &l, std::string_view format) {
    validate_limits(l);
    if (v.format_ref.size() > l.reference_bytes || v.document.size() < 0 ||
        std::uintmax_t(v.document.size()) > l.document_bytes ||
        v.format_ref.size() >= std::string{}.max_size() ||
        std::uintmax_t(v.document.size()) >= std::uintmax_t(QByteArray::maxSize()))
        fail(Error::input_limit);
    Size input(l.value_owned_bytes, Error::input_limit);
    input.add(v.format_ref.size());
    input.add(std::uint64_t(v.document.size()));
    if (v.format_ref.empty())
        fail(Error::invalid_reference);
    if (v.document.isNull())
        fail(Error::invalid_shape);
    if (v.format_ref != format)
        fail(Error::unsupported_format);
    return {v.document.constData(), std::size_t(v.document.size())};
}
template <class View>
auto decode_body(std::string_view bytes, const Limits &l,
                 reference_capture_detail::BodyBounds mode) {
    return allocation_boundary([&] {
        validate_limits(l);
        const auto bounds = mode == reference_capture_detail::BodyBounds::retained
                                ? Error::result_limit
                                : Error::input_limit;
        if (bytes.size() > l.document_bytes || bytes.size() > l.value_owned_bytes ||
            bytes.size() >= std::uintmax_t(QByteArray::maxSize()))
            fail(bounds);
        View borrowed;
        Reader reader(bytes);
        scan(reader, borrowed);
        validate(borrowed, l, bounds);
        return own(borrowed);
    });
}
} // namespace
namespace reference_capture_detail {
ReferenceConfiguration decode_configuration_body(std::string_view v, const Limits &l,
                                                 BodyBounds b) {
    return decode_body<ConfigurationView>(v, l, b);
}
ReferenceArrivalCapture decode_arrival_body(std::string_view v, const Limits &l, BodyBounds b) {
    return decode_body<ArrivalView>(v, l, b);
}
ReferenceExecutionBinding decode_execution_body(std::string_view v, const Limits &l, BodyBounds b) {
    return decode_body<ExecutionView>(v, l, b);
}
ReferenceInitialCapture decode_initial_body(std::string_view v, const Limits &l, BodyBounds b) {
    return decode_body<InitialView>(v, l, b);
}
} // namespace reference_capture_detail
ApplicationEnvelope encode_reference_configuration(const ReferenceConfiguration &v,
                                                   const Limits &l) {
    return encode<ConfigurationView>(v, l, configuration_format);
}
ReferenceConfiguration decode_reference_configuration(const ApplicationEnvelope &v,
                                                      const Limits &l) {
    return allocation_boundary([&] {
        return reference_capture_detail::decode_configuration_body(
            outer(v, l, configuration_format), l,
            reference_capture_detail::BodyBounds::public_input);
    });
}
ApplicationEnvelope encode_reference_arrival(const ReferenceArrivalCapture &v, const Limits &l) {
    return encode<ArrivalView>(v, l, arrival_format);
}
ReferenceArrivalCapture decode_reference_arrival(const ApplicationEnvelope &v, const Limits &l) {
    return allocation_boundary([&] {
        return reference_capture_detail::decode_arrival_body(
            outer(v, l, arrival_format), l, reference_capture_detail::BodyBounds::public_input);
    });
}
ApplicationEnvelope encode_reference_execution(const ReferenceExecutionBinding &v,
                                               const Limits &l) {
    return encode<ExecutionView>(v, l, execution_format);
}
ReferenceExecutionBinding decode_reference_execution(const ApplicationEnvelope &v,
                                                     const Limits &l) {
    return allocation_boundary([&] {
        return reference_capture_detail::decode_execution_body(
            outer(v, l, execution_format), l, reference_capture_detail::BodyBounds::public_input);
    });
}
ApplicationEnvelope encode_reference_initial_capture(const ReferenceInitialCapture &v,
                                                     const Limits &l) {
    return encode<InitialView>(v, l, initial_format);
}
ReferenceInitialCapture decode_reference_initial_capture(const ApplicationEnvelope &v,
                                                         const Limits &l) {
    return allocation_boundary([&] {
        return reference_capture_detail::decode_initial_body(
            outer(v, l, initial_format), l, reference_capture_detail::BodyBounds::public_input);
    });
}
} // namespace subathon::server
