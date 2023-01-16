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

#ifndef INCLUDED_PARSEDCOMMAND
#define INCLUDED_PARSEDCOMMAND

#include <buildboxcommon_temporaryfile.h>
#include <list>
#include <set>
#include <string>
#include <vector>

namespace recc {

/**
 * Represents the result of parsing a compiler command.
 * NOTE: THIS CLASS SHOULD BE TREATED AS PRIVATE, USAGE SHOULD GO THROUGH
 * PARSEDCOMMMANDFACTORY.H
 */
class ParsedCommand {
  public:
    ParsedCommand(const std::vector<std::string> &command,
                  const std::string &workingDirectory);
    ParsedCommand()
        : d_compilerCommand(false), d_isClang(false),
          d_producesSunMakeRules(false), d_containsUnsupportedOptions(false),
          d_dependencyFileAIX(nullptr)
    {
    }

    /**
     * Returns true if the given command is a supported compiler command.
     */
    bool is_compiler_command() const { return d_compilerCommand; }

    /**
     * Returns true if this is a gcc command.
     */
    bool is_gcc() const { return d_isGcc; }

    /**
     * Returns true if this is a clang command.
     */
    bool is_clang() const { return d_isClang; }

    /**
     * Returns true if this is a Sun Studio command.
     */
    bool is_sun_studio() const { return d_isSunStudio; }

    /**
     * Returns true if this is a AIX command.
     */
    bool is_AIX() const { return d_dependencyFileAIX != nullptr; }

    /**
     * Returns the original command that was passed to the constructor,
     * with absolute paths replaced with equivalent relative paths.
     */
    std::vector<std::string> get_command() const { return d_command; }

    /**
     * Return a command that prints this command's dependencies in Makefile
     * format. If this command is not a supported compiler command, the
     * result is undefined.
     */
    std::vector<std::string> get_dependencies_command() const
    {
        return d_dependenciesCommand;
    }

    /**
     * Return compiler basename specified from the command.
     */
    std::string get_compiler() const { return d_compiler; }

    /**
     * Return the name of the file the compiler will write the source
     * dependencies to on AIX.
     *
     * If the compiler command doesn't include a AIX compiler, return
     * empty.
     */
    std::string get_aix_dependency_file_name() const
    {
        if (d_dependencyFileAIX != nullptr) {
            return d_dependencyFileAIX->strname();
        }
        return "";
    }

    /**
     * Return the non deps output files specified in the command arguments.
     *
     * This is not necessarily all of the files the command will create.
     * (For example, if no output files are specified, many compilers will
     * write to a.out by default.)
     */
    std::set<std::string> get_products() const 
    { return d_commandProducts; 
    }

    /**
     * Return the deps output files specified in the command arguments.
     */
    std::set<std::string> get_deps_products() const
    {
        return d_commandDepsProducts;
    }

    /**
     * If true, the dependencies command will produce nonstandard Sun-style
     * make rules where one dependency is listed per line and spaces aren't
     * escaped.
     */
    bool produces_sun_make_rules() const { return d_producesSunMakeRules; }

    /**
     * Converts a command path (e.g. "/usr/bin/gcc-4.7") to a command name
     * (e.g. "gcc")
     */
    static std::string commandBasename(const std::string &path);

    bool d_compilerCommand;
    bool d_md_option_set = false;
    bool d_isGcc = false;
    bool d_isClang = false;
    bool d_isSunStudio = false;
    bool d_producesSunMakeRules;
    bool d_containsUnsupportedOptions;
    bool d_upload_all_include_dirs = false;
    std::string d_compiler;
    std::list<std::string> d_originalCommand;
    std::vector<std::string> d_defaultDepsCommand;
    std::vector<std::string> d_preProcessorOptions;
    std::vector<std::string> d_command;
    std::vector<std::string> d_dependenciesCommand;
    std::vector<std::string> d_inputFiles;
    std::set<std::string> d_commandProducts;
    std::set<std::string> d_commandDepsProducts;
    std::set<std::string> d_includeDirs;
    std::unique_ptr<buildboxcommon::TemporaryFile> d_dependencyFileAIX;
};

} // namespace recc

#endif
