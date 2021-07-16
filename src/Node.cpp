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
#include "Node.h"

//..............................................................................

Node::Node()
{
	m_nodeKind = NodeKind_Undefined;
	m_flags = 0;
	m_index = -1;
	m_masterIndex = -1;
}

void
Node::trace()
{
	printf("%s\n", m_name.sz());
}

bool
Node::markReachable()
{
	if (m_flags & NodeFlag_Reachable)
		return false;

	m_flags |= NodeFlag_Reachable;
	return true;
}

//..............................................................................

GrammarNode::GrammarNode()
{
	m_quantifierKind = 0;
	m_quantifiedNode = NULL;
}

void
GrammarNode::trace()
{
	printf(
		"%s\n"
		"\t  FIRST:  %s%s\n"
		"\t  FOLLOW: %s%s\n",
		m_name.sz(),
		nodeArrayToString(&m_firstArray).sz(),
		isNullable() ? " <eps>" : "",
		nodeArrayToString(&m_followArray).sz(),
		isFinal() ? " $" : ""
		);
}

bool
GrammarNode::markNullable()
{
	if (isNullable())
		return false;

	m_flags |= GrammarNodeFlag_Nullable;
	return true;
}

bool
GrammarNode::markFinal()
{
	if (isFinal())
		return false;

	m_flags |= GrammarNodeFlag_Final;
	return true;
}

bool
GrammarNode::markWeaklyReachable()
{
	if (m_flags & GrammarNodeFlag_WeaklyReachable)
		return false;

	m_flags |= GrammarNodeFlag_WeaklyReachable;
	return true;
}

void
GrammarNode::luaExportSrcPos(
	lua::LuaState* luaState,
	const lex::LineCol& lineCol
	)
{
	luaState->setMemberString("filePath", m_srcPos.m_filePath);
	luaState->setMemberInteger("line", lineCol.m_line);
	luaState->setMemberInteger("col", lineCol.m_col);
}

GrammarNode*
GrammarNode::stripBeacon()
{
	return m_nodeKind == NodeKind_Beacon ? ((BeaconNode*)this)->m_target : this;
}

static
bool
isParenthesNeeded(const sl::StringRef& string)
{
	int level = 0;

	const char* p = string.cp();
	const char* end = string.getEnd();

	for (; p < end; p++)
		switch (*p)
		{
		case ' ':
			if (!level)
				return true;

		case '(':
			level++;
			break;

		case ')':
			level--;
			break;
		}

	return false;
}

sl::String
GrammarNode::getBnfString()
{
	if (!m_quantifierKind)
		return m_name;

	ASSERT(m_quantifiedNode);
	sl::String string = m_quantifiedNode->getBnfString();

	return sl::formatString(
		isParenthesNeeded(string) ? "(%s)%c" : "%s%c",
		string.sz(),
		m_quantifierKind
		);
}

void
GrammarNode::buildFirstFollowArrays(const sl::ArrayRef<SymbolNode*>& tokenArray)
{
	m_firstArray.clear();
	m_followArray.clear();

	for (
		size_t i = m_firstSet.findBit(0);
		i != -1;
		i = m_firstSet.findBit(i + 1)
		)
	{
		SymbolNode* token = tokenArray[i];
		m_firstArray.append(token);
	}

	for (
		size_t i = m_followSet.findBit(0);
		i != -1;
		i = m_followSet.findBit(i + 1)
		)
	{
		SymbolNode* token = tokenArray[i];
		m_followArray.append(token);
	}
}

bool
GrammarNode::propagateChildGrammarProps(GrammarNode* child)
{
	bool hasChanged = false;

	if (m_firstSet.merge(child->m_firstSet, sl::BitOpKind_Or))
		hasChanged = true;

	if (child->m_followSet.merge(m_followSet, sl::BitOpKind_Or))
		hasChanged = true;

	if (child->isNullable())
		if (markNullable())
			hasChanged = true;

	if (isFinal())
		if (child->markFinal())
			hasChanged = true;

	return hasChanged;
}

//..............................................................................

