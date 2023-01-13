// Copyright 2020 Bloomberg Finance L.P
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

#ifndef INCLUDED_PARSEDCOMMANDFACTORY
#define INCLUDED_PARSEDCOMMANDFACTORY

#include <compilerdefaults.h>
#include <functional>
#include <initializer_list>
#include <list>
#include <map>
#include <memory>
#include <parsedcommand.h>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace recc {

class ParsedCommandFactory {
  public:
    /**
     * An map from a string to a function pointer, lexigraphically sorted in
     * decending order.
     */
    typedef std::map<std::string,
                     std::function<void(ParsedCommand *, const std::string &,
                                        const std::string &)>,
                     std::greater<std::string>>
        CompilerParseRulesMap;
    /**
     * An unordered_map from a set of strings to a CompilerParseRulesMap.
     * This is a catalog of parse rules, mapped by compiler name.
     */
    typedef std::unordered_map<SupportedCompilers::CompilerListType,
                               CompilerParseRulesMap, CompilerListTypeHasher>
        ParsedCommandMap;

    /**
     * Default overloaded factory methods for creating a parsedCommand.
     * Calling these methods is the only way to create a parsedCommand.
     */
    static ParsedCommand
    createParsedCommand(const std::vector<std::string> &command,
                        const std::string &workingDirectory);
    static ParsedCommand createParsedCommand(char **argv,
                                             const char *workingDirectory);
    static ParsedCommand
    createParsedCommand(std::initializer_list<std::string> command);

    /**
     * Convert a null-terminated list of C strings to a vector of C++
     * strings.
     */
    static std::vector<std::string> vectorFromArgv(const char *const *argv);

    /**
     * Constructs and returns a ParsedCommandMap.
     */
    static ParsedCommandFactory::ParsedCommandMap getParsedCommandMap();

  private:
    /**
     *  This method iterates through the parsedCommandMap, comparing the
     * options in the command to each option in the map, if matching, applying
     * the coresponding option function.
     *
     * This method modifies the state of the passed in ParsedCommand object.
     */
    static void parseCommand(ParsedCommand *command,
                             const CompilerParseRulesMap &options,
                             const std::string &workingDirectory);

    ParsedCommandFactory() = delete;
};

struct ParseRule {

    static void
    parseInterfersWithDepsOption(ParsedCommand *command,
                                 const std::string &workingDirectory,
                                 const std::string &option);

    static void parseIsInputPathOption(ParsedCommand *command,
                                       const std::string &workingDirectory,
                                       const std::string &option);

    static void
    parseIsEqualInputPathOption(ParsedCommand *command,
                                const std::string &workingDirectory,
                                const std::string &option);

    static void parseIsCompileOption(ParsedCommand *command,
                                     const std::string &workingDirectory,
                                     const std::string &option);

    static void parseOptionRedirectsOutput(ParsedCommand *command,
                                           const std::string &workingDirectory,
                                           const std::string &option);

    static void
    parseOptionRedirectsDepsOutput(ParsedCommand *command,
                                   const std::string &workingDirectory,
                                   const std::string &option);

    static void parseOptionDepsRuleTarget(ParsedCommand *command,
                                          const std::string &workingDirectory,
                                          const std::string &option);

    static void
    parseIsPreprocessorArgOption(ParsedCommand *command,
                                 const std::string &workingDirectory,
                                 const std::string &option);

    static void parseIsMacro(ParsedCommand *command,
                             const std::string &workingDirectory,
                             const std::string &option);

    static void parseOptionSetsGccLanguage(ParsedCommand *command,
                                           const std::string &workingDirectory,
                                           const std::string &option);

    static void parseOptionIsUnsupported(ParsedCommand *command,
                                         const std::string &workingDirectory,
                                         const std::string &option);
};

struct ParseRuleHelper {
    /**
     * Match command option passed in, to compiler options in compiler option
     * map. Return a pair including the function or an empty function depending
     * on if there was a match.
     */
    static std::pair<std::string,
                     std::function<void(ParsedCommand *, const std::string &,
                                        const std::string &)>>
    matchCompilerOptions(
        const std::string &option,
        const ParsedCommandFactory::CompilerParseRulesMap &options);

    /**
     * This helper deals with gcc options parsing, which can have a space after
     * an option, or no space.
     **/
    static void parseGccOption(ParsedCommand *command,
                               const std::string &workingDirectory,
                               const std::string &option, bool toDeps = true,
                               bool isOutput = false, bool depsOutput = false);
    /**
     * This helper deals with pushing the correct command to the various
     * vectors in parsedCommand, depending on things such as if the command
     * specifies output, or is needed by the dependency command run locally.
     **/
    static void appendAndRemoveOption(ParsedCommand *command,
                                      const std::string &workingDirectory,
                                      bool isPath, bool toDeps,
                                      bool isOutput = false,
                                      bool depsOutput = false);

    /**
     * Parse a comma-separated list and store the results in the given
    vector.
     */
    static void parseStageOptionList(const std::string &option,
                                     std::vector<std::string> *result);
};

} // namespace recc

#endif
