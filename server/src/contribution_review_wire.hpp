// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "application_result_wire.hpp"

#include <QByteArrayView>
#include <QStringDecoder>
#include <QtEndian>
#include <bit>
#include <set>
#include <subathon/server/contribution_review.hpp>
#include <subathon/server/sqlite_application_plans.hpp>

namespace subathon::server::review_detail {

constexpr qsizetype entry_limit = 60 * 1024;
constexpr qsizetype envelope_limit = 4 * 1024;
constexpr qsizetype manifest_limit = 16 * entry_limit + envelope_limit;
constexpr size_t row_limit = 16;

struct Invalid {};
struct Oversize {};

inline void require(bool condition) {
    if (!condition)
        throw Invalid{};
}

inline QByteArray bytes(std::string_view value) {
    return QByteArray(value.data(), qsizetype(value.size()));
}

inline bool utf8(std::string_view text) {
    QStringDecoder decode(QStringDecoder::Utf8);
    (void)QString(decode(QByteArrayView(text.data(), qsizetype(text.size()))));
    return !decode.hasError();
}

class Encode {
  public:
    static constexpr bool reading = false;
    explicit Encode(qsizetype limit = entry_limit) : limit_(limit) {}

    void raw(QByteArrayView value) {
        if (value.size() > limit_ - data.size())
            throw Oversize{};
        data.append(value);
    }
    void u8(unsigned value) {
        require(value <= 255);
        const auto ch = char(value);
        raw(QByteArrayView(&ch, 1));
    }
    void u32(size_t value) {
        if (value > size_t(limit_))
            throw Oversize{};
        quint32 encoded = qToBigEndian(quint32(value));
        raw(QByteArrayView(reinterpret_cast<const char *>(&encoded), 4));
    }
    void b(const QByteArray &value) {
        u32(size_t(value.size()));
        raw(value);
    }
    void b(std::string_view value) {
        u32(value.size());
        raw(QByteArrayView(value.data(), qsizetype(value.size())));
    }
    void i64(std::int64_t value) { number(std::bit_cast<std::uint64_t>(value)); }
    void number(std::uint64_t value) {
        quint64 encoded = qToBigEndian(quint64(value));
        raw(QByteArrayView(reinterpret_cast<const char *>(&encoded), 8));
    }
    void exact(const core::Exact &value) {
        const auto parts = encode_plan_exact(value, 4096);
        b(parts.numerator);
        b(parts.denominator);
    }
    template <class T, class F> void opt(const std::optional<T> &value, F visit) {
        u8(value.has_value());
        if (value)
            visit(*value);
    }
    template <class T, class F> void vec(const std::vector<T> &values, F visit) {
        u32(values.size());
        for (const auto &value : values)
            visit(value);
    }

    template <class E> void tag(E value, E max) {
        require(unsigned(value) <= unsigned(max));
        u8(unsigned(value));
    }
    void text(std::string_view value, bool nonempty = false, size_t max = entry_limit) {
        if (value.size() > max || value.size() > size_t(limit_ - data.size()))
            throw Oversize{};
        require((!nonempty || !value.empty()) && utf8(value));
        b(value);
    }
    void ref(std::string_view value) { text(value, true, 4096); }
    void flag(bool value) { u8(value); }
    void blob(const QByteArray &value) { b(value); }
    template <class V, class F> void sequence(const V &values, F visit) {
        if (values.size() > size_t(limit_ - data.size()))
            throw Oversize{};
        vec(values, visit);
    }
    QByteArray data;

