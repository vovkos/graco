# This file is part of AXL (R) Library
# Tibbo Technology Inc (C) 2004-2013. All rights reserved
# Author: Vladimir Gladkov

project_virtual_folder (llkc)

#..............................................................................
#
# app folder
#

set (
	APP_H_LIST
	llkc_ClassMgr.h
	llkc_Config.h
	llkc_DefineMgr.h
	llkc_Generator.h
	llkc_LaDfaBuilder.h
	llkc_Lexer.h
	llkc_Module.h
	llkc_Node.h
	llkc_NodeMgr.h
	llkc_Parser.h
	llkc_ParseTableBuilder.h
	llkc_ProductionBuilder.h
	)

set (
	APP_CPP_LIST
	llkc.cpp
	llkc_ClassMgr.cpp
	llkc_DefineMgr.cpp
	llkc_Generator.cpp
	llkc_LaDfaBuilder.cpp
	llkc_Lexer.rl.cpp
	llkc_Module.cpp
	llkc_Node.cpp
	llkc_NodeMgr.cpp
	llkc_Parser.cpp
	llkc_ParseTableBuilder.cpp
	llkc_ProductionBuilder.cpp
	)

set (
	APP_RL_LIST
	llkc_Lexer.rl
	)

push_and_set (CMAKE_CURRENT_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_ragel_step (
	llkc_Lexer.rl.cpp
	llkc_Lexer.rl
	)

pop (CMAKE_CURRENT_BINARY_DIR)

source_virtual_folder (
	app
	${APP_H_LIST}
	${APP_CPP_LIST}
	${APP_RL_LIST}
	)

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# frame folder
#

set (
	FRAME_LUAF_LIST
	${GEPARD_FRAME_DIR}/llk_ParserFrame.h.luaf
	)

source_virtual_folder (
	frame
	${FRAME_LUAF_LIST}
	)


# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# llk folder
#

set (
	LLK_H_LIST
	${GEPARD_INC_DIR}/llk_Ast.h
	${GEPARD_INC_DIR}/llk_Node.h
	${GEPARD_INC_DIR}/llk_Parser.h
	${GEPARD_INC_DIR}/llk_Pch.h
	)

set (LLK_CPP_LIST)

source_virtual_folder (
	llk
	${LLK_H_LIST}
	${LLK_CPP_LIST}
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
# gepard llk grammar compiler
#

include_directories (
	${PATH_LUA_INC_DIR}
	${PATH_AXL_INC_DIR}
	${PATH_GEPARD_INC_DIR}
	)

link_directories (
	${PATH_LUA_LIB_DIR}
	${PATH_AXL_LIB_DIR}
	)

add_executable (
	llkc
	${PCH_H}
	${PCH_CPP}
	${APP_H_LIST}
	${APP_CPP_LIST}
	${APP_RL_LIST}
	${FRAME_LUAF_LIST}
	${LLK_H_LIST}
	)

target_link_libraries (
	llkc
	axl_core
	axl_io
	axl_lua
	lua
	)

if (UNIX)
	target_link_libraries (
		llkc
		dl
		pthread
		)
endif ()

if (GEPARD_USE_PCH)
	set_pch (
		llkc
		${PCH_H}
		${PCH_CPP}
		)
endif ()

#..............................................................................
