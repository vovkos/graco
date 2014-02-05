#include "pch.h"
#include "llkc_NodeMgr.h"

//.............................................................................

CNodeMgr::CNodeMgr ()
{
	m_MasterCount = 0;
	m_pPrimaryStartSymbol = NULL;

	// we use the same master index for epsilon production and eof token:
	// token with index 0 is eof token
	// parse table entry equal 0 is epsilon production

	m_EofTokenNode.m_Kind = ENode_Token;
	m_AnyTokenNode.m_Flags = ESymbolNodeFlag_EofToken;
	m_EofTokenNode.m_Name = "$";
	m_EofTokenNode.m_Index = 0;
	m_EofTokenNode.m_MasterIndex = 0;

	m_AnyTokenNode.m_Kind = ENode_Token;
	m_AnyTokenNode.m_Flags = ESymbolNodeFlag_AnyToken;
	m_AnyTokenNode.m_Name = "<any>";
	m_AnyTokenNode.m_Index = 1;
	m_AnyTokenNode.m_MasterIndex = 1;

	m_EpsilonNode.m_Kind = ENode_Epsilon;
	m_EpsilonNode.m_Flags |= EGrammarNodeFlag_Nullable;
	m_EpsilonNode.m_Name = "<epsilon>";
	m_EpsilonNode.m_MasterIndex = 0; 

	m_StartPragmaSymbol.m_Flags |= ESymbolNodeFlag_Pragma;
	m_StartPragmaSymbol.m_Name = "<pragma>";
}

void
CNodeMgr::Clear ()
{
	m_CharTokenList.Clear ();
	m_NamedTokenList.Clear ();
	m_NamedSymbolList.Clear ();
	m_TempSymbolList.Clear ();
	m_SequenceList.Clear ();
	m_BeaconList.Clear ();
	m_DispatcherList.Clear ();
	m_ActionList.Clear ();
	m_ArgumentList.Clear ();
	m_ConflictList.Clear ();
	m_LaDfaList.Clear ();

	m_TokenMap.Clear ();
	m_SymbolMap.Clear ();
	m_AnyTokenNode.m_FirstArray.Clear ();
	m_AnyTokenNode.m_FirstSet.Clear ();
	
	m_StartPragmaSymbol.m_Index = -1;
	m_StartPragmaSymbol.m_MasterIndex = -1;
	m_StartPragmaSymbol.m_ProductionArray.Clear ();

	m_MasterCount = 0;
	m_pPrimaryStartSymbol = NULL;
}

void
CNodeMgr::Trace ()
{
	TraceNodeArray ("TOKENS", &m_TokenArray);
	TraceNodeArray ("SYMBOLS", &m_SymbolArray);
	TraceNodeList ("SEQUENCES", m_SequenceList.GetHead ());
	TraceNodeList ("BEACONS", m_BeaconList.GetHead ());
	TraceNodeList ("DISPATCHERS", m_DispatcherList.GetHead ());
	TraceNodeList ("ACTIONS", m_ActionList.GetHead ());
	TraceNodeList ("ARGUMENTS", m_ArgumentList.GetHead ());
	TraceNodeList ("CONFLICTS", m_ConflictList.GetHead ());
	TraceNodeList ("LOOKAHEAD DFA", m_LaDfaList.GetHead ());
}

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

CSymbolNode*
CNodeMgr::GetTokenNode (int Token)
{
	rtl::CHashTableMapIteratorT <int, CSymbolNode*> Node = m_TokenMap.Goto (Token);
	if (Node->m_Value)
		return Node->m_Value;

	CSymbolNode* pNode = AXL_MEM_NEW (CSymbolNode);
	pNode->m_Kind = ENode_Token;
	pNode->m_CharToken = Token;
	
	if (isprint (Token))
		pNode->m_Name.Format ("\'%c\'", (char) Token);
	else 
		pNode->m_Name.Format ("\\0%d", Token);

	m_CharTokenList.InsertTail (pNode);
	Node->m_Value = pNode;

	return pNode;
}