SymbolNode::SymbolNode()
{
	m_nodeKind = NodeKind_Symbol;
	m_charToken = 0;
	m_synchronizer = NULL;
	m_resolver = NULL;
	m_resolverPriority = 0;
	m_lookaheadLimit = 1;
	m_enterIndex = -1;
	m_leaveIndex = -1;
}

void
SymbolNode::addProduction(GrammarNode* node)
{
	if (node->m_nodeKind == NodeKind_Symbol &&
		!(node->m_flags & SymbolNodeFlag_User) &&
		!((SymbolNode*)node)->m_synchronizer &&
		!((SymbolNode*)node)->m_resolver)
	{
		if (m_flags & SymbolNodeFlag_User)
		{
			m_quantifierKind = node->m_quantifierKind;
			m_quantifiedNode = node->m_quantifiedNode;
		}

		m_productionArray.append(((SymbolNode*)node)->m_productionArray); // merge temp symbol productions
	}
	else
	{
		m_productionArray.append(node);
	}
}

bool
SymbolNode::markReachable()
{
	if (!GrammarNode::markReachable())
		return false;

	if (m_synchronizer)
		m_synchronizer->markWeaklyReachable();

	if (m_resolver)
		m_resolver->markReachable();

	size_t count = m_productionArray.getCount();
	for (size_t i = 0; i < count; i++)
	{
		GrammarNode* child = m_productionArray[i];
		child->markReachable();
	}

	return true;
}

bool
SymbolNode::markWeaklyReachable()
{
	if (!GrammarNode::markWeaklyReachable())
		return false;

	if (m_synchronizer)
		m_synchronizer->markWeaklyReachable();

	if (m_resolver)
		m_resolver->markWeaklyReachable();

	size_t count = m_productionArray.getCount();
	for (size_t i = 0; i < count; i++)
	{
		GrammarNode* child = m_productionArray[i];
		child->markWeaklyReachable();
	}

	return true;
}

bool
SymbolNode::propagateGrammarProps()
{
	bool result = false;

	size_t childrenCount = m_productionArray.getCount();

	for (size_t j = 0; j < childrenCount; j++)
	{
		GrammarNode* production = m_productionArray[j];
		if (propagateChildGrammarProps(production))
			result = true;
	}

	return result;
}

void
SymbolNode::trace()
{
	GrammarNode::trace();

	if (m_nodeKind == NodeKind_Token)
		return;

	if (m_synchronizer)
		printf(
			"\t  SYNC:   %s\n",
			nodeArrayToString(&m_synchronizer->m_firstArray).sz()
			);

	if (m_resolver)
		printf("\t  RSLVR:  %s\n", m_resolver->m_name.sz());

	size_t childrenCount = m_productionArray.getCount();

	for (size_t i = 0; i < childrenCount; i++)
	{
		Node* child = m_productionArray[i];
		printf("\t  -> %s\n", child->getProductionString().sz());
	}
}

