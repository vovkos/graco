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

//..............................................................................

class ParseTableBuilder {
protected:
	NodeMgr* m_nodeMgr;
	sl::Array<Node*>* m_parseTable;

public:
	ParseTableBuilder(
		NodeMgr* nodeMgr,
		sl::Array<Node*>* parseTable
	) {
		m_nodeMgr = nodeMgr;
		m_parseTable = parseTable;
	}

	bool
	build();

protected:
	void
	calcGrammarProps();

	void
	addProductionToParseTable(
		SymbolNode* symbol,
		GrammarNode* production
	);

	void
	addAnyTokenProductionToParseTable(
		SymbolNode* symbol,
		GrammarNode* production
	);

	size_t
	addParseTableEntry(
		SymbolNode* symbol,
		SymbolNode* token,
		GrammarNode* production
	); // returns number of conflicting productions
};

//..............................................................................
