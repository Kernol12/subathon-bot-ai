// SPDX-License-Identifier: GPL-3.0-or-later
// Source-only ordinary fixture tests. Synthetic trees are NOT SDK startup evidence.
#include "native_fixture.hpp"
#include <cstdio>
namespace account_capture_test {
namespace {
class ScratchTree : public testing::Test {
    Deadline watchdog;

  protected:
    OwnedScratch tree;
    void SetUp() override { tree.acquire(); }
    static DirectoryFd directory(int parent, const char *name) {
        return DirectoryFd(::openat(parent, name, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    }
    static void mkdir(int parent, const char *name) {
        if (::mkdirat(parent, name, 0700))
            throw std::runtime_error("FIXTURE: synthetic directory");
    }
    static void file(int parent, const char *name) {
        int fd = ::openat(parent, name, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (fd < 0)
            throw std::runtime_error("FIXTURE: synthetic file");
        ::close(fd); // empty synthetic file; no raw input/credential payload
    }
    static bool type(int parent, const char *name, mode_t expected) {
        struct stat info{};
        return ::fstatat(parent, name, &info, AT_SYMLINK_NOFOLLOW) == 0 &&
               (info.st_mode & S_IFMT) == expected;
    }
    DirectoryFd tmp() {
        tree.initial_directories();
        auto upload = directory(tree.fd(), "upload");
        mkdir(upload.get(), "tmp");
        return directory(upload.get(), "tmp");
    }
    void removed_leaf_stable_parent() {
        struct stat info{};
        EXPECT_EQ(::lstat(tree.path(), &info), -1);
        EXPECT_EQ(errno, ENOENT);
        EXPECT_EQ(::lstat(std::filesystem::path(runtime_config::scratch_parent).c_str(), &info), 0);
        EXPECT_TRUE(S_ISDIR(info.st_mode));
    }
};
TEST_F(ScratchTree, EmptyAcquiredLeafBeforeStartupIsRemoved) {
    ASSERT_TRUE(tree.remove(false));
    removed_leaf_stable_parent();
    EXPECT_TRUE(tree.remove(false)); // repeated cleanup performs no new deletion
}
TEST_F(ScratchTree, PartialInitialDirectoryCreationIsAnAllowedSubset) {
    mkdir(tree.fd(), "empty");
    ASSERT_TRUE(tree.remove(false));
    removed_leaf_stable_parent();
}
TEST_F(ScratchTree, PartialSdkStartupBucketsAreAnAllowedSubset) {
    auto temporary = tmp();
    for (auto name : {"00", "0A", "F0", "FF"})
        mkdir(temporary.get(), name);
    ASSERT_TRUE(tree.remove(false));
    removed_leaf_stable_parent();
}
TEST_F(ScratchTree, CompleteSelectedSdkUppercase256BucketsAreRemoved) {
    auto temporary = tmp();
    // Independent test DATA from selected SDK %02x + uppercase and frozen Python
    // allowed_dirs. Does not call the cleanup helper's bucket-name function.
    for (unsigned i = 0; i < 256; ++i) {
        std::array<char, 3> name{};
        ASSERT_EQ(std::snprintf(name.data(), name.size(), "%02X", i), 2);
        mkdir(temporary.get(), name.data());
    }
    ASSERT_TRUE(tree.remove(true));
    removed_leaf_stable_parent();
}
TEST_F(ScratchTree, IncompleteTreeCannotClaimHealthyServingAndIsPreserved) {
    auto temporary = tmp();
    mkdir(temporary.get(), "00");
    EXPECT_FALSE(tree.remove(true));
    EXPECT_TRUE(type(tree.fd(), "empty", S_IFDIR));
    EXPECT_TRUE(type(temporary.get(), "00", S_IFDIR));
    ASSERT_TRUE(tree.remove(false)); // known partial startup, not a serving result
    removed_leaf_stable_parent();
}
TEST_F(ScratchTree, UnexpectedRootFileIsPreservedWithoutDeletingKnownSiblings) {
    tree.initial_directories();
    file(tree.fd(), "unexpected");
    EXPECT_FALSE(tree.remove(false));
    EXPECT_TRUE(type(tree.fd(), "unexpected", S_IFREG));
    EXPECT_TRUE(type(tree.fd(), "empty", S_IFDIR));
    EXPECT_TRUE(type(tree.fd(), "upload", S_IFDIR));
    // Only the test author removes its explicitly created synthetic bad entry,
    // AFTER verifying that fixture cleanup preserved it. No broad cleanup.
    ASSERT_EQ(::unlinkat(tree.fd(), "unexpected", 0), 0);
    EXPECT_TRUE(tree.remove(false));
}
TEST_F(ScratchTree, UnexpectedBucketNameAndOneOverCountPreserveEntireTree) {
    auto temporary = tmp();
    for (unsigned i = 0; i < 256; ++i) {
        std::array<char, 3> name{};
        ASSERT_EQ(std::snprintf(name.data(), name.size(), "%02X", i), 2);
        mkdir(temporary.get(), name.data());
    }
    mkdir(temporary.get(), "GG"); // 257th entry, also outside the exact name allowlist
    EXPECT_FALSE(tree.remove(true));
    EXPECT_TRUE(type(temporary.get(), "GG", S_IFDIR));
    EXPECT_TRUE(type(temporary.get(), "00", S_IFDIR));
    EXPECT_TRUE(type(temporary.get(), "FF", S_IFDIR));
    ASSERT_EQ(::unlinkat(temporary.get(), "GG", AT_REMOVEDIR), 0);
    EXPECT_TRUE(tree.remove(true));
}
TEST_F(ScratchTree, LowercaseHexAliasIsNotAnAllowedSdkBucket) {
    auto temporary = tmp();
    mkdir(temporary.get(), "0A");
    mkdir(temporary.get(), "0a");
    EXPECT_FALSE(tree.remove(false));
    EXPECT_TRUE(type(temporary.get(), "0A", S_IFDIR));
    EXPECT_TRUE(type(temporary.get(), "0a", S_IFDIR));
    ASSERT_EQ(::unlinkat(temporary.get(), "0a", AT_REMOVEDIR), 0);
    EXPECT_TRUE(tree.remove(false));
}
TEST_F(ScratchTree, RegularFileAtAllowedDirectoryNameIsPreserved) {
    file(tree.fd(), "upload");
    EXPECT_FALSE(tree.remove(false));
    EXPECT_TRUE(type(tree.fd(), "upload", S_IFREG));
    ASSERT_EQ(::unlinkat(tree.fd(), "upload", 0), 0);
    EXPECT_TRUE(tree.remove(false));
}
TEST_F(ScratchTree, NonemptyKnownBucketIsPreservedAndNeverRecursivelyDeleted) {
    auto temporary = tmp();
    mkdir(temporary.get(), "00");
    mkdir(temporary.get(), "FF");
    auto bucket = directory(temporary.get(), "00");
    file(bucket.get(), "payload");
    EXPECT_FALSE(tree.remove(false));
    EXPECT_TRUE(type(bucket.get(), "payload", S_IFREG));
    EXPECT_TRUE(type(temporary.get(), "FF", S_IFDIR));
    ASSERT_EQ(::unlinkat(bucket.get(), "payload", 0), 0);
    EXPECT_TRUE(tree.remove(false));
}
TEST_F(ScratchTree, SymlinkAtKnownNameIsPreservedWithoutFollowingSiblingTarget) {
    OwnedScratch sibling;
    sibling.acquire();
    sibling.initial_directories();
    ASSERT_EQ(::symlinkat(sibling.path(), tree.fd(), "empty"), 0);
    EXPECT_FALSE(tree.remove(false));
    EXPECT_TRUE(type(tree.fd(), "empty", S_IFLNK));
    EXPECT_TRUE(type(sibling.fd(), "empty", S_IFDIR));
    EXPECT_TRUE(type(sibling.fd(), "upload", S_IFDIR));
    ASSERT_EQ(::unlinkat(tree.fd(), "empty", 0), 0);
    EXPECT_TRUE(tree.remove(false));
    EXPECT_TRUE(sibling.remove(false));
}
TEST_F(ScratchTree, SharedParentSurvivesOneLeafCleanupWhileAnotherLeafIsAlive) {
    OwnedScratch sibling;
    sibling.acquire();
    ASSERT_TRUE(tree.remove(false));
    removed_leaf_stable_parent();
    EXPECT_EQ(::mkdirat(sibling.fd(), "empty", 0700), 0);
    EXPECT_TRUE(sibling.remove(false));
}
TEST_F(ScratchTree, ChangedLeafPathIdentityIsRefusedBeforeAnyDeletion) {
    const std::filesystem::path original(tree.path());
    auto moved = original;
    moved += "-moved";
    ASSERT_EQ(::rename(original.c_str(), moved.c_str()), 0);
    ASSERT_EQ(::mkdir(original.c_str(), 0700), 0); // known synthetic replacement
    EXPECT_FALSE(tree.remove(false));
    struct stat info{};
    EXPECT_EQ(::lstat(original.c_str(), &info), 0);
    EXPECT_EQ(::lstat(moved.c_str(), &info), 0);
    ASSERT_EQ(::rmdir(original.c_str()), 0);
    ASSERT_EQ(::rename(moved.c_str(), original.c_str()), 0);
    EXPECT_TRUE(tree.remove(false));
}
} // namespace
} // namespace account_capture_test
