// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.hpp"

#include <atomic>
#include <chrono>
#include <type_traits>

namespace {
using namespace rates_012_fixture;
using namespace std::chrono_literals;
using namespace std::string_view_literals;

static_assert(!std::is_copy_constructible_v<Permit> && !std::is_copy_constructible_v<Attempt>);
static_assert(app.size() == 32 && upper_app.size() == 32);

void observe_refusal(Fixture &f, const RateCredentialChange &request, const Staged &staged,
                     Result expected) {
    expect(staged.outcome.status, expected,
           "Authenticated mutation refuses with the exact category");
    require(!staged.outcome.revision && staged.draws == 0, "Refusal yields no revision or draws");
    normal(staged.completion);
    (void)f.owner.finish_change(request, staged.completion);
}

void unknown(const SqliteWorkResult &done) {
    require(done.transaction == TransactionOutcome::outcome_unknown &&
                done.error == WorkError::uncertain_outcome && !done.value,
            "Real lost acknowledgement reports unknown without success value");
}

class SyntheticSetup final : public era::SelectionPort {
  public:
    era::SelectionUpdate setup;
    era::SelectionUpdate selected_setup() const override { return setup; }
};

class SyntheticHttp final : public era::HttpPort {
  public:
    // Local transport model. The adapter owns accounting and retry evidence.
    explicit SyntheticHttp(std::size_t cap) : cap_(cap) {}

    era::Dispatch send(const era::HttpRequest &request) override {
        if (request.private_url.size() > cap_)
            return {era::DispatchKind::not_sent};
        ++gets;
        shape = request.method == "GET" && !request.follow_redirects;
        return {era::DispatchKind::sent};
    }

    int gets{};
    bool shape{};

  private:
    std::size_t cap_;
};

era::Request latest(std::string ref) {
    return {era::Operation::latest,
            std::move(ref),
            "evidence",
            era::RequestOrigin::manual,
            "USD",
            {"CHF"},
            {}};
}

const era::Clocks tick{0, era::Elapsed{"test-clock", 1000}};

class LogCheck final {
  public:
    LogCheck() : prior_(qInstallMessageHandler(handle)) {
        leaked_ = false;
        sql_warning_ = false;
    }
    ~LogCheck() { qInstallMessageHandler(prior_); }
    bool clean() const { return !leaked_.load() && !sql_warning_.load(); }

  private:
    static void handle(QtMsgType type, const QMessageLogContext &, const QString &message) {
        // A failing redaction check must itself emit neither canary nor native text.
        if (message.contains("synthetic-wire-canary") ||
            message.contains("synthetic-native-canary") ||
            message.contains("synthetic-key-path-marker") || message.contains("FXR1"))
            leaked_ = true;
        if (type == QtWarningMsg)
            sql_warning_ = true;
    }
    QtMessageHandler prior_;
    static inline std::atomic_bool leaked_{};
    static inline std::atomic_bool sql_warning_{};
};

class RateCredentialTests final : public QObject {
    Q_OBJECT

  private slots:
    void init() {
        qt_test_support::fail_on_sql_warnings();
        QVERIFY(native::prepare_sdk());
    }

    void F1_exactRecordReopenAndConnector() {
        kf::checked([] {
            Fixture f;
            connector(f);
            const auto stamp = f.db.rows(cat::history_sql);
            const auto schema = f.db.rows(cat::schema_sql);
            const auto expected_catalog = cat::literal_v16();
            cat::expect_schema(schema, expected_catalog.steps.back());
            require(stamp.size() == 16 &&
                        stamp.back() == cat::history_row(expected_catalog.steps.back()),
                    "The v16 credential catalog has the independent exact stamp");

            f.put(change(Verb::put), v, r1);
            auto envelope = f.db.envelope();
            require(fx_blob.size() == 11 && envelope.size() == 95,
                    "Literal eleven-byte scope selects exactly a 95-byte SCR1 record");
            independent_open(fx_blob, envelope, r1, n1, v);
            f.db.reopen();
            f.key = key_at(f.key_path);
            const auto current = reservation();
            f.publish(current);
            (void)deliver(f, current, v);
            expect_connector(f);
            require(
                f.db.envelope(blob(cv::scope)) == blob(cv::record) &&
                    f.db.rows(cat::history_sql) == stamp && f.db.rows(cat::schema_sql) == schema,
                "Connector record, ordered catalog history and exact objects remain byte-exact");
        });
    }

    void F2_exactScopeIsolation() {
        kf::checked([] {
            Fixture f;
            f.db.committed([&](const auto &session) {
                const std::array<Revision, 1> draws{cv::nonce};
                const native::Controls observe(native::Fault::none, 0, draws);
                require(
                    std::holds_alternative<CredentialStaged>(
                        CredentialRepository(session, f.key).stage_put(cv::exact, bytes("FXR1"sv))),
                    "Connector owner accepts four opaque bytes equal to the FX magic");
            });
            expect_connector(f, bytes("FXR1"sv));
            f.put(change(Verb::put), v, r1);
            const auto original = f.db.envelope();
            const std::pair<RateCredentialScope, const char *> alternatives[] = {
                {{"J", fx.provider, "A", "S"}, "00014a0801000141000153"},
                {{"I", fx.provider, "B", "S"}, "0001490801000142000153"},
                {{"I", fx.provider, "A", "U"}, "0001490801000141000155"},
                {ox, "0001490901000141000153"}};
            std::uint64_t order = 20;
            for (const auto &[scope, encoded] : alternatives) {
                const auto clean = reservation(r1, scope, order++);
                f.publish(clean);
                f.refused_use(clean, clean, Result::missing);
                f.db.inject(QByteArray::fromHex(encoded), original);
                const auto foreign = reservation(r1, scope, order++);
                f.reserve(foreign);
                f.refused_use(foreign, foreign, Result::authentication_failed);
                require(f.db.envelope() == original,
                        "Exact lookups preserve the original envelope");
            }
            const auto primary = reservation(r1, fx, order++);
            f.publish(primary);
            (void)deliver(f, primary, v);

            Fixture isolated;
            isolated.db.inject(fx_blob, f.db.envelope(blob(cv::scope)));
            const auto request = reservation();
            isolated.publish(request);
            isolated.refused_use(request, request, Result::authentication_failed);
        });
    }

