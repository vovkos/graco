// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#include "Node.h"

//.............................................................................

class NodeMgr
{
	friend class Module;
	friend class Parser;
	friend class LaDfaBuilder;
	friend class ParseTableBuilder;

protected:
	rtl::HashTableMap <int, SymbolNode*, rtl::HashId <int>, rtl::Cmp <int> > m_tokenMap;
	rtl::StringHashTableMap <SymbolNode*> m_symbolMap;

	GrammarNode m_epsilonNode;
	SymbolNode m_eofTokenNode;
	SymbolNode m_anyTokenNode;
	SymbolNode m_startPragmaSymbol;
	SymbolNode* m_primaryStartSymbol;

	rtl::StdList <SymbolNode> m_charTokenList;
	rtl::StdList <SymbolNode> m_namedTokenList;
	rtl::StdList <SymbolNode> m_namedSymbolList;
	rtl::StdList <SymbolNode> m_tempSymbolList;
	rtl::StdList <SequenceNode> m_sequenceList;
	rtl::StdList <BeaconNode> m_beaconList;
	rtl::StdList <DispatcherNode> m_dispatcherList;
	rtl::StdList <ActionNode> m_actionList;
	rtl::StdList <ArgumentNode> m_argumentList;
	rtl::StdList <ConflictNode> m_conflictList;
	rtl::StdList <LaDfaNode> m_laDfaList;

	rtl::Array <SymbolNode*> m_tokenArray;  // char tokens + named tokens
	rtl::Array <SymbolNode*> m_symbolArray; // named symbols + temp symbols

	size_t m_masterCount;

public:
	NodeMgr ();

	bool
	isEmpty ()
	{
		return m_namedSymbolList.isEmpty ();
	}

	void
	clear ();

	void
	luaExport (lua::LuaState* luaState);

	SymbolNode*
	getTokenNode (int token);

	SymbolNode*
	getSymbolNode (const rtl::String& name);

	SymbolNode*
	createTempSymbolNode ();

	SequenceNode*
	createSequenceNode ();

	SequenceNode*
	createSequenceNode (GrammarNode* node);

	BeaconNode*
	createBeaconNode (SymbolNode* target);

	void
	deleteBeaconNode (BeaconNode* node)
	{
		m_beaconList.erase (node);
	}

	void
	deleteLaDfaNode (LaDfaNode* node)
	{
		m_laDfaList.erase (node);
	}

	DispatcherNode*
	createDispatcherNode (SymbolNode* symbol);

	ActionNode*
	createActionNode ();

	ArgumentNode*
	createArgumentNode ();

	ConflictNode*
	createConflictNode ();

	LaDfaNode*
	createLaDfaNode ();

	GrammarNode*
	createQuantifierNode (
		GrammarNode* node,
		int kind
		);

	void
	trace ();

	void
	luaExport ();

	void
	markReachableNodes ();

	void
	deleteUnreachableNodes ();

	void 
	indexTokens ();

	void 
	indexSymbols ();

	void 
	indexSequences ();

	void 
	indexBeacons ();

	void 
	indexDispatchers ();

	void 
	indexActions ();

	void 
	indexArguments ();

	void 
	indexLaDfaNodes ();

protected:
	void
	luaExportTokenTable (lua::LuaState* luaState);

	void
	luaExportSymbolTable (lua::LuaState* luaState);

	void
	luaExportSequenceTable (lua::LuaState* luaState);

	void
	luaExportLaDfaTable (lua::LuaState* luaState);

	void
	luaExportNodeArray (
		lua::LuaState* luaState,
		const char* name,
		Node* const* node,
		size_t count
		);

	void
	luaExportNodeList (
		lua::LuaState* luaState,
		const char* name,
		rtl::Iterator <Node> node,
		size_t countHint = 1
		);
};


//.............................................................................
