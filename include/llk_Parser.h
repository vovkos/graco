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

#pragma once

#define _LLK_PARSER_H

#include "llk_Node.h"

// #define _LLK_RANDOM_SYNTAX_ERRORS      1
// #define _LLK_RANDOM_SEMANTIC_ERRORS    1
// #define _LLK_RANDOM_ERRORS_PROBABILITY 32

#if (_LLK_RANDOM_SYNTAX_ERRORS || _LLK_RANDOM_SEMANTIC_ERRORS)
#	define _LLK_RANDOM_ERRORS 1
#endif

namespace llk {

//..............................................................................

template <
	typename T,
	typename Token0
>
class Parser {
public:
	typedef Token0 Token;
	typedef typename Token::TokenKind TokenKind;
	typedef llk::TokenNode<Token> TokenNode;
	typedef llk::SymbolNode SymbolNode;
	typedef llk::LaDfaNode<Token> LaDfaNode;

protected:
	enum Flag {
		Flag_TokenMatch            = 0x0001,
		Flag_Synchronize           = 0x0010,
		Flag_PostSynchronize       = 0x0020,
		Flag_RecoveryFailureErrors = 0x0100,
#if (_LLK_RANDOM_ERRORS)
		Flag_NoRandomErrors        = 0x1000,
#endif
	};

	enum ErrorKind {
		ErrorKind_Syntax,
		ErrorKind_Semantic,
	};

	enum RecoveryAction {
		RecoveryAction_Fail,
		RecoveryAction_Synchronize,
		RecoveryAction_Continue,
	};

	enum MatchResult {
		MatchResult_Fail,
		MatchResult_NextToken,
		MatchResult_NextTokenNoAdvance,
		MatchResult_Continue,
	};

	enum LaDfaResult {
		LaDfaResult_Fail,
		LaDfaResult_Production,
		LaDfaResult_Resolver,
	};

	struct LaDfaTransition {
		uint_t m_flags;
		size_t m_productionIndex;
		size_t m_resolverIndex;
		size_t m_resolverElseIndex;
	};

protected:
	axl::sl::StringRef m_fileName;

	axl::mem::Pool<Token>* m_tokenPool;
	NodeAllocator<T>* m_nodeAllocator;
	axl::sl::Array<Node*> m_predictionStack;
	axl::sl::Array<SymbolNode*> m_symbolStack;
	axl::sl::Array<SymbolNode*> m_catchStack;
	axl::sl::Array<LaDfaNode*> m_resolverStack;

	axl::sl::SimpleHashTable<int, size_t> m_syncTokenSet;
	axl::sl::List<Token> m_tokenList;
	axl::sl::Iterator<Token> m_tokenCursor;
	uint_t m_flags;

public:
	Parser() {
		// the same parser is normally never shared among threads
		// if it does, be sure to update the node allocator and token pool

		m_tokenPool = axl::mem::getCurrentThreadPool<Token>();
		m_nodeAllocator = getCurrentThreadNodeAllocator<T>();
		m_flags = 0;
	}

	~Parser() {
		clear();
	}

	axl::mem::Pool<Token>*
	getTokenPool() const {
		return m_tokenPool;
	}
	
	static
	void
	clearNodeAllocator() {
		getCurrentThreadNodeAllocator<T>()->clear();
	}

	SymbolNode*
	create(
		const axl::sl::StringRef& fileName,
		int symbol = T::StartSymbol
	) {
		clear();
		m_fileName = fileName;
		return (SymbolNode*)pushPrediction(T::SymbolFirst + symbol);
	}

	void
	clear() {
		m_fileName.clear();
		m_tokenPool->put(&m_tokenList);

		size_t count = m_predictionStack.getCount();
		for (size_t i = 0; i < count; i++) {
			Node* node = m_predictionStack[i];
			if (!(node->m_flags & NodeFlag_Locator))
				m_nodeAllocator->free(node);
		}

		m_predictionStack.clear();
		m_symbolStack.clear();
		m_resolverStack.clear();
		m_tokenList.clear();
		m_tokenCursor = NULL;
		m_flags = 0;
	}