    void F3_authorityPrecedesRowAuthentication() {
        kf::checked([] {
            Fixture f;
            f.put(change(Verb::put), v, r1);
            auto corrupted = f.db.envelope();
            corrupted.back() ^= 1;
            f.db.inject(fx_blob, corrupted);

            const auto current = reservation();
            f.publish(current);
            f.refused_use(current, {}, Result::not_authorized, true, true);
            const auto erase = change(Verb::erase, r1);
            expect(f.owner.admit_change(erase, {}, {}), Result::not_authorized,
                   "The use-only principal has no separate erase grant");
            auto stale = current;
            stale.selection.generation = "old-generation";
            f.refused_use(stale, current, Result::stale, true, true);
            f.refused_use(current, current, Result::authentication_failed);
            require(f.db.envelope() == corrupted, "Precedence probes preserve the corrupted row");
        });
    }

    void F4_revisionAndNoDrawOnIdenticalPut() {
        kf::checked([] {
            Fixture f;
            f.put(change(Verb::put), v, r1);
            const auto original = f.db.envelope();
            const auto replace = change(Verb::put, r1);
            const auto same = f.stage(replace, v, second_draws);
            expect(same.outcome.status, Result::unchanged, "Exact authenticated identical put");
            normal(same.completion);
            expect(f.owner.finish_change(replace, same.completion), Result::unchanged,
                   "Acknowledged unchanged result preserves current selection");
            require(same.draws == 0 && same.outcome.revision == r1 && f.db.envelope() == original,
                    "Unchanged preserves ciphertext and revision without nonce or revision draw");

            const auto old = reservation(r1, fx, 50);
            f.publish(old);
            f.put(replace, w, r2, second_draws);
            independent_open(fx_blob, f.db.envelope(), r2, n2, w);
            f.refused_use(old, old, Result::stale);
            const auto replacement = fresh(r2, 51);
            f.publish(replacement);
            (void)deliver(f, replacement, w);

            Fixture twin;
            twin.put(change(Verb::put), v, r1);
            const auto twin_original = twin.db.envelope();
            const std::array repeated{r1, n2};
            const auto refused = twin.stage(replace, w, repeated);
            expect(refused.outcome.status, Result::crypto_unavailable,
                   "A replacement refuses an exact prior revision draw");
            normal(refused.completion);
            (void)twin.owner.finish_change(replace, refused.completion);
            require(!refused.outcome.revision && refused.draws == 1 &&
                        twin.db.envelope() == twin_original,
                    "Repeated revision refuses before any nonce draw or record mutation");
            const auto twin_current = reservation();
            twin.publish(twin_current);
            (void)deliver(twin, twin_current, v);
        });
    }

    void F5_revocationRebindingAndLateSettlement() {
        kf::checked([] {
            const auto initial = reservation();
            Fixture revoked;
            revoked.put(change(Verb::put), v, r1);
            const auto original = revoked.db.envelope();
            revoked.publish(initial);
            auto queued = revoked.permit(initial);
            revoked.owner.revoke(fx, "G2");
            blocked(revoked, queued, initial, initial);
            revoked.refused_use(initial, initial, Result::not_authorized, true, true);
            require(revoked.db.envelope() == original, "Revocation can leave an intact row");

            Fixture outstanding;
            outstanding.put(change(Verb::put), v, r1);
            outstanding.publish(initial);
            auto accepted = deliver(outstanding, initial, v);
            outstanding.owner.revoke(fx, "G2");
            expect(outstanding.owner.finish_attempt(
                       accepted, initial,
                       {era::DispatchKind::sent,
                        era::SendTime{era::SendTimeKind::exact, {"test-clock", 7}}}),
                   Result::accounting_only,
                   "Obsolete sent attempt settles only its own accounting");
            require(!accepted, "Settlement consumes the obsolete attempt");
            outstanding.refused_use(initial, initial, Result::not_authorized, true, true);

            Fixture bindings;
            bindings.put(change(Verb::put), v, r1);
            bindings.publish(initial);
            auto unissued = initial;
            unissued.request = "unissued-request";
            bindings.refused_use(unissued, unissued, Result::stale, true, true);
            using Rebind = void (*)(Reservation &);
            const Rebind rebindings[] = {
                +[](Reservation &r) { ++r.order; },
                +[](Reservation &r) { r.request += "-other"; },
                +[](Reservation &r) {
                    r.selection.scope = ox;
                    r.selection.profile = oer::Profile::free;
                },
                +[](Reservation &r) { r.route = era::Operation::history; },
                +[](Reservation &r) { r.selection.quota_scope += "2"; },
                +[](Reservation &r) { r.selection.configuration += "2"; },
                +[](Reservation &r) { r.selection.generation += "2"; },
                +[](Reservation &r) { r.selection.selection += "2"; },
                +[](Reservation &r) { r.selection.revision = r2; },
                +[](Reservation &r) { r.selection.profile = era::Profile::free; }};
            std::uint64_t order = 10;
            for (const auto rebind : rebindings) {
                const auto exact = reservation(r1, fx, order++);
                bindings.reserve(exact);
                auto permit = bindings.permit(exact);
                auto different = exact;
                rebind(different);
                blocked(bindings, permit, different, exact);
                blocked(bindings, permit, exact, exact);
            }
            const auto exact = reservation(r1, fx, order);
            bindings.reserve(exact);
            auto permit = bindings.permit(exact);
            Handoff capture(exact, v);
            Attempt attempt;
            expect(bindings.owner.consume_keyed_rate_use(permit, exact, capture, attempt),
                   Result::accepted, "The still-valid exact permit hands off once");
            require(!permit && attempt && capture.count == 1 && capture.exact,
                    "A successful handoff consumes the permit");
            blocked(bindings, permit, exact, exact);
        });
    }