void
SymbolNode::luaExport(lua::LuaState* luaState)
{
	if (m_nodeKind == NodeKind_Token)
	{
		luaState->createTable(1);

		if (m_flags & SymbolNodeFlag_EofToken)
			luaState->setMemberBoolean("isEofToken", true);
		else if (m_flags & SymbolNodeFlag_AnyToken)
			luaState->setMemberBoolean("isAnyToken", true);
		else if (m_flags & SymbolNodeFlag_User)
			luaState->setMemberString("name", m_name);
		else
			luaState->setMemberInteger("token", m_charToken);

		return;
	}

	luaState->createTable(0, 5);
	luaState->setMemberString("name", m_name);

	luaState->setMemberBoolean(
		"isCustomClass",
		!m_valueBlock.isEmpty() ||
		!m_paramBlock.isEmpty() ||
		!m_localBlock.isEmpty()
		);

	luaState->createTable(0, 3);
	luaExportSrcPos(luaState, m_srcPos);
	luaState->setMember("srcPos");

	if (!m_valueBlock.isEmpty())
	{
		luaState->setMemberString("valueBlock", m_valueBlock);
		luaState->setMemberInteger("valueLine", m_valueLineCol.m_line);
	}

	if (!m_paramBlock.isEmpty())
	{
		luaState->setMemberString("paramBlock", m_paramBlock);
		luaState->setMemberInteger("paramLine", m_paramLineCol.m_line);
	}

	if (!m_localBlock.isEmpty())
	{
		luaState->setMemberString("localBlock", m_localBlock);
		luaState->setMemberInteger("localLine", m_localLineCol.m_line);
	}

	if (!m_enterBlock.isEmpty())
	{
		luaState->setMemberString("enterBlock", m_enterBlock);
		luaState->setMemberInteger("enterLine", m_enterLineCol.m_line);
		luaState->setMemberInteger("enterIndex", m_enterIndex);
	}

	if (!m_leaveBlock.isEmpty())
	{
		luaState->setMemberString("leaveBlock", m_leaveBlock);
		luaState->setMemberInteger("leaveLine", m_leaveLineCol.m_line);
		luaState->setMemberInteger("leaveIndex", m_leaveIndex);
	}

	luaState->createTable(m_paramNameList.getCount());

	sl::BoxIterator<sl::StringRef> it = m_paramNameList.getHead();
	for (size_t i = 1; it; it++, i++)
		luaState->setArrayElementString(i, *it);

	luaState->setMember("paramNameTable");

	if (m_synchronizer)
	{
		size_t count = m_synchronizer->m_firstArray.getCount();
		luaState->createTable(count);

		for (size_t i = 0; i < count; i++)
		{
			SymbolNode* token = m_synchronizer->m_firstArray[i];
			luaState->getGlobalArrayElement("TokenTable", token->m_index + 1);
			luaState->setArrayElement(i + 1);
		}

		luaState->setMember("syncTokenTable");
	}

	size_t childrenCount = m_productionArray.getCount();
	luaState->createTable(childrenCount);

	for (size_t i = 0; i < childrenCount; i++)
	{
		Node* child = m_productionArray[i];
		luaState->setArrayElementInteger(i + 1, child->m_masterIndex);
	}

	luaState->setMember("productionTable");
}

sl::String
SymbolNode::getBnfString()
{
	if (m_nodeKind == NodeKind_Token || (m_flags & SymbolNodeFlag_User))
		return m_name;

	if (m_quantifierKind)
		return GrammarNode::getBnfString();

	size_t productionCount = m_productionArray.getCount();
	if (productionCount == 1)
		return m_productionArray[0]->stripBeacon()->getBnfString();

	sl::String string = "(";
	string += m_productionArray[0]->stripBeacon()->getBnfString();
	for (size_t i = 1; i < productionCount; i++)
	{
		string += " | ";
		string += m_productionArray[i]->stripBeacon()->getBnfString();
	}

	string += ")";
	return string;
}

//..............................................................................

void
SequenceNode::append(GrammarNode* node)
{
	if (node->m_nodeKind == NodeKind_Sequence)
		m_sequence.append(((SequenceNode*)node)->m_sequence); // merge sequences
	else
		m_sequence.append(node);
}

bool
SequenceNode::markReachable()
{
	if (!GrammarNode::markReachable())
		return false;

	size_t count = m_sequence.getCount();
	for (size_t i = 0; i < count; i++)
		m_sequence[i]->markReachable();

	return true;
}

bool
SequenceNode::markWeaklyReachable()
{
	if (!GrammarNode::markWeaklyReachable())
		return false;

	size_t count = m_sequence.getCount();
	for (size_t i = 0; i < count; i++)
		m_sequence[i]->markWeaklyReachable();

	return true;
}

