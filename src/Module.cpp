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

#include "pch.h"
#include "Module.h"
#include "ProductionBuilder.h"
#include "ParseTableBuilder.h"
#include "LaDfaBuilder.h"

//..............................................................................

Module::Module() {
	m_maxUsedLookahead = 1;
}

void
Module::clear() {
	m_sourceCache.clear();
	m_parseTable.clear();
	m_defineMgr.clear();
	m_nodeMgr.clear();
	m_importList.clear();
	m_maxUsedLookahead = 1;
}

bool
Module::build(const CmdLine* cmdLine) {
	bool result;

	m_parseTable.clear();

	if (m_nodeMgr.isEmpty()) {
		err::setError("grammar is empty");
		return false;
	}

	// check reachability from start symbols

	m_nodeMgr.markReachableNodes();
	m_nodeMgr.deleteUnreachableNodes();

	// after marking we can assign default start symbol

	if (!m_nodeMgr.m_primaryStartSymbol)
		m_nodeMgr.m_primaryStartSymbol = *m_nodeMgr.m_namedSymbolList.getHead();

	m_nodeMgr.indexTokens();
	m_nodeMgr.indexSymbols();
	m_nodeMgr.indexSequences();
	m_nodeMgr.indexActions();
	m_nodeMgr.indexArguments();

	// build productions

	ProductionBuilder productionBuilder(&m_nodeMgr);

	sl::Iterator<SymbolNode> symbolIt = m_nodeMgr.m_namedSymbolList.getHead();
	for (; symbolIt; symbolIt++) {
		SymbolNode* symbol = *symbolIt;

		if (symbol->m_resolver) {
			ASSERT(symbol->m_resolver->m_productionArray.getCount() == 1);
			result = productionBuilder.build(symbol->m_resolver, &symbol->m_resolver->m_productionArray.rwi()[0]);
			if (!result)
				return false;
		}

		size_t count = symbol->m_productionArray.getCount();
		sl::Array<GrammarNode*>::Rwi rwi = symbol->m_productionArray;
		for (size_t i = 0; i < count; i++) {
			result = productionBuilder.build(symbol, &rwi[i]);
			if (!result)
				return false;
		}
	}

	m_nodeMgr.indexBeacons(); // index only after unneeded beacons have been removed
	m_nodeMgr.indexDispatchers();

	// build parse table

	ParseTableBuilder parseTableBuilder(&m_nodeMgr, &m_parseTable);
	result = parseTableBuilder.build();
	if (!result)
		return false;

	// resolve conflicts

	LaDfaBuilder builder(cmdLine, &m_nodeMgr, &m_parseTable);
	sl::Iterator<ConflictNode> conflictIt = m_nodeMgr.m_conflictList.getHead();
	for (; conflictIt; conflictIt++) {
		ConflictNode* conflict = *conflictIt;

		conflict->m_resultNode = builder.build(conflict);
		if (!conflict->m_resultNode)
			return false;
	}

	// replace conflicts with dfas or with direct productions (could happen in conflicts with epsilon productions or with anytoken)

	sl::Array<Node*>::Rwi rwi = m_parseTable;
	size_t tokenCount = m_nodeMgr.m_tokenArray.getCount();
	conflictIt = m_nodeMgr.m_conflictList.getHead();
	for (; conflictIt; conflictIt++) {
		ConflictNode* conflict = *conflictIt;
		Node** production = &rwi[conflict->m_symbol->m_index * tokenCount + conflict->m_token->m_index];
		ASSERT(*production == conflict);
		*production = conflict->m_resultNode;
	}

	symbolIt = m_nodeMgr.m_namedSymbolList.getHead();
	for (; symbolIt; symbolIt++)
		if (symbolIt->m_resolver && !(symbolIt->m_flags & SymbolNodeFlag_ResolverUsed)) {
			err::setError("unused resolver");
			lex::pushSrcPosError(symbolIt->m_srcPos);
			return false;
		}

	m_nodeMgr.indexLaDfaNodes();
	m_maxUsedLookahead = builder.getMaxUsedLookahead();
	return true;
}

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

void
Module::trace() {
	printf("LOOKAHEAD: %d\n", m_maxUsedLookahead);
	m_nodeMgr.trace();
}

bool
Module::writeBnfFile(
	const sl::StringRef& fileName,
	BnfDialect dialect
) {
	sl::String bufferString;

	io::File file;
	bool result =
		file.open(fileName) &&
		file.setSize(0);

	if (!result)
		return false;

	sl::String string = generateBnfString(dialect);
	file.write(string, string.getLength());
	return true;
}

sl::String
Module::generateBnfString(BnfDialect dialect) {
	sl::String string;
	sl::String sequenceString;
	sl::StringRef orString = "\t|\t";
	sl::StringRef equString = dialect == BnfDialect_Graco ? "\t:\t" : "\t::=\t";
	sl::StringRef termString = dialect == BnfDialect_Graco ? "\t;\n\n" : "\n";

	sl::Iterator<SymbolNode> node = m_nodeMgr.m_namedSymbolList.getHead();
	for (; node; node++) {
		SymbolNode* symbol = *node;
		if (symbol->m_productionArray.isEmpty())
			continue;

		if (dialect == BnfDialect_Graco) {
			if (symbol->m_flags & SymbolNodeFlag_Start)
				string.append("start\n");

			if (symbol->m_flags & SymbolNodeFlag_Nullable)
				string.append("nullable\n");

			if (symbol->m_flags & SymbolNodeFlag_Pragma)
				string.append("pragma\n");
		}

		string.append(symbol->m_name);
		string.append('\n');

		size_t productionCount = symbol->m_productionArray.getCount();

		if (symbol->m_quantifierKind) {
			string.append(equString);
			string.append(symbol->GrammarNode::getBnfString());
			string.append('\n');
		} else for (size_t i = 0; i < productionCount; i++) {
			string.append(i ? orString : equString);
			string.append(symbol->m_productionArray[i]->getBnfString());
			string.append('\n');
		}

		string.append(termString);
	}

	return string;
}

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

void
Module::luaExport(lua::LuaState* luaState) {
	m_defineMgr.luaExport(luaState);
	m_nodeMgr.luaExport(luaState);
	luaExportParseTable(luaState);
}

void
Module::luaExportParseTable(lua::LuaState* luaState) {
	size_t symbolCount = m_nodeMgr.m_symbolArray.getCount();
	size_t tokenCount = m_nodeMgr.m_tokenArray.getCount();

	luaState->createTable(symbolCount);

	for (size_t i = 0, k = 0; i < symbolCount; i++) {
		luaState->createTable(tokenCount);

		for (size_t j = 0; j < tokenCount; j++, k++) {
			Node* production = m_parseTable[k];
			luaState->setArrayElementInteger(j + 1, production ? production->m_masterIndex : -1);
		}

		luaState->setArrayElement(i + 1);
	}

	luaState->setGlobal("ParseTable");
}

//..............................................................................
