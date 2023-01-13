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

#ifndef INCLUDED_GRPCCHANNELS
#define INCLUDED_GRPCCHANNELS

#include <buildboxcommon_connectionoptions.h>

namespace recc {

class GrpcChannels {
  public:
    /**
     * builds appropriate channels from environment
     * variables. Will return a channel for cas, a
     * channel for the build server and one for
     * the action cache.
     */
    static GrpcChannels get_channels_from_config();

    const buildboxcommon::ConnectionOptions *server() { return &d_server; }
    const buildboxcommon::ConnectionOptions *cas() { return &d_cas; }
    const buildboxcommon::ConnectionOptions *action_cache()
    {
        return &d_action_cache;
    }

  private:
    /*
     * Left private as this object should be constructed using
     * 'get_channels_from_config'.
     */
    GrpcChannels(buildboxcommon::ConnectionOptions server,
                 buildboxcommon::ConnectionOptions cas,
                 buildboxcommon::ConnectionOptions action_cache)
        : d_server(server), d_cas(cas), d_action_cache(action_cache)
    {
    }

    buildboxcommon::ConnectionOptions d_server;
    buildboxcommon::ConnectionOptions d_cas;
    buildboxcommon::ConnectionOptions d_action_cache;
};

} // namespace recc

#endif