    void F6_exactEraseAndAuthorizedAbsence() {
        kf::checked([] {
            Fixture f;
            connector(f);
            f.put(change(Verb::put), v, r1);
            const auto original = f.db.envelope();
            const auto before_revoke = reservation();
            f.publish(before_revoke);
            auto pending = f.permit(before_revoke);
            f.owner.revoke(fx, "G2");
            blocked(f, pending, before_revoke, before_revoke);
            const auto empty = change(Verb::erase);
            expect(f.owner.admit_change(empty, empty, {}), Result::invalid_input,
                   "Erase with an empty expectation is invalid");
            const auto wrong = change(Verb::erase, rx);
            observe_refusal(f, wrong, f.stage(wrong, {}), Result::stale);
            require(f.db.envelope() == original, "Stale erase preserves the FX envelope");

            const auto exact = change(Verb::erase, r1);
            const auto erased = f.stage(exact, {});
            expect(erased.outcome.status, Result::staged_erase, "Authenticated exact erase stages");
            require(!erased.outcome.revision && erased.draws == 0,
                    "Erase produces no new revision");
            normal(erased.completion);
            expect(f.owner.finish_change(exact, erased.completion), Result::committed_change,
                   "Acknowledged erase publishes completion");
            auto [read, completion] = f.readback();
            expect(read.status, Result::missing,
                   "Authorized private management readback sees absence");
            require(!read.revision && f.db.envelope().isEmpty() &&
                        f.db.envelope(blob(cv::scope)) == blob(cv::record),
                    "Exact absence has no revision and connector row is untouched");
            f.refused_use(before_revoke, before_revoke, Result::not_authorized, true, true);
            observe_refusal(f, exact, f.stage(exact, {}), Result::stale);
            f.put(change(Verb::put), v, r2, second_draws);
            independent_open(fx_blob, f.db.envelope(), r2, n2, v);
            auto old = fresh(r1, 60);
            old.selection.generation = "G3";
            f.publish(old);
            f.refused_use(old, old, Result::stale);
            auto replaced = fresh(r2, 61);
            replaced.selection.generation = "G3";
            f.publish(replaced);
            (void)deliver(f, replaced, v);
        });
    }

    void F7_lostCommitAndFailedPreparation() {
        kf::checked([] {
            Fixture f;
            f.put(change(Verb::put), v, r1);
            const auto old_selection = reservation(r1, fx, 70);
            f.publish(old_selection);
            const auto replace = change(Verb::put, r1);
            f.db.lose_commit_ack();
            const auto staged = f.stage(replace, w, second_draws);
            expect(staged.outcome.status, Result::staged_put, "Replacement is provisional");
            require(staged.outcome.revision == r2 && staged.draws == 2,
                    "The acknowledged candidate before COMMIT is r2");
            unknown(staged.completion);
            expect(f.owner.finish_change(replace, staged.completion), Result::held,
                   "Lost acknowledgement holds use and publishes no success");
            f.db.reopen();
            f.refused_use(old_selection, old_selection, Result::held);
            auto [read, done] = f.readback();
            expect(read.status, Result::present, "Healthy management readback sees an exact row");
            require(read.revision == r2, "Healthy readback establishes the committed candidate");
            expect(f.owner.reconcile(replace, read, done), Result::reconciled,
                   "Confirm exact candidate without mutation retry");
            f.refused_use(old_selection, old_selection, Result::stale);
            const auto repaired = fresh(r2, 71);
            f.publish(repaired);
            (void)deliver(f, repaired, w);

            auto failure_request = repaired;
            failure_request.request = "request-72";
            failure_request.order = 72;
            f.reserve(failure_request);
            auto failed = f.prepare(failure_request, failure_request, Control::post_commit_failure);
            expect(failed.status, Result::prepared,
                   "Inject failure after filling the guarded slot");
            require(failed.slot && failed.completion.transaction == TransactionOutcome::committed &&
                        failed.completion.error == WorkError::work_exception &&
                        !failed.completion.value,
                    "Commit is known but callback completion is abnormal and carries no success");
            require(!f.owner.finish_keyed_rate_use(std::move(failed.slot), failed.completion),
                    "A filled slot never releases a permit after a work error");
        });
    }

