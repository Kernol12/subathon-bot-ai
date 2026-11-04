// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/application_service.hpp>

#include "sqlite_application_hold_values.hpp"
#include <limits>
#include <string_view>

namespace subathon::server {
namespace {
using Error = ApplicationServiceInputError;
constexpr std::string_view command_format = "subathon.internal.application-command.v1";
[[noreturn]] void invalid(Error e) { throw ApplicationServiceInputFailure(e); }

// A scan stores byte views and scalar fields only. No caller-controlled string or
// collection is allocated until all framing, typed and ownership bounds pass.
struct CommandView {
    std::string_view scope, operation, plan, installation, principal;
    std::uint8_t subject{0}, principal_kind{0}, purpose{0}, preview_mode{0};
    std::uint64_t receipt{0}, revision{0};
    std::optional<std::uint64_t> contribution;
    std::optional<std::string_view> preview, actor, decision, reason;
    std::size_t choice_count{0};
};
struct ChoiceView {
    std::string_view format, document;
    bool null_document{false};
};
std::optional<std::string_view> view(const std::optional<std::string> &v) {
    return v ? std::optional<std::string_view>{*v} : std::nullopt;
}
CommandView view(const OriginalApplicationCommand &c) {
    CommandView v;
    v.scope = c.operation.operation_scope;
    v.operation = c.operation.operation_ref;
    v.subject = static_cast<std::uint8_t>(c.subject_kind);
    v.receipt = c.receipt_id.value;
    if (c.contribution_id)
        v.contribution = c.contribution_id->value;
    v.plan = c.plan.plan_ref;
    v.revision = c.plan.revision;
    v.principal_kind = static_cast<std::uint8_t>(c.principal.kind);
    v.installation = c.principal.installation_ref;
    v.principal = c.principal.principal_ref;
    v.purpose = static_cast<std::uint8_t>(c.purpose);
    v.preview_mode = static_cast<std::uint8_t>(c.preview_mode);
    v.preview = view(c.preview_state_token);
    v.actor = view(c.actor_ref);
    v.decision = view(c.decision_ref);
    v.reason = view(c.reason);
    v.choice_count = c.choices.size();
    return v;
}
void validate_options(const ApplicationServiceLimits &l) {
    try {
        bundle_detail::validate_options(l.bundles);
        hold_detail::validate_options(l.holds);
    } catch (const SqliteBundleInputFailure &) {
        invalid(Error::invalid_options);
    } catch (const SqliteHoldInputFailure &) {
        invalid(Error::invalid_options);
    }
    const auto positive = [](qsizetype n) {
        return n > 0 && std::uintmax_t(n) < std::uintmax_t(std::numeric_limits<qsizetype>::max()) &&
               std::uintmax_t(n) < std::uintmax_t(std::numeric_limits<qint64>::max()) &&
               std::uintmax_t(n) < std::uintmax_t(std::numeric_limits<std::size_t>::max());
    };
    for (const auto n : {l.reference_bytes,
                         l.choice_bytes,
                         l.command_bytes,
                         l.storage_document_bytes,
                         l.choices_per_command,
                         l.clock_segments,
                         l.input_owned_bytes,
                         l.result_owned_bytes,
                         l.result_items,
                         l.intermediate_owned_bytes,
                         l.intermediate_items,
                         l.bundles.reference_bytes,
                         l.bundles.value_bytes,
                         l.bundles.component_bytes,
                         l.bundles.owned_bytes,
                         l.holds.references.reference_bytes,
                         l.holds.references.value_bytes,
                         l.holds.references.component_bytes,
                         l.holds.references.owned_bytes,
                         l.holds.owned_bytes})
        if (!positive(n))
            invalid(Error::invalid_options);
    if (std::uintmax_t(l.choices_per_command) >= std::vector<ApplicationEnvelope>{}.max_size() ||
        std::uintmax_t(l.clock_segments) >= std::vector<core::TimerClockStep>{}.max_size())
        invalid(Error::invalid_options);
}
class Size final {
  public:
    explicit Size(std::size_t limit, Error error) : limit_(limit), error_(error) {}
    void add(std::size_t amount) {
        if (amount > limit_ - size_)
            invalid(error_);
        size_ += amount;
    }
    [[nodiscard]] std::size_t get() const { return size_; }

