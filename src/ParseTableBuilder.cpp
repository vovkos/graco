#include "pch.h"
#include "ParseTableBuilder.h"

//.............................................................................

bool
CParseTableBuilder::Build ()
{
	CalcFirstFollow ();

	// build parse table

	size_t SymbolCount = m_pNodeMgr->m_SymbolArray.GetCount ();
	size_t TerminalCount = m_pNodeMgr->m_TokenArray.GetCount ();

	m_pParseTable->SetCount (SymbolCount * TerminalCount);

	// normal productions

	for (size_t i = 0; i < SymbolCount; i++)
	{
		CSymbolNode* pNode = m_pNodeMgr->m_SymbolArray [i];

		if (pNode->m_Flags & ESymbolNodeFlag_Named)
		{
			if (pNode->IsNullable () && !(pNode->m_Flags & ESymbolNodeFlag_Nullable))
			{
				err::SetFormatStringError (
					"'%s': nullable symbols must be explicitly marked as 'nullable'", 
					pNode->m_Name.cc () // thanks a lot gcc
					);
				err::PushSrcPosError (pNode->m_SrcPos);
				return false;
			}

			if (!pNode->IsNullable () && (pNode->m_Flags & ESymbolNodeFlag_Nullable))
			{
				err::SetFormatStringError (
					"'%s': marked as 'nullable' but is not nullable", 
					pNode->m_Name.cc () 
					);
				err::PushSrcPosError (pNode->m_SrcPos);
				return false;
			}
		}

		if (pNode->m_Flags & ESymbolNodeFlag_Pragma)
		{
			if (pNode->IsNullable ())
			{
				err::SetFormatStringError (
					"'%s': pragma cannot be nullable", 
					pNode->m_Name.cc () 
					);
				err::PushSrcPosError (pNode->m_SrcPos);
				return false;
			}

			if (pNode->m_FirstSet.GetBit (1))
			{
				err::SetFormatStringError (
					"'%s': pragma cannot start with 'anytoken'", 
					pNode->m_Name.cc () 
					);
				err::PushSrcPosError (pNode->m_SrcPos);
				return false;
			}
		}

		size_t ChildrenCount = pNode->m_ProductionArray.GetCount ();
		for (size_t j = 0; j < ChildrenCount; j++)
		{
			CGrammarNode* pProduction = pNode->m_ProductionArray [j];
			AddProductionToParseTable (pNode, pProduction); 
		}
	}

	// anytoken productions

	// if 
	//	production' FIRST contains anytoken 
	// or 
	//	production is nullable and symbol' FOLLOW contains anytoken 
	// then 
	//	set all parse table entries to this production

	for (size_t i = 0; i < SymbolCount; i++)
	{
		CSymbolNode* pNode = m_pNodeMgr->m_SymbolArray [i];

		size_t ChildrenCount = pNode->m_ProductionArray.GetCount ();
		for (size_t j = 0; j < ChildrenCount; j++)
		{
			CGrammarNode* pProduction = pNode->m_ProductionArray [j];
			
			if (pProduction->m_FirstSet.GetBit (1) || (pProduction->IsNullable () && pNode->m_FollowSet.GetBit (1)))
				AddAnyTokenProductionToParseTable (pNode, pProduction); 
		}
	}

	return true;
}

void
CParseTableBuilder::AddProductionToParseTable (
	CSymbolNode* pSymbol,
	CGrammarNode* pProduction
	)
{
	size_t Count;
	
	Count = pProduction->m_FirstArray.GetCount ();
	for (size_t i = 0; i < Count; i++)
	{
		CSymbolNode* pToken = pProduction->m_FirstArray [i];
		AddParseTableEntry (pSymbol, pToken, pProduction);
	}

	if (!pProduction->IsNullable ())
		return;

	Count = pSymbol->m_FollowArray.GetCount ();
	for (size_t i = 0; i < Count; i++)
	{
		CSymbolNode* pToken = pSymbol->m_FollowArray [i];
		AddParseTableEntry (pSymbol, pToken, pProduction);
	}

	if (pSymbol->IsFinal ())
		AddParseTableEntry (pSymbol, &m_pNodeMgr->m_EofTokenNode, pProduction);
}

