#include "pch.h"
#include "ProductionBuilder.h"

//.............................................................................

CProductionBuilder::CProductionBuilder (CNodeMgr* pNodeMgr)
{
	m_pNodeMgr = pNodeMgr;
	m_pSymbol = NULL;
	m_pProduction = NULL;
	m_pDispatcher = NULL;
	m_pResolver = NULL;
}

CGrammarNode*
CProductionBuilder::Build (
	CSymbolNode* pSymbol,
	CGrammarNode* pProduction
	)
{
	bool Result;
	size_t FormalArgCount;

	switch (pProduction->m_Kind)
	{
	case ENode_Epsilon:
	case ENode_Token:
		return pProduction;

	case ENode_Symbol:
		if (pProduction->m_Flags & ESymbolNodeFlag_Named)
			return pProduction;

		// else fall through

	case ENode_Action:
	case ENode_Sequence:
		break;

	case ENode_Beacon:
		CBeaconNode* pBeacon;
		
		pBeacon = (CBeaconNode*) pProduction;

		FormalArgCount = pBeacon->m_pTarget->m_ArgNameList.GetCount ();		 
		if (FormalArgCount)
		{
			err::SetFormatStringError (
				"'%s' takes %d arguments, passed none", 
				pBeacon->m_pTarget->m_Name.cc (), // thanks a lot gcc 
				FormalArgCount
				);
			err::PushSrcPosError (pBeacon->m_SrcPos);
			return NULL;
		}

		pProduction = pBeacon->m_pTarget;
		m_pNodeMgr->DeleteBeaconNode (pBeacon);
		return pProduction;

	default:
		ASSERT (false);
		return NULL;
	}

	m_ActionArray.Clear ();
	m_ArgumentArray.Clear ();
	m_BeaconArray.Clear ();
	m_BeaconDeleteArray.Clear ();
	m_BeaconMap.Clear ();

	m_pSymbol = pSymbol;
	m_pProduction = pProduction;
	m_pDispatcher = NULL;
	m_pResolver = NULL;

	Result = Scan (pProduction);
	if (!Result)
		return NULL;
	
	Result = ProcessAllUserCode ();
	if (!Result)
	{
		EnsureSrcPosError ();
		return NULL;
	}

	FindAndReplaceUnusedBeacons (pProduction);
	
	size_t Count = m_BeaconDeleteArray.GetCount ();
	for (size_t i = 0; i < Count; i++)
		m_pNodeMgr->DeleteBeaconNode (m_BeaconDeleteArray [i]);

	return pProduction;
}

bool
CProductionBuilder::ProcessAllUserCode ()
{
	bool Result;

	size_t Count = m_ActionArray.GetCount ();
	for (size_t i = 0; i < Count; i++)
	{
		CActionNode* pNode = m_ActionArray [i];
		if (pNode->m_Flags & EUserNodeFlag_UserCodeProcessed)
			continue;

		Result = ProcessUserCode (pNode->m_SrcPos, &pNode->m_UserCode, pNode->m_pResolver);
		if (!Result)
			return false;

		pNode->m_Flags |= EUserNodeFlag_UserCodeProcessed;
		pNode->m_pDispatcher = m_pDispatcher;
	}

	Count = m_ArgumentArray.GetCount ();
	for (size_t i = 0; i < Count; i++)
	{
		CArgumentNode* pNode = m_ArgumentArray [i];
		if (pNode->m_Flags & EUserNodeFlag_UserCodeProcessed)
			continue;

		rtl::CBoxIteratorT <rtl::CString> It = pNode->m_ArgValueList.GetHead ();
		for (; It; It++)
		{
			Result = ProcessUserCode (pNode->m_SrcPos, &*It, pNode->m_pResolver);
			if (!Result)
				return false;
		}

		pNode->m_Flags |= EUserNodeFlag_UserCodeProcessed;
		pNode->m_pDispatcher = m_pDispatcher;
	}

	return true;
}

