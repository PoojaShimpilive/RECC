// Copyright 2019 Bloomberg Finance L.P
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
#include <protos.h>
#include <reccdefaults.h>
#include <requestmetadata.h>
#include <unistd.h>

namespace recc {

const std::string RequestMetadataGenerator::RECC_METADATA_TOOL_NAME = "recc";

#ifndef RECC_VERSION
#error "RECC_VERSION is not defined"
#else
const std::string RequestMetadataGenerator::RECC_METADATA_TOOL_VERSION =
    RECC_VERSION; // set by CMake
#endif

const std::string RequestMetadataGenerator::RECC_METADATA_HEADER_NAME =
    "build.bazel.remote.execution.v2.requestmetadata-bin";

proto::ToolDetails RequestMetadataGenerator::recc_tool_details()
{
    proto::ToolDetails toolDetails;
    toolDetails.set_tool_name(RECC_METADATA_TOOL_NAME);
    toolDetails.set_tool_version(RECC_METADATA_TOOL_VERSION);
    return toolDetails;
}

std::string RequestMetadataGenerator::tool_invocation_id()
{
    const std::string hostName = hostname();
    const std::string parentPID = std::to_string(getppid());

    return hostName + ":" + parentPID;
}

std::string RequestMetadataGenerator::hostname()
{
    char hostname[DEFAULT_RECC_HOSTNAME_MAX_LENGTH + 1];
    hostname[DEFAULT_RECC_HOSTNAME_MAX_LENGTH] = '\0';

    const int error = gethostname(hostname, sizeof(hostname) - 1);

    if (error) {
        return "";
    }

    return std::string(hostname);
}

} // namespace recc
