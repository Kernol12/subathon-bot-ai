// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_account_sessions_fixture.hpp"
#include <subathon/server/sqlite_account_sessions.hpp>

namespace installation_epoch_fixture {
using namespace subathon::server;
using account_sessions_fixture::blob;
using account_sessions_fixture::Cleanup;
using account_sessions_fixture::require;
using account_sessions_fixture::scenario;

enum class Order { current, after_rotation, queued_behind_rotation };

inline void order_rows() {
    QTest::addColumn<int>("order");
    QTest::newRow("current-E") << int(Order::current);
    QTest::newRow("O1-after-F") << int(Order::after_rotation);
    QTest::newRow("O2-queued-under-E") << int(Order::queued_behind_rotation);
}

template <class T, class... Types> const T &selected(const std::variant<Types...> &value) {
    const auto *result = std::get_if<T>(&value);
    require(result, "Expected exact typed alternative");
    return *result;
}

struct Clock final : CurrentSessionClock {
    std::int64_t now = 2000;
    unsigned calls = 0;
    SessionClockResolution sample_current() override {
        ++calls;
        return SessionClockSample{now};
    }
};

inline void committed(const SqliteWorkResult &work) {
    require(work.transaction == TransactionOutcome::committed && work.error == WorkError::none &&
                work.value.has_value(),
            "Real acknowledged commit with a normal value");
}

inline void rolled_back(const SqliteWorkResult &work) {
    require(work.transaction == TransactionOutcome::rolled_back &&
                work.error == WorkError::not_committed && !work.value,
            "Real acknowledged rollback with no normal value");
}

inline const QString installation_table = "subathon_account_installation";
inline const QStringList identity_tables{installation_table, "subathon_accounts",
                                         "subathon_account_sessions", "subathon_local_credentials"};

// The observer and active-session reads use the same projection. The listed schema tables
// have rowids. Capture each table separately so empty tables remain part of the image.
using Image = QList<SqlRows>;
template <class Read> Image read_image(const QStringList &tables, Read read) {
    Image image;
    for (const auto &table : tables)
        image.append(read("SELECT * FROM main." + table + " ORDER BY rowid"));
    return image;
}
inline Image active_image(const SqliteWorkSession &session, const QStringList &tables) {
    return read_image(tables, [&](const QString &sql) { return session.execute({sql, {}}); });
}

inline SqlRows installation_row(bool rotated) {
    return {{1, blob("I"), blob(rotated ? "F" : "E"), blob(rotated ? "N" : "J")}};
}

inline void stage_rotation(const SqliteWorkSession &session) {
    SqliteAccountSessionUnit unit(session, {{}});
    const auto read = unit.read_installation("I");
    const auto &expected = selected<StoredInstallationFacts>(read);
    require(expected.current.installation_ref == "I" && expected.current.session_epoch_ref == "E" &&
                expected.revision_ref == "J",
            "Rotation reads exact I/E/J expectation through the real store");
    require(unit.replace_installation_epoch(expected, "F", "N") == LifecycleWriteResult::staged,
            "Ordinary installation CAS stages F/N");
}

enum class Step {
    admit_rotation,
    admit_target,
    rotation_body,
    rotation_completion,
    target_body,
    target_completion
};

// Both enqueue calls in O2 run on one owner turn without processing events. Observe
// the independently expected F image in the rotation completion before target entry.
template <class File> class Ordering {
  public:
    Ordering(File &file, SqliteWriter &writer, Order order, QStringList tables)
        : file_(file), writer_(writer), order_(order), tables_(std::move(tables)), before_(image()),
          baseline_(before_) {
        require(!tables_.isEmpty() && tables_.front() == installation_table &&
                    before_.front() == installation_row(false),
                "Before admission, the committed installation is I/E/J");
        if (order_ != Order::current)
            baseline_[0] = installation_row(true);
    }

    void run(SqliteWriter::Work body, SqliteWriter::WorkCompletion completion) {
        if (order_ != Order::current) {
            enqueue_rotation();
            if (order_ == Order::after_rotation) {
                require(QTest::qWaitFor([&] { return rotation_done_; }, 10000),
                        "O1 observes completed rotation before admitting target");
                rethrow();
            }
        }

        require(writer_.enqueue_work(
                    [&](const auto &context) {
                        trace_.push_back(Step::target_body);
                        return observe([&] {
                            if (order_ != Order::current)
                                require(rotation_done_ && !error_,
                                        "Target enters only after normal rotation completion");
                            return body(context);
                        });
                    },
                    [&](auto work) {
                        trace_.push_back(Step::target_completion);
                        capture([&] { completion(std::move(work)); });
                        target_done_ = true;
                    }) == StorageError::none,
                "Writer admits the ordinary target");
        trace_.push_back(Step::admit_target);

        if (order_ == Order::queued_behind_rotation)
            require(!rotation_done_ && !target_done_ && image() == before_,
                    "Both O2 admissions returned under committed E before the owner yielded");

        require(QTest::qWaitFor([&] { return target_done_; }, 10000), "Target completion observed");
        rethrow();
        require(trace_ == expected_trace(), "Exact FIFO admission, body and completion sequence");
    }

    [[nodiscard]] bool current() const { return order_ == Order::current; }
    [[nodiscard]] Image image() const {
        return read_image(tables_, [&](const QString &sql) { return file_.rows(sql); });
    }
    [[nodiscard]] const Image &baseline() const { return baseline_; }
    void unchanged_inside(const SqliteWorkSession &session) const {
        require(active_image(session, tables_) == baseline_,
                "Active transaction has no staged changes before explicit rollback");
    }
    void unchanged_after() const {
        require(image() == baseline_, "Committed image equals the pre-target baseline");
    }

  private:
    void enqueue_rotation() {
        require(writer_.enqueue_work(
                    [&](const auto &context) {
                        trace_.push_back(Step::rotation_body);
                        return observe([&]() -> QVariant {
                            const auto session = context.begin();
                            Cleanup cleanup{session};
                            stage_rotation(session);
                            session.commit();
                            return 1;
                        });
                    },
                    [&](const auto &work) {
                        trace_.push_back(Step::rotation_completion);
                        capture([&] {
                            committed(work);
                            require(image() == baseline_,
                                    "Acknowledged F preserves account, E session and command rows");
                        });
                        rotation_done_ = true;
                    }) == StorageError::none,
                "Writer admits standalone rotation");
        trace_.push_back(Step::admit_rotation);
    }

    template <class F> QVariant observe(F call) {
        try {
            return call();
        } catch (...) {
            if (!error_)
                error_ = std::current_exception();
            throw;
        }
    }
    template <class F> void capture(F call) {
        try {
            call();
        } catch (...) {
            if (!error_)
                error_ = std::current_exception();
        }
    }
    void rethrow() const {
        if (error_)
            std::rethrow_exception(error_);
    }
    [[nodiscard]] std::vector<Step> expected_trace() const {
        using enum Step;
        switch (order_) {
        case Order::current:
            return {admit_target, target_body, target_completion};
        case Order::after_rotation:
            return {admit_rotation, rotation_body, rotation_completion,
                    admit_target,   target_body,   target_completion};
        case Order::queued_behind_rotation:
            return {admit_rotation,      admit_target, rotation_body,
                    rotation_completion, target_body,  target_completion};
        }
        return {};
    }

    File &file_;
    SqliteWriter &writer_;
    Order order_;
    QStringList tables_;
    Image before_, baseline_;
    std::vector<Step> trace_;
    std::exception_ptr error_;
    bool rotation_done_ = false, target_done_ = false;
};

template <class File> void rotate(File &file, SqliteWriter &writer, const QStringList &tables) {
    const auto before = read_image(tables, [&](const QString &sql) { return file.rows(sql); });
    require(before.front() == installation_row(false), "Standalone control begins at E/J");
    account_sessions_fixture::run(writer, [&](auto context) -> QVariant {
        const auto session = context.begin();
        Cleanup cleanup{session};
        stage_rotation(session);
        session.commit();
        return 1;
    });
    auto expected = before;
    expected[0] = installation_row(true);
    require(read_image(tables, [&](const QString &sql) { return file.rows(sql); }) == expected,
            "Standalone control rotation changed only I's epoch and revision");
}

inline SessionRecordFacts record(char account = 'A') {
    AccountSessionDigest digest{};
    digest.fill(static_cast<std::uint8_t>(account));
    return {"I",
            std::string(1, account),
            std::string("s") + account,
            "E",
            "C",
            "v",
            digest,
            SessionRecordState::active,
            1000,
            1000,
            86401000};
}
inline SessionLookupRequest lookup(const SessionRecordFacts &row) {
    return {row.installation_ref, row.digest};
}

inline SqlRow session_row(const SessionRecordFacts &row,
                          SessionRetention retention = SessionRetention::normal) {
    return {
        blob(row.installation_ref),
        blob(row.account_ref),
        blob(row.session_ref),
        blob(row.installation_epoch_ref),
        blob(row.account_epoch_ref),
        blob(row.record_revision_ref),
        1,
        QByteArray(reinterpret_cast<const char *>(row.digest.data()), qsizetype(row.digest.size())),
        int(row.state),
        int(retention),
        qlonglong(row.lifetime_started_utc_ms),
        qlonglong(row.authenticated_utc_ms),
        qlonglong(row.expires_utc_ms)};
}

inline void seed_identity(const SqliteWorkSession &session, std::string_view accounts) {
    (void)session.execute(
        {"INSERT INTO subathon_account_installation VALUES(1,X'49',X'45',X'4a')", {}});
    for (const char account : accounts) {
        (void)session.execute({"INSERT INTO subathon_accounts VALUES(X'49',?,1,X'43',X'4b')",
                               {QByteArray(1, account)}});
        (void)session.execute(
            {"INSERT INTO subathon_account_sessions VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?)",
             session_row(record(account))});
    }
}

inline void exact_session(const AccountSessionResolution &result, const SessionRecordFacts &row,
                          bool fresh = true) {
    const auto &actor = selected<ResolvedAccountSession>(result);
    // SAS1 + 32-byte zero read scope + u64be first ordinal, all from the caller's literal scope.
    require(actor.installation_ref == row.installation_ref &&
                actor.account_ref == row.account_ref && actor.session_ref == row.session_ref &&
                actor.record_revision_ref == row.record_revision_ref &&
                actor.snapshot_ref == "SAS1" + std::string(40, '\0') &&
                actor.authenticated_utc_ms == row.authenticated_utc_ms &&
                actor.expires_utc_ms == row.expires_utc_ms && actor.fresh == fresh,
            "Exact actor, session, snapshot, revision, authentication and expiry");
}
} // namespace installation_epoch_fixture