bool
CProductionBuilder::Scan (CGrammarNode* pNode)
{
	bool Result;

	if (pNode->m_Flags & ENodeFlag_RecursionStopper)
		return true;

	CSymbolNode* pSymbol;
	CSequenceNode* pSequence;
	CActionNode* pAction;
	CArgumentNode* pArgument;

	size_t ChildrenCount;

	switch (pNode->m_Kind)
	{
	case ENode_Epsilon:
	case ENode_Token:
		break;

	case ENode_Symbol:
		if (pNode->m_Flags & ESymbolNodeFlag_Named) 
			break;

		pSymbol = (CSymbolNode*) pNode;
		pSymbol->m_Flags |= ENodeFlag_RecursionStopper;

		if (pSymbol->m_pResolver)
		{
			CGrammarNode* pResolver = m_pResolver;
			m_pResolver = pSymbol->m_pResolver;

			Result = Scan (pSymbol->m_pResolver);
			if (!Result)
				return false;

			m_pResolver = pResolver;
		}

		ChildrenCount = pSymbol->m_ProductionArray.GetCount ();
		for (size_t i = 0; i < ChildrenCount; i++)
		{
			CGrammarNode* pChild = pSymbol->m_ProductionArray [i];
			Result = Scan (pChild);
			if (!Result)
				return false;
		}

		pSymbol->m_Flags &= ~ENodeFlag_RecursionStopper;
		break;

	case ENode_Sequence:
		pSequence = (CSequenceNode*) pNode;
		pSequence->m_Flags |= ENodeFlag_RecursionStopper;

		ChildrenCount = pSequence->m_Sequence.GetCount ();
		for (size_t i = 0; i < ChildrenCount; i++)
		{
			CGrammarNode* pChild = pSequence->m_Sequence [i];
			Result = Scan (pChild);
			if (!Result)
				return false;
		}

		pSequence->m_Flags &= ~ENodeFlag_RecursionStopper;
		break;

	case ENode_Beacon:
		Result = AddBeacon ((CBeaconNode*) pNode);
		if (!Result)
			return false;

		break;

	case ENode_Action:
		pAction = (CActionNode*) pNode;
		pAction->m_pProductionSymbol = m_pSymbol;
		pAction->m_pResolver = m_pResolver;
		m_ActionArray.Append (pAction);
		break;

	case ENode_Argument:
		pArgument = (CArgumentNode*) pNode;
		pArgument->m_pProductionSymbol = m_pSymbol;
		pArgument->m_pResolver = m_pResolver;
		m_ArgumentArray.Append (pArgument);
		break;

	default:
		ASSERT (false);
	}

	return true;
}

bool
CProductionBuilder::AddBeacon (CBeaconNode* pBeacon)
{
	if (pBeacon->m_Flags & EBeaconNodeFlag_Added)
		return true;

	if (!pBeacon->m_Label.IsEmpty ())
	{
		rtl::CStringHashTableMapIteratorT <CBeaconNode*> It = m_BeaconMap.Goto (pBeacon->m_Label);
		if (!It->m_Value)
			It->m_Value = pBeacon;
	}

	if (pBeacon->m_pTarget->m_Kind == ENode_Symbol)
	{
		CSymbolNode* pNode = (CSymbolNode*) pBeacon->m_pTarget;
		size_t FormalArgCount = pNode->m_ArgNameList.GetCount ();
		size_t ActualArgCount = pBeacon->m_pArgument ? pBeacon->m_pArgument->m_ArgValueList.GetCount () : 0;
		
		if (FormalArgCount != ActualArgCount)
		{
			err::SetFormatStringError (
				"'%s' takes %d arguments, passed %d", 
				pNode->m_Name.cc (), 
				FormalArgCount, 
				ActualArgCount
				);
			err::PushSrcPosError (pBeacon->m_SrcPos);
			return false;
		}
	}

	m_BeaconArray.Append (pBeacon);
	pBeacon->m_Flags |= EBeaconNodeFlag_Added;
	pBeacon->m_pResolver = m_pResolver;
	return true;
}

void
CProductionBuilder::FindAndReplaceUnusedBeacons (CGrammarNode*& pNode)
{
	if (pNode->m_Flags & ENodeFlag_RecursionStopper)
		return;

	CSymbolNode* pSymbol;
	CSequenceNode* pSequence;
	CBeaconNode* pBeacon;

	size_t Count;

	switch (pNode->m_Kind)
	{
	case ENode_Epsilon:
	case ENode_Token:
	case ENode_Action:
	case ENode_Argument:
		break;

	case ENode_Beacon:
		pBeacon = (CBeaconNode*) pNode;
		if (pBeacon->m_SlotIndex != -1)
			break;

		if (!(pBeacon->m_Flags & EBeaconNodeFlag_Deleted))
		{
			m_BeaconDeleteArray.Append (pBeacon);
			pBeacon->m_Flags |= EBeaconNodeFlag_Deleted;
		}	

		pNode = pBeacon->m_pTarget; // replace
		break;

	case ENode_Symbol:
		if (pNode->m_Flags & ESymbolNodeFlag_Named) 
			break;

		pSymbol = (CSymbolNode*) pNode;
		pSymbol->m_Flags |= ENodeFlag_RecursionStopper;

		if (pSymbol->m_pResolver)
			FindAndReplaceUnusedBeacons (pSymbol->m_pResolver);

		Count = pSymbol->m_ProductionArray.GetCount ();
		for (size_t i = 0; i < Count; i++)
			FindAndReplaceUnusedBeacons (pSymbol->m_ProductionArray [i]);

		pSymbol->m_Flags &= ~ENodeFlag_RecursionStopper;
		break;

	case ENode_Sequence:
		pSequence = (CSequenceNode*) pNode;
		pSequence->m_Flags |= ENodeFlag_RecursionStopper;

		Count = pSequence->m_Sequence.GetCount ();
		for (size_t i = 0; i < Count; i++)
			FindAndReplaceUnusedBeacons (pSequence->m_Sequence [i]);

		pSequence->m_Flags &= ~ENodeFlag_RecursionStopper;
		break;

	default:
		ASSERT (false);
	}
}

