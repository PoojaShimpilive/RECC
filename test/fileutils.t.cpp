// Copyright 2018 Bloomberg Finance L.P
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fileutils.h>

#include <env.h>
#include <subprocess.h>

#include <buildboxcommon_temporarydirectory.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <exception>
#include <functional>
#include <gtest/gtest.h>
#include <mutex>
#include <sys/stat.h>
#include <vector>
using namespace recc;

TEST(FileUtilsTest, FileContents)
{
    buildboxcommon::TemporaryDirectory tempDir;
    const std::string fileName = tempDir.name() + std::string("/testfile.txt");

    EXPECT_THROW(buildboxcommon::FileUtils::getFileContents(fileName.c_str()),
                 std::exception);
    buildboxcommon::FileUtils::writeFileAtomically(fileName, "File contents");
    EXPECT_EQ(buildboxcommon::FileUtils::getFileContents(fileName.c_str()),
              "File contents");

    buildboxcommon::FileUtils::writeFileAtomically(fileName,
                                                   "Overwrite, don't append");
    EXPECT_EQ(buildboxcommon::FileUtils::getFileContents(fileName.c_str()),
              "Overwrite, don't append");
}

TEST(HasPathPrefixTest, AbsolutePaths)
{
    EXPECT_TRUE(FileUtils::hasPathPrefix("/a/b/c/", "/a/b"));
    EXPECT_TRUE(FileUtils::hasPathPrefix("/a/b/c/", "/a/b/"));
    EXPECT_TRUE(FileUtils::hasPathPrefix("/a/b/c", "/a/b"));
    EXPECT_TRUE(FileUtils::hasPathPrefix("/a/b/c", "/a/b/"));

    EXPECT_FALSE(FileUtils::hasPathPrefix("/a/c/d", "/a/b/"));

    EXPECT_FALSE(FileUtils::hasPathPrefix("/a/boo", "/a/b/"));
    EXPECT_FALSE(FileUtils::hasPathPrefix("/a/boo", "/a/b"));
    EXPECT_FALSE(FileUtils::hasPathPrefix("/a/boo", "/a/b/a/boo"));
    EXPECT_FALSE(FileUtils::hasPathPrefix("/a/boo", "/a/b/a/boo/"));

    EXPECT_TRUE(FileUtils::hasPathPrefix("/a/../b/", "/a"));
    EXPECT_TRUE(FileUtils::hasPathPrefix("/a/../b/", "/a/"));
    EXPECT_TRUE(FileUtils::hasPathPrefix("/a/../b", "/a"));
    EXPECT_TRUE(FileUtils::hasPathPrefix("/a/../b", "/a/"));
}

TEST(HasPathPrefixTest, RelativePaths)
{
    EXPECT_TRUE(FileUtils::hasPathPrefix("a/b/c/", "a/b"));
    EXPECT_TRUE(FileUtils::hasPathPrefix("a/b/c/", "a/b/"));
    EXPECT_TRUE(FileUtils::hasPathPrefix("a/b/c", "a/b"));
    EXPECT_TRUE(FileUtils::hasPathPrefix("a/b/c", "a/b/"));
    EXPECT_TRUE(FileUtils::hasPathPrefix("/a/b/c", "/a/b/c"));

    EXPECT_FALSE(FileUtils::hasPathPrefix("a/c/d", "a/b/"));

    EXPECT_FALSE(FileUtils::hasPathPrefix("a/boo", "a/b/"));
    EXPECT_FALSE(FileUtils::hasPathPrefix("a/boo", "a/b"));
    EXPECT_FALSE(FileUtils::hasPathPrefix("a/boo", "a/b/a/boo"));
    EXPECT_FALSE(FileUtils::hasPathPrefix("a/boo", "a/b/a/boo/"));

    EXPECT_TRUE(FileUtils::hasPathPrefix("a/../b/", "a"));
    EXPECT_TRUE(FileUtils::hasPathPrefix("a/../b/", "a/"));
    EXPECT_TRUE(FileUtils::hasPathPrefix("a/../b", "a"));
    EXPECT_TRUE(FileUtils::hasPathPrefix("a/../b", "a/"));

    EXPECT_FALSE(FileUtils::hasPathPrefix("/a/b/c/", "a/b/"));
    EXPECT_FALSE(FileUtils::hasPathPrefix("/a/b/c/", "a/b"));
    EXPECT_FALSE(FileUtils::hasPathPrefix("/a/b/c", "a/b/"));
    EXPECT_FALSE(FileUtils::hasPathPrefix("/a/b/c", "a/b"));
}

TEST(HasPathPrefixesTest, PathTests)
{
    const std::set<std::string> prefixes = {"/usr/include",
                                            "/opt/rh/devtoolset-7"};

    EXPECT_TRUE(FileUtils::hasPathPrefixes("/usr/include/stat.h", prefixes));
    EXPECT_FALSE(FileUtils::hasPathPrefixes("usr/include/stat.h", prefixes));
    EXPECT_TRUE(
        FileUtils::hasPathPrefixes("/opt/rh/devtoolset-7/foo.h", prefixes));
    EXPECT_FALSE(FileUtils::hasPathPrefixes("/opt/rh/foo.h", prefixes));

    EXPECT_TRUE(FileUtils::hasPathPrefixes("/some/dir/foo.h", {"/"}));
    EXPECT_FALSE(FileUtils::hasPathPrefixes("/", {"/some/other/dir"}));

    EXPECT_TRUE(FileUtils::hasPathPrefixes("/some/dir,withcomma/foo.h",
                                           {"/some/dir,withcomma/"}));
}

