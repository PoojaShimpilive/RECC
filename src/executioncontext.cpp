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

#include <actionbuilder.h>
#include <deps.h>
#include <digestgenerator.h>
#include <env.h>
#include <executioncontext.h>
#include <fileutils.h>
#include <grpcchannels.h>
#include <metricsconfig.h>
#include <parsedcommandfactory.h>
#include <reccdefaults.h>
#include <remoteexecutionclient.h>
#include <requestmetadata.h>
#include <subprocess.h>

#include <cstdio>
#include <cstring>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <iostream>
#include <random>
#include <sys/stat.h>
#include <unistd.h>

#include <buildboxcommon_logging.h>
#include <buildboxcommonmetrics_countingmetricutil.h>
#include <buildboxcommonmetrics_durationmetrictimer.h>
#include <buildboxcommonmetrics_durationmetricvalue.h>
#include <buildboxcommonmetrics_metricteeguard.h>
#include <buildboxcommonmetrics_publisherguard.h>
#include <buildboxcommonmetrics_statsdpublisher.h>
#include <buildboxcommonmetrics_totaldurationmetricvalue.h>

#define TIMER_NAME_EXECUTE_ACTION "recc.execute_action"
#define TIMER_NAME_FIND_MISSING_BLOBS "recc.find_missing_blobs"
#define TIMER_NAME_QUERY_ACTION_CACHE "recc.query_action_cache"
#define TIMER_NAME_UPLOAD_MISSING_BLOBS "recc.upload_missing_blobs"
#define TIMER_NAME_DOWNLOAD_BLOBS "recc.download_blobs"

#define COUNTER_NAME_ACTION_CACHE_HIT "recc.action_cache_hit"
#define COUNTER_NAME_ACTION_CACHE_MISS "recc.action_cache_miss"
#define COUNTER_NAME_UPLOAD_BLOBS_CACHE_HIT "recc.upload_blobs_cache_hit"
#define COUNTER_NAME_UPLOAD_BLOBS_CACHE_MISS "recc.upload_blobs_cache_miss"
#define COUNTER_NAME_INPUT_SIZE_BYTES "recc.input_size_bytes"