	void
	enableRecoveryFailureErrors(bool isEnabled) {
		if (isEnabled)
			m_flags |= Flag_RecoveryFailureErrors;
		else
			m_flags &= ~Flag_RecoveryFailureErrors;
	}

#if (_LLK_RANDOM_ERRORS)
	void
	disableRandomErrors() {
		m_flags |= Flag_NoRandomErrors;
	}
#endif

	bool
	consumeToken(Token* token) {
		bool result;

		if (token->m_token == -1) {
			axl::err::setFormatStringError("invalid character '\\x%x'", token->m_data.m_integer);
			axl::lex::ensureSrcPosError(m_fileName, token->m_pos);
			m_tokenPool->put(token);
			return false;
		}

		if (m_flags & Flag_Synchronize) {
			MatchResult matchResult = synchronize(token);
			if (matchResult == MatchResult_NextToken) {
				m_tokenPool->put(token);
				return true;
			} else if (matchResult == MatchResult_Fail) {
				m_tokenPool->put(token);
				axl::lex::ensureSrcPosError(m_fileName, token->m_pos);
				return false;
			}
		}

		m_tokenCursor = m_tokenList.insertTail(token);

		const size_t* parseTable = static_cast<T*>(this)->getParseTable();
		size_t tokenIndex = static_cast<T*>(this)->getTokenIndex(token->m_token);
		ASSERT(tokenIndex < T::TokenCount);

		// first check for pragma productions out of band

		if (T::PragmaStartSymbol != -1) {
			size_t productionIndex = parseTable[T::PragmaStartSymbol * T::TokenCount + tokenIndex];
			if (productionIndex != -1 && productionIndex != 0)
				pushPrediction(productionIndex);
		}

		m_flags &= ~Flag_TokenMatch;

		for (;;) {
			if (m_flags & Flag_Synchronize) {
				MatchResult matchResult = synchronize(*m_tokenCursor);
				switch (matchResult) {
				case MatchResult_Continue:
					break;

				case MatchResult_NextToken:
					result = advanceTokenCursor();
					if (!result)
						return true; // no more tokens, we are done

					// fall through

				case MatchResult_NextTokenNoAdvance:
					tokenIndex = static_cast<T*>(this)->getTokenIndex(m_tokenCursor->m_token);
					ASSERT(tokenIndex < T::TokenCount);
					m_flags &= ~Flag_TokenMatch;
					break;

				default:
					ASSERT(false);
				}
			}

			MatchResult matchResult;

			Node* node = getPredictionTop();
			if (!node) {
				matchResult = matchEmptyPredictionStack();
			} else {
				switch (node->m_nodeKind) {
				case NodeKind_Token:
					matchResult = matchTokenNode((TokenNode*)node, tokenIndex);
					break;

				case NodeKind_Symbol:
					matchResult = matchSymbolNode((SymbolNode*)node, parseTable, tokenIndex);
					break;

				case NodeKind_Sequence:
					matchResult = matchSequenceNode(node);
					break;

				case NodeKind_Action:
					matchResult = matchActionNode(node);
					break;

				case NodeKind_Argument:
					ASSERT(node->m_flags & NodeFlag_Matched); // was handled during matching NodeKind_Symbol
					popPrediction();
					matchResult = MatchResult_Continue;
					break;

				case NodeKind_LaDfa:
					matchResult = matchLaDfaNode((LaDfaNode*)node);
					break;

				default:
					ASSERT(false);
				}
			}

			m_flags &= ~Flag_PostSynchronize;

			if (matchResult == MatchResult_Fail) {
				if (m_resolverStack.isEmpty()) {
					axl::lex::ensureSrcPosError(m_fileName, token->m_pos);
					return false;
				}

				matchResult = rollbackResolver();
				ASSERT(matchResult != MatchResult_Fail); // failed resolver means there is another possibility!
			}

			switch (matchResult) {
			case MatchResult_Continue:
				break;

			case MatchResult_NextToken:
				result = advanceTokenCursor();
				if (!result)
					return true; // no more tokens, we are done

				// fall through

			case MatchResult_NextTokenNoAdvance:
				tokenIndex = static_cast<T*>(this)->getTokenIndex(m_tokenCursor->m_token);
				ASSERT(tokenIndex < T::TokenCount);
				m_flags &= ~Flag_TokenMatch;
				break;

			default:
				ASSERT(false);
			}
		}
	}