TEST(FileUtilsTest, GetCurrentWorkingDirectory)
{
    const std::vector<std::string> command = {"pwd"};
    const auto commandResult = Subprocess::execute(command, true);
    if (commandResult.d_exitCode == 0) {
        EXPECT_EQ(commandResult.d_stdOut,
                  FileUtils::getCurrentWorkingDirectory() + "\n");
    }
}

TEST(FileUtilsTest, ParentDirectoryLevels)
{
    EXPECT_EQ(FileUtils::parentDirectoryLevels(""), 0);
    EXPECT_EQ(FileUtils::parentDirectoryLevels("/"), 0);
    EXPECT_EQ(FileUtils::parentDirectoryLevels("."), 0);
    EXPECT_EQ(FileUtils::parentDirectoryLevels("./"), 0);

    EXPECT_EQ(FileUtils::parentDirectoryLevels(".."), 1);
    EXPECT_EQ(FileUtils::parentDirectoryLevels("../"), 1);
    EXPECT_EQ(FileUtils::parentDirectoryLevels("../.."), 2);
    EXPECT_EQ(FileUtils::parentDirectoryLevels("../../"), 2);

    EXPECT_EQ(FileUtils::parentDirectoryLevels("a/b/c.txt"), 0);
    EXPECT_EQ(FileUtils::parentDirectoryLevels("a/../../b.txt"), 1);
    EXPECT_EQ(
        FileUtils::parentDirectoryLevels("a/../../b/c/d/../../../../test.txt"),
        2);
}

TEST(FileUtilsTest, LastNSegments)
{
    EXPECT_EQ(FileUtils::lastNSegments("abc", 0), "");
    EXPECT_EQ(FileUtils::lastNSegments("abc", 1), "abc");
    EXPECT_ANY_THROW(FileUtils::lastNSegments("abc", 2));
    EXPECT_ANY_THROW(FileUtils::lastNSegments("abc", 3));

    EXPECT_EQ(FileUtils::lastNSegments("/abc", 0), "");
    EXPECT_EQ(FileUtils::lastNSegments("/abc", 1), "abc");
    EXPECT_ANY_THROW(FileUtils::lastNSegments("/abc", 2));
    EXPECT_ANY_THROW(FileUtils::lastNSegments("/abc", 3));

    EXPECT_EQ(FileUtils::lastNSegments("/a/bc", 0), "");
    EXPECT_EQ(FileUtils::lastNSegments("/a/bc", 1), "bc");
    EXPECT_EQ(FileUtils::lastNSegments("/a/bc", 2), "a/bc");
    EXPECT_ANY_THROW(FileUtils::lastNSegments("/a/bc", 3));

    EXPECT_EQ(FileUtils::lastNSegments("/a/bb/c/dd/e", 0), "");
    EXPECT_EQ(FileUtils::lastNSegments("/a/bb/c/dd/e", 1), "e");
    EXPECT_EQ(FileUtils::lastNSegments("/a/bb/c/dd/e", 2), "dd/e");
    EXPECT_EQ(FileUtils::lastNSegments("/a/bb/c/dd/e", 3), "c/dd/e");
    EXPECT_EQ(FileUtils::lastNSegments("/a/bb/c/dd/e", 4), "bb/c/dd/e");
    EXPECT_EQ(FileUtils::lastNSegments("/a/bb/c/dd/e", 5), "a/bb/c/dd/e");
    EXPECT_ANY_THROW(FileUtils::lastNSegments("/a/bb/c/dd/e", 6));

    EXPECT_EQ(FileUtils::lastNSegments("/a/bb/c/dd/e/", 0), "");
    EXPECT_EQ(FileUtils::lastNSegments("/a/bb/c/dd/e/", 1), "e");
    EXPECT_EQ(FileUtils::lastNSegments("/a/bb/c/dd/e/", 2), "dd/e");
    EXPECT_EQ(FileUtils::lastNSegments("/a/bb/c/dd/e/", 3), "c/dd/e");
    EXPECT_EQ(FileUtils::lastNSegments("/a/bb/c/dd/e/", 4), "bb/c/dd/e");
    EXPECT_EQ(FileUtils::lastNSegments("/a/bb/c/dd/e/", 5), "a/bb/c/dd/e");
    EXPECT_ANY_THROW(FileUtils::lastNSegments("/a/bb/c/dd/e/", 6));
}

TEST(FileUtilsTest, AbsolutePaths)
{
    EXPECT_EQ(false, FileUtils::isAbsolutePath("../hello"));
    EXPECT_EQ(true, FileUtils::isAbsolutePath("/../hello/"));
    EXPECT_EQ(false, FileUtils::isAbsolutePath(""));
    EXPECT_EQ(true, FileUtils::isAbsolutePath("/hello/world"));
}