CSymbolNode*
CNodeMgr::GetSymbolNode (const rtl::CString& Name)
{
	rtl::CStringHashTableMapIteratorT <CSymbolNode*> It = m_SymbolMap.Goto (Name);
	if (It->m_Value)
		return It->m_Value;

	CSymbolNode* pNode = AXL_MEM_NEW (CSymbolNode);
	pNode->m_Kind = ENode_Symbol;
	pNode->m_Flags = ESymbolNodeFlag_Named;
	pNode->m_Name = Name;
	
	m_NamedSymbolList.InsertTail (pNode);
	It->m_Value = pNode;
	
	return pNode;
}

CSymbolNode*
CNodeMgr::CreateTempSymbolNode ()
{
	CSymbolNode* pNode = AXL_MEM_NEW (CSymbolNode);
	pNode->m_Name.Format ("_tmp%d", m_TempSymbolList.GetCount () + 1);
	m_TempSymbolList.InsertTail (pNode);
	return pNode;
}

CSequenceNode*
CNodeMgr::CreateSequenceNode ()
{
	CSequenceNode* pNode = AXL_MEM_NEW (CSequenceNode);
	pNode->m_Name.Format ("_seq%d", m_SequenceList.GetCount () + 1);
	m_SequenceList.InsertTail (pNode);
	return pNode;
}

CSequenceNode*
CNodeMgr::CreateSequenceNode (CGrammarNode* pNode)
{
	CSequenceNode* pSequenceNode = CreateSequenceNode ();
	pSequenceNode->Append (pNode);
	return pSequenceNode;
}

CBeaconNode*
CNodeMgr::CreateBeaconNode (CSymbolNode* pTarget)
{
	CBeaconNode* pBeaconNode = AXL_MEM_NEW (CBeaconNode);
	pBeaconNode->m_Kind = ENode_Beacon;
	pBeaconNode->m_pTarget = pTarget;
	if (pTarget->m_Kind == ENode_Symbol)
		pBeaconNode->m_Label = pTarget->m_Name;
	pBeaconNode->m_Name.Format (
		"_bcn%d(%s)", 
		m_BeaconList.GetCount () + 1, 
		pTarget->m_Name.cc () // thanks a lot gcc
		);
	m_BeaconList.InsertTail (pBeaconNode);
	return pBeaconNode;
}

CDispatcherNode*
CNodeMgr::CreateDispatcherNode (CSymbolNode* pSymbol)
{
	CDispatcherNode* pDispatcherNode = AXL_MEM_NEW (CDispatcherNode);
	pDispatcherNode->m_Name.Format ("_dsp%d", m_DispatcherList.GetCount () + 1);
	pDispatcherNode->m_pSymbol = pSymbol;
	m_DispatcherList.InsertTail (pDispatcherNode);
	return pDispatcherNode;
}

CActionNode*
CNodeMgr::CreateActionNode ()
{
	CActionNode* pNode = AXL_MEM_NEW (CActionNode);
	pNode->m_Name.Format ("_act%d", m_ActionList.GetCount () + 1);
	m_ActionList.InsertTail (pNode);
	return pNode;
}

CArgumentNode*
CNodeMgr::CreateArgumentNode ()
{
	CArgumentNode* pNode = AXL_MEM_NEW (CArgumentNode);
	pNode->m_Name.Format ("_arg%d", m_ArgumentList.GetCount () + 1);
	m_ArgumentList.InsertTail (pNode);
	return pNode;
}

CConflictNode*
CNodeMgr::CreateConflictNode ()
{
	CConflictNode* pNode = AXL_MEM_NEW (CConflictNode);
	pNode->m_Name.Format ("_cnf%d", m_ConflictList.GetCount () + 1);
	m_ConflictList.InsertTail (pNode);
	return pNode;
}