	// debug

	void
	traceSymbolStack() {
		intptr_t count = m_symbolStack.getCount();
		TRACE("SYMBOL STACK (%d symbols):\n", count);

		for (intptr_t i = count - 1; i >= 0; i--) {
			SymbolNode* node = m_symbolStack[i];
			TRACE("%p %s\n", node, static_cast<T*>(this)->getSymbolName(node->m_index));
		}
	}

	void
	tracePredictionStack() {
		intptr_t count = m_predictionStack.getCount();
		TRACE("PREDICTION STACK (%d nodes):\n", count);

		for (intptr_t i = count - 1; i >= 0; i--) {
			Node* node = m_predictionStack[i];

			const char* extra = node->m_nodeKind == NodeKind_Symbol ?
				static_cast<T*>(this)->getSymbolName(node->m_index) :
				"";

			TRACE("%p %s (%d) %s\n", node, getNodeKindString(node->m_nodeKind), node->m_index, extra);
		}
	}

	void
	traceTokenList() {
		TRACE("TOKEN LIST (%d tokens):\n", m_tokenList.getCount());

		axl::sl::ConstIterator<Token> it = m_tokenList.getHead();
		for (; it; it++) {
			TRACE("%s '%s' %s\n", it->getName(), it->getText(), it == m_tokenCursor ? "<--" : "");
		}
	}

	// public info

	Node*
	getPredictionTop() {
		return !m_predictionStack.isEmpty() ? m_predictionStack.getBack() : NULL;
	}

	SymbolNode*
	getSymbolTop() {
		return !m_symbolStack.isEmpty() ? m_symbolStack.getBack() : NULL;
	}

	SymbolNode*
	getCatchTop() {
		return !m_catchStack.isEmpty() ? m_catchStack.getBack() : NULL;
	}

protected:
	// default recover action: fail on semantic error, synchronize on syntax error

	RecoveryAction
	processError(ErrorKind errorKind) {
		if (errorKind != ErrorKind_Syntax)
			return RecoveryAction_Fail;

		fprintf(stderr, "syntax error: %s\n", axl::err::getLastErrorDescription().sz());
		return RecoveryAction_Synchronize;
	}

	void
	onSynchronizeSkipToken(const Token* token) {}

	void
	onSynchronized(const Token* token) {}

	bool
	isNamedSymbol(const SymbolNode* symbol) {
		return symbol->m_index < T::NamedSymbolCount;
	}

	bool
	isCatchSymbol(const SymbolNode* symbol) {
		return
			symbol->m_index >= T::NamedSymbolCount &&
			symbol->m_index < T::NamedSymbolCount + T::CatchSymbolCount;
	}

	RecoveryAction
	recover(ErrorKind errorKind) {
		ASSERT(m_resolverStack.isEmpty());

		if (errorKind == ErrorKind_Syntax && (m_flags & Flag_PostSynchronize)) {
			// synchronizer token must match (otherwise, it's a bad choice of sync tokens)

			if (m_flags & Flag_RecoveryFailureErrors) {
				axl::err::setFormatStringError(
					"synchronizer token '%s' didn't match (adjust the 'catch' clause in the grammar)",
					m_tokenCursor->getName()
				);

				axl::lex::pushSrcPosError(m_fileName, m_tokenCursor->m_pos);
			}

			return RecoveryAction_Fail;
		}

		axl::lex::ensureSrcPosError(m_fileName, m_tokenCursor->m_pos);
		RecoveryAction action = static_cast<T*>(this)->processError(errorKind);
		ASSERT(action != RecoveryAction_Continue || errorKind != ErrorKind_Syntax); // can't continue on syntax errors

		if (action != RecoveryAction_Synchronize)
			return action;

		m_syncTokenSet.clear();

		size_t count = m_catchStack.getCount();
		for (intptr_t i = count - 1; i >= 0; i--) {
			SymbolNode* node = m_catchStack[i];
			int const* p = static_cast<T*>(this)->getSyncTokenSet(node->m_index);
			for (; *p != -1; p++)
				m_syncTokenSet.addIfNotExists(*p, i);
		}

		if (m_syncTokenSet.isEmpty()) {
			if (m_flags & Flag_RecoveryFailureErrors) {
				axl::err::setError("unable to recover from previous error(s)");
				axl::lex::pushSrcPosError(m_fileName, m_tokenCursor->m_pos);
			}

			return RecoveryAction_Fail;
		}

		// reset and wait for a synchornization token

		m_tokenList.clearButEntry(m_tokenCursor);
		m_flags |= Flag_Synchronize;
		return RecoveryAction_Synchronize;
	}

