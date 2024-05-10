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
#include "LaDfaBuilder.h"

//..............................................................................

LaDfaThread::LaDfaThread() {
	m_match = LaDfaThreadMatchKind_None;
	m_state = NULL;
	m_production = NULL;
	m_resolver = NULL;
	m_resolverPriority = 0;
}

//..............................................................................

LaDfaState::LaDfaState() {
	m_index = -1;
	m_lookahead = 0;
	m_flags = 0;
	m_dfaNode = NULL;
	m_fromState = NULL;
	m_token = NULL;
}

LaDfaThread*
LaDfaState::createThread(LaDfaThread* src) {
	LaDfaThread* thread = new LaDfaThread;
	thread->m_state = this;

	if (src) {
		ASSERT(!src->m_resolver);

		thread->m_production = src->m_production;
		thread->m_stack = src->m_stack;
	}

	m_activeThreadList.insertTail(thread);
	return thread;
}

bool
LaDfaState::calcResolved() {
	sl::Iterator<LaDfaThread> thread;

	if (m_activeThreadList.isEmpty()) {
		m_dfaNode->m_flags |= LaDfaNodeFlag_Resolved;
		return true;
	}

	thread = m_activeThreadList.getHead();

	Node* originalProduction = thread->m_production;

	for (thread++; thread; thread++) {
		if (thread->m_production != originalProduction)
			return false;
	}

	thread = m_completeThreadList.getHead();
	for (; thread; thread++) {
		if (thread->m_production != originalProduction)
			return false;
	}

	m_dfaNode->m_flags |= LaDfaNodeFlag_Resolved;
	return true;
}

Node*
LaDfaState::getResolvedProduction() {
	LaDfaThread* activeThread = *m_activeThreadList.getHead();
	LaDfaThread* completeThread = *m_completeThreadList.getHead();
	LaDfaThread* epsilonThread = *m_epsilonThreadList.getHead();

	if (isAnyTokenIgnored())
		return
			activeThread && activeThread->m_match != LaDfaThreadMatchKind_AnyToken ? activeThread->m_production :
			completeThread && completeThread->m_match != LaDfaThreadMatchKind_AnyToken ? completeThread->m_production :
			epsilonThread ? epsilonThread->m_production : NULL;
	else
		return
			activeThread ? activeThread->m_production :
			completeThread ? completeThread->m_production :
			epsilonThread ? epsilonThread->m_production : NULL;

}

Node*
LaDfaState::getDefaultProduction() {
	LaDfaThread* completeThread = *m_completeThreadList.getHead();
	LaDfaThread* epsilonThread = *m_epsilonThreadList.getHead();

	return
		completeThread ? completeThread->m_production :
		epsilonThread ? epsilonThread->m_production :
		m_fromState ? m_fromState->getDefaultProduction() : NULL;
}

//..............................................................................

LaDfaBuilder::LaDfaBuilder(
	const CmdLine* cmdLine,
	NodeMgr* nodeMgr,
	const sl::Array<Node*>* parseTable
) {
	m_cmdLine = cmdLine;
	m_nodeMgr = nodeMgr;
	m_parseTable = parseTable;
	m_conflict = NULL;
	m_maxUsedLookahead = 1;
}

static
int
cmpResolverThreadPriority(
	const void* p1,
	const void* p2
) {
	LaDfaThread* thread1 = *(LaDfaThread**) p1;
	LaDfaThread* thread2 = *(LaDfaThread**) p2;

	// sort from highest priority to lowest

	return
		thread1->m_resolverPriority < thread2->m_resolverPriority ? 1 :
		thread1->m_resolverPriority > thread2->m_resolverPriority ? -1 : 0;
}

