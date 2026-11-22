// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../support/credential_native_controls.hpp"
#include "../support/credential_key_fixture.hpp"
#include "../support/sqlite_credential_catalog_fixture.hpp"
#include "boundary.hpp"

#include <algorithm>
#include <array>
#include <source_location>
#include <subathon/server/testing/sqlite_writer_work_faults.hpp>

namespace rates_012_fixture {
using namespace subathon::server;
using namespace subathon::server::rate_credentials;
using credential_test_support::blob;
using credential_test_support::require;
namespace kf = credential_test_support;
namespace cv = credential_vectors;
namespace native = credential_native;
namespace cat = credential_catalog_fixture;
namespace record = credential_detail;
namespace test = subathon::server::testing;
using Buffer = record::GuardedBytes;

constexpr Revision filled(unsigned char b) {
    Revision out{};
    out.fill(b);
    return out;
}
inline constexpr auto r1 = filled(0x11);
inline constexpr auto r2 = filled(0x22);
inline constexpr auto rx = filled(0x33);
inline constexpr auto n1 = filled(0xa1);
inline constexpr auto n2 = filled(0xa2);
inline constexpr std::array first_draws{r1, n1};
inline constexpr std::array second_draws{r2, n2};
inline constexpr std::string_view v = "v1!";
inline constexpr std::string_view w = "v2!";
inline constexpr std::string_view app = "0000000000000000000000000000000a";
inline constexpr std::string_view upper_app = "0000000000000000000000000000000A";
inline const RateCredentialScope fx{"I", KeyedRateProvider::exchange_rate_api, "A", "S"};
inline const RateCredentialScope ox{"I", KeyedRateProvider::open_exchange_rates, "A", "S"};
inline const QByteArray fx_blob = QByteArray::fromHex("0001490801000141000153");
inline const QByteArray ox_blob = QByteArray::fromHex("0001490901000141000153");
inline constexpr auto select_sql =
    "SELECT record FROM main.subathon_provider_credentials WHERE scope=?";
inline constexpr auto upsert_sql =
    "INSERT INTO main.subathon_provider_credentials(scope,record) VALUES(?,?) "
    "ON CONFLICT(scope) DO UPDATE SET record=excluded.record";

inline void expect(Result actual, Result expected, const char *step,
                   std::source_location where = std::source_location::current()) {
    require(actual == expected,
            QString("%1 (category %2, expected %3)").arg(step).arg(int(actual)).arg(int(expected)),
            where);
}

inline std::span<const unsigned char> bytes(std::string_view value) {
    return {reinterpret_cast<const unsigned char *>(value.data()), value.size()};
}
inline std::span<const unsigned char> bytes(const QByteArray &value) {
    return bytes(std::string_view(value.constData(), static_cast<std::size_t>(value.size())));
}

inline std::unique_ptr<Buffer> guarded(std::string_view input) {
    auto buffer = std::make_unique<Buffer>(std::max<std::size_t>(input.size(), 1));
    require(buffer->bytes().size() == std::max<std::size_t>(input.size(), 1),
            "Synthetic input uses checked guarded storage");
    std::ranges::copy(bytes(input), buffer->bytes().begin());
    return buffer;
}

inline CredentialKey key_at(const QByteArray &path) {
    auto result = kf::provision_path(path);
    require(std::holds_alternative<CredentialKey>(result), "Protected fixture key opens");
    return std::get<CredentialKey>(std::move(result));
}

inline void normal(const SqliteWorkResult &work) {
    financial_refund_fixture::normal(work);
    require(work.value->metaType() == QMetaType::fromType<int>() && work.value->toInt() == 1,
            "Only the fixed redacted work marker is published");
}

class Database final {
  public:
    Database() {
        require(dir.isValid(), "Disposable database directory exists");
        open(financial_refund_fixture::limits());
        cat::expect_report(
            cat::migrate(*writer, MigrationMode::advance, financial_refunds_repository_manifest()),
            CatalogState::current, 15, 15, 15);
        cat::expect_report(cat::migrate(*writer, MigrationMode::advance,
                                        provider_credentials_repository_manifest()),
                           CatalogState::current, 16, 16, 1);
        reopen();
    }

    void reopen() {
        auto stopped = std::make_shared<bool>(false);
        require(writer->shutdown([stopped] { *stopped = true; }) == StorageError::none &&
                    QTest::qWaitFor([stopped] { return *stopped; }, 10000),
                "Writer shutdown acknowledges before healthy reopen");
        writer.reset();
        open({});
    }

