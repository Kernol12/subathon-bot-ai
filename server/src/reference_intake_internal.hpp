// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "reference_state_internal.hpp"
#include "sqlite_application_bundle_access.hpp"
#include <subathon/server/sqlite_application_plans.hpp>
#include <subathon/server/sqlite_receipts.hpp>
namespace subathon::server::reference_intake_detail {
using namespace reference_runtime_detail;
struct Refuse {
    ReferenceUnavailable value;
};
[[noreturn]] inline void refuse(ReferenceGate g, const char *reason) { throw Refuse{{g, reason}}; }
struct Cleanup {
    SqliteWorkSession session;
    bool owned = true;
    ~Cleanup() {
        if (owned)
            (void)session.cleanup();
    }
};
inline SqliteBundleLimits bundle_limits(const ReferenceRuntimeLimits &l) {
    auto b = l.service.bundles;
    b.reference_bytes =
        qsizetype(std::min(std::uint64_t(b.reference_bytes), l.values.reference_bytes));
    b.value_bytes = qsizetype(std::min(std::uint64_t(b.value_bytes), l.values.document_bytes));
    b.owned_bytes = qsizetype(std::min(std::uint64_t(b.owned_bytes), l.values.value_owned_bytes));
    b.input_items = qsizetype(std::min(std::uint64_t(b.input_items), l.values.value_items));
    b.result_items = qsizetype(std::min(std::uint64_t(b.result_items), l.values.value_items));
    return b;
}
inline RetainedLimits retained_limits(const ReferenceRuntimeLimits &l) {
    return {qsizetype(l.values.reference_bytes), qsizetype(l.values.document_bytes)};
}
inline ReferenceConfiguration admitted_config(ReferenceConfigurationSource &source,
                                              const std::string &ref,
                                              const ReferenceRuntimeLimits &l) {
    auto r = borrow([&] { return source.snapshot(ref); });
    auto a = admit_reference_configuration(ref, r, l.values);
    if (auto u = std::get_if<ReferenceUnavailable>(&a))
        throw Refuse{*u};
    return std::get<ReferenceAdmittedConfiguration>(std::move(a)).configuration;
}
inline ReferenceConfiguration retained_config(const SqliteWorkSession &s,
                                              ReferenceConfigurationSource &source,
                                              const std::string &ref,
                                              const ReferenceRuntimeLimits &l) {
    auto cfg = admitted_config(source, ref, l);
    auto row =
        SqliteRetainedContextStore(s, retained_limits(l)).find({RetainedKind::configuration, ref});
    if (!row)
        refuse(ReferenceGate::not_ready, "reference.configuration-unavailable.v1");
    auto decoded = decode_reference_configuration(
        {"subathon.reference.configuration.v1", row->document}, l.values);
    data(!row->configuration && reference_detail::same(decoded, cfg));
    return cfg;
}
inline ReferenceArrivalCapture arrival(const SqliteWorkSession &s, const ContributionReceipt &r,
                                       ReferenceConfigurationSource &source,
                                       const ReferenceRuntimeLimits &l) {
    auto row = SqliteRetainedContextStore(s, retained_limits(l))
                   .find({RetainedKind::arrival_context, r.arrival.modifier_context_ref});
    data(bool(row) && row->configuration &&
         row->configuration->reference == r.arrival.configuration_ref);
    auto a = decode_reference_arrival({"subathon.reference.arrival.v1", row->document}, l.values);
    auto cfg = retained_config(s, source, r.arrival.configuration_ref, l);
    data(reference_detail::same(a.configuration, cfg) &&
         a.arrival_context_ref == r.arrival.modifier_context_ref &&
         a.server_arrival_utc_ms == r.arrival.server_timestamp_utc_ms &&
         a.observation.connection_ref == r.arrival.connection_ref &&
         a.observation.event_id == r.identity.opaque_id &&
         cfg.synthetic_origin == r.identity.origin &&
         cfg.receipt_namespace == r.identity.identifier_namespace &&
         r.facts.event_type == "synthetic.reference.support.v1" && r.facts.schema_version == 1 &&
         !r.facts.money && !r.facts.payment_reference && !r.facts.activity_reference);
    return a;
}
// Before legacy selected decoders see payload, admit types/lengths for the fixed
// receipt and contribution closures. The original decoder still owns semantics.
inline void metadata(const SqliteWorkSession &s, const char *table, const QStringList &cols,
                     const char *key, const QByteArray &id, const ReferenceRuntimeLimits &l) {
    QStringList projection;
    for (const auto &c : cols) {
        projection << ("typeof(" + c + ")") << ("length(CAST(" + c + " AS BLOB))");
    }
    auto rows = s.execute(
        {"SELECT " + projection.join(',') + " FROM main." + table + " WHERE " + key + "=? LIMIT 1",
         {id}});
    std::uint64_t total = 0;
    for (const auto &r : rows)
        for (qsizetype i = 0; i < r.size(); i += 2) {
            auto type = r[i].toString();
            if (type == "null")
                continue;
            data(type == "integer" || type == "blob");
            auto n = r[i + 1].toULongLong();
            const auto &name = cols[i / 2];
            const bool value = name == "event_type" || name == "actor_id" ||
                               name == "actor_display_name" || name == "money_original_decimal" ||
                               name == "money_currency" || name == "original_decimal" ||
                               name == "currency";
            const auto field = value ? std::min(l.values.document_bytes,
                                                std::uint64_t(l.service.bundles.value_bytes))
                                     : std::min(l.values.reference_bytes,
                                                std::uint64_t(l.service.bundles.reference_bytes));
            require(n <= field, Error::result_limit);
            const auto cost = type == "integer" ? std::uint64_t{8} : n;
            require(cost <= std::min(l.values.value_owned_bytes,
                                     std::uint64_t(l.service.bundles.owned_bytes)) -
                                total,
                    Error::result_limit);
            total += cost;
        }
}
inline ContributionReceipt receipt(const SqliteWorkSession &s, core::ReceiptId id,
                                   const ReferenceRuntimeLimits &l) {
    metadata(s, "subathon_receipts", receipt_columns, "receipt_id", encode_receipt_uint64(id.value),
             l);
    auto r = Access(s, bundle_limits(l)).original(id);
    data(bool(r));
    return *r;
}
inline void live(const SqliteWorkSession &s, ReferenceApplicationAdapters *runtime,
                 const ReferenceRuntimeLimits &l) {
    if (!runtime)
        refuse(ReferenceGate::not_ready, "reference.runtime-unavailable.v1");
    auto head = SqliteApplicationBundleStore(s, bundle_limits(l)).read_state();
    if (!head)
        refuse(ReferenceGate::not_ready, "reference.runtime-unavailable.v1");
    auto r = runtime->current(*head);
    if (auto u = std::get_if<ReferenceUnavailable>(&r))
        throw Refuse{*u};
}
inline std::string draw(ReferenceEntropy &entropy, std::string_view domain,
                        const ReferenceRuntimeLimits &l) {
    require(l.values.reference_bytes >= 36, Error::result_limit);
    auto e = borrow([&] { return entropy.read32(); });
    auto p = measure(l.values, Error::result_limit);
    p.p.fixed(41);
    if (e.error != ReferenceEntropyError::none || e.received != 32)
        refuse(ReferenceGate::not_ready, "reference.entropy.v1");
    std::string out(domain);
    out.append(reinterpret_cast<const char *>(e.bytes.data()), 32);
    return out;
}
using reference_runtime_detail::fields;
struct ReceiptValue {
    core::ReceiptIntakeResult receipt;
    ReferenceArrivalCapture original;
};
#define IF(T, ...)                                                                                 \
    template <class P> void fields(P &p, const T &v) { p(__VA_ARGS__); }
IF(ReceiptValue, v.receipt, v.original)
#undef IF
struct OutputMeasure : Measure {
    using Measure::one;
    void one(const ReceiptValue &v) {
        one(v.receipt);
        one(v.original);
    }
    void one(const core::ReceiptIntakeResult &v) {
        one(v.receipt_id.value);
        one(v.disposition);
    }
    void one(const ContributionLinkResult &v) {
        one(v.contribution_id);
        one(v.disposition);
    }
    void one(const ReferenceInitialCapture &v) {
        p(v.key.installation_ref, v.key.source, v.receipt_id.value, v.contribution_id.value,
          v.configuration_ref, v.arrival_context_ref, v.event_ref, v.plan, v.execution_context_ref);
    }
    template <class... T> void operator()(const T &...v) { (one(v), ...); }
};
inline OutputMeasure output_measure(const ReferenceRuntimeLimits &l) {
    return {measure(l.values, Error::result_limit)};
}
struct Lease {
    std::shared_ptr<std::atomic<std::uint64_t>> count;
    std::atomic<bool> released = false;
    explicit Lease(std::shared_ptr<std::atomic<std::uint64_t>> c) : count(std::move(c)) {
        ++*count;
    }
    void release() {
        if (!released.exchange(true))
            --*count;
    }
    ~Lease() { release(); }
};
template <class P, class C, class F, class Publish>
ReferencePreparedWork
pair(const ReferenceRuntimeLimits &l, std::shared_ptr<std::atomic<std::uint64_t>> count,
     std::function<void(C)> callback, F fn, Publish publish, std::uint64_t presences = 2,
     ReferenceApplicationAdapters *runtime = nullptr) {
    require(count->load() < l.pending_reservations, Error::input_limit);
    auto lease = std::make_shared<Lease>(count);
    struct State {
        std::optional<P> expected;
        std::optional<ReferenceUnavailable> diagnosis;
        std::atomic<bool> started = false, completed = false;
    };
    auto state = std::make_shared<State>();
    return {[=](SqliteWorkContext context) -> QVariant {
                (void)lease;
                require(!state->started.exchange(true), Error::invalid_binding);
                const auto session = context.begin();
                Cleanup cleanup{session};
                auto project = [&](const P &value) {
                    auto p = output_measure(l);
                    p.p.fixed(3 + presences);
                    p(value, value);
                };
                try {
                    auto result = fn(session, project, cleanup);
                    state->expected = result.template value<P>();
                    return result;
                } catch (const Refuse &r) {
                    if (!cleanup.owned)
                        throw;
                    session.rollback();
                    auto p = measure(l.values, Error::result_limit);
                    try {
                        p.p.fixed(3 + presences);
                        p(r.value);
                        state->diagnosis = r.value;
                    } catch (const ReferenceInputFailure &) {
                    }
                    return {};
                } catch (const ReferenceInputFailure &e) {
                    if (e.code() != Error::result_limit || !cleanup.owned)
                        throw;
                    session.rollback();
                    try {
                        auto p = measure(l.values, Error::result_limit);
                        p.p.fixed(3 + presences);
                        ReferenceUnavailable u{ReferenceGate::not_ready,
                                               "reference.result-limit.v1"};
                        p(u);
                        state->diagnosis = std::move(u);
                    } catch (const ReferenceInputFailure &) {
                    }
                    return {};
                } catch (const DependencyAllocation &e) {
                    std::rethrow_exception(e.original);
                } catch (const std::bad_alloc &) {
                    fail(Error::allocation_failure);
                } catch (const std::length_error &) {
                    fail(Error::allocation_failure);
                }
            },
            [=](SqliteWorkResult work) {
                require(!state->completed.exchange(true), Error::invalid_binding);
                C out{};
                out.work = work;
                // Intake cannot advance an existing application anchor. A known
                // commit with a body error suppresses output, but creates no
                // application reservation to reconcile. Unknown still stops it.
                if (runtime && work.transaction == TransactionOutcome::outcome_unknown) {
                    ApplicationServiceCompletion halted;
                    halted.transaction = work.transaction;
                    halted.work_error = work.error;
                    runtime->settle(halted, {});
                }
                if (work.transaction == TransactionOutcome::committed &&
                    work.error == WorkError::none && work.value &&
                    work.value->template canConvert<P>() && state->expected) {
                    auto held = work.value->template value<P>();
                    if (equal(held, *state->expected))
                        publish(out, held);
                }
                if (work.transaction == TransactionOutcome::rolled_back &&
                    work.error == WorkError::not_committed && !work.value)
                    out.refusal = state->diagnosis;
                lease->release();
                callback(std::move(out));
            }};
}
// Commit/cleanup ownership transfers only after actual begin succeeds.
struct ReceiptOwnerRepository final : ReceiptRepository {
    ReceiptRepository &repository;
    Cleanup &cleanup;
    ReceiptOwnerRepository(ReceiptRepository &r, Cleanup &c) : repository(r), cleanup(c) {}
    std::unique_ptr<ReceiptTransaction> begin() override {
        auto p = repository.begin();
        cleanup.owned = false;
        return p;
    }
};
struct ContributionOwnerRepository final : ContributionRepository {
    ContributionRepository &repository;
    Cleanup &cleanup;
    ContributionOwnerRepository(ContributionRepository &r, Cleanup &c)
        : repository(r), cleanup(c) {}
    std::unique_ptr<ContributionTransaction> begin() override {
        auto p = repository.begin();
        cleanup.owned = false;
        return p;
    }
};
} // namespace subathon::server::reference_intake_detail