Node*
LaDfaBuilder::build(ConflictNode* conflict) {
	ASSERT(conflict->m_nodeKind == NodeKind_Conflict);

	m_conflict = conflict;

	size_t tokenCount = m_nodeMgr->m_tokenArray.getCount();

	m_stateList.clear();

	LaDfaState* state0 = createState();
	state0->m_dfaNode = m_nodeMgr->createLaDfaNode();

	size_t count = conflict->m_productionArray.getCount();
	for (size_t i = 0; i < count; i++) {
		GrammarNode* production = conflict->m_productionArray[i];

		LaDfaThread* thread = state0->createThread();
		thread->m_production = production;

		if (production->m_nodeKind != NodeKind_Epsilon)
			thread->m_stack.append(production);
		else
			state0->m_flags |= LaDfaStateFlag_EpsilonProduction;
	}

	LaDfaState* state1;
	bool result = transition(&state1, state0, conflict->m_token);
	if (!result) {
		err::setFormatStringError(
			"conflict at %s:%s causes depth overflow, check for left recursion",
			conflict->m_symbol->m_name.sz(),
			conflict->m_token->m_name.sz()
		);

		lex::pushSrcPosError(conflict->m_symbol->m_srcPos);
		return NULL;
	}

	size_t lookahead = 1;

	if (!state1->isResolved()) {
		sl::Array<LaDfaState*> stateArray;
		stateArray.append(state1);

		while (!stateArray.isEmpty() && lookahead < conflict->m_symbol->m_lookaheadLimit) {
			lookahead++;

			sl::Array<LaDfaState*> nextStateArray;

			size_t stateCount = stateArray.getCount();
			for (size_t j = 0; j < stateCount; j++) {
				LaDfaState* state = stateArray[j];

				for (size_t k = 0; k < tokenCount; k++) {
					SymbolNode* token = m_nodeMgr->m_tokenArray[k];

					LaDfaState* newState;
					result = transition(&newState, state, token);
					if (!result) {
						err::setFormatStringError(
							"conflict at %s:%s causes depth overflow, check for left recursion",
							conflict->m_symbol->m_name.sz(),
							conflict->m_token->m_name.sz()
						);

						lex::pushSrcPosError(conflict->m_symbol->m_srcPos);
						return NULL;
					}

					if (newState && !newState->isResolved())
						nextStateArray.append(newState);
				}
			}

			stateArray = nextStateArray;
		}

		if (!stateArray.isEmpty()) {
			LaDfaState* state = stateArray[0];
			sl::BoxList<sl::String> tokenNameList;

			for (; state != state0; state = state->m_fromState)
				tokenNameList.insertHead(state->m_token->m_name);

			sl::String tokenSeqString;
			sl::BoxIterator<sl::String> tokenName = tokenNameList.getHead();
			for (; tokenName; tokenName++) {
				tokenSeqString.append(*tokenName);
				tokenSeqString.append(' ');
			}

			err::setFormatStringError(
				"conflict at %s:%s could not be resolved with %d token lookahead; e.g. %s",
				conflict->m_symbol->m_name.sz(),
				conflict->m_token->m_name.sz(),
				conflict->m_symbol->m_lookaheadLimit,
				tokenSeqString.sz()
			);

			lex::pushSrcPosError(conflict->m_symbol->m_srcPos);
			return NULL;
		}
	}

	if (lookahead > m_maxUsedLookahead)
		m_maxUsedLookahead = lookahead;

	sl::Iterator<LaDfaState> it = m_stateList.getHead();
	for (; it; it++) {
		LaDfaState* state = *it;

		if (state->m_completeThreadList.getCount() > 1 ||
			state->m_epsilonThreadList.getCount() > 1) {
			err::setFormatStringError(
				"conflict at %s:%s: multiple productions complete with %s",
				conflict->m_symbol->m_name.sz(),
				conflict->m_token->m_name.sz(),
				state->m_token->m_name.sz()
			);
			lex::pushSrcPosError(conflict->m_symbol->m_srcPos);
			return NULL;
		}

		if (!state->m_resolverThreadList.isEmpty()) { // chain all resolvers
			size_t count = state->m_resolverThreadList.getCount();

			sl::Array<LaDfaThread*> resolverThreadArray;
			resolverThreadArray.setCount(count);
			sl::Array<LaDfaThread*>::Rwi rwi = resolverThreadArray;

			sl::Iterator<LaDfaThread> resolverThread = state->m_resolverThreadList.getHead();
			for (size_t i = 0; resolverThread; resolverThread++, i++)
				rwi[i] = *resolverThread;

			qsort(rwi.p(), count, sizeof(LaDfaThread*), cmpResolverThreadPriority);

			for (size_t i = 0; i < count; i++) {
				LaDfaThread* resolverThread = resolverThreadArray[i];

				LaDfaNode* dfaElse = m_nodeMgr->createLaDfaNode();
				dfaElse->m_flags = state->m_dfaNode->m_flags;
				dfaElse->m_transitionArray = state->m_dfaNode->m_transitionArray;

				state->m_dfaNode->m_resolver = resolverThread->m_resolver;
				state->m_dfaNode->m_production = resolverThread->m_production;
				state->m_dfaNode->m_resolverElse = dfaElse;
				state->m_dfaNode->m_transitionArray.clear();

				dfaElse->m_resolverUplink = state->m_dfaNode;
				state->m_dfaNode = dfaElse;
			}
		}

		ASSERT(!state->m_dfaNode->m_resolver);

		if (state->isResolved()) {
			state->m_dfaNode->m_flags |= LaDfaNodeFlag_Leaf;
			state->m_dfaNode->m_production = state->getResolvedProduction();

			if (state->m_dfaNode->m_resolverUplink) {
				LaDfaNode* uplink = state->m_dfaNode->m_resolverUplink;

				if (!state->m_dfaNode->m_production ||
					state->m_dfaNode->m_production == uplink->m_production) {
					// here we handle situation like
					// 1) sym: resolver ({1}) 'a' | resolver ({2}) 'b' (we have a chain of 2 resolver with empty tail)
					// or
					// 2) both resolver 'then' and 'else' branch point to the same production (this happens when resolver applies not to the original conflict)
					// in either case we we can safely eliminate the resolver {2}

					uplink->m_flags |= LaDfaNodeFlag_Leaf;
					uplink->m_resolver = NULL;
					uplink->m_resolverElse = NULL;

					m_nodeMgr->deleteLaDfaNode(state->m_dfaNode);
					state->m_dfaNode = uplink;
				}
			}
		} else {
			state->m_dfaNode->m_production = state->getDefaultProduction();
		}
	}

	if (m_cmdLine->m_flags & CmdLineFlag_Verbose)
		trace();

	if (state1->m_resolverThreadList.isEmpty() &&
		(state1->m_dfaNode->m_flags & LaDfaNodeFlag_Leaf)) {
		// can happen on active-vs-complete-vs-epsion conflicts

		state0->m_dfaNode->m_flags |= LaDfaNodeFlag_Leaf; // don't index state0
		return state1->m_dfaNode->m_production;
	}

	return state0->m_dfaNode;
}

