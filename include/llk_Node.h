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

#define _LLK_NODE_H

#include "llk_Pch.h"

namespace llk {

// these are run-time nodes (as opposed to compile-time nodes in src/Node.h)

//..............................................................................

enum NodeKind
{
	NodeKind_Undefined = 0,
	NodeKind_Token,
	NodeKind_Symbol,
	NodeKind_Sequence,
	NodeKind_Action,
	NodeKind_Argument,
	NodeKind_LaDfa,

	NodeKind__Count,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

inline
const char*
getNodeKindString(NodeKind nodeKind)
{
	static const char* stringTable[NodeKind__Count] =
	{
		"undefined-node-kind", // NodeKind_Undefined
		"token-node",          // NodeKind_Token,
		"symbol-node",         // NodeKind_Symbol,
		"sequence-node",       // NodeKind_Sequence,
		"action-node",         // NodeKind_Action,
		"argument-node",       // NodeKind_Argument,
		"lookahead-dfa-node",  // NodeKind_LaDfa,
	};

	return nodeKind >= 0 && nodeKind < NodeKind__Count ?
		stringTable[nodeKind] :
		stringTable[NodeKind_Undefined];
}

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

enum NodeFlag
{
	NodeFlag_Locator = 0x0001, // used to locate token/value from actions (applies to token & symbol nodes)
	NodeFlag_Matched = 0x0002, // applies to token & symbol & argument nodes
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

struct Node: axl::sl::ListLink
{
	NodeKind m_nodeKind;
	uint_t m_flags;
	size_t m_index;

	Node()
	{
		m_nodeKind = NodeKind_Undefined;
		m_flags = 0;
		m_index = -1;
	}

	virtual
	~Node()
	{
	}

	const char*
	getNodeKindString()
	{
		return llk::getNodeKindString(m_nodeKind);
	}
};

//..............................................................................

template <class Token>
struct TokenNode: Node
{
	Token m_token;

	TokenNode()
	{
		m_nodeKind = NodeKind_Token;
	}
};

//..............................................................................

enum SymbolNodeFlag
{
	SymbolNodeFlag_Stacked  = 0x0010,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

struct SymbolNodeValue
{
	axl::lex::LineCol m_firstTokenPos;
	axl::lex::LineCol m_lastTokenPos;
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

struct SymbolNode: Node
{
	axl::sl::List<Node> m_locatorList;
	axl::sl::Array<Node*> m_locatorArray;
	size_t m_enterIndex;
	size_t m_leaveIndex;

	SymbolNode()
	{
		m_nodeKind = NodeKind_Symbol;
		m_enterIndex = -1;
		m_leaveIndex = -1;
	}

	SymbolNodeValue*
	getValue()
	{
		return (SymbolNodeValue*)(this + 1);
	}
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

template <typename Value>
struct SymbolNodeImpl: SymbolNode
{
	Value m_value;

	SymbolNodeImpl()
	{
		ASSERT(getValue() == &m_value);
	}
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

typedef SymbolNodeImpl<SymbolNodeValue> StdSymbolNode;

//..............................................................................

enum LaDfaNodeFlag
{
	LaDfaNodeFlag_PreResolver        = 0x0010,
	LaDfaNodeFlag_HasChainedResolver = 0x0020,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

template <class Token>
struct LaDfaNode: Node
{
	size_t m_resolverThenIndex;
	size_t m_resolverElseIndex;
	axl::sl::BoxIterator<Token> m_reparseLaDfaTokenCursor;
	axl::sl::BoxIterator<Token> m_reparseResolverTokenCursor;

	LaDfaNode()
	{
		m_nodeKind = NodeKind_LaDfa;
		m_resolverThenIndex = -1;
		m_resolverElseIndex = -1;
	}
};

//..............................................................................

template <typename Parser>
class NodeAllocator: public axl::ref::RefCount
{
public:
	enum
	{
		MaxNodeSize = Parser::MaxNodeSize,
	};

protected:
	axl::sl::List<Node, axl::sl::ImplicitPtrCast<Node, axl::sl::ListLink>, axl::mem::StdFree> m_freeList;

public:
	template <typename T>
	T*
	allocate()
	{
		ASSERT(sizeof(T) <= MaxNodeSize);

		Node* node = !m_freeList.isEmpty() ?
			m_freeList.removeHead() :
			(Node*)AXL_MEM_ALLOCATE(MaxNodeSize);

		return new(node)T;
	}

	void
	free(Node* node)
	{
		node->~Node();
		m_freeList.insertHead(node);
	}
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

template <typename Parser>
NodeAllocator<Parser>*
createCurrentThreadNodeAllocator()
{
	axl::ref::Ptr<NodeAllocator<Parser> > allocator = AXL_REF_NEW(NodeAllocator<Parser>);
	axl::sys::setTlsPtrSlotValue<NodeAllocator<Parser> >(allocator);
	return allocator;
}

template <typename Parser>
NodeAllocator<Parser>*
getCurrentThreadNodeAllocator()
{
	NodeAllocator<Parser>* allocator = axl::sys::getTlsPtrSlotValue<NodeAllocator<Parser> >();
	return allocator ? allocator : createCurrentThreadNodeAllocator<Parser>();
}

//..............................................................................

template <
	typename Parser,
	typename T
	>
T*
allocateNode()
{
	return getCurrentThreadNodeAllocator<Parser>()->template allocate<T>();
}

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

template <typename Parser>
class DeleteNode
{
public:
	void
	operator () (Node* node)
	{
		getCurrentThreadNodeAllocator<Parser>()->free(node);
	}
};

//..............................................................................

} // namespace llk
