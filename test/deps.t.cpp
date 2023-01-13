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
#include <parsedcommand.h>

#include <buildboxcommon_fileutils.h>

#include <gtest/gtest.h>

using namespace recc;

// Different compilers and platforms might have
// report default includes, which makes comparing the results
// of the dependency command difficult. To mitigate this,
// any header added to "default_includes" will be filtered out
// of the expected set.
//
// This also will normalize the paths of all elements passed in,
// due to some compilers adding `./` to the front of some relative
// paths.
std::set<std::string> default_includes = {"/usr/include/stdc-predef.h"};
std::set<std::string>
filter_default_includes(const std::set<std::string> &paths)
{
    std::set<std::string> result;
    for (const auto &path : paths) {
        if (default_includes.find(path) == default_includes.end()) {
            result.insert(
                buildboxcommon::FileUtils::normalizePath(path.c_str()));
        }
    }
    return result;
}

// Set in the top-level CMakeLists.txt depending on the platform.
#ifdef RECC_PLATFORM_COMPILER

// Some compilers, like xlc, need certain environment variables to
// get the dependencies properly. So parse the env for these tests
TEST(DepsTest, Empty)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "-I.", "empty.c"});
    std::set<std::string> expectedDeps = {"empty.c"};
    EXPECT_EQ(expectedDeps, filter_default_includes(
                                Deps::get_file_info(command).d_dependencies));

    // Ensure dependencies match for /usr/bin/cc and RECC_PLATFORM_COMPILER
    const auto cc_cmd = ParsedCommandFactory::createParsedCommand(
        {"cc", "-c", "-I.", "empty.c"});
    EXPECT_EQ(
        filter_default_includes(Deps::get_file_info(command).d_dependencies),
        filter_default_includes(Deps::get_file_info(cc_cmd).d_dependencies));
}

TEST(DepsTest, SimpleInclude)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "-I.", "includes_empty.c"});
    std::set<std::string> expected = {"includes_empty.c", "empty.h"};
    EXPECT_EQ(expected, filter_default_includes(
                            Deps::get_file_info(command).d_dependencies));
}

TEST(DepsTest, RecursiveDependency)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "-I.", "includes_includes_empty.c"});
    std::set<std::string> expected = {"includes_includes_empty.c",
                                      "includes_empty.h", "empty.h"};
    EXPECT_EQ(expected, filter_default_includes(
                            Deps::get_file_info(command).d_dependencies));
}

TEST(DepsTest, MultiFile)
{
    // Exclude this test on AIX, as the compiler doesn't support writing
    // multiple source files dependency information to the same file without
    // overriding the contents.
    if (strcmp(RECC_PLATFORM_COMPILER, "xlc") != 0) {
        Env::parse_config_variables();
        const auto command = ParsedCommandFactory::createParsedCommand(
            {RECC_PLATFORM_COMPILER, "-c", "-I.", "includes_includes_empty.c",
             "includes_empty.c"});
        std::set<std::string> expected = {"includes_includes_empty.c",
                                          "includes_empty.c",
                                          "includes_empty.h", "empty.h"};
        EXPECT_EQ(expected, filter_default_includes(
                                Deps::get_file_info(command).d_dependencies));
    }
}

TEST(DepsTest, EdgeCases)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "-I.", "edge_cases.c"});
    std::set<std::string> expected = {"edge_cases.c", "empty.h",
                                      "header with spaces.h"};
    EXPECT_EQ(expected, filter_default_includes(
                            Deps::get_file_info(command).d_dependencies));
}

TEST(DepsTest, OutputArgument)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "-I.", "includes_empty.c", "-o",
         "/dev/null"});
    std::set<std::string> expected = {"includes_empty.c", "empty.h"};
    EXPECT_EQ(expected, filter_default_includes(
                            Deps::get_file_info(command).d_dependencies));
}

TEST(DepsTest, OutputArgumentNoSpace)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "-I.", "includes_empty.c",
         "-o/dev/null"});
    std::set<std::string> expected = {"includes_empty.c", "empty.h"};
    EXPECT_EQ(expected, filter_default_includes(
                            Deps::get_file_info(command).d_dependencies));
}

TEST(DepsTest, PreprocessorOutputArgument)
{
    if (ParsedCommand::commandBasename(RECC_PLATFORM_COMPILER) == "gcc") {
        const auto command = ParsedCommandFactory::createParsedCommand(
            {RECC_PLATFORM_COMPILER, "-c", "-I.", "includes_empty.c",
             "-Wp,-MMD"});
        std::set<std::string> expected = {"includes_empty.c", "empty.h"};
        EXPECT_EQ(expected, filter_default_includes(
                                Deps::get_file_info(command).d_dependencies));
    }
}