bool
SequenceNode::propagateGrammarProps()
{
	bool hasChanged = false;

	size_t childrenCount = m_sequence.getCount();

	// FIRST between parent-child

	bool isNullable = true;
	for (size_t j = 0; j < childrenCount; j++)
	{
		GrammarNode* child = m_sequence[j];
		if (m_firstSet.merge(child->m_firstSet, sl::BitOpKind_Or))
			hasChanged = true;

		if (!child->isNullable())
		{
			isNullable = false;
			break;
		}
	}

	if (isNullable) // all nullable
		if (markNullable())
			hasChanged = true;

	// FOLLOW between parent-child

	for (intptr_t j = childrenCount - 1; j >= 0; j--)
	{
		GrammarNode* child = m_sequence[j];
		if (child->m_followSet.merge(m_followSet, sl::BitOpKind_Or))
			hasChanged = true;

		if (isFinal())
			if (child->markFinal())
				hasChanged = true;

		if (!child->isNullable())
			break;
	}

	// FOLLOW between child-child

	if (childrenCount >= 2)
		for (size_t j = 0; j < childrenCount - 1; j++)
		{
			GrammarNode* child = m_sequence[j];
			for (size_t k = j + 1; k < childrenCount; k++)
			{
				GrammarNode* next = m_sequence[k];
				if (child->m_followSet.merge(next->m_firstSet, sl::BitOpKind_Or))
					hasChanged = true;

				if (!next->isNullable())
					break;
			}
		}

	return hasChanged;
}

void
SequenceNode::trace()
{
	GrammarNode::trace();
	printf("\t  %s\n", nodeArrayToString(&m_sequence).sz());
}

void
SequenceNode::luaExport(lua::LuaState* luaState)
{
	luaState->createTable(0, 2);
	luaState->setMemberString("name", m_name);

	size_t count = m_sequence.getCount();
	luaState->createTable(count);

	for (size_t j = 0; j < count; j++)
	{
		Node* child = m_sequence[j];
		luaState->setArrayElementInteger(j + 1, child->m_masterIndex);
	}

	luaState->setMember("sequence");
}

sl::String
SequenceNode::getProductionString()
{
	return sl::formatString(
		"%s: %s",
		m_name.sz(),
		nodeArrayToString(&m_sequence).sz()
		);
}

sl::String
SequenceNode::getBnfString()
{
	if (m_quantifierKind)
		return GrammarNode::getBnfString();

	sl::String sequenceString;

	size_t sequenceLength = m_sequence.getCount();
	ASSERT(sequenceLength > 1);

	for (size_t i = 0; i < sequenceLength; i++)
	{
		GrammarNode* sequenceEntry = m_sequence[i]->stripBeacon();

		sl::String entryString = sequenceEntry->getBnfString();
		if (entryString.isEmpty())
			continue;

		if (!sequenceString.isEmpty())
			sequenceString.append(' ');

		sequenceString.append(entryString);
	}

	return sequenceString;
}

//..............................................................................

UserNode::UserNode()
{
	m_flags = GrammarNodeFlag_Nullable;
	m_productionSymbol = NULL;
	m_dispatcher = NULL;
}

//..............................................................................

void
ActionNode::trace()
{
	printf(
		"%s\n"
		"\t  SYMBOL:     %s\n"
		"\t  DISPATCHER: %s\n"
		"\t  { %s }\n",
		m_name.sz(),
		m_productionSymbol->m_name.sz(),
		m_dispatcher ? m_dispatcher->m_name.sz() : "NONE",
		m_userCode.sz()
		);
}

void
ActionNode::luaExport(lua::LuaState* luaState)
{
	luaState->createTable(0, 2);

	if (m_dispatcher)
	{
		luaState->getGlobalArrayElement("DispatcherTable", m_dispatcher->m_index + 1);
		luaState->setMember("dispatcher");
	}

	luaState->getGlobalArrayElement("SymbolTable", m_productionSymbol->m_index + 1);
	luaState->setMember("productionSymbol");

	luaState->setMemberString("userCode", m_userCode);

	luaState->createTable(0, 3);
	luaExportSrcPos(luaState, m_srcPos);
	luaState->setMember("srcPos");
}

//..............................................................................

ArgumentNode::ArgumentNode()
{
	m_nodeKind = NodeKind_Argument;
	m_targetSymbol = NULL;
}

void
ArgumentNode::trace()
{
	printf(
		"%s\n"
		"\t  SYMBOL: %s\n"
		"\t  DISPATCHER: %s\n"
		"\t  TARGET SYMBOL: %s\n"
		"\t  <",
		m_name.sz(),
		m_productionSymbol->m_name.sz(),
		m_dispatcher ? m_dispatcher->m_name.sz() : "NONE",
		m_targetSymbol->m_name.sz()
		);

	sl::BoxIterator<sl::String> it = m_argValueList.getHead();
	ASSERT(it); // empty argument should have been eliminated

	printf("%s", it->sz ());

	for (it++; it; it++)
		printf(", %s", it->sz ());

	printf(">\n");
}

