#...............................................................................
#
#  This file is part of the Graco toolkit.
#
#  Graco is distributed under the MIT license.
#  For details see accompanying license.txt file,
#  the public copy of which is also available at:
#  http://tibbo.com/downloads/archive/graco/license.txt
#
#...............................................................................

set(
	AXL_PATH_LIST

	LUA_INC_DIR
	LUA_LIB_DIR
	LUA_LIB_NAME
	AXL_CMAKE_DIR
	RAGEL_EXE
	7Z_EXE
	SPHINX_BUILD_EXE
	PDFLATEX_EXE
)

set(
	AXL_IMPORT_LIST

	REQUIRED
		ragel
		axl
		lua
	OPTIONAL
		7z
		sphinx
		latex
	)

#...............................................................................
