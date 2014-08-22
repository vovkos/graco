#include "pch.h"
#include "Module.h"
#include "ProductionBuilder.h"
#include "ParseTableBuilder.h"
#include "LaDfaBuilder.h"

#include "axl_g_WarningSuppression.h" // gcc loses warning suppression from pch

//.............................................................................

CModule::CModule ()
{
	m_LookaheadLimit = 0;
	m_Lookahead = 1;
}

void
CModule::Clear ()
{
	m_ParseTable.Clear ();
	m_ClassMgr.Clear ();
	m_DefineMgr.Clear ();
	m_NodeMgr.Clear ();
	m_ImportList.Clear ();
	m_Lookahead = 1;
	m_LookaheadLimit = 0;
}

bool
CModule::Build (CConfig* pConfig)
{
	bool Result;

	m_ParseTable.Clear ();

	if (m_NodeMgr.IsEmpty ())
	{
		err::SetStringError ("grammar is empty");
		return false;
	}

	if (!m_ClassMgr.Verify ())
		return false;

	// check reachability from start symbols

	m_NodeMgr.MarkReachableNodes ();
	m_NodeMgr.DeleteUnreachableNodes ();
	m_ClassMgr.DeleteUnreachableClasses ();

	// after marking we can assign default start symbol

	if (!m_NodeMgr.m_pPrimaryStartSymbol)
		m_NodeMgr.m_pPrimaryStartSymbol = *m_NodeMgr.m_NamedSymbolList.GetHead ();

	m_NodeMgr.IndexTokens ();
	m_NodeMgr.IndexSymbols ();
	m_NodeMgr.IndexSequences ();
	m_NodeMgr.IndexActions ();
	m_NodeMgr.IndexArguments ();

	// build productions

	CProductionBuilder ProductionBuilder (&m_NodeMgr);

	rtl::CIteratorT <CSymbolNode> Symbol = m_NodeMgr.m_NamedSymbolList.GetHead ();
	for (; Symbol; Symbol++)
	{
		size_t Count = Symbol->m_ProductionArray.GetCount ();
		for (size_t i = 0; i < Count; i++)
		{
			CGrammarNode* pProduction = Symbol->m_ProductionArray [i];
			pProduction = ProductionBuilder.Build (*Symbol, pProduction);
			if (!pProduction)
				return false;

			Symbol->m_ProductionArray [i] = pProduction;
		}
	}

	m_NodeMgr.IndexBeacons (); // index only after unneeded beacons have been removed
	m_NodeMgr.IndexDispatchers ();

	// build parse table

	CParseTableBuilder ParseTableBuilder (&m_NodeMgr, &m_ParseTable);
	Result = ParseTableBuilder.Build ();
	if (!Result)
		return false;

	// resolve conflicts

	CLaDfaBuilder Builder (&m_NodeMgr, &m_ParseTable, m_LookaheadLimit ? m_LookaheadLimit : 2);

	size_t TokenCount = m_NodeMgr.m_TokenArray.GetCount ();

	rtl::CIteratorT <CConflictNode> Conflict = m_NodeMgr.m_ConflictList.GetHead ();
	for (; Conflict; Conflict++)
	{
		CConflictNode* pConflict = *Conflict;

		pConflict->m_pResultNode = Builder.Build (pConfig, pConflict);
		if (!pConflict->m_pResultNode)
			return false;
	}

	// replace conflicts with dfas or with direct productions (could happen in conflicts with epsilon productions or with anytoken)

	Conflict = m_NodeMgr.m_ConflictList.GetHead ();
	for (; Conflict; Conflict++)
	{
		CConflictNode* pConflict = *Conflict;
		CNode** ppProduction = &m_ParseTable [pConflict->m_pSymbol->m_Index * TokenCount + pConflict->m_pToken->m_Index];

		ASSERT (*ppProduction == pConflict);

		*ppProduction = pConflict->m_pResultNode;
	}

	m_Lookahead = Builder.GetLookahead ();

	m_NodeMgr.IndexLaDfaNodes ();

	return true;
}

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

void
CModule::Trace ()
{
	printf ("lookahead = %d\n", m_Lookahead);
	m_NodeMgr.Trace ();
}

