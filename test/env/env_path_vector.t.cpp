// Copyright 2019 Bloomberg Finance L.P
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
#include <gtest/gtest.h>

typedef std::vector<std::pair<std::string, std::string>>
    vector_of_string_pairs;
using namespace recc;

TEST(EnvPathMap, MultipleInputs)
{
    std::string testString = "/hello=/recc:/hi=/be";
    vector_of_string_pairs test_vector = {std::make_pair("/hello", "/recc"),
                                          std::make_pair("/hi", "/be")};
    auto return_vector = Env::vector_from_delimited_string(testString);
    ASSERT_EQ(test_vector, return_vector);
}

TEST(EnvPathMap, TrailingColon)
{
    auto testString = "/hello=/recc:";
    vector_of_string_pairs test_vector = {std::make_pair("/hello", "/recc")};
    auto return_vector = Env::vector_from_delimited_string(testString);
    ASSERT_EQ(test_vector, return_vector);
}

TEST(EnvPathMap, NoValueForKey)
{
    auto testString = "/usr/bin/path=/usr/bin:/usr";
    vector_of_string_pairs test_vector = {
        std::make_pair("/usr/bin/path", "/usr/bin")};
    auto return_vector = Env::vector_from_delimited_string(testString);
    ASSERT_EQ(test_vector, return_vector);
}

TEST(EnvPathMap, NotAbsolutePaths)
{
    auto testString = "hello=recc";
    vector_of_string_pairs test_vector = {};
    auto return_vector = Env::vector_from_delimited_string(testString);
    ASSERT_EQ(test_vector, return_vector);
}
