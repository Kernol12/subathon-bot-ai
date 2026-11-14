// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <limits>
#include <subathon/server/sqlite_permission_graph.hpp>
#include <type_traits>

namespace subathon::server::permission_graph {
using namespace core;
using Failure = PermissionGraphFailure;
[[noreturn]] inline void fail(Failure e) { throw PermissionGraphStorageFailure(e); }
inline void data(bool ok) {
    if (!ok)
        fail(Failure::invalid_data);
}
inline void space(bool ok) {
    if (!ok)
        fail(Failure::resource_limit);
}
inline void add(std::uint64_t &n, std::uint64_t amount, std::uint64_t cap) {
    space(amount <= cap - n);
    n += amount;
}
// One declaration-order traversal shared by projection and the structural codec.
// No generic serialized object is retained as the authoritative policy store.
#define PG_FIELDS(Type, ...)                                                                       \
    template <class F, class V>                                                                    \
        requires std::is_same_v<std::remove_cvref_t<V>, Type>                                      \
    void fields(F &f, V &v) {                                                                      \
        f(__VA_ARGS__);                                                                            \
    }
PG_FIELDS(PermissionResourceConstraint, v.slot, v.mode, v.allowed)
PG_FIELDS(PermissionQuantityConstraint, v.slot, v.unit, v.mode, v.lower, v.upper)
PG_FIELDS(PermissionPolicyEntry, v.origin, v.source, v.role, v.operation, v.effect, v.grant,
          v.resources, v.quantities)
PG_FIELDS(PermissionQuantityDefinition, v.slot, v.unit, v.kind, v.minimum, v.maximum,
          v.allowed_modes)
PG_FIELDS(PermissionOperationDefinition, v.operation, v.classification, v.resource_slots,
          v.quantity_slots, v.helper_target_slot)
PG_FIELDS(PermissionCatalog, v.state, v.revision, v.operations)
PG_FIELDS(PermissionManagementBinding, v.operation, v.target_slot)
PG_FIELDS(PermissionGraphTrust, v.catalog, v.role_edit, v.membership_edit, v.override_edit)
PG_FIELDS(PermissionEditableRole, v.role, v.entries)
PG_FIELDS(PermissionEditableAccount, v.account, v.assigned_roles, v.individual_entries)
PG_FIELDS(PermissionEditableState, v.state, v.roles, v.accounts)
PG_FIELDS(PermissionOwnerFacts, v.state, v.account)
PG_FIELDS(PermissionResourceBinding, v.slot, v.resource)
PG_FIELDS(PermissionQuantityBinding, v.slot, v.unit, v.value)
PG_FIELDS(PermissionRequest, v.operation, v.resources, v.quantities)
PG_FIELDS(StoredInstallationFacts, v.current.installation_ref, v.current.session_epoch_ref,
          v.revision_ref)
PG_FIELDS(StoredAccountFacts, v.current.installation_ref, v.current.account_ref,
          v.current.session_epoch_ref, v.current.application_access_enabled, v.revision_ref)
PG_FIELDS(StoredPermissionGraph, v.installation, v.revision, v.owner, v.catalog, v.use, v.grant,
          v.current_installation, v.current_accounts)
PG_FIELDS(PermissionGraphEditFacts, v.before, v.after, v.management)
#undef PG_FIELDS
template <class F, class V>
    requires std::is_same_v<std::remove_cvref_t<V>, PermissionRational>
void fields(F &f, V &v) {
    f.component(v.numerator);
    f.component(v.denominator);
}
struct Projection {
    const PermissionGraphLimits &limits;
    std::uint64_t cap, bytes = 0, items = 0;
    bool framed = false, empty = false;
    void fixed(std::uint64_t n) { add(bytes, n, cap); }
    void elements(std::uint64_t n) { add(items, n, limits.items); }
    void text(std::string_view s, bool numeric) {
        space(s.size() <= (numeric ? limits.component_bytes : limits.reference_bytes));
        space(s.size() <= std::string{}.max_size());
        if (numeric)
            space(s.size() <=
                  (std::min(std::uint64_t(SIZE_MAX), std::uint64_t(PTRDIFF_MAX)) - 1) / 16);
        if (!numeric && s.empty())
            empty = true;
        fixed(s.size());
        if (framed)
            fixed(8);
    }
    void component(const std::string &s) { text(s, true); }
    void one(const std::string &s) { text(s, false); }
    void one(std::string_view s) { text(s, false); }
    template <class T>
        requires(std::is_enum_v<T> || std::is_integral_v<T>)
    void one(T) {
        static_assert(!std::is_enum_v<T> || sizeof(T) == 1,
                      "Permission graph enums have one-byte persisted and semantic width");
        fixed(sizeof(T));
    }
    template <class T> void one(const std::optional<T> &v) {
        fixed(1);
        if (v)
            one(*v);
    }
    template <class T> void one(const std::vector<T> &v) {
        space(v.size() <= std::vector<T>{}.max_size());
        elements(v.size());
        if (framed)
            fixed(8);
        for (const auto &x : v)
            one(x);
    }
    template <class T>
        requires(!std::is_enum_v<T> && !std::is_integral_v<T>)
    void one(const T &v) {
        fields(*this, v);
    }
    template <class... T> void operator()(const T &...v) { (one(v), ...); }
};
// First pass is allocation-free: even vectors are scanned through a single
// stack value. It projects every string and nested vector before any DTO copy.
class Decoder {
    std::string_view input_;
    std::size_t at_ = 0;
    Projection &projection_;
    bool own_;
    std::uint64_t integer(unsigned width) {
        data(width <= input_.size() - at_);
        std::uint64_t n = 0;
        for (unsigned i = 0; i < width; ++i)
            n = (n << 8) | static_cast<unsigned char>(input_[at_++]);
        return n;
    }
    void text(std::string &v, bool numeric) {
        const auto n = integer(8);
        // Counts/length bounds take priority over truncation.
        space(n <=
              (numeric ? projection_.limits.component_bytes : projection_.limits.reference_bytes));
        space(n <= v.max_size());
        if (numeric)
            space(n <= (std::min(std::uint64_t(SIZE_MAX), std::uint64_t(PTRDIFF_MAX)) - 1) / 16);
        projection_.fixed(n);
        data(n <= input_.size() - at_);
        if (!numeric && !n)
            projection_.empty = true;
        if (own_)
            v.assign(input_.data() + at_, static_cast<std::size_t>(n));
        at_ += static_cast<std::size_t>(n);
    }

