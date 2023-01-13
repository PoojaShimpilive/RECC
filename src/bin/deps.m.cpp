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

#include <deps.h>
#include <env.h>
#include <fileutils.h>

#include <buildboxcommon_logging.h>

#include <cstring>
#include <iostream>
#include <regex>

using namespace recc;

const std::string HELP(
    "USAGE: deps<command>\n"
    "\n"
    "Attempts to determine the files needed to execute the given compiler\n"
    "command, then prints a newline-separated list of them.");

int main(int argc, char *argv[])
{
    buildboxcommon::logging::Logger::getLoggerInstance().initialize(argv[0]);

    Env::set_config_locations();
    Env::parse_config_variables();
    const std::string cwd = FileUtils::getCurrentWorkingDirectory();

    if (argc <= 1 || strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0) {
        BUILDBOX_LOG_WARNING(HELP);
        return 0;
    }
    try {
        const auto parsedCommand =
            ParsedCommandFactory::createParsedCommand(&argv[1], cwd.c_str());
        const auto deps = Deps::get_file_info(parsedCommand).d_dependencies;
        for (const auto &dep : deps) {
            BUILDBOX_LOG_INFO(dep);
        }
    }
    catch (const subprocess_failed_error &e) {
        exit(e.d_error_code);
    }
}
