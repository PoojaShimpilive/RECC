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

using namespace recc;

TEST(EnvUtilsTest, SubstringAtNthTest)
{
    auto test_string = "HELLO_WORLD";
    ASSERT_EQ("HELLO", Env::substring_until_nth_token(test_string, "_", 1));
    ASSERT_EQ("", Env::substring_until_nth_token(test_string, "_", 2));
    ASSERT_EQ("", Env::substring_until_nth_token(test_string, "_", 5));

    test_string = "WEST/WORLD/HI";
    ASSERT_EQ("WEST", Env::substring_until_nth_token(test_string, "/", 1));
    ASSERT_EQ("", Env::substring_until_nth_token(test_string, "_", 1));
    ASSERT_EQ("WEST/WORLD",
              Env::substring_until_nth_token(test_string, "/", 2));

    test_string = "HELLO_";
    ASSERT_EQ("HELLO", Env::substring_until_nth_token(test_string, "_", 1));
    ASSERT_EQ("", Env::substring_until_nth_token(test_string, "_", 3));
    ASSERT_EQ("", Env::substring_until_nth_token(test_string, "_", 4));

    test_string = "HELLO_WORLD_HELLO";
    ASSERT_EQ("HELLO", Env::substring_until_nth_token(test_string, "_", 1));
    ASSERT_EQ("HELLO_WORLD",
              Env::substring_until_nth_token(test_string, "_", 2));
    ASSERT_EQ("", Env::substring_until_nth_token(test_string, "_", 3));

    test_string = "HELLO_WORLD_HELLO_WORLD";
    ASSERT_EQ("HELLO", Env::substring_until_nth_token(test_string, "_", 1));
    ASSERT_EQ("HELLO_WORLD",
              Env::substring_until_nth_token(test_string, "_", 2));
    ASSERT_EQ("HELLO_WORLD_HELLO",
              Env::substring_until_nth_token(test_string, "_", 3));
}
