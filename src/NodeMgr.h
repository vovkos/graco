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

class NodeMgr {
	friend class Module;
	friend class Parser;
	friend class LaDfaBuilder;
	friend class ParseTableBuilder;

protected:
	sl::SimpleHashTable<int, SymbolNode*> m_tokenMap;
	sl::StringHashTable<SymbolNode*> m_symbolMap;

	GrammarNode m_epsilonNode;
	SymbolNode m_eofTokenNode;
	SymbolNode m_anyTokenNode;
	SymbolNode m_pragmaStartSymbol;
	SymbolNode* m_primaryStartSymbol;

	sl::List<SymbolNode> m_charTokenList;
	sl::List<SymbolNode> m_namedTokenList;
	sl::List<SymbolNode> m_namedSymbolList;
	sl::List<SymbolNode> m_catchSymbolList;
	sl::List<SymbolNode> m_tempSymbolList;
	sl::List<SequenceNode> m_sequenceList;
	sl::List<BeaconNode> m_beaconList;
	sl::List<DispatcherNode> m_dispatcherList;
	sl::List<ActionNode> m_actionList;
	sl::List<ArgumentNode> m_argumentList;
	sl::List<ConflictNode> m_conflictList;
	sl::List<LaDfaNode> m_laDfaList;
	sl::List<GrammarNode> m_weaklyReachableNodeList;

	sl::Array<SymbolNode*> m_tokenArray;   // char tokens + named tokens
	sl::Array<SymbolNode*> m_symbolArray;  // named symbols + temp symbols
	sl::Array<SymbolNode*> m_enterArray;   // named symbols with enter actions
	sl::Array<SymbolNode*> m_leaveArray;   // named symbols with leave actions

	size_t m_lookaheadLimit;
	size_t m_masterCount;

public:
	NodeMgr();

	bool
	isEmpty() {
		return m_namedSymbolList.isEmpty();
	}

	void
	clear();

	void
	luaExport(lua::LuaState* luaState);

	SymbolNode*
	getTokenNode(int token);

	SymbolNode*
	getSymbolNode(const sl::StringRef& name);

	SymbolNode*
	createCatchSymbolNode();

	SymbolNode*
	createTempSymbolNode();

	SequenceNode*
	createSequenceNode();

	SequenceNode*
	createSequenceNode(GrammarNode* node);

	BeaconNode*
	createBeaconNode(SymbolNode* target);

	void
	deleteBeaconNode(BeaconNode* node);

	void
	deleteLaDfaNode(LaDfaNode* node) {
		m_laDfaList.erase(node);
	}

	DispatcherNode*
	createDispatcherNode(SymbolNode* symbol);

	ActionNode*
	createActionNode();

	ArgumentNode*
	createArgumentNode();

	ConflictNode*
	createConflictNode();

	LaDfaNode*
	createLaDfaNode();

	GrammarNode*
	createQuantifierNode(
		GrammarNode* node,
		int kind
	);

	void
	trace();

	void
	markReachableNodes();

	void
	deleteUnreachableNodes();

	void
	indexTokens();

	void
	indexSymbols();

	void
	indexSequences();

	void
	indexBeacons();

	void
	indexDispatchers();

	void
	indexActions();

	void
	indexArguments();

	void
	indexLaDfaNodes();

protected:
	template <typename T>
	void
	deleteUnreachableNodes(sl::List<T>* list);

	void
	luaExportLaDfaTable(lua::LuaState* luaState);

	void
	luaExportNodeArray(
		lua::LuaState* luaState,
		const sl::StringRef& name,
		Node* const* node,
		size_t count
	);

	void
	luaExportSymbolNodeRefArray(
		lua::LuaState* luaState,
		const sl::StringRef& name,
		SymbolNode* const* node,
		size_t count
	);

	void
	luaExportNodeList(
		lua::LuaState* luaState,
		const sl::StringRef& name,
		sl::Iterator<Node> node,
		size_t countHint = 1
	);
};


//..............................................................................