CLaDfaNode*
CNodeMgr::CreateLaDfaNode ()
{
	CLaDfaNode* pNode = AXL_MEM_NEW (CLaDfaNode);
	pNode->m_Name.Format ("_dfa%d", m_LaDfaList.GetCount () + 1);
	m_LaDfaList.InsertTail (pNode);
	return pNode;
}

CGrammarNode*
CNodeMgr::CreateQuantifierNode (
	CGrammarNode* pNode,
	int Kind
	)
{
	CSequenceNode* pTempSeq;
	CSymbolNode* pTempAlt;

	if (pNode->m_Kind == ENode_Action || pNode->m_Kind == ENode_Epsilon)
	{
		err::SetFormatStringError ("can't apply quantifier to action or epsilon nodes");
		return NULL;
	}

	switch (Kind)
	{
	case '?':
		pTempAlt = CreateTempSymbolNode ();
		pTempAlt->AddProduction (pNode);
		pTempAlt->AddProduction (&m_EpsilonNode);
		return pTempAlt;

	case '*':
		pTempAlt = CreateTempSymbolNode ();

		pTempSeq = CreateSequenceNode ();
		pTempSeq->Append (pNode);
		pTempSeq->Append (pTempAlt);

		pTempAlt->AddProduction (pTempSeq);
		pTempAlt->AddProduction (&m_EpsilonNode);

		return pTempAlt;
		
	case '+': 
		pTempSeq = CreateSequenceNode ();
		pTempSeq->Append (pNode);
		pTempSeq->Append (CreateQuantifierNode (pNode, '*'));

		return pTempSeq;

	default:
		ASSERT (false);
		return NULL;
	}
}

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

void
CNodeMgr::MarkReachableNodes ()
{
	rtl::CIteratorT <CSymbolNode> Node = m_NamedSymbolList.GetHead ();

	if (m_pPrimaryStartSymbol)
		for (; Node; Node++)
		{
			CSymbolNode* pNode = *Node;
			if (pNode->m_Flags & ESymbolNodeFlag_Start)
				pNode->MarkReachable ();
		}
	else
		for (; Node; Node++)
		{
			CSymbolNode* pNode = *Node;
			pNode->MarkReachable ();
		}

	m_StartPragmaSymbol.MarkReachable (); 
}

template <typename T>
static
void
DeleteUnreachableNodesFromList (rtl::CStdListT <T>* pList)
{
	rtl::CIteratorT <T> Node = pList->GetHead ();
	while (Node)
	{
		T* pNode = *Node++;
		if (!pNode->IsReachable ())
			pList->Delete (pNode);	
	}
}

void
CNodeMgr::DeleteUnreachableNodes ()
{
	DeleteUnreachableNodesFromList (&m_CharTokenList);
	DeleteUnreachableNodesFromList (&m_NamedSymbolList);
	DeleteUnreachableNodesFromList (&m_TempSymbolList);
	DeleteUnreachableNodesFromList (&m_SequenceList);
	DeleteUnreachableNodesFromList (&m_BeaconList);
	DeleteUnreachableNodesFromList (&m_ActionList);
	DeleteUnreachableNodesFromList (&m_ArgumentList);
}

void 
CNodeMgr::IndexTokens ()
{
	size_t i = 0;

	size_t Count = m_CharTokenList.GetCount () + 2;
	m_TokenArray.SetCount (Count);

	m_TokenArray [i++] = &m_EofTokenNode;
	m_TokenArray [i++] = &m_AnyTokenNode;

	rtl::CIteratorT <CSymbolNode> Node = m_CharTokenList.GetHead ();
	for (; Node; Node++, i++)
	{
		CSymbolNode* pNode = *Node;
		pNode->m_Index = i;
		pNode->m_MasterIndex = i;
		m_TokenArray [i] = pNode;
	}

	m_MasterCount = i;
}