TEST(DepsTest, Subdirectory)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "-I.", "-Isubdirectory",
         "includes_from_subdirectory.c"});
    std::set<std::string> expected = {"includes_from_subdirectory.c",
                                      "subdirectory/header.h"};
    EXPECT_EQ(expected, filter_default_includes(
                            Deps::get_file_info(command).d_dependencies));
}

TEST(DepsTest, SystemSubdirectory)
{
    if (ParsedCommand::commandBasename(RECC_PLATFORM_COMPILER) == "gcc") {
        Env::parse_config_variables();
        const auto command = ParsedCommandFactory::createParsedCommand(
            {RECC_PLATFORM_COMPILER, "-c", "-I.", "-isystemsubdirectory",
             "includes_from_subdirectory.c"});
        std::set<std::string> expected = {"includes_from_subdirectory.c",
                                          "subdirectory/header.h"};
        EXPECT_EQ(expected, filter_default_includes(
                                Deps::get_file_info(command).d_dependencies));
    }
}

TEST(DepsTest, InputInSubdirectory)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "subdirectory/empty.c"});
    std::set<std::string> expected = {"subdirectory/empty.c"};
    EXPECT_EQ(expected, filter_default_includes(
                            Deps::get_file_info(command).d_dependencies));
}

TEST(DepsTest, SubprocessFailure)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "empty.c", "--clearly-invalid-option",
         "invalid_file.c"});
    EXPECT_THROW(Deps::get_file_info(command), subprocess_failed_error);
}

// The exact location and amount of headers
// pulled in by this include may differ on platform,
// so just check that both the file referenced and
// some other number of includes are present.
TEST(DepsTest, GlobalPathsAllowed)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "ctype_include.c"});
    std::set<std::string> expected = {"ctype_include.c"};
    const auto dependencies = Deps::get_file_info(command).d_dependencies;

    EXPECT_GT(dependencies.size(), 1);
    EXPECT_NE(dependencies.end(), dependencies.find("ctype_include.c"));
}

TEST(DepsTest, ClangCrtbegin)
{
    // clang-format off
    std::string clang_v_common =
        "clang version 9.0.0 (https://github.com/llvm/llvm-project/ 67510fac36d27b2e22c7cd955fc167136b737b93)\n"
        "Target: x86_64-unknown-linux-gnu\n"
        "Thread model: posix\n"
        "InstalledDir: /home/user/clang/bin\n"
        "Found candidate GCC installation: /usr/lib/gcc/i686-linux-gnu/5\n"
        "Found candidate GCC installation: /usr/lib/gcc/i686-linux-gnu/5.4.0\n"
        "Found candidate GCC installation: /usr/lib/gcc/i686-linux-gnu/6\n"
        "Found candidate GCC installation: /usr/lib/gcc/i686-linux-gnu/6.0.0\n"
        "Found candidate GCC installation: /usr/lib/gcc/x86_64-linux-gnu/5\n"
        "Found candidate GCC installation: /usr/lib/gcc/x86_64-linux-gnu/5.4.0\n"
        "Found candidate GCC installation: /usr/lib/gcc/x86_64-linux-gnu/6\n"
        "Found candidate GCC installation: /usr/lib/gcc/x86_64-linux-gnu/6.0.0\n"
        "Selected GCC installation: /usr/lib/gcc/x86_64-linux-gnu/5.4.0\n"
        "Candidate multilib: .;@m64\n"
        "Candidate multilib: 32;@m32\n"
        "Candidate multilib: x32;@mx32\n";
    // clang-format on

    std::string clang_v_dot = clang_v_common + "Selected multilib: .;@m64\n";
    std::string clang_v_foo = clang_v_common + "Selected multilib: foo;@m64\n";

    std::string expected_dot =
        "/usr/lib/gcc/x86_64-linux-gnu/5.4.0/crtbegin.o";
    std::string found = Deps::crtbegin_from_clang_v(clang_v_dot);
    EXPECT_EQ(expected_dot, found);

    std::string expected_foo =
        "/usr/lib/gcc/x86_64-linux-gnu/5.4.0/foo/crtbegin.o";
    found = Deps::crtbegin_from_clang_v(clang_v_foo);
    EXPECT_EQ(expected_foo, found);
}

TEST(ProductsTest, OutputArgument)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "-o", "some_output.exe", "empty.c"});

    auto products = Deps::get_file_info(command).d_possibleProducts;

    std::set<std::string> expected_products = {"some_output.exe"};
    EXPECT_EQ(expected_products, products);
}

TEST(ProductsTest, NormalizesPath)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "-o", "out/subdir/../../../empty",
         "empty.c"});

    auto products = Deps::get_file_info(command).d_possibleProducts;
    std::set<std::string> expected_products = {"../empty"};

    EXPECT_EQ(expected_products, products);
}

