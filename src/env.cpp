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

#include <env.h>

#include <buildboxcommon_exception.h>
#include <buildboxcommon_fileutils.h>
#include <buildboxcommon_logging.h>
#include <buildboxcommon_protos.h>

#include <algorithm>
#include <cstring>
#include <ctype.h>
#include <digestgenerator.h>
#include <env.h>
#include <fileutils.h>

#include <fstream>
#include <functional>
#include <iostream>
#include <reccdefaults.h>
#include <regex>
#include <sstream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>
BUILDBOX_LOG_DEBUG("RECC_REAPI_VERSION ENV LOG1");
namespace recc {

// Leave these empty so that parse_config_variables can print warnings if not
// specified
std::string RECC_SERVER = "";
std::string RECC_CAS_SERVER = "";
std::string RECC_ACTION_CACHE_SERVER = "";

// Include default values for the following, no need to print warnings if not
// specified
std::string RECC_INSTANCE = DEFAULT_RECC_INSTANCE;
std::string RECC_DEPS_DIRECTORY_OVERRIDE =
    DEFAULT_RECC_DEPS_DIRECTORY_OVERRIDE;
std::string RECC_PROJECT_ROOT = DEFAULT_RECC_PROJECT_ROOT;
std::string TMPDIR = DEFAULT_RECC_TMPDIR;
std::string RECC_ACCESS_TOKEN_PATH = DEFAULT_RECC_ACCESS_TOKEN_PATH;
std::string RECC_CORRELATED_INVOCATIONS_ID =
    DEFAULT_RECC_CORRELATED_INVOCATIONS_ID;
std::string RECC_METRICS_FILE = DEFAULT_RECC_METRICS_FILE;
std::string RECC_METRICS_UDP_SERVER = DEFAULT_RECC_METRICS_UDP_SERVER;
bool RECC_NO_PATH_REWRITE = DEFAULT_RECC_NO_PATH_REWRITE;
std::string RECC_PREFIX_MAP = DEFAULT_RECC_PREFIX_MAP;
std::vector<std::pair<std::string, std::string>> RECC_PREFIX_REPLACEMENT;

std::string RECC_CAS_DIGEST_FUNCTION = DEFAULT_RECC_CAS_DIGEST_FUNCTION;
std::string RECC_WORKING_DIR_PREFIX = DEFAULT_RECC_WORKING_DIR_PREFIX;
std::string RECC_ACTION_SALT = DEFAULT_RECC_ACTION_SALT;

bool RECC_NO_EXECUTE = false;
bool RECC_ENABLE_METRICS = DEFAULT_RECC_ENABLE_METRICS;
bool RECC_FORCE_REMOTE = DEFAULT_RECC_FORCE_REMOTE;
bool RECC_CACHE_ONLY = DEFAULT_RECC_CACHE_ONLY;
bool RECC_CACHE_UPLOAD_LOCAL_BUILD = DEFAULT_RECC_CACHE_UPLOAD_LOCAL_BUILD;
bool RECC_ACTION_UNCACHEABLE = DEFAULT_RECC_ACTION_UNCACHEABLE;
bool RECC_SKIP_CACHE = DEFAULT_RECC_SKIP_CACHE;
bool RECC_DONT_SAVE_OUTPUT = DEFAULT_RECC_DONT_SAVE_OUTPUT;
bool RECC_CACHE_UPLOAD_FAILED_BUILD = DEFAULT_RECC_CACHE_UPLOAD_FAILED_BUILD;
bool RECC_SERVER_AUTH_GOOGLEAPI = DEFAULT_RECC_SERVER_AUTH_GOOGLEAPI;
bool RECC_SERVER_SSL =
    DEFAULT_RECC_SERVER_SSL; // deprecated: inferred from URL
bool RECC_DEPS_GLOBAL_PATHS = DEFAULT_RECC_DEPS_GLOBAL_PATHS;
std::string RECC_LOG_LEVEL = DEFAULT_RECC_LOG_LEVEL;
std::string RECC_LOG_DIRECTORY = DEFAULT_RECC_LOG_DIRECTORY;
bool RECC_VERBOSE = DEFAULT_RECC_VERBOSE;
bool RECC_CAS_GET_CAPABILITIES = false;
bool RECC_PRESERVE_ENV = false;

int RECC_RETRY_LIMIT = DEFAULT_RECC_RETRY_LIMIT;
int RECC_RETRY_DELAY = DEFAULT_RECC_RETRY_DELAY;
int RECC_REQUEST_TIMEOUT = DEFAULT_RECC_REQUEST_TIMEOUT;
int RECC_KEEPALIVE_TIME = DEFAULT_RECC_KEEPALIVE_TIME;

// Hidden variables (not displayed in the help string)
std::string RECC_AUTH_UNCONFIGURED_MSG = DEFAULT_RECC_AUTH_UNCONFIGURED_MSG;

#ifdef CMAKE_INSTALL_DIR
std::string RECC_INSTALL_DIR = std::string(CMAKE_INSTALL_DIR);
#else
std::string RECC_INSTALL_DIR = std::string("");
#endif

#ifdef RECC_CONFIG_PREFIX_DIR
std::string RECC_CUSTOM_PREFIX = std::string(RECC_CONFIG_PREFIX_DIR);
#else
std::string RECC_CUSTOM_PREFIX = std::string("");
#endif

std::set<std::string> RECC_DEPS_OVERRIDE = DEFAULT_RECC_DEPS_OVERRIDE;
std::set<std::string> RECC_OUTPUT_FILES_OVERRIDE =
    DEFAULT_RECC_OUTPUT_FILES_OVERRIDE;
std::set<std::string> RECC_OUTPUT_DIRECTORIES_OVERRIDE =
    DEFAULT_RECC_OUTPUT_DIRECTORIES_OVERRIDE;
std::set<std::string> RECC_DEPS_EXCLUDE_PATHS =
    DEFAULT_RECC_DEPS_EXCLUDE_PATHS;
std::set<std::string> RECC_ENV_TO_READ = DEFAULT_RECC_ENV_TO_READ;

std::map<std::string, std::string> RECC_DEPS_ENV = DEFAULT_RECC_DEPS_ENV;
std::map<std::string, std::string> RECC_REMOTE_ENV = DEFAULT_RECC_REMOTE_ENV;
std::map<std::string, std::string> RECC_REMOTE_PLATFORM =
    DEFAULT_RECC_REMOTE_PLATFORM;

// Keep this empty initially and have set_config_locations() populate it
std::deque<std::string> RECC_CONFIG_LOCATIONS = {};
int RECC_MAX_THREADS = DEFAULT_RECC_MAX_THREADS;

std::string RECC_REAPI_VERSION = DEFAULT_RECC_REAPI_VERSION;

namespace {

/**
 * Call "change_case" on each character in the string "value". By default
 * converting each character to uppercase.
 *
 * Meant to be used with ::toupper and ::tolower
 */
void to_case(std::function<int(int)> change_case, std::string *const value,
             const std::string::size_type &start_pos,
             const std::string::size_type &end_pos)
{

    transform(
        value->cbegin() + static_cast<std::string::difference_type>(start_pos),
        value->cbegin() + static_cast<std::string::difference_type>(end_pos),
        value->begin(), change_case);
}

/**
 * Make a copy of input string, but stripping passed in char
 */
std::string stripChar(const std::string &str, const char value)
{
    std::string tmp;
    tmp.resize(str.size());
    auto it = std::copy_if(str.begin(), str.end(), tmp.begin(),
                           [value](char c) { return (c != value); });
    tmp.resize(static_cast<size_t>(std::distance(tmp.begin(), it)));
    return tmp;
}

/**
 * Parse a 'sep' delimited list, storing its items in the given set.
 */
void parse_set(const char *str, std::set<std::string> *result, const char sep)
{
    const char escape = '\\';
    while (true) {
        const char *cur_delim = strchr(str, sep);
        if (cur_delim == nullptr) {
            result->insert(std::string(str));
            return;
        }
        else {
            // if we see an escaped delimiter, scan past all such occurances
            // avoiding invalid memory reads of memory before pointer
            // 'str', ie; ","
            if ((cur_delim > str) && *(cur_delim - 1) == escape) {
                do {
                    cur_delim = strchr(cur_delim + 1, sep);
                } while (cur_delim != nullptr && *(cur_delim - 1) == escape);
                if (cur_delim == nullptr) {
                    result->insert(stripChar(str, escape));
                    return;
                }
            }

            std::string tmp(str, static_cast<size_t>(cur_delim - str));
            result->insert(stripChar(tmp, escape));
            str = cur_delim + 1;
        }
    }
}

/**
 * Formats line to be used in parse_config_variables.
 * Modifies parameter passed to it by pointer.
 */
void format_config_string(std::string *const line)
{
    std::string::size_type start_pos = 0;
    std::string::size_type end_pos = line->find('=');
    std::string map_key = Env::substring_until_nth_token(*line, "_", 2);
    // Convert the map key to lowercase when checking for property names.
    to_case(::tolower, &map_key, start_pos, map_key.size());

    // Handle map configuration variables. Only convert keys, not value.
    if (map_key == "remote_platform" || map_key == "deps_env" ||
        map_key == "remote_env") {
        end_pos = map_key.size();
    }

    // Convert the entire key to uppercase as expected by
    // parse_config_variables.
    to_case(::toupper, line, start_pos, end_pos);
    const std::string tmpdirConfig = "TMPDIR";

    // prefix "RECC_" to name, unless name is TMPDIR
    if (line->substr(0, tmpdirConfig.length()) != tmpdirConfig) {
        *line = "RECC_" + *line;
    }
}

/*
 * Parse the config variables, and pass to parse_config_variables
 */
void parse_config_files(const std::string &config_file_name)
{
    std::ifstream config(config_file_name);
    std::string line;
    std::vector<std::string> env_array;
    std::vector<char *> env_cstrings;

    while (getline(config, line)) {
        if (line.empty() || isspace(line[0]) || line[0] == '#') {
            continue;
        }
        format_config_string(&line);
        env_array.push_back(line);
    }
    // first push std::strings into vector, then push_back char *
    // done for easy const char** conversion
    for (const std::string &i : env_array) {
        env_cstrings.push_back(const_cast<char *>(i.c_str()));
    }
    env_cstrings.push_back(nullptr);
    Env::parse_config_variables(env_cstrings.data());
}

} // namespace

// clang-format off

// https://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Pragmas.html
// Suppress unused parameter warnings and sign conversion warnings stemming from the helper macros below.
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wsign-conversion\"")
_Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")