void
LaDfaBuilder::trace() {
	sl::Iterator<LaDfaState> it = m_stateList.getHead();
	for (; it; it++) {
		LaDfaState* state = *it;

		printf(
			"%3d %s %d/%d/%d/%d (a/r/c/e)\n",
			state->m_index,
			state->isResolved() ? "*" : " ",
			state->m_activeThreadList.getCount(),
			state->m_resolverThreadList.getCount(),
			state->m_completeThreadList.getCount(),
			state->m_epsilonThreadList.getCount()
		);

		sl::Iterator<LaDfaThread> thread;

		if (!state->m_activeThreadList.isEmpty()) {
			printf("\tACTIVE:   ");

			thread = state->m_activeThreadList.getHead();
			for (; thread; thread++)
				printf("%s ", thread->m_production->m_name.sz());

			printf("\n");
		}

		if (!state->m_resolverThreadList.isEmpty()) {
			printf("\tRESOLVER: ");

			thread = state->m_resolverThreadList.getHead();
			for (; thread; thread++)
				printf("%s ", thread->m_production->m_name.sz());

			printf("\n");
		}

		if (!state->m_completeThreadList.isEmpty()) {
			printf("\tCOMPLETE: ");

			thread = state->m_completeThreadList.getHead();
			for (; thread; thread++)
				printf("%s ", thread->m_production->m_name.sz());

			printf("\n");
		}

		if (!state->m_epsilonThreadList.isEmpty()) {
			printf("\tEPSILON: ");

			thread = state->m_epsilonThreadList.getHead();
			for (; thread; thread++)
				printf("%s ", thread->m_production->m_name.sz());

			printf("\n");
		}

		if (!state->isResolved()) {
			size_t moveCount = state->m_transitionArray.getCount();
			for (size_t i = 0; i < moveCount; i++) {
				LaDfaState* moveTo = state->m_transitionArray[i];
				printf(
					"\t%s -> %d\n",
					moveTo->m_token->m_name.sz(),
					moveTo->m_index
				);
			}
		}
	}
}

LaDfaState*
LaDfaBuilder::createState() {
	LaDfaState* state = new LaDfaState;
	state->m_index = m_stateList.getCount();
	m_stateList.insertTail(state);

	return state;
}

bool
LaDfaBuilder::transition(
	LaDfaState** resultState,
	LaDfaState* state,
	SymbolNode* token
) {
	bool result;

	LaDfaState* newState = createState();
	newState->m_token = token;
	newState->m_fromState = state;
	newState->m_lookahead = state->m_lookahead + 1;
	newState->m_flags = state->m_flags & LaDfaStateFlag_EpsilonProduction; // propagate epsilon

	sl::Iterator<LaDfaThread> threadIt = state->m_activeThreadList.getHead();
	for (; threadIt; threadIt++) {
		LaDfaThread* newThread = newState->createThread(*threadIt);
		result = processThread(newThread, 0);
		if (!result)
			return false;
	}

	threadIt = newState->m_activeThreadList.getHead();
	while (threadIt) {
		LaDfaThread* thread = *threadIt++;

		if (thread->m_match == LaDfaThreadMatchKind_AnyToken && newState->isAnyTokenIgnored()) {
			newState->m_activeThreadList.erase(thread); // delete anytoken thread in favor of concrete token
		} else if (thread->m_stack.isEmpty()) {
			newState->m_activeThreadList.remove(thread);

			if (thread->m_match)
				newState->m_completeThreadList.insertTail(thread);
			else
				newState->m_epsilonThreadList.insertTail(thread);
		}
	}

	if (newState->isEmpty()) {
		m_stateList.erase(newState);
		*resultState = NULL;
		return true;
	}

	newState->m_dfaNode = m_nodeMgr->createLaDfaNode();
	newState->m_dfaNode->m_token = token;
	newState->calcResolved();

	state->m_dfaNode->m_transitionArray.append(newState->m_dfaNode);
	state->m_transitionArray.append(newState);
	*resultState = newState;

	return true;
}

