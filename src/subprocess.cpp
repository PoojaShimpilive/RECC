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

#include <subprocess.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <map>
#include <memory>
#include <sstream>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>

#include <buildboxcommon_logging.h>

namespace recc {

namespace {
static std::array<int, 2> createPipe()
{
    std::array<int, 2> pipe_fds = {0, 0};

    if (pipe(pipe_fds.data()) == -1) {
        BUILDBOX_LOG_ERROR("Error calling `pipe()`: " << strerror(errno));
        throw std::system_error(errno, std::system_category());
    }
    return pipe_fds;
}

} // namespace

Subprocess::SubprocessResult
Subprocess::execute(const std::vector<std::string> &command, bool pipeStdOut,
                    bool pipeStdErr,
                    const std::map<std::string, std::string> &env)
{
    // Convert the command to a char*[]
    size_t argc = command.size();
    std::unique_ptr<const char *[]> argv(new const char *[argc + 1]);
    for (size_t i = 0; i < argc; ++i) {
        argv[i] = command[i].c_str();
    }
    argv[argc] = nullptr;

    // Pipe, fork and exec
    auto stdOutPipeFDs = createPipe();
    auto stdErrPipeFDs = createPipe();

    const auto pid = fork();
    if (pid == -1) {
        BUILDBOX_LOG_ERROR("Error calling `fork()`: " << strerror(errno));
        throw std::system_error(errno, std::system_category());
    }

    if (pid == 0) {
        // (runs only in the child)
        if (pipeStdOut) {
            close(stdOutPipeFDs[0]);
            dup2(stdOutPipeFDs[1],
                 STDOUT_FILENO); // redirect stdout to input end of pipe
            close(stdOutPipeFDs[1]);
        }
        if (pipeStdErr) {
            close(stdErrPipeFDs[0]);
            dup2(stdErrPipeFDs[1],
                 STDERR_FILENO); // redirect stderr to input end of pipe
            close(stdErrPipeFDs[1]);
        }

        for (const auto &envPair : env) {
            setenv(envPair.first.c_str(), envPair.second.c_str(), 1);
        }

        const auto exec_status =
            execvp(argv[0], const_cast<char *const *>(argv.get()));

        int exit_code = 1;
        if (exec_status != 0) {
            const auto exec_error = errno;
            // Following the Bash convention for exit codes.
            // (https://gnu.org/software/bash/manual/html_node/Exit-Status.html)
            if (exec_error == ENOENT) {
                exit_code = 127; // "command not found"
            }
            else {
                exit_code = 126; // Command invoked cannot execute
            }
        }

        _Exit(exit_code);
    }

    // (runs only in the parent)
    SubprocessResult result;

    // Get the output from the child process
    fd_set fdSet;
    FD_ZERO(&fdSet);
    if (pipeStdOut) {
        FD_SET(stdOutPipeFDs[0], &fdSet);
        close(stdOutPipeFDs[1]);
    }
    if (pipeStdErr) {
        FD_SET(stdErrPipeFDs[0], &fdSet);
        close(stdErrPipeFDs[1]);
    }

    char buffer[4096];
    while (FD_ISSET(stdOutPipeFDs[0], &fdSet) ||
           FD_ISSET(stdErrPipeFDs[0], &fdSet)) {
        fd_set readFDSet = fdSet;

        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        select(FD_SETSIZE, &readFDSet, nullptr, nullptr, &timeout);

        if (FD_ISSET(stdOutPipeFDs[0], &readFDSet)) {
            ssize_t bytesRead = read(stdOutPipeFDs[0], buffer, sizeof(buffer));
            if (bytesRead > 0) {
                result.d_stdOut.append(buffer, static_cast<size_t>(bytesRead));
            }
            else {
                close(stdOutPipeFDs[0]);
                FD_CLR(stdOutPipeFDs[0], &fdSet);
            }
        }
        if (FD_ISSET(stdErrPipeFDs[0], &readFDSet)) {
            ssize_t bytesRead = read(stdErrPipeFDs[0], buffer, sizeof(buffer));
            if (bytesRead > 0) {
                result.d_stdErr.append(buffer, static_cast<size_t>(bytesRead));
            }
            else {
                close(stdErrPipeFDs[0]);
                FD_CLR(stdErrPipeFDs[0], &fdSet);
            }
        }
    }

    // Get the status code from the child process
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        throw std::system_error(errno, std::system_category());
    }

    if (WIFEXITED(status)) {
        result.d_exitCode = WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status)) {
        result.d_exitCode = 128 + WTERMSIG(status);
        // Exit code as returned by Bash.
        // (https://gnu.org/software/bash/manual/html_node/Exit-Status.html)
    }
    else {
        /* According to the documentation for `waitpid()` we should never get
         * here:
         *
         * "If the information pointed to by stat_loc was stored by a call to
         * waitpid() that did not specify the WUNTRACED  or
         * CONTINUED flags, or by a call to the wait() function,
         * exactly one of the macros WIFEXITED(*stat_loc) and
         * WIFSIGNALED(*stat_loc) shall evaluate to a non-zero value."
         *
         * (https://pubs.opengroup.org/onlinepubs/009695399/functions/wait.html)
         */
        throw std::runtime_error(
            "`waitpid()` returned an unexpected status: " +
            std::to_string(status));
    }

    return result;
}
} // namespace recc
