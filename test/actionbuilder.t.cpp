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

#include <actionbuilder.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_merklize.h>
#include <buildboxcommonmetrics_durationmetricvalue.h>
#include <buildboxcommonmetrics_testingutils.h>
#include <digestgenerator.h>
#include <env.h>
#include <fileutils.h>
#include <fstream>
#include <protos.h>

#include <gtest/gtest.h>

#define TIMER_NAME_COMPILER_DEPS "recc.compiler_deps"
#define TIMER_NAME_BUILD_MERKLE_TREE "recc.build_merkle_tree"

using namespace recc;
using namespace buildboxcommon::buildboxcommonmetrics;

class ActionBuilderTestFixture
    : public ActionBuilder,
      public ::testing::TestWithParam<const char *> {
  protected:
    ActionBuilderTestFixture() : cwd(FileUtils::getCurrentWorkingDirectory())
    {
    }

    void SetUp() override
    {
        d_previous_recc_replacement_map = RECC_PREFIX_REPLACEMENT;
        d_previous_deps_override = RECC_DEPS_OVERRIDE;
        d_previous_force_remote = RECC_FORCE_REMOTE;
        d_previous_deps_global_path = RECC_DEPS_GLOBAL_PATHS;
        d_previous_deps_exclude_paths = RECC_DEPS_EXCLUDE_PATHS;
        d_previous_working_dir_prefix = RECC_WORKING_DIR_PREFIX;
        d_previous_reapi_version = RECC_REAPI_VERSION;
        d_previous_remote_platform = RECC_REMOTE_PLATFORM;
        d_previous_action_salt = RECC_ACTION_SALT;

        const char *path = std::getenv("PATH");
        if (path != nullptr) {
            d_previous_path = path;
        }

        // Don't let host environment affect command proto
        unsetenv("LANG");
        unsetenv("LC_ALL");
        unsetenv("LC_CTYPE");
        unsetenv("LC_MESSAGES");
        unsetenv("LD_LIBRARY_PATH");
        unsetenv("SOURCE_DATE_EPOCH");
    }

    void TearDown() override
    {
        RECC_PREFIX_REPLACEMENT = d_previous_recc_replacement_map;
        RECC_DEPS_OVERRIDE = d_previous_deps_override;
        RECC_FORCE_REMOTE = d_previous_force_remote;
        RECC_DEPS_GLOBAL_PATHS = d_previous_deps_global_path;
        RECC_DEPS_EXCLUDE_PATHS = d_previous_deps_exclude_paths;
        RECC_WORKING_DIR_PREFIX = d_previous_working_dir_prefix;
        RECC_REAPI_VERSION = d_previous_reapi_version;
        RECC_REMOTE_PLATFORM = d_previous_remote_platform;
        RECC_ACTION_SALT = d_previous_action_salt;

        setenv("PATH", d_previous_path.c_str(), 1);
    }

    void writeDependenciesToTempFile(const std::string &dependency_file_name)
    {
        const std::string path_to_hello = cwd + "/" + "hello.cpp";
        // references the hello.cpp file in test/data/actionbuilder/hello.cpp
        const std::string make_rule = "hello.o: " + path_to_hello + "\n";
        std::ofstream fakedependencyfile;
        fakedependencyfile.open(dependency_file_name);
        fakedependencyfile << make_rule;
        fakedependencyfile.close();
    }

    std::string findCommandInPath(const std::string &command)
    {
        std::string path = std::getenv("PATH");
        int startPos = 0;
        int nextColon = 0;
        while (nextColon >= 0 && nextColon < path.length()) {
            int nextColon = path.find(':', startPos);
            std::string dir = path.substr(startPos, nextColon - startPos);
            std::string fullCommand = dir + "/" + command;
            if (buildboxcommon::FileUtils::isRegularFile(
                    fullCommand.c_str()) &&
                buildboxcommon::FileUtils::isExecutable(fullCommand.c_str())) {
                return fullCommand;
            }
            else {
                startPos = nextColon + 1;
            }
        }
        return "";
    }

    buildboxcommon::digest_string_map blobs;
    buildboxcommon::digest_string_map digest_to_filepaths;
    std::string cwd;

  private:
    std::vector<std::pair<std::string, std::string>>
        d_previous_recc_replacement_map;
    std::set<std::string> d_previous_deps_override;
    std::set<std::string> d_previous_deps_exclude_paths;
    bool d_previous_force_remote;
    bool d_previous_deps_global_path;
    std::string d_previous_working_dir_prefix;
    std::string d_previous_reapi_version;
    std::map<std::string, std::string> d_previous_remote_platform;
    std::string d_previous_action_salt;
    std::string d_previous_path;
};

TEST_F(ActionBuilderTestFixture, BuildSimpleCommandWithOutputFiles)
{
    RECC_REAPI_VERSION = "2.0";

    const std::vector<std::string> command_arguments = {
        "/my/fake/gcc", "-c", "hello.cpp", "-o", "hello.o"};
    const std::set<std::string> output_files = {"hello.o"};
    const std::string working_directory = ".";

    const proto::Command command_proto = this->generateCommandProto(
        command_arguments, output_files, {}, {}, {}, working_directory);

    ASSERT_TRUE(std::equal(command_arguments.cbegin(),
                           command_arguments.cend(),
                           command_proto.arguments().cbegin()));

    ASSERT_TRUE(std::equal(output_files.cbegin(), output_files.cend(),
                           command_proto.output_files().cbegin()));

    ASSERT_EQ(command_proto.working_directory(), working_directory);

    ASSERT_TRUE(command_proto.environment_variables().empty());
    ASSERT_TRUE(command_proto.platform().properties().empty());
    ASSERT_TRUE(command_proto.output_directories().empty());
    ASSERT_TRUE(command_proto.output_paths().empty());
}

TEST_P(ActionBuilderTestFixture, BuildSimpleCommandWithOutputPaths)
{
    const std::vector<std::string> command_arguments = {
        "/my/fake/gcc", "-c", "hello.cpp", "-o", "hello.o"};
    const std::set<std::string> output_files = {"hello.o"};
    const std::string working_directory = ".";

    const proto::Command command_proto = this->generateCommandProto(
        command_arguments, output_files, {}, {}, {}, working_directory);

    ASSERT_TRUE(std::equal(command_arguments.cbegin(),
                           command_arguments.cend(),
                           command_proto.arguments().cbegin()));

    ASSERT_EQ(command_proto.working_directory(), working_directory);
    ASSERT_TRUE(command_proto.environment_variables().empty());
    ASSERT_TRUE(command_proto.platform().properties().empty());

    ASSERT_EQ(command_proto.output_paths_size(), 1);
    ASSERT_EQ(command_proto.output_paths(0), "hello.o");

    ASSERT_TRUE(command_proto.output_files().empty());
    ASSERT_TRUE(command_proto.output_directories().empty());
}

