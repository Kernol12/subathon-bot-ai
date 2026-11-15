// SPDX-License-Identifier: GPL-3.0-or-later
#include "application_service_command_internal.hpp"
#include "reference_capture_codec_p.hpp"
#include "reference_validation_internal.hpp"
#include <QCryptographicHash>
#include <array>
#include <charconv>
#include <subathon/server/reference_application.hpp>
#include <tuple>

namespace subathon::server {
namespace {
using namespace reference_detail;
ReferenceRuleEvaluation derive(const ReferenceRule &r, const Limits &l) {
    // Preproject the adapter's intermediate core input before its owned strings.
    auto p = projection(l);
    p(r.rule_ref, r.rule_version_ref, r.action_ref, r.action_version_ref, r.timer_ref);
    p.fixed(9);
    p(std::string_view("synthetic.reference.support.v1"),
      std::string_view("synthetic.reference.support.v1"), std::string_view("time_add"),
      std::string_view("ms"));
    ReferenceRuleInput in{r.rule_ref,
                          r.rule_version_ref,
                          r.action_ref,
                          r.action_version_ref,
                          r.timer_ref,
                          true,
                          "synthetic.reference.support.v1",
                          "synthetic.reference.support.v1",
                          "time_add",
                          "ms",
                          r.add_ms};
    try {
        auto out =
            derive_reference_rule(in, {l.reference_bytes, l.value_owned_bytes, l.value_items});
        return {std::move(out.selection), std::move(*out.request)};
    } catch (const ReferenceDerivationFailure &e) {
        switch (e.code()) {
        case ReferenceDerivationError::invalid_limits:
            fail(Error::invalid_limits);
        case ReferenceDerivationError::input_limit:
            fail(Error::input_limit);
        case ReferenceDerivationError::invalid_reference:
            fail(Error::invalid_reference);
        case ReferenceDerivationError::invalid_value:
            fail(Error::invalid_value);
        case ReferenceDerivationError::result_limit:
            fail(Error::result_limit);
        case ReferenceDerivationError::allocation_failure:
            fail(Error::allocation_failure);
        }
        throw;
    }
}
} // namespace
ReferenceAdmissionResolution
admit_reference_configuration(const std::string &expected,
                              const ReferenceConfigurationResolution &r, const Limits &l) {
    valid_limits(l);
    auto p = projection(l);
    p(expected, r);
    Semantics tags{true};
    tags(expected, r);
    Semantics refs{false};
    refs(expected, r);
    facts_shape(r);
    facts_numeric(r);
    facts_binding(expected, r);
    return allocate([&]() -> ReferenceAdmissionResolution {
        const auto *f = supported(r);
        if (!f)
            return copy_refusal(std::get<ReferenceUnavailable>(r), l);
        if (auto g = refusal(*f))
            return unavailable(*g, l);
        return admitted(*f, l);
    });
}
ReferenceRuleResolution evaluate_reference_rule(const ReferenceArrivalCapture &a,
                                                const ReferenceConfigurationResolution &r,
                                                const Limits &l) {
    valid_limits(l);
    auto p = projection(l);
    p(a, r);
    Semantics tags{true};
    tags(a, r);
    Semantics refs{false};
    refs(a, r);
    arrival_shape(a);
    facts_shape(r);
    numeric(a.configuration);
    facts_numeric(r);
    binding(a.configuration.rule);
    facts_binding(a.configuration.configuration_ref, r);
    return allocate([&]() -> ReferenceRuleResolution {
        const auto *f = supported(r);
        if (!f)
            return copy_refusal(std::get<ReferenceUnavailable>(r), l);
        if (auto g = refusal(*f))
            return unavailable(*g, l);
        const auto config = admitted(*f, l);
        require(same(a.configuration, config.configuration), Error::invalid_binding);
        return derive(a.configuration.rule, l);
    });
}
namespace {
using AuthorityFailure = ReferenceAuthorityFailure;
AccessDecision authority_failure(AuthorityFailure f) {
    switch (f) {
    case AuthorityFailure::denied:
        return ApplicationAccessDenied{};
    case AuthorityFailure::missing:
    case AuthorityFailure::unavailable:
    case AuthorityFailure::resource_limit:
        return ApplicationAccessUnknown{};
    default:
        corrupt();
    }
}
void authority_limits(const ReferenceAuthorityLimits &l) {
    reference_authority::valid_limits(l);
    // The imported validator checks arithmetic (including the permission L^4
    // bound). Check the independently configurable container ceilings as well.
    const auto invalid = [](bool b) {
        if (b)
            throw ReferenceAuthorityInputFailure(AuthorityFailure::invalid_data);
    };
    invalid(l.graph.rows >= std::vector<StoredAccountFacts>{}.max_size());
    invalid(l.graph.items >= std::vector<PermissionPolicyEntry>{}.max_size());
    invalid(l.result_items >= std::vector<PermissionPolicyEntry>{}.max_size());
    invalid(l.permissions.result_items >= std::vector<PermissionSourceExplanation>{}.max_size());
}
void command_preflight(const OriginalApplicationCommand &c, const Limits &l) {
    // The owning service codec applies its default conditional/shape/wire checks
    // after this complete borrowed pass, before any command envelope is built.
    auto p = projection(l);
    p(c.operation, c.subject_kind, c.receipt_id.value);
    p.fixed(1);
    if (c.contribution_id)
        p(c.contribution_id->value);
    p(c.plan, c.principal, c.purpose, c.preview_mode, c.preview_state_token, c.actor_ref,
      c.decision_ref);
    if (c.reason) {
        p.fixed(1);
        require(c.reason->size() <= l.document_bytes, Error::input_limit);
        p.fixed(c.reason->size());
    } else
        p.fixed(1);
    require(c.choices.size() <= c.choices.max_size() &&
                c.choices.capacity() <= c.choices.max_size() && c.choices.size() <= p.item_cap,
            Error::input_limit);
    p.items = c.choices.size();
    for (const auto &v : c.choices) {
        p(v.format_ref);
        require(std::uint64_t(v.document.size()) <= l.document_bytes, Error::input_limit);
        p.fixed(std::uint64_t(v.document.size()));
    }
}
void command_envelope_preflight(const OriginalApplicationCommand &c, const Limits &l) {
    // Size the service's SAC1 intermediate without building a command or buffer.
    // The imported codec still validates meanings and its own defaults afterward.
    auto value = projection(l, Error::result_limit);
    value(std::string_view("subathon.internal.application-command.v1"));
    const auto body_cap = std::min(l.document_bytes, std::uint64_t{UINT32_MAX});
    std::uint64_t body = 0;
    const auto fixed = [&](std::uint64_t n) {
        require(body <= body_cap && n <= body_cap - body, Error::result_limit);
        body += n;
        value.fixed(n);
    };
    const auto blob = [&](std::uint64_t n) {
        require(n <= UINT32_MAX, Error::result_limit);
        fixed(4);
        fixed(n);
    };
    fixed(4); // SAC1
    blob(c.operation.operation_scope.size());
    blob(c.operation.operation_ref.size());
    fixed(1 + 8 + 1); // subject, receipt, contribution presence
    if (c.contribution_id)
        fixed(8);
    blob(c.plan.plan_ref.size());
    fixed(8 + 1); // plan revision, principal kind
    blob(c.principal.installation_ref.size());
    blob(c.principal.principal_ref.size());
    fixed(1 + 1); // purpose, preview mode
    for (const auto *v : {&c.preview_state_token, &c.actor_ref, &c.decision_ref, &c.reason}) {
        fixed(1);
        if (*v)
            blob((*v)->size());
    }
    require(c.choices.size() <= UINT32_MAX, Error::result_limit);
    fixed(4);
    for (const auto &v : c.choices) {
        blob(v.format_ref.size());
        blob(std::uint64_t(v.document.size()));
    }
    // The complete envelope has no vector items: choices are framed body bytes.
}
std::optional<RetainedDocument> read(const ApplicationReferenceReadView &v, RetainedKind kind,
                                     const std::string &ref, const Limits &l) {
    auto p = projection(l);
    p(kind, ref); // before constructing the owning key
    auto key = allocate([&] { return RetainedKey{kind, ref}; });
    auto out = v.find_retained(key); // Original reader/work/view exceptions escape.
    if (out) {
        auto result = projection(l, Error::result_limit);
        result(out->key.kind, out->key.reference);
        require(std::uint64_t(out->document.size()) <= l.document_bytes &&
                    std::uint64_t(out->document.size()) <= UINT32_MAX,
                Error::result_limit);
        result.fixed(std::uint64_t(out->document.size()));
        result.fixed(1);
        if (out->configuration)
            result(out->configuration->reference);
        data(out->key == key && !out->document.isNull());
    }
    return out;
}
std::string_view bytes(const QByteArray &b) { return {b.constData(), std::size_t(b.size())}; }
std::string execution_ref(const OriginalApplicationCommand &c, const Limits &l) {
    data(c.principal.kind == ApplicationPrincipalKind::automation &&
         c.subject_kind == ApplicationSubjectKind::contribution && c.contribution_id &&
         c.purpose == OriginalApplicationPurpose::captured_original && c.choices.size() == 1);
    const auto &choice = c.choices[0];
    data(choice.format_ref == "subathon.reference.execution-ref.v1");
    const auto b = bytes(choice.document);
    data(!choice.document.isNull() && b.size() >= 8 && b.substr(0, 4) == "SRF1");
    std::uint32_t length = 0;
    for (unsigned char x : b.substr(4, 4))
        length = (length << 8) | x;
    data(b.size() - 8 == length && length != 0);
    auto p = projection(l);
    p(b.substr(8));
    return std::string(b.substr(8));
}
void match_command(const OriginalApplicationCommand &c, const ReferenceExecutionBinding &x) {
    data(same(c.principal, x.rule.automation) &&
         c.principal.installation_ref == x.installation_ref && c.receipt_id == x.receipt_id &&
         c.contribution_id && *c.contribution_id == x.contribution_id && same(c.plan, x.plan));
}
// Full, raw catalog equality against the selected compiled reference profile.
// Constants are borrowed; no temporary owning catalog precedes nested preflight.
bool reference_catalog(const PermissionCatalog &c) {
    constexpr std::array<std::string_view, 10> names{
        "reference.time-add.v1",  "reference.application-read.v1",
        "reference.hold-read.v1", "roles.edit",
        "members.edit",           "overrides.edit",
        "access.manage",          "owner.recover",
        "helper.reset",           "timer.snapshot.read"};
    if (c.state != PermissionFactsState::complete || c.revision != "permission-reference-v1" ||
        c.operations.size() != names.size())
        return false;
    for (std::size_t i = 0; i < names.size(); ++i) {
        const auto &o = c.operations[i];
        const auto classification = i == 6   ? PermissionOperationClass::protected_access_management
                                    : i == 7 ? PermissionOperationClass::protected_recovery
                                    : i == 8 ? PermissionOperationClass::helper_password_reset
                                             : PermissionOperationClass::ordinary;
        if (o.operation != names[i] || o.classification != classification)
            return false;
        if (i < 3) {
            if (o.resource_slots.size() != 3 || o.resource_slots[0] != "installation" ||
                o.resource_slots[1] != "timer" || o.resource_slots[2] != "source")
                return false;
        } else if (i == 6 || i == 7) {
            if (!o.resource_slots.empty())
                return false;
        } else {
            const auto slot = i == 3 ? "role" : i == 8 ? "target" : i == 9 ? "timer" : "account";
            if (o.resource_slots.size() != 1 || o.resource_slots[0] != slot)
                return false;
        }
        if (i == 8) {
            if (!o.helper_target_slot || *o.helper_target_slot != "target")
                return false;
        } else if (o.helper_target_slot)
            return false;
        if (i != 0) {
            if (!o.quantity_slots.empty())
                return false;
        } else {
            if (o.quantity_slots.size() != 1)
                return false;
            const auto &q = o.quantity_slots[0];
            if (q.slot != "adjustment" || q.unit != "ms" ||
                q.kind != PermissionNumberKind::integer || q.allowed_modes != 15 || !q.minimum ||
                q.minimum->numerator != "0" || q.minimum->denominator != "1" || !q.maximum ||
                q.maximum->numerator != "9223372036854775807" || q.maximum->denominator != "1")
                return false;
        }
    }
    return true;
}
void registry_binding(const ReferenceAuthorityRecord &r, const ReferenceAuthorityRequest &q) {
    const auto &b = r.binding;
    const auto &x = q.captured;
    const auto &rule = x.rule;
    data(b.installation == x.installation_ref && b.capability == rule.automation_capability_ref &&
         b.principal == q.principal.principal_ref && b.rule == rule.rule_ref &&
         b.rule_version == rule.rule_version_ref && b.action == rule.action_ref &&
         b.action_version == rule.action_version_ref && b.timer == rule.timer_ref &&
         b.origin == x.source.origin && b.identifier_namespace == x.source.identifier_namespace &&
         b.requested_ms == rule.add_ms && b.requested_ms >= 0 && !r.original_author.empty() &&
         !r.execution_account.empty() && !r.actor.empty() && !r.session.empty() &&
         !r.session_revision.empty() && r.created_revision != 0 &&
         r.created_revision <= r.state_revision && r.state_revision <= r.head_revision &&
         unsigned(r.state) <= unsigned(ReferenceAuthorityState::revoked));
}
void fingerprint(const ReferenceAuthorityRecord &r, std::string_view expected) {
    data(expected.size() == 36 && expected.substr(0, 4) == "RAA1");
    QCryptographicHash hash(QCryptographicHash::Sha256);
    auto raw = [&](std::string_view b) {
        hash.addData(QByteArrayView(b.data(), qsizetype(b.size())));
    };
    auto integer = [&](std::uint64_t value, unsigned width) {
        std::array<char, 8> b{};
        for (unsigned i = 0; i < width; ++i)
            b[width - i - 1] = char((value >> (8 * i)) & 255);
        raw({b.data(), width});
    };
    auto field = [&](std::string_view b) {
        data(b.size() <= UINT32_MAX);
        integer(b.size(), 4);
        raw(b);
    };
    raw("RAA1");
    field(r.binding.installation);
    integer(r.head_revision, 8);
    field(r.binding.capability);
    integer(r.created_revision, 8);
    integer(r.state_revision, 8);
    integer(unsigned(r.state), 1);
    field(r.original_author);
    field(r.execution_account);
    auto digest = hash.resultView();
    data(expected.substr(4) == std::string_view(digest.data(), std::size_t(digest.size())));
}
std::uint64_t source_size(const ReferenceAuthorityBinding &b, const ReferenceAuthorityLimits &l) {
    const auto ceiling = std::min(l.reference_bytes, l.permissions.reference_bytes);
    require(ceiling >= 16, Error::result_limit);
    std::uint64_t n = 16;
    for (std::string_view s :
         {std::string_view(b.origin.provider), std::string_view(b.origin.source_account),
          std::string_view(b.identifier_namespace)}) {
        require(s.size() <= UINT32_MAX && s.size() <= ceiling - n, Error::result_limit);
        n += s.size();
    }
    require(n < std::string{}.max_size(), Error::result_limit);
    return n;
}
bool source_scope(std::string_view actual, const ReferenceAuthorityBinding &b, std::uint64_t n) {
    if (actual.size() != n || actual.substr(0, 4) != "RSS1")
        return false;
    std::size_t at = 4;
    for (std::string_view s :
         {std::string_view(b.origin.provider), std::string_view(b.origin.source_account),
          std::string_view(b.identifier_namespace)}) {
        std::uint32_t length = 0;
        for (unsigned char x : actual.substr(at, 4))
            length = (length << 8) | x;
        at += 4;
        if (length != s.size() || actual.substr(at, s.size()) != s)
            return false;
        at += s.size();
    }
    return true;
}
void action_scope(const ReferenceAuthorityFacts &f, const ReferenceAuthorityLimits &l,
                  ReferenceAuthorityUse use) {
    const auto &b = f.registry.binding;
    const auto n = source_size(b, l); // checked RSS1 before any construction/numeric work
    std::array<char, 20> decimal{};
    const auto end = std::to_chars(decimal.data(), decimal.data() + decimal.size(), b.requested_ms);
    const std::string_view amount(decimal.data(), std::size_t(end.ptr - decimal.data()));
    require(amount.size() <= l.permissions.component_bytes, Error::result_limit);
    const auto rational = [&](const PermissionRational &r) {
        return r.numerator == amount && r.denominator == "1";
    };
    const auto resource = [&](std::string_view slot, std::string_view value) {
        if (slot == "installation")
            return value == b.installation;
        if (slot == "timer")
            return value == b.timer;
        return slot == "source" && source_scope(value, b, n);
    };
    const auto &request = f.permission_input.request;
    const bool executing = use == ReferenceAuthorityUse::execute_time_add;
    const auto operation = executing ? "reference.time-add.v1"
                           : use == ReferenceAuthorityUse::disclose_application
                               ? "reference.application-read.v1"
                               : "reference.hold-read.v1";
    data(request.operation == operation && request.resources.size() == 3 &&
         request.quantities.size() == (executing ? 1u : 0u));
    for (std::size_t i = 0; i < request.resources.size(); ++i) {
        const auto &r = request.resources[i];
        data(resource(r.slot, r.resource));
        for (std::size_t j = 0; j < i; ++j)
            data(r.slot != request.resources[j].slot);
    }
    if (executing) {
        const auto &q = request.quantities[0];
        data(q.slot == "adjustment" && q.unit == "ms" && rational(q.value));
    }
    data(f.original_author_input.actions.size() == 1);
    const auto &a = f.original_author_input.actions[0];
    data(a.operation == request.operation && a.resources.size() == 3 &&
         a.quantities.size() == (executing ? 1u : 0u));
    for (std::size_t i = 0; i < a.resources.size(); ++i) {
        const auto &r = a.resources[i];
        data(r.mode == PermissionResourceMode::allowed_set && r.allowed.size() == 1 &&
             resource(r.slot, r.allowed[0]));
        for (std::size_t j = 0; j < i; ++j)
            data(r.slot != a.resources[j].slot);
    }
    if (executing) {
        const auto &quantity = a.quantities[0];
        data(quantity.slot == "adjustment" && quantity.unit == "ms" &&
             quantity.mode == PermissionQuantityMode::closed_interval && quantity.lower &&
             quantity.upper && rational(*quantity.lower) && rational(*quantity.upper));
    }
}
void authority_preflight(const ReferenceAuthorityResolution &r, const ReferenceAuthorityLimits &l) {
    Projection all{l.result_bytes,
                   l.result_items,
                   l.reference_bytes,
                   l.result_bytes,
                   0,
                   0,
                   Error::result_limit,
                   l.permissions.component_bytes};
    all(r.value);
    if (const auto *f = std::get_if<ReferenceAuthorityFacts>(&r.value)) {
        // Each imported calculator retains its own WHOLE argument charge, not
        // merely the selected operation/row. This happens before fingerprint work.
        Projection nested{l.permissions.input_owned_bytes,
                          l.permissions.input_items,
                          l.permissions.reference_bytes,
                          0,
                          0,
                          0,
                          Error::result_limit,
                          l.permissions.component_bytes};
        nested(f->original_author_input);
        nested.bytes = nested.items = 0;
        nested(f->permission_input);
    }
}
AccessDecision consume(const ReferenceAuthorityResolution &r, const ReferenceAuthorityRequest &q,
                       const ReferenceAuthorityLimits &l, const Limits &output_limits) {
    authority_preflight(r, l);
    if (const auto *f = std::get_if<AuthorityFailure>(&r.value))
        return authority_failure(*f);
    const auto &f = std::get<ReferenceAuthorityFacts>(r.value);
    data(same(f.binding, q));
    registry_binding(f.registry, q);
    if (f.registry.state != ReferenceAuthorityState::authorized)
        return ApplicationAccessDenied{};
    const auto &a = f.original_author_input;
    const auto &e = f.permission_input;
    data(!f.permission_revision_ref.empty() && !f.automation_revision_ref.empty() &&
         f.permission_revision_ref.size() == 36 &&
         f.permission_revision_ref.compare(0, 4, "RAR1") == 0 &&
         a.snapshot == f.permission_revision_ref && e.snapshot == f.permission_revision_ref &&
         a.author == f.registry.original_author && e.actor == f.registry.execution_account &&
         a.owner.state == PermissionOwnerState::present && a.owner.account &&
         !a.owner.account->empty() && same(a.owner, e.owner) && reference_catalog(a.catalog) &&
         reference_catalog(e.catalog));
    // Preflight the full RSS1/composite and complete singleton before hash or kernels.
    action_scope(f, l, q.use);
    return allocate([&]() -> AccessDecision {
        fingerprint(f.registry, f.automation_revision_ref);
        const auto author = revalidate_permission_automation(a, l.permissions);
        // Evaluate both independently. A denied first channel does not hide an
        // invalid/unknown second raw policy.
        const auto subject = evaluate_effective_permission(e, l.permissions);
        data(author.actions.size() == 1);
        if (author.actions[0].disposition != PermissionAutomationDisposition::authorized ||
            subject.data().outcome != PermissionOutcome::permit)
            return ApplicationAccessDenied{};
        auto output = projection(output_limits, Error::result_limit);
        output(f.permission_revision_ref);
        return ApplicationAccessAllowed{f.permission_revision_ref};
    });
}
AccessDecision execute(const ApplicationReferenceReadView &view,
                       const OriginalApplicationCommand &c, ReferenceConfigurationSource &configs,
                       ReferenceAuthoritySource &authority, const Limits &l,
                       const ReferenceAuthorityLimits &al) {
    command_preflight(c, l);
    command_envelope_preflight(c, l);
    auto ref = allocate([&] {
        (void)encode_original_application_command(c); // imported complete service validation
        return execution_ref(c, l);
    });
    auto execution_document = read(view, RetainedKind::arrival_context, ref, l);
    if (!execution_document)
        return ApplicationAccessUnknown{};
    data(execution_document->configuration &&
         !execution_document->configuration->reference.empty());
    const auto x = allocate([&] {
        return reference_capture_detail::decode_execution_body(
            bytes(execution_document->document), l, reference_capture_detail::BodyBounds::retained);
    });
    data(x.execution_context_ref == ref);
    match_command(c, x);
    auto arrival_document = read(view, RetainedKind::arrival_context, x.arrival_context_ref, l);
    if (!arrival_document)
        return ApplicationAccessUnknown{};
    data(arrival_document->configuration == execution_document->configuration);
    const auto a = allocate([&] {
        return reference_capture_detail::decode_arrival_body(
            bytes(arrival_document->document), l, reference_capture_detail::BodyBounds::retained);
    });
    const auto &configuration_ref = execution_document->configuration->reference;
    auto configuration_document = read(view, RetainedKind::configuration, configuration_ref, l);
    if (!configuration_document)
        return ApplicationAccessUnknown{};
    data(!configuration_document->configuration);
    const auto cfg = allocate([&] {
        return reference_capture_detail::decode_configuration_body(
            bytes(configuration_document->document), l,
            reference_capture_detail::BodyBounds::retained);
    });
    data(a.arrival_context_ref == x.arrival_context_ref &&
         cfg.configuration_ref == configuration_ref && same(cfg, a.configuration) &&
         same(cfg.rule, x.rule) && cfg.synthetic_origin == x.source.origin &&
         cfg.receipt_namespace == x.source.identifier_namespace &&
         a.observation.event_id == x.source.opaque_id);
    auto complete = configs.load(view, configuration_ref); // trusted producer, no snapshot/cache
    const auto evaluated = evaluate_reference_rule(a, complete, l);
    if (std::holds_alternative<ReferenceUnavailable>(evaluated))
        return ApplicationAccessUnknown{};
    data(std::get<ReferenceRuleEvaluation>(evaluated).request.requested_units == x.rule.add_ms);
    // Project the complete authority request before cloning any capture or command fields.
    Projection p{al.input_bytes, UINT64_MAX, al.reference_bytes, 0};
    p(ReferenceAuthorityUse::execute_time_add, c.principal, x, c.operation);
    p.fixed(2); // absent historical operation/hold
    auto q = allocate([&] {
        return ReferenceAuthorityRequest{ReferenceAuthorityUse::execute_time_add,
                                         c.principal,
                                         x,
                                         c.operation,
                                         std::nullopt,
                                         std::nullopt};
    });
    reference_authority::request_shape(q);
    auto fresh = authority.resolve(view, q); // a NEW full request on EVERY attempt
    return consume(fresh, q, al, l);
}
} // namespace
AccessDecision check_reference_execution(const ApplicationReferenceReadView &view,
                                         const OriginalApplicationCommand &c,
                                         ReferenceConfigurationSource &configs,
                                         ReferenceAuthoritySource &authority, const Limits &l,
                                         const ReferenceAuthorityLimits &al) {
    valid_limits(l);      // Caller options retain the declared input-error boundary.
    authority_limits(al); // Preserve caller errors; only source failures translate below.
    try {
        return execute(view, c, configs, authority, l, al);
    } catch (const ReferenceInputFailure &e) {
        switch (e.code()) {
        case Error::input_limit:
        case Error::result_limit:
            return ApplicationAccessUnknown{};
        case Error::allocation_failure:
            throw;
        default:
            corrupt();
        }
    } catch (const ReferenceAuthorityInputFailure &e) {
        return authority_failure(e.code());
    } catch (const EffectivePermissionFailure &e) {
        switch (e.code()) {
        case EffectivePermissionError::input_limit:
        case EffectivePermissionError::result_limit:
        case EffectivePermissionError::allocation_failure:
        case EffectivePermissionError::facts_unavailable:
            return ApplicationAccessUnknown{};
        default:
            corrupt();
        }
    } catch (const ApplicationServiceInputFailure &e) {
        switch (e.error()) {
        case ApplicationServiceInputError::collection_limit:
        case ApplicationServiceInputError::owned_byte_limit:
        case ApplicationServiceInputError::result_limit:
            return ApplicationAccessUnknown{};
        default:
            corrupt();
        }
    }
    // In particular: no SqliteWorkFailure, RetentionFailure or view exception catch.
    // The actual reader/service owns rollback and preserves its original outcome.
}
namespace reference_detail {
ReferenceExecutionBinding captured_binding(const ApplicationReferenceReadView &view,
                                           const OriginalApplicationCommand &c,
                                           ReferenceConfigurationSource &configs, const Limits &l) {
    command_preflight(c, l);
    command_envelope_preflight(c, l);
    (void)encode_original_application_command(c);
    auto ref = execution_ref(c, l);
    auto row = read(view, RetainedKind::arrival_context, ref, l);
    data(bool(row) && bool(row->configuration));
    auto x = reference_capture_detail::decode_execution_body(
        bytes(row->document), l, reference_capture_detail::BodyBounds::retained);
    match_command(c, x);
    data(x.execution_context_ref == ref);
    auto arrival = read(view, RetainedKind::arrival_context, x.arrival_context_ref, l);
    data(bool(arrival) && arrival->configuration == row->configuration);
    auto a = reference_capture_detail::decode_arrival_body(
        bytes(arrival->document), l, reference_capture_detail::BodyBounds::retained);
    auto cfg = configs.load(view, row->configuration->reference);
    auto evaluated = evaluate_reference_rule(a, cfg, l);
    data(!std::holds_alternative<ReferenceUnavailable>(evaluated) &&
         same(x.rule, a.configuration.rule) && a.arrival_context_ref == x.arrival_context_ref &&
         a.configuration.synthetic_origin == x.source.origin &&
         a.configuration.receipt_namespace == x.source.identifier_namespace &&
         a.observation.event_id == x.source.opaque_id);
    return x;
}
AccessDecision disclose(const ApplicationReferenceReadView &view,
                        const OriginalApplicationCommand &c, ReferenceConfigurationSource &configs,
                        ReferenceAuthoritySource &authority, ReferenceAuthorityUse use,
                        const ApplicationOperationKey &operation,
                        const std::optional<std::string_view> &hold, const Limits &l,
                        const ReferenceAuthorityLimits &al, const ApplicationEnvelope &history) {
    valid_limits(l);
    authority_limits(al);
    auto p0 = projection(l);
    p0(history.format_ref);
    require(std::uint64_t(history.document.size()) <= l.document_bytes, Error::input_limit);
    p0.fixed(std::uint64_t(history.document.size()));
    ApplicationServiceLimits sl;
    sl.reference_bytes = qsizetype(l.reference_bytes);
    sl.storage_document_bytes = qsizetype(l.document_bytes);
    sl.intermediate_owned_bytes = qsizetype(l.value_owned_bytes);
    sl.intermediate_items = qsizetype(l.value_items);
    auto historical = service_detail::decode_retained_command(history, sl);
    data(same(historical.operation, operation) && same(historical.principal, c.principal));
    auto x = captured_binding(view, historical, configs, l);
    // Authority requests retain their OWN input budget, just like execution.
    // Historical keys are borrowed through the call until this complete pass fits.
    Projection p{al.input_bytes, UINT64_MAX, al.reference_bytes, 0};
    p(use, c.principal, x, c.operation);
    p.fixed(1); // present historical operation
    p(operation, hold);
    ReferenceAuthorityRequest q{
        use,         c.principal, std::move(x),
        c.operation, operation,   hold ? std::optional<std::string>(*hold) : std::nullopt};
    auto r = authority.resolve(view, q);
    auto result = projection(l, Error::result_limit);
    result(r.value);
    return consume(r, q, al, l);
}
} // namespace reference_detail
} // namespace subathon::server
