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

#include <env.h>
#include <gtest/gtest.h>
#include <iostream>
#include <reccdefaults.h>
#include <string>

using namespace recc;

/*
 * Test modifies RECC_CONFIG_LOCATIONS directly.
 *
 * WARNING: Test will fail if not invoked by "make test".
 */
TEST(EnvTest, EnvMultipleConfigs)
{
    unsetenv("RECC_SERVER");

    // this should be in {SOMEWHERE}/test/ if running 'make test'
    std::string cwd_recc = "./recc";
    std::string home = "./.recc";

    RECC_CONFIG_LOCATIONS.push_back(cwd_recc);

    Env::parse_config_variables();

    // should take value of file in /test/recc/recc.conf.
    EXPECT_EQ(RECC_SERVER, "http://localhost:99999");

    RECC_CONFIG_LOCATIONS.push_back(home);

    Env::parse_config_variables();

    // should take value of file in /test/.recc/recc.conf.
    EXPECT_EQ(RECC_SERVER, "http://localhost:10001");
}