void Env::parse_config_variables(const char *const *env)
{
#define VARS_START()                                                          \
    if (strncmp(env[i], "RECC_", 4) != 0 &&                                   \
        strncmp(env[i], "TMPDIR", 6) != 0) {                                  \
        continue;                                                             \
    }
#define STRVAR(name)                                                          \
    else if (strncmp(env[i], #name "=", strlen(#name "=")) == 0)              \
    {                                                                         \
        name = std::string(env[i] + strlen(#name "="));                       \
    }
#define BOOLVAR(name)                                                         \
    else if (strncmp(env[i], #name "=", strlen(#name "=")) == 0)              \
    {                                                                         \
        name = strlen(env[i]) > strlen(#name "=");                            \
    }
#define INTVAR(name)                                                          \
    else if (strncmp(env[i], #name "=", strlen(#name "=")) == 0)              \
    {                                                                         \
        name = std::stoi(std::string(env[i] + strlen(#name "=")));            \
    }
#define SETVAR(name, delim)                                                   \
    else if (strncmp(env[i], #name "=", strlen(#name "=")) == 0)              \
    {                                                                         \
        parse_set(env[i] + strlen(#name "="), &name, delim);                  \
    }
#define MAPVAR(name)                                                          \
    else if (strncmp(env[i], #name "_", strlen(#name "_")) == 0)              \
    {                                                                         \
        const char *equals = strchr(env[i], '=');                             \
        std::string key(env[i] + strlen(#name "_"),                           \
                        equals - env[i] - strlen(#name "_"));                 \
        name[key] = std::string(equals + 1);                                  \
    }

    // Parse all the options from ENV
    for (int i = 0; env[i] != nullptr; ++i) {
        VARS_START()
        STRVAR(RECC_SERVER)
        STRVAR(RECC_CAS_SERVER)
        STRVAR(RECC_ACTION_CACHE_SERVER)
        STRVAR(RECC_INSTANCE)
        STRVAR(RECC_DEPS_DIRECTORY_OVERRIDE)
        STRVAR(RECC_PROJECT_ROOT)
        STRVAR(TMPDIR)
        STRVAR(RECC_ACCESS_TOKEN_PATH)
        STRVAR(RECC_AUTH_UNCONFIGURED_MSG)
        STRVAR(RECC_CORRELATED_INVOCATIONS_ID)
        STRVAR(RECC_METRICS_FILE)
        STRVAR(RECC_METRICS_UDP_SERVER)
        STRVAR(RECC_PREFIX_MAP)
        STRVAR(RECC_CAS_DIGEST_FUNCTION)
        STRVAR(RECC_WORKING_DIR_PREFIX)
        STRVAR(RECC_REAPI_VERSION)
        STRVAR(RECC_ACTION_SALT)

        BOOLVAR(RECC_NO_EXECUTE)
        BOOLVAR(RECC_ENABLE_METRICS)
        BOOLVAR(RECC_FORCE_REMOTE)
        BOOLVAR(RECC_CACHE_ONLY)
        BOOLVAR(RECC_CACHE_UPLOAD_LOCAL_BUILD)
        BOOLVAR(RECC_ACTION_UNCACHEABLE)
        BOOLVAR(RECC_SKIP_CACHE)
        BOOLVAR(RECC_DONT_SAVE_OUTPUT)
        BOOLVAR(RECC_CACHE_UPLOAD_FAILED_BUILD)
        BOOLVAR(RECC_SERVER_AUTH_GOOGLEAPI)
        BOOLVAR(RECC_SERVER_SSL)
        BOOLVAR(RECC_DEPS_GLOBAL_PATHS)
        BOOLVAR(RECC_CAS_GET_CAPABILITIES)
        BOOLVAR(RECC_PRESERVE_ENV)
        BOOLVAR(RECC_NO_PATH_REWRITE)

        INTVAR(RECC_RETRY_LIMIT)
        INTVAR(RECC_RETRY_DELAY)
        INTVAR(RECC_REQUEST_TIMEOUT)
        INTVAR(RECC_KEEPALIVE_TIME)
        INTVAR(RECC_MAX_THREADS)

        SETVAR(RECC_DEPS_OVERRIDE, ',')
        SETVAR(RECC_OUTPUT_FILES_OVERRIDE, ',')
        SETVAR(RECC_OUTPUT_DIRECTORIES_OVERRIDE, ',')
        SETVAR(RECC_DEPS_EXCLUDE_PATHS, ',')
        SETVAR(RECC_ENV_TO_READ, ',')

        MAPVAR(RECC_DEPS_ENV)
        MAPVAR(RECC_REMOTE_ENV)
        MAPVAR(RECC_REMOTE_PLATFORM)
    }
}

void Env::setup_logger_from_environment(const char* programName)
{
    if (const char* logDirectoryVar = getenv("RECC_LOG_DIRECTORY")) {
        RECC_LOG_DIRECTORY = std::string(logDirectoryVar);
    }
    if (const char* logLevelVar = getenv("RECC_LOG_LEVEL")) {
        RECC_LOG_LEVEL = std::string(logLevelVar);
    }
    if (getenv("RECC_VERBOSE") != nullptr) {
        RECC_VERBOSE = true;
    }

    buildboxcommon::LogLevel logLevel;
    if (RECC_VERBOSE) {
        logLevel = buildboxcommon::LogLevel::DEBUG;
    }
    else {
        const auto &validLogLevels =
            buildboxcommon::logging::stringToLogLevelMap();
        auto logLevelIt = validLogLevels.find(RECC_LOG_LEVEL);
        if (logLevelIt == validLogLevels.cend()) {
            BUILDBOXCOMMON_THROW_EXCEPTION(std::invalid_argument,
                                           "Invalid log level \""
                                               << RECC_LOG_LEVEL << "\"");
        }
        logLevel = logLevelIt->second;
    }
    BUILDBOX_LOG_SET_LEVEL(logLevel);

    auto &logger = buildboxcommon::logging::Logger::getLoggerInstance();
    if (!RECC_LOG_DIRECTORY.empty()) {
        buildboxcommon::FileUtils::createDirectory(RECC_LOG_DIRECTORY.c_str());
        logger.setOutputDirectory(RECC_LOG_DIRECTORY.c_str());
        // (The destination needs to be changed before initializing the logger.)
    }
    logger.initialize(programName);
}

_Pragma("GCC diagnostic pop")

    // clang-format on
    void Env::find_and_parse_config_files()
{
    for (auto file_location : RECC_CONFIG_LOCATIONS) {
        std::ifstream config(file_location);
        if (config.good()) {
            // append name of config file, defined by DEFAULT_RECC_CONFIG
            file_location = file_location + "/" + DEFAULT_RECC_CONFIG;
            parse_config_files(file_location);
        }
    }
}

void Env::handle_special_defaults()
{
    // In cache-only mode no execution server is used, however, RECC_SERVER
    // still needs to be set. Use the value from RECC_ACTION_CACHE_SERVER or
    // RECC_CAS_SERVER.
    // If none of these environment variables are set, follow the common logic:
    // use the default server and print a warning.
    if (RECC_CACHE_ONLY && RECC_SERVER.empty()) {
        if (!RECC_ACTION_CACHE_SERVER.empty()) {
            RECC_SERVER = RECC_ACTION_CACHE_SERVER;
        }
        else if (!RECC_CAS_SERVER.empty()) {
            RECC_SERVER = RECC_CAS_SERVER;
        }
    }

    if (RECC_SERVER.empty()) {
        RECC_SERVER = DEFAULT_RECC_SERVER;
        BUILDBOX_LOG_WARNING("Warning: no RECC_SERVER environment variable "
                             "specified."
                             << " Using default server (" << RECC_SERVER
                             << ")");
    }
    else {
        // Deprecate this in the future, allow old configs to work for now.
        RECC_SERVER = Env::backwardsCompatibleURL(RECC_SERVER);
    }

    if (RECC_CAS_SERVER.empty()) {
        if (RECC_ACTION_CACHE_SERVER.empty()) {
            RECC_CAS_SERVER = RECC_SERVER;
            BUILDBOX_LOG_DEBUG("No RECC_CAS_SERVER environment variable "
                               "specified."
                               << " Using the same as RECC_SERVER ("
                               << RECC_CAS_SERVER << ")");
        }
        else {
            // Since it makes most sense for the action cache and the CAS to
            // live together rather than the CAS living with the Execution
            // Service, using the AC endpoint.
            RECC_CAS_SERVER = RECC_ACTION_CACHE_SERVER;
            BUILDBOX_LOG_DEBUG(
                "No RECC_CAS_SERVER environment variable specified. Using the "
                "same RECC_ACTION_CACHE_SERVER ("
                << RECC_ACTION_CACHE_SERVER << ")");
        }
    }
    else {
        // Deprecate this in the future, allow old configs to work for now.
        RECC_CAS_SERVER = Env::backwardsCompatibleURL(RECC_CAS_SERVER);
    }

    if (RECC_ACTION_CACHE_SERVER.empty()) {
        RECC_ACTION_CACHE_SERVER = RECC_CAS_SERVER;
        BUILDBOX_LOG_DEBUG("No RECC_ACTION_CACHE_SERVER environment variable "
                           "specified."
                           << " Using the same as RECC_CAS_SERVER ("
                           << RECC_CAS_SERVER << ")");
    }
    else {
        // Deprecate this in the future, allow old configs to work for now.
        RECC_ACTION_CACHE_SERVER =
            Env::backwardsCompatibleURL(RECC_ACTION_CACHE_SERVER);
    }

    if (!RECC_SERVER_AUTH_GOOGLEAPI) {
        if (!RECC_AUTH_UNCONFIGURED_MSG.empty()) {
            BUILDBOX_LOG_WARNING(RECC_AUTH_UNCONFIGURED_MSG);
        }
    }

    if (RECC_PROJECT_ROOT.empty()) {
        RECC_PROJECT_ROOT = FileUtils::getCurrentWorkingDirectory();
        BUILDBOX_LOG_DEBUG("No RECC_PROJECT_ROOT directory specified. "
                           << "Defaulting to current working directory ("
                           << RECC_PROJECT_ROOT << ")");
    }
    else if (RECC_PROJECT_ROOT.front() != '/') {
        RECC_PROJECT_ROOT = buildboxcommon::FileUtils::makePathAbsolute(
            RECC_PROJECT_ROOT, FileUtils::getCurrentWorkingDirectory());
        BUILDBOX_LOG_WARNING(
            "Warning: RECC_PROJECT_ROOT was set to a relative "
            "path. "
            << "Rewriting to absolute path " << RECC_PROJECT_ROOT);
    }

    if (RECC_REMOTE_PLATFORM.empty()) {
        BUILDBOX_LOG_WARNING("Warning: RECC_REMOTE_PLATFORM has no values.");
    }

    if (RECC_METRICS_FILE.size() && RECC_METRICS_UDP_SERVER.size()) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error, "You can either set RECC_METRICS_FILE or "
                                "RECC_METRICS_UDP_SERVER, but not both.");
    }

    if (!RECC_PREFIX_MAP.empty()) {
        RECC_PREFIX_REPLACEMENT =
            Env::vector_from_delimited_string(RECC_PREFIX_MAP);
    }

    if (DigestGenerator::stringToDigestFunctionMap().count(
            RECC_CAS_DIGEST_FUNCTION) == 0) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "Unknown digest function set in RECC_CAS_DIGEST_FUNCTION: \"" +
                RECC_CAS_DIGEST_FUNCTION + "\".");
    }

    if (RECC_MAX_THREADS == 0) {
        RECC_MAX_THREADS = 1;
    }
}

void Env::assert_reapi_version_is_valid()
{
    if (!proto::s_reapiSupportedVersions.count(RECC_REAPI_VERSION)) {
        BUILDBOXCOMMON_THROW_EXCEPTION(
            std::runtime_error,
            "Unknown REAPI version set in RECC_REAPI_VERSION: \""
                << RECC_REAPI_VERSION << "\".");
    }
}

void Env::verify_files_writeable()
{
    if (RECC_METRICS_FILE.size()) {
        std::ofstream of;
        of.open(RECC_METRICS_FILE, std::ofstream::out | std::ofstream::app);
        if (!of.good()) {
            BUILDBOXCOMMON_THROW_EXCEPTION(
                std::runtime_error,
                "Cannot open RECC_METRICS_FILE for writing: " +
                    RECC_METRICS_FILE);
        }
        of.close();
    }
}

std::deque<std::string> Env::evaluate_config_locations()
{
    // Note that the order in which the config locations are pushed
    // is significant.
    std::deque<std::string> config_order;
    const std::string cwd_recc = "./recc";
    config_order.push_front(cwd_recc);

    const char *home = getenv("HOME");
    if (home != nullptr and home[0] != '\0') {
        config_order.push_front(home + std::string("/.recc"));
    }

    if (!RECC_CUSTOM_PREFIX.empty()) {
        config_order.push_front(RECC_CUSTOM_PREFIX);
    }

    if (!RECC_INSTALL_DIR.empty()) {
        config_order.push_front(RECC_INSTALL_DIR + "/../etc/recc");
    }

    return config_order;
}

std::vector<std::pair<std::string, std::string>>
Env::vector_from_delimited_string(std::string prefix_map,
                                  const std::string &first_delimiter,
                                  const std::string &second_delimiter)
{
    std::vector<std::pair<std::string, std::string>> return_vector;
    // To reduce code duplication, lambda parses key/value by second
    // delimiter, and emplaces back into the vector.
    auto emplace_key_values = [&return_vector,
                               &second_delimiter](auto key_value) {
        const auto equal_pos = key_value.find(second_delimiter);
        // Extra check in case there is input with no second
        // delimiter.
        if (equal_pos == std::string::npos) {
            BUILDBOX_LOG_WARNING(
                "Incorrect path specification for key/value: ["
                << key_value << "] please see README for usage.")
            return;
        }
        // Check if key/values are absolute paths
        std::string key = key_value.substr(0, equal_pos);
        std::string value = key_value.substr(equal_pos + 1);
        key = buildboxcommon::FileUtils::normalizePath(key.c_str());
        value = buildboxcommon::FileUtils::normalizePath(value.c_str());
        if (!FileUtils::isAbsolutePath(key.c_str()) &&
            !FileUtils::isAbsolutePath(value.c_str())) {
            BUILDBOX_LOG_WARNING("Input paths must be absolute: [" << key_value
                                                                   << "]");
            return;
        }
        if (FileUtils::hasPathPrefix(RECC_PROJECT_ROOT, key)) {
            BUILDBOX_LOG_WARNING("Path to replace: ["
                                 << key.c_str()
                                 << "] is a prefix of the project root: ["
                                 << RECC_PROJECT_ROOT << "]");
        }
        return_vector.emplace_back(std::make_pair(key, value));
    };
    size_t delim_pos = std::string::npos;
    // Iterate while we can find the first delimiter
    while ((delim_pos = prefix_map.find(first_delimiter)) !=
           std::string::npos) {
        emplace_key_values(prefix_map.substr(0, delim_pos));
        // Erase the key/value, including the delimiter
        prefix_map.erase(0, delim_pos + 1);
    }
    // If there is only one key/value, or it's the final key/value pair after
    // multiple, emplace it back.
    if (!prefix_map.empty() &&
        prefix_map.find(first_delimiter) == std::string::npos) {
        emplace_key_values(prefix_map);
    }
    return return_vector;
}

void Env::set_config_locations()
{
    set_config_locations(Env::evaluate_config_locations());
}

void Env::set_config_locations(const std::deque<std::string> &config_order)
{
    RECC_CONFIG_LOCATIONS = config_order;
}

std::string Env::substring_until_nth_token(const std::string &value,
                                           const std::string &character,
                                           const std::string::size_type &pos)
{
    std::size_t i = 0;
    std::string result, next_string = value;
    while (i < pos) {
        auto position = next_string.find(character);
        if (position == std::string::npos ||
            position + character.size() > next_string.size()) {
            return std::string();
        }
        if (!result.empty())
            result += character;
        result += next_string.substr(0, position);
        next_string = next_string.substr(position + character.size());
        ++i;
    }
    return result;
}

const std::string Env::backwardsCompatibleURL(const std::string &url)
{
    // Construct the new URL format if it is in the previous format
    // which doesn't include the protocol
    if (!(url.find("http://") == 0 || url.find("https://") == 0 ||
          url.find("unix:") == 0)) {
        // Use the hint provided by the deprecated flag
        // to use https protocol instead, if set
        if (RECC_SERVER_SSL) {
            return "https://" + url;
        }
        else {
            return "http://" + url;
        }
    }
    else {
        if (RECC_SERVER_SSL && url.find("https://") != 0) {
            BUILDBOXCOMMON_THROW_EXCEPTION(
                std::runtime_error,
                "URL set to url=[" + url +
                    "], with incompatible flag RECC_SERVER_SSL set. (URL must "
                    "be of the format `https://...` with this flag).");
        }
        return url;
    }
}

void Env::parse_config_variables()
{
    Env::find_and_parse_config_files();
    Env::parse_config_variables(environ);
    Env::handle_special_defaults();
    Env::assert_reapi_version_is_valid();
    Env::verify_files_writeable();
}

std::pair<int, int> Env::version_string_to_pair(const std::string &version)
{
    static const std::regex regex("^(\\d)\\.(\\d)$");

    std::smatch matches;
    if (std::regex_search(version, matches, regex) && matches.size() == 3) {
        const int major = std::stoi(matches[1]);
        const int minor = std::stoi(matches[2]);
        return std::make_pair(major, minor);
    }

    throw std::invalid_argument("Could not parse X.Y version from: '" +
                                version + "'");
}

bool Env::configured_reapi_version_equal_to_or_newer_than(
    const std::string &version)
{
    return version_string_to_pair(RECC_REAPI_VERSION) >=
           version_string_to_pair(version);
}

} // namespace recc
