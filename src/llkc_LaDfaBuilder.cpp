#include "pch.h"
#include "llkc_LaDfaBuilder.h"

//.............................................................................

CLaDfaThread::CLaDfaThread ()
{
	m_Match = ELaDfaThreadMatch_None;
	m_pState = NULL;
	m_pProduction = NULL;
	m_pResolver = NULL;
	m_ResolverPriority = 0;
}

//.............................................................................

CLaDfaState::CLaDfaState ()
{
	m_Index = -1;
	m_Flags = 0;
	m_pDfaNode = NULL;
	m_pFromState = NULL;
	m_pToken = NULL;
}

CLaDfaThread*
CLaDfaState::CreateThread (CLaDfaThread* pSrc)
{
	CLaDfaThread* pThread = AXL_MEM_NEW (CLaDfaThread);
	pThread->m_pState = this;

	if (pSrc)
	{
		ASSERT (!pSrc->m_pResolver);

		pThread->m_pProduction = pSrc->m_pProduction;
		pThread->m_Stack = pSrc->m_Stack;
	}

	m_ActiveThreadList.InsertTail (pThread);
	return pThread;
}

bool
CLaDfaState::CalcResolved ()
{
	rtl::CIteratorT <CLaDfaThread> Thread;

	if (m_ActiveThreadList.IsEmpty ())
	{
		m_pDfaNode->m_Flags |= ELaDfaNodeFlag_Resolved;
		return true;
	}

	Thread = m_ActiveThreadList.GetHead ();

	CNode* pOriginalProduction = Thread->m_pProduction;

	for (Thread++; Thread; Thread++)
	{
		if (Thread->m_pProduction != pOriginalProduction)
			return false;
	}

	Thread = m_CompleteThreadList.GetHead ();
	for (; Thread; Thread++)
	{
		if (Thread->m_pProduction != pOriginalProduction)
			return false;
	}

	m_pDfaNode->m_Flags |= ELaDfaNodeFlag_Resolved;
	return true;
}

CNode*
CLaDfaState::GetResolvedProduction ()
{
	CLaDfaThread* pActiveThread = *m_ActiveThreadList.GetHead ();
	CLaDfaThread* pCompleteThread = *m_CompleteThreadList.GetHead ();
	CLaDfaThread* pEpsilonThread = *m_EpsilonThreadList.GetHead ();

	if (IsAnyTokenIgnored ())
		return
			pActiveThread && pActiveThread->m_Match != ELaDfaThreadMatch_AnyToken ? pActiveThread->m_pProduction :
			pCompleteThread && pCompleteThread->m_Match != ELaDfaThreadMatch_AnyToken ? pCompleteThread->m_pProduction :
			pEpsilonThread ? pEpsilonThread->m_pProduction : NULL;
	else
		return
			pActiveThread ? pActiveThread->m_pProduction :
			pCompleteThread ? pCompleteThread->m_pProduction :
			pEpsilonThread ? pEpsilonThread->m_pProduction : NULL;

}

CNode*
CLaDfaState::GetDefaultProduction ()
{
	CLaDfaThread* pCompleteThread = *m_CompleteThreadList.GetHead ();
	CLaDfaThread* pEpsilonThread = *m_EpsilonThreadList.GetHead ();

	return
		pCompleteThread ? pCompleteThread->m_pProduction :
		pEpsilonThread ? pEpsilonThread->m_pProduction :
		m_pFromState ? m_pFromState->GetDefaultProduction () : NULL;
}

//.............................................................................

CLaDfaBuilder::CLaDfaBuilder (
	CNodeMgr* pNodeMgr,
	rtl::CArrayT <CNode*>* pParseTable,
	size_t LookeaheadLimit
	)
{
	m_pNodeMgr = pNodeMgr;
	m_pParseTable = pParseTable;
	m_LookeaheadLimit = LookeaheadLimit;
	m_Lookeahead = 1;
}

static
int
CmpResolverThreadPriority (
	const void* p1,
	const void* p2
	)
{
	CLaDfaThread* pThread1 = *(CLaDfaThread**) p1;
	CLaDfaThread* pThread2 = *(CLaDfaThread**) p2;

	// sort from highest priority to lowest

	return
		pThread1->m_ResolverPriority < pThread2->m_ResolverPriority ? 1 :
		pThread1->m_ResolverPriority > pThread2->m_ResolverPriority ? -1 : 0;
}

