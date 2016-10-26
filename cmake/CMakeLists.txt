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

configure_file (
	graco_config.cmake.in
	graco_config.cmake
	@ONLY
	)

# make GRACO_CMAKE_DIR available to any project within the same source tree

set (
	GRACO_CMAKE_DIR
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	CACHE INTERNAL
	"In-source-tree Graco CMake directories"
	)

#...............................................................................
