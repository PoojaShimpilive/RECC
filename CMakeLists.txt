cmake_minimum_required(VERSION 3.13)

project(recc VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_STATIC "Build statically" OFF)
if(BUILD_STATIC)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
    message(STATUS "${CMAKE_CURRENT_LIST_FILE}: setting CMAKE_FIND_LIBRARY_SUFFIXES to ${CMAKE_FIND_LIBRARY_SUFFIXES}")
endif()

find_package(BuildboxCommon REQUIRED)
if(NOT BuildboxCommon_FOUND)
        message(FATAL_ERROR "recc requires linking against the buildbox-common library. Please build and install it first.")
endif()

include(cmake/deps.cmake)
include(GNUInstallDirs)

# gcc on AIX can't deal with -isystem that contains C++ .h files
if(${CMAKE_SYSTEM_NAME} MATCHES "AIX" AND ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    set(CMAKE_NO_SYSTEM_FROM_IMPORTED ON)
endif()

# Platform-specific settings
if(CMAKE_SYSTEM_NAME MATCHES "SunOS")
    add_definitions(-DRECC_PLATFORM_COMPILER="CC")
    if(BUILD_STATIC)
        set(_EXTRA_LDD_FLAGS "-static-libgcc -static-libstdc++ -Wl,-z,relax=comdat")
        set(OS_LIBS "-lresolv -lsocket -lnsl")
    endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "AIX")
    add_definitions(-DRECC_PLATFORM_COMPILER="xlc")
    if(BUILD_STATIC)
        # Depending on versions, static linking of runtimes using gcc can
        # trigger some bugs
        # e.g. https://gcc.gnu.org/bugzilla/show_bug.cgi?id=33704 If using
        # static linking with gcc on aix be sure to run the tests. Failures
        # with GTEST EXPECT_THROW etc. originate from these bugs and are fixed
        # using the linker flag `-blazy` as detailed in the above bug
        # report. Hence inclusion here.
        set(_EXTRA_LDD_FLAGS "-static-libgcc -static-libstdc++ -Wl,-bbigtoc,-blazy")
    else()
        # when dynamically linking on AIX, ie; setting -DBUILD_STATIC=OFF,
        # we can get a "Table Of Contents too big" error, so we set --bbigtoc here
        set(_EXTRA_LDD_FLAGS "-Wl,-bbigtoc")
    endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-DRECC_PLATFORM_COMPILER="gcc")
    if(BUILD_STATIC)
        set(OS_LIBS "-lresolv -ldl")
    endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    add_definitions(-DRECC_PLATFORM_COMPILER="clang")
endif()

# Determine recc's version if none was provided
if(NOT RECC_VERSION)
    find_program(GIT git)

    if(GIT STREQUAL "GIT-NOTFOUND")
        set(GIT_SHA "unknown")
        message("Could not find git, setting version to 'unknown'")
    else()
        execute_process(COMMAND ${GIT} rev-parse --verify HEAD --short
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        OUTPUT_VARIABLE GIT_SHA
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
        message(STATUS "Using git commit as version: ${GIT_SHA}")
    endif()

    set(RECC_VERSION ${GIT_SHA})
endif()

add_definitions(-DRECC_VERSION="${RECC_VERSION}")

# define DEFAULT_RECC_INSTANCE
add_definitions(-DDEFAULT_RECC_INSTANCE="${DEFAULT_RECC_INSTANCE}")

# define CMAKE_INSTALL_DIR
add_definitions(-DCMAKE_INSTALL_DIR="${CMAKE_INSTALL_PREFIX}")

# define RECC_CONFIG_PREFIX_DIR
add_definitions(-DRECC_CONFIG_PREFIX_DIR="${RECC_CONFIG_PREFIX_DIR}")

add_subdirectory(src)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ReccConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(cmake/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/ReccConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/recc
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ReccConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ReccConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/recc
)

include(CTest)
if(BUILD_TESTING)
    find_file(BuildboxGTestSetup BuildboxGTestSetup.cmake HINTS ${BuildboxCommon_DIR})
    include(${BuildboxGTestSetup})
    include_directories(third_party/grpc/include)
    add_subdirectory(test)
endif()
