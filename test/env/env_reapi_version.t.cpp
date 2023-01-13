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

#include <env.h>
#include <gtest/gtest.h>

#include <protos.h>

using namespace recc;

TEST(EnvReapiVersion, DefaultValue)
{
    const char *testEnviron[] = {nullptr};
    Env::parse_config_variables(testEnviron);
    // need this for testing, since we are calling parse_config_variables
    // directly.
    Env::handle_special_defaults();

    EXPECT_EQ(RECC_REAPI_VERSION, "2.2");
}

TEST(EnvReapiVersion, ValidValue)
{
    const char *testEnviron[] = {"RECC_REAPI_VERSION=2.1", nullptr};
    ASSERT_TRUE(proto::s_reapiSupportedVersions.count("2.1"));

    Env::parse_config_variables(testEnviron);

    ASSERT_NO_THROW(Env::assert_reapi_version_is_valid());
    ASSERT_EQ(RECC_REAPI_VERSION, "2.1");
}

TEST(EnvReapiVersion, InvalidValue)
{
    const char *testEnviron[] = {"RECC_REAPI_VERSION=12.3", nullptr};
    ASSERT_FALSE(proto::s_reapiSupportedVersions.count("12.3"));
    Env::parse_config_variables(testEnviron);

    ASSERT_THROW(Env::assert_reapi_version_is_valid(), std::runtime_error);
}

TEST(EnvReapiVersion, TestVersionStringToIntPair)
{
    // string -> expected pair
    const std::unordered_map<std::string, std::pair<int, int>>
        expected_values = {
            {"0.1", {0, 1}},
            {"2.0", {2, 0}},
            {"2.2", {2, 2}},
        };

    for (const auto &entry : expected_values) {
        std::pair<int, int> pair;
        ASSERT_NO_THROW(pair = Env::version_string_to_pair(entry.first));
        EXPECT_EQ(pair, entry.second);
    }
}

TEST(EnvReapiVersion, TestVersionStringToIntPairError)
{
    EXPECT_THROW(Env::version_string_to_pair("2"), std::invalid_argument);
    EXPECT_THROW(Env::version_string_to_pair("2.0.0"), std::invalid_argument);
    EXPECT_THROW(Env::version_string_to_pair("3.14"), std::invalid_argument);
    EXPECT_THROW(Env::version_string_to_pair("versionFoo"),
                 std::invalid_argument);
}

TEST(EnvReapiVersion, ConfiguredReapiVersionNewerThan)
{
    RECC_REAPI_VERSION = "2.0";
    EXPECT_TRUE(Env::configured_reapi_version_equal_to_or_newer_than("1.9"));
    EXPECT_TRUE(Env::configured_reapi_version_equal_to_or_newer_than("2.0"));
    EXPECT_FALSE(Env::configured_reapi_version_equal_to_or_newer_than("2.1"));
}
