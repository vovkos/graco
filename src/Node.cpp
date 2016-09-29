#include "pch.h"
#include "Node.h"
#include "ClassMgr.h"

#include "axl_g_WarningSuppression.h" // gcc loses warning suppression from pch

//.............................................................................

Node::Node ()
{
	m_kind = NodeKind_Undefined;
	m_index = -1;
	m_masterIndex = -1;
	m_flags = 0;
}

void
Node::trace ()
{
	printf ("%s\n", m_name.cc ()); // thanks a lot gcc
}

bool
Node::markReachable ()
{
	if (m_flags & NodeFlag_Reachable)
		return false;

	m_flags |= NodeFlag_Reachable;
	return true;
}

//.............................................................................

void
GrammarNode::trace ()
{
	printf (
		"%s\n"
		"\t  FIRST:  %s%s\n"
		"\t  FOLLOW: %s%s\n",
		m_name.cc (),
		nodeArrayToString (&m_firstArray).cc (),
		isNullable () ? " <eps>" : "",
		nodeArrayToString (&m_followArray).cc (),
		isFinal () ? " $" : ""
		);
}

bool
GrammarNode::markNullable ()
{
	if (isNullable ())
		return false;

	m_flags |= GrammarNodeFlag_Nullable;
	return true;
}

bool
GrammarNode::markFinal ()
{
	if (isFinal ())
		return false;

	m_flags |= GrammarNodeFlag_Final;
	return true;
}

void
GrammarNode::luaExportSrcPos (
	lua::LuaState* luaState,
	const lex::LineCol& lineCol
	)
{
	luaState->setMemberString ("FilePath", m_srcPos.m_filePath);
	luaState->setMemberInteger ("Line", lineCol.m_line);
	luaState->setMemberInteger ("Col", lineCol.m_col);
}

GrammarNode*
GrammarNode::stripBeacon ()
{
	return m_kind == NodeKind_Beacon ? ((BeaconNode*) this)->m_target : this;
}

