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

#include <remoteexecutionsignals.h>

#include <buildboxcommon_logging.h>

#include <cerrno>
#include <cstdio>
#include <pthread.h>
#include <signal.h>
#include <string.h>

namespace recc {

void Signal::setup_signal_handler(int signal, void (*handler)(int))
{
    /* Set up signal handling for the Execute() request */
    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_flags = SA_RESETHAND; // If the user hits Control-C again, stop.
    if (sigemptyset(&sa.sa_mask) != 0) {
        const std::string errorReason = strerror(errno);
        BUILDBOX_LOG_ERROR("Unable to clear signal set: " << errorReason);
        return;
    }
    if (sigaction(signal, &sa, nullptr) != 0) {
        const std::string errorReason = strerror(errno);
        BUILDBOX_LOG_ERROR(
            "Unable to register cancellation handler: " << errorReason);
    }
}

void Signal::block_sigint()
{
    sigset_t signal_set;
    if (sigemptyset(&signal_set) != 0 || sigaddset(&signal_set, SIGINT) != 0) {
        const std::string errorReason = strerror(errno);
        BUILDBOX_LOG_ERROR("Unable to block SIGINT: " << errorReason);
        return;
    }

    if (pthread_sigmask(SIG_BLOCK, &signal_set, nullptr) != 0) {
        const std::string errorReason = strerror(errno);
        BUILDBOX_LOG_ERROR("Unable to block SIGINT: " << errorReason);
    }
}

void Signal::unblock_sigint()
{
    sigset_t signal_set;
    if (sigemptyset(&signal_set) != 0 || sigaddset(&signal_set, SIGINT) != 0) {
        const std::string errorReason = strerror(errno);
        BUILDBOX_LOG_ERROR("Unable to unblock SIGINT: " << errorReason);
        return;
    }

    if (pthread_sigmask(SIG_UNBLOCK, &signal_set, nullptr) != 0) {
        const std::string errorReason = strerror(errno);
        BUILDBOX_LOG_ERROR("Unable to block SIGINT: " << errorReason);
    }
}

} // namespace recc
