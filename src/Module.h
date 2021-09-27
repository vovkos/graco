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
#include "CmdLine.h"

//..............................................................................

enum BnfDialect {
	BnfDialect_Classic,
	BnfDialect_Graco,
};

//..............................................................................

class Module {
	friend class Parser;

protected:
	sl::BoxList<sl::String> m_sourceCache;
	sl::Array<Node*> m_parseTable;
	size_t m_maxUsedLookahead;
	DefineMgr m_defineMgr;
	NodeMgr m_nodeMgr;

public:
	sl::BoxList<sl::String> m_importList;

public:
	Module();

	void
	clear();

	const sl::String&
	cacheSource(const sl::StringRef& source) {
		return *m_sourceCache.insertTail(source);
	}

	size_t
	getMaxUsedLookahead() {
		return m_maxUsedLookahead;
	}

	bool
	build(const CmdLine* cmdLine);

	void
	luaExport(lua::LuaState* luaState);

	void
	trace();

	sl::String
	generateBnfString(BnfDialect dialect = BnfDialect_Classic);

	bool
	writeBnfFile(
		const sl::StringRef& fileName,
		BnfDialect dialect = BnfDialect_Classic
	);

protected:
	void
	luaExportParseTable(lua::LuaState* luaState);
};

//..............................................................................