CProductionBuilder::EVariable
CProductionBuilder::FindVariable (
	int Index,
	CBeaconNode** ppBeacon
	)
{
	if (Index == 0)
		return EVariable_This;

	size_t BeaconIndex = Index - 1;
	size_t BeaconCount = m_BeaconArray.GetCount ();
	
	if (BeaconIndex >= BeaconCount)
	{
		err::SetFormatStringError ("locator '$%d' is out of range ($1..$%d)", BeaconIndex + 1, BeaconCount);
		return EVariable_Undefined;
	}

	CBeaconNode* pBeacon = m_BeaconArray [BeaconIndex];
	*ppBeacon = pBeacon;
	return pBeacon->m_pTarget->m_Kind == ENode_Token ? 
		EVariable_TokenBeacon :
		EVariable_SymbolBeacon;
}

CProductionBuilder::EVariable
CProductionBuilder::FindVariable (
	const char* pName,
	CBeaconNode** ppBeacon
	)
{
	rtl::CStringHashTableMapIteratorT <CBeaconNode*> It = m_BeaconMap.Find (pName);
	if (It)
	{
		CBeaconNode* pBeacon = It->m_Value;
		*ppBeacon = pBeacon;
		return pBeacon->m_pTarget->m_Kind == ENode_Token ? 
			EVariable_TokenBeacon :
			EVariable_SymbolBeacon;
	}

	rtl::CHashTableIteratorT <const char*> It2 = m_pSymbol->m_LocalNameSet.Find (pName);
	if (It2)
		return EVariable_Local;

	It2 = m_pSymbol->m_ArgNameSet.Find (pName);
	if (It2)
		return EVariable_Arg;

	err::SetFormatStringError ("locator '$%s' not found", pName);
	return EVariable_Undefined;
}

bool
CProductionBuilder::ProcessUserCode (
	lex::CSrcPos& SrcPos,
	rtl::CString* pUserCode,
	CGrammarNode* pResolver
	)
{
	const CToken* pToken;

	rtl::CString ResultString;

	CLexer::Create (
		GetMachineState (ELexerMachine_UserCode2ndPass), 
		SrcPos.m_FilePath, 
		*pUserCode
		);

	SetLineCol (SrcPos);

	const char* p = *pUserCode;

	EVariable VariableKind;
	CBeaconNode* pBeacon;

	for (;;)
	{
		pToken = GetToken ();
		if (pToken->m_Token <= 0)
			break;

		switch (pToken->m_Token)
		{
		case EToken_Integer:
			VariableKind = FindVariable (pToken->m_Data.m_Integer, &pBeacon);
			break;

		case EToken_Identifier:
			VariableKind = FindVariable (pToken->m_Data.m_String, &pBeacon);
			break;

		default:
			NextToken ();
			continue;
		}

		if (!VariableKind)
			return false;

		ResultString.Append (p, pToken->m_Pos.m_p - p);

		switch (VariableKind)
		{
		case EVariable_SymbolBeacon:
			if (pBeacon->m_pTarget->m_Flags & ESymbolNodeFlag_NoAst)
			{
				err::SetFormatStringError (
					"'%s' is declared as 'noast' and cannot be referenced from user actions", 
					pBeacon->m_pTarget->m_Name.cc () 
					);
				return false;
			}

			// and fall through

		case EVariable_TokenBeacon:
			if (pBeacon->m_pResolver != pResolver)
			{
				err::SetFormatStringError (
					"cross-resolver reference to locator '%s'", 
					pToken->GetText ().cc () 
					);
				return false;
			}

			if (pBeacon->m_SlotIndex == -1)
			{
				if (!m_pDispatcher)
					m_pDispatcher = m_pNodeMgr->CreateDispatcherNode (m_pSymbol);

				pBeacon->m_SlotIndex = m_pDispatcher->m_BeaconArray.GetCount ();
				m_pDispatcher->m_BeaconArray.Append (pBeacon);
			}

			ResultString.AppendFormat ("$%d", pBeacon->m_SlotIndex);
			break;

		case EVariable_This:
			if (pResolver)
			{
				err::SetFormatStringError ("resolvers cannot reference left side of production");
				return false;
			}

			ResultString.Append ('$');
			break;

		case EVariable_Arg:
			if (pResolver)
			{
				err::SetFormatStringError ("resolvers cannot reference arguments");
				return false;
			}

			ResultString.AppendFormat (
				"$arg.%s", 
				pToken->m_Data.m_String.cc () 
				);
			break;

		case EVariable_Local:
			if (pResolver)
			{
				err::SetFormatStringError ("resolvers cannot reference locals");
				return false;
			}

			ResultString.AppendFormat (
				"$local.%s", 
				pToken->m_Data.m_String.cc () 
				);
			break;
		}

		p = pToken->m_Pos.m_p + pToken->m_Pos.m_Length;

		NextToken ();
	}

	ASSERT (!pToken->m_Token);
	ResultString.Append (p, pToken->m_Pos.m_p - p);

	*pUserCode = ResultString;
	return true;
}

//.............................................................................
