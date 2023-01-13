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

#ifndef INCLUDED_ENV
#define INCLUDED_ENV

#include <deque>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace recc {

/**
 * The URI of the server to use, e.g. http://localhost:8085
 */
extern std::string RECC_SERVER;

/**
 * The URI of the CAS server to use. By default, uses RECC_SERVER.
 */
extern std::string RECC_CAS_SERVER;

/**
 * Whether to issue a `GetCapabilities()` request to the CAS server.
 */
extern bool RECC_CAS_GET_CAPABILITIES;

/**
 * Digest function to use to calculate Digests of blobs in CAS.
 */
extern std::string RECC_CAS_DIGEST_FUNCTION;

/**
 * The URI of the action cache server to use. By default, uses
 * RECC_CAS_SERVER if set or RECC_SERVER if not.
 */
extern std::string RECC_ACTION_CACHE_SERVER;

/**
 * The instance name to pass to the server. The default is the empty
 * std::string.
 */
extern std::string RECC_INSTANCE;

/**
 * If set, the contents of this directory (and its subdirectories) will be sent
 * to the worker.
 *
 * If both this and RECC_DEPS_OVERRIDE are set, the directory value is used.
 */
extern std::string RECC_DEPS_DIRECTORY_OVERRIDE;

/**
 * The root of the project.
 *
 * Only files inside the project root will be rewritten to relative paths.
 */
extern std::string RECC_PROJECT_ROOT;

/**
 * The file or UDP Host:Port to write metrics to.
 * Empty string for both indicates stderr.
 * Only taken into account when RECC_ENABLE_METRICS is true.
 */
extern std::string RECC_METRICS_FILE;
extern std::string RECC_METRICS_UDP_SERVER;

/**
 * Disable relative path rewriting for recc.
 */
extern bool RECC_NO_PATH_REWRITE;

/**
 * If set, recc will report all entries returned by the dependency command
 * even if they are absolute paths.
 */
extern bool RECC_DEPS_GLOBAL_PATHS;

/**
 * The location to store temporary files. (Currently used only by the tests.)
 */
extern std::string TMPDIR;

/**
 * Maximum level of displayed log messages.
 */
extern std::string RECC_LOG_LEVEL;

/**
 * Location to store files with log messages (instead of stderr).
 */
extern std::string RECC_LOG_DIRECTORY;

/**
 * Enables verbose output, which is logged to stderr.
 */
extern bool RECC_VERBOSE;

/**
 * Only calculate the dependencies of the command and output the action digest.
 */
extern bool RECC_NO_EXECUTE;

/**
 * Enables metric collection to the place specified by RECC_METRICS_FILE.
 */
extern bool RECC_ENABLE_METRICS;

/**
 * Sends the command to the build server, even if deps doesn't think it's a
 * compiler command.
 */
extern bool RECC_FORCE_REMOTE;

/**
 * Only try to fetch from the cache, and build locally if no results
 * are available.
 */
extern bool RECC_CACHE_ONLY;

/**
 * Upload action result to action cache after local build.
 */
extern bool RECC_CACHE_UPLOAD_LOCAL_BUILD;

/**
 * Sets the `do_not_cache` flag in the Action to indicate that it can never be
 * cached.
 */
extern bool RECC_ACTION_UNCACHEABLE;

/**
 * Sets the `skip_cache_lookup` flag in the ExecuteRequest to re-run the action
 * instead of looking it up in the cache.
 */
extern bool RECC_SKIP_CACHE;

/**
 * Prevents compilation output from being saved to disk.
 */
extern bool RECC_DONT_SAVE_OUTPUT;

/**
 * If true, cache action results even with non-zero subprocess exit codes
 */
extern bool RECC_CACHE_UPLOAD_FAILED_BUILD;

/**
 * Use Google's authentication to talk to the build server. Also applies to the
 * CAS server. Not setting this implies insecure communication.
 */
extern bool RECC_SERVER_AUTH_GOOGLEAPI;

/**
 * The maximum number of times to retry an RPC call before failing.
 */
extern int RECC_RETRY_LIMIT;

/**
 * The base delay between retries. If the first request is request 0,
 * the delay between request n and request n+1 is RECC_RETRY_DELAY * 2^n.
 */
