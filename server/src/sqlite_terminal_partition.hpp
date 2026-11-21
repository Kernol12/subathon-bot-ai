// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "application_result_wire.hpp"

#include <QByteArrayView>
#include <QtEndian>
#include <algorithm>
#include <bit>
#include <limits>

namespace subathon::server::terminal_partition {

inline constexpr auto result_format = "subathon.internal.application-result.v2";

enum class Catalog { legacy, review, connector_intake, invalid };

inline Catalog catalog(const SqliteWorkSession &session) {
    const auto marker = session.execute(
        {"SELECT CASE WHEN typeof(version)<>'integer' THEN 3 "
         "WHEN version=19 AND migration_id COLLATE BINARY='contribution-review-v1' THEN 1 "
         "WHEN version=20 AND migration_id COLLATE BINARY='connector-intake-storage-v1' THEN 2 "
         "WHEN version<19 THEN 0 ELSE 3 END "
         "FROM main.subathon_migration_history "
         "WHERE EXISTS(SELECT 1 FROM main.subathon_catalog WHERE singleton=1 "
         "AND typeof(catalog_format)='integer' AND catalog_format=1 "
         "AND store_identity='subathon.sqlite-store') ORDER BY version DESC LIMIT 1",
         {}});
    if (marker.empty())
        return Catalog::legacy;
    return static_cast<Catalog>(marker.front().front().toInt());
}

inline bool selected(const SqliteWorkSession &session) {
    const auto current = catalog(session);
    if (current == Catalog::invalid)
        throw SqliteBundleStorageFailure(SqliteBundleStorageReason::invalid_stored_data);
    return current == Catalog::review || current == Catalog::connector_intake;
}

struct Invalid {};

inline void require(bool value) {
    if (!value)
        throw Invalid{};
}

struct Mask {
    QByteArrayView tags;
    std::size_t terminals{};
    bool has_effect(std::size_t ordinal) const {
        return ordinal > 0 && ordinal <= std::size_t(tags.size()) &&
               tags[qsizetype(ordinal - 1)] == 0;
    }
};

// Read the footer at the only permitted offset after the plan count is known.
// At prepare time the effect vector also identifies the sole valid body end.
inline Mask footer(QByteArrayView doc, std::size_t count) {
    require(doc.startsWith("SAR2") && count > 0 && count <= std::size_t(doc.size()) &&
            std::size_t(doc.size()) - count >= 12);
    const auto offset = doc.size() - qsizetype(count) - 8;
    require(qFromBigEndian<quint32>(doc.data() + offset) == count);
    const auto terminal_count = qFromBigEndian<quint32>(doc.data() + offset + 4);
    auto tags = doc.sliced(offset + 8);
    require(terminal_count > 0 && terminal_count <= count &&
            std::ranges::all_of(tags, [](char tag) { return tag >= 0 && tag <= 2; }) &&
            std::size_t(std::ranges::count_if(tags, [](char tag) { return tag != 0; })) ==
                terminal_count);
    return {tags, terminal_count};
}

// Compare the retained bytes directly against the typed physical result. This
// checks exact framing without decoding arbitrary "exact" integers from it or
// allocating a second copy of the document.
class Match {
  public:
    explicit Match(QByteArrayView bytes) : remaining_(bytes) {}
    void raw(QByteArrayView bytes) {
        require(remaining_.startsWith(bytes));
        remaining_ = remaining_.sliced(bytes.size());
    }
    void u8(unsigned value) {
        const char byte = char(value);
        raw(QByteArrayView(&byte, 1));
    }
    void u32(std::size_t value) {
        require(value <= std::numeric_limits<quint32>::max());
        char bytes[4];
        qToBigEndian(quint32(value), bytes);
        raw(QByteArrayView(bytes, sizeof(bytes)));
    }
    void i64(std::int64_t value) {
        char bytes[8];
        qToBigEndian(std::bit_cast<std::uint64_t>(value), bytes);
        raw(QByteArrayView(bytes, sizeof(bytes)));
    }
    void b(std::string_view value) {
        u32(value.size());
        raw(QByteArrayView(value.data(), qsizetype(value.size())));
    }
    void b(const QByteArray &value) {
        b(std::string_view(value.constData(), size_t(value.size())));
    }
    void exact(const core::Exact &value) {
        b(boost::multiprecision::numerator(value).str());
        b(boost::multiprecision::denominator(value).str());
    }
    template <class T, class F> void opt(const std::optional<T> &value, F f) {
        u8(value.has_value());
        if (value)
            f(*value);
    }
    template <class T, class F> void vec(const std::vector<T> &values, F f) {
        u32(values.size());
        for (const auto &value : values)
            f(value);
    }
    QByteArrayView remaining() const { return remaining_; }

  private:
    QByteArrayView remaining_;
};

template <class Bundle> Mask match_result(const Bundle &b) {
    const QByteArrayView doc(b.result.document);
    require(doc.startsWith("SAR2"));
    Match match(doc.sliced(4));
    application_result_wire::put_result_fields(
        match, b.intent.choices, b.intent.expected_state_token, b.advanced_before.token,
        b.after.token, false, b.effects, b.jobs, b.transitions);
    const auto remaining = match.remaining();
    require(remaining.size() > 8);
    const auto mask = footer(doc, std::size_t(remaining.size() - 8));
    require(mask.terminals + b.effects.size() == std::size_t(mask.tags.size()));
    for (const auto &effect : b.effects)
        require(mask.has_effect(std::size_t(effect.request_ordinal)));
    // This storage path has no independent authorizer for a source-less job.
    // A physical job may point only to a physical request in this bundle.
    for (const auto &job : b.jobs)
        require(job.source_request_ordinal &&
                mask.has_effect(std::size_t(*job.source_request_ordinal)));
    return mask;
}

} // namespace subathon::server::terminal_partition
