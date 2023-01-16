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

// bin/recc.cpp
//
// Runs a build command remotely. If the given command is not a build command,
// it's actually run locally.

#include <signal.h>
#include <string>

#include <buildboxcommon_grpcerror.h>
#include <buildboxcommon_logging.h>

#include <digestgenerator.h>
#include <env.h>
#include <executioncontext.h>
#include <reccdefaults.h>
#include <remoteexecutionsignals.h>
#include <requestmetadata.h>

using namespace recc;

namespace {

/**
 * NOTE: If a variable is intended to be used in a configuration file, omit the
 * "RECC_" prefix.
 */
const std::string HELP(
    "USAGE: recc <command>\n"
    "\n"
    "If the given command is a compile command, runs it on a remote build\n"
    "server. Otherwise, runs it locally.\n"
    "\n"
    "If the command is to be executed remotely, it must specify either a \n"
    "relative or absolute path to an executable.\n"
    "\n"
    "The following environment variables can be used to change recc's\n"
    "behavior. To set them in a recc.conf file, omit the \"RECC_\" prefix.\n"
    "\n"
    "RECC_SERVER - the URI of the server to use (e.g. http://localhost:8085)\n"
    "\n"
    "RECC_CAS_SERVER - the URI of the CAS server to use (by default, \n"
    "                  uses RECC_ACTION_CACHE_SERVER if set. Else "
    "RECC_SERVER)\n"
    "\n"
    "RECC_ACTION_CACHE_SERVER - the URI of the Action Cache server to use (by "
    "default,\n"
    "                  uses RECC_CAS_SERVER. Else RECC_SERVER)\n"
    "\n"
    "RECC_INSTANCE - the instance name to pass to the server (defaults "
    "to \"" DEFAULT_RECC_INSTANCE "\") \n"
    "\n"
    "RECC_CACHE_ONLY - if set to any value, runs recc in cache-only mode. In\n"
    "                  this mode, recc will build anything not available in \n"
    "                  the remote cache locally, rather than failing to "
    "build.\n"
    "\n"
    "RECC_CACHE_UPLOAD_FAILED_BUILD - Upload action results regardless of the "
    "exit\n"
    "                                 code of the sub-process executing the "
    "action.\n"
    "                                 This setting defaults to true. If set "
    "to false\n"
    "                                 only successful action results(exit "
    "codes equal to zero)\n"
    "                                 will be uploaded.\n"
    "\n"
    "RECC_PROJECT_ROOT - the top-level directory of the project source.\n"
    "                    If the command contains paths inside the root, they\n"
    "                    will be rewritten to relative paths (by default, \n"
    "                    uses the current working directory)\n"
    "\n"
    "RECC_SERVER_AUTH_GOOGLEAPI - use default google authentication when\n"
    "                             communicating over gRPC, instead of\n"
    "                             using an insecure connection\n"
    "\n"
    "RECC_ACCESS_TOKEN_PATH - path specifying location of access token (JWT, "
    "OAuth, etc) to be attached to all secure connections.\n"
    "                         Defaults to \"" DEFAULT_RECC_ACCESS_TOKEN_PATH
    "\"\n"
    "RECC_LOG_LEVEL - logging verbosity level [optional, default "
    "= " DEFAULT_RECC_LOG_LEVEL ", supported = " +
    buildboxcommon::logging::stringifyLogLevels() +
    "] \n"
    "RECC_LOG_DIRECTORY - if set to a directory, output log messages to files "
    "in that location\n"
    "\n"
    "RECC_VERBOSE - if set to any value, equivalent to RECC_LOG_LEVEL=debug\n"
    "\n"
    "RECC_ENABLE_METRICS - if set to any value, enable metric collection \n"
    "\n"
    "RECC_METRICS_FILE - write metrics to that file (Default/Empty string — "
    "stderr). Cannot be used with RECC_METRICS_UDP_SERVER.\n"
    "\n"
    "RECC_METRICS_UDP_SERVER - write metrics to the specified host:UDP_Port.\n"
    " Cannot be used with RECC_METRICS_FILE\n"
    "\n"
    "RECC_NO_PATH_REWRITE - if set to any value, do not rewrite absolute "
    "paths to be relative.\n"
    "\n"
    "RECC_FORCE_REMOTE - if set to any value, send all commands to the \n"
    "                    build server. (Non-compile commands won't be \n"
    "                    executed locally, which can cause some builds to \n"
    "                    fail.)\n"
    "\n"
    "RECC_ACTION_UNCACHEABLE - if set to any value, sets `do_not_cache` \n"
    "                          flag to indicate that the build action can \n"
    "                          never be cached\n"
    "\n"
    "RECC_SKIP_CACHE - if set to any value, sets `skip_cache_lookup` flag \n"
    "                  to re-run the build action instead of looking it up \n"
    "                  in the cache\n"
    "\n"
    "RECC_DONT_SAVE_OUTPUT - if set to any value, prevent build output from \n"
    "                        being saved to local disk\n"
    "\n"
    "RECC_DEPS_GLOBAL_PATHS - if set to any value, report all entries \n"
    "                         returned by the dependency command, even if \n"
    "                         they are absolute paths\n"
    "\n"
    "RECC_DEPS_OVERRIDE - comma-separated list of files to send to the\n"
    "                     build server (by default, run `deps` to\n"
    "                     determine this)\n"
    "\n"
    "RECC_DEPS_DIRECTORY_OVERRIDE - directory to send to the build server\n"
    "                               (if both this and RECC_DEPS_OVERRIDE\n"
    "                               are set, this one is used)\n"
    "\n"
    "RECC_OUTPUT_FILES_OVERRIDE - comma-separated list of files to\n"
    "                             request from the build server (by\n"
    "                             default, `deps` guesses)\n"
    "\n"
    "RECC_OUTPUT_DIRECTORIES_OVERRIDE - comma-separated list of\n"
    "                                   directories to request (by\n"
    "                                   default, `deps` guesses)\n"
    "\n"
    "RECC_DEPS_EXCLUDE_PATHS - comma-separated list of paths to exclude from\n"
    "                          the input root\n"
    "\n"
    "RECC_DEPS_ENV_[var] - sets [var] for local dependency detection\n"
    "                      commands\n"
    "\n"
    "RECC_PRESERVE_ENV - if set to any value, preserve all non-recc \n"
    "                    environment variables in the remote"
    "\n"
    "RECC_ENV_TO_READ - comma-separated list of specific environment \n"
    "                       variables to preserve from the local environment\n"
    "                       (can be used to preserve RECC_ variables, unlike\n"
    "                       RECC_PRESERVE_ENV)\n"
    "\n"
    "RECC_REMOTE_ENV_[var] - sets [var] in the remote build environment\n"
    "\n"
    "RECC_REMOTE_PLATFORM_[key] - specifies a platform property,\n"
    "                             which the build server uses to select\n"
    "                             the build worker\n"
    "\n"
    "RECC_RETRY_LIMIT - number of times to retry failed requests (default "
    "0).\n"
    "\n"
    "RECC_RETRY_DELAY - base delay (in ms) between retries\n"
    "                   grows exponentially (default 1000ms)\n"
    "\n"
    "RECC_REQUEST_TIMEOUT - how long to wait for gRPC request responses\n"
    "                       in seconds. (default: no timeout))\n"
    "\n"
    "RECC_KEEPALIVE_TIME - period for gRPC keepalive pings\n"
    "                      in seconds. (default: no keepalive pings))\n"
    "\n"
    "RECC_PREFIX_MAP - specify path mappings to replace. The source and "
    "destination must both be absolute paths. \n"
    "Supports multiple paths, separated by "
    "colon(:). Ex. RECC_PREFIX_MAP=/usr/bin=/usr/local/bin)\n"
    "\n"
    "RECC_CAS_DIGEST_FUNCTION - specify what hash function to use to "
    "calculate digests.\n"
    "                           (Default: "
    "\"" DEFAULT_RECC_CAS_DIGEST_FUNCTION "\")\n"
    "                           Supported values: " +
    DigestGenerator::supportedDigestFunctionsList() +
    "\n\n"
    "RECC_WORKING_DIR_PREFIX - directory to prefix the command's working\n"
    "                          directory, and input paths relative to it\n"
    "RECC_MAX_THREADS -   Allow some operations to utilize multiple cores."
    "Default: 4 \n"
    "                     A value of -1 specifies use all available cores.\n"
    "RECC_REAPI_VERSION - Version of the Remote Execution API to use. "
    "(Default: \"" DEFAULT_RECC_REAPI_VERSION "\")\n"
    "                     Supported values: " +
    proto::reapiSupportedVersionsList() +
    "\n"
    "RECC_NO_EXECUTE    - If set, only attempt to build an Action and "
    "calculate its digest,\n"
    "                     without running the command");

enum ReturnCode {
    RC_OK = 0,
    RC_USAGE = 100,
    RC_EXEC_FAILURE = 101,
    RC_GRPC_ERROR = 102
};

} // namespace