	MatchResult
	synchronize(const Token* token) {
		ASSERT(m_flags & Flag_Synchronize);
		ASSERT(m_resolverStack.isEmpty());
		ASSERT(!m_syncTokenSet.isEmpty());

		size_t i = m_syncTokenSet.findValue(token->m_token, -1);
		if (i == -1) {
			static_cast<T*>(this)->onSynchronizeSkipToken(token);
			return MatchResult_NextToken;
		}

		// pop the catcher

		SymbolNode* catcher = m_catchStack[i];
		ASSERT(catcher->m_flags & SymbolNodeFlag_Stacked);
		catcher->m_flags &= ~SymbolNodeFlag_Stacked;
		m_catchStack.setCount(i);

		// call leave() on symbols above the catcher

		size_t k = m_symbolStack.getCount() - 1;
		for (; k >= catcher->m_catchSymbolCount; k--) {
			SymbolNode* symbol = m_symbolStack[k];
			ASSERT(symbol->m_flags & SymbolNodeFlag_Stacked);
			if (symbol->m_leaveIndex != -1) {
				m_symbolStack.setCount(k + 1); // leave() uses the top of the stack
				static_cast<T*>(this)->leave(symbol->m_leaveIndex); // ignore result
			}
		}

		m_symbolStack.setCount(catcher->m_catchSymbolCount);

		// pop everything above the catcher off prediction stack

		intptr_t j = m_predictionStack.getCount() - 1;
		for (; j >= 0; j--) {
			Node* node = m_predictionStack[j];
			if (node == catcher)
				break;

			if (!(node->m_flags & NodeFlag_Locator))
				m_nodeAllocator->free(node);
		}

		bool isEof = token->m_token == T::EofToken;
		if (!isEof) {
			j++; // keep the catcher on prediction stack (otherwise, pop the eof-catcher)
			m_flags |= Flag_PostSynchronize; // synchronizer token *must* match
		}

		m_predictionStack.setCount(j);
		m_flags &= ~Flag_Synchronize;
		static_cast<T*>(this)->onSynchronized(token);
		return MatchResult_Continue;
	}

#if (_LLK_RANDOM_ERRORS)
	bool
	isRandomError(const char* description) {
		if ((m_flags & Flag_NoRandomErrors) ||
			m_resolverStack.isEmpty() ||
			rand() % _LLK_RANDOM_ERRORS_PROBABILITY)
			return false;

		axl::err::setFormatStringError("random error: %s", description);
		axl::lex::pushSrcPosError(m_fileName, m_tokenCursor->m_pos);
		return true;
	}
#endif

	bool
	advanceTokenCursor() {
		m_tokenCursor++;

		Node* node = getPredictionTop();
		if (m_resolverStack.isEmpty() && (!node || node->m_nodeKind != NodeKind_LaDfa)) {
			m_tokenPool->put(m_tokenList.removeHead()); // nobody gonna reparse this token
			ASSERT(m_tokenCursor == m_tokenList.getHead());
		}

		if (!m_tokenCursor)
			return false;

		return true;
	}

	// match against different kinds of nodes on top of prediction stack

	MatchResult
	matchEmptyPredictionStack() {
		if ((m_flags & Flag_TokenMatch) || m_tokenCursor->m_token == T::EofToken)
			return MatchResult_NextToken;

		axl::err::setFormatStringError("prediction stack empty while parsing '%s'", m_tokenCursor->getName());
		return MatchResult_Fail;
	}