void
ArgumentNode::luaExport(lua::LuaState* luaState)
{
	luaState->createTable(0, 3);

	if (m_dispatcher)
	{
		luaState->getGlobalArrayElement("DispatcherTable", m_dispatcher->m_index + 1);
		luaState->setMember("dispatcher");
	}

	luaState->getGlobalArrayElement("SymbolTable", m_productionSymbol->m_index + 1);
	luaState->setMember("productionSymbol");
	luaState->getGlobalArrayElement("SymbolTable", m_targetSymbol->m_index + 1);
	luaState->setMember("targetSymbol");

	luaState->createTable(m_argValueList.getCount());

	sl::BoxIterator<sl::String> it = m_argValueList.getHead();
	ASSERT(it); // empty argument sh ould have been eliminated

	for (size_t i = 1; it; it++, i++)
		luaState->setArrayElementString(i, *it);

	luaState->setMember("valueTable");

	luaState->createTable(0, 3);
	luaExportSrcPos(luaState, m_srcPos);
	luaState->setMember("srcPos");
}

//..............................................................................

BeaconNode::BeaconNode()
{
	m_nodeKind = NodeKind_Beacon;
	m_slotIndex = -1;
	m_target = NULL;
	m_argument = NULL;
}

bool
BeaconNode::markReachable()
{
	if (!GrammarNode::markReachable())
		return false;

	m_target->markReachable();
	return true;
}

bool
BeaconNode::markWeaklyReachable()
{
	if (!GrammarNode::markWeaklyReachable())
		return false;

	m_target->markWeaklyReachable();
	return true;
}

bool
BeaconNode::propagateGrammarProps()
{
	return propagateChildGrammarProps(m_target);
}

void
BeaconNode::trace()
{
	ASSERT(m_target);

	GrammarNode::trace();

	printf(
		"\t  $%d => %s\n",
		m_slotIndex,
		m_target->m_name.sz()
		);
}

void
BeaconNode::luaExport(lua::LuaState* luaState)
{
	luaState->createTable(0, 2);
	luaState->setMemberInteger("slot", m_slotIndex);
	luaState->setMemberInteger("target", m_target->m_masterIndex);
}

//..............................................................................

DispatcherNode::DispatcherNode()
{
	m_nodeKind = NodeKind_Dispatcher;
	m_symbol = NULL;
}

void
DispatcherNode::trace()
{
	ASSERT(m_symbol);

	printf(
		"%s\n"
		"\t  @ %s\n"
		"\t  %s\n",
		m_name.sz(),
		m_symbol->m_name.sz(),
		nodeArrayToString(&m_beaconArray).sz()
		);
}

void
DispatcherNode::luaExport(lua::LuaState* luaState)
{
	luaState->createTable(0, 3);

	luaState->getGlobalArrayElement("SymbolTable", m_symbol->m_index + 1);
	luaState->setMember("symbol");

	size_t beaconCount = m_beaconArray.getCount();
	luaState->createTable(beaconCount);

	for (size_t j = 0; j < beaconCount; j++)
	{
		BeaconNode* beacon = m_beaconArray[j];
		ASSERT(beacon->m_slotIndex == j);

		luaState->createTable(1);
		if (beacon->m_target->m_nodeKind == NodeKind_Symbol)
		{
			luaState->getGlobalArrayElement("SymbolTable", beacon->m_target->m_index + 1);
			luaState->setMember("symbol");
		}

		luaState->setArrayElement(j + 1);
	}

	luaState->setMember("beaconTable");
}

//..............................................................................

ConflictNode::ConflictNode()
{
	m_nodeKind = NodeKind_Conflict;
	m_symbol = NULL;
	m_token = NULL;
	m_resultNode = NULL;
}