TEST_F(ActionBuilderTestFixture, PlatformPropertiesInActionWhenSupported)
{
    RECC_REAPI_VERSION = "2.2";
    // v2.2 adds the `Action.platform` field.

    RECC_REMOTE_PLATFORM = {{"OS", "linux"}, {"ISA", "x86"}};

    const std::vector<std::string> recc_args = {"./gcc", "-c", "hello.cpp",
                                                "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());

    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);

    ASSERT_NE(actionPtr, nullptr);

    ASSERT_EQ(actionPtr->platform().properties_size(), 2);
    for (const auto &property : actionPtr->platform().properties()) {
        ASSERT_TRUE(RECC_REMOTE_PLATFORM.count(property.name()));
        EXPECT_EQ(RECC_REMOTE_PLATFORM.at(property.name()), property.value());
    }
}

TEST_F(ActionBuilderTestFixture, IncludeDirectoriesUploaded)
{
    RECC_REAPI_VERSION = "2.2";
    // v2.2 adds the `Action.platform` field.
    RECC_WORKING_DIR_PREFIX = "recc-build";

    const std::vector<std::string> include_directories = {
        "../deps", "/dir/not/exist", "/no/dir"};

    const std::vector<std::string> recc_args = {"./gcc",
                                                "-I" + include_directories[0],
                                                "-Wmissing-include-dirs",
                                                "-I",
                                                include_directories[1],
                                                "-I=" + include_directories[2],
                                                "-c",
                                                "hello.cpp",
                                                "-o",
                                                "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());

    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);

    ASSERT_EQ(blobs.size(), 4);
}

TEST_F(ActionBuilderTestFixture, IncludeDirectoriesUploadedWerror)
{
    RECC_REAPI_VERSION = "2.2";
    // v2.2 adds the `Action.platform` field.
    RECC_WORKING_DIR_PREFIX = "recc-build";

    const std::vector<std::string> include_directories = {
        "../deps", "/dir/not/exist", "/no/dir"};

    const std::vector<std::string> recc_args = {"./gcc",
                                                "-I" + include_directories[0],
                                                "-Werror=missing-include-dirs",
                                                "-I",
                                                include_directories[1],
                                                "-I=" + include_directories[2],
                                                "-c",
                                                "hello.cpp",
                                                "-o",
                                                "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());

    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);

    ASSERT_EQ(blobs.size(), 4);
}

TEST_F(ActionBuilderTestFixture, IncludeDirectoriesNotUploaded)
{
    RECC_REAPI_VERSION = "2.2";
    // v2.2 adds the `Action.platform` field.

    const std::vector<std::string> include_directories = {
        "../deps", "/dir/not/exist", "/no/dir"};

    const std::vector<std::string> recc_args = {"./gcc",
                                                "-I" + include_directories[0],
                                                "-I",
                                                include_directories[1],
                                                "-I=" + include_directories[2],
                                                "-c",
                                                "hello.cpp",
                                                "-o",
                                                "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());

    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);

    ASSERT_EQ(blobs.size(), 2);
}

TEST_F(ActionBuilderTestFixture, GetDependenciesAIX)
{
    const std::vector<std::string> recc_args = {
        "./xlc", "-c", cwd + "/" + "hello.cpp ", "-o", "hello.o"};
    std::set<std::string> deps;
    std::set<std::string> prod;
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    ASSERT_FALSE(command.get_aix_dependency_file_name().empty());

    writeDependenciesToTempFile(command.get_aix_dependency_file_name());

    RECC_DEPS_GLOBAL_PATHS = 1;
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);
}

TEST_F(ActionBuilderTestFixture, NoDeps)
{
    // When no dependency output is produced, we should not build an action.
    // The dependencies include input source file(s), so we would have nothing
    // to compile on the remote end.
    const std::vector<std::string> recc_args = {"./no_output/gcc", "-c",
                                                "hello.cpp", "-o", "hello.o"};
    std::set<std::string> deps;
    std::set<std::string> prod;
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    RECC_DEPS_GLOBAL_PATHS = 1;
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_EQ(actionPtr, nullptr);
}

TEST_F(ActionBuilderTestFixture, UnsupportedFileType)
{
    // When no outputs are produced, we should not build an action.
    // recc currently only supports compiles that produce outputs, so gcc -M
    // or gcc -v would be run locally due to no output files
    std::string fullGccPath = findCommandInPath("gcc");
    if (fullGccPath.empty()) {
        // TODO: replace with GTEST_SKIP() when the buildbox-common docker
        // container's debian version supports it
        BUILDBOX_LOG_INFO("Could not find gcc in PATH. Skipping test.")
    }
    else {
        const std::vector<std::string> recc_args = {fullGccPath, "-c",
                                                    "hello.s"};
        std::set<std::string> deps;
        std::set<std::string> prod;
        const auto command =
            ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
        RECC_DEPS_GLOBAL_PATHS = 1;
        const auto actionPtr = ActionBuilder::BuildAction(
            command, cwd, &blobs, &digest_to_filepaths);
        ASSERT_EQ(actionPtr, nullptr);
    }
}

TEST_F(ActionBuilderTestFixture, GccDashXAssemblerWithDashMReturnsNoDeps)
{
    // gcc's "-x assembler" flag combined with "-M" produces no output
    std::string fullGccPath = findCommandInPath("gcc");
    if (fullGccPath.empty()) {
        // TODO: replace with GTEST_SKIP() when the buildbox-common docker
        // container's debian version supports it
        BUILDBOX_LOG_INFO("Could not find gcc in PATH. Skipping test.")
    }
    else {
        const std::vector<std::string> recc_args = {
            fullGccPath, "-x", "assembler", "-c", "hello.s", "-o", "hello.o"};
        std::set<std::string> deps;
        std::set<std::string> prod;
        const auto command =
            ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
        RECC_DEPS_GLOBAL_PATHS = 1;
        const auto actionPtr = ActionBuilder::BuildAction(
            command, cwd, &blobs, &digest_to_filepaths);
        ASSERT_EQ(actionPtr, nullptr);
    }
}

TEST_F(ActionBuilderTestFixture, IllegalNonCompileCommand)
{
    const std::vector<std::string> recc_args = {"command-with-no-path", "foo",
                                                "bar"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());

    ASSERT_TRUE(command.get_aix_dependency_file_name().empty());

    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_EQ(actionPtr, nullptr);
}

TEST_F(ActionBuilderTestFixture, CompileCommandWithPathLookup)
{
    const std::vector<std::string> recc_args = {"gcc", "-c", "hello.cpp", "-o",
                                                "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());

    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);
}

TEST_F(ActionBuilderTestFixture, BuildMerkleTreeVerifyMetricsCollection)
{
    DependencyPairs dep_pairs;
    buildMerkleTree(dep_pairs, "cwd", nullptr, nullptr);
    EXPECT_TRUE(
        collectedByName<DurationMetricValue>(TIMER_NAME_BUILD_MERKLE_TREE));
}

TEST_F(ActionBuilderTestFixture, GetDependenciesVerifyMetricsCollection)
{
    const std::vector<std::string> recc_args = {"./gcc", "-c", "hello.cpp",
                                                "-o", "hello.o"};
    std::set<std::string> deps;
    std::set<std::string> prod;
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    getDependencies(command, &deps, &prod);
    EXPECT_TRUE(
        collectedByName<DurationMetricValue>(TIMER_NAME_COMPILER_DEPS));
}

