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

#include <deque>
#include <env.h>
#include <stdlib.h>
#include <string>

#include <gtest/gtest.h>

using namespace recc;

/* This test is here to make sure the order advertised in README.md and
 *  the comments of src/env.h remain accurate
 */
TEST(EnvTest, EnvConfigOrderAdvertised)
{
    // Make sure outside env doesn't get prioritized
    unsetenv("RECC_SERVER");
    unsetenv("RECC_CAS_SERVER");
    unsetenv("TMPDIR");
    unsetenv("HOME");

    // Set-up values to check against
    setenv("HOME", "/tmp/path/to/home", 0);
    RECC_CUSTOM_PREFIX = "/tmp/recc/custom/prefix";
    RECC_INSTALL_DIR = "/tmp/recc/install/dir";

    const std::deque<std::string> config_order =
        Env::evaluate_config_locations();
    /* config_order.size() must be at least 4, might be higher if more
     * locations are defined in reccdefaults.h
     */
    ASSERT_TRUE(config_order.size() >= 4);

    // Doing it in reverse just so that it's easier to compare against the
    // prioritized list
    std::deque<std::string>::const_reverse_iterator rit =
        config_order.rbegin();

    EXPECT_EQ("./recc", *rit);
    rit++;

    EXPECT_EQ("/tmp/path/to/home/.recc", *rit);
    rit++;

    EXPECT_EQ("/tmp/recc/custom/prefix", *rit);
    rit++;

    EXPECT_EQ("/tmp/recc/install/dir/../etc/recc", *rit);
    rit++;
}