CNode*
CLaDfaBuilder::Build (
	CConfig* pConfig,
	CConflictNode* pConflict,
	size_t* pLookahead
	)
{
	ASSERT (pConflict->m_Kind == ENode_Conflict);

	size_t TokenCount = m_pNodeMgr->m_TokenArray.GetCount ();

	m_StateList.Clear ();

	CLaDfaState* pState0 = CreateState ();
	pState0->m_pDfaNode = m_pNodeMgr->CreateLaDfaNode ();

	size_t Count = pConflict->m_ProductionArray.GetCount ();
	for (size_t i = 0; i < Count; i++)
	{
		CNode* pProduction = pConflict->m_ProductionArray [i];
		CLaDfaThread* pThread = pState0->CreateThread ();
		pThread->m_pProduction = pProduction;

		if (pProduction->m_Kind == ENode_Symbol)
		{
			CSymbolNode* pSymbolNode = (CSymbolNode*) pProduction;
			if (pSymbolNode->m_pResolver)
			{
				ASSERT (pSymbolNode->m_ProductionArray.GetCount () == 1);
				pThread->m_pProduction = pSymbolNode->m_ProductionArray [0]; // adjust root production
			}
		}

		if (pProduction->m_Kind != ENode_Epsilon)
			pThread->m_Stack.Append (pProduction);
		else
			pState0->m_Flags |= ELaDfaStateFlag_EpsilonProduction;
	}

	CLaDfaState* pState1 = Transition (pState0, pConflict->m_pToken);

	size_t Lookahead = 1;

	if (!pState1->IsResolved ())
	{
		rtl::CArrayT <CLaDfaState*> StateArray;
		StateArray.Append (pState1);

		while (!StateArray.IsEmpty () && Lookahead < m_LookeaheadLimit)
		{
			Lookahead++;

			rtl::CArrayT <CLaDfaState*> NextStateArray;

			size_t StateCount = StateArray.GetCount ();
			for (size_t j = 0; j < StateCount; j++)
			{
				CLaDfaState* pState = StateArray [j];

				for (size_t k = 0; k < TokenCount; k++)
				{
					CSymbolNode* pToken = m_pNodeMgr->m_TokenArray [k];

					CLaDfaState* pNewState = Transition (pState, pToken);
					if (pNewState && !pNewState->IsResolved ())
						NextStateArray.Append (pNewState);
				}
			}

			StateArray = NextStateArray;
		}

		if (!StateArray.IsEmpty ())
		{
			size_t Count = StateArray.GetCount ();
			CLaDfaState* pState = StateArray [0];
			rtl::CBoxListT <rtl::CString> TokenNameList;

			for (; pState != pState0; pState = pState->m_pFromState)
				TokenNameList.InsertHead (pState->m_pToken->m_Name);

			rtl::CString TokenSeqString;
			rtl::CBoxIteratorT <rtl::CString> TokenName = TokenNameList.GetHead ();
			for (; TokenName; TokenName++)
			{
				TokenSeqString.Append (*TokenName);
				TokenSeqString.Append (' ');
			}

			err::SetFormatStringError (
				"conflict at %s:%s could not be resolved with %d token lookahead; e.g. %s",
				pConflict->m_pSymbol->m_Name.cc (), // thanks a lot gcc
				pConflict->m_pToken->m_Name.cc (),
				m_LookeaheadLimit,
				TokenSeqString.cc ()
				);

			err::PushSrcPosError (pConflict->m_pSymbol->m_SrcPos);
			return NULL;
		}
	}

	if (Lookahead > m_Lookeahead)
		m_Lookeahead = Lookahead;

	if (pLookahead)
		*pLookahead = Lookahead;

	rtl::CIteratorT <CLaDfaState> State = m_StateList.GetHead ();
	for (; State; State++)
	{
		CLaDfaState* pState = *State;

		if (pState->m_CompleteThreadList.GetCount () > 1 ||
			pState->m_EpsilonThreadList.GetCount () > 1)
		{
			err::SetFormatStringError (
				"conflict at %s:%s: multiple productions complete with %s",
				pConflict->m_pSymbol->m_Name.cc (),
				pConflict->m_pToken->m_Name.cc (),
				pState->m_pToken->m_Name.cc ()
				);
			err::PushSrcPosError (pConflict->m_pSymbol->m_SrcPos);
			return NULL;
		}

		if (!pState->m_ResolverThreadList.IsEmpty ()) // chain all resolvers
		{
			size_t Count = pState->m_ResolverThreadList.GetCount ();

			rtl::CArrayT <CLaDfaThread*> ResolverThreadArray;
			ResolverThreadArray.SetCount (Count);

			rtl::CIteratorT <CLaDfaThread> ResolverThread = pState->m_ResolverThreadList.GetHead ();
			for (size_t i = 0; ResolverThread; ResolverThread++, i++)
				ResolverThreadArray [i] = *ResolverThread;

			qsort (ResolverThreadArray, Count, sizeof (CLaDfaThread*), CmpResolverThreadPriority);

			for (size_t i = 0; i < Count; i++)
			{
				CLaDfaThread* pResolverThread = ResolverThreadArray [i];

				CLaDfaNode* pDfaElse = m_pNodeMgr->CreateLaDfaNode ();
				pDfaElse->m_Flags = pState->m_pDfaNode->m_Flags;
				pDfaElse->m_TransitionArray = pState->m_pDfaNode->m_TransitionArray;

				pState->m_pDfaNode->m_pResolver = pResolverThread->m_pResolver;
				pState->m_pDfaNode->m_pProduction = pResolverThread->m_pProduction;
				pState->m_pDfaNode->m_pResolverElse = pDfaElse;
				pState->m_pDfaNode->m_TransitionArray.Clear ();

				pDfaElse->m_pResolverUplink = pState->m_pDfaNode;
				pState->m_pDfaNode = pDfaElse;
			}
		}

		ASSERT (!pState->m_pDfaNode->m_pResolver);

		if (pState->IsResolved ())
		{
			pState->m_pDfaNode->m_Flags |= ELaDfaNodeFlag_Leaf;
			pState->m_pDfaNode->m_pProduction = pState->GetResolvedProduction ();

			if (pState->m_pDfaNode->m_pResolverUplink)
			{
				CLaDfaNode* pUplink = pState->m_pDfaNode->m_pResolverUplink;

				if (!pState->m_pDfaNode->m_pProduction ||
					pState->m_pDfaNode->m_pProduction == pUplink->m_pProduction)
				{
					// here we handle situation like
					// 1) sym: resolver ({1}) 'a' | resolver ({2}) 'b' (we have a chain of 2 resolver with empty tail)
					// or
					// 2) both resolver 'then' and 'else' branch point to the same production (this happens when resolver applies not to the original conflict)
					// in either case we we can safely eliminate the resolver {2}

					pUplink->m_Flags |= ELaDfaNodeFlag_Leaf;
					pUplink->m_pResolver = NULL;
					pUplink->m_pResolverElse = NULL;

					m_pNodeMgr->DeleteLaDfaNode (pState->m_pDfaNode);
					pState->m_pDfaNode = pUplink;
				}
			}
		}
		else
		{
			pState->m_pDfaNode->m_pProduction = pState->GetDefaultProduction ();
		}
	}

	if (pConfig->m_Flags & EConfigFlag_Verbose)
		Trace ();

	if (pState1->m_ResolverThreadList.IsEmpty () &&
		(pState1->m_pDfaNode->m_Flags & ELaDfaNodeFlag_Leaf))
	{
		// can happen on active-vs-complete-vs-epsion conflicts

		pState0->m_pDfaNode->m_Flags |= ELaDfaNodeFlag_Leaf; // don't index state0
		return pState1->m_pDfaNode->m_pProduction;
	}

	return pState0->m_pDfaNode;
}