  private:
    std::size_t limit_, size_{0};
    Error error_;
};
class Reader final {
  public:
    explicit Reader(std::string_view data) : data_(data) {}
    [[nodiscard]] std::size_t remaining() const { return data_.size() - offset_; }
    [[nodiscard]] std::size_t position() const { return offset_; }
    std::string_view bytes(std::size_t n) {
        if (n > remaining())
            invalid(Error::invalid_body);
        const auto value = data_.substr(offset_, n);
        offset_ += n;
        return value;
    }
    std::uint8_t u8() { return static_cast<std::uint8_t>(bytes(1)[0]); }
    std::uint64_t unsigned_be(std::size_t n) {
        std::uint64_t value = 0;
        for (unsigned char b : bytes(n))
            value = (value << 8) | b;
        return value;
    }
    std::string_view blob() { return bytes(static_cast<std::size_t>(unsigned_be(4))); }
    bool present() {
        const auto tag = u8();
        if (tag > 1)
            invalid(Error::invalid_body);
        return tag == 1;
    }
    std::optional<std::string_view> optional_blob() {
        return present() ? std::optional<std::string_view>{blob()} : std::nullopt;
    }

  private:
    std::string_view data_;
    std::size_t offset_{0};
};
struct ScannedCommand {
    CommandView command;
    std::string_view choices;
};
ScannedCommand scan(std::string_view bytes) {
    Reader r(bytes);
    if (r.bytes(4) != "SAC1")
        invalid(Error::invalid_body);
    ScannedCommand out;
    auto &v = out.command;
    v.scope = r.blob();
    v.operation = r.blob();
    v.subject = r.u8();
    v.receipt = r.unsigned_be(8);
    if (r.present())
        v.contribution = r.unsigned_be(8);
    v.plan = r.blob();
    v.revision = r.unsigned_be(8);
    v.principal_kind = r.u8();
    v.installation = r.blob();
    v.principal = r.blob();
    v.purpose = r.u8();
    v.preview_mode = r.u8();
    v.preview = r.optional_blob();
    v.actor = r.optional_blob();
    v.decision = r.optional_blob();
    v.reason = r.optional_blob();
    v.choice_count = static_cast<std::size_t>(r.unsigned_be(4));
    out.choices = bytes.substr(r.position());
    // Each Env requires two U32 length fields, even for empty DATA. This bound
    // also prevents an enormous claimed count from creating an unbounded loop.
    if (v.choice_count > r.remaining() / 8)
        invalid(Error::invalid_body);
    for (std::size_t i = 0; i < v.choice_count; ++i) {
        (void)r.blob();
        (void)r.blob();
    }
    if (r.remaining() != 0)
        invalid(Error::invalid_body);
    return out;
}

template <class Visit>
std::size_t validate(const CommandView &v, const Visit &visit, const ApplicationServiceLimits &l,
                     Error aggregate) {
    const auto bounded = [](std::string_view s, qsizetype limit) {
        return s.size() <= std::size_t(limit);
    };
    const auto key = [&](std::string_view s) {
        if (s.empty() || !bounded(s, l.reference_bytes))
            invalid(Error::invalid_key);
    };
    for (const auto s : {v.scope, v.operation, v.plan, v.installation, v.principal})
        key(s);
    if (v.preview)
        key(*v.preview);
    // Optional metadata may be present-empty, and is DATA, not an authority key.
    if (v.subject > 1 || v.principal_kind > 1 || v.purpose > 1 || v.preview_mode > 1 ||
        v.receipt == 0 || v.revision == 0 || (v.subject == 0) != bool(v.contribution))
        invalid(Error::invalid_source);
    if (v.choice_count > std::size_t(l.choices_per_command) ||
        v.choice_count > std::numeric_limits<std::uint32_t>::max())
        invalid(Error::collection_limit);
    for (const auto &s : {v.actor, v.decision})
        if (s && !bounded(*s, l.reference_bytes))
            invalid(Error::invalid_body);
    if (v.reason && !bounded(*v.reason, l.choice_bytes))
        invalid(Error::invalid_body);
    visit([&](ChoiceView choice) {
        if (choice.format.empty() || !bounded(choice.format, l.reference_bytes) ||
            choice.null_document || !bounded(choice.document, l.choice_bytes))
            invalid(Error::invalid_body);
    });
    if ((v.preview_mode == 1 && !v.preview) ||
        (v.purpose == 1 && (!v.decision || v.decision->empty() || !v.reason || v.reason->empty())))
        invalid(Error::invalid_conditional_group);
    Size owned(
        std::size_t(aggregate == Error::result_limit ? l.result_owned_bytes : l.input_owned_bytes),
        aggregate);
    // Logical scalars omit wire option tags, lengths and collection headers.
    owned.add(20);
    if (v.contribution)
        owned.add(8);
    for (const auto s : {v.scope, v.operation, v.plan, v.installation, v.principal})
        owned.add(s.size());
    for (const auto &s : {v.preview, v.actor, v.decision, v.reason})
        if (s)
            owned.add(s->size());
    visit([&](ChoiceView choice) {
        owned.add(choice.format.size());
        owned.add(choice.document.size());
    });
    return owned.get();
}

// The same grammar drives a checked sizing sink before the allocating sink.
class Sizing final {
  public:
    explicit Sizing(std::size_t limit) : size_(limit, Error::invalid_body) {}
    void integer(std::uint64_t, std::size_t n) { size_.add(n); }
    void bytes(std::string_view b) { size_.add(b.size()); }
    [[nodiscard]] std::size_t get() const { return size_.get(); }

