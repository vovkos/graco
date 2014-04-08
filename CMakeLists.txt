# This file is part of Gepard project
# Tibbo Technology Inc (C) 2004-2013. All rights reserved
# Author: Vladimir Gladkov

#..............................................................................

project (gepard)

cmake_minimum_required (VERSION 2.8)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake")

include (CMakeUtils)

#..............................................................................
#
# Standard settings
#

include (CMakeStdSettings)

create_std_settings ()

message (STATUS "Target CPU:                 ${TARGET_CPU}")

if (HAS_BUILD_TYPE_SETTING)
	message (STATUS "Build type:                 ${CMAKE_BUILD_TYPE}")
endif ()

message (STATUS "C++ flags (Debug):         ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
message (STATUS "C++ flags (Release):       ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")

get_directory_property (_CXX_DEFINITIONS COMPILE_DEFINITIONS)

if (_CXX_DEFINITIONS)
	message (STATUS "C++ definitions:            ${_CXX_DEFINITIONS}")
endif ()

#..............................................................................
#
# Paths to libs & tools
#

ensure_file_exists (
	${CMAKE_MODULE_PATH}/CMakePaths.cmake
	${CMAKE_MODULE_PATH}/CMakePaths-template.cmake
	)

include (CMakePaths)

message (STATUS "Path to RAGEL executable:   ${PATH_RAGEL_EXE}")
message (STATUS "Path to AXL includes:       ${PATH_AXL_INC_DIR}")
message (STATUS "Path to AXL libraries:      ${PATH_AXL_LIB_DIR}")

#..............................................................................
#
# main
#

set (GEPARD_ROOT_DIR  ${CMAKE_CURRENT_LIST_DIR})
set (GEPARD_INC_DIR   ${GEPARD_ROOT_DIR}/include)
set (GEPARD_FRAME_DIR ${GEPARD_ROOT_DIR}/frame)

option (
	GEPARD_USE_PCH
	"Use precompiled headers when building Gepard project"
	ON
	)

set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

enable_testing ()
add_subdirectory (src)

#..............................................................................
