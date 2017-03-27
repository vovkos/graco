.. .............................................................................
..
..  This file is part of the Graco toolkit.
..
..  Graco is distributed under the MIT license.
..  For details see accompanying license.txt file,
..  the public copy of which is also available at:
..  http://tibbo.com/downloads/archive/graco/license.txt
..
.. .............................................................................

paths.cmake
===========

.. expand-macro:: paths-cmake Graco

.. code-block:: bash

	LUA_INC_DIR         # path to Lua C include directory
	LUA_LIB_DIR         # path to Lua library directory
	LUA_LIB_NAME        # (optional) name of Lua library (lua/lua51/lua52/lua53)
	RAGEL_EXE           # path to Ragel executable
	7Z_EXE              # (optional) path to 7-Zip executable
	SPHINX_BUILD_EXE    # (optional) path to Sphinx compiler executable sphinx-build
	PDFLATEX_EXE        # (optional) path to Latex-to-PDF compiler

.. expand-macro:: dependencies-cmake AXL

On Windows you will need to specify paths to Lua librariy -- they are unlikely to be found automatically.

.. rubric:: Sample paths.cmake on Windows:

.. code-block:: cmake

	set (LUA_VERSION   5.2.1)
	set (RAGEL_VERSION 6.7)

	set (LUA_INC_DIR c:/Develop/lua/lua-${LUA_VERSION}/include)

	if ("${TARGET_CPU}" STREQUAL "amd64")
		set (LUA_LIB_DIR c:/Develop/lua/lua-${LUA_VERSION}/lib/amd64/${CONFIGURATION_SUFFIX})
	else ()
		set (LUA_LIB_DIR c:/Develop/lua/lua-${LUA_VERSION}/lib/x86/${CONFIGURATION_SUFFIX})
	endif()

	set (7Z_EXE           "c:/Program Files/7-Zip/7z.exe")
	set (RAGEL_EXE        c:/Develop/ragel/ragel-${RAGEL_VERSION}/ragel.exe)
	set (SPHINX_BUILD_EXE c:/Develop/ActivePython/Scripts/sphinx-build.exe)
	set (PDFLATEX_EXE     "c:/Program Files (x86)/MiKTeX 2.9/miktex/bin/pdflatex.exe")