TEST_F(ActionBuilderTestFixture, OutputFilesAndDirectories)
{
    RECC_REAPI_VERSION = "2.0";

    const std::set<std::string> output_files = {"hello.o", "foo.o", "bar.o"};
    const std::set<std::string> output_directories = {"export/"};

    const proto::Command command_proto = this->generateCommandProto(
        {}, output_files, output_directories, {}, {}, "");

    ASSERT_TRUE(std::equal(output_files.cbegin(), output_files.cend(),
                           command_proto.output_files().cbegin()));

    ASSERT_TRUE(std::equal(output_directories.cbegin(),
                           output_directories.cend(),
                           command_proto.output_directories().cbegin()));

    ASSERT_TRUE(command_proto.output_paths().empty());
}

TEST_P(ActionBuilderTestFixture, OutputPaths)
{
    RECC_REAPI_VERSION = "2.1";

    const std::set<std::string> output_files = {"hello.o", "foo.o", "bar.o"};
    const std::set<std::string> output_directories = {"export/"};

    const proto::Command command_proto = this->generateCommandProto(
        {}, output_files, output_directories, {}, {}, "");

    const std::vector<std::string> expectedOutputPaths = {"hello.o", "foo.o",
                                                          "bar.o", "export/"};

    ASSERT_TRUE(std::is_permutation(expectedOutputPaths.cbegin(),
                                    expectedOutputPaths.cend(),
                                    command_proto.output_paths().cbegin()));

    ASSERT_TRUE(command_proto.output_files().empty());
    ASSERT_TRUE(command_proto.output_directories().empty());
}

TEST_F(ActionBuilderTestFixture, EnvironmentVariables)
{
    std::map<std::string, std::string> environment;
    environment["PATH"] = "/usr/bin:/opt/bin:/bin";
    environment["GREETING"] = "hello!";

    const proto::Command command_proto =
        this->generateCommandProto({}, {}, {}, environment, {}, "");

    ASSERT_EQ(command_proto.environment_variables_size(), 2);
    for (const auto &variable : command_proto.environment_variables()) {
        ASSERT_TRUE(environment.count(variable.name()));
        ASSERT_EQ(variable.value(), environment.at(variable.name()));
    }
}

TEST_F(ActionBuilderTestFixture, PreserveEnvironmentVariables)
{
    bool oldReccPreserveEnv = RECC_PRESERVE_ENV;
    RECC_PRESERVE_ENV = true;

    // These are the variables we'll eventually set in the environment
    std::map<std::string, std::string> testEnv = {
        {"MY_BOGUS_TEST_VAR_1", "MY_BOGUS_VALUE_1"},
        {"MY_BOGUS_TEST_VAR_2", "MY_BOGUS_VALUE_2"},
        {"RECC_BOGUS_VAR", "RECC_BOGUS_VALUE"}};

    // Save the old values of the bogus vars and unset them in the env
    std::map<std::string, std::string> oldEnv;
    for (const auto &envPair : testEnv) {
        const std::string envVar = envPair.first;
        const char *envVal = getenv(envVar.c_str());
        if (envVal != nullptr) {
            oldEnv[envVar] = envVal;
        }
        else {
            oldEnv[envVar] = "";
        }
        unsetenv(envVar.c_str());
    }

    ParsedCommand command;

    // When we call prepareRemoteEnv, we shouldn't see the test vars
    std::map<std::string, std::string> remoteEnv =
        ActionBuilder::prepareRemoteEnv(command);
    for (const auto &envPair : testEnv) {
        const std::string &envVar = envPair.first;
        ASSERT_FALSE(remoteEnv.count(envVar));
    }

    // Populate the test variables
    for (const auto &envPair : testEnv) {
        setenv(envPair.first.c_str(), envPair.second.c_str(), 1);
    }

    // Now calling prepareRemoteEnv should have the variables from testEnv,
    // except for those with "RECC_" as a prefix
    remoteEnv = ActionBuilder::prepareRemoteEnv(command);
    for (const auto &envPair : testEnv) {
        const std::string &envVar = envPair.first;
        if (envVar.rfind("RECC_", 0) == 0) {
            ASSERT_FALSE(remoteEnv.count(envVar));
        }
        else {
            ASSERT_TRUE(remoteEnv.count(envVar));
            ASSERT_TRUE(remoteEnv[envVar] == testEnv[envVar]);
        }
    }

    // Restore the environment back to the way it was
    for (const auto &envPair : oldEnv) {
        setenv(envPair.first.c_str(), envPair.second.c_str(), 1);
    }
    RECC_PRESERVE_ENV = oldReccPreserveEnv;
}

TEST_F(ActionBuilderTestFixture, ReccEnvvarsToRead)
{
    std::set<std::string> oldReccEnvvarsToRead = RECC_ENV_TO_READ;
    RECC_ENV_TO_READ = std::set<std::string>{
        "MY_BOGUS_VAR_1", "RECC_BOGUS_VAR", "MY_UNSET_VAR", "RECC_UNSET_VAR"};

    // We will add these variables to the environment at the time of test
    std::map<std::string, std::string> testEnv = {
        {"MY_BOGUS_VAR_1", "MY_BOGUS_VALUE_1"},
        {"MY_BOGUS_VAR_2", "MY_BOGUS_VALUE_2"},
        {"RECC_BOGUS_VAR", "RECC_BOGUS_VALUE"}};

    // Save the old values of the bogus vars and populate them according to the
    // test environment
    std::map<std::string, std::string> oldEnv;
    for (const auto &envPair : testEnv) {
        const std::string envVar = envPair.first;
        const char *envVal = getenv(envVar.c_str());
        if (envVal != nullptr) {
            oldEnv[envVar] = envVal;
        }
        else {
            oldEnv[envVar] = "";
        }
        setenv(envVar.c_str(), envPair.second.c_str(), 1);
    }

    ParsedCommand command;

    // When we call prepareRemoteEnv, we should see the variables from
    // RECC_ENV_TO_READ if they were in the environment
    std::map<std::string, std::string> remoteEnv =
        ActionBuilder::prepareRemoteEnv(command);
    for (const auto &envPair : testEnv) {
        const std::string &envVar = envPair.first;
        // Make sure the variable is in RECC_ENV_TO_READ
        if (RECC_ENV_TO_READ.count(envVar)) {
            ASSERT_TRUE(remoteEnv.count(envVar));
            ASSERT_TRUE(remoteEnv[envVar] == testEnv[envVar]);
        }
    }

    // Restore the environment back to the way it was
    for (const auto &envPair : oldEnv) {
        setenv(envPair.first.c_str(), envPair.second.c_str(), 1);
    }
    RECC_ENV_TO_READ = oldReccEnvvarsToRead;
}

