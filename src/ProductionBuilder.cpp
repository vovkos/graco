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
#include "ProductionBuilder.h"

//..............................................................................

ProductionBuilder::ProductionBuilder(NodeMgr* nodeMgr) {
	m_nodeMgr = nodeMgr;
	m_symbol = NULL;
	m_dispatcher = NULL;
}

bool
ProductionBuilder::build(
	SymbolNode* symbol,
	GrammarNode** production
) {
	GrammarNode* result = build(symbol, *production);
	if (!result)
		return false;

	*production = result;
	return true;
}

GrammarNode*
ProductionBuilder::build(
	SymbolNode* symbol,
	GrammarNode* production
) {
	bool result;
	size_t paramCount;

	switch (production->m_nodeKind) {
	case NodeKind_Epsilon:
	case NodeKind_Token:
		return production;

	case NodeKind_Symbol:
		if (production->m_flags & SymbolNodeFlag_User)
			return production;

		// else fall through

	case NodeKind_Action:
	case NodeKind_Sequence:
		break;

	case NodeKind_Beacon:
		BeaconNode* beacon;
		beacon = (BeaconNode*)production;

		paramCount = beacon->m_target->m_paramNameList.getCount();
		if (paramCount) {
			err::setFormatStringError(
				"'%s' takes %d parameters, passed none",
				beacon->m_target->m_name.sz(),
				paramCount
			);
			lex::pushSrcPosError(beacon->m_srcPos);
			return NULL;
		}

		production = beacon->m_target;
		m_nodeMgr->deleteBeaconNode(beacon);
		return production;

	default:
		ASSERT(false);
		return NULL;
	}

	m_actionArray.clear();
	m_argumentArray.clear();
	m_beaconArray.clear();
	m_beaconDeleteArray.clear();
	m_beaconMap.clear();

	m_symbol = symbol;
	m_dispatcher = NULL;

	result = scan(production);
	if (!result)
		return NULL;

	result = processAllUserCode();
	if (!result)
		return NULL;

	findAndReplaceUnusedBeacons(&production);

	size_t count = m_beaconDeleteArray.getCount();
	for (size_t i = 0; i < count; i++)
		m_nodeMgr->deleteBeaconNode(m_beaconDeleteArray[i]);

	return production;
}

bool
ProductionBuilder::processAllUserCode() {
	bool result;

	size_t count = m_actionArray.getCount();
	for (size_t i = 0; i < count; i++) {
		ActionNode* node = m_actionArray[i];
		if (node->m_flags & UserNodeFlag_UserCodeProcessed)
			continue;

		result = processUserCode(node->m_srcPos, &node->m_userCode);
		if (!result) {
			lex::ensureSrcPosError(node->m_srcPos);
			return false;
		}

		node->m_flags |= UserNodeFlag_UserCodeProcessed;
		node->m_dispatcher = m_dispatcher;
	}

	count = m_argumentArray.getCount();
	for (size_t i = 0; i < count; i++) {
		ArgumentNode* node = m_argumentArray[i];
		if (node->m_flags & UserNodeFlag_UserCodeProcessed)
			continue;

		sl::BoxIterator<sl::String> it = node->m_argValueList.getHead();
		for (; it; it++) {
			result = processUserCode(node->m_srcPos, &*it);
			if (!result) {
				lex::ensureSrcPosError(node->m_srcPos);
				return false;
			}
		}

		node->m_flags |= UserNodeFlag_UserCodeProcessed;
		node->m_dispatcher = m_dispatcher;
	}

	return true;
}

