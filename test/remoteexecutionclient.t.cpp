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

#include <buildboxcommonmetrics_durationmetricvalue.h>
#include <buildboxcommonmetrics_testingutils.h>
#include <digestgenerator.h>
#include <env.h>
#include <fileutils.h>
#include <remoteexecutionclient.h>

#include <buildboxcommon_grpctestserver.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_temporarydirectory.h>

#include <build/bazel/remote/execution/v2/remote_execution_mock.grpc.pb.h>
#include <build/buildgrid/local_cas_mock.grpc.pb.h>
#include <gmock/gmock.h>
#include <google/bytestream/bytestream_mock.grpc.pb.h>
#include <google/longrunning/operations_mock.grpc.pb.h>
#include <google/protobuf/util/message_differencer.h>
#include <grpcpp/test/mock_stream.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <set>
#include <unistd.h>

#define TIMER_NAME_FETCH_WRITE_RESULTS "recc.fetch_write_results"

using namespace recc;
using namespace buildboxcommon::buildboxcommonmetrics;
using namespace testing;

/*
 * This fixture sets up all of the dependencies for execute_action
 *
 * Any dependencies can be overridden by setting them in the respective tests
 */
class RemoteExecutionClientTestFixture : public ::testing::Test {
  protected:
    buildboxcommon::GrpcTestServer testServer;
    std::shared_ptr<proto::Execution::StubInterface> executionStub;
    std::shared_ptr<proto::MockContentAddressableStorageStub> casStub;
    std::shared_ptr<proto::MockCapabilitiesStub> casCapabilitiesStub;
    std::shared_ptr<proto::MockActionCacheStub> actionCacheStub;
    std::shared_ptr<google::longrunning::Operations::StubInterface>
        operationsStub;
    std::shared_ptr<google::bytestream::MockByteStreamStub> byteStreamStub;
    std::shared_ptr<build::buildgrid::MockLocalContentAddressableStorageStub>
        localCasStub;

    std::shared_ptr<buildboxcommon::GrpcClient> grpcClient;
    std::shared_ptr<buildboxcommon::CASClient> casClient;
    std::shared_ptr<RemoteExecutionClient> reClient;

    const std::string instance_name = "";

    proto::Digest actionDigest;
    proto::ExecuteRequest expectedExecuteRequest;

    proto::Digest stdErrDigest;

    proto::Digest treeDigest;

    google::longrunning::Operation operation;

    grpc::testing::MockClientReader<google::bytestream::ReadResponse> *reader;

    google::bytestream::ReadResponse readResponse;

    RemoteExecutionClientTestFixture()
        : grpcClient(std::make_shared<buildboxcommon::GrpcClient>()),
          casClient(std::make_shared<buildboxcommon::CASClient>(grpcClient)),
          reClient(std::make_shared<RemoteExecutionClient>(
              casClient, grpcClient, grpcClient)),
          executionStub(proto::Execution::NewStub(testServer.channel())),
          casStub(
              std::make_shared<proto::MockContentAddressableStorageStub>()),
          actionCacheStub(std::make_shared<proto::MockActionCacheStub>()),
          operationsStub(proto::Operations::NewStub(testServer.channel())),
          byteStreamStub(
              std::make_shared<google::bytestream::MockByteStreamStub>()),
          localCasStub(
              std::make_shared<
                  build::buildgrid::MockLocalContentAddressableStorageStub>())
    {
        grpcClient->setInstanceName(instance_name);
        casClient->init(byteStreamStub, casStub, localCasStub,
                        casCapabilitiesStub);
        reClient->init(executionStub, actionCacheStub, operationsStub);

        // Construct the Digest we're passing in, and the ExecuteRequest we
        // expect the RemoteExecutionClient to send as a result.
        actionDigest.set_hash("Action digest hash here");
        *expectedExecuteRequest.mutable_action_digest() = actionDigest;

        // Begin constructing a fake ExecuteResponse to return to the this->
        proto::ExecuteResponse executeResponse;
        const auto actionResultProto = executeResponse.mutable_result();
        actionResultProto->set_stdout_raw("Raw stdout.");
        std::string stdErr("Stderr, which will be sent as a digest.");
        stdErrDigest = DigestGenerator::make_digest(stdErr);
        *actionResultProto->mutable_stderr_digest() = stdErrDigest;
        actionResultProto->set_exit_code(123);

        // Add an output file to the response.
        proto::OutputFile outputFile;
        outputFile.set_path("some/path/with/slashes.txt");
        outputFile.mutable_digest()->set_hash("File hash goes here");
        outputFile.mutable_digest()->set_size_bytes(1);
        *actionResultProto->add_output_files() = outputFile;

        // Add an output tree (with nested subdirectories) to the response.
        proto::Tree tree;
        auto treeRootFile = tree.mutable_root()->add_files();
        treeRootFile->mutable_digest()->set_hash("File hash goes here");
        treeRootFile->mutable_digest()->set_size_bytes(1);
        treeRootFile->set_name("out.txt");
        auto childDirectory = tree.add_children();
        auto childDirectoryFile = childDirectory->add_files();
        childDirectoryFile->set_name("a.out");
        childDirectoryFile->mutable_digest()->set_hash("Executable file hash");
        childDirectoryFile->mutable_digest()->set_size_bytes(1);
        childDirectoryFile->set_is_executable(true);
        auto nestedChildDirectory = tree.add_children();
        auto nestedChildDirectoryFile = nestedChildDirectory->add_files();
        nestedChildDirectoryFile->set_name("q.mk");
        nestedChildDirectoryFile->mutable_digest()->set_hash("q.mk file hash");
        nestedChildDirectoryFile->mutable_digest()->set_size_bytes(1);
        *childDirectory->add_directories()->mutable_digest() =
            DigestGenerator::make_digest(*nestedChildDirectory);
        childDirectory->mutable_directories(0)->set_name("nested");
        *tree.mutable_root()->add_directories()->mutable_digest() =
            DigestGenerator::make_digest(*childDirectory);
        tree.mutable_root()->mutable_directories(0)->set_name("subdirectory");
        treeDigest = DigestGenerator::make_digest(tree);
        *actionResultProto->add_output_directories()->mutable_tree_digest() =
            treeDigest;
        actionResultProto->mutable_output_directories(0)->set_path(
            "output/directory");

        // Return a completed Operation when the client sends the Execute
        // request.
        operation.set_done(true);
        operation.mutable_response()->PackFrom(executeResponse);
    }