TEST_F(ActionBuilderTestFixture, ReccEnvVarsRespectPrefixMap)
{
    // Update the values of PATH/LD_LIBRARY_PATH, which are captured as part of
    // the Action by default for all compilers
    std::map<std::string, std::string> testEnv = {
        {"PATH", "/usr/local/bin:/my/example/path:/bin"},
        {"LD_LIBRARY_PATH", "/some/other/path"}};
    std::map<std::string, std::string> oldEnv;
    for (const auto &envPair : testEnv) {
        const std::string envVar = envPair.first;
        std::cerr << "Looking at envVar " << envVar << std::endl;
        const char *envVal = getenv(envVar.c_str());
        if (envVal != nullptr) {
            oldEnv[envVar] = envVal;
        }
        else {
            oldEnv[envVar] = "";
        }
        setenv(envVar.c_str(), envPair.second.c_str(), 1);
    }

    // First verify that env variables are identical without
    // RECC_PREFIX_REPLACEMENT
    ParsedCommand command;
    std::map<std::string, std::string> remoteEnv =
        ActionBuilder::prepareRemoteEnv(command);
    for (const auto &envPair : testEnv) {
        const std::string &envVar = envPair.first;
        ASSERT_TRUE(remoteEnv.count(envVar));
        ASSERT_EQ(remoteEnv[envVar], testEnv[envVar]);
    }

    // Set RECC_PREFIX_REPLACEMENT and verify the expected path subtitutions
    // happen in both variables
    auto oldPrefixMap = RECC_PREFIX_REPLACEMENT;
    RECC_PREFIX_REPLACEMENT = {{"/my/example/path", "/rewritten/path"},
                               {"/some/other/path", "/path2"}};
    std::map<std::string, std::string> prefixMappedEnv = {
        {"PATH", "/usr/local/bin:/rewritten/path:/bin"},
        {"LD_LIBRARY_PATH", "/path2"}};

    remoteEnv = ActionBuilder::prepareRemoteEnv(command);
    for (const auto &envPair : testEnv) {
        const std::string &envVar = envPair.first;
        ASSERT_TRUE(remoteEnv.count(envVar));
        ASSERT_EQ(remoteEnv[envVar], prefixMappedEnv[envVar]);
    }

    // Do another test with edge cases like set but empty env variables,
    // trailing ':', and multiple replacements in the same variable
    testEnv = {{"PATH", "/some/other/path:/my/example/path:/bin:"},
               {"LD_LIBRARY_PATH", ""}};
    prefixMappedEnv = {{"PATH", "/path2:/rewritten/path:/bin:"},
                       {"LD_LIBRARY_PATH", ""}};
    for (const auto envPair : testEnv) {
        setenv(envPair.first.c_str(), envPair.second.c_str(), 1);
    }

    remoteEnv = ActionBuilder::prepareRemoteEnv(command);
    for (const auto &envPair : testEnv) {
        const std::string &envVar = envPair.first;
        ASSERT_TRUE(remoteEnv.count(envVar));
        ASSERT_EQ(remoteEnv[envVar], prefixMappedEnv[envVar]);
    }

    // Restore the environment back to the way it was
    for (const auto &envPair : oldEnv) {
        setenv(envPair.first.c_str(), envPair.second.c_str(), 1);
    }
    RECC_PREFIX_REPLACEMENT = oldPrefixMap;
}

TEST_F(ActionBuilderTestFixture, PlatformProperties)
{
    std::map<std::string, std::string> platform_properties;
    platform_properties["OS"] = "linux";
    platform_properties["ISA"] = "x86";

    const proto::Command command_proto =
        this->generateCommandProto({}, {}, {}, {}, platform_properties, "");

    ASSERT_EQ(command_proto.platform().properties_size(), 2);
    for (const auto &property : command_proto.platform().properties()) {
        ASSERT_TRUE(platform_properties.count(property.name()));
        ASSERT_EQ(property.value(), platform_properties.at(property.name()));
    }
}

TEST_F(ActionBuilderTestFixture, WorkingDirectory)
{
    // Working directories are not manipulated:
    const auto paths = {"..", "/home/user/dev", "./subdir1/subdir2"};
    for (const auto &working_directory : paths) {
        const proto::Command command_proto =
            this->generateCommandProto({}, {}, {}, {}, {}, working_directory);
        ASSERT_EQ(working_directory, command_proto.working_directory());
    }
}

TEST_F(ActionBuilderTestFixture, PathsArePrefixed)
{
    RECC_PREFIX_REPLACEMENT = {{"/usr/bin", "/opt"}};
    const auto working_directory = "/usr/bin";

    const proto::Command command_proto =
        this->generateCommandProto({}, {}, {}, {}, {}, working_directory);

    ASSERT_EQ(command_proto.working_directory(), "/opt");
}

TEST_F(ActionBuilderTestFixture, CommonAncestorPath1)
{
    const DependencyPairs dependencies = {std::make_pair("dep.c", "dep.c")};
    const std::set<std::string> output_paths = {"../build/"};
    const std::string working_directory = "/tmp/workspace/";

    const auto commonAncestor =
        commonAncestorPath(dependencies, output_paths, working_directory);
    ASSERT_EQ(commonAncestor, "workspace");
}

TEST_F(ActionBuilderTestFixture, CommonAncestorPath2)
{
    const DependencyPairs dependencies = {std::make_pair("dep.c", "dep.c")};
    const std::set<std::string> output_paths = {"../../build/"};
    const std::string working_directory = "/tmp/workspace/";

    const auto commonAncestor =
        commonAncestorPath(dependencies, output_paths, working_directory);
    ASSERT_EQ(commonAncestor, "tmp/workspace");
}

TEST_F(ActionBuilderTestFixture, CommonAncestorPath3)
{
    const DependencyPairs dependencies = {
        std::make_pair("src/dep.c", "src/dep.c")};
    const std::set<std::string> output_paths = {"build/out/"};
    const std::string working_directory = "/tmp/workspace/";

    const auto commonAncestor =
        commonAncestorPath(dependencies, output_paths, working_directory);
    ASSERT_EQ(commonAncestor, "");
}

TEST_F(ActionBuilderTestFixture, WorkingDirectoryPrefixEmptyPrefix)
{
    for (const auto &path : {"dir/", "/tmp/subdir"}) {
        ASSERT_EQ(path, prefixWorkingDirectory(path, ""));
    }
}

TEST_F(ActionBuilderTestFixture, WorkingDirectoryPrefix)
{
    const auto prefix = "/home/user/dev";

    ASSERT_EQ(prefixWorkingDirectory("dir/", prefix), "/home/user/dev/dir/");
    ASSERT_EQ(prefixWorkingDirectory("tmp/subdir", prefix),
              "/home/user/dev/tmp/subdir");
}

TEST_F(ActionBuilderTestFixture, ActionSalt)
{
    const std::vector<std::string> recc_args = {"./gcc", "-c", "hello.cpp",
                                                "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());

    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);
    const auto actionDigest = DigestGenerator::make_digest(*actionPtr);

    blobs.clear();
    digest_to_filepaths.clear();

    RECC_ACTION_SALT = "salt";

    const auto saltedActionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(saltedActionPtr, nullptr);
    const auto saltedActionDigest =
        DigestGenerator::make_digest(*saltedActionPtr);

    /* Salt must affect action digest */
    EXPECT_NE(actionDigest, saltedActionDigest);
}

TEST_P(ActionBuilderTestFixture, ActionContainsExpectedCompileCommand)
{
    const std::string working_dir_prefix = GetParam();
    RECC_WORKING_DIR_PREFIX = working_dir_prefix;

    const std::vector<std::string> recc_args = {"./gcc", "-c", "hello.cpp",
                                                "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());

    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);

    ASSERT_NE(actionPtr, nullptr);

    proto::Command expected_command;
    for (const auto &arg : recc_args) {
        expected_command.add_arguments(arg);
    }
    expected_command.add_output_paths("hello.o");
    expected_command.set_working_directory(working_dir_prefix);

    // PATH is preserved by default
    auto env = expected_command.add_environment_variables();
    env->set_name("PATH");
    env->set_value(getenv("PATH"));

    ASSERT_EQ(actionPtr->command_digest(),
              DigestGenerator::make_digest(expected_command));
}

