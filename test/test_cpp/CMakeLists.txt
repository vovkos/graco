# This file is part of AXL (R) Library
# Tibbo Technology Inc (C) 2004-2015. All rights reserved
# Author: Vladimir Gladkov

#..............................................................................
#
# app folder
#

set (
	APP_H_LIST
	Lexer.h
	)

set (
	APP_CPP_LIST
	main.cpp
	Lexer.cpp
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

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
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

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
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
	
#..............................................................................
#
# test_cpp app
#

include_directories (
	${AXL_INC_DIR}
	${GRACO_INC_DIR}
	${GEN_DIR}
	${CMAKE_CURRENT_LIST_DIR}
	)

link_directories (${AXL_LIB_DIR})

add_executable (
	test_cpp
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
	test_cpp 
	graco
	)	

set_target_properties (
	test_cpp
	PROPERTIES
	FOLDER test
	)
	
target_link_libraries (
	test_cpp
	axl_fsm
	axl_io
	axl_core
	)

if (UNIX)
	target_link_libraries (
		test_cpp
		pthread
		)
endif ()

axl_set_pch (
	test_cpp
	${PCH_H}
	${PCH_CPP}
	)

#..............................................................................
