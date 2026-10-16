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

#
# app folder
#

set(
	APP_CPP_LIST
	main.cpp
)

set(
	APP_IN_LIST
	Parser.cpp.in
)

source_group(
	app
	FILES
	${APP_CPP_LIST}
	${APP_IN_LIST}
)

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# llk folder (each test grammar registers its own test case)
#

file(
	GLOB
	LLK_LIST
	CONFIGURE_DEPENDS
	RELATIVE ${CMAKE_CURRENT_LIST_DIR}
	llk/*.llk
)

source_group(
	llk
	FILES
	${LLK_LIST}
)

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# gen folder
#

set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${GEN_DIR})

set(GEN_LLK_H_LIST)
set(GEN_LLK_CPP_LIST)
set(GEN_CPP_LIST)
set(TEST_NAME_LIST)

axl_push_and_set(CMAKE_CURRENT_BINARY_DIR ${GEN_DIR})

foreach(_LLK ${LLK_LIST})
	get_filename_component(_NAME ${_LLK} NAME_WE)

	add_graco_double_step(
		${_NAME}.llk.h
		${_NAME}.llk.cpp
		CppParser.h.in
		CppParser.cpp.in
		${_LLK}
	)

	# *.llk.cpp is included manually via a wrapper

	configure_file(
		Parser.cpp.in
		${GEN_DIR}/${_NAME}.cpp
		@ONLY
	)

	list(APPEND GEN_LLK_H_LIST ${GEN_DIR}/${_NAME}.llk.h)
	list(APPEND GEN_LLK_CPP_LIST ${GEN_DIR}/${_NAME}.llk.cpp)
	list(APPEND GEN_CPP_LIST ${GEN_DIR}/${_NAME}.cpp)
	list(APPEND TEST_NAME_LIST ${_NAME})
endforeach()

axl_pop(CMAKE_CURRENT_BINARY_DIR)

# *.llk.cpp is not a declared output of the graco double-step

set_source_files_properties(
	${GEN_LLK_CPP_LIST}
	PROPERTIES
	GENERATED TRUE
)

axl_exclude_from_build(${GEN_LLK_CPP_LIST}) # include "*.llk.cpp" manually

source_group(
	gen
	FILES
	${GEN_LLK_H_LIST}
	${GEN_LLK_CPP_LIST}
	${GEN_CPP_LIST}
)

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# pch folder
#

set(PCH_H   pch.h)

source_group(
	pch
	FILES
	${PCH_H}
	REGULAR_EXPRESSION cmake_pch
)

#...............................................................................
#
# graco_test_auto app
#

include_directories(
	${AXL_INC_DIR}
	${GRACO_INC_DIR}
	${GEN_DIR}
	${CMAKE_CURRENT_LIST_DIR}
)

link_directories(${AXL_LIB_DIR})

add_executable(
	graco_test_auto
	${PCH_H}
	${APP_CPP_LIST}
	${APP_IN_LIST}
	${LLK_LIST}
	${GEN_LLK_H_LIST}
	${GEN_LLK_CPP_LIST}
	${GEN_CPP_LIST}
)

add_dependencies(
	graco_test_auto
	graco
)

set_target_properties(
	graco_test_auto
	PROPERTIES
	FOLDER test
)

target_link_libraries(
	graco_test_auto
	axl_test
	axl_lex
	axl_io
	axl_core
)

if(UNIX AND NOT APPLE)
	target_link_libraries(
		graco_test_auto
		pthread
		dl
		rt
	)
endif()

target_precompile_headers(
	graco_test_auto
	PRIVATE
	${PCH_H}
)

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

foreach(_NAME ${TEST_NAME_LIST})
	add_test(
		NAME graco-${_NAME}
		COMMAND graco_test_auto ${_NAME}
	)
endforeach()

#...............................................................................
