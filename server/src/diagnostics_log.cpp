// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics_private.hpp"

#include <QDate>
#include <QFile>
#include <QJsonDocument>
#include <algorithm>
#include <cerrno>
#include <condition_variable>
#include <dirent.h>
#include <fcntl.h>
#include <future>
#include <map>
#include <span>
#include <subathon/server/testing/diagnostics.hpp>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

namespace subathon::server::diagnostics {
namespace {
using namespace detail;
constexpr std::size_t max_files = 366;
constexpr std::size_t max_rows = 245'760;
constexpr off_t max_bytes = 60 * 1024 * 1024;

class Fd {
  public:
    explicit Fd(int value = -1) : fd_(value) {}
    ~Fd() { (void)close(); }
    Fd(Fd &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    Fd &operator=(Fd &&other) noexcept {
        (void)close();
        fd_ = std::exchange(other.fd_, -1);
        return *this;
    }
    int get() const { return fd_; }
    explicit operator bool() const { return fd_ >= 0; }
    int release() { return std::exchange(fd_, -1); }
    bool close() { return fd_ < 0 || ::close(std::exchange(fd_, -1)) == 0; }

  private:
    int fd_;
};

bool same_identity(const struct stat &a, const struct stat &b) {
    return a.st_dev == b.st_dev && a.st_ino == b.st_ino;
}
bool private_dir(const struct stat &value) {
    return S_ISDIR(value.st_mode) && (value.st_mode & 07577) == 0500 && value.st_uid == geteuid();
}
bool private_file(const struct stat &value) {
    return S_ISREG(value.st_mode) && (value.st_mode & 07577) == 0400 && value.st_uid == geteuid() &&
           value.st_nlink == 1;
}
bool same_file(const struct stat &a, const struct stat &b) {
    return private_file(a) && private_file(b) && same_identity(a, b) && a.st_size == b.st_size &&
           a.st_ctim.tv_sec == b.st_ctim.tv_sec && a.st_ctim.tv_nsec == b.st_ctim.tv_nsec;
}

Fd open_directory(const QString &path) {
    if (!path.startsWith('/') || path.size() > 4096 || path.contains(QChar::Null))
        return Fd{};
    Fd dir(open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC));
    const auto encoded = QFile::encodeName(path);
    if (encoded.size() > 4096 || encoded.contains('\0'))
        return Fd{};
    const auto parts = encoded.split('/');
    if (parts.size() < 2)
        return Fd{};
    for (const auto &part : std::span(parts.constData() + 1, std::size_t(parts.size() - 1))) {
        if (!dir || part.isEmpty() || part == "." || part == "..")
            return Fd{};
        dir = Fd(
            openat(dir.get(), part.constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    }
    return dir;
}

bool valid_name(QByteArrayView name) {
    if (name.size() != 14 || name.sliced(10) != ".log")
        return false;
    const auto date = name.first(10).toByteArray();
    const auto decoded = QDate::fromString(QString::fromLatin1(date), Qt::ISODate);
    return decoded.isValid() && decoded.year() > 0 && decoded.year() <= 9999 &&
           decoded.toString(Qt::ISODate).toLatin1() == date;
}

struct Day {
    struct stat info{};
    off_t complete_bytes = 0;
    std::size_t rows = 0;
    std::optional<Utc> last;
};

struct Scan {
    std::map<QByteArray, Day> days;
    std::deque<Row> tail;
    off_t bytes = 0;
    std::size_t rows = 0;
    std::optional<Utc> latest;
};

class Store {
  public:
    Store(QString path, unsigned days, const testing::LogControl *control)
        : path_(std::move(path)), window_(days >= 1 && days <= 365 ? day_ms * days : 0),
          clock_(control ? control->utc : system_utc), hook_(control ? control->mutate : nullptr) {}

    void hold() {
        std::lock_guard lock(gate_);
        held_ = true;
        invalidate();
    }

    void discontinuity() {
        std::lock_guard lock(gate_);
        invalidate();
    }

    void sweep() {
        if (auto scan = inspect()) {
            for (const auto &[name, day] : scan->days) {
                if (!day.last || expired(*day.last, scan->now)) {
                    if (!begin_mutation(Mutation::unlink, scan->latest,
                                        [this, name, day] { return unlink_day(name, day); }))
                        return;
                    if (outcome_ == MutationOutcome::changed) {
                        bytes_ -= day.info.st_size;
                        rows_ -= day.rows;
                        days_.erase(name);
                    }
                }
            }
            flush_notices();
        }
    }

    View snapshot() {
        auto scan = inspect();
        if (!scan)
            return {};
        auto &tail = scan->tail;
        while (!tail.empty() && expired(tail.front().utc, scan->now))
            tail.pop_front();
        const bool limited = tail.size() > tail_limit;
        if (limited)
            tail.pop_front();
        std::lock_guard lock(gate_);
        if (!ready_ || held_ || pending_ || epoch_ != scan->epoch)
            return {};
        return {std::move(tail), scan->epoch, true, limited};
    }

    // This lock orders the verdict and transfer with mutation starts and clock
    // invalidations. The owner never holds it across a filesystem operation.
    void publish(View view, const std::function<void(View)> &transfer) {
        std::unique_lock lock(gate_, std::try_to_lock);
        if (!lock || held_ || pending_) {
            transfer({});
            return;
        }
        const auto now = checked_locked(latest_);
        if (!now || !view.available || !ready_ || epoch_ != view.epoch) {
            transfer({});
            return;
        }
        while (!view.rows.empty() && expired(view.rows.front().utc, *now))
            view.rows.pop_front();
        transfer(std::move(view));
    }

    void record(Observation observation) {
        if (!current_time())
            return;
        if ((!have_usage_ || !ready()) && !inspect())
            return;
        flush_notices();
        if (append(observation) == Append::capacity) {
            // An expired file may free space. Keep notices coalesced if all
            // eligible records still occupy the store.
            if (auto scan = inspect()) {
                bool reclaimable = std::ranges::any_of(scan->days, [&](const auto &entry) {
                    return !entry.second.last || expired(*entry.second.last, scan->now);
                });
                if (reclaimable) {
                    sweep();
                    if (append(observation) != Append::capacity)
                        return;
                }
            }
            capacity_notice_ = true;
        }
    }

    std::vector<Notice> notices() {
        std::lock_guard lock(gate_);
        std::vector<Notice> result;
        if (retention_notice_)
            result.push_back(Notice::retention_unavailable);
        if (capacity_notice_)
            result.push_back(Notice::capacity_reached);
        return result;
    }

    MutationOutcome run_pending() {
        if (!deferred_ || ran_deferred_)
            return MutationOutcome::uncertain;
        ran_deferred_ = true;
        return (*deferred_)();
    }
    void settle_pending(MutationOutcome outcome) {
        if (!deferred_)
            return;
        deferred_.reset();
        ran_deferred_ = false;
        have_usage_ = false;
        settle(outcome, true);
    }

  private:
    struct TimedScan : Scan {
        Utc now = 0;
        std::uint64_t epoch = 0;
    };
    enum class Append { recorded, refused, capacity };

    bool ready() {
        std::lock_guard lock(gate_);
        return ready_;
    }
    bool expired(Utc at, Utc now) const { return now - at >= window_; }

    void invalidate() {
        ready_ = false;
        retention_notice_ = true;
        if (epoch_ == std::numeric_limits<std::uint64_t>::max())
            held_ = true;
        else
            ++epoch_;
    }
    std::optional<Utc> checked_locked(std::optional<Utc> latest) {
        std::optional<Utc> now;
        try {
            if (clock_ && window_ && !held_)
                now = clock_();
        } catch (...) {
            // Reject a failed observation without disclosing producer text.
        }
        if (!now || *now < min_utc || *now > max_utc || (last_sample_ && *now < *last_sample_)) {
            invalidate();
            return {};
        }
        if (latest && *latest > *now) {
            invalidate();
            return {};
        }
        last_sample_ = now;
        return now;
    }
    std::optional<Utc> current_time(std::optional<Utc> scanned = {}) {
        std::lock_guard lock(gate_);
        if (held_ || pending_)
            return {};
        return checked_locked(scanned ? scanned : latest_);
    }

    bool verify_directory() {
        auto fresh = open_directory(path_);
        struct stat observed{}, retained{};
        if (!fresh || fstat(fresh.get(), &observed) != 0 || !private_dir(observed))
            return false;
        if (!directory_) {
            directory_ = std::move(fresh);
            return true;
        }
        return fstat(directory_.get(), &retained) == 0 && private_dir(retained) &&
               same_identity(observed, retained);
    }
    bool named(const QByteArray &name, const struct stat &expected) const {
        struct stat entry{};
        return fstatat(directory_.get(), name.constData(), &entry, AT_SYMLINK_NOFOLLOW) == 0 &&
               same_file(expected, entry);
    }
    Fd open_file(const QByteArray &name, int flags, struct stat &info) const {
        Fd fd(openat(directory_.get(), name.constData(),
                     flags | O_NONBLOCK | O_NOFOLLOW | O_CLOEXEC));
        if (!fd || fstat(fd.get(), &info) != 0 || !named(name, info))
            return Fd{};
        return fd;
    }

    bool scan_day(const QByteArray &name, Day &day, Scan &scan) {
        auto fd = open_file(name, O_RDONLY, day.info);
        if (!fd || day.info.st_size < 0 || day.info.st_size > max_bytes - scan.bytes)
            return false;
        scan.bytes += day.info.st_size;
        const auto date =
            QDate::fromString(QString::fromLatin1(QByteArrayView(name).first(10)), Qt::ISODate);
        const auto day_start = QDate(1970, 1, 1).daysTo(date) * day_ms;
        off_t offset = 0;
        std::array<char, 16 * 1024> buffer;
        std::array<char, 255> line;
        std::size_t size = 0;
        while (offset < day.info.st_size) {
            const auto count =
                read(fd.get(), buffer.data(),
                     std::size_t(std::min<off_t>(buffer.size(), day.info.st_size - offset)));
            if (count <= 0)
                return false;
            for (ssize_t i = 0; i < count; ++i) {
                const auto byte = buffer[std::size_t(i)];
                if (byte != '\n') {
                    if (size == line.size())
                        return false;
                    line[size++] = byte;
                    continue;
                }
                if (scan.rows == max_rows)
                    return false;
                const auto row = parse_row({line.data(), qsizetype(size)});
                if (!row || row->utc < day_start || row->utc >= day_start + day_ms ||
                    (day.last && row->utc < *day.last))
                    return false;
                day.last = row->utc;
                scan.latest = row->utc;
                day.complete_bytes = offset + i + 1;
                ++day.rows;
                ++scan.rows;
                size = 0;
                if (scan.tail.size() == tail_limit + 1)
                    scan.tail.pop_front();
                scan.tail.push_back(*row);
            }
            offset += count;
        }
        return named(name, day.info) && fd.close();
    }

    bool scan_files(Scan &scan) {
        if (!verify_directory())
            return false;
        Fd descriptor(openat(directory_.get(), ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC));
        if (!descriptor)
            return false;
        const auto close_dir = [](DIR *value) { (void)closedir(value); };
        const std::unique_ptr<DIR, decltype(close_dir)> dir(fdopendir(descriptor.get()), close_dir);
        if (!dir)
            return false;
        // fdopendir owns its descriptor on success. Keep the verified service
        // directory separately pinned for every file access.
        (void)descriptor.release();
        for (;;) {
            errno = 0;
            auto *entry = readdir(dir.get());
            if (!entry) {
                if (errno != 0)
                    return false;
                break;
            }
            const QByteArrayView name(entry->d_name);
            if (name == "." || name == "..")
                continue;
            if (scan.days.size() == max_files || !valid_name(name) ||
                !scan.days.try_emplace(name.toByteArray()).second)
                return false;
        }
        for (auto &[name, day] : scan.days)
            if (!scan_day(name, day, scan))
                return false;
        return verify_directory();
    }

    std::optional<TimedScan> inspect() {
        if (!current_time())
            return {};
        TimedScan scan;
        if (!scan_files(scan)) {
            hold();
            return {};
        }
        if (!current_time(scan.latest))
            return {};
        for (auto &[name, day] : scan.days) {
            if (day.complete_bytes == day.info.st_size)
                continue;
            if (adopted_) {
                hold();
                return {};
            }
            if (!begin_mutation(Mutation::repair, scan.latest,
                                [this, name, day] { return repair_day(name, day); }))
                return {};
            if (outcome_ != MutationOutcome::changed || !read_stat(name, day)) {
                hold();
                return {};
            }
        }
        scan.bytes = 0;
        for (const auto &[name, day] : scan.days)
            scan.bytes += day.info.st_size;
        {
            std::lock_guard lock(gate_);
            const auto now = checked_locked(scan.latest);
            if (!now)
                return {};
            latest_ = scan.latest;
            scan.now = *now;
            scan.epoch = epoch_;
            ready_ = true;
        }
        days_ = scan.days;
        bytes_ = scan.bytes;
        rows_ = scan.rows;
        adopted_ = have_usage_ = true;
        return scan;
    }

    bool read_stat(const QByteArray &name, Day &day) const {
        struct stat info{};
        if (fstatat(directory_.get(), name.constData(), &info, AT_SYMLINK_NOFOLLOW) != 0 ||
            !private_file(info) || !same_identity(info, day.info) ||
            info.st_size != day.complete_bytes)
            return false;
        day.info = info;
        return true;
    }

    MutationOutcome repair_day(const QByteArray &name, const Day &day) {
        struct stat info{};
        auto fd = open_file(name, O_WRONLY, info);
        if (!fd || !same_file(day.info, info) || ftruncate(fd.get(), day.complete_bytes) != 0 ||
            fsync(fd.get()) != 0 || !fd.close())
            return MutationOutcome::uncertain;
        return MutationOutcome::changed;
    }

    MutationOutcome unlink_day(const QByteArray &name, const Day &day) {
        if (!named(name, day.info))
            return MutationOutcome::uncertain;
        if (unlinkat(directory_.get(), name.constData(), 0) != 0)
            return named(name, day.info) ? MutationOutcome::unchanged_refusal
                                         : MutationOutcome::uncertain;
        return fsync(directory_.get()) == 0 ? MutationOutcome::changed : MutationOutcome::uncertain;
    }

    bool begin_mutation(Mutation kind, std::optional<Utc> latest,
                        std::function<MutationOutcome()> work) {
        {
            std::lock_guard lock(gate_);
            if (held_ || pending_ || !checked_locked(latest ? latest : latest_))
                return false;
            pending_ = true;
        }
        auto verify_and_run = [this, work = std::move(work)] {
            return verify_directory() ? work() : MutationOutcome::uncertain;
        };
        const auto result = hook_ ? hook_(kind, verify_and_run) : std::optional(verify_and_run());
        if (!result) {
            deferred_ = std::move(verify_and_run);
            return false;
        }
        settle(*result);
        return *result != MutationOutcome::uncertain;
    }

    void settle(MutationOutcome outcome, bool deferred = false) {
        std::lock_guard lock(gate_);
        outcome_ = outcome;
        if (outcome == MutationOutcome::uncertain) {
            held_ = true;
            invalidate();
        } else if (deferred) {
            // Before another snapshot or append the owner must recover usage
            // after the suspended operation. A staged pre-mutation view stays
            // safe once the owner proves the current view.
            ready_ = false;
        }
        pending_ = false;
    }

    MutationOutcome write_line(const QByteArray &name, const QByteArray &line, Day &day,
                               bool exists) {
        struct stat info{};
        Fd fd;
        if (exists) {
            fd = Fd(openat(directory_.get(), name.constData(),
                           O_WRONLY | O_APPEND | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC));
            if (!fd)
                return named(name, day.info) ? MutationOutcome::unchanged_refusal
                                             : MutationOutcome::uncertain;
            if (fstat(fd.get(), &info) != 0 || !same_file(day.info, info) || !named(name, info))
                return MutationOutcome::uncertain;
        } else {
            fd = Fd(openat(directory_.get(), name.constData(),
                           O_WRONLY | O_APPEND | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600));
            if (!fd) {
                struct stat unexpected{};
                const auto error = errno;
                if (error != EEXIST &&
                    fstatat(directory_.get(), name.constData(), &unexpected, AT_SYMLINK_NOFOLLOW) <
                        0 &&
                    errno == ENOENT)
                    return MutationOutcome::unchanged_refusal;
                return MutationOutcome::uncertain;
            }
            if (fstat(fd.get(), &info) != 0 || !named(name, info) || info.st_size != 0)
                return MutationOutcome::uncertain;
        }
        const auto written = write(fd.get(), line.constData(), std::size_t(line.size()));
        if (written != line.size()) {
            if (exists && written < 0 && named(name, info) && fd.close())
                return MutationOutcome::unchanged_refusal;
            return MutationOutcome::uncertain;
        }
        struct stat after{};
        if (fsync(fd.get()) != 0 || fstat(fd.get(), &after) != 0 || !same_identity(info, after) ||
            after.st_size != info.st_size + line.size() || !named(name, after) || !fd.close() ||
            (!exists && fsync(directory_.get()) != 0))
            return MutationOutcome::uncertain;
        day.info = after;
        day.complete_bytes = after.st_size;
        return MutationOutcome::changed;
    }

    Append append(Observation observation) {
        if (!have_usage_)
            return Append::refused;
        // Sample, encode and mark the pending append under the same clock gate,
        // so a publication sample cannot overtake the append timestamp.
        std::unique_lock lock(gate_);
        if (held_ || pending_ || !ready_)
            return Append::refused;
        const auto now = checked_locked(latest_);
        if (!now)
            return Append::refused;
        const auto name = date_name(*now);
        const auto line =
            QJsonDocument(encode({*now, observation})).toJson(QJsonDocument::Compact) + '\n';
        const auto previous = days_.find(name);
        const bool exists = previous != days_.end();
        if (rows_ == max_rows || (!exists && days_.size() == max_files) ||
            line.size() > max_bytes - bytes_)
            return Append::capacity;
        auto day = std::make_shared<Day>(exists ? previous->second : Day{});
        pending_ = true;
        lock.unlock();
        auto work = [this, name, line, day, exists] {
            return verify_directory() ? write_line(name, line, *day, exists)
                                      : MutationOutcome::uncertain;
        };
        const auto result = hook_ ? hook_(Mutation::append, work) : std::optional(work());
        if (!result) {
            deferred_ = std::move(work);
            return Append::refused;
        }
        if (*result == MutationOutcome::changed) {
            bytes_ += line.size();
            ++rows_;
            ++day->rows;
            day->last = now;
            days_.insert_or_assign(name, *day);
            std::lock_guard update(gate_);
            latest_ = now;
        }
        settle(*result);
        return *result == MutationOutcome::changed ? Append::recorded : Append::refused;
    }

    void flush_notices() {
        bool retention;
        {
            std::lock_guard lock(gate_);
            if (!ready_ || pending_ || held_)
                return;
            retention = std::exchange(retention_notice_, false);
        }
        if (retention)
            (void)append({Code::retention_unavailable, {}});
        if (capacity_notice_) {
            capacity_notice_ = false;
            if (append({Code::capacity_reached, {}}) == Append::capacity)
                capacity_notice_ = true;
        }
    }

    QString path_;
    Utc window_;
    std::function<std::optional<Utc>()> clock_;
    decltype(testing::LogControl::mutate) hook_;

    std::mutex gate_;
    std::optional<Utc> last_sample_, latest_;
    std::uint64_t epoch_ = 0;
    bool ready_ = false, pending_ = false, held_ = false, retention_notice_ = false;

    // Everything below belongs solely to the I/O thread.
    Fd directory_;
    std::map<QByteArray, Day> days_;
    off_t bytes_ = 0;
    std::size_t rows_ = 0;
    bool adopted_ = false, have_usage_ = false, capacity_notice_ = false;
    std::optional<std::function<MutationOutcome()>> deferred_;
    bool ran_deferred_ = false;
    MutationOutcome outcome_ = MutationOutcome::uncertain;
};
} // namespace

struct LogOwner::Impl {
    Impl(QString directory, unsigned days, const testing::LogControl *control)
        : store(std::move(directory), days, control), manual(control != nullptr),
          worker([this] { run(); }) {}
    ~Impl() {
        {
            std::lock_guard lock(queue_gate);
            stopping = true;
        }
        wake.notify_one();
        worker.join();
    }

