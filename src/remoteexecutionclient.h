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

#ifndef INCLUDED_REMOTEEXECUTIONCLIENT
#define INCLUDED_REMOTEEXECUTIONCLIENT

#include <protos.h>

#include <buildboxcommon_casclient.h>
#include <buildboxcommon_connectionoptions.h>
#include <buildboxcommon_grpcclient.h>
#include <buildboxcommon_remoteexecutionclient.h>

#include <atomic>
#include <map>
#include <set>

namespace recc {

typedef std::shared_ptr<
    grpc::ClientAsyncReaderInterface<google::longrunning::Operation>>
    ReaderPointer;

typedef std::shared_ptr<google::longrunning::Operation> OperationPointer;

class RemoteExecutionClient : public buildboxcommon::RemoteExecutionClient {
  private:
    std::shared_ptr<buildboxcommon::CASClient> d_casClient;

  public:
    explicit RemoteExecutionClient(
        std::shared_ptr<buildboxcommon::CASClient> casClient,
        std::shared_ptr<buildboxcommon::GrpcClient> executionGrpcClient,
        std::shared_ptr<buildboxcommon::GrpcClient> actionCacheGrpcClient)
        : buildboxcommon::RemoteExecutionClient(executionGrpcClient,
                                                actionCacheGrpcClient),
          d_casClient(casClient)
    {
    }

    /**
     * Run the action with the given digest on the given server, waiting
     * synchronously for it to complete. The Action must already be present in
     * the server's CAS.
     */
    proto::ActionResult executeAction(const proto::Digest &actionDigest,
                                      const std::atomic_bool &stop_requested,
                                      bool skipCache = false);

    /**
     * Write the given ActionResult's output files to disk.
     */
    void writeFilesToDisk(const proto::ActionResult &result,
                          const char *root = ".");
};
} // namespace recc
#endif