void
CLaDfaBuilder::Trace ()
{
	rtl::CIteratorT <CLaDfaState> State = m_StateList.GetHead ();
	for (; State; State++)
	{
		CLaDfaState* pState = *State;

		printf (
			"%3d %s %d/%d/%d/%d (a/r/c/e)\n",
			pState->m_Index,
			pState->IsResolved () ? "*" : " ",
			pState->m_ActiveThreadList.GetCount (),
			pState->m_ResolverThreadList.GetCount (),
			pState->m_CompleteThreadList.GetCount (),
			pState->m_EpsilonThreadList.GetCount ()
			);

		rtl::CIteratorT <CLaDfaThread> Thread;

		if (!pState->m_ActiveThreadList.IsEmpty ())
		{
			printf ("\tACTIVE:   ");

			Thread = pState->m_ActiveThreadList.GetHead ();
			for (; Thread; Thread++)
				printf ("%s ", Thread->m_pProduction->m_Name.cc ());

			printf ("\n");
		}

		if (!pState->m_ResolverThreadList.IsEmpty ())
		{
			printf ("\tRESOLVER: ");

			Thread = pState->m_ResolverThreadList.GetHead ();
			for (; Thread; Thread++)
				printf ("%s ", Thread->m_pProduction->m_Name.cc ());

			printf ("\n");
		}

		if (!pState->m_CompleteThreadList.IsEmpty ())
		{
			printf ("\tCOMPLETE: ");

			Thread = pState->m_CompleteThreadList.GetHead ();
			for (; Thread; Thread++)
				printf ("%s ", Thread->m_pProduction->m_Name.cc ());

			printf ("\n");
		}

		if (!pState->m_EpsilonThreadList.IsEmpty ())
		{
			printf ("\tEPSILON: ");

			Thread = pState->m_EpsilonThreadList.GetHead ();
			for (; Thread; Thread++)
				printf ("%s ", Thread->m_pProduction->m_Name.cc ());

			printf ("\n");
		}

		if (!pState->IsResolved ())
		{
			size_t MoveCount = pState->m_TransitionArray.GetCount ();
			for (size_t i = 0; i < MoveCount; i++)
			{
				CLaDfaState* pMoveTo = pState->m_TransitionArray [i];
				printf (
					"\t%s -> %d\n",
					pMoveTo->m_pToken->m_Name.cc (),
					pMoveTo->m_Index
					);
			}
		}
	}
}