    SqliteWorkResult work(SqliteWriter::Work body) {
        return milestone_group_restart_fixture::work(*writer, std::move(body));
    }

    template <class Body> void committed(Body body) {
        auto completion = work([&](auto context) -> QVariant {
            auto session = context.begin();
            body(session);
            session.commit();
            return 1;
        });
        normal(completion);
    }

    SqlRows rows(const QString &sql, QList<QVariant> bindings = {}) const {
        SqliteReader observer;
        require(observer.open(path()) == StorageError::none, "Committed observer opens");
        auto reply = observer.read({sql, std::move(bindings)});
        require(reply.error == StorageError::none, "Committed observer reads");
        return std::move(reply.rows);
    }

    QByteArray envelope(const QByteArray &scope = fx_blob) const {
        auto found = rows(select_sql, {scope});
        if (found.empty())
            return {};
        require(found.size() == 1 && found[0].size() == 1 &&
                    found[0][0].metaType() == QMetaType::fromType<QByteArray>(),
                "Observer sees one encrypted BLOB");
        return found[0][0].toByteArray();
    }

    void inject(const QByteArray &scope, const QByteArray &value) {
        committed([&](const auto &s) { (void)s.execute({upsert_sql, {scope, value}}); });
    }

    void lose_ack(test::WorkControlFault fault) {
        require(test::SqliteWriterWorkAccess::inject_next_control_ack_loss(*writer, fault) ==
                    StorageError::none,
                "Install the shipped acknowledgement-loss fault for a real SQL control");
    }
    void lose_commit_ack() { lose_ack(test::WorkControlFault::lose_next_commit_ack); }

  private:
    QString path() const { return dir.filePath("rates-012.sqlite"); }
    void open(StorageLimits limits) {
        writer = std::make_unique<SqliteWriter>();
        require(writer->open(path(), limits) == StorageError::none, "Disposable writer opens");
    }

    QTemporaryDir dir;
    std::unique_ptr<SqliteWriter> writer;
};

inline Reservation reservation(Revision revision = r1, RateCredentialScope scope = fx,
                               std::uint64_t order = 1) {
    const bool er = scope.provider == KeyedRateProvider::exchange_rate_api;
    return {{std::move(scope), "G", "C", "T",
             er ? Profile(era::Profile::pro) : Profile(oer::Profile::free), "Q", revision},
            "request-" + std::to_string(order),
            er ? Route(era::Operation::latest) : Route(oer::Operation::latest),
            order};
}

inline Reservation fresh(Revision revision, std::uint64_t order, RateCredentialScope scope = fx) {
    auto next = reservation(revision, std::move(scope), order);
    next.selection.selection += std::to_string(order);
    return next;
}

inline oer::AdmissionIdentity admission(const Reservation &request) {
    const auto &selection = request.selection;
    return {{selection.scope.installation,
             selection.quota_scope,
             selection.configuration,
             {reinterpret_cast<const char *>(selection.revision.data()), selection.revision.size()},
             std::get<oer::Profile>(selection.profile)},
            request.request,
            request.order};
}

inline RateCredentialChange change(Verb verb, std::optional<Revision> expected = {},
                                   RateCredentialScope scope = fx) {
    return {std::move(scope), verb, "management-decision", expected};
}

struct Staged {
    MutationResult outcome{Result::feature_not_implemented};
    SqliteWorkResult completion;
    std::size_t draws{};
};

enum class Control { commit, rollback, post_commit_failure };

class Fixture final {
  public:
    Fixture() : key_path(keys.file()), key(make_key()) {}

    CredentialKey make_key() {
        kf::KeyFixture::write(key_path);
        return key_at(key_path);
    }

    Staged stage(const RateCredentialChange &request, std::string_view value,
                 std::span<const Revision> draws = {}, Control control = Control::commit) {
        expect(owner.admit_change(request, request, value.size()), Result::admitted,
               "Admit mutation and fence its exact scope");
        // Empty erase has no buffer. Empty put is a separate admission refusal case.
        auto input = value.empty() ? nullptr : guarded(value);
        Staged staged;
        staged.completion = db.work([&](auto context) -> QVariant {
            const auto session = context.begin();
            require(native::prepare_sdk(), "Native SDK ready in writer work before draw controls");
            const native::Controls observe(native::Fault::none, 0, draws);
            staged.outcome = input ? owner.stage_rate_credential_put(session, &key, request, *input)
                                   : owner.stage_rate_credential_erase(session, &key, request);
            staged.draws = observe.observation().nonce_draws;
            if (control == Control::rollback) {
                session.rollback();
            } else {
                session.commit();
                if (control == Control::post_commit_failure)
                    throw SqliteWorkFailure(WorkError::work_exception);
            }
            return 1;
        });
        return staged;
    }

