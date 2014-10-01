#include "pch.h"
#include "ProductionBuilder.h"

//.............................................................................

ProductionBuilder::ProductionBuilder (NodeMgr* nodeMgr)
{
	m_nodeMgr = nodeMgr;
	m_symbol = NULL;
	m_production = NULL;
	m_dispatcher = NULL;
	m_resolver = NULL;
}

GrammarNode*
ProductionBuilder::build (
	SymbolNode* symbol,
	GrammarNode* production
	)
{
	bool result;
	size_t formalArgCount;

	switch (production->m_kind)
	{
	case NodeKind_Epsilon:
	case NodeKind_Token:
		return production;

	case NodeKind_Symbol:
		if (production->m_flags & SymbolNodeFlagKind_Named)
			return production;

		// else fall through

	case NodeKind_Action:
	case NodeKind_Sequence:
		break;

	case NodeKind_Beacon:
		BeaconNode* beacon;
		
		beacon = (BeaconNode*) production;

		formalArgCount = beacon->m_target->m_argNameList.getCount ();		 
		if (formalArgCount)
		{
			err::setFormatStringError (
				"'%s' takes %d arguments, passed none", 
				beacon->m_target->m_name.cc (), // thanks a lot gcc 
				formalArgCount
				);
			err::pushSrcPosError (beacon->m_srcPos);
			return NULL;
		}

		production = beacon->m_target;
		m_nodeMgr->deleteBeaconNode (beacon);
		return production;

	default:
		ASSERT (false);
		return NULL;
	}

	m_actionArray.clear ();
	m_argumentArray.clear ();
	m_beaconArray.clear ();
	m_beaconDeleteArray.clear ();
	m_beaconMap.clear ();

	m_symbol = symbol;
	m_production = production;
	m_dispatcher = NULL;
	m_resolver = NULL;

	result = scan (production);
	if (!result)
		return NULL;
	
	result = processAllUserCode ();
	if (!result)
	{
		ensureSrcPosError ();
		return NULL;
	}

	findAndReplaceUnusedBeacons (production);
	
	size_t count = m_beaconDeleteArray.getCount ();
	for (size_t i = 0; i < count; i++)
		m_nodeMgr->deleteBeaconNode (m_beaconDeleteArray [i]);

	return production;
}

bool
ProductionBuilder::processAllUserCode ()
{
	bool result;

	size_t count = m_actionArray.getCount ();
	for (size_t i = 0; i < count; i++)
	{
		ActionNode* node = m_actionArray [i];
		if (node->m_flags & UserNodeFlagKind_UserCodeProcessed)
			continue;

		result = processUserCode (node->m_srcPos, &node->m_userCode, node->m_resolver);
		if (!result)
			return false;

		node->m_flags |= UserNodeFlagKind_UserCodeProcessed;
		node->m_dispatcher = m_dispatcher;
	}

	count = m_argumentArray.getCount ();
	for (size_t i = 0; i < count; i++)
	{
		ArgumentNode* node = m_argumentArray [i];
		if (node->m_flags & UserNodeFlagKind_UserCodeProcessed)
			continue;

		rtl::BoxIterator <rtl::String> it = node->m_argValueList.getHead ();
		for (; it; it++)
		{
			result = processUserCode (node->m_srcPos, &*it, node->m_resolver);
			if (!result)
				return false;
		}

		node->m_flags |= UserNodeFlagKind_UserCodeProcessed;
		node->m_dispatcher = m_dispatcher;
	}

	return true;
}