    ~RemoteExecutionClientTestFixture() {}
};

MATCHER_P(MessageEq, expected, "")
{
    return google::protobuf::util::MessageDifferencer::Equals(expected, arg);
}

TEST_F(RemoteExecutionClientTestFixture, ExecuteActionTest)
{
    std::thread serverHandler([this]() {
        buildboxcommon::GrpcTestServerContext ctx(
            &testServer, "/build.bazel.remote.execution.v2.Execution/Execute");
        ctx.read(expectedExecuteRequest);
        ctx.writeAndFinish(operation);
    });

    // Ask the client to execute the action, and make sure the result is
    // correct.
    std::atomic_bool stop_requested(false);
    const auto actionResult =
        reClient->executeAction(actionDigest, stop_requested);

    serverHandler.join();

    EXPECT_EQ(actionResult.exit_code(), 123);
    EXPECT_FALSE(actionResult.has_stdout_digest());
    EXPECT_TRUE(actionResult.has_stderr_digest());
    EXPECT_EQ(actionResult.stdout_raw(), "Raw stdout.");
    EXPECT_EQ(actionResult.stderr_digest().hash(), stdErrDigest.hash());

    EXPECT_EQ(actionResult.output_files_size(), 1);
    EXPECT_EQ(actionResult.output_files(0).path(),
              "some/path/with/slashes.txt");
    EXPECT_EQ(actionResult.output_files(0).digest().hash(),
              "File hash goes here");

    EXPECT_EQ(actionResult.output_directories_size(), 1);
    EXPECT_EQ(actionResult.output_directories(0).path(), "output/directory");
    EXPECT_EQ(actionResult.output_directories(0).tree_digest().hash(),
              treeDigest.hash());
}

TEST_F(RemoteExecutionClientTestFixture, WriteFilesToDisk)
{
    buildboxcommon::TemporaryDirectory tempDir;

    const std::string testContent("Test file content!");

    proto::ActionResult testResult;
    proto::Digest d = DigestGenerator::make_digest(testContent);
    proto::OutputFile testFile;
    testFile.mutable_digest()->CopyFrom(d);
    testFile.set_path("test.txt");
    testFile.set_is_executable(true);
    *testResult.add_output_files() = testFile;

    // Allow the client to fetch the file from CAS.
    proto::BatchReadBlobsRequest expectedBatchRequest;
    *expectedBatchRequest.add_digests() = testFile.digest();
    proto::BatchReadBlobsResponse batchResponse;
    auto batchEntry = batchResponse.add_responses();
    batchEntry->mutable_digest()->CopyFrom(testFile.digest());
    batchEntry->set_data(testContent);
    EXPECT_CALL(*casStub,
                BatchReadBlobs(_, MessageEq(expectedBatchRequest), _))
        .WillOnce(
            DoAll(SetArgPointee<2>(batchResponse), Return(grpc::Status::OK)));

    reClient->writeFilesToDisk(testResult, tempDir.name());

    const std::string expectedPath = std::string(tempDir.name()) + "/test.txt";
    EXPECT_TRUE(buildboxcommon::FileUtils::isExecutable(expectedPath.c_str()));
    EXPECT_EQ(buildboxcommon::FileUtils::getFileContents(expectedPath.c_str()),
              testContent);
}