bool
LaDfaBuilder::processThread(
	LaDfaThread* thread,
	size_t depth
) {
	if (depth > m_cmdLine->m_conflictDepthLimit)
		return false;

	SymbolNode* token = thread->m_state->m_token;

	thread->m_match = LaDfaThreadMatchKind_None;

	size_t tokenCount = m_nodeMgr->m_tokenArray.getCount();
	for (;;) {
		if (thread->m_stack.isEmpty())
			break;

		Node* node = thread->m_stack.getBack();
		Node* production;
		SymbolNode* symbol;
		ConflictNode* conflict;
		SequenceNode* sequence;
		size_t childrenCount;

		switch (node->m_nodeKind) {
		case NodeKind_Token:
			if (thread->m_match)
				return true;

			ASSERT(node->m_masterIndex);

			if ((node->m_flags & SymbolNodeFlag_AnyToken) && token->m_masterIndex != 0) { // EOF does not match ANY
				thread->m_stack.pop();
				thread->m_match = LaDfaThreadMatchKind_AnyToken;
				break;
			}

			if (node != token) { // could happen after epsilon production
				thread->m_state->m_activeThreadList.erase(thread);
				return true;
			}

			thread->m_stack.pop();
			thread->m_match = LaDfaThreadMatchKind_Token;
			thread->m_state->m_flags |= LaDfaStateFlag_TokenMatch;
			break;

		case NodeKind_Symbol:
			if (thread->m_match)
				return true;

			production = (*m_parseTable)[node->m_index * tokenCount + token->m_index];
			if (!production) { // could happen after epsilon production
				thread->m_state->m_activeThreadList.erase(thread);
				return true;
			}

			// ok this thread seems to stay active, let's check if we can eliminate it with resolver

			symbol = (SymbolNode*)node;
			if (symbol->m_resolver && thread->m_state->m_lookahead == 1) { // only use resolvers at the first step
				if (m_cmdLine->m_flags & CmdLineFlag_Verbose)
					printf(
						"  RESOLVER of %s is used for conflict at %s:%s\n",
						symbol->m_name.sz(),
						m_conflict->m_symbol->m_name.sz(),
						m_conflict->m_token->m_name.sz()
					);

				symbol->m_flags |= SymbolNodeFlag_ResolverUsed;
				thread->m_resolver = symbol->m_resolver;
				thread->m_resolverPriority = symbol->m_resolverPriority;
				thread->m_state->m_activeThreadList.remove(thread);
				thread->m_state->m_resolverThreadList.insertTail(thread);
				return true;
			}

			thread->m_stack.pop();

			if (production->m_nodeKind != NodeKind_Epsilon)
				thread->m_stack.append(production);
			else
				thread->m_state->m_flags |= LaDfaStateFlag_EpsilonProduction;

			break;

		case NodeKind_Sequence:
			if (thread->m_match)
				return true;

			thread->m_stack.pop();

			sequence = (SequenceNode*)node;
			childrenCount = sequence->m_sequence.getCount();
			for (intptr_t i = childrenCount - 1; i >= 0; i--) {
				Node* child = sequence->m_sequence[i];
				thread->m_stack.append(child);
			}

			break;

		case NodeKind_Beacon:
			thread->m_stack.pop();
			thread->m_stack.append(((BeaconNode*)node)->m_target);
			break;

		case NodeKind_Action:
		case NodeKind_Argument:
			thread->m_stack.pop();
			break;

		case NodeKind_Conflict:
			thread->m_stack.pop();

			conflict = (ConflictNode*)node;
			childrenCount = conflict->m_productionArray.getCount();
			depth++;
			for (size_t i = 0; i < childrenCount; i++) {
				Node* child = conflict->m_productionArray[i];
				LaDfaThread* newThread = thread->m_state->createThread(thread);

				if (child->m_nodeKind != NodeKind_Epsilon)
					newThread->m_stack.append(child);

				bool result = processThread(newThread, depth);
				if (!result)
					return false;
			}

			thread->m_state->m_activeThreadList.erase(thread);
			return true;

		default:
			ASSERT(false);
		}
	}

	return true;
}

//..............................................................................