void
CParseTableBuilder::AddAnyTokenProductionToParseTable (
	CSymbolNode* pSymbol,
	CGrammarNode* pProduction
	)
{
	size_t TokenCount = m_pNodeMgr->m_TokenArray.GetCount ();

	// skip EOF and ANYTOKEN

	for (size_t i = 2; i < TokenCount; i++)
	{
		CSymbolNode* pToken = m_pNodeMgr->m_TokenArray [i];
		AddParseTableEntry (pSymbol, pToken, pProduction);
	}
}

size_t
CParseTableBuilder::AddParseTableEntry (
	CSymbolNode* pSymbol,
	CSymbolNode* pToken,
	CGrammarNode* pProduction
	)
{
	size_t TokenCount = m_pNodeMgr->m_TokenArray.GetCount ();
		
	CNode** ppProduction = *m_pParseTable + pSymbol->m_Index * TokenCount + pToken->m_Index;
	CNode* pOldProduction = *ppProduction;

	if (!pOldProduction)
	{
		*ppProduction = pProduction;
		return 0;
	}
	
	if (pOldProduction == pProduction)
		return 0;

	CConflictNode* pConflict;
	if (pOldProduction->m_Kind != ENode_Conflict)
	{
		pConflict = m_pNodeMgr->CreateConflictNode ();
		pConflict->m_pSymbol = pSymbol;
		pConflict->m_pToken = pToken;

		pConflict->m_ProductionArray.SetCount (2);
		pConflict->m_ProductionArray [0] = (CGrammarNode*) pOldProduction; 
		pConflict->m_ProductionArray [1] = pProduction;
	
		*ppProduction = pConflict; // later will be replaced with lookahead DFA
	}
	else
	{
		pConflict = (CConflictNode*) pOldProduction;
		size_t Count = pConflict->m_ProductionArray.GetCount ();
		
		size_t i = 0;
		for (; i < Count; i++)
			if (pConflict->m_ProductionArray [i] == pProduction)
				break;

		if (i >= Count) // not found
			pConflict->m_ProductionArray.Append (pProduction);
	}

	return pConflict->m_ProductionArray.GetCount ();
}

bool
PropagateParentChild (
	CGrammarNode* pParent,
	CGrammarNode* pChild
	)
{
	bool HasChanged = false;

	if (pParent->m_FirstSet.Merge (pChild->m_FirstSet, rtl::EBitOp_Or))
		HasChanged = true;			

	if (pChild->m_FollowSet.Merge (pParent->m_FollowSet, rtl::EBitOp_Or))
		HasChanged = true;			

	if (pChild->IsNullable ())
		if (pParent->MarkNullable ())
			HasChanged = true;

	if (pParent->IsFinal ())
		if (pChild->MarkFinal ())
			HasChanged = true;

	return HasChanged;
}