void
ConflictNode::trace()
{
	ASSERT(m_symbol);
	ASSERT(m_token);

	printf(
		"%s\n"
		"\t  on %s in %s\n"
		"\t  DFA:      %s\n"
		"\t  POSSIBLE:\n",
		m_name.sz(),
		m_token->m_name.sz(),
		m_symbol->m_name.sz(),
		m_resultNode ? m_resultNode->m_name.sz() : "<none>"
		);

	size_t count = m_productionArray.getCount();
	for (size_t i = 0; i < count; i++)
	{
		Node* node = m_productionArray[i];
		printf("\t  \t-> %s\n", node->getProductionString().sz());
	}
}

//..............................................................................

LaDfaNode::LaDfaNode()
{
	m_nodeKind = NodeKind_LaDfa;
	m_token = NULL;
	m_resolver = NULL;
	m_resolverElse = NULL;
	m_resolverUplink = NULL;
	m_production = NULL;
}

void
LaDfaNode::trace()
{
	printf(
		"%s%s\n",
		m_name.sz(),
		(m_flags & LaDfaNodeFlag_Leaf) ? "*" :
		(m_flags & LaDfaNodeFlag_Resolved) ? "~" : ""
		);

	if (m_resolver)
	{
		printf(
			"\t  if resolver (%s) %s\n"
			"\t  else %s\n",
			m_resolver->m_name.sz(),
			m_production->getProductionString().sz(),
			m_resolverElse->getProductionString().sz()
			);
	}
	else
	{
		size_t count = m_transitionArray.getCount();
		for (size_t i = 0; i < count; i++)
		{
			LaDfaNode* child = m_transitionArray[i];
			printf(
				"\t  %s -> %s\n",
				child->m_token->m_name.sz(),
				child->getProductionString().sz()
				);
		}

		if (m_production)
		{
			printf(
				"\t  . -> %s\n",
				m_production->getProductionString().sz()
				);
		}
	}

	printf("\n");
}

size_t
getTransitionIndex(Node* node)
{
	if (node->m_nodeKind != NodeKind_LaDfa || !(node->m_flags & LaDfaNodeFlag_Leaf))
		return node->m_masterIndex;

	LaDfaNode* laDfaNode = (LaDfaNode*)node;
	ASSERT(laDfaNode->m_production && laDfaNode->m_production->m_nodeKind != NodeKind_LaDfa);
	return laDfaNode->m_production->m_masterIndex;
}

void
LaDfaNode::luaExportResolverMembers(lua::LuaState* luaState)
{
	luaState->setMemberString("name", m_name);
	luaState->setMemberInteger("resolver", m_resolver->m_masterIndex);
	luaState->setMemberInteger("production", m_production->m_masterIndex);
	luaState->setMemberInteger("resolverElse", getTransitionIndex (m_resolverElse));
	luaState->setMemberBoolean("hasChainedResolver", ((LaDfaNode*) m_resolverElse)->m_resolver != NULL);
}

void
LaDfaNode::luaExport(lua::LuaState* luaState)
{
	ASSERT(!(m_flags & LaDfaNodeFlag_Leaf));

	if (m_resolver)
	{
		luaState->createTable(0, 3);
		luaExportResolverMembers(luaState);
		return;
	}

	size_t childrenCount = m_transitionArray.getCount();
	ASSERT(childrenCount);

	luaState->createTable(0, 2);

	luaState->createTable(childrenCount);

	Node* defaultProduction = m_production;

	for (size_t i = 0; i < childrenCount; i++)
	{
		LaDfaNode* child = m_transitionArray[i];

		if ((child->m_token->m_flags & SymbolNodeFlag_AnyToken) && !defaultProduction)
			defaultProduction = child->m_production;

		luaState->createTable(0, 4);
		luaState->getGlobalArrayElement("TokenTable", child->m_token->m_index + 1);
		luaState->setMember("token");

		if (child->m_resolver)
			child->luaExportResolverMembers(luaState);
		else
			luaState->setMemberInteger("production", getTransitionIndex(child));

		luaState->setArrayElement(i + 1);
	}

	luaState->setMember("transitionTable");

	if (defaultProduction)
		luaState->setMemberInteger("defaultProduction", getTransitionIndex(defaultProduction));
}

//..............................................................................
