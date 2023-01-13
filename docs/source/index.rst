recc
====

``recc`` is the Remote Execution Caching Compiler. It is a cross between
``ccache`` and ``distcc`` using the `Remote Execution
APIs <https://github.com/bazelbuild/remote-apis>`_.

When invoked with a C/C++ compilation command, it
will use the APIs to communicate with a Remote Execution service (such
as `BuildGrid <http://buildgrid.build/>`_) to first determine whether
the same build has been done before and is cached. If the build is
cached remotely, download the result. Otherwise, enqueue the build to be
executed remotely using the same execution service which will then
dispatch the job to a worker, such as
`buildbox-worker <https://gitlab.com/BuildGrid/buildbox/buildbox-worker>`_.

More information about ``recc`` and ``BuildGrid`` can be found in the
`presentation <https://www.youtube.com/watch?v=w1ZA4Rrf91I>`__ we gave
at Bazelcon 2018, and on this `blog
post <https://www.codethink.com/articles/2018/introducing-buildgrid/>`_.

``recc`` is in active development and in use at `Bloomberg <https://www.techatbloomberg.com/>`__.
On Linux it primarily supports gcc; and on Solaris/Aix it supports
vendor compilers. Clang support is currently (very) experimental.

For information regarding contributing to this project, please read the
`Contribution guide <https://gitlab.com/BuildGrid/recc/-/blob/master/CONTRIBUTING.md>`_.


.. toctree::
   :maxdepth: 1
   :caption: Contents:

   installation.rst
   running.rst
   cmake-integration.rst
   additional-utilities.rst


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
