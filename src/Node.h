// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#include "axl_g_WarningSuppression.h" // gcc loses warning suppression from pch

//.............................................................................

// forwards

class Class;
class GrammarNode;
class SymbolNode;
class BeaconNode;
class DispatcherNode;
class LaDfaNode;

//.............................................................................

enum NodeKind
{
	NodeKind_Undefined = 0,
	NodeKind_Epsilon,
	NodeKind_Token,
	NodeKind_Symbol,
	NodeKind_Sequence,
	NodeKind_Action,
	NodeKind_Argument,
	NodeKind_Beacon,
	NodeKind_Dispatcher,
	NodeKind_Conflict,
	NodeKind_LaDfa,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

enum NodeFlagKind
{
	NodeFlagKind_RecursionStopper = 0x0001,
	NodeFlagKind_Reachable        = 0x0002,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class Node: public rtl::ListLink
{
public:
	NodeKind m_kind;
	size_t m_index;
	size_t m_masterIndex;

	rtl::String m_name;
	int m_flags;

public:
	Node ();

	virtual
	~Node ()
	{
	}

	virtual
	void
	trace ();

	virtual
	void
	luaExport (lua::LuaState* luaState)
	{
	}

	bool
	isReachable ()
	{
		return (m_flags & NodeFlagKind_Reachable) != 0;
	}

	virtual
	bool
	markReachable ();

	virtual
	rtl::String
	getProductionString ()
	{
		return m_name;
	}

	virtual
	rtl::String
	getBnfString ()
	{
		return m_name;
	}
};

//.............................................................................

enum GrammarNodeFlagKind
{
	GrammarNodeFlagKind_Nullable = 0x0010,
	GrammarNodeFlagKind_Final    = 0x0020,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class GrammarNode: public Node
{
public:
	lex::SrcPos m_srcPos;

	int m_quantifierKind; // '?' '*' '+'
	GrammarNode* m_quantifiedNode;

	rtl::Array <SymbolNode*> m_firstArray;
	rtl::Array <SymbolNode*> m_followArray;

	rtl::BitMap m_firstSet;
	rtl::BitMap m_followSet;

public:
	GrammarNode ()
	{
		m_quantifierKind = 0;
	}

	virtual
	void
	trace ();

	bool
	isNullable ()
	{
		return (m_flags & GrammarNodeFlagKind_Nullable) != 0;
	}

	bool
	isFinal ()
	{
		return (m_flags & GrammarNodeFlagKind_Final) != 0;
	}

	bool
	markNullable ();

	bool
	markFinal ();

	GrammarNode*
	stripBeacon ();

	virtual
	rtl::String
	getBnfString ();

protected:
	void
	luaExportSrcPos (
		lua::LuaState* luaState,
		const lex::LineCol& lineCol
		);
};

//.............................................................................

enum SymbolNodeFlagKind
{
	SymbolNodeFlagKind_Named        = 0x0100,
	SymbolNodeFlagKind_EofToken     = 0x0200,
	SymbolNodeFlagKind_AnyToken     = 0x0400,
	SymbolNodeFlagKind_Pragma       = 0x0800,
	SymbolNodeFlagKind_Start        = 0x1000,
	SymbolNodeFlagKind_NoAst        = 0x2000,
	SymbolNodeFlagKind_ResolverUsed = 0x4000,
	SymbolNodeFlagKind_Nullable     = 0x8000,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class SymbolNode: public GrammarNode
{
public:
	int m_charToken;

	Class* m_class;
	GrammarNode* m_resolver;
	size_t m_resolverPriority;
	rtl::Array <GrammarNode*> m_productionArray;

	rtl::String m_arg;
	rtl::String m_local;
	rtl::String m_enter;
	rtl::String m_leave;

	lex::LineCol m_argLineCol;
	lex::LineCol m_localLineCol;
	lex::LineCol m_enterLineCol;
	lex::LineCol m_leaveLineCol;

	rtl::BoxList <rtl::String> m_argNameList;
	rtl::BoxList <rtl::String> m_localNameList;
	rtl::StringHashTable m_argNameSet;
	rtl::StringHashTable m_localNameSet;

public:
	SymbolNode ();

	rtl::String
	getArgName (size_t index);

	void
	addProduction (GrammarNode* node);

	virtual
	bool
	markReachable ();

	virtual
	void
	trace ();

	virtual
	void
	luaExport (lua::LuaState* luaState);

	virtual
	rtl::String
	getBnfString ();
};

//.............................................................................

class SequenceNode: public GrammarNode
{
public:
	rtl::Array <GrammarNode*> m_sequence;

public:
	SequenceNode ();

	void
	append (GrammarNode* node);

	virtual
	bool
	markReachable ();

	virtual
	void
	trace ();

	virtual
	void
	luaExport (lua::LuaState* luaState);

	virtual
	rtl::String
	getProductionString ();

