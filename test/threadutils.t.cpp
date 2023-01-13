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

#include <algorithm>
#include <env.h>
#include <functional>
#include <gtest/gtest.h>
#include <threadutils.h>
#include <vector>

using namespace recc;

std::mutex ContainerWriteMutex;
TEST(ParallelizeContainerOperations, EvenNumberElements)
{
    // Even number of elements
    std::vector<int> testVector(60, 1);
    std::vector<int> expectedVector(60, 2);
    std::vector<int> pushBackVector;

    std::function<void(std::vector<int>::iterator, std::vector<int>::iterator)>
        doubleVectorValues = [&pushBackVector](
                                 std::vector<int>::iterator start,
                                 std::vector<int>::iterator end) {
            for (; start < end; ++start) {
                const std::lock_guard<std::mutex> lock(ContainerWriteMutex);
                pushBackVector.push_back(*start * 2);
            }
        };

    ThreadUtils::parallelizeContainerOperations(testVector,
                                                doubleVectorValues);
    EXPECT_EQ(pushBackVector.size(), expectedVector.size());
    EXPECT_EQ(pushBackVector, expectedVector);
    // Even elements, odd number of threads
    RECC_MAX_THREADS = 3;
    pushBackVector.clear();
    ThreadUtils::parallelizeContainerOperations(testVector,
                                                doubleVectorValues);
    EXPECT_EQ(pushBackVector.size(), expectedVector.size());
    EXPECT_EQ(pushBackVector, expectedVector);
}

TEST(ParallelizeContainerOperations, OddNumberElements)
{
    // Odd number of elements
    std::vector<int> testVector(59, 1);
    std::vector<int> expectedVector(59, 2);
    std::vector<int> pushBackVector;

    std::function<void(std::vector<int>::iterator, std::vector<int>::iterator)>
        doubleVectorValues = [&pushBackVector](
                                 std::vector<int>::iterator start,
                                 std::vector<int>::iterator end) {
            for (; start < end; ++start) {
                const std::lock_guard<std::mutex> lock(ContainerWriteMutex);
                pushBackVector.push_back(*start * 2);
            }
        };

    ThreadUtils::parallelizeContainerOperations(testVector,
                                                doubleVectorValues);
    EXPECT_EQ(pushBackVector.size(), expectedVector.size());
    EXPECT_EQ(pushBackVector, expectedVector);

    // Odd elements, odd number of threads
    RECC_MAX_THREADS = 3;
    pushBackVector.clear();
    ThreadUtils::parallelizeContainerOperations(testVector,
                                                doubleVectorValues);
    EXPECT_EQ(pushBackVector.size(), expectedVector.size());
    EXPECT_EQ(pushBackVector, expectedVector);
}

TEST(ParallelizeContainerOperations, LargeNumberOfElements)
{
    // Very large number of elements
    std::vector<int> testVector(5000, 1);
    std::vector<int> expectedVector(5000, 2);
    std::vector<int> pushBackVector;

    std::function<void(std::vector<int>::iterator, std::vector<int>::iterator)>
        doubleVectorValues = [&pushBackVector](
                                 std::vector<int>::iterator start,
                                 std::vector<int>::iterator end) {
            for (; start < end; ++start) {
                const std::lock_guard<std::mutex> lock(ContainerWriteMutex);
                pushBackVector.push_back(*start * 2);
            }
        };

    ThreadUtils::parallelizeContainerOperations(testVector,
                                                doubleVectorValues);
    EXPECT_EQ(pushBackVector.size(), expectedVector.size());
    EXPECT_EQ(pushBackVector, expectedVector);
}

TEST(ParallelizeContainerOperations, EdgeCasesElements)
{
    std::vector<int> testVector(60, 1);
    std::vector<int> expectedVector(60, 2);
    std::vector<int> pushBackVector;

    std::function<void(std::vector<int>::iterator, std::vector<int>::iterator)>
        doubleVectorValues = [&pushBackVector](
                                 std::vector<int>::iterator start,
                                 std::vector<int>::iterator end) {
            for (; start < end; ++start) {
                const std::lock_guard<std::mutex> lock(ContainerWriteMutex);
                pushBackVector.push_back(*start * 2);
            }
        };

    ThreadUtils::parallelizeContainerOperations(testVector,
                                                doubleVectorValues);
    EXPECT_EQ(pushBackVector.size(), expectedVector.size());
    EXPECT_EQ(pushBackVector, expectedVector);

    // set numThreads to 0
    testVector.resize(50);
    expectedVector.resize(50);
    pushBackVector.clear();

    RECC_MAX_THREADS = 0;
    ThreadUtils::parallelizeContainerOperations(testVector,
                                                doubleVectorValues);
    EXPECT_EQ(pushBackVector.size(), expectedVector.size());
    EXPECT_EQ(pushBackVector, expectedVector);

    // less than 50 elements
    testVector.resize(20);
    expectedVector.resize(20);
    pushBackVector.clear();

    ThreadUtils::parallelizeContainerOperations(testVector,
                                                doubleVectorValues);
    EXPECT_EQ(pushBackVector.size(), expectedVector.size());
    EXPECT_EQ(pushBackVector, expectedVector);
}

TEST(ParallelizeContainerOperations, AllElementsAdded)
{
    // Ensure ALL elements are added when using multiple threads to append.
    std::vector<int> testVector;
    std::vector<int> expectedVector;
    std::vector<int> pushBackVector;
    const int num_items = 500;

    for (unsigned int i = 0; i < num_items; i++) {
        testVector.push_back(i);
        expectedVector.push_back(i * 2);
    }

    std::function<void(std::vector<int>::iterator, std::vector<int>::iterator)>
        doubleVectorValues = [&pushBackVector](
                                 std::vector<int>::iterator start,
                                 std::vector<int>::iterator end) {
            for (; start < end; ++start) {
                const std::lock_guard<std::mutex> lock(ContainerWriteMutex);
                pushBackVector.emplace_back(*start * 2);
            }
        };
    ThreadUtils::parallelizeContainerOperations(testVector,
                                                doubleVectorValues);
    EXPECT_EQ(pushBackVector.size(), expectedVector.size());
    EXPECT_TRUE(
        std::is_permutation(pushBackVector.begin(), pushBackVector.end(),
                            expectedVector.begin(), expectedVector.end()));
}
