// SPDX-License-Identifier: GPL-3.0-or-later
#include "reference_capture_codec_p.hpp"
#include "reference_validation_internal.hpp"
namespace subathon::server {
namespace {
using namespace reference_detail;
void validate(const std::string &ref, const std::vector<ReferenceConfigurationVersion> &v,
              const ReferenceAdapterLimits &l) {
    valid_limits(l);
    auto p = projection(l);
    p(ref);
    require(v.size() <= l.value_items, Error::input_limit);
    p.items += v.size();
    for (const auto &x : v)
        p(x.reference, x.contents);
    Semantics tags{true}, refs{false};
    for (const auto &x : v)
        tags(x.contents);
    refs(ref);
    for (const auto &x : v)
        refs(x.reference, x.contents);
    for (const auto &x : v)
        facts_shape(x.contents);
    for (const auto &x : v)
        facts_numeric(x.contents);
    for (std::size_t i = 0; i < v.size(); ++i) {
        facts_binding(v[i].reference, v[i].contents);
        for (std::size_t j = 0; j < i; ++j)
            require(v[i].reference != v[j].reference, Error::invalid_binding);
    }
}
void affinity(std::thread::id owner) {
    if (owner != std::this_thread::get_id())
        throw SqliteWorkFailure(WorkError::wrong_thread);
}
} // namespace
ReferenceConfigurationArchive::ReferenceConfigurationArchive(
    std::vector<ReferenceConfigurationVersion> versions, ReferenceAdapterLimits limits)
    : versions_(std::move(versions)), limits_(limits) {}
ReferenceConfigurationResolution ReferenceConfigurationArchive::snapshot(const std::string &ref) {
    validate(ref, versions_, limits_);
    affinity(owner_);
    return allocate([&]() -> ReferenceConfigurationResolution {
        for (const auto &v : versions_)
            if (v.reference == ref) {
                if (const auto *f = supported(v.contents)) {
                    if (auto g = refusal(*f))
                        return unavailable(*g, limits_);
                }
                auto p = projection(limits_, Error::result_limit);
                p(v.contents);
                return v.contents;
            }
        return unavailable(ReferenceGate::not_ready, limits_);
    });
}
ReferenceConfigurationResolution
ReferenceConfigurationArchive::load(const ApplicationReferenceReadView &view,
                                    const std::string &ref) {
    validate(ref, versions_, limits_);
    // This actual finite operation checks the view BEFORE archive affinity.
    auto key_size = projection(limits_, Error::result_limit);
    key_size(RetainedKind::configuration, ref);
    const auto key = allocate([&] { return RetainedKey{RetainedKind::configuration, ref}; });
    const auto row = view.find_retained(key);
    affinity(owner_);
    auto selected = snapshot(ref);
    if (!row)
        return unavailable(ReferenceGate::not_ready, limits_);
    auto p = projection(limits_, Error::result_limit);
    p(row->key.kind, row->key.reference);
    p.fixed(1);
    if (row->configuration)
        p(row->configuration->reference);
    require(std::uint64_t(row->document.size()) <= limits_.document_bytes, Error::result_limit);
    p.fixed(std::uint64_t(row->document.size()));
    data(row->key == key && !row->configuration);
    const auto decoded = reference_capture_detail::decode_configuration_body(
        {row->document.constData(), std::size_t(row->document.size())}, limits_,
        reference_capture_detail::BodyBounds::retained);
    if (const auto *f = std::get_if<ReferenceConfigurationFacts>(&selected))
        data(same(decoded, f->projection));
    return selected;
}
} // namespace subathon::server
