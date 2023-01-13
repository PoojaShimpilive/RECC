.. _recc-cmake-integration:

CMake Integration
-----------------

To integrate ``recc`` with CMake, replace/set these variables in your
toolchain file.

.. code:: cmake

    set(CMAKE_C_COMPILER_LAUNCHER ${LOCATION_OF_RECC})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${LOCATION_OF_RECC})

Setting these flags will invoke recc, with the compiler and generated
arguments.
