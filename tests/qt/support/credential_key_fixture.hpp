// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../fixtures/credential_vectors.hpp"
#include "sqlite_test_support.hpp"

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

namespace credential_test_support {
using subathon::server::CredentialError;
using subathon::server::CredentialProvisionResult;
using subathon::server::provision;
namespace v = credential_vectors;

class FixtureFailure final : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

inline void require(bool condition, const QString &message,
                    std::source_location location = std::source_location::current()) {
    try {
        qt_test_support::require(condition, message, location);
    } catch (const std::runtime_error &failure) {
        throw FixtureFailure(failure.what());
    }
}

template <class Body> void checked(Body body) {
    try {
        body();
    } catch (const FixtureFailure &failure) {
        QFAIL(failure.what());
    } catch (...) {
        QFAIL("Unexpected exception in credential test");
    }
}

inline QByteArray blob(std::span<const unsigned char> source) {
    return QByteArray(reinterpret_cast<const char *>(source.data()),
                      static_cast<qsizetype>(source.size()));
}

inline QString protected_home() {
    const auto uid = ::geteuid();
    require(uid != 0, "Credential filesystem tests require a non-root service UID");
    const auto *entry = ::getpwuid(uid);
    require(entry && entry->pw_dir && entry->pw_dir[0] == '/', "Service UID has an absolute home");
    const auto home = QFile::decodeName(entry->pw_dir);
    auto path = home;
    for (;;) {
        struct stat info{};
        require(::lstat(QFile::encodeName(path).constData(), &info) == 0 && S_ISDIR(info.st_mode) &&
                    (info.st_uid == 0 || info.st_uid == uid) &&
                    (info.st_mode & (S_IWGRP | S_IWOTH)) == 0,
                "Fixture ancestors must be protected and free of symlinks");
        if (path == "/")
            return home;
        path = QFileInfo(path).absolutePath();
    }
}

class KeyFixture final {
  public:
    KeyFixture() : dir(protected_home() + "/.session006-XXXXXX") {
        require(dir.isValid(), "Can create a private directory beneath the service home");
    }

    QByteArray file(const QString &name = "synthetic-key-path-marker") const {
        return QFile::encodeName(dir.filePath(name));
    }

    static void write(const QByteArray &path, const QByteArray &contents = blob(v::key_file),
                      mode_t mode = 0600) {
        const int fd = ::open(path.constData(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        require(fd >= 0, "Create disposable synthetic key file");
        const auto written =
            ::write(fd, contents.constData(), static_cast<size_t>(contents.size()));
        const auto chmod_status = ::fchmod(fd, mode);
        const auto closed = ::close(fd);
        require(written == contents.size() && chmod_status == 0 && closed == 0,
                "Write fixture bytes and establish exact native permissions");
    }

    QTemporaryDir dir;
};

inline CredentialProvisionResult provision_path(const QByteArray &path) {
    return provision({std::string_view(path.constData(), static_cast<std::size_t>(path.size()))});
}

template <class Result>
void expect_error(const Result &result, CredentialError expected,
                  std::source_location location = std::source_location::current()) {
    const auto *error = std::get_if<CredentialError>(&result);
    require(error != nullptr, "Expected a typed refusal with no returned value", location);
    require(*error == expected,
            QString("Credential category %1, expected %2")
                .arg(static_cast<int>(*error))
                .arg(static_cast<int>(expected)),
            location);
}

} // namespace credential_test_support
