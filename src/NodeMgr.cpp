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

#include "pch.h"
#include "NodeMgr.h"

//..............................................................................

NodeMgr::NodeMgr()
{
	// we use the same master index for epsilon production and eof token:
	// token with index 0 is eof token
	// parse table entry equal 0 is epsilon production

	m_eofTokenNode.m_nodeKind = NodeKind_Token;
	m_anyTokenNode.m_flags = SymbolNodeFlag_EofToken;
	m_eofTokenNode.m_name = "$";
	m_eofTokenNode.m_index = 0;
	m_eofTokenNode.m_masterIndex = 0;

	m_anyTokenNode.m_nodeKind = NodeKind_Token;
	m_anyTokenNode.m_flags = SymbolNodeFlag_AnyToken;
	m_anyTokenNode.m_name = "any";
	m_anyTokenNode.m_index = 1;
	m_anyTokenNode.m_masterIndex = 1;

	m_epsilonNode.m_nodeKind = NodeKind_Epsilon;
	m_epsilonNode.m_flags |= GrammarNodeFlag_Nullable;
	m_epsilonNode.m_name = "epsilon";
	m_epsilonNode.m_masterIndex = 0;

	m_pragmaStartSymbol.m_flags |= SymbolNodeFlag_Pragma;
	m_pragmaStartSymbol.m_name = "pragma";
	m_primaryStartSymbol = NULL;

	m_lookaheadLimit = 1;
	m_masterCount = 0;
}

void
NodeMgr::clear()
{
	m_tokenMap.clear();
	m_symbolMap.clear();
	m_anyTokenNode.m_firstArray.clear();
	m_anyTokenNode.m_firstSet.clear();

	m_pragmaStartSymbol.m_index = -1;
	m_pragmaStartSymbol.m_masterIndex = -1;
	m_pragmaStartSymbol.m_productionArray.clear();
	m_primaryStartSymbol = NULL;

	m_charTokenList.clear();
	m_namedTokenList.clear();
	m_namedSymbolList.clear();
	m_catchSymbolList.clear();
	m_tempSymbolList.clear();
	m_sequenceList.clear();
	m_beaconList.clear();
	m_dispatcherList.clear();
	m_actionList.clear();
	m_argumentList.clear();
	m_conflictList.clear();
	m_laDfaList.clear();
	m_weaklyReachableNodeList.clear();

	m_tokenArray.clear();
	m_symbolArray.clear();

	m_lookaheadLimit = 1;
	m_masterCount = 0;
}

void
NodeMgr::trace()
{
	traceNodeArray("TOKENS", &m_tokenArray);
	traceNodeArray("SYMBOLS", &m_symbolArray);
	traceNodeList("SEQUENCES", m_sequenceList.getHead ());
	traceNodeList("BEACONS", m_beaconList.getHead ());
	traceNodeList("DISPATCHERS", m_dispatcherList.getHead ());
	traceNodeList("ACTIONS", m_actionList.getHead ());
	traceNodeList("ARGUMENTS", m_argumentList.getHead ());
	traceNodeList("CONFLICTS", m_conflictList.getHead ());
	traceNodeList("LOOKAHEAD DFA", m_laDfaList.getHead ());
}

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

SymbolNode*
NodeMgr::getTokenNode(int token)
{
	sl::HashTableIterator<int, SymbolNode*> mapIt = m_tokenMap.visit(token);
	if (mapIt->m_value)
		return mapIt->m_value;

	SymbolNode* node = AXL_MEM_NEW(SymbolNode);
	node->m_nodeKind = NodeKind_Token;
	node->m_charToken = token;

	if (isprint(token))
		node->m_name.format("\'%c\'", (char) token);
	else
		node->m_name.format("\\0%d", token);

	m_charTokenList.insertTail(node);
	mapIt->m_value = node;

	return node;
}

SymbolNode*
NodeMgr::getSymbolNode(const sl::StringRef& name)
{
	sl::StringHashTableIterator<SymbolNode*> mapIt = m_symbolMap.visit(name);
	if (mapIt->m_value)
		return mapIt->m_value;

	SymbolNode* node = AXL_MEM_NEW(SymbolNode);
	node->m_flags = SymbolNodeFlag_User;
	node->m_name = name;

	m_namedSymbolList.insertTail(node);
	mapIt->m_value = node;

	return node;
}

SymbolNode*
NodeMgr::createCatchSymbolNode()
{
	SymbolNode* node = AXL_MEM_NEW(SymbolNode);
	node->m_name.format("_cat%d", m_catchSymbolList.getCount() + 1);
	node->m_lookaheadLimit = m_lookaheadLimit;
	m_catchSymbolList.insertTail(node);
	return node;
}

