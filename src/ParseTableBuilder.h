// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#include "NodeMgr.h"

//..............................................................................

class ParseTableBuilder
{
protected:
	NodeMgr* m_nodeMgr;
	sl::Array <Node*>* m_parseTable;

public:
	ParseTableBuilder (
		NodeMgr* nodeMgr,
		sl::Array <Node*>* parseTable
		)
	{
		m_nodeMgr = nodeMgr;
		m_parseTable = parseTable;
	}

	bool
	build ();

protected:
	void
	calcFirstFollow ();

	void
	buildFirstFollowArrays (GrammarNode* node);

	void
	addProductionToParseTable (
		SymbolNode* symbol,
		GrammarNode* production
		);

	void
	addAnyTokenProductionToParseTable (
		SymbolNode* symbol,
		GrammarNode* production
		);

	size_t
	addParseTableEntry (
		SymbolNode* symbol,
		SymbolNode* token,
		GrammarNode* production
		); // returns number of conflicting productions
};

//..............................................................................
