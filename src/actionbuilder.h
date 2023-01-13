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

#ifndef INCLUDED_ACTIONBUILDER
#define INCLUDED_ACTIONBUILDER

#include <deps.h>
#include <protos.h>

#include <buildboxcommon_merklize.h>
#include <buildboxcommonmetrics_durationmetricvalue.h>

#include <memory>
#include <unordered_map>

namespace recc {

extern std::mutex ContainerWriteMutex;
extern std::mutex LogWriteMutex;

// Path to file on disk and it's associated location
// to be placed in the input root merkle tree
typedef std::pair<std::string, std::string> PathRewritePair;
typedef std::vector<PathRewritePair> DependencyPairs;

class ActionBuilder {
  private:
    typedef std::function<void(
        const std::string &,
        buildboxcommon::buildboxcommonmetrics::DurationMetricValue)>
        WriteMetricCallback;
    WriteMetricCallback d_durationMetricCallback;

  public:
    ActionBuilder(
        const WriteMetricCallback &duration_metric_callback = nullptr);

    /**
     * Build an `Action` from the given `ParsedCommand` and working directory.
     *
     * Returns `nullptr` if an action could not be built due to invoking a
     * non-compile command, an output files in specified in a directory
     * unrelated to the current working directory, or a command that does not
     * contain either a relative or absolute path to an executable.
     *
     * `digest_to_filepaths` and `blobs` are used to store parsed input and
     * output files, which will get uploaded to CAS by the caller.
     */
    std::shared_ptr<proto::Action>
    BuildAction(const ParsedCommand &command, const std::string &cwd,
                buildboxcommon::digest_string_map *digest_to_filepaths,
                buildboxcommon::digest_string_map *blobs,
                std::set<std::string> *products = nullptr);

  protected: // for unit testing
    static proto::Command generateCommandProto(
        const std::vector<std::string> &command,
        const std::set<std::string> &products,
        const std::set<std::string> &outputDirectories,
        const std::map<std::string, std::string> &remoteEnvironment,
        const std::map<std::string, std::string> &platformProperties,
        const std::string &workingDirectory);

    /**
     * Populates a `Command` protobuf from a `ParsedCommand` and additional
     * information.
     *
     * It sets the command's arguments, its output directories, the environment
     * variables and platform properties for the remote.
     */
    static proto::Command populateCommandProto(
        const std::vector<std::string> &command,
        const std::set<std::string> &products,
        const std::set<std::string> &outputDirectories,
        const std::map<std::string, std::string> &remoteEnvironment,
        const std::map<std::string, std::string> &platformProperties,
        const std::string &workingDirectory);

    /**
     * Given a vector of filesystem -> Merkle path pairs to dependency and
     * output files, builds a Merkle tree.
     *
     * Adds the files to `buildboxcommon::NestedDirectory` and
     * `digest_to_filepaths`.
     *
     * If necessary, modifies the contents of `commandWorkingDirectory`.
     */
    void
    buildMerkleTree(DependencyPairs &deps_paths, const std::string &cwd,
                    buildboxcommon::NestedDirectory *nestedDirectory,
                    buildboxcommon::digest_string_map *digest_to_filepaths);

    /**
     * Gathers the `CommandFileInfo` belonging to the given `command` and
     * populates its dependency and product list (the latter only if no
     * overrides are set).
     */
    void getDependencies(const ParsedCommand &command,
                         std::set<std::string> *dependencies,
                         std::set<std::string> *products);

    /** Scans the list of dependencies and output files and strips
     * `workingDirectory` to the level of the common ancestor. For
     * dependencies, only look at the filesystem path, not the merkle path
     */
    static std::string
    commonAncestorPath(const DependencyPairs &dependencies,
                       const std::set<std::string> &products,
                       const std::string &workingDirectory);

    /**
     * If prefix is not empty, prepends it to the working directory path.
     * Otherwise `workingDirectory` is return unmodified.
     */
    static std::string
    prefixWorkingDirectory(const std::string &workingDirectory,
                           const std::string &prefix);

    /**
     * Populate the given environment map with the environment.
     */
    static void populateRemoteEnvWithNonReccVars(
        const char *const *env, std::map<std::string, std::string> *remoteEnv);

    /**
     * Prepare the remote environment.
     */
    static std::map<std::string, std::string>
    prepareRemoteEnv(const ParsedCommand &command);
};

} // namespace recc

#endif
