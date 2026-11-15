// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/reference_application.hpp>

namespace subathon::server {
// API-011 public contract revision 1. DATA supplied by the trusted configuration
// owner at queue construction, not SRC1, connector input or an admission permit.
// Every version contains the ENTIRE unprojected enabled configuration and its
// immutable owner revision. Unknown categories remain unknown. Unrepresentable
// known shapes use an explicit unsupported resolution, never a narrow placeholder.
struct ReferenceConfigurationVersion {
    std::string reference;
    ReferenceConfigurationResolution contents;
};
class ReferenceNotImplemented final : public std::logic_error {
  public:
    explicit ReferenceNotImplemented(const char *method) : std::logic_error(method) {}
};
// Inert constructor moves owned startup versions; no validation/admission occurs
// until a method. No mutation/editor, pruning or current-version substitution API.
// Owner thread = construction thread; all calls, including refusals, check affinity.
// snapshot/load validate the WHOLE archive (including later/losing versions) before
// selecting. Duplicate version refs or mismatched ref/facts/projection are binding
// failures; duplicate refs within facts are shape failures. Owner revisions nonempty.
// Validate all fields/tags/numbers even after known unsupported/unknown members.
// Use RULES-006 admission priority and preserve unknown versus known unsupported.
// Missing exact version returns not_ready/reference.configuration-unavailable.v1.
// Retained historical versions never become today's rules. Returned facts own bytes.
// load additionally performs an actual finite find_retained(configuration,ref),
// compares admitted SRC1 byte/value fields and link absence; a missing retained row
// is unavailable, known contradictory data is ReferenceExecutionFailure(invalid_data).
// A valid request on default/copied-expired/foreign view preserves the ORIGINAL
// ApplicationReferenceViewFailure; invalid input/limits win BEFORE view access.
// Original SQL/retention errors propagate; this source never cleans/commits work.
// Resources: inherited ReferenceAdapterLimits apply independently to complete
// (requested_ref + archive) input, selected output and every codec intermediate.
// B(string/bytes)=length; B(enum/bool)=1; B(i64/u64)=8; B(optional)=1+present;
// B(struct)=sum ALL fields/copies; variant=held alternative only; vector I=size+
// sum nested I, B=sum nested B. Version adds reference+held contents; versions
// vector includes every version and every nested enabled rule/category member.
// Limits/dependency objects uncharged. Every ref/trigger/category/shape string uses
// reference_bytes; payload/document uses its field ceiling. BEFORE any copy/hash/
// numeric/codec allocation: limits positive, representable size_t/ptrdiff_t/
// qsizetype/qint64 and container max_size (n+1); checked U32 framing; whole B/I,
// tags, refs, shape, numeric, bindings, profile, result projection, allocation.
// Input excess=input_limit, output excess=result_limit, own allocation failure=
// allocation_failure via ReferenceInputFailure. Pairwise duplicate work O(I^2*R),
// full scan O(B+I), fixed depth. No eager clone/sort. No SQL outside finite view.
// Affinity failure is SqliteWorkFailure(wrong_thread), after pure input priority.
class ReferenceConfigurationArchive final : public ReferenceConfigurationSource {
  public:
    explicit ReferenceConfigurationArchive(std::vector<ReferenceConfigurationVersion>,
                                           ReferenceAdapterLimits = {});
    ReferenceConfigurationResolution snapshot(const std::string &) override;
    ReferenceConfigurationResolution load(const ApplicationReferenceReadView &,
                                          const std::string &) override;

  private:
    std::vector<ReferenceConfigurationVersion> versions_;
    ReferenceAdapterLimits limits_;
};
} // namespace subathon::server
