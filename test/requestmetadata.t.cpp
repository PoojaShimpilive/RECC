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
#include <requestmetadata.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace recc;
using namespace testing;

TEST(RequestMetadataTest, ToolDetails)
{
    RequestMetadataGenerator metadataGenerator;
    proto::ToolDetails toolDetails = metadataGenerator.recc_tool_details();

    ASSERT_EQ(toolDetails.tool_name(),
              metadataGenerator.RECC_METADATA_TOOL_NAME);
    ASSERT_EQ(toolDetails.tool_version(),
              metadataGenerator.RECC_METADATA_TOOL_VERSION);
}

TEST(RequestMetadataTest, ToolInvocationID)
{
    RequestMetadataGenerator metadataGenerator;
    const std::string toolInvocationId =
        metadataGenerator.tool_invocation_id();

    std::size_t separator = toolInvocationId.find(':');
    ASSERT_NE(separator, std::string::npos);

    const std::string hostName = toolInvocationId.substr(0, separator);
    const std::string parentPid = toolInvocationId.substr(separator + 1);

    // PPID is a number:
    ASSERT_NO_THROW(std::stoi(parentPid));
}
