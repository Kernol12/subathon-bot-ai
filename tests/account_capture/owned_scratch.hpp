// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Ordinary fixture resource ownership, not an SDK implementation or a launcher.
#include "runtime_config.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
namespace account_capture_test {
class DirectoryFd {
    int fd_ = -1;

  public:
    explicit DirectoryFd(int fd = -1) noexcept : fd_(fd) {}
    DirectoryFd(const DirectoryFd &) = delete;
    DirectoryFd &operator=(const DirectoryFd &) = delete;
    DirectoryFd(DirectoryFd &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    DirectoryFd &operator=(DirectoryFd &&other) noexcept {
        if (this != &other) {
            if (fd_ >= 0)
                ::close(fd_);
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }
    ~DirectoryFd() {
        if (fd_ >= 0)
            ::close(fd_);
    }
    int get() const noexcept { return fd_; }
};
class OwnedScratch {
    DirectoryFd parent_, leaf_;
    std::array<char, 534> path_{};
    std::array<char, 15> name_{}; // capture- + six mkdtemp characters, NUL
    bool acquired_ = false;
    struct Tree {
        bool empty = false, upload = false, tmp = false;
        std::array<bool, 256> buckets{};
    };
    static_assert(sizeof(Tree) <= 264);
    enum class Level { root, empty, upload, tmp, bucket };
    static int child(int parent, const char *name) noexcept {
        return ::openat(parent, name, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    }
    static bool owned(int fd) noexcept {
        struct stat info{};
        return fd >= 0 && ::fstat(fd, &info) == 0 && S_ISDIR(info.st_mode) &&
               info.st_uid == ::geteuid();
    }
    static int hex(char c) noexcept {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return -1;
    }
    static std::array<char, 3> bucket_name(unsigned i) noexcept {
        constexpr char digits[] = "0123456789ABCDEF";
        return {digits[i / 16], digits[i % 16], '\0'};
    }
    // Exact bounded immediate listing only. No recursion and no deletion here.
    static bool inspect(int fd, Level level, Tree &tree) noexcept {
        if (!owned(fd))
            return false;
        int copy = child(fd, ".");
        if (copy < 0)
            return false;
        DIR *dir = ::fdopendir(copy);
        if (!dir) {
            ::close(copy);
            return false;
        }
        bool valid = true;
        unsigned entries = 0, turns = 0;
        const unsigned cap = level == Level::root     ? 2
                             : level == Level::upload ? 1
                             : level == Level::tmp    ? 256
                                                      : 0;
        for (;;) {
            if (++turns > cap + 3) {
                valid = false;
                break;
            } // at most cap + dot/dotdot + EOF
            errno = 0;
            auto *entry = ::readdir(dir);
            if (!entry) {
                if (errno)
                    valid = false;
                break;
            }
            auto size = ::strnlen(entry->d_name, 256);
            if (size == 0 || size == 256) {
                valid = false;
                break;
            }
            const std::string_view name(entry->d_name, size);
            if (name == "." || name == "..")
                continue;
            if (++entries > cap) {
                valid = false;
                break;
            }
            struct stat info{};
            if (::fstatat(fd, entry->d_name, &info, AT_SYMLINK_NOFOLLOW) ||
                !S_ISDIR(info.st_mode) || info.st_uid != ::geteuid()) {
                valid = false;
                break;
            }
            bool *seen = nullptr;
            if (level == Level::root && name == "empty")
                seen = &tree.empty;
            else if (level == Level::root && name == "upload")
                seen = &tree.upload;
            else if (level == Level::upload && name == "tmp")
                seen = &tree.tmp;
            else if (level == Level::tmp && size == 2 && hex(name[0]) >= 0 && hex(name[1]) >= 0)
                seen = &tree.buckets[hex(name[0]) * 16 + hex(name[1])];
            if (!seen || *seen) {
                valid = false;
                break;
            }
            *seen = true;
        }
        if (::closedir(dir))
            valid = false;
        return valid;
    }
    bool same_leaf() const noexcept {
        struct stat actual{}, pinned{};
        return owned(parent_.get()) && owned(leaf_.get()) && ::fstat(leaf_.get(), &pinned) == 0 &&
               ::fstatat(parent_.get(), name_.data(), &actual, AT_SYMLINK_NOFOLLOW) == 0 &&
               S_ISDIR(actual.st_mode) && actual.st_uid == ::geteuid() &&
               actual.st_dev == pinned.st_dev && actual.st_ino == pinned.st_ino;
    }

  public:
    OwnedScratch() = default;
    void acquire() {
        if (parent_.get() >= 0 || acquired_)
            throw std::runtime_error("FIXTURE: scratch already acquired");
        parent_ = DirectoryFd([&] {
            const auto path = std::filesystem::path(runtime_config::scratch_parent);
            if (!path.is_absolute() || path.lexically_normal() != path ||
                path.filename() != "account-capture-runtime")
                throw std::runtime_error("FIXTURE: configured scratch confinement");
            struct stat caller{};
            if (::lstat(path.parent_path().c_str(), &caller) || !S_ISDIR(caller.st_mode) ||
                caller.st_uid != ::geteuid())
                throw std::runtime_error("FIXTURE: caller build directory");
            // Shared parent has BUILD-directory lifetime: fixtures NEVER remove it.
            if (::mkdir(path.c_str(), 0700) && errno != EEXIST)
                throw std::runtime_error("FIXTURE: scratch parent");
            int fd = ::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
            if (!owned(fd)) {
                if (fd >= 0)
                    ::close(fd);
                throw std::runtime_error("FIXTURE: scratch parent ownership");
            }
            return fd;
        }());
        auto pattern =
            (std::filesystem::path(runtime_config::scratch_parent) / "capture-XXXXXX").string();
        if (pattern.size() >= path_.size())
            throw std::runtime_error("FIXTURE: scratch length");
        std::array<char, 534> candidate{};
        std::memcpy(candidate.data(), pattern.c_str(), pattern.size() + 1);
        if (!::mkdtemp(candidate.data()))
            throw std::runtime_error("FIXTURE: scratch creation");
        path_ = candidate;
        // mkdtemp changes exactly its last six characters; retain the fixed
        // acquired basename without any fallible allocation after acquisition.
        std::memcpy(name_.data(), candidate.data() + pattern.size() - 14, 14);
        acquired_ = true;
        // Pinned FD + relative checks prevent cleanup following a replaced leaf.
        leaf_ = DirectoryFd(child(parent_.get(), name_.data()));
        if (!same_leaf())
            throw std::runtime_error("FIXTURE: acquired leaf identity");
    }
    OwnedScratch(const OwnedScratch &) = delete;
    OwnedScratch &operator=(const OwnedScratch &) = delete;
    int fd() const noexcept { return leaf_.get(); }
    const char *path() const noexcept { return path_.data(); }
    void initial_directories() {
        for (const char *name : {"empty", "upload"})
            if (::mkdirat(leaf_.get(), name, 0700))
                throw std::runtime_error("FIXTURE: initial scratch directory");
    }
    // Caller must have stopped/joined SDK writers. Preflight WHOLE tree before any
    // deletion. Partial startup admits only an exact subset of these known dirs.
    bool remove(bool serving) noexcept {
        if (!acquired_)
            return true;
        if (!same_leaf())
            return false;
        Tree tree;
        if (!inspect(leaf_.get(), Level::root, tree))
            return false;
        if (tree.empty) {
            DirectoryFd empty(child(leaf_.get(), "empty"));
            if (!inspect(empty.get(), Level::empty, tree))
                return false;
        }
        DirectoryFd upload(tree.upload ? child(leaf_.get(), "upload") : -1);
        if (tree.upload && !inspect(upload.get(), Level::upload, tree))
            return false;
        DirectoryFd tmp(tree.tmp ? child(upload.get(), "tmp") : -1);
        if (tree.tmp && !inspect(tmp.get(), Level::tmp, tree))
            return false;
        for (unsigned i = 0; i < 256; ++i)
            if (tree.buckets[i]) {
                const auto name = bucket_name(i);
                DirectoryFd bucket(child(tmp.get(), name.data()));
                if (!inspect(bucket.get(), Level::bucket, tree))
                    return false;
            }
        if (serving &&
            (!tree.empty || !tree.upload || !tree.tmp ||
             !std::all_of(tree.buckets.begin(), tree.buckets.end(), [](bool b) { return b; })))
            return false;
        // Only preflighted fixed names. AT_REMOVEDIR cannot unlink files/symlinks;
        // newly nonempty directories fail and preserve unexpected contents.
        for (unsigned i = 0; i < 256; ++i)
            if (tree.buckets[i]) {
                const auto name = bucket_name(i);
                if (::unlinkat(tmp.get(), name.data(), AT_REMOVEDIR))
                    return false;
            }
        if (tree.tmp && ::unlinkat(upload.get(), "tmp", AT_REMOVEDIR))
            return false;
        if (tree.upload && ::unlinkat(leaf_.get(), "upload", AT_REMOVEDIR))
            return false;
        if (tree.empty && ::unlinkat(leaf_.get(), "empty", AT_REMOVEDIR))
            return false;
        if (!same_leaf() || ::unlinkat(parent_.get(), name_.data(), AT_REMOVEDIR))
            return false;
        acquired_ = false;
        return true;
    }
    // Destruction closes only descriptors. A failed/unrequested cleanup preserves
    // the acquired private leaf for root inspection, never broad recursive deletion.
};
static_assert(sizeof(OwnedScratch) <= 576);
} // namespace account_capture_test
