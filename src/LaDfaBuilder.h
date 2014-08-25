// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#include "NodeMgr.h"
#include "CmdLine.h"

class CLaDfaState;

//.............................................................................

enum ELaDfaThreadMatch
{
	ELaDfaThreadMatch_None,
	ELaDfaThreadMatch_Token,
	ELaDfaThreadMatch_AnyToken,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class CLaDfaThread: public rtl::TListLink
{
public:
	ELaDfaThreadMatch m_Match;
	CLaDfaState* m_pState;
	CGrammarNode* m_pResolver;
	size_t m_ResolverPriority;
	CNode* m_pProduction;
	rtl::CArrayT <CNode*> m_Stack;

public:
	CLaDfaThread ();
};

//.............................................................................

enum ELaDfaStateFlag
{
	ELaDfaStateFlag_TokenMatch        = 1,
	ELaDfaStateFlag_EpsilonProduction = 2, 
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class CLaDfaState: public rtl::TListLink
{
public:
	size_t m_Index;
	int m_Flags;

	rtl::CStdListT <CLaDfaThread> m_ActiveThreadList;
	rtl::CStdListT <CLaDfaThread> m_ResolverThreadList;
	rtl::CStdListT <CLaDfaThread> m_CompleteThreadList;
	rtl::CStdListT <CLaDfaThread> m_EpsilonThreadList;

	CLaDfaState* m_pFromState;
	CSymbolNode* m_pToken;
	rtl::CArrayT <CLaDfaState*> m_TransitionArray;
	CLaDfaNode* m_pDfaNode;

public:
	CLaDfaState ();

	bool
	IsResolved ()
	{
		return (m_pDfaNode->m_Flags & ELaDfaNodeFlag_Resolved) != 0;
	}

	bool
	IsAnyTokenIgnored ()
	{
		return (m_Flags & ELaDfaStateFlag_TokenMatch) || (m_Flags & ELaDfaStateFlag_EpsilonProduction);
	}

	bool
	IsEmpty ()
	{
		return 
			m_ActiveThreadList.IsEmpty () &&
			m_ResolverThreadList.IsEmpty () &&
			m_CompleteThreadList.IsEmpty () &&
			m_EpsilonThreadList.IsEmpty ();
	}

	bool
	CalcResolved ();

	CLaDfaThread*
	CreateThread (CLaDfaThread* pSrc = NULL);

	CNode* 
	GetResolvedProduction ();

	CNode* 
	GetDefaultProduction ();
};

//.............................................................................

class CLaDfaBuilder
{
protected:
	rtl::CStdListT <CLaDfaState> m_StateList;
	CNodeMgr* m_pNodeMgr;
	rtl::CArrayT <CNode*>* m_pParseTable;
	size_t m_LookeaheadLimit;
	size_t m_Lookeahead;

public:
	CLaDfaBuilder (	
		CNodeMgr* pNodeMgr,
		rtl::CArrayT <CNode*>* pParseTable,
		size_t LookeaheadLimit = 2
		);

	CNode*
	Build (
		TCmdLine* pCmdLine,
		CConflictNode* pConflict,
		size_t* pLookahead = NULL
		); // returns DFA or immediate production

	void
	Trace ();

	size_t
	GetLookahead ()
	{
		return m_Lookeahead;
	}

protected:
	CLaDfaState* 
	CreateState ();

	CLaDfaState*
	Transition (
		CLaDfaState* pState,
		CSymbolNode* pToken
		);

	void
	ProcessThread (CLaDfaThread* pThread);
};

//.............................................................................
