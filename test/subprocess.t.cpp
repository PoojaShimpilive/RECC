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

#include <subprocess.h>

#include <fstream>

#include <buildboxcommon_temporarydirectory.h>

#include <gtest/gtest.h>

using namespace recc;

TEST(SubprocessTest, True)
{
    std::vector<std::string> command = {"true"};
    auto result = Subprocess::execute(command);
    EXPECT_EQ(result.d_exitCode, 0);
}

TEST(SubprocessTest, False)
{
    std::vector<std::string> command = {"false"};
    auto result = Subprocess::execute(command);
    EXPECT_NE(result.d_exitCode, 0);
}

TEST(SubprocessTest, CommandNotFound)
{
    std::vector<std::string> command = {"this-command-does-not-exist-1234"};
    auto result = Subprocess::execute(command);
    EXPECT_EQ(result.d_exitCode, 127); // "command not found" error
}

TEST(SubprocessTest, CommandIsNotAnExecutable)
{
    // Creating an empty file, which will fail when trying to execute it:
    buildboxcommon::TemporaryDirectory temp_dir;
    const std::string file_path = std::string(temp_dir.name()) + "/file.txt";
    std::ofstream file(file_path);
    file.close();

    std::vector<std::string> command = {file_path};
    auto result = Subprocess::execute(command);
    EXPECT_EQ(result.d_exitCode, 126);
    // "Command invoked cannot execute" error
}

TEST(SubprocessTest, OutputPipes)
{
    std::vector<std::string> command = {"echo", "hello", "world"};
    auto result = Subprocess::execute(command, true, true);
    EXPECT_EQ(result.d_exitCode, 0);
    EXPECT_EQ(result.d_stdOut, "hello world\n");
    EXPECT_EQ(result.d_stdErr, "");
}

TEST(SubprocessTest, Environment)
{
    std::vector<std::string> command = {"env"};
    std::map<std::string, std::string> env = {
        {"RECC_SUBPROCESS_TEST_VAR", "value123456"}};
    auto result = Subprocess::execute(command, true, true, env);
    EXPECT_TRUE(result.d_stdOut.find("RECC_SUBPROCESS_TEST_VAR=value123456") !=
                std::string::npos);
    EXPECT_EQ(result.d_exitCode, 0);
}
