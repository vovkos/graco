# This file is part of AXL (R) Library
# Tibbo Technology Inc (C) 2004-2013. All rights reserved
# Author: Vladimir Gladkov

#..............................................................................

set (GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)

#..............................................................................
#
# app folder
#

set (
	APP_H_LIST
	ClassMgr.h
	Config.h
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
	)

set (
	APP_CPP_LIST
	main.cpp
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

source_virtual_folder (
	app
	${APP_H_LIST}
	${APP_CPP_LIST}
	${APP_RL_LIST}
	)

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# gen folder
#

push_and_set (CMAKE_CURRENT_BINARY_DIR ${GEN_DIR})
file (MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_ragel_step (
	Lexer.rl.cpp
	Lexer.rl
	)

set (
	GEN_RL_CPP_LIST
	${GEN_DIR}/Lexer.rl.cpp
	)

pop (CMAKE_CURRENT_BINARY_DIR)

exclude_from_build (${GEN_RL_CPP_LIST})  # include "*.rl.cpp" manually

source_virtual_folder (
	gen
	${GEN_RL_CPP_LIST}
	)

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# llk folder
#

set (
	LLK_H_LIST
	${INC_DIR}/llk_Ast.h
	${INC_DIR}/llk_Node.h
	${INC_DIR}/llk_Parser.h
	${INC_DIR}/llk_Pch.h
	)

source_virtual_folder (
	llk
	${LLK_H_LIST}
	)

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# frame folder
#

set (
	FRAME_LIST
	${FRAME_DIR}/llk_Parser.h.frame
	)

source_virtual_folder (
	frame
	${FRAME_LIST}
	)

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# pch folder
#

set (PCH_H   pch.h)
set (PCH_CPP pch.cpp)

source_virtual_folder (
	pch
	${PCH_H}
	${PCH_CPP}
	)

#..............................................................................
#
# bulldozer llk grammar compiler
#

include_directories (
	${PATH_LUA_INC_DIR}
	${PATH_AXL_INC_DIR}
	${INC_DIR}
	${GEN_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	)

link_directories (
	${PATH_LUA_LIB_DIR}
	${PATH_AXL_LIB_DIR}
	)

add_executable (
	bulldozer
	${PCH_H}
	${PCH_CPP}
	${APP_H_LIST}
	${APP_CPP_LIST}
	${APP_RL_LIST}
	${GEN_RL_CPP_LIST}
	${LLK_H_LIST}
	${FRAME_LIST}
	)

set_pch (
	bulldozer
	${PCH_H}
	${PCH_CPP}
	)

target_link_libraries (
	bulldozer
	axl_core
	axl_io
	axl_lua
	lua
	)

if (UNIX)
	target_link_libraries (
		bulldozer
		dl
		rt
		pthread
		)
endif ()

#..............................................................................
