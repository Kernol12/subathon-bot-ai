// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QByteArray>
#include <QByteArrayView>
#include <QString>
#include <functional>
#include <memory>
#include <optional>
#include <subathon/core/effective_permissions.hpp>
#include <subathon/server/connector_lifecycle.hpp>
#include <subathon/server/session_resolution.hpp>
#include <subathon/server/sqlite_permission_graph.hpp>
#include <variant>

namespace subathon::server::diagnostics {

using Utc = std::int64_t;
enum class Field { disposition, schema_version };
struct Selector {
    std::uint64_t receipt;
    std::int64_t attempt;
    std::vector<Field> fields;
};
using AttemptValue = std::variant<core::ReceiptDisposition, std::uint32_t>;
using Attempt = std::optional<std::vector<AttemptValue>>;
struct Scope {
    std::string installation, actor, session;
    bool operator==(const Scope &) const = default;
};
struct Session {
    Scope scope;
    Utc utc, expires;
};
enum class Use { allowed, denied, unavailable };
struct Gate {
    Use use{Use::unavailable};
    std::optional<Scope> scope;
    std::vector<Attempt> attempts;
    std::optional<Utc> expires;
};

// All calls in a gate use one authoritative turn and the same coherent source.
// The transport composition acknowledges the read before returning its result.
class Current {
  public:
    virtual ~Current() = default;
    virtual std::optional<Session> session() = 0;
    virtual Use use(const Scope &, const core::PermissionRequest &) = 0;
    virtual Attempt attempt(const Scope &, const Selector &) = 0;
};
[[nodiscard]] Gate read_gate(Current &, const std::vector<Selector> &,
                             const Scope *prepared_scope = nullptr);
[[nodiscard]] bool normally_acknowledged(const SqliteWorkResult &);
[[nodiscard]] Gate acknowledge_gate(Gate, const SqliteWorkResult &);

// Borrow the host's active writer session, owning bearer lookup and scoped
// guarded clock. Call within a writer body, then acknowledge its real completion.
[[nodiscard]] Gate read_sqlite_gate(const SqliteWorkSession &, const SessionLookupRequest &,
                                    CurrentSessionClock &, const PermissionGraphTrust &,
                                    const std::vector<Selector> &, const Scope *prepared = nullptr);

struct Version {
    unsigned major, minor, patch;
};
struct Versions {
    std::optional<Version> server, qt_runtime;
};
[[nodiscard]] Versions running_versions();

enum class Mutation { append, unlink, repair };
enum class MutationOutcome { changed, unchanged_refusal, uncertain };
enum class Notice { retention_unavailable, capacity_reached };
namespace testing {
struct LogControl;
}

namespace detail {
struct Snapshot;
struct View;
} // namespace detail

class LogOwner final {
  public:
    // The service directory is provisioned by the operator before construction.
    explicit LogOwner(QString directory, unsigned retention_days = 14);
    ~LogOwner();
    LogOwner(const LogOwner &) = delete;
    LogOwner &operator=(const LogOwner &) = delete;

    // Call only at the host's actual serving transition. Bounded try-handoff.
    void host_entered_serving() noexcept;
    // For an owned clock monitor reporting a known discontinuity. Invalidates
    // all staged log views in publication order.
    void clock_discontinuity();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit LogOwner(QString, unsigned, const testing::LogControl &);
    void connector_report_unavailable(ConnectorProvider) noexcept;
    bool snapshot(const std::shared_ptr<detail::Snapshot> &);
    void publish(detail::View, const std::function<void(detail::View)> &);
    friend class Connections;
    friend class Export;
    friend struct testing::LogControl;
};

// One installation's registered adapter observations. All access must run
// on the same authoritative queue as its Export.
// Only validated health/gap cross this boundary. Export never calls adapters.
class Connections final {
  public:
    explicit Connections(LogOwner &);
    ~Connections();
    void register_adapter(unsigned, ConnectorProvider);
    void unregister_adapter(unsigned);
    void report(unsigned, const ConnectorAdapter &);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    friend class Export;
};

enum class Status { artifact, invalid_selection, denied, selection_unavailable };
struct Reply {
    Status status;
    QByteArray artifact;
};
using Ticket = std::uint64_t;
using ReadGate = std::function<Gate(const std::vector<Selector> &, const Scope *)>;
using Transfer = std::function<void(Reply)>;
struct PublicationRead {
    const std::vector<Selector> &selectors;
    const Scope &prepared_scope;
};

// Owned by the authoritative queue. One private reservation, no backlog.
// The ready callback may run synchronously in stage() or later on the log thread.
// It must always post to the authoritative queue for the next host turn.
// Call handoff at the actual first transfer, in the real current writer/lifecycle
// scope. read_current, can_transfer and transfer must complete in that same
// uninterrupted turn. The host owns lease/budget/stop/disconnect/normal outcome.
// can_transfer and transfer must be bounded and nonblocking. They must not call
// LogOwner while the final publication gate is held.
// For a protected result, can_transfer also checks the supplied expiry against
// the guarded publication clock in its live completion scope.
class Export final {
  public:
    explicit Export(LogOwner &, const Connections &, std::function<Versions()> = running_versions);
    ~Export();
    // Reserve and preflight before enqueueing either writer transaction. The
    // returned selection is borrowed until the next operation on this owner.
    [[nodiscard]] Ticket begin(std::optional<QByteArrayView>);
    [[nodiscard]] const std::vector<Selector> *selection(Ticket) const;
    // Pass freshly acknowledged preparation from the writer completion.
    // Return false if the ticket or work was refused.
    [[nodiscard]] bool stage(Ticket, Gate, std::function<void()> ready = {});
    // An admitted artifact or miss needs a new coherent read. Copy the bounded
    // inputs into the final writer unit. Use that unit's normal acknowledgement
    // only within the host scope that orders actual transfer against later work.
    [[nodiscard]] std::optional<PublicationRead> publication_read(Ticket) const;
    void handoff(Ticket, const ReadGate &read_current,
                 const std::function<bool(std::optional<Utc> expires)> &can_transfer,
                 const Transfer &, qsizetype host_byte_limit = 128 * 1024);
    void cancel(Ticket);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace subathon::server::diagnostics
