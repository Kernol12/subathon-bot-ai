// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QByteArray>
#include <QString>
#include <functional>
#include <memory>
#include <optional>
#include <subathon/core/effective_permissions.hpp>
#include <subathon/server/connector_lifecycle.hpp>
#include <subathon/server/sqlite_writer.hpp>
#include <variant>
#include <vector>

namespace release_002_stage1 {

// Private test composition only. A scope identifies the current writer and session
// in this fixture. Production obtains both through its protected account boundary.
enum class Gate { preparation, handoff };
struct SessionScope {
    std::string installation;
    std::string actor;
    std::string session;
    bool operator==(const SessionScope &) const = default;
};
struct SessionObservation {
    SessionScope scope;
    // From a guarded, current command scope distinct from the operational clock.
    std::int64_t guarded_utc_ms;
    std::int64_t expires_utc_ms;
};
enum class Use { allowed, denied, unavailable };
enum class Field { disposition, schema_version };
struct Selector {
    std::uint64_t receipt;
    std::int64_t attempt;
    std::vector<Field> fields;
    bool operator==(const Selector &) const = default;
};
using AttemptValue = std::variant<subathon::core::ReceiptDisposition, std::uint32_t>;
struct AttemptRead {
    subathon::server::SqliteWorkResult completion;
    // On a normal acknowledgement, nullopt denotes a definite miss. Values
    // follow requested field order and carry no other receipt data.
    std::optional<std::vector<AttemptValue>> values;
};

struct VersionTuple {
    unsigned major, minor, patch;
};
struct VersionObservations {
    std::optional<VersionTuple> server;
    std::optional<VersionTuple> qt_runtime;
};
enum class Mutation { append, unlink, repair };
enum class MutationOutcome { changed, unchanged_refusal, uncertain };
enum class LogNotice { retention_unavailable, capacity_reached };

struct Ports {
    std::function<std::optional<std::int64_t>()> log_utc;
    std::function<VersionObservations()> versions;
    std::function<std::optional<SessionObservation>(Gate, bool require_fresh)> current_session;
    std::function<Use(Gate, const SessionScope &, const subathon::core::PermissionRequest &)>
        current_use;
    std::function<AttemptRead(Gate, const SessionScope &, const Selector &)> read_attempt;
    // Test scheduler may hold a completion once the owner declares a mutation
    // pending, or supply a fault outcome instead of invoking the file operation.
    std::function<void(Mutation, std::function<MutationOutcome()>,
                       std::function<void(MutationOutcome)>)>
        mutate;
};

enum class ReplyStatus { artifact, invalid_selection, denied, selection_unavailable };
struct Reply {
    ReplyStatus status;
    QByteArray artifact;
};
using Transfer = std::function<void(Reply)>;
struct ExportId {
    std::uint64_t value;
};
using AdapterId = unsigned;

// Internal negative-shape control. The enum represents the presence of
// disallowed synthetic members. It carries no bytes from a request or provider.
enum class ExtraMembers { header_identifier_and_message };

class Stage1Boundary {
  public:
    virtual ~Stage1Boundary() = default;
    virtual void startup() = 0;
    virtual void sweep() = 0;
    virtual void host_entered_serving() = 0;
    virtual void register_adapter(AdapterId, subathon::server::ConnectorProvider) = 0;
    virtual void report(AdapterId, const subathon::server::ConnectorAdapter &,
                        const subathon::server::ConnectorSetupContext & = {}) = 0;
    virtual void submit_connector_candidate(AdapterId, ExtraMembers) = 0;
    // One deterministic owner turn for queued producer observations.
    virtual void drain_observations() = 0;
    virtual std::vector<LogNotice> pending_notices() const = 0;

    // A present request is bounded selector JSON only. The fixture exercises
    // local selection validation with no transport or bearer serialization.
    virtual ExportId prepare(std::optional<QByteArray> selector_json) = 0;
    // Take the real final gate in the same controlled step as invoking transfer.
    // No callback means withheld. A ticket cannot be used to transfer twice.
    virtual void handoff(ExportId, const Transfer &transfer) = 0;
};

std::unique_ptr<Stage1Boundary> make_stage1_boundary(QString private_log_directory, Ports ports);

} // namespace release_002_stage1
