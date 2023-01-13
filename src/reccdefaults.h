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

#ifndef INCLUDED_RECCDEFAULTS
#define INCLUDED_RECCDEFAULTS

#include <chrono>
#include <limits.h>

// clang-format off
// allow default instance name to be set at compile time
#ifndef DEFAULT_RECC_INSTANCE
#define DEFAULT_RECC_INSTANCE ""
#endif
#define DEFAULT_RECC_POLL_WAIT std::chrono::seconds(1)
#define DEFAULT_RECC_RETRY_LIMIT 0
#define DEFAULT_RECC_RETRY_DELAY 1000
#define DEFAULT_RECC_REQUEST_TIMEOUT 0
#define DEFAULT_RECC_KEEPALIVE_TIME 0
#define DEFAULT_RECC_SERVER "http://localhost:8085"
#define DEFAULT_RECC_TMPDIR "/tmp"
#define DEFAULT_RECC_TMP_PREFIX "recc"
#define DEFAULT_RECC_SERVER_AUTH_GOOGLEAPI 0
#define DEFAULT_RECC_ACCESS_TOKEN_PATH ""
#define DEFAULT_RECC_SERVER_SSL 0
#define DEFAULT_RECC_CONFIG "recc.conf"
#define DEFAULT_RECC_PROJECT_ROOT ""
#define DEFAULT_RECC_DEPS_GLOBAL_PATHS 0
#define DEFAULT_RECC_AUTH_UNCONFIGURED_MSG ""
#define DEFAULT_RECC_CORRELATED_INVOCATIONS_ID ""
#define DEFAULT_RECC_METRICS_FILE ""
#define DEFAULT_RECC_METRICS_UDP_SERVER ""
#define DEFAULT_RECC_NO_PATH_REWRITE 0
#define DEFAULT_RECC_PREFIX_MAP ""
#define DEFAULT_RECC_LOG_LEVEL "error"
#define DEFAULT_RECC_LOG_DIRECTORY ""
#define DEFAULT_RECC_VERBOSE 0
#define DEFAULT_RECC_ENABLE_METRICS 0
#define DEFAULT_RECC_FORCE_REMOTE 0
#define DEFAULT_RECC_CACHE_ONLY 0
#define DEFAULT_RECC_CACHE_UPLOAD_LOCAL_BUILD 0
#define DEFAULT_RECC_ACTION_UNCACHEABLE 0
#define DEFAULT_RECC_SKIP_CACHE 0
#define DEFAULT_RECC_DONT_SAVE_OUTPUT 0
#define DEFAULT_RECC_CACHE_UPLOAD_FAILED_BUILD 1
#define DEFAULT_RECC_WORKING_DIR_PREFIX ""
#define DEFAULT_RECC_ACTION_SALT ""

#define DEFAULT_RECC_DEPS_DIRECTORY_OVERRIDE ""
#define DEFAULT_RECC_DEPS_OVERRIDE {}
#define DEFAULT_RECC_OUTPUT_FILES_OVERRIDE {}
#define DEFAULT_RECC_OUTPUT_DIRECTORIES_OVERRIDE {}
#define DEFAULT_RECC_DEPS_EXCLUDE_PATHS {}
#define DEFAULT_RECC_ENV_TO_READ {}

#define DEFAULT_RECC_DEPS_ENV {}
#define DEFAULT_RECC_REMOTE_ENV {}
#define DEFAULT_RECC_REMOTE_PLATFORM {}

#define DEFAULT_RECC_CAS_DIGEST_FUNCTION "SHA256"
#define DEFAULT_RECC_MAX_THREADS 4

#define DEFAULT_RECC_REAPI_VERSION "2.2"

#ifdef HOST_NAME_MAX
    #define DEFAULT_RECC_HOSTNAME_MAX_LENGTH HOST_NAME_MAX
#else
    #define DEFAULT_RECC_HOSTNAME_MAX_LENGTH 255
#endif

#endif