extern int RECC_RETRY_DELAY;

/**
 * The maximum amount of time to wait on for each gRPC request, in seconds.
 * 0 indicates no deadline.
 */
extern int RECC_REQUEST_TIMEOUT;

/**
 * The period for gRPC keepalive pings, in seconds.
 * 0 indicates no keepalive pings.
 */
extern int RECC_KEEPALIVE_TIME;

/**
 * Use a secure SSL/TLS channel to talk to the execution and CAS servers.
 * (deprecated, but forces URLs missing protocol to be prefixed with https://)
 */
extern bool RECC_SERVER_SSL;

/**
 * Preserve the client's environment variables in the build process in the
 * remote, except variables prefixed with "RECC_." Preserved environment
 * variables can be explicitly overwritten with the RECC_REMOTE_ENV_<VAR>
 * option.
 */
extern bool RECC_PRESERVE_ENV;

/**
 * File path to Access Token to use for server/cas/action_cache requests
 */
extern std::string RECC_ACCESS_TOKEN_PATH;

/**
 * Customisable msg to display to indicate that authentication has not been
 * configured.
 */
extern std::string RECC_AUTH_UNCONFIGURED_MSG;

/**
 * Defined by cmake when building.
 * On linux defaults to: /usr/local
 */
extern std::string RECC_INSTALL_DIR;

/**
 * Optionally defined when compiling using
 * -DRECC_CONFIG_PREFIX_DIR=/path/to/prefix/dir
 *
 */
extern std::string RECC_CUSTOM_PREFIX;

/**
 * A comma-separated list of input file paths to send to the build server. If
 * this isn't set, deps is called to determine the input files.
 */

BUILDBOX_LOG_DEBUG("RECC_REAPI_VERSION Env LOG4");
extern std::set<std::string> RECC_DEPS_OVERRIDE;

/**
 * A comma-separated list of output file paths to request from the build
 * server. If this isn't set, deps attempts to guess the output files by
 * examining the command.
 */
extern std::set<std::string> RECC_OUTPUT_FILES_OVERRIDE;

/**
 * A comma-separated list of output directories to request from the build
 * server. If this is set, deps isn't called.
 */
extern std::set<std::string> RECC_OUTPUT_DIRECTORIES_OVERRIDE;

/**
 * A comma-separated list of directory prefixes to be excluded from the input
 * dependency list. The immediate usage for this allows recc to not include
 * system header files as part of the input root
 */
extern std::set<std::string> RECC_DEPS_EXCLUDE_PATHS;

/**
 * A comma-separated list of environment variables to read from the local
 * machine, which will be passed in the environment to the worker. This can be
 * used to preserve variables starting with "RECC_," which RECC_PRESERVE_ENV
 * does not do.
 */
extern std::set<std::string> RECC_ENV_TO_READ;

// Maps are given by passing an environment variable for each item in the map.
// For example, RECC_REMOTE_ENV_PATH=/usr/bin could be used to specify the PATH
// passed to the remote build server.

/**
 * Environment variables to apply to dependency commands, which are run on the
 * local machine.
 */
extern std::map<std::string, std::string> RECC_DEPS_ENV;

/**
 * Environment variables to send to the build server. For example, you can set
 * the remote server's PATH using RECC_REMOTE_ENV_PATH=/usr/bin. Overwites
 * variables preserved with RECC_PRESERVE_ENV and RECC_ENV_TO_READ.
 */
extern std::map<std::string, std::string> RECC_REMOTE_ENV;

/**
 * Platform requirements to send to the build server. For example, you can
 * require x86_64 using RECC_REMOTE_PLATFORM_arch=x86_64
 */
extern std::map<std::string, std::string> RECC_REMOTE_PLATFORM;

/**
 * Only gets defined if RECC_PREFIX_MAP is populated.
 * Contains pairs of the prefixes in the order defined by RECC_PREFIX_MAP.
 */
extern std::vector<std::pair<std::string, std::string>>
    RECC_PREFIX_REPLACEMENT;

/**
 * Used to specify absolute paths for finding recc.conf.
 * If specifying absolute path, only include up until directory containing
 * config, no trailing "/". Additions to the list should be in order of
 * priority: LEAST-> MOST important.
 *
 * Additional locations based on the runtime environment are included in:
 * env.cpp:find_and_parse_config_files()
 */
