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

#ifndef INCLUDED_REQUESTMETADATA
#define INCLUDED_REQUESTMETADATA

#include <protos.h>

/** According to the REAPI specification:
 *
 * "An optional Metadata to attach to any RPC request to tell the server about
 * an external context of the request."
 *
 * "[...] To use it, the client attaches the header to the call using the
 * canonical proto serialization[.]"

 * name: `build.bazel.remote.execution.v2.requestmetadata-bin`
 * contents: the base64 encoded binary `RequestMetadata` message.
 */

namespace recc {

class RequestMetadataGenerator {
  public:
    static proto::ToolDetails recc_tool_details();

    static std::string tool_invocation_id();

    static const std::string RECC_METADATA_TOOL_NAME;
    static const std::string RECC_METADATA_TOOL_VERSION;

    static const std::string RECC_METADATA_HEADER_NAME;

    static std::string hostname();
};

} // namespace recc

#endif