TEST_P(ActionBuilderTestFixture, ActionCompileCommandGoldenDigests)
{
    RECC_REAPI_VERSION = "2.2";
    const std::string working_dir_prefix = GetParam();
    RECC_WORKING_DIR_PREFIX = working_dir_prefix;

    // Don't let host environment affect command proto
    unsetenv("PATH");

    const std::vector<std::string> recc_args = {"./gcc", "-c", "hello.cpp",
                                                "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());

    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);

    /** Modifying these hash values should be done carefully and only if
     * absolutely required.
     */
    if (working_dir_prefix.empty()) {
        EXPECT_EQ(
            "a20cd0b097bcf6bc5c4d1fb5c040ac76017b55029d4ea65f6e4a0c689286f8ae",
            actionPtr->command_digest().hash());
    }
    else {
        EXPECT_EQ(
            "ecaaa7b62e0d78ec65ba503a24eda212c9a48a618e1ec608dc48a69e79f92fb7",
            actionPtr->command_digest().hash());
    }
}

TEST_P(ActionBuilderTestFixture, ActionNonCompileCommandGoldenDigests)
{
    /* [!] WARNING [!]
     * Changes that make this test fail will produce caches misses.
     */

    const std::string working_dir_prefix = GetParam();
    if (!working_dir_prefix.empty()) {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
    }

    // Don't let host environment affect command proto
    unsetenv("PATH");

    RECC_FORCE_REMOTE = true;

    const std::vector<std::string> recc_args = {"/bin/ls"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);

    /** Modifying these hash values should be done carefully and only if
     * absolutely required.
     */
    if (working_dir_prefix.empty()) {
        EXPECT_EQ(
            "87c8a7778c3fc31f5b15bbe34d5f49e0d6bb1dbcc00551e8f14927bf23fe6536",
            actionPtr->command_digest().hash());
    }
    else {
        EXPECT_EQ(
            "1a54c359f6bcdcc7f70fd201645bd829c8c689c77af5de5d0104d31490563dad",
            actionPtr->command_digest().hash());
    }
}

// This is a representation of a flat merkle tree, where things are stored in
// the required order of traversal.
typedef std::vector<std::unordered_map<std::string, std::vector<std::string>>>
    MerkleTree;

// Recursively verify that a merkle tree matches an expected input layout.
// This doesn't look at the hashes, just that the declared layout matches
void verify_merkle_tree(const proto::Digest &digest,
                        const MerkleTree &expected, size_t &index, size_t end,
                        const buildboxcommon::digest_string_map &blobs)
{
    ASSERT_LE(index, end) << "Reached end of expected output early";
    auto currentLevel = expected[index];
    const auto current_blob = blobs.find(digest);
    ASSERT_NE(current_blob, blobs.end())
        << "No blob found for digest " << digest.hash();

    proto::Directory directory;
    ASSERT_TRUE(directory.ParseFromString(current_blob->second));

    // Exit early if there are more/less files or dirs in the given tree than
    // expected
    ASSERT_EQ(directory.files().size(), currentLevel["files"].size())
        << "Wrong number of files at current level";
    ASSERT_EQ(directory.directories().size(),
              currentLevel["directories"].size())
        << "Wrong number of directories at current level";

    size_t f_index = 0;
    for (const auto &file : directory.files()) {
        ASSERT_EQ(file.name(), currentLevel["files"][f_index])
            << "Wrong file found";
        f_index++;
    }

    size_t d_index = 0;
    for (const auto &subdirectory : directory.directories()) {
        ASSERT_EQ(subdirectory.name(), currentLevel["directories"][d_index])
            << "Wrong directory found";
        d_index++;
    }

    // All the files/directories at this level are correct, now check all the
    // subdirectories
    for (const auto &subdirectory : directory.directories()) {
        verify_merkle_tree(subdirectory.digest(), expected, ++index, end,
                           blobs);
    }
}

/**
 * Verify that the generated Action has the correct working directory
 */
void verify_working_directory(const proto::Digest &digest,
                              const std::string &expected_workdir,
                              const buildboxcommon::digest_string_map &blobs)
{
    proto::Command command_proto;
    const auto command_blob = blobs.find(digest);
    ASSERT_TRUE(command_proto.ParseFromString(command_blob->second));
    EXPECT_EQ(command_proto.working_directory(), expected_workdir);
}

/**
 * Builds an action for test/data/actionbuilder/hello.cpp and checks against
 * precomputed digest values, as well as checks the structure of the input_root
 */
TEST_P(ActionBuilderTestFixture, ActionContainsExpectedMerkleTree)
{
    const std::string working_dir_prefix = GetParam();

    if (!working_dir_prefix.empty()) {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
    }
    RECC_DEPS_OVERRIDE = {"hello.cpp"};
    const std::vector<std::string> recc_args = {"/my/fake/gcc", "-c",
                                                "hello.cpp", "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());

    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);

    ASSERT_NE(actionPtr, nullptr);

    MerkleTree expected_tree;
    if (working_dir_prefix.empty()) {
        EXPECT_EQ(
            "05a73e8a93752c197cb58e4bdb43bd1d696fa02ecff899a339c4ecacbab2c14b",
            actionPtr->input_root_digest().hash());
        // The expected result is a single file at the root
        expected_tree = {{{"files", {"hello.cpp"}}}};
    }
    else {
        EXPECT_EQ(
            "edd3e49326719324c5af290089e30ec58f11dd9eb2feb029930c95fb8ca32eea",
            actionPtr->input_root_digest().hash());

        // The expected result is a single file, under the working_dir_prefix
        // directory
        expected_tree = {{{"directories", {working_dir_prefix}}},
                         {{"files", {"hello.cpp"}}}};
    }
    // Check the layout of the input_root is what we expect
    size_t startIndex = 0;
    verify_merkle_tree(actionPtr->input_root_digest(), expected_tree,
                       startIndex, expected_tree.size(), blobs);
}

/**
 * Verify that intermediate directories are added to the merkle tree if the
 * path of a dependency contains `..` segments before normalization.
 */
TEST_P(ActionBuilderTestFixture, ActionContainsExpectedMerkleTreeDotDot)
{
    const std::string working_dir_prefix = GetParam();

    if (!working_dir_prefix.empty()) {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
    }
    RECC_DEPS_OVERRIDE = {"foo/../hello.cpp", "foo/../bar/../hello.cpp"};
    const std::vector<std::string> recc_args = {
        "/my/fake/gcc", "-c", "foo/../hello.cpp", "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());

    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);

    ASSERT_NE(actionPtr, nullptr);

    MerkleTree expected_tree;
    if (working_dir_prefix.empty()) {
        // The expected result is a single file at the root and empty `foo` and
        // `bar` directories
        expected_tree = {
            {{"directories", {"bar", "foo"}}, {"files", {"hello.cpp"}}},
            {},
            {}};
    }
    else {
        // The expected result is a single file and empty `foo` and `bar`
        // directories, under the working_dir_prefix directory
        expected_tree = {
            {{"directories", {working_dir_prefix}}},
            {{"directories", {"bar", "foo"}}, {"files", {"hello.cpp"}}},
            {},
            {}};
    }
    // Check the layout of the input_root is what we expect
    size_t startIndex = 0;
    verify_merkle_tree(actionPtr->input_root_digest(), expected_tree,
                       startIndex, expected_tree.size(), blobs);
}

