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

#include <algorithm>
#include <env.h>
#include <gtest/gtest.h>
#include <map>
#include <vector>

using namespace recc;

const int MAX = 120;

// parameter is the environment variable, i.e RECC_REMOTE_ENV
// parameterStr is name of environment variable, i.e "RECC_REMOTE_ENV"
void permute_test(const std::vector<const char *> expectedOrderVec,
                  const std::map<std::string, std::string> parameter,
                  const std::string &parameterStr)
{
    int counter = 0;
    std::vector<const char *> permuteVec = expectedOrderVec;
    do {
        ++counter;
        // parse_config_variables expects nullptr at end
        // sets environ variable, an array of std::strings terminated by
        // nullptr
        permuteVec.push_back(nullptr);

        Env::parse_config_variables(permuteVec.data());
        // remove nullptr
        permuteVec.pop_back();

        int v_In = 0;

        // ensure ordering is the same as expected Vector
        for (const auto &platformIter : parameter) {
            std::string map_val =
                parameterStr + platformIter.first + "=" + platformIter.second;
            EXPECT_EQ(map_val, std::string(expectedOrderVec[v_In]));
            ++v_In;
        }
    } while (counter < MAX &&
             next_permutation(permuteVec.begin(), permuteVec.end()));
}

TEST(EnvTest, PlatformEnvCheckOrder)
{
    const std::vector<const char *> expectedOrderVec = {
        "RECC_REMOTE_PLATFORM_arch=x86_64", "RECC_REMOTE_PLATFORM_test=x64_86",
        "RECC_REMOTE_PLATFORM_zed=win10"};
    permute_test(expectedOrderVec, RECC_REMOTE_PLATFORM,
                 "RECC_REMOTE_PLATFORM");
}

TEST(EnvTest, RemoteEnvCheckOrder)
{
    const std::vector<const char *> expectedOrderVec = {
        "RECC_REMOTE_ENV_PATH=/usr/bin", "RECC_REMOTE_ENV_PWD=/usr",
        "RECC_REMOTE_ENV_USER=rkennedy76"};
    permute_test(expectedOrderVec, RECC_REMOTE_ENV, "RECC_REMOTE_ENV");
}

TEST(EnvTest, DepsEnvCheckOrder)
{
    const std::vector<const char *> expectedOrderVec = {
        "RECC_DEPS_ENV_OSTYPE=linux-gnu", "RECC_DEPS_ENV_SHELL=/bin/bash",
        "RECC_DEPS_ENV_USER=rkennedy76"};
    permute_test(expectedOrderVec, RECC_DEPS_ENV, "RECC_DEPS_ENV");
}