void 
CNodeMgr::IndexSymbols ()
{
	size_t i = 0;
	size_t j = m_MasterCount;
	
	rtl::CIteratorT <CSymbolNode> Node = m_NamedSymbolList.GetHead ();
	while (Node)
	{
		CSymbolNode* pNode = *Node++;
		if (!pNode->m_ProductionArray.IsEmpty ())	
			continue;

		pNode->m_Kind = ENode_Token;
		pNode->m_Index = j;
		pNode->m_MasterIndex = j;

		j++;

		m_NamedSymbolList.Remove (pNode);
		m_NamedTokenList.InsertTail (pNode);
		m_TokenArray.Append (pNode);
	}

	size_t Count = m_NamedSymbolList.GetCount () + m_TempSymbolList.GetCount ();
	m_SymbolArray.SetCount (Count);

	Node = m_NamedSymbolList.GetHead ();
	for (; Node; Node++, i++, j++)
	{
		CSymbolNode* pNode = *Node;
		pNode->m_Index = i;
		pNode->m_MasterIndex = j;
		m_SymbolArray [i] = pNode;
	}

	Node = m_TempSymbolList.GetHead ();
	for (; Node; Node++, i++, j++)
	{
		CSymbolNode* pNode = *Node;
		pNode->m_Index = i;
		pNode->m_MasterIndex = j;
		m_SymbolArray [i] = pNode;
	}

	if (!m_StartPragmaSymbol.m_ProductionArray.IsEmpty ())
	{
		m_StartPragmaSymbol.m_Index = i;
		m_StartPragmaSymbol.m_MasterIndex = j;
		m_SymbolArray.Append (&m_StartPragmaSymbol);

		i++;
		j++;
	}

	m_MasterCount = j;
}

void 
CNodeMgr::IndexSequences ()
{
	size_t i = 0;
	size_t j = m_MasterCount;

	size_t Count = m_SequenceList.GetCount ();

	rtl::CIteratorT <CSequenceNode> Node = m_SequenceList.GetHead ();
	for (; Node; Node++, i++, j++)
	{
		CSequenceNode* pNode = *Node;
		pNode->m_Index = i;
		pNode->m_MasterIndex = j;
	}

	m_MasterCount = j;
}

void 
CNodeMgr::IndexBeacons ()
{
	size_t i = 0;
	size_t j = m_MasterCount;

	rtl::CIteratorT <CBeaconNode> Node = m_BeaconList.GetHead ();
	for (; Node; Node++, i++, j++)
	{
		CBeaconNode* pNode = *Node;
		pNode->m_Index = i;
		pNode->m_MasterIndex = j;
	}

	m_MasterCount = j;
}

void 
CNodeMgr::IndexDispatchers ()
{
	size_t i = 0;

	rtl::CIteratorT <CDispatcherNode> Node = m_DispatcherList.GetHead ();
	for (; Node; Node++, i++)
	{
		CDispatcherNode* pNode = *Node;
		pNode->m_Index = i;
	}
}

void 
CNodeMgr::IndexActions ()
{
	size_t i = 0;
	size_t j = m_MasterCount;

	rtl::CIteratorT <CActionNode> Node = m_ActionList.GetHead ();
	for (; Node; Node++, i++, j++)
	{
		CActionNode* pNode = *Node;
		pNode->m_Index = i;
		pNode->m_MasterIndex = j;
	}

	m_MasterCount = j;
}

void 
CNodeMgr::IndexArguments ()
{
	size_t i = 0;
	size_t j = m_MasterCount;

	rtl::CIteratorT <CArgumentNode> Node = m_ArgumentList.GetHead ();
	for (; Node; Node++, i++, j++)
	{
		CArgumentNode* pNode = *Node;
		pNode->m_Index = i;
		pNode->m_MasterIndex = j;
	}

	m_MasterCount = j;
}

