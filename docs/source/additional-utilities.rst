.. _recc-additional-utilities:

Additional utilities
--------------------

This repo includes a couple of additional utilities that may be useful
for debugging. They aren't installed by default, but running ``make``
will place them in the ``bin`` subdirectory of the project root.

-  ``deps [command]`` - Print the names of the files needed to run the
   given command. (``recc`` uses this to decide which files to send to
   the Remote Execution server.)