CLaDfaState*
CLaDfaBuilder::CreateState ()
{
	CLaDfaState* pState = AXL_MEM_NEW (CLaDfaState);
	pState->m_Index = m_StateList.GetCount ();
	m_StateList.InsertTail (pState);

	return pState;
}

CLaDfaState*
CLaDfaBuilder::Transition (
	CLaDfaState* pState,
	CSymbolNode* pToken
	)
{
	CLaDfaState* pNewState = CreateState ();
	pNewState->m_pToken = pToken;
	pNewState->m_pFromState = pState;
	pNewState->m_Flags = pState->m_Flags & ELaDfaStateFlag_EpsilonProduction; // propagate epsilon

	rtl::CIteratorT <CLaDfaThread> Thread = pState->m_ActiveThreadList.GetHead ();
	for (; Thread; Thread++)
	{
		CLaDfaThread* pNewThread = pNewState->CreateThread (*Thread);
		ProcessThread (pNewThread);
	}

	Thread = pNewState->m_ActiveThreadList.GetHead ();
	while (Thread)
	{
		CLaDfaThread* pThread = *Thread++;

		if (pThread->m_Match == ELaDfaThreadMatch_AnyToken && pNewState->IsAnyTokenIgnored ())
		{
			pNewState->m_ActiveThreadList.Delete (pThread); // delete anytoken thread in favor of concrete token
		}
		else if (pThread->m_Stack.IsEmpty ())
		{
			pNewState->m_ActiveThreadList.Remove (pThread);

			if (pThread->m_Match)
				pNewState->m_CompleteThreadList.InsertTail (pThread);
			else
				pNewState->m_EpsilonThreadList.InsertTail (pThread);
		}
	}

	if (pNewState->IsEmpty ())
	{
		m_StateList.Delete (pNewState);
		return NULL;
	}

	pNewState->m_pDfaNode = m_pNodeMgr->CreateLaDfaNode ();
	pNewState->m_pDfaNode->m_pToken = pToken;
	pNewState->CalcResolved ();

	pState->m_pDfaNode->m_TransitionArray.Append (pNewState->m_pDfaNode);
	pState->m_TransitionArray.Append (pNewState);
	return pNewState;
}

