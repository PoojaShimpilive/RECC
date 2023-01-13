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

using namespace recc;

class EnvTest : public ::testing::Test {
  protected:
    void TearDown() override
    {
        RECC_SERVER = RECC_CAS_SERVER = RECC_ACTION_CACHE_SERVER = "";
        RECC_SERVER_SSL = false;
        RECC_FORCE_REMOTE = false;
        RECC_CACHE_ONLY = false;
        RECC_DEPS_OVERRIDE = RECC_OUTPUT_FILES_OVERRIDE = {};
        RECC_REMOTE_ENV.clear();
        RECC_DEPS_EXCLUDE_PATHS.clear();
    }
};

TEST_F(EnvTest, EnvSetTest)
{
    const char *testEnviron[] = {"RECC_SERVER=http://server:1234",
                                 "RECC_FORCE_REMOTE=1",
                                 "RECC_DEPS_OVERRIDE=oneitem",
                                 "RECC_OUTPUT_FILES_OVERRIDE=one,two,three",
                                 "RECC_REMOTE_ENV_key=val",
                                 "RECC_REMOTE_ENV_anotherkey=anotherval",
                                 "RECC_DEPS_EXCLUDE_PATHS=/usr/include,/opt/"
                                 "rh/devtoolset-7,/some/dir\\,withcomma",
                                 "TMPDIR=/some/tmp/dir",
                                 nullptr};
    const std::string expectedServer = "http://server:1234";
    const std::set<std::string> expectedDeps = {"oneitem"};
    const std::set<std::string> expectedOutputFiles = {"one", "two", "three"};
    const std::set<std::string> expectedExcludePaths = {
        "/usr/include", "/opt/rh/devtoolset-7", "/some/dir,withcomma"};
    const std::map<std::string, std::string> expectedRemoteEnv = {
        {"key", "val"}, {"anotherkey", "anotherval"}};
    Env::parse_config_variables(testEnviron);

    EXPECT_EQ(expectedServer, RECC_SERVER);
    EXPECT_TRUE(RECC_CAS_SERVER.empty());
    EXPECT_TRUE(RECC_ACTION_CACHE_SERVER.empty());

    Env::handle_special_defaults();
    // If the cas and action cache servers are not set then they
    // should fall back to RECC_SERVER.
    EXPECT_EQ(expectedServer, RECC_CAS_SERVER);
    EXPECT_EQ(expectedServer, RECC_ACTION_CACHE_SERVER);
    EXPECT_TRUE(RECC_FORCE_REMOTE);
    EXPECT_EQ(expectedDeps, RECC_DEPS_OVERRIDE);
    EXPECT_EQ(expectedOutputFiles, RECC_OUTPUT_FILES_OVERRIDE);
    EXPECT_EQ(expectedRemoteEnv, RECC_REMOTE_ENV);
    EXPECT_EQ("/some/tmp/dir", TMPDIR);

    EXPECT_EQ(expectedExcludePaths, RECC_DEPS_EXCLUDE_PATHS);
}

TEST_F(EnvTest, EnvSetTest2)
{
    const char *testEnviron[] = {"RECC_DEPS_EXCLUDE_PATHS=,", nullptr};
    const std::set<std::string> expectedExcludePaths = {""};
    Env::parse_config_variables(testEnviron);
    Env::handle_special_defaults();
    EXPECT_EQ(expectedExcludePaths, RECC_DEPS_EXCLUDE_PATHS);
}

TEST_F(EnvTest, EnvSetTestWithCAS)
{
    const char *testEnviron[] = {"RECC_SERVER=http://server:1234",
                                 "RECC_CAS_SERVER=http://casserver:123456",
                                 nullptr};
    const std::string expectedServer = "http://server:1234";
    const std::string expectedCasServer = "http://casserver:123456";
    Env::parse_config_variables(testEnviron);

    EXPECT_EQ(expectedServer, RECC_SERVER);
    EXPECT_EQ(expectedCasServer, RECC_CAS_SERVER);
    EXPECT_TRUE(RECC_ACTION_CACHE_SERVER.empty());

    Env::handle_special_defaults();
    // The CAS setting should be respected and the AC should use the CAS.
    EXPECT_EQ(expectedServer, RECC_SERVER);
    EXPECT_EQ(expectedCasServer, RECC_CAS_SERVER);
    EXPECT_EQ(expectedCasServer, RECC_ACTION_CACHE_SERVER);
}

TEST_F(EnvTest, EnvSetTestWithOnlyACCAS)
{
    const char *testEnviron[] = {
        "RECC_SERVER=http://server:1234",
        "RECC_ACTION_CACHE_SERVER=http://acserver:123456", nullptr};
    const std::string expectedServer = "http://server:1234";
    const std::string expectedAcServer = "http://acserver:123456";
    Env::parse_config_variables(testEnviron);

    EXPECT_EQ(expectedServer, RECC_SERVER);
    EXPECT_TRUE(RECC_CAS_SERVER.empty());
    EXPECT_EQ(expectedAcServer, RECC_ACTION_CACHE_SERVER);

    Env::handle_special_defaults();
    // If only AC and server are set then CAS should fallback to
    // where the AC is.
    EXPECT_EQ(expectedServer, RECC_SERVER);
    EXPECT_EQ(expectedAcServer, RECC_CAS_SERVER);
    EXPECT_EQ(expectedAcServer, RECC_ACTION_CACHE_SERVER);
}

