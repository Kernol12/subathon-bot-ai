// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <subathon/server/sqlite_application_bundle.hpp>
#include <subathon/server/sqlite_application_holds.hpp>
#include <variant>

namespace subathon::server::selected_record_read {
// Private borrowed-session contract, revision 1 (ACCESS-011 supplemental examples).
// Independent of reference authority. These are selected structural reads, not
// authorization, current policy, execution permits or acknowledged durability.
enum class Failure { missing, invalid_data, resource_limit };
using ApplicationResult = std::variant<RecordedApplicationBundle, Failure>;
using HoldResult = std::variant<ApplicationHoldView, Failure>;

// Validate limits and the complete borrowed key before SQL/ownership, using the
// owning bundle/hold input exceptions for invalid options or keys. A missing
// selected root returns missing; missing/corrupt REQUIRED selected children,
// counts, envelopes or duplicated bindings return invalid_data. Selected byte,
// item, numeric-component or aggregate output overage returns resource_limit.
// No partial value. Every original SqliteWorkFailure escapes UNCHANGED, including
// nested retained/plan/state reads; never translate it to a local failure.
//
// Before ANY unbounded payload or output ownership: plan the COMPLETE selected
// closure from scalar types, length(CAST(column AS BLOB)) and bounded counts;
// check every field and checked aggregate/nested expansion against the supplied
// existing limits and container capacities. Wrong-class TEXT is counted in BYTES.
// Decode admitted fields with the owning strict decoders and preserve all selected
// record invariants. Metadata planning is itself bounded by those same limits.
//
// Borrow exactly this caller's revocable session. No rollback/cleanup, BEGIN,
// commit, writes, new connection, nested work, retry or sticky error cache. A local
// failure leaves the SAME ticket active; only its caller decides transaction fate.
// Values are provisional until caller COMMIT AND normal work completion. Existing
// public storage facades retain their own API, behavior and failure policies.

// Exact key lookup, complete RecordedApplicationBundle including selected state,
// contribution, effects/components, jobs, transitions, reversal provenance,
// result/audit and owning immutable source/plan/configuration closure. Do not
// substitute today's candidate/current head for recorded historical snapshots.
[[nodiscard]] ApplicationResult read_application(const SqliteWorkSession &session,
                                                 const ApplicationOperationKey &operation,
                                                 const SqliteBundleLimits &limits);

// Exact hold lookup: complete cause + source/plan/state evidence, first detection
// and first version, current version and its finite directly referenced producer
// event (including bounded resolution batch/replacements if applicable). Validate
// the directly referenced current parent version's scalar linkage. Initial/current
// and direct-parent/producer locators use indexed finite seeks, not lifetime
// version scans or supersession-lineage traversal. Supersedes is retained as a
// reference, not recursively expanded. Deduplicate repeated selected locators in
// the plan while charging repeated RETURNED ownership under the existing limits.
// Do not claim that unselected intermediate versions/events/ancestors are valid.
[[nodiscard]] HoldResult read_hold(const SqliteWorkSession &session, const std::string &hold_ref,
                                   const SqliteHoldLimits &limits);
} // namespace subathon::server::selected_record_read
