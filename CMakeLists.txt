# This file is part of Bulldozer project
# Tibbo Technology Inc (C) 2004-2013. All rights reserved
# Author: Vladimir Gladkov

#..............................................................................

project (bulldozer)

cmake_minimum_required (VERSION 2.8)

include ("${CMAKE_CURRENT_LIST_DIR}/../cmake/master.cmake")

print_std_settings ()

check_and_print_libs (
	RAGEL
	AXL
	)

include (${AXL_CMAKE_DIR}/axl.cmake)

#..............................................................................

set (BULLDOZER_ROOT_DIR     ${CMAKE_CURRENT_LIST_DIR})
set (BULLDOZER_PACKAGE_DIR  ${CMAKE_CURRENT_BINARY_DIR}/package)

add_subdirectory (cmake)
	
include (${CMAKE_CURRENT_BINARY_DIR}/cmake/bulldozer.cmake)
include (version.cmake)

add_subdirectory (src)
add_subdirectory (package)

#..............................................................................
