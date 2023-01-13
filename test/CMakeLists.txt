# Every directory containing a source file needed by the tests, must be added here.
include_directories(fixtures/ env/ ../src/)

# This macro creates the specific test executable, linking against defined above, google test and google mock.
# Current limitation is the TEST_SOURCE must only be one file.
# If more than 1 file is needed, combine the sources into a list.
macro(add_recc_test TEST_NAME TEST_SOURCE)
    # Create a separate test executable per test source.
    add_executable(${TEST_NAME} ${TEST_SOURCE})

    # This allows us to pass an optional argument if the cwd for the test is not the default.
    set(ExtraMacroArgs ${ARGN})
    list(LENGTH ExtraMacroArgs NumExtraArgs)
    if(${NumExtraArgs} GREATER 0)
      list(GET ExtraMacroArgs 0 TEST_WORKING_DIRECTORY)
    else()
      set(TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY})
    target_link_libraries(${TEST_NAME} PUBLIC
        ${_EXTRA_LDD_FLAGS}
        remoteexecution
        ${GTEST_TARGET}
        ${GMOCK_TARGET}
        ${GTEST_MAIN_TARGET}
    )
endmacro()

add_recc_test(subprocess_tests subprocess.t.cpp)
add_recc_test(parsedcommand_tests parsedcommand.t.cpp)
add_recc_test(digestgenerator_tests digestgenerator.t.cpp)
add_recc_test(remoteexecutionclient_tests remoteexecutionclient.t.cpp)
add_recc_test(fileutils_tests fileutils.t.cpp)
add_recc_test(requestmetadata_tests requestmetadata.t.cpp)
add_recc_test(threading_tests threadutils.t.cpp)
add_recc_test(parsed_command_factory_tests parsedcommandfactory.t.cpp)

add_recc_test(env_set_test env/env_set.t.cpp)
add_recc_test(env_default_cas_test env/env_default_cas.t.cpp)
add_recc_test(env_default_action_cache_test env/env_default_action_cache.t.cpp)
add_recc_test(env_different_cas_test env/env_different_cas.t.cpp)
add_recc_test(env_different_action_cache_test env/env_different_action_cache.t.cpp)
add_recc_test(env_file_advertised_order_test env/env_file_advertised_order.t.cpp)
add_recc_test(env_check_order_test env/env_check_order.t.cpp)
add_recc_test(env_utils_test env/env_utils.t.cpp)
add_recc_test(env_path_vector_test env/env_path_vector.t.cpp)
add_recc_test(env_reapi_version_test env/env_reapi_version.t.cpp)

# These tests include an extra arg, containing the working directory of the test.
add_recc_test(actionbuilder_tests actionbuilder.t.cpp ${CMAKE_CURRENT_SOURCE_DIR}/data/actionbuilder)
add_recc_test(deps_tests deps.t.cpp ${CMAKE_CURRENT_SOURCE_DIR}/data/deps)
add_recc_test(env_from_file_override_test env/env_from_file_override.t.cpp ${CMAKE_CURRENT_SOURCE_DIR}/data/)
add_recc_test(env_multiple_configs_test env/env_multiple_configs.t.cpp ${CMAKE_CURRENT_SOURCE_DIR}/data/)
add_recc_test(env_from_file_test env/env_from_file.t.cpp ${CMAKE_CURRENT_SOURCE_DIR}/data/)