SymbolNode*
NodeMgr::createTempSymbolNode()
{
	SymbolNode* node = AXL_MEM_NEW(SymbolNode);
	node->m_name.format("_tmp%d", m_tempSymbolList.getCount() + 1);
	node->m_lookaheadLimit = m_lookaheadLimit;
	m_tempSymbolList.insertTail(node);
	return node;
}

SequenceNode*
NodeMgr::createSequenceNode()
{
	SequenceNode* node = AXL_MEM_NEW(SequenceNode);
	node->m_name.format("_seq%d", m_sequenceList.getCount() + 1);
	m_sequenceList.insertTail(node);
	return node;
}

SequenceNode*
NodeMgr::createSequenceNode(GrammarNode* node)
{
	SequenceNode* sequenceNode = createSequenceNode();
	sequenceNode->append(node);
	return sequenceNode;
}

BeaconNode*
NodeMgr::createBeaconNode(SymbolNode* target)
{
	BeaconNode* beaconNode = AXL_MEM_NEW(BeaconNode);
	beaconNode->m_target = target;

	if (target->m_nodeKind == NodeKind_Symbol)
		beaconNode->m_label = target->m_name;

	beaconNode->m_name.format(
		"_bcn%d(%s)",
		m_beaconList.getCount() + 1,
		target->m_name.sz()
		);

	m_beaconList.insertTail(beaconNode);
	return beaconNode;
}

void
NodeMgr::deleteBeaconNode(BeaconNode* node)
{
	if (node->m_flags & NodeFlag_Reachable)
	{
		m_beaconList.erase(node);
	}
	else
	{
		ASSERT(node->m_flags & GrammarNodeFlag_WeaklyReachable); // otherwise should have been deleted
		m_weaklyReachableNodeList.erase(node);
	}
}


DispatcherNode*
NodeMgr::createDispatcherNode(SymbolNode* symbol)
{
	DispatcherNode* dispatcherNode = AXL_MEM_NEW(DispatcherNode);
	dispatcherNode->m_name.format("_dsp%d", m_dispatcherList.getCount() + 1);
	dispatcherNode->m_symbol = symbol;
	m_dispatcherList.insertTail(dispatcherNode);
	return dispatcherNode;
}

ActionNode*
NodeMgr::createActionNode()
{
	ActionNode* node = AXL_MEM_NEW(ActionNode);
	node->m_name.format("_act%d", m_actionList.getCount() + 1);
	m_actionList.insertTail(node);
	return node;
}

ArgumentNode*
NodeMgr::createArgumentNode()
{
	ArgumentNode* node = AXL_MEM_NEW(ArgumentNode);
	node->m_name.format("_arg%d", m_argumentList.getCount() + 1);
	m_argumentList.insertTail(node);
	return node;
}

ConflictNode*
NodeMgr::createConflictNode()
{
	ConflictNode* node = AXL_MEM_NEW(ConflictNode);
	node->m_name.format("_cnf%d", m_conflictList.getCount() + 1);
	m_conflictList.insertTail(node);
	return node;
}

LaDfaNode*
NodeMgr::createLaDfaNode()
{
	LaDfaNode* node = AXL_MEM_NEW(LaDfaNode);
	node->m_name.format("_dfa%d", m_laDfaList.getCount() + 1);
	m_laDfaList.insertTail(node);
	return node;
}

GrammarNode*
NodeMgr::createQuantifierNode(
	GrammarNode* node,
	int kind
	)
{
	SequenceNode* tempSeq;
	SymbolNode* tempAlt;

	if (node->m_nodeKind == NodeKind_Action || node->m_nodeKind == NodeKind_Epsilon)
	{
		err::setFormatStringError("can't apply quantifier to action or epsilon nodes");
		return NULL;
	}

	GrammarNode* resultNode;

	switch (kind)
	{
	case '?':
		tempAlt = createTempSymbolNode();
		tempAlt->addProduction(node);
		tempAlt->addProduction(&m_epsilonNode);
		resultNode = tempAlt;
		break;

	case '*':
		tempAlt = createTempSymbolNode();
		tempSeq = createSequenceNode();
		tempSeq->append(node);
		tempSeq->append(tempAlt);

		tempAlt->addProduction(tempSeq);
		tempAlt->addProduction(&m_epsilonNode);
		resultNode = tempAlt;
		break;

	case '+':
		tempSeq = createSequenceNode();
		tempSeq->append(node);
		tempSeq->append(createQuantifierNode(node, '*'));
		resultNode = tempSeq;
		break;

	default:
		ASSERT(false);
		return NULL;
	}

	resultNode->m_quantifierKind = kind;
	resultNode->m_quantifiedNode = node;
	return resultNode;
}

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