TEST_F(RemoteExecutionClientTestFixture, VerifyMetricsWriteFilesToDisk)
{
    buildboxcommon::TemporaryDirectory tempDir;

    const std::string testContent("Test file content!");

    proto::ActionResult testResult;
    proto::Digest d = DigestGenerator::make_digest(testContent);
    proto::OutputFile testFile;
    testFile.mutable_digest()->CopyFrom(d);
    testFile.set_path("test.txt");
    testFile.set_is_executable(true);
    *testResult.add_output_files() = testFile;

    // Allow the client to fetch the file from CAS.
    proto::BatchReadBlobsRequest expectedBatchRequest;
    *expectedBatchRequest.add_digests() = testFile.digest();
    proto::BatchReadBlobsResponse batchResponse;
    auto batchEntry = batchResponse.add_responses();
    batchEntry->mutable_digest()->CopyFrom(testFile.digest());
    batchEntry->set_data(testContent);
    EXPECT_CALL(*casStub,
                BatchReadBlobs(_, MessageEq(expectedBatchRequest), _))
        .WillOnce(
            DoAll(SetArgPointee<2>(batchResponse), Return(grpc::Status::OK)));

    reClient->writeFilesToDisk(testResult, tempDir.name());
    EXPECT_TRUE(
        collectedByName<DurationMetricValue>(TIMER_NAME_FETCH_WRITE_RESULTS));
}

TEST_F(RemoteExecutionClientTestFixture, ActionCacheTestMiss)
{
    EXPECT_CALL(*actionCacheStub, GetActionResult(_, _, _))
        .WillOnce(Return(grpc::Status(grpc::NOT_FOUND, "not found")));
    // `NOT_FOUND` => return false and do not write the ActionResult parameter.

    proto::ActionResult actionResult;
    actionResult.set_exit_code(123);

    proto::ActionResult actionResultOut = actionResult;
    std::set<std::string> outputs;

    const bool in_cache = reClient->fetchFromActionCache(actionDigest, outputs,
                                                         &actionResultOut);

    EXPECT_FALSE(in_cache);
    EXPECT_EQ(actionResultOut.exit_code(), actionResult.exit_code());
}

TEST_F(RemoteExecutionClientTestFixture, ActionCacheTestHit)
{
    EXPECT_CALL(*actionCacheStub, GetActionResult(_, _, _))
        .WillOnce(Return(grpc::Status::OK));
    // Return true and write the ActionResult parameter with the fetched one.

    proto::ActionResult actionResult;
    actionResult.set_exit_code(123);

    proto::ActionResult actionResultOut = actionResult;
    std::set<std::string> outputs;

    const bool in_cache = reClient->fetchFromActionCache(actionDigest, outputs,
                                                         &actionResultOut);

    EXPECT_TRUE(in_cache);
    EXPECT_EQ(actionResultOut.exit_code(), 0);
}

TEST_F(RemoteExecutionClientTestFixture, ActionCacheTestServerError)
{
    EXPECT_CALL(*actionCacheStub, GetActionResult(_, _, _))
        .WillOnce(Return(grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                                      "permission denied")));
    // All other server errors other than `NOT_FOUND` are thrown.

    proto::ActionResult actionResultOut;
    std::set<std::string> outputs;
    ASSERT_THROW(reClient->fetchFromActionCache(actionDigest, outputs,
                                                &actionResultOut),
                 std::runtime_error);
}

TEST_F(RemoteExecutionClientTestFixture, ActionCacheTestMissNoActionResult)
{
    EXPECT_CALL(*actionCacheStub, GetActionResult(_, _, _))
        .WillOnce(Return(grpc::Status(grpc::NOT_FOUND, "not found")));
    // `NOT_FOUND` => return false and do not write the ActionResult parameter.

    std::set<std::string> outputs;
    const bool in_cache =
        reClient->fetchFromActionCache(actionDigest, outputs, nullptr);

    EXPECT_FALSE(in_cache);
}

TEST_F(RemoteExecutionClientTestFixture, ActionCacheHitNoActionResult)
{
    EXPECT_CALL(*actionCacheStub, GetActionResult(_, _, _))
        .WillOnce(Return(grpc::Status::OK));

    std::set<std::string> outputs;

    const bool in_cache =
        reClient->fetchFromActionCache(actionDigest, outputs, nullptr);

    EXPECT_TRUE(in_cache);
}