    void F8_boundEchoAndShippedSanitizer() {
        kf::checked([] {
            struct EchoCase {
                std::string_view json;
                EchoShape shape;
                std::string_view value;
                oer::EchoMatch expected;
                bool foreign{};
                bool duplicate_field{};
            };
            constexpr EchoCase cases[] = {{R"("0000000000000000000000000000000a")",
                                           EchoShape::single, app, oer::EchoMatch::match},
                                          {R"("0000000000000000000000000000000A")",
                                           EchoShape::single, upper_app, oer::EchoMatch::mismatch},
                                          {{}, EchoShape::absent, {}, oer::EchoMatch::unknown},
                                          {"17", EchoShape::invalid, {}, oer::EchoMatch::unknown},
                                          {R"("0000000000000000000000000000000a")",
                                           EchoShape::single, app, oer::EchoMatch::match, true},
                                          {R"("0000000000000000000000000000000a")",
                                           EchoShape::invalid,
                                           {},
                                           oer::EchoMatch::unknown,
                                           false,
                                           true}};
            // Exercise the real #400 sanitizer for every fixed frame as an
            // independent control. Its legacy DTOs receive literal synthetic bytes.
            const auto sanitized = [](const Reservation &request, const EchoCase &test) {
                namespace wire = oer::private_transport;
                const auto identity = admission(request);
                const oer::Admission issued{
                    identity,
                    {oer::Operation::usage, request.request, "evidence", {}, {}, {}, {}},
                    "GET",
                    "https://openexchangerates.org/api/usage.json",
                    "Authorization",
                    false,
                    16 * 1024,
                    256 * 1024,
                    256 * 1024,
                    4s};
                const auto handoff =
                    wire::make_handoff(issued, {identity.selection, std::string(app)});
                require(bool(handoff), "Shipped synthetic handoff accepts literal admission");
                std::string json =
                    R"({"status":200,"data":{"status":"active","plan":{},"usage":{"requests_quota":1000,"requests_remaining":7})";
                if (!test.json.empty())
                    json += R"(,"app_id":)" + std::string(test.json);
                if (test.duplicate_field)
                    json += R"(,"app_\u0069d":17)";
                json += "}}";
                const auto size = json.size();
                const oer::Clocks receipt{0, oer::Elapsed{"test-clock", 4}};
                const auto reply = wire::sanitize_reply(
                    *handoff,
                    {200,
                     {{"Date", "Thu, 01 Jan 2026 00:00:00 GMT"}, {"X-Fixture", std::string(app)}},
                     std::move(json),
                     size,
                     size,
                     receipt});
                require(reply.identity == identity && reply.dispatch == oer::Dispatch::sent,
                        "Shipped reply retains independent admission and dispatch evidence");
                if (test.duplicate_field) {
                    const auto *failure = std::get_if<oer::TransportFailure>(&reply.payload);
                    require(failure && failure->kind == oer::TransportFailureKind::invalid_document,
                            "Shipped sanitizer rejects escaped duplicate echo before projection");
                    return;
                }
                const auto *frame = std::get_if<oer::SanitizedFrame>(&reply.payload);
                require(frame && frame->status == 200 && frame->receipt == receipt &&
                            frame->wire_bytes == size && frame->decoded_bytes == size &&
                            frame->echo && frame->echo->verified_for(identity) &&
                            frame->echo->match == test.expected,
                        "Shipped sanitizer supplies the independently predicted safe echo fact");
                require(frame->headers == oer::Headers{{"Date", "Thu, 01 Jan 2026 00:00:00 GMT"}} &&
                            frame->body.find("requests_remaining") != std::string::npos &&
                            frame->body.find("app_id") == std::string::npos &&
                            frame->body.find(app) == std::string::npos &&
                            frame->body.find(upper_app) == std::string::npos,
                        "Retained reply omits raw echoes and credential-bearing headers");
            };
            std::uint64_t order = 80;
            for (const auto &row : cases) {
                auto use = reservation(r1, ox, order++);
                use.route = oer::Operation::usage;
                sanitized(use, row);
            }

            Fixture f;
            f.put(change(Verb::put, {}, ox), app, r1);
            require(f.db.envelope(ox_blob).size() == 124, "OER payload produces 124-byte SCR1");
            f.owner.publish_selection(reservation(r1, ox).selection);
            order = 80;
            for (const auto &row : cases) {
                auto use = reservation(r1, ox, order++);
                use.route = oer::Operation::usage;
                f.reserve(use);
                auto attempt = deliver(f, use, app);
                const auto identity = admission(use);
                auto reply = identity;
                if (row.foreign)
                    ++reply.reservation_order;
                const native::Controls frees(native::Fault::none, 32);
                auto echo = row.shape == EchoShape::single ? guarded(row.value) : nullptr;
                auto fact =
                    f.owner.complete_rate_usage_echo(attempt, reply, {row.shape, std::move(echo)});
                require(!attempt, "Every usage completion consumes guarded attempt material");
                if (row.shape == EchoShape::single)
                    require(frees.observation().releases == 1,
                            "Completion releases the owned guarded echo exactly once");
                if (row.foreign) {
                    require(!fact, "A foreign reply cannot mint a bound fact");
                } else {
                    require(fact && fact->admission == identity &&
                                fact->selection == use.selection && fact->match == row.expected,
                            "Guarded completion yields only bound match, mismatch or unknown");
                }
                require(
                    !f.owner.complete_rate_usage_echo(attempt, identity, {EchoShape::absent, {}}),
                    "Duplicate finish cannot mint an eligible fact");
            }
        });
    }