void
NodeMgr::markReachableNodes()
{
	sl::Iterator<SymbolNode> nodeIt = m_namedSymbolList.getHead();

	if (m_primaryStartSymbol)
		for (; nodeIt; nodeIt++)
		{
			SymbolNode* node = *nodeIt;
			if (node->m_flags & SymbolNodeFlag_Start)
				node->markReachable();
		}
	else
		for (; nodeIt; nodeIt++)
		{
			SymbolNode* node = *nodeIt;
			node->markReachable();
		}

	m_pragmaStartSymbol.markReachable();
}

template <typename T>
void
NodeMgr::deleteUnreachableNodes(sl::List<T>* list)
{
	sl::Iterator<T> nodeIt = list->getHead();
	while (nodeIt)
	{
		T* node = *nodeIt++;
		if (!(node->m_flags & NodeFlag_Reachable))
			if (node->m_flags & GrammarNodeFlag_WeaklyReachable)
			{
				list->remove(node);
				m_weaklyReachableNodeList.insertTail(node);
			}
			else
			{
				list->erase(node);
			}
	}
}

void
NodeMgr::deleteUnreachableNodes()
{
	deleteUnreachableNodes(&m_charTokenList);
	deleteUnreachableNodes(&m_namedSymbolList);
	deleteUnreachableNodes(&m_tempSymbolList);
	deleteUnreachableNodes(&m_sequenceList);
	deleteUnreachableNodes(&m_beaconList);
	deleteUnreachableNodes(&m_actionList);
	deleteUnreachableNodes(&m_argumentList);
}

void
NodeMgr::indexTokens()
{
	size_t i = 0;

	size_t count = m_charTokenList.getCount() + 2;
	m_tokenArray.setCount(count);

	m_tokenArray[i++] = &m_eofTokenNode;
	m_tokenArray[i++] = &m_anyTokenNode;

	sl::Iterator<SymbolNode> nodeIt = m_charTokenList.getHead();
	for (; nodeIt; nodeIt++, i++)
	{
		SymbolNode* node = *nodeIt;
		node->m_index = i;
		node->m_masterIndex = i;
		m_tokenArray[i] = node;
	}

	m_masterCount = i;
}

void
NodeMgr::indexSymbols()
{
	size_t i = 0;
	size_t j = m_masterCount;

	sl::Iterator<SymbolNode> nodeIt = m_namedSymbolList.getHead();
	while (nodeIt)
	{
		SymbolNode* node = *nodeIt++;
		if (!node->m_productionArray.isEmpty())
			continue;

		node->m_nodeKind = NodeKind_Token;
		node->m_index = j;
		node->m_masterIndex = j;

		j++;

		m_namedSymbolList.remove(node);
		m_namedTokenList.insertTail(node);
		m_tokenArray.append(node);
	}

	size_t count =
		m_namedSymbolList.getCount() +
		m_catchSymbolList.getCount() +
		m_tempSymbolList.getCount();

	m_symbolArray.setCount(count);

	nodeIt = m_namedSymbolList.getHead();
	for (; nodeIt; nodeIt++, i++, j++)
	{
		SymbolNode* node = *nodeIt;
		node->m_index = i;
		node->m_masterIndex = j;
		m_symbolArray[i] = node;
	}

	nodeIt = m_catchSymbolList.getHead();
	for (; nodeIt; nodeIt++, i++, j++)
	{
		SymbolNode* node = *nodeIt;
		node->m_index = i;
		node->m_masterIndex = j;
		m_symbolArray[i] = node;
	}

	nodeIt = m_tempSymbolList.getHead();
	for (; nodeIt; nodeIt++, i++, j++)
	{
		SymbolNode* node = *nodeIt;
		node->m_index = i;
		node->m_masterIndex = j;
		m_symbolArray[i] = node;
	}

	if (!m_pragmaStartSymbol.m_productionArray.isEmpty())
	{
		m_pragmaStartSymbol.m_index = i;
		m_pragmaStartSymbol.m_masterIndex = j;
		m_symbolArray.append(&m_pragmaStartSymbol);

		i++;
		j++;
	}

	m_masterCount = j;
}

void
NodeMgr::indexSequences()
{
	size_t i = 0;
	size_t j = m_masterCount;

	size_t count = m_sequenceList.getCount();

	sl::Iterator<SequenceNode> nodeIt = m_sequenceList.getHead();
	for (; nodeIt; nodeIt++, i++, j++)
	{
		SequenceNode* node = *nodeIt;
		node->m_index = i;
		node->m_masterIndex = j;
	}

	m_masterCount = j;
}

void
NodeMgr::indexBeacons()
{
	size_t i = 0;
	size_t j = m_masterCount;

	sl::Iterator<BeaconNode> nodeIt = m_beaconList.getHead();
	for (; nodeIt; nodeIt++, i++, j++)
	{
		BeaconNode* node = *nodeIt;
		node->m_index = i;
		node->m_masterIndex = j;
	}

	m_masterCount = j;
}