bool
CModule::WriteBnfFile (const char* pFileName)
{
	rtl::CString BufferString;

	io::CFile File;
	bool Result =
		File.Open (pFileName) &&
		File.SetSize (0);

	if (!Result)
		return false;

	rtl::CString String = GenerateBnfString ();
	File.Write (String, String.GetLength ());
	return true;
}

rtl::CString
CModule::GenerateBnfString ()
{
	rtl::CString String;
	rtl::CString SequenceString;

	String.Format ("lookahead = %d;\n\n", m_LookaheadLimit);

	rtl::CIteratorT <CSymbolNode> Node = m_NodeMgr.m_NamedSymbolList.GetHead ();
	for (; Node; Node++)
	{
		CSymbolNode* pSymbol = *Node;
		if (pSymbol->m_ProductionArray.IsEmpty ())
			continue;

		if (pSymbol->m_Flags & ESymbolNodeFlag_Start)
			String.Append ("start\n");

		if (pSymbol->m_Flags & ESymbolNodeFlag_Nullable)
			String.Append ("nullable\n");

		if (pSymbol->m_Flags & ESymbolNodeFlag_Pragma)
			String.Append ("pragma\n");

		String.Append (pSymbol->m_Name);
		String.Append ('\n');

		size_t ProductionCount = pSymbol->m_ProductionArray.GetCount ();

		if (pSymbol->m_QuantifierKind)
		{
			String.Append ("\t:\t");
			String.Append (pSymbol->CGrammarNode::GetBnfString ());
			String.Append ('\n');
		}
		else for (size_t i = 0; i < ProductionCount; i++)
		{
			String.Append (i ? "\t|\t" : "\t:\t");
			String.Append (pSymbol->m_ProductionArray [i]->GetBnfString ());
			String.Append ('\n');
		}

		String.Append ("\t;\n\n");
	}

	return String;
}

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

void
CModule::Export (lua::CLuaState* pLuaState)
{
	ExportDefines (pLuaState);
	ExportClassTable (pLuaState);
	ExportParseTable (pLuaState);
	pLuaState->SetGlobalInteger ("Lookahead", m_Lookahead);
	m_NodeMgr.Export (pLuaState);
}

void
CModule::ExportDefines (lua::CLuaState* pLuaState)
{
	rtl::CIteratorT <CDefine> Define = m_DefineMgr.GetHead ();
	for (; Define; Define++)
	{
		CDefine* pDefine = *Define;

		switch (pDefine->m_Kind)
		{
		case EDefine_String:
			pLuaState->SetGlobalString (pDefine->m_Name, pDefine->m_StringValue);
			break;

		case EDefine_Integer:
			pLuaState->SetGlobalInteger (pDefine->m_Name, pDefine->m_IntegerValue);
			break;
		}
	}
}

void
CModule::ExportClassTable (lua::CLuaState* pLuaState)
{
	size_t Count = m_ClassMgr.GetCount ();

	pLuaState->CreateTable (Count);

	rtl::CIteratorT <CClass> Class = m_ClassMgr.GetHead ();
	for (size_t i = 1; Class; Class++, i++)
	{
		CClass* pClass = *Class;
		pClass->Export (pLuaState);
		pLuaState->SetArrayElement (i);
	}

	pLuaState->SetGlobal ("ClassTable");
}

void
CModule::ExportParseTable (lua::CLuaState* pLuaState)
{
	size_t SymbolCount = m_NodeMgr.m_SymbolArray.GetCount ();
	size_t TokenCount = m_NodeMgr.m_TokenArray.GetCount ();

	pLuaState->CreateTable (SymbolCount);

	for (size_t i = 0, k = 0; i < SymbolCount; i++)
	{
		pLuaState->CreateTable (TokenCount);

		for (size_t j = 0; j < TokenCount; j++, k++)
		{
			CNode* pProduction = m_ParseTable [k];
			pLuaState->SetArrayElementInteger (j + 1, pProduction ? pProduction->m_MasterIndex : -1);
		}

		pLuaState->SetArrayElement (i + 1);
	}

	pLuaState->SetGlobal ("ParseTable");
}

//.............................................................................
