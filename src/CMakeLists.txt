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
	ClassMgr.h
	CmdLine.h
	DefineMgr.h
	Generator.h
	LaDfaBuilder.h
	Lexer.h
	Module.h
	Node.h
	NodeMgr.h
	Parser.h
	ParseTableBuilder.h
	ProductionBuilder.h
	version.h.in
	)

set (
	APP_CPP_LIST
	main.cpp
	CmdLine.cpp
	ClassMgr.cpp
	DefineMgr.cpp
	Generator.cpp
	LaDfaBuilder.cpp
	Lexer.cpp
	Module.cpp
	Node.cpp
	NodeMgr.cpp
	Parser.cpp
	ParseTableBuilder.cpp
	ProductionBuilder.cpp
	)

set (
	APP_RL_LIST
	Lexer.rl
	)

source_group (
	app
	FILES
	${APP_H_LIST}
	${APP_CPP_LIST}
	${APP_RL_LIST}
	)

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# llk folder
#

set (
	LLK_H_LIST
	${GRACO_INC_DIR}/llk_Ast.h
	${GRACO_INC_DIR}/llk_Node.h
	${GRACO_INC_DIR}/llk_Parser.h
	${GRACO_INC_DIR}/llk_Pch.h
	)

source_group (
	llk
	FILES
	${LLK_H_LIST}
	)

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# frame folder
#

file (
	GLOB FRAME_LIST
	${GRACO_ROOT_DIR}/frame/*.*
	)

source_group (
	frame
	FILES
	${FRAME_LIST}
	)

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# res folder
#

set (
	RES_RC_LIST
	res.rc
	)

source_group (
	res
	FILES
	${RES_RC_LIST}
	)

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# gen folder
#

set (GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)
file (MAKE_DIRECTORY ${GEN_DIR})

axl_push_and_set (CMAKE_CURRENT_BINARY_DIR ${GEN_DIR})

configure_file (
	version.h.in
	${GEN_DIR}/version.h
	)

set (
	GEN_H_LIST
	${GEN_DIR}/version.h
	)

add_ragel_step (
	Lexer.rl.cpp
	Lexer.rl
	)

set (
	GEN_RL_CPP_LIST
	${GEN_DIR}/Lexer.rl.cpp
	)

axl_pop (CMAKE_CURRENT_BINARY_DIR)

axl_exclude_from_build (${GEN_RL_CPP_LIST})  # include "*.rl.cpp" manually

source_group (
	gen
	FILES
	${GEN_RL_CPP_LIST}
	${GEN_H_LIST}
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
# graco LL(k) grammar compiler
#

include_directories (
	${LUA_INC_DIR}
	${AXL_INC_DIR}
	${GEN_DIR}
	)

link_directories (
	${LUA_LIB_DIR}
	${AXL_LIB_DIR}
	)

add_executable (
	graco
	${PCH_H}
	${PCH_CPP}
	${APP_H_LIST}
	${APP_CPP_LIST}
	${APP_RL_LIST}
	${GEN_H_LIST}
	${GEN_RL_CPP_LIST}
	${LLK_H_LIST}
	${FRAME_LIST}
	${RES_RC_LIST}
	)

axl_set_pch (
	graco
	${PCH_H}
	${PCH_CPP}
	)

target_link_libraries (
	graco
	axl_st
	axl_lua
	axl_lex
	axl_io
	axl_core
	${LUA_LIB_NAME}
	)

if (UNIX)
	target_link_libraries (
		graco
		dl
		pthread
		)

	if (NOT APPLE)
		target_link_libraries (
			graco
			rt
			)
	endif ()
endif ()

if (WIN32 AND LUA_DLL_DIR)
	add_custom_command (
		TARGET graco
		POST_BUILD
		COMMAND
			echo Copying DLL files for graco...
		COMMAND
			${CMAKE_COMMAND} -E copy
			${LUA_DLL_DIR}/${LUA_LIB_NAME}.dll
			${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CONFIGURATION_SUFFIX}
		)
endif ()

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

install (
	TARGETS graco
	DESTINATION ${GRACO_INSTALL_BIN_SUBDIR}
	)

#...............................................................................
