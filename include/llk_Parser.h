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

namespace llk {

//..............................................................................

template <
	typename T,
	typename Token0
	>
class Parser
{
public:
	typedef Token0 Token;
	typedef typename Token::TokenKind TokenKind;
	typedef llk::TokenNode<Token> TokenNode;
	typedef llk::SymbolNode SymbolNode;
	typedef llk::StdSymbolNode StdSymbolNode;
	typedef llk::LaDfaNode<Token> LaDfaNode;

protected:
	enum Flag
	{
		Flag_TokenMatch      = 0x01,
		Flag_Synchronize     = 0x10,
		Flag_PostSynchronize = 0x20,
	};

	enum ErrorKind
	{
		ErrorKind_Syntax,
		ErrorKind_Semantic,
	};

	enum RecoverAction
	{
		RecoverAction_Fail,
		RecoverAction_Synchronize,
		RecoverAction_Continue,
	};

	enum MatchResult
	{
		MatchResult_Fail,
		MatchResult_NextToken,
		MatchResult_NextTokenNoAdvance,
		MatchResult_Continue,
	};

	enum LaDfaResult
	{
		LaDfaResult_Fail,
		LaDfaResult_Production,
		LaDfaResult_Resolver,
	};

	struct LaDfaTransition
	{
		uint_t m_flags;
		size_t m_productionIndex;
		size_t m_resolverIndex;
		size_t m_resolverElseIndex;
	};

protected:
	axl::sl::StringRef m_fileName;

	axl::sl::List<Node, axl::sl::ImplicitPtrCast<Node, axl::sl::ListLink>, DeleteNode<T> > m_nodeList;
	axl::sl::Array<Node*> m_predictionStack;
	axl::sl::Array<SymbolNode*> m_symbolStack;
	axl::sl::Array<LaDfaNode*> m_resolverStack;

	axl::sl::SimpleHashTable<int, size_t> m_syncTokenSet;
	axl::sl::BoxList<Token> m_tokenList;
	axl::sl::BoxIterator<Token> m_tokenCursor;
	Token m_currentToken;
	Token m_lastMatchedToken;
	uint_t m_flags;

public:
	Parser()
	{
		m_flags = 0;
	}

	SymbolNode*
	create(
		const axl::sl::StringRef& fileName,
		int symbol = T::StartSymbol
		)
	{
		clear();
		m_fileName = fileName;
		return (SymbolNode*)pushPrediction(T::SymbolFirst + symbol);
	}

	void
	clear()
	{
		m_fileName.clear();
		m_nodeList.clear();
		m_predictionStack.clear();
		m_symbolStack.clear();
		m_resolverStack.clear();
		m_tokenList.clear();
		m_tokenCursor = NULL;
		m_flags = 0;
	}