void 
CNodeMgr::IndexLaDfaNodes ()
{
	size_t i = 0;
	size_t j = m_MasterCount;
	
	rtl::CIteratorT <CLaDfaNode> Node = m_LaDfaList.GetHead ();
	for (; Node; Node++)
	{
		CLaDfaNode* pNode = *Node;

		if (!(pNode->m_Flags & ELaDfaNodeFlag_Leaf) &&       // don't index leaves
			(!pNode->m_pResolver || pNode->m_pResolverUplink)) // and non-chained resolvers
		{
			pNode->m_Index = i++;
			pNode->m_MasterIndex = j++;
		}
	}

	m_MasterCount = j;
}

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

void
CNodeMgr::Export (lua::CLuaState* pLuaState)
{
	pLuaState->SetGlobalInteger ("StartSymbol", m_pPrimaryStartSymbol ? m_pPrimaryStartSymbol->m_Index : -1);
	pLuaState->SetGlobalInteger ("StartPragmaSymbol", m_StartPragmaSymbol.m_Index);
	pLuaState->SetGlobalInteger ("NamedTokenCount", m_NamedTokenList.GetCount ());
	pLuaState->SetGlobalInteger ("NamedSymbolCount", m_NamedSymbolList.GetCount ());

	ExportNodeArray (pLuaState, "TokenTable", (CNode**) (CSymbolNode**) m_TokenArray, m_TokenArray.GetCount ());
	ExportNodeArray (pLuaState, "SymbolTable", (CNode**) (CSymbolNode**) m_SymbolArray, m_SymbolArray.GetCount ());	
	ExportNodeList (pLuaState, "SequenceTable", m_SequenceList.GetHead (), m_SequenceList.GetCount ());	
	ExportNodeList (pLuaState, "BeaconTable", m_BeaconList.GetHead (), m_BeaconList.GetCount ());
	ExportNodeList (pLuaState, "DispatcherTable", m_DispatcherList.GetHead (), m_DispatcherList.GetCount ());
	ExportNodeList (pLuaState, "ActionTable", m_ActionList.GetHead (), m_ActionList.GetCount ());
	ExportNodeList (pLuaState, "ArgumentTable", m_ArgumentList.GetHead (), m_ArgumentList.GetCount ());
	
	ExportLaDfaTable (pLuaState);
}

void
CNodeMgr::ExportNodeArray (
	lua::CLuaState* pLuaState,
	const char* pName,
	CNode* const* ppNode,
	size_t Count
	)
{
	pLuaState->CreateTable (Count);

	for (size_t i = 0; i < Count; i++)
	{
		CNode* pNode = ppNode [i];
		pNode->Export (pLuaState);
		pLuaState->SetArrayElement (i + 1);
	}


	pLuaState->SetGlobal (pName);
}

void
CNodeMgr::ExportNodeList (
	lua::CLuaState* pLuaState,
	const char* pName,
	rtl::CIteratorT <CNode> Node,
	size_t CountEstimate
	)
{
	pLuaState->CreateTable (CountEstimate);

	size_t i = 1;

	for (; Node; Node++, i++)
	{
		Node->Export (pLuaState);
		pLuaState->SetArrayElement (i);
	}

	pLuaState->SetGlobal (pName);
}

void
CNodeMgr::ExportLaDfaTable (lua::CLuaState* pLuaState)
{
	pLuaState->CreateTable (m_LaDfaList.GetCount ());

	size_t i = 1;
	rtl::CIteratorT <CLaDfaNode> Node = m_LaDfaList.GetHead ();

	for (; Node; Node++)
	{
		CLaDfaNode* pNode = *Node;

		if (pNode->m_MasterIndex != -1)
		{
			pNode->Export (pLuaState);
			pLuaState->SetArrayElement (i++);
		}
	}

	pLuaState->SetGlobal ("LaDfaTable");
}

//.............................................................................
