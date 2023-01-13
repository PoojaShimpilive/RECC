 .. _recc-installation:

Installation
============

Dependencies
------------

Currently recc relies on:

-  `buildbox-common <https://gitlab.com/BuildGrid/buildbox/buildbox-common>`__
-  `gRPC <https://grpc.io/>`__
-  `Protobuf <https://github.com/google/protobuf/>`__
-  `OpenSSL <https://www.openssl.org/>`__
-  `CMake <https://cmake.org/>`__
-  `GoogleTest <https://github.com/google/googletest>`__
-  `pkg-config <https://www.freedesktop.org/wiki/Software/pkg-config/>`__

Installing buildbox-common
~~~~~~~~~~~~~~~~~~~~~~~~~~

Follow the instructions to install buildbox-common using the README
located here: https://gitlab.com/BuildGrid/buildbox/buildbox-common

Once buildbox-common is installed, you should have the necessary
dependencies to run recc.

Compiling
---------

Once you've `installed the dependencies <#dependencies>`__, you can
compile ``recc`` using the following commands:

.. code:: sh

    $ mkdir build
    $ cd build
    $ cmake .. && make

If you want to set the instance name at compile time, then you may pass
in a special flag:

.. code:: sh

    cmake -DDEFAULT_RECC_INSTANCE=name_you_want ../ && make

Note that on macOS, you'll need to manually specify the locations of
OpenSSL and GoogleTest when running CMake:

.. code:: sh

    $ cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DGTEST_SOURCE_ROOT=/wherever/you/unzipped/googletest/to .. && make

The optional flag ``-DRECC_VERSION`` allows setting the version string
that recc will attach as metadata to its request headers. If none is
set, CMake will try to determine the current git commit and use the
short SHA as a version value. (If that fails, the version will be set to
"unknown".)

Running tests
~~~~~~~~~~~~~

To run tests, first compile the project (see above), then run:

.. code:: sh

    $ make test

Running tests on macOS Mojave(10.14)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``/usr/include/``'s has been deprecated on Mojave, for certain tests to
work, you must install Apple command line tools.

.. code:: sh

    $ xcode-select --install
    $ sudo installer -pkg /Library/Developer/CommandLineTools/Packages/macOS_SDK_headers_for_macOS_10.14.pkg -target /

Compiling statically
~~~~~~~~~~~~~~~~~~~~

You can compile recc statically with the ``-DBUILD_STATIC=ON`` option.
All of recc's dependencies must be available as static libraries
(``.a``\ files) and visible in ``${CMAKE_MODULE_PATH}``.
