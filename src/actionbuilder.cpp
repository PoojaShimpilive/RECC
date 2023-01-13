// Copyright 2020 Bloomberg Finance L.P
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

#include <actionbuilder.h>

#include <digestgenerator.h>
#include <env.h>
#include <fileutils.h>
#include <reccdefaults.h>
#include <threadutils.h>

#include <buildboxcommon_logging.h>
#include <buildboxcommonmetrics_durationmetrictimer.h>
#include <buildboxcommonmetrics_metricteeguard.h>

#include <set>
#include <thread>

#define TIMER_NAME_COMPILER_DEPS "recc.compiler_deps"
#define TIMER_NAME_BUILD_MERKLE_TREE "recc.build_merkle_tree"

namespace recc {

namespace {

// Do path replacement and normalize
const std::string normalize_replace_root(const std::string &path)
{
    // If the path matches any in RECC_PATH_PREFIX, replace it if
    // necessary, and normalize path.
    const std::string replacedRoot = FileUtils::resolvePathFromPrefixMap(path);

    // Get the relativePath from the current PROJECT_ROOT.
    std::string relativePath =
        FileUtils::rewritePathToRelative(replacedRoot, RECC_PROJECT_ROOT);

    // Prepend the RECC_WORKING_DIR_PREFIX if relative and normalize path
    if (relativePath[0] != '/' && !RECC_WORKING_DIR_PREFIX.empty()) {
        relativePath.insert(0, RECC_WORKING_DIR_PREFIX + "/");
    }
    const std::string normalizedReplacedRoot =
        buildboxcommon::FileUtils::normalizePath(relativePath.c_str());

    return normalizedReplacedRoot;
}

} // unnamed namespace

std::mutex ContainerWriteMutex;
std::mutex LogWriteMutex;

proto::Command ActionBuilder::populateCommandProto(
    const std::vector<std::string> &command,
    const std::set<std::string> &outputFiles,
    const std::set<std::string> &outputDirectories,
    const std::map<std::string, std::string> &remoteEnvironment,
    const std::map<std::string, std::string> &platformProperties,
    const std::string &workingDirectory)
{
    proto::Command commandProto;

    for (const auto &arg : command) {
        commandProto.add_arguments(arg);
    }

    for (const auto &envIter : remoteEnvironment) {
        auto envVar = commandProto.add_environment_variables();
        envVar->set_name(envIter.first);
        envVar->set_value(envIter.second);
    }

    // REAPI v2.1 deprecated the `output_files` and `output_directories` fields
    // of the `Command` message, replacing them with `output_paths`.
    const bool outputPathsSupported =
        Env::configured_reapi_version_equal_to_or_newer_than("2.1");

    for (const auto &file : outputFiles) {
        if (outputPathsSupported) {
            commandProto.add_output_paths(file);
        }
        else {
            commandProto.add_output_files(file);
        }
    }

    for (const auto &directory : outputDirectories) {
        if (outputPathsSupported) {
            commandProto.add_output_paths(directory);
        }
        else {
            commandProto.add_output_directories(directory);
        }
    }

    for (const auto &platformIter : platformProperties) {
        if (!platformIter.second.empty()) {
            auto property = commandProto.mutable_platform()->add_properties();
            property->set_name(platformIter.first);
            property->set_value(platformIter.second);
        }
    }

    commandProto.set_working_directory(workingDirectory);

    return commandProto;
}

std::string
ActionBuilder::commonAncestorPath(const DependencyPairs &dependencies,
                                  const std::set<std::string> &products,
                                  const std::string &workingDirectory)
{
    int parentsNeeded = 0;
    for (const auto &dep : dependencies) {
        parentsNeeded = std::max(parentsNeeded,
                                 FileUtils::parentDirectoryLevels(dep.second));
    }

    for (const auto &product : products) {
        parentsNeeded =
            std::max(parentsNeeded, FileUtils::parentDirectoryLevels(product));
    }

    return FileUtils::lastNSegments(workingDirectory, parentsNeeded);
}

std::string
ActionBuilder::prefixWorkingDirectory(const std::string &workingDirectory,
                                      const std::string &prefix)
{
    if (prefix.empty()) {
        return workingDirectory;
    }

    return prefix + "/" + workingDirectory;
}

std::string getMerklePath(const std::string &path, const std::string &cwd,
                          buildboxcommon::NestedDirectory *nestedDirectory)
{
    // If this path is relative, prepend the remote cwd to it
    // and normalize it, getting rid of any '../' present
    std::string merklePath(path);
    if (merklePath[0] != '/' && !cwd.empty()) {
        merklePath = cwd + "/" + merklePath;
    }

    // If the path contains `..` segments before normalization, we need to
    // ensure that the directories preceding the `..` exist in the merkle tree.
    // Otherwise, the OS will fail to resolve the non-normalized path during
    // action execution with `ENOENT`.
    size_t pos = 0;
    while (true) {
        size_t dotdot = merklePath.find("/../", pos);
        if (dotdot == std::string::npos) {
            break;
        }
        if (dotdot != pos) {
            // `..` segment follows a segment that isn't `..`
            std::string merkleDirectoryPath =
                buildboxcommon::FileUtils::normalizePath(
                    merklePath.substr(0, dotdot).c_str());

            const std::lock_guard<std::mutex> lock(ContainerWriteMutex);
            nestedDirectory->addDirectory(merkleDirectoryPath.c_str());
        }
        // Set position right after the processed `..` segment
        pos = dotdot + strlen("/..");
    }

    merklePath = buildboxcommon::FileUtils::normalizePath(merklePath.c_str());

    // don't include a dependency if:
    // 1. It's an absolute path and RECC_DEPS_GLOBAL_PATHS is not specified
    // 2. It's has a prefix of an entry specified in RECC_DEPS_EXCLUDE_PATHS
    if ((merklePath[0] == '/' && !RECC_DEPS_GLOBAL_PATHS) ||
        FileUtils::hasPathPrefixes(merklePath, RECC_DEPS_EXCLUDE_PATHS)) {
        const std::lock_guard<std::mutex> lock(LogWriteMutex);
        BUILDBOX_LOG_DEBUG("Skipping \"" << merklePath << "\"");
        return "";
    }

    return merklePath;
}

void addFileToMerkleTreeHelper(
    const PathRewritePair &dep_paths, const std::string &cwd,
    buildboxcommon::NestedDirectory *nestedDirectory,
    buildboxcommon::digest_string_map *digest_to_filepaths)
{
    const std::string merklePath =
        getMerklePath(dep_paths.second, cwd, nestedDirectory);
    if (merklePath.empty()) {
        // Path is excluded
        return;
    }

    // This follows symlinks
    const auto file = buildboxcommon::File(dep_paths.first.c_str());

    {
        const std::lock_guard<std::mutex> lock(ContainerWriteMutex);
        nestedDirectory->add(file, merklePath.c_str());
        (*digest_to_filepaths)[file.d_digest] = dep_paths.first;
    }
}

void addDirectoryToMerkleTreeHelper(
    const std::string &path, const std::string &cwd,
    buildboxcommon::NestedDirectory *nestedDirectory)
{
    const std::string merklePath = getMerklePath(path, cwd, nestedDirectory);
    if (merklePath.empty()) {
        // Path is excluded
        return;
    }

    {
        const std::lock_guard<std::mutex> lock(ContainerWriteMutex);
        nestedDirectory->addDirectory(merklePath.c_str());
    }
}

void ActionBuilder::buildMerkleTree(
    DependencyPairs &dependency_paths, const std::string &cwd,
    buildboxcommon::NestedDirectory *nestedDirectory,
    buildboxcommon::digest_string_map *digest_to_filepaths)
{ // Timed function
    buildboxcommon::buildboxcommonmetrics::MetricTeeGuard<
        buildboxcommon::buildboxcommonmetrics::DurationMetricTimer>
        mt(TIMER_NAME_BUILD_MERKLE_TREE, d_durationMetricCallback);

    BUILDBOX_LOG_DEBUG("Building Merkle tree");

    std::function<void(DependencyPairs::iterator, DependencyPairs::iterator)>
        createMerkleTreeFromIterators = [&](DependencyPairs::iterator start,
                                            DependencyPairs::iterator end) {
            for (; start != end; ++start) {
                addFileToMerkleTreeHelper(*start, cwd, nestedDirectory,
                                          digest_to_filepaths);
            }
        };
    ThreadUtils::parallelizeContainerOperations(dependency_paths,
                                                createMerkleTreeFromIterators);
}

/*
 * Stuff the entire environment into the provided map, aside from variables
 * starting with "RECC_".
 */
void ActionBuilder::populateRemoteEnvWithNonReccVars(
    const char *const *env, std::map<std::string, std::string> *remoteEnv)
{
    for (int i = 0; env[i] != nullptr; ++i) {
        std::string envItem(env[i]);
        if (envItem.rfind("RECC_", 0) == 0) {
            continue;
        }
        size_t equalsPosition = envItem.find("=");
        if (equalsPosition == std::string::npos) {
            throw std::runtime_error("Could not parse environment setting " +
                                     envItem);
        }

        std::string key = envItem.substr(0, equalsPosition);
        std::string value = envItem.substr(equalsPosition + 1);

        (*remoteEnv)[key] = value;
    }
}

/**
 * Prepare the remote environment from the configuration.
 */
std::map<std::string, std::string>
ActionBuilder::prepareRemoteEnv(const ParsedCommand &command)
{
    std::map<std::string, std::string> remoteEnv;
    if (RECC_PRESERVE_ENV) {
        populateRemoteEnvWithNonReccVars(environ, &remoteEnv);
    }
    else if (RECC_ENV_TO_READ.empty()) {
        // https://gcc.gnu.org/onlinedocs/gcc/Environment-Variables.html
        // https://gcc.gnu.org/onlinedocs/libgomp/Environment-Variables.html
        // https://clang.llvm.org/docs/CommandGuide/clang.html
        // https://docs.oracle.com/cd/E37069_01/html/E54439/cc-1.html
        // https://www.ibm.com/docs/en/xl-c-and-cpp-aix/16.1?topic=variables-compile-time-link-time-environment
        // https://www.openmp.org/spec-html/5.0/openmpch6.html

        RECC_ENV_TO_READ.insert({"PATH", "LD_LIBRARY_PATH", "LANG", "LC_CTYPE",
                                 "LC_MESSAGES", "LC_ALL"});

        if (command.is_gcc() || command.is_clang()) {
            RECC_ENV_TO_READ.insert({"CPATH", "C_INCLUDE_PATH",
                                     "CPLUS_INCLUDE_PATH", "OBJC_INCLUDE_PATH",
                                     "OBJCPLUS_INCLUDE_PATH",
                                     "SOURCE_DATE_EPOCH"});
        }

        if (command.is_gcc()) {
            RECC_ENV_TO_READ.insert(
                {"GCC_COMPARE_DEBUG", "GCC_EXEC_PREFIX", "COMPILER_PATH",
                 "LIBRARY_PATH", "GCC_EXTRA_DIAGNOSTIC_OUTPUT",
                 "DEPENDENCIES_OUTPUT", "GOMP_CPU_AFFINITY", "GOMP_DEBUG",
                 "GOMP_STACKSIZE", "GOMP_SPINCOUNT",
                 "GOMP_RTEMS_THREAD_POOLS"});
        }

        if (command.is_gcc() || command.is_sun_studio()) {
            RECC_ENV_TO_READ.insert({"SUNPRO_DEPENDENCIES"});
        }
        if (command.is_sun_studio()) {
            RECC_ENV_TO_READ.insert({"PARALLEL", "STACKSIZE"});
        }

        if (command.is_AIX()) {
            RECC_ENV_TO_READ.insert(
                {"LIBPATH", "NLSPATH", "OBJECT_MODE", "XLC_USR_CONFIG"});
        }

        RECC_ENV_TO_READ.insert(
            {"OMP_CANCELLATION", "OMP_DISPLAY_ENV", "OMP_DYNAMIC",
             "OMP_MAX_ACTIVE_LEVELS", "OMP_MAX_TASK_PRIORITY", "OMP_NESTED",
             "OMP_NUM_TEAMS", "OMP_NUM_THREADS", "OMP_PROC_BIND", "OMP_PLACES",
             "OMP_STACKSIZE", "OMP_SCHEDULE", "OMP_TARGET_OFFLOAD",
             "OMP_TEAMS_THREAD_LIMIT", "OMP_THREAD_LIMIT", "OMP_WAIT_POLICY"});
    }

    // Environment variables which have path-like segments delimited by ':'
    std::unordered_set<std::string> pathLikeEnv = {"PATH",
                                                   "LD_LIBRARY_PATH",
                                                   "CPATH",
                                                   "C_INCLUDE_PATH",
                                                   "CPLUS_INCLUDE_PATH",
                                                   "OBJC_INCLUDE_PATH",
                                                   "OBJCPLUS_INCLUDE_PATH",
                                                   "COMPILER_PATH",
                                                   "LIBRARY_PATH",
                                                   "LIB_PATH"};
    for (const std::string &envVar : RECC_ENV_TO_READ) {
        const char *envVal = getenv(envVar.c_str());
        if (envVal != nullptr) {
            if (pathLikeEnv.find(envVar) != pathLikeEnv.end() &&
                strcmp(envVal, "") != 0) {
                // Split the string on ':' to get the individual paths
                // and run each through prefix map resolution, joining them
                // back up afterwords
                const std::string pathPart = envVal;
                std::string pathMapEnv;
                size_t delim_pos = std::string::npos;
                size_t delim_prev = 0;
                while ((delim_pos = pathPart.find(':', delim_prev)) !=
                       std::string::npos) {
                    const std::string pathPiece =
                        pathPart.substr(delim_prev, delim_pos - delim_prev);
                    if (!pathPiece.empty()) {
                        pathMapEnv +=
                            FileUtils::resolvePathFromPrefixMap(pathPiece) +
                            ":";
                    }
                    delim_prev = delim_pos + 1;
                }
                // Grab the last path
                const std::string pathPiece = pathPart.substr(delim_prev);
                if (!pathPiece.empty()) {
                    pathMapEnv +=
                        FileUtils::resolvePathFromPrefixMap(pathPiece);
                }
                remoteEnv[envVar] = pathMapEnv;
            }
            else {
                remoteEnv[envVar] = envVal;
            }
        }
    }
    for (std::pair<std::string, std::string> overrideElem : RECC_REMOTE_ENV) {
        remoteEnv[overrideElem.first] = overrideElem.second;
    }
    return remoteEnv;
}

void ActionBuilder::getDependencies(const ParsedCommand &command,
                                    std::set<std::string> *dependencies,
                                    std::set<std::string> *products)
{

    BUILDBOX_LOG_DEBUG("Getting dependencies using the command:");
    if (RECC_VERBOSE == true) {
        std::ostringstream dep_command;
        for (auto &depc : command.get_dependencies_command()) 
        {
            dep_command << depc << " ";
        }
       // std::cerr<<"ACTIONBUILDER 4444444"<<depc<<std::endl;
        //BUILDBOX_LOG_DEBUG(dep_command);
        BUILDBOX_LOG_DEBUG(dep_command.str());
    }

    CommandFileInfo fileInfo;
    { // Timed block

          BUILDBOX_LOG_DEBUG("IN FILEINFO OF DEPS7777");
        buildboxcommon::buildboxcommonmetrics::MetricTeeGuard<
            buildboxcommon::buildboxcommonmetrics::DurationMetricTimer>
            mt(TIMER_NAME_COMPILER_DEPS, d_durationMetricCallback);
             BUILDBOX_LOG_DEBUG("ABOVE FILEINFO OF DEPS7777");
        fileInfo = Deps::get_file_info(command);
         //BUILDBOX_LOG_DEBUG("AFTER FILEINFO OF DEPS7777");
    }

    *dependencies = fileInfo.d_dependencies;

    if (RECC_OUTPUT_DIRECTORIES_OVERRIDE.empty() &&
        RECC_OUTPUT_FILES_OVERRIDE.empty()) {
        *products = fileInfo.d_possibleProducts;
    }
}

std::shared_ptr<proto::Action> ActionBuilder::BuildAction(
    const ParsedCommand &command, const std::string &cwd,
    buildboxcommon::digest_string_map *blobs,
    buildboxcommon::digest_string_map *digest_to_filepaths,
    std::set<std::string> *productsPtr)
{
    if (!command.is_compiler_command() && !RECC_FORCE_REMOTE) {
        return nullptr;
    }

    std::string commandWorkingDirectory;
    buildboxcommon::NestedDirectory nestedDirectory;

    std::set<std::string> products = RECC_OUTPUT_FILES_OVERRIDE;
    if (!RECC_DEPS_DIRECTORY_OVERRIDE.empty()) {
        BUILDBOX_LOG_DEBUG("Building Merkle tree using directory override");
        // When RECC_DEPS_DIRECTORY_OVERRIDE is set, we will not follow
        // symlinks to help us avoid getting into an endless loop.
        nestedDirectory = buildboxcommon::make_nesteddirectory(
            RECC_DEPS_DIRECTORY_OVERRIDE.c_str(), digest_to_filepaths, false);

        const auto replacedRoot =
            normalize_replace_root(RECC_DEPS_DIRECTORY_OVERRIDE);

        BUILDBOX_LOG_DEBUG("Mapping local file path: ["
                           << RECC_DEPS_DIRECTORY_OVERRIDE
                           << "] to normalized-relative (if)updated: ["
                           << replacedRoot << "]");

        // Create parent directory structure based on the normalized/replaced
        // path.
        const auto pathComponents = FileUtils::parseDirectories(replacedRoot);
        for (auto it = pathComponents.rbegin(); it != pathComponents.rend();
             it++) {
            auto parentDirectory = buildboxcommon::NestedDirectory();
            parentDirectory.d_subdirs->emplace(*it,
                                               std::move(nestedDirectory));
            nestedDirectory = std::move(parentDirectory);
        }

        commandWorkingDirectory = RECC_WORKING_DIR_PREFIX;
    }
    else {
        std::set<std::string> deps;
        if (RECC_DEPS_OVERRIDE.empty() && !RECC_FORCE_REMOTE) {
            try {
                getDependencies(command, &deps, &products);

                // If no dependencies are found, there won't be any input files
                // in the remote, so run the build locally.
                if (deps.empty()) {
                    BUILDBOX_LOG_INFO("No deps found. Running locally.");
                    return nullptr;
                }

                // Similarly if no output files are found we aren't actually
                // compiling anything that recc understands, run the build
                // locally
                if (products.empty()) {
                    BUILDBOX_LOG_INFO("No products found. Running locally.");
                    return nullptr;
                }
            }
            catch (const std::invalid_argument &e) {
                BUILDBOX_LOG_INFO(e.what());
                return nullptr;
            }
            catch (const subprocess_failed_error &) {
                BUILDBOX_LOG_DEBUG("Running locally to display the error.");
                return nullptr;
            }
        }
        else {
            deps = RECC_DEPS_OVERRIDE;
        }

        // Go through all the dependencies and apply any required path
        // transformations, constructing DependencyParis
        // corresponding to filesystem path -> transformed merkle tree path
        DependencyPairs dep_path_pairs;
        for (const auto &dep : deps) {
            std::string modifiedDep(dep);
            if (modifiedDep[0] == '/') {
                modifiedDep = FileUtils::resolvePathFromPrefixMap(modifiedDep);
                // Make path relative if needed
                modifiedDep =
                    FileUtils::rewritePathToRelative(modifiedDep, cwd);
                BUILDBOX_LOG_DEBUG("Mapping local path: ["
                                   << dep << "] to remote path: ["
                                   << modifiedDep << "]");
            }
            dep_path_pairs.push_back(std::make_pair(dep, modifiedDep));
        }

        if (RECC_NO_PATH_REWRITE && RECC_WORKING_DIR_PREFIX.empty()) {
            commandWorkingDirectory = cwd;
            while (FileUtils::isAbsolutePath(commandWorkingDirectory)) {
                commandWorkingDirectory = commandWorkingDirectory.substr(1);
            }
        }
        else {
            const auto commonAncestor =
                commonAncestorPath(dep_path_pairs, products, cwd);
            commandWorkingDirectory = prefixWorkingDirectory(
                commonAncestor, RECC_WORKING_DIR_PREFIX);
        }

        buildMerkleTree(dep_path_pairs, commandWorkingDirectory,
                        &nestedDirectory, digest_to_filepaths);
    }

    if (!commandWorkingDirectory.empty()) {
        commandWorkingDirectory = buildboxcommon::FileUtils::normalizePath(
            commandWorkingDirectory.c_str());
        nestedDirectory.addDirectory(commandWorkingDirectory.c_str());
    }

    if (command.d_upload_all_include_dirs) {
        for (const auto &include_dir : command.d_includeDirs) {
            addDirectoryToMerkleTreeHelper(
                include_dir, commandWorkingDirectory, &nestedDirectory);
        }
    }

    for (const auto &product : products) {
        if (!product.empty() && product[0] == '/') {
            BUILDBOX_LOG_DEBUG(
                "Command produces file in a location unrelated "
                "to the current directory, so running locally.");
            BUILDBOX_LOG_DEBUG(
                "(use RECC_OUTPUT_[FILES|DIRECTORIES]_OVERRIDE to "
                "override)");
            return nullptr;
        }
    }

    const auto directoryDigest = nestedDirectory.to_digest(blobs);

    std::map<std::string, std::string> remoteEnv = prepareRemoteEnv(command);
    const proto::Command commandProto = generateCommandProto(
        command.get_command(), products, RECC_OUTPUT_DIRECTORIES_OVERRIDE,
        remoteEnv, RECC_REMOTE_PLATFORM, commandWorkingDirectory);
    BUILDBOX_LOG_DEBUG("Command: " << commandProto.ShortDebugString());

    const auto commandDigest = DigestGenerator::make_digest(commandProto);
    (*blobs)[commandDigest] = commandProto.SerializeAsString();

    proto::Action action;
    action.mutable_command_digest()->CopyFrom(commandDigest);
    action.mutable_input_root_digest()->CopyFrom(directoryDigest);
    action.set_do_not_cache(RECC_ACTION_UNCACHEABLE);
    if (!RECC_ACTION_SALT.empty()) {
        action.set_salt(RECC_ACTION_SALT);
    }

    // REAPI v2.2 allows setting the platform property list in the `Action`
    // message, which allows servers to immediately read it without having to
    // dereference the corresponding `Command`.
    if (Env::configured_reapi_version_equal_to_or_newer_than("2.2")) {
        action.mutable_platform()->CopyFrom(commandProto.platform());
    }

    if (productsPtr != nullptr) {
        *productsPtr = products;
    }

    return std::make_shared<proto::Action>(action);
}

build::bazel::remote::execution::v2::Command
ActionBuilder::generateCommandProto(
    const std::vector<std::string> &command,
    const std::set<std::string> &products,
    const std::set<std::string> &outputDirectories,
    const std::map<std::string, std::string> &remoteEnvironment,
    const std::map<std::string, std::string> &platformProperties,
    const std::string &workingDirectory)
{
    // If dependency paths aren't absolute, they are made absolute by
    // having the CWD prepended, and then normalized and replaced. If that
    // is the case, and the CWD contains a replaced prefix, then replace
    // it.
    const auto resolvedWorkingDirectory =
        FileUtils::resolvePathFromPrefixMap(workingDirectory);

    return ActionBuilder::populateCommandProto(
        command, products, outputDirectories, remoteEnvironment,
        platformProperties, resolvedWorkingDirectory);
}

ActionBuilder::ActionBuilder(
    const WriteMetricCallback &duration_metric_callback)
{
    if (duration_metric_callback == nullptr) {
        // no-op callback
        d_durationMetricCallback = [](auto &&...) {};
    }
    else {
        d_durationMetricCallback = duration_metric_callback;
    }
}

} // namespace recc