    void offer(Observation observation) noexcept {
        if (manual) {
            enqueue(observation);
            return;
        }
        std::unique_lock lock(queue_gate, std::try_to_lock);
        if (!lock || stopping)
            return;
        enqueue(observation);
        lock.unlock();
        wake.notify_one();
    }
    void enqueue(Observation observation) noexcept {
        std::unique_lock lock(producer_gate, std::try_to_lock);
        if (lock && count < observations.size())
            observations[(head + count++) % observations.size()] = observation;
    }
    bool snapshot(const std::shared_ptr<Snapshot> &snapshot) {
        std::unique_lock lock(queue_gate, std::defer_lock);
        if (manual)
            lock.lock();
        else if (!lock.try_lock())
            return false;
        if (stopping || queued_snapshot || snapshot_running)
            return false;
        queued_snapshot = snapshot;
        lock.unlock();
        wake.notify_one();
        return true;
    }
    std::optional<Observation> dequeue() {
        std::lock_guard lock(producer_gate);
        if (!count)
            return {};
        auto next = observations[head];
        head = (head + 1) % observations.size();
        --count;
        return next;
    }
    void drain() {
        for (std::size_t i = 0; i < observations.size(); ++i) {
            auto next = dequeue();
            if (!next)
                return;
            store.record(*next);
        }
    }
    // Synchronous barrier is reachable only from the deterministic fixture.
    template <class F> auto test_turn(F fn) {
        using Result = std::invoke_result_t<F, Store &>;
        auto result = std::make_shared<std::promise<Result>>();
        auto future = result->get_future();
        {
            std::lock_guard lock(queue_gate);
            control_turn = [this, result, fn = std::move(fn)] {
                try {
                    if constexpr (std::is_void_v<Result>) {
                        fn(store);
                        result->set_value();
                    } else {
                        result->set_value(fn(store));
                    }
                } catch (...) {
                    store.hold();
                    result->set_exception(std::current_exception());
                }
            };
        }
        wake.notify_one();
        return future.get();
    }