  private:
    qsizetype limit_;
};

class Decode {
  public:
    static constexpr bool reading = true;
    explicit Decode(QByteArrayView data) : remaining_(data) {}
    QByteArrayView take(qsizetype size) {
        require(size >= 0 && size <= remaining_.size() && size <= allowance_);
        allowance_ -= size;
        const auto result = remaining_.first(size);
        remaining_ = remaining_.sliced(size);
        return result;
    }
    void magic(QByteArrayView value) { require(take(value.size()) == value); }
    void end() const { require(remaining_.empty()); }
    unsigned u8() { return static_cast<unsigned char>(take(1).front()); }
    quint32 u32() { return qFromBigEndian<quint32>(take(4).data()); }
    void number(std::uint64_t &value) { value = qFromBigEndian<quint64>(take(8).data()); }
    void i64(std::int64_t &value) {
        std::uint64_t bits{};
        number(bits);
        value = std::bit_cast<std::int64_t>(bits);
    }
    void flag(bool &value) {
        const auto tag = u8();
        require(tag < 2);
        value = tag == 1;
    }
    template <class E> void tag(E &value, E max) {
        const auto v = u8();
        require(v <= unsigned(max));
        value = static_cast<E>(v);
    }
    void text(std::string &value, bool nonempty = false, size_t max = entry_limit) {
        const auto size = u32();
        require(size <= max && (!nonempty || size != 0));
        const auto v = take(size);
        const std::string_view text(v.data(), size);
        require(utf8(text));
        value.assign(text);
    }
    void ref(std::string &value) { text(value, true, 4096); }
    void blob(QByteArray &value) {
        auto data = take(u32());
        value = QByteArray(data.data(), data.size());
    }
    void exact(core::Exact &value) {
        const auto start = take(u32());
        const auto end = take(u32());
        require(start.size() <= 4096 && end.size() <= 4096);
        value = decode_plan_exact({start.toByteArray(), end.toByteArray()}, 4096);
    }
    template <class V, class F> void opt(V &value, F visit) {
        bool present = false;
        flag(present);
        if (present)
            visit(value.emplace());
    }
    template <class V, class F> void sequence(V &values, F visit) {
        const auto count = u32();
        require(count <= remaining_.size() && count <= allowance_);
        for (quint32 i = 0; i < count; ++i)
            visit(values.emplace_back());
    }
    qsizetype remaining() const { return remaining_.size(); }
    void record_limit(qsizetype bytes) { allowance_ = bytes; }