TEST_F(EnvTest, EnvTestServerBackwardCompatible)
{
    const char *testEnviron[] = {"RECC_SERVER=oldserver:1234", nullptr};
    const std::string expectedServer = "http://oldserver:1234";
    Env::parse_config_variables(testEnviron);
    Env::handle_special_defaults();
    EXPECT_EQ(expectedServer, RECC_SERVER);
    EXPECT_EQ(expectedServer, RECC_CAS_SERVER);
    EXPECT_EQ(expectedServer, RECC_ACTION_CACHE_SERVER);
}

TEST_F(EnvTest, EnvTestServerBackwardCompatibleSSL)
{
    const char *testEnviron[] = {"RECC_SERVER=oldserver:1234",
                                 "RECC_SERVER_SSL=1", nullptr};
    const std::string expectedServer = "https://oldserver:1234";
    Env::parse_config_variables(testEnviron);
    Env::handle_special_defaults();
    EXPECT_EQ(expectedServer, RECC_SERVER);
    EXPECT_EQ(expectedServer, RECC_CAS_SERVER);
    EXPECT_EQ(expectedServer, RECC_ACTION_CACHE_SERVER);
}

TEST_F(EnvTest, EnvTestServerBackwardCompatibleSeparate)
{
    const char *testEnviron[] = {
        "RECC_SERVER=oldserver:1234", "RECC_CAS_SERVER=oldserver:5678",
        "RECC_ACTION_CACHE_SERVER=oldserver:1776", nullptr};

    const std::string expectedServer = "http://oldserver:1234";
    const std::string expectedCasServer = "http://oldserver:5678";
    const std::string expectedACServer = "http://oldserver:1776";

    Env::parse_config_variables(testEnviron);
    Env::handle_special_defaults();

    EXPECT_EQ(expectedServer, RECC_SERVER);
    EXPECT_EQ(expectedCasServer, RECC_CAS_SERVER);
    EXPECT_EQ(expectedACServer, RECC_ACTION_CACHE_SERVER);
}

TEST_F(EnvTest, EnvTestCacheOnly)
{
    // Verify fallback from empty RECC_CAS_SERVER and RECC_ACTION_CACHE_SERVER
    // to RECC_SERVER also works in cache-only mode.
    const char *testEnviron[] = {"RECC_SERVER=http://casserver:123456",
                                 "RECC_CACHE_ONLY=1", nullptr};

    const std::string expectedServer = "http://casserver:123456";
    const std::string expectedCasServer = "http://casserver:123456";
    const std::string expectedACServer = "http://casserver:123456";

    Env::parse_config_variables(testEnviron);
    Env::handle_special_defaults();

    EXPECT_EQ(expectedServer, RECC_SERVER);
    EXPECT_EQ(expectedCasServer, RECC_CAS_SERVER);
    EXPECT_EQ(expectedACServer, RECC_ACTION_CACHE_SERVER);

    EXPECT_EQ(true, RECC_CACHE_ONLY);
}

TEST_F(EnvTest, EnvTestCacheOnlyCAS)
{
    const char *testEnviron[] = {"RECC_CAS_SERVER=http://casserver:123456",
                                 "RECC_CACHE_ONLY=1", nullptr};

    const std::string expectedServer = "http://casserver:123456";
    const std::string expectedCasServer = "http://casserver:123456";
    const std::string expectedACServer = "http://casserver:123456";

    Env::parse_config_variables(testEnviron);
    Env::handle_special_defaults();

    EXPECT_EQ(expectedServer, RECC_SERVER);
    EXPECT_EQ(expectedCasServer, RECC_CAS_SERVER);
    EXPECT_EQ(expectedACServer, RECC_ACTION_CACHE_SERVER);

    EXPECT_EQ(true, RECC_CACHE_ONLY);
}

TEST_F(EnvTest, EnvTestCacheOnlyForceFalse)
{
    const char *testEnviron[] = {"RECC_CAS_SERVER=http://casserver:123456",
                                 nullptr};

    const std::string expectedServer = "http://localhost:8085";
    const std::string expectedCasServer = "http://casserver:123456";
    const std::string expectedACServer = "http://casserver:123456";

    Env::parse_config_variables(testEnviron);
    Env::handle_special_defaults();

    EXPECT_EQ(expectedServer, RECC_SERVER);
    EXPECT_EQ(expectedCasServer, RECC_CAS_SERVER);
    EXPECT_EQ(expectedACServer, RECC_ACTION_CACHE_SERVER);

    EXPECT_EQ(false, RECC_CACHE_ONLY);
}