	virtual
	rtl::String
	getBnfString ();
};

//.............................................................................

enum UserNodeFlagKind
{
	UserNodeFlagKind_UserCodeProcessed = 0x010000, // prevent double processing in '+' quantifier
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class UserNode: public GrammarNode
{
public:
	SymbolNode* m_productionSymbol;
	DispatcherNode* m_dispatcher;
	GrammarNode* m_resolver;

	UserNode ();
};

//.............................................................................

class ActionNode: public UserNode
{
public:
	rtl::String m_userCode;

public:
	ActionNode ();

	virtual
	void
	trace ();

	virtual
	void
	luaExport (lua::LuaState* luaState);

	virtual
	rtl::String
	getBnfString ()
	{
		return rtl::String ();
	}
};

//.............................................................................

class ArgumentNode: public UserNode
{
public:
	SymbolNode* m_targetSymbol;
	rtl::BoxList <rtl::String> m_argValueList;

public:
	ArgumentNode ();

	virtual
	void
	trace ();

	virtual
	void
	luaExport (lua::LuaState* luaState);

	virtual
	rtl::String
	getBnfString ()
	{
		return rtl::String ();
	}
};

//.............................................................................

enum BeaconNodeFlagKind
{
	BeaconNodeFlagKind_Added   = 0x0100,
	BeaconNodeFlagKind_Deleted = 0x0200,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class BeaconNode: public GrammarNode
{
public:
	rtl::String m_label;
	size_t m_slotIndex;
	SymbolNode* m_target;
	ArgumentNode* m_argument;
	GrammarNode* m_resolver;

public:
	BeaconNode ();

	virtual
	bool
	markReachable ();

	virtual
	void
	trace ();

	virtual
	void
	luaExport (lua::LuaState* luaState);

	virtual
	rtl::String
	getBnfString ()
	{
		return m_target ? m_target->getBnfString () : m_name;
	}
};

//.............................................................................

class DispatcherNode: public Node
{
public:
	SymbolNode* m_symbol;
	rtl::Array <BeaconNode*> m_beaconArray;

public:
	DispatcherNode ()
	{
		m_kind = NodeKind_Dispatcher;
	}

	virtual
	void
	trace ();

	virtual
	void
	luaExport (lua::LuaState* luaState);
};

//.............................................................................

class ConflictNode: public Node
{
public:
	SymbolNode* m_symbol;
	SymbolNode* m_token;
	Node* m_resultNode; // lookahead DFA or immediate production
	rtl::Array <GrammarNode*> m_productionArray;

public:
	ConflictNode ();

	virtual
	void
	trace ();

	virtual
	void
	luaExport (lua::LuaState* luaState)
	{
		ASSERT (false); // all the conflicts should be resolved
	}

	void
	pushError ()
	{
		err::pushFormatStringError (
			"conflict at '%s':'%s'",
			m_symbol->m_name.cc (), // thanks a lot gcc
			m_token->m_name.cc ()
			);
	}
};

//.............................................................................

enum LaDfaNodeFlagKind
{
	LaDfaNodeFlagKind_Leaf     = 0x100,
	LaDfaNodeFlagKind_Resolved = 0x200,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class LaDfaNode: public Node
{
public:
	SymbolNode* m_token;
	GrammarNode* m_resolver;
	Node* m_resolverElse;
	LaDfaNode* m_resolverUplink;
	Node* m_production;

	rtl::Array <LaDfaNode*> m_transitionArray;

public:
	LaDfaNode ();

	virtual
	void
	trace ();

	virtual
	void
	luaExport (lua::LuaState* luaState);

protected:
	void
	luaExportResolverMembers (lua::LuaState* luaState);
};

//.............................................................................

template <typename T>
void
traceNodeList (
	const char* name,
	rtl::Iterator <T> nodeIt
	)
{
	printf ("%s\n", name);

	for (; nodeIt; nodeIt++)
	{
		T* node = *nodeIt;

		printf ("%3d/%-3d\t", node->m_index, node->m_masterIndex);
		node->trace ();
	}
}

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

template <typename T>
void
traceNodeArray (
	const char* name,
	const rtl::Array <T*>* array
	)
{
	printf ("%s\n", name);

	size_t count = array->getCount ();
	for (size_t i = 0; i < count; i++)
	{
		T* node = (*array) [i];

		printf ("%3d/%-3d ", node->m_index, node->m_masterIndex);
		node->trace ();
	}
}

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

template <typename T>
rtl::String
nodeArrayToString (const rtl::Array <T*>* array)
{
	size_t count = array->getCount ();
	if (!count)
		return rtl::String ();

	rtl::String string = (*array) [0]->m_name;

	for (size_t i = 1; i < count; i++)
	{
		Node* node = (*array) [i];
		string += ' ';
		string += node->m_name;
	}

	return string;
}

//.............................................................................