    void run() {
        using Clock = std::chrono::steady_clock;
        auto next_sweep = Clock::now();
        for (;;) {
            std::shared_ptr<Snapshot> snapshot;
            std::function<void()> turn;
            bool maintenance = false;
            {
                std::unique_lock lock(queue_gate);
                const auto work = [&] {
                    return stopping || queued_snapshot || control_turn || (!manual && count);
                };
                if (manual)
                    wake.wait(lock, work);
                else
                    wake.wait_until(lock, next_sweep, work);
                if (stopping)
                    return;
                maintenance = !manual && Clock::now() >= next_sweep;
                if (!maintenance) {
                    snapshot = std::exchange(queued_snapshot, {});
                    snapshot_running = bool(snapshot);
                    if (!snapshot)
                        turn = std::exchange(control_turn, {});
                }
            }
            if (snapshot) {
                View view;
                try {
                    if (!snapshot->abandoned.load())
                        view = store.snapshot();
                } catch (...) {
                    store.hold();
                }
                {
                    std::lock_guard lock(snapshot->mutex);
                    if (!snapshot->abandoned.load())
                        snapshot->result.emplace(std::move(view));
                }
                {
                    std::lock_guard lock(queue_gate);
                    snapshot_running = false;
                }
                try {
                    if (!snapshot->abandoned.load() && snapshot->ready)
                        snapshot->ready();
                } catch (...) {
                    // A failed notification cannot publish a staged result.
                }
                continue;
            }
            if (turn) {
                turn();
                continue;
            }
            try {
                if (maintenance) {
                    next_sweep = Clock::now() + std::chrono::minutes(1);
                    store.sweep();
                } else if (auto next = dequeue()) {
                    store.record(*next);
                }
            } catch (...) {
                store.hold();
            }
        }
    }