void
CLaDfaBuilder::ProcessThread (CLaDfaThread* pThread)
{
	CSymbolNode* pToken = pThread->m_pState->m_pToken;

	pThread->m_Match = ELaDfaThreadMatch_None;

	size_t TokenCount = m_pNodeMgr->m_TokenArray.GetCount ();
	for (;;)
	{
		if (pThread->m_Stack.IsEmpty ())
			break;

		CNode* pNode = pThread->m_Stack.GetBack ();
		CNode* pProduction;
		CSymbolNode* pSymbol;
		CConflictNode* pConflict;
		CSequenceNode* pSequence;
		size_t ChildrenCount;

		switch (pNode->m_Kind)
		{
		case ENode_Token:
			if (pThread->m_Match)
				return;

			ASSERT (pNode->m_MasterIndex);

			if ((pNode->m_Flags & ESymbolNodeFlag_AnyToken) && pToken->m_MasterIndex != 0) // EOF does not match ANY
			{
				pThread->m_Stack.Pop ();
				pThread->m_Match = ELaDfaThreadMatch_AnyToken;
				break;
			}

			if (pNode != pToken) // could happen after epsilon production
			{
				pThread->m_pState->m_ActiveThreadList.Delete (pThread);
				return;
			}

			pThread->m_Stack.Pop ();
			pThread->m_Match = ELaDfaThreadMatch_Token;
			pThread->m_pState->m_Flags |= ELaDfaStateFlag_TokenMatch;
			break;

		case ENode_Symbol:
			if (pThread->m_Match)
				return;

			pProduction = (*m_pParseTable) [pNode->m_Index * TokenCount + pToken->m_Index];
			if (!pProduction)  // could happen after epsilon production
			{
				pThread->m_pState->m_ActiveThreadList.Delete (pThread);
				return;
			}

			// ok this thread seems to stay active, let's check if we can eliminate it with resolver

			pSymbol = (CSymbolNode*) pNode;
			if (pSymbol->m_pResolver)
			{
				pThread->m_pResolver = pSymbol->m_pResolver;
				pThread->m_ResolverPriority = pSymbol->m_ResolverPriority;
				pThread->m_pState->m_ActiveThreadList.Remove (pThread);
				pThread->m_pState->m_ResolverThreadList.InsertTail (pThread);
				return;
			}

			pThread->m_Stack.Pop ();

			if (pProduction->m_Kind != ENode_Epsilon)
				pThread->m_Stack.Append (pProduction);
			else
				pThread->m_pState->m_Flags |= ELaDfaStateFlag_EpsilonProduction;

			break;

		case ENode_Sequence:
			if (pThread->m_Match)
				return;

			pThread->m_Stack.Pop ();

			pSequence = (CSequenceNode*) pNode;
			ChildrenCount = pSequence->m_Sequence.GetCount ();
			for (intptr_t i = ChildrenCount - 1; i >= 0; i--)
			{
				CNode* pChild = pSequence->m_Sequence [i];
				pThread->m_Stack.Append (pChild);
			}

			break;

		case ENode_Beacon:
			pThread->m_Stack.Pop ();
			pThread->m_Stack.Append (((CBeaconNode*) pNode)->m_pTarget);
			break;

		case ENode_Action:
		case ENode_Argument:
			pThread->m_Stack.Pop ();
			break;

		case ENode_Conflict:
			pThread->m_Stack.Pop ();

			pConflict = (CConflictNode*) pNode;
			ChildrenCount = pConflict->m_ProductionArray.GetCount ();
			for (size_t i = 0; i < ChildrenCount; i++)
			{
				CNode* pChild = pConflict->m_ProductionArray [i];
				CLaDfaThread* pNewThread = pThread->m_pState->CreateThread (pThread);

				if (pChild->m_Kind != ENode_Epsilon)
					pNewThread->m_Stack.Append (pChild);

				ProcessThread (pNewThread);
			}

			pThread->m_pState->m_ActiveThreadList.Delete (pThread);
			return;

		default:
			ASSERT (false);
		}
	}
}

//.............................................................................
