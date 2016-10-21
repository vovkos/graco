//..............................................................................
//
//  This file is part of the Graco toolkit.
//
//  Graco is distributed under the MIT license.
//  For details see accompanying license.txt file,
//  the public copy of which is also available at:
//  http://tibbo.com/downloads/archive/graco/license.txt
//
//..............................................................................

#pragma once

#include "NodeMgr.h"
#include "DefineMgr.h"
#include "ClassMgr.h"
#include "CmdLine.h"

//..............................................................................

class Module
{
	friend class Parser;

protected:
	sl::Array <Node*> m_parseTable;
	size_t m_lookaheadLimit;
	size_t m_lookahead;
	ClassMgr m_classMgr;
	DefineMgr m_defineMgr;
	NodeMgr m_nodeMgr;

public:
	sl::BoxList <sl::String> m_importList;

public:
	Module ();

	void
	clear ();

	bool
	build (CmdLine* cmdLine);

	void
	luaExport (lua::LuaState* luaState);

	void
	trace ();

	sl::String
	generateBnfString ();

	bool
	writeBnfFile (const sl::StringRef& fileName);

protected:
	void
	luaExportDefines (lua::LuaState* luaState);

	void
	luaExportClassTable (lua::LuaState* luaState);

	void
	luaExportParseTable (lua::LuaState* luaState);
};

//..............................................................................