extern std::deque<std::string> RECC_CONFIG_LOCATIONS;

/**
 * Value sent as part of the optional RequestMetadata header values attached
 * to requests.
 */
extern std::string RECC_CORRELATED_INVOCATIONS_ID;

/**
 * Prefix directory to be prepended to the working directory, as well as
 * all input paths relative to it.
 */
extern std::string RECC_WORKING_DIR_PREFIX;

/**
 * Specify the maximum number of system threads available to the recc process.
 * -1 specifies use as many system threads as cores
 */
extern int RECC_MAX_THREADS;

/**
 * Version of the Remote Execution API to use.
 */
extern std::string RECC_REAPI_VERSION;

/**
 * Optional salt value to place the `Action` into a separate cache namespace.
 */
extern std::string RECC_ACTION_SALT;

/**
 * The process environment.
 */
extern "C" char **environ;

struct Env {
    /**
     * Parse the given environment and store it in the corresponding global
     * variables.
     *
     * environ should be an array of "VARIABLE=value" std::strings whose last
     * entry is nullptr.
     */
    static void parse_config_variables(const char *const *environ);

    /**
     * Parse the environment variables related to logging, set up the
     * buildbox logger, and initialize it. (Must be called exactly once from
     * `main()`).
     *
     * The `programName` argument should be set to argv[0].
     *
     * Note that this method must be called before using the logging macros or
     * invoking functions that do so.
     */
    static void setup_logger_from_environment(const char *programName);

    /**
     * Finds config files specified in RECC_CONFIG_LOCATIONS and passes
     * variables to parse_config_variables
     */
    static void find_and_parse_config_files();

    /**
     * Handles the case that RECC_SERVER and RECC_CAS_SERVER have not been set.
     */
    static void handle_special_defaults();

    /**
     * Asserts that RECC_REAPI_VERSION is set to a valid value.
     */
    static void assert_reapi_version_is_valid();

    /**
     * Verifies that the files referred to in the configuration can be actually
     * written to.
     */
    static void verify_files_writeable();

    /*
     * Evaluates ENV and Returns a prioritized deque with the config locations
     * as follows:
     *  1. ${cwd}/recc
     *  2. ~/.recc
     *  3. ${RECC_CONFIG_PREFIX_DIR}
     *  4. ${INSTALL_DIR}/../etc/recc
     */
    static std::deque<std::string> evaluate_config_locations();

    /**
     * Given a string, return a vector of pairs containing key=value pairs of
     *the string split at the delimiter, key/values split by second delimiter.
     * Default delimiters = ":", "=".
     * Ex. recc=build:build=recc will return a vector [(recc, build),(build,
     *recc)]
     **/
    static std::vector<std::pair<std::string, std::string>>
    vector_from_delimited_string(std::string prefix_map,
                                 const std::string &first_delimiter = ":",
                                 const std::string &second_delimiter = "=");

    /**
     * Sets the prioritized configuration file locations from
     * evaluate_config_locations() -- default ordering
     */
    static void set_config_locations();

    /**
     * Sets the prioritized configuration file locations as specified
     * in config_order
     */
    static void
    set_config_locations(const std::deque<std::string> &config_order);

    /**
     * Return a substring ending at the nth occurrence of a character. If a nth
     * occurrence isn't found, return empty string.
     */
    static std::string
    substring_until_nth_token(const std::string &value,
                              const std::string &character,
                              const std::string::size_type &pos);

    /**
     * Adds a default protocol prefix to the server URL from the config, if
     * missing and prints a message telling the user to update their configs To
     * be deprecated.
     */
    static const std::string backwardsCompatibleURL(const std::string &url);

    /**
     * Calculate and set the config locations, parse the config files from
     * those, then parse the environment variables for overrides and run some
     * sanity checks (handle_special_defaults) on the resulting config. Takes
     * optional parameter specifying source file which calls it, to pass to
     * handle_special_defaults
     */
    static void parse_config_variables();

    static std::pair<int, int>
    version_string_to_pair(const std::string &version);

    static bool configured_reapi_version_equal_to_or_newer_than(
        const std::string &version);
};

} // namespace recc

#endif