	MatchResult
	matchTokenNode(
		TokenNode* node,
		size_t tokenIndex
	) {
		if (m_flags & Flag_TokenMatch)
			return MatchResult_NextToken;

#if (_LLK_RANDOM_SYNTAX_ERRORS)
		if (isRandomError("match-token"))
			return recover(ErrorKind_Syntax) ? MatchResult_Continue : MatchResult_Fail;
#endif

		if (node->m_index != T::AnyToken && node->m_index != tokenIndex) {
			if (!m_resolverStack.isEmpty())
				return MatchResult_Fail; // rollback resolver

			int expectedToken = static_cast<T*>(this)->getTokenFromIndex(node->m_index);
			axl::lex::setExpectedTokenError(Token::getName(expectedToken), m_tokenCursor->getName());
			return recover(ErrorKind_Syntax) ? MatchResult_Continue : MatchResult_Fail;
		}

		if (node->m_flags & NodeFlag_Locator) {
			node->m_token = **m_tokenCursor;
			node->m_flags |= NodeFlag_Matched;
		}

		m_flags |= Flag_TokenMatch;

		popPrediction();
		return MatchResult_Continue; // don't advance to next token just yet (execute following actions)
	}

	MatchResult
	matchSymbolNode(
		SymbolNode* node,
		const size_t* parseTable,
		size_t tokenIndex
	) {
		bool result;

		if (node->m_flags & SymbolNodeFlag_Stacked) {
			if (isCatchSymbol(node)) {
				popCatch();
				popPrediction();
				return MatchResult_Continue;
			}

			ASSERT(getSymbolTop() == node);
			node->m_flags |= NodeFlag_Matched;

			if (node->m_leaveIndex != -1) {
				result = static_cast<T*>(this)->leave(node->m_leaveIndex);

#if (_LLK_RANDOM_SEMANTIC_ERRORS)
				if (isRandomError("leave"))
					result = false;
#endif

				if (!result) {
					if (!m_resolverStack.isEmpty())
						return MatchResult_Fail; // rollback resolver

					RecoveryAction action = recover(ErrorKind_Semantic);
					if (action == RecoveryAction_Fail)
						return MatchResult_Fail;
					else if (action == RecoveryAction_Synchronize)
						return MatchResult_Continue;
				}
			}

			popSymbol();
			popPrediction();
			return MatchResult_Continue;
		}

		if (m_flags & Flag_TokenMatch)
			return MatchResult_NextToken;

		if (node->m_index < T::NamedSymbolCount) {
			Node* argument = getArgument();
			if (argument) {
				static_cast<T*>(this)->argument(argument->m_index, node);
				argument->m_flags |= NodeFlag_Matched;
			}

			pushSymbol(node);

			if (node->m_enterIndex != -1) {
				result = static_cast<T*>(this)->enter(node->m_enterIndex);

#if (_LLK_RANDOM_SEMANTIC_ERRORS)
				if (isRandomError("enter"))
					result = false;
#endif

				if (!result ) {
					if (!m_resolverStack.isEmpty())
						return MatchResult_Fail; // rollback resolver

					RecoveryAction action = recover(ErrorKind_Semantic);
					if (action == RecoveryAction_Fail)
						return MatchResult_Fail;
					else if (action == RecoveryAction_Synchronize)
						return MatchResult_Continue;
				}
			}
		} else if (node->m_index < T::NamedSymbolCount + T::CatchSymbolCount) {
			pushCatch(node);
		}

#if (_LLK_RANDOM_SYNTAX_ERRORS)
		if (isRandomError("parseTable"))
			return recover(ErrorKind_Syntax) ? MatchResult_Continue : MatchResult_Fail;
#endif

		size_t productionIndex = parseTable[node->m_index * T::TokenCount + tokenIndex];
		if (productionIndex == -1) {
			if (!m_resolverStack.isEmpty())
				return MatchResult_Fail; // rollback resolver

			SymbolNode* symbol = getSymbolTop();
			ASSERT(symbol);
			axl::err::setFormatStringError(
				"unexpected '%s' in '%s'",
				m_tokenCursor->getName(),
				static_cast<T*>(this)->getSymbolName(symbol->m_index)
			);

			return recover(ErrorKind_Syntax) ? MatchResult_Continue : MatchResult_Fail;
		}

		ASSERT(productionIndex < T::TotalCount);

		if (node->m_index >= T::NamedSymbolCount + T::CatchSymbolCount)
			popPrediction();

		pushPrediction(productionIndex);
		return MatchResult_Continue;
	}