  private:
    Size size_;
};
class Writer final {
  public:
    explicit Writer(qsizetype size) { value_.reserve(size); }
    void integer(std::uint64_t value, std::size_t n) {
        for (std::size_t i = n; i != 0; --i)
            value_.append(static_cast<char>((value >> ((i - 1) * 8)) & 0xff));
    }
    void bytes(std::string_view b) { value_.append(b.data(), qsizetype(b.size())); }
    [[nodiscard]] QByteArray finish() && { return std::move(value_); }

  private:
    QByteArray value_;
};
template <class Sink> void blob(Sink &s, std::string_view b) {
    if (b.size() > std::numeric_limits<std::uint32_t>::max())
        invalid(Error::invalid_body);
    s.integer(b.size(), 4);
    s.bytes(b);
}
template <class Sink> void optional_blob(Sink &s, const std::optional<std::string_view> &b) {
    s.integer(bool(b), 1);
    if (b)
        blob(s, *b);
}
template <class Sink, class Visit> void write(Sink &s, const CommandView &v, const Visit &visit) {
    s.bytes("SAC1");
    blob(s, v.scope);
    blob(s, v.operation);
    s.integer(v.subject, 1);
    s.integer(v.receipt, 8);
    s.integer(bool(v.contribution), 1);
    if (v.contribution)
        s.integer(*v.contribution, 8);
    blob(s, v.plan);
    s.integer(v.revision, 8);
    s.integer(v.principal_kind, 1);
    blob(s, v.installation);
    blob(s, v.principal);
    s.integer(v.purpose, 1);
    s.integer(v.preview_mode, 1);
    optional_blob(s, v.preview);
    optional_blob(s, v.actor);
    optional_blob(s, v.decision);
    optional_blob(s, v.reason);
    s.integer(v.choice_count, 4);
    visit([&](ChoiceView choice) {
        blob(s, choice.format);
        blob(s, choice.document);
    });
}
std::optional<std::string> own(const std::optional<std::string_view> &v) {
    return v ? std::optional<std::string>{*v} : std::nullopt;
}
} // namespace

ApplicationEnvelope encode_original_application_command(const OriginalApplicationCommand &command,
                                                        const ApplicationServiceLimits &limits) {
    validate_options(limits);
    auto v = view(command);
    // An invalid C++ enum must not wrap to an accepted U8. Mark it for the
    // shared source stage, which still preserves required-key precedence.
    if (int(command.subject_kind) < 0 || int(command.subject_kind) > 1 ||
        int(command.principal.kind) < 0 || int(command.principal.kind) > 1 ||
        int(command.purpose) < 0 || int(command.purpose) > 1 || int(command.preview_mode) < 0 ||
        int(command.preview_mode) > 1)
        v.subject = 255;
    const auto visit = [&](auto consume) {
        for (const auto &choice : command.choices)
            consume(ChoiceView{choice.format_ref,
                               {choice.document.constData(), std::size_t(choice.document.size())},
                               choice.document.isNull()});
    };
    (void)validate(v, visit, limits, Error::owned_byte_limit);
    Sizing sizing(std::min(std::size_t(limits.command_bytes),
                           std::size_t(std::numeric_limits<std::uint32_t>::max())));
    write(sizing, v, visit);
    Size result(std::size_t(limits.result_owned_bytes), Error::result_limit);
    result.add(command_format.size());
    result.add(sizing.get());
    Writer writer(qsizetype(sizing.get()));
    write(writer, v, visit);
    return {std::string(command_format), std::move(writer).finish()};
}
OriginalApplicationCommand
decode_original_application_command(const ApplicationEnvelope &envelope,
                                    const ApplicationServiceLimits &limits) {
    validate_options(limits);
    if (envelope.format_ref.empty() ||
        envelope.format_ref.size() > std::size_t(limits.reference_bytes) ||
        envelope.document.isNull() || envelope.document.size() > limits.command_bytes ||
        std::uintmax_t(envelope.document.size()) > std::numeric_limits<std::uint32_t>::max())
        invalid(Error::invalid_body);
    Size input(std::size_t(limits.input_owned_bytes), Error::owned_byte_limit);
    input.add(envelope.format_ref.size());
    input.add(std::size_t(envelope.document.size()));
    if (envelope.format_ref != command_format)
        throw ApplicationServiceUnsupportedFormat(envelope.format_ref);
    const auto scanned =
        scan({envelope.document.constData(), std::size_t(envelope.document.size())});
    const auto &v = scanned.command;
    const auto visit = [&](auto consume) {
        Reader choices(scanned.choices);
        for (std::size_t i = 0; i < v.choice_count; ++i) {
            const auto format = choices.blob();
            const auto document = choices.blob();
            consume(ChoiceView{format, document, false});
        }
    };
    (void)validate(v, visit, limits, Error::result_limit);
    if (v.choice_count > std::size_t(limits.result_items))
        invalid(Error::result_limit);
    OriginalApplicationCommand out;
    out.operation = {std::string(v.scope), std::string(v.operation)};
    out.subject_kind = static_cast<ApplicationSubjectKind>(v.subject);
    out.receipt_id = {v.receipt};
    if (v.contribution)
        out.contribution_id = core::ContributionId{*v.contribution};
    out.plan = {std::string(v.plan), v.revision};
    out.principal = {static_cast<ApplicationPrincipalKind>(v.principal_kind),
                     std::string(v.installation), std::string(v.principal)};
    out.purpose = static_cast<OriginalApplicationPurpose>(v.purpose);
    out.preview_mode = static_cast<ApplicationPreviewMode>(v.preview_mode);
    out.preview_state_token = own(v.preview);
    out.actor_ref = own(v.actor);
    out.decision_ref = own(v.decision);
    out.reason = own(v.reason);
    out.choices.reserve(v.choice_count);
    visit([&](ChoiceView choice) {
        out.choices.push_back(
            {std::string(choice.format),
             QByteArray(choice.document.data(), qsizetype(choice.document.size()))});
    });
    return out;
}
} // namespace subathon::server
