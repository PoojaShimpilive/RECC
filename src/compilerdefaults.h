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

#ifndef INCLUDED_COMPILERDEFAULTS
#define INCLUDED_COMPILERDEFAULTS

#include <set>
#include <string>
#include <vector>

namespace recc {

struct SupportedCompilers {
    typedef std::set<std::string> CompilerListType;

    // Lists of compilers that support the same options.
    static const CompilerListType Gcc;
    static const CompilerListType GccPreprocessor;
    static const CompilerListType SunCPP;
    static const CompilerListType AIX;
    static const CompilerListType CCompilers;

    // Lists of options needed by the corresponding compiler to get dependency
    // information from a source file. These commands will be added to the
    // dependency command created by ParsedCommand. See the ParsedCommand
    // constructor for usage.
    static const std::vector<std::string> GccDefaultDeps;
    static const std::vector<std::string> SunCPPDefaultDeps;
    static const std::vector<std::string> AIXDefaultDeps;

    // Supported languages for compilers
    static const std::set<std::string> GccSupportedLanguages;
};

struct CompilerListTypeHasher {
    size_t operator()(const SupportedCompilers::CompilerListType &list) const
    {
        // This was copied from boost::hash_range, which iterates and calls
        // boost::hash_combine on a range of elements.
        // https://www.boost.org/doc/libs/1_35_0/doc/html/boost/hash_combine_id241013.html
        size_t seed = 0;
        for (const auto &val : list) {
            seed ^= std::hash<std::string>{}(val) + 0x9e3779b9 + (seed << 6) +
                    (seed >> 2);
        }
        return seed;
    }
};

} // namespace recc
#endif