    void put(const RateCredentialChange &request, std::string_view value, Revision revision,
             std::span<const Revision> draws = first_draws) {
        const auto staged = stage(request, value, draws);
        expect(staged.outcome.status, Result::staged_put,
               "Stage exact guarded FX insert or replace");
        require(staged.outcome.revision == revision && staged.draws == 2,
                "Changed put returns the first draw, with exactly two draws");
        normal(staged.completion);
        expect(owner.finish_change(request, staged.completion), Result::committed_change,
               "Publish changed put only on normal acknowledged completion");
    }

    std::pair<Readback, SqliteWorkResult> readback(RateCredentialScope scope = fx) {
        Readback read{Result::feature_not_implemented};
        const auto request = change(Verb::inspect, {}, std::move(scope));
        auto done = db.work([&](auto context) -> QVariant {
            const auto session = context.begin();
            read = owner.inspect(session, &key, request, request);
            session.commit();
            return 1;
        });
        normal(done);
        return {read, done};
    }

    struct Load {
        Result status{Result::feature_not_implemented};
        SqliteWorkResult completion;
        WorkSlot slot;
    };

    Load prepare(const Reservation &request, const UseGrant &current,
                 Control control = Control::commit, bool key_available = true,
                 bool expect_no_sql = false) {
        Load out;
        out.completion = db.work([&](auto context) -> QVariant {
            const auto session = context.begin();
            out.status = owner.prepare_keyed_rate_use(session, key_available ? &key : nullptr,
                                                      request, current, out.slot);
            if (expect_no_sql) {
                // A charged lookup would exhaust the default 64-statement cap.
                for (int i = 0; i < 64; ++i)
                    (void)session.execute({"SELECT 1", {}});
            }
            if (control == Control::rollback) {
                session.rollback();
            } else {
                session.commit();
                if (control == Control::post_commit_failure)
                    throw SqliteWorkFailure(WorkError::work_exception);
            }
            return 1;
        });
        return out;
    }

    void refused_use(const Reservation &request, const UseGrant &current, Result expected,
                     bool key_available = true, bool expect_no_sql = false) {
        auto load = prepare(request, current, Control::commit, key_available, expect_no_sql);
        normal(load.completion);
        expect(load.status, expected, "Refuse keyed preparation at the selected precedence");
        require(!load.slot && !owner.finish_keyed_rate_use(std::move(load.slot), load.completion),
                "A refused preparation leaves no private slot or permit");
    }

    Permit permit(const Reservation &request) {
        auto load = prepare(request, request);
        normal(load.completion);
        expect(load.status, Result::prepared, "Prepare the exact current guarded selection");
        require(bool(load.slot), "Successful preparation fills the private work slot");
        auto permit = owner.finish_keyed_rate_use(std::move(load.slot), load.completion);
        require(bool(permit), "Normal writer completion releases one move-only permit");
        return permit;
    }

    Database db;
    kf::KeyFixture keys;
    QByteArray key_path;
    CredentialKey key;
    CredentialOwner owner;
};

class Handoff final : public QualifiedTransport {
  public:
    Handoff(const Reservation &expected, std::string_view value)
        : expected_(expected), value_(value) {}

    void accept(const Reservation &facts, std::span<const unsigned char> secret) noexcept override {
        ++count;
        exact = facts == expected_ && secret.size() == value_.size() &&
                sodium_memcmp(secret.data(), value_.data(), value_.size()) == 0;
    }

    int count{};
    bool exact{};