    void F9_boundsAndDefaultWriterBudget() {
        kf::checked([] {
            Fixture f;
            const auto admit_put = [&](RateCredentialScope scope, std::size_t size, Result want,
                                       unsigned char fill = 'A', bool authorized = true) {
                const auto put = change(Verb::put, {}, std::move(scope));
                Buffer input(std::max<std::size_t>(size, 1));
                require(input.bytes().size() == std::max<std::size_t>(size, 1),
                        "Admission probe value uses checked guarded storage");
                auto value = input.bytes().first(size);
                std::ranges::fill(value, fill);
                const ManagementGrant grant = authorized ? ManagementGrant{put} : std::nullopt;
                expect(f.owner.admit_change(put, grant, value), want,
                       "Admission applies size, shape and then authority before any writer unit");
            };
            {
                require(native::prepare_sdk(), "SDK ready before observing refusal draws");
                const native::Controls observe;
                admit_put(fx, 0, Result::invalid_input);
                auto no_ref = fx;
                no_ref.account.clear();
                admit_put(no_ref, 65509, Result::resource_limit);
                auto too_long = fx;
                too_long.slot = std::string(4097, 'S');
                admit_put(too_long, 3, Result::resource_limit);
                admit_put(ox, 33, Result::resource_limit);
                admit_put(ox, 31, Result::invalid_input);
                admit_put(ox, 32, Result::invalid_input, 'z');
                admit_put(ox, 32, Result::invalid_input, 'z', false);
                admit_put(ox, 32, Result::not_authorized, 'A', false);
                require(observe.observation().nonce_draws == 0,
                        "Shape and authority refusals draw neither revision nor nonce");
            }
            require(f.db.rows("SELECT scope FROM subathon_provider_credentials").empty(),
                    "Shape and authority refusals write no credential row");

            f.put(change(Verb::put), "x", r1);
            require(f.db.envelope().size() == 93, "One opaque value byte gives 93-byte SCR1");
            // Construct the entire fixed upper-boundary value in guarded memory.
            const auto put = change(Verb::put, r1);
            Buffer maximum(65508);
            require(maximum.bytes().size() == 65508, "Maximum fixture value is locked");
            std::ranges::fill(maximum.bytes(), 'x');
            const auto value = std::string_view(
                reinterpret_cast<const char *>(maximum.bytes().data()), maximum.bytes().size());
            f.put(put, value, r2, second_draws);
            require(f.db.envelope().size() == 65600, "Maximum opaque value gives 65,600-byte SCR1");

            const RateCredentialScope wide{std::string(4096, 'I'), fx.provider,
                                           std::string(4096, 'A'), std::string(4096, 'S')};
            const auto encoded = QByteArray::fromHex("1000") + QByteArray(4096, 'I') +
                                 QByteArray::fromHex("08011000") + QByteArray(4096, 'A') +
                                 QByteArray::fromHex("1000") + QByteArray(4096, 'S');
            f.put(change(Verb::put, {}, wide), v, rx, std::array{rx, filled(0xa3)});
            require(encoded.size() == 12296 && f.db.envelope(encoded).size() == 95,
                    "Three independently framed maximum refs fit default writer budgets");
            require(f.db.rows("SELECT count(*) FROM subathon_provider_credentials")[0][0].toInt() ==
                        2,
                    "Boundary/refusal probes create only the two authorized FX rows");
        });
    }