static std::atomic_bool s_sigintReceived(false);

/**
 * Signal handler to mark the remote execution task for cancellation
 */
static void setSigintReceived(int) { s_sigintReceived = true; }

int main(int argc, char *argv[])
{
    //--deps 
    Env::setup_logger_from_environment(argv[0]);

    if (argc <= 1) {
        std::cerr << "USAGE: recc <command>" << std::endl;
        std::cerr << "(run \"recc --help\" for details)" << std::endl;
        return RC_USAGE;
    }
    else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        std::cout << HELP << std::endl;
        return RC_OK;
    }
    else if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        const std::string version =
            RequestMetadataGenerator::RECC_METADATA_TOOL_VERSION;
        const std::string versionMessage = "recc version: " + version;
        std::cout << versionMessage << std::endl;
        return RC_OK;
    }
    else if (strcmp(argv[1], "--deps") == 0) {
        std::cerr << "recc: recognized custome compiler option '" << argv[1] << "'"
                  << std::endl;
        std::cerr << "USAGE: recc --deps <command>" << std::endl;
       // std::cerr << "(run \"recc --help\" for details)" << std::endl;
        return RC_OK;
    }
    else if (argv[1][0] == '-') {
        std::cerr << "recc: unrecognized option '" << argv[1] << "'"
                  << std::endl;
        std::cerr << "USAGE: recc <command>" << std::endl;
        std::cerr << "(run \"recc --help\" for details)" << std::endl;
        return RC_USAGE;

    }
    

    Signal::setup_signal_handler(SIGINT, setSigintReceived);

    try {
        // Parsing of recc options is complete. The remaining arguments are the
        // compiler command line.
       /* if(argv[2][0]!=NULL){
            const auto parsedCommand =
            ParsedCommandFactory::createParsedCommand(&argv[1], cwd.c_str());
            const auto deps = Deps::get_file_info(parsedCommand).d_dependencies;
            for (const auto &dep : deps) {
                BUILDBOX_LOG_INFO(dep);
        }*/
        ExecutionContext context;
        context.setStopToken(s_sigintReceived);
        return context.execute(argc - 1, &argv[1]);
    }
    catch (const std::invalid_argument &e) {
        return RC_USAGE;
    }
    catch (const buildboxcommon::GrpcError &e) {
        if (e.status.error_code() == grpc::StatusCode::CANCELLED) {
            exit(130); // Ctrl+C exit code
        }
        return RC_GRPC_ERROR;
    }
    catch (const std::exception &e) {
        return RC_EXEC_FAILURE;
    }
}