namespace recc {

int ExecutionContext::execLocally(int argc, char *argv[])
{
    buildboxcommon::buildboxcommonmetrics::MetricTeeGuard<
        buildboxcommon::buildboxcommonmetrics::DurationMetricTimer>
        mt(TIMER_NAME_EXECUTE_ACTION, d_addDurationMetricCallback);

    auto subprocessResult = Subprocess::execute(
        std::vector<std::string>(argv, argv + argc), false, false);
    return subprocessResult.d_exitCode;
}

proto::ActionResult ExecutionContext::execLocallyWithActionResult(
    int argc, char *argv[], buildboxcommon::digest_string_map *blobs,
    buildboxcommon::digest_string_map *digest_to_filepaths,
    const std::set<std::string> &products)
{
    buildboxcommon::buildboxcommonmetrics::MetricTeeGuard<
        buildboxcommon::buildboxcommonmetrics::DurationMetricTimer>
        mt(TIMER_NAME_EXECUTE_ACTION, d_addDurationMetricCallback);

    proto::ActionResult actionResult;

    auto subprocessResult = Subprocess::execute(
        std::vector<std::string>(argv, argv + argc), true, true);
    std::cout << subprocessResult.d_stdOut;
    std::cerr << subprocessResult.d_stdErr;

    actionResult.set_exit_code(subprocessResult.d_exitCode);

    // Digest captured streams and mark them for upload
    const auto stdoutDigest =
        DigestGenerator::make_digest(subprocessResult.d_stdOut);
    const auto stderrDigest =
        DigestGenerator::make_digest(subprocessResult.d_stdErr);
    (*blobs)[stdoutDigest] = subprocessResult.d_stdOut;
    (*blobs)[stderrDigest] = subprocessResult.d_stdErr;
    actionResult.mutable_stdout_digest()->CopyFrom(stdoutDigest);
    actionResult.mutable_stderr_digest()->CopyFrom(stderrDigest);

    for (const std::string &outputPath : products) {
        // Only upload products produced by the compiler
        if (buildboxcommon::FileUtils::isRegularFile(outputPath.c_str())) {
            const auto file = buildboxcommon::File(outputPath.c_str());
            (*digest_to_filepaths)[file.d_digest] = outputPath;
            auto outputFile = actionResult.add_output_files();
            outputFile->set_path(outputPath);
            outputFile->mutable_digest()->CopyFrom(file.d_digest);
            outputFile->set_is_executable(file.d_executable);
        }
    }

    return actionResult;
}

/**
 * Upload the given resources to the CAS server. This first sends a
 * FindMissingBlobsRequest to determine which resources need to be
 * uploaded, then uses the ByteStream and BatchUpdateBlobs APIs to upload
 * them.
 */
void ExecutionContext::uploadResources(
    const buildboxcommon::digest_string_map &blobs,
    const buildboxcommon::digest_string_map &digest_to_filepaths)
{
    std::vector<proto::Digest> digestsToUpload;
    std::vector<proto::Digest> missingDigests;

    for (const auto &i : blobs) {
        digestsToUpload.push_back(i.first);
    }
    for (const auto &i : digest_to_filepaths) {
        digestsToUpload.push_back(i.first);
    }

    {
        // Timed block
        buildboxcommon::buildboxcommonmetrics::MetricTeeGuard<
            buildboxcommon::buildboxcommonmetrics::DurationMetricTimer>
            mt(TIMER_NAME_FIND_MISSING_BLOBS, d_addDurationMetricCallback);

        missingDigests = d_casClient->findMissingBlobs(digestsToUpload);
    }

    std::vector<buildboxcommon::CASClient::UploadRequest> upload_requests;
    upload_requests.reserve(missingDigests.size());
    for (const auto &digest : missingDigests) {
        // Finding the data in one of the source maps:
        if (blobs.count(digest)) {
            upload_requests.emplace_back(digest, blobs.at(digest));
        }
        else if (digest_to_filepaths.count(digest)) {
            const auto path = digest_to_filepaths.at(digest);
            upload_requests.push_back(
                buildboxcommon::CASClient::UploadRequest::from_path(digest,
                                                                    path));
        }
        else {
            throw std::runtime_error(
                "FindMissingBlobs returned non-existent digest");
        }
    }

    {
        // Timed block
        buildboxcommon::buildboxcommonmetrics::MetricTeeGuard<
            buildboxcommon::buildboxcommonmetrics::DurationMetricTimer>
            mt(TIMER_NAME_UPLOAD_MISSING_BLOBS, d_addDurationMetricCallback);

        d_casClient->uploadBlobs(upload_requests);
    }

    const int64_t uploadCacheHits =
        digestsToUpload.size() - missingDigests.size();
    const int64_t uploadCacheMisses = missingDigests.size();
    buildboxcommon::buildboxcommonmetrics::CountingMetricUtil::
        recordCounterMetric(COUNTER_NAME_UPLOAD_BLOBS_CACHE_HIT,
                            uploadCacheHits);
    d_counterMetrics[COUNTER_NAME_UPLOAD_BLOBS_CACHE_HIT] = uploadCacheHits;
    buildboxcommon::buildboxcommonmetrics::CountingMetricUtil::
        recordCounterMetric(COUNTER_NAME_UPLOAD_BLOBS_CACHE_MISS,
                            uploadCacheMisses);
    d_counterMetrics[COUNTER_NAME_UPLOAD_BLOBS_CACHE_MISS] = uploadCacheMisses;
}

int64_t ExecutionContext::calculateTotalSize(
    const buildboxcommon::digest_string_map &blobs,
    const buildboxcommon::digest_string_map &digest_to_filepaths)
{
    int64_t totalSize = 0;

    for (const auto &i : blobs) {
        totalSize += i.first.size_bytes();
    }
    for (const auto &i : digest_to_filepaths) {
        totalSize += i.first.size_bytes();
    }

    return totalSize;
}

void ExecutionContext::setStopToken(const std::atomic_bool &stop_requested)
{
    this->d_stopRequested = &stop_requested;
}

std::string getRandomString()
{
    std::random_device randomDevice;
    std::uniform_int_distribution<uint32_t> randomDistribution;
    std::stringstream stream;
    stream << std::hex << std::setw(8) << std::setfill('0')
           << randomDistribution(randomDevice);
    return stream.str();
}

int ExecutionContext::execute(int argc, char *argv[])
{
    try {
        Env::set_config_locations();
        Env::parse_config_variables();
    }
    catch (const std::invalid_argument &e) {
        BUILDBOX_LOG_ERROR("Error parsing config: " << e.what());
        throw;
    }

    BUILDBOX_LOG_DEBUG("RECC_REAPI_VERSION == '" << RECC_REAPI_VERSION << "'");

    std::shared_ptr<StatsDPublisherType> statsDPublisher;
    try {
        statsDPublisher = get_statsdpublisher_from_config();
    }
    catch (const std::runtime_error &e) {
        BUILDBOX_LOG_ERROR(
            "Could not initialize statsD publisher: " << e.what());
        throw;
    }

    buildboxcommon::buildboxcommonmetrics::PublisherGuard<StatsDPublisherType>
        statsDPublisherGuard(RECC_ENABLE_METRICS, *statsDPublisher);

    d_addDurationMetricCallback =
        std::bind(&ExecutionContext::addDurationMetric, this,
                  std::placeholders::_1, std::placeholders::_2);

    const std::string cwd = FileUtils::getCurrentWorkingDirectory();
    const auto command =
        ParsedCommandFactory::createParsedCommand(argv, cwd.c_str());

    buildboxcommon::digest_string_map blobs;
    buildboxcommon::digest_string_map digest_to_filepaths;
    std::set<std::string> products;

    std::shared_ptr<proto::Action> actionPtr;
    if (command.is_compiler_command() || RECC_FORCE_REMOTE) {
        // Trying to build an `Action`:
        try {
            ActionBuilder actionBuilder(d_addDurationMetricCallback);
            actionPtr = actionBuilder.BuildAction(
                command, cwd, &blobs, &digest_to_filepaths, &products);
        }
        catch (const std::invalid_argument &) {
            BUILDBOX_LOG_ERROR(
                "Invalid `argv[0]` value in command: \"" +
                command.get_command().at(0) +
                "\". The Remote Execution API requires it to specify "
                "either a relative or absolute path to an executable.");
            throw;
        }

        // Calculate and record total size of input blobs
        const int64_t inputSize =
            calculateTotalSize(blobs, digest_to_filepaths);
        buildboxcommon::buildboxcommonmetrics::CountingMetricUtil::
            recordCounterMetric(COUNTER_NAME_INPUT_SIZE_BYTES, inputSize);
        d_counterMetrics[COUNTER_NAME_INPUT_SIZE_BYTES] = inputSize;
    }
    else {
        BUILDBOX_LOG_INFO("Not a compiler command, so running locally. (Use "
                          "RECC_FORCE_REMOTE=1 to force remote execution)");
    }

    // If we don't need to build an `Action` or if the process fails, we defer
    // to running the command locally (unless we are in no-build mode):
    if (!actionPtr) {
        if (RECC_NO_EXECUTE) {
            BUILDBOX_LOG_INFO("Command would have run locally but "
                              "RECC_NO_EXECUTE is enabled, exiting.");
            return 0;
        }
        return execLocally(argc, argv);
    }

    const proto::Action action = *actionPtr;
    const proto::Digest actionDigest = DigestGenerator::make_digest(action);
    this->d_actionDigest = actionDigest;

    BUILDBOX_LOG_DEBUG("Action Digest: " << actionDigest
                                         << " Action Contents: "
                                         << action.ShortDebugString());
    if (RECC_NO_EXECUTE) {
        BUILDBOX_LOG_INFO("RECC_NO_EXECUTE is enabled, exiting.");
        return 0;
    }

    // Setting up the gRPC connections:
    std::unique_ptr<GrpcChannels> returnChannels;
    try {
        returnChannels = std::make_unique<GrpcChannels>(
            GrpcChannels::get_channels_from_config());
    }
    catch (const std::runtime_error &e) {
        BUILDBOX_LOG_ERROR("Invalid argument in channel config: " << e.what());
        throw;
    }

    const auto configured_digest_function =
        DigestGenerator::stringToDigestFunctionMap().at(
            RECC_CAS_DIGEST_FUNCTION);

    auto casGrpcClient = std::make_shared<buildboxcommon::GrpcClient>();
    casGrpcClient->init(*returnChannels->cas());
    auto executionGrpcClient = std::make_shared<buildboxcommon::GrpcClient>();
    executionGrpcClient->init(*returnChannels->server());
    auto actionCacheGrpcClient =
        std::make_shared<buildboxcommon::GrpcClient>();
    actionCacheGrpcClient->init(*returnChannels->action_cache());

    casGrpcClient->setToolDetails(
        RequestMetadataGenerator::RECC_METADATA_TOOL_NAME,
        RequestMetadataGenerator::RECC_METADATA_TOOL_VERSION);
    casGrpcClient->setRequestMetadata(
        proto::toString(actionDigest),
        RequestMetadataGenerator::tool_invocation_id(),
        RECC_CORRELATED_INVOCATIONS_ID);

    executionGrpcClient->setToolDetails(
        RequestMetadataGenerator::RECC_METADATA_TOOL_NAME,
        RequestMetadataGenerator::RECC_METADATA_TOOL_VERSION);
    executionGrpcClient->setRequestMetadata(
        proto::toString(actionDigest),
        RequestMetadataGenerator::tool_invocation_id(),
        RECC_CORRELATED_INVOCATIONS_ID);

    actionCacheGrpcClient->setToolDetails(
        RequestMetadataGenerator::RECC_METADATA_TOOL_NAME,
        RequestMetadataGenerator::RECC_METADATA_TOOL_VERSION);
    actionCacheGrpcClient->setRequestMetadata(
        proto::toString(actionDigest),
        RequestMetadataGenerator::tool_invocation_id(),
        RECC_CORRELATED_INVOCATIONS_ID);

    d_casClient = std::make_shared<buildboxcommon::CASClient>(
        casGrpcClient, configured_digest_function);
    d_casClient->init(RECC_CAS_GET_CAPABILITIES);

    RemoteExecutionClient reClient(d_casClient, executionGrpcClient,
                                   actionCacheGrpcClient);
    reClient.init();

    bool action_in_cache = false;
    proto::ActionResult result;

    // If allowed, we look in the action cache first:
    if (!RECC_SKIP_CACHE) {
        try {
            { // Timed block
                buildboxcommon::buildboxcommonmetrics::MetricTeeGuard<
                    buildboxcommon::buildboxcommonmetrics::DurationMetricTimer>
                    mt(TIMER_NAME_QUERY_ACTION_CACHE,
                       d_addDurationMetricCallback);

                action_in_cache = reClient.fetchFromActionCache(
                    actionDigest, command.get_products(), &result);
                if (action_in_cache) {
                    buildboxcommon::buildboxcommonmetrics::CountingMetricUtil::
                        recordCounterMetric(COUNTER_NAME_ACTION_CACHE_HIT, 1);
                    d_counterMetrics[COUNTER_NAME_ACTION_CACHE_HIT] = 1;
                    BUILDBOX_LOG_INFO("Action Cache hit for [" << actionDigest
                                                               << "]");
                }
                else {
                    buildboxcommon::buildboxcommonmetrics::CountingMetricUtil::
                        recordCounterMetric(COUNTER_NAME_ACTION_CACHE_MISS, 1);
                    d_counterMetrics[COUNTER_NAME_ACTION_CACHE_MISS] = 1;
                }
            }
        }
        catch (const std::exception &e) {
            BUILDBOX_LOG_ERROR("Error while querying action cache at \""
                               << RECC_ACTION_CACHE_SERVER
                               << "\": " << e.what());
        }
    }

    // If the results for the action are not cached, we upload the
    // necessary resources to CAS:
    if (!action_in_cache) {
        blobs[actionDigest] = action.SerializeAsString();

        if (RECC_CACHE_ONLY) {
            bool cache_upload_local_build =
                RECC_CACHE_UPLOAD_LOCAL_BUILD && !RECC_ACTION_UNCACHEABLE;
            BUILDBOX_LOG_INFO(
                "Action not cached and running in cache-only mode, "
                "executing locally");
            if (!cache_upload_local_build) {
                return execLocally(argc, argv);
            }
            else {
                // There is no need to upload input files in cache-only mode.
                digest_to_filepaths.clear();

                const auto actionResult = execLocallyWithActionResult(
                    argc, argv, &blobs, &digest_to_filepaths, products);
                const size_t number_of_outputs =
                    actionResult.output_files_size();

                if (actionResult.exit_code() != 0 &&
                    !RECC_CACHE_UPLOAD_FAILED_BUILD) {
                    BUILDBOX_LOG_WARNING(
                        "Not uploading actionResult due to exit_code = "
                        << actionResult.exit_code()
                        << ", RECC_CACHE_UPLOAD_FAILED_BUILD = "
                        << std::boolalpha << RECC_CACHE_UPLOAD_FAILED_BUILD);
                }
                else if (number_of_outputs != products.size()) {
                    BUILDBOX_LOG_WARNING(
                        "Not uploading actionResult due to "
                        << (products.size() - number_of_outputs)
                        << " of the requested output files not being found");
                }
                else {
                    BUILDBOX_LOG_DEBUG("Uploading local build...");
                    try {
                        uploadResources(blobs, digest_to_filepaths);
                    }
                    catch (const std::exception &e) {
                        BUILDBOX_LOG_WARNING(
                            "Error while uploading local build to CAS at \""
                            << RECC_CAS_SERVER << "\": " << e.what());
                        // Skipping update of action cache
                        return actionResult.exit_code();
                    }

                    try {
                        reClient.updateActionCache(actionDigest, actionResult);
                        BUILDBOX_LOG_INFO("Action cache updated");
                    }
                    catch (const std::exception &e) {
                        // Only log warning as local execution was still
                        // successful
                        BUILDBOX_LOG_WARNING(
                            "Error while calling `UpdateActionCache()` on \""
                            << RECC_ACTION_CACHE_SERVER << "\": " << e.what());
                    }
                }

                // Store action result for access by the caller of this method.
                this->d_actionResult = actionResult;

                return actionResult.exit_code();
            }
        }

        BUILDBOX_LOG_INFO("Executing action remotely... [actionDigest="
                          << actionDigest << "]");

        BUILDBOX_LOG_DEBUG("Uploading resources...");
        try {
            uploadResources(blobs, digest_to_filepaths);
        }
        catch (const std::exception &e) {
            BUILDBOX_LOG_ERROR("Error while uploading resources to CAS at \""
                               << RECC_CAS_SERVER << "\": " << e.what());
            throw;
        }

        // And call `Execute()`:
        try {
            // Timed block
            buildboxcommon::buildboxcommonmetrics::MetricTeeGuard<
                buildboxcommon::buildboxcommonmetrics::DurationMetricTimer>
                mt(TIMER_NAME_EXECUTE_ACTION, d_addDurationMetricCallback);

            result = reClient.executeAction(actionDigest, *d_stopRequested,
                                            RECC_SKIP_CACHE);
            BUILDBOX_LOG_INFO("Remote execution finished with exit code "
                              << result.exit_code());
        }
        catch (const std::exception &e) {
            BUILDBOX_LOG_ERROR("Error while calling `Execute()` on \""
                               << RECC_SERVER << "\": " << e.what());
            throw;
        }
    }

    // Store action result for access by the caller of this method.
    this->d_actionResult = result;

    const int exitCode = result.exit_code();
    if (exitCode == 0 && result.output_files_size() == 0 &&
        products.size() != 0) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "Action produced none of the of the expected output_files");
    }
    try {
        if (RECC_DONT_SAVE_OUTPUT) {
            // We still call `writeFilesToDisk()` for stdout and stderr.
            // Clear output files and directories to skip download and write.
            result.clear_output_files();
            result.clear_output_symlinks();
            result.clear_output_directories();
        }

        const auto randomStr = getRandomString();

        // Add stdout and stderr as output files if they aren't embedded.
        // This allows download of stdout, stderr and output files in a
        // single batch.
        const std::string stdoutFilename = ".recc-stdout-" + randomStr;
        const std::string stderrFilename = ".recc-stderr-" + randomStr;
        const bool fetchStdout = result.has_stdout_digest() &&
                                 result.stdout_digest().size_bytes() > 0;
        const bool fetchStderr = result.has_stderr_digest() &&
                                 result.stderr_digest().size_bytes() > 0;
        if (fetchStdout) {
            proto::OutputFile outputFile;
            outputFile.mutable_digest()->CopyFrom(result.stdout_digest());
            outputFile.set_path(stdoutFilename);
            *result.add_output_files() = outputFile;
        }
        if (fetchStderr) {
            proto::OutputFile outputFile;
            outputFile.mutable_digest()->CopyFrom(result.stderr_digest());
            outputFile.set_path(stderrFilename);
            *result.add_output_files() = outputFile;
        }

        {
            // Timed block
            buildboxcommon::buildboxcommonmetrics::MetricTeeGuard<
                buildboxcommon::buildboxcommonmetrics::DurationMetricTimer>
                mt(TIMER_NAME_DOWNLOAD_BLOBS, d_addDurationMetricCallback);

            reClient.writeFilesToDisk(result);
        }

        /* These don't use logging macros because they are compiler output
         */
        if (fetchStdout) {
            std::ifstream file(stdoutFilename);
            std::cout << file.rdbuf();
            unlink(stdoutFilename.c_str());
        }
        else {
            std::cout << result.stdout_raw();
        }
        if (fetchStderr) {
            std::ifstream file(stderrFilename);
            std::cerr << file.rdbuf();
            unlink(stderrFilename.c_str());
        }
        else {
            std::cerr << result.stderr_raw();
        }

        return exitCode;
    }
    catch (const std::exception &e) {
        BUILDBOX_LOG_ERROR(e.what());
        throw;
    }
}

const std::map<std::string,
               buildboxcommon::buildboxcommonmetrics::DurationMetricValue> *
ExecutionContext::getDurationMetrics() const
{
    return &d_durationMetrics;
}

void ExecutionContext::addDurationMetric(
    const std::string &name,
    buildboxcommon::buildboxcommonmetrics::DurationMetricValue value)
{
    d_durationMetrics[name] = value;
}

const std::map<std::string, int64_t> *
ExecutionContext::getCounterMetrics() const
{
    return &d_counterMetrics;
}

const buildboxcommon::Digest &ExecutionContext::getActionDigest() const
{
    return d_actionDigest;
}

const buildboxcommon::ActionResult &ExecutionContext::getActionResult() const
{
    return d_actionResult;
}

buildboxcommon::CASClient *ExecutionContext::getCasClient() const
{
    return d_casClient.get();
}

} // namespace recc
