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

Libraries
=========

These libraries are **required** for building the Graco.

* Lua

	Graco uses Lua string templates for generating C++ code from the ``.llk`` grammar files. Therefore, Lua headers and libraries are required for building Graco.

	.. expand-macro:: lua-common-info

* AXL

	Graco uses AXL as a general purpose C++ support library.

	.. expand-macro:: axl-bundle-info Graco graco graco_b