TEST_P(ActionBuilderTestFixture, ActionBuiltGoldenDigests)
{
    const std::string working_dir_prefix = GetParam();

    if (!working_dir_prefix.empty()) {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
    }

    // Don't let host environment affect command proto
    unsetenv("PATH");

    const std::vector<std::string> recc_args = {"./gcc", "-c", "hello.cpp",
                                                "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());

    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);

    ASSERT_NE(actionPtr, nullptr);

    const auto actionDigest = DigestGenerator::make_digest(*actionPtr);
    if (working_dir_prefix.empty()) {
        EXPECT_EQ(142, actionDigest.size_bytes());
        EXPECT_EQ(
            "415cef529a7b9aa58bcbac3fc11bcbc60fca53b1e57d7cf8ff054c46f80e1866",
            actionDigest.hash());
    }
    else {
        EXPECT_EQ(142, actionDigest.size_bytes());
        EXPECT_EQ(
            "450df928c5e6afa2591a074fce25648d16c8789000d5b2f66d72b580f7b00985",
            actionDigest.hash());
    }
}

/**
 * Non-compile commands return nullptrs, indicating that they are intended to
 * be run locally if RECC_FORCE_REMOTE isn't set
 */
TEST_P(ActionBuilderTestFixture, NonCompileCommand)
{
    const std::string working_dir_prefix = GetParam();
    if (!working_dir_prefix.empty()) {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
    }

    RECC_FORCE_REMOTE = false;

    const std::vector<std::string> recc_args = {"ls"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);

    ASSERT_EQ(actionPtr, nullptr);
}

/**
 * However, force remoting a non-compile command still generates an action
 */
TEST_P(ActionBuilderTestFixture, NonCompileCommandForceRemoteMerkleTree)
{
    const std::string working_dir_prefix = GetParam();
    if (!working_dir_prefix.empty()) {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
    }

    RECC_FORCE_REMOTE = true;

    const std::vector<std::string> recc_args = {"/bin/ls"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);

    MerkleTree expected_tree;
    std::string expected_working_dir;
    if (working_dir_prefix.empty()) {
        EXPECT_EQ(
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            actionPtr->input_root_digest().hash());
        // No input files, so the tree/working-dir is empty
        expected_tree = {{{}}};
        expected_working_dir = "";
    }
    else {
        EXPECT_EQ(
            "eadd312151cd61b24291b3f0d4ec461476354e8147379246c7024903cf17cdbb",
            actionPtr->input_root_digest().hash());

        // Expect a single top level directory equal to RECC_WORKING_DIR_PREFIX
        expected_tree = {{{"directories", {working_dir_prefix}}},
                         {{"files", {}}}};
        expected_working_dir = working_dir_prefix;
    }

    // Verify the command working directory matches
    verify_working_directory(actionPtr->command_digest(), expected_working_dir,
                             blobs);
    // Check the layout of the input_root is what we expect
    size_t startIndex = 0;
    verify_merkle_tree(actionPtr->input_root_digest(), expected_tree,
                       startIndex, expected_tree.size(), blobs);
}

TEST_P(ActionBuilderTestFixture, NonCompileCommandForceRemoteGoldenDigests)
{
    const std::string working_dir_prefix = GetParam();
    if (!working_dir_prefix.empty()) {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
    }

    // Don't let host environment affect command proto
    unsetenv("PATH");

    RECC_FORCE_REMOTE = true;

    const std::vector<std::string> recc_args = {"/bin/ls"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);

    const auto digest = DigestGenerator::make_digest(*actionPtr);
    if (working_dir_prefix.empty()) {
        EXPECT_EQ(140, digest.size_bytes());
        EXPECT_EQ(
            "c718489624f4078a96261090329a864c20add37856953fdaa1a200500d9ebf9d",
            digest.hash());
    }
    else {
        EXPECT_EQ(142, digest.size_bytes());
        EXPECT_EQ(
            "f19a533d8d743f5c8b317e8074e75c0affee9d3d095b307e2ff50b8d44f07f58",
            digest.hash());
    }
}

/**
 * Make sure an absolute file dependencies are represented properly. Do this by
 * using RECC_DEPS_OVERRIDE to return a deterministic set of dependencies.
 * Otherwise things are platform dependant
 */
