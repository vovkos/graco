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

project (graco)

cmake_minimum_required (VERSION 3.3)

include (cmake/axl_cmake.cmake NO_POLICY_SCOPE)

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

set (GRACO_ROOT_DIR     ${CMAKE_CURRENT_LIST_DIR})
set (GRACO_BIN_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
set (GRACO_DOC_DIR      ${CMAKE_CURRENT_BINARY_DIR}/doc)
set (GRACO_PACKAGE_DIR  ${CMAKE_CURRENT_BINARY_DIR}/package)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${GRACO_BIN_BASE_DIR}/${CONFIGURATION_SUFFIX_0})

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

add_subdirectory (cmake)

include (${CMAKE_CURRENT_BINARY_DIR}/cmake/graco_config.cmake)

add_subdirectory (src)
add_subdirectory (doc)
add_subdirectory (samples)
add_subdirectory (test)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/package/CMakeLists.txt")
	add_subdirectory (package)
endif ()

#...............................................................................
