// Copyright 2021 Bloomberg Finance L.P
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

#ifndef INCLUDED_EXECUTIONCONTEXT
#define INCLUDED_EXECUTIONCONTEXT

#include <atomic>

#include <buildboxcommon_casclient.h>
#include <buildboxcommon_protos.h>
#include <buildboxcommonmetrics_durationmetricvalue.h>

namespace recc {

/**
 * The ExecutionContext class holds the state for command execution.
 */
class ExecutionContext {
  public:
    /**
     * Set the stop token, which cancels execution at the next possible point
     * when it becomes true.
     */
    void setStopToken(const std::atomic_bool &stop_requested);

    /**
     * Execute the specified command. Depending on the configuration, this may
     * use remote execution or local execution with caching.
     */
    int execute(int argc, char *argv[]);

    const std::map<std::string,
                   buildboxcommon::buildboxcommonmetrics::DurationMetricValue>
        *getDurationMetrics() const;

    const std::map<std::string, int64_t> *getCounterMetrics() const;

    const buildboxcommon::Digest &getActionDigest() const;

    const buildboxcommon::ActionResult &getActionResult() const;

    buildboxcommon::CASClient *getCasClient() const;

  private:
    const std::atomic_bool *d_stopRequested;
    std::map<std::string,
             buildboxcommon::buildboxcommonmetrics::DurationMetricValue>
        d_durationMetrics;
    std::function<void(
        const std::string &,
        buildboxcommon::buildboxcommonmetrics::DurationMetricValue)>
        d_addDurationMetricCallback;
    std::map<std::string, int64_t> d_counterMetrics;
    buildboxcommon::Digest d_actionDigest;
    buildboxcommon::ActionResult d_actionResult;

    std::shared_ptr<buildboxcommon::CASClient> d_casClient;

    int execLocally(int argc, char *argv[]);

    buildboxcommon::ActionResult execLocallyWithActionResult(
        int argc, char *argv[], buildboxcommon::digest_string_map *blobs,
        buildboxcommon::digest_string_map *digest_to_filepaths,
        const std::set<std::string> &products);

    void uploadResources(
        const buildboxcommon::digest_string_map &blobs,
        const buildboxcommon::digest_string_map &digest_to_filepaths);

    int64_t calculateTotalSize(
        const buildboxcommon::digest_string_map &blobs,
        const buildboxcommon::digest_string_map &digest_to_filepaths);

    void addDurationMetric(
        const std::string &name,
        buildboxcommon::buildboxcommonmetrics::DurationMetricValue value);
};

} // namespace recc

#endif
