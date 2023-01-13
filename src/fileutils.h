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

#ifndef INCLUDED_FILEUTILS
#define INCLUDED_FILEUTILS

#include <functional>
#include <iostream>
#include <iterator>
#include <reccdefaults.h>
#include <set>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>
namespace recc {

struct FileUtils {
    /**
     * Return the 'struct stat' given an absolute file path
     */
    static struct stat getStat(const std::string &path,
                               const bool followSymlinks);

    static bool isRegularFileOrSymlink(const struct stat &s);
    static bool isExecutable(const struct stat &s);
    static bool isSymlink(const struct stat &s);

    /**
     * Given the path to a symlink, return a std::string with its contents.
     *
     * The path must be a path to a symlink that exists on disk. It can be
     * absolute or relative to the current directory.
     */
    static std::string getSymlinkContents(const std::string &path,
                                          const struct stat &statResult);

    /**
     * Returns true if "path" has "prefix" as a prefix.
     *
     * Before performing the check, a trailing slash is appended to prefix if
     * it doesn't have one since it is assumed that prefix is a directory.
     *
     * Note that this isn't just "is_subdirectory_of": /a/ is a prefix of
     * /a/../b/.
     */
    static bool hasPathPrefix(const std::string &path,
                              const std::string &prefix);

    static bool hasPathPrefixes(const std::string &path,
                                const std::set<std::string> &prefixes);

    /**
     * Return the current working directory.
     *
     * If the current working directory cannot be determined (if the user does
     * not have permission to read the current directory, for example), return
     * the empty std::string and log a warning.
     */
    static std::string getCurrentWorkingDirectory();

    /**
     * Return the number of levels of parent directory needed to follow the
     * given path.
     *
     * For example, "a/b/c.txt" has zero parent directory levels,
     * "a/../../b.txt" has one, and "../.." has two.
     */
    static int parentDirectoryLevels(const std::string &path);

    /**
     * Return a std::string containing the last N segments of the given path,
     * without a trailing slash.
     *
     * If the given path doesn't have that many segments, throws an exception.
     */
    static std::string lastNSegments(const std::string &path, const int n);

    /**
     * Determine if the path is absolute.
     * Replace with std::filesystem if compiling with C++17.
     */
    static bool isAbsolutePath(const std::string &path);

    /**
     * Check and replace input str if a path matches one in
     * PREFIX_REPLACEMENT_MAP.
     */
    static std::string resolvePathFromPrefixMap(const std::string &path);

    static std::vector<std::string> parseDirectories(const std::string &path);

    /**
     * This helper modifies the given path to make it suitable for running
     * remotely. The path is replaced if matching a option in RECC_PREFIX_MAP,
     * and then made relative to the working directory
     *
     * Calling this function also normalizes the path by default, but this can
     * be disabled by by setting the corresponding param to false.
     */
    static std::string modifyPathForRemote(const std::string &path,
                                           const std::string &workingDirectory,
                                           bool normalizePath = true);

    /**
     * This method will call makePathRelative on the path if the path has
     * the workingDirectory in it's prefix and RECC_NO_PATH_REWRITE is not
     * enabled.
     */
    static std::string
    rewritePathToRelative(const std::string &path,
                          const std::string &workingDirectory);
};

} // namespace recc

#endif