void
NodeMgr::indexDispatchers()
{
	size_t i = 0;

	sl::Iterator<DispatcherNode> nodeIt = m_dispatcherList.getHead();
	for (; nodeIt; nodeIt++, i++)
	{
		DispatcherNode* node = *nodeIt;
		node->m_index = i;
	}
}

void
NodeMgr::indexActions()
{
	size_t i = 0;
	size_t j = m_masterCount;

	sl::Iterator<ActionNode> nodeIt = m_actionList.getHead();
	for (; nodeIt; nodeIt++, i++, j++)
	{
		ActionNode* node = *nodeIt;
		node->m_index = i;
		node->m_masterIndex = j;
	}

	m_masterCount = j;
}

void
NodeMgr::indexArguments()
{
	size_t i = 0;
	size_t j = m_masterCount;

	sl::Iterator<ArgumentNode> nodeIt = m_argumentList.getHead();
	for (; nodeIt; nodeIt++, i++, j++)
	{
		ArgumentNode* node = *nodeIt;
		node->m_index = i;
		node->m_masterIndex = j;
	}

	m_masterCount = j;
}

void
NodeMgr::indexLaDfaNodes()
{
	size_t i = 0;
	size_t j = m_masterCount;

	sl::Iterator<LaDfaNode> nodeIt = m_laDfaList.getHead();
	for (; nodeIt; nodeIt++)
	{
		LaDfaNode* node = *nodeIt;

		if (!(node->m_flags & LaDfaNodeFlag_Leaf) &&       // don't index leaves
			(!node->m_resolver || node->m_resolverUplink)) // and non-chained resolvers
		{
			node->m_index = i++;
			node->m_masterIndex = j++;
		}
	}

	m_masterCount = j;
}

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

void
NodeMgr::luaExport(lua::LuaState* luaState)
{
	luaState->setGlobalInteger("StartSymbol", m_primaryStartSymbol ? m_primaryStartSymbol->m_index : -1);
	luaState->setGlobalInteger("PragmaStartSymbol", m_pragmaStartSymbol.m_index);
	luaState->setGlobalInteger("NamedTokenCount", m_namedTokenList.getCount());
	luaState->setGlobalInteger("NamedSymbolCount", m_namedSymbolList.getCount());
	luaState->setGlobalInteger("CatchSymbolCount", m_catchSymbolList.getCount());

	luaExportNodeArray(luaState, "TokenTable", (Node**) (SymbolNode**) m_tokenArray, m_tokenArray.getCount());
	luaExportNodeArray(luaState, "SymbolTable", (Node**) (SymbolNode**) m_symbolArray, m_symbolArray.getCount());
	luaExportNodeList(luaState, "SequenceTable", m_sequenceList.getHead (), m_sequenceList.getCount());
	luaExportNodeList(luaState, "BeaconTable", m_beaconList.getHead (), m_beaconList.getCount());
	luaExportNodeList(luaState, "DispatcherTable", m_dispatcherList.getHead (), m_dispatcherList.getCount());
	luaExportNodeList(luaState, "ActionTable", m_actionList.getHead (), m_actionList.getCount());
	luaExportNodeList(luaState, "ArgumentTable", m_argumentList.getHead (), m_argumentList.getCount());

	luaExportLaDfaTable(luaState);
}

void
NodeMgr::luaExportNodeArray(
	lua::LuaState* luaState,
	const sl::StringRef& name,
	Node* const* nodeArray,
	size_t count
	)
{
	luaState->createTable(count);

	for (size_t i = 0; i < count; i++)
	{
		Node* node = nodeArray[i];
		node->luaExport(luaState);
		luaState->setArrayElement(i + 1);
	}


	luaState->setGlobal(name);
}

void
NodeMgr::luaExportNodeList(
	lua::LuaState* luaState,
	const sl::StringRef& name,
	sl::Iterator<Node> nodeIt,
	size_t countEstimate
	)
{
	luaState->createTable(countEstimate);

	size_t i = 1;

	for (; nodeIt; nodeIt++, i++)
	{
		nodeIt->luaExport(luaState);
		luaState->setArrayElement(i);
	}

	luaState->setGlobal(name);
}

void
NodeMgr::luaExportLaDfaTable(lua::LuaState* luaState)
{
	luaState->createTable(m_laDfaList.getCount());

	size_t i = 1;
	sl::Iterator<LaDfaNode> nodeIt = m_laDfaList.getHead();

	for (; nodeIt; nodeIt++)
	{
		LaDfaNode* node = *nodeIt;

		if (node->m_masterIndex != -1)
		{
			node->luaExport(luaState);
			luaState->setArrayElement(i++);
		}
	}

	luaState->setGlobal("LaDfaTable");
}

//..............................................................................
