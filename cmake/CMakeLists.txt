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

string (REGEX REPLACE "[a-z]+" ".." GRACO_INSTALL_ROOT_SUBDIR ${GRACO_INSTALL_CMAKE_SUBDIR})

configure_file (
	graco_config_build.cmake.in
	graco_config.cmake
	@ONLY
	)

configure_file (
	graco_config_install.cmake.in
	graco_config_install.cmake
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

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

install (
	FILES
		import_graco.cmake
		graco_step.cmake

	DESTINATION ${GRACO_INSTALL_CMAKE_SUBDIR}
	)

install (
	FILES ${CMAKE_CURRENT_BINARY_DIR}/graco_config_install.cmake
	RENAME graco_config.cmake
	DESTINATION ${GRACO_INSTALL_CMAKE_SUBDIR}
	)

install (
	FILES ${GRACO_ROOT_DIR}/version.cmake
	RENAME graco_version.cmake
	DESTINATION ${GRACO_INSTALL_CMAKE_SUBDIR}
	)

#...............................................................................
