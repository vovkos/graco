# This file is part of Graco project
# Tibbo Technology Inc (C) 2004-2013. All rights reserved
# Author: Vladimir Gladkov

#..............................................................................

project (graco)

cmake_minimum_required (VERSION 2.8)

include (cmake/axl_cmake.cmake NO_POLICY_SCOPE)

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

set (GRACO_ROOT_DIR     ${CMAKE_CURRENT_LIST_DIR})
set (GRACO_BIN_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
set (GRACO_DOC_DIR      ${CMAKE_CURRENT_BINARY_DIR}/doc)
set (GRACO_PACKAGE_DIR  ${CMAKE_CURRENT_BINARY_DIR}/package)

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

add_subdirectory (cmake)

include (${CMAKE_CURRENT_BINARY_DIR}/cmake/graco_config.cmake)

add_subdirectory (src)
add_subdirectory (test)
add_subdirectory (package)

#..............................................................................