    Store store;
    const bool manual;
    std::mutex queue_gate;
    std::condition_variable wake;
    std::mutex producer_gate;
    std::array<Observation, 256> observations{};
    std::size_t head = 0, count = 0;
    std::shared_ptr<Snapshot> queued_snapshot;
    bool snapshot_running = false, stopping = false;
    std::function<void()> control_turn;
    std::thread worker;
};

LogOwner::LogOwner(QString directory, unsigned days)
    : impl_(std::make_unique<Impl>(std::move(directory), days, nullptr)) {}
LogOwner::LogOwner(QString directory, unsigned days, const testing::LogControl &control)
    : impl_(std::make_unique<Impl>(std::move(directory), days, &control)) {}
LogOwner::~LogOwner() = default;
void LogOwner::host_entered_serving() noexcept { impl_->offer({Code::serving, {}}); }
void LogOwner::clock_discontinuity() { impl_->store.discontinuity(); }
void LogOwner::connector_report_unavailable(ConnectorProvider provider) noexcept {
    if (name(provider))
        impl_->offer({Code::report_unavailable, provider});
}
bool LogOwner::snapshot(const std::shared_ptr<Snapshot> &snapshot) {
    return impl_->snapshot(snapshot);
}
void LogOwner::publish(View view, const std::function<void(View)> &transfer) {
    impl_->store.publish(std::move(view), transfer);
}

std::unique_ptr<LogOwner> testing::LogControl::open(QString directory, unsigned days) const {
    return std::unique_ptr<LogOwner>(new LogOwner(std::move(directory), days, *this));
}
void testing::LogControl::sweep(LogOwner &owner) {
    owner.impl_->test_turn([](Store &store) { store.sweep(); });
}
void testing::LogControl::drain(LogOwner &owner) {
    owner.impl_->test_turn([&](Store &) { owner.impl_->drain(); });
}
void testing::LogControl::barrier(LogOwner &owner) {
    owner.impl_->test_turn([](Store &) {});
}
std::vector<Notice> testing::LogControl::notices(LogOwner &owner) {
    return owner.impl_->test_turn([](Store &store) { return store.notices(); });
}
MutationOutcome testing::LogControl::run_pending(LogOwner &owner) {
    return owner.impl_->test_turn([](Store &store) { return store.run_pending(); });
}
void testing::LogControl::settle(LogOwner &owner, MutationOutcome outcome) {
    owner.impl_->test_turn([&](Store &store) { store.settle_pending(outcome); });
}
} // namespace subathon::server::diagnostics