    void F10_rotationCodecCompatibility() {
        kf::checked([] {
            Fixture f;
            connector(f);
            const auto original_connector = f.db.envelope(blob(cv::scope));
            auto replacement_file = cv::key_file;
            replacement_file[4] ^= 1;
            replacement_file[20] ^= 1;
            const record::KeyIdView next_id(replacement_file.data() + 4, 16);
            const record::KeyView next_key(replacement_file.data() + 20, 32);
            const auto alternate = f.keys.file("synthetic-rotation-key");
            kf::KeyFixture::write(alternate, blob(replacement_file));
            auto protected_replacement = key_at(alternate);

            const auto qualify = [&](const QByteArray &scope, const QByteArray &sealed,
                                     record::KeyView material) -> std::optional<CredentialValue> {
                const bool is_connector = scope == blob(cv::scope);
                if (!is_connector && validate_rate_scope(bytes(scope)) != Result::present)
                    return {};
                auto opened = record::open_record(bytes(scope), bytes(sealed), cv::id, material);
                auto *payload = std::get_if<CredentialValue>(&opened);
                if (!payload)
                    return {};
                if (!is_connector) {
                    const auto inspection = inspect_rate_payload(bytes(scope), payload->bytes());
                    if (inspection.status != Result::present || inspection.revision != r1)
                        return {};
                }
                return std::move(*payload);
            };
            const auto reseal = [&](const QByteArray &scope, const QByteArray &sealed,
                                    Revision nonce) {
                auto prior = qualify(scope, sealed, cv::key);
                require(bool(prior), "Old material qualifies the exact source row");
                record::EncodedBytes output;
                {
                    const std::array<Revision, 1> draws{nonce};
                    const native::Controls observe(native::Fault::none, 0, draws);
                    output = record::seal_record(bytes(scope), prior->bytes(), next_id, next_key);
                    require(observe.observation().nonce_draws == 1,
                            "Modeled reseal draws one distinct new nonce");
                }
                const auto *next = std::get_if<record::RecordBytes>(&output);
                require(next && blob(*next) != sealed && blob(*next).mid(4, 16) == blob(next_id) &&
                            blob(*next).mid(20, 24) == blob(nonce),
                        "Reseal changes record, public key identifier and nonce");
                {
                    auto verified = record::open_record(bytes(scope), *next, next_id, next_key);
                    const auto *payload = std::get_if<CredentialValue>(&verified);
                    require(payload && std::ranges::equal(payload->bytes(), prior->bytes()),
                            "New key verifies full byte equality without disclosing plaintext");
                    if (scope == fx_blob) {
                        const auto inspected = inspect_rate_payload(bytes(scope), payload->bytes());
                        expect(inspected.status, Result::present,
                               "New key preserves qualified FX payload");
                        require(inspected.revision == r1 &&
                                    std::ranges::equal(payload->bytes().subspan(28), bytes(v)),
                                "FX credential revision and value survive reseal exactly");
                    }
                }
                kf::expect_error(record::open_record(bytes(scope), *next, cv::id, cv::key),
                                 CredentialError::key_unavailable);
                return blob(*next);
            };
            const auto next_connector =
                reseal(blob(cv::scope), original_connector, cv::second_nonce);
            f.put(change(Verb::put), v, r1);
            const auto original = f.db.envelope();
            auto changed_scope = fx_blob;
            changed_scope[3] = '\x0a';
            Fixture unknown_family;
            unknown_family.db.inject(changed_scope, original);
            const auto foreign_rows =
                unknown_family.db.rows("SELECT scope,record FROM subathon_provider_credentials");
            require(foreign_rows.size() == 1 && foreign_rows[0][0].toByteArray() == changed_scope &&
                        foreign_rows[0][1].toByteArray() == original,
                    "Disposable inventory carries an actual unknown-tag row");
            expect(validate_rate_scope(bytes(foreign_rows[0][0].toByteArray())),
                   Result::invalid_record, "Rotation codec refuses an unknown scope family");
            auto tag = original;
            tag.back() ^= 1;
            auto ciphertext = original;
            ciphertext[48] ^= 1;
            require(!qualify(fx_blob, tag, cv::key) && !qualify(fx_blob, ciphertext, cv::key) &&
                        !qualify(fx_blob, original, next_key) &&
                        !qualify(changed_scope, original, cv::key),
                    "Tag, ciphertext, wrong material with same ID and unknown family refuse");

            const auto inventory =
                f.db.rows("SELECT scope,record FROM subathon_provider_credentials ORDER BY scope");
            require(inventory.size() == 2, "Inventory contains both exact encrypted families");
            const auto next_fx = reseal(fx_blob, original, n2);
            f.db.committed([&](const auto &s) {
                (void)s.execute({upsert_sql, {fx_blob, next_fx}});
                (void)s.execute({upsert_sql, {blob(cv::scope), next_connector}});
            });
            f.key = std::move(protected_replacement);
            expect_connector(f);
            const auto rotated = fresh(r1, 100);
            f.publish(rotated);
            (void)deliver(f, rotated, v);
        });
    }

    void F11_restoreRequiresFreshRecoveryAuthority() {
        kf::checked([] {
            Fixture f;
            f.put(change(Verb::put), v, r1);
            const auto older = f.db.envelope();
            const auto old = reservation();
            f.publish(old);
            auto pre_restore = f.permit(old);
            f.put(change(Verb::put, r1), w, r2, second_draws);
            const auto newer = f.db.envelope();
            independent_open(fx_blob, older, r1, n1, v);
            independent_open(fx_blob, newer, r2, n2, w);

            // Materialize the older authenticated row in isolated recovery state.
            Fixture recovery;
            recovery.db.inject(fx_blob, older);
            recovery.owner.revoke(fx, "recovery-generation");
            blocked(recovery, pre_restore, old, old);
            recovery.refused_use(old, old, Result::not_authorized, true, true);
            auto wrong = fresh(r2, 111);
            wrong.selection.generation = "post-revocation-generation";
            recovery.publish(wrong);
            recovery.refused_use(wrong, wrong, Result::stale);
            auto reviewed = fresh(r1, 112);
            reviewed.selection.generation = "reviewed-recovery-generation";
            recovery.publish(reviewed);
            (void)deliver(recovery, reviewed, v);
        });
    }