	MatchResult
	matchSequenceNode(Node* node) {
		if (m_flags & Flag_TokenMatch)
			return MatchResult_NextToken;

		const size_t* p = static_cast<T*>(this)->getSequence(node->m_index);

		popPrediction();
		for (; *p != -1; p++)
			pushPrediction(*p);

		return MatchResult_Continue;
	}

	MatchResult
	matchActionNode(Node* node) {
		bool result = static_cast<T*>(this)->action(node->m_index);

#if (_LLK_RANDOM_SEMANTIC_ERRORS)
		if (isRandomError("action"))
			result = false;
#endif

		if (!result) {
			if (!m_resolverStack.isEmpty())
				return MatchResult_Fail; // rollback resolver

			RecoveryAction action = recover(ErrorKind_Semantic);
			if (action == RecoveryAction_Fail)
				return MatchResult_Fail;
			else if (action == RecoveryAction_Synchronize)
				return MatchResult_Continue;
		}

		popPrediction();
		return MatchResult_Continue;
	}

	MatchResult
	matchLaDfaNode(LaDfaNode* node) {
		if (node->m_flags & LaDfaNodeFlag_PreResolver) {
			ASSERT(getPreResolverTop() == node);

			// successful match of resolver

			size_t productionIndex = node->m_resolverThenIndex;
			ASSERT(productionIndex < T::LaDfaFirst);

			m_tokenCursor = node->m_reparseLaDfaTokenCursor;

			popPreResolver();
			popPrediction();
			pushPrediction(productionIndex);

			return MatchResult_NextTokenNoAdvance;
		}

		if (!node->m_reparseLaDfaTokenCursor)
			node->m_reparseLaDfaTokenCursor = m_tokenCursor;

		LaDfaTransition transition = { 0 };

		LaDfaResult laDfaResult = static_cast<T*>(this)->laDfa(
			node->m_index,
			m_tokenCursor->m_token,
			&transition
		);

		switch (laDfaResult) {
		case LaDfaResult_Production:
			if (transition.m_productionIndex >= T::LaDfaFirst &&
				transition.m_productionIndex < T::LaDfaEnd) {
				// stil in lookahead DFA, need more tokens...
				node->m_index = transition.m_productionIndex - T::LaDfaFirst;
				return MatchResult_NextToken;
			} else {
				// resolved! continue parsing
				m_tokenCursor = node->m_reparseLaDfaTokenCursor;

				popPrediction();
				pushPrediction(transition.m_productionIndex);
				return MatchResult_NextTokenNoAdvance;
			}

			break;

		case LaDfaResult_Resolver:
			node->m_flags = transition.m_flags;
			node->m_resolverThenIndex = transition.m_productionIndex;
			node->m_resolverElseIndex = transition.m_resolverElseIndex;
			node->m_reparseResolverTokenCursor = m_tokenCursor;
			pushPreResolver(node);
			pushPrediction(transition.m_resolverIndex);
			return MatchResult_Continue;

		default:
			ASSERT(laDfaResult == LaDfaResult_Fail);

			if (m_resolverStack.isEmpty()) { // can't rollback so set error
				SymbolNode* symbol = getSymbolTop();
				ASSERT(symbol);
				axl::err::setFormatStringError(
					"unexpected '%s' while trying to resolve a conflict in '%s'",
					m_tokenCursor->getName(),
					static_cast<T*>(this)->getSymbolName(symbol->m_index)
				);
			}

			return MatchResult_Fail;
		}
	}

	// rollback

