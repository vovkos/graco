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
#include "ParseTableBuilder.h"

//..............................................................................

bool
ParseTableBuilder::build()
{
	calcGrammarProps();

	// build parse table

	size_t symbolCount = m_nodeMgr->m_symbolArray.getCount();
	size_t terminalCount = m_nodeMgr->m_tokenArray.getCount();

	m_parseTable->setCountZeroConstruct(symbolCount * terminalCount);

	// normal productions

	for (size_t i = 0; i < symbolCount; i++)
	{
		SymbolNode* node = m_nodeMgr->m_symbolArray[i];

		if (node->m_synchronizer)
		{
			ASSERT(node->m_productionArray.getCount() == 1);
			GrammarNode* production = node->m_productionArray[0];
			if (!production->isNullable())
			{
				err::setError("'synchronize' applied to a non-nullable production");
				lex::pushSrcPosError(node->m_srcPos);
				return false;
			}
		}

		if (node->m_flags & SymbolNodeFlag_User)
		{
			if (node->isNullable() && !(node->m_flags & SymbolNodeFlag_Nullable))
			{
				err::setFormatStringError(
					"'%s': nullable symbols must be explicitly marked as 'nullable'",
					node->m_name.sz()
					);
				lex::pushSrcPosError(node->m_srcPos);
				return false;
			}

			if (!node->isNullable() && (node->m_flags & SymbolNodeFlag_Nullable))
			{
				err::setFormatStringError(
					"'%s': not nullable but marked as 'nullable'",
					node->m_name.sz()
					);
				lex::pushSrcPosError(node->m_srcPos);
				return false;
			}
		}

		if (node->m_flags & SymbolNodeFlag_Pragma)
		{
			if (node->isNullable())
			{
				err::setFormatStringError(
					"'%s': pragma cannot be nullable",
					node->m_name.sz()
					);
				lex::pushSrcPosError(node->m_srcPos);
				return false;
			}

			if (node->m_firstSet.getBit(1))
			{
				err::setFormatStringError(
					"'%s': pragma cannot start with 'anytoken'",
					node->m_name.sz()
					);
				lex::pushSrcPosError(node->m_srcPos);
				return false;
			}
		}

		size_t childrenCount = node->m_productionArray.getCount();
		for (size_t j = 0; j < childrenCount; j++)
		{
			GrammarNode* production = node->m_productionArray[j];
			addProductionToParseTable(node, production);
		}
	}

	// anytoken productions

	// if
	//	production' FIRST contains anytoken
	// or
	//	production is nullable and symbol' FOLLOW contains anytoken
	// then
	//	set all parse table entries to this production

	for (size_t i = 0; i < symbolCount; i++)
	{
		SymbolNode* node = m_nodeMgr->m_symbolArray[i];

		size_t childrenCount = node->m_productionArray.getCount();
		for (size_t j = 0; j < childrenCount; j++)
		{
			GrammarNode* production = node->m_productionArray[j];

			if (production->m_firstSet.getBit(1) || (production->isNullable() && node->m_followSet.getBit(1)))
				addAnyTokenProductionToParseTable(node, production);
		}
	}

	return true;
}

void
ParseTableBuilder::addProductionToParseTable(
	SymbolNode* symbol,
	GrammarNode* production
	)
{
	size_t count;

	count = production->m_firstArray.getCount();
	for (size_t i = 0; i < count; i++)
	{
		SymbolNode* token = production->m_firstArray[i];
		addParseTableEntry(symbol, token, production);
	}

	if (!production->isNullable())
		return;

	count = symbol->m_followArray.getCount();
	for (size_t i = 0; i < count; i++)
	{
		SymbolNode* token = symbol->m_followArray[i];
		addParseTableEntry(symbol, token, production);
	}

	if (symbol->isFinal())
		addParseTableEntry(symbol, &m_nodeMgr->m_eofTokenNode, production);
}

void
ParseTableBuilder::addAnyTokenProductionToParseTable(
	SymbolNode* symbol,
	GrammarNode* production
	)
{
	size_t tokenCount = m_nodeMgr->m_tokenArray.getCount();

	// skip EOF and ANYTOKEN

	for (size_t i = 2; i < tokenCount; i++)
	{
		SymbolNode* token = m_nodeMgr->m_tokenArray[i];
		addParseTableEntry(symbol, token, production);
	}
}

