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
ParseTableBuilder::build ()
{
	calcFirstFollow ();

	// build parse table

	size_t symbolCount = m_nodeMgr->m_symbolArray.getCount ();
	size_t terminalCount = m_nodeMgr->m_tokenArray.getCount ();

	m_parseTable->setCountZeroConstruct (symbolCount * terminalCount);

	// normal productions

	for (size_t i = 0; i < symbolCount; i++)
	{
		SymbolNode* node = m_nodeMgr->m_symbolArray [i];

		if (node->m_flags & SymbolNodeFlag_Named)
		{
			if (node->isNullable () && !(node->m_flags & SymbolNodeFlag_Nullable))
			{
				err::setFormatStringError (
					"'%s': nullable symbols must be explicitly marked as 'nullable'",
					node->m_name.sz ()
					);
				lex::pushSrcPosError (node->m_srcPos);
				return false;
			}

			if (!node->isNullable () && (node->m_flags & SymbolNodeFlag_Nullable))
			{
				err::setFormatStringError (
					"'%s': marked as 'nullable' but is not nullable",
					node->m_name.sz ()
					);
				lex::pushSrcPosError (node->m_srcPos);
				return false;
			}
		}

		if (node->m_flags & SymbolNodeFlag_Pragma)
		{
			if (node->isNullable ())
			{
				err::setFormatStringError (
					"'%s': pragma cannot be nullable",
					node->m_name.sz ()
					);
				lex::pushSrcPosError (node->m_srcPos);
				return false;
			}

			if (node->m_firstSet.getBit (1))
			{
				err::setFormatStringError (
					"'%s': pragma cannot start with 'anytoken'",
					node->m_name.sz ()
					);
				lex::pushSrcPosError (node->m_srcPos);
				return false;
			}
		}

		size_t childrenCount = node->m_productionArray.getCount ();
		for (size_t j = 0; j < childrenCount; j++)
		{
			GrammarNode* production = node->m_productionArray [j];
			addProductionToParseTable (node, production);
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
		SymbolNode* node = m_nodeMgr->m_symbolArray [i];

		size_t childrenCount = node->m_productionArray.getCount ();
		for (size_t j = 0; j < childrenCount; j++)
		{
			GrammarNode* production = node->m_productionArray [j];

			if (production->m_firstSet.getBit (1) || (production->isNullable () && node->m_followSet.getBit (1)))
				addAnyTokenProductionToParseTable (node, production);
		}
	}

	return true;
}

void
ParseTableBuilder::addProductionToParseTable (
	SymbolNode* symbol,
	GrammarNode* production
	)
{
	size_t count;

	count = production->m_firstArray.getCount ();
	for (size_t i = 0; i < count; i++)
	{
		SymbolNode* token = production->m_firstArray [i];
		addParseTableEntry (symbol, token, production);
	}

	if (!production->isNullable ())
		return;

	count = symbol->m_followArray.getCount ();
	for (size_t i = 0; i < count; i++)
	{
		SymbolNode* token = symbol->m_followArray [i];
		addParseTableEntry (symbol, token, production);
	}

	if (symbol->isFinal ())
		addParseTableEntry (symbol, &m_nodeMgr->m_eofTokenNode, production);
}

void
ParseTableBuilder::addAnyTokenProductionToParseTable (
	SymbolNode* symbol,
	GrammarNode* production
	)
{
	size_t tokenCount = m_nodeMgr->m_tokenArray.getCount ();

	// skip EOF and ANYTOKEN

	for (size_t i = 2; i < tokenCount; i++)
	{
		SymbolNode* token = m_nodeMgr->m_tokenArray [i];
		addParseTableEntry (symbol, token, production);
	}
}

size_t
ParseTableBuilder::addParseTableEntry (
	SymbolNode* symbol,
	SymbolNode* token,
	GrammarNode* production
	)
{
	size_t tokenCount = m_nodeMgr->m_tokenArray.getCount ();

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
	if (oldProduction->m_kind != NodeKind_Conflict)
	{
		conflict = m_nodeMgr->createConflictNode ();
		conflict->m_symbol = symbol;
		conflict->m_token = token;

		conflict->m_productionArray.setCount (2);
		conflict->m_productionArray [0] = (GrammarNode*) oldProduction;
		conflict->m_productionArray [1] = production;

		*productionSlot = conflict; // later will be replaced with lookahead DFA
	}
	else
	{
		conflict = (ConflictNode*) oldProduction;
		size_t count = conflict->m_productionArray.getCount ();

		size_t i = 0;
		for (; i < count; i++)
			if (conflict->m_productionArray [i] == production)
				break;

		if (i >= count) // not found
			conflict->m_productionArray.append (production);
	}

	return conflict->m_productionArray.getCount ();
}

bool
propagateParentChild (
	GrammarNode* parent,
	GrammarNode* child
	)
{
	bool hasChanged = false;

	if (parent->m_firstSet.merge (child->m_firstSet, sl::BitOpKind_Or))
		hasChanged = true;

	if (child->m_followSet.merge (parent->m_followSet, sl::BitOpKind_Or))
		hasChanged = true;

	if (child->isNullable ())
		if (parent->markNullable ())
			hasChanged = true;

	if (parent->isFinal ())
		if (child->markFinal ())
			hasChanged = true;

	return hasChanged;
}

void
ParseTableBuilder::calcFirstFollow ()
{
	bool hasChanged;

	GrammarNode* startSymbol = m_nodeMgr->m_symbolArray [0];
	startSymbol->markFinal ();

	size_t tokenCount = m_nodeMgr->m_tokenArray.getCount ();
	size_t symbolCount = m_nodeMgr->m_symbolArray.getCount ();

	for (size_t i = 1; i < tokenCount; i++)
	{
		SymbolNode* node = m_nodeMgr->m_tokenArray [i];
		node->m_firstSet.setBitResize (node->m_masterIndex, true);
	}

	for (size_t i = 0; i < symbolCount; i++)
	{
		SymbolNode* node = m_nodeMgr->m_symbolArray [i];
		node->m_firstSet.setBitCount (tokenCount);
		node->m_followSet.setBitCount (tokenCount);

		if (node->m_resolver)
			node->m_resolver->m_followSet.setBitResize (1); // set anytoken FOLLOW for resolver

		if (node->m_flags & SymbolNodeFlag_Start)
			node->markFinal ();
	}

	sl::Iterator <SequenceNode> sequence = m_nodeMgr->m_sequenceList.getHead ();
	for (; sequence; sequence++)
	{
		sequence->m_firstSet.setBitCount (tokenCount);
		sequence->m_followSet.setBitCount (tokenCount);
	}

	sl::Iterator <BeaconNode> beacon = m_nodeMgr->m_beaconList.getHead ();
	for (; beacon; beacon++)
	{
		beacon->m_firstSet.setBitCount (tokenCount);
		beacon->m_followSet.setBitCount (tokenCount);
	}

	do
	{
		hasChanged = false;

		for (size_t i = 0; i < symbolCount; i++)
		{
			SymbolNode* node = m_nodeMgr->m_symbolArray [i];
			size_t childrenCount = node->m_productionArray.getCount ();

			for (size_t j = 0; j < childrenCount; j++)
			{
				GrammarNode* production = node->m_productionArray [j];
				if (propagateParentChild (node, production))
					hasChanged = true;
			}
		}

		sequence = m_nodeMgr->m_sequenceList.getHead ();
		for (; sequence; sequence++)
		{
			SequenceNode* node = *sequence;
			size_t childrenCount = node->m_sequence.getCount ();

			// FIRST between parent-child

			bool isNullable = true;
			for (size_t j = 0; j < childrenCount; j++)
			{
				GrammarNode* child = node->m_sequence [j];
				if (node->m_firstSet.merge (child->m_firstSet, sl::BitOpKind_Or))
					hasChanged = true;

				if (!child->isNullable ())
				{
					isNullable = false;
					break;
				}
			}

			if (isNullable) // all nullable
				if (node->markNullable ())
					hasChanged = true;

			// FOLLOW between parent-child

			for (intptr_t j = childrenCount - 1; j >= 0; j--)
			{
				GrammarNode* child = node->m_sequence [j];
				if (child->m_followSet.merge (node->m_followSet, sl::BitOpKind_Or))
					hasChanged = true;

				if (node->isFinal ())
					if (child->markFinal ())
						hasChanged = true;

				if (!child->isNullable ())
					break;
			}

			// FOLLOW between child-child

			if (childrenCount >= 2)
				for (size_t j = 0; j < childrenCount - 1; j++)
				{
					GrammarNode* child = node->m_sequence [j];
					for (size_t k = j + 1; k < childrenCount; k++)
					{
						GrammarNode* next = node->m_sequence [k];
						if (child->m_followSet.merge (next->m_firstSet, sl::BitOpKind_Or))
							hasChanged = true;

						if (!next->isNullable ())
							break;
					}
				}
		}

		beacon = m_nodeMgr->m_beaconList.getHead ();
		for (; beacon; beacon++)
		{
			if (propagateParentChild (*beacon, beacon->m_target))
				hasChanged = true;
		}

	} while (hasChanged);

	buildFirstFollowArrays (&m_nodeMgr->m_anyTokenNode);

	for (size_t i = 2; i < tokenCount; i++)
	{
		SymbolNode* node = m_nodeMgr->m_tokenArray [i];
		buildFirstFollowArrays (node);
	}

	for (size_t i = 0; i < symbolCount; i++)
	{
		SymbolNode* node = m_nodeMgr->m_symbolArray [i];
		buildFirstFollowArrays (node);
	}

	sequence = m_nodeMgr->m_sequenceList.getHead ();
	for (; sequence; sequence++)
		buildFirstFollowArrays (*sequence);

	beacon = m_nodeMgr->m_beaconList.getHead ();
	for (; beacon; beacon++)
		buildFirstFollowArrays (*beacon);
}

void
ParseTableBuilder::buildFirstFollowArrays (GrammarNode* node)
{
	node->m_firstArray.clear ();
	node->m_followArray.clear ();

	for (
		size_t i = node->m_firstSet.findBit (0);
		i != -1;
		i = node->m_firstSet.findBit (i + 1)
		)
	{
		SymbolNode* token = m_nodeMgr->m_tokenArray [i];
		node->m_firstArray.append (token);
	}

	for (
		size_t i = node->m_followSet.findBit (0);
		i != -1;
		i = node->m_followSet.findBit (i + 1)
		)
	{
		SymbolNode* token = m_nodeMgr->m_tokenArray [i];
		node->m_followArray.append (token);
	}
}

//..............................................................................
