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

//..............................................................................

// forwards

class GrammarNode;
class SymbolNode;
class BeaconNode;
class DispatcherNode;
class LaDfaNode;

//..............................................................................

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

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

enum NodeFlag
{
	NodeFlag_RecursionStopper = 0x0001,
	NodeFlag_Reachable        = 0x0002,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class Node: public sl::ListLink
{
public:
	NodeKind m_nodeKind;
	uint_t m_flags;
	size_t m_index;
	size_t m_masterIndex;

	sl::String m_name;

public:
	Node();

	virtual
	~Node()
	{
	}

	virtual
	void
	trace();

	virtual
	void
	luaExport(lua::LuaState* luaState)
	{
	}

	virtual
	bool
	markReachable();

	virtual
	sl::String
	getProductionString()
	{
		return m_name;
	}

	virtual
	sl::String
	getBnfString()
	{
		return m_name;
	}
};

//..............................................................................

enum GrammarNodeFlag
{
	GrammarNodeFlag_Nullable        = 0x0010,
	GrammarNodeFlag_Final           = 0x0020,
	GrammarNodeFlag_WeaklyReachable = 0x0040, // remove from the grammar, but don't delete
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class GrammarNode: public Node
{
public:
	lex::SrcPos m_srcPos;
	int m_quantifierKind; // '?' '*' '+'
	GrammarNode* m_quantifiedNode;

	sl::Array<SymbolNode*> m_firstArray;
	sl::Array<SymbolNode*> m_followArray;

	sl::BitMap m_firstSet;
	sl::BitMap m_followSet;

public:
	GrammarNode();

	virtual
	void
	trace();

	bool
	isNullable()
	{
		return (m_flags & GrammarNodeFlag_Nullable) != 0;
	}

	bool
	isFinal()
	{
		return (m_flags & GrammarNodeFlag_Final) != 0;
	}

	bool
	markNullable();

	bool
	markFinal();

	virtual
	bool
	markWeaklyReachable();

	bool
	initializeFirstFollowSets(size_t tokenCount)
	{
		return m_firstSet.setBitCount(tokenCount) && m_followSet.setBitCount(tokenCount);
	}

	void
	buildFirstFollowArrays(const sl::ArrayRef<SymbolNode*>& tokenArray);

	virtual
	bool
	propagateGrammarProps()
	{
		return false;
	}

	GrammarNode*
	stripBeacon();

	virtual
	sl::String
	getBnfString();

protected:
	bool
	propagateChildGrammarProps(GrammarNode* child);

	void
	luaExportSrcPos(
		lua::LuaState* luaState,
		const lex::LineCol& lineCol
		);
};

//..............................................................................

enum SymbolNodeFlag
{
	SymbolNodeFlag_User         = 0x0100,
	SymbolNodeFlag_EofToken     = 0x0200,
	SymbolNodeFlag_AnyToken     = 0x0400,
	SymbolNodeFlag_Pragma       = 0x1000,
	SymbolNodeFlag_Start        = 0x2000,
	SymbolNodeFlag_Nullable     = 0x4000,
	SymbolNodeFlag_ResolverUsed = 0x8000,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class SymbolNode: public GrammarNode
{
public:
	int m_charToken;

	GrammarNode* m_synchronizer;
	SymbolNode* m_resolver;
	size_t m_resolverPriority;
	size_t m_lookaheadLimit;

	sl::Array<GrammarNode*> m_productionArray;

	sl::StringRef m_valueBlock;
	lex::LineCol m_valueLineCol;

	sl::StringRef m_paramBlock;
	lex::LineCol m_paramLineCol;
	sl::BoxList<sl::StringRef> m_paramNameList;
	sl::StringHashTable<bool> m_paramNameSet;

	sl::StringRef m_localBlock;
	lex::LineCol m_localLineCol;
	sl::BoxList<sl::StringRef> m_localNameList;
	sl::StringHashTable<bool> m_localNameSet;

	sl::StringRef m_enterBlock;
	lex::LineCol m_enterLineCol;
	size_t m_enterIndex;

	sl::StringRef m_leaveBlock;
	lex::LineCol m_leaveLineCol;
	size_t m_leaveIndex;

public:
	SymbolNode();

	void
	addProduction(GrammarNode* node);

	virtual
	bool
	markReachable();

	virtual
	bool
	markWeaklyReachable();

	virtual
	void
	trace();

	virtual
	void
	luaExport(lua::LuaState* luaState);

	virtual
	sl::String
	getBnfString();

	virtual
	bool
	propagateGrammarProps();
};

//..............................................................................

class SequenceNode: public GrammarNode
{
public:
	sl::Array<GrammarNode*> m_sequence;

public:
	SequenceNode()
	{
		m_nodeKind = NodeKind_Sequence;
	}

	void
	append(GrammarNode* node);

	virtual
	bool
	markReachable();

	virtual
	bool
	markWeaklyReachable();

