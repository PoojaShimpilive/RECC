FROM registry.gitlab.com/buildgrid/buildbox/buildbox-common:latest

ARG ENABLE_TESTS=true
ARG EXTRA_CMAKE_FLAGS=
ARG RUN_GCOV=
ARG GCOV_FLGS="-g -O0 -fprofile-arcs -ftest-coverage"
ARG MAKE_VERBOSE=

ENV TEST_DEPENDS=${ENABLE_TESTS:+googletest}

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    google-mock \
    libabsl-dev \
    libc-ares-dev \
    libssl-dev \
    make \
    pkg-config \
    "${TEST_DEPENDS}" \
    && apt-get clean

# The grpc pkg-config file in Debian 11 is missing the Abseil libraries,
# which breaks static linking. The upstream pkg-config file in recent grpc
# seems correct, though. Patch pkg-config file in this container:
RUN sed -i -e '/^Libs.private:/s/$/ -labsl_hash -labsl_city -labsl_raw_hash_set -labsl_hashtablez_sampler -labsl_exponential_biased -labsl_statusor -labsl_status -labsl_cord -labsl_bad_optional_access -labsl_bad_variant_access -labsl_str_format_internal -labsl_synchronization -labsl_graphcycles_internal -labsl_stacktrace -labsl_symbolize -labsl_debugging_internal -labsl_demangle_internal -labsl_malloc_internal -labsl_time -labsl_civil_time -labsl_strings -labsl_strings_internal -labsl_base -labsl_spinlock_wait -labsl_int128 -labsl_throw_delegate -labsl_raw_logging_internal -labsl_log_severity -labsl_time_zone/' /usr/lib/x86_64-linux-gnu/pkgconfig/grpc.pc

WORKDIR /recc
COPY . .

# "build" directory is in .dockerignore
RUN mkdir build && cd build && \
    if [ -z "${ENABLE_TESTS}" ]; then \
        echo "Building without tests" && \
        cmake -DBUILD_TESTING=OFF "${EXTRA_CMAKE_FLAGS}" .. && \
        make -j$(nproc) && \
        echo "Produced the following binaries: " && find /recc/build/bin/ \
    ;else \
        echo "Building with tests" && \
        if [ -n "${RUN_GCOV}" ]; then \
            GCOV_CMAKE_FLAGS="-DCMAKE_CXX_FLAGS=$GCOV_FLGS -DCMAKE_EXE_LINKER_FLAGS=$GCOV_FLGS -DCMAKE_SHARED_LINKER_FLAGS=$GCOV_FLGS -DCMAKE_CXX_OUTPUT_EXTENSION_REPLACE=ON" \
        ;fi && \
        cmake -DBUILD_TESTING=ON -DGTEST_SOURCE_ROOT=/usr/src/googletest "${EXTRA_CMAKE_FLAGS}" "${GCOV_CMAKE_FLAGS-}" .. && \
        VERBOSE=${MAKE_VERBOSE} make -j$(nproc) && ctest --verbose && \
        echo "Produced the following binaries: " && find /recc/build/bin/ \
    ;fi

# Extend PATH to include the recc binaries
ENV PATH "/recc/build/bin:$PATH"

# Generate GCOV files (if enabled)
# Only keep files under the src/ directory;
# more files will be generated from libraries, protos, etc
RUN if [ -n "${ENABLE_TESTS}" ] && [ -n "${RUN_GCOV}" ]; then \
        echo "Generating gcov files..." && \
        mkdir /recc/gcov && cd /recc/gcov && \
        find "/recc/" \
            -type f -name "*.gcno" \
            -exec gcov \
                --branch-counts \
                --branch-probabilities \
                --preserve-paths "{}" \;  && \
        find "/recc/gcov/" -type f \
            ! -name "#recc#src#*" \
            -delete \
    ;fi
