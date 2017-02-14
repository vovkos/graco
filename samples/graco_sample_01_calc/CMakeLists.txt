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

set (
	APP_H_LIST
	Lexer.h
	Value.h
	)

set (
	APP_CPP_LIST
	main.cpp
	Lexer.cpp
	Parser.cpp
	Value.cpp
	)

set (
	APP_RL_LIST
	Lexer.rl
	)

set (
	APP_LLK_LIST
	Parser.llk
	)

source_group (
	app
	FILES
	${APP_H_LIST}
	${APP_CPP_LIST}
	${APP_RL_LIST}
	${APP_LLK_LIST}
	)

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# gen folder
#

set (GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)
file (MAKE_DIRECTORY ${GEN_DIR})

axl_push_and_set (CMAKE_CURRENT_BINARY_DIR ${GEN_DIR})

add_ragel_step (
	Lexer.rl.cpp
	Lexer.rl
	)

add_graco_double_step (
	Parser.llk.h
	Parser.llk.cpp
	CppParser.h.in
	CppParser.cpp.in
	Parser.llk
	)

axl_pop (CMAKE_CURRENT_BINARY_DIR)

set (
	GEN_LLK_H_LIST
	${GEN_DIR}/Parser.llk.h
	)

set (
	GEN_LLK_CPP_LIST
	${GEN_DIR}/Parser.llk.cpp
	)

axl_exclude_from_build (${GEN_RL_CPP_LIST})  # include "*.rl.cpp" manually
axl_exclude_from_build (${GEN_LLK_CPP_LIST}) # include "*.llk.cpp" manually

source_group (
	gen
	FILES
	${GEN_RL_CPP_LIST}
	${GEN_LLK_H_LIST}
	${GEN_LLK_CPP_LIST}
	)

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# pch folder
#

set (PCH_H   pch.h)
set (PCH_CPP pch.cpp)

source_group (
	pch
	FILES
	${PCH_H}
	${PCH_CPP}
	)

#...............................................................................
#
# graco_sample_01_calc app
#

include_directories (
	${AXL_INC_DIR}
	${GRACO_INC_DIR}
	${GEN_DIR}
	${CMAKE_CURRENT_LIST_DIR}
	)

link_directories (${AXL_LIB_DIR})

add_executable (
	graco_sample_01_calc
	${PCH_H}
	${PCH_CPP}
	${APP_H_LIST}
	${APP_CPP_LIST}
	${APP_RL_LIST}
	${APP_LLK_LIST}
	${GEN_RL_CPP_LIST}
	${GEN_LLK_H_LIST}
	# ${GEN_LLK_CPP_LIST} # currently, cmake can't handle double-file output
	)

add_dependencies (
	graco_sample_01_calc
	graco
	)

set_target_properties (
	graco_sample_01_calc
	PROPERTIES
	FOLDER samples
	)

target_link_libraries (
	graco_sample_01_calc
	axl_fsm
	axl_io
	axl_core
	)

if (UNIX)
	target_link_libraries (
		graco_sample_01_calc
		pthread
		)
endif ()

axl_set_pch (
	graco_sample_01_calc
	${PCH_H}
	${PCH_CPP}
	)

#...............................................................................