	virtual
	void
	trace();

	virtual
	void
	luaExport(lua::LuaState* luaState);

	virtual
	sl::String
	getProductionString();

	virtual
	sl::String
	getBnfString();

	virtual
	bool
	propagateGrammarProps();
};

//..............................................................................

enum UserNodeFlag
{
	UserNodeFlag_UserCodeProcessed = 0x010000, // prevent double processing in '+' quantifier
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class UserNode: public GrammarNode
{
public:
	SymbolNode* m_productionSymbol;
	DispatcherNode* m_dispatcher;

	UserNode();
};

//..............................................................................

class ActionNode: public UserNode
{
public:
	sl::String m_userCode;

public:
	ActionNode()
	{
		m_nodeKind = NodeKind_Action;
	}

	virtual
	void
	trace();

	virtual
	void
	luaExport(lua::LuaState* luaState);

	virtual
	sl::String
	getBnfString()
	{
		return sl::String();
	}
};

//..............................................................................

class ArgumentNode: public UserNode
{
public:
	SymbolNode* m_targetSymbol;
	sl::BoxList<sl::String> m_argValueList;

public:
	ArgumentNode();

	virtual
	void
	trace();

	virtual
	void
	luaExport(lua::LuaState* luaState);

	virtual
	sl::String
	getBnfString()
	{
		return sl::String();
	}
};

//..............................................................................

enum BeaconNodeFlag
{
	BeaconNodeFlag_Added   = 0x0100,
	BeaconNodeFlag_Deleted = 0x0200,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class BeaconNode: public GrammarNode
{
public:
	sl::StringRef m_label;
	size_t m_slotIndex;
	SymbolNode* m_target;
	ArgumentNode* m_argument;

public:
	BeaconNode();

	virtual
	bool
	markReachable();

	virtual
	bool
	markWeaklyReachable();

	virtual
	void
	trace();

	virtual
	void
	luaExport(lua::LuaState* luaState);

	virtual
	sl::String
	getBnfString()
	{
		return m_target ? m_target->getBnfString() : m_name;
	}

	virtual
	bool
	propagateGrammarProps();
};

//..............................................................................

class DispatcherNode: public Node
{
public:
	SymbolNode* m_symbol;
	sl::Array<BeaconNode*> m_beaconArray;

public:
	DispatcherNode();

	virtual
	void
	trace();

	virtual
	void
	luaExport(lua::LuaState* luaState);
};

//..............................................................................

class ConflictNode: public Node
{
public:
	SymbolNode* m_symbol;
	SymbolNode* m_token;
	Node* m_resultNode; // lookahead DFA or immediate production
	sl::Array<GrammarNode*> m_productionArray;

public:
	ConflictNode();

	virtual
	void
	trace();

	virtual
	void
	luaExport(lua::LuaState* luaState)
	{
		ASSERT(false); // all the conflicts should be resolved
	}

	void
	pushError()
	{
		err::pushFormatStringError(
			"conflict at '%s':'%s'",
			m_symbol->m_name.sz(),
			m_token->m_name.sz()
			);
	}
};

//..............................................................................

enum LaDfaNodeFlag
{
	LaDfaNodeFlag_Leaf     = 0x100,
	LaDfaNodeFlag_Resolved = 0x200,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class LaDfaNode: public Node
{
public:
	SymbolNode* m_token;
	GrammarNode* m_resolver;
	Node* m_resolverElse;
	LaDfaNode* m_resolverUplink;
	Node* m_production;

	sl::Array<LaDfaNode*> m_transitionArray;

public:
	LaDfaNode();

	virtual
	void
	trace();

	virtual
	void
	luaExport(lua::LuaState* luaState);

protected:
	void
	luaExportResolverMembers(lua::LuaState* luaState);
};

//..............................................................................

template <typename T>
void
traceNodeList(
	const sl::StringRef& name,
	sl::Iterator<T> nodeIt
	)
{
	printf("%s\n", name.sz());

	for (; nodeIt; nodeIt++)
	{
		T* node = *nodeIt;

		printf("%3d/%-3d\t", node->m_index, node->m_masterIndex);
		node->trace();
	}
}

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

template <typename T>
void
traceNodeArray(
	const sl::StringRef& name,
	const sl::Array<T*>* array
	)
{
	printf("%s\n", name.sz());

	size_t count = array->getCount();
	for (size_t i = 0; i < count; i++)
	{
		T* node = (*array) [i];

		printf("%3d/%-3d ", node->m_index, node->m_masterIndex);
		node->trace();
	}
}

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

template <typename T>
sl::String
nodeArrayToString(const sl::Array<T*>* array)
{
	size_t count = array->getCount();
	if (!count)
		return sl::String();

	sl::String string = (*array) [0]->m_name;

	for (size_t i = 1; i < count; i++)
	{
		Node* node = (*array) [i];
		string += ' ';
		string += node->m_name;
	}

	return string;
}

//..............................................................................