  private:
    QByteArrayView remaining_;
    qsizetype allowance_{manifest_limit};
};

template <class W, class T> void state_fields(W &w, T &s) {
    auto &c = s.conditions;
    auto &v = c.valuation;
    auto &a = c.evidence_admission;
    w.tag(s.payment_evidence, core::PaymentEvidence::not_applicable);
    w.flag(v.monetary);
    w.opt(v.original_money, [&](auto &m) {
        w.text(m.original_decimal, true);
        w.ref(m.currency);
    });
    w.opt(v.resolution_ref, [&](auto &r) { w.ref(r); });
    w.flag(v.missing_rate);
    w.tag(c.review, core::ContributionReview::rejected);
    w.tag(a.policy, core::EvidencePolicy::record_only);
    w.ref(a.policy_ref);
    w.opt(a.review_decision, [&](auto &d) {
        w.tag(d.choice, core::EvidenceReviewChoice::accept_labelled_report);
        w.ref(d.decision_ref);
        w.ref(d.actor_ref);
    });
    w.tag(c.application, core::ContributionApplication::applied);
    w.tag(c.reversal, core::ContributionReversal::full);
    w.tag(c.settlement, core::SettlementGate::clear);
}

template <class W, class T> void plan_fields(W &w, T &r) {
    auto ref = [&](auto &v) { w.ref(v); };
    auto number = [&](auto &v) { w.number(v); };
    auto time = [&](auto &v) { w.i64(v); };
    auto money = [&](auto &m) {
        w.text(m.original_decimal, true);
        w.ref(m.currency);
    };
    auto link = [&](auto &l) {
        w.ref(l.identifier_namespace);
        w.ref(l.opaque_id);
    };
    w.number(r.revision);
    w.opt(r.parent_revision, number);
    auto &e = r.captured.event;
    w.ref(e.event_ref);
    w.opt(e.parent_event_ref, ref);
    w.ref(e.root_event_ref);
    w.number(e.receipt_id.value);
    w.ref(e.identity.origin.provider);
    w.ref(e.identity.origin.source_account);
    w.ref(e.identity.identifier_namespace);
    w.ref(e.identity.opaque_id);
    w.ref(e.facts.event_type);
    if constexpr (W::reading)
        e.facts.schema_version = w.u32();
    else
        w.u32(e.facts.schema_version);
    w.opt(e.facts.source_timestamp_utc_ms, time);
    w.opt(e.facts.actor_id, ref);
    w.opt(e.facts.actor_display_name, [&](auto &v) { w.text(v); });
    w.opt(e.facts.money, money);
    w.opt(e.facts.payment_reference, link);
    w.opt(e.facts.activity_reference, link);
    w.i64(e.arrival.server_timestamp_utc_ms);
    w.ref(e.arrival.connection_ref);
    w.ref(e.arrival.configuration_ref);
    w.ref(e.arrival.modifier_context_ref);
    w.opt(e.contribution_id, [&](auto &id) { w.number(id.value); });
    state_fields(w, r.captured.contribution_state);
    auto &v = r.captured.valuation;
    w.tag(v.state, core::PlanValuationState::resolved);
    w.opt(v.historical, [&](auto &h) {
        w.ref(h.resolution_ref);
        money(h.original_money);
        w.text(h.comparison_currency, true);
        w.text(h.effective_rate_decimal, true);
        w.opt(h.fetched_rate_decimal, [&](auto &s) { w.text(s, true); });
        w.ref(h.snapshot_ref);
        w.opt(h.source_ref, ref);
        w.opt(h.rate_timestamp_utc_ms, time);
        w.opt(h.fetch_timestamp_utc_ms, time);
        w.opt(h.manual_decision_ref, ref);
    });
    require(bool(v.historical) == (v.state == core::PlanValuationState::resolved));
    w.sequence(r.captured.requested_effects, [&](auto &q) {
        w.ref(q.action_ref);
        w.ref(q.action_version_ref);
        w.ref(q.rule_ref);
        w.ref(q.target_ref);
        w.tag(q.unit, core::RequestedUnit::whole_count);
        w.flag(q.requires_historical_valuation);
        w.exact(q.exact_requested_units);
        w.i64(q.requested_units);
        w.text(q.calculation_explanation);
        w.ref(q.reversal_behavior_ref);
    });
    w.sequence(r.rules, [&](auto &q) {
        w.ref(q.rule_ref);
        w.ref(q.version_ref);
        w.flag(q.selected);
        w.text(q.explanation);
    });
    w.sequence(r.modifiers, [&](auto &m) {
        w.ref(m.definition_version_ref);
        w.ref(m.instance_revision_ref);
        w.ref(m.target_ref);
        w.ref(m.group_ref);
        w.ref(m.group_operator);
        w.ref(m.target_operator);
        w.text(m.original_factor_decimal, true);
    });
    w.opt(r.override_decision, [&](auto &d) {
        w.tag(d.authority, core::OverrideAuthority::authorized);
        w.ref(d.actor_ref);
        w.text(d.reason, true);
        w.ref(d.decision_ref);
    });
}

template <class W, class T> void selector_fields(W &w, T &s) {
    w.number(s.cid.value);
    w.number(s.receipt.value);
    require(s.receipt.value != 0);
}
template <class W, class T> void timer_fields(W &w, T &t) {
    w.ref(t.locator.hold_ref);
    w.ref(t.locator.version_ref);
    w.i64(t.ordinal);
    w.i64(t.signed_request);
    require(t.ordinal > 0 && t.signed_request < 0);
}
template <class W, class T> void options(W &w, T &i) {
    w.tag(i.mode, PausedAcceptanceMode::silent);
    w.opt(i.milestones, [&](auto &list) {
        w.sequence(list, [&](auto &c) {
            w.ref(c.occurrence.ref);
            w.tag(c.choice, MilestoneActionChoice::skip);
        });
    });
    w.opt(i.pending_decision_ref, [&](auto &r) { w.ref(r); });
    w.opt(i.timer, [&](auto &t) { timer_fields(w, t); });
    w.opt(i.deficits, [&](auto &list) {
        w.sequence(list, [&](auto &d) {
            w.i64(d.ordinal);
            w.i64(d.signed_request);
            w.tag(d.policy, core::RefundDeficitPolicy::RequireReview);
            require(d.ordinal > 0 && d.signed_request < 0 &&
                    d.policy != core::RefundDeficitPolicy::RequireReview);
        });
    });
    std::string_view previous;
    w.sequence(i.ordinary_closures, [&](auto &c) {
        w.ref(c.hold_ref);
        require(previous.empty() || previous < c.hold_ref);
        previous = c.hold_ref;
        w.ref(c.expected_version_ref);
        w.ref(c.proposed_version_ref);
        require(c.expected_version_ref != c.proposed_version_ref);
        w.ref(c.decision.format_ref);
        w.blob(c.decision.document);
        // Review never owns replacement holds.
        if constexpr (W::reading)
            require(w.u8() == 0);
        else {
            require(!c.replacement && !c.decision.document.isNull());
            w.u8(0);
        }
    });
    w.opt(i.revision, [&](auto &r) { plan_fields(w, r); });
    using V = ReviewVerb;
    const bool held =
        i.verb == V::continue_timer || i.verb == V::reject_held || i.verb == V::retain_held;
    require(held == i.timer.has_value());
    require(i.verb == V::continue_timer || !i.deficits);
    require(i.verb != V::apply_accepted || i.pending_decision_ref);
    require(i.verb == V::apply_accepted || i.verb == V::continue_timer || !i.pending_decision_ref);
    if (i.verb == V::reject || i.verb == V::reject_held || i.verb == V::retain_held) {
        require(i.mode == PausedAcceptanceMode::normal && !i.revision);
        // Existing reject clients send the neutral explicit empty choice vector.
        require(!i.milestones || i.milestones->empty());
    }
    require((i.verb != V::reject && i.verb != V::retain_held) || i.ordinary_closures.empty());
}

template <class W, class T> void entry_fields(W &w, T &e) {
    selector_fields(w, e.intent.selector);
    w.tag(e.intent.verb, ReviewVerb::retain_held);
    w.blob(e.basis);
    require(!e.basis.isEmpty());
    options(w, e.intent);
    w.ref(e.decision_ref);
    w.text(e.reason, true);
}
template <class W, class T> void identity(W &w, T &k) {
    w.ref(k.operation_scope);
    w.ref(k.operation_ref);
    require(k.operation_scope == "review");
    if (k.operation_scope.size() + k.operation_ref.size() + 8 > 256)
        throw Oversize{};
}
template <class W, class T> void principal_fields(W &w, T &p) {
    w.ref(p.installation_ref);
    w.tag(p.kind, ApplicationPrincipalKind::automation);
    w.ref(p.principal_ref);
}

inline void selection_order(auto &entries, auto select) {
    std::ranges::sort(entries, {}, [&](const auto &e) { return select(e).receipt.value; });
    std::set<std::uint64_t> ids;
    std::uint64_t last{};
    for (const auto &entry : entries) {
        const auto &s = select(entry);
        require(s.receipt.value > last && ids.insert(s.cid.value).second);
        last = s.receipt.value;
    }
}

inline QByteArray manifest(const ReviewCommand &command) {
    Encode header(envelope_limit);
    header.raw("SRW1");
    header.ref(command.principal.installation_ref);
    identity(header, command.key);
    header.tag(command.principal.kind, ApplicationPrincipalKind::automation);
    header.ref(command.principal.principal_ref);
    header.ref(command.actor);
    header.u32(command.entries.size());
    for (const auto &entry : command.entries) {
        Encode record;
        entry_fields(record, entry);
        header.data.append(record.data);
    }
    return std::move(header.data);
}

inline ReviewCommand decode_manifest(const QByteArray &doc) {
    require(doc.size() <= manifest_limit);
    Decode read(doc);
    ReviewCommand out;
    read.magic("SRW1");
    read.ref(out.principal.installation_ref);
    identity(read, out.key);
    read.tag(out.principal.kind, ApplicationPrincipalKind::automation);
    read.ref(out.principal.principal_ref);
    read.ref(out.actor);
    const auto count = read.u32();
    require(count > 0 && count <= row_limit && doc.size() - read.remaining() <= envelope_limit);
    std::uint64_t last{};
    std::set<std::uint64_t> ids;
    for (quint32 n = 0; n < count; ++n) {
        auto &entry = out.entries.emplace_back();
        const auto before = read.remaining();
        read.record_limit(entry_limit);
        entry_fields(read, entry);
        const auto &s = entry.intent.selector;
        require(before - read.remaining() <= entry_limit && s.receipt.value > last &&
                ids.insert(s.cid.value).second);
        last = s.receipt.value;
    }
    read.end();
    require(manifest(out) == doc);
    return out;
}

} // namespace subathon::server::review_detail
