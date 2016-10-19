// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#include "NodeMgr.h"
#include "CmdLine.h"

class LaDfaState;

//..............................................................................

enum LaDfaThreadMatchKind
{
	LaDfaThreadMatchKind_None,
	LaDfaThreadMatchKind_Token,
	LaDfaThreadMatchKind_AnyToken,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class LaDfaThread: public sl::ListLink
{
public:
	LaDfaThreadMatchKind m_match;
	LaDfaState* m_state;
	GrammarNode* m_resolver;
	size_t m_resolverPriority;
	Node* m_production;
	sl::Array <Node*> m_stack;

public:
	LaDfaThread ();
};

//..............................................................................

enum LaDfaStateFlag
{
	LaDfaStateFlag_TokenMatch        = 1,
	LaDfaStateFlag_EpsilonProduction = 2,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class LaDfaState: public sl::ListLink
{
public:
	size_t m_index;
	int m_flags;

	sl::StdList <LaDfaThread> m_activeThreadList;
	sl::StdList <LaDfaThread> m_resolverThreadList;
	sl::StdList <LaDfaThread> m_completeThreadList;
	sl::StdList <LaDfaThread> m_epsilonThreadList;

	LaDfaState* m_fromState;
	SymbolNode* m_token;
	sl::Array <LaDfaState*> m_transitionArray;
	LaDfaNode* m_dfaNode;

public:
	LaDfaState ();

	bool
	isResolved ()
	{
		return (m_dfaNode->m_flags & LaDfaNodeFlag_Resolved) != 0;
	}

	bool
	isAnyTokenIgnored ()
	{
		return (m_flags & LaDfaStateFlag_TokenMatch) || (m_flags & LaDfaStateFlag_EpsilonProduction);
	}

	bool
	isEmpty ()
	{
		return
			m_activeThreadList.isEmpty () &&
			m_resolverThreadList.isEmpty () &&
			m_completeThreadList.isEmpty () &&
			m_epsilonThreadList.isEmpty ();
	}

	bool
	calcResolved ();

	LaDfaThread*
	createThread (LaDfaThread* src = NULL);

	Node*
	getResolvedProduction ();

	Node*
	getDefaultProduction ();
};

//..............................................................................

class LaDfaBuilder
{
protected:
	sl::StdList <LaDfaState> m_stateList;
	NodeMgr* m_nodeMgr;
	sl::Array <Node*>* m_parseTable;
	size_t m_lookeaheadLimit;
	size_t m_lookeahead;

public:
	LaDfaBuilder (
		NodeMgr* nodeMgr,
		sl::Array <Node*>* parseTable,
		size_t lookeaheadLimit = 2
		);

	Node*
	build (
		CmdLine* cmdLine,
		ConflictNode* conflict,
		size_t* lookahead = NULL
		); // returns DFA or immediate production

	void
	trace ();

	size_t
	getLookahead ()
	{
		return m_lookeahead;
	}

protected:
	LaDfaState*
	createState ();

	LaDfaState*
	transition (
		LaDfaState* state,
		SymbolNode* token
		);

	void
	processThread (LaDfaThread* thread);
};

//..............................................................................
