// Copyright 2018-2021 Bloomberg Finance L.P
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

#include <remoteexecutionclient.h>

#include <digestgenerator.h>
#include <env.h>
#include <fileutils.h>
#include <reccdefaults.h>

#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommonmetrics_durationmetrictimer.h>
#include <buildboxcommonmetrics_metricguard.h>

#include <functional>

#define TIMER_NAME_FETCH_WRITE_RESULTS "recc.fetch_write_results"

namespace recc {

proto::ActionResult
RemoteExecutionClient::executeAction(const proto::Digest &actionDigest,
                                     const std::atomic_bool &stop_requested,
                                     bool skipCache)
{
    proto::ActionResult resultProto =
        buildboxcommon::RemoteExecutionClient::executeAction(
            actionDigest, stop_requested, skipCache);
    if (RECC_VERBOSE) {
        BUILDBOX_LOG_DEBUG("Action result contains: [Files="
                           << resultProto.output_files_size()
                           << "], [Directories="
                           << resultProto.output_directories_size() << "]");

        for (int i = 0; i < resultProto.output_files_size(); ++i) {
            auto fileProto = resultProto.output_files(i);
            BUILDBOX_LOG_DEBUG("File digest=["
                               << fileProto.digest().hash() << "/"
                               << fileProto.digest().size_bytes() << "] :"
                               << " path=[" << fileProto.path() << "]");
        }
        for (int i = 0; i < resultProto.output_directories_size(); ++i) {
            auto dirProto = resultProto.output_directories(i);
            BUILDBOX_LOG_DEBUG("Directory tree digest=["
                               << dirProto.tree_digest().hash() << "/"
                               << dirProto.tree_digest().size_bytes() << "] :"
                               << " path=[" << dirProto.path() << "]");
        }
    }
    return resultProto;
}

void RemoteExecutionClient::writeFilesToDisk(const proto::ActionResult &result,
                                             const char *root)
{
    // Timed function
    buildboxcommon::buildboxcommonmetrics::MetricGuard<
        buildboxcommon::buildboxcommonmetrics::DurationMetricTimer>
        mt(TIMER_NAME_FETCH_WRITE_RESULTS);

    buildboxcommon::FileDescriptor root_dirfd(
        open(root, O_RDONLY | O_DIRECTORY));
    if (root_dirfd.get() < 0) {
        BUILDBOXCOMMON_THROW_SYSTEM_EXCEPTION(
            std::system_error, errno, std::system_category,
            "Error opening directory at path \"" << root << "\"");
    }

    downloadOutputs(d_casClient.get(), result, root_dirfd.get());
}

} // namespace recc