bool
ProductionBuilder::scan (GrammarNode* node)
{
	bool result;

	if (node->m_flags & NodeFlagKind_RecursionStopper)
		return true;

	SymbolNode* symbol;
	SequenceNode* sequence;
	ActionNode* action;
	ArgumentNode* argument;

	size_t childrenCount;

	switch (node->m_kind)
	{
	case NodeKind_Epsilon:
	case NodeKind_Token:
		break;

	case NodeKind_Symbol:
		if (node->m_flags & SymbolNodeFlagKind_Named) 
			break;

		symbol = (SymbolNode*) node;
		symbol->m_flags |= NodeFlagKind_RecursionStopper;

		if (symbol->m_resolver)
		{
			GrammarNode* resolver = m_resolver;
			m_resolver = symbol->m_resolver;

			result = scan (symbol->m_resolver);
			if (!result)
				return false;

			m_resolver = resolver;
		}

		childrenCount = symbol->m_productionArray.getCount ();
		for (size_t i = 0; i < childrenCount; i++)
		{
			GrammarNode* child = symbol->m_productionArray [i];
			result = scan (child);
			if (!result)
				return false;
		}

		symbol->m_flags &= ~NodeFlagKind_RecursionStopper;
		break;

	case NodeKind_Sequence:
		sequence = (SequenceNode*) node;
		sequence->m_flags |= NodeFlagKind_RecursionStopper;

		childrenCount = sequence->m_sequence.getCount ();
		for (size_t i = 0; i < childrenCount; i++)
		{
			GrammarNode* child = sequence->m_sequence [i];
			result = scan (child);
			if (!result)
				return false;
		}

		sequence->m_flags &= ~NodeFlagKind_RecursionStopper;
		break;

	case NodeKind_Beacon:
		result = addBeacon ((BeaconNode*) node);
		if (!result)
			return false;

		break;

	case NodeKind_Action:
		action = (ActionNode*) node;
		action->m_productionSymbol = m_symbol;
		action->m_resolver = m_resolver;
		m_actionArray.append (action);
		break;

	case NodeKind_Argument:
		argument = (ArgumentNode*) node;
		argument->m_productionSymbol = m_symbol;
		argument->m_resolver = m_resolver;
		m_argumentArray.append (argument);
		break;

	default:
		ASSERT (false);
	}

	return true;
}

bool
ProductionBuilder::addBeacon (BeaconNode* beacon)
{
	if (beacon->m_flags & BeaconNodeFlagKind_Added)
		return true;

	if (!beacon->m_label.isEmpty ())
	{
		rtl::StringHashTableMapIterator <BeaconNode*> it = m_beaconMap.visit (beacon->m_label);
		if (!it->m_value)
			it->m_value = beacon;
	}

	if (beacon->m_target->m_kind == NodeKind_Symbol)
	{
		SymbolNode* node = (SymbolNode*) beacon->m_target;
		size_t formalArgCount = node->m_argNameList.getCount ();
		size_t actualArgCount = beacon->m_argument ? beacon->m_argument->m_argValueList.getCount () : 0;
		
		if (formalArgCount != actualArgCount)
		{
			err::setFormatStringError (
				"'%s' takes %d arguments, passed %d", 
				node->m_name.cc (), 
				formalArgCount, 
				actualArgCount
				);
			err::pushSrcPosError (beacon->m_srcPos);
			return false;
		}
	}

	m_beaconArray.append (beacon);
	beacon->m_flags |= BeaconNodeFlagKind_Added;
	beacon->m_resolver = m_resolver;
	return true;
}

void
ProductionBuilder::findAndReplaceUnusedBeacons (GrammarNode*& node)
{
	if (node->m_flags & NodeFlagKind_RecursionStopper)
		return;

	SymbolNode* symbol;
	SequenceNode* sequence;
	BeaconNode* beacon;

	size_t count;

	switch (node->m_kind)
	{
	case NodeKind_Epsilon:
	case NodeKind_Token:
	case NodeKind_Action:
	case NodeKind_Argument:
		break;

	case NodeKind_Beacon:
		beacon = (BeaconNode*) node;
		if (beacon->m_slotIndex != -1)
			break;

		if (!(beacon->m_flags & BeaconNodeFlagKind_Deleted))
		{
			m_beaconDeleteArray.append (beacon);
			beacon->m_flags |= BeaconNodeFlagKind_Deleted;
		}	

		node = beacon->m_target; // replace
		break;

	case NodeKind_Symbol:
		if (node->m_flags & SymbolNodeFlagKind_Named) 
			break;

		symbol = (SymbolNode*) node;
		symbol->m_flags |= NodeFlagKind_RecursionStopper;

		if (symbol->m_resolver)
			findAndReplaceUnusedBeacons (symbol->m_resolver);

		count = symbol->m_productionArray.getCount ();
		for (size_t i = 0; i < count; i++)
			findAndReplaceUnusedBeacons (symbol->m_productionArray [i]);

		symbol->m_flags &= ~NodeFlagKind_RecursionStopper;
		break;

	case NodeKind_Sequence:
		sequence = (SequenceNode*) node;
		sequence->m_flags |= NodeFlagKind_RecursionStopper;

		count = sequence->m_sequence.getCount ();
		for (size_t i = 0; i < count; i++)
			findAndReplaceUnusedBeacons (sequence->m_sequence [i]);

		sequence->m_flags &= ~NodeFlagKind_RecursionStopper;
		break;

	default:
		ASSERT (false);
	}
}