bool
ProductionBuilder::scan(GrammarNode* node) {
	bool result;

	if (node->m_flags & NodeFlag_RecursionStopper)
		return true;

	SymbolNode* symbol;
	SequenceNode* sequence;
	ActionNode* action;
	ArgumentNode* argument;

	size_t childrenCount;

	switch (node->m_nodeKind) {
	case NodeKind_Epsilon:
	case NodeKind_Token:
		break;

	case NodeKind_Symbol:
		if (node->m_flags & SymbolNodeFlag_User)
			break;

		symbol = (SymbolNode*)node;
		ASSERT(!symbol->m_resolver);

		symbol->m_flags |= NodeFlag_RecursionStopper;

		childrenCount = symbol->m_productionArray.getCount();
		for (size_t i = 0; i < childrenCount; i++) {
			GrammarNode* child = symbol->m_productionArray[i];
			result = scan(child);
			if (!result)
				return false;
		}

		symbol->m_flags &= ~NodeFlag_RecursionStopper;
		break;

	case NodeKind_Sequence:
		sequence = (SequenceNode*)node;
		sequence->m_flags |= NodeFlag_RecursionStopper;

		childrenCount = sequence->m_sequence.getCount();
		for (size_t i = 0; i < childrenCount; i++) {
			GrammarNode* child = sequence->m_sequence[i];
			result = scan(child);
			if (!result)
				return false;
		}

		sequence->m_flags &= ~NodeFlag_RecursionStopper;
		break;

	case NodeKind_Beacon:
		result = addBeacon((BeaconNode*)node);
		if (!result)
			return false;

		break;

	case NodeKind_Action:
		action = (ActionNode*)node;
		action->m_productionSymbol = m_symbol;
		m_actionArray.append(action);
		break;

	case NodeKind_Argument:
		argument = (ArgumentNode*)node;
		argument->m_productionSymbol = m_symbol;
		m_argumentArray.append(argument);
		break;

	default:
		ASSERT(false);
	}

	return true;
}

bool
ProductionBuilder::addBeacon(BeaconNode* beacon) {
	if (beacon->m_flags & BeaconNodeFlag_Added)
		return true;

	if (!beacon->m_label.isEmpty()) {
		sl::StringHashTableIterator<BeaconNode*> it = m_beaconMap.visit(beacon->m_label);
		if (!it->m_value)
			it->m_value = beacon;
	}

	if (beacon->m_target->m_nodeKind == NodeKind_Symbol) {
		SymbolNode* node = (SymbolNode*)beacon->m_target;
		size_t paramCount = node->m_paramNameList.getCount();
		size_t actualArgCount = beacon->m_argument ? beacon->m_argument->m_argValueList.getCount() : 0;

		if (paramCount != actualArgCount) {
			err::setFormatStringError(
				"'%s' takes %d parameters, passed %d",
				node->m_name.sz(),
				paramCount,
				actualArgCount
			);
			lex::pushSrcPosError(beacon->m_srcPos);
			return false;
		}
	}

	m_beaconArray.append(beacon);
	beacon->m_flags |= BeaconNodeFlag_Added;
	return true;
}

void
ProductionBuilder::findAndReplaceUnusedBeacons(GrammarNode** node0) {
	GrammarNode* node = *node0;

	if (node->m_flags & NodeFlag_RecursionStopper)
		return;

	SymbolNode* symbol;
	SequenceNode* sequence;
	BeaconNode* beacon;

	size_t count;

	switch (node->m_nodeKind) {
	case NodeKind_Epsilon:
	case NodeKind_Token:
	case NodeKind_Action:
	case NodeKind_Argument:
		break;

	case NodeKind_Beacon:
		beacon = (BeaconNode*)node;
		if (beacon->m_slotIndex != -1)
			break;

		if (!(beacon->m_flags & BeaconNodeFlag_Deleted)) {
			m_beaconDeleteArray.append(beacon);
			beacon->m_flags |= BeaconNodeFlag_Deleted;
		}

		*node0 = beacon->m_target; // replace
		break;

	case NodeKind_Symbol: {
		if (node->m_flags & SymbolNodeFlag_User)
			break;

		symbol = (SymbolNode*)node;
		symbol->m_flags |= NodeFlag_RecursionStopper;

		if (symbol->m_synchronizer)
			findAndReplaceUnusedBeacons(&symbol->m_synchronizer);

		count = symbol->m_productionArray.getCount();
		sl::Array<GrammarNode*>::Rwi rwi = symbol->m_productionArray;
		for (size_t i = 0; i < count; i++)
			findAndReplaceUnusedBeacons(&rwi[i]);

		symbol->m_flags &= ~NodeFlag_RecursionStopper;
		break;
		}

	case NodeKind_Sequence: {
		sequence = (SequenceNode*)node;
		sequence->m_flags |= NodeFlag_RecursionStopper;

		count = sequence->m_sequence.getCount();
		sl::Array<GrammarNode*>::Rwi rwi = sequence->m_sequence;
		for (size_t i = 0; i < count; i++)
			findAndReplaceUnusedBeacons(&rwi[i]);

		sequence->m_flags &= ~NodeFlag_RecursionStopper;
		break;
		}

	default:
		ASSERT(false);
	}
}