  private:
    const Reservation &expected_;
    const std::string_view value_;
};

inline Attempt deliver(Fixture &f, const Reservation &request, std::string_view value) {
    auto permit = f.permit(request);
    Handoff capture(request, value);
    Attempt attempt;
    expect(f.owner.consume_keyed_rate_use(permit, request, capture, attempt), Result::accepted,
           "Accept the permit in the exact current owner turn");
    require(!permit && attempt && capture.count == 1 && capture.exact,
            "One guarded handoff sees the expected bytes and all reservation fields");
    return attempt;
}

inline void blocked(Fixture &f, Permit &permit, const UseGrant &current,
                    const Reservation &reference) {
    Handoff capture(reference, v);
    Attempt attempt;
    const auto status = f.owner.consume_keyed_rate_use(permit, current, capture, attempt);
    require(
        (status == Result::stale || status == Result::not_authorized || status == Result::held) &&
            !permit && !attempt && capture.count == 0,
        "Invalidated or rebound permit is consumed with zero private handoffs");
}

inline void connector(Fixture &f) {
    require(native::prepare_sdk(), "Native SDK ready before separate connector draw");
    f.db.committed([&](const auto &session) {
        const std::array<Revision, 1> draws{cv::nonce};
        const native::Controls observe(native::Fault::none, 0, draws);
        const auto result =
            CredentialRepository(session, f.key).stage_put(cv::exact, cv::plaintext);
        require(std::holds_alternative<CredentialStaged>(result) &&
                    observe.observation().nonce_draws == 1,
                "Shipped connector put stages with its own observed nonce");
    });
    require(f.db.envelope(blob(cv::scope)) == blob(cv::record),
            "Shipped independent 71-byte connector envelope is exact");
}

inline void expect_connector(Fixture &f, std::span<const unsigned char> expected = cv::plaintext) {
    f.db.committed([&](const auto &session) {
        const auto result = CredentialRepository(session, f.key).load(cv::exact);
        const auto *value = std::get_if<CredentialValue>(&result);
        require(value && std::ranges::equal(value->bytes(), expected),
                "Shipped connector owner returns its exact opaque bytes");
    });
}

inline Buffer &fill_payload(Buffer &payload, const Revision &revision, std::string_view secret,
                            std::string_view magic = "FXR1") {
    require(payload.bytes().size() == 28 + secret.size(), "Guarded payload capacity is exact");
    std::ranges::copy(bytes(magic), payload.bytes().begin());
    std::ranges::copy(revision, payload.bytes().begin() + 4);
    std::ranges::copy(bytes(secret), payload.bytes().begin() + 28);
    return payload;
}

inline QByteArray seal_fixture(const QByteArray &scope, const Buffer &payload,
                               const Revision &nonce = n1) {
    const std::array<Revision, 1> draws{nonce};
    const native::Controls observe(native::Fault::none, 0, draws);
    const auto encrypted = record::seal_record(bytes(scope), payload.bytes(), cv::id, cv::key);
    const auto *envelope = std::get_if<record::RecordBytes>(&encrypted);
    require(envelope && observe.observation().nonce_draws == 1,
            "Explicit malformed or competing fixture payload seals once");
    return blob(*envelope);
}

inline void independent_open(const QByteArray &scope, const QByteArray &envelope,
                             const Revision &revision, const Revision &nonce,
                             std::string_view secret) {
    constexpr char domain[] = "subathon.provider-credential.v1";
    const auto header = envelope.first(48);
    const auto aad = QByteArray(domain, sizeof(domain)) + header + scope;
    require(header.first(4) == "SCR1" && header.mid(4, 16) == blob(cv::id) &&
                header.mid(20, 24) == blob(nonce) &&
                header.mid(44, 4) == QByteArray::fromHex("0000001f"),
            "Literal SCR1 header matches the independent 31-byte payload");
    Buffer plain(31);
    require(plain.bytes().size() == 31, "Independent decrypt output is locked");
    unsigned long long size{};
    const auto encrypted = bytes(envelope).subspan(48);
    require(crypto_aead_xchacha20poly1305_ietf_decrypt(
                plain.bytes().data(), &size, nullptr, encrypted.data(), encrypted.size(),
                bytes(aad).data(), aad.size(), nonce.data(), cv::key.data()) == 0 &&
                size == 31,
            "Independent XChaCha open authenticates canonical scope and literal header");
    const auto actual = plain.bytes();
    require(std::ranges::equal(actual.first(4), bytes(std::string_view("FXR1"))) &&
                std::ranges::equal(actual.subspan(4, 24), revision) &&
                std::ranges::equal(actual.subspan(28), bytes(secret)),
            "Guarded payload equals literal magic, revision, and synthetic value");
}

} // namespace rates_012_fixture