    void F12_keylessMalformedAuthenticatedPayloadAndRedaction() {
        kf::checked([] {
            Fixture f;
            constexpr auto canary = "synthetic-wire-canary"sv;
            connector(f);
            // Open has no credential in its synthetic setup and no repository/SDK probe.
            SyntheticSetup setup;
            setup.setup = {
                {era::Profile::open, {"I", era::ScopeKind::open_usage, "Q"}, "C", "T", {}},
                {},
                {},
                era::AuthorizedChange::unchanged,
                era::ScopeContinuity::qualified};
            SyntheticHttp http(1024);
            {
                const native::Controls keyless(native::Fault::readiness);
                era::Adapter open(setup, http);
                require(open.select() == era::SelectionStatus::selected, "Keyless Open selects");
                const auto started = open.start(latest("keyless"), tick);
                require(std::holds_alternative<era::Started>(started) && http.gets == 1 &&
                            http.shape && keyless.observation().readiness == 0,
                        "Keyless Open uses one synthetic route and no credential SDK work");
            }

            LogCheck logs;
            f.db.committed([](const auto &s) {
                (void)s.execute({"CREATE UNIQUE INDEX \"synthetic-native-canary\" ON "
                                 "subathon_provider_credentials ((1)) "
                                 "WHERE length(record) IN (71,95)",
                                 {}});
            });
            f.put(change(Verb::put), canary, r1);
            const auto current = reservation();
            f.publish(current);
            f.refused_use(current, current, Result::key_unavailable, false);
            unsigned char nonce = 0xa3;
            for (const auto &[magic, secret] :
                 {std::pair{"ZZZ1"sv, "x"sv}, std::pair{"FXR1"sv, ""sv}}) {
                Buffer payload(28 + secret.size());
                const auto bad = seal_fixture(fx_blob, fill_payload(payload, r1, secret, magic),
                                              filled(nonce++));
                f.db.inject(fx_blob, bad);
                f.refused_use(current, current, Result::invalid_record);
            }
            // SQLite identifies an expression index by name in its native
            // constraint error. The 71-byte connector occupies the index.
            // Replacement by the 95-byte W record must conflict and roll back.
            Buffer valid(28 + canary.size());
            const auto original =
                seal_fixture(fx_blob, fill_payload(valid, r1, canary), filled(nonce));
            f.db.inject(fx_blob, original);
            const auto replace = change(Verb::put, r1);
            const auto aborted = f.stage(replace, w, second_draws);
            require(aborted.completion.transaction == TransactionOutcome::rolled_back &&
                        aborted.completion.error == WorkError::sql_failure &&
                        !aborted.completion.value && f.db.envelope() == original,
                    "Typed SQL failure rolls back and releases no plaintext or success value");
            expect(f.owner.finish_change(replace, aborted.completion), Result::known_rollback,
                   "Acknowledged rollback reopens still-current selection");
            const auto resumed = reservation(r1, fx, 120);
            f.reserve(resumed);
            (void)deliver(f, resumed, canary);
            require(logs.clean(),
                    "Visible logs omit guarded bytes, FX framing, paths and raw errors");
        });
    }

    void F13_mutationFenceSpansCommitDeliveryAndRollback() {
        kf::checked([] {
            for (const auto control : {Control::commit, Control::rollback}) {
                const auto old = reservation();
                Fixture f;
                f.put(change(Verb::put), v, r1);
                const auto original = f.db.envelope();
                f.publish(old);
                auto unused = f.permit(old);
                const auto still_queued = reservation(r1, fx, 130);
                f.reserve(still_queued);
                auto untouched = f.permit(still_queued);
                const auto replace = change(Verb::put, r1);
                const auto staged = f.stage(replace, w, second_draws, control);
                expect(staged.outcome.status, Result::staged_put,
                       "Replacement stages behind fence");
                require(staged.outcome.revision == r2 && staged.draws == 2,
                        "Replacement stage witnesses revision then nonce");
                const bool committed = control == Control::commit;
                if (committed) {
                    normal(staged.completion);
                    independent_open(fx_blob, f.db.envelope(), r2, n2, w);
                } else {
                    require(staged.completion.transaction == TransactionOutcome::rolled_back &&
                                staged.completion.error == WorkError::not_committed &&
                                !staged.completion.value && f.db.envelope() == original,
                            "Known rollback leaves r1 and publishes no normal work value");
                }

                // Keep the normal writer callback's result undelivered to the
                // serialized credential owner while other owner work runs.
                blocked(f, unused, old, old);
                const auto while_fenced = reservation(r1, fx, 131);
                f.reserve(while_fenced);
                f.refused_use(while_fenced, while_fenced, Result::held);
                expect(f.owner.finish_change(replace, staged.completion),
                       committed ? Result::committed_change : Result::known_rollback,
                       "Owner accepts distinct commit or proved rollback");
                blocked(f, untouched, still_queued, still_queued);
                if (committed) {
                    const auto previous = reservation(r1, fx, 132);
                    f.reserve(previous);
                    f.refused_use(previous, previous, Result::stale);
                    const auto replacement = fresh(r2, 133);
                    f.publish(replacement);
                    (void)deliver(f, replacement, w);
                } else {
                    const auto resumed = reservation(r1, fx, 134);
                    f.reserve(resumed);
                    (void)deliver(f, resumed, v);
                }
            }
        });
    }

    void F14_uncertainInsertAbsenceCandidateOrDisagreement() {
        kf::checked([] {
            enum class Witness { absent, candidate, unrelated };
            for (const auto witness : {Witness::absent, Witness::candidate, Witness::unrelated}) {
                Fixture f;
                const auto current = reservation(r1, fx, 140);
                f.publish(current);
                const auto insert = change(Verb::put);
                const bool absent = witness == Witness::absent;
                if (absent) {
                    f.db.lose_ack(test::WorkControlFault::lose_next_rollback_ack);
                } else {
                    f.db.lose_commit_ack();
                }
                const auto staged =
                    f.stage(insert, v, first_draws, absent ? Control::rollback : Control::commit);
                expect(staged.outcome.status, Result::staged_put,
                       "Initial empty-expectation insert stages");
                require(staged.outcome.revision == r1 && staged.draws == 2,
                        "Retained insert candidate is r1 with exactly one attempted mutation");
                unknown(staged.completion);
                expect(f.owner.finish_change(insert, staged.completion), Result::held,
                       "Abnormal insert completion cannot publish mutation success");
                f.db.reopen();
                if (witness == Witness::unrelated) {
                    Buffer other(31);
                    const auto unrelated =
                        seal_fixture(fx_blob, fill_payload(other, rx, v), filled(0xa3));
                    f.db.inject(fx_blob, unrelated);
                }
                const auto after_uncertain = f.db.envelope();
                f.refused_use(current, current, Result::held);
                auto [read, done] = f.readback();
                expect(read.status, absent ? Result::missing : Result::present,
                       "Healthy authorized management readback reports actual row state");
                const auto expected =
                    absent ? std::nullopt : std::optional{witness == Witness::candidate ? r1 : rx};
                require(read.revision == expected,
                        "Readback reports only exact revision or absence");
                expect(f.owner.reconcile(insert, read, done),
                       witness == Witness::unrelated ? Result::held : Result::reconciled,
                       "Empty expectation allows proved absence, never an unrelated revision");
                require(f.db.envelope() == after_uncertain,
                        "Reconciliation performs no blind insert retry or replacement");
                if (absent) {
                    require(after_uncertain.isEmpty(),
                            "Resolved absence cannot select a credential");
                    // Authorize an exact lookup of the absent row.
                    const auto absent_probe = fresh(r1, 143);
                    f.publish(absent_probe);
                    f.refused_use(absent_probe, absent_probe, Result::missing);
                } else if (witness == Witness::candidate) {
                    f.refused_use(current, current, Result::stale, true, true);
                    const auto selected = fresh(r1, 141);
                    f.publish(selected);
                    (void)deliver(f, selected, v);
                } else {
                    const auto competing = fresh(rx, 142);
                    f.publish(competing);
                    f.refused_use(competing, competing, Result::held);
                }
            }
        });
    }