	bool
	parseToken(const Token* token)
	{
		bool result;

		if (m_flags & Flag_Synchronize)
		{
			MatchResult matchResult = synchronize(token);
			if (matchResult == MatchResult_NextToken)
				return true;
			else if (matchResult == MatchResult_Fail)
				return false;
		}

		m_tokenCursor = m_tokenList.insertTail(*token);
		m_currentToken = *token;

		const size_t* parseTable = static_cast<T*>(this)->getParseTable();
		size_t tokenIndex = static_cast<T*>(this)->getTokenIndex(token->m_token);
		ASSERT(tokenIndex < T::TokenCount);

		// first check for pragma productions out of band

		if (T::PragmaStartSymbol != -1)
		{
			size_t productionIndex = parseTable[T::PragmaStartSymbol * T::TokenCount + tokenIndex];
			if (productionIndex != -1 && productionIndex != 0)
				pushPrediction(productionIndex);
		}

		m_flags &= ~Flag_TokenMatch;

		for (;;)
		{
			if (m_flags & Flag_Synchronize)
			{
				MatchResult matchResult = synchronize(&m_currentToken);
				if (matchResult == MatchResult_NextToken)
					return true;
				else if (matchResult == MatchResult_Fail)
					return false;
			}

			MatchResult matchResult;

			Node* node = getPredictionTop();
			if (!node)
			{
				matchResult = matchEmptyPredictionStack();
			}
			else
			{
				switch (node->m_nodeKind)
				{
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

			if (matchResult == MatchResult_Fail)
			{
				if (m_resolverStack.isEmpty())
					return false;

				matchResult = rollbackResolver();
				ASSERT(matchResult != MatchResult_Fail); // failed resolver means there is another possibility!
			}

			switch (matchResult)
			{
			case MatchResult_Continue:
				break;

			case MatchResult_NextToken:
				result = advanceTokenCursor();
				if (!result)
					return true; // no more tokens, we are done

				// fall through

			case MatchResult_NextTokenNoAdvance:
				m_currentToken = *m_tokenCursor;
				tokenIndex = static_cast<T*>(this)->getTokenIndex(m_currentToken.m_token);
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
	traceSymbolStack()
	{
		intptr_t count = m_symbolStack.getCount();
		TRACE("SYMBOL STACK (%d symbols):\n", count);

		for (intptr_t i = count - 1; i >= 0; i--)
		{
			SymbolNode* node = m_symbolStack[i];
			SymbolNodeValue* value = node->getValue();
			TRACE("%p %s", node, static_cast<T*>(this)->getSymbolName(node->m_index));
			TRACE(" (%d:%d)", value->m_firstTokenPos.m_line + 1, value->m_firstTokenPos.m_col + 1);
			TRACE("\n");
		}
	}

	void
	tracePredictionStack()
	{
		intptr_t count = m_predictionStack.getCount();
		TRACE("PREDICTION STACK (%d nodes):\n", count);

		for (intptr_t i = count - 1; i >= 0; i--)
		{
			Node* node = m_predictionStack[i];

			const char* extra = node->m_nodeKind == NodeKind_Symbol ?
				static_cast<T*>(this)->getSymbolName(node->m_index) :
				"";

			TRACE("%p %s (%d) %s\n", node, getNodeKindString(node->m_nodeKind), node->m_index, extra);
		}
	}

	void
	traceTokenList()
	{
		axl::sl::BoxIterator<Token> token = m_tokenList.getHead();

		TRACE("TOKEN LIST (%d tokens):\n", m_tokenList.getCount());
		for (; token; token++)
		{
			TRACE("%s '%s' %s\n", token->getName(), token->getText(), token == m_tokenCursor ? "<--" : "");
		}
	}

	// public info

	const Token&
	getLastMatchedToken()
	{
		return m_lastMatchedToken;
	}

	const Token&
	getCurrentToken()
	{
		return m_currentToken;
	}

	Node*
	getPredictionTop()
	{
		size_t count = m_predictionStack.getCount();
		return count ? m_predictionStack[count - 1] : NULL;
	}

	size_t
	getSymbolStackSize()
	{
		return m_symbolStack.getCount();
	}

	SymbolNode*
	getSymbolTop()
	{
		size_t count = m_symbolStack.getCount();
		return count ? m_symbolStack[count - 1] : NULL;
	}

protected:
	// default recover action: fail on semantic error, synchronize on syntax error

	RecoverAction
	processError(ErrorKind errorKind)
	{
		if (errorKind != ErrorKind_Syntax)
			return RecoverAction_Fail;

		fprintf(stderr, "syntax error: %s\n", axl::err::getLastErrorDescription().sz());
		return RecoverAction_Synchronize;
	}

	bool
	isCatchSymbolIndex(size_t index)
	{
		return index >= T::NamedSymbolCount && index < T::NamedSymbolCount + T::CatchSymbolCount;
	}

	RecoverAction
	recover(ErrorKind errorKind)
	{
		if (errorKind == ErrorKind_Syntax && (m_flags & Flag_PostSynchronize))
		{
			// synchronizer token must match (otherwise, it's a bad choice of sync tokens)

			axl::err::setFormatStringError(
				"synchronizer token '%s' didn't match (adjust the 'catch' clause in the grammar)",
				m_currentToken.getName()
				);

			axl::lex::pushSrcPosError(m_fileName, m_currentToken.m_pos);
			return RecoverAction_Fail;
		}

		axl::lex::ensureSrcPosError(m_fileName, m_currentToken.m_pos);
		RecoverAction action = static_cast<T*>(this)->processError(errorKind);
		ASSERT(action != RecoverAction_Continue || errorKind != ErrorKind_Syntax); // can't continue on syntax errors

		if (action != RecoverAction_Synchronize)
			return action;

		size_t catcherCount = 0;
		m_syncTokenSet.clear();

		size_t count = m_symbolStack.getCount();
		for (intptr_t i = count - 1; i >= 0; i--)
		{
			SymbolNode* symbol = m_symbolStack[i];
			if (isCatchSymbolIndex(symbol->m_index))
			{
				catcherCount++;
				int const* p = static_cast<T*>(this)->getSyncTokenSet(symbol->m_index);
				for (; *p != -1; p++)
					m_syncTokenSet.addIfNotExists(*p, i);
			}
		}

		if (m_syncTokenSet.isEmpty())
		{
			axl::err::setError("empty synchronization set (consider adding a 'catch' clause to the grammar)");
			axl::lex::pushSrcPosError(m_fileName, m_currentToken.m_pos);
			return RecoverAction_Fail;
		}

		// reset and wait for a synchornization token

		m_resolverStack.clear();
		m_tokenList.clear();
		m_tokenCursor = NULL;
		m_flags |= Flag_Synchronize;
		return RecoverAction_Synchronize;
	}

	MatchResult
	synchronize(const Token* token)
	{
		ASSERT((m_flags & Flag_Synchronize) && !m_syncTokenSet.isEmpty());

		size_t i = m_syncTokenSet.findValue(token->m_token, -1);
		if (i == -1)
			return MatchResult_NextToken;

		Node* catcher = m_symbolStack[i];
		ASSERT(isCatchSymbolIndex(catcher->m_index));

		// pop everything above the catcher off prediction stack

		intptr_t j = m_predictionStack.getCount() - 1;
		for (; j >= 0; j--)
		{
			Node* node = m_predictionStack[j];
			if (node == catcher)
				break;

			if (!(node->m_flags & NodeFlag_Locator))
				m_nodeList.erase(node);
		}

		ASSERT(catcher->m_flags & SymbolNodeFlag_Stacked);
		catcher->m_flags &= ~SymbolNodeFlag_Stacked;
		m_symbolStack.setCount(i); // remove catcher from symbol stack...
		m_predictionStack.setCount(j + 1); // ...but keep it on prediction stack

		m_flags &= ~Flag_Synchronize;
		m_flags |= Flag_PostSynchronize; // synchronizer token must match
		return MatchResult_Continue;
	}

	bool
	advanceTokenCursor()
	{
		m_tokenCursor++;

		Node* node = getPredictionTop();
		if (m_resolverStack.isEmpty() && (!node || node->m_nodeKind != NodeKind_LaDfa))
		{
			m_tokenList.removeHead(); // nobody gonna reparse this token
			ASSERT(m_tokenCursor == m_tokenList.getHead());
		}

		if (!m_tokenCursor)
			return false;

		return true;
	}

	// match against different kinds of nodes on top of prediction stack

	MatchResult
	matchEmptyPredictionStack()
	{
		if ((m_flags & Flag_TokenMatch) || m_currentToken.m_token == T::EofToken)
			return MatchResult_NextToken;

		axl::err::setFormatStringError("prediction stack empty while parsing '%s'", m_currentToken.getName());
		return MatchResult_Fail;
	}

	MatchResult
	matchTokenNode(
		TokenNode* node,
		size_t tokenIndex
		)
	{
		if (m_flags & Flag_TokenMatch)
			return MatchResult_NextToken;

		if (node->m_index != T::AnyToken && node->m_index != tokenIndex)
		{
			if (!m_resolverStack.isEmpty())
				return MatchResult_Fail; // rollback resolver

			int expectedToken = static_cast<T*>(this)->getTokenFromIndex(node->m_index);
			axl::lex::setExpectedTokenError(Token::getName(expectedToken), m_currentToken.getName());
			return recover(ErrorKind_Syntax) ? MatchResult_Continue : MatchResult_Fail;
		}

		if (node->m_flags & NodeFlag_Locator)
		{
			node->m_token = m_currentToken;
			node->m_flags |= NodeFlag_Matched;
		}

		m_lastMatchedToken = m_currentToken;
		m_flags |= Flag_TokenMatch;

		popPrediction();
		return MatchResult_Continue; // don't advance to next token just yet (execute following actions)
	}

	MatchResult
	matchSymbolNode(
		SymbolNode* node,
		const size_t* parseTable,
		size_t tokenIndex
		)
	{
		bool result;

		if (node->m_flags & SymbolNodeFlag_Stacked)
		{
			if (getSymbolTop() != node)
			{
				traceSymbolStack();
				tracePredictionStack();
			}

			ASSERT(getSymbolTop() == node);
			node->getValue()->m_lastTokenPos = m_lastMatchedToken.m_pos;
			node->m_flags |= NodeFlag_Matched;

			if (node->m_flags & SymbolNodeFlag_HasLeave)
			{
				result = static_cast<T*>(this)->leave(node->m_index);
				if (!result)
				{
					if (!m_resolverStack.isEmpty())
						return MatchResult_Fail; // rollback resolver

					RecoverAction action = recover(ErrorKind_Semantic);
					if (action == RecoverAction_Fail)
						return MatchResult_Fail;
					else if (action == RecoverAction_Synchronize)
						return MatchResult_Continue;
				}
			}

			popSymbol();
			popPrediction();
			return MatchResult_Continue;
		}

		if (m_flags & Flag_TokenMatch)
			return MatchResult_NextToken;

		if (node->m_index < T::NamedSymbolCount)
		{
			Node* argument = getArgument();
			if (argument)
			{
				static_cast<T*>(this)->argument(argument->m_index, node);
				argument->m_flags |= NodeFlag_Matched;
			}

			pushSymbol(node);

			if (node->m_flags & SymbolNodeFlag_HasEnter)
			{
				result = static_cast<T*>(this)->enter(node->m_index);
				if (!result)
				{
					if (!m_resolverStack.isEmpty())
						return MatchResult_Fail; // rollback resolver

					RecoverAction action = recover(ErrorKind_Semantic);
					if (action == RecoverAction_Fail)
						return MatchResult_Fail;
					else if (action == RecoverAction_Synchronize)
						return MatchResult_Continue;
				}
			}
		}
		else if (node->m_index < T::NamedSymbolCount + T::CatchSymbolCount)
		{
			pushSymbol(node); // keep catch on symbol stack, too
		}

		size_t productionIndex = parseTable[node->m_index * T::TokenCount + tokenIndex];
		if (productionIndex == -1)
		{
			if (!m_resolverStack.isEmpty())
				return MatchResult_Fail; // rollback resolver

			SymbolNode* symbol = getSymbolTop();
			ASSERT(symbol);
			axl::err::setFormatStringError(
				"unexpected '%s' in '%s'",
				m_currentToken.getName(),
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
	matchSequenceNode(Node* node)
	{
		if (m_flags & Flag_TokenMatch)
			return MatchResult_NextToken;

		const size_t* p = static_cast<T*>(this)->getSequence(node->m_index);

		popPrediction();
		for (; *p != -1; p++)
			pushPrediction(*p);

		return MatchResult_Continue;
	}

	MatchResult
	matchActionNode(Node* node)
	{
		bool result = static_cast<T*>(this)->action(node->m_index);
		if (!result)
		{
			if (!m_resolverStack.isEmpty())
				return MatchResult_Fail; // rollback resolver

			RecoverAction action = recover(ErrorKind_Semantic);
			if (action == RecoverAction_Fail)
				return MatchResult_Fail;
			else if (action == RecoverAction_Synchronize)
				return MatchResult_Continue;
		}

		popPrediction();
		return MatchResult_Continue;
	}

	MatchResult
	matchLaDfaNode(LaDfaNode* node)
	{
		if (node->m_flags & LaDfaNodeFlag_PreResolver)
		{
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
			m_currentToken.m_token,
			&transition
			);

		switch (laDfaResult)
		{
		case LaDfaResult_Production:
			if (transition.m_productionIndex >= T::LaDfaFirst &&
				transition.m_productionIndex < T::LaDfaEnd)
			{
				// stil in lookahead DFA, need more tokens...
				node->m_index = transition.m_productionIndex - T::LaDfaFirst;
				return MatchResult_NextToken;
			}
			else
			{
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

			if (m_resolverStack.isEmpty()) // can't rollback so set error
			{
				SymbolNode* symbol = getSymbolTop();
				ASSERT(symbol);
				axl::err::setFormatStringError(
					"unexpected '%s' while trying to resolve a conflict in '%s'",
					m_currentToken.getName(),
					static_cast<T*>(this)->getSymbolName(symbol->m_index)
					);
			}

			return MatchResult_Fail;
		}
	}

	// rollback

	MatchResult
	rollbackResolver()
	{
		LaDfaNode* laDfaNode = getPreResolverTop();
		ASSERT(laDfaNode);
		ASSERT(laDfaNode->m_flags & LaDfaNodeFlag_PreResolver);

		// keep popping prediction stack until pre-resolver dfa node

		while (!m_predictionStack.isEmpty())
		{
			Node* node = getPredictionTop();

			if (node->m_nodeKind == NodeKind_Symbol && (node->m_flags & SymbolNodeFlag_Stacked))
			{
				ASSERT(getSymbolTop() == node);

				// do NOT call leave() during resolver unwinding
				// cause leave() assumes parsing of the symbol is complete

				popSymbol();
			}

			if (node == laDfaNode)
				break; // found it!!

			popPrediction();
		}

		ASSERT(getPredictionTop() == laDfaNode);
		popPreResolver();

		m_tokenCursor = laDfaNode->m_reparseResolverTokenCursor;

		// switch to resolver-else branch

		if (laDfaNode->m_resolverElseIndex >= T::LaDfaFirst &&
			laDfaNode->m_resolverElseIndex < T::LaDfaEnd)
		{
			// still in lookahead DFA after rollback...

			laDfaNode->m_index = laDfaNode->m_resolverElseIndex - T::LaDfaFirst;

			if (!(laDfaNode->m_flags & LaDfaNodeFlag_HasChainedResolver))
				return MatchResult_NextToken; // if no chained resolver, advance to next token
		}
		else
		{
			size_t productionIndex = laDfaNode->m_resolverElseIndex;
			popPrediction();
			pushPrediction(productionIndex);
		}

		return MatchResult_NextTokenNoAdvance;
	}

	// create nodes

	Node*
	createNode(size_t masterIndex)
	{
		ASSERT(masterIndex < T::TotalCount);

		Node* node = NULL;

		if (masterIndex < T::TokenEnd)
		{
			node = allocateNode<T, TokenNode>();
			node->m_index = masterIndex;
		}
		else if (masterIndex < T::TokenEnd + T::NamedSymbolCount)
		{
			size_t index = masterIndex - T::SymbolFirst;
			SymbolNode* symbolNode = static_cast<T*>(this)->createSymbolNode(index);
			node = symbolNode;
		}
		else if (masterIndex < T::TokenEnd + T::NamedSymbolCount + T::CatchSymbolCount)
		{
			node = allocateNode<T, StdSymbolNode>();
			node->m_index = masterIndex - T::SymbolFirst;
		}
		else if (masterIndex < T::SymbolEnd)
		{
			node = allocateNode<T, SymbolNode>();
			node->m_index = masterIndex - T::SymbolFirst;
		}
		else if (masterIndex < T::SequenceEnd)
		{
			node = allocateNode<T, Node>();
			node->m_nodeKind = NodeKind_Sequence;
			node->m_index = masterIndex - T::SequenceFirst;
		}
		else if (masterIndex < T::ActionEnd)
		{
			node = allocateNode<T, Node>();
			node->m_nodeKind = NodeKind_Action;
			node->m_index = masterIndex - T::ActionFirst;
		}
		else if (masterIndex < T::ArgumentEnd)
		{
			node = allocateNode<T, Node>();
			node->m_nodeKind = NodeKind_Argument;
			node->m_index = masterIndex - T::ArgumentFirst;
		}
		else if (masterIndex < T::BeaconEnd)
		{
			const size_t* p = static_cast<T*>(this)->getBeacon(masterIndex - T::BeaconFirst);
			size_t slotIndex = p[0];
			size_t targetIndex = p[1];

			node = createNode(targetIndex);
			ASSERT(node->m_nodeKind == NodeKind_Token || node->m_nodeKind == NodeKind_Symbol);

			node->m_flags |= NodeFlag_Locator;

			SymbolNode* symbolNode = getSymbolTop();
			ASSERT(symbolNode);

			symbolNode->m_locatorArray.ensureCountZeroConstruct(slotIndex + 1);
			symbolNode->m_locatorArray[slotIndex] = node;
			symbolNode->m_locatorList.insertTail(node);
		}
		else
		{
			ASSERT(masterIndex < T::LaDfaEnd);
			node = allocateNode<T, LaDfaNode>();
			node->m_index = masterIndex - T::LaDfaFirst;
		}

		return node;
	}

	// prediction stack

	Node*
	getArgument()
	{
		size_t count = m_predictionStack.getCount();
		if (count < 2)
			return NULL;

		Node* node = m_predictionStack[count - 2];
		return node->m_nodeKind == NodeKind_Argument ? node : NULL;
	}

	Node*
	pushPrediction(size_t masterIndex)
	{
		if (!masterIndex) // check for epsilon production
			return NULL;

		Node* node = createNode(masterIndex);
		if (!(node->m_flags & NodeFlag_Locator))
			m_nodeList.insertTail(node);

		m_predictionStack.append(node);
		return node;
	}

	void
	popPrediction()
	{
		size_t count = m_predictionStack.getCount();
		if (!count)
		{
			ASSERT(false);
			return;
		}

		Node* node = m_predictionStack[count - 1];
		ASSERT(!(node->m_flags & SymbolNodeFlag_Stacked));

		if (!(node->m_flags & NodeFlag_Locator))
			m_nodeList.erase(node);

		m_predictionStack.setCount(count - 1);
	}

	// symbol stack

	void
	pushSymbol(SymbolNode* node)
	{
		m_symbolStack.append(node);
		node->m_flags |= SymbolNodeFlag_Stacked;

		SymbolNodeValue* value = node->getValue();
		value->m_firstTokenPos = m_currentToken.m_pos;
		value->m_lastTokenPos = m_currentToken.m_pos;
	}

	void
	popSymbol()
	{
		size_t count = m_symbolStack.getCount();
		if (!count)
		{
			ASSERT(false);
			return;
		}

		SymbolNode* node = m_symbolStack[count - 1];
		node->m_flags &= ~SymbolNodeFlag_Stacked;
		m_symbolStack.setCount(count - 1);
	}

	// resolver stack

	LaDfaNode*
	getPreResolverTop()
	{
		size_t count = m_resolverStack.getCount();
		return count ? m_resolverStack[count - 1] : NULL;
	}

	void
	pushPreResolver(LaDfaNode* node)
	{
		m_resolverStack.append(node);
		node->m_flags |= LaDfaNodeFlag_PreResolver;
	}

	void
	popPreResolver()
	{
		size_t count = m_resolverStack.getCount();
		if (!count)
		{
			ASSERT(false);
			return;
		}

		LaDfaNode* node = m_resolverStack[count - 1];
		node->m_flags &= ~LaDfaNodeFlag_PreResolver;

		m_resolverStack.setCount(count - 1);
	}

	// locators

	Node*
	getLocator(size_t index)
	{
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
	getTokenLocator(size_t index)
	{
		Node* node = getLocator(index);
		return node && node->m_nodeKind == NodeKind_Token ? &((TokenNode*)node)->m_token : NULL;
	}

	SymbolNodeValue*
	getSymbolLocator(size_t index)
	{
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

	// RecoverAction
	// processError(ErrorKind errorKind);
};

//..............................................................................

} // namespace llk