	MatchResult
	rollbackResolver() {
		LaDfaNode* laDfaNode = getPreResolverTop();

		// keep popping prediction stack until pre-resolver dfa node

		while (!m_predictionStack.isEmpty()) {
			Node* node = getPredictionTop();
			if (node == laDfaNode)
				break; // found it!!

			if (node->m_nodeKind == NodeKind_Symbol && (node->m_flags & SymbolNodeFlag_Stacked)) {
				SymbolNode* symbol = (SymbolNode*) node;
				if (isCatchSymbol(symbol)) {
					ASSERT(symbol == getCatchTop());
					popCatch();
				} else {
					ASSERT(symbol == getSymbolTop());
					if (symbol->m_leaveIndex != -1) // call leave() even when in a resolver
						static_cast<T*>(this)->leave(symbol->m_leaveIndex);

					popSymbol();
				}
			}

			popPrediction();
		}

		ASSERT(getPredictionTop() == laDfaNode);
		popPreResolver();

		m_tokenCursor = laDfaNode->m_reparseResolverTokenCursor;

		// switch to resolver-else branch

		if (laDfaNode->m_resolverElseIndex >= T::LaDfaFirst &&
			laDfaNode->m_resolverElseIndex < T::LaDfaEnd) {
			// still in lookahead DFA after rollback...

			laDfaNode->m_index = laDfaNode->m_resolverElseIndex - T::LaDfaFirst;

			if (!(laDfaNode->m_flags & LaDfaNodeFlag_HasChainedResolver))
				return MatchResult_NextToken; // if no chained resolver, advance to next token
		} else {
			size_t productionIndex = laDfaNode->m_resolverElseIndex;
			popPrediction();
			pushPrediction(productionIndex);
		}

		return MatchResult_NextTokenNoAdvance;
	}

	// create nodes

	Node*
	createNode(size_t masterIndex) {
		ASSERT(masterIndex < T::TotalCount);

		Node* node = NULL;

		if (masterIndex < T::TokenEnd) {
			node = m_nodeAllocator->template allocate<TokenNode>();
			node->m_index = masterIndex;
		} else if (masterIndex < T::TokenEnd + T::NamedSymbolCount) {
			size_t index = masterIndex - T::SymbolFirst;
			SymbolNode* symbolNode = static_cast<T*>(this)->createSymbolNode(index);
			node = symbolNode;
		} else if (masterIndex < T::TokenEnd + T::NamedSymbolCount + T::CatchSymbolCount) {
			node = m_nodeAllocator->template allocate<SymbolNode>();
			node->m_index = masterIndex - T::SymbolFirst;
		} else if (masterIndex < T::SymbolEnd) {
			node = m_nodeAllocator->template allocate<SymbolNode>();
			node->m_index = masterIndex - T::SymbolFirst;
		} else if (masterIndex < T::SequenceEnd) {
			node = m_nodeAllocator->template allocate<Node>();
			node->m_nodeKind = NodeKind_Sequence;
			node->m_index = masterIndex - T::SequenceFirst;
		} else if (masterIndex < T::ActionEnd) {
			node = m_nodeAllocator->template allocate<Node>();
			node->m_nodeKind = NodeKind_Action;
			node->m_index = masterIndex - T::ActionFirst;
		} else if (masterIndex < T::ArgumentEnd) {
			node = m_nodeAllocator->template allocate<Node>();
			node->m_nodeKind = NodeKind_Argument;
			node->m_index = masterIndex - T::ArgumentFirst;
		} else if (masterIndex < T::BeaconEnd) {
			const size_t* p = static_cast<T*>(this)->getBeacon(masterIndex - T::BeaconFirst);
			size_t slotIndex = p[0];
			size_t targetIndex = p[1];

			node = createNode(targetIndex);
			ASSERT(node->m_nodeKind == NodeKind_Token || node->m_nodeKind == NodeKind_Symbol);

			node->m_flags |= NodeFlag_Locator;

			SymbolNode* symbolNode = getSymbolTop();
			ASSERT(symbolNode && symbolNode->m_index < T::NamedSymbolCount);

			symbolNode->m_locatorArray.ensureCountZeroConstruct(slotIndex + 1);
			symbolNode->m_locatorArray[slotIndex] = node;
			symbolNode->m_locatorList.insertTail(node);
		} else {
			ASSERT(masterIndex < T::LaDfaEnd);
			node = m_nodeAllocator->template allocate<LaDfaNode>();
			node->m_index = masterIndex - T::LaDfaFirst;
		}

		return node;
	}

	// prediction stack

	Node*
	getArgument() {
		size_t count = m_predictionStack.getCount();
		if (count < 2)
			return NULL;

		Node* node = m_predictionStack[count - 2];
		return node->m_nodeKind == NodeKind_Argument ? node : NULL;
	}