    void F15_transportCapAndShippedNotSentAccounting() {
        kf::checked([] {
            constexpr std::string_view value =
                "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
            static_assert(value.size() == 65);
            constexpr std::size_t cap = 64;
            const auto request = reservation(r1, fx, 150);
            // The synthetic HTTP port models refusal of a complete target.
            // Only dispatch accounting and retry come from shipped #398.
            SyntheticSetup setup;
            const era::SelectionBinding binding{
                era::Profile::pro,
                {"I", era::ScopeKind::account_quota, "Q"},
                "C",
                "T",
                std::string(reinterpret_cast<const char *>(r1.data()), r1.size())};
            setup.setup = {binding,
                           era::Credential{binding, std::string(value)},
                           {},
                           era::AuthorizedChange::unchanged,
                           era::ScopeContinuity::qualified};
            SyntheticHttp http(cap);
            era::Adapter adapter(setup, http);
            require(adapter.select() == era::SelectionStatus::selected,
                    "Exact synthetic Pro selects");
            const auto start = adapter.start(latest(request.request), tick);
            const auto *started = std::get_if<era::Started>(&start);
            require(started && started->dispatch.kind == era::DispatchKind::not_sent &&
                        !started->dispatch.time && http.gets == 0,
                    "Target above the qualified cap sends zero GETs and has no send time");
            const auto outcome = adapter.finish(
                *started, era::FailedTransport{era::TransportFailure::resource_limit, tick});
            const auto *failure = std::get_if<era::Failure>(&outcome.outcome);
            require(failure && failure->kind == era::FailureKind::resource_limit &&
                        outcome.dispatch.kind == era::DispatchKind::not_sent && !failure->safe_code,
                    "Refusal is redacted resource_limit with independent not_sent evidence");
            const auto quota = adapter.quota_view(binding.scope, tick);
            require(quota.local_total == era::Usage{0, 0} && !quota.quota_held,
                    "Not-sent admission charges zero quota and adds no quota hold");
            const auto waiting = adapter.start(latest("backoff"), tick);
            const auto *wait = std::get_if<era::Failure>(&waiting);
            require(wait && wait->kind == era::FailureKind::retry_wait && http.gets == 0,
                    "Existing local adapter backoff applies without adding a credential hold");

            Fixture f;
            const auto put = change(Verb::put);
            f.put(put, value, r1);
            const auto envelope = f.db.envelope();
            require(envelope.size() == 157, "Wire-ineligible stored value has exact 157-byte SCR1");
            struct BoundedHandoff final : QualifiedTransport {
                explicit BoundedHandoff(std::size_t cap) : target_cap(cap) {}
                void accept(const Reservation &,
                            std::span<const unsigned char> secret) noexcept override {
                    ++offers;
                    // Each opaque byte occupies at least one encoded byte.
                    // This witness can refuse before constructing any URL.
                    if (secret.size() > target_cap)
                        local = era::TransportFailure::resource_limit;
                }
                std::size_t target_cap;
                int offers{};
                std::optional<era::TransportFailure> local;
            } transport(cap);
            f.publish(request);
            auto permit = f.permit(request);
            Attempt accepted;
            expect(f.owner.consume_keyed_rate_use(permit, request, transport, accepted),
                   Result::accepted, "Qualified private transport accepts exactly one offer");
            require(!permit && accepted && transport.offers == 1 &&
                        transport.local == era::TransportFailure::resource_limit,
                    "Guarded input alone exceeds the complete target cap");

            expect(f.owner.finish_attempt(accepted, request, outcome.dispatch),
                   Result::current_reply,
                   "Local refusal finishes exact guarded attempt without marking a send");
            require(!accepted && f.db.envelope() == envelope,
                    "Completion relinquishes the owning attempt and preserves the exact record");
            auto [read, done] = f.readback();
            expect(read.status, Result::present,
                   "Private readback leaves stored revision unchanged");
            require(read.revision == r1, "No automatic replacement is attempted");
        });
    }
};
} // namespace

QTEST_GUILESS_MAIN(RateCredentialTests)
#include "tests.moc"