size_t
ParseTableBuilder::addParseTableEntry(
	SymbolNode* symbol,
	SymbolNode* token,
	GrammarNode* production
	)
{
	size_t tokenCount = m_nodeMgr->m_tokenArray.getCount();

	Node** productionSlot = *m_parseTable + symbol->m_index * tokenCount + token->m_index;
	Node* oldProduction = *productionSlot;

	if (!oldProduction)
	{
		*productionSlot = production;
		return 0;
	}

	if (oldProduction == production)
		return 0;

	ConflictNode* conflict;
	if (oldProduction->m_nodeKind != NodeKind_Conflict)
	{
		GrammarNode* firstProduction = (GrammarNode*)oldProduction;

		conflict = m_nodeMgr->createConflictNode();
		conflict->m_symbol = symbol;
		conflict->m_token = token;

		conflict->m_productionArray.setCount(2);
		conflict->m_productionArray[0] = firstProduction;
		conflict->m_productionArray[1] = production;
		conflict->m_lookaheadLimit = AXL_MAX(firstProduction->m_lookaheadLimit, production->m_lookaheadLimit);

		*productionSlot = conflict; // later will be replaced with lookahead DFA
	}
	else
	{
		conflict = (ConflictNode*)oldProduction;
		size_t count = conflict->m_productionArray.getCount();

		size_t i = 0;
		for (; i < count; i++)
			if (conflict->m_productionArray[i] == production)
				break;

		if (i >= count) // not found
		{
			conflict->m_productionArray.append(production);
			if (conflict->m_lookaheadLimit < production->m_lookaheadLimit)
				conflict->m_lookaheadLimit = production->m_lookaheadLimit;
		}
	}

	return conflict->m_productionArray.getCount();
}

void
ParseTableBuilder::calcGrammarProps()
{
	bool hasChanged;

	GrammarNode* startSymbol = m_nodeMgr->m_symbolArray[0];
	startSymbol->markFinal();

	size_t tokenCount = m_nodeMgr->m_tokenArray.getCount();
	size_t symbolCount = m_nodeMgr->m_symbolArray.getCount();

	for (size_t i = 1; i < tokenCount; i++)
	{
		SymbolNode* node = m_nodeMgr->m_tokenArray[i];
		node->m_firstSet.setBitResize(node->m_masterIndex, true);
	}

	for (size_t i = 0; i < symbolCount; i++)
	{
		SymbolNode* node = m_nodeMgr->m_symbolArray[i];
		node->initializeFirstFollowSets(tokenCount);

		if (node->m_resolver)
			node->m_resolver->m_followSet.setBitResize(1); // set anytoken FOLLOW for resolver

		if (node->m_flags & SymbolNodeFlag_Start)
			node->markFinal();
	}

	sl::Iterator<SequenceNode> sequenceIt = m_nodeMgr->m_sequenceList.getHead();
	for (; sequenceIt; sequenceIt++)
		sequenceIt->initializeFirstFollowSets(tokenCount);

	sl::Iterator<BeaconNode> beaconIt = m_nodeMgr->m_beaconList.getHead();
	for (; beaconIt; beaconIt++)
		beaconIt->initializeFirstFollowSets(tokenCount);

	sl::Iterator<GrammarNode> wrNodeIt = m_nodeMgr->m_weaklyReachableNodeList.getHead();
	for (; wrNodeIt; wrNodeIt++)
		wrNodeIt->initializeFirstFollowSets(tokenCount);

	do
	{
		hasChanged = false;

		for (size_t i = 0; i < symbolCount; i++)
			if (m_nodeMgr->m_symbolArray[i]->propagateGrammarProps())
				hasChanged = true;

		sequenceIt = m_nodeMgr->m_sequenceList.getHead();
		for (; sequenceIt; sequenceIt++)
			if (sequenceIt->propagateGrammarProps())
				hasChanged = true;

		beaconIt = m_nodeMgr->m_beaconList.getHead();
		for (; beaconIt; beaconIt++)
			if (beaconIt->propagateGrammarProps())
				hasChanged = true;

		wrNodeIt = m_nodeMgr->m_weaklyReachableNodeList.getHead();
		for (; wrNodeIt; wrNodeIt++)
			if (wrNodeIt->propagateGrammarProps())
				hasChanged = true;

	} while (hasChanged);

	m_nodeMgr->m_anyTokenNode.buildFirstFollowArrays(m_nodeMgr->m_tokenArray);

	for (size_t i = 2; i < tokenCount; i++)
		m_nodeMgr->m_tokenArray[i]->buildFirstFollowArrays(m_nodeMgr->m_tokenArray);

	for (size_t i = 0; i < symbolCount; i++)
		m_nodeMgr->m_symbolArray[i]->buildFirstFollowArrays(m_nodeMgr->m_tokenArray);

	sequenceIt = m_nodeMgr->m_sequenceList.getHead();
	for (; sequenceIt; sequenceIt++)
		sequenceIt->buildFirstFollowArrays(m_nodeMgr->m_tokenArray);

	beaconIt = m_nodeMgr->m_beaconList.getHead();
	for (; beaconIt; beaconIt++)
		beaconIt->buildFirstFollowArrays(m_nodeMgr->m_tokenArray);

	wrNodeIt = m_nodeMgr->m_weaklyReachableNodeList.getHead();
	for (; wrNodeIt; wrNodeIt++)
		wrNodeIt->buildFirstFollowArrays(m_nodeMgr->m_tokenArray);
}

//..............................................................................