  public:
    Decoder(std::string_view bytes, Projection &p, bool own)
        : input_(bytes), projection_(p), own_(own) {}
    Decoder(const QByteArray &bytes, Projection &p, bool own)
        : Decoder(std::string_view(bytes.constData(), static_cast<std::size_t>(bytes.size())), p,
                  own) {}
    void component(std::string &v) { text(v, true); }
    void one(std::string &v) { text(v, false); }
    template <class T>
        requires(std::is_enum_v<T> || std::is_integral_v<T>)
    void one(T &v) {
        static_assert(!std::is_enum_v<T> || sizeof(T) == 1,
                      "Permission graph enums have one-byte persisted and semantic width");
        v = static_cast<T>(integer(sizeof(T)));
        projection_.fixed(sizeof(T));
    }
    template <class T> void one(std::optional<T> &v) {
        const auto tag = integer(1);
        projection_.fixed(1);
        data(tag <= 1);
        if (tag) {
            T x{};
            one(x);
            if (own_)
                v = std::move(x);
        }
    }
    template <class T> void one(std::vector<T> &v) {
        const auto n = integer(8);
        projection_.elements(n);
        space(n <= v.max_size());
        if (own_)
            v.reserve(static_cast<std::size_t>(n));
        for (std::uint64_t i = 0; i < n; ++i) {
            T x{};
            one(x);
            if (own_)
                v.push_back(std::move(x));
        }
    }
    template <class T>
        requires(!std::is_enum_v<T> && !std::is_integral_v<T>)
    void one(T &v) {
        fields(*this, v);
    }
    template <class... T> void operator()(T &...v) { (one(v), ...); }
    void finish() const { data(at_ == input_.size()); }
    template <class T> T read() {
        T v{};
        one(v);
        data(at_ == input_.size());
        return v;
    }
};
class Encoder {
    QByteArray *out_ = nullptr;
    std::string_view expected_;
    std::size_t at_ = 0;
    void append(const char *text, std::size_t n) {
        if (out_) {
            out_->append(text, static_cast<qsizetype>(n));
        } else {
            data(n <= expected_.size() - at_);
            data(expected_.substr(at_, n) == std::string_view(text, n));
            at_ += n;
        }
    }

  public:
    explicit Encoder(QByteArray &out) : out_(&out) {}
    explicit Encoder(std::string_view expected) : expected_(expected) {}
    void finish() const {
        if (!out_)
            data(at_ == expected_.size());
    }
    void integer(std::uint64_t n, unsigned width) {
        for (unsigned i = width; i > 0; --i) {
            const char byte = char((n >> ((i - 1) * 8)) & 255);
            append(&byte, 1);
        }
    }
    void one(const std::string &v) {
        integer(v.size(), 8);
        append(v.data(), v.size());
    }
    void component(const std::string &v) { one(v); }
    template <class T>
        requires(std::is_enum_v<T> || std::is_integral_v<T>)
    void one(T v) {
        static_assert(!std::is_enum_v<T> || sizeof(T) == 1,
                      "Permission graph enums have one-byte persisted and semantic width");
        integer(static_cast<std::uint64_t>(v), sizeof(T));
    }
    template <class T> void one(const std::optional<T> &v) {
        integer(bool(v), 1);
        if (v)
            one(*v);
    }
    template <class T> void one(const std::vector<T> &v) {
        integer(v.size(), 8);
        for (const auto &x : v)
            one(x);
    }
    template <class T>
        requires(!std::is_enum_v<T> && !std::is_integral_v<T>)
    void one(const T &v) {
        fields(*this, v);
    }
    template <class... T> void operator()(const T &...v) { (one(v), ...); }
};
template <class T> QByteArray encode(const T &v, const PermissionGraphLimits &l) {
    Projection p{l, l.selected_bytes};
    p.framed = true;
    p(v);
    space(p.bytes <= static_cast<std::uint64_t>(QByteArray::maxSize()));
    QByteArray out;
    out.reserve(static_cast<qsizetype>(p.bytes));
    Encoder e{out};
    e(v);
    return out;
}
// Internal precondition: the complete source/result dry pass has already
// scanned this payload. This second pass only owns its admitted fields.
template <class T> T decode(const QByteArray &b, const PermissionGraphLimits &l) {
    Projection p{l, l.selected_bytes};
    Decoder d{b, p, true};
    return d.read<T>();
}
inline QByteArray blob(std::string_view s) {
    return QByteArray(s.data(), static_cast<qsizetype>(s.size()));
}
inline bool byte_less(const std::string &a, const std::string &b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                                        [](unsigned char x, unsigned char y) { return x < y; });
}
} // namespace subathon::server::permission_graph