TEST_P(ActionBuilderTestFixture, AbsolutePathActionBuilt)
{
    const std::string working_dir_prefix = GetParam();

    MerkleTree expected_tree;
    std::string expected_working_dir;
    if (working_dir_prefix.empty()) {
        expected_tree = {{{"files", {"hello.cpp"}}, {"directories", {"usr"}}},
                         {{"directories", {"include"}}},
                         {{"files", {"ctype.h"}}}};
        expected_working_dir = "";
    }
    else {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
        expected_tree = {{{"directories", {working_dir_prefix, "usr"}}},
                         {{"files", {"hello.cpp"}}},
                         {{"directories", {"include"}}},
                         {{"files", {"ctype.h"}}}};
        expected_working_dir = working_dir_prefix;
    }
    RECC_DEPS_OVERRIDE = {"/usr/include/ctype.h", "hello.cpp"};
    RECC_DEPS_GLOBAL_PATHS = 1;

    const std::vector<std::string> recc_args = {"/my/fake/gcc", "-c",
                                                "hello.cpp", "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);

    // Verify the command working directory matches
    verify_working_directory(actionPtr->command_digest(), expected_working_dir,
                             blobs);

    // Since this test unfortunately relies on the hash of a system installed
    // file, it can't compare the input root digest. Instead go through the
    // merkle tree and verify each level has the correct layout.

    const auto current_digest = actionPtr->input_root_digest();
    size_t startIndex = 0;
    verify_merkle_tree(current_digest, expected_tree, startIndex,
                       expected_tree.size(), blobs);
}

/**
 * Make sure that absolute path dependencies are filtered out if
 * RECC_DEPS_GLOBAL_PATHS is not specified.
 */
TEST_P(ActionBuilderTestFixture, RelativePathActionBuilt)
{
    const std::string working_dir_prefix = GetParam();

    MerkleTree expected_tree;
    std::string expected_working_dir;
    if (working_dir_prefix.empty()) {
        expected_tree = {{{"files", {"hello.cpp"}}}};
        expected_working_dir = "";
    }
    else {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
        expected_tree = {{{"directories", {working_dir_prefix}}},
                         {{"files", {"hello.cpp"}}}};
        expected_working_dir = working_dir_prefix;
    }
    RECC_DEPS_OVERRIDE = {"/usr/include/ctype.h", "hello.cpp"};

    const std::vector<std::string> recc_args = {"/my/fake/gcc", "-c",
                                                "hello.cpp", "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);

    // Verify the command working directory matches
    verify_working_directory(actionPtr->command_digest(), expected_working_dir,
                             blobs);

    // Since this test unfortunately relies on the hash of a system installed
    // file, it can't compare the input root digest. Instead go through the
    // merkle tree and verify each level has the correct layout.

    const auto current_digest = actionPtr->input_root_digest();
    size_t startIndex = 0;
    verify_merkle_tree(current_digest, expected_tree, startIndex,
                       expected_tree.size(), blobs);
}

/**
 * Test to make sure that paths with .. are properly resolved using
 * information from the current directory, and that it doesn't cause
 * issues with absolute paths
 */
TEST_P(ActionBuilderTestFixture, RelativePathAndAbsolutePathWithCwd)
{
    const std::string working_dir_prefix = GetParam();

    MerkleTree expected_tree;
    std::string expected_working_dir;
    if (working_dir_prefix.empty()) {
        expected_tree = {{{"directories", {"actionbuilder", "deps", "usr"}}},
                         {{"files", {"hello.cpp"}}},
                         {{"files", {"empty.c"}}},
                         {{"directories", {"include"}}},
                         {{"files", {"ctype.h"}}}};
        expected_working_dir = "actionbuilder";
    }
    else {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
        expected_tree = {{{"directories", {working_dir_prefix, "usr"}}},
                         {{"directories", {"actionbuilder", "deps"}}},
                         {{"files", {"hello.cpp"}}},
                         {{"files", {"empty.c"}}},
                         {{"directories", {"include"}}},
                         {{"files", {"ctype.h"}}}};
        expected_working_dir = working_dir_prefix + "/actionbuilder";
    }
    cwd = FileUtils::getCurrentWorkingDirectory();
    RECC_DEPS_OVERRIDE = {"/usr/include/ctype.h", "../deps/empty.c",
                          "hello.cpp"};
    RECC_DEPS_GLOBAL_PATHS = 1;

    const std::vector<std::string> recc_args = {"/my/fake/gcc", "-c",
                                                "hello.cpp", "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);

    // Verify the command working directory matches
    verify_working_directory(actionPtr->command_digest(), expected_working_dir,
                             blobs);

    const auto current_digest = actionPtr->input_root_digest();
    size_t startIndex = 0;
    verify_merkle_tree(current_digest, expected_tree, startIndex,
                       expected_tree.size(), blobs);
}

TEST_P(ActionBuilderTestFixture, ExcludePath)
{
    const std::string working_dir_prefix = GetParam();

    MerkleTree expected_tree;
    std::string expected_working_dir;
    if (working_dir_prefix.empty()) {
        expected_tree = {{{"directories", {"actionbuilder", "deps"}}},
                         {{"files", {"hello.cpp"}}},
                         {{"files", {"empty.c"}}}};
        expected_working_dir = "actionbuilder";
    }
    else {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
        expected_tree = {{{"directories", {working_dir_prefix}}},
                         {{"directories", {"actionbuilder", "deps"}}},
                         {{"files", {"hello.cpp"}}},
                         {{"files", {"empty.c"}}}};
        expected_working_dir = working_dir_prefix + "/actionbuilder";
    }
    cwd = FileUtils::getCurrentWorkingDirectory();
    RECC_DEPS_OVERRIDE = {"/usr/include/ctype.h", "../deps/empty.c",
                          "hello.cpp"};
    RECC_DEPS_GLOBAL_PATHS = 1;
    RECC_DEPS_EXCLUDE_PATHS = {"/usr/include"};

    const std::vector<std::string> recc_args = {"/my/fake/gcc", "-c",
                                                "hello.cpp", "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);

    // Verify the command working directory matches
    verify_working_directory(actionPtr->command_digest(), expected_working_dir,
                             blobs);

    const auto current_digest = actionPtr->input_root_digest();
    size_t startIndex = 0;
    verify_merkle_tree(current_digest, expected_tree, startIndex,
                       expected_tree.size(), blobs);
}

TEST_P(ActionBuilderTestFixture, ExcludePathMultipleMatchOne)
{
    const std::string working_dir_prefix = GetParam();

    MerkleTree expected_tree;
    std::string expected_working_dir;
    if (working_dir_prefix.empty()) {
        expected_tree = {{{"directories", {"actionbuilder", "deps"}}},
                         {{"files", {"hello.cpp"}}},
                         {{"files", {"empty.c"}}}};
        expected_working_dir = "actionbuilder";
    }
    else {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
        expected_tree = {{{"directories", {working_dir_prefix}}},
                         {{"directories", {"actionbuilder", "deps"}}},
                         {{"files", {"hello.cpp"}}},
                         {{"files", {"empty.c"}}}};
        expected_working_dir = working_dir_prefix + "/actionbuilder";
    }

    cwd = FileUtils::getCurrentWorkingDirectory();
    RECC_DEPS_OVERRIDE = {"/usr/include/ctype.h", "../deps/empty.c",
                          "hello.cpp"};
    RECC_DEPS_GLOBAL_PATHS = 1;
    RECC_DEPS_EXCLUDE_PATHS = {"/foo/bar", "/usr/include"};

    std::vector<std::string> recc_args = {"/my/fake/gcc", "-c", "hello.cpp",
                                          "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);

    // Verify the command working directory matches
    verify_working_directory(actionPtr->command_digest(), expected_working_dir,
                             blobs);

    const auto current_digest = actionPtr->input_root_digest();
    size_t startIndex = 0;
    verify_merkle_tree(current_digest, expected_tree, startIndex,
                       expected_tree.size(), blobs);
}

TEST_P(ActionBuilderTestFixture, ExcludePathNoMatch)
{
    const std::string working_dir_prefix = GetParam();

    MerkleTree expected_tree;
    std::string expected_working_dir;
    if (working_dir_prefix.empty()) {
        expected_tree = {{{"directories", {"actionbuilder", "deps", "usr"}}},
                         {{"files", {"hello.cpp"}}},
                         {{"files", {"empty.c"}}},
                         {{"directories", {"include"}}},
                         {{"files", {"ctype.h"}}}};
        expected_working_dir = "actionbuilder";
    }
    else {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
        expected_tree = {{{"directories", {working_dir_prefix, "usr"}}},
                         {{"directories", {"actionbuilder", "deps"}}},
                         {{"files", {"hello.cpp"}}},
                         {{"files", {"empty.c"}}},
                         {{"directories", {"include"}}},
                         {{"files", {"ctype.h"}}}};
        expected_working_dir = working_dir_prefix + "/actionbuilder";
    }
    cwd = FileUtils::getCurrentWorkingDirectory();
    RECC_DEPS_OVERRIDE = {"/usr/include/ctype.h", "../deps/empty.c",
                          "hello.cpp"};
    RECC_DEPS_GLOBAL_PATHS = 1;
    RECC_DEPS_EXCLUDE_PATHS = {"/foo/bar"};

    const std::vector<std::string> recc_args = {"/my/fake/gcc", "-c",
                                                "hello.cpp", "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);

    // Verify the command working directory matches
    verify_working_directory(actionPtr->command_digest(), expected_working_dir,
                             blobs);

    const auto current_digest = actionPtr->input_root_digest();
    size_t startIndex = 0;
    verify_merkle_tree(current_digest, expected_tree, startIndex,
                       expected_tree.size(), blobs);
}

TEST_P(ActionBuilderTestFixture, ExcludePathSingleWithMultiInput)
{
    const std::string working_dir_prefix = GetParam();

    MerkleTree expected_tree;
    std::string expected_working_dir;
    if (working_dir_prefix.empty()) {
        expected_tree = {{{"directories", {"actionbuilder", "deps", "usr"}}},
                         {{"files", {"hello.cpp"}}},
                         {{"files", {"empty.c"}}},
                         {{"directories", {"include"}}},
                         {{"files", {"ctype.h"}}}};
        expected_working_dir = "actionbuilder";
    }
    else {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
        expected_tree = {{{"directories", {working_dir_prefix, "usr"}}},
                         {{"directories", {"actionbuilder", "deps"}}},
                         {{"files", {"hello.cpp"}}},
                         {{"files", {"empty.c"}}},
                         {{"directories", {"include"}}},
                         {{"files", {"ctype.h"}}}};
        expected_working_dir = working_dir_prefix + "/actionbuilder";
    }

    cwd = FileUtils::getCurrentWorkingDirectory();
    RECC_DEPS_OVERRIDE = {"/usr/include/ctype.h",
                          "/usr/include/net/ethernet.h", "../deps/empty.c",
                          "hello.cpp"};
    RECC_DEPS_GLOBAL_PATHS = 1;
    RECC_DEPS_EXCLUDE_PATHS = {"/usr/include/net"};

    const std::vector<std::string> recc_args = {"/my/fake/gcc", "-c",
                                                "hello.cpp", "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);

    // Verify the command working directory matches
    verify_working_directory(actionPtr->command_digest(), expected_working_dir,
                             blobs);

    const auto current_digest = actionPtr->input_root_digest();
    size_t startIndex = 0;
    verify_merkle_tree(current_digest, expected_tree, startIndex,
                       expected_tree.size(), blobs);
}

/**
 * Make sure that the working directory of the action exists in the
 * input root, regardless if there are any files in it.
 */
TEST_P(ActionBuilderTestFixture, EmptyWorkingDirInputRoot)
{
    std::string working_dir_prefix = GetParam();
    MerkleTree expected_tree;
    std::string expected_working_dir;
    if (working_dir_prefix.empty()) {
        expected_tree = {{{"directories", {"actionbuilder", "deps"}}},
                         {{"files", {}}},
                         {{"files", {"empty.c"}}}};
        expected_working_dir = "actionbuilder";
    }
    else {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
        expected_tree = {{{"directories", {working_dir_prefix}}},
                         {{"directories", {"actionbuilder", "deps"}}},
                         {{"files", {}}},
                         {{"files", {"empty.c"}}}};
        expected_working_dir = working_dir_prefix + "/actionbuilder";
    }
    cwd = FileUtils::getCurrentWorkingDirectory();
    RECC_DEPS_OVERRIDE = {"../deps/empty.c"};

    const std::vector<std::string> recc_args = {
        "/my/fake/gcc", "-c", "../deps/empty.c", "-o", "empty.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);

    // Verify the command working directory matches
    verify_working_directory(actionPtr->command_digest(), expected_working_dir,
                             blobs);

    const auto current_digest = actionPtr->input_root_digest();

    size_t startIndex = 0;
    verify_merkle_tree(current_digest, expected_tree, startIndex,
                       expected_tree.size(), blobs);
}

/**
 * Trying to output to an unrelated directory returns a nullptr, forcing a
 * local execution.
 */
TEST_P(ActionBuilderTestFixture, UnrelatedOutputDirectory)
{
    const std::string working_dir_prefix = GetParam();
    if (!working_dir_prefix.empty()) {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
    }
    const std::vector<std::string> recc_args = {"./gcc", "-c", "hello.cpp",
                                                "-o", "/fakedirname/hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);

    ASSERT_EQ(actionPtr, nullptr);
}

/**
 * Force path replacement, and make sure paths are correctly replaced in the
 * resulting action.
 */
TEST_P(ActionBuilderTestFixture, SimplePathReplacement)
{
    const std::string working_dir_prefix = GetParam();

    MerkleTree expected_tree;
    std::string expected_working_dir;
    if (working_dir_prefix.empty()) {
        expected_tree = {{{"files", {"hello.cpp"}}, {"directories", {"usr"}}},
                         {{"files", {"ctype.h"}}}};
        expected_working_dir = "";
    }
    else {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
        expected_tree = {{{"directories", {working_dir_prefix, "usr"}}},
                         {{"files", {"hello.cpp"}}},
                         {{"files", {"ctype.h"}}}};
        expected_working_dir = working_dir_prefix;
    }

    // Replace all paths to /usr/include to /usr
    RECC_PREFIX_REPLACEMENT = {{"/usr/include", "/usr"}};

    RECC_DEPS_OVERRIDE = {"/usr/include/ctype.h", "hello.cpp"};
    RECC_DEPS_GLOBAL_PATHS = 1;

    const std::vector<std::string> recc_args = {"/my/fake/gcc", "-c",
                                                "hello.cpp", "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);

    // Verify the command working directory matches
    verify_working_directory(actionPtr->command_digest(), expected_working_dir,
                             blobs);

    const auto current_digest = actionPtr->input_root_digest();
    size_t startIndex = 0;
    verify_merkle_tree(current_digest, expected_tree, startIndex,
                       expected_tree.size(), blobs);
}

/**
 * Test that absolute paths are made relative to RECC_PROJECT_ROOT
 * and that if a working_dir_prefix is specified that it is prepended
 */
TEST_P(ActionBuilderTestFixture, PathsMadeRelativeToProjectRoot)
{
    const std::string working_dir_prefix = GetParam();

    MerkleTree expected_tree;
    std::string expected_working_dir;
    if (working_dir_prefix.empty()) {
        expected_tree = {{{"files", {"hello.cpp"}}}};
        expected_working_dir = "";
    }
    else {
        RECC_WORKING_DIR_PREFIX = working_dir_prefix;
        expected_tree = {{{"directories", {working_dir_prefix}}},
                         {{"files", {"hello.cpp"}}}};
        expected_working_dir = working_dir_prefix;
    }

    RECC_PROJECT_ROOT = cwd;
    RECC_DEPS_OVERRIDE = {cwd + "/hello.cpp"};
    RECC_DEPS_GLOBAL_PATHS = 1;

    const std::vector<std::string> recc_args = {"/my/fake/gcc", "-c",
                                                "hello.cpp", "-o", "hello.o"};
    const auto command =
        ParsedCommandFactory::createParsedCommand(recc_args, cwd.c_str());
    const auto actionPtr =
        ActionBuilder::BuildAction(command, cwd, &blobs, &digest_to_filepaths);
    ASSERT_NE(actionPtr, nullptr);

    // Verify the command working directory matches
    verify_working_directory(actionPtr->command_digest(), expected_working_dir,
                             blobs);

    const auto current_digest = actionPtr->input_root_digest();
    size_t startIndex = 0;
    verify_merkle_tree(current_digest, expected_tree, startIndex,
                       expected_tree.size(), blobs);
}

// Run each test twice, once with no working_dir_prefix set, and one with
// it set to "recc-build"
INSTANTIATE_TEST_CASE_P(ActionBuilder, ActionBuilderTestFixture,
                        testing::Values("", "recc-build"));