TEST(ProductsTest, OutputArgumentNoSpace)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "-osome_output.exe", "empty.c"});

    auto products = Deps::get_file_info(command).d_possibleProducts;
    std::set<std::string> expected_products = {"some_output.exe"};

    EXPECT_EQ(expected_products, products);
}

TEST(ProductsTest, DefaultCompileOutput)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "empty.c"});

    auto products = Deps::get_file_info(command).d_possibleProducts;
    std::set<std::string> expected_products = {"empty.o"};

    EXPECT_EQ(expected_products, products);
}

TEST(ProductsTest, DefaultPrecompiledHeaderOutput)
{
    if (ParsedCommand::commandBasename(RECC_PLATFORM_COMPILER) == "gcc") {
        const auto command = ParsedCommandFactory::createParsedCommand(
            {RECC_PLATFORM_COMPILER, "-c", "empty.h"});

        auto products = Deps::get_file_info(command).d_possibleProducts;
        std::set<std::string> expected_products = {"empty.h.gch"};

        EXPECT_EQ(expected_products, products);
    }
}

TEST(ProductsTest, DefaultCompileMDOutput)
{
    if (ParsedCommand::commandBasename(RECC_PLATFORM_COMPILER) == "gcc") {
        Env::parse_config_variables();
        const auto command = ParsedCommandFactory::createParsedCommand(
            {RECC_PLATFORM_COMPILER, "-c", "empty.c", "-MD"});

        auto products = Deps::get_file_info(command).d_possibleProducts;
        std::set<std::string> expected_products = {"empty.o", "empty.d"};

        EXPECT_EQ(expected_products, products);
    }
}

TEST(ProductsTest, DefaultCompileMDMFOutput)
{
    if (ParsedCommand::commandBasename(RECC_PLATFORM_COMPILER) == "gcc") {
        Env::parse_config_variables();
        const auto command = ParsedCommandFactory::createParsedCommand(
            {RECC_PLATFORM_COMPILER, "-c", "empty.c", "-MD", "-MF",
             "outputfile"});

        auto products = Deps::get_file_info(command).d_possibleProducts;
        std::set<std::string> expected_products = {"empty.o", "outputfile"};

        EXPECT_EQ(expected_products, products);
    }
}

TEST(ProductsTest, DefaultCompileMDMTOutput)
{
    if (ParsedCommand::commandBasename(RECC_PLATFORM_COMPILER) == "gcc") {
        Env::parse_config_variables();
        const auto command = ParsedCommandFactory::createParsedCommand(
            {RECC_PLATFORM_COMPILER, "-c", "empty.c", "-MD", "-MT", "foo.o"});

        auto products = Deps::get_file_info(command).d_possibleProducts;
        std::set<std::string> expected_products = {"empty.o", "empty.d"};

        EXPECT_EQ(expected_products, products);
    }
}

TEST(ProductsTest, DefaultCompileUnhandledOptionOutput)
{
    if (ParsedCommand::commandBasename(RECC_PLATFORM_COMPILER) == "gcc") {
        Env::parse_config_variables();
        const auto command = ParsedCommandFactory::createParsedCommand(
            {RECC_PLATFORM_COMPILER, "-c", "empty.c", "-Werror"});

        auto products = Deps::get_file_info(command).d_possibleProducts;
        std::set<std::string> expected_products = {"empty.o"};

        EXPECT_EQ(expected_products, products);
    }
}

TEST(ProductsTest, DefaultLinkOutput)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "empty.c"});

    auto products = Deps::get_file_info(command).d_possibleProducts;
    std::set<std::string> expected_products = {"a.out"};

    EXPECT_EQ(expected_products, products);
}

TEST(ProductsTest, Subdirectory)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "subdirectory/empty.c"});

    auto products = Deps::get_file_info(command).d_possibleProducts;

    std::set<std::string> expected_products = {"empty.o"};
    EXPECT_EQ(expected_products, products);
}

TEST(ProductsTest, SubdirectoryPrecompiledHeader)
{
    if (ParsedCommand::commandBasename(RECC_PLATFORM_COMPILER) == "gcc") {
        Env::parse_config_variables();
        const auto command = ParsedCommandFactory::createParsedCommand(
            {RECC_PLATFORM_COMPILER, "-c", "subdirectory/header.h"});

        auto products = Deps::get_file_info(command).d_possibleProducts;

        std::set<std::string> expected_products = {"header.h.gch"};
        EXPECT_EQ(expected_products, products);
    }
}

TEST(ProductsTest, SubdirectoryLink)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "subdirectory/empty.c"});

    auto products = Deps::get_file_info(command).d_possibleProducts;

    std::set<std::string> expected_products = {"a.out"};
    EXPECT_EQ(expected_products, products);
}

