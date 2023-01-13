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

#ifndef INCLUDED_THREADUTILS
#define INCLUDED_THREADUTILS

#include <env.h>
#include <mutex>
#include <thread>

namespace recc {

struct ThreadUtils {

    /**
     * Apply doWorkInRange to a range of elements in the container, in
     * parallel. Will partition the container according to the RECC_MAX_THREADS
     * global variable.
     * If there are less than 50 elements in the container, the work will not
     * be parallelized.
     *
     * NOTE: This fuction makes no guarantees about thread safety or
     * ordering of the parallel operations done in doWorkInRange. It is up to
     * the caller to ensure operations performed in doWorkInRange are
     * thread-safe and ordered.
     */
    template <class ContainerT>
    static void parallelizeContainerOperations(
        ContainerT &container,
        std::function<void(typename ContainerT::iterator,
                           typename ContainerT::iterator)> &doWorkInRange)
    {
        int numThreads = RECC_MAX_THREADS;
        typename ContainerT::iterator start = container.begin();
        typename ContainerT::iterator end = container.end();
        const auto containerLength = container.size();
        if (containerLength < 50 || numThreads == 0) {
            doWorkInRange(start, end);
        }
        else {
            if (numThreads < 0) {
                // This call can return 0. If so, default to numThreads.
                int available_threads =
                    static_cast<int>(std::thread::hardware_concurrency());
                numThreads =
                    available_threads ? available_threads : numThreads;
            }
            if (numThreads == 0) {
                numThreads = 1;
            }

            auto positiveNumThreads = static_cast<unsigned long>(numThreads);
            const auto numItemsPerPartition =
                containerLength / positiveNumThreads;
            std::vector<std::thread> threadObjects;
            // The conversion to unsigned is fine here, as we handle negative
            // values for numThreads above.
            threadObjects.reserve(positiveNumThreads);
            end = start;
            for (auto partition = 0; partition < numThreads; ++partition) {
                // To handle a none evenly divisible number of objects, the
                // last partition goes to the end of the range.
                //
                // Additionally, the main thread will process the remaining
                // elements.
                if (partition == numThreads - 1) {
                    end = container.end();
                    doWorkInRange(start, end);
                    continue;
                }
                else {
                    std::advance(end, static_cast<long>(numItemsPerPartition));
                }
                threadObjects.push_back(
                    std::thread(std::ref(doWorkInRange), start, end));
                start = end;
            }
            for (auto &thread : threadObjects) {
                thread.join();
            }
        }
    }
};

} // namespace recc

#endif
