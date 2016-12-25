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

CMake Configuration Step
========================

.. expand-macro:: cmake-configure graco_b

.. rubric:: Sample Linux output:

::

	AXL CMake:
	    Invoked from:        /home/vladimir/Projects/graco_b/CMakeLists.txt
	    dependencies.cmake:  /home/vladimir/Projects/graco_b/dependencies.cmake
	    settings.cmake:      /home/vladimir/Projects/settings.cmake
	    paths.cmake:         /home/vladimir/Projects/paths.cmake
	    Target CPU:          amd64
	    Build configuration: Debug
	C/C++:
	    C Compiler:          /usr/bin/cc
	    C flags (Debug):     -m64 -mcx16 -fPIC -fvisibility=hidden -Wno-multichar -g
	    C flags (Release):   -m64 -mcx16 -fPIC -fvisibility=hidden -Wno-multichar -O3 -DNDEBUG
	    C++ Compiler:        /usr/bin/c++
	    C++ flags (Debug):   -m64 -mcx16 -fPIC -fvisibility=hidden -Wno-multichar -g
	    C++ flags (Release): -m64 -mcx16 -fPIC -fvisibility=hidden -Wno-multichar -O3 -DNDEBUG
	Dependency path definitions:
	    7Z_EXE:              /usr/bin/7z
	    LUA_INC_DIR:         /home/vladimir/Develop/lua/lua-5.2.1/include
	    LUA_LIB_DIR:         /home/vladimir/Develop/lua/lua-5.2.1/lib-amd64
	    RAGEL_EXE:           /usr/bin/ragel
	Lua paths:
	    Includes:            /home/vladimir/Develop/lua/lua-5.2.1/include
	    Library dir:         /home/vladimir/Develop/lua/lua-5.2.1/lib-amd64
	    Library name:        lua
	AXL paths:
	    CMake files:         /home/vladimir/Projects/graco_b/axl/cmake;/home/vladimir/Projects/graco_b/build/axl/cmake
	    Includes:            /home/vladimir/Projects/graco_b/axl/include
	    Libraries:           /home/vladimir/Projects/graco_b/build/axl/lib/Debug
	Configuring done
