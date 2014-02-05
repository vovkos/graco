// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#include "llkc_Node.h"

//.............................................................................

class CNodeMgr
{
	friend class CModule;
	friend class CParser;
	friend class CLaDfaBuilder;
	friend class CParseTableBuilder;

protected:
	rtl::CHashTableMapT <int, CSymbolNode*, rtl::CHashIdT <int>, rtl::CCmpT <int> > m_TokenMap;
	rtl::CStringHashTableMapT <CSymbolNode*> m_SymbolMap;

	CGrammarNode m_EpsilonNode;
	CSymbolNode m_EofTokenNode;
	CSymbolNode m_AnyTokenNode;
	CSymbolNode m_StartPragmaSymbol;
	CSymbolNode* m_pPrimaryStartSymbol;

	rtl::CStdListT <CSymbolNode> m_CharTokenList;
	rtl::CStdListT <CSymbolNode> m_NamedTokenList;
	rtl::CStdListT <CSymbolNode> m_NamedSymbolList;
	rtl::CStdListT <CSymbolNode> m_TempSymbolList;
	rtl::CStdListT <CSequenceNode> m_SequenceList;
	rtl::CStdListT <CBeaconNode> m_BeaconList;
	rtl::CStdListT <CDispatcherNode> m_DispatcherList;
	rtl::CStdListT <CActionNode> m_ActionList;
	rtl::CStdListT <CArgumentNode> m_ArgumentList;
	rtl::CStdListT <CConflictNode> m_ConflictList;
	rtl::CStdListT <CLaDfaNode> m_LaDfaList;

	rtl::CArrayT <CSymbolNode*> m_TokenArray;  // char tokens + named tokens
	rtl::CArrayT <CSymbolNode*> m_SymbolArray; // named symbols + temp symbols

	size_t m_MasterCount;

public:
	CNodeMgr ();

	bool
	IsEmpty ()
	{
		return m_NamedSymbolList.IsEmpty ();
	}

	void
	Clear ();

	void
	Export (lua::CLuaState* pLuaState);

	CSymbolNode*
	GetTokenNode (int Token);

	CSymbolNode*
	GetSymbolNode (const rtl::CString& Name);

	CSymbolNode*
	CreateTempSymbolNode ();

	CSequenceNode*
	CreateSequenceNode ();

	CSequenceNode*
	CreateSequenceNode (CGrammarNode* pNode);

	CBeaconNode*
	CreateBeaconNode (CSymbolNode* pTarget);

	void
	DeleteBeaconNode (CBeaconNode* pNode)
	{
		m_BeaconList.Delete (pNode);
	}

	void
	DeleteLaDfaNode (CLaDfaNode* pNode)
	{
		m_LaDfaList.Delete (pNode);
	}

	CDispatcherNode*
	CreateDispatcherNode (CSymbolNode* pSymbol);

	CActionNode*
	CreateActionNode ();

	CArgumentNode*
	CreateArgumentNode ();

	CConflictNode*
	CreateConflictNode ();

	CLaDfaNode*
	CreateLaDfaNode ();

	CGrammarNode*
	CreateQuantifierNode (
		CGrammarNode* pNode,
		int Kind
		);

	void
	Trace ();

	void
	Export ();

	void
	MarkReachableNodes ();

	void
	DeleteUnreachableNodes ();

	void 
	IndexTokens ();

	void 
	IndexSymbols ();

	void 
	IndexSequences ();

	void 
	IndexBeacons ();

	void 
	IndexDispatchers ();

	void 
	IndexActions ();

	void 
	IndexArguments ();

	void 
	IndexLaDfaNodes ();

protected:
	void
	ExportTokenTable (lua::CLuaState* pLuaState);

	void
	ExportSymbolTable (lua::CLuaState* pLuaState);

	void
	ExportSequenceTable (lua::CLuaState* pLuaState);

	void
	ExportLaDfaTable (lua::CLuaState* pLuaState);

	void
	ExportNodeArray (
		lua::CLuaState* pLuaState,
		const char* pName,
		CNode* const* ppNode,
		size_t Count
		);

	void
	ExportNodeList (
		lua::CLuaState* pLuaState,
		const char* pName,
		rtl::CIteratorT <CNode> Node,
		size_t CountHint = 1
		);
};


//.............................................................................