	Node*
	pushPrediction(size_t masterIndex) {
		if (!masterIndex) // check for epsilon production
			return NULL;

		Node* node = createNode(masterIndex);
		m_predictionStack.append(node);
		return node;
	}

	void
	popPrediction() {
		Node* node = m_predictionStack.getBackAndPop();
		ASSERT(!(node->m_flags & (SymbolNodeFlag_Stacked | LaDfaNodeFlag_PreResolver)));

		if (!(node->m_flags & NodeFlag_Locator))
			m_nodeAllocator->free(node);
	}

	// symbol stack

	void
	pushSymbol(SymbolNode* node) {
		ASSERT(isNamedSymbol(node));
		m_symbolStack.append(node);
		node->m_flags |= SymbolNodeFlag_Stacked;
	}

	void
	popSymbol() {
		SymbolNode* node = m_symbolStack.getBackAndPop();
		ASSERT(node->m_flags & SymbolNodeFlag_Stacked);
		node->m_flags &= ~SymbolNodeFlag_Stacked;
	}

	// catch stack

	void
	pushCatch(SymbolNode* node) {
		ASSERT(isCatchSymbol(node));
		m_catchStack.append(node);
		node->m_catchSymbolCount = m_symbolStack.getCount();
		node->m_flags |= SymbolNodeFlag_Stacked;
	}

	void
	popCatch() {
		SymbolNode* node = m_catchStack.getBackAndPop();
		ASSERT(node->m_flags & SymbolNodeFlag_Stacked);
		node->m_flags &= ~SymbolNodeFlag_Stacked;
	}

	// resolver stack

	LaDfaNode*
	getPreResolverTop() {
		ASSERT(m_resolverStack.getBack()->m_flags & LaDfaNodeFlag_PreResolver);
		return m_resolverStack.getBack();
	}

	void
	pushPreResolver(LaDfaNode* node) {
		m_resolverStack.append(node);
		node->m_flags |= LaDfaNodeFlag_PreResolver;
	}

	void
	popPreResolver() {
		LaDfaNode* node = m_resolverStack.getBackAndPop();
		ASSERT(node->m_flags & LaDfaNodeFlag_PreResolver);
		node->m_flags &= ~LaDfaNodeFlag_PreResolver;
	}

	// locators

	Node*
	getLocator(size_t index) {
		SymbolNode* symbolNode = getSymbolTop();
		if (!symbolNode)
			return NULL;

		size_t count = symbolNode->m_locatorArray.getCount();
		if (index >= count)
			return NULL;

		Node* node = symbolNode->m_locatorArray[index];
		if (!node || !(node->m_flags & NodeFlag_Matched))
			return NULL;

		return node;
	}

	const Token*
	getTokenLocator(size_t index) {
		Node* node = getLocator(index);
		return node && node->m_nodeKind == NodeKind_Token ? &((TokenNode*)node)->m_token : NULL;
	}

	void*
	getSymbolLocator(size_t index) {
		Node* node = getLocator(index);
		return node && node->m_nodeKind == NodeKind_Symbol ? ((SymbolNode*)node)->getValue() : NULL;
	}

	// must be implemented in derived class:

	// static
	// const size_t*
	// getParseTable();

	// static
	// const size_t*
	// getSequence(size_t index);

	// static
	// size_t
	// getTokenIndex(int token);

	// static
	// int
	// getTokenFromIndex(size_t index);

	// static
	// const char*
	// getSymbolName(size_t index);

	// static
	// SymbolNode*
	// createSymbolNode(size_t index); // allocate node with llk::NodeAllocator

	// static
	// const size_t*
	// getBeacon(size_t index);

	// bool
	// action(size_t index);

	// bool
	// argument(
	//		size_t index,
	//		SymbolNode* symbol
	//		);

	// bool
	// enter(size_t index)

	// bool
	// leave(size_t index)

	// LaDfaResult
	// laDfa(
	//		size_t index,
	//		int lookaheadToken,
	//		LaDfaTransition* transition
	//		);

	// const size_t*
	// getSyncTokenSet(size_t index);

	// optionally implement:

	// RecoveryAction
	// processError(ErrorKind errorKind);
};

//..............................................................................

} // namespace llk
