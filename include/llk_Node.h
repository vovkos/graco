// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#define _LLK_NODE_H

#include "llk_Ast.h"

namespace llk {
	
//.............................................................................

enum ENode
{
	ENode_Undefined = 0,
	ENode_Token,
	ENode_Symbol,
	ENode_Sequence,
	ENode_Action,	
	ENode_Argument,	
	ENode_LaDfa,	

	ENode__Count,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

inline
const char*
GetNodeKindString (ENode NodeKind)
{
	static const char* StringTable [ENode__Count] = 
	{
		"undefined-node-kind", // ENode_Undefined
		"token-node",          // ENode_Token,
		"symbol-node",         // ENode_Symbol,
		"sequence-node",       //  ENode_Sequence,
		"action-node",         // ENode_Action,	
		"argument-node",       // ENode_Argument,	
		"lookahead-dfa-node",  // ENode_LaDfa,	
	};

	return NodeKind >= 0 && NodeKind < ENode__Count ? 
		StringTable [NodeKind] : 
		StringTable [ENode_Undefined];
}

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

enum ENodeFlag
{
	ENodeFlag_Locator = 0x01, // used to locate AST / token from actions (applies to token & symbol nodes)
	ENodeFlag_Matched = 0x02, // applies to token & symbol & argument nodes
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class CNode: public axl::rtl::TListLink
{
public:
	ENode m_Kind;
	uint_t m_Flags;
	size_t m_Index;

public:
	CNode ()
	{
		m_Kind = ENode_Undefined;
		m_Flags = 0;
		m_Index = -1;
	}

	virtual
	~CNode ()
	{
	}

	const char*
	GetNodeKindString ()
	{
		return llk::GetNodeKindString (m_Kind);
	}
};

//.............................................................................

template <class TToken>
class CTokenNodeT: public CNode
{
public:
	typedef TToken CToken;

public:
	CToken m_Token;

public:
	CTokenNodeT ()
	{
		m_Kind = ENode_Token;
	}
};

//.............................................................................

enum ESymbolNodeFlag
{
	ESymbolNodeFlag_Stacked = 0x0010,
	ESymbolNodeFlag_Named   = 0x0020,
	ESymbolNodeFlag_Pragma  = 0x0040,
	ESymbolNodeFlag_HasEnter  = 0x0100,
	ESymbolNodeFlag_HasLeave  = 0x0200,
	ESymbolNodeFlag_KeepAst   = 0x0400,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

template <class TAstNode>
class CSymbolNodeT: public CNode
{
public:
	typedef TAstNode CAstNode;

public:
	CAstNode* m_pAstNode;

	axl::rtl::CStdListT <CNode> m_LocatorList;
	axl::rtl::CArrayT <CNode*> m_LocatorArray;

public:
	CSymbolNodeT ()
	{
		m_Kind = ENode_Symbol;
		m_pAstNode = NULL;
	}

	virtual
	~CSymbolNodeT ()
	{
		if (m_pAstNode && !(m_Flags & ESymbolNodeFlag_KeepAst))
			AXL_MEM_DELETE (m_pAstNode);
	}
};

//.............................................................................

enum ELaDfaNodeFlag
{
	ELaDfaNodeFlag_PreResolver        = 0x0010,
	ELaDfaNodeFlag_HasChainedResolver = 0x0020,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

template <class TToken>
class CLaDfaNodeT: public CNode
{
public:
	typedef TToken CToken;

public:
	size_t m_ResolverThenIndex;
	size_t m_ResolverElseIndex;

	axl::rtl::CBoxIteratorT <CToken> m_ReparseLaDfaTokenCursor;
	axl::rtl::CBoxIteratorT <CToken> m_ReparseResolverTokenCursor;

public:
	CLaDfaNodeT ()
	{
		m_Kind = ENode_LaDfa;
		m_ResolverThenIndex = -1;
		m_ResolverElseIndex = -1;
	}
};

//.............................................................................

} // namespace llk