ProductionBuilder::VariableKind
ProductionBuilder::findVariable(
	int index,
	BeaconNode** beacon_o
) {
	if (index == 0)
		return VariableKind_This;

	size_t beaconIndex = index - 1;
	size_t beaconCount = m_beaconArray.getCount();

	if (beaconIndex >= beaconCount) {
		err::setFormatStringError("locator '$%d' is out of range ($1..$%d)", beaconIndex + 1, beaconCount);
		return VariableKind_Undefined;
	}

	BeaconNode* beacon = m_beaconArray[beaconIndex];
	*beacon_o = beacon;
	return beacon->m_target->m_nodeKind == NodeKind_Token ?
		VariableKind_TokenBeacon :
		VariableKind_SymbolBeacon;
}

ProductionBuilder::VariableKind
ProductionBuilder::findVariable(
	const sl::StringRef& name,
	BeaconNode** beacon_o
) {
	sl::StringHashTableIterator<BeaconNode*> it = m_beaconMap.find(name);
	if (it) {
		BeaconNode* beacon = it->m_value;
		*beacon_o = beacon;
		return beacon->m_target->m_nodeKind == NodeKind_Token ?
			VariableKind_TokenBeacon :
			VariableKind_SymbolBeacon;
	}

	sl::StringHashTableIterator<bool> it2 = m_symbol->m_localNameSet.find(name);
	if (it2)
		return VariableKind_Local;

	it2 = m_symbol->m_paramNameSet.find(name);
	if (it2)
		return VariableKind_Param;

	err::setFormatStringError("locator '$%s' not found", name.sz());
	return VariableKind_Undefined;
}

bool
ProductionBuilder::processUserCode(
	lex::SrcPos& srcPos,
	sl::String* userCode
) {
	const Token* token;

	sl::String resultString;

	Lexer::create(getMachineState(LexerMachine_UserCode2ndPass), *userCode);
	setLineCol(srcPos);

	VariableKind variableKind;
	BeaconNode* beacon;
    const char* p = userCode->cp();

	for (;;) {
		token = getToken();
		if (token->m_token <= 0)
			break;

		switch (token->m_token) {
		case TokenKind_Integer:
			variableKind = findVariable(token->m_data.m_integer, &beacon);
			break;

		case TokenKind_Identifier:
			variableKind = findVariable(token->m_data.m_string, &beacon);
			break;

		default:
			nextToken();
			continue;
		}

		if (!variableKind)
			return false;

		resultString.append(p, token->m_pos.m_p - p);

		switch (variableKind) {
		case VariableKind_SymbolBeacon:
		case VariableKind_TokenBeacon:
			if (beacon->m_slotIndex == -1) {
				if (!m_dispatcher)
					m_dispatcher = m_nodeMgr->createDispatcherNode(m_symbol);

				beacon->m_slotIndex = m_dispatcher->m_beaconArray.getCount();
				m_dispatcher->m_beaconArray.append(beacon);
			}

			resultString.appendFormat("$%d", beacon->m_slotIndex);
			break;

		case VariableKind_This:
			resultString.append('$');
			break;

		case VariableKind_Param:
			resultString.appendFormat("$param.%s", token->m_data.m_string.sz());
			break;

		case VariableKind_Local:
			resultString.appendFormat("$local.%s", token->m_data.m_string.sz());
			break;

		default:
			ASSERT(false);
		}

		p = token->m_pos.m_p + token->m_pos.m_length;

		nextToken();
	}

	ASSERT(!token->m_token);
	resultString.append(p, token->m_pos.m_p - p);

	*userCode = resultString;
	return true;
}

//..............................................................................
