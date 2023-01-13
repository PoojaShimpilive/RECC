find_package(gRPC)
if(gRPC_FOUND)
    set(GRPC_TARGET gRPC::grpc++)
    set(GRPC_CPP_PLUGIN $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
else()
    find_package(PkgConfig REQUIRED)
    if(BUILD_STATIC)
        list(APPEND PKG_CONFIG_EXECUTABLE "--static")
    endif()
    pkg_check_modules(gRPC REQUIRED IMPORTED_TARGET grpc++)
    if(BUILD_STATIC)
        list(POP_BACK PKG_CONFIG_EXECUTABLE)
    endif()
    set(GRPC_TARGET PkgConfig::gRPC)
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
endif()

if(BUILD_STATIC)
    find_package(ZLIB REQUIRED)
    # When statically linking against grpc++, it would appear
    # that providing -lgrpc++ is not enough. There are known issues
    # with this - one example is https://github.com/grpc/grpc/issues/15054.
    # Some experimentation revealed that including a "-L</path/to/other/grpc/libs -lgrpc"
    # rule allowed the linker to resolve all symbols and succeed.
    # To accomplish this, we use any one of the populated CMake variables
    # that provides the path and then set the
    # 'STATIC_GRPC_LINKER_RULE' to be used in downstream CMake code.
    if (NOT ${gRPC_LIBRARY_DIRS} STREQUAL "")
        set(STATIC_GRPC_LINKER_RULE "-L${gRPC_LIBRARY_DIRS} -lgrpc")
    elseif (NOT ${gRPC_LIBDIR} STREQUAL "")
        set(STATIC_GRPC_LINKER_RULE "-L${gRPC_LIBDIR} -lgrpc")
    endif()
endif()
