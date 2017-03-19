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

#include "Node.h"

//..............................................................................

class NodeMgr
{
	friend class Module;
	friend class Parser;
	friend class LaDfaBuilder;
	friend class ParseTableBuilder;

protected:
	sl::SimpleHashTable <int, SymbolNode*> m_tokenMap;
	sl::StringHashTable <SymbolNode*> m_symbolMap;

	GrammarNode m_epsilonNode;
	SymbolNode m_eofTokenNode;
	SymbolNode m_anyTokenNode;
	SymbolNode m_startPragmaSymbol;
	SymbolNode* m_primaryStartSymbol;

	sl::StdList <SymbolNode> m_charTokenList;
	sl::StdList <SymbolNode> m_namedTokenList;
	sl::StdList <SymbolNode> m_namedSymbolList;
	sl::StdList <SymbolNode> m_tempSymbolList;
	sl::StdList <SequenceNode> m_sequenceList;
	sl::StdList <BeaconNode> m_beaconList;
	sl::StdList <DispatcherNode> m_dispatcherList;
	sl::StdList <ActionNode> m_actionList;
	sl::StdList <ArgumentNode> m_argumentList;
	sl::StdList <ConflictNode> m_conflictList;
	sl::StdList <LaDfaNode> m_laDfaList;

	sl::Array <SymbolNode*> m_tokenArray;  // char tokens + named tokens
	sl::Array <SymbolNode*> m_symbolArray; // named symbols + temp symbols

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
	getSymbolNode (const sl::StringRef& name);

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
		const sl::StringRef& name,
		Node* const* node,
		size_t count
		);

	void
	luaExportNodeList (
		lua::LuaState* luaState,
		const sl::StringRef& name,
		sl::Iterator <Node> node,
		size_t countHint = 1
		);
};


//..............................................................................