ProductionBuilder::VariableKind
ProductionBuilder::findVariable (
	int index,
	BeaconNode** beacon_o
	)
{
	if (index == 0)
		return VariableKind_This;

	size_t beaconIndex = index - 1;
	size_t beaconCount = m_beaconArray.getCount ();
	
	if (beaconIndex >= beaconCount)
	{
		err::setFormatStringError ("locator '$%d' is out of range ($1..$%d)", beaconIndex + 1, beaconCount);
		return VariableKind_Undefined;
	}

	BeaconNode* beacon = m_beaconArray [beaconIndex];
	*beacon_o = beacon;
	return beacon->m_target->m_kind == NodeKind_Token ? 
		VariableKind_TokenBeacon :
		VariableKind_SymbolBeacon;
}

ProductionBuilder::VariableKind
ProductionBuilder::findVariable (
	const char* name,
	BeaconNode** beacon_o
	)
{
	rtl::StringHashTableMapIterator <BeaconNode*> it = m_beaconMap.find (name);
	if (it)
	{
		BeaconNode* beacon = it->m_value;
		*beacon_o = beacon;
		return beacon->m_target->m_kind == NodeKind_Token ? 
			VariableKind_TokenBeacon :
			VariableKind_SymbolBeacon;
	}

	rtl::HashTableIterator <const char*> it2 = m_symbol->m_localNameSet.find (name);
	if (it2)
		return VariableKind_Local;

	it2 = m_symbol->m_argNameSet.find (name);
	if (it2)
		return VariableKind_Arg;

	err::setFormatStringError ("locator '$%s' not found", name);
	return VariableKind_Undefined;
}

bool
ProductionBuilder::processUserCode (
	lex::SrcPos& srcPos,
	rtl::String* userCode,
	GrammarNode* resolver
	)
{
	const Token* token;

	rtl::String resultString;

	Lexer::create (
		getMachineState (LexerMachineKind_UserCode2ndPass), 
		srcPos.m_filePath, 
		*userCode
		);

	setLineCol (srcPos);

	const char* p = *userCode;

	VariableKind variableKind;
	BeaconNode* beacon;

	for (;;)
	{
		token = getToken ();
		if (token->m_token <= 0)
			break;

		switch (token->m_token)
		{
		case TokenKind_Integer:
			variableKind = findVariable (token->m_data.m_integer, &beacon);
			break;

		case TokenKind_Identifier:
			variableKind = findVariable (token->m_data.m_string, &beacon);
			break;

		default:
			nextToken ();
			continue;
		}

		if (!variableKind)
			return false;

		resultString.append (p, token->m_pos.m_p - p);

		switch (variableKind)
		{
		case VariableKind_SymbolBeacon:
			if (beacon->m_target->m_flags & SymbolNodeFlagKind_NoAst)
			{
				err::setFormatStringError (
					"'%s' is declared as 'noast' and cannot be referenced from user actions", 
					beacon->m_target->m_name.cc () 
					);
				return false;
			}

			// and fall through

		case VariableKind_TokenBeacon:
			if (beacon->m_resolver != resolver)
			{
				err::setFormatStringError (
					"cross-resolver reference to locator '%s'", 
					token->getText ().cc () 
					);
				return false;
			}

			if (beacon->m_slotIndex == -1)
			{
				if (!m_dispatcher)
					m_dispatcher = m_nodeMgr->createDispatcherNode (m_symbol);

				beacon->m_slotIndex = m_dispatcher->m_beaconArray.getCount ();
				m_dispatcher->m_beaconArray.append (beacon);
			}

			resultString.appendFormat ("$%d", beacon->m_slotIndex);
			break;

		case VariableKind_This:
			if (resolver)
			{
				err::setFormatStringError ("resolvers cannot reference left side of production");
				return false;
			}

			resultString.append ('$');
			break;

		case VariableKind_Arg:
			if (resolver)
			{
				err::setFormatStringError ("resolvers cannot reference arguments");
				return false;
			}

			resultString.appendFormat (
				"$arg.%s", 
				token->m_data.m_string.cc () 
				);
			break;

		case VariableKind_Local:
			if (resolver)
			{
				err::setFormatStringError ("resolvers cannot reference locals");
				return false;
			}

			resultString.appendFormat (
				"$local.%s", 
				token->m_data.m_string.cc () 
				);
			break;
		}

		p = token->m_pos.m_p + token->m_pos.m_length;

		nextToken ();
	}

	ASSERT (!token->m_token);
	resultString.append (p, token->m_pos.m_p - p);

	*userCode = resultString;
	return true;
}

//.............................................................................
