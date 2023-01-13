.. _RECC Configuration Variables:

RECC Configuration Variables
==========================

* ``RECC_SERVER`` - the URI of the server to use (e.g. http://localhost:8085)
* ``RECC_CAS_SERVER`` - the URI of the CAS server to use (by default, uses ``RECC_ACTION_CACHE_SERVER`` if set. Else ``RECC_SERVER``)
* ``RECC_ACTION_CACHE_SERVER`` - the URI of the Action Cache server to use (by default, uses ``RECC_CAS_SERVER``. Else ``RECC_SERVER``)
* ``RECC_INSTANCE`` - the instance name to pass to the server (defaults to "dev")

----

* ``RECC_CACHE_ONLY`` - if set to any value, runs recc in cache-only mode. In this mode, recc will build anything not available in the remote cache locally, rather than failing to build.
* ``RECC_CACHE_UPLOAD_LOCAL_BUILD`` - if set to any value, upload action result to Action Cache server after local build. Can only be used with ``RECC_CACHE_ONLY``.

----

* ``RECC_NO_EXECUTE`` - if set to any value, recc will only attempt to build an action and compute its digest. (It will **not** invoke the command locally nor execute it remotely.)

----

* ``RECC_PROJECT_ROOT`` - the top-level directory of the project source. If the command contains paths inside the root, they will be rewritten to relative paths (by default, uses the current working directory)

----

* ``RECC_SERVER_AUTH_GOOGLEAPI`` - use default google authentication when communicating over gRPC, instead of using an insecure connection
* ``RECC_ACCESS_TOKEN_PATH`` - path specifying location of access token (JWT, OAuth, etc) to be attached to all secure connections. Defaults to ""

----

* ``RECC_LOG_LEVEL`` - logging verbosity level [optional, default = error, supported = trace/debug/info/warning/error]
* ``RECC_VERBOSE`` - if set to any value, equivalent to RECC_LOG_LEVEL=debug
* ``RECC_LOG_DIRECTORY`` - instead of printing to stderr, write log files in this location (follows `glog's file-naming convention <https://github.com/google/glog#severity-levels>`_)

----

* ``RECC_ENABLE_METRICS`` - if set to any value, enable metric collection
* ``RECC_METRICS_FILE`` - write metrics to that file (Default/Empty string — stderr). Cannot be used with ``RECC_METRICS_UDP_SERVER``.
* ``RECC_METRICS_UDP_SERVER`` - write metrics to the specified host:UDP_Port. Cannot be used with ``RECC_METRICS_FILE``

----

* ``RECC_NO_PATH_REWRITE`` - if set to any value, do not rewrite absolute paths to be relative. Defaults to false.

----

* ``RECC_FORCE_REMOTE`` - if set to any value, send all commands to the build server. (Non-compile commands won't be executed locally, which can cause some builds to fail.)

----

* ``RECC_ACTION_UNCACHEABLE`` - if set to any value, sets `do_not_cache` flag to indicate that the build action can never be cached
* ``RECC_SKIP_CACHE`` - if set to any value, sets `skip_cache_lookup` flag to re-run the build action instead of looking it up in the cache
* ``RECC_ACTION_SALT`` - optional salt value to place the `Action` into a separate cache namespace

----

* ``RECC_DONT_SAVE_OUTPUT`` - if set to any value, prevent build output from being saved to local disk

----

* ``RECC_DEPS_GLOBAL_PATHS`` - if set to any value, report all entries returned by the dependency command, even if they are absolute paths
* ``RECC_DEPS_OVERRIDE`` - comma-separated list of files to send to the build server (by default, run `deps` to determine this)
* ``RECC_DEPS_DIRECTORY_OVERRIDE`` - directory to send to the build server (if both this and ``RECC_DEPS_OVERRIDE`` are set, this one is used)
* ``RECC_OUTPUT_FILES_OVERRIDE`` - comma-separated list of files to request from the build server (by default, `deps` guesses)
* ``RECC_OUTPUT_DIRECTORIES_OVERRIDE`` - comma-separated list of directories to request (by default, `deps` guesses)
* ``RECC_DEPS_EXCLUDE_PATHS`` - comma-separated list of paths to exclude from the input root
* ``RECC_DEPS_ENV_[var]`` - sets [var] for local dependency detection commands

----

* ``RECC_PRESERVE_ENV`` - if set to any value, preserve all non-recc environment variables in the remote
* ``RECC_ENV_TO_READ`` - comma-separated list of specific environment variables to preserve from the local environment (can be used to preserve ``RECC_`` variables, unlike ``RECC_PRESERVE_ENV``, default is compiler-specific)
* ``RECC_REMOTE_ENV_[var]`` - sets [var] in the remote build environment
----

* ``RECC_REMOTE_PLATFORM_[key]`` - specifies a platform property, which the build server uses to select the build worker
----

* ``RECC_RETRY_LIMIT`` - number of times to retry failed requests (default 0).
* ``RECC_RETRY_DELAY`` - base delay (in ms) between retries grows exponentially (default 1000ms)
* ``RECC_KEEPALIVE_TIME`` - period (in s) between gRPC keepalive pings (disabled by default)
----

* ``RECC_PREFIX_MAP`` - specify path mappings to replace. The source and destination must both be absolute paths. Supports multiple paths, separated by colon(:). Ex. ``RECC_PREFIX_MAP=/usr/bin=/usr/local/bin``)
----

* ``RECC_CAS_DIGEST_FUNCTION`` - specify what hash function to use to calculate digests. (Default: "SHA256") Supported values: "MD5", "SHA1", "SHA256", "SHA384", "SHA512"
----

* ``RECC_WORKING_DIR_PREFIX`` - directory to prefix the command's working directory, and input paths relative to it
----

* ``RECC_MAX_THREADS`` -   Allow some operations to utilize multiple cores. (Default: 4) A value of -1 specifies use all available cores.
----

* ``RECC_REAPI_VERSION`` - Version of the Remote Execution API to use. (Default: "2.0") Supported values: "2.0", "2.1", "2.2"
