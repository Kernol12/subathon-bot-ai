// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/credential_storage.hpp>

#include "credential_repository_internal.hpp"

#include <QByteArray>
#include <algorithm>
#include <array>
#include <cerrno>
#include <fcntl.h>
#include <new>
#include <string>
#include <subathon/server/sqlite_writer.hpp>
#include <sys/stat.h>
#include <unistd.h>

namespace subathon::server {
namespace {
using namespace credential_detail;

constexpr std::size_t max_path_size = 4095;
constexpr std::array<unsigned char, 4> key_magic{'S', 'K', 'P', '1'};
constexpr std::size_t key_size = key_magic.size() + KeyIdView::extent + KeyView::extent;
static_assert(key_size == 52);

class FileDescriptor final {
  public:
    explicit FileDescriptor(int fd) noexcept : fd_(fd) {}
    FileDescriptor(const FileDescriptor &) = delete;
    FileDescriptor &operator=(const FileDescriptor &) = delete;
    ~FileDescriptor() { reset(-1); }

    void reset(int fd) noexcept {
        if (fd_ >= 0)
            ::close(fd_);
        fd_ = fd;
    }

    [[nodiscard]] int get() const noexcept { return fd_; }

  private:
    int fd_;
};

bool valid_absolute_path(std::string_view path) {
    if (!path.starts_with('/') || path.find('\0') != std::string_view::npos)
        return false;
    for (std::size_t start = 1;;) {
        const auto end = path.find('/', start);
        const auto component =
            path.substr(start, end == std::string_view::npos ? end : end - start);
        if (component.empty() || component == "." || component == "..")
            return false;
        if (end == std::string_view::npos)
            return true;
        start = end + 1;
    }
}

CredentialError filesystem_error(int error) noexcept {
    return error == ELOOP || error == ENOTDIR ? CredentialError::key_file_rejected
                                              : CredentialError::key_unavailable;
}

bool protected_directory(const struct stat &metadata, uid_t service_uid) noexcept {
    return S_ISDIR(metadata.st_mode) && (metadata.st_uid == 0 || metadata.st_uid == service_uid) &&
           (metadata.st_mode & (S_IWGRP | S_IWOTH)) == 0;
}

bool protected_key(const struct stat &metadata, uid_t service_uid) noexcept {
    const auto mode = metadata.st_mode & 07777;
    return S_ISREG(metadata.st_mode) && metadata.st_uid == service_uid &&
           (mode == 0400 || mode == 0600);
}

} // namespace

namespace credential_detail {
// Metadata precedes any payload projection. Both statements use the same
// serialized writer session and the full canonical BLOB key.
static bool sql_integer(const QVariant &value) {
    return value.metaType().id() == QMetaType::Int || value.metaType().id() == QMetaType::LongLong;
}

SelectedRecord select_record(const SqliteWorkSession &session, const QByteArray &scope) {
    const auto metadata =
        session.execute({"SELECT typeof(scope),typeof(record),length(scope),"
                         "CASE WHEN typeof(record)='blob' THEN length(record) END "
                         "FROM main.subathon_provider_credentials WHERE scope=? LIMIT 2",
                         {scope}});
    if (metadata.empty())
        return CredentialMissing{};
    if (metadata.size() != 1 || metadata.front().size() != 4)
        return CredentialError::invalid_record;
    const auto &fields = metadata.front();
    if (fields[0].metaType().id() != QMetaType::QString ||
        fields[1].metaType().id() != QMetaType::QString ||
        fields[0].toString() != QStringLiteral("blob") ||
        fields[1].toString() != QStringLiteral("blob") || !sql_integer(fields[2]) ||
        !sql_integer(fields[3]) || fields[2].toLongLong() != scope.size())
        return CredentialError::invalid_record;
    const auto size = fields[3].toLongLong();
    if (size > static_cast<qlonglong>(envelope_overhead + max_plaintext_size))
        return CredentialError::resource_limit;
    if (size <= static_cast<qlonglong>(envelope_overhead))
        return CredentialError::invalid_record;

    const auto payload = session.execute(
        {"SELECT scope,record FROM main.subathon_provider_credentials WHERE scope=? "
         "AND typeof(scope)='blob' AND length(scope)=? "
         "AND typeof(record)='blob' AND length(record)=? LIMIT 2",
         {scope, qlonglong(scope.size()), size}});
    if (payload.size() != 1 || payload.front().size() != 2)
        return CredentialError::invalid_record;
    const auto &row = payload.front();
    if (row[0].metaType().id() != QMetaType::QByteArray ||
        row[1].metaType().id() != QMetaType::QByteArray || row[0].toByteArray() != scope)
        return CredentialError::invalid_record;
    auto record = row[1].toByteArray();
    if (record.size() != size)
        return CredentialError::invalid_record;
    return record;
}

QByteArray blob(std::span<const unsigned char> bytes) {
    return QByteArray(reinterpret_cast<const char *>(bytes.data()), qsizetype(bytes.size()));
}

std::span<const unsigned char> byte_view(const QByteArray &blob) {
    return {reinterpret_cast<const unsigned char *>(blob.constData()), std::size_t(blob.size())};
}
// Guard replacement and deletion by the exact ciphertext authenticated by the caller.
static void require_one(const SqlRows &acknowledged) {
    if (acknowledged.size() != 1 || acknowledged.front().size() != 1 ||
        !sql_integer(acknowledged.front().front()) ||
        acknowledged.front().front().toLongLong() != 1)
        throw SqliteWorkFailure(WorkError::sql_failure);
}

void store_record(const SqliteWorkSession &session, const QByteArray &scope,
                  const QByteArray *prior, const QByteArray &record) {
    require_one(prior ? session.execute({"UPDATE main.subathon_provider_credentials SET record=? "
                                         "WHERE scope=? AND record=? RETURNING 1",
                                         {record, scope, *prior}})
                      : session.execute({"INSERT INTO main.subathon_provider_credentials(scope,record) "
                                         "VALUES(?,?) RETURNING 1",
                                         {scope, record}}));
}

void erase_record(const SqliteWorkSession &session, const QByteArray &scope,
                  const QByteArray &prior) {
    require_one(session.execute({"DELETE FROM main.subathon_provider_credentials "
                                 "WHERE scope=? AND record=? RETURNING 1",
                                 {scope, prior}}));
}
} // namespace credential_detail

struct CredentialKey::Storage {
    credential_detail::GuardedBytes file{key_size + 1};