void
CParseTableBuilder::CalcFirstFollow ()
{
	bool HasChanged;

	CGrammarNode* pStartSymbol = m_pNodeMgr->m_SymbolArray [0];
	pStartSymbol->MarkFinal ();
	
	size_t TokenCount = m_pNodeMgr->m_TokenArray.GetCount ();
	size_t SymbolCount = m_pNodeMgr->m_SymbolArray.GetCount ();

	for (size_t i = 1; i < TokenCount; i++) 
	{
		CSymbolNode* pNode = m_pNodeMgr->m_TokenArray [i];
		pNode->m_FirstSet.SetBitResize (pNode->m_MasterIndex, true);
	}

	for (size_t i = 0; i < SymbolCount; i++)
	{
		CSymbolNode* pNode = m_pNodeMgr->m_SymbolArray [i];
		pNode->m_FirstSet.SetBitCount (TokenCount);
		pNode->m_FollowSet.SetBitCount (TokenCount);

		if (pNode->m_pResolver)
			pNode->m_pResolver->m_FollowSet.SetBitResize (1); // set anytoken FOLLOW for resolver

		if (pNode->m_Flags & ESymbolNodeFlag_Start)
			pNode->MarkFinal ();
	}

	rtl::CIteratorT <CSequenceNode> Sequence = m_pNodeMgr->m_SequenceList.GetHead ();
	for (; Sequence; Sequence++)
	{
		Sequence->m_FirstSet.SetBitCount (TokenCount);
		Sequence->m_FollowSet.SetBitCount (TokenCount);
	}

	rtl::CIteratorT <CBeaconNode> Beacon = m_pNodeMgr->m_BeaconList.GetHead ();
	for (; Beacon; Beacon++)
	{
		Beacon->m_FirstSet.SetBitCount (TokenCount);
		Beacon->m_FollowSet.SetBitCount (TokenCount);
	}

	do 
	{
		HasChanged = false;

		for (size_t i = 0; i < SymbolCount; i++)
		{
			CSymbolNode* pNode = m_pNodeMgr->m_SymbolArray [i];
			size_t ChildrenCount = pNode->m_ProductionArray.GetCount ();

			for (size_t j = 0; j < ChildrenCount; j++)
			{
				CGrammarNode* pProduction = pNode->m_ProductionArray [j];
				if (PropagateParentChild (pNode, pProduction))
					HasChanged = true;
			}
		}

		Sequence = m_pNodeMgr->m_SequenceList.GetHead ();
		for (; Sequence; Sequence++)
		{
			CSequenceNode* pNode = *Sequence;
			size_t ChildrenCount = pNode->m_Sequence.GetCount ();

			// FIRST between parent-child

			bool IsNullable = true;
			for (size_t j = 0; j < ChildrenCount; j++)
			{
				CGrammarNode* pChild = pNode->m_Sequence [j];
				if (pNode->m_FirstSet.Merge (pChild->m_FirstSet, rtl::EBitOp_Or))
					HasChanged = true;

				if (!pChild->IsNullable ())
				{
					IsNullable = false;
					break;
				}
			}

			if (IsNullable) // all nullable
				if (pNode->MarkNullable ())
					HasChanged = true;

			// FOLLOW between parent-child

			for (intptr_t j = ChildrenCount - 1; j >= 0; j--)
			{
				CGrammarNode* pChild = pNode->m_Sequence [j];
				if (pChild->m_FollowSet.Merge (pNode->m_FollowSet, rtl::EBitOp_Or))
					HasChanged = true;

				if (pNode->IsFinal ())
					if (pChild->MarkFinal ())
						HasChanged = true;

				if (!pChild->IsNullable ())
					break;
			}

			// FOLLOW between child-child

			if (ChildrenCount >= 2)
				for (size_t j = 0; j < ChildrenCount - 1; j++)
				{
					CGrammarNode* pChild = pNode->m_Sequence [j];
					for (size_t k = j + 1; k < ChildrenCount; k++)
					{
						CGrammarNode* pNext = pNode->m_Sequence [k];
						if (pChild->m_FollowSet.Merge (pNext->m_FirstSet, rtl::EBitOp_Or))
							HasChanged = true;

						if (!pNext->IsNullable ())
							break;
					}
				}
		}

		Beacon = m_pNodeMgr->m_BeaconList.GetHead ();
		for (; Beacon; Beacon++)
		{
			if (PropagateParentChild (*Beacon, Beacon->m_pTarget))
				HasChanged = true;
		}

	} while (HasChanged);

	BuildFirstFollowArrays (&m_pNodeMgr->m_AnyTokenNode);

	for (size_t i = 2; i < TokenCount; i++)
	{
		CSymbolNode* pNode = m_pNodeMgr->m_TokenArray [i];
		BuildFirstFollowArrays (pNode);
	}

	for (size_t i = 0; i < SymbolCount; i++)
	{
		CSymbolNode* pNode = m_pNodeMgr->m_SymbolArray [i];
		BuildFirstFollowArrays (pNode);
	}

	Sequence = m_pNodeMgr->m_SequenceList.GetHead ();
	for (; Sequence; Sequence++)
		BuildFirstFollowArrays (*Sequence);

	Beacon = m_pNodeMgr->m_BeaconList.GetHead ();
	for (; Beacon; Beacon++)
		BuildFirstFollowArrays (*Beacon);
}

void
CParseTableBuilder::BuildFirstFollowArrays (CGrammarNode* pNode)
{
	pNode->m_FirstArray.Clear ();
	pNode->m_FollowArray.Clear ();

	for (
		size_t i = pNode->m_FirstSet.FindBit (0);
		i != -1;
		i = pNode->m_FirstSet.FindBit (i + 1)
		)
	{
		CSymbolNode* pToken = m_pNodeMgr->m_TokenArray [i];
		pNode->m_FirstArray.Append (pToken);
	}

	for (
		size_t i = pNode->m_FollowSet.FindBit (0);
		i != -1;
		i = pNode->m_FollowSet.FindBit (i + 1)
		)
	{
		CSymbolNode* pToken = m_pNodeMgr->m_TokenArray [i];
		pNode->m_FollowArray.Append (pToken);
	}
}

//.............................................................................
