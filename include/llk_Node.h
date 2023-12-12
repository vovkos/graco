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

enum NodeKind {
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
getNodeKindString(NodeKind nodeKind) {
	static const char* stringTable[NodeKind__Count] = {
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

enum NodeFlag {
	NodeFlag_Locator = 0x0001, // used to locate token/value from actions (applies to token & symbol nodes)
	NodeFlag_Matched = 0x0002, // applies to token & symbol & argument nodes
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

struct Node: axl::sl::ListLink {
	NodeKind m_nodeKind;
	uint_t m_flags;
	size_t m_index;

	Node() {
		m_nodeKind = NodeKind_Undefined;
		m_flags = 0;
		m_index = -1;
	}

	virtual
	~Node() {}

	const char*
	getNodeKindString() {
		return llk::getNodeKindString(m_nodeKind);
	}
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class DeallocateNode {
public:
	void
	operator () (Node* node) const {
		node->~Node();
		axl::mem::deallocate(node);
	}
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

typedef axl::sl::ImplicitPtrCast<Node, axl::sl::ListLink> GetNodeLink;
typedef axl::sl::List<Node, GetNodeLink, axl::mem::Deallocate> NodeList;

//..............................................................................

class NodeAllocatorBase: public axl::rc::RefCount {
protected:
	NodeList m_freeList;

public:
	void
	free(Node* node) {
		node->~Node();
		m_freeList.insertHead(node);
	}

	void
	free(NodeList* list) {
		for (axl::sl::Iterator<Node> it = list->getHead(); it; it++)
			it->~Node();

		m_freeList.insertListHead(list);
	}

	void
	clear() {
		m_freeList.clear();
	}
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

template <typename T>
class NodeAllocator: public NodeAllocatorBase {
public:
	enum {
		MaxNodeSize = T::MaxNodeSize,
	};

public:
	template <typename N>
	N*
	allocate() {
		ASSERT(sizeof(N) <= MaxNodeSize);

		Node* node = !m_freeList.isEmpty() ?
			m_freeList.removeHead() :
			(Node*)axl::mem::allocate(MaxNodeSize);

		return new (node) N;
	}
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

template <typename T>
NodeAllocator<T>*
getCurrentThreadNodeAllocator() {
	NodeAllocator<T>* allocator = axl::sys::getTlsPtrSlotValue<NodeAllocator<T> >();
	if (allocator)
		return allocator;

	axl::rc::Ptr<NodeAllocator<T> > newAllocator = AXL_RC_NEW(NodeAllocator<T>);
	axl::sys::setTlsPtrSlotValue<NodeAllocator<T> >(newAllocator);
	return newAllocator;
}

//..............................................................................

template <class Token>
struct TokenNode: Node {
	Token m_token;

	TokenNode() {
		m_nodeKind = NodeKind_Token;
	}
};

//..............................................................................

enum SymbolNodeFlag {
	SymbolNodeFlag_Stacked = 0x0010,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

struct SymbolNode: Node {
	axl::sl::Array<Node*> m_locatorArray;
	NodeList m_locatorList;
	NodeAllocatorBase* m_nodeAllocator;

	union {
		struct {
			size_t m_enterIndex;
			size_t m_leaveIndex;
		};

		size_t m_catchSymbolCount;
		uint64_t _m_padding;
	};

	SymbolNode() {
		AXL_ASSERT_NO_TAIL_PADDING(SymbolNode);
		m_nodeKind = NodeKind_Symbol;
		m_enterIndex = -1;
		m_leaveIndex = -1;
		m_nodeAllocator = NULL;
	}

	~SymbolNode() {
		ASSERT(m_nodeAllocator || m_locatorList.isEmpty());
		if (m_nodeAllocator)
			m_nodeAllocator->free(&m_locatorList);
	}

	void*
	getValue() {
		return (this + 1);
	}
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

template <typename Value>
struct SymbolNodeImpl: SymbolNode {
	Value m_value;

	SymbolNodeImpl() {
		ASSERT(getValue() == &m_value);
	}
};

//..............................................................................

enum LaDfaNodeFlag {
	LaDfaNodeFlag_PreResolver        = 0x0010,
	LaDfaNodeFlag_HasChainedResolver = 0x0020,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

template <class Token>
struct LaDfaNode: Node {
	size_t m_resolverThenIndex;
	size_t m_resolverElseIndex;
	axl::sl::Iterator<Token> m_reparseLaDfaTokenCursor;
	axl::sl::Iterator<Token> m_reparseResolverTokenCursor;

	LaDfaNode() {
		m_nodeKind = NodeKind_LaDfa;
		m_resolverThenIndex = -1;
		m_resolverElseIndex = -1;
	}
};

//..............................................................................

} // namespace llk
