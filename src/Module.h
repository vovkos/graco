// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#include "NodeMgr.h"
#include "DefineMgr.h"
#include "ClassMgr.h"
#include "CmdLine.h"

//.............................................................................

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

//.............................................................................
