#include "pch.h"
#include "Module.h"
#include "ProductionBuilder.h"
#include "ParseTableBuilder.h"
#include "LaDfaBuilder.h"

#include "axl_g_WarningSuppression.h" // gcc loses warning suppression from pch

//.............................................................................

Module::Module ()
{
	m_lookaheadLimit = 0;
	m_lookahead = 1;
}

void
Module::clear ()
{
	m_parseTable.clear ();
	m_classMgr.clear ();
	m_defineMgr.clear ();
	m_nodeMgr.clear ();
	m_importList.clear ();
	m_lookahead = 1;
	m_lookaheadLimit = 0;
}

bool
Module::build (CmdLine* cmdLine)
{
	bool result;

	m_parseTable.clear ();

	if (m_nodeMgr.isEmpty ())
	{
		err::setStringError ("grammar is empty");
		return false;
	}

	if (!m_classMgr.verify ())
		return false;

	// check reachability from start symbols

	m_nodeMgr.markReachableNodes ();
	m_nodeMgr.deleteUnreachableNodes ();
	m_classMgr.deleteUnreachableClasses ();

	// after marking we can assign default start symbol

	if (!m_nodeMgr.m_primaryStartSymbol)
		m_nodeMgr.m_primaryStartSymbol = *m_nodeMgr.m_namedSymbolList.getHead ();

	m_nodeMgr.indexTokens ();
	m_nodeMgr.indexSymbols ();
	m_nodeMgr.indexSequences ();
	m_nodeMgr.indexActions ();
	m_nodeMgr.indexArguments ();

	// build productions

	ProductionBuilder productionBuilder (&m_nodeMgr);

	rtl::Iterator <SymbolNode> symbolIt = m_nodeMgr.m_namedSymbolList.getHead ();
	for (; symbolIt; symbolIt++)
	{
		SymbolNode* symbol = *symbolIt;

		size_t count = symbol->m_productionArray.getCount ();
		for (size_t i = 0; i < count; i++)
		{
			GrammarNode* production = symbol->m_productionArray [i];
			production = productionBuilder.build (symbol, production);
			if (!production)
				return false;

			symbol->m_productionArray [i] = production;
		}
	}

	m_nodeMgr.indexBeacons (); // index only after unneeded beacons have been removed
	m_nodeMgr.indexDispatchers ();

	// build parse table

	ParseTableBuilder parseTableBuilder (&m_nodeMgr, &m_parseTable);
	result = parseTableBuilder.build ();
	if (!result)
		return false;

	// resolve conflicts

	LaDfaBuilder builder (&m_nodeMgr, &m_parseTable, m_lookaheadLimit ? m_lookaheadLimit : 2);

	size_t tokenCount = m_nodeMgr.m_tokenArray.getCount ();

	rtl::Iterator <ConflictNode> conflictIt = m_nodeMgr.m_conflictList.getHead ();
	for (; conflictIt; conflictIt++)
	{
		ConflictNode* conflict = *conflictIt;

		conflict->m_resultNode = builder.build (cmdLine, conflict);
		if (!conflict->m_resultNode)
			return false;
	}

	// replace conflicts with dfas or with direct productions (could happen in conflicts with epsilon productions or with anytoken)

	conflictIt = m_nodeMgr.m_conflictList.getHead ();
	for (; conflictIt; conflictIt++)
	{
		ConflictNode* conflict = *conflictIt;
		Node** production = &m_parseTable [conflict->m_symbol->m_index * tokenCount + conflict->m_token->m_index];

		ASSERT (*production == conflict);

		*production = conflict->m_resultNode;
	}

	m_lookahead = builder.getLookahead ();

	m_nodeMgr.indexLaDfaNodes ();

	return true;
}

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

void
Module::trace ()
{
	printf ("lookahead = %d\n", m_lookahead);
	m_nodeMgr.trace ();
}

bool
Module::writeBnfFile (const char* fileName)
{
	rtl::String bufferString;

	io::File file;
	bool result =
		file.open (fileName) &&
		file.setSize (0);

	if (!result)
		return false;

	rtl::String string = generateBnfString ();
	file.write (string, string.getLength ());
	return true;
}

rtl::String
Module::generateBnfString ()
{
	rtl::String string;
	rtl::String sequenceString;

	string.format ("lookahead = %d;\n\n", m_lookaheadLimit);

	rtl::Iterator <SymbolNode> node = m_nodeMgr.m_namedSymbolList.getHead ();
	for (; node; node++)
	{
		SymbolNode* symbol = *node;
		if (symbol->m_productionArray.isEmpty ())
			continue;

		if (symbol->m_flags & SymbolNodeFlagKind_Start)
			string.append ("start\n");

		if (symbol->m_flags & SymbolNodeFlagKind_Nullable)
			string.append ("nullable\n");

		if (symbol->m_flags & SymbolNodeFlagKind_Pragma)
			string.append ("pragma\n");

		string.append (symbol->m_name);
		string.append ('\n');

		size_t productionCount = symbol->m_productionArray.getCount ();

		if (symbol->m_quantifierKind)
		{
			string.append ("\t:\t");
			string.append (symbol->GrammarNode::getBnfString ());
			string.append ('\n');
		}
		else for (size_t i = 0; i < productionCount; i++)
		{
			string.append (i ? "\t|\t" : "\t:\t");
			string.append (symbol->m_productionArray [i]->getBnfString ());
			string.append ('\n');
		}

		string.append ("\t;\n\n");
	}

	return string;
}

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

void
Module::luaExport (lua::LuaState* luaState)
{
	luaExportDefines (luaState);
	luaExportClassTable (luaState);
	luaExportParseTable (luaState);
	luaState->setGlobalInteger ("Lookahead", m_lookahead);
	m_nodeMgr.luaExport (luaState);
}

void
Module::luaExportDefines (lua::LuaState* luaState)
{
	rtl::Iterator <Define> defineIt = m_defineMgr.getHead ();
	for (; defineIt; defineIt++)
	{
		Define* define = *defineIt;

		switch (define->m_kind)
		{
		case DefineKind_String:
			luaState->setGlobalString (define->m_name, define->m_stringValue);
			break;

		case DefineKind_Integer:
			luaState->setGlobalInteger (define->m_name, define->m_integerValue);
			break;
		}
	}
}

void
Module::luaExportClassTable (lua::LuaState* luaState)
{
	size_t count = m_classMgr.getCount ();

	luaState->createTable (count);

	rtl::Iterator <Class> it = m_classMgr.getHead ();
	for (size_t i = 1; it; it++, i++)
	{
		Class* cls = *it;
		cls->luaExport (luaState);
		luaState->setArrayElement (i);
	}

	luaState->setGlobal ("ClassTable");
}

void
Module::luaExportParseTable (lua::LuaState* luaState)
{
	size_t symbolCount = m_nodeMgr.m_symbolArray.getCount ();
	size_t tokenCount = m_nodeMgr.m_tokenArray.getCount ();

	luaState->createTable (symbolCount);

	for (size_t i = 0, k = 0; i < symbolCount; i++)
	{
		luaState->createTable (tokenCount);

		for (size_t j = 0; j < tokenCount; j++, k++)
		{
			Node* production = m_parseTable [k];
			luaState->setArrayElementInteger (j + 1, production ? production->m_masterIndex : -1);
		}

		luaState->setArrayElement (i + 1);
	}

	luaState->setGlobal ("ParseTable");
}

//.............................................................................
