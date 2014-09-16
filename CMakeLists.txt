# This file is part of Bulldozer project
# Tibbo Technology Inc (C) 2004-2013. All rights reserved
# Author: Vladimir Gladkov

#..............................................................................

project (bulldozer)

cmake_minimum_required (VERSION 2.8)

include (${CMAKE_CURRENT_LIST_DIR}/../cmake/Master.cmake)

print_std_settings ()

check_and_print_std_libs (
	RAGEL
	AXL
	)

configure_file (
	cmake/bulldozer.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/cmake/bulldozer.cmake
	@ONLY
	)

configure_file (
	cmake/bulldozer.cmake.in
	${CMAKE_CURRENT_LIST_DIR}/cmake/latest/bulldozer-${TARGET_CPU}.cmake
	@ONLY
	)

configure_file (
	cmake/bulldozer-latest.cmake.in
	${CMAKE_CURRENT_LIST_DIR}/cmake/latest/bulldozer.cmake
	@ONLY
	)

include (${CMAKE_CURRENT_BINARY_DIR}/cmake/bulldozer.cmake)
include (${AXL_CMAKE_DIR}/axl.cmake)
include (version.cmake)

add_subdirectory (src)

#..............................................................................
