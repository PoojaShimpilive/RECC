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
#include <stdlib.h>

#include <gtest/gtest.h>

using namespace recc;

TEST(EnvTest, EnvFromFile)
{
    // Make sure outside env doesn't get prioritized
    unsetenv("RECC_SERVER");
    unsetenv("RECC_CAS_SERVER");
    unsetenv("TMPDIR");
    unsetenv("RECC_AUTH_UNCONFIGURED_MSG");
    unsetenv("RECC_REMOTE_PLATFORM_arch");
    unsetenv("RECC_REMOTE_PLATFORM_OSFamily");
    unsetenv("RECC_DEPS_ENV_Header");
    unsetenv("RECC_REMOTE_ENV_Header");

    // should be set from file in data/recc/recc.conf
    const std::string expectedReccServer = "http://localhost:99999";
    const std::string expectedRecCasServer = "http://localhost:66666";
    const std::string expectedTMPDIR = "/tmp/dir";
    const std::string expectedAuthMsg = "authentication not configured";
    const std::string expectedRemoteArch = "x86_64";
    const std::string expectedRemoteOS = "linux";
    const std::string expecteddepsHeader = "/usr/local/bin";
    const std::string expectedenvHeader = "/usr/bin";
    const std::string expectedDocker = "docker";
    const std::string expectedUpperCaseProperty = "uppercase";

    // In this test use the usual config location list
    Env::set_config_locations();
    Env::parse_config_variables();

    EXPECT_EQ(expectedReccServer, RECC_SERVER);
    EXPECT_EQ(expectedRecCasServer, RECC_CAS_SERVER);
    EXPECT_EQ(expectedTMPDIR, TMPDIR);
    EXPECT_EQ(expectedAuthMsg, RECC_AUTH_UNCONFIGURED_MSG);
    EXPECT_EQ(expectedRemoteArch, RECC_REMOTE_PLATFORM["arch"]);
    EXPECT_EQ(expectedRemoteOS, RECC_REMOTE_PLATFORM["OSFamily"]);
    EXPECT_EQ(expectedDocker, RECC_REMOTE_PLATFORM["Docker_Container"]);
    EXPECT_EQ(expecteddepsHeader, RECC_DEPS_ENV["Header"]);
    EXPECT_EQ(expectedenvHeader, RECC_REMOTE_ENV["Header"]);
    EXPECT_EQ(expectedRemoteOS, RECC_REMOTE_PLATFORM["OSFamily"]);
    EXPECT_EQ(expectedUpperCaseProperty, RECC_REMOTE_PLATFORM["test"]);
}