static
bool
isParenthesNeeded (const char* p)
{
	int level = 0;

	for (;; p++)
		switch (*p)
		{
		case 0:
			return false;

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
}

sl::String
GrammarNode::getBnfString ()
{
	if (!m_quantifierKind)
		return m_name;

	ASSERT (m_quantifiedNode);
	sl::String string = m_quantifiedNode->getBnfString ();

	return sl::String::format_s (
		isParenthesNeeded (string) ? "(%s)%c" : "%s%c",
		string.cc (),
		m_quantifierKind
		);
}

//.............................................................................

SymbolNode::SymbolNode ()
{
	m_kind = NodeKind_Symbol;
	m_charToken = 0;
	m_class = NULL;
	m_resolver = NULL;
	m_resolverPriority = 0;
}

sl::String
SymbolNode::getArgName (size_t index)
{
	ASSERT (index < m_argNameList.getCount ());

	sl::BoxIterator <sl::String> it = m_argNameList.getHead ();
	for (size_t i = 0; i < index; i++)
		it++;

	return *it;
}

void
SymbolNode::addProduction (GrammarNode* node)
{
	if (node->m_kind == NodeKind_Symbol &&
		!(node->m_flags & SymbolNodeFlag_Named) &&
		!((SymbolNode*) node)->m_resolver)
	{
		if (m_flags & SymbolNodeFlag_Named)
		{
			m_quantifierKind = node->m_quantifierKind;
			m_quantifiedNode = node->m_quantifiedNode;
		}

		m_productionArray.append (((SymbolNode*) node)->m_productionArray); // merge temp symbol productions
	}
	else
	{
		m_productionArray.append (node);
	}
}

bool
SymbolNode::markReachable ()
{
	if (!Node::markReachable ())
		return false;

	if (m_resolver)
		m_resolver->markReachable ();

	if (m_class)
		m_class->m_flags |= ClassFlag_Reachable;

	size_t count = m_productionArray.getCount ();
	for (size_t i = 0; i < count; i++)
	{
		Node* child = m_productionArray [i];
		child->markReachable ();
	}

	return true;
}

void
SymbolNode::trace ()
{
	GrammarNode::trace ();

	if (m_kind == NodeKind_Token)
		return;

	if (m_resolver)
		printf ("\t  RSLVR:  %s\n", m_resolver->m_name.cc ());

	if (m_class)
		printf ("\t  CLASS:  %s\n", m_class->m_name.cc ());

	size_t childrenCount = m_productionArray.getCount ();

	for (size_t i = 0; i < childrenCount; i++)
	{
		Node* child = m_productionArray [i];
		printf ("\t  -> %s\n", child->getProductionString ().cc ());
	}
}

void
SymbolNode::luaExport (lua::LuaState* luaState)
{
	if (m_kind == NodeKind_Token)
	{
		luaState->createTable (1);

		if (m_flags & SymbolNodeFlag_EofToken)
			luaState->setMemberBoolean ("IsEofToken", true);
		else if (m_flags & SymbolNodeFlag_AnyToken)
			luaState->setMemberBoolean ("IsAnyToken", true);
		else if (m_flags & SymbolNodeFlag_Named)
			luaState->setMemberString ("Name", m_name);
		else
			luaState->setMemberInteger ("Token", m_charToken);

		return;
	}

	luaState->createTable (0, 5);
	luaState->setMemberString ("Name", m_name);
	luaState->setMemberBoolean ("IsCustom", !m_arg.isEmpty () || !m_local.isEmpty ());

	luaState->createTable (0, 3);
	luaExportSrcPos (luaState, m_srcPos);
	luaState->setMember ("SrcPos");

	if (m_flags & SymbolNodeFlag_NoAst)
		luaState->setMemberBoolean ("IsNoAst", true);
	else if (m_class)
		luaState->setMemberString ("Class", m_class->m_name);

	if (!m_arg.isEmpty ())
	{
		luaState->setMemberString ("Arg", m_arg);
		luaState->setMemberInteger ("ArgLine", m_argLineCol.m_line);
	}

	if (!m_local.isEmpty ())
	{
		luaState->setMemberString ("Local", m_local);
		luaState->setMemberInteger ("LocalLine", m_localLineCol.m_line);
	}

	if (!m_enter.isEmpty ())
	{
		luaState->setMemberString ("Enter", m_enter);
		luaState->setMemberInteger ("EnterLine", m_enterLineCol.m_line);
	}

	if (!m_leave.isEmpty ())
	{
		luaState->setMemberString ("Leave", m_leave);
		luaState->setMemberInteger ("LeaveLine", m_leaveLineCol.m_line);
	}

	luaState->createTable (m_argNameList.getCount ());

	sl::BoxIterator <sl::String> it = m_argNameList.getHead ();
	for (size_t i = 1; it; it++, i++)
		luaState->setArrayElementString (i, *it);

	luaState->setMember ("ArgNameTable");

	size_t childrenCount = m_productionArray.getCount ();
	luaState->createTable (childrenCount);

	for (size_t i = 0; i < childrenCount; i++)
	{
		Node* child = m_productionArray [i];
		luaState->setArrayElementInteger (i + 1, child->m_masterIndex);
	}

	luaState->setMember ("ProductionTable");
}

sl::String
SymbolNode::getBnfString ()
{
	if (m_kind == NodeKind_Token || (m_flags & SymbolNodeFlag_Named))
		return m_name;

	if (m_quantifierKind)
		return GrammarNode::getBnfString ();

	size_t productionCount = m_productionArray.getCount ();
	if (productionCount == 1)
		return m_productionArray [0]->stripBeacon ()->getBnfString ();

	sl::String string = "(";
	string += m_productionArray [0]->stripBeacon ()->getBnfString ();
	for (size_t i = 1; i < productionCount; i++)
	{
		string += " | ";
		string += m_productionArray [i]->stripBeacon ()->getBnfString ();
	}

	string += ")";
	return string;
}

//.............................................................................

SequenceNode::SequenceNode ()
{
	m_kind = NodeKind_Sequence;
}

void
SequenceNode::append (GrammarNode* node)
{
	if (node->m_kind == NodeKind_Sequence)
		m_sequence.append (((SequenceNode*) node)->m_sequence); // merge sequences
	else
		m_sequence.append (node);
}

bool
SequenceNode::markReachable ()
{
	if (!Node::markReachable ())
		return false;

	size_t count = m_sequence.getCount ();
	for (size_t i = 0; i < count; i++)
	{
		Node* child = m_sequence [i];
		child->markReachable ();
	}

	return true;
}

void
SequenceNode::trace ()
{
	GrammarNode::trace ();
	printf ("\t  %s\n", nodeArrayToString (&m_sequence).cc ());
}

void
SequenceNode::luaExport (lua::LuaState* luaState)
{
	luaState->createTable (0, 2);
	luaState->setMemberString ("Name", m_name);

	size_t count = m_sequence.getCount ();
	luaState->createTable (count);

	for (size_t j = 0; j < count; j++)
	{
		Node* child = m_sequence [j];
		luaState->setArrayElementInteger (j + 1, child->m_masterIndex);
	}

	luaState->setMember ("Sequence");
}

sl::String
SequenceNode::getProductionString ()
{
	return sl::String::format_s (
		"%s: %s",
		m_name.cc (),
		nodeArrayToString (&m_sequence).cc ()
		);
}

sl::String
SequenceNode::getBnfString ()
{
	if (m_quantifierKind)
		return GrammarNode::getBnfString ();

	sl::String sequenceString;

	size_t sequenceLength = m_sequence.getCount ();
	ASSERT (sequenceLength > 1);

	for (size_t i = 0; i < sequenceLength; i++)
	{
		GrammarNode* sequenceEntry = m_sequence [i]->stripBeacon ();

		sl::String entryString = sequenceEntry->getBnfString ();
		if (entryString.isEmpty ())
			continue;

		if (!sequenceString.isEmpty ())
			sequenceString.append (' ');

		sequenceString.append (entryString);
	}

	return sequenceString;
}

//.............................................................................

UserNode::UserNode ()
{
	m_flags = GrammarNodeFlag_Nullable;
	m_productionSymbol = NULL;
	m_dispatcher = NULL;
	m_resolver = NULL;
}

//.............................................................................

ActionNode::ActionNode ()
{
	m_kind = NodeKind_Action;
}

void
ActionNode::trace ()
{
	printf (
		"%s\n"
		"\t  SYMBOL:     %s\n"
		"\t  DISPATCHER: %s\n"
		"\t  { %s }\n",
		m_name.cc (),
		m_productionSymbol->m_name.cc (),
		m_dispatcher ? m_dispatcher->m_name.cc () : "NONE",
		m_userCode.cc ()
		);
}

void
ActionNode::luaExport (lua::LuaState* luaState)
{
	luaState->createTable (0, 2);

	if (m_dispatcher)
	{
		luaState->getGlobalArrayElement ("DispatcherTable", m_dispatcher->m_index + 1);
		luaState->setMember ("Dispatcher");
	}

	luaState->getGlobalArrayElement ("SymbolTable", m_productionSymbol->m_index + 1);
	luaState->setMember ("ProductionSymbol");

	luaState->setMemberString ("UserCode", m_userCode);

	luaState->createTable (0, 3);
	luaExportSrcPos (luaState, m_srcPos);
	luaState->setMember ("SrcPos");
}

//.............................................................................

ArgumentNode::ArgumentNode ()
{
	m_kind = NodeKind_Argument;
	m_targetSymbol = NULL;
}

void
ArgumentNode::trace ()
{
	printf (
		"%s\n"
		"\t  SYMBOL: %s\n"
		"\t  DISPATCHER: %s\n"
		"\t  TARGET SYMBOL: %s\n"
		"\t  <",
		m_name.cc (),
		m_productionSymbol->m_name.cc (),
		m_dispatcher ? m_dispatcher->m_name.cc () : "NONE",
		m_targetSymbol->m_name.cc ()
		);

	sl::BoxIterator <sl::String> it = m_argValueList.getHead ();
	ASSERT (it); // empty argument should have been eliminated

	printf ("%s", it->cc ());

	for (it++; it; it++)
		printf (", %s", it->cc ());

	printf (">\n");
}

void
ArgumentNode::luaExport (lua::LuaState* luaState)
{
	luaState->createTable (0, 3);

	if (m_dispatcher)
	{
		luaState->getGlobalArrayElement ("DispatcherTable", m_dispatcher->m_index + 1);
		luaState->setMember ("Dispatcher");
	}

	luaState->getGlobalArrayElement ("SymbolTable", m_productionSymbol->m_index + 1);
	luaState->setMember ("ProductionSymbol");
	luaState->getGlobalArrayElement ("SymbolTable", m_targetSymbol->m_index + 1);
	luaState->setMember ("TargetSymbol");

	luaState->createTable (m_argValueList.getCount ());

	sl::BoxIterator <sl::String> it = m_argValueList.getHead ();
	ASSERT (it); // empty argument should have been eliminated

	for (size_t i = 1; it; it++, i++)
		luaState->setArrayElementString (i, *it);

	luaState->setMember ("ValueTable");

	luaState->createTable (0, 3);
	luaExportSrcPos (luaState, m_srcPos);
	luaState->setMember ("SrcPos");
}

//.............................................................................

BeaconNode::BeaconNode ()
{
	m_kind = NodeKind_Beacon;
	m_slotIndex = -1;
	m_target = NULL;
	m_argument = NULL;
	m_resolver = NULL;
}

bool
BeaconNode::markReachable ()
{
	if (!Node::markReachable ())
		return false;

	m_target->markReachable ();
	return true;
}

void
BeaconNode::trace ()
{
	ASSERT (m_target);

	GrammarNode::trace ();

	printf (
		"\t  $%d => %s\n",
		m_slotIndex,
		m_target->m_name.cc ()
		);
}

void
BeaconNode::luaExport (lua::LuaState* luaState)
{
	luaState->createTable (0, 2);
	luaState->setMemberInteger ("Slot", m_slotIndex);
	luaState->setMemberInteger ("Target", m_target->m_masterIndex);
}

//.............................................................................

void
DispatcherNode::trace ()
{
	ASSERT (m_symbol);

	printf (
		"%s\n"
		"\t  @ %s\n"
		"\t  %s\n",
		m_name.cc (),
		m_symbol->m_name.cc (),
		nodeArrayToString (&m_beaconArray).cc ()
		);
}

void
DispatcherNode::luaExport (lua::LuaState* luaState)
{
	luaState->createTable (0, 3);

	luaState->getGlobalArrayElement ("SymbolTable", m_symbol->m_index + 1);
	luaState->setMember ("Symbol");

	size_t beaconCount = m_beaconArray.getCount ();
	luaState->createTable (beaconCount);

	for (size_t j = 0; j < beaconCount; j++)
	{
		BeaconNode* beacon = m_beaconArray [j];
		ASSERT (beacon->m_slotIndex == j);

		luaState->createTable (1);
		if (beacon->m_target->m_kind == NodeKind_Symbol)
		{
			luaState->getGlobalArrayElement ("SymbolTable", beacon->m_target->m_index + 1);
			luaState->setMember ("Symbol");
		}

		luaState->setArrayElement (j + 1);
	}

	luaState->setMember ("BeaconTable");
}

//.............................................................................

ConflictNode::ConflictNode ()
{
	m_kind = NodeKind_Conflict;
	m_symbol = NULL;
	m_token = NULL;
	m_resultNode = NULL;
}

void
ConflictNode::trace ()
{
	ASSERT (m_symbol);
	ASSERT (m_token);

	printf (
		"%s\n"
		"\t  on %s in %s\n"
		"\t  DFA:      %s\n"
		"\t  POSSIBLE:\n",
		m_name.cc (),
		m_token->m_name.cc (),
		m_symbol->m_name.cc (),
		m_resultNode ? m_resultNode->m_name.cc () : "<none>"
		);

	size_t count = m_productionArray.getCount ();
	for (size_t i = 0; i < count; i++)
	{
		Node* node = m_productionArray [i];
		printf ("\t  \t-> %s\n", node->getProductionString ().cc ());
	}
}

//.............................................................................

LaDfaNode::LaDfaNode ()
{
	m_kind = NodeKind_LaDfa;
	m_token = NULL;
	m_resolver = NULL;
	m_resolverElse = NULL;
	m_resolverUplink = NULL;
	m_production = NULL;
}

void
LaDfaNode::trace ()
{
	printf (
		"%s%s\n",
		m_name.cc (),
		(m_flags & LaDfaNodeFlag_Leaf) ? "*" :
		(m_flags & LaDfaNodeFlag_Resolved) ? "~" : ""
		);

	if (m_resolver)
	{
		printf (
			"\t  if resolver (%s) %s\n"
			"\t  else %s\n",
			m_resolver->m_name.cc (),
			m_production->getProductionString ().cc (),
			m_resolverElse->getProductionString ().cc ()
			);
	}
	else
	{
		size_t count = m_transitionArray.getCount ();
		for (size_t i = 0; i < count; i++)
		{
			LaDfaNode* child = m_transitionArray [i];
			printf (
				"\t  %s -> %s\n",
				child->m_token->m_name.cc (),
				child->getProductionString ().cc ()
				);
		}

		if (m_production)
		{
			printf (
				"\t  . -> %s\n",
				m_production->getProductionString ().cc ()
				);
		}
	}

	printf ("\n");
}

size_t
getTransitionIndex (Node* node)
{
	if (node->m_kind != NodeKind_LaDfa || !(node->m_flags & LaDfaNodeFlag_Leaf))
		return node->m_masterIndex;

	LaDfaNode* laDfaNode = (LaDfaNode*) node;
	ASSERT (laDfaNode->m_production && laDfaNode->m_production->m_kind != NodeKind_LaDfa);
	return laDfaNode->m_production->m_masterIndex;
}

void
LaDfaNode::luaExportResolverMembers (lua::LuaState* luaState)
{
	luaState->setMemberString ("Name", m_name);
	luaState->setMemberInteger ("Resolver", m_resolver->m_masterIndex);
	luaState->setMemberInteger ("Production", m_production->m_masterIndex);
	luaState->setMemberInteger ("ResolverElse", getTransitionIndex (m_resolverElse));
	luaState->setMemberBoolean ("HasChainedResolver", ((LaDfaNode*) m_resolverElse)->m_resolver != NULL);
}

void
LaDfaNode::luaExport (lua::LuaState* luaState)
{
	ASSERT (!(m_flags & LaDfaNodeFlag_Leaf));

	if (m_resolver)
	{
		luaState->createTable (0, 3);
		luaExportResolverMembers (luaState);
		return;
	}

	size_t childrenCount = m_transitionArray.getCount ();
	ASSERT (childrenCount);

	luaState->createTable (0, 2);

	luaState->createTable (childrenCount);

	for (size_t i = 0; i < childrenCount; i++)
	{
		LaDfaNode* child = m_transitionArray [i];

		luaState->createTable (0, 4);

		luaState->getGlobalArrayElement ("TokenTable", child->m_token->m_index + 1);
		luaState->setMember ("Token");

		if (child->m_resolver)
			child->luaExportResolverMembers (luaState);
		else
			luaState->setMemberInteger ("Production", getTransitionIndex (child));

		luaState->setArrayElement (i + 1);
	}

	luaState->setMember ("TransitionTable");

	if (m_production)
		luaState->setMemberInteger ("DefaultProduction", getTransitionIndex (m_production));
}

//.............................................................................
