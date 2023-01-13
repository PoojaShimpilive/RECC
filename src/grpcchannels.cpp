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
//

#include <env.h>
#include <grpcchannels.h>

#include <sstream>
#include <string>

namespace recc {

GrpcChannels GrpcChannels::get_channels_from_config()
{
    buildboxcommon::ConnectionOptions options[3];

    options[0].setUrl(RECC_SERVER);
    options[1].setUrl(RECC_CAS_SERVER);
    options[2].setUrl(RECC_ACTION_CACHE_SERVER);

    for (auto &option : options) {
        const std::string retryLimitStr = std::to_string(RECC_RETRY_LIMIT);
        const std::string retryDelayStr = std::to_string(RECC_RETRY_DELAY);
        const std::string requestTimeoutStr =
            std::to_string(RECC_REQUEST_TIMEOUT);
        const std::string keepaliveTimeStr =
            std::to_string(RECC_KEEPALIVE_TIME);

        option.setInstanceName(RECC_INSTANCE);

        option.setRetryLimit(retryLimitStr);
        option.setRetryDelay(retryDelayStr);
        option.setRequestTimeout(requestTimeoutStr);
        option.setKeepaliveTime(keepaliveTimeStr);

        if (!RECC_ACCESS_TOKEN_PATH.empty()) {
            option.setAccessTokenPath(RECC_ACCESS_TOKEN_PATH);
        }

        option.setUseGoogleApiAuth(RECC_SERVER_AUTH_GOOGLEAPI);
    }

    return GrpcChannels(options[0], options[1], options[2]);
}

} // namespace recc