// Test paths are rewritten
TEST(PathRewriteTest, SimpleRewriting)
{
    RECC_PREFIX_REPLACEMENT = {{"/hello/hi", "/hello"},
                               {"/usr/bin/system/bin/hello", "/usr/system"}};
    std::string test_path = "/hello/hi/file.txt";
    ASSERT_EQ("/hello/file.txt",
              FileUtils::resolvePathFromPrefixMap(test_path));

    test_path = "/usr/bin/system/bin/hello/file.txt";
    ASSERT_EQ("/usr/system/file.txt",
              FileUtils::resolvePathFromPrefixMap(test_path));

    test_path = "/hello/bin/not_replaced.txt";
    ASSERT_EQ(test_path, FileUtils::resolvePathFromPrefixMap(test_path));
}

// Test more complicated paths
TEST(PathRewriteTest, ComplicatedPathRewriting)
{
    RECC_PREFIX_REPLACEMENT = {{"/hello/hi", "/hello"},
                               {"/usr/bin/system/bin/hello", "/usr/system"},
                               {"/bin", "/"}};

    auto test_path = "/usr/bin/system/bin/hello/world/";
    ASSERT_EQ("/usr/system/world",
              FileUtils::resolvePathFromPrefixMap(test_path));

    // Don't rewrite non-absolute path
    test_path = "../hello/hi/hi.txt";
    ASSERT_EQ(test_path, FileUtils::resolvePathFromPrefixMap(test_path));

    test_path = "/bin/hello/file.txt";
    ASSERT_EQ("/hello/file.txt",
              FileUtils::resolvePathFromPrefixMap(test_path));
}

TEST(PathReplacement, modifyRemotePathUnmodified)
{
    // If a given path doesn't match any PREFIX_REPLACEMENT
    // rules and can't be made relative, it's returned unmodified
    RECC_PROJECT_ROOT = "/home/nobody/";
    RECC_PREFIX_REPLACEMENT = {{"/home", "/hi"}};

    const auto workingDir = "/home";

    const std::string replacedPath =
        FileUtils::modifyPathForRemote("/other/dir/nobody/test", workingDir);

    EXPECT_EQ("/other/dir/nobody/test", replacedPath);
}

TEST(PathReplacement, modifyRemotePathPrefixMatch)
{
    // Match a PREFIX_REPLACEMENT rule, but the replaced path
    // isn't eligable to be made relative, so it's returned absolute
    RECC_PROJECT_ROOT = "/home/nobody/";
    RECC_PREFIX_REPLACEMENT = {{"/home", "/hi"}};

    const auto workingDir = "/home";

    const std::string replacedPath =
        FileUtils::modifyPathForRemote("/home/nobody/test", workingDir);

    EXPECT_EQ("/hi/nobody/test", replacedPath);
}

TEST(PathReplacement, modifyRemotePathMadeRelative)
{
    // Path doesn't match any PREFIX_REPLACEMENT rules,
    // but can be made relative to RECC_PROJECT_ROOT
    RECC_PROJECT_ROOT = "/other";
    RECC_PREFIX_REPLACEMENT = {{"/home", "/hi"}};

    const auto workingDir = "/other";

    const std::string replacedPath =
        FileUtils::modifyPathForRemote("/other/nobody/test", workingDir);

    EXPECT_EQ("nobody/test", replacedPath);
}

TEST(PathReplacement, modifyRemotePathPrefixAndRelativeMatch)
{
    // Path matches a PREFIX_REPLACEMENT rule, and the replaced
    // path can be made relative to RECC_PROJECT_ROOT
    RECC_PROJECT_ROOT = "/home/";
    RECC_PREFIX_REPLACEMENT = {{"/home/nobody/", "/home"}};

    const auto workingDir = "/home";

    const std::string replacedPath =
        FileUtils::modifyPathForRemote("/home/nobody/test", workingDir);

    EXPECT_EQ("test", replacedPath);
}

TEST(PathReplacement, normalizeRemotePath)
{
    RECC_PROJECT_ROOT = "/home/nobody/";
    RECC_PREFIX_REPLACEMENT = {{"/home", "/hi"}};

    const auto workingDir = "/home";

    // If a given path doesn't match any PREFIX_REPLACEMENT
    // rules and can't be made relative, it's returned unmodified
    // if RECC_NO_PATH_REWRITE is set
    RECC_NO_PATH_REWRITE = true;
    const std::string replacedPathNoRewrite =
        FileUtils::modifyPathForRemote("//other/dir/nobody/test", workingDir);
    EXPECT_EQ("//other/dir/nobody/test", replacedPathNoRewrite);

    // It's normalized but otherwise unmodified
    // if RECC_NO_PATH_REWRITE is not set
    RECC_NO_PATH_REWRITE = false;
    const std::string replacedPath =
        FileUtils::modifyPathForRemote("//other/dir/nobody/test", workingDir);
    EXPECT_EQ("/other/dir/nobody/test", replacedPath);
}
