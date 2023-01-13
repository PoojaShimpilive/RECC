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

#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>

#include <cstring>
#include <env.h>
#include <fstream>
#include <sstream>
#include <unistd.h>

namespace recc {

bool FileUtils::isRegularFileOrSymlink(const struct stat &s)
{
    return (S_ISREG(s.st_mode) || S_ISLNK(s.st_mode));
}

struct stat FileUtils::getStat(const std::string &path,
                               const bool followSymlinks)
{
    if (path.empty()) {
        const std::string error = "invalid args: path empty";
        BUILDBOX_LOG_ERROR(error);
        throw std::runtime_error(error);
    }

    struct stat statResult;
    const int rc = (followSymlinks ? stat(path.c_str(), &statResult)
                                   : lstat(path.c_str(), &statResult));
    if (rc < 0) {
        BUILDBOX_LOG_ERROR("Error calling "
                           << (followSymlinks ? "stat()" : "lstat()")
                           << " for path \"" << path << "\": "
                           << "rc = " << rc << ", errno = [" << errno << ":"
                           << strerror(errno) << "]");
        throw std::system_error(errno, std::system_category());
    }

    return statResult;
}

bool FileUtils::isExecutable(const struct stat &s)
{
    return s.st_mode & S_IXUSR;
}

bool FileUtils::isSymlink(const struct stat &s) { return S_ISLNK(s.st_mode); }

std::string FileUtils::getSymlinkContents(const std::string &path,
                                          const struct stat &statResult)
{
    if (path.empty()) {
        const std::string error = "invalid args: path is empty";
        BUILDBOX_LOG_ERROR(error);
        throw std::runtime_error(error);
    }

    if (!S_ISLNK(statResult.st_mode)) {
        std::ostringstream oss;
        oss << "file \"" << path << "\" is not a symlink";
        BUILDBOX_LOG_ERROR(oss.str());
        throw std::runtime_error(oss.str());
    }

    std::string contents(static_cast<size_t>(statResult.st_size), '\0');
    const ssize_t rc = readlink(path.c_str(), &contents[0], contents.size());
    if (rc < 0) {
        std::ostringstream oss;
        oss << "readlink failed for \"" << path << "\", rc = " << rc
            << ", errno = [" << errno << ":" << strerror(errno) << "]";
        BUILDBOX_LOG_ERROR(oss.str());
        throw std::runtime_error(oss.str());
    }

    return contents;
}

bool FileUtils::hasPathPrefix(const std::string &path,
                              const std::string &prefix)
{
    /* A path can never have the empty path as a prefix */
    if (prefix.empty()) {
        return false;
    }
    if (path == prefix) {
        return true;
    }

    /*
     * Make sure prefix ends in a slash.
     * This is so we don't return true if path = /foo and prefix = /foobar
     */
    std::string tmpPrefix(prefix);
    if (tmpPrefix.back() != '/') {
        tmpPrefix.push_back('/');
    }

    return path.substr(0, tmpPrefix.length()) == tmpPrefix;
}

bool FileUtils::hasPathPrefixes(const std::string &path,
                                const std::set<std::string> &pathPrefixes)
{
    for (const auto &prefix : pathPrefixes) {
        if (FileUtils::hasPathPrefix(path, prefix)) {
            return true;
        }
    }

    return false;
}

std::string FileUtils::getCurrentWorkingDirectory()
{
    unsigned int bufferSize = 1024;
    while (true) {
        std::unique_ptr<char[]> buffer(new char[bufferSize]);
        char *cwd = getcwd(buffer.get(), bufferSize);

        if (cwd != nullptr) {
            return std::string(cwd);
        }
        else if (errno == ERANGE) {
            bufferSize *= 2;
        }
        else {
            const std::string errorReason = strerror(errno);
            BUILDBOX_LOG_ERROR(
                "Warning: could not get current working directory: "
                << errorReason);
            return std::string();
        }
    }
}

int FileUtils::parentDirectoryLevels(const std::string &path)
{
    int currentLevel = 0;
    int lowestLevel = 0;
    const char *path_p = path.c_str();

    while (*path_p != 0) {
        const char *slash = strchr(path_p, '/');
        if (!slash) {
            break;
        }

        const auto segmentLength = slash - path_p;
        if (segmentLength == 0 || (segmentLength == 1 && path_p[0] == '.')) {
            // Empty or dot segments don't change the level.
        }
        else if (segmentLength == 2 && path_p[0] == '.' && path_p[1] == '.') {
            currentLevel--;
            lowestLevel = std::min(lowestLevel, currentLevel);
        }
        else {
            currentLevel++;
        }

        path_p = slash + 1;
    }
    if (strcmp(path_p, "..") == 0) {
        currentLevel--;
        lowestLevel = std::min(lowestLevel, currentLevel);
    }
    return -lowestLevel;
}

std::string FileUtils::lastNSegments(const std::string &path, const int n)
{
    if (n == 0) {
        return "";
    }

    const char *path_p = path.c_str();
    const auto pathLength = strlen(path_p);
    const char *substringStart = path_p + pathLength - 1;
    unsigned int substringLength = 1;
    int slashesSeen = 0;

    if (path_p[pathLength - 1] == '/') {
        substringLength = 0;
    }

    while (substringStart != path_p) {
        if (*(substringStart - 1) == '/') {
            slashesSeen++;
            if (slashesSeen == n) {
                return std::string(substringStart, substringLength);
            }
        }
        substringStart--;
        substringLength++;
    }

    // The path might only be one segment (no slashes)
    if (slashesSeen == 0 && n == 1) {
        return std::string(path_p, pathLength);
    }
    throw std::logic_error("Not enough segments in path");
}

bool FileUtils::isAbsolutePath(const std::string &path)
{
    return ((!path.empty()) && path[0] == '/');
}

std::string FileUtils::resolvePathFromPrefixMap(const std::string &path)
{
    if (RECC_PREFIX_REPLACEMENT.empty()) {
        return path;
    }

    // Iterate through dictionary, replacing path if it includes key, with
    // value.
    for (const auto &pair : RECC_PREFIX_REPLACEMENT) {
        // Check if prefix is found in the path, and that it is a prefix.
        if (FileUtils::hasPathPrefix(path, pair.first)) {
            // Append a trailing slash to the replacement, in cases of
            // replacing `/` Double slashes will get removed during
            // normalization.
            const std::string replaced_path =
                pair.second + '/' + path.substr(pair.first.length());
            const std::string newPath =
                buildboxcommon::FileUtils::normalizePath(
                    replaced_path.c_str());
            return newPath;
        }
    }
    return path;
}

std::vector<std::string> FileUtils::parseDirectories(const std::string &path)
{
    std::vector<std::string> result;
    char *token = std::strtok((char *)path.c_str(), "/");
    while (token != nullptr) {
        result.emplace_back(token);
        token = std::strtok(nullptr, "/");
    }

    return result;
}

std::string FileUtils::modifyPathForRemote(const std::string &path,
                                           const std::string &workingDirectory,
                                           bool normalizePath)
{
    std::string replacedPath = FileUtils::resolvePathFromPrefixMap(path);
    replacedPath =
        FileUtils::rewritePathToRelative(replacedPath, workingDirectory);
    if (normalizePath && !RECC_NO_PATH_REWRITE) {
        replacedPath =
            buildboxcommon::FileUtils::normalizePath(replacedPath.c_str());
    }
    return replacedPath;
}

std::string
FileUtils::rewritePathToRelative(const std::string &path,
                                 const std::string &workingDirectory)
{
    std::string modifiedPath = path;
    if (!RECC_NO_PATH_REWRITE &&
        FileUtils::hasPathPrefix(modifiedPath, RECC_PROJECT_ROOT)) {
        modifiedPath = buildboxcommon::FileUtils::makePathRelative(
            modifiedPath, workingDirectory);
    }
    return modifiedPath;
}

} // namespace recc