TEST(ProductsTest, DefaultOutputUnsupportedFileSuffix)
{
    Env::parse_config_variables();
    const auto command = ParsedCommandFactory::createParsedCommand(
        {RECC_PLATFORM_COMPILER, "-c", "empty.i"});

    EXPECT_THROW(Deps::get_file_info(command), std::invalid_argument);
}

TEST(ProductsTest, PreprocessorArgument)
{
    if (ParsedCommand::commandBasename(RECC_PLATFORM_COMPILER) == "gcc") {
        const auto command = ParsedCommandFactory::createParsedCommand(
            {RECC_PLATFORM_COMPILER, "-c", "empty.c", "-Wp,-MMD", "-o",
             "build.o"});
        std::set<std::string> deps = {"empty.c"};

        auto products = Deps::get_file_info(command).d_possibleProducts;
        std::set<std::string> expected_products = {"build.o", "build.d"};

        EXPECT_EQ(expected_products, products);
    }
}

TEST(ProductsTest, PreprocessorArgumentNoOutput)
{
    if (ParsedCommand::commandBasename(RECC_PLATFORM_COMPILER) == "gcc") {
        const auto command = ParsedCommandFactory::createParsedCommand(
            {RECC_PLATFORM_COMPILER, "-c", "empty.c", "-Wp,-MMD"});
        std::set<std::string> deps = {"empty.c"};

        auto products = Deps::get_file_info(command).d_possibleProducts;
        std::set<std::string> expected_products = {"empty.o", "empty.d"};

        EXPECT_EQ(expected_products, products);
    }
}

TEST(ProductsTest, PreprocessorArgumentMF)
{
    if (ParsedCommand::commandBasename(RECC_PLATFORM_COMPILER) == "gcc") {
        const auto command = ParsedCommandFactory::createParsedCommand(
            {RECC_PLATFORM_COMPILER, "-c", "empty.c", "-Wp,-MMD,-MF,mmfile",
             "-o", "build.o"});
        std::set<std::string> deps = {"empty.c"};

        auto products = Deps::get_file_info(command).d_possibleProducts;
        std::set<std::string> expected_products = {"build.o", "mmfile"};

        EXPECT_EQ(expected_products, products);
    }
}

TEST(ProductsTest, PreprocessorArgumentMFNoOutput)
{
    if (ParsedCommand::commandBasename(RECC_PLATFORM_COMPILER) == "gcc") {
        const auto command = ParsedCommandFactory::createParsedCommand(
            {RECC_PLATFORM_COMPILER, "-c", "empty.c", "-Wp,-MMD,-MF,mmfile"});
        std::set<std::string> deps = {"empty.c"};

        auto products = Deps::get_file_info(command).d_possibleProducts;
        std::set<std::string> expected_products = {"empty.o", "mmfile"};

        EXPECT_EQ(expected_products, products);
    }
}

#endif

TEST(DepsFromMakeRulesTest, GccStyleMakefile)
{
    std::string makeRules =
        "sample.o: sample.c sample.h /usr/include/cstring.h \\\n"
        "   subdir/sample.h\n"
        "rule2.o: sample.h";
    std::set<std::string> expected = {
        "sample.c", "sample.h", "/usr/include/cstring.h", "subdir/sample.h"};

    auto dependencies = Deps::dependencies_from_make_rules(makeRules);

    EXPECT_EQ(expected, dependencies);
}

TEST(DepsFromMakeRulesTest, SunStyleMakefile)
{
    std::string makeRules = "sample.o : ./sample.c\n"
                            "sample.o : ./sample.h\n"
                            "sample.o : /usr/include/cstring.h\n"
                            "sample.o : ./subdir/sample.h\n"
                            "rule2.o : ./sample.h\n"
                            "rule3.o : ./sample with spaces.c";
    std::set<std::string> expected = {
        "./sample.c", "./sample.h", "/usr/include/cstring.h",
        "./subdir/sample.h", "./sample with spaces.c"};

    auto dependencies = Deps::dependencies_from_make_rules(makeRules, true);

    EXPECT_EQ(expected, dependencies);
}

TEST(DepsFromMakeRulesTest, LargeMakeOutput)
{
    auto makeRules =
        buildboxcommon::FileUtils::getFileContents("giant_make_output.mk");

    auto dependencies = Deps::dependencies_from_make_rules(makeRules);

    EXPECT_EQ(679, dependencies.size());
    EXPECT_NE(dependencies.end(), dependencies.find("hello.c"));
    EXPECT_NE(dependencies.end(), dependencies.find("hello.h"));
    EXPECT_NE(dependencies.end(), dependencies.find("final_dependency.h"));
}
