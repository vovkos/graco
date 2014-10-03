// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#define _LLK_AST_H

namespace llk {

//.............................................................................

template <class Token_0>
class AstNode: public axl::rtl::ListLink
{
public:
	typedef Token_0 Token;

public:
	int m_kind;

	Token m_firstToken;
	Token m_lastToken;

	// later create a wrapper for ast tree
	// we don't really need tree fields and listlink until we plan to keep ast nodes

	AstNode* m_parent;
	axl::rtl::Array <AstNode*> m_children;

public:
	AstNode ()
	{
		m_kind = -1;
		m_parent = NULL;
	}

	virtual
	~AstNode () // could be subclassed
	{
	}
};

//.............................................................................

template <class AstNode_0>
class Ast
{
public:
	typedef AstNode_0 AstNode;

protected:
	axl::rtl::StdList <AstNode> m_list;
	AstNode* m_root;

public:
	Ast ()
	{
		m_root = NULL;
	}

	AstNode*
	getRoot ()
	{
		return m_root;
	}

	void
	clear ()
	{
		m_list.clear ();
		m_root = NULL;
	}

	void
	add (AstNode* astNode)
	{
		m_list.insertTail (astNode);
		if (!m_root)
			m_root = astNode;
	}
};

//.............................................................................

} // namespace llk