    [[nodiscard]] credential_detail::KeyIdView id() const noexcept {
        return file.bytes().subspan<key_magic.size(), credential_detail::KeyIdView::extent>();
    }
    [[nodiscard]] credential_detail::KeyView key() const noexcept {
        return file.bytes()
            .subspan<key_magic.size() + credential_detail::KeyIdView::extent,
                     credential_detail::KeyView::extent>();
    }
};

bool RepositoryAccess::usable(const CredentialKey *key) noexcept {
    return key && key->storage_;
}

CredentialLoadResult RepositoryAccess::open(const CredentialKey &key,
                                            std::span<const unsigned char> scope,
                                            const QByteArray &record) {
    auto opened = open_record(scope, byte_view(record), key.storage_->id(), key.storage_->key());
    if (const auto *error = std::get_if<CredentialError>(&opened))
        return *error;
    return std::get<CredentialValue>(std::move(opened));
}

EncodedBytes RepositoryAccess::seal_guarded(const CredentialKey &key,
                                           std::span<const unsigned char> scope,
                                           const GuardedBytes &input) {
    return seal_guarded_record(scope, input, key.storage_->id(), key.storage_->key());
}

CredentialKey::CredentialKey(std::unique_ptr<Storage> storage) noexcept
    : storage_(std::move(storage)) {}
CredentialKey::~CredentialKey() = default;
CredentialKey::CredentialKey(CredentialKey &&) noexcept = default;
CredentialKey &CredentialKey::operator=(CredentialKey &&) noexcept = default;

CredentialProvisionResult provision(PrivateKeyPath path) {
    if (path.bytes.size() > max_path_size)
        return CredentialError::resource_limit;
    if (!valid_absolute_path(path.bytes))
        return CredentialError::key_file_rejected;
    const auto service_uid = ::geteuid();
    if (service_uid == 0)
        return CredentialError::key_file_rejected;

    constexpr int directory_flags = O_PATH | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;
    FileDescriptor directory(::open("/", directory_flags));
    if (directory.get() < 0)
        return filesystem_error(errno);
    auto remaining = path.bytes.substr(1);
    for (;;) {
        struct stat ancestor{};
        if (::fstat(directory.get(), &ancestor) != 0)
            return filesystem_error(errno);
        if (!protected_directory(ancestor, service_uid))
            return CredentialError::key_file_rejected;

        const auto slash = remaining.find('/');
        if (slash == std::string_view::npos)
            break;
        const std::string component(remaining.substr(0, slash));
        const int next = ::openat(directory.get(), component.c_str(), directory_flags);
        if (next < 0)
            return filesystem_error(errno);
        directory.reset(next);
        remaining.remove_prefix(slash + 1);
    }

    const std::string name(remaining);
    struct stat inspected{};
    if (::fstatat(directory.get(), name.c_str(), &inspected, AT_SYMLINK_NOFOLLOW) != 0)
        return filesystem_error(errno);
    if (!protected_key(inspected, service_uid))
        return CredentialError::key_file_rejected;

    const FileDescriptor file(
        ::openat(directory.get(), name.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW));
    if (file.get() < 0)
        return filesystem_error(errno);
    struct stat opened{};
    if (::fstat(file.get(), &opened) != 0)
        return filesystem_error(errno);
    if (opened.st_dev != inspected.st_dev || opened.st_ino != inspected.st_ino ||
        !protected_key(opened, service_uid))
        return CredentialError::key_file_rejected;
    if (opened.st_size != static_cast<off_t>(key_size))
        return CredentialError::invalid_key_file;
    if (!sodium_ready())
        return CredentialError::crypto_unavailable;

    auto storage =
        std::unique_ptr<CredentialKey::Storage>(new (std::nothrow) CredentialKey::Storage);
    if (!storage || storage->file.bytes().empty())
        return CredentialError::crypto_unavailable;
    auto buffer = storage->file.bytes();
    std::size_t count = 0;
    while (count < buffer.size()) {
        const auto received = ::read(file.get(), buffer.data() + count, buffer.size() - count);
        if (received < 0) {
            if (errno == EINTR)
                continue;
            return filesystem_error(errno);
        }
        if (received == 0)
            break;
        count += static_cast<std::size_t>(received);
    }
    if (count != key_size || !std::ranges::equal(buffer.first<key_magic.size()>(), key_magic))
        return CredentialError::invalid_key_file;
    return CredentialKey(std::move(storage));
}

CredentialRepository::CredentialRepository(const SqliteWorkSession &session,
                                           const CredentialKey &key) noexcept
    : session_(session), key_(key) {}

CredentialLoadResult CredentialRepository::load(const CredentialScope &scope) const {
    const auto encoded = encode_scope(scope);
    if (const auto *error = std::get_if<CredentialError>(&encoded))
        return *error;
    if (!key_.storage_)
        return CredentialError::key_unavailable;
    const auto &canonical = std::get<RecordBytes>(encoded);
    const auto selected = select_record(session_, blob(canonical));
    if (const auto *error = std::get_if<CredentialError>(&selected))
        return *error;
    if (std::holds_alternative<CredentialMissing>(selected))
        return CredentialMissing{};

    return RepositoryAccess::open(key_, canonical, std::get<QByteArray>(selected));
}

CredentialPutResult CredentialRepository::stage_put(const CredentialScope &scope,
                                                    std::span<const unsigned char> bytes) const {
    const auto encoded = encode_put_scope(scope, bytes.size());
    if (const auto *error = std::get_if<CredentialError>(&encoded))
        return *error;
    if (!key_.storage_)
        return CredentialError::key_unavailable;

    const auto &canonical = std::get<RecordBytes>(encoded);
    const auto scope_blob = blob(canonical);
    const auto selected = select_record(session_, scope_blob);
    if (const auto *error = std::get_if<CredentialError>(&selected))
        return *error;
    const auto *prior = std::get_if<QByteArray>(&selected);
    if (prior) {
        const auto opened =
            open_record(canonical, byte_view(*prior), key_.storage_->id(), key_.storage_->key());
        if (const auto *error = std::get_if<CredentialError>(&opened))
            return *error;
        // The authenticated prior plaintext is erased before seal_record takes its copy.
    }

    const auto sealed = seal_record(canonical, bytes, key_.storage_->id(), key_.storage_->key());
    if (const auto *error = std::get_if<CredentialError>(&sealed))
        return *error;
    const auto record = blob(std::get<RecordBytes>(sealed));

    store_record(session_, scope_blob, prior, record);
    return CredentialStaged{};
}

std::optional<CredentialValue> finish_credential_load(std::optional<CredentialLoadResult> slot,
                                                      const SqliteWorkResult &completion) {
    if (slot && completion.transaction == TransactionOutcome::committed &&
        completion.error == WorkError::none && completion.value) {
        auto *value = std::get_if<CredentialValue>(&*slot);
        if (value && !value->bytes().empty())
            return std::move(*value);
    }
    return std::nullopt;
}
} // namespace subathon::server